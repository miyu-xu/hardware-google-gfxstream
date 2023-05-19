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

namespace gfxstream {
namespace tests {
namespace {

using namespace std::chrono_literals;
using testing::Eq;
using testing::Not;
using testing::NotNull;

uint32_t GetMemoryType(const vkhpp::raii::PhysicalDevice& physicalDevice,
                       const vkhpp::MemoryRequirements& memoryRequirements,
                       vkhpp::MemoryPropertyFlags memoryProperties) {
  const auto props = physicalDevice.getMemoryProperties();
  for (uint32_t i = 0; i < props.memoryTypeCount; i++) {
    if (!(memoryRequirements.memoryTypeBits & (1 << i))) {
      continue;
    }
    if ((props.memoryTypes[i].propertyFlags & memoryProperties) != memoryProperties) {
      continue;
    }
    return i;
  }
  return -1;
}

template <typename DurationType>
constexpr uint64_t AsVkTimeout(DurationType duration) {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count());
}

class GfxstreamEnd2EndVkTest : public GfxstreamEnd2EndTest {};

TEST_P(GfxstreamEnd2EndVkTest, Basic) {
    auto [instance, physicalDevice, device, queue, queueFamilyIndex] =
        VK_ASSERT(SetUpTypicalVkTestEnvironment());
}

TEST_P(GfxstreamEnd2EndVkTest, VulkanImportAHB) {
    auto [instance, physicalDevice, device, queue, queueFamilyIndex] =
        VK_ASSERT(SetUpTypicalVkTestEnvironment());

    const uint32_t width = 32;
    const uint32_t height = 32;
    auto ahb = mGralloc->allocate(width, height, DRM_FORMAT_ABGR8888);

    const vkhpp::NativeBufferANDROID imageNativeBufferInfo = {
        .handle = (const uint32_t*)ahb->asAHardwareBuffer(),
    };

    const vkhpp::ImageCreateInfo imageCreateInfo = {
        .pNext = &imageNativeBufferInfo,
        .imageType = vkhpp::ImageType::e2D,
        .extent.width = width,
        .extent.height = height,
        .extent.depth = 1,
        .mipLevels = 1,
        .arrayLayers = 1,
        .format = vkhpp::Format::eR8G8B8A8Unorm,
        .tiling = vkhpp::ImageTiling::eOptimal,
        .initialLayout = vkhpp::ImageLayout::eUndefined,
        .usage = vkhpp::ImageUsageFlagBits::eSampled |
                 vkhpp::ImageUsageFlagBits::eTransferDst |
                 vkhpp::ImageUsageFlagBits::eTransferSrc,
        .sharingMode = vkhpp::SharingMode::eExclusive,
        .samples = vkhpp::SampleCountFlagBits::e1,
    };
    auto image = VK_ASSERT(vkhpp::raii::Image::create(device, imageCreateInfo));
    auto imageMemoryRequirements = image.getMemoryRequirements();

    const uint32_t imageMemoryIndex =
        GetMemoryType(physicalDevice, imageMemoryRequirements, vkhpp::MemoryPropertyFlagBits::eDeviceLocal);
    ASSERT_THAT(imageMemoryIndex, Not(Eq(-1)));

    const vkhpp::MemoryAllocateInfo imageMemoryAllocateInfo = {
        .allocationSize = imageMemoryRequirements.size,
        .memoryTypeIndex = imageMemoryIndex,
    };
    auto imageMemory = VK_ASSERT(vkhpp::raii::DeviceMemory::create(device, imageMemoryAllocateInfo));
    image.bindMemory(*imageMemory, 0);

    const vkhpp::BufferCreateInfo bufferCreateInfo = {
        .size = static_cast<VkDeviceSize>(12 * 1024 * 1024),
        .usage = vkhpp::BufferUsageFlagBits::eTransferDst |
                 vkhpp::BufferUsageFlagBits::eTransferSrc,
        .sharingMode = vkhpp::SharingMode::eExclusive,
    };
    auto stagingBuffer = VK_ASSERT(vkhpp::raii::Buffer::create(device, bufferCreateInfo));

    const auto stagingBufferMemoryRequirements = stagingBuffer.getMemoryRequirements();
    const auto stagingBufferMemoryType =
         GetMemoryType(physicalDevice, stagingBufferMemoryRequirements,
                       vkhpp::MemoryPropertyFlagBits::eHostVisible |
                       vkhpp::MemoryPropertyFlagBits::eHostCoherent);

    const vkhpp::MemoryAllocateInfo stagingBufferMemoryAllocateInfo = {
        .allocationSize = stagingBufferMemoryRequirements.size,
        .memoryTypeIndex = stagingBufferMemoryType,
    };
    auto stagingBufferMemory = VK_ASSERT(vkhpp::raii::DeviceMemory::create(device, stagingBufferMemoryAllocateInfo));
    stagingBuffer.bindMemory(*stagingBufferMemory, 0);

    const vkhpp::CommandPoolCreateInfo commandPoolCreateInfo = {
        .queueFamilyIndex = queueFamilyIndex,
    };
    auto commandPool = VK_ASSERT(vkhpp::raii::CommandPool::create(device, commandPoolCreateInfo));

    const vkhpp::CommandBufferAllocateInfo commandBufferAllocateInfo = {
        .level = vkhpp::CommandBufferLevel::ePrimary,
        .commandPool = *commandPool,
        .commandBufferCount = 1,
    };
    auto commandBuffers = VK_ASSERT(vkhpp::raii::CommandBuffers::create(device, commandBufferAllocateInfo));
    auto commandBuffer = std::move(commandBuffers[0]);

    const vkhpp::CommandBufferBeginInfo commandBufferBeginInfo = {
        .flags = vkhpp::CommandBufferUsageFlagBits::eOneTimeSubmit,
    };
    commandBuffer.begin(commandBufferBeginInfo);

    commandBuffer.end();

    std::vector<vkhpp::CommandBuffer> commandBufferHandles;
    commandBufferHandles.push_back(*commandBuffer);

    auto transferFence = VK_ASSERT(vkhpp::raii::Fence::create(device, vkhpp::FenceCreateInfo()));

    const vkhpp::SubmitInfo submitInfo = {
        .commandBufferCount = static_cast<uint32_t>(commandBufferHandles.size()),
        .pCommandBuffers = commandBufferHandles.data(),
    };
    queue.submit(submitInfo, *transferFence);

    auto waitResult = device.waitForFences(*transferFence, VK_TRUE, AsVkTimeout(3s));
    ASSERT_THAT(waitResult, Eq(vkhpp::Result::eSuccess));

    std::vector<vkhpp::Semaphore> semaphores;
    int fence = queue.signalReleaseImageANDROID(semaphores, *image);
    ASSERT_THAT(fence, Not(Eq(-1)));

    ASSERT_THAT(mSync->wait(fence, 3000), Eq(0));
}

TEST_P(GfxstreamEnd2EndVkTest, HostMemory) {
    static constexpr const vkhpp::DeviceSize kSize = 16 * 1024;

    auto [instance, physicalDevice, device, queue, queueFamilyIndex] =
        VK_ASSERT(SetUpTypicalVkTestEnvironment());

    uint32_t hostMemoryTypeIndex = -1;
    const auto memoryProperties = physicalDevice.getMemoryProperties();
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        const vkhpp::MemoryType& memoryType = memoryProperties.memoryTypes[i];
        if (memoryType.propertyFlags & vkhpp::MemoryPropertyFlagBits::eHostVisible) {
            hostMemoryTypeIndex = i;
        }
    }
    if (hostMemoryTypeIndex == -1) {
        GTEST_SKIP() << "Skipping test due to no host visible memory type.";
        return;
    }

    const vkhpp::MemoryAllocateInfo memoryAllocateInfo = {
        .allocationSize = kSize,
        .memoryTypeIndex = hostMemoryTypeIndex,
    };
    auto memory = VK_ASSERT(vkhpp::raii::DeviceMemory::create(device, memoryAllocateInfo));

    auto* bytes = reinterpret_cast<uint8_t*>(memory.mapMemory(0, VK_WHOLE_SIZE, vkhpp::MemoryMapFlags{}));
    ASSERT_THAT(bytes, NotNull());

    std::memset(bytes, 0xFF, kSize);

    const vkhpp::MappedMemoryRange range = {
        .memory = *memory,
        .offset = 0,
        .size = kSize,
    };
    device.flushMappedMemoryRanges({range});
    device.invalidateMappedMemoryRanges({range});

    for (uint32_t i = 0; i < kSize; ++i) {
        EXPECT_THAT(bytes[i], Eq(0xFF));
    }
}

INSTANTIATE_TEST_CASE_P(GfxstreamEnd2EndTests, GfxstreamEnd2EndVkTest,
                        ::testing::ValuesIn({
                            TestParams{
                                .with_gl = false,
                                .with_vk = true,
                            },
                            TestParams{
                                .with_gl = true,
                                .with_vk = true,
                            },
                        }),
                        &GetTestName);

}  // namespace
}  // namespace tests
}  // namespace gfxstream