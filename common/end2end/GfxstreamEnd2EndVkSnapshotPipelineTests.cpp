// Copyright (C) 2024 The Android Open Source Project
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

#include <string>

#include "GfxstreamEnd2EndTests.h"
#include "gfxstream/RutabagaLayerTestUtils.h"
#include "simple_shader_frag.h"
#include "simple_shader_vert.h"

namespace gfxstream {
namespace tests {
namespace {

using testing::Eq;
using testing::Ge;
using testing::IsEmpty;
using testing::IsNull;
using testing::Not;
using testing::NotNull;

struct PipelineInfo {
    vkhpp::UniqueRenderPass renderPass;
    vkhpp::UniqueDescriptorSetLayout descriptorSetLayout;
    vkhpp::UniquePipelineLayout pipelineLayout;
    vkhpp::UniqueShaderModule vertexShaderModule;
    vkhpp::UniqueShaderModule fragmentShaderModule;
    vkhpp::UniquePipeline pipeline;
};

struct ImageInfo {
    vkhpp::UniqueImage image;
    vkhpp::UniqueDeviceMemory memory;
    vkhpp::UniqueImageView imageView;
};

class GfxstreamEnd2EndVkSnapshotPipelineTest : public GfxstreamEnd2EndTest {
    protected:
        vkhpp::UniqueRenderPass createRenderPass(vkhpp::Device device);
        std::unique_ptr<ImageInfo> createColorAttachment(vkhpp::PhysicalDevice physicalDevice, vkhpp::Device device);
        std::unique_ptr<PipelineInfo> createPipeline(vkhpp::Device device);
        static const uint32_t kFbWidth = 32;
        static const uint32_t kFbHeight = 32;
};

// A blue triangle
const float kVertexData[] = {
    -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    0.0f,  0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
};

vkhpp::UniqueRenderPass GfxstreamEnd2EndVkSnapshotPipelineTest::createRenderPass(vkhpp::Device device) {
    vkhpp::AttachmentDescription colorAttachmentDescription = {
        .format = vkhpp::Format::eR8G8B8A8Unorm,
        .loadOp = vkhpp::AttachmentLoadOp::eDontCare,
        .finalLayout = vkhpp::ImageLayout::ePresentSrcKHR,
    };
    vkhpp::AttachmentReference attachmentReference = {
        .attachment = 0,
        .layout = vkhpp::ImageLayout::eColorAttachmentOptimal,
    };
    vkhpp::SubpassDescription subpassDescription = {
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentReference,
    };
    vkhpp::RenderPassCreateInfo renderPassCreateInfo = {
        .attachmentCount = 1,
        .pAttachments = &colorAttachmentDescription,
        .subpassCount = 1,
        .pSubpasses = &subpassDescription,
    };
    return device.createRenderPassUnique(renderPassCreateInfo).value;
}

std::unique_ptr<PipelineInfo> GfxstreamEnd2EndVkSnapshotPipelineTest::createPipeline(vkhpp::Device device) {
    std::unique_ptr<PipelineInfo> res(new PipelineInfo);
    res->renderPass = createRenderPass(device);

    vkhpp::DescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
    res->descriptorSetLayout =
        device.createDescriptorSetLayoutUnique(descriptorSetLayoutInfo).value;
    res->pipelineLayout =
        device.createPipelineLayoutUnique(vkhpp::PipelineLayoutCreateInfo{}).value;

    vkhpp::ShaderModuleCreateInfo vertexShaderModuleCreateInfo = {
        .codeSize = sizeof(kSimpleShaderVert),
        .pCode = (const uint32_t*)kSimpleShaderVert,
    };
    vkhpp::ShaderModuleCreateInfo fragmentShaderModuleCreateInfo = {
        .codeSize = sizeof(kSimpleShaderFrag),
        .pCode = (const uint32_t*)kSimpleShaderFrag,
    };
    res->vertexShaderModule = device.createShaderModuleUnique(vertexShaderModuleCreateInfo).value;
    res->fragmentShaderModule =
        device.createShaderModuleUnique(fragmentShaderModuleCreateInfo).value;

    vkhpp::PipelineShaderStageCreateInfo pipelineShaderStageCreateInfos[2] = {
        {
            .stage = vkhpp::ShaderStageFlagBits::eVertex,
            .module = *res->vertexShaderModule,
            .pName = "main",
        },
        {
            .stage = vkhpp::ShaderStageFlagBits::eFragment,
            .module = *res->fragmentShaderModule,
            .pName = "main",
        },
    };

    const vkhpp::VertexInputBindingDescription vertexInputBindingDescription = {
        .stride = 32,
    };
    vkhpp::VertexInputAttributeDescription vertexInputAttributeDescriptions[2] = {
        {
            .location = 0,
            .format = vkhpp::Format::eR32G32B32A32Sfloat,
            .offset = 0,
        },
        {
            .location = 1,
            .format = vkhpp::Format::eR32G32B32A32Sfloat,
            .offset = 16,
        },
    };
    const vkhpp::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertexInputBindingDescription,
        .vertexAttributeDescriptionCount = 2,
        .pVertexAttributeDescriptions = vertexInputAttributeDescriptions,
    };

    const vkhpp::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = {
        .topology = vkhpp::PrimitiveTopology::eTriangleList,
    };

    const vkhpp::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = {
        .viewportCount = 1,
        .scissorCount = 1,
    };

    const vkhpp::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = {
        .cullMode = vkhpp::CullModeFlagBits::eNone,
        .lineWidth = 1.0f,
    };

    const vkhpp::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = {
        .rasterizationSamples = vkhpp::SampleCountFlagBits::e1,
    };
    const vkhpp::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = {};
    const vkhpp::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState = {
        .colorBlendOp = vkhpp::BlendOp::eAdd,
        .srcAlphaBlendFactor = vkhpp::BlendFactor::eZero,
        .dstAlphaBlendFactor = vkhpp::BlendFactor::eZero,
        .alphaBlendOp = vkhpp::BlendOp::eAdd,
        .colorWriteMask = vkhpp::ColorComponentFlagBits::eR | vkhpp::ColorComponentFlagBits::eG |
                          vkhpp::ColorComponentFlagBits::eB | vkhpp::ColorComponentFlagBits::eA};
    const vkhpp::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = {
        .attachmentCount = 1,
        .pAttachments = &pipelineColorBlendAttachmentState,
    };
    const vkhpp::DynamicState dynamicStates[2] = {vkhpp::DynamicState::eViewport,
                                                  vkhpp::DynamicState::eScissor};
    const vkhpp::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo = {
        .dynamicStateCount = 2,
        .pDynamicStates = dynamicStates,
    };

    vkhpp::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
        .stageCount = 2,
        .pStages = pipelineShaderStageCreateInfos,
        .pVertexInputState = &pipelineVertexInputStateCreateInfo,
        .pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo,
        .pViewportState = &pipelineViewportStateCreateInfo,
        .pRasterizationState = &pipelineRasterizationStateCreateInfo,
        .pMultisampleState = &pipelineMultisampleStateCreateInfo,
        .pDepthStencilState = &pipelineDepthStencilStateCreateInfo,
        .pColorBlendState = &pipelineColorBlendStateCreateInfo,
        .pDynamicState = &pipelineDynamicStateCreateInfo,
        .layout = *res->pipelineLayout,
        .renderPass = *res->renderPass,
    };

    res->pipeline =
        device.createGraphicsPipelineUnique(nullptr, graphicsPipelineCreateInfo).value;

    return res;
}

std::unique_ptr<ImageInfo> GfxstreamEnd2EndVkSnapshotPipelineTest::createColorAttachment(vkhpp::PhysicalDevice physicalDevice, vkhpp::Device device) {
    std::unique_ptr<ImageInfo> res(new ImageInfo);

    const vkhpp::ImageCreateInfo imageCreateInfo = {
        .pNext = nullptr,
        .imageType = vkhpp::ImageType::e2D,
        .extent.width = kFbWidth,
        .extent.height = kFbHeight,
        .extent.depth = 1,
        .mipLevels = 1,
        .arrayLayers = 1,
        .format = vkhpp::Format::eR8G8B8A8Unorm,
        .tiling = vkhpp::ImageTiling::eOptimal,
        .initialLayout = vkhpp::ImageLayout::eUndefined,
        .usage = vkhpp::ImageUsageFlagBits::eColorAttachment | vkhpp::ImageUsageFlagBits::eSampled | vkhpp::ImageUsageFlagBits::eTransferDst |
                 vkhpp::ImageUsageFlagBits::eTransferSrc,
        .sharingMode = vkhpp::SharingMode::eExclusive,
        .samples = vkhpp::SampleCountFlagBits::e1,
    };
    res->image = device.createImageUnique(imageCreateInfo).value;

    vkhpp::MemoryRequirements imageMemoryRequirements{};
    device.getImageMemoryRequirements(*res->image, &imageMemoryRequirements);

    const uint32_t imageMemoryIndex = GetMemoryType(physicalDevice, imageMemoryRequirements,
                                                    vkhpp::MemoryPropertyFlagBits::eDeviceLocal);

    const vkhpp::MemoryAllocateInfo imageMemoryAllocateInfo = {
        .allocationSize = imageMemoryRequirements.size,
        .memoryTypeIndex = imageMemoryIndex,
    };

    res->memory = device.allocateMemoryUnique(imageMemoryAllocateInfo).value;

    device.bindImageMemory(*res->image, *res->memory, 0);

    const vkhpp::ImageViewCreateInfo imageViewCreateInfo = {
        .image = *res->image,
        .viewType = vkhpp::ImageViewType::e2D,
        .format = vkhpp::Format::eR8G8B8A8Unorm,
        .subresourceRange =
            {
                .aspectMask = vkhpp::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };
    res->imageView = device.createImageViewUnique(imageViewCreateInfo).value;
    return res;
}

TEST_P(GfxstreamEnd2EndVkSnapshotPipelineTest, CanRecreateShaderModule) {
    auto [instance, physicalDevice, device, queue, queueFamilyIndex] =
        VK_ASSERT(SetUpTypicalVkTestEnvironment());
    auto pipelineInfo = createPipeline(device.get());
    ASSERT_THAT(pipelineInfo->renderPass, IsValidHandle());
    ASSERT_THAT(pipelineInfo->descriptorSetLayout, IsValidHandle());
    ASSERT_THAT(pipelineInfo->pipelineLayout, IsValidHandle());
    ASSERT_THAT(pipelineInfo->vertexShaderModule, IsValidHandle());
    ASSERT_THAT(pipelineInfo->fragmentShaderModule, IsValidHandle());
    ASSERT_THAT(pipelineInfo->pipeline, IsValidHandle());

    // Check if snapshot can restore the pipeline even after shaders are destroyed.
    pipelineInfo->vertexShaderModule.reset();
    pipelineInfo->fragmentShaderModule.reset();

    SnapshotSaveAndLoad();
    // Don't crash
    // TODO(b/330763497): try to render something
    // TODO(b/330766521): fix dangling shader modules after snapshot load
}

// vkCreateDescriptorPool injects extra handles into the internal handle map, thus add
// a test for it.
TEST_P(GfxstreamEnd2EndVkSnapshotPipelineTest, CanSnapshotDescriptorPool) {
    auto [instance, physicalDevice, device, queue, queueFamilyIndex] =
        VK_ASSERT(SetUpTypicalVkTestEnvironment());
    std::vector<vkhpp::DescriptorPoolSize> sizes = {
        {
            .descriptorCount = 10,
        },
    };
    vkhpp::DescriptorPoolCreateInfo descriptorPoolCreateInfo = {
        .maxSets = 10,
        .poolSizeCount = static_cast<uint32_t>(sizes.size()),
        .pPoolSizes = sizes.data(),
    };
    auto descriptorPool0 = device->createDescriptorPoolUnique(descriptorPoolCreateInfo).value;
    ASSERT_THAT(descriptorPool0, IsValidHandle());
    auto descriptorPool1 = device->createDescriptorPoolUnique(descriptorPoolCreateInfo).value;
    ASSERT_THAT(descriptorPool1, IsValidHandle());

    vkhpp::DescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
    auto descriptorSetLayout =
        device->createDescriptorSetLayoutUnique(descriptorSetLayoutInfo).value;
    ASSERT_THAT(descriptorSetLayout, IsValidHandle());

    SnapshotSaveAndLoad();

    const std::vector<vkhpp::DescriptorSetLayout> descriptorSetLayouts(1, *descriptorSetLayout);

    vkhpp::DescriptorSetAllocateInfo descriptorSetAllocateInfo0 = {
        .descriptorPool = *descriptorPool0,
        .descriptorSetCount = 1,
        .pSetLayouts = descriptorSetLayouts.data(),
    };
    auto descriptorSets0 = device->allocateDescriptorSetsUnique(descriptorSetAllocateInfo0);
    EXPECT_THAT(descriptorSets0.result, Eq(vkhpp::Result::eSuccess));

    vkhpp::DescriptorSetAllocateInfo descriptorSetAllocateInfo1 = {
        .descriptorPool = *descriptorPool1,
        .descriptorSetCount = 1,
        .pSetLayouts = descriptorSetLayouts.data(),
    };
    auto descriptorSets1 = device->allocateDescriptorSetsUnique(descriptorSetAllocateInfo1);
    EXPECT_THAT(descriptorSets1.result, Eq(vkhpp::Result::eSuccess));
}

TEST_P(GfxstreamEnd2EndVkSnapshotPipelineTest, CanSnapshotFramebuffer) {
    auto [instance, physicalDevice, device, queue, queueFamilyIndex] =
        VK_ASSERT(SetUpTypicalVkTestEnvironment());
    auto renderPass = createRenderPass(device.get());
    ASSERT_THAT(renderPass, IsValidHandle());

    auto colorAttachmentInfo = createColorAttachment(physicalDevice, device.get());
    ASSERT_THAT(colorAttachmentInfo->image, IsValidHandle());
    ASSERT_THAT(colorAttachmentInfo->memory, IsValidHandle());
    ASSERT_THAT(colorAttachmentInfo->imageView, IsValidHandle());

    const std::vector<vkhpp::ImageView> attachments(1, *colorAttachmentInfo->imageView);
    vkhpp::FramebufferCreateInfo framebufferCreateInfo = {
        .renderPass = *renderPass,
        .attachmentCount = 1,
        .pAttachments = attachments.data(),
        .width = kFbWidth,
        .height = kFbHeight,
        .layers = 1,
    };
    auto framebuffer = device->createFramebufferUnique(framebufferCreateInfo).value;
    ASSERT_THAT(framebuffer, IsValidHandle());

    SnapshotSaveAndLoad();
}

INSTANTIATE_TEST_CASE_P(GfxstreamEnd2EndTests, GfxstreamEnd2EndVkSnapshotPipelineTest,
                        ::testing::ValuesIn({
                            TestParams{
                                .with_gl = false,
                                .with_vk = true,
                                .with_vk_snapshot = true,
                            },
                        }),
                        &GetTestName);
}  // namespace
}  // namespace tests
}  // namespace gfxstream