// Copyright (C) 2023 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "GfxstreamEnd2EndTests.h"

#include <filesystem>
#include <string>

#include "aemu/base/Path.h"
#include "drm_fourcc.h"
#include "gfxstream/ImageUtils.h"

constexpr const bool kSaveImagesIfComparisonFailed = true;

namespace gfxstream {
namespace tests {
namespace {

struct Ok {};

using testing::Eq;

std::string GetTestDataPath(const std::string& basename) {
    const std::filesystem::path testBinaryDirectory = gfxstream::guest::getProgramDirectory();
    return (testBinaryDirectory / "testdata" / basename).string();
}

class GfxstreamEnd2EndCompositionTest : public GfxstreamEnd2EndTest {
  protected:
    struct Image {
        uint32_t width;
        uint32_t height;
        std::vector<uint32_t> pixels;
    };
    GlExpected<Image> LoadImage(const std::string& basename) {
        const std::string filepath = GetTestDataPath(basename);
        if (!std::filesystem::exists(filepath)) {
            return android::base::unexpected("File " + filepath + " does not exist.");
        }
        if (!std::filesystem::is_regular_file(filepath)) {
            return android::base::unexpected("File " + filepath + " is not a regular file.");
        }

        Image image;

        uint32_t sourceWidth = 0;
        uint32_t sourceHeight = 0;
        std::vector<uint32_t> sourcePixels;
        if (!LoadRGBAFromPng(filepath, &image.width, &image.height, &image.pixels)) {
            return android::base::unexpected("Failed to load " + filepath + " as RGBA PNG.");
        }

        return image;
    }

    GlExpected<ScopedAHardwareBuffer> CreateAHBFromImage(const std::string& basename) {
        auto image = GL_EXPECT(LoadImage(basename));

        auto ahb = GL_EXPECT(ScopedAHardwareBuffer::Allocate(*mGralloc, image.width, image.height, DRM_FORMAT_ABGR8888));

        {
            uint8_t* ahbPixels = GL_EXPECT(ahb.Lock());
            std::memcpy(ahbPixels, image.pixels.data(), image.pixels.size() * sizeof(uint32_t));
            ahb.Unlock();
        }

        return std::move(ahb);
    }

    bool ArePixelsSimilar(uint32_t actualPixel, uint32_t expectedPixel) {
        const uint8_t* actualRGBA = reinterpret_cast<const uint8_t*>(&actualPixel);
        const uint8_t* expectedRGBA = reinterpret_cast<const uint8_t*>(&expectedPixel);

        constexpr const uint32_t kRGBA8888Tolerance = 2;
        for (uint32_t channel = 0; channel < 4; channel++) {
            const uint8_t actualChannel = actualRGBA[channel];
            const uint8_t expectedChannel = expectedRGBA[channel];

            if ((std::max(actualChannel, expectedChannel) -
                 std::min(actualChannel, expectedChannel)) > kRGBA8888Tolerance) {
                return false;
            }
        }
        return true;
    }

    bool AreImagesSimilar(const uint32_t width,
                          const uint32_t height,
                          const uint32_t* actualPixels,
                          const uint32_t* expectedPixels) {
        bool comparisonFailed = false;

        uint32_t reportedIncorrectPixels = 0;
        constexpr const uint32_t kMaxReportedIncorrectPixels = 5;

        for (uint32_t y = 0; y < height; y++) {
            for (uint32_t x = 0; x < width; x++) {
                const uint32_t actualPixel = actualPixels[y * height + x];
                const uint32_t expectedPixel = expectedPixels[y * width + x];
                if (!ArePixelsSimilar(actualPixel, expectedPixel)) {
                    comparisonFailed = true;
                    if (reportedIncorrectPixels < kMaxReportedIncorrectPixels) {
                        reportedIncorrectPixels++;
                        const uint8_t* actualRGBA = reinterpret_cast<const uint8_t*>(&actualPixel);
                        const uint8_t* expectedRGBA = reinterpret_cast<const uint8_t*>(&expectedPixel);
                        ADD_FAILURE()
                            << "Pixel comparison failed at (" << x << ", " << y << ") "
                            << " with actual "
                            << " r:" << static_cast<int>(actualRGBA[0])
                            << " g:" << static_cast<int>(actualRGBA[1])
                            << " b:" << static_cast<int>(actualRGBA[2])
                            << " a:" << static_cast<int>(actualRGBA[3])
                            << " but expected "
                            << " r:" << static_cast<int>(expectedRGBA[0])
                            << " g:" << static_cast<int>(expectedRGBA[1])
                            << " b:" << static_cast<int>(expectedRGBA[2])
                            << " a:" << static_cast<int>(expectedRGBA[3]);
                    }
                }
            }
        }
        return !comparisonFailed;
    }

    GlExpected<Ok> CompareAHBWithGolden(ScopedAHardwareBuffer& ahb, const std::string& goldenBasename) {
        Image golden = GL_EXPECT(LoadImage(goldenBasename));

        Image actual;
        actual.width = golden.width;
        actual.height = golden.height;
        actual.pixels.resize(actual.width * actual.height);
        {
            uint8_t* ahbPixels = GL_EXPECT(ahb.Lock());
            std::memcpy(actual.pixels.data(), ahbPixels, actual.pixels.size() * sizeof(uint32_t));
            ahb.Unlock();
        }

        if (actual.width != golden.width) {
            return android::base::unexpected("Actual width:" + std::to_string(actual.width));
        }

        if (!AreImagesSimilar(actual.width, actual.height, actual.pixels.data(), golden.pixels.data())) {
            if (kSaveImagesIfComparisonFailed) {
                const std::string output = (std::filesystem::temp_directory_path() / goldenBasename).string();
                SaveRGBAToPng(actual.width, actual.height, actual.pixels.data(), output);
                ADD_FAILURE() << "Saved image comparison actual image to " << output;
            }
            return android::base::unexpected("Image comparison failed.");
        }
        return {};
    }
};

TEST_P(GfxstreamEnd2EndCompositionTest, BasicComposition) {
    ScopedRenderControlDevice rcDevice(*mRc);

    auto layer1Ahb = GL_ASSERT(CreateAHBFromImage("256x256_android.png"));
    auto layer2Ahb = GL_ASSERT(CreateAHBFromImage("256x256_android_with_transparency.png"));
    auto resultAhb = GL_ASSERT(ScopedAHardwareBuffer::Allocate(*mGralloc, 256, 256, DRM_FORMAT_ABGR8888));

    const RenderControlComposition composition = {
        .displayId = 0,
        .compositionResultColorBufferHandle = mGralloc->getHostHandle(resultAhb),
    };
    const RenderControlCompositionLayer compositionLayers[2] = {
        {
            .colorBufferHandle = mGralloc->getHostHandle(layer1Ahb),
            .composeMode = HWC2_COMPOSITION_DEVICE,
            .displayFrame = {
                .left = 0,
                .top = 0,
                .right = 256,
                .bottom = 256,
            },
            .crop = {
                .left = 0,
                .top = 0,
                .right = static_cast<float>(256),
                .bottom = static_cast<float>(256),
            },
            .blendMode = HWC2_BLEND_MODE_PREMULTIPLIED,
            .alpha = 1.0,
            .color = {
                .r = 0,
                .g = 0,
                .b = 0,
                .a = 0,
            },
            .transform = static_cast<hwc_transform_t>(0),
        },
        {
            .colorBufferHandle = mGralloc->getHostHandle(layer2Ahb),
            .composeMode = HWC2_COMPOSITION_DEVICE,
            .displayFrame = {
                .left = 64,
                .top = 32,
                .right = 128,
                .bottom = 160,
            },
            .crop = {
                .left = 0,
                .top = 0,
                .right = static_cast<float>(256),
                .bottom = static_cast<float>(256),
            },
            .blendMode = HWC2_BLEND_MODE_PREMULTIPLIED,
            .alpha = 1.0,
            .color = {
                .r = 0,
                .g = 0,
                .b = 0,
                .a = 0,
            },
            .transform = static_cast<hwc_transform_t>(0),
        },
    };

    ASSERT_THAT(mRc->rcCompose(rcDevice, &composition, 2, compositionLayers), Eq(0));

    GL_ASSERT(CompareAHBWithGolden(resultAhb, "256x256_golden_basic_composition.png"));
}

INSTANTIATE_TEST_CASE_P(GfxstreamEnd2EndTests, GfxstreamEnd2EndCompositionTest,
                        ::testing::ValuesIn({
                            TestParams{
                                .with_gl = true,
                                .with_vk = false,
                            },
                            TestParams{
                                .with_gl = true,
                                .with_vk = true,
                            },
                            TestParams{
                                .with_gl = false,
                                .with_vk = true,
                            },
                        }),
                        &GetTestName);

}  // namespace
}  // namespace tests
}  // namespace gfxstream

