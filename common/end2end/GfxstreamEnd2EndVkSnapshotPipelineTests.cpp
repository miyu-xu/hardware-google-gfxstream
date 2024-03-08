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

class GfxstreamEnd2EndVkSnapshotPipelineTest : public GfxstreamEnd2EndTest {};

// A blue triangle
const float kVertexData[] = {
	-1.0f, 0.0f, 0.0f, 1.0f,	0.0f, 0.0f, 1.0f, 1.0f,
	1.0f, 0.0f, 0.0f, 1.0f,		0.0f, 0.0f, 1.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,		0.0f, 0.0f, 1.0f, 1.0f,
};

TEST_P(GfxstreamEnd2EndVkSnapshotPipelineTest, CanRecreateShaderModule) {
    auto [instance, physicalDevice, device, queue, queueFamilyIndex] =
        VK_ASSERT(SetUpTypicalVkTestEnvironment());
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
    auto renderPass = device->createRenderPassUnique(renderPassCreateInfo).value;

    vkhpp::DescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
    auto descriptorSetLayout = device->createDescriptorSetLayoutUnique(descriptorSetLayoutInfo).value;
    auto pipelineLayout = device->createPipelineLayoutUnique(vkhpp::PipelineLayoutCreateInfo {}).value;

    vkhpp::ShaderModuleCreateInfo vertexShaderModuleCreateInfo = {
    	.codeSize = sizeof(simple_shader_vert),
    	.pCode = (const uint32_t*)simple_shader_vert,
    };
    vkhpp::ShaderModuleCreateInfo fragmentShaderModuleCreateInfo = {
    	.codeSize = sizeof(simple_shader_frag),
    	.pCode = (const uint32_t*)simple_shader_frag,
    };
    auto vertexShaderModule = device->createShaderModuleUnique(vertexShaderModuleCreateInfo).value;
    auto fragmentShaderModule = device->createShaderModuleUnique(fragmentShaderModuleCreateInfo).value;

    vkhpp::PipelineShaderStageCreateInfo pipelineShaderStageCreateInfos[2] = {
       {
      	.stage = vkhpp::ShaderStageFlagBits::eVertex,
      	.module = *vertexShaderModule,
      	.pName = "main",
      },
      {
      	.stage = vkhpp::ShaderStageFlagBits::eFragment,
      	.module = *fragmentShaderModule,
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
    	.cullMode = vkhpp::CullModeFlagBits::eFrontAndBack,
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
	         .colorWriteMask = vkhpp::ColorComponentFlagBits::eR | vkhpp::ColorComponentFlagBits::eG | vkhpp::ColorComponentFlagBits::eB | vkhpp::ColorComponentFlagBits::eA
    };
    const vkhpp::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = {
    	.attachmentCount = 1,
    	.pAttachments = &pipelineColorBlendAttachmentState,
    };
    const vkhpp::DynamicState dynamicStates[2] = { vkhpp::DynamicState::eViewport, vkhpp::DynamicState::eScissor };
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
		.layout = *pipelineLayout,
		.renderPass = *renderPass,
    };

    auto resultWithPipeline = device->createGraphicsPipelineUnique( nullptr, graphicsPipelineCreateInfo );
    ASSERT_THAT(resultWithPipeline.result, IsVkSuccess());
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