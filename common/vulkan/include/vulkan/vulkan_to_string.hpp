// Copyright 2015-2021 The Khronos Group Inc.
// 
// SPDX-License-Identifier: Apache-2.0 OR MIT
//

// This header is generated from the Khronos Vulkan XML API Registry.

#ifndef VULKAN_TO_STRING_HPP
#  define VULKAN_TO_STRING_HPP

#include <vulkan/vulkan_enums.hpp>

#if __cpp_lib_format
#  include <format>   // std::format
#else
#  include <sstream>  // std::stringstream
#endif

namespace VULKAN_HPP_NAMESPACE
{

  //==========================
  //=== BITMASKs to_string ===
  //==========================


  //=== VK_VERSION_1_0 ===

  VULKAN_HPP_INLINE std::string to_string( FormatFeatureFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & FormatFeatureFlagBits::eSampledImage ) result += "SampledImage | ";
    if ( value & FormatFeatureFlagBits::eStorageImage ) result += "StorageImage | ";
    if ( value & FormatFeatureFlagBits::eStorageImageAtomic ) result += "StorageImageAtomic | ";
    if ( value & FormatFeatureFlagBits::eUniformTexelBuffer ) result += "UniformTexelBuffer | ";
    if ( value & FormatFeatureFlagBits::eStorageTexelBuffer ) result += "StorageTexelBuffer | ";
    if ( value & FormatFeatureFlagBits::eStorageTexelBufferAtomic ) result += "StorageTexelBufferAtomic | ";
    if ( value & FormatFeatureFlagBits::eVertexBuffer ) result += "VertexBuffer | ";
    if ( value & FormatFeatureFlagBits::eColorAttachment ) result += "ColorAttachment | ";
    if ( value & FormatFeatureFlagBits::eColorAttachmentBlend ) result += "ColorAttachmentBlend | ";
    if ( value & FormatFeatureFlagBits::eDepthStencilAttachment ) result += "DepthStencilAttachment | ";
    if ( value & FormatFeatureFlagBits::eBlitSrc ) result += "BlitSrc | ";
    if ( value & FormatFeatureFlagBits::eBlitDst ) result += "BlitDst | ";
    if ( value & FormatFeatureFlagBits::eSampledImageFilterLinear ) result += "SampledImageFilterLinear | ";
    if ( value & FormatFeatureFlagBits::eTransferSrc ) result += "TransferSrc | ";
    if ( value & FormatFeatureFlagBits::eTransferDst ) result += "TransferDst | ";
    if ( value & FormatFeatureFlagBits::eMidpointChromaSamples ) result += "MidpointChromaSamples | ";
    if ( value & FormatFeatureFlagBits::eSampledImageYcbcrConversionLinearFilter ) result += "SampledImageYcbcrConversionLinearFilter | ";
    if ( value & FormatFeatureFlagBits::eSampledImageYcbcrConversionSeparateReconstructionFilter ) result += "SampledImageYcbcrConversionSeparateReconstructionFilter | ";
    if ( value & FormatFeatureFlagBits::eSampledImageYcbcrConversionChromaReconstructionExplicit ) result += "SampledImageYcbcrConversionChromaReconstructionExplicit | ";
    if ( value & FormatFeatureFlagBits::eSampledImageYcbcrConversionChromaReconstructionExplicitForceable ) result += "SampledImageYcbcrConversionChromaReconstructionExplicitForceable | ";
    if ( value & FormatFeatureFlagBits::eDisjoint ) result += "Disjoint | ";
    if ( value & FormatFeatureFlagBits::eCositedChromaSamples ) result += "CositedChromaSamples | ";
    if ( value & FormatFeatureFlagBits::eSampledImageFilterMinmax ) result += "SampledImageFilterMinmax | ";
    if ( value & FormatFeatureFlagBits::eSampledImageFilterCubicIMG ) result += "SampledImageFilterCubicIMG | ";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
    if ( value & FormatFeatureFlagBits::eVideoDecodeOutputKHR ) result += "VideoDecodeOutputKHR | ";
    if ( value & FormatFeatureFlagBits::eVideoDecodeDpbKHR ) result += "VideoDecodeDpbKHR | ";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
    if ( value & FormatFeatureFlagBits::eAccelerationStructureVertexBufferKHR ) result += "AccelerationStructureVertexBufferKHR | ";
    if ( value & FormatFeatureFlagBits::eFragmentDensityMapEXT ) result += "FragmentDensityMapEXT | ";
    if ( value & FormatFeatureFlagBits::eFragmentShadingRateAttachmentKHR ) result += "FragmentShadingRateAttachmentKHR | ";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
    if ( value & FormatFeatureFlagBits::eVideoEncodeInputKHR ) result += "VideoEncodeInputKHR | ";
    if ( value & FormatFeatureFlagBits::eVideoEncodeDpbKHR ) result += "VideoEncodeDpbKHR | ";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( ImageCreateFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & ImageCreateFlagBits::eSparseBinding ) result += "SparseBinding | ";
    if ( value & ImageCreateFlagBits::eSparseResidency ) result += "SparseResidency | ";
    if ( value & ImageCreateFlagBits::eSparseAliased ) result += "SparseAliased | ";
    if ( value & ImageCreateFlagBits::eMutableFormat ) result += "MutableFormat | ";
    if ( value & ImageCreateFlagBits::eCubeCompatible ) result += "CubeCompatible | ";
    if ( value & ImageCreateFlagBits::eAlias ) result += "Alias | ";
    if ( value & ImageCreateFlagBits::eSplitInstanceBindRegions ) result += "SplitInstanceBindRegions | ";
    if ( value & ImageCreateFlagBits::e2DArrayCompatible ) result += "2DArrayCompatible | ";
    if ( value & ImageCreateFlagBits::eBlockTexelViewCompatible ) result += "BlockTexelViewCompatible | ";
    if ( value & ImageCreateFlagBits::eExtendedUsage ) result += "ExtendedUsage | ";
    if ( value & ImageCreateFlagBits::eProtected ) result += "Protected | ";
    if ( value & ImageCreateFlagBits::eDisjoint ) result += "Disjoint | ";
    if ( value & ImageCreateFlagBits::eCornerSampledNV ) result += "CornerSampledNV | ";
    if ( value & ImageCreateFlagBits::eSampleLocationsCompatibleDepthEXT ) result += "SampleLocationsCompatibleDepthEXT | ";
    if ( value & ImageCreateFlagBits::eSubsampledEXT ) result += "SubsampledEXT | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( ImageUsageFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & ImageUsageFlagBits::eTransferSrc ) result += "TransferSrc | ";
    if ( value & ImageUsageFlagBits::eTransferDst ) result += "TransferDst | ";
    if ( value & ImageUsageFlagBits::eSampled ) result += "Sampled | ";
    if ( value & ImageUsageFlagBits::eStorage ) result += "Storage | ";
    if ( value & ImageUsageFlagBits::eColorAttachment ) result += "ColorAttachment | ";
    if ( value & ImageUsageFlagBits::eDepthStencilAttachment ) result += "DepthStencilAttachment | ";
    if ( value & ImageUsageFlagBits::eTransientAttachment ) result += "TransientAttachment | ";
    if ( value & ImageUsageFlagBits::eInputAttachment ) result += "InputAttachment | ";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
    if ( value & ImageUsageFlagBits::eVideoDecodeDstKHR ) result += "VideoDecodeDstKHR | ";
    if ( value & ImageUsageFlagBits::eVideoDecodeSrcKHR ) result += "VideoDecodeSrcKHR | ";
    if ( value & ImageUsageFlagBits::eVideoDecodeDpbKHR ) result += "VideoDecodeDpbKHR | ";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
    if ( value & ImageUsageFlagBits::eFragmentDensityMapEXT ) result += "FragmentDensityMapEXT | ";
    if ( value & ImageUsageFlagBits::eFragmentShadingRateAttachmentKHR ) result += "FragmentShadingRateAttachmentKHR | ";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
    if ( value & ImageUsageFlagBits::eVideoEncodeDstKHR ) result += "VideoEncodeDstKHR | ";
    if ( value & ImageUsageFlagBits::eVideoEncodeSrcKHR ) result += "VideoEncodeSrcKHR | ";
    if ( value & ImageUsageFlagBits::eVideoEncodeDpbKHR ) result += "VideoEncodeDpbKHR | ";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
    if ( value & ImageUsageFlagBits::eInvocationMaskHUAWEI ) result += "InvocationMaskHUAWEI | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( InstanceCreateFlags )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( MemoryHeapFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & MemoryHeapFlagBits::eDeviceLocal ) result += "DeviceLocal | ";
    if ( value & MemoryHeapFlagBits::eMultiInstance ) result += "MultiInstance | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( MemoryPropertyFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & MemoryPropertyFlagBits::eDeviceLocal ) result += "DeviceLocal | ";
    if ( value & MemoryPropertyFlagBits::eHostVisible ) result += "HostVisible | ";
    if ( value & MemoryPropertyFlagBits::eHostCoherent ) result += "HostCoherent | ";
    if ( value & MemoryPropertyFlagBits::eHostCached ) result += "HostCached | ";
    if ( value & MemoryPropertyFlagBits::eLazilyAllocated ) result += "LazilyAllocated | ";
    if ( value & MemoryPropertyFlagBits::eProtected ) result += "Protected | ";
    if ( value & MemoryPropertyFlagBits::eDeviceCoherentAMD ) result += "DeviceCoherentAMD | ";
    if ( value & MemoryPropertyFlagBits::eDeviceUncachedAMD ) result += "DeviceUncachedAMD | ";
    if ( value & MemoryPropertyFlagBits::eRdmaCapableNV ) result += "RdmaCapableNV | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( QueueFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & QueueFlagBits::eGraphics ) result += "Graphics | ";
    if ( value & QueueFlagBits::eCompute ) result += "Compute | ";
    if ( value & QueueFlagBits::eTransfer ) result += "Transfer | ";
    if ( value & QueueFlagBits::eSparseBinding ) result += "SparseBinding | ";
    if ( value & QueueFlagBits::eProtected ) result += "Protected | ";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
    if ( value & QueueFlagBits::eVideoDecodeKHR ) result += "VideoDecodeKHR | ";
    if ( value & QueueFlagBits::eVideoEncodeKHR ) result += "VideoEncodeKHR | ";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( SampleCountFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & SampleCountFlagBits::e1 ) result += "1 | ";
    if ( value & SampleCountFlagBits::e2 ) result += "2 | ";
    if ( value & SampleCountFlagBits::e4 ) result += "4 | ";
    if ( value & SampleCountFlagBits::e8 ) result += "8 | ";
    if ( value & SampleCountFlagBits::e16 ) result += "16 | ";
    if ( value & SampleCountFlagBits::e32 ) result += "32 | ";
    if ( value & SampleCountFlagBits::e64 ) result += "64 | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( DeviceCreateFlags )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( DeviceQueueCreateFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & DeviceQueueCreateFlagBits::eProtected ) result += "Protected | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( PipelineStageFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & PipelineStageFlagBits::eTopOfPipe ) result += "TopOfPipe | ";
    if ( value & PipelineStageFlagBits::eDrawIndirect ) result += "DrawIndirect | ";
    if ( value & PipelineStageFlagBits::eVertexInput ) result += "VertexInput | ";
    if ( value & PipelineStageFlagBits::eVertexShader ) result += "VertexShader | ";
    if ( value & PipelineStageFlagBits::eTessellationControlShader ) result += "TessellationControlShader | ";
    if ( value & PipelineStageFlagBits::eTessellationEvaluationShader ) result += "TessellationEvaluationShader | ";
    if ( value & PipelineStageFlagBits::eGeometryShader ) result += "GeometryShader | ";
    if ( value & PipelineStageFlagBits::eFragmentShader ) result += "FragmentShader | ";
    if ( value & PipelineStageFlagBits::eEarlyFragmentTests ) result += "EarlyFragmentTests | ";
    if ( value & PipelineStageFlagBits::eLateFragmentTests ) result += "LateFragmentTests | ";
    if ( value & PipelineStageFlagBits::eColorAttachmentOutput ) result += "ColorAttachmentOutput | ";
    if ( value & PipelineStageFlagBits::eComputeShader ) result += "ComputeShader | ";
    if ( value & PipelineStageFlagBits::eTransfer ) result += "Transfer | ";
    if ( value & PipelineStageFlagBits::eBottomOfPipe ) result += "BottomOfPipe | ";
    if ( value & PipelineStageFlagBits::eHost ) result += "Host | ";
    if ( value & PipelineStageFlagBits::eAllGraphics ) result += "AllGraphics | ";
    if ( value & PipelineStageFlagBits::eAllCommands ) result += "AllCommands | ";
    if ( value & PipelineStageFlagBits::eTransformFeedbackEXT ) result += "TransformFeedbackEXT | ";
    if ( value & PipelineStageFlagBits::eConditionalRenderingEXT ) result += "ConditionalRenderingEXT | ";
    if ( value & PipelineStageFlagBits::eAccelerationStructureBuildKHR ) result += "AccelerationStructureBuildKHR | ";
    if ( value & PipelineStageFlagBits::eRayTracingShaderKHR ) result += "RayTracingShaderKHR | ";
    if ( value & PipelineStageFlagBits::eTaskShaderNV ) result += "TaskShaderNV | ";
    if ( value & PipelineStageFlagBits::eMeshShaderNV ) result += "MeshShaderNV | ";
    if ( value & PipelineStageFlagBits::eFragmentDensityProcessEXT ) result += "FragmentDensityProcessEXT | ";
    if ( value & PipelineStageFlagBits::eFragmentShadingRateAttachmentKHR ) result += "FragmentShadingRateAttachmentKHR | ";
    if ( value & PipelineStageFlagBits::eCommandPreprocessNV ) result += "CommandPreprocessNV | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( MemoryMapFlags )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( ImageAspectFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & ImageAspectFlagBits::eColor ) result += "Color | ";
    if ( value & ImageAspectFlagBits::eDepth ) result += "Depth | ";
    if ( value & ImageAspectFlagBits::eStencil ) result += "Stencil | ";
    if ( value & ImageAspectFlagBits::eMetadata ) result += "Metadata | ";
    if ( value & ImageAspectFlagBits::ePlane0 ) result += "Plane0 | ";
    if ( value & ImageAspectFlagBits::ePlane1 ) result += "Plane1 | ";
    if ( value & ImageAspectFlagBits::ePlane2 ) result += "Plane2 | ";
    if ( value & ImageAspectFlagBits::eMemoryPlane0EXT ) result += "MemoryPlane0EXT | ";
    if ( value & ImageAspectFlagBits::eMemoryPlane1EXT ) result += "MemoryPlane1EXT | ";
    if ( value & ImageAspectFlagBits::eMemoryPlane2EXT ) result += "MemoryPlane2EXT | ";
    if ( value & ImageAspectFlagBits::eMemoryPlane3EXT ) result += "MemoryPlane3EXT | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( SparseImageFormatFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & SparseImageFormatFlagBits::eSingleMiptail ) result += "SingleMiptail | ";
    if ( value & SparseImageFormatFlagBits::eAlignedMipSize ) result += "AlignedMipSize | ";
    if ( value & SparseImageFormatFlagBits::eNonstandardBlockSize ) result += "NonstandardBlockSize | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( SparseMemoryBindFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & SparseMemoryBindFlagBits::eMetadata ) result += "Metadata | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( FenceCreateFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & FenceCreateFlagBits::eSignaled ) result += "Signaled | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( SemaphoreCreateFlags )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( EventCreateFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & EventCreateFlagBits::eDeviceOnlyKHR ) result += "DeviceOnlyKHR | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( QueryPipelineStatisticFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & QueryPipelineStatisticFlagBits::eInputAssemblyVertices ) result += "InputAssemblyVertices | ";
    if ( value & QueryPipelineStatisticFlagBits::eInputAssemblyPrimitives ) result += "InputAssemblyPrimitives | ";
    if ( value & QueryPipelineStatisticFlagBits::eVertexShaderInvocations ) result += "VertexShaderInvocations | ";
    if ( value & QueryPipelineStatisticFlagBits::eGeometryShaderInvocations ) result += "GeometryShaderInvocations | ";
    if ( value & QueryPipelineStatisticFlagBits::eGeometryShaderPrimitives ) result += "GeometryShaderPrimitives | ";
    if ( value & QueryPipelineStatisticFlagBits::eClippingInvocations ) result += "ClippingInvocations | ";
    if ( value & QueryPipelineStatisticFlagBits::eClippingPrimitives ) result += "ClippingPrimitives | ";
    if ( value & QueryPipelineStatisticFlagBits::eFragmentShaderInvocations ) result += "FragmentShaderInvocations | ";
    if ( value & QueryPipelineStatisticFlagBits::eTessellationControlShaderPatches ) result += "TessellationControlShaderPatches | ";
    if ( value & QueryPipelineStatisticFlagBits::eTessellationEvaluationShaderInvocations ) result += "TessellationEvaluationShaderInvocations | ";
    if ( value & QueryPipelineStatisticFlagBits::eComputeShaderInvocations ) result += "ComputeShaderInvocations | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( QueryPoolCreateFlags )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( QueryResultFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & QueryResultFlagBits::e64 ) result += "64 | ";
    if ( value & QueryResultFlagBits::eWait ) result += "Wait | ";
    if ( value & QueryResultFlagBits::eWithAvailability ) result += "WithAvailability | ";
    if ( value & QueryResultFlagBits::ePartial ) result += "Partial | ";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
    if ( value & QueryResultFlagBits::eWithStatusKHR ) result += "WithStatusKHR | ";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( BufferCreateFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & BufferCreateFlagBits::eSparseBinding ) result += "SparseBinding | ";
    if ( value & BufferCreateFlagBits::eSparseResidency ) result += "SparseResidency | ";
    if ( value & BufferCreateFlagBits::eSparseAliased ) result += "SparseAliased | ";
    if ( value & BufferCreateFlagBits::eProtected ) result += "Protected | ";
    if ( value & BufferCreateFlagBits::eDeviceAddressCaptureReplay ) result += "DeviceAddressCaptureReplay | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( BufferUsageFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & BufferUsageFlagBits::eTransferSrc ) result += "TransferSrc | ";
    if ( value & BufferUsageFlagBits::eTransferDst ) result += "TransferDst | ";
    if ( value & BufferUsageFlagBits::eUniformTexelBuffer ) result += "UniformTexelBuffer | ";
    if ( value & BufferUsageFlagBits::eStorageTexelBuffer ) result += "StorageTexelBuffer | ";
    if ( value & BufferUsageFlagBits::eUniformBuffer ) result += "UniformBuffer | ";
    if ( value & BufferUsageFlagBits::eStorageBuffer ) result += "StorageBuffer | ";
    if ( value & BufferUsageFlagBits::eIndexBuffer ) result += "IndexBuffer | ";
    if ( value & BufferUsageFlagBits::eVertexBuffer ) result += "VertexBuffer | ";
    if ( value & BufferUsageFlagBits::eIndirectBuffer ) result += "IndirectBuffer | ";
    if ( value & BufferUsageFlagBits::eShaderDeviceAddress ) result += "ShaderDeviceAddress | ";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
    if ( value & BufferUsageFlagBits::eVideoDecodeSrcKHR ) result += "VideoDecodeSrcKHR | ";
    if ( value & BufferUsageFlagBits::eVideoDecodeDstKHR ) result += "VideoDecodeDstKHR | ";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
    if ( value & BufferUsageFlagBits::eTransformFeedbackBufferEXT ) result += "TransformFeedbackBufferEXT | ";
    if ( value & BufferUsageFlagBits::eTransformFeedbackCounterBufferEXT ) result += "TransformFeedbackCounterBufferEXT | ";
    if ( value & BufferUsageFlagBits::eConditionalRenderingEXT ) result += "ConditionalRenderingEXT | ";
    if ( value & BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR ) result += "AccelerationStructureBuildInputReadOnlyKHR | ";
    if ( value & BufferUsageFlagBits::eAccelerationStructureStorageKHR ) result += "AccelerationStructureStorageKHR | ";
    if ( value & BufferUsageFlagBits::eShaderBindingTableKHR ) result += "ShaderBindingTableKHR | ";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
    if ( value & BufferUsageFlagBits::eVideoEncodeDstKHR ) result += "VideoEncodeDstKHR | ";
    if ( value & BufferUsageFlagBits::eVideoEncodeSrcKHR ) result += "VideoEncodeSrcKHR | ";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( BufferViewCreateFlags )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( ImageViewCreateFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & ImageViewCreateFlagBits::eFragmentDensityMapDynamicEXT ) result += "FragmentDensityMapDynamicEXT | ";
    if ( value & ImageViewCreateFlagBits::eFragmentDensityMapDeferredEXT ) result += "FragmentDensityMapDeferredEXT | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( ShaderModuleCreateFlags )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( PipelineCacheCreateFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & PipelineCacheCreateFlagBits::eExternallySynchronizedEXT ) result += "ExternallySynchronizedEXT | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( ColorComponentFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & ColorComponentFlagBits::eR ) result += "R | ";
    if ( value & ColorComponentFlagBits::eG ) result += "G | ";
    if ( value & ColorComponentFlagBits::eB ) result += "B | ";
    if ( value & ColorComponentFlagBits::eA ) result += "A | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( CullModeFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & CullModeFlagBits::eFront ) result += "Front | ";
    if ( value & CullModeFlagBits::eBack ) result += "Back | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( PipelineColorBlendStateCreateFlags )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( PipelineCreateFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & PipelineCreateFlagBits::eDisableOptimization ) result += "DisableOptimization | ";
    if ( value & PipelineCreateFlagBits::eAllowDerivatives ) result += "AllowDerivatives | ";
    if ( value & PipelineCreateFlagBits::eDerivative ) result += "Derivative | ";
    if ( value & PipelineCreateFlagBits::eViewIndexFromDeviceIndex ) result += "ViewIndexFromDeviceIndex | ";
    if ( value & PipelineCreateFlagBits::eDispatchBase ) result += "DispatchBase | ";
    if ( value & PipelineCreateFlagBits::eVkPipelineRasterizationStateCreateFragmentShadingRateAttachmentKHR ) result += "VkPipelineRasterizationStateCreateFragmentShadingRateAttachmentKHR | ";
    if ( value & PipelineCreateFlagBits::eVkPipelineRasterizationStateCreateFragmentDensityMapAttachmentEXT ) result += "VkPipelineRasterizationStateCreateFragmentDensityMapAttachmentEXT | ";
    if ( value & PipelineCreateFlagBits::eRayTracingNoNullAnyHitShadersKHR ) result += "RayTracingNoNullAnyHitShadersKHR | ";
    if ( value & PipelineCreateFlagBits::eRayTracingNoNullClosestHitShadersKHR ) result += "RayTracingNoNullClosestHitShadersKHR | ";
    if ( value & PipelineCreateFlagBits::eRayTracingNoNullMissShadersKHR ) result += "RayTracingNoNullMissShadersKHR | ";
    if ( value & PipelineCreateFlagBits::eRayTracingNoNullIntersectionShadersKHR ) result += "RayTracingNoNullIntersectionShadersKHR | ";
    if ( value & PipelineCreateFlagBits::eRayTracingSkipTrianglesKHR ) result += "RayTracingSkipTrianglesKHR | ";
    if ( value & PipelineCreateFlagBits::eRayTracingSkipAabbsKHR ) result += "RayTracingSkipAabbsKHR | ";
    if ( value & PipelineCreateFlagBits::eRayTracingShaderGroupHandleCaptureReplayKHR ) result += "RayTracingShaderGroupHandleCaptureReplayKHR | ";
    if ( value & PipelineCreateFlagBits::eDeferCompileNV ) result += "DeferCompileNV | ";
    if ( value & PipelineCreateFlagBits::eCaptureStatisticsKHR ) result += "CaptureStatisticsKHR | ";
    if ( value & PipelineCreateFlagBits::eCaptureInternalRepresentationsKHR ) result += "CaptureInternalRepresentationsKHR | ";
    if ( value & PipelineCreateFlagBits::eIndirectBindableNV ) result += "IndirectBindableNV | ";
    if ( value & PipelineCreateFlagBits::eLibraryKHR ) result += "LibraryKHR | ";
    if ( value & PipelineCreateFlagBits::eFailOnPipelineCompileRequiredEXT ) result += "FailOnPipelineCompileRequiredEXT | ";
    if ( value & PipelineCreateFlagBits::eEarlyReturnOnFailureEXT ) result += "EarlyReturnOnFailureEXT | ";
    if ( value & PipelineCreateFlagBits::eRayTracingAllowMotionNV ) result += "RayTracingAllowMotionNV | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( PipelineDepthStencilStateCreateFlags )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( PipelineDynamicStateCreateFlags )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( PipelineInputAssemblyStateCreateFlags )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( PipelineLayoutCreateFlags )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( PipelineMultisampleStateCreateFlags )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( PipelineRasterizationStateCreateFlags )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( PipelineShaderStageCreateFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & PipelineShaderStageCreateFlagBits::eAllowVaryingSubgroupSizeEXT ) result += "AllowVaryingSubgroupSizeEXT | ";
    if ( value & PipelineShaderStageCreateFlagBits::eRequireFullSubgroupsEXT ) result += "RequireFullSubgroupsEXT | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( PipelineTessellationStateCreateFlags )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( PipelineVertexInputStateCreateFlags )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( PipelineViewportStateCreateFlags )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( ShaderStageFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & ShaderStageFlagBits::eVertex ) result += "Vertex | ";
    if ( value & ShaderStageFlagBits::eTessellationControl ) result += "TessellationControl | ";
    if ( value & ShaderStageFlagBits::eTessellationEvaluation ) result += "TessellationEvaluation | ";
    if ( value & ShaderStageFlagBits::eGeometry ) result += "Geometry | ";
    if ( value & ShaderStageFlagBits::eFragment ) result += "Fragment | ";
    if ( value & ShaderStageFlagBits::eCompute ) result += "Compute | ";
    if ( value & ShaderStageFlagBits::eRaygenKHR ) result += "RaygenKHR | ";
    if ( value & ShaderStageFlagBits::eAnyHitKHR ) result += "AnyHitKHR | ";
    if ( value & ShaderStageFlagBits::eClosestHitKHR ) result += "ClosestHitKHR | ";
    if ( value & ShaderStageFlagBits::eMissKHR ) result += "MissKHR | ";
    if ( value & ShaderStageFlagBits::eIntersectionKHR ) result += "IntersectionKHR | ";
    if ( value & ShaderStageFlagBits::eCallableKHR ) result += "CallableKHR | ";
    if ( value & ShaderStageFlagBits::eTaskNV ) result += "TaskNV | ";
    if ( value & ShaderStageFlagBits::eMeshNV ) result += "MeshNV | ";
    if ( value & ShaderStageFlagBits::eSubpassShadingHUAWEI ) result += "SubpassShadingHUAWEI | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( SamplerCreateFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & SamplerCreateFlagBits::eSubsampledEXT ) result += "SubsampledEXT | ";
    if ( value & SamplerCreateFlagBits::eSubsampledCoarseReconstructionEXT ) result += "SubsampledCoarseReconstructionEXT | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( DescriptorPoolCreateFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & DescriptorPoolCreateFlagBits::eFreeDescriptorSet ) result += "FreeDescriptorSet | ";
    if ( value & DescriptorPoolCreateFlagBits::eUpdateAfterBind ) result += "UpdateAfterBind | ";
    if ( value & DescriptorPoolCreateFlagBits::eHostOnlyVALVE ) result += "HostOnlyVALVE | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( DescriptorPoolResetFlags )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( DescriptorSetLayoutCreateFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool ) result += "UpdateAfterBindPool | ";
    if ( value & DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR ) result += "PushDescriptorKHR | ";
    if ( value & DescriptorSetLayoutCreateFlagBits::eHostOnlyPoolVALVE ) result += "HostOnlyPoolVALVE | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( AccessFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & AccessFlagBits::eIndirectCommandRead ) result += "IndirectCommandRead | ";
    if ( value & AccessFlagBits::eIndexRead ) result += "IndexRead | ";
    if ( value & AccessFlagBits::eVertexAttributeRead ) result += "VertexAttributeRead | ";
    if ( value & AccessFlagBits::eUniformRead ) result += "UniformRead | ";
    if ( value & AccessFlagBits::eInputAttachmentRead ) result += "InputAttachmentRead | ";
    if ( value & AccessFlagBits::eShaderRead ) result += "ShaderRead | ";
    if ( value & AccessFlagBits::eShaderWrite ) result += "ShaderWrite | ";
    if ( value & AccessFlagBits::eColorAttachmentRead ) result += "ColorAttachmentRead | ";
    if ( value & AccessFlagBits::eColorAttachmentWrite ) result += "ColorAttachmentWrite | ";
    if ( value & AccessFlagBits::eDepthStencilAttachmentRead ) result += "DepthStencilAttachmentRead | ";
    if ( value & AccessFlagBits::eDepthStencilAttachmentWrite ) result += "DepthStencilAttachmentWrite | ";
    if ( value & AccessFlagBits::eTransferRead ) result += "TransferRead | ";
    if ( value & AccessFlagBits::eTransferWrite ) result += "TransferWrite | ";
    if ( value & AccessFlagBits::eHostRead ) result += "HostRead | ";
    if ( value & AccessFlagBits::eHostWrite ) result += "HostWrite | ";
    if ( value & AccessFlagBits::eMemoryRead ) result += "MemoryRead | ";
    if ( value & AccessFlagBits::eMemoryWrite ) result += "MemoryWrite | ";
    if ( value & AccessFlagBits::eTransformFeedbackWriteEXT ) result += "TransformFeedbackWriteEXT | ";
    if ( value & AccessFlagBits::eTransformFeedbackCounterReadEXT ) result += "TransformFeedbackCounterReadEXT | ";
    if ( value & AccessFlagBits::eTransformFeedbackCounterWriteEXT ) result += "TransformFeedbackCounterWriteEXT | ";
    if ( value & AccessFlagBits::eConditionalRenderingReadEXT ) result += "ConditionalRenderingReadEXT | ";
    if ( value & AccessFlagBits::eColorAttachmentReadNoncoherentEXT ) result += "ColorAttachmentReadNoncoherentEXT | ";
    if ( value & AccessFlagBits::eAccelerationStructureReadKHR ) result += "AccelerationStructureReadKHR | ";
    if ( value & AccessFlagBits::eAccelerationStructureWriteKHR ) result += "AccelerationStructureWriteKHR | ";
    if ( value & AccessFlagBits::eFragmentDensityMapReadEXT ) result += "FragmentDensityMapReadEXT | ";
    if ( value & AccessFlagBits::eFragmentShadingRateAttachmentReadKHR ) result += "FragmentShadingRateAttachmentReadKHR | ";
    if ( value & AccessFlagBits::eCommandPreprocessReadNV ) result += "CommandPreprocessReadNV | ";
    if ( value & AccessFlagBits::eCommandPreprocessWriteNV ) result += "CommandPreprocessWriteNV | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( AttachmentDescriptionFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & AttachmentDescriptionFlagBits::eMayAlias ) result += "MayAlias | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( DependencyFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & DependencyFlagBits::eByRegion ) result += "ByRegion | ";
    if ( value & DependencyFlagBits::eDeviceGroup ) result += "DeviceGroup | ";
    if ( value & DependencyFlagBits::eViewLocal ) result += "ViewLocal | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( FramebufferCreateFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & FramebufferCreateFlagBits::eImageless ) result += "Imageless | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( RenderPassCreateFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & RenderPassCreateFlagBits::eTransformQCOM ) result += "TransformQCOM | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( SubpassDescriptionFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & SubpassDescriptionFlagBits::ePerViewAttributesNVX ) result += "PerViewAttributesNVX | ";
    if ( value & SubpassDescriptionFlagBits::ePerViewPositionXOnlyNVX ) result += "PerViewPositionXOnlyNVX | ";
    if ( value & SubpassDescriptionFlagBits::eFragmentRegionQCOM ) result += "FragmentRegionQCOM | ";
    if ( value & SubpassDescriptionFlagBits::eShaderResolveQCOM ) result += "ShaderResolveQCOM | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( CommandPoolCreateFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & CommandPoolCreateFlagBits::eTransient ) result += "Transient | ";
    if ( value & CommandPoolCreateFlagBits::eResetCommandBuffer ) result += "ResetCommandBuffer | ";
    if ( value & CommandPoolCreateFlagBits::eProtected ) result += "Protected | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( CommandPoolResetFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & CommandPoolResetFlagBits::eReleaseResources ) result += "ReleaseResources | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( CommandBufferResetFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & CommandBufferResetFlagBits::eReleaseResources ) result += "ReleaseResources | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( CommandBufferUsageFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & CommandBufferUsageFlagBits::eOneTimeSubmit ) result += "OneTimeSubmit | ";
    if ( value & CommandBufferUsageFlagBits::eRenderPassContinue ) result += "RenderPassContinue | ";
    if ( value & CommandBufferUsageFlagBits::eSimultaneousUse ) result += "SimultaneousUse | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( QueryControlFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & QueryControlFlagBits::ePrecise ) result += "Precise | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( StencilFaceFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & StencilFaceFlagBits::eFront ) result += "Front | ";
    if ( value & StencilFaceFlagBits::eBack ) result += "Back | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  //=== VK_VERSION_1_1 ===

  VULKAN_HPP_INLINE std::string to_string( SubgroupFeatureFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & SubgroupFeatureFlagBits::eBasic ) result += "Basic | ";
    if ( value & SubgroupFeatureFlagBits::eVote ) result += "Vote | ";
    if ( value & SubgroupFeatureFlagBits::eArithmetic ) result += "Arithmetic | ";
    if ( value & SubgroupFeatureFlagBits::eBallot ) result += "Ballot | ";
    if ( value & SubgroupFeatureFlagBits::eShuffle ) result += "Shuffle | ";
    if ( value & SubgroupFeatureFlagBits::eShuffleRelative ) result += "ShuffleRelative | ";
    if ( value & SubgroupFeatureFlagBits::eClustered ) result += "Clustered | ";
    if ( value & SubgroupFeatureFlagBits::eQuad ) result += "Quad | ";
    if ( value & SubgroupFeatureFlagBits::ePartitionedNV ) result += "PartitionedNV | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( PeerMemoryFeatureFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & PeerMemoryFeatureFlagBits::eCopySrc ) result += "CopySrc | ";
    if ( value & PeerMemoryFeatureFlagBits::eCopyDst ) result += "CopyDst | ";
    if ( value & PeerMemoryFeatureFlagBits::eGenericSrc ) result += "GenericSrc | ";
    if ( value & PeerMemoryFeatureFlagBits::eGenericDst ) result += "GenericDst | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( MemoryAllocateFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & MemoryAllocateFlagBits::eDeviceMask ) result += "DeviceMask | ";
    if ( value & MemoryAllocateFlagBits::eDeviceAddress ) result += "DeviceAddress | ";
    if ( value & MemoryAllocateFlagBits::eDeviceAddressCaptureReplay ) result += "DeviceAddressCaptureReplay | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( CommandPoolTrimFlags )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( DescriptorUpdateTemplateCreateFlags )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( ExternalMemoryHandleTypeFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & ExternalMemoryHandleTypeFlagBits::eOpaqueFd ) result += "OpaqueFd | ";
    if ( value & ExternalMemoryHandleTypeFlagBits::eOpaqueWin32 ) result += "OpaqueWin32 | ";
    if ( value & ExternalMemoryHandleTypeFlagBits::eOpaqueWin32Kmt ) result += "OpaqueWin32Kmt | ";
    if ( value & ExternalMemoryHandleTypeFlagBits::eD3D11Texture ) result += "D3D11Texture | ";
    if ( value & ExternalMemoryHandleTypeFlagBits::eD3D11TextureKmt ) result += "D3D11TextureKmt | ";
    if ( value & ExternalMemoryHandleTypeFlagBits::eD3D12Heap ) result += "D3D12Heap | ";
    if ( value & ExternalMemoryHandleTypeFlagBits::eD3D12Resource ) result += "D3D12Resource | ";
    if ( value & ExternalMemoryHandleTypeFlagBits::eDmaBufEXT ) result += "DmaBufEXT | ";
#if defined( VK_USE_PLATFORM_ANDROID_KHR )
    if ( value & ExternalMemoryHandleTypeFlagBits::eAndroidHardwareBufferANDROID ) result += "AndroidHardwareBufferANDROID | ";
#endif /*VK_USE_PLATFORM_ANDROID_KHR*/
    if ( value & ExternalMemoryHandleTypeFlagBits::eHostAllocationEXT ) result += "HostAllocationEXT | ";
    if ( value & ExternalMemoryHandleTypeFlagBits::eHostMappedForeignMemoryEXT ) result += "HostMappedForeignMemoryEXT | ";
#if defined( VK_USE_PLATFORM_FUCHSIA )
    if ( value & ExternalMemoryHandleTypeFlagBits::eZirconVmoFUCHSIA ) result += "ZirconVmoFUCHSIA | ";
#endif /*VK_USE_PLATFORM_FUCHSIA*/
    if ( value & ExternalMemoryHandleTypeFlagBits::eRdmaAddressNV ) result += "RdmaAddressNV | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( ExternalMemoryFeatureFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & ExternalMemoryFeatureFlagBits::eDedicatedOnly ) result += "DedicatedOnly | ";
    if ( value & ExternalMemoryFeatureFlagBits::eExportable ) result += "Exportable | ";
    if ( value & ExternalMemoryFeatureFlagBits::eImportable ) result += "Importable | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( ExternalFenceHandleTypeFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & ExternalFenceHandleTypeFlagBits::eOpaqueFd ) result += "OpaqueFd | ";
    if ( value & ExternalFenceHandleTypeFlagBits::eOpaqueWin32 ) result += "OpaqueWin32 | ";
    if ( value & ExternalFenceHandleTypeFlagBits::eOpaqueWin32Kmt ) result += "OpaqueWin32Kmt | ";
    if ( value & ExternalFenceHandleTypeFlagBits::eSyncFd ) result += "SyncFd | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( ExternalFenceFeatureFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & ExternalFenceFeatureFlagBits::eExportable ) result += "Exportable | ";
    if ( value & ExternalFenceFeatureFlagBits::eImportable ) result += "Importable | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( FenceImportFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & FenceImportFlagBits::eTemporary ) result += "Temporary | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( SemaphoreImportFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & SemaphoreImportFlagBits::eTemporary ) result += "Temporary | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( ExternalSemaphoreHandleTypeFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd ) result += "OpaqueFd | ";
    if ( value & ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32 ) result += "OpaqueWin32 | ";
    if ( value & ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32Kmt ) result += "OpaqueWin32Kmt | ";
    if ( value & ExternalSemaphoreHandleTypeFlagBits::eD3D12Fence ) result += "D3D12Fence | ";
    if ( value & ExternalSemaphoreHandleTypeFlagBits::eSyncFd ) result += "SyncFd | ";
#if defined( VK_USE_PLATFORM_FUCHSIA )
    if ( value & ExternalSemaphoreHandleTypeFlagBits::eZirconEventFUCHSIA ) result += "ZirconEventFUCHSIA | ";
#endif /*VK_USE_PLATFORM_FUCHSIA*/

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( ExternalSemaphoreFeatureFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & ExternalSemaphoreFeatureFlagBits::eExportable ) result += "Exportable | ";
    if ( value & ExternalSemaphoreFeatureFlagBits::eImportable ) result += "Importable | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  //=== VK_VERSION_1_2 ===

  VULKAN_HPP_INLINE std::string to_string( DescriptorBindingFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & DescriptorBindingFlagBits::eUpdateAfterBind ) result += "UpdateAfterBind | ";
    if ( value & DescriptorBindingFlagBits::eUpdateUnusedWhilePending ) result += "UpdateUnusedWhilePending | ";
    if ( value & DescriptorBindingFlagBits::ePartiallyBound ) result += "PartiallyBound | ";
    if ( value & DescriptorBindingFlagBits::eVariableDescriptorCount ) result += "VariableDescriptorCount | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( ResolveModeFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & ResolveModeFlagBits::eSampleZero ) result += "SampleZero | ";
    if ( value & ResolveModeFlagBits::eAverage ) result += "Average | ";
    if ( value & ResolveModeFlagBits::eMin ) result += "Min | ";
    if ( value & ResolveModeFlagBits::eMax ) result += "Max | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( SemaphoreWaitFlags value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & SemaphoreWaitFlagBits::eAny ) result += "Any | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  //=== VK_KHR_surface ===

  VULKAN_HPP_INLINE std::string to_string( CompositeAlphaFlagsKHR value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & CompositeAlphaFlagBitsKHR::eOpaque ) result += "Opaque | ";
    if ( value & CompositeAlphaFlagBitsKHR::ePreMultiplied ) result += "PreMultiplied | ";
    if ( value & CompositeAlphaFlagBitsKHR::ePostMultiplied ) result += "PostMultiplied | ";
    if ( value & CompositeAlphaFlagBitsKHR::eInherit ) result += "Inherit | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  //=== VK_KHR_swapchain ===

  VULKAN_HPP_INLINE std::string to_string( SwapchainCreateFlagsKHR value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & SwapchainCreateFlagBitsKHR::eSplitInstanceBindRegions ) result += "SplitInstanceBindRegions | ";
    if ( value & SwapchainCreateFlagBitsKHR::eProtected ) result += "Protected | ";
    if ( value & SwapchainCreateFlagBitsKHR::eMutableFormat ) result += "MutableFormat | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( DeviceGroupPresentModeFlagsKHR value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & DeviceGroupPresentModeFlagBitsKHR::eLocal ) result += "Local | ";
    if ( value & DeviceGroupPresentModeFlagBitsKHR::eRemote ) result += "Remote | ";
    if ( value & DeviceGroupPresentModeFlagBitsKHR::eSum ) result += "Sum | ";
    if ( value & DeviceGroupPresentModeFlagBitsKHR::eLocalMultiDevice ) result += "LocalMultiDevice | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  //=== VK_KHR_display ===

  VULKAN_HPP_INLINE std::string to_string( DisplayModeCreateFlagsKHR )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( DisplayPlaneAlphaFlagsKHR value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & DisplayPlaneAlphaFlagBitsKHR::eOpaque ) result += "Opaque | ";
    if ( value & DisplayPlaneAlphaFlagBitsKHR::eGlobal ) result += "Global | ";
    if ( value & DisplayPlaneAlphaFlagBitsKHR::ePerPixel ) result += "PerPixel | ";
    if ( value & DisplayPlaneAlphaFlagBitsKHR::ePerPixelPremultiplied ) result += "PerPixelPremultiplied | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( DisplaySurfaceCreateFlagsKHR )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( SurfaceTransformFlagsKHR value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & SurfaceTransformFlagBitsKHR::eIdentity ) result += "Identity | ";
    if ( value & SurfaceTransformFlagBitsKHR::eRotate90 ) result += "Rotate90 | ";
    if ( value & SurfaceTransformFlagBitsKHR::eRotate180 ) result += "Rotate180 | ";
    if ( value & SurfaceTransformFlagBitsKHR::eRotate270 ) result += "Rotate270 | ";
    if ( value & SurfaceTransformFlagBitsKHR::eHorizontalMirror ) result += "HorizontalMirror | ";
    if ( value & SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate90 ) result += "HorizontalMirrorRotate90 | ";
    if ( value & SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate180 ) result += "HorizontalMirrorRotate180 | ";
    if ( value & SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate270 ) result += "HorizontalMirrorRotate270 | ";
    if ( value & SurfaceTransformFlagBitsKHR::eInherit ) result += "Inherit | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

#if defined( VK_USE_PLATFORM_XLIB_KHR )
  //=== VK_KHR_xlib_surface ===

  VULKAN_HPP_INLINE std::string to_string( XlibSurfaceCreateFlagsKHR )
  {
    return "{}";
  }
#endif /*VK_USE_PLATFORM_XLIB_KHR*/

#if defined( VK_USE_PLATFORM_XCB_KHR )
  //=== VK_KHR_xcb_surface ===

  VULKAN_HPP_INLINE std::string to_string( XcbSurfaceCreateFlagsKHR )
  {
    return "{}";
  }
#endif /*VK_USE_PLATFORM_XCB_KHR*/

#if defined( VK_USE_PLATFORM_WAYLAND_KHR )
  //=== VK_KHR_wayland_surface ===

  VULKAN_HPP_INLINE std::string to_string( WaylandSurfaceCreateFlagsKHR )
  {
    return "{}";
  }
#endif /*VK_USE_PLATFORM_WAYLAND_KHR*/

#if defined( VK_USE_PLATFORM_ANDROID_KHR )
  //=== VK_KHR_android_surface ===

  VULKAN_HPP_INLINE std::string to_string( AndroidSurfaceCreateFlagsKHR )
  {
    return "{}";
  }
#endif /*VK_USE_PLATFORM_ANDROID_KHR*/

#if defined( VK_USE_PLATFORM_WIN32_KHR )
  //=== VK_KHR_win32_surface ===

  VULKAN_HPP_INLINE std::string to_string( Win32SurfaceCreateFlagsKHR )
  {
    return "{}";
  }
#endif /*VK_USE_PLATFORM_WIN32_KHR*/

  //=== VK_EXT_debug_report ===

  VULKAN_HPP_INLINE std::string to_string( DebugReportFlagsEXT value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & DebugReportFlagBitsEXT::eInformation ) result += "Information | ";
    if ( value & DebugReportFlagBitsEXT::eWarning ) result += "Warning | ";
    if ( value & DebugReportFlagBitsEXT::ePerformanceWarning ) result += "PerformanceWarning | ";
    if ( value & DebugReportFlagBitsEXT::eError ) result += "Error | ";
    if ( value & DebugReportFlagBitsEXT::eDebug ) result += "Debug | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

#if defined( VK_ENABLE_BETA_EXTENSIONS )
  //=== VK_KHR_video_queue ===

  VULKAN_HPP_INLINE std::string to_string( VideoCodecOperationFlagsKHR value )
  {
    if ( !value )
      return "{}";

    std::string result;
#if defined( VK_ENABLE_BETA_EXTENSIONS )
    if ( value & VideoCodecOperationFlagBitsKHR::eEncodeH264EXT ) result += "EncodeH264EXT | ";
    if ( value & VideoCodecOperationFlagBitsKHR::eEncodeH265EXT ) result += "EncodeH265EXT | ";
    if ( value & VideoCodecOperationFlagBitsKHR::eDecodeH264EXT ) result += "DecodeH264EXT | ";
    if ( value & VideoCodecOperationFlagBitsKHR::eDecodeH265EXT ) result += "DecodeH265EXT | ";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( VideoChromaSubsamplingFlagsKHR value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & VideoChromaSubsamplingFlagBitsKHR::eMonochrome ) result += "Monochrome | ";
    if ( value & VideoChromaSubsamplingFlagBitsKHR::e420 ) result += "420 | ";
    if ( value & VideoChromaSubsamplingFlagBitsKHR::e422 ) result += "422 | ";
    if ( value & VideoChromaSubsamplingFlagBitsKHR::e444 ) result += "444 | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( VideoComponentBitDepthFlagsKHR value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & VideoComponentBitDepthFlagBitsKHR::e8 ) result += "8 | ";
    if ( value & VideoComponentBitDepthFlagBitsKHR::e10 ) result += "10 | ";
    if ( value & VideoComponentBitDepthFlagBitsKHR::e12 ) result += "12 | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( VideoCapabilityFlagsKHR value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & VideoCapabilityFlagBitsKHR::eProtectedContent ) result += "ProtectedContent | ";
    if ( value & VideoCapabilityFlagBitsKHR::eSeparateReferenceImages ) result += "SeparateReferenceImages | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( VideoSessionCreateFlagsKHR value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & VideoSessionCreateFlagBitsKHR::eProtectedContent ) result += "ProtectedContent | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( VideoBeginCodingFlagsKHR )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( VideoEndCodingFlagsKHR )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( VideoCodingControlFlagsKHR value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & VideoCodingControlFlagBitsKHR::eReset ) result += "Reset | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( VideoCodingQualityPresetFlagsKHR value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & VideoCodingQualityPresetFlagBitsKHR::eNormal ) result += "Normal | ";
    if ( value & VideoCodingQualityPresetFlagBitsKHR::ePower ) result += "Power | ";
    if ( value & VideoCodingQualityPresetFlagBitsKHR::eQuality ) result += "Quality | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }
#endif /*VK_ENABLE_BETA_EXTENSIONS*/

#if defined( VK_ENABLE_BETA_EXTENSIONS )
  //=== VK_KHR_video_decode_queue ===

  VULKAN_HPP_INLINE std::string to_string( VideoDecodeFlagsKHR value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & VideoDecodeFlagBitsKHR::eReserved0 ) result += "Reserved0 | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }
#endif /*VK_ENABLE_BETA_EXTENSIONS*/

  //=== VK_EXT_transform_feedback ===

  VULKAN_HPP_INLINE std::string to_string( PipelineRasterizationStateStreamCreateFlagsEXT )
  {
    return "{}";
  }

#if defined( VK_ENABLE_BETA_EXTENSIONS )
  //=== VK_EXT_video_encode_h264 ===

  VULKAN_HPP_INLINE std::string to_string( VideoEncodeH264CapabilityFlagsEXT value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & VideoEncodeH264CapabilityFlagBitsEXT::eCabac ) result += "Cabac | ";
    if ( value & VideoEncodeH264CapabilityFlagBitsEXT::eCavlc ) result += "Cavlc | ";
    if ( value & VideoEncodeH264CapabilityFlagBitsEXT::eWeightedBiPredImplicit ) result += "WeightedBiPredImplicit | ";
    if ( value & VideoEncodeH264CapabilityFlagBitsEXT::eTransform8X8 ) result += "Transform8X8 | ";
    if ( value & VideoEncodeH264CapabilityFlagBitsEXT::eChromaQpOffset ) result += "ChromaQpOffset | ";
    if ( value & VideoEncodeH264CapabilityFlagBitsEXT::eSecondChromaQpOffset ) result += "SecondChromaQpOffset | ";
    if ( value & VideoEncodeH264CapabilityFlagBitsEXT::eDeblockingFilterDisabled ) result += "DeblockingFilterDisabled | ";
    if ( value & VideoEncodeH264CapabilityFlagBitsEXT::eDeblockingFilterEnabled ) result += "DeblockingFilterEnabled | ";
    if ( value & VideoEncodeH264CapabilityFlagBitsEXT::eDeblockingFilterPartial ) result += "DeblockingFilterPartial | ";
    if ( value & VideoEncodeH264CapabilityFlagBitsEXT::eMultipleSlicePerFrame ) result += "MultipleSlicePerFrame | ";
    if ( value & VideoEncodeH264CapabilityFlagBitsEXT::eEvenlyDistributedSliceSize ) result += "EvenlyDistributedSliceSize | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( VideoEncodeH264InputModeFlagsEXT value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & VideoEncodeH264InputModeFlagBitsEXT::eFrame ) result += "Frame | ";
    if ( value & VideoEncodeH264InputModeFlagBitsEXT::eSlice ) result += "Slice | ";
    if ( value & VideoEncodeH264InputModeFlagBitsEXT::eNonVcl ) result += "NonVcl | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( VideoEncodeH264OutputModeFlagsEXT value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & VideoEncodeH264OutputModeFlagBitsEXT::eFrame ) result += "Frame | ";
    if ( value & VideoEncodeH264OutputModeFlagBitsEXT::eSlice ) result += "Slice | ";
    if ( value & VideoEncodeH264OutputModeFlagBitsEXT::eNonVcl ) result += "NonVcl | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( VideoEncodeH264CreateFlagsEXT value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & VideoEncodeH264CreateFlagBitsEXT::eReserved0 ) result += "Reserved0 | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }
#endif /*VK_ENABLE_BETA_EXTENSIONS*/

#if defined( VK_ENABLE_BETA_EXTENSIONS )
  //=== VK_EXT_video_encode_h265 ===

  VULKAN_HPP_INLINE std::string to_string( VideoEncodeH265CapabilityFlagsEXT )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( VideoEncodeH265InputModeFlagsEXT value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & VideoEncodeH265InputModeFlagBitsEXT::eFrame ) result += "Frame | ";
    if ( value & VideoEncodeH265InputModeFlagBitsEXT::eSlice ) result += "Slice | ";
    if ( value & VideoEncodeH265InputModeFlagBitsEXT::eNonVcl ) result += "NonVcl | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( VideoEncodeH265OutputModeFlagsEXT value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & VideoEncodeH265OutputModeFlagBitsEXT::eFrame ) result += "Frame | ";
    if ( value & VideoEncodeH265OutputModeFlagBitsEXT::eSlice ) result += "Slice | ";
    if ( value & VideoEncodeH265OutputModeFlagBitsEXT::eNonVcl ) result += "NonVcl | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( VideoEncodeH265CreateFlagsEXT )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( VideoEncodeH265CtbSizeFlagsEXT value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & VideoEncodeH265CtbSizeFlagBitsEXT::e8 ) result += "8 | ";
    if ( value & VideoEncodeH265CtbSizeFlagBitsEXT::e16 ) result += "16 | ";
    if ( value & VideoEncodeH265CtbSizeFlagBitsEXT::e32 ) result += "32 | ";
    if ( value & VideoEncodeH265CtbSizeFlagBitsEXT::e64 ) result += "64 | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }
#endif /*VK_ENABLE_BETA_EXTENSIONS*/

#if defined( VK_ENABLE_BETA_EXTENSIONS )
  //=== VK_EXT_video_decode_h264 ===

  VULKAN_HPP_INLINE std::string to_string( VideoDecodeH264PictureLayoutFlagsEXT value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & VideoDecodeH264PictureLayoutFlagBitsEXT::eInterlacedInterleavedLines ) result += "InterlacedInterleavedLines | ";
    if ( value & VideoDecodeH264PictureLayoutFlagBitsEXT::eInterlacedSeparatePlanes ) result += "InterlacedSeparatePlanes | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( VideoDecodeH264CreateFlagsEXT )
  {
    return "{}";
  }
#endif /*VK_ENABLE_BETA_EXTENSIONS*/

  //=== VK_KHR_dynamic_rendering ===

  VULKAN_HPP_INLINE std::string to_string( RenderingFlagsKHR value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & RenderingFlagBitsKHR::eContentsSecondaryCommandBuffers ) result += "ContentsSecondaryCommandBuffers | ";
    if ( value & RenderingFlagBitsKHR::eSuspending ) result += "Suspending | ";
    if ( value & RenderingFlagBitsKHR::eResuming ) result += "Resuming | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

#if defined( VK_USE_PLATFORM_GGP )
  //=== VK_GGP_stream_descriptor_surface ===

  VULKAN_HPP_INLINE std::string to_string( StreamDescriptorSurfaceCreateFlagsGGP )
  {
    return "{}";
  }
#endif /*VK_USE_PLATFORM_GGP*/

  //=== VK_NV_external_memory_capabilities ===

  VULKAN_HPP_INLINE std::string to_string( ExternalMemoryHandleTypeFlagsNV value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & ExternalMemoryHandleTypeFlagBitsNV::eOpaqueWin32 ) result += "OpaqueWin32 | ";
    if ( value & ExternalMemoryHandleTypeFlagBitsNV::eOpaqueWin32Kmt ) result += "OpaqueWin32Kmt | ";
    if ( value & ExternalMemoryHandleTypeFlagBitsNV::eD3D11Image ) result += "D3D11Image | ";
    if ( value & ExternalMemoryHandleTypeFlagBitsNV::eD3D11ImageKmt ) result += "D3D11ImageKmt | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( ExternalMemoryFeatureFlagsNV value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & ExternalMemoryFeatureFlagBitsNV::eDedicatedOnly ) result += "DedicatedOnly | ";
    if ( value & ExternalMemoryFeatureFlagBitsNV::eExportable ) result += "Exportable | ";
    if ( value & ExternalMemoryFeatureFlagBitsNV::eImportable ) result += "Importable | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

#if defined( VK_USE_PLATFORM_VI_NN )
  //=== VK_NN_vi_surface ===

  VULKAN_HPP_INLINE std::string to_string( ViSurfaceCreateFlagsNN )
  {
    return "{}";
  }
#endif /*VK_USE_PLATFORM_VI_NN*/

  //=== VK_EXT_conditional_rendering ===

  VULKAN_HPP_INLINE std::string to_string( ConditionalRenderingFlagsEXT value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & ConditionalRenderingFlagBitsEXT::eInverted ) result += "Inverted | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  //=== VK_EXT_display_surface_counter ===

  VULKAN_HPP_INLINE std::string to_string( SurfaceCounterFlagsEXT value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & SurfaceCounterFlagBitsEXT::eVblank ) result += "Vblank | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  //=== VK_NV_viewport_swizzle ===

  VULKAN_HPP_INLINE std::string to_string( PipelineViewportSwizzleStateCreateFlagsNV )
  {
    return "{}";
  }

  //=== VK_EXT_discard_rectangles ===

  VULKAN_HPP_INLINE std::string to_string( PipelineDiscardRectangleStateCreateFlagsEXT )
  {
    return "{}";
  }

  //=== VK_EXT_conservative_rasterization ===

  VULKAN_HPP_INLINE std::string to_string( PipelineRasterizationConservativeStateCreateFlagsEXT )
  {
    return "{}";
  }

  //=== VK_EXT_depth_clip_enable ===

  VULKAN_HPP_INLINE std::string to_string( PipelineRasterizationDepthClipStateCreateFlagsEXT )
  {
    return "{}";
  }

  //=== VK_KHR_performance_query ===

  VULKAN_HPP_INLINE std::string to_string( PerformanceCounterDescriptionFlagsKHR value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & PerformanceCounterDescriptionFlagBitsKHR::ePerformanceImpacting ) result += "PerformanceImpacting | ";
    if ( value & PerformanceCounterDescriptionFlagBitsKHR::eConcurrentlyImpacted ) result += "ConcurrentlyImpacted | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( AcquireProfilingLockFlagsKHR )
  {
    return "{}";
  }

#if defined( VK_USE_PLATFORM_IOS_MVK )
  //=== VK_MVK_ios_surface ===

  VULKAN_HPP_INLINE std::string to_string( IOSSurfaceCreateFlagsMVK )
  {
    return "{}";
  }
#endif /*VK_USE_PLATFORM_IOS_MVK*/

#if defined( VK_USE_PLATFORM_MACOS_MVK )
  //=== VK_MVK_macos_surface ===

  VULKAN_HPP_INLINE std::string to_string( MacOSSurfaceCreateFlagsMVK )
  {
    return "{}";
  }
#endif /*VK_USE_PLATFORM_MACOS_MVK*/

  //=== VK_EXT_debug_utils ===

  VULKAN_HPP_INLINE std::string to_string( DebugUtilsMessageSeverityFlagsEXT value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & DebugUtilsMessageSeverityFlagBitsEXT::eVerbose ) result += "Verbose | ";
    if ( value & DebugUtilsMessageSeverityFlagBitsEXT::eInfo ) result += "Info | ";
    if ( value & DebugUtilsMessageSeverityFlagBitsEXT::eWarning ) result += "Warning | ";
    if ( value & DebugUtilsMessageSeverityFlagBitsEXT::eError ) result += "Error | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( DebugUtilsMessageTypeFlagsEXT value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & DebugUtilsMessageTypeFlagBitsEXT::eGeneral ) result += "General | ";
    if ( value & DebugUtilsMessageTypeFlagBitsEXT::eValidation ) result += "Validation | ";
    if ( value & DebugUtilsMessageTypeFlagBitsEXT::ePerformance ) result += "Performance | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( DebugUtilsMessengerCallbackDataFlagsEXT )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( DebugUtilsMessengerCreateFlagsEXT )
  {
    return "{}";
  }

  //=== VK_NV_fragment_coverage_to_color ===

  VULKAN_HPP_INLINE std::string to_string( PipelineCoverageToColorStateCreateFlagsNV )
  {
    return "{}";
  }

  //=== VK_KHR_acceleration_structure ===

  VULKAN_HPP_INLINE std::string to_string( GeometryFlagsKHR value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & GeometryFlagBitsKHR::eOpaque ) result += "Opaque | ";
    if ( value & GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation ) result += "NoDuplicateAnyHitInvocation | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( GeometryInstanceFlagsKHR value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable ) result += "TriangleFacingCullDisable | ";
    if ( value & GeometryInstanceFlagBitsKHR::eTriangleFlipFacing ) result += "TriangleFlipFacing | ";
    if ( value & GeometryInstanceFlagBitsKHR::eForceOpaque ) result += "ForceOpaque | ";
    if ( value & GeometryInstanceFlagBitsKHR::eForceNoOpaque ) result += "ForceNoOpaque | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( BuildAccelerationStructureFlagsKHR value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & BuildAccelerationStructureFlagBitsKHR::eAllowUpdate ) result += "AllowUpdate | ";
    if ( value & BuildAccelerationStructureFlagBitsKHR::eAllowCompaction ) result += "AllowCompaction | ";
    if ( value & BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace ) result += "PreferFastTrace | ";
    if ( value & BuildAccelerationStructureFlagBitsKHR::ePreferFastBuild ) result += "PreferFastBuild | ";
    if ( value & BuildAccelerationStructureFlagBitsKHR::eLowMemory ) result += "LowMemory | ";
    if ( value & BuildAccelerationStructureFlagBitsKHR::eMotionNV ) result += "MotionNV | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( AccelerationStructureCreateFlagsKHR value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & AccelerationStructureCreateFlagBitsKHR::eDeviceAddressCaptureReplay ) result += "DeviceAddressCaptureReplay | ";
    if ( value & AccelerationStructureCreateFlagBitsKHR::eMotionNV ) result += "MotionNV | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  //=== VK_NV_framebuffer_mixed_samples ===

  VULKAN_HPP_INLINE std::string to_string( PipelineCoverageModulationStateCreateFlagsNV )
  {
    return "{}";
  }

  //=== VK_EXT_validation_cache ===

  VULKAN_HPP_INLINE std::string to_string( ValidationCacheCreateFlagsEXT )
  {
    return "{}";
  }

  //=== VK_AMD_pipeline_compiler_control ===

  VULKAN_HPP_INLINE std::string to_string( PipelineCompilerControlFlagsAMD )
  {
    return "{}";
  }

#if defined( VK_ENABLE_BETA_EXTENSIONS )
  //=== VK_EXT_video_decode_h265 ===

  VULKAN_HPP_INLINE std::string to_string( VideoDecodeH265CreateFlagsEXT )
  {
    return "{}";
  }
#endif /*VK_ENABLE_BETA_EXTENSIONS*/

  //=== VK_EXT_pipeline_creation_feedback ===

  VULKAN_HPP_INLINE std::string to_string( PipelineCreationFeedbackFlagsEXT value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & PipelineCreationFeedbackFlagBitsEXT::eValid ) result += "Valid | ";
    if ( value & PipelineCreationFeedbackFlagBitsEXT::eApplicationPipelineCacheHit ) result += "ApplicationPipelineCacheHit | ";
    if ( value & PipelineCreationFeedbackFlagBitsEXT::eBasePipelineAcceleration ) result += "BasePipelineAcceleration | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

#if defined( VK_USE_PLATFORM_FUCHSIA )
  //=== VK_FUCHSIA_imagepipe_surface ===

  VULKAN_HPP_INLINE std::string to_string( ImagePipeSurfaceCreateFlagsFUCHSIA )
  {
    return "{}";
  }
#endif /*VK_USE_PLATFORM_FUCHSIA*/

#if defined( VK_USE_PLATFORM_METAL_EXT )
  //=== VK_EXT_metal_surface ===

  VULKAN_HPP_INLINE std::string to_string( MetalSurfaceCreateFlagsEXT )
  {
    return "{}";
  }
#endif /*VK_USE_PLATFORM_METAL_EXT*/

  //=== VK_AMD_shader_core_properties2 ===

  VULKAN_HPP_INLINE std::string to_string( ShaderCorePropertiesFlagsAMD )
  {
    return "{}";
  }

  //=== VK_EXT_tooling_info ===

  VULKAN_HPP_INLINE std::string to_string( ToolPurposeFlagsEXT value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & ToolPurposeFlagBitsEXT::eValidation ) result += "Validation | ";
    if ( value & ToolPurposeFlagBitsEXT::eProfiling ) result += "Profiling | ";
    if ( value & ToolPurposeFlagBitsEXT::eTracing ) result += "Tracing | ";
    if ( value & ToolPurposeFlagBitsEXT::eAdditionalFeatures ) result += "AdditionalFeatures | ";
    if ( value & ToolPurposeFlagBitsEXT::eModifyingFeatures ) result += "ModifyingFeatures | ";
    if ( value & ToolPurposeFlagBitsEXT::eDebugReporting ) result += "DebugReporting | ";
    if ( value & ToolPurposeFlagBitsEXT::eDebugMarkers ) result += "DebugMarkers | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  //=== VK_NV_coverage_reduction_mode ===

  VULKAN_HPP_INLINE std::string to_string( PipelineCoverageReductionStateCreateFlagsNV )
  {
    return "{}";
  }

  //=== VK_EXT_headless_surface ===

  VULKAN_HPP_INLINE std::string to_string( HeadlessSurfaceCreateFlagsEXT )
  {
    return "{}";
  }

  //=== VK_NV_device_generated_commands ===

  VULKAN_HPP_INLINE std::string to_string( IndirectStateFlagsNV value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & IndirectStateFlagBitsNV::eFlagFrontface ) result += "FlagFrontface | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( IndirectCommandsLayoutUsageFlagsNV value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & IndirectCommandsLayoutUsageFlagBitsNV::eExplicitPreprocess ) result += "ExplicitPreprocess | ";
    if ( value & IndirectCommandsLayoutUsageFlagBitsNV::eIndexedSequences ) result += "IndexedSequences | ";
    if ( value & IndirectCommandsLayoutUsageFlagBitsNV::eUnorderedSequences ) result += "UnorderedSequences | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  //=== VK_EXT_device_memory_report ===

  VULKAN_HPP_INLINE std::string to_string( DeviceMemoryReportFlagsEXT )
  {
    return "{}";
  }

  //=== VK_EXT_private_data ===

  VULKAN_HPP_INLINE std::string to_string( PrivateDataSlotCreateFlagsEXT )
  {
    return "{}";
  }

#if defined( VK_ENABLE_BETA_EXTENSIONS )
  //=== VK_KHR_video_encode_queue ===

  VULKAN_HPP_INLINE std::string to_string( VideoEncodeFlagsKHR value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & VideoEncodeFlagBitsKHR::eReserved0 ) result += "Reserved0 | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( VideoEncodeRateControlFlagsKHR value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & VideoEncodeRateControlFlagBitsKHR::eReset ) result += "Reset | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( VideoEncodeRateControlModeFlagsKHR value )
  {
    if ( !value )
      return "{}";

    std::string result;

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }
#endif /*VK_ENABLE_BETA_EXTENSIONS*/

  //=== VK_NV_device_diagnostics_config ===

  VULKAN_HPP_INLINE std::string to_string( DeviceDiagnosticsConfigFlagsNV value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & DeviceDiagnosticsConfigFlagBitsNV::eEnableShaderDebugInfo ) result += "EnableShaderDebugInfo | ";
    if ( value & DeviceDiagnosticsConfigFlagBitsNV::eEnableResourceTracking ) result += "EnableResourceTracking | ";
    if ( value & DeviceDiagnosticsConfigFlagBitsNV::eEnableAutomaticCheckpoints ) result += "EnableAutomaticCheckpoints | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  //=== VK_KHR_synchronization2 ===

  VULKAN_HPP_INLINE std::string to_string( PipelineStageFlags2KHR value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & PipelineStageFlagBits2KHR::eTopOfPipe ) result += "TopOfPipe | ";
    if ( value & PipelineStageFlagBits2KHR::eDrawIndirect ) result += "DrawIndirect | ";
    if ( value & PipelineStageFlagBits2KHR::eVertexInput ) result += "VertexInput | ";
    if ( value & PipelineStageFlagBits2KHR::eVertexShader ) result += "VertexShader | ";
    if ( value & PipelineStageFlagBits2KHR::eTessellationControlShader ) result += "TessellationControlShader | ";
    if ( value & PipelineStageFlagBits2KHR::eTessellationEvaluationShader ) result += "TessellationEvaluationShader | ";
    if ( value & PipelineStageFlagBits2KHR::eGeometryShader ) result += "GeometryShader | ";
    if ( value & PipelineStageFlagBits2KHR::eFragmentShader ) result += "FragmentShader | ";
    if ( value & PipelineStageFlagBits2KHR::eEarlyFragmentTests ) result += "EarlyFragmentTests | ";
    if ( value & PipelineStageFlagBits2KHR::eLateFragmentTests ) result += "LateFragmentTests | ";
    if ( value & PipelineStageFlagBits2KHR::eColorAttachmentOutput ) result += "ColorAttachmentOutput | ";
    if ( value & PipelineStageFlagBits2KHR::eComputeShader ) result += "ComputeShader | ";
    if ( value & PipelineStageFlagBits2KHR::eAllTransfer ) result += "AllTransfer | ";
    if ( value & PipelineStageFlagBits2KHR::eBottomOfPipe ) result += "BottomOfPipe | ";
    if ( value & PipelineStageFlagBits2KHR::eHost ) result += "Host | ";
    if ( value & PipelineStageFlagBits2KHR::eAllGraphics ) result += "AllGraphics | ";
    if ( value & PipelineStageFlagBits2KHR::eAllCommands ) result += "AllCommands | ";
    if ( value & PipelineStageFlagBits2KHR::eCopy ) result += "Copy | ";
    if ( value & PipelineStageFlagBits2KHR::eResolve ) result += "Resolve | ";
    if ( value & PipelineStageFlagBits2KHR::eBlit ) result += "Blit | ";
    if ( value & PipelineStageFlagBits2KHR::eClear ) result += "Clear | ";
    if ( value & PipelineStageFlagBits2KHR::eIndexInput ) result += "IndexInput | ";
    if ( value & PipelineStageFlagBits2KHR::eVertexAttributeInput ) result += "VertexAttributeInput | ";
    if ( value & PipelineStageFlagBits2KHR::ePreRasterizationShaders ) result += "PreRasterizationShaders | ";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
    if ( value & PipelineStageFlagBits2KHR::eVideoDecode ) result += "VideoDecode | ";
    if ( value & PipelineStageFlagBits2KHR::eVideoEncode ) result += "VideoEncode | ";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
    if ( value & PipelineStageFlagBits2KHR::eTransformFeedbackEXT ) result += "TransformFeedbackEXT | ";
    if ( value & PipelineStageFlagBits2KHR::eConditionalRenderingEXT ) result += "ConditionalRenderingEXT | ";
    if ( value & PipelineStageFlagBits2KHR::eCommandPreprocessNV ) result += "CommandPreprocessNV | ";
    if ( value & PipelineStageFlagBits2KHR::eFragmentShadingRateAttachment ) result += "FragmentShadingRateAttachment | ";
    if ( value & PipelineStageFlagBits2KHR::eAccelerationStructureBuild ) result += "AccelerationStructureBuild | ";
    if ( value & PipelineStageFlagBits2KHR::eRayTracingShader ) result += "RayTracingShader | ";
    if ( value & PipelineStageFlagBits2KHR::eFragmentDensityProcessEXT ) result += "FragmentDensityProcessEXT | ";
    if ( value & PipelineStageFlagBits2KHR::eTaskShaderNV ) result += "TaskShaderNV | ";
    if ( value & PipelineStageFlagBits2KHR::eMeshShaderNV ) result += "MeshShaderNV | ";
    if ( value & PipelineStageFlagBits2KHR::eSubpassShadingHUAWEI ) result += "SubpassShadingHUAWEI | ";
    if ( value & PipelineStageFlagBits2KHR::eInvocationMaskHUAWEI ) result += "InvocationMaskHUAWEI | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( AccessFlags2KHR value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & AccessFlagBits2KHR::eIndirectCommandRead ) result += "IndirectCommandRead | ";
    if ( value & AccessFlagBits2KHR::eIndexRead ) result += "IndexRead | ";
    if ( value & AccessFlagBits2KHR::eVertexAttributeRead ) result += "VertexAttributeRead | ";
    if ( value & AccessFlagBits2KHR::eUniformRead ) result += "UniformRead | ";
    if ( value & AccessFlagBits2KHR::eInputAttachmentRead ) result += "InputAttachmentRead | ";
    if ( value & AccessFlagBits2KHR::eShaderRead ) result += "ShaderRead | ";
    if ( value & AccessFlagBits2KHR::eShaderWrite ) result += "ShaderWrite | ";
    if ( value & AccessFlagBits2KHR::eColorAttachmentRead ) result += "ColorAttachmentRead | ";
    if ( value & AccessFlagBits2KHR::eColorAttachmentWrite ) result += "ColorAttachmentWrite | ";
    if ( value & AccessFlagBits2KHR::eDepthStencilAttachmentRead ) result += "DepthStencilAttachmentRead | ";
    if ( value & AccessFlagBits2KHR::eDepthStencilAttachmentWrite ) result += "DepthStencilAttachmentWrite | ";
    if ( value & AccessFlagBits2KHR::eTransferRead ) result += "TransferRead | ";
    if ( value & AccessFlagBits2KHR::eTransferWrite ) result += "TransferWrite | ";
    if ( value & AccessFlagBits2KHR::eHostRead ) result += "HostRead | ";
    if ( value & AccessFlagBits2KHR::eHostWrite ) result += "HostWrite | ";
    if ( value & AccessFlagBits2KHR::eMemoryRead ) result += "MemoryRead | ";
    if ( value & AccessFlagBits2KHR::eMemoryWrite ) result += "MemoryWrite | ";
    if ( value & AccessFlagBits2KHR::eShaderSampledRead ) result += "ShaderSampledRead | ";
    if ( value & AccessFlagBits2KHR::eShaderStorageRead ) result += "ShaderStorageRead | ";
    if ( value & AccessFlagBits2KHR::eShaderStorageWrite ) result += "ShaderStorageWrite | ";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
    if ( value & AccessFlagBits2KHR::eVideoDecodeRead ) result += "VideoDecodeRead | ";
    if ( value & AccessFlagBits2KHR::eVideoDecodeWrite ) result += "VideoDecodeWrite | ";
    if ( value & AccessFlagBits2KHR::eVideoEncodeRead ) result += "VideoEncodeRead | ";
    if ( value & AccessFlagBits2KHR::eVideoEncodeWrite ) result += "VideoEncodeWrite | ";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
    if ( value & AccessFlagBits2KHR::eTransformFeedbackWriteEXT ) result += "TransformFeedbackWriteEXT | ";
    if ( value & AccessFlagBits2KHR::eTransformFeedbackCounterReadEXT ) result += "TransformFeedbackCounterReadEXT | ";
    if ( value & AccessFlagBits2KHR::eTransformFeedbackCounterWriteEXT ) result += "TransformFeedbackCounterWriteEXT | ";
    if ( value & AccessFlagBits2KHR::eConditionalRenderingReadEXT ) result += "ConditionalRenderingReadEXT | ";
    if ( value & AccessFlagBits2KHR::eCommandPreprocessReadNV ) result += "CommandPreprocessReadNV | ";
    if ( value & AccessFlagBits2KHR::eCommandPreprocessWriteNV ) result += "CommandPreprocessWriteNV | ";
    if ( value & AccessFlagBits2KHR::eFragmentShadingRateAttachmentRead ) result += "FragmentShadingRateAttachmentRead | ";
    if ( value & AccessFlagBits2KHR::eAccelerationStructureRead ) result += "AccelerationStructureRead | ";
    if ( value & AccessFlagBits2KHR::eAccelerationStructureWrite ) result += "AccelerationStructureWrite | ";
    if ( value & AccessFlagBits2KHR::eFragmentDensityMapReadEXT ) result += "FragmentDensityMapReadEXT | ";
    if ( value & AccessFlagBits2KHR::eColorAttachmentReadNoncoherentEXT ) result += "ColorAttachmentReadNoncoherentEXT | ";
    if ( value & AccessFlagBits2KHR::eInvocationMaskReadHUAWEI ) result += "InvocationMaskReadHUAWEI | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  VULKAN_HPP_INLINE std::string to_string( SubmitFlagsKHR value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & SubmitFlagBitsKHR::eProtected ) result += "Protected | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

  //=== VK_NV_ray_tracing_motion_blur ===

  VULKAN_HPP_INLINE std::string to_string( AccelerationStructureMotionInfoFlagsNV )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( AccelerationStructureMotionInstanceFlagsNV )
  {
    return "{}";
  }

#if defined( VK_USE_PLATFORM_DIRECTFB_EXT )
  //=== VK_EXT_directfb_surface ===

  VULKAN_HPP_INLINE std::string to_string( DirectFBSurfaceCreateFlagsEXT )
  {
    return "{}";
  }
#endif /*VK_USE_PLATFORM_DIRECTFB_EXT*/

  //=== VK_KHR_format_feature_flags2 ===

  VULKAN_HPP_INLINE std::string to_string( FormatFeatureFlags2KHR value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & FormatFeatureFlagBits2KHR::eSampledImage ) result += "SampledImage | ";
    if ( value & FormatFeatureFlagBits2KHR::eStorageImage ) result += "StorageImage | ";
    if ( value & FormatFeatureFlagBits2KHR::eStorageImageAtomic ) result += "StorageImageAtomic | ";
    if ( value & FormatFeatureFlagBits2KHR::eUniformTexelBuffer ) result += "UniformTexelBuffer | ";
    if ( value & FormatFeatureFlagBits2KHR::eStorageTexelBuffer ) result += "StorageTexelBuffer | ";
    if ( value & FormatFeatureFlagBits2KHR::eStorageTexelBufferAtomic ) result += "StorageTexelBufferAtomic | ";
    if ( value & FormatFeatureFlagBits2KHR::eVertexBuffer ) result += "VertexBuffer | ";
    if ( value & FormatFeatureFlagBits2KHR::eColorAttachment ) result += "ColorAttachment | ";
    if ( value & FormatFeatureFlagBits2KHR::eColorAttachmentBlend ) result += "ColorAttachmentBlend | ";
    if ( value & FormatFeatureFlagBits2KHR::eDepthStencilAttachment ) result += "DepthStencilAttachment | ";
    if ( value & FormatFeatureFlagBits2KHR::eBlitSrc ) result += "BlitSrc | ";
    if ( value & FormatFeatureFlagBits2KHR::eBlitDst ) result += "BlitDst | ";
    if ( value & FormatFeatureFlagBits2KHR::eSampledImageFilterLinear ) result += "SampledImageFilterLinear | ";
    if ( value & FormatFeatureFlagBits2KHR::eSampledImageFilterCubicEXT ) result += "SampledImageFilterCubicEXT | ";
    if ( value & FormatFeatureFlagBits2KHR::eTransferSrc ) result += "TransferSrc | ";
    if ( value & FormatFeatureFlagBits2KHR::eTransferDst ) result += "TransferDst | ";
    if ( value & FormatFeatureFlagBits2KHR::eSampledImageFilterMinmax ) result += "SampledImageFilterMinmax | ";
    if ( value & FormatFeatureFlagBits2KHR::eMidpointChromaSamples ) result += "MidpointChromaSamples | ";
    if ( value & FormatFeatureFlagBits2KHR::eSampledImageYcbcrConversionLinearFilter ) result += "SampledImageYcbcrConversionLinearFilter | ";
    if ( value & FormatFeatureFlagBits2KHR::eSampledImageYcbcrConversionSeparateReconstructionFilter ) result += "SampledImageYcbcrConversionSeparateReconstructionFilter | ";
    if ( value & FormatFeatureFlagBits2KHR::eSampledImageYcbcrConversionChromaReconstructionExplicit ) result += "SampledImageYcbcrConversionChromaReconstructionExplicit | ";
    if ( value & FormatFeatureFlagBits2KHR::eSampledImageYcbcrConversionChromaReconstructionExplicitForceable ) result += "SampledImageYcbcrConversionChromaReconstructionExplicitForceable | ";
    if ( value & FormatFeatureFlagBits2KHR::eDisjoint ) result += "Disjoint | ";
    if ( value & FormatFeatureFlagBits2KHR::eCositedChromaSamples ) result += "CositedChromaSamples | ";
    if ( value & FormatFeatureFlagBits2KHR::eStorageReadWithoutFormat ) result += "StorageReadWithoutFormat | ";
    if ( value & FormatFeatureFlagBits2KHR::eStorageWriteWithoutFormat ) result += "StorageWriteWithoutFormat | ";
    if ( value & FormatFeatureFlagBits2KHR::eSampledImageDepthComparison ) result += "SampledImageDepthComparison | ";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
    if ( value & FormatFeatureFlagBits2KHR::eVideoDecodeOutput ) result += "VideoDecodeOutput | ";
    if ( value & FormatFeatureFlagBits2KHR::eVideoDecodeDpb ) result += "VideoDecodeDpb | ";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
    if ( value & FormatFeatureFlagBits2KHR::eAccelerationStructureVertexBuffer ) result += "AccelerationStructureVertexBuffer | ";
    if ( value & FormatFeatureFlagBits2KHR::eFragmentDensityMapEXT ) result += "FragmentDensityMapEXT | ";
    if ( value & FormatFeatureFlagBits2KHR::eFragmentShadingRateAttachment ) result += "FragmentShadingRateAttachment | ";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
    if ( value & FormatFeatureFlagBits2KHR::eVideoEncodeInput ) result += "VideoEncodeInput | ";
    if ( value & FormatFeatureFlagBits2KHR::eVideoEncodeDpb ) result += "VideoEncodeDpb | ";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }

#if defined( VK_USE_PLATFORM_FUCHSIA )
  //=== VK_FUCHSIA_buffer_collection ===

  VULKAN_HPP_INLINE std::string to_string( ImageFormatConstraintsFlagsFUCHSIA )
  {
    return "{}";
  }

  VULKAN_HPP_INLINE std::string to_string( ImageConstraintsInfoFlagsFUCHSIA value )
  {
    if ( !value )
      return "{}";

    std::string result;
    if ( value & ImageConstraintsInfoFlagBitsFUCHSIA::eCpuReadRarely ) result += "CpuReadRarely | ";
    if ( value & ImageConstraintsInfoFlagBitsFUCHSIA::eCpuReadOften ) result += "CpuReadOften | ";
    if ( value & ImageConstraintsInfoFlagBitsFUCHSIA::eCpuWriteRarely ) result += "CpuWriteRarely | ";
    if ( value & ImageConstraintsInfoFlagBitsFUCHSIA::eCpuWriteOften ) result += "CpuWriteOften | ";
    if ( value & ImageConstraintsInfoFlagBitsFUCHSIA::eProtectedOptional ) result += "ProtectedOptional | ";

    return "{ " + result.substr( 0, result.size() - 3 ) + " }";
  }
#endif /*VK_USE_PLATFORM_FUCHSIA*/

#if defined( VK_USE_PLATFORM_SCREEN_QNX )
  //=== VK_QNX_screen_surface ===

  VULKAN_HPP_INLINE std::string to_string( ScreenSurfaceCreateFlagsQNX )
  {
    return "{}";
  }
#endif /*VK_USE_PLATFORM_SCREEN_QNX*/



  //=======================
  //=== ENUMs to_string ===
  //=======================

  VULKAN_HPP_INLINE std::string toHexString( uint32_t value )
  {
#if __cpp_lib_format
    return std::format( "{:x}", value );
#else
    std::stringstream stream;
    stream << std::hex << value;
    return stream.str();
#endif
  }


  //=== VK_VERSION_1_0 ===


  VULKAN_HPP_INLINE std::string to_string( Result value )
  {
    switch ( value )
    {
      case Result::eSuccess : return "Success";
      case Result::eNotReady : return "NotReady";
      case Result::eTimeout : return "Timeout";
      case Result::eEventSet : return "EventSet";
      case Result::eEventReset : return "EventReset";
      case Result::eIncomplete : return "Incomplete";
      case Result::eErrorOutOfHostMemory : return "ErrorOutOfHostMemory";
      case Result::eErrorOutOfDeviceMemory : return "ErrorOutOfDeviceMemory";
      case Result::eErrorInitializationFailed : return "ErrorInitializationFailed";
      case Result::eErrorDeviceLost : return "ErrorDeviceLost";
      case Result::eErrorMemoryMapFailed : return "ErrorMemoryMapFailed";
      case Result::eErrorLayerNotPresent : return "ErrorLayerNotPresent";
      case Result::eErrorExtensionNotPresent : return "ErrorExtensionNotPresent";
      case Result::eErrorFeatureNotPresent : return "ErrorFeatureNotPresent";
      case Result::eErrorIncompatibleDriver : return "ErrorIncompatibleDriver";
      case Result::eErrorTooManyObjects : return "ErrorTooManyObjects";
      case Result::eErrorFormatNotSupported : return "ErrorFormatNotSupported";
      case Result::eErrorFragmentedPool : return "ErrorFragmentedPool";
      case Result::eErrorUnknown : return "ErrorUnknown";
      case Result::eErrorOutOfPoolMemory : return "ErrorOutOfPoolMemory";
      case Result::eErrorInvalidExternalHandle : return "ErrorInvalidExternalHandle";
      case Result::eErrorFragmentation : return "ErrorFragmentation";
      case Result::eErrorInvalidOpaqueCaptureAddress : return "ErrorInvalidOpaqueCaptureAddress";
      case Result::eErrorSurfaceLostKHR : return "ErrorSurfaceLostKHR";
      case Result::eErrorNativeWindowInUseKHR : return "ErrorNativeWindowInUseKHR";
      case Result::eSuboptimalKHR : return "SuboptimalKHR";
      case Result::eErrorOutOfDateKHR : return "ErrorOutOfDateKHR";
      case Result::eErrorIncompatibleDisplayKHR : return "ErrorIncompatibleDisplayKHR";
      case Result::eErrorValidationFailedEXT : return "ErrorValidationFailedEXT";
      case Result::eErrorInvalidShaderNV : return "ErrorInvalidShaderNV";
      case Result::eErrorInvalidDrmFormatModifierPlaneLayoutEXT : return "ErrorInvalidDrmFormatModifierPlaneLayoutEXT";
      case Result::eErrorNotPermittedEXT : return "ErrorNotPermittedEXT";
#if defined( VK_USE_PLATFORM_WIN32_KHR )
      case Result::eErrorFullScreenExclusiveModeLostEXT : return "ErrorFullScreenExclusiveModeLostEXT";
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
      case Result::eThreadIdleKHR : return "ThreadIdleKHR";
      case Result::eThreadDoneKHR : return "ThreadDoneKHR";
      case Result::eOperationDeferredKHR : return "OperationDeferredKHR";
      case Result::eOperationNotDeferredKHR : return "OperationNotDeferredKHR";
      case Result::ePipelineCompileRequiredEXT : return "PipelineCompileRequiredEXT";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( StructureType value )
  {
    switch ( value )
    {
      case StructureType::eApplicationInfo : return "ApplicationInfo";
      case StructureType::eInstanceCreateInfo : return "InstanceCreateInfo";
      case StructureType::eDeviceQueueCreateInfo : return "DeviceQueueCreateInfo";
      case StructureType::eDeviceCreateInfo : return "DeviceCreateInfo";
      case StructureType::eSubmitInfo : return "SubmitInfo";
      case StructureType::eMemoryAllocateInfo : return "MemoryAllocateInfo";
      case StructureType::eMappedMemoryRange : return "MappedMemoryRange";
      case StructureType::eBindSparseInfo : return "BindSparseInfo";
      case StructureType::eFenceCreateInfo : return "FenceCreateInfo";
      case StructureType::eSemaphoreCreateInfo : return "SemaphoreCreateInfo";
      case StructureType::eEventCreateInfo : return "EventCreateInfo";
      case StructureType::eQueryPoolCreateInfo : return "QueryPoolCreateInfo";
      case StructureType::eBufferCreateInfo : return "BufferCreateInfo";
      case StructureType::eBufferViewCreateInfo : return "BufferViewCreateInfo";
      case StructureType::eImageCreateInfo : return "ImageCreateInfo";
      case StructureType::eImageViewCreateInfo : return "ImageViewCreateInfo";
      case StructureType::eShaderModuleCreateInfo : return "ShaderModuleCreateInfo";
      case StructureType::ePipelineCacheCreateInfo : return "PipelineCacheCreateInfo";
      case StructureType::ePipelineShaderStageCreateInfo : return "PipelineShaderStageCreateInfo";
      case StructureType::ePipelineVertexInputStateCreateInfo : return "PipelineVertexInputStateCreateInfo";
      case StructureType::ePipelineInputAssemblyStateCreateInfo : return "PipelineInputAssemblyStateCreateInfo";
      case StructureType::ePipelineTessellationStateCreateInfo : return "PipelineTessellationStateCreateInfo";
      case StructureType::ePipelineViewportStateCreateInfo : return "PipelineViewportStateCreateInfo";
      case StructureType::ePipelineRasterizationStateCreateInfo : return "PipelineRasterizationStateCreateInfo";
      case StructureType::ePipelineMultisampleStateCreateInfo : return "PipelineMultisampleStateCreateInfo";
      case StructureType::ePipelineDepthStencilStateCreateInfo : return "PipelineDepthStencilStateCreateInfo";
      case StructureType::ePipelineColorBlendStateCreateInfo : return "PipelineColorBlendStateCreateInfo";
      case StructureType::ePipelineDynamicStateCreateInfo : return "PipelineDynamicStateCreateInfo";
      case StructureType::eGraphicsPipelineCreateInfo : return "GraphicsPipelineCreateInfo";
      case StructureType::eComputePipelineCreateInfo : return "ComputePipelineCreateInfo";
      case StructureType::ePipelineLayoutCreateInfo : return "PipelineLayoutCreateInfo";
      case StructureType::eSamplerCreateInfo : return "SamplerCreateInfo";
      case StructureType::eDescriptorSetLayoutCreateInfo : return "DescriptorSetLayoutCreateInfo";
      case StructureType::eDescriptorPoolCreateInfo : return "DescriptorPoolCreateInfo";
      case StructureType::eDescriptorSetAllocateInfo : return "DescriptorSetAllocateInfo";
      case StructureType::eWriteDescriptorSet : return "WriteDescriptorSet";
      case StructureType::eCopyDescriptorSet : return "CopyDescriptorSet";
      case StructureType::eFramebufferCreateInfo : return "FramebufferCreateInfo";
      case StructureType::eRenderPassCreateInfo : return "RenderPassCreateInfo";
      case StructureType::eCommandPoolCreateInfo : return "CommandPoolCreateInfo";
      case StructureType::eCommandBufferAllocateInfo : return "CommandBufferAllocateInfo";
      case StructureType::eCommandBufferInheritanceInfo : return "CommandBufferInheritanceInfo";
      case StructureType::eCommandBufferBeginInfo : return "CommandBufferBeginInfo";
      case StructureType::eRenderPassBeginInfo : return "RenderPassBeginInfo";
      case StructureType::eBufferMemoryBarrier : return "BufferMemoryBarrier";
      case StructureType::eImageMemoryBarrier : return "ImageMemoryBarrier";
      case StructureType::eMemoryBarrier : return "MemoryBarrier";
      case StructureType::eLoaderInstanceCreateInfo : return "LoaderInstanceCreateInfo";
      case StructureType::eLoaderDeviceCreateInfo : return "LoaderDeviceCreateInfo";
      case StructureType::ePhysicalDeviceSubgroupProperties : return "PhysicalDeviceSubgroupProperties";
      case StructureType::eBindBufferMemoryInfo : return "BindBufferMemoryInfo";
      case StructureType::eBindImageMemoryInfo : return "BindImageMemoryInfo";
      case StructureType::ePhysicalDevice16BitStorageFeatures : return "PhysicalDevice16BitStorageFeatures";
      case StructureType::eMemoryDedicatedRequirements : return "MemoryDedicatedRequirements";
      case StructureType::eMemoryDedicatedAllocateInfo : return "MemoryDedicatedAllocateInfo";
      case StructureType::eMemoryAllocateFlagsInfo : return "MemoryAllocateFlagsInfo";
      case StructureType::eDeviceGroupRenderPassBeginInfo : return "DeviceGroupRenderPassBeginInfo";
      case StructureType::eDeviceGroupCommandBufferBeginInfo : return "DeviceGroupCommandBufferBeginInfo";
      case StructureType::eDeviceGroupSubmitInfo : return "DeviceGroupSubmitInfo";
      case StructureType::eDeviceGroupBindSparseInfo : return "DeviceGroupBindSparseInfo";
      case StructureType::eBindBufferMemoryDeviceGroupInfo : return "BindBufferMemoryDeviceGroupInfo";
      case StructureType::eBindImageMemoryDeviceGroupInfo : return "BindImageMemoryDeviceGroupInfo";
      case StructureType::ePhysicalDeviceGroupProperties : return "PhysicalDeviceGroupProperties";
      case StructureType::eDeviceGroupDeviceCreateInfo : return "DeviceGroupDeviceCreateInfo";
      case StructureType::eBufferMemoryRequirementsInfo2 : return "BufferMemoryRequirementsInfo2";
      case StructureType::eImageMemoryRequirementsInfo2 : return "ImageMemoryRequirementsInfo2";
      case StructureType::eImageSparseMemoryRequirementsInfo2 : return "ImageSparseMemoryRequirementsInfo2";
      case StructureType::eMemoryRequirements2 : return "MemoryRequirements2";
      case StructureType::eSparseImageMemoryRequirements2 : return "SparseImageMemoryRequirements2";
      case StructureType::ePhysicalDeviceFeatures2 : return "PhysicalDeviceFeatures2";
      case StructureType::ePhysicalDeviceProperties2 : return "PhysicalDeviceProperties2";
      case StructureType::eFormatProperties2 : return "FormatProperties2";
      case StructureType::eImageFormatProperties2 : return "ImageFormatProperties2";
      case StructureType::ePhysicalDeviceImageFormatInfo2 : return "PhysicalDeviceImageFormatInfo2";
      case StructureType::eQueueFamilyProperties2 : return "QueueFamilyProperties2";
      case StructureType::ePhysicalDeviceMemoryProperties2 : return "PhysicalDeviceMemoryProperties2";
      case StructureType::eSparseImageFormatProperties2 : return "SparseImageFormatProperties2";
      case StructureType::ePhysicalDeviceSparseImageFormatInfo2 : return "PhysicalDeviceSparseImageFormatInfo2";
      case StructureType::ePhysicalDevicePointClippingProperties : return "PhysicalDevicePointClippingProperties";
      case StructureType::eRenderPassInputAttachmentAspectCreateInfo : return "RenderPassInputAttachmentAspectCreateInfo";
      case StructureType::eImageViewUsageCreateInfo : return "ImageViewUsageCreateInfo";
      case StructureType::ePipelineTessellationDomainOriginStateCreateInfo : return "PipelineTessellationDomainOriginStateCreateInfo";
      case StructureType::eRenderPassMultiviewCreateInfo : return "RenderPassMultiviewCreateInfo";
      case StructureType::ePhysicalDeviceMultiviewFeatures : return "PhysicalDeviceMultiviewFeatures";
      case StructureType::ePhysicalDeviceMultiviewProperties : return "PhysicalDeviceMultiviewProperties";
      case StructureType::ePhysicalDeviceVariablePointersFeatures : return "PhysicalDeviceVariablePointersFeatures";
      case StructureType::eProtectedSubmitInfo : return "ProtectedSubmitInfo";
      case StructureType::ePhysicalDeviceProtectedMemoryFeatures : return "PhysicalDeviceProtectedMemoryFeatures";
      case StructureType::ePhysicalDeviceProtectedMemoryProperties : return "PhysicalDeviceProtectedMemoryProperties";
      case StructureType::eDeviceQueueInfo2 : return "DeviceQueueInfo2";
      case StructureType::eSamplerYcbcrConversionCreateInfo : return "SamplerYcbcrConversionCreateInfo";
      case StructureType::eSamplerYcbcrConversionInfo : return "SamplerYcbcrConversionInfo";
      case StructureType::eBindImagePlaneMemoryInfo : return "BindImagePlaneMemoryInfo";
      case StructureType::eImagePlaneMemoryRequirementsInfo : return "ImagePlaneMemoryRequirementsInfo";
      case StructureType::ePhysicalDeviceSamplerYcbcrConversionFeatures : return "PhysicalDeviceSamplerYcbcrConversionFeatures";
      case StructureType::eSamplerYcbcrConversionImageFormatProperties : return "SamplerYcbcrConversionImageFormatProperties";
      case StructureType::eDescriptorUpdateTemplateCreateInfo : return "DescriptorUpdateTemplateCreateInfo";
      case StructureType::ePhysicalDeviceExternalImageFormatInfo : return "PhysicalDeviceExternalImageFormatInfo";
      case StructureType::eExternalImageFormatProperties : return "ExternalImageFormatProperties";
      case StructureType::ePhysicalDeviceExternalBufferInfo : return "PhysicalDeviceExternalBufferInfo";
      case StructureType::eExternalBufferProperties : return "ExternalBufferProperties";
      case StructureType::ePhysicalDeviceIdProperties : return "PhysicalDeviceIdProperties";
      case StructureType::eExternalMemoryBufferCreateInfo : return "ExternalMemoryBufferCreateInfo";
      case StructureType::eExternalMemoryImageCreateInfo : return "ExternalMemoryImageCreateInfo";
      case StructureType::eExportMemoryAllocateInfo : return "ExportMemoryAllocateInfo";
      case StructureType::ePhysicalDeviceExternalFenceInfo : return "PhysicalDeviceExternalFenceInfo";
      case StructureType::eExternalFenceProperties : return "ExternalFenceProperties";
      case StructureType::eExportFenceCreateInfo : return "ExportFenceCreateInfo";
      case StructureType::eExportSemaphoreCreateInfo : return "ExportSemaphoreCreateInfo";
      case StructureType::ePhysicalDeviceExternalSemaphoreInfo : return "PhysicalDeviceExternalSemaphoreInfo";
      case StructureType::eExternalSemaphoreProperties : return "ExternalSemaphoreProperties";
      case StructureType::ePhysicalDeviceMaintenance3Properties : return "PhysicalDeviceMaintenance3Properties";
      case StructureType::eDescriptorSetLayoutSupport : return "DescriptorSetLayoutSupport";
      case StructureType::ePhysicalDeviceShaderDrawParametersFeatures : return "PhysicalDeviceShaderDrawParametersFeatures";
      case StructureType::ePhysicalDeviceVulkan11Features : return "PhysicalDeviceVulkan11Features";
      case StructureType::ePhysicalDeviceVulkan11Properties : return "PhysicalDeviceVulkan11Properties";
      case StructureType::ePhysicalDeviceVulkan12Features : return "PhysicalDeviceVulkan12Features";
      case StructureType::ePhysicalDeviceVulkan12Properties : return "PhysicalDeviceVulkan12Properties";
      case StructureType::eImageFormatListCreateInfo : return "ImageFormatListCreateInfo";
      case StructureType::eAttachmentDescription2 : return "AttachmentDescription2";
      case StructureType::eAttachmentReference2 : return "AttachmentReference2";
      case StructureType::eSubpassDescription2 : return "SubpassDescription2";
      case StructureType::eSubpassDependency2 : return "SubpassDependency2";
      case StructureType::eRenderPassCreateInfo2 : return "RenderPassCreateInfo2";
      case StructureType::eSubpassBeginInfo : return "SubpassBeginInfo";
      case StructureType::eSubpassEndInfo : return "SubpassEndInfo";
      case StructureType::ePhysicalDevice8BitStorageFeatures : return "PhysicalDevice8BitStorageFeatures";
      case StructureType::ePhysicalDeviceDriverProperties : return "PhysicalDeviceDriverProperties";
      case StructureType::ePhysicalDeviceShaderAtomicInt64Features : return "PhysicalDeviceShaderAtomicInt64Features";
      case StructureType::ePhysicalDeviceShaderFloat16Int8Features : return "PhysicalDeviceShaderFloat16Int8Features";
      case StructureType::ePhysicalDeviceFloatControlsProperties : return "PhysicalDeviceFloatControlsProperties";
      case StructureType::eDescriptorSetLayoutBindingFlagsCreateInfo : return "DescriptorSetLayoutBindingFlagsCreateInfo";
      case StructureType::ePhysicalDeviceDescriptorIndexingFeatures : return "PhysicalDeviceDescriptorIndexingFeatures";
      case StructureType::ePhysicalDeviceDescriptorIndexingProperties : return "PhysicalDeviceDescriptorIndexingProperties";
      case StructureType::eDescriptorSetVariableDescriptorCountAllocateInfo : return "DescriptorSetVariableDescriptorCountAllocateInfo";
      case StructureType::eDescriptorSetVariableDescriptorCountLayoutSupport : return "DescriptorSetVariableDescriptorCountLayoutSupport";
      case StructureType::ePhysicalDeviceDepthStencilResolveProperties : return "PhysicalDeviceDepthStencilResolveProperties";
      case StructureType::eSubpassDescriptionDepthStencilResolve : return "SubpassDescriptionDepthStencilResolve";
      case StructureType::ePhysicalDeviceScalarBlockLayoutFeatures : return "PhysicalDeviceScalarBlockLayoutFeatures";
      case StructureType::eImageStencilUsageCreateInfo : return "ImageStencilUsageCreateInfo";
      case StructureType::ePhysicalDeviceSamplerFilterMinmaxProperties : return "PhysicalDeviceSamplerFilterMinmaxProperties";
      case StructureType::eSamplerReductionModeCreateInfo : return "SamplerReductionModeCreateInfo";
      case StructureType::ePhysicalDeviceVulkanMemoryModelFeatures : return "PhysicalDeviceVulkanMemoryModelFeatures";
      case StructureType::ePhysicalDeviceImagelessFramebufferFeatures : return "PhysicalDeviceImagelessFramebufferFeatures";
      case StructureType::eFramebufferAttachmentsCreateInfo : return "FramebufferAttachmentsCreateInfo";
      case StructureType::eFramebufferAttachmentImageInfo : return "FramebufferAttachmentImageInfo";
      case StructureType::eRenderPassAttachmentBeginInfo : return "RenderPassAttachmentBeginInfo";
      case StructureType::ePhysicalDeviceUniformBufferStandardLayoutFeatures : return "PhysicalDeviceUniformBufferStandardLayoutFeatures";
      case StructureType::ePhysicalDeviceShaderSubgroupExtendedTypesFeatures : return "PhysicalDeviceShaderSubgroupExtendedTypesFeatures";
      case StructureType::ePhysicalDeviceSeparateDepthStencilLayoutsFeatures : return "PhysicalDeviceSeparateDepthStencilLayoutsFeatures";
      case StructureType::eAttachmentReferenceStencilLayout : return "AttachmentReferenceStencilLayout";
      case StructureType::eAttachmentDescriptionStencilLayout : return "AttachmentDescriptionStencilLayout";
      case StructureType::ePhysicalDeviceHostQueryResetFeatures : return "PhysicalDeviceHostQueryResetFeatures";
      case StructureType::ePhysicalDeviceTimelineSemaphoreFeatures : return "PhysicalDeviceTimelineSemaphoreFeatures";
      case StructureType::ePhysicalDeviceTimelineSemaphoreProperties : return "PhysicalDeviceTimelineSemaphoreProperties";
      case StructureType::eSemaphoreTypeCreateInfo : return "SemaphoreTypeCreateInfo";
      case StructureType::eTimelineSemaphoreSubmitInfo : return "TimelineSemaphoreSubmitInfo";
      case StructureType::eSemaphoreWaitInfo : return "SemaphoreWaitInfo";
      case StructureType::eSemaphoreSignalInfo : return "SemaphoreSignalInfo";
      case StructureType::ePhysicalDeviceBufferDeviceAddressFeatures : return "PhysicalDeviceBufferDeviceAddressFeatures";
      case StructureType::eBufferDeviceAddressInfo : return "BufferDeviceAddressInfo";
      case StructureType::eBufferOpaqueCaptureAddressCreateInfo : return "BufferOpaqueCaptureAddressCreateInfo";
      case StructureType::eMemoryOpaqueCaptureAddressAllocateInfo : return "MemoryOpaqueCaptureAddressAllocateInfo";
      case StructureType::eDeviceMemoryOpaqueCaptureAddressInfo : return "DeviceMemoryOpaqueCaptureAddressInfo";
      case StructureType::eSwapchainCreateInfoKHR : return "SwapchainCreateInfoKHR";
      case StructureType::ePresentInfoKHR : return "PresentInfoKHR";
      case StructureType::eDeviceGroupPresentCapabilitiesKHR : return "DeviceGroupPresentCapabilitiesKHR";
      case StructureType::eImageSwapchainCreateInfoKHR : return "ImageSwapchainCreateInfoKHR";
      case StructureType::eBindImageMemorySwapchainInfoKHR : return "BindImageMemorySwapchainInfoKHR";
      case StructureType::eAcquireNextImageInfoKHR : return "AcquireNextImageInfoKHR";
      case StructureType::eDeviceGroupPresentInfoKHR : return "DeviceGroupPresentInfoKHR";
      case StructureType::eDeviceGroupSwapchainCreateInfoKHR : return "DeviceGroupSwapchainCreateInfoKHR";
      case StructureType::eDisplayModeCreateInfoKHR : return "DisplayModeCreateInfoKHR";
      case StructureType::eDisplaySurfaceCreateInfoKHR : return "DisplaySurfaceCreateInfoKHR";
      case StructureType::eDisplayPresentInfoKHR : return "DisplayPresentInfoKHR";
#if defined( VK_USE_PLATFORM_XLIB_KHR )
      case StructureType::eXlibSurfaceCreateInfoKHR : return "XlibSurfaceCreateInfoKHR";
#endif /*VK_USE_PLATFORM_XLIB_KHR*/
#if defined( VK_USE_PLATFORM_XCB_KHR )
      case StructureType::eXcbSurfaceCreateInfoKHR : return "XcbSurfaceCreateInfoKHR";
#endif /*VK_USE_PLATFORM_XCB_KHR*/
#if defined( VK_USE_PLATFORM_WAYLAND_KHR )
      case StructureType::eWaylandSurfaceCreateInfoKHR : return "WaylandSurfaceCreateInfoKHR";
#endif /*VK_USE_PLATFORM_WAYLAND_KHR*/
#if defined( VK_USE_PLATFORM_ANDROID_KHR )
      case StructureType::eAndroidSurfaceCreateInfoKHR : return "AndroidSurfaceCreateInfoKHR";
#endif /*VK_USE_PLATFORM_ANDROID_KHR*/
#if defined( VK_USE_PLATFORM_WIN32_KHR )
      case StructureType::eWin32SurfaceCreateInfoKHR : return "Win32SurfaceCreateInfoKHR";
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
#if defined( VK_USE_PLATFORM_ANDROID_KHR )
      case StructureType::eNativeBufferANDROID : return "NativeBufferANDROID";
#endif /*VK_USE_PLATFORM_ANDROID_KHR*/
      case StructureType::eDebugReportCallbackCreateInfoEXT : return "DebugReportCallbackCreateInfoEXT";
      case StructureType::ePipelineRasterizationStateRasterizationOrderAMD : return "PipelineRasterizationStateRasterizationOrderAMD";
      case StructureType::eDebugMarkerObjectNameInfoEXT : return "DebugMarkerObjectNameInfoEXT";
      case StructureType::eDebugMarkerObjectTagInfoEXT : return "DebugMarkerObjectTagInfoEXT";
      case StructureType::eDebugMarkerMarkerInfoEXT : return "DebugMarkerMarkerInfoEXT";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
      case StructureType::eVideoProfileKHR : return "VideoProfileKHR";
      case StructureType::eVideoCapabilitiesKHR : return "VideoCapabilitiesKHR";
      case StructureType::eVideoPictureResourceKHR : return "VideoPictureResourceKHR";
      case StructureType::eVideoGetMemoryPropertiesKHR : return "VideoGetMemoryPropertiesKHR";
      case StructureType::eVideoBindMemoryKHR : return "VideoBindMemoryKHR";
      case StructureType::eVideoSessionCreateInfoKHR : return "VideoSessionCreateInfoKHR";
      case StructureType::eVideoSessionParametersCreateInfoKHR : return "VideoSessionParametersCreateInfoKHR";
      case StructureType::eVideoSessionParametersUpdateInfoKHR : return "VideoSessionParametersUpdateInfoKHR";
      case StructureType::eVideoBeginCodingInfoKHR : return "VideoBeginCodingInfoKHR";
      case StructureType::eVideoEndCodingInfoKHR : return "VideoEndCodingInfoKHR";
      case StructureType::eVideoCodingControlInfoKHR : return "VideoCodingControlInfoKHR";
      case StructureType::eVideoReferenceSlotKHR : return "VideoReferenceSlotKHR";
      case StructureType::eVideoQueueFamilyProperties2KHR : return "VideoQueueFamilyProperties2KHR";
      case StructureType::eVideoProfilesKHR : return "VideoProfilesKHR";
      case StructureType::ePhysicalDeviceVideoFormatInfoKHR : return "PhysicalDeviceVideoFormatInfoKHR";
      case StructureType::eVideoFormatPropertiesKHR : return "VideoFormatPropertiesKHR";
      case StructureType::eVideoDecodeInfoKHR : return "VideoDecodeInfoKHR";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
      case StructureType::eDedicatedAllocationImageCreateInfoNV : return "DedicatedAllocationImageCreateInfoNV";
      case StructureType::eDedicatedAllocationBufferCreateInfoNV : return "DedicatedAllocationBufferCreateInfoNV";
      case StructureType::eDedicatedAllocationMemoryAllocateInfoNV : return "DedicatedAllocationMemoryAllocateInfoNV";
      case StructureType::ePhysicalDeviceTransformFeedbackFeaturesEXT : return "PhysicalDeviceTransformFeedbackFeaturesEXT";
      case StructureType::ePhysicalDeviceTransformFeedbackPropertiesEXT : return "PhysicalDeviceTransformFeedbackPropertiesEXT";
      case StructureType::ePipelineRasterizationStateStreamCreateInfoEXT : return "PipelineRasterizationStateStreamCreateInfoEXT";
      case StructureType::eCuModuleCreateInfoNVX : return "CuModuleCreateInfoNVX";
      case StructureType::eCuFunctionCreateInfoNVX : return "CuFunctionCreateInfoNVX";
      case StructureType::eCuLaunchInfoNVX : return "CuLaunchInfoNVX";
      case StructureType::eImageViewHandleInfoNVX : return "ImageViewHandleInfoNVX";
      case StructureType::eImageViewAddressPropertiesNVX : return "ImageViewAddressPropertiesNVX";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
      case StructureType::eVideoEncodeH264CapabilitiesEXT : return "VideoEncodeH264CapabilitiesEXT";
      case StructureType::eVideoEncodeH264SessionCreateInfoEXT : return "VideoEncodeH264SessionCreateInfoEXT";
      case StructureType::eVideoEncodeH264SessionParametersCreateInfoEXT : return "VideoEncodeH264SessionParametersCreateInfoEXT";
      case StructureType::eVideoEncodeH264SessionParametersAddInfoEXT : return "VideoEncodeH264SessionParametersAddInfoEXT";
      case StructureType::eVideoEncodeH264VclFrameInfoEXT : return "VideoEncodeH264VclFrameInfoEXT";
      case StructureType::eVideoEncodeH264DpbSlotInfoEXT : return "VideoEncodeH264DpbSlotInfoEXT";
      case StructureType::eVideoEncodeH264NaluSliceEXT : return "VideoEncodeH264NaluSliceEXT";
      case StructureType::eVideoEncodeH264EmitPictureParametersEXT : return "VideoEncodeH264EmitPictureParametersEXT";
      case StructureType::eVideoEncodeH264ProfileEXT : return "VideoEncodeH264ProfileEXT";
      case StructureType::eVideoEncodeH265CapabilitiesEXT : return "VideoEncodeH265CapabilitiesEXT";
      case StructureType::eVideoEncodeH265SessionCreateInfoEXT : return "VideoEncodeH265SessionCreateInfoEXT";
      case StructureType::eVideoEncodeH265SessionParametersCreateInfoEXT : return "VideoEncodeH265SessionParametersCreateInfoEXT";
      case StructureType::eVideoEncodeH265SessionParametersAddInfoEXT : return "VideoEncodeH265SessionParametersAddInfoEXT";
      case StructureType::eVideoEncodeH265VclFrameInfoEXT : return "VideoEncodeH265VclFrameInfoEXT";
      case StructureType::eVideoEncodeH265DpbSlotInfoEXT : return "VideoEncodeH265DpbSlotInfoEXT";
      case StructureType::eVideoEncodeH265NaluSliceEXT : return "VideoEncodeH265NaluSliceEXT";
      case StructureType::eVideoEncodeH265EmitPictureParametersEXT : return "VideoEncodeH265EmitPictureParametersEXT";
      case StructureType::eVideoEncodeH265ProfileEXT : return "VideoEncodeH265ProfileEXT";
      case StructureType::eVideoEncodeH265ReferenceListsEXT : return "VideoEncodeH265ReferenceListsEXT";
      case StructureType::eVideoDecodeH264CapabilitiesEXT : return "VideoDecodeH264CapabilitiesEXT";
      case StructureType::eVideoDecodeH264SessionCreateInfoEXT : return "VideoDecodeH264SessionCreateInfoEXT";
      case StructureType::eVideoDecodeH264PictureInfoEXT : return "VideoDecodeH264PictureInfoEXT";
      case StructureType::eVideoDecodeH264MvcEXT : return "VideoDecodeH264MvcEXT";
      case StructureType::eVideoDecodeH264ProfileEXT : return "VideoDecodeH264ProfileEXT";
      case StructureType::eVideoDecodeH264SessionParametersCreateInfoEXT : return "VideoDecodeH264SessionParametersCreateInfoEXT";
      case StructureType::eVideoDecodeH264SessionParametersAddInfoEXT : return "VideoDecodeH264SessionParametersAddInfoEXT";
      case StructureType::eVideoDecodeH264DpbSlotInfoEXT : return "VideoDecodeH264DpbSlotInfoEXT";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
      case StructureType::eTextureLodGatherFormatPropertiesAMD : return "TextureLodGatherFormatPropertiesAMD";
      case StructureType::eRenderingInfoKHR : return "RenderingInfoKHR";
      case StructureType::eRenderingAttachmentInfoKHR : return "RenderingAttachmentInfoKHR";
      case StructureType::ePipelineRenderingCreateInfoKHR : return "PipelineRenderingCreateInfoKHR";
      case StructureType::ePhysicalDeviceDynamicRenderingFeaturesKHR : return "PhysicalDeviceDynamicRenderingFeaturesKHR";
      case StructureType::eCommandBufferInheritanceRenderingInfoKHR : return "CommandBufferInheritanceRenderingInfoKHR";
      case StructureType::eRenderingFragmentShadingRateAttachmentInfoKHR : return "RenderingFragmentShadingRateAttachmentInfoKHR";
      case StructureType::eRenderingFragmentDensityMapAttachmentInfoEXT : return "RenderingFragmentDensityMapAttachmentInfoEXT";
      case StructureType::eAttachmentSampleCountInfoAMD : return "AttachmentSampleCountInfoAMD";
      case StructureType::eMultiviewPerViewAttributesInfoNVX : return "MultiviewPerViewAttributesInfoNVX";
#if defined( VK_USE_PLATFORM_GGP )
      case StructureType::eStreamDescriptorSurfaceCreateInfoGGP : return "StreamDescriptorSurfaceCreateInfoGGP";
#endif /*VK_USE_PLATFORM_GGP*/
      case StructureType::ePhysicalDeviceCornerSampledImageFeaturesNV : return "PhysicalDeviceCornerSampledImageFeaturesNV";
      case StructureType::eExternalMemoryImageCreateInfoNV : return "ExternalMemoryImageCreateInfoNV";
      case StructureType::eExportMemoryAllocateInfoNV : return "ExportMemoryAllocateInfoNV";
#if defined( VK_USE_PLATFORM_WIN32_KHR )
      case StructureType::eImportMemoryWin32HandleInfoNV : return "ImportMemoryWin32HandleInfoNV";
      case StructureType::eExportMemoryWin32HandleInfoNV : return "ExportMemoryWin32HandleInfoNV";
      case StructureType::eWin32KeyedMutexAcquireReleaseInfoNV : return "Win32KeyedMutexAcquireReleaseInfoNV";
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
      case StructureType::eValidationFlagsEXT : return "ValidationFlagsEXT";
#if defined( VK_USE_PLATFORM_VI_NN )
      case StructureType::eViSurfaceCreateInfoNN : return "ViSurfaceCreateInfoNN";
#endif /*VK_USE_PLATFORM_VI_NN*/
      case StructureType::ePhysicalDeviceTextureCompressionAstcHdrFeaturesEXT : return "PhysicalDeviceTextureCompressionAstcHdrFeaturesEXT";
      case StructureType::eImageViewAstcDecodeModeEXT : return "ImageViewAstcDecodeModeEXT";
      case StructureType::ePhysicalDeviceAstcDecodeFeaturesEXT : return "PhysicalDeviceAstcDecodeFeaturesEXT";
#if defined( VK_USE_PLATFORM_WIN32_KHR )
      case StructureType::eImportMemoryWin32HandleInfoKHR : return "ImportMemoryWin32HandleInfoKHR";
      case StructureType::eExportMemoryWin32HandleInfoKHR : return "ExportMemoryWin32HandleInfoKHR";
      case StructureType::eMemoryWin32HandlePropertiesKHR : return "MemoryWin32HandlePropertiesKHR";
      case StructureType::eMemoryGetWin32HandleInfoKHR : return "MemoryGetWin32HandleInfoKHR";
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
      case StructureType::eImportMemoryFdInfoKHR : return "ImportMemoryFdInfoKHR";
      case StructureType::eMemoryFdPropertiesKHR : return "MemoryFdPropertiesKHR";
      case StructureType::eMemoryGetFdInfoKHR : return "MemoryGetFdInfoKHR";
#if defined( VK_USE_PLATFORM_WIN32_KHR )
      case StructureType::eWin32KeyedMutexAcquireReleaseInfoKHR : return "Win32KeyedMutexAcquireReleaseInfoKHR";
      case StructureType::eImportSemaphoreWin32HandleInfoKHR : return "ImportSemaphoreWin32HandleInfoKHR";
      case StructureType::eExportSemaphoreWin32HandleInfoKHR : return "ExportSemaphoreWin32HandleInfoKHR";
      case StructureType::eD3D12FenceSubmitInfoKHR : return "D3D12FenceSubmitInfoKHR";
      case StructureType::eSemaphoreGetWin32HandleInfoKHR : return "SemaphoreGetWin32HandleInfoKHR";
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
      case StructureType::eImportSemaphoreFdInfoKHR : return "ImportSemaphoreFdInfoKHR";
      case StructureType::eSemaphoreGetFdInfoKHR : return "SemaphoreGetFdInfoKHR";
      case StructureType::ePhysicalDevicePushDescriptorPropertiesKHR : return "PhysicalDevicePushDescriptorPropertiesKHR";
      case StructureType::eCommandBufferInheritanceConditionalRenderingInfoEXT : return "CommandBufferInheritanceConditionalRenderingInfoEXT";
      case StructureType::ePhysicalDeviceConditionalRenderingFeaturesEXT : return "PhysicalDeviceConditionalRenderingFeaturesEXT";
      case StructureType::eConditionalRenderingBeginInfoEXT : return "ConditionalRenderingBeginInfoEXT";
      case StructureType::ePresentRegionsKHR : return "PresentRegionsKHR";
      case StructureType::ePipelineViewportWScalingStateCreateInfoNV : return "PipelineViewportWScalingStateCreateInfoNV";
      case StructureType::eSurfaceCapabilities2EXT : return "SurfaceCapabilities2EXT";
      case StructureType::eDisplayPowerInfoEXT : return "DisplayPowerInfoEXT";
      case StructureType::eDeviceEventInfoEXT : return "DeviceEventInfoEXT";
      case StructureType::eDisplayEventInfoEXT : return "DisplayEventInfoEXT";
      case StructureType::eSwapchainCounterCreateInfoEXT : return "SwapchainCounterCreateInfoEXT";
      case StructureType::ePresentTimesInfoGOOGLE : return "PresentTimesInfoGOOGLE";
      case StructureType::ePhysicalDeviceMultiviewPerViewAttributesPropertiesNVX : return "PhysicalDeviceMultiviewPerViewAttributesPropertiesNVX";
      case StructureType::ePipelineViewportSwizzleStateCreateInfoNV : return "PipelineViewportSwizzleStateCreateInfoNV";
      case StructureType::ePhysicalDeviceDiscardRectanglePropertiesEXT : return "PhysicalDeviceDiscardRectanglePropertiesEXT";
      case StructureType::ePipelineDiscardRectangleStateCreateInfoEXT : return "PipelineDiscardRectangleStateCreateInfoEXT";
      case StructureType::ePhysicalDeviceConservativeRasterizationPropertiesEXT : return "PhysicalDeviceConservativeRasterizationPropertiesEXT";
      case StructureType::ePipelineRasterizationConservativeStateCreateInfoEXT : return "PipelineRasterizationConservativeStateCreateInfoEXT";
      case StructureType::ePhysicalDeviceDepthClipEnableFeaturesEXT : return "PhysicalDeviceDepthClipEnableFeaturesEXT";
      case StructureType::ePipelineRasterizationDepthClipStateCreateInfoEXT : return "PipelineRasterizationDepthClipStateCreateInfoEXT";
      case StructureType::eHdrMetadataEXT : return "HdrMetadataEXT";
      case StructureType::eSharedPresentSurfaceCapabilitiesKHR : return "SharedPresentSurfaceCapabilitiesKHR";
#if defined( VK_USE_PLATFORM_WIN32_KHR )
      case StructureType::eImportFenceWin32HandleInfoKHR : return "ImportFenceWin32HandleInfoKHR";
      case StructureType::eExportFenceWin32HandleInfoKHR : return "ExportFenceWin32HandleInfoKHR";
      case StructureType::eFenceGetWin32HandleInfoKHR : return "FenceGetWin32HandleInfoKHR";
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
      case StructureType::eImportFenceFdInfoKHR : return "ImportFenceFdInfoKHR";
      case StructureType::eFenceGetFdInfoKHR : return "FenceGetFdInfoKHR";
      case StructureType::ePhysicalDevicePerformanceQueryFeaturesKHR : return "PhysicalDevicePerformanceQueryFeaturesKHR";
      case StructureType::ePhysicalDevicePerformanceQueryPropertiesKHR : return "PhysicalDevicePerformanceQueryPropertiesKHR";
      case StructureType::eQueryPoolPerformanceCreateInfoKHR : return "QueryPoolPerformanceCreateInfoKHR";
      case StructureType::ePerformanceQuerySubmitInfoKHR : return "PerformanceQuerySubmitInfoKHR";
      case StructureType::eAcquireProfilingLockInfoKHR : return "AcquireProfilingLockInfoKHR";
      case StructureType::ePerformanceCounterKHR : return "PerformanceCounterKHR";
      case StructureType::ePerformanceCounterDescriptionKHR : return "PerformanceCounterDescriptionKHR";
      case StructureType::ePhysicalDeviceSurfaceInfo2KHR : return "PhysicalDeviceSurfaceInfo2KHR";
      case StructureType::eSurfaceCapabilities2KHR : return "SurfaceCapabilities2KHR";
      case StructureType::eSurfaceFormat2KHR : return "SurfaceFormat2KHR";
      case StructureType::eDisplayProperties2KHR : return "DisplayProperties2KHR";
      case StructureType::eDisplayPlaneProperties2KHR : return "DisplayPlaneProperties2KHR";
      case StructureType::eDisplayModeProperties2KHR : return "DisplayModeProperties2KHR";
      case StructureType::eDisplayPlaneInfo2KHR : return "DisplayPlaneInfo2KHR";
      case StructureType::eDisplayPlaneCapabilities2KHR : return "DisplayPlaneCapabilities2KHR";
#if defined( VK_USE_PLATFORM_IOS_MVK )
      case StructureType::eIosSurfaceCreateInfoMVK : return "IosSurfaceCreateInfoMVK";
#endif /*VK_USE_PLATFORM_IOS_MVK*/
#if defined( VK_USE_PLATFORM_MACOS_MVK )
      case StructureType::eMacosSurfaceCreateInfoMVK : return "MacosSurfaceCreateInfoMVK";
#endif /*VK_USE_PLATFORM_MACOS_MVK*/
      case StructureType::eDebugUtilsObjectNameInfoEXT : return "DebugUtilsObjectNameInfoEXT";
      case StructureType::eDebugUtilsObjectTagInfoEXT : return "DebugUtilsObjectTagInfoEXT";
      case StructureType::eDebugUtilsLabelEXT : return "DebugUtilsLabelEXT";
      case StructureType::eDebugUtilsMessengerCallbackDataEXT : return "DebugUtilsMessengerCallbackDataEXT";
      case StructureType::eDebugUtilsMessengerCreateInfoEXT : return "DebugUtilsMessengerCreateInfoEXT";
#if defined( VK_USE_PLATFORM_ANDROID_KHR )
      case StructureType::eAndroidHardwareBufferUsageANDROID : return "AndroidHardwareBufferUsageANDROID";
      case StructureType::eAndroidHardwareBufferPropertiesANDROID : return "AndroidHardwareBufferPropertiesANDROID";
      case StructureType::eAndroidHardwareBufferFormatPropertiesANDROID : return "AndroidHardwareBufferFormatPropertiesANDROID";
      case StructureType::eImportAndroidHardwareBufferInfoANDROID : return "ImportAndroidHardwareBufferInfoANDROID";
      case StructureType::eMemoryGetAndroidHardwareBufferInfoANDROID : return "MemoryGetAndroidHardwareBufferInfoANDROID";
      case StructureType::eExternalFormatANDROID : return "ExternalFormatANDROID";
      case StructureType::eAndroidHardwareBufferFormatProperties2ANDROID : return "AndroidHardwareBufferFormatProperties2ANDROID";
#endif /*VK_USE_PLATFORM_ANDROID_KHR*/
      case StructureType::ePhysicalDeviceInlineUniformBlockFeaturesEXT : return "PhysicalDeviceInlineUniformBlockFeaturesEXT";
      case StructureType::ePhysicalDeviceInlineUniformBlockPropertiesEXT : return "PhysicalDeviceInlineUniformBlockPropertiesEXT";
      case StructureType::eWriteDescriptorSetInlineUniformBlockEXT : return "WriteDescriptorSetInlineUniformBlockEXT";
      case StructureType::eDescriptorPoolInlineUniformBlockCreateInfoEXT : return "DescriptorPoolInlineUniformBlockCreateInfoEXT";
      case StructureType::eSampleLocationsInfoEXT : return "SampleLocationsInfoEXT";
      case StructureType::eRenderPassSampleLocationsBeginInfoEXT : return "RenderPassSampleLocationsBeginInfoEXT";
      case StructureType::ePipelineSampleLocationsStateCreateInfoEXT : return "PipelineSampleLocationsStateCreateInfoEXT";
      case StructureType::ePhysicalDeviceSampleLocationsPropertiesEXT : return "PhysicalDeviceSampleLocationsPropertiesEXT";
      case StructureType::eMultisamplePropertiesEXT : return "MultisamplePropertiesEXT";
      case StructureType::ePhysicalDeviceBlendOperationAdvancedFeaturesEXT : return "PhysicalDeviceBlendOperationAdvancedFeaturesEXT";
      case StructureType::ePhysicalDeviceBlendOperationAdvancedPropertiesEXT : return "PhysicalDeviceBlendOperationAdvancedPropertiesEXT";
      case StructureType::ePipelineColorBlendAdvancedStateCreateInfoEXT : return "PipelineColorBlendAdvancedStateCreateInfoEXT";
      case StructureType::ePipelineCoverageToColorStateCreateInfoNV : return "PipelineCoverageToColorStateCreateInfoNV";
      case StructureType::eWriteDescriptorSetAccelerationStructureKHR : return "WriteDescriptorSetAccelerationStructureKHR";
      case StructureType::eAccelerationStructureBuildGeometryInfoKHR : return "AccelerationStructureBuildGeometryInfoKHR";
      case StructureType::eAccelerationStructureDeviceAddressInfoKHR : return "AccelerationStructureDeviceAddressInfoKHR";
      case StructureType::eAccelerationStructureGeometryAabbsDataKHR : return "AccelerationStructureGeometryAabbsDataKHR";
      case StructureType::eAccelerationStructureGeometryInstancesDataKHR : return "AccelerationStructureGeometryInstancesDataKHR";
      case StructureType::eAccelerationStructureGeometryTrianglesDataKHR : return "AccelerationStructureGeometryTrianglesDataKHR";
      case StructureType::eAccelerationStructureGeometryKHR : return "AccelerationStructureGeometryKHR";
      case StructureType::eAccelerationStructureVersionInfoKHR : return "AccelerationStructureVersionInfoKHR";
      case StructureType::eCopyAccelerationStructureInfoKHR : return "CopyAccelerationStructureInfoKHR";
      case StructureType::eCopyAccelerationStructureToMemoryInfoKHR : return "CopyAccelerationStructureToMemoryInfoKHR";
      case StructureType::eCopyMemoryToAccelerationStructureInfoKHR : return "CopyMemoryToAccelerationStructureInfoKHR";
      case StructureType::ePhysicalDeviceAccelerationStructureFeaturesKHR : return "PhysicalDeviceAccelerationStructureFeaturesKHR";
      case StructureType::ePhysicalDeviceAccelerationStructurePropertiesKHR : return "PhysicalDeviceAccelerationStructurePropertiesKHR";
      case StructureType::eAccelerationStructureCreateInfoKHR : return "AccelerationStructureCreateInfoKHR";
      case StructureType::eAccelerationStructureBuildSizesInfoKHR : return "AccelerationStructureBuildSizesInfoKHR";
      case StructureType::ePhysicalDeviceRayTracingPipelineFeaturesKHR : return "PhysicalDeviceRayTracingPipelineFeaturesKHR";
      case StructureType::ePhysicalDeviceRayTracingPipelinePropertiesKHR : return "PhysicalDeviceRayTracingPipelinePropertiesKHR";
      case StructureType::eRayTracingPipelineCreateInfoKHR : return "RayTracingPipelineCreateInfoKHR";
      case StructureType::eRayTracingShaderGroupCreateInfoKHR : return "RayTracingShaderGroupCreateInfoKHR";
      case StructureType::eRayTracingPipelineInterfaceCreateInfoKHR : return "RayTracingPipelineInterfaceCreateInfoKHR";
      case StructureType::ePhysicalDeviceRayQueryFeaturesKHR : return "PhysicalDeviceRayQueryFeaturesKHR";
      case StructureType::ePipelineCoverageModulationStateCreateInfoNV : return "PipelineCoverageModulationStateCreateInfoNV";
      case StructureType::ePhysicalDeviceShaderSmBuiltinsFeaturesNV : return "PhysicalDeviceShaderSmBuiltinsFeaturesNV";
      case StructureType::ePhysicalDeviceShaderSmBuiltinsPropertiesNV : return "PhysicalDeviceShaderSmBuiltinsPropertiesNV";
      case StructureType::eDrmFormatModifierPropertiesListEXT : return "DrmFormatModifierPropertiesListEXT";
      case StructureType::ePhysicalDeviceImageDrmFormatModifierInfoEXT : return "PhysicalDeviceImageDrmFormatModifierInfoEXT";
      case StructureType::eImageDrmFormatModifierListCreateInfoEXT : return "ImageDrmFormatModifierListCreateInfoEXT";
      case StructureType::eImageDrmFormatModifierExplicitCreateInfoEXT : return "ImageDrmFormatModifierExplicitCreateInfoEXT";
      case StructureType::eImageDrmFormatModifierPropertiesEXT : return "ImageDrmFormatModifierPropertiesEXT";
      case StructureType::eDrmFormatModifierPropertiesList2EXT : return "DrmFormatModifierPropertiesList2EXT";
      case StructureType::eValidationCacheCreateInfoEXT : return "ValidationCacheCreateInfoEXT";
      case StructureType::eShaderModuleValidationCacheCreateInfoEXT : return "ShaderModuleValidationCacheCreateInfoEXT";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
      case StructureType::ePhysicalDevicePortabilitySubsetFeaturesKHR : return "PhysicalDevicePortabilitySubsetFeaturesKHR";
      case StructureType::ePhysicalDevicePortabilitySubsetPropertiesKHR : return "PhysicalDevicePortabilitySubsetPropertiesKHR";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
      case StructureType::ePipelineViewportShadingRateImageStateCreateInfoNV : return "PipelineViewportShadingRateImageStateCreateInfoNV";
      case StructureType::ePhysicalDeviceShadingRateImageFeaturesNV : return "PhysicalDeviceShadingRateImageFeaturesNV";
      case StructureType::ePhysicalDeviceShadingRateImagePropertiesNV : return "PhysicalDeviceShadingRateImagePropertiesNV";
      case StructureType::ePipelineViewportCoarseSampleOrderStateCreateInfoNV : return "PipelineViewportCoarseSampleOrderStateCreateInfoNV";
      case StructureType::eRayTracingPipelineCreateInfoNV : return "RayTracingPipelineCreateInfoNV";
      case StructureType::eAccelerationStructureCreateInfoNV : return "AccelerationStructureCreateInfoNV";
      case StructureType::eGeometryNV : return "GeometryNV";
      case StructureType::eGeometryTrianglesNV : return "GeometryTrianglesNV";
      case StructureType::eGeometryAabbNV : return "GeometryAabbNV";
      case StructureType::eBindAccelerationStructureMemoryInfoNV : return "BindAccelerationStructureMemoryInfoNV";
      case StructureType::eWriteDescriptorSetAccelerationStructureNV : return "WriteDescriptorSetAccelerationStructureNV";
      case StructureType::eAccelerationStructureMemoryRequirementsInfoNV : return "AccelerationStructureMemoryRequirementsInfoNV";
      case StructureType::ePhysicalDeviceRayTracingPropertiesNV : return "PhysicalDeviceRayTracingPropertiesNV";
      case StructureType::eRayTracingShaderGroupCreateInfoNV : return "RayTracingShaderGroupCreateInfoNV";
      case StructureType::eAccelerationStructureInfoNV : return "AccelerationStructureInfoNV";
      case StructureType::ePhysicalDeviceRepresentativeFragmentTestFeaturesNV : return "PhysicalDeviceRepresentativeFragmentTestFeaturesNV";
      case StructureType::ePipelineRepresentativeFragmentTestStateCreateInfoNV : return "PipelineRepresentativeFragmentTestStateCreateInfoNV";
      case StructureType::ePhysicalDeviceImageViewImageFormatInfoEXT : return "PhysicalDeviceImageViewImageFormatInfoEXT";
      case StructureType::eFilterCubicImageViewImageFormatPropertiesEXT : return "FilterCubicImageViewImageFormatPropertiesEXT";
      case StructureType::eDeviceQueueGlobalPriorityCreateInfoEXT : return "DeviceQueueGlobalPriorityCreateInfoEXT";
      case StructureType::eImportMemoryHostPointerInfoEXT : return "ImportMemoryHostPointerInfoEXT";
      case StructureType::eMemoryHostPointerPropertiesEXT : return "MemoryHostPointerPropertiesEXT";
      case StructureType::ePhysicalDeviceExternalMemoryHostPropertiesEXT : return "PhysicalDeviceExternalMemoryHostPropertiesEXT";
      case StructureType::ePhysicalDeviceShaderClockFeaturesKHR : return "PhysicalDeviceShaderClockFeaturesKHR";
      case StructureType::ePipelineCompilerControlCreateInfoAMD : return "PipelineCompilerControlCreateInfoAMD";
      case StructureType::eCalibratedTimestampInfoEXT : return "CalibratedTimestampInfoEXT";
      case StructureType::ePhysicalDeviceShaderCorePropertiesAMD : return "PhysicalDeviceShaderCorePropertiesAMD";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
      case StructureType::eVideoDecodeH265CapabilitiesEXT : return "VideoDecodeH265CapabilitiesEXT";
      case StructureType::eVideoDecodeH265SessionCreateInfoEXT : return "VideoDecodeH265SessionCreateInfoEXT";
      case StructureType::eVideoDecodeH265SessionParametersCreateInfoEXT : return "VideoDecodeH265SessionParametersCreateInfoEXT";
      case StructureType::eVideoDecodeH265SessionParametersAddInfoEXT : return "VideoDecodeH265SessionParametersAddInfoEXT";
      case StructureType::eVideoDecodeH265ProfileEXT : return "VideoDecodeH265ProfileEXT";
      case StructureType::eVideoDecodeH265PictureInfoEXT : return "VideoDecodeH265PictureInfoEXT";
      case StructureType::eVideoDecodeH265DpbSlotInfoEXT : return "VideoDecodeH265DpbSlotInfoEXT";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
      case StructureType::eDeviceMemoryOverallocationCreateInfoAMD : return "DeviceMemoryOverallocationCreateInfoAMD";
      case StructureType::ePhysicalDeviceVertexAttributeDivisorPropertiesEXT : return "PhysicalDeviceVertexAttributeDivisorPropertiesEXT";
      case StructureType::ePipelineVertexInputDivisorStateCreateInfoEXT : return "PipelineVertexInputDivisorStateCreateInfoEXT";
      case StructureType::ePhysicalDeviceVertexAttributeDivisorFeaturesEXT : return "PhysicalDeviceVertexAttributeDivisorFeaturesEXT";
#if defined( VK_USE_PLATFORM_GGP )
      case StructureType::ePresentFrameTokenGGP : return "PresentFrameTokenGGP";
#endif /*VK_USE_PLATFORM_GGP*/
      case StructureType::ePipelineCreationFeedbackCreateInfoEXT : return "PipelineCreationFeedbackCreateInfoEXT";
      case StructureType::ePhysicalDeviceComputeShaderDerivativesFeaturesNV : return "PhysicalDeviceComputeShaderDerivativesFeaturesNV";
      case StructureType::ePhysicalDeviceMeshShaderFeaturesNV : return "PhysicalDeviceMeshShaderFeaturesNV";
      case StructureType::ePhysicalDeviceMeshShaderPropertiesNV : return "PhysicalDeviceMeshShaderPropertiesNV";
      case StructureType::ePhysicalDeviceFragmentShaderBarycentricFeaturesNV : return "PhysicalDeviceFragmentShaderBarycentricFeaturesNV";
      case StructureType::ePhysicalDeviceShaderImageFootprintFeaturesNV : return "PhysicalDeviceShaderImageFootprintFeaturesNV";
      case StructureType::ePipelineViewportExclusiveScissorStateCreateInfoNV : return "PipelineViewportExclusiveScissorStateCreateInfoNV";
      case StructureType::ePhysicalDeviceExclusiveScissorFeaturesNV : return "PhysicalDeviceExclusiveScissorFeaturesNV";
      case StructureType::eCheckpointDataNV : return "CheckpointDataNV";
      case StructureType::eQueueFamilyCheckpointPropertiesNV : return "QueueFamilyCheckpointPropertiesNV";
      case StructureType::ePhysicalDeviceShaderIntegerFunctions2FeaturesINTEL : return "PhysicalDeviceShaderIntegerFunctions2FeaturesINTEL";
      case StructureType::eQueryPoolPerformanceQueryCreateInfoINTEL : return "QueryPoolPerformanceQueryCreateInfoINTEL";
      case StructureType::eInitializePerformanceApiInfoINTEL : return "InitializePerformanceApiInfoINTEL";
      case StructureType::ePerformanceMarkerInfoINTEL : return "PerformanceMarkerInfoINTEL";
      case StructureType::ePerformanceStreamMarkerInfoINTEL : return "PerformanceStreamMarkerInfoINTEL";
      case StructureType::ePerformanceOverrideInfoINTEL : return "PerformanceOverrideInfoINTEL";
      case StructureType::ePerformanceConfigurationAcquireInfoINTEL : return "PerformanceConfigurationAcquireInfoINTEL";
      case StructureType::ePhysicalDevicePciBusInfoPropertiesEXT : return "PhysicalDevicePciBusInfoPropertiesEXT";
      case StructureType::eDisplayNativeHdrSurfaceCapabilitiesAMD : return "DisplayNativeHdrSurfaceCapabilitiesAMD";
      case StructureType::eSwapchainDisplayNativeHdrCreateInfoAMD : return "SwapchainDisplayNativeHdrCreateInfoAMD";
#if defined( VK_USE_PLATFORM_FUCHSIA )
      case StructureType::eImagepipeSurfaceCreateInfoFUCHSIA : return "ImagepipeSurfaceCreateInfoFUCHSIA";
#endif /*VK_USE_PLATFORM_FUCHSIA*/
      case StructureType::ePhysicalDeviceShaderTerminateInvocationFeaturesKHR : return "PhysicalDeviceShaderTerminateInvocationFeaturesKHR";
#if defined( VK_USE_PLATFORM_METAL_EXT )
      case StructureType::eMetalSurfaceCreateInfoEXT : return "MetalSurfaceCreateInfoEXT";
#endif /*VK_USE_PLATFORM_METAL_EXT*/
      case StructureType::ePhysicalDeviceFragmentDensityMapFeaturesEXT : return "PhysicalDeviceFragmentDensityMapFeaturesEXT";
      case StructureType::ePhysicalDeviceFragmentDensityMapPropertiesEXT : return "PhysicalDeviceFragmentDensityMapPropertiesEXT";
      case StructureType::eRenderPassFragmentDensityMapCreateInfoEXT : return "RenderPassFragmentDensityMapCreateInfoEXT";
      case StructureType::ePhysicalDeviceSubgroupSizeControlPropertiesEXT : return "PhysicalDeviceSubgroupSizeControlPropertiesEXT";
      case StructureType::ePipelineShaderStageRequiredSubgroupSizeCreateInfoEXT : return "PipelineShaderStageRequiredSubgroupSizeCreateInfoEXT";
      case StructureType::ePhysicalDeviceSubgroupSizeControlFeaturesEXT : return "PhysicalDeviceSubgroupSizeControlFeaturesEXT";
      case StructureType::eFragmentShadingRateAttachmentInfoKHR : return "FragmentShadingRateAttachmentInfoKHR";
      case StructureType::ePipelineFragmentShadingRateStateCreateInfoKHR : return "PipelineFragmentShadingRateStateCreateInfoKHR";
      case StructureType::ePhysicalDeviceFragmentShadingRatePropertiesKHR : return "PhysicalDeviceFragmentShadingRatePropertiesKHR";
      case StructureType::ePhysicalDeviceFragmentShadingRateFeaturesKHR : return "PhysicalDeviceFragmentShadingRateFeaturesKHR";
      case StructureType::ePhysicalDeviceFragmentShadingRateKHR : return "PhysicalDeviceFragmentShadingRateKHR";
      case StructureType::ePhysicalDeviceShaderCoreProperties2AMD : return "PhysicalDeviceShaderCoreProperties2AMD";
      case StructureType::ePhysicalDeviceCoherentMemoryFeaturesAMD : return "PhysicalDeviceCoherentMemoryFeaturesAMD";
      case StructureType::ePhysicalDeviceShaderImageAtomicInt64FeaturesEXT : return "PhysicalDeviceShaderImageAtomicInt64FeaturesEXT";
      case StructureType::ePhysicalDeviceMemoryBudgetPropertiesEXT : return "PhysicalDeviceMemoryBudgetPropertiesEXT";
      case StructureType::ePhysicalDeviceMemoryPriorityFeaturesEXT : return "PhysicalDeviceMemoryPriorityFeaturesEXT";
      case StructureType::eMemoryPriorityAllocateInfoEXT : return "MemoryPriorityAllocateInfoEXT";
      case StructureType::eSurfaceProtectedCapabilitiesKHR : return "SurfaceProtectedCapabilitiesKHR";
      case StructureType::ePhysicalDeviceDedicatedAllocationImageAliasingFeaturesNV : return "PhysicalDeviceDedicatedAllocationImageAliasingFeaturesNV";
      case StructureType::ePhysicalDeviceBufferDeviceAddressFeaturesEXT : return "PhysicalDeviceBufferDeviceAddressFeaturesEXT";
      case StructureType::eBufferDeviceAddressCreateInfoEXT : return "BufferDeviceAddressCreateInfoEXT";
      case StructureType::ePhysicalDeviceToolPropertiesEXT : return "PhysicalDeviceToolPropertiesEXT";
      case StructureType::eValidationFeaturesEXT : return "ValidationFeaturesEXT";
      case StructureType::ePhysicalDevicePresentWaitFeaturesKHR : return "PhysicalDevicePresentWaitFeaturesKHR";
      case StructureType::ePhysicalDeviceCooperativeMatrixFeaturesNV : return "PhysicalDeviceCooperativeMatrixFeaturesNV";
      case StructureType::eCooperativeMatrixPropertiesNV : return "CooperativeMatrixPropertiesNV";
      case StructureType::ePhysicalDeviceCooperativeMatrixPropertiesNV : return "PhysicalDeviceCooperativeMatrixPropertiesNV";
      case StructureType::ePhysicalDeviceCoverageReductionModeFeaturesNV : return "PhysicalDeviceCoverageReductionModeFeaturesNV";
      case StructureType::ePipelineCoverageReductionStateCreateInfoNV : return "PipelineCoverageReductionStateCreateInfoNV";
      case StructureType::eFramebufferMixedSamplesCombinationNV : return "FramebufferMixedSamplesCombinationNV";
      case StructureType::ePhysicalDeviceFragmentShaderInterlockFeaturesEXT : return "PhysicalDeviceFragmentShaderInterlockFeaturesEXT";
      case StructureType::ePhysicalDeviceYcbcrImageArraysFeaturesEXT : return "PhysicalDeviceYcbcrImageArraysFeaturesEXT";
      case StructureType::ePhysicalDeviceProvokingVertexFeaturesEXT : return "PhysicalDeviceProvokingVertexFeaturesEXT";
      case StructureType::ePipelineRasterizationProvokingVertexStateCreateInfoEXT : return "PipelineRasterizationProvokingVertexStateCreateInfoEXT";
      case StructureType::ePhysicalDeviceProvokingVertexPropertiesEXT : return "PhysicalDeviceProvokingVertexPropertiesEXT";
#if defined( VK_USE_PLATFORM_WIN32_KHR )
      case StructureType::eSurfaceFullScreenExclusiveInfoEXT : return "SurfaceFullScreenExclusiveInfoEXT";
      case StructureType::eSurfaceCapabilitiesFullScreenExclusiveEXT : return "SurfaceCapabilitiesFullScreenExclusiveEXT";
      case StructureType::eSurfaceFullScreenExclusiveWin32InfoEXT : return "SurfaceFullScreenExclusiveWin32InfoEXT";
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
      case StructureType::eHeadlessSurfaceCreateInfoEXT : return "HeadlessSurfaceCreateInfoEXT";
      case StructureType::ePhysicalDeviceLineRasterizationFeaturesEXT : return "PhysicalDeviceLineRasterizationFeaturesEXT";
      case StructureType::ePipelineRasterizationLineStateCreateInfoEXT : return "PipelineRasterizationLineStateCreateInfoEXT";
      case StructureType::ePhysicalDeviceLineRasterizationPropertiesEXT : return "PhysicalDeviceLineRasterizationPropertiesEXT";
      case StructureType::ePhysicalDeviceShaderAtomicFloatFeaturesEXT : return "PhysicalDeviceShaderAtomicFloatFeaturesEXT";
      case StructureType::ePhysicalDeviceIndexTypeUint8FeaturesEXT : return "PhysicalDeviceIndexTypeUint8FeaturesEXT";
      case StructureType::ePhysicalDeviceExtendedDynamicStateFeaturesEXT : return "PhysicalDeviceExtendedDynamicStateFeaturesEXT";
      case StructureType::ePhysicalDevicePipelineExecutablePropertiesFeaturesKHR : return "PhysicalDevicePipelineExecutablePropertiesFeaturesKHR";
      case StructureType::ePipelineInfoKHR : return "PipelineInfoKHR";
      case StructureType::ePipelineExecutablePropertiesKHR : return "PipelineExecutablePropertiesKHR";
      case StructureType::ePipelineExecutableInfoKHR : return "PipelineExecutableInfoKHR";
      case StructureType::ePipelineExecutableStatisticKHR : return "PipelineExecutableStatisticKHR";
      case StructureType::ePipelineExecutableInternalRepresentationKHR : return "PipelineExecutableInternalRepresentationKHR";
      case StructureType::ePhysicalDeviceShaderAtomicFloat2FeaturesEXT : return "PhysicalDeviceShaderAtomicFloat2FeaturesEXT";
      case StructureType::ePhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT : return "PhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT";
      case StructureType::ePhysicalDeviceDeviceGeneratedCommandsPropertiesNV : return "PhysicalDeviceDeviceGeneratedCommandsPropertiesNV";
      case StructureType::eGraphicsShaderGroupCreateInfoNV : return "GraphicsShaderGroupCreateInfoNV";
      case StructureType::eGraphicsPipelineShaderGroupsCreateInfoNV : return "GraphicsPipelineShaderGroupsCreateInfoNV";
      case StructureType::eIndirectCommandsLayoutTokenNV : return "IndirectCommandsLayoutTokenNV";
      case StructureType::eIndirectCommandsLayoutCreateInfoNV : return "IndirectCommandsLayoutCreateInfoNV";
      case StructureType::eGeneratedCommandsInfoNV : return "GeneratedCommandsInfoNV";
      case StructureType::eGeneratedCommandsMemoryRequirementsInfoNV : return "GeneratedCommandsMemoryRequirementsInfoNV";
      case StructureType::ePhysicalDeviceDeviceGeneratedCommandsFeaturesNV : return "PhysicalDeviceDeviceGeneratedCommandsFeaturesNV";
      case StructureType::ePhysicalDeviceInheritedViewportScissorFeaturesNV : return "PhysicalDeviceInheritedViewportScissorFeaturesNV";
      case StructureType::eCommandBufferInheritanceViewportScissorInfoNV : return "CommandBufferInheritanceViewportScissorInfoNV";
      case StructureType::ePhysicalDeviceShaderIntegerDotProductFeaturesKHR : return "PhysicalDeviceShaderIntegerDotProductFeaturesKHR";
      case StructureType::ePhysicalDeviceShaderIntegerDotProductPropertiesKHR : return "PhysicalDeviceShaderIntegerDotProductPropertiesKHR";
      case StructureType::ePhysicalDeviceTexelBufferAlignmentFeaturesEXT : return "PhysicalDeviceTexelBufferAlignmentFeaturesEXT";
      case StructureType::ePhysicalDeviceTexelBufferAlignmentPropertiesEXT : return "PhysicalDeviceTexelBufferAlignmentPropertiesEXT";
      case StructureType::eCommandBufferInheritanceRenderPassTransformInfoQCOM : return "CommandBufferInheritanceRenderPassTransformInfoQCOM";
      case StructureType::eRenderPassTransformBeginInfoQCOM : return "RenderPassTransformBeginInfoQCOM";
      case StructureType::ePhysicalDeviceDeviceMemoryReportFeaturesEXT : return "PhysicalDeviceDeviceMemoryReportFeaturesEXT";
      case StructureType::eDeviceDeviceMemoryReportCreateInfoEXT : return "DeviceDeviceMemoryReportCreateInfoEXT";
      case StructureType::eDeviceMemoryReportCallbackDataEXT : return "DeviceMemoryReportCallbackDataEXT";
      case StructureType::ePhysicalDeviceRobustness2FeaturesEXT : return "PhysicalDeviceRobustness2FeaturesEXT";
      case StructureType::ePhysicalDeviceRobustness2PropertiesEXT : return "PhysicalDeviceRobustness2PropertiesEXT";
      case StructureType::eSamplerCustomBorderColorCreateInfoEXT : return "SamplerCustomBorderColorCreateInfoEXT";
      case StructureType::ePhysicalDeviceCustomBorderColorPropertiesEXT : return "PhysicalDeviceCustomBorderColorPropertiesEXT";
      case StructureType::ePhysicalDeviceCustomBorderColorFeaturesEXT : return "PhysicalDeviceCustomBorderColorFeaturesEXT";
      case StructureType::ePipelineLibraryCreateInfoKHR : return "PipelineLibraryCreateInfoKHR";
      case StructureType::ePresentIdKHR : return "PresentIdKHR";
      case StructureType::ePhysicalDevicePresentIdFeaturesKHR : return "PhysicalDevicePresentIdFeaturesKHR";
      case StructureType::ePhysicalDevicePrivateDataFeaturesEXT : return "PhysicalDevicePrivateDataFeaturesEXT";
      case StructureType::eDevicePrivateDataCreateInfoEXT : return "DevicePrivateDataCreateInfoEXT";
      case StructureType::ePrivateDataSlotCreateInfoEXT : return "PrivateDataSlotCreateInfoEXT";
      case StructureType::ePhysicalDevicePipelineCreationCacheControlFeaturesEXT : return "PhysicalDevicePipelineCreationCacheControlFeaturesEXT";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
      case StructureType::eVideoEncodeInfoKHR : return "VideoEncodeInfoKHR";
      case StructureType::eVideoEncodeRateControlInfoKHR : return "VideoEncodeRateControlInfoKHR";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
      case StructureType::ePhysicalDeviceDiagnosticsConfigFeaturesNV : return "PhysicalDeviceDiagnosticsConfigFeaturesNV";
      case StructureType::eDeviceDiagnosticsConfigCreateInfoNV : return "DeviceDiagnosticsConfigCreateInfoNV";
      case StructureType::eMemoryBarrier2KHR : return "MemoryBarrier2KHR";
      case StructureType::eBufferMemoryBarrier2KHR : return "BufferMemoryBarrier2KHR";
      case StructureType::eImageMemoryBarrier2KHR : return "ImageMemoryBarrier2KHR";
      case StructureType::eDependencyInfoKHR : return "DependencyInfoKHR";
      case StructureType::eSubmitInfo2KHR : return "SubmitInfo2KHR";
      case StructureType::eSemaphoreSubmitInfoKHR : return "SemaphoreSubmitInfoKHR";
      case StructureType::eCommandBufferSubmitInfoKHR : return "CommandBufferSubmitInfoKHR";
      case StructureType::ePhysicalDeviceSynchronization2FeaturesKHR : return "PhysicalDeviceSynchronization2FeaturesKHR";
      case StructureType::eQueueFamilyCheckpointProperties2NV : return "QueueFamilyCheckpointProperties2NV";
      case StructureType::eCheckpointData2NV : return "CheckpointData2NV";
      case StructureType::ePhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR : return "PhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR";
      case StructureType::ePhysicalDeviceZeroInitializeWorkgroupMemoryFeaturesKHR : return "PhysicalDeviceZeroInitializeWorkgroupMemoryFeaturesKHR";
      case StructureType::ePhysicalDeviceFragmentShadingRateEnumsPropertiesNV : return "PhysicalDeviceFragmentShadingRateEnumsPropertiesNV";
      case StructureType::ePhysicalDeviceFragmentShadingRateEnumsFeaturesNV : return "PhysicalDeviceFragmentShadingRateEnumsFeaturesNV";
      case StructureType::ePipelineFragmentShadingRateEnumStateCreateInfoNV : return "PipelineFragmentShadingRateEnumStateCreateInfoNV";
      case StructureType::eAccelerationStructureGeometryMotionTrianglesDataNV : return "AccelerationStructureGeometryMotionTrianglesDataNV";
      case StructureType::ePhysicalDeviceRayTracingMotionBlurFeaturesNV : return "PhysicalDeviceRayTracingMotionBlurFeaturesNV";
      case StructureType::eAccelerationStructureMotionInfoNV : return "AccelerationStructureMotionInfoNV";
      case StructureType::ePhysicalDeviceYcbcr2Plane444FormatsFeaturesEXT : return "PhysicalDeviceYcbcr2Plane444FormatsFeaturesEXT";
      case StructureType::ePhysicalDeviceFragmentDensityMap2FeaturesEXT : return "PhysicalDeviceFragmentDensityMap2FeaturesEXT";
      case StructureType::ePhysicalDeviceFragmentDensityMap2PropertiesEXT : return "PhysicalDeviceFragmentDensityMap2PropertiesEXT";
      case StructureType::eCopyCommandTransformInfoQCOM : return "CopyCommandTransformInfoQCOM";
      case StructureType::ePhysicalDeviceImageRobustnessFeaturesEXT : return "PhysicalDeviceImageRobustnessFeaturesEXT";
      case StructureType::ePhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR : return "PhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR";
      case StructureType::eCopyBufferInfo2KHR : return "CopyBufferInfo2KHR";
      case StructureType::eCopyImageInfo2KHR : return "CopyImageInfo2KHR";
      case StructureType::eCopyBufferToImageInfo2KHR : return "CopyBufferToImageInfo2KHR";
      case StructureType::eCopyImageToBufferInfo2KHR : return "CopyImageToBufferInfo2KHR";
      case StructureType::eBlitImageInfo2KHR : return "BlitImageInfo2KHR";
      case StructureType::eResolveImageInfo2KHR : return "ResolveImageInfo2KHR";
      case StructureType::eBufferCopy2KHR : return "BufferCopy2KHR";
      case StructureType::eImageCopy2KHR : return "ImageCopy2KHR";
      case StructureType::eImageBlit2KHR : return "ImageBlit2KHR";
      case StructureType::eBufferImageCopy2KHR : return "BufferImageCopy2KHR";
      case StructureType::eImageResolve2KHR : return "ImageResolve2KHR";
      case StructureType::ePhysicalDevice4444FormatsFeaturesEXT : return "PhysicalDevice4444FormatsFeaturesEXT";
      case StructureType::ePhysicalDeviceRgba10X6FormatsFeaturesEXT : return "PhysicalDeviceRgba10X6FormatsFeaturesEXT";
#if defined( VK_USE_PLATFORM_DIRECTFB_EXT )
      case StructureType::eDirectfbSurfaceCreateInfoEXT : return "DirectfbSurfaceCreateInfoEXT";
#endif /*VK_USE_PLATFORM_DIRECTFB_EXT*/
      case StructureType::ePhysicalDeviceMutableDescriptorTypeFeaturesVALVE : return "PhysicalDeviceMutableDescriptorTypeFeaturesVALVE";
      case StructureType::eMutableDescriptorTypeCreateInfoVALVE : return "MutableDescriptorTypeCreateInfoVALVE";
      case StructureType::ePhysicalDeviceVertexInputDynamicStateFeaturesEXT : return "PhysicalDeviceVertexInputDynamicStateFeaturesEXT";
      case StructureType::eVertexInputBindingDescription2EXT : return "VertexInputBindingDescription2EXT";
      case StructureType::eVertexInputAttributeDescription2EXT : return "VertexInputAttributeDescription2EXT";
      case StructureType::ePhysicalDeviceDrmPropertiesEXT : return "PhysicalDeviceDrmPropertiesEXT";
      case StructureType::ePhysicalDevicePrimitiveTopologyListRestartFeaturesEXT : return "PhysicalDevicePrimitiveTopologyListRestartFeaturesEXT";
      case StructureType::eFormatProperties3KHR : return "FormatProperties3KHR";
#if defined( VK_USE_PLATFORM_FUCHSIA )
      case StructureType::eImportMemoryZirconHandleInfoFUCHSIA : return "ImportMemoryZirconHandleInfoFUCHSIA";
      case StructureType::eMemoryZirconHandlePropertiesFUCHSIA : return "MemoryZirconHandlePropertiesFUCHSIA";
      case StructureType::eMemoryGetZirconHandleInfoFUCHSIA : return "MemoryGetZirconHandleInfoFUCHSIA";
      case StructureType::eImportSemaphoreZirconHandleInfoFUCHSIA : return "ImportSemaphoreZirconHandleInfoFUCHSIA";
      case StructureType::eSemaphoreGetZirconHandleInfoFUCHSIA : return "SemaphoreGetZirconHandleInfoFUCHSIA";
      case StructureType::eBufferCollectionCreateInfoFUCHSIA : return "BufferCollectionCreateInfoFUCHSIA";
      case StructureType::eImportMemoryBufferCollectionFUCHSIA : return "ImportMemoryBufferCollectionFUCHSIA";
      case StructureType::eBufferCollectionImageCreateInfoFUCHSIA : return "BufferCollectionImageCreateInfoFUCHSIA";
      case StructureType::eBufferCollectionPropertiesFUCHSIA : return "BufferCollectionPropertiesFUCHSIA";
      case StructureType::eBufferConstraintsInfoFUCHSIA : return "BufferConstraintsInfoFUCHSIA";
      case StructureType::eBufferCollectionBufferCreateInfoFUCHSIA : return "BufferCollectionBufferCreateInfoFUCHSIA";
      case StructureType::eImageConstraintsInfoFUCHSIA : return "ImageConstraintsInfoFUCHSIA";
      case StructureType::eImageFormatConstraintsInfoFUCHSIA : return "ImageFormatConstraintsInfoFUCHSIA";
      case StructureType::eSysmemColorSpaceFUCHSIA : return "SysmemColorSpaceFUCHSIA";
      case StructureType::eBufferCollectionConstraintsInfoFUCHSIA : return "BufferCollectionConstraintsInfoFUCHSIA";
#endif /*VK_USE_PLATFORM_FUCHSIA*/
      case StructureType::eSubpassShadingPipelineCreateInfoHUAWEI : return "SubpassShadingPipelineCreateInfoHUAWEI";
      case StructureType::ePhysicalDeviceSubpassShadingFeaturesHUAWEI : return "PhysicalDeviceSubpassShadingFeaturesHUAWEI";
      case StructureType::ePhysicalDeviceSubpassShadingPropertiesHUAWEI : return "PhysicalDeviceSubpassShadingPropertiesHUAWEI";
      case StructureType::ePhysicalDeviceInvocationMaskFeaturesHUAWEI : return "PhysicalDeviceInvocationMaskFeaturesHUAWEI";
      case StructureType::eMemoryGetRemoteAddressInfoNV : return "MemoryGetRemoteAddressInfoNV";
      case StructureType::ePhysicalDeviceExternalMemoryRdmaFeaturesNV : return "PhysicalDeviceExternalMemoryRdmaFeaturesNV";
      case StructureType::ePhysicalDeviceExtendedDynamicState2FeaturesEXT : return "PhysicalDeviceExtendedDynamicState2FeaturesEXT";
#if defined( VK_USE_PLATFORM_SCREEN_QNX )
      case StructureType::eScreenSurfaceCreateInfoQNX : return "ScreenSurfaceCreateInfoQNX";
#endif /*VK_USE_PLATFORM_SCREEN_QNX*/
      case StructureType::ePhysicalDeviceColorWriteEnableFeaturesEXT : return "PhysicalDeviceColorWriteEnableFeaturesEXT";
      case StructureType::ePipelineColorWriteCreateInfoEXT : return "PipelineColorWriteCreateInfoEXT";
      case StructureType::ePhysicalDeviceGlobalPriorityQueryFeaturesEXT : return "PhysicalDeviceGlobalPriorityQueryFeaturesEXT";
      case StructureType::eQueueFamilyGlobalPriorityPropertiesEXT : return "QueueFamilyGlobalPriorityPropertiesEXT";
      case StructureType::ePhysicalDeviceMultiDrawFeaturesEXT : return "PhysicalDeviceMultiDrawFeaturesEXT";
      case StructureType::ePhysicalDeviceMultiDrawPropertiesEXT : return "PhysicalDeviceMultiDrawPropertiesEXT";
      case StructureType::ePhysicalDeviceBorderColorSwizzleFeaturesEXT : return "PhysicalDeviceBorderColorSwizzleFeaturesEXT";
      case StructureType::eSamplerBorderColorComponentMappingCreateInfoEXT : return "SamplerBorderColorComponentMappingCreateInfoEXT";
      case StructureType::ePhysicalDevicePageableDeviceLocalMemoryFeaturesEXT : return "PhysicalDevicePageableDeviceLocalMemoryFeaturesEXT";
      case StructureType::ePhysicalDeviceMaintenance4FeaturesKHR : return "PhysicalDeviceMaintenance4FeaturesKHR";
      case StructureType::ePhysicalDeviceMaintenance4PropertiesKHR : return "PhysicalDeviceMaintenance4PropertiesKHR";
      case StructureType::eDeviceBufferMemoryRequirementsKHR : return "DeviceBufferMemoryRequirementsKHR";
      case StructureType::eDeviceImageMemoryRequirementsKHR : return "DeviceImageMemoryRequirementsKHR";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( ObjectType value )
  {
    switch ( value )
    {
      case ObjectType::eUnknown : return "Unknown";
      case ObjectType::eInstance : return "Instance";
      case ObjectType::ePhysicalDevice : return "PhysicalDevice";
      case ObjectType::eDevice : return "Device";
      case ObjectType::eQueue : return "Queue";
      case ObjectType::eSemaphore : return "Semaphore";
      case ObjectType::eCommandBuffer : return "CommandBuffer";
      case ObjectType::eFence : return "Fence";
      case ObjectType::eDeviceMemory : return "DeviceMemory";
      case ObjectType::eBuffer : return "Buffer";
      case ObjectType::eImage : return "Image";
      case ObjectType::eEvent : return "Event";
      case ObjectType::eQueryPool : return "QueryPool";
      case ObjectType::eBufferView : return "BufferView";
      case ObjectType::eImageView : return "ImageView";
      case ObjectType::eShaderModule : return "ShaderModule";
      case ObjectType::ePipelineCache : return "PipelineCache";
      case ObjectType::ePipelineLayout : return "PipelineLayout";
      case ObjectType::eRenderPass : return "RenderPass";
      case ObjectType::ePipeline : return "Pipeline";
      case ObjectType::eDescriptorSetLayout : return "DescriptorSetLayout";
      case ObjectType::eSampler : return "Sampler";
      case ObjectType::eDescriptorPool : return "DescriptorPool";
      case ObjectType::eDescriptorSet : return "DescriptorSet";
      case ObjectType::eFramebuffer : return "Framebuffer";
      case ObjectType::eCommandPool : return "CommandPool";
      case ObjectType::eSamplerYcbcrConversion : return "SamplerYcbcrConversion";
      case ObjectType::eDescriptorUpdateTemplate : return "DescriptorUpdateTemplate";
      case ObjectType::eSurfaceKHR : return "SurfaceKHR";
      case ObjectType::eSwapchainKHR : return "SwapchainKHR";
      case ObjectType::eDisplayKHR : return "DisplayKHR";
      case ObjectType::eDisplayModeKHR : return "DisplayModeKHR";
      case ObjectType::eDebugReportCallbackEXT : return "DebugReportCallbackEXT";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
      case ObjectType::eVideoSessionKHR : return "VideoSessionKHR";
      case ObjectType::eVideoSessionParametersKHR : return "VideoSessionParametersKHR";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
      case ObjectType::eCuModuleNVX : return "CuModuleNVX";
      case ObjectType::eCuFunctionNVX : return "CuFunctionNVX";
      case ObjectType::eDebugUtilsMessengerEXT : return "DebugUtilsMessengerEXT";
      case ObjectType::eAccelerationStructureKHR : return "AccelerationStructureKHR";
      case ObjectType::eValidationCacheEXT : return "ValidationCacheEXT";
      case ObjectType::eAccelerationStructureNV : return "AccelerationStructureNV";
      case ObjectType::ePerformanceConfigurationINTEL : return "PerformanceConfigurationINTEL";
      case ObjectType::eDeferredOperationKHR : return "DeferredOperationKHR";
      case ObjectType::eIndirectCommandsLayoutNV : return "IndirectCommandsLayoutNV";
      case ObjectType::ePrivateDataSlotEXT : return "PrivateDataSlotEXT";
#if defined( VK_USE_PLATFORM_FUCHSIA )
      case ObjectType::eBufferCollectionFUCHSIA : return "BufferCollectionFUCHSIA";
#endif /*VK_USE_PLATFORM_FUCHSIA*/
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( VendorId value )
  {
    switch ( value )
    {
      case VendorId::eVIV : return "VIV";
      case VendorId::eVSI : return "VSI";
      case VendorId::eKazan : return "Kazan";
      case VendorId::eCodeplay : return "Codeplay";
      case VendorId::eMESA : return "MESA";
      case VendorId::ePocl : return "Pocl";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( PipelineCacheHeaderVersion value )
  {
    switch ( value )
    {
      case PipelineCacheHeaderVersion::eOne : return "One";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( Format value )
  {
    switch ( value )
    {
      case Format::eUndefined : return "Undefined";
      case Format::eR4G4UnormPack8 : return "R4G4UnormPack8";
      case Format::eR4G4B4A4UnormPack16 : return "R4G4B4A4UnormPack16";
      case Format::eB4G4R4A4UnormPack16 : return "B4G4R4A4UnormPack16";
      case Format::eR5G6B5UnormPack16 : return "R5G6B5UnormPack16";
      case Format::eB5G6R5UnormPack16 : return "B5G6R5UnormPack16";
      case Format::eR5G5B5A1UnormPack16 : return "R5G5B5A1UnormPack16";
      case Format::eB5G5R5A1UnormPack16 : return "B5G5R5A1UnormPack16";
      case Format::eA1R5G5B5UnormPack16 : return "A1R5G5B5UnormPack16";
      case Format::eR8Unorm : return "R8Unorm";
      case Format::eR8Snorm : return "R8Snorm";
      case Format::eR8Uscaled : return "R8Uscaled";
      case Format::eR8Sscaled : return "R8Sscaled";
      case Format::eR8Uint : return "R8Uint";
      case Format::eR8Sint : return "R8Sint";
      case Format::eR8Srgb : return "R8Srgb";
      case Format::eR8G8Unorm : return "R8G8Unorm";
      case Format::eR8G8Snorm : return "R8G8Snorm";
      case Format::eR8G8Uscaled : return "R8G8Uscaled";
      case Format::eR8G8Sscaled : return "R8G8Sscaled";
      case Format::eR8G8Uint : return "R8G8Uint";
      case Format::eR8G8Sint : return "R8G8Sint";
      case Format::eR8G8Srgb : return "R8G8Srgb";
      case Format::eR8G8B8Unorm : return "R8G8B8Unorm";
      case Format::eR8G8B8Snorm : return "R8G8B8Snorm";
      case Format::eR8G8B8Uscaled : return "R8G8B8Uscaled";
      case Format::eR8G8B8Sscaled : return "R8G8B8Sscaled";
      case Format::eR8G8B8Uint : return "R8G8B8Uint";
      case Format::eR8G8B8Sint : return "R8G8B8Sint";
      case Format::eR8G8B8Srgb : return "R8G8B8Srgb";
      case Format::eB8G8R8Unorm : return "B8G8R8Unorm";
      case Format::eB8G8R8Snorm : return "B8G8R8Snorm";
      case Format::eB8G8R8Uscaled : return "B8G8R8Uscaled";
      case Format::eB8G8R8Sscaled : return "B8G8R8Sscaled";
      case Format::eB8G8R8Uint : return "B8G8R8Uint";
      case Format::eB8G8R8Sint : return "B8G8R8Sint";
      case Format::eB8G8R8Srgb : return "B8G8R8Srgb";
      case Format::eR8G8B8A8Unorm : return "R8G8B8A8Unorm";
      case Format::eR8G8B8A8Snorm : return "R8G8B8A8Snorm";
      case Format::eR8G8B8A8Uscaled : return "R8G8B8A8Uscaled";
      case Format::eR8G8B8A8Sscaled : return "R8G8B8A8Sscaled";
      case Format::eR8G8B8A8Uint : return "R8G8B8A8Uint";
      case Format::eR8G8B8A8Sint : return "R8G8B8A8Sint";
      case Format::eR8G8B8A8Srgb : return "R8G8B8A8Srgb";
      case Format::eB8G8R8A8Unorm : return "B8G8R8A8Unorm";
      case Format::eB8G8R8A8Snorm : return "B8G8R8A8Snorm";
      case Format::eB8G8R8A8Uscaled : return "B8G8R8A8Uscaled";
      case Format::eB8G8R8A8Sscaled : return "B8G8R8A8Sscaled";
      case Format::eB8G8R8A8Uint : return "B8G8R8A8Uint";
      case Format::eB8G8R8A8Sint : return "B8G8R8A8Sint";
      case Format::eB8G8R8A8Srgb : return "B8G8R8A8Srgb";
      case Format::eA8B8G8R8UnormPack32 : return "A8B8G8R8UnormPack32";
      case Format::eA8B8G8R8SnormPack32 : return "A8B8G8R8SnormPack32";
      case Format::eA8B8G8R8UscaledPack32 : return "A8B8G8R8UscaledPack32";
      case Format::eA8B8G8R8SscaledPack32 : return "A8B8G8R8SscaledPack32";
      case Format::eA8B8G8R8UintPack32 : return "A8B8G8R8UintPack32";
      case Format::eA8B8G8R8SintPack32 : return "A8B8G8R8SintPack32";
      case Format::eA8B8G8R8SrgbPack32 : return "A8B8G8R8SrgbPack32";
      case Format::eA2R10G10B10UnormPack32 : return "A2R10G10B10UnormPack32";
      case Format::eA2R10G10B10SnormPack32 : return "A2R10G10B10SnormPack32";
      case Format::eA2R10G10B10UscaledPack32 : return "A2R10G10B10UscaledPack32";
      case Format::eA2R10G10B10SscaledPack32 : return "A2R10G10B10SscaledPack32";
      case Format::eA2R10G10B10UintPack32 : return "A2R10G10B10UintPack32";
      case Format::eA2R10G10B10SintPack32 : return "A2R10G10B10SintPack32";
      case Format::eA2B10G10R10UnormPack32 : return "A2B10G10R10UnormPack32";
      case Format::eA2B10G10R10SnormPack32 : return "A2B10G10R10SnormPack32";
      case Format::eA2B10G10R10UscaledPack32 : return "A2B10G10R10UscaledPack32";
      case Format::eA2B10G10R10SscaledPack32 : return "A2B10G10R10SscaledPack32";
      case Format::eA2B10G10R10UintPack32 : return "A2B10G10R10UintPack32";
      case Format::eA2B10G10R10SintPack32 : return "A2B10G10R10SintPack32";
      case Format::eR16Unorm : return "R16Unorm";
      case Format::eR16Snorm : return "R16Snorm";
      case Format::eR16Uscaled : return "R16Uscaled";
      case Format::eR16Sscaled : return "R16Sscaled";
      case Format::eR16Uint : return "R16Uint";
      case Format::eR16Sint : return "R16Sint";
      case Format::eR16Sfloat : return "R16Sfloat";
      case Format::eR16G16Unorm : return "R16G16Unorm";
      case Format::eR16G16Snorm : return "R16G16Snorm";
      case Format::eR16G16Uscaled : return "R16G16Uscaled";
      case Format::eR16G16Sscaled : return "R16G16Sscaled";
      case Format::eR16G16Uint : return "R16G16Uint";
      case Format::eR16G16Sint : return "R16G16Sint";
      case Format::eR16G16Sfloat : return "R16G16Sfloat";
      case Format::eR16G16B16Unorm : return "R16G16B16Unorm";
      case Format::eR16G16B16Snorm : return "R16G16B16Snorm";
      case Format::eR16G16B16Uscaled : return "R16G16B16Uscaled";
      case Format::eR16G16B16Sscaled : return "R16G16B16Sscaled";
      case Format::eR16G16B16Uint : return "R16G16B16Uint";
      case Format::eR16G16B16Sint : return "R16G16B16Sint";
      case Format::eR16G16B16Sfloat : return "R16G16B16Sfloat";
      case Format::eR16G16B16A16Unorm : return "R16G16B16A16Unorm";
      case Format::eR16G16B16A16Snorm : return "R16G16B16A16Snorm";
      case Format::eR16G16B16A16Uscaled : return "R16G16B16A16Uscaled";
      case Format::eR16G16B16A16Sscaled : return "R16G16B16A16Sscaled";
      case Format::eR16G16B16A16Uint : return "R16G16B16A16Uint";
      case Format::eR16G16B16A16Sint : return "R16G16B16A16Sint";
      case Format::eR16G16B16A16Sfloat : return "R16G16B16A16Sfloat";
      case Format::eR32Uint : return "R32Uint";
      case Format::eR32Sint : return "R32Sint";
      case Format::eR32Sfloat : return "R32Sfloat";
      case Format::eR32G32Uint : return "R32G32Uint";
      case Format::eR32G32Sint : return "R32G32Sint";
      case Format::eR32G32Sfloat : return "R32G32Sfloat";
      case Format::eR32G32B32Uint : return "R32G32B32Uint";
      case Format::eR32G32B32Sint : return "R32G32B32Sint";
      case Format::eR32G32B32Sfloat : return "R32G32B32Sfloat";
      case Format::eR32G32B32A32Uint : return "R32G32B32A32Uint";
      case Format::eR32G32B32A32Sint : return "R32G32B32A32Sint";
      case Format::eR32G32B32A32Sfloat : return "R32G32B32A32Sfloat";
      case Format::eR64Uint : return "R64Uint";
      case Format::eR64Sint : return "R64Sint";
      case Format::eR64Sfloat : return "R64Sfloat";
      case Format::eR64G64Uint : return "R64G64Uint";
      case Format::eR64G64Sint : return "R64G64Sint";
      case Format::eR64G64Sfloat : return "R64G64Sfloat";
      case Format::eR64G64B64Uint : return "R64G64B64Uint";
      case Format::eR64G64B64Sint : return "R64G64B64Sint";
      case Format::eR64G64B64Sfloat : return "R64G64B64Sfloat";
      case Format::eR64G64B64A64Uint : return "R64G64B64A64Uint";
      case Format::eR64G64B64A64Sint : return "R64G64B64A64Sint";
      case Format::eR64G64B64A64Sfloat : return "R64G64B64A64Sfloat";
      case Format::eB10G11R11UfloatPack32 : return "B10G11R11UfloatPack32";
      case Format::eE5B9G9R9UfloatPack32 : return "E5B9G9R9UfloatPack32";
      case Format::eD16Unorm : return "D16Unorm";
      case Format::eX8D24UnormPack32 : return "X8D24UnormPack32";
      case Format::eD32Sfloat : return "D32Sfloat";
      case Format::eS8Uint : return "S8Uint";
      case Format::eD16UnormS8Uint : return "D16UnormS8Uint";
      case Format::eD24UnormS8Uint : return "D24UnormS8Uint";
      case Format::eD32SfloatS8Uint : return "D32SfloatS8Uint";
      case Format::eBc1RgbUnormBlock : return "Bc1RgbUnormBlock";
      case Format::eBc1RgbSrgbBlock : return "Bc1RgbSrgbBlock";
      case Format::eBc1RgbaUnormBlock : return "Bc1RgbaUnormBlock";
      case Format::eBc1RgbaSrgbBlock : return "Bc1RgbaSrgbBlock";
      case Format::eBc2UnormBlock : return "Bc2UnormBlock";
      case Format::eBc2SrgbBlock : return "Bc2SrgbBlock";
      case Format::eBc3UnormBlock : return "Bc3UnormBlock";
      case Format::eBc3SrgbBlock : return "Bc3SrgbBlock";
      case Format::eBc4UnormBlock : return "Bc4UnormBlock";
      case Format::eBc4SnormBlock : return "Bc4SnormBlock";
      case Format::eBc5UnormBlock : return "Bc5UnormBlock";
      case Format::eBc5SnormBlock : return "Bc5SnormBlock";
      case Format::eBc6HUfloatBlock : return "Bc6HUfloatBlock";
      case Format::eBc6HSfloatBlock : return "Bc6HSfloatBlock";
      case Format::eBc7UnormBlock : return "Bc7UnormBlock";
      case Format::eBc7SrgbBlock : return "Bc7SrgbBlock";
      case Format::eEtc2R8G8B8UnormBlock : return "Etc2R8G8B8UnormBlock";
      case Format::eEtc2R8G8B8SrgbBlock : return "Etc2R8G8B8SrgbBlock";
      case Format::eEtc2R8G8B8A1UnormBlock : return "Etc2R8G8B8A1UnormBlock";
      case Format::eEtc2R8G8B8A1SrgbBlock : return "Etc2R8G8B8A1SrgbBlock";
      case Format::eEtc2R8G8B8A8UnormBlock : return "Etc2R8G8B8A8UnormBlock";
      case Format::eEtc2R8G8B8A8SrgbBlock : return "Etc2R8G8B8A8SrgbBlock";
      case Format::eEacR11UnormBlock : return "EacR11UnormBlock";
      case Format::eEacR11SnormBlock : return "EacR11SnormBlock";
      case Format::eEacR11G11UnormBlock : return "EacR11G11UnormBlock";
      case Format::eEacR11G11SnormBlock : return "EacR11G11SnormBlock";
      case Format::eAstc4x4UnormBlock : return "Astc4x4UnormBlock";
      case Format::eAstc4x4SrgbBlock : return "Astc4x4SrgbBlock";
      case Format::eAstc5x4UnormBlock : return "Astc5x4UnormBlock";
      case Format::eAstc5x4SrgbBlock : return "Astc5x4SrgbBlock";
      case Format::eAstc5x5UnormBlock : return "Astc5x5UnormBlock";
      case Format::eAstc5x5SrgbBlock : return "Astc5x5SrgbBlock";
      case Format::eAstc6x5UnormBlock : return "Astc6x5UnormBlock";
      case Format::eAstc6x5SrgbBlock : return "Astc6x5SrgbBlock";
      case Format::eAstc6x6UnormBlock : return "Astc6x6UnormBlock";
      case Format::eAstc6x6SrgbBlock : return "Astc6x6SrgbBlock";
      case Format::eAstc8x5UnormBlock : return "Astc8x5UnormBlock";
      case Format::eAstc8x5SrgbBlock : return "Astc8x5SrgbBlock";
      case Format::eAstc8x6UnormBlock : return "Astc8x6UnormBlock";
      case Format::eAstc8x6SrgbBlock : return "Astc8x6SrgbBlock";
      case Format::eAstc8x8UnormBlock : return "Astc8x8UnormBlock";
      case Format::eAstc8x8SrgbBlock : return "Astc8x8SrgbBlock";
      case Format::eAstc10x5UnormBlock : return "Astc10x5UnormBlock";
      case Format::eAstc10x5SrgbBlock : return "Astc10x5SrgbBlock";
      case Format::eAstc10x6UnormBlock : return "Astc10x6UnormBlock";
      case Format::eAstc10x6SrgbBlock : return "Astc10x6SrgbBlock";
      case Format::eAstc10x8UnormBlock : return "Astc10x8UnormBlock";
      case Format::eAstc10x8SrgbBlock : return "Astc10x8SrgbBlock";
      case Format::eAstc10x10UnormBlock : return "Astc10x10UnormBlock";
      case Format::eAstc10x10SrgbBlock : return "Astc10x10SrgbBlock";
      case Format::eAstc12x10UnormBlock : return "Astc12x10UnormBlock";
      case Format::eAstc12x10SrgbBlock : return "Astc12x10SrgbBlock";
      case Format::eAstc12x12UnormBlock : return "Astc12x12UnormBlock";
      case Format::eAstc12x12SrgbBlock : return "Astc12x12SrgbBlock";
      case Format::eG8B8G8R8422Unorm : return "G8B8G8R8422Unorm";
      case Format::eB8G8R8G8422Unorm : return "B8G8R8G8422Unorm";
      case Format::eG8B8R83Plane420Unorm : return "G8B8R83Plane420Unorm";
      case Format::eG8B8R82Plane420Unorm : return "G8B8R82Plane420Unorm";
      case Format::eG8B8R83Plane422Unorm : return "G8B8R83Plane422Unorm";
      case Format::eG8B8R82Plane422Unorm : return "G8B8R82Plane422Unorm";
      case Format::eG8B8R83Plane444Unorm : return "G8B8R83Plane444Unorm";
      case Format::eR10X6UnormPack16 : return "R10X6UnormPack16";
      case Format::eR10X6G10X6Unorm2Pack16 : return "R10X6G10X6Unorm2Pack16";
      case Format::eR10X6G10X6B10X6A10X6Unorm4Pack16 : return "R10X6G10X6B10X6A10X6Unorm4Pack16";
      case Format::eG10X6B10X6G10X6R10X6422Unorm4Pack16 : return "G10X6B10X6G10X6R10X6422Unorm4Pack16";
      case Format::eB10X6G10X6R10X6G10X6422Unorm4Pack16 : return "B10X6G10X6R10X6G10X6422Unorm4Pack16";
      case Format::eG10X6B10X6R10X63Plane420Unorm3Pack16 : return "G10X6B10X6R10X63Plane420Unorm3Pack16";
      case Format::eG10X6B10X6R10X62Plane420Unorm3Pack16 : return "G10X6B10X6R10X62Plane420Unorm3Pack16";
      case Format::eG10X6B10X6R10X63Plane422Unorm3Pack16 : return "G10X6B10X6R10X63Plane422Unorm3Pack16";
      case Format::eG10X6B10X6R10X62Plane422Unorm3Pack16 : return "G10X6B10X6R10X62Plane422Unorm3Pack16";
      case Format::eG10X6B10X6R10X63Plane444Unorm3Pack16 : return "G10X6B10X6R10X63Plane444Unorm3Pack16";
      case Format::eR12X4UnormPack16 : return "R12X4UnormPack16";
      case Format::eR12X4G12X4Unorm2Pack16 : return "R12X4G12X4Unorm2Pack16";
      case Format::eR12X4G12X4B12X4A12X4Unorm4Pack16 : return "R12X4G12X4B12X4A12X4Unorm4Pack16";
      case Format::eG12X4B12X4G12X4R12X4422Unorm4Pack16 : return "G12X4B12X4G12X4R12X4422Unorm4Pack16";
      case Format::eB12X4G12X4R12X4G12X4422Unorm4Pack16 : return "B12X4G12X4R12X4G12X4422Unorm4Pack16";
      case Format::eG12X4B12X4R12X43Plane420Unorm3Pack16 : return "G12X4B12X4R12X43Plane420Unorm3Pack16";
      case Format::eG12X4B12X4R12X42Plane420Unorm3Pack16 : return "G12X4B12X4R12X42Plane420Unorm3Pack16";
      case Format::eG12X4B12X4R12X43Plane422Unorm3Pack16 : return "G12X4B12X4R12X43Plane422Unorm3Pack16";
      case Format::eG12X4B12X4R12X42Plane422Unorm3Pack16 : return "G12X4B12X4R12X42Plane422Unorm3Pack16";
      case Format::eG12X4B12X4R12X43Plane444Unorm3Pack16 : return "G12X4B12X4R12X43Plane444Unorm3Pack16";
      case Format::eG16B16G16R16422Unorm : return "G16B16G16R16422Unorm";
      case Format::eB16G16R16G16422Unorm : return "B16G16R16G16422Unorm";
      case Format::eG16B16R163Plane420Unorm : return "G16B16R163Plane420Unorm";
      case Format::eG16B16R162Plane420Unorm : return "G16B16R162Plane420Unorm";
      case Format::eG16B16R163Plane422Unorm : return "G16B16R163Plane422Unorm";
      case Format::eG16B16R162Plane422Unorm : return "G16B16R162Plane422Unorm";
      case Format::eG16B16R163Plane444Unorm : return "G16B16R163Plane444Unorm";
      case Format::ePvrtc12BppUnormBlockIMG : return "Pvrtc12BppUnormBlockIMG";
      case Format::ePvrtc14BppUnormBlockIMG : return "Pvrtc14BppUnormBlockIMG";
      case Format::ePvrtc22BppUnormBlockIMG : return "Pvrtc22BppUnormBlockIMG";
      case Format::ePvrtc24BppUnormBlockIMG : return "Pvrtc24BppUnormBlockIMG";
      case Format::ePvrtc12BppSrgbBlockIMG : return "Pvrtc12BppSrgbBlockIMG";
      case Format::ePvrtc14BppSrgbBlockIMG : return "Pvrtc14BppSrgbBlockIMG";
      case Format::ePvrtc22BppSrgbBlockIMG : return "Pvrtc22BppSrgbBlockIMG";
      case Format::ePvrtc24BppSrgbBlockIMG : return "Pvrtc24BppSrgbBlockIMG";
      case Format::eAstc4x4SfloatBlockEXT : return "Astc4x4SfloatBlockEXT";
      case Format::eAstc5x4SfloatBlockEXT : return "Astc5x4SfloatBlockEXT";
      case Format::eAstc5x5SfloatBlockEXT : return "Astc5x5SfloatBlockEXT";
      case Format::eAstc6x5SfloatBlockEXT : return "Astc6x5SfloatBlockEXT";
      case Format::eAstc6x6SfloatBlockEXT : return "Astc6x6SfloatBlockEXT";
      case Format::eAstc8x5SfloatBlockEXT : return "Astc8x5SfloatBlockEXT";
      case Format::eAstc8x6SfloatBlockEXT : return "Astc8x6SfloatBlockEXT";
      case Format::eAstc8x8SfloatBlockEXT : return "Astc8x8SfloatBlockEXT";
      case Format::eAstc10x5SfloatBlockEXT : return "Astc10x5SfloatBlockEXT";
      case Format::eAstc10x6SfloatBlockEXT : return "Astc10x6SfloatBlockEXT";
      case Format::eAstc10x8SfloatBlockEXT : return "Astc10x8SfloatBlockEXT";
      case Format::eAstc10x10SfloatBlockEXT : return "Astc10x10SfloatBlockEXT";
      case Format::eAstc12x10SfloatBlockEXT : return "Astc12x10SfloatBlockEXT";
      case Format::eAstc12x12SfloatBlockEXT : return "Astc12x12SfloatBlockEXT";
      case Format::eG8B8R82Plane444UnormEXT : return "G8B8R82Plane444UnormEXT";
      case Format::eG10X6B10X6R10X62Plane444Unorm3Pack16EXT : return "G10X6B10X6R10X62Plane444Unorm3Pack16EXT";
      case Format::eG12X4B12X4R12X42Plane444Unorm3Pack16EXT : return "G12X4B12X4R12X42Plane444Unorm3Pack16EXT";
      case Format::eG16B16R162Plane444UnormEXT : return "G16B16R162Plane444UnormEXT";
      case Format::eA4R4G4B4UnormPack16EXT : return "A4R4G4B4UnormPack16EXT";
      case Format::eA4B4G4R4UnormPack16EXT : return "A4B4G4R4UnormPack16EXT";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( FormatFeatureFlagBits value )
  {
    switch ( value )
    {
      case FormatFeatureFlagBits::eSampledImage : return "SampledImage";
      case FormatFeatureFlagBits::eStorageImage : return "StorageImage";
      case FormatFeatureFlagBits::eStorageImageAtomic : return "StorageImageAtomic";
      case FormatFeatureFlagBits::eUniformTexelBuffer : return "UniformTexelBuffer";
      case FormatFeatureFlagBits::eStorageTexelBuffer : return "StorageTexelBuffer";
      case FormatFeatureFlagBits::eStorageTexelBufferAtomic : return "StorageTexelBufferAtomic";
      case FormatFeatureFlagBits::eVertexBuffer : return "VertexBuffer";
      case FormatFeatureFlagBits::eColorAttachment : return "ColorAttachment";
      case FormatFeatureFlagBits::eColorAttachmentBlend : return "ColorAttachmentBlend";
      case FormatFeatureFlagBits::eDepthStencilAttachment : return "DepthStencilAttachment";
      case FormatFeatureFlagBits::eBlitSrc : return "BlitSrc";
      case FormatFeatureFlagBits::eBlitDst : return "BlitDst";
      case FormatFeatureFlagBits::eSampledImageFilterLinear : return "SampledImageFilterLinear";
      case FormatFeatureFlagBits::eTransferSrc : return "TransferSrc";
      case FormatFeatureFlagBits::eTransferDst : return "TransferDst";
      case FormatFeatureFlagBits::eMidpointChromaSamples : return "MidpointChromaSamples";
      case FormatFeatureFlagBits::eSampledImageYcbcrConversionLinearFilter : return "SampledImageYcbcrConversionLinearFilter";
      case FormatFeatureFlagBits::eSampledImageYcbcrConversionSeparateReconstructionFilter : return "SampledImageYcbcrConversionSeparateReconstructionFilter";
      case FormatFeatureFlagBits::eSampledImageYcbcrConversionChromaReconstructionExplicit : return "SampledImageYcbcrConversionChromaReconstructionExplicit";
      case FormatFeatureFlagBits::eSampledImageYcbcrConversionChromaReconstructionExplicitForceable : return "SampledImageYcbcrConversionChromaReconstructionExplicitForceable";
      case FormatFeatureFlagBits::eDisjoint : return "Disjoint";
      case FormatFeatureFlagBits::eCositedChromaSamples : return "CositedChromaSamples";
      case FormatFeatureFlagBits::eSampledImageFilterMinmax : return "SampledImageFilterMinmax";
      case FormatFeatureFlagBits::eSampledImageFilterCubicIMG : return "SampledImageFilterCubicIMG";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
      case FormatFeatureFlagBits::eVideoDecodeOutputKHR : return "VideoDecodeOutputKHR";
      case FormatFeatureFlagBits::eVideoDecodeDpbKHR : return "VideoDecodeDpbKHR";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
      case FormatFeatureFlagBits::eAccelerationStructureVertexBufferKHR : return "AccelerationStructureVertexBufferKHR";
      case FormatFeatureFlagBits::eFragmentDensityMapEXT : return "FragmentDensityMapEXT";
      case FormatFeatureFlagBits::eFragmentShadingRateAttachmentKHR : return "FragmentShadingRateAttachmentKHR";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
      case FormatFeatureFlagBits::eVideoEncodeInputKHR : return "VideoEncodeInputKHR";
      case FormatFeatureFlagBits::eVideoEncodeDpbKHR : return "VideoEncodeDpbKHR";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( ImageCreateFlagBits value )
  {
    switch ( value )
    {
      case ImageCreateFlagBits::eSparseBinding : return "SparseBinding";
      case ImageCreateFlagBits::eSparseResidency : return "SparseResidency";
      case ImageCreateFlagBits::eSparseAliased : return "SparseAliased";
      case ImageCreateFlagBits::eMutableFormat : return "MutableFormat";
      case ImageCreateFlagBits::eCubeCompatible : return "CubeCompatible";
      case ImageCreateFlagBits::eAlias : return "Alias";
      case ImageCreateFlagBits::eSplitInstanceBindRegions : return "SplitInstanceBindRegions";
      case ImageCreateFlagBits::e2DArrayCompatible : return "2DArrayCompatible";
      case ImageCreateFlagBits::eBlockTexelViewCompatible : return "BlockTexelViewCompatible";
      case ImageCreateFlagBits::eExtendedUsage : return "ExtendedUsage";
      case ImageCreateFlagBits::eProtected : return "Protected";
      case ImageCreateFlagBits::eDisjoint : return "Disjoint";
      case ImageCreateFlagBits::eCornerSampledNV : return "CornerSampledNV";
      case ImageCreateFlagBits::eSampleLocationsCompatibleDepthEXT : return "SampleLocationsCompatibleDepthEXT";
      case ImageCreateFlagBits::eSubsampledEXT : return "SubsampledEXT";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( ImageTiling value )
  {
    switch ( value )
    {
      case ImageTiling::eOptimal : return "Optimal";
      case ImageTiling::eLinear : return "Linear";
      case ImageTiling::eDrmFormatModifierEXT : return "DrmFormatModifierEXT";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( ImageType value )
  {
    switch ( value )
    {
      case ImageType::e1D : return "1D";
      case ImageType::e2D : return "2D";
      case ImageType::e3D : return "3D";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( ImageUsageFlagBits value )
  {
    switch ( value )
    {
      case ImageUsageFlagBits::eTransferSrc : return "TransferSrc";
      case ImageUsageFlagBits::eTransferDst : return "TransferDst";
      case ImageUsageFlagBits::eSampled : return "Sampled";
      case ImageUsageFlagBits::eStorage : return "Storage";
      case ImageUsageFlagBits::eColorAttachment : return "ColorAttachment";
      case ImageUsageFlagBits::eDepthStencilAttachment : return "DepthStencilAttachment";
      case ImageUsageFlagBits::eTransientAttachment : return "TransientAttachment";
      case ImageUsageFlagBits::eInputAttachment : return "InputAttachment";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
      case ImageUsageFlagBits::eVideoDecodeDstKHR : return "VideoDecodeDstKHR";
      case ImageUsageFlagBits::eVideoDecodeSrcKHR : return "VideoDecodeSrcKHR";
      case ImageUsageFlagBits::eVideoDecodeDpbKHR : return "VideoDecodeDpbKHR";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
      case ImageUsageFlagBits::eFragmentDensityMapEXT : return "FragmentDensityMapEXT";
      case ImageUsageFlagBits::eFragmentShadingRateAttachmentKHR : return "FragmentShadingRateAttachmentKHR";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
      case ImageUsageFlagBits::eVideoEncodeDstKHR : return "VideoEncodeDstKHR";
      case ImageUsageFlagBits::eVideoEncodeSrcKHR : return "VideoEncodeSrcKHR";
      case ImageUsageFlagBits::eVideoEncodeDpbKHR : return "VideoEncodeDpbKHR";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
      case ImageUsageFlagBits::eInvocationMaskHUAWEI : return "InvocationMaskHUAWEI";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( InternalAllocationType value )
  {
    switch ( value )
    {
      case InternalAllocationType::eExecutable : return "Executable";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( MemoryHeapFlagBits value )
  {
    switch ( value )
    {
      case MemoryHeapFlagBits::eDeviceLocal : return "DeviceLocal";
      case MemoryHeapFlagBits::eMultiInstance : return "MultiInstance";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( MemoryPropertyFlagBits value )
  {
    switch ( value )
    {
      case MemoryPropertyFlagBits::eDeviceLocal : return "DeviceLocal";
      case MemoryPropertyFlagBits::eHostVisible : return "HostVisible";
      case MemoryPropertyFlagBits::eHostCoherent : return "HostCoherent";
      case MemoryPropertyFlagBits::eHostCached : return "HostCached";
      case MemoryPropertyFlagBits::eLazilyAllocated : return "LazilyAllocated";
      case MemoryPropertyFlagBits::eProtected : return "Protected";
      case MemoryPropertyFlagBits::eDeviceCoherentAMD : return "DeviceCoherentAMD";
      case MemoryPropertyFlagBits::eDeviceUncachedAMD : return "DeviceUncachedAMD";
      case MemoryPropertyFlagBits::eRdmaCapableNV : return "RdmaCapableNV";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( PhysicalDeviceType value )
  {
    switch ( value )
    {
      case PhysicalDeviceType::eOther : return "Other";
      case PhysicalDeviceType::eIntegratedGpu : return "IntegratedGpu";
      case PhysicalDeviceType::eDiscreteGpu : return "DiscreteGpu";
      case PhysicalDeviceType::eVirtualGpu : return "VirtualGpu";
      case PhysicalDeviceType::eCpu : return "Cpu";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( QueueFlagBits value )
  {
    switch ( value )
    {
      case QueueFlagBits::eGraphics : return "Graphics";
      case QueueFlagBits::eCompute : return "Compute";
      case QueueFlagBits::eTransfer : return "Transfer";
      case QueueFlagBits::eSparseBinding : return "SparseBinding";
      case QueueFlagBits::eProtected : return "Protected";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
      case QueueFlagBits::eVideoDecodeKHR : return "VideoDecodeKHR";
      case QueueFlagBits::eVideoEncodeKHR : return "VideoEncodeKHR";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( SampleCountFlagBits value )
  {
    switch ( value )
    {
      case SampleCountFlagBits::e1 : return "1";
      case SampleCountFlagBits::e2 : return "2";
      case SampleCountFlagBits::e4 : return "4";
      case SampleCountFlagBits::e8 : return "8";
      case SampleCountFlagBits::e16 : return "16";
      case SampleCountFlagBits::e32 : return "32";
      case SampleCountFlagBits::e64 : return "64";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( SystemAllocationScope value )
  {
    switch ( value )
    {
      case SystemAllocationScope::eCommand : return "Command";
      case SystemAllocationScope::eObject : return "Object";
      case SystemAllocationScope::eCache : return "Cache";
      case SystemAllocationScope::eDevice : return "Device";
      case SystemAllocationScope::eInstance : return "Instance";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( InstanceCreateFlagBits )
  {
    return "(void)";
  }


  VULKAN_HPP_INLINE std::string to_string( DeviceCreateFlagBits )
  {
    return "(void)";
  }


  VULKAN_HPP_INLINE std::string to_string( PipelineStageFlagBits value )
  {
    switch ( value )
    {
      case PipelineStageFlagBits::eTopOfPipe : return "TopOfPipe";
      case PipelineStageFlagBits::eDrawIndirect : return "DrawIndirect";
      case PipelineStageFlagBits::eVertexInput : return "VertexInput";
      case PipelineStageFlagBits::eVertexShader : return "VertexShader";
      case PipelineStageFlagBits::eTessellationControlShader : return "TessellationControlShader";
      case PipelineStageFlagBits::eTessellationEvaluationShader : return "TessellationEvaluationShader";
      case PipelineStageFlagBits::eGeometryShader : return "GeometryShader";
      case PipelineStageFlagBits::eFragmentShader : return "FragmentShader";
      case PipelineStageFlagBits::eEarlyFragmentTests : return "EarlyFragmentTests";
      case PipelineStageFlagBits::eLateFragmentTests : return "LateFragmentTests";
      case PipelineStageFlagBits::eColorAttachmentOutput : return "ColorAttachmentOutput";
      case PipelineStageFlagBits::eComputeShader : return "ComputeShader";
      case PipelineStageFlagBits::eTransfer : return "Transfer";
      case PipelineStageFlagBits::eBottomOfPipe : return "BottomOfPipe";
      case PipelineStageFlagBits::eHost : return "Host";
      case PipelineStageFlagBits::eAllGraphics : return "AllGraphics";
      case PipelineStageFlagBits::eAllCommands : return "AllCommands";
      case PipelineStageFlagBits::eTransformFeedbackEXT : return "TransformFeedbackEXT";
      case PipelineStageFlagBits::eConditionalRenderingEXT : return "ConditionalRenderingEXT";
      case PipelineStageFlagBits::eAccelerationStructureBuildKHR : return "AccelerationStructureBuildKHR";
      case PipelineStageFlagBits::eRayTracingShaderKHR : return "RayTracingShaderKHR";
      case PipelineStageFlagBits::eTaskShaderNV : return "TaskShaderNV";
      case PipelineStageFlagBits::eMeshShaderNV : return "MeshShaderNV";
      case PipelineStageFlagBits::eFragmentDensityProcessEXT : return "FragmentDensityProcessEXT";
      case PipelineStageFlagBits::eFragmentShadingRateAttachmentKHR : return "FragmentShadingRateAttachmentKHR";
      case PipelineStageFlagBits::eCommandPreprocessNV : return "CommandPreprocessNV";
      case PipelineStageFlagBits::eNoneKHR : return "NoneKHR";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( MemoryMapFlagBits )
  {
    return "(void)";
  }


  VULKAN_HPP_INLINE std::string to_string( ImageAspectFlagBits value )
  {
    switch ( value )
    {
      case ImageAspectFlagBits::eColor : return "Color";
      case ImageAspectFlagBits::eDepth : return "Depth";
      case ImageAspectFlagBits::eStencil : return "Stencil";
      case ImageAspectFlagBits::eMetadata : return "Metadata";
      case ImageAspectFlagBits::ePlane0 : return "Plane0";
      case ImageAspectFlagBits::ePlane1 : return "Plane1";
      case ImageAspectFlagBits::ePlane2 : return "Plane2";
      case ImageAspectFlagBits::eMemoryPlane0EXT : return "MemoryPlane0EXT";
      case ImageAspectFlagBits::eMemoryPlane1EXT : return "MemoryPlane1EXT";
      case ImageAspectFlagBits::eMemoryPlane2EXT : return "MemoryPlane2EXT";
      case ImageAspectFlagBits::eMemoryPlane3EXT : return "MemoryPlane3EXT";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( SparseImageFormatFlagBits value )
  {
    switch ( value )
    {
      case SparseImageFormatFlagBits::eSingleMiptail : return "SingleMiptail";
      case SparseImageFormatFlagBits::eAlignedMipSize : return "AlignedMipSize";
      case SparseImageFormatFlagBits::eNonstandardBlockSize : return "NonstandardBlockSize";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( SparseMemoryBindFlagBits value )
  {
    switch ( value )
    {
      case SparseMemoryBindFlagBits::eMetadata : return "Metadata";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( FenceCreateFlagBits value )
  {
    switch ( value )
    {
      case FenceCreateFlagBits::eSignaled : return "Signaled";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( SemaphoreCreateFlagBits )
  {
    return "(void)";
  }


  VULKAN_HPP_INLINE std::string to_string( EventCreateFlagBits value )
  {
    switch ( value )
    {
      case EventCreateFlagBits::eDeviceOnlyKHR : return "DeviceOnlyKHR";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( QueryPipelineStatisticFlagBits value )
  {
    switch ( value )
    {
      case QueryPipelineStatisticFlagBits::eInputAssemblyVertices : return "InputAssemblyVertices";
      case QueryPipelineStatisticFlagBits::eInputAssemblyPrimitives : return "InputAssemblyPrimitives";
      case QueryPipelineStatisticFlagBits::eVertexShaderInvocations : return "VertexShaderInvocations";
      case QueryPipelineStatisticFlagBits::eGeometryShaderInvocations : return "GeometryShaderInvocations";
      case QueryPipelineStatisticFlagBits::eGeometryShaderPrimitives : return "GeometryShaderPrimitives";
      case QueryPipelineStatisticFlagBits::eClippingInvocations : return "ClippingInvocations";
      case QueryPipelineStatisticFlagBits::eClippingPrimitives : return "ClippingPrimitives";
      case QueryPipelineStatisticFlagBits::eFragmentShaderInvocations : return "FragmentShaderInvocations";
      case QueryPipelineStatisticFlagBits::eTessellationControlShaderPatches : return "TessellationControlShaderPatches";
      case QueryPipelineStatisticFlagBits::eTessellationEvaluationShaderInvocations : return "TessellationEvaluationShaderInvocations";
      case QueryPipelineStatisticFlagBits::eComputeShaderInvocations : return "ComputeShaderInvocations";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( QueryResultFlagBits value )
  {
    switch ( value )
    {
      case QueryResultFlagBits::e64 : return "64";
      case QueryResultFlagBits::eWait : return "Wait";
      case QueryResultFlagBits::eWithAvailability : return "WithAvailability";
      case QueryResultFlagBits::ePartial : return "Partial";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
      case QueryResultFlagBits::eWithStatusKHR : return "WithStatusKHR";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( QueryType value )
  {
    switch ( value )
    {
      case QueryType::eOcclusion : return "Occlusion";
      case QueryType::ePipelineStatistics : return "PipelineStatistics";
      case QueryType::eTimestamp : return "Timestamp";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
      case QueryType::eResultStatusOnlyKHR : return "ResultStatusOnlyKHR";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
      case QueryType::eTransformFeedbackStreamEXT : return "TransformFeedbackStreamEXT";
      case QueryType::ePerformanceQueryKHR : return "PerformanceQueryKHR";
      case QueryType::eAccelerationStructureCompactedSizeKHR : return "AccelerationStructureCompactedSizeKHR";
      case QueryType::eAccelerationStructureSerializationSizeKHR : return "AccelerationStructureSerializationSizeKHR";
      case QueryType::eAccelerationStructureCompactedSizeNV : return "AccelerationStructureCompactedSizeNV";
      case QueryType::ePerformanceQueryINTEL : return "PerformanceQueryINTEL";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
      case QueryType::eVideoEncodeBitstreamBufferRangeKHR : return "VideoEncodeBitstreamBufferRangeKHR";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( QueryPoolCreateFlagBits )
  {
    return "(void)";
  }


  VULKAN_HPP_INLINE std::string to_string( BufferCreateFlagBits value )
  {
    switch ( value )
    {
      case BufferCreateFlagBits::eSparseBinding : return "SparseBinding";
      case BufferCreateFlagBits::eSparseResidency : return "SparseResidency";
      case BufferCreateFlagBits::eSparseAliased : return "SparseAliased";
      case BufferCreateFlagBits::eProtected : return "Protected";
      case BufferCreateFlagBits::eDeviceAddressCaptureReplay : return "DeviceAddressCaptureReplay";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( BufferUsageFlagBits value )
  {
    switch ( value )
    {
      case BufferUsageFlagBits::eTransferSrc : return "TransferSrc";
      case BufferUsageFlagBits::eTransferDst : return "TransferDst";
      case BufferUsageFlagBits::eUniformTexelBuffer : return "UniformTexelBuffer";
      case BufferUsageFlagBits::eStorageTexelBuffer : return "StorageTexelBuffer";
      case BufferUsageFlagBits::eUniformBuffer : return "UniformBuffer";
      case BufferUsageFlagBits::eStorageBuffer : return "StorageBuffer";
      case BufferUsageFlagBits::eIndexBuffer : return "IndexBuffer";
      case BufferUsageFlagBits::eVertexBuffer : return "VertexBuffer";
      case BufferUsageFlagBits::eIndirectBuffer : return "IndirectBuffer";
      case BufferUsageFlagBits::eShaderDeviceAddress : return "ShaderDeviceAddress";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
      case BufferUsageFlagBits::eVideoDecodeSrcKHR : return "VideoDecodeSrcKHR";
      case BufferUsageFlagBits::eVideoDecodeDstKHR : return "VideoDecodeDstKHR";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
      case BufferUsageFlagBits::eTransformFeedbackBufferEXT : return "TransformFeedbackBufferEXT";
      case BufferUsageFlagBits::eTransformFeedbackCounterBufferEXT : return "TransformFeedbackCounterBufferEXT";
      case BufferUsageFlagBits::eConditionalRenderingEXT : return "ConditionalRenderingEXT";
      case BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR : return "AccelerationStructureBuildInputReadOnlyKHR";
      case BufferUsageFlagBits::eAccelerationStructureStorageKHR : return "AccelerationStructureStorageKHR";
      case BufferUsageFlagBits::eShaderBindingTableKHR : return "ShaderBindingTableKHR";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
      case BufferUsageFlagBits::eVideoEncodeDstKHR : return "VideoEncodeDstKHR";
      case BufferUsageFlagBits::eVideoEncodeSrcKHR : return "VideoEncodeSrcKHR";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( SharingMode value )
  {
    switch ( value )
    {
      case SharingMode::eExclusive : return "Exclusive";
      case SharingMode::eConcurrent : return "Concurrent";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( BufferViewCreateFlagBits )
  {
    return "(void)";
  }


  VULKAN_HPP_INLINE std::string to_string( ImageLayout value )
  {
    switch ( value )
    {
      case ImageLayout::eUndefined : return "Undefined";
      case ImageLayout::eGeneral : return "General";
      case ImageLayout::eColorAttachmentOptimal : return "ColorAttachmentOptimal";
      case ImageLayout::eDepthStencilAttachmentOptimal : return "DepthStencilAttachmentOptimal";
      case ImageLayout::eDepthStencilReadOnlyOptimal : return "DepthStencilReadOnlyOptimal";
      case ImageLayout::eShaderReadOnlyOptimal : return "ShaderReadOnlyOptimal";
      case ImageLayout::eTransferSrcOptimal : return "TransferSrcOptimal";
      case ImageLayout::eTransferDstOptimal : return "TransferDstOptimal";
      case ImageLayout::ePreinitialized : return "Preinitialized";
      case ImageLayout::eDepthReadOnlyStencilAttachmentOptimal : return "DepthReadOnlyStencilAttachmentOptimal";
      case ImageLayout::eDepthAttachmentStencilReadOnlyOptimal : return "DepthAttachmentStencilReadOnlyOptimal";
      case ImageLayout::eDepthAttachmentOptimal : return "DepthAttachmentOptimal";
      case ImageLayout::eDepthReadOnlyOptimal : return "DepthReadOnlyOptimal";
      case ImageLayout::eStencilAttachmentOptimal : return "StencilAttachmentOptimal";
      case ImageLayout::eStencilReadOnlyOptimal : return "StencilReadOnlyOptimal";
      case ImageLayout::ePresentSrcKHR : return "PresentSrcKHR";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
      case ImageLayout::eVideoDecodeDstKHR : return "VideoDecodeDstKHR";
      case ImageLayout::eVideoDecodeSrcKHR : return "VideoDecodeSrcKHR";
      case ImageLayout::eVideoDecodeDpbKHR : return "VideoDecodeDpbKHR";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
      case ImageLayout::eSharedPresentKHR : return "SharedPresentKHR";
      case ImageLayout::eFragmentDensityMapOptimalEXT : return "FragmentDensityMapOptimalEXT";
      case ImageLayout::eFragmentShadingRateAttachmentOptimalKHR : return "FragmentShadingRateAttachmentOptimalKHR";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
      case ImageLayout::eVideoEncodeDstKHR : return "VideoEncodeDstKHR";
      case ImageLayout::eVideoEncodeSrcKHR : return "VideoEncodeSrcKHR";
      case ImageLayout::eVideoEncodeDpbKHR : return "VideoEncodeDpbKHR";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
      case ImageLayout::eReadOnlyOptimalKHR : return "ReadOnlyOptimalKHR";
      case ImageLayout::eAttachmentOptimalKHR : return "AttachmentOptimalKHR";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( ComponentSwizzle value )
  {
    switch ( value )
    {
      case ComponentSwizzle::eIdentity : return "Identity";
      case ComponentSwizzle::eZero : return "Zero";
      case ComponentSwizzle::eOne : return "One";
      case ComponentSwizzle::eR : return "R";
      case ComponentSwizzle::eG : return "G";
      case ComponentSwizzle::eB : return "B";
      case ComponentSwizzle::eA : return "A";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( ImageViewCreateFlagBits value )
  {
    switch ( value )
    {
      case ImageViewCreateFlagBits::eFragmentDensityMapDynamicEXT : return "FragmentDensityMapDynamicEXT";
      case ImageViewCreateFlagBits::eFragmentDensityMapDeferredEXT : return "FragmentDensityMapDeferredEXT";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( ImageViewType value )
  {
    switch ( value )
    {
      case ImageViewType::e1D : return "1D";
      case ImageViewType::e2D : return "2D";
      case ImageViewType::e3D : return "3D";
      case ImageViewType::eCube : return "Cube";
      case ImageViewType::e1DArray : return "1DArray";
      case ImageViewType::e2DArray : return "2DArray";
      case ImageViewType::eCubeArray : return "CubeArray";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( ShaderModuleCreateFlagBits )
  {
    return "(void)";
  }


  VULKAN_HPP_INLINE std::string to_string( BlendFactor value )
  {
    switch ( value )
    {
      case BlendFactor::eZero : return "Zero";
      case BlendFactor::eOne : return "One";
      case BlendFactor::eSrcColor : return "SrcColor";
      case BlendFactor::eOneMinusSrcColor : return "OneMinusSrcColor";
      case BlendFactor::eDstColor : return "DstColor";
      case BlendFactor::eOneMinusDstColor : return "OneMinusDstColor";
      case BlendFactor::eSrcAlpha : return "SrcAlpha";
      case BlendFactor::eOneMinusSrcAlpha : return "OneMinusSrcAlpha";
      case BlendFactor::eDstAlpha : return "DstAlpha";
      case BlendFactor::eOneMinusDstAlpha : return "OneMinusDstAlpha";
      case BlendFactor::eConstantColor : return "ConstantColor";
      case BlendFactor::eOneMinusConstantColor : return "OneMinusConstantColor";
      case BlendFactor::eConstantAlpha : return "ConstantAlpha";
      case BlendFactor::eOneMinusConstantAlpha : return "OneMinusConstantAlpha";
      case BlendFactor::eSrcAlphaSaturate : return "SrcAlphaSaturate";
      case BlendFactor::eSrc1Color : return "Src1Color";
      case BlendFactor::eOneMinusSrc1Color : return "OneMinusSrc1Color";
      case BlendFactor::eSrc1Alpha : return "Src1Alpha";
      case BlendFactor::eOneMinusSrc1Alpha : return "OneMinusSrc1Alpha";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( BlendOp value )
  {
    switch ( value )
    {
      case BlendOp::eAdd : return "Add";
      case BlendOp::eSubtract : return "Subtract";
      case BlendOp::eReverseSubtract : return "ReverseSubtract";
      case BlendOp::eMin : return "Min";
      case BlendOp::eMax : return "Max";
      case BlendOp::eZeroEXT : return "ZeroEXT";
      case BlendOp::eSrcEXT : return "SrcEXT";
      case BlendOp::eDstEXT : return "DstEXT";
      case BlendOp::eSrcOverEXT : return "SrcOverEXT";
      case BlendOp::eDstOverEXT : return "DstOverEXT";
      case BlendOp::eSrcInEXT : return "SrcInEXT";
      case BlendOp::eDstInEXT : return "DstInEXT";
      case BlendOp::eSrcOutEXT : return "SrcOutEXT";
      case BlendOp::eDstOutEXT : return "DstOutEXT";
      case BlendOp::eSrcAtopEXT : return "SrcAtopEXT";
      case BlendOp::eDstAtopEXT : return "DstAtopEXT";
      case BlendOp::eXorEXT : return "XorEXT";
      case BlendOp::eMultiplyEXT : return "MultiplyEXT";
      case BlendOp::eScreenEXT : return "ScreenEXT";
      case BlendOp::eOverlayEXT : return "OverlayEXT";
      case BlendOp::eDarkenEXT : return "DarkenEXT";
      case BlendOp::eLightenEXT : return "LightenEXT";
      case BlendOp::eColordodgeEXT : return "ColordodgeEXT";
      case BlendOp::eColorburnEXT : return "ColorburnEXT";
      case BlendOp::eHardlightEXT : return "HardlightEXT";
      case BlendOp::eSoftlightEXT : return "SoftlightEXT";
      case BlendOp::eDifferenceEXT : return "DifferenceEXT";
      case BlendOp::eExclusionEXT : return "ExclusionEXT";
      case BlendOp::eInvertEXT : return "InvertEXT";
      case BlendOp::eInvertRgbEXT : return "InvertRgbEXT";
      case BlendOp::eLineardodgeEXT : return "LineardodgeEXT";
      case BlendOp::eLinearburnEXT : return "LinearburnEXT";
      case BlendOp::eVividlightEXT : return "VividlightEXT";
      case BlendOp::eLinearlightEXT : return "LinearlightEXT";
      case BlendOp::ePinlightEXT : return "PinlightEXT";
      case BlendOp::eHardmixEXT : return "HardmixEXT";
      case BlendOp::eHslHueEXT : return "HslHueEXT";
      case BlendOp::eHslSaturationEXT : return "HslSaturationEXT";
      case BlendOp::eHslColorEXT : return "HslColorEXT";
      case BlendOp::eHslLuminosityEXT : return "HslLuminosityEXT";
      case BlendOp::ePlusEXT : return "PlusEXT";
      case BlendOp::ePlusClampedEXT : return "PlusClampedEXT";
      case BlendOp::ePlusClampedAlphaEXT : return "PlusClampedAlphaEXT";
      case BlendOp::ePlusDarkerEXT : return "PlusDarkerEXT";
      case BlendOp::eMinusEXT : return "MinusEXT";
      case BlendOp::eMinusClampedEXT : return "MinusClampedEXT";
      case BlendOp::eContrastEXT : return "ContrastEXT";
      case BlendOp::eInvertOvgEXT : return "InvertOvgEXT";
      case BlendOp::eRedEXT : return "RedEXT";
      case BlendOp::eGreenEXT : return "GreenEXT";
      case BlendOp::eBlueEXT : return "BlueEXT";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( ColorComponentFlagBits value )
  {
    switch ( value )
    {
      case ColorComponentFlagBits::eR : return "R";
      case ColorComponentFlagBits::eG : return "G";
      case ColorComponentFlagBits::eB : return "B";
      case ColorComponentFlagBits::eA : return "A";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( CompareOp value )
  {
    switch ( value )
    {
      case CompareOp::eNever : return "Never";
      case CompareOp::eLess : return "Less";
      case CompareOp::eEqual : return "Equal";
      case CompareOp::eLessOrEqual : return "LessOrEqual";
      case CompareOp::eGreater : return "Greater";
      case CompareOp::eNotEqual : return "NotEqual";
      case CompareOp::eGreaterOrEqual : return "GreaterOrEqual";
      case CompareOp::eAlways : return "Always";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( CullModeFlagBits value )
  {
    switch ( value )
    {
      case CullModeFlagBits::eNone : return "None";
      case CullModeFlagBits::eFront : return "Front";
      case CullModeFlagBits::eBack : return "Back";
      case CullModeFlagBits::eFrontAndBack : return "FrontAndBack";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( DynamicState value )
  {
    switch ( value )
    {
      case DynamicState::eViewport : return "Viewport";
      case DynamicState::eScissor : return "Scissor";
      case DynamicState::eLineWidth : return "LineWidth";
      case DynamicState::eDepthBias : return "DepthBias";
      case DynamicState::eBlendConstants : return "BlendConstants";
      case DynamicState::eDepthBounds : return "DepthBounds";
      case DynamicState::eStencilCompareMask : return "StencilCompareMask";
      case DynamicState::eStencilWriteMask : return "StencilWriteMask";
      case DynamicState::eStencilReference : return "StencilReference";
      case DynamicState::eViewportWScalingNV : return "ViewportWScalingNV";
      case DynamicState::eDiscardRectangleEXT : return "DiscardRectangleEXT";
      case DynamicState::eSampleLocationsEXT : return "SampleLocationsEXT";
      case DynamicState::eRayTracingPipelineStackSizeKHR : return "RayTracingPipelineStackSizeKHR";
      case DynamicState::eViewportShadingRatePaletteNV : return "ViewportShadingRatePaletteNV";
      case DynamicState::eViewportCoarseSampleOrderNV : return "ViewportCoarseSampleOrderNV";
      case DynamicState::eExclusiveScissorNV : return "ExclusiveScissorNV";
      case DynamicState::eFragmentShadingRateKHR : return "FragmentShadingRateKHR";
      case DynamicState::eLineStippleEXT : return "LineStippleEXT";
      case DynamicState::eCullModeEXT : return "CullModeEXT";
      case DynamicState::eFrontFaceEXT : return "FrontFaceEXT";
      case DynamicState::ePrimitiveTopologyEXT : return "PrimitiveTopologyEXT";
      case DynamicState::eViewportWithCountEXT : return "ViewportWithCountEXT";
      case DynamicState::eScissorWithCountEXT : return "ScissorWithCountEXT";
      case DynamicState::eVertexInputBindingStrideEXT : return "VertexInputBindingStrideEXT";
      case DynamicState::eDepthTestEnableEXT : return "DepthTestEnableEXT";
      case DynamicState::eDepthWriteEnableEXT : return "DepthWriteEnableEXT";
      case DynamicState::eDepthCompareOpEXT : return "DepthCompareOpEXT";
      case DynamicState::eDepthBoundsTestEnableEXT : return "DepthBoundsTestEnableEXT";
      case DynamicState::eStencilTestEnableEXT : return "StencilTestEnableEXT";
      case DynamicState::eStencilOpEXT : return "StencilOpEXT";
      case DynamicState::eVertexInputEXT : return "VertexInputEXT";
      case DynamicState::ePatchControlPointsEXT : return "PatchControlPointsEXT";
      case DynamicState::eRasterizerDiscardEnableEXT : return "RasterizerDiscardEnableEXT";
      case DynamicState::eDepthBiasEnableEXT : return "DepthBiasEnableEXT";
      case DynamicState::eLogicOpEXT : return "LogicOpEXT";
      case DynamicState::ePrimitiveRestartEnableEXT : return "PrimitiveRestartEnableEXT";
      case DynamicState::eColorWriteEnableEXT : return "ColorWriteEnableEXT";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( FrontFace value )
  {
    switch ( value )
    {
      case FrontFace::eCounterClockwise : return "CounterClockwise";
      case FrontFace::eClockwise : return "Clockwise";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( LogicOp value )
  {
    switch ( value )
    {
      case LogicOp::eClear : return "Clear";
      case LogicOp::eAnd : return "And";
      case LogicOp::eAndReverse : return "AndReverse";
      case LogicOp::eCopy : return "Copy";
      case LogicOp::eAndInverted : return "AndInverted";
      case LogicOp::eNoOp : return "NoOp";
      case LogicOp::eXor : return "Xor";
      case LogicOp::eOr : return "Or";
      case LogicOp::eNor : return "Nor";
      case LogicOp::eEquivalent : return "Equivalent";
      case LogicOp::eInvert : return "Invert";
      case LogicOp::eOrReverse : return "OrReverse";
      case LogicOp::eCopyInverted : return "CopyInverted";
      case LogicOp::eOrInverted : return "OrInverted";
      case LogicOp::eNand : return "Nand";
      case LogicOp::eSet : return "Set";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( PipelineCreateFlagBits value )
  {
    switch ( value )
    {
      case PipelineCreateFlagBits::eDisableOptimization : return "DisableOptimization";
      case PipelineCreateFlagBits::eAllowDerivatives : return "AllowDerivatives";
      case PipelineCreateFlagBits::eDerivative : return "Derivative";
      case PipelineCreateFlagBits::eViewIndexFromDeviceIndex : return "ViewIndexFromDeviceIndex";
      case PipelineCreateFlagBits::eDispatchBase : return "DispatchBase";
      case PipelineCreateFlagBits::eVkPipelineRasterizationStateCreateFragmentShadingRateAttachmentKHR : return "VkPipelineRasterizationStateCreateFragmentShadingRateAttachmentKHR";
      case PipelineCreateFlagBits::eVkPipelineRasterizationStateCreateFragmentDensityMapAttachmentEXT : return "VkPipelineRasterizationStateCreateFragmentDensityMapAttachmentEXT";
      case PipelineCreateFlagBits::eRayTracingNoNullAnyHitShadersKHR : return "RayTracingNoNullAnyHitShadersKHR";
      case PipelineCreateFlagBits::eRayTracingNoNullClosestHitShadersKHR : return "RayTracingNoNullClosestHitShadersKHR";
      case PipelineCreateFlagBits::eRayTracingNoNullMissShadersKHR : return "RayTracingNoNullMissShadersKHR";
      case PipelineCreateFlagBits::eRayTracingNoNullIntersectionShadersKHR : return "RayTracingNoNullIntersectionShadersKHR";
      case PipelineCreateFlagBits::eRayTracingSkipTrianglesKHR : return "RayTracingSkipTrianglesKHR";
      case PipelineCreateFlagBits::eRayTracingSkipAabbsKHR : return "RayTracingSkipAabbsKHR";
      case PipelineCreateFlagBits::eRayTracingShaderGroupHandleCaptureReplayKHR : return "RayTracingShaderGroupHandleCaptureReplayKHR";
      case PipelineCreateFlagBits::eDeferCompileNV : return "DeferCompileNV";
      case PipelineCreateFlagBits::eCaptureStatisticsKHR : return "CaptureStatisticsKHR";
      case PipelineCreateFlagBits::eCaptureInternalRepresentationsKHR : return "CaptureInternalRepresentationsKHR";
      case PipelineCreateFlagBits::eIndirectBindableNV : return "IndirectBindableNV";
      case PipelineCreateFlagBits::eLibraryKHR : return "LibraryKHR";
      case PipelineCreateFlagBits::eFailOnPipelineCompileRequiredEXT : return "FailOnPipelineCompileRequiredEXT";
      case PipelineCreateFlagBits::eEarlyReturnOnFailureEXT : return "EarlyReturnOnFailureEXT";
      case PipelineCreateFlagBits::eRayTracingAllowMotionNV : return "RayTracingAllowMotionNV";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( PipelineShaderStageCreateFlagBits value )
  {
    switch ( value )
    {
      case PipelineShaderStageCreateFlagBits::eAllowVaryingSubgroupSizeEXT : return "AllowVaryingSubgroupSizeEXT";
      case PipelineShaderStageCreateFlagBits::eRequireFullSubgroupsEXT : return "RequireFullSubgroupsEXT";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( PolygonMode value )
  {
    switch ( value )
    {
      case PolygonMode::eFill : return "Fill";
      case PolygonMode::eLine : return "Line";
      case PolygonMode::ePoint : return "Point";
      case PolygonMode::eFillRectangleNV : return "FillRectangleNV";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( PrimitiveTopology value )
  {
    switch ( value )
    {
      case PrimitiveTopology::ePointList : return "PointList";
      case PrimitiveTopology::eLineList : return "LineList";
      case PrimitiveTopology::eLineStrip : return "LineStrip";
      case PrimitiveTopology::eTriangleList : return "TriangleList";
      case PrimitiveTopology::eTriangleStrip : return "TriangleStrip";
      case PrimitiveTopology::eTriangleFan : return "TriangleFan";
      case PrimitiveTopology::eLineListWithAdjacency : return "LineListWithAdjacency";
      case PrimitiveTopology::eLineStripWithAdjacency : return "LineStripWithAdjacency";
      case PrimitiveTopology::eTriangleListWithAdjacency : return "TriangleListWithAdjacency";
      case PrimitiveTopology::eTriangleStripWithAdjacency : return "TriangleStripWithAdjacency";
      case PrimitiveTopology::ePatchList : return "PatchList";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( ShaderStageFlagBits value )
  {
    switch ( value )
    {
      case ShaderStageFlagBits::eVertex : return "Vertex";
      case ShaderStageFlagBits::eTessellationControl : return "TessellationControl";
      case ShaderStageFlagBits::eTessellationEvaluation : return "TessellationEvaluation";
      case ShaderStageFlagBits::eGeometry : return "Geometry";
      case ShaderStageFlagBits::eFragment : return "Fragment";
      case ShaderStageFlagBits::eCompute : return "Compute";
      case ShaderStageFlagBits::eAllGraphics : return "AllGraphics";
      case ShaderStageFlagBits::eAll : return "All";
      case ShaderStageFlagBits::eRaygenKHR : return "RaygenKHR";
      case ShaderStageFlagBits::eAnyHitKHR : return "AnyHitKHR";
      case ShaderStageFlagBits::eClosestHitKHR : return "ClosestHitKHR";
      case ShaderStageFlagBits::eMissKHR : return "MissKHR";
      case ShaderStageFlagBits::eIntersectionKHR : return "IntersectionKHR";
      case ShaderStageFlagBits::eCallableKHR : return "CallableKHR";
      case ShaderStageFlagBits::eTaskNV : return "TaskNV";
      case ShaderStageFlagBits::eMeshNV : return "MeshNV";
      case ShaderStageFlagBits::eSubpassShadingHUAWEI : return "SubpassShadingHUAWEI";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( StencilOp value )
  {
    switch ( value )
    {
      case StencilOp::eKeep : return "Keep";
      case StencilOp::eZero : return "Zero";
      case StencilOp::eReplace : return "Replace";
      case StencilOp::eIncrementAndClamp : return "IncrementAndClamp";
      case StencilOp::eDecrementAndClamp : return "DecrementAndClamp";
      case StencilOp::eInvert : return "Invert";
      case StencilOp::eIncrementAndWrap : return "IncrementAndWrap";
      case StencilOp::eDecrementAndWrap : return "DecrementAndWrap";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( VertexInputRate value )
  {
    switch ( value )
    {
      case VertexInputRate::eVertex : return "Vertex";
      case VertexInputRate::eInstance : return "Instance";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( PipelineColorBlendStateCreateFlagBits )
  {
    return "(void)";
  }


  VULKAN_HPP_INLINE std::string to_string( PipelineDepthStencilStateCreateFlagBits )
  {
    return "(void)";
  }


  VULKAN_HPP_INLINE std::string to_string( PipelineDynamicStateCreateFlagBits )
  {
    return "(void)";
  }


  VULKAN_HPP_INLINE std::string to_string( PipelineInputAssemblyStateCreateFlagBits )
  {
    return "(void)";
  }


  VULKAN_HPP_INLINE std::string to_string( PipelineLayoutCreateFlagBits )
  {
    return "(void)";
  }


  VULKAN_HPP_INLINE std::string to_string( PipelineMultisampleStateCreateFlagBits )
  {
    return "(void)";
  }


  VULKAN_HPP_INLINE std::string to_string( PipelineRasterizationStateCreateFlagBits )
  {
    return "(void)";
  }


  VULKAN_HPP_INLINE std::string to_string( PipelineTessellationStateCreateFlagBits )
  {
    return "(void)";
  }


  VULKAN_HPP_INLINE std::string to_string( PipelineVertexInputStateCreateFlagBits )
  {
    return "(void)";
  }


  VULKAN_HPP_INLINE std::string to_string( PipelineViewportStateCreateFlagBits )
  {
    return "(void)";
  }


  VULKAN_HPP_INLINE std::string to_string( BorderColor value )
  {
    switch ( value )
    {
      case BorderColor::eFloatTransparentBlack : return "FloatTransparentBlack";
      case BorderColor::eIntTransparentBlack : return "IntTransparentBlack";
      case BorderColor::eFloatOpaqueBlack : return "FloatOpaqueBlack";
      case BorderColor::eIntOpaqueBlack : return "IntOpaqueBlack";
      case BorderColor::eFloatOpaqueWhite : return "FloatOpaqueWhite";
      case BorderColor::eIntOpaqueWhite : return "IntOpaqueWhite";
      case BorderColor::eFloatCustomEXT : return "FloatCustomEXT";
      case BorderColor::eIntCustomEXT : return "IntCustomEXT";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( Filter value )
  {
    switch ( value )
    {
      case Filter::eNearest : return "Nearest";
      case Filter::eLinear : return "Linear";
      case Filter::eCubicIMG : return "CubicIMG";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( SamplerAddressMode value )
  {
    switch ( value )
    {
      case SamplerAddressMode::eRepeat : return "Repeat";
      case SamplerAddressMode::eMirroredRepeat : return "MirroredRepeat";
      case SamplerAddressMode::eClampToEdge : return "ClampToEdge";
      case SamplerAddressMode::eClampToBorder : return "ClampToBorder";
      case SamplerAddressMode::eMirrorClampToEdge : return "MirrorClampToEdge";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( SamplerCreateFlagBits value )
  {
    switch ( value )
    {
      case SamplerCreateFlagBits::eSubsampledEXT : return "SubsampledEXT";
      case SamplerCreateFlagBits::eSubsampledCoarseReconstructionEXT : return "SubsampledCoarseReconstructionEXT";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( SamplerMipmapMode value )
  {
    switch ( value )
    {
      case SamplerMipmapMode::eNearest : return "Nearest";
      case SamplerMipmapMode::eLinear : return "Linear";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( DescriptorPoolCreateFlagBits value )
  {
    switch ( value )
    {
      case DescriptorPoolCreateFlagBits::eFreeDescriptorSet : return "FreeDescriptorSet";
      case DescriptorPoolCreateFlagBits::eUpdateAfterBind : return "UpdateAfterBind";
      case DescriptorPoolCreateFlagBits::eHostOnlyVALVE : return "HostOnlyVALVE";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( DescriptorSetLayoutCreateFlagBits value )
  {
    switch ( value )
    {
      case DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool : return "UpdateAfterBindPool";
      case DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR : return "PushDescriptorKHR";
      case DescriptorSetLayoutCreateFlagBits::eHostOnlyPoolVALVE : return "HostOnlyPoolVALVE";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( DescriptorType value )
  {
    switch ( value )
    {
      case DescriptorType::eSampler : return "Sampler";
      case DescriptorType::eCombinedImageSampler : return "CombinedImageSampler";
      case DescriptorType::eSampledImage : return "SampledImage";
      case DescriptorType::eStorageImage : return "StorageImage";
      case DescriptorType::eUniformTexelBuffer : return "UniformTexelBuffer";
      case DescriptorType::eStorageTexelBuffer : return "StorageTexelBuffer";
      case DescriptorType::eUniformBuffer : return "UniformBuffer";
      case DescriptorType::eStorageBuffer : return "StorageBuffer";
      case DescriptorType::eUniformBufferDynamic : return "UniformBufferDynamic";
      case DescriptorType::eStorageBufferDynamic : return "StorageBufferDynamic";
      case DescriptorType::eInputAttachment : return "InputAttachment";
      case DescriptorType::eInlineUniformBlockEXT : return "InlineUniformBlockEXT";
      case DescriptorType::eAccelerationStructureKHR : return "AccelerationStructureKHR";
      case DescriptorType::eAccelerationStructureNV : return "AccelerationStructureNV";
      case DescriptorType::eMutableVALVE : return "MutableVALVE";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( DescriptorPoolResetFlagBits )
  {
    return "(void)";
  }


  VULKAN_HPP_INLINE std::string to_string( AccessFlagBits value )
  {
    switch ( value )
    {
      case AccessFlagBits::eIndirectCommandRead : return "IndirectCommandRead";
      case AccessFlagBits::eIndexRead : return "IndexRead";
      case AccessFlagBits::eVertexAttributeRead : return "VertexAttributeRead";
      case AccessFlagBits::eUniformRead : return "UniformRead";
      case AccessFlagBits::eInputAttachmentRead : return "InputAttachmentRead";
      case AccessFlagBits::eShaderRead : return "ShaderRead";
      case AccessFlagBits::eShaderWrite : return "ShaderWrite";
      case AccessFlagBits::eColorAttachmentRead : return "ColorAttachmentRead";
      case AccessFlagBits::eColorAttachmentWrite : return "ColorAttachmentWrite";
      case AccessFlagBits::eDepthStencilAttachmentRead : return "DepthStencilAttachmentRead";
      case AccessFlagBits::eDepthStencilAttachmentWrite : return "DepthStencilAttachmentWrite";
      case AccessFlagBits::eTransferRead : return "TransferRead";
      case AccessFlagBits::eTransferWrite : return "TransferWrite";
      case AccessFlagBits::eHostRead : return "HostRead";
      case AccessFlagBits::eHostWrite : return "HostWrite";
      case AccessFlagBits::eMemoryRead : return "MemoryRead";
      case AccessFlagBits::eMemoryWrite : return "MemoryWrite";
      case AccessFlagBits::eTransformFeedbackWriteEXT : return "TransformFeedbackWriteEXT";
      case AccessFlagBits::eTransformFeedbackCounterReadEXT : return "TransformFeedbackCounterReadEXT";
      case AccessFlagBits::eTransformFeedbackCounterWriteEXT : return "TransformFeedbackCounterWriteEXT";
      case AccessFlagBits::eConditionalRenderingReadEXT : return "ConditionalRenderingReadEXT";
      case AccessFlagBits::eColorAttachmentReadNoncoherentEXT : return "ColorAttachmentReadNoncoherentEXT";
      case AccessFlagBits::eAccelerationStructureReadKHR : return "AccelerationStructureReadKHR";
      case AccessFlagBits::eAccelerationStructureWriteKHR : return "AccelerationStructureWriteKHR";
      case AccessFlagBits::eFragmentDensityMapReadEXT : return "FragmentDensityMapReadEXT";
      case AccessFlagBits::eFragmentShadingRateAttachmentReadKHR : return "FragmentShadingRateAttachmentReadKHR";
      case AccessFlagBits::eCommandPreprocessReadNV : return "CommandPreprocessReadNV";
      case AccessFlagBits::eCommandPreprocessWriteNV : return "CommandPreprocessWriteNV";
      case AccessFlagBits::eNoneKHR : return "NoneKHR";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( AttachmentDescriptionFlagBits value )
  {
    switch ( value )
    {
      case AttachmentDescriptionFlagBits::eMayAlias : return "MayAlias";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( AttachmentLoadOp value )
  {
    switch ( value )
    {
      case AttachmentLoadOp::eLoad : return "Load";
      case AttachmentLoadOp::eClear : return "Clear";
      case AttachmentLoadOp::eDontCare : return "DontCare";
      case AttachmentLoadOp::eNoneEXT : return "NoneEXT";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( AttachmentStoreOp value )
  {
    switch ( value )
    {
      case AttachmentStoreOp::eStore : return "Store";
      case AttachmentStoreOp::eDontCare : return "DontCare";
      case AttachmentStoreOp::eNoneKHR : return "NoneKHR";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( DependencyFlagBits value )
  {
    switch ( value )
    {
      case DependencyFlagBits::eByRegion : return "ByRegion";
      case DependencyFlagBits::eDeviceGroup : return "DeviceGroup";
      case DependencyFlagBits::eViewLocal : return "ViewLocal";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( FramebufferCreateFlagBits value )
  {
    switch ( value )
    {
      case FramebufferCreateFlagBits::eImageless : return "Imageless";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( PipelineBindPoint value )
  {
    switch ( value )
    {
      case PipelineBindPoint::eGraphics : return "Graphics";
      case PipelineBindPoint::eCompute : return "Compute";
      case PipelineBindPoint::eRayTracingKHR : return "RayTracingKHR";
      case PipelineBindPoint::eSubpassShadingHUAWEI : return "SubpassShadingHUAWEI";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( RenderPassCreateFlagBits value )
  {
    switch ( value )
    {
      case RenderPassCreateFlagBits::eTransformQCOM : return "TransformQCOM";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( SubpassDescriptionFlagBits value )
  {
    switch ( value )
    {
      case SubpassDescriptionFlagBits::ePerViewAttributesNVX : return "PerViewAttributesNVX";
      case SubpassDescriptionFlagBits::ePerViewPositionXOnlyNVX : return "PerViewPositionXOnlyNVX";
      case SubpassDescriptionFlagBits::eFragmentRegionQCOM : return "FragmentRegionQCOM";
      case SubpassDescriptionFlagBits::eShaderResolveQCOM : return "ShaderResolveQCOM";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( CommandPoolCreateFlagBits value )
  {
    switch ( value )
    {
      case CommandPoolCreateFlagBits::eTransient : return "Transient";
      case CommandPoolCreateFlagBits::eResetCommandBuffer : return "ResetCommandBuffer";
      case CommandPoolCreateFlagBits::eProtected : return "Protected";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( CommandPoolResetFlagBits value )
  {
    switch ( value )
    {
      case CommandPoolResetFlagBits::eReleaseResources : return "ReleaseResources";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( CommandBufferLevel value )
  {
    switch ( value )
    {
      case CommandBufferLevel::ePrimary : return "Primary";
      case CommandBufferLevel::eSecondary : return "Secondary";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( CommandBufferResetFlagBits value )
  {
    switch ( value )
    {
      case CommandBufferResetFlagBits::eReleaseResources : return "ReleaseResources";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( CommandBufferUsageFlagBits value )
  {
    switch ( value )
    {
      case CommandBufferUsageFlagBits::eOneTimeSubmit : return "OneTimeSubmit";
      case CommandBufferUsageFlagBits::eRenderPassContinue : return "RenderPassContinue";
      case CommandBufferUsageFlagBits::eSimultaneousUse : return "SimultaneousUse";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( QueryControlFlagBits value )
  {
    switch ( value )
    {
      case QueryControlFlagBits::ePrecise : return "Precise";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( IndexType value )
  {
    switch ( value )
    {
      case IndexType::eUint16 : return "Uint16";
      case IndexType::eUint32 : return "Uint32";
      case IndexType::eNoneKHR : return "NoneKHR";
      case IndexType::eUint8EXT : return "Uint8EXT";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( StencilFaceFlagBits value )
  {
    switch ( value )
    {
      case StencilFaceFlagBits::eFront : return "Front";
      case StencilFaceFlagBits::eBack : return "Back";
      case StencilFaceFlagBits::eFrontAndBack : return "FrontAndBack";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( SubpassContents value )
  {
    switch ( value )
    {
      case SubpassContents::eInline : return "Inline";
      case SubpassContents::eSecondaryCommandBuffers : return "SecondaryCommandBuffers";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_VERSION_1_1 ===


  VULKAN_HPP_INLINE std::string to_string( SubgroupFeatureFlagBits value )
  {
    switch ( value )
    {
      case SubgroupFeatureFlagBits::eBasic : return "Basic";
      case SubgroupFeatureFlagBits::eVote : return "Vote";
      case SubgroupFeatureFlagBits::eArithmetic : return "Arithmetic";
      case SubgroupFeatureFlagBits::eBallot : return "Ballot";
      case SubgroupFeatureFlagBits::eShuffle : return "Shuffle";
      case SubgroupFeatureFlagBits::eShuffleRelative : return "ShuffleRelative";
      case SubgroupFeatureFlagBits::eClustered : return "Clustered";
      case SubgroupFeatureFlagBits::eQuad : return "Quad";
      case SubgroupFeatureFlagBits::ePartitionedNV : return "PartitionedNV";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( PeerMemoryFeatureFlagBits value )
  {
    switch ( value )
    {
      case PeerMemoryFeatureFlagBits::eCopySrc : return "CopySrc";
      case PeerMemoryFeatureFlagBits::eCopyDst : return "CopyDst";
      case PeerMemoryFeatureFlagBits::eGenericSrc : return "GenericSrc";
      case PeerMemoryFeatureFlagBits::eGenericDst : return "GenericDst";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( MemoryAllocateFlagBits value )
  {
    switch ( value )
    {
      case MemoryAllocateFlagBits::eDeviceMask : return "DeviceMask";
      case MemoryAllocateFlagBits::eDeviceAddress : return "DeviceAddress";
      case MemoryAllocateFlagBits::eDeviceAddressCaptureReplay : return "DeviceAddressCaptureReplay";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( CommandPoolTrimFlagBits )
  {
    return "(void)";
  }


  VULKAN_HPP_INLINE std::string to_string( PointClippingBehavior value )
  {
    switch ( value )
    {
      case PointClippingBehavior::eAllClipPlanes : return "AllClipPlanes";
      case PointClippingBehavior::eUserClipPlanesOnly : return "UserClipPlanesOnly";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( TessellationDomainOrigin value )
  {
    switch ( value )
    {
      case TessellationDomainOrigin::eUpperLeft : return "UpperLeft";
      case TessellationDomainOrigin::eLowerLeft : return "LowerLeft";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( DeviceQueueCreateFlagBits value )
  {
    switch ( value )
    {
      case DeviceQueueCreateFlagBits::eProtected : return "Protected";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( SamplerYcbcrModelConversion value )
  {
    switch ( value )
    {
      case SamplerYcbcrModelConversion::eRgbIdentity : return "RgbIdentity";
      case SamplerYcbcrModelConversion::eYcbcrIdentity : return "YcbcrIdentity";
      case SamplerYcbcrModelConversion::eYcbcr709 : return "Ycbcr709";
      case SamplerYcbcrModelConversion::eYcbcr601 : return "Ycbcr601";
      case SamplerYcbcrModelConversion::eYcbcr2020 : return "Ycbcr2020";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( SamplerYcbcrRange value )
  {
    switch ( value )
    {
      case SamplerYcbcrRange::eItuFull : return "ItuFull";
      case SamplerYcbcrRange::eItuNarrow : return "ItuNarrow";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( ChromaLocation value )
  {
    switch ( value )
    {
      case ChromaLocation::eCositedEven : return "CositedEven";
      case ChromaLocation::eMidpoint : return "Midpoint";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( DescriptorUpdateTemplateType value )
  {
    switch ( value )
    {
      case DescriptorUpdateTemplateType::eDescriptorSet : return "DescriptorSet";
      case DescriptorUpdateTemplateType::ePushDescriptorsKHR : return "PushDescriptorsKHR";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( DescriptorUpdateTemplateCreateFlagBits )
  {
    return "(void)";
  }


  VULKAN_HPP_INLINE std::string to_string( ExternalMemoryHandleTypeFlagBits value )
  {
    switch ( value )
    {
      case ExternalMemoryHandleTypeFlagBits::eOpaqueFd : return "OpaqueFd";
      case ExternalMemoryHandleTypeFlagBits::eOpaqueWin32 : return "OpaqueWin32";
      case ExternalMemoryHandleTypeFlagBits::eOpaqueWin32Kmt : return "OpaqueWin32Kmt";
      case ExternalMemoryHandleTypeFlagBits::eD3D11Texture : return "D3D11Texture";
      case ExternalMemoryHandleTypeFlagBits::eD3D11TextureKmt : return "D3D11TextureKmt";
      case ExternalMemoryHandleTypeFlagBits::eD3D12Heap : return "D3D12Heap";
      case ExternalMemoryHandleTypeFlagBits::eD3D12Resource : return "D3D12Resource";
      case ExternalMemoryHandleTypeFlagBits::eDmaBufEXT : return "DmaBufEXT";
#if defined( VK_USE_PLATFORM_ANDROID_KHR )
      case ExternalMemoryHandleTypeFlagBits::eAndroidHardwareBufferANDROID : return "AndroidHardwareBufferANDROID";
#endif /*VK_USE_PLATFORM_ANDROID_KHR*/
      case ExternalMemoryHandleTypeFlagBits::eHostAllocationEXT : return "HostAllocationEXT";
      case ExternalMemoryHandleTypeFlagBits::eHostMappedForeignMemoryEXT : return "HostMappedForeignMemoryEXT";
#if defined( VK_USE_PLATFORM_FUCHSIA )
      case ExternalMemoryHandleTypeFlagBits::eZirconVmoFUCHSIA : return "ZirconVmoFUCHSIA";
#endif /*VK_USE_PLATFORM_FUCHSIA*/
      case ExternalMemoryHandleTypeFlagBits::eRdmaAddressNV : return "RdmaAddressNV";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( ExternalMemoryFeatureFlagBits value )
  {
    switch ( value )
    {
      case ExternalMemoryFeatureFlagBits::eDedicatedOnly : return "DedicatedOnly";
      case ExternalMemoryFeatureFlagBits::eExportable : return "Exportable";
      case ExternalMemoryFeatureFlagBits::eImportable : return "Importable";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( ExternalFenceHandleTypeFlagBits value )
  {
    switch ( value )
    {
      case ExternalFenceHandleTypeFlagBits::eOpaqueFd : return "OpaqueFd";
      case ExternalFenceHandleTypeFlagBits::eOpaqueWin32 : return "OpaqueWin32";
      case ExternalFenceHandleTypeFlagBits::eOpaqueWin32Kmt : return "OpaqueWin32Kmt";
      case ExternalFenceHandleTypeFlagBits::eSyncFd : return "SyncFd";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( ExternalFenceFeatureFlagBits value )
  {
    switch ( value )
    {
      case ExternalFenceFeatureFlagBits::eExportable : return "Exportable";
      case ExternalFenceFeatureFlagBits::eImportable : return "Importable";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( FenceImportFlagBits value )
  {
    switch ( value )
    {
      case FenceImportFlagBits::eTemporary : return "Temporary";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( SemaphoreImportFlagBits value )
  {
    switch ( value )
    {
      case SemaphoreImportFlagBits::eTemporary : return "Temporary";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( ExternalSemaphoreHandleTypeFlagBits value )
  {
    switch ( value )
    {
      case ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd : return "OpaqueFd";
      case ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32 : return "OpaqueWin32";
      case ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32Kmt : return "OpaqueWin32Kmt";
      case ExternalSemaphoreHandleTypeFlagBits::eD3D12Fence : return "D3D12Fence";
      case ExternalSemaphoreHandleTypeFlagBits::eSyncFd : return "SyncFd";
#if defined( VK_USE_PLATFORM_FUCHSIA )
      case ExternalSemaphoreHandleTypeFlagBits::eZirconEventFUCHSIA : return "ZirconEventFUCHSIA";
#endif /*VK_USE_PLATFORM_FUCHSIA*/
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( ExternalSemaphoreFeatureFlagBits value )
  {
    switch ( value )
    {
      case ExternalSemaphoreFeatureFlagBits::eExportable : return "Exportable";
      case ExternalSemaphoreFeatureFlagBits::eImportable : return "Importable";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_VERSION_1_2 ===


  VULKAN_HPP_INLINE std::string to_string( DriverId value )
  {
    switch ( value )
    {
      case DriverId::eAmdProprietary : return "AmdProprietary";
      case DriverId::eAmdOpenSource : return "AmdOpenSource";
      case DriverId::eMesaRadv : return "MesaRadv";
      case DriverId::eNvidiaProprietary : return "NvidiaProprietary";
      case DriverId::eIntelProprietaryWindows : return "IntelProprietaryWindows";
      case DriverId::eIntelOpenSourceMESA : return "IntelOpenSourceMESA";
      case DriverId::eImaginationProprietary : return "ImaginationProprietary";
      case DriverId::eQualcommProprietary : return "QualcommProprietary";
      case DriverId::eArmProprietary : return "ArmProprietary";
      case DriverId::eGoogleSwiftshader : return "GoogleSwiftshader";
      case DriverId::eGgpProprietary : return "GgpProprietary";
      case DriverId::eBroadcomProprietary : return "BroadcomProprietary";
      case DriverId::eMesaLlvmpipe : return "MesaLlvmpipe";
      case DriverId::eMoltenvk : return "Moltenvk";
      case DriverId::eCoreaviProprietary : return "CoreaviProprietary";
      case DriverId::eJuiceProprietary : return "JuiceProprietary";
      case DriverId::eVerisiliconProprietary : return "VerisiliconProprietary";
      case DriverId::eMesaTurnip : return "MesaTurnip";
      case DriverId::eMesaV3Dv : return "MesaV3Dv";
      case DriverId::eMesaPanvk : return "MesaPanvk";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( ShaderFloatControlsIndependence value )
  {
    switch ( value )
    {
      case ShaderFloatControlsIndependence::e32BitOnly : return "32BitOnly";
      case ShaderFloatControlsIndependence::eAll : return "All";
      case ShaderFloatControlsIndependence::eNone : return "None";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( DescriptorBindingFlagBits value )
  {
    switch ( value )
    {
      case DescriptorBindingFlagBits::eUpdateAfterBind : return "UpdateAfterBind";
      case DescriptorBindingFlagBits::eUpdateUnusedWhilePending : return "UpdateUnusedWhilePending";
      case DescriptorBindingFlagBits::ePartiallyBound : return "PartiallyBound";
      case DescriptorBindingFlagBits::eVariableDescriptorCount : return "VariableDescriptorCount";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( ResolveModeFlagBits value )
  {
    switch ( value )
    {
      case ResolveModeFlagBits::eNone : return "None";
      case ResolveModeFlagBits::eSampleZero : return "SampleZero";
      case ResolveModeFlagBits::eAverage : return "Average";
      case ResolveModeFlagBits::eMin : return "Min";
      case ResolveModeFlagBits::eMax : return "Max";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( SamplerReductionMode value )
  {
    switch ( value )
    {
      case SamplerReductionMode::eWeightedAverage : return "WeightedAverage";
      case SamplerReductionMode::eMin : return "Min";
      case SamplerReductionMode::eMax : return "Max";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( SemaphoreType value )
  {
    switch ( value )
    {
      case SemaphoreType::eBinary : return "Binary";
      case SemaphoreType::eTimeline : return "Timeline";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( SemaphoreWaitFlagBits value )
  {
    switch ( value )
    {
      case SemaphoreWaitFlagBits::eAny : return "Any";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_KHR_surface ===


  VULKAN_HPP_INLINE std::string to_string( SurfaceTransformFlagBitsKHR value )
  {
    switch ( value )
    {
      case SurfaceTransformFlagBitsKHR::eIdentity : return "Identity";
      case SurfaceTransformFlagBitsKHR::eRotate90 : return "Rotate90";
      case SurfaceTransformFlagBitsKHR::eRotate180 : return "Rotate180";
      case SurfaceTransformFlagBitsKHR::eRotate270 : return "Rotate270";
      case SurfaceTransformFlagBitsKHR::eHorizontalMirror : return "HorizontalMirror";
      case SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate90 : return "HorizontalMirrorRotate90";
      case SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate180 : return "HorizontalMirrorRotate180";
      case SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate270 : return "HorizontalMirrorRotate270";
      case SurfaceTransformFlagBitsKHR::eInherit : return "Inherit";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( PresentModeKHR value )
  {
    switch ( value )
    {
      case PresentModeKHR::eImmediate : return "Immediate";
      case PresentModeKHR::eMailbox : return "Mailbox";
      case PresentModeKHR::eFifo : return "Fifo";
      case PresentModeKHR::eFifoRelaxed : return "FifoRelaxed";
      case PresentModeKHR::eSharedDemandRefresh : return "SharedDemandRefresh";
      case PresentModeKHR::eSharedContinuousRefresh : return "SharedContinuousRefresh";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( ColorSpaceKHR value )
  {
    switch ( value )
    {
      case ColorSpaceKHR::eSrgbNonlinear : return "SrgbNonlinear";
      case ColorSpaceKHR::eDisplayP3NonlinearEXT : return "DisplayP3NonlinearEXT";
      case ColorSpaceKHR::eExtendedSrgbLinearEXT : return "ExtendedSrgbLinearEXT";
      case ColorSpaceKHR::eDisplayP3LinearEXT : return "DisplayP3LinearEXT";
      case ColorSpaceKHR::eDciP3NonlinearEXT : return "DciP3NonlinearEXT";
      case ColorSpaceKHR::eBt709LinearEXT : return "Bt709LinearEXT";
      case ColorSpaceKHR::eBt709NonlinearEXT : return "Bt709NonlinearEXT";
      case ColorSpaceKHR::eBt2020LinearEXT : return "Bt2020LinearEXT";
      case ColorSpaceKHR::eHdr10St2084EXT : return "Hdr10St2084EXT";
      case ColorSpaceKHR::eDolbyvisionEXT : return "DolbyvisionEXT";
      case ColorSpaceKHR::eHdr10HlgEXT : return "Hdr10HlgEXT";
      case ColorSpaceKHR::eAdobergbLinearEXT : return "AdobergbLinearEXT";
      case ColorSpaceKHR::eAdobergbNonlinearEXT : return "AdobergbNonlinearEXT";
      case ColorSpaceKHR::ePassThroughEXT : return "PassThroughEXT";
      case ColorSpaceKHR::eExtendedSrgbNonlinearEXT : return "ExtendedSrgbNonlinearEXT";
      case ColorSpaceKHR::eDisplayNativeAMD : return "DisplayNativeAMD";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( CompositeAlphaFlagBitsKHR value )
  {
    switch ( value )
    {
      case CompositeAlphaFlagBitsKHR::eOpaque : return "Opaque";
      case CompositeAlphaFlagBitsKHR::ePreMultiplied : return "PreMultiplied";
      case CompositeAlphaFlagBitsKHR::ePostMultiplied : return "PostMultiplied";
      case CompositeAlphaFlagBitsKHR::eInherit : return "Inherit";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_KHR_swapchain ===


  VULKAN_HPP_INLINE std::string to_string( SwapchainCreateFlagBitsKHR value )
  {
    switch ( value )
    {
      case SwapchainCreateFlagBitsKHR::eSplitInstanceBindRegions : return "SplitInstanceBindRegions";
      case SwapchainCreateFlagBitsKHR::eProtected : return "Protected";
      case SwapchainCreateFlagBitsKHR::eMutableFormat : return "MutableFormat";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( DeviceGroupPresentModeFlagBitsKHR value )
  {
    switch ( value )
    {
      case DeviceGroupPresentModeFlagBitsKHR::eLocal : return "Local";
      case DeviceGroupPresentModeFlagBitsKHR::eRemote : return "Remote";
      case DeviceGroupPresentModeFlagBitsKHR::eSum : return "Sum";
      case DeviceGroupPresentModeFlagBitsKHR::eLocalMultiDevice : return "LocalMultiDevice";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_KHR_display ===


  VULKAN_HPP_INLINE std::string to_string( DisplayPlaneAlphaFlagBitsKHR value )
  {
    switch ( value )
    {
      case DisplayPlaneAlphaFlagBitsKHR::eOpaque : return "Opaque";
      case DisplayPlaneAlphaFlagBitsKHR::eGlobal : return "Global";
      case DisplayPlaneAlphaFlagBitsKHR::ePerPixel : return "PerPixel";
      case DisplayPlaneAlphaFlagBitsKHR::ePerPixelPremultiplied : return "PerPixelPremultiplied";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( DisplayModeCreateFlagBitsKHR )
  {
    return "(void)";
  }


  VULKAN_HPP_INLINE std::string to_string( DisplaySurfaceCreateFlagBitsKHR )
  {
    return "(void)";
  }

#if defined( VK_USE_PLATFORM_XLIB_KHR )
  //=== VK_KHR_xlib_surface ===


  VULKAN_HPP_INLINE std::string to_string( XlibSurfaceCreateFlagBitsKHR )
  {
    return "(void)";
  }
#endif /*VK_USE_PLATFORM_XLIB_KHR*/

#if defined( VK_USE_PLATFORM_XCB_KHR )
  //=== VK_KHR_xcb_surface ===


  VULKAN_HPP_INLINE std::string to_string( XcbSurfaceCreateFlagBitsKHR )
  {
    return "(void)";
  }
#endif /*VK_USE_PLATFORM_XCB_KHR*/

#if defined( VK_USE_PLATFORM_WAYLAND_KHR )
  //=== VK_KHR_wayland_surface ===


  VULKAN_HPP_INLINE std::string to_string( WaylandSurfaceCreateFlagBitsKHR )
  {
    return "(void)";
  }
#endif /*VK_USE_PLATFORM_WAYLAND_KHR*/

#if defined( VK_USE_PLATFORM_ANDROID_KHR )
  //=== VK_KHR_android_surface ===


  VULKAN_HPP_INLINE std::string to_string( AndroidSurfaceCreateFlagBitsKHR )
  {
    return "(void)";
  }
#endif /*VK_USE_PLATFORM_ANDROID_KHR*/

#if defined( VK_USE_PLATFORM_WIN32_KHR )
  //=== VK_KHR_win32_surface ===


  VULKAN_HPP_INLINE std::string to_string( Win32SurfaceCreateFlagBitsKHR )
  {
    return "(void)";
  }
#endif /*VK_USE_PLATFORM_WIN32_KHR*/

  //=== VK_EXT_debug_report ===


  VULKAN_HPP_INLINE std::string to_string( DebugReportFlagBitsEXT value )
  {
    switch ( value )
    {
      case DebugReportFlagBitsEXT::eInformation : return "Information";
      case DebugReportFlagBitsEXT::eWarning : return "Warning";
      case DebugReportFlagBitsEXT::ePerformanceWarning : return "PerformanceWarning";
      case DebugReportFlagBitsEXT::eError : return "Error";
      case DebugReportFlagBitsEXT::eDebug : return "Debug";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( DebugReportObjectTypeEXT value )
  {
    switch ( value )
    {
      case DebugReportObjectTypeEXT::eUnknown : return "Unknown";
      case DebugReportObjectTypeEXT::eInstance : return "Instance";
      case DebugReportObjectTypeEXT::ePhysicalDevice : return "PhysicalDevice";
      case DebugReportObjectTypeEXT::eDevice : return "Device";
      case DebugReportObjectTypeEXT::eQueue : return "Queue";
      case DebugReportObjectTypeEXT::eSemaphore : return "Semaphore";
      case DebugReportObjectTypeEXT::eCommandBuffer : return "CommandBuffer";
      case DebugReportObjectTypeEXT::eFence : return "Fence";
      case DebugReportObjectTypeEXT::eDeviceMemory : return "DeviceMemory";
      case DebugReportObjectTypeEXT::eBuffer : return "Buffer";
      case DebugReportObjectTypeEXT::eImage : return "Image";
      case DebugReportObjectTypeEXT::eEvent : return "Event";
      case DebugReportObjectTypeEXT::eQueryPool : return "QueryPool";
      case DebugReportObjectTypeEXT::eBufferView : return "BufferView";
      case DebugReportObjectTypeEXT::eImageView : return "ImageView";
      case DebugReportObjectTypeEXT::eShaderModule : return "ShaderModule";
      case DebugReportObjectTypeEXT::ePipelineCache : return "PipelineCache";
      case DebugReportObjectTypeEXT::ePipelineLayout : return "PipelineLayout";
      case DebugReportObjectTypeEXT::eRenderPass : return "RenderPass";
      case DebugReportObjectTypeEXT::ePipeline : return "Pipeline";
      case DebugReportObjectTypeEXT::eDescriptorSetLayout : return "DescriptorSetLayout";
      case DebugReportObjectTypeEXT::eSampler : return "Sampler";
      case DebugReportObjectTypeEXT::eDescriptorPool : return "DescriptorPool";
      case DebugReportObjectTypeEXT::eDescriptorSet : return "DescriptorSet";
      case DebugReportObjectTypeEXT::eFramebuffer : return "Framebuffer";
      case DebugReportObjectTypeEXT::eCommandPool : return "CommandPool";
      case DebugReportObjectTypeEXT::eSurfaceKHR : return "SurfaceKHR";
      case DebugReportObjectTypeEXT::eSwapchainKHR : return "SwapchainKHR";
      case DebugReportObjectTypeEXT::eDebugReportCallbackEXT : return "DebugReportCallbackEXT";
      case DebugReportObjectTypeEXT::eDisplayKHR : return "DisplayKHR";
      case DebugReportObjectTypeEXT::eDisplayModeKHR : return "DisplayModeKHR";
      case DebugReportObjectTypeEXT::eValidationCacheEXT : return "ValidationCacheEXT";
      case DebugReportObjectTypeEXT::eSamplerYcbcrConversion : return "SamplerYcbcrConversion";
      case DebugReportObjectTypeEXT::eDescriptorUpdateTemplate : return "DescriptorUpdateTemplate";
      case DebugReportObjectTypeEXT::eCuModuleNVX : return "CuModuleNVX";
      case DebugReportObjectTypeEXT::eCuFunctionNVX : return "CuFunctionNVX";
      case DebugReportObjectTypeEXT::eAccelerationStructureKHR : return "AccelerationStructureKHR";
      case DebugReportObjectTypeEXT::eAccelerationStructureNV : return "AccelerationStructureNV";
#if defined( VK_USE_PLATFORM_FUCHSIA )
      case DebugReportObjectTypeEXT::eBufferCollectionFUCHSIA : return "BufferCollectionFUCHSIA";
#endif /*VK_USE_PLATFORM_FUCHSIA*/
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_AMD_rasterization_order ===


  VULKAN_HPP_INLINE std::string to_string( RasterizationOrderAMD value )
  {
    switch ( value )
    {
      case RasterizationOrderAMD::eStrict : return "Strict";
      case RasterizationOrderAMD::eRelaxed : return "Relaxed";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

#if defined( VK_ENABLE_BETA_EXTENSIONS )
  //=== VK_KHR_video_queue ===


  VULKAN_HPP_INLINE std::string to_string( VideoCodecOperationFlagBitsKHR value )
  {
    switch ( value )
    {
      case VideoCodecOperationFlagBitsKHR::eInvalid : return "Invalid";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
      case VideoCodecOperationFlagBitsKHR::eEncodeH264EXT : return "EncodeH264EXT";
      case VideoCodecOperationFlagBitsKHR::eEncodeH265EXT : return "EncodeH265EXT";
      case VideoCodecOperationFlagBitsKHR::eDecodeH264EXT : return "DecodeH264EXT";
      case VideoCodecOperationFlagBitsKHR::eDecodeH265EXT : return "DecodeH265EXT";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( VideoChromaSubsamplingFlagBitsKHR value )
  {
    switch ( value )
    {
      case VideoChromaSubsamplingFlagBitsKHR::eInvalid : return "Invalid";
      case VideoChromaSubsamplingFlagBitsKHR::eMonochrome : return "Monochrome";
      case VideoChromaSubsamplingFlagBitsKHR::e420 : return "420";
      case VideoChromaSubsamplingFlagBitsKHR::e422 : return "422";
      case VideoChromaSubsamplingFlagBitsKHR::e444 : return "444";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( VideoComponentBitDepthFlagBitsKHR value )
  {
    switch ( value )
    {
      case VideoComponentBitDepthFlagBitsKHR::eInvalid : return "Invalid";
      case VideoComponentBitDepthFlagBitsKHR::e8 : return "8";
      case VideoComponentBitDepthFlagBitsKHR::e10 : return "10";
      case VideoComponentBitDepthFlagBitsKHR::e12 : return "12";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( VideoCapabilityFlagBitsKHR value )
  {
    switch ( value )
    {
      case VideoCapabilityFlagBitsKHR::eProtectedContent : return "ProtectedContent";
      case VideoCapabilityFlagBitsKHR::eSeparateReferenceImages : return "SeparateReferenceImages";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( VideoSessionCreateFlagBitsKHR value )
  {
    switch ( value )
    {
      case VideoSessionCreateFlagBitsKHR::eDefault : return "Default";
      case VideoSessionCreateFlagBitsKHR::eProtectedContent : return "ProtectedContent";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( VideoCodingControlFlagBitsKHR value )
  {
    switch ( value )
    {
      case VideoCodingControlFlagBitsKHR::eDefault : return "Default";
      case VideoCodingControlFlagBitsKHR::eReset : return "Reset";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( VideoCodingQualityPresetFlagBitsKHR value )
  {
    switch ( value )
    {
      case VideoCodingQualityPresetFlagBitsKHR::eNormal : return "Normal";
      case VideoCodingQualityPresetFlagBitsKHR::ePower : return "Power";
      case VideoCodingQualityPresetFlagBitsKHR::eQuality : return "Quality";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( QueryResultStatusKHR value )
  {
    switch ( value )
    {
      case QueryResultStatusKHR::eError : return "Error";
      case QueryResultStatusKHR::eNotReady : return "NotReady";
      case QueryResultStatusKHR::eComplete : return "Complete";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( VideoBeginCodingFlagBitsKHR )
  {
    return "(void)";
  }


  VULKAN_HPP_INLINE std::string to_string( VideoEndCodingFlagBitsKHR )
  {
    return "(void)";
  }
#endif /*VK_ENABLE_BETA_EXTENSIONS*/

#if defined( VK_ENABLE_BETA_EXTENSIONS )
  //=== VK_KHR_video_decode_queue ===


  VULKAN_HPP_INLINE std::string to_string( VideoDecodeFlagBitsKHR value )
  {
    switch ( value )
    {
      case VideoDecodeFlagBitsKHR::eDefault : return "Default";
      case VideoDecodeFlagBitsKHR::eReserved0 : return "Reserved0";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }
#endif /*VK_ENABLE_BETA_EXTENSIONS*/

  //=== VK_EXT_transform_feedback ===


  VULKAN_HPP_INLINE std::string to_string( PipelineRasterizationStateStreamCreateFlagBitsEXT )
  {
    return "(void)";
  }

#if defined( VK_ENABLE_BETA_EXTENSIONS )
  //=== VK_EXT_video_encode_h264 ===


  VULKAN_HPP_INLINE std::string to_string( VideoEncodeH264CapabilityFlagBitsEXT value )
  {
    switch ( value )
    {
      case VideoEncodeH264CapabilityFlagBitsEXT::eCabac : return "Cabac";
      case VideoEncodeH264CapabilityFlagBitsEXT::eCavlc : return "Cavlc";
      case VideoEncodeH264CapabilityFlagBitsEXT::eWeightedBiPredImplicit : return "WeightedBiPredImplicit";
      case VideoEncodeH264CapabilityFlagBitsEXT::eTransform8X8 : return "Transform8X8";
      case VideoEncodeH264CapabilityFlagBitsEXT::eChromaQpOffset : return "ChromaQpOffset";
      case VideoEncodeH264CapabilityFlagBitsEXT::eSecondChromaQpOffset : return "SecondChromaQpOffset";
      case VideoEncodeH264CapabilityFlagBitsEXT::eDeblockingFilterDisabled : return "DeblockingFilterDisabled";
      case VideoEncodeH264CapabilityFlagBitsEXT::eDeblockingFilterEnabled : return "DeblockingFilterEnabled";
      case VideoEncodeH264CapabilityFlagBitsEXT::eDeblockingFilterPartial : return "DeblockingFilterPartial";
      case VideoEncodeH264CapabilityFlagBitsEXT::eMultipleSlicePerFrame : return "MultipleSlicePerFrame";
      case VideoEncodeH264CapabilityFlagBitsEXT::eEvenlyDistributedSliceSize : return "EvenlyDistributedSliceSize";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( VideoEncodeH264InputModeFlagBitsEXT value )
  {
    switch ( value )
    {
      case VideoEncodeH264InputModeFlagBitsEXT::eFrame : return "Frame";
      case VideoEncodeH264InputModeFlagBitsEXT::eSlice : return "Slice";
      case VideoEncodeH264InputModeFlagBitsEXT::eNonVcl : return "NonVcl";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( VideoEncodeH264OutputModeFlagBitsEXT value )
  {
    switch ( value )
    {
      case VideoEncodeH264OutputModeFlagBitsEXT::eFrame : return "Frame";
      case VideoEncodeH264OutputModeFlagBitsEXT::eSlice : return "Slice";
      case VideoEncodeH264OutputModeFlagBitsEXT::eNonVcl : return "NonVcl";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( VideoEncodeH264CreateFlagBitsEXT value )
  {
    switch ( value )
    {
      case VideoEncodeH264CreateFlagBitsEXT::eDefault : return "Default";
      case VideoEncodeH264CreateFlagBitsEXT::eReserved0 : return "Reserved0";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }
#endif /*VK_ENABLE_BETA_EXTENSIONS*/

#if defined( VK_ENABLE_BETA_EXTENSIONS )
  //=== VK_EXT_video_encode_h265 ===


  VULKAN_HPP_INLINE std::string to_string( VideoEncodeH265InputModeFlagBitsEXT value )
  {
    switch ( value )
    {
      case VideoEncodeH265InputModeFlagBitsEXT::eFrame : return "Frame";
      case VideoEncodeH265InputModeFlagBitsEXT::eSlice : return "Slice";
      case VideoEncodeH265InputModeFlagBitsEXT::eNonVcl : return "NonVcl";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( VideoEncodeH265OutputModeFlagBitsEXT value )
  {
    switch ( value )
    {
      case VideoEncodeH265OutputModeFlagBitsEXT::eFrame : return "Frame";
      case VideoEncodeH265OutputModeFlagBitsEXT::eSlice : return "Slice";
      case VideoEncodeH265OutputModeFlagBitsEXT::eNonVcl : return "NonVcl";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( VideoEncodeH265CtbSizeFlagBitsEXT value )
  {
    switch ( value )
    {
      case VideoEncodeH265CtbSizeFlagBitsEXT::e8 : return "8";
      case VideoEncodeH265CtbSizeFlagBitsEXT::e16 : return "16";
      case VideoEncodeH265CtbSizeFlagBitsEXT::e32 : return "32";
      case VideoEncodeH265CtbSizeFlagBitsEXT::e64 : return "64";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( VideoEncodeH265CapabilityFlagBitsEXT )
  {
    return "(void)";
  }


  VULKAN_HPP_INLINE std::string to_string( VideoEncodeH265CreateFlagBitsEXT )
  {
    return "(void)";
  }
#endif /*VK_ENABLE_BETA_EXTENSIONS*/

#if defined( VK_ENABLE_BETA_EXTENSIONS )
  //=== VK_EXT_video_decode_h264 ===


  VULKAN_HPP_INLINE std::string to_string( VideoDecodeH264PictureLayoutFlagBitsEXT value )
  {
    switch ( value )
    {
      case VideoDecodeH264PictureLayoutFlagBitsEXT::eProgressive : return "Progressive";
      case VideoDecodeH264PictureLayoutFlagBitsEXT::eInterlacedInterleavedLines : return "InterlacedInterleavedLines";
      case VideoDecodeH264PictureLayoutFlagBitsEXT::eInterlacedSeparatePlanes : return "InterlacedSeparatePlanes";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( VideoDecodeH264CreateFlagBitsEXT )
  {
    return "(void)";
  }
#endif /*VK_ENABLE_BETA_EXTENSIONS*/

  //=== VK_AMD_shader_info ===


  VULKAN_HPP_INLINE std::string to_string( ShaderInfoTypeAMD value )
  {
    switch ( value )
    {
      case ShaderInfoTypeAMD::eStatistics : return "Statistics";
      case ShaderInfoTypeAMD::eBinary : return "Binary";
      case ShaderInfoTypeAMD::eDisassembly : return "Disassembly";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_KHR_dynamic_rendering ===


  VULKAN_HPP_INLINE std::string to_string( RenderingFlagBitsKHR value )
  {
    switch ( value )
    {
      case RenderingFlagBitsKHR::eContentsSecondaryCommandBuffers : return "ContentsSecondaryCommandBuffers";
      case RenderingFlagBitsKHR::eSuspending : return "Suspending";
      case RenderingFlagBitsKHR::eResuming : return "Resuming";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

#if defined( VK_USE_PLATFORM_GGP )
  //=== VK_GGP_stream_descriptor_surface ===


  VULKAN_HPP_INLINE std::string to_string( StreamDescriptorSurfaceCreateFlagBitsGGP )
  {
    return "(void)";
  }
#endif /*VK_USE_PLATFORM_GGP*/

  //=== VK_NV_external_memory_capabilities ===


  VULKAN_HPP_INLINE std::string to_string( ExternalMemoryHandleTypeFlagBitsNV value )
  {
    switch ( value )
    {
      case ExternalMemoryHandleTypeFlagBitsNV::eOpaqueWin32 : return "OpaqueWin32";
      case ExternalMemoryHandleTypeFlagBitsNV::eOpaqueWin32Kmt : return "OpaqueWin32Kmt";
      case ExternalMemoryHandleTypeFlagBitsNV::eD3D11Image : return "D3D11Image";
      case ExternalMemoryHandleTypeFlagBitsNV::eD3D11ImageKmt : return "D3D11ImageKmt";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( ExternalMemoryFeatureFlagBitsNV value )
  {
    switch ( value )
    {
      case ExternalMemoryFeatureFlagBitsNV::eDedicatedOnly : return "DedicatedOnly";
      case ExternalMemoryFeatureFlagBitsNV::eExportable : return "Exportable";
      case ExternalMemoryFeatureFlagBitsNV::eImportable : return "Importable";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_EXT_validation_flags ===


  VULKAN_HPP_INLINE std::string to_string( ValidationCheckEXT value )
  {
    switch ( value )
    {
      case ValidationCheckEXT::eAll : return "All";
      case ValidationCheckEXT::eShaders : return "Shaders";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

#if defined( VK_USE_PLATFORM_VI_NN )
  //=== VK_NN_vi_surface ===


  VULKAN_HPP_INLINE std::string to_string( ViSurfaceCreateFlagBitsNN )
  {
    return "(void)";
  }
#endif /*VK_USE_PLATFORM_VI_NN*/

  //=== VK_EXT_conditional_rendering ===


  VULKAN_HPP_INLINE std::string to_string( ConditionalRenderingFlagBitsEXT value )
  {
    switch ( value )
    {
      case ConditionalRenderingFlagBitsEXT::eInverted : return "Inverted";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_EXT_display_surface_counter ===


  VULKAN_HPP_INLINE std::string to_string( SurfaceCounterFlagBitsEXT value )
  {
    switch ( value )
    {
      case SurfaceCounterFlagBitsEXT::eVblank : return "Vblank";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_EXT_display_control ===


  VULKAN_HPP_INLINE std::string to_string( DisplayPowerStateEXT value )
  {
    switch ( value )
    {
      case DisplayPowerStateEXT::eOff : return "Off";
      case DisplayPowerStateEXT::eSuspend : return "Suspend";
      case DisplayPowerStateEXT::eOn : return "On";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( DeviceEventTypeEXT value )
  {
    switch ( value )
    {
      case DeviceEventTypeEXT::eDisplayHotplug : return "DisplayHotplug";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( DisplayEventTypeEXT value )
  {
    switch ( value )
    {
      case DisplayEventTypeEXT::eFirstPixelOut : return "FirstPixelOut";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_NV_viewport_swizzle ===


  VULKAN_HPP_INLINE std::string to_string( ViewportCoordinateSwizzleNV value )
  {
    switch ( value )
    {
      case ViewportCoordinateSwizzleNV::ePositiveX : return "PositiveX";
      case ViewportCoordinateSwizzleNV::eNegativeX : return "NegativeX";
      case ViewportCoordinateSwizzleNV::ePositiveY : return "PositiveY";
      case ViewportCoordinateSwizzleNV::eNegativeY : return "NegativeY";
      case ViewportCoordinateSwizzleNV::ePositiveZ : return "PositiveZ";
      case ViewportCoordinateSwizzleNV::eNegativeZ : return "NegativeZ";
      case ViewportCoordinateSwizzleNV::ePositiveW : return "PositiveW";
      case ViewportCoordinateSwizzleNV::eNegativeW : return "NegativeW";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( PipelineViewportSwizzleStateCreateFlagBitsNV )
  {
    return "(void)";
  }

  //=== VK_EXT_discard_rectangles ===


  VULKAN_HPP_INLINE std::string to_string( DiscardRectangleModeEXT value )
  {
    switch ( value )
    {
      case DiscardRectangleModeEXT::eInclusive : return "Inclusive";
      case DiscardRectangleModeEXT::eExclusive : return "Exclusive";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( PipelineDiscardRectangleStateCreateFlagBitsEXT )
  {
    return "(void)";
  }

  //=== VK_EXT_conservative_rasterization ===


  VULKAN_HPP_INLINE std::string to_string( ConservativeRasterizationModeEXT value )
  {
    switch ( value )
    {
      case ConservativeRasterizationModeEXT::eDisabled : return "Disabled";
      case ConservativeRasterizationModeEXT::eOverestimate : return "Overestimate";
      case ConservativeRasterizationModeEXT::eUnderestimate : return "Underestimate";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( PipelineRasterizationConservativeStateCreateFlagBitsEXT )
  {
    return "(void)";
  }

  //=== VK_EXT_depth_clip_enable ===


  VULKAN_HPP_INLINE std::string to_string( PipelineRasterizationDepthClipStateCreateFlagBitsEXT )
  {
    return "(void)";
  }

  //=== VK_KHR_performance_query ===


  VULKAN_HPP_INLINE std::string to_string( PerformanceCounterDescriptionFlagBitsKHR value )
  {
    switch ( value )
    {
      case PerformanceCounterDescriptionFlagBitsKHR::ePerformanceImpacting : return "PerformanceImpacting";
      case PerformanceCounterDescriptionFlagBitsKHR::eConcurrentlyImpacted : return "ConcurrentlyImpacted";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( PerformanceCounterScopeKHR value )
  {
    switch ( value )
    {
      case PerformanceCounterScopeKHR::eCommandBuffer : return "CommandBuffer";
      case PerformanceCounterScopeKHR::eRenderPass : return "RenderPass";
      case PerformanceCounterScopeKHR::eCommand : return "Command";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( PerformanceCounterStorageKHR value )
  {
    switch ( value )
    {
      case PerformanceCounterStorageKHR::eInt32 : return "Int32";
      case PerformanceCounterStorageKHR::eInt64 : return "Int64";
      case PerformanceCounterStorageKHR::eUint32 : return "Uint32";
      case PerformanceCounterStorageKHR::eUint64 : return "Uint64";
      case PerformanceCounterStorageKHR::eFloat32 : return "Float32";
      case PerformanceCounterStorageKHR::eFloat64 : return "Float64";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( PerformanceCounterUnitKHR value )
  {
    switch ( value )
    {
      case PerformanceCounterUnitKHR::eGeneric : return "Generic";
      case PerformanceCounterUnitKHR::ePercentage : return "Percentage";
      case PerformanceCounterUnitKHR::eNanoseconds : return "Nanoseconds";
      case PerformanceCounterUnitKHR::eBytes : return "Bytes";
      case PerformanceCounterUnitKHR::eBytesPerSecond : return "BytesPerSecond";
      case PerformanceCounterUnitKHR::eKelvin : return "Kelvin";
      case PerformanceCounterUnitKHR::eWatts : return "Watts";
      case PerformanceCounterUnitKHR::eVolts : return "Volts";
      case PerformanceCounterUnitKHR::eAmps : return "Amps";
      case PerformanceCounterUnitKHR::eHertz : return "Hertz";
      case PerformanceCounterUnitKHR::eCycles : return "Cycles";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( AcquireProfilingLockFlagBitsKHR )
  {
    return "(void)";
  }

#if defined( VK_USE_PLATFORM_IOS_MVK )
  //=== VK_MVK_ios_surface ===


  VULKAN_HPP_INLINE std::string to_string( IOSSurfaceCreateFlagBitsMVK )
  {
    return "(void)";
  }
#endif /*VK_USE_PLATFORM_IOS_MVK*/

#if defined( VK_USE_PLATFORM_MACOS_MVK )
  //=== VK_MVK_macos_surface ===


  VULKAN_HPP_INLINE std::string to_string( MacOSSurfaceCreateFlagBitsMVK )
  {
    return "(void)";
  }
#endif /*VK_USE_PLATFORM_MACOS_MVK*/

  //=== VK_EXT_debug_utils ===


  VULKAN_HPP_INLINE std::string to_string( DebugUtilsMessageSeverityFlagBitsEXT value )
  {
    switch ( value )
    {
      case DebugUtilsMessageSeverityFlagBitsEXT::eVerbose : return "Verbose";
      case DebugUtilsMessageSeverityFlagBitsEXT::eInfo : return "Info";
      case DebugUtilsMessageSeverityFlagBitsEXT::eWarning : return "Warning";
      case DebugUtilsMessageSeverityFlagBitsEXT::eError : return "Error";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( DebugUtilsMessageTypeFlagBitsEXT value )
  {
    switch ( value )
    {
      case DebugUtilsMessageTypeFlagBitsEXT::eGeneral : return "General";
      case DebugUtilsMessageTypeFlagBitsEXT::eValidation : return "Validation";
      case DebugUtilsMessageTypeFlagBitsEXT::ePerformance : return "Performance";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( DebugUtilsMessengerCallbackDataFlagBitsEXT )
  {
    return "(void)";
  }


  VULKAN_HPP_INLINE std::string to_string( DebugUtilsMessengerCreateFlagBitsEXT )
  {
    return "(void)";
  }

  //=== VK_EXT_blend_operation_advanced ===


  VULKAN_HPP_INLINE std::string to_string( BlendOverlapEXT value )
  {
    switch ( value )
    {
      case BlendOverlapEXT::eUncorrelated : return "Uncorrelated";
      case BlendOverlapEXT::eDisjoint : return "Disjoint";
      case BlendOverlapEXT::eConjoint : return "Conjoint";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_NV_fragment_coverage_to_color ===


  VULKAN_HPP_INLINE std::string to_string( PipelineCoverageToColorStateCreateFlagBitsNV )
  {
    return "(void)";
  }

  //=== VK_KHR_acceleration_structure ===


  VULKAN_HPP_INLINE std::string to_string( AccelerationStructureTypeKHR value )
  {
    switch ( value )
    {
      case AccelerationStructureTypeKHR::eTopLevel : return "TopLevel";
      case AccelerationStructureTypeKHR::eBottomLevel : return "BottomLevel";
      case AccelerationStructureTypeKHR::eGeneric : return "Generic";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( AccelerationStructureBuildTypeKHR value )
  {
    switch ( value )
    {
      case AccelerationStructureBuildTypeKHR::eHost : return "Host";
      case AccelerationStructureBuildTypeKHR::eDevice : return "Device";
      case AccelerationStructureBuildTypeKHR::eHostOrDevice : return "HostOrDevice";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( GeometryFlagBitsKHR value )
  {
    switch ( value )
    {
      case GeometryFlagBitsKHR::eOpaque : return "Opaque";
      case GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation : return "NoDuplicateAnyHitInvocation";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( GeometryInstanceFlagBitsKHR value )
  {
    switch ( value )
    {
      case GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable : return "TriangleFacingCullDisable";
      case GeometryInstanceFlagBitsKHR::eTriangleFlipFacing : return "TriangleFlipFacing";
      case GeometryInstanceFlagBitsKHR::eForceOpaque : return "ForceOpaque";
      case GeometryInstanceFlagBitsKHR::eForceNoOpaque : return "ForceNoOpaque";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( BuildAccelerationStructureFlagBitsKHR value )
  {
    switch ( value )
    {
      case BuildAccelerationStructureFlagBitsKHR::eAllowUpdate : return "AllowUpdate";
      case BuildAccelerationStructureFlagBitsKHR::eAllowCompaction : return "AllowCompaction";
      case BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace : return "PreferFastTrace";
      case BuildAccelerationStructureFlagBitsKHR::ePreferFastBuild : return "PreferFastBuild";
      case BuildAccelerationStructureFlagBitsKHR::eLowMemory : return "LowMemory";
      case BuildAccelerationStructureFlagBitsKHR::eMotionNV : return "MotionNV";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( CopyAccelerationStructureModeKHR value )
  {
    switch ( value )
    {
      case CopyAccelerationStructureModeKHR::eClone : return "Clone";
      case CopyAccelerationStructureModeKHR::eCompact : return "Compact";
      case CopyAccelerationStructureModeKHR::eSerialize : return "Serialize";
      case CopyAccelerationStructureModeKHR::eDeserialize : return "Deserialize";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( GeometryTypeKHR value )
  {
    switch ( value )
    {
      case GeometryTypeKHR::eTriangles : return "Triangles";
      case GeometryTypeKHR::eAabbs : return "Aabbs";
      case GeometryTypeKHR::eInstances : return "Instances";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( AccelerationStructureCompatibilityKHR value )
  {
    switch ( value )
    {
      case AccelerationStructureCompatibilityKHR::eCompatible : return "Compatible";
      case AccelerationStructureCompatibilityKHR::eIncompatible : return "Incompatible";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( AccelerationStructureCreateFlagBitsKHR value )
  {
    switch ( value )
    {
      case AccelerationStructureCreateFlagBitsKHR::eDeviceAddressCaptureReplay : return "DeviceAddressCaptureReplay";
      case AccelerationStructureCreateFlagBitsKHR::eMotionNV : return "MotionNV";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( BuildAccelerationStructureModeKHR value )
  {
    switch ( value )
    {
      case BuildAccelerationStructureModeKHR::eBuild : return "Build";
      case BuildAccelerationStructureModeKHR::eUpdate : return "Update";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_NV_framebuffer_mixed_samples ===


  VULKAN_HPP_INLINE std::string to_string( CoverageModulationModeNV value )
  {
    switch ( value )
    {
      case CoverageModulationModeNV::eNone : return "None";
      case CoverageModulationModeNV::eRgb : return "Rgb";
      case CoverageModulationModeNV::eAlpha : return "Alpha";
      case CoverageModulationModeNV::eRgba : return "Rgba";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( PipelineCoverageModulationStateCreateFlagBitsNV )
  {
    return "(void)";
  }

  //=== VK_EXT_validation_cache ===


  VULKAN_HPP_INLINE std::string to_string( ValidationCacheHeaderVersionEXT value )
  {
    switch ( value )
    {
      case ValidationCacheHeaderVersionEXT::eOne : return "One";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( ValidationCacheCreateFlagBitsEXT )
  {
    return "(void)";
  }

  //=== VK_NV_shading_rate_image ===


  VULKAN_HPP_INLINE std::string to_string( ShadingRatePaletteEntryNV value )
  {
    switch ( value )
    {
      case ShadingRatePaletteEntryNV::eNoInvocations : return "NoInvocations";
      case ShadingRatePaletteEntryNV::e16InvocationsPerPixel : return "16InvocationsPerPixel";
      case ShadingRatePaletteEntryNV::e8InvocationsPerPixel : return "8InvocationsPerPixel";
      case ShadingRatePaletteEntryNV::e4InvocationsPerPixel : return "4InvocationsPerPixel";
      case ShadingRatePaletteEntryNV::e2InvocationsPerPixel : return "2InvocationsPerPixel";
      case ShadingRatePaletteEntryNV::e1InvocationPerPixel : return "1InvocationPerPixel";
      case ShadingRatePaletteEntryNV::e1InvocationPer2X1Pixels : return "1InvocationPer2X1Pixels";
      case ShadingRatePaletteEntryNV::e1InvocationPer1X2Pixels : return "1InvocationPer1X2Pixels";
      case ShadingRatePaletteEntryNV::e1InvocationPer2X2Pixels : return "1InvocationPer2X2Pixels";
      case ShadingRatePaletteEntryNV::e1InvocationPer4X2Pixels : return "1InvocationPer4X2Pixels";
      case ShadingRatePaletteEntryNV::e1InvocationPer2X4Pixels : return "1InvocationPer2X4Pixels";
      case ShadingRatePaletteEntryNV::e1InvocationPer4X4Pixels : return "1InvocationPer4X4Pixels";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( CoarseSampleOrderTypeNV value )
  {
    switch ( value )
    {
      case CoarseSampleOrderTypeNV::eDefault : return "Default";
      case CoarseSampleOrderTypeNV::eCustom : return "Custom";
      case CoarseSampleOrderTypeNV::ePixelMajor : return "PixelMajor";
      case CoarseSampleOrderTypeNV::eSampleMajor : return "SampleMajor";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_NV_ray_tracing ===


  VULKAN_HPP_INLINE std::string to_string( AccelerationStructureMemoryRequirementsTypeNV value )
  {
    switch ( value )
    {
      case AccelerationStructureMemoryRequirementsTypeNV::eObject : return "Object";
      case AccelerationStructureMemoryRequirementsTypeNV::eBuildScratch : return "BuildScratch";
      case AccelerationStructureMemoryRequirementsTypeNV::eUpdateScratch : return "UpdateScratch";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_EXT_global_priority ===


  VULKAN_HPP_INLINE std::string to_string( QueueGlobalPriorityEXT value )
  {
    switch ( value )
    {
      case QueueGlobalPriorityEXT::eLow : return "Low";
      case QueueGlobalPriorityEXT::eMedium : return "Medium";
      case QueueGlobalPriorityEXT::eHigh : return "High";
      case QueueGlobalPriorityEXT::eRealtime : return "Realtime";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_AMD_pipeline_compiler_control ===


  VULKAN_HPP_INLINE std::string to_string( PipelineCompilerControlFlagBitsAMD )
  {
    return "(void)";
  }

  //=== VK_EXT_calibrated_timestamps ===


  VULKAN_HPP_INLINE std::string to_string( TimeDomainEXT value )
  {
    switch ( value )
    {
      case TimeDomainEXT::eDevice : return "Device";
      case TimeDomainEXT::eClockMonotonic : return "ClockMonotonic";
      case TimeDomainEXT::eClockMonotonicRaw : return "ClockMonotonicRaw";
      case TimeDomainEXT::eQueryPerformanceCounter : return "QueryPerformanceCounter";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

#if defined( VK_ENABLE_BETA_EXTENSIONS )
  //=== VK_EXT_video_decode_h265 ===


  VULKAN_HPP_INLINE std::string to_string( VideoDecodeH265CreateFlagBitsEXT )
  {
    return "(void)";
  }
#endif /*VK_ENABLE_BETA_EXTENSIONS*/

  //=== VK_AMD_memory_overallocation_behavior ===


  VULKAN_HPP_INLINE std::string to_string( MemoryOverallocationBehaviorAMD value )
  {
    switch ( value )
    {
      case MemoryOverallocationBehaviorAMD::eDefault : return "Default";
      case MemoryOverallocationBehaviorAMD::eAllowed : return "Allowed";
      case MemoryOverallocationBehaviorAMD::eDisallowed : return "Disallowed";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_EXT_pipeline_creation_feedback ===


  VULKAN_HPP_INLINE std::string to_string( PipelineCreationFeedbackFlagBitsEXT value )
  {
    switch ( value )
    {
      case PipelineCreationFeedbackFlagBitsEXT::eValid : return "Valid";
      case PipelineCreationFeedbackFlagBitsEXT::eApplicationPipelineCacheHit : return "ApplicationPipelineCacheHit";
      case PipelineCreationFeedbackFlagBitsEXT::eBasePipelineAcceleration : return "BasePipelineAcceleration";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_INTEL_performance_query ===


  VULKAN_HPP_INLINE std::string to_string( PerformanceConfigurationTypeINTEL value )
  {
    switch ( value )
    {
      case PerformanceConfigurationTypeINTEL::eCommandQueueMetricsDiscoveryActivated : return "CommandQueueMetricsDiscoveryActivated";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( QueryPoolSamplingModeINTEL value )
  {
    switch ( value )
    {
      case QueryPoolSamplingModeINTEL::eManual : return "Manual";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( PerformanceOverrideTypeINTEL value )
  {
    switch ( value )
    {
      case PerformanceOverrideTypeINTEL::eNullHardware : return "NullHardware";
      case PerformanceOverrideTypeINTEL::eFlushGpuCaches : return "FlushGpuCaches";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( PerformanceParameterTypeINTEL value )
  {
    switch ( value )
    {
      case PerformanceParameterTypeINTEL::eHwCountersSupported : return "HwCountersSupported";
      case PerformanceParameterTypeINTEL::eStreamMarkerValidBits : return "StreamMarkerValidBits";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( PerformanceValueTypeINTEL value )
  {
    switch ( value )
    {
      case PerformanceValueTypeINTEL::eUint32 : return "Uint32";
      case PerformanceValueTypeINTEL::eUint64 : return "Uint64";
      case PerformanceValueTypeINTEL::eFloat : return "Float";
      case PerformanceValueTypeINTEL::eBool : return "Bool";
      case PerformanceValueTypeINTEL::eString : return "String";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

#if defined( VK_USE_PLATFORM_FUCHSIA )
  //=== VK_FUCHSIA_imagepipe_surface ===


  VULKAN_HPP_INLINE std::string to_string( ImagePipeSurfaceCreateFlagBitsFUCHSIA )
  {
    return "(void)";
  }
#endif /*VK_USE_PLATFORM_FUCHSIA*/

#if defined( VK_USE_PLATFORM_METAL_EXT )
  //=== VK_EXT_metal_surface ===


  VULKAN_HPP_INLINE std::string to_string( MetalSurfaceCreateFlagBitsEXT )
  {
    return "(void)";
  }
#endif /*VK_USE_PLATFORM_METAL_EXT*/

  //=== VK_KHR_fragment_shading_rate ===


  VULKAN_HPP_INLINE std::string to_string( FragmentShadingRateCombinerOpKHR value )
  {
    switch ( value )
    {
      case FragmentShadingRateCombinerOpKHR::eKeep : return "Keep";
      case FragmentShadingRateCombinerOpKHR::eReplace : return "Replace";
      case FragmentShadingRateCombinerOpKHR::eMin : return "Min";
      case FragmentShadingRateCombinerOpKHR::eMax : return "Max";
      case FragmentShadingRateCombinerOpKHR::eMul : return "Mul";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_AMD_shader_core_properties2 ===


  VULKAN_HPP_INLINE std::string to_string( ShaderCorePropertiesFlagBitsAMD )
  {
    return "(void)";
  }

  //=== VK_EXT_tooling_info ===


  VULKAN_HPP_INLINE std::string to_string( ToolPurposeFlagBitsEXT value )
  {
    switch ( value )
    {
      case ToolPurposeFlagBitsEXT::eValidation : return "Validation";
      case ToolPurposeFlagBitsEXT::eProfiling : return "Profiling";
      case ToolPurposeFlagBitsEXT::eTracing : return "Tracing";
      case ToolPurposeFlagBitsEXT::eAdditionalFeatures : return "AdditionalFeatures";
      case ToolPurposeFlagBitsEXT::eModifyingFeatures : return "ModifyingFeatures";
      case ToolPurposeFlagBitsEXT::eDebugReporting : return "DebugReporting";
      case ToolPurposeFlagBitsEXT::eDebugMarkers : return "DebugMarkers";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_EXT_validation_features ===


  VULKAN_HPP_INLINE std::string to_string( ValidationFeatureEnableEXT value )
  {
    switch ( value )
    {
      case ValidationFeatureEnableEXT::eGpuAssisted : return "GpuAssisted";
      case ValidationFeatureEnableEXT::eGpuAssistedReserveBindingSlot : return "GpuAssistedReserveBindingSlot";
      case ValidationFeatureEnableEXT::eBestPractices : return "BestPractices";
      case ValidationFeatureEnableEXT::eDebugPrintf : return "DebugPrintf";
      case ValidationFeatureEnableEXT::eSynchronizationValidation : return "SynchronizationValidation";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( ValidationFeatureDisableEXT value )
  {
    switch ( value )
    {
      case ValidationFeatureDisableEXT::eAll : return "All";
      case ValidationFeatureDisableEXT::eShaders : return "Shaders";
      case ValidationFeatureDisableEXT::eThreadSafety : return "ThreadSafety";
      case ValidationFeatureDisableEXT::eApiParameters : return "ApiParameters";
      case ValidationFeatureDisableEXT::eObjectLifetimes : return "ObjectLifetimes";
      case ValidationFeatureDisableEXT::eCoreChecks : return "CoreChecks";
      case ValidationFeatureDisableEXT::eUniqueHandles : return "UniqueHandles";
      case ValidationFeatureDisableEXT::eShaderValidationCache : return "ShaderValidationCache";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_NV_cooperative_matrix ===


  VULKAN_HPP_INLINE std::string to_string( ScopeNV value )
  {
    switch ( value )
    {
      case ScopeNV::eDevice : return "Device";
      case ScopeNV::eWorkgroup : return "Workgroup";
      case ScopeNV::eSubgroup : return "Subgroup";
      case ScopeNV::eQueueFamily : return "QueueFamily";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( ComponentTypeNV value )
  {
    switch ( value )
    {
      case ComponentTypeNV::eFloat16 : return "Float16";
      case ComponentTypeNV::eFloat32 : return "Float32";
      case ComponentTypeNV::eFloat64 : return "Float64";
      case ComponentTypeNV::eSint8 : return "Sint8";
      case ComponentTypeNV::eSint16 : return "Sint16";
      case ComponentTypeNV::eSint32 : return "Sint32";
      case ComponentTypeNV::eSint64 : return "Sint64";
      case ComponentTypeNV::eUint8 : return "Uint8";
      case ComponentTypeNV::eUint16 : return "Uint16";
      case ComponentTypeNV::eUint32 : return "Uint32";
      case ComponentTypeNV::eUint64 : return "Uint64";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_NV_coverage_reduction_mode ===


  VULKAN_HPP_INLINE std::string to_string( CoverageReductionModeNV value )
  {
    switch ( value )
    {
      case CoverageReductionModeNV::eMerge : return "Merge";
      case CoverageReductionModeNV::eTruncate : return "Truncate";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( PipelineCoverageReductionStateCreateFlagBitsNV )
  {
    return "(void)";
  }

  //=== VK_EXT_provoking_vertex ===


  VULKAN_HPP_INLINE std::string to_string( ProvokingVertexModeEXT value )
  {
    switch ( value )
    {
      case ProvokingVertexModeEXT::eFirstVertex : return "FirstVertex";
      case ProvokingVertexModeEXT::eLastVertex : return "LastVertex";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

#if defined( VK_USE_PLATFORM_WIN32_KHR )
  //=== VK_EXT_full_screen_exclusive ===


  VULKAN_HPP_INLINE std::string to_string( FullScreenExclusiveEXT value )
  {
    switch ( value )
    {
      case FullScreenExclusiveEXT::eDefault : return "Default";
      case FullScreenExclusiveEXT::eAllowed : return "Allowed";
      case FullScreenExclusiveEXT::eDisallowed : return "Disallowed";
      case FullScreenExclusiveEXT::eApplicationControlled : return "ApplicationControlled";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }
#endif /*VK_USE_PLATFORM_WIN32_KHR*/

  //=== VK_EXT_headless_surface ===


  VULKAN_HPP_INLINE std::string to_string( HeadlessSurfaceCreateFlagBitsEXT )
  {
    return "(void)";
  }

  //=== VK_EXT_line_rasterization ===


  VULKAN_HPP_INLINE std::string to_string( LineRasterizationModeEXT value )
  {
    switch ( value )
    {
      case LineRasterizationModeEXT::eDefault : return "Default";
      case LineRasterizationModeEXT::eRectangular : return "Rectangular";
      case LineRasterizationModeEXT::eBresenham : return "Bresenham";
      case LineRasterizationModeEXT::eRectangularSmooth : return "RectangularSmooth";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_KHR_pipeline_executable_properties ===


  VULKAN_HPP_INLINE std::string to_string( PipelineExecutableStatisticFormatKHR value )
  {
    switch ( value )
    {
      case PipelineExecutableStatisticFormatKHR::eBool32 : return "Bool32";
      case PipelineExecutableStatisticFormatKHR::eInt64 : return "Int64";
      case PipelineExecutableStatisticFormatKHR::eUint64 : return "Uint64";
      case PipelineExecutableStatisticFormatKHR::eFloat64 : return "Float64";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_NV_device_generated_commands ===


  VULKAN_HPP_INLINE std::string to_string( IndirectStateFlagBitsNV value )
  {
    switch ( value )
    {
      case IndirectStateFlagBitsNV::eFlagFrontface : return "FlagFrontface";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( IndirectCommandsTokenTypeNV value )
  {
    switch ( value )
    {
      case IndirectCommandsTokenTypeNV::eShaderGroup : return "ShaderGroup";
      case IndirectCommandsTokenTypeNV::eStateFlags : return "StateFlags";
      case IndirectCommandsTokenTypeNV::eIndexBuffer : return "IndexBuffer";
      case IndirectCommandsTokenTypeNV::eVertexBuffer : return "VertexBuffer";
      case IndirectCommandsTokenTypeNV::ePushConstant : return "PushConstant";
      case IndirectCommandsTokenTypeNV::eDrawIndexed : return "DrawIndexed";
      case IndirectCommandsTokenTypeNV::eDraw : return "Draw";
      case IndirectCommandsTokenTypeNV::eDrawTasks : return "DrawTasks";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( IndirectCommandsLayoutUsageFlagBitsNV value )
  {
    switch ( value )
    {
      case IndirectCommandsLayoutUsageFlagBitsNV::eExplicitPreprocess : return "ExplicitPreprocess";
      case IndirectCommandsLayoutUsageFlagBitsNV::eIndexedSequences : return "IndexedSequences";
      case IndirectCommandsLayoutUsageFlagBitsNV::eUnorderedSequences : return "UnorderedSequences";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_EXT_device_memory_report ===


  VULKAN_HPP_INLINE std::string to_string( DeviceMemoryReportEventTypeEXT value )
  {
    switch ( value )
    {
      case DeviceMemoryReportEventTypeEXT::eAllocate : return "Allocate";
      case DeviceMemoryReportEventTypeEXT::eFree : return "Free";
      case DeviceMemoryReportEventTypeEXT::eImport : return "Import";
      case DeviceMemoryReportEventTypeEXT::eUnimport : return "Unimport";
      case DeviceMemoryReportEventTypeEXT::eAllocationFailed : return "AllocationFailed";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( DeviceMemoryReportFlagBitsEXT )
  {
    return "(void)";
  }

  //=== VK_EXT_private_data ===


  VULKAN_HPP_INLINE std::string to_string( PrivateDataSlotCreateFlagBitsEXT )
  {
    return "(void)";
  }

  //=== VK_EXT_pipeline_creation_cache_control ===


  VULKAN_HPP_INLINE std::string to_string( PipelineCacheCreateFlagBits value )
  {
    switch ( value )
    {
      case PipelineCacheCreateFlagBits::eExternallySynchronizedEXT : return "ExternallySynchronizedEXT";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

#if defined( VK_ENABLE_BETA_EXTENSIONS )
  //=== VK_KHR_video_encode_queue ===


  VULKAN_HPP_INLINE std::string to_string( VideoEncodeFlagBitsKHR value )
  {
    switch ( value )
    {
      case VideoEncodeFlagBitsKHR::eDefault : return "Default";
      case VideoEncodeFlagBitsKHR::eReserved0 : return "Reserved0";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( VideoEncodeRateControlFlagBitsKHR value )
  {
    switch ( value )
    {
      case VideoEncodeRateControlFlagBitsKHR::eDefault : return "Default";
      case VideoEncodeRateControlFlagBitsKHR::eReset : return "Reset";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( VideoEncodeRateControlModeFlagBitsKHR value )
  {
    switch ( value )
    {
      case VideoEncodeRateControlModeFlagBitsKHR::eNone : return "None";
      case VideoEncodeRateControlModeFlagBitsKHR::eCbr : return "Cbr";
      case VideoEncodeRateControlModeFlagBitsKHR::eVbr : return "Vbr";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }
#endif /*VK_ENABLE_BETA_EXTENSIONS*/

  //=== VK_NV_device_diagnostics_config ===


  VULKAN_HPP_INLINE std::string to_string( DeviceDiagnosticsConfigFlagBitsNV value )
  {
    switch ( value )
    {
      case DeviceDiagnosticsConfigFlagBitsNV::eEnableShaderDebugInfo : return "EnableShaderDebugInfo";
      case DeviceDiagnosticsConfigFlagBitsNV::eEnableResourceTracking : return "EnableResourceTracking";
      case DeviceDiagnosticsConfigFlagBitsNV::eEnableAutomaticCheckpoints : return "EnableAutomaticCheckpoints";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_KHR_synchronization2 ===


  VULKAN_HPP_INLINE std::string to_string( PipelineStageFlagBits2KHR value )
  {
    switch ( value )
    {
      case PipelineStageFlagBits2KHR::eNone : return "None";
      case PipelineStageFlagBits2KHR::eTopOfPipe : return "TopOfPipe";
      case PipelineStageFlagBits2KHR::eDrawIndirect : return "DrawIndirect";
      case PipelineStageFlagBits2KHR::eVertexInput : return "VertexInput";
      case PipelineStageFlagBits2KHR::eVertexShader : return "VertexShader";
      case PipelineStageFlagBits2KHR::eTessellationControlShader : return "TessellationControlShader";
      case PipelineStageFlagBits2KHR::eTessellationEvaluationShader : return "TessellationEvaluationShader";
      case PipelineStageFlagBits2KHR::eGeometryShader : return "GeometryShader";
      case PipelineStageFlagBits2KHR::eFragmentShader : return "FragmentShader";
      case PipelineStageFlagBits2KHR::eEarlyFragmentTests : return "EarlyFragmentTests";
      case PipelineStageFlagBits2KHR::eLateFragmentTests : return "LateFragmentTests";
      case PipelineStageFlagBits2KHR::eColorAttachmentOutput : return "ColorAttachmentOutput";
      case PipelineStageFlagBits2KHR::eComputeShader : return "ComputeShader";
      case PipelineStageFlagBits2KHR::eAllTransfer : return "AllTransfer";
      case PipelineStageFlagBits2KHR::eBottomOfPipe : return "BottomOfPipe";
      case PipelineStageFlagBits2KHR::eHost : return "Host";
      case PipelineStageFlagBits2KHR::eAllGraphics : return "AllGraphics";
      case PipelineStageFlagBits2KHR::eAllCommands : return "AllCommands";
      case PipelineStageFlagBits2KHR::eCopy : return "Copy";
      case PipelineStageFlagBits2KHR::eResolve : return "Resolve";
      case PipelineStageFlagBits2KHR::eBlit : return "Blit";
      case PipelineStageFlagBits2KHR::eClear : return "Clear";
      case PipelineStageFlagBits2KHR::eIndexInput : return "IndexInput";
      case PipelineStageFlagBits2KHR::eVertexAttributeInput : return "VertexAttributeInput";
      case PipelineStageFlagBits2KHR::ePreRasterizationShaders : return "PreRasterizationShaders";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
      case PipelineStageFlagBits2KHR::eVideoDecode : return "VideoDecode";
      case PipelineStageFlagBits2KHR::eVideoEncode : return "VideoEncode";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
      case PipelineStageFlagBits2KHR::eTransformFeedbackEXT : return "TransformFeedbackEXT";
      case PipelineStageFlagBits2KHR::eConditionalRenderingEXT : return "ConditionalRenderingEXT";
      case PipelineStageFlagBits2KHR::eCommandPreprocessNV : return "CommandPreprocessNV";
      case PipelineStageFlagBits2KHR::eFragmentShadingRateAttachment : return "FragmentShadingRateAttachment";
      case PipelineStageFlagBits2KHR::eAccelerationStructureBuild : return "AccelerationStructureBuild";
      case PipelineStageFlagBits2KHR::eRayTracingShader : return "RayTracingShader";
      case PipelineStageFlagBits2KHR::eFragmentDensityProcessEXT : return "FragmentDensityProcessEXT";
      case PipelineStageFlagBits2KHR::eTaskShaderNV : return "TaskShaderNV";
      case PipelineStageFlagBits2KHR::eMeshShaderNV : return "MeshShaderNV";
      case PipelineStageFlagBits2KHR::eSubpassShadingHUAWEI : return "SubpassShadingHUAWEI";
      case PipelineStageFlagBits2KHR::eInvocationMaskHUAWEI : return "InvocationMaskHUAWEI";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( AccessFlagBits2KHR value )
  {
    switch ( value )
    {
      case AccessFlagBits2KHR::eNone : return "None";
      case AccessFlagBits2KHR::eIndirectCommandRead : return "IndirectCommandRead";
      case AccessFlagBits2KHR::eIndexRead : return "IndexRead";
      case AccessFlagBits2KHR::eVertexAttributeRead : return "VertexAttributeRead";
      case AccessFlagBits2KHR::eUniformRead : return "UniformRead";
      case AccessFlagBits2KHR::eInputAttachmentRead : return "InputAttachmentRead";
      case AccessFlagBits2KHR::eShaderRead : return "ShaderRead";
      case AccessFlagBits2KHR::eShaderWrite : return "ShaderWrite";
      case AccessFlagBits2KHR::eColorAttachmentRead : return "ColorAttachmentRead";
      case AccessFlagBits2KHR::eColorAttachmentWrite : return "ColorAttachmentWrite";
      case AccessFlagBits2KHR::eDepthStencilAttachmentRead : return "DepthStencilAttachmentRead";
      case AccessFlagBits2KHR::eDepthStencilAttachmentWrite : return "DepthStencilAttachmentWrite";
      case AccessFlagBits2KHR::eTransferRead : return "TransferRead";
      case AccessFlagBits2KHR::eTransferWrite : return "TransferWrite";
      case AccessFlagBits2KHR::eHostRead : return "HostRead";
      case AccessFlagBits2KHR::eHostWrite : return "HostWrite";
      case AccessFlagBits2KHR::eMemoryRead : return "MemoryRead";
      case AccessFlagBits2KHR::eMemoryWrite : return "MemoryWrite";
      case AccessFlagBits2KHR::eShaderSampledRead : return "ShaderSampledRead";
      case AccessFlagBits2KHR::eShaderStorageRead : return "ShaderStorageRead";
      case AccessFlagBits2KHR::eShaderStorageWrite : return "ShaderStorageWrite";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
      case AccessFlagBits2KHR::eVideoDecodeRead : return "VideoDecodeRead";
      case AccessFlagBits2KHR::eVideoDecodeWrite : return "VideoDecodeWrite";
      case AccessFlagBits2KHR::eVideoEncodeRead : return "VideoEncodeRead";
      case AccessFlagBits2KHR::eVideoEncodeWrite : return "VideoEncodeWrite";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
      case AccessFlagBits2KHR::eTransformFeedbackWriteEXT : return "TransformFeedbackWriteEXT";
      case AccessFlagBits2KHR::eTransformFeedbackCounterReadEXT : return "TransformFeedbackCounterReadEXT";
      case AccessFlagBits2KHR::eTransformFeedbackCounterWriteEXT : return "TransformFeedbackCounterWriteEXT";
      case AccessFlagBits2KHR::eConditionalRenderingReadEXT : return "ConditionalRenderingReadEXT";
      case AccessFlagBits2KHR::eCommandPreprocessReadNV : return "CommandPreprocessReadNV";
      case AccessFlagBits2KHR::eCommandPreprocessWriteNV : return "CommandPreprocessWriteNV";
      case AccessFlagBits2KHR::eFragmentShadingRateAttachmentRead : return "FragmentShadingRateAttachmentRead";
      case AccessFlagBits2KHR::eAccelerationStructureRead : return "AccelerationStructureRead";
      case AccessFlagBits2KHR::eAccelerationStructureWrite : return "AccelerationStructureWrite";
      case AccessFlagBits2KHR::eFragmentDensityMapReadEXT : return "FragmentDensityMapReadEXT";
      case AccessFlagBits2KHR::eColorAttachmentReadNoncoherentEXT : return "ColorAttachmentReadNoncoherentEXT";
      case AccessFlagBits2KHR::eInvocationMaskReadHUAWEI : return "InvocationMaskReadHUAWEI";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( SubmitFlagBitsKHR value )
  {
    switch ( value )
    {
      case SubmitFlagBitsKHR::eProtected : return "Protected";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_NV_fragment_shading_rate_enums ===


  VULKAN_HPP_INLINE std::string to_string( FragmentShadingRateNV value )
  {
    switch ( value )
    {
      case FragmentShadingRateNV::e1InvocationPerPixel : return "1InvocationPerPixel";
      case FragmentShadingRateNV::e1InvocationPer1X2Pixels : return "1InvocationPer1X2Pixels";
      case FragmentShadingRateNV::e1InvocationPer2X1Pixels : return "1InvocationPer2X1Pixels";
      case FragmentShadingRateNV::e1InvocationPer2X2Pixels : return "1InvocationPer2X2Pixels";
      case FragmentShadingRateNV::e1InvocationPer2X4Pixels : return "1InvocationPer2X4Pixels";
      case FragmentShadingRateNV::e1InvocationPer4X2Pixels : return "1InvocationPer4X2Pixels";
      case FragmentShadingRateNV::e1InvocationPer4X4Pixels : return "1InvocationPer4X4Pixels";
      case FragmentShadingRateNV::e2InvocationsPerPixel : return "2InvocationsPerPixel";
      case FragmentShadingRateNV::e4InvocationsPerPixel : return "4InvocationsPerPixel";
      case FragmentShadingRateNV::e8InvocationsPerPixel : return "8InvocationsPerPixel";
      case FragmentShadingRateNV::e16InvocationsPerPixel : return "16InvocationsPerPixel";
      case FragmentShadingRateNV::eNoInvocations : return "NoInvocations";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( FragmentShadingRateTypeNV value )
  {
    switch ( value )
    {
      case FragmentShadingRateTypeNV::eFragmentSize : return "FragmentSize";
      case FragmentShadingRateTypeNV::eEnums : return "Enums";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_NV_ray_tracing_motion_blur ===


  VULKAN_HPP_INLINE std::string to_string( AccelerationStructureMotionInstanceTypeNV value )
  {
    switch ( value )
    {
      case AccelerationStructureMotionInstanceTypeNV::eStatic : return "Static";
      case AccelerationStructureMotionInstanceTypeNV::eMatrixMotion : return "MatrixMotion";
      case AccelerationStructureMotionInstanceTypeNV::eSrtMotion : return "SrtMotion";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( AccelerationStructureMotionInfoFlagBitsNV )
  {
    return "(void)";
  }


  VULKAN_HPP_INLINE std::string to_string( AccelerationStructureMotionInstanceFlagBitsNV )
  {
    return "(void)";
  }

#if defined( VK_USE_PLATFORM_DIRECTFB_EXT )
  //=== VK_EXT_directfb_surface ===


  VULKAN_HPP_INLINE std::string to_string( DirectFBSurfaceCreateFlagBitsEXT )
  {
    return "(void)";
  }
#endif /*VK_USE_PLATFORM_DIRECTFB_EXT*/

  //=== VK_KHR_ray_tracing_pipeline ===


  VULKAN_HPP_INLINE std::string to_string( RayTracingShaderGroupTypeKHR value )
  {
    switch ( value )
    {
      case RayTracingShaderGroupTypeKHR::eGeneral : return "General";
      case RayTracingShaderGroupTypeKHR::eTrianglesHitGroup : return "TrianglesHitGroup";
      case RayTracingShaderGroupTypeKHR::eProceduralHitGroup : return "ProceduralHitGroup";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( ShaderGroupShaderKHR value )
  {
    switch ( value )
    {
      case ShaderGroupShaderKHR::eGeneral : return "General";
      case ShaderGroupShaderKHR::eClosestHit : return "ClosestHit";
      case ShaderGroupShaderKHR::eAnyHit : return "AnyHit";
      case ShaderGroupShaderKHR::eIntersection : return "Intersection";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

  //=== VK_KHR_format_feature_flags2 ===


  VULKAN_HPP_INLINE std::string to_string( FormatFeatureFlagBits2KHR value )
  {
    switch ( value )
    {
      case FormatFeatureFlagBits2KHR::eSampledImage : return "SampledImage";
      case FormatFeatureFlagBits2KHR::eStorageImage : return "StorageImage";
      case FormatFeatureFlagBits2KHR::eStorageImageAtomic : return "StorageImageAtomic";
      case FormatFeatureFlagBits2KHR::eUniformTexelBuffer : return "UniformTexelBuffer";
      case FormatFeatureFlagBits2KHR::eStorageTexelBuffer : return "StorageTexelBuffer";
      case FormatFeatureFlagBits2KHR::eStorageTexelBufferAtomic : return "StorageTexelBufferAtomic";
      case FormatFeatureFlagBits2KHR::eVertexBuffer : return "VertexBuffer";
      case FormatFeatureFlagBits2KHR::eColorAttachment : return "ColorAttachment";
      case FormatFeatureFlagBits2KHR::eColorAttachmentBlend : return "ColorAttachmentBlend";
      case FormatFeatureFlagBits2KHR::eDepthStencilAttachment : return "DepthStencilAttachment";
      case FormatFeatureFlagBits2KHR::eBlitSrc : return "BlitSrc";
      case FormatFeatureFlagBits2KHR::eBlitDst : return "BlitDst";
      case FormatFeatureFlagBits2KHR::eSampledImageFilterLinear : return "SampledImageFilterLinear";
      case FormatFeatureFlagBits2KHR::eSampledImageFilterCubicEXT : return "SampledImageFilterCubicEXT";
      case FormatFeatureFlagBits2KHR::eTransferSrc : return "TransferSrc";
      case FormatFeatureFlagBits2KHR::eTransferDst : return "TransferDst";
      case FormatFeatureFlagBits2KHR::eSampledImageFilterMinmax : return "SampledImageFilterMinmax";
      case FormatFeatureFlagBits2KHR::eMidpointChromaSamples : return "MidpointChromaSamples";
      case FormatFeatureFlagBits2KHR::eSampledImageYcbcrConversionLinearFilter : return "SampledImageYcbcrConversionLinearFilter";
      case FormatFeatureFlagBits2KHR::eSampledImageYcbcrConversionSeparateReconstructionFilter : return "SampledImageYcbcrConversionSeparateReconstructionFilter";
      case FormatFeatureFlagBits2KHR::eSampledImageYcbcrConversionChromaReconstructionExplicit : return "SampledImageYcbcrConversionChromaReconstructionExplicit";
      case FormatFeatureFlagBits2KHR::eSampledImageYcbcrConversionChromaReconstructionExplicitForceable : return "SampledImageYcbcrConversionChromaReconstructionExplicitForceable";
      case FormatFeatureFlagBits2KHR::eDisjoint : return "Disjoint";
      case FormatFeatureFlagBits2KHR::eCositedChromaSamples : return "CositedChromaSamples";
      case FormatFeatureFlagBits2KHR::eStorageReadWithoutFormat : return "StorageReadWithoutFormat";
      case FormatFeatureFlagBits2KHR::eStorageWriteWithoutFormat : return "StorageWriteWithoutFormat";
      case FormatFeatureFlagBits2KHR::eSampledImageDepthComparison : return "SampledImageDepthComparison";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
      case FormatFeatureFlagBits2KHR::eVideoDecodeOutput : return "VideoDecodeOutput";
      case FormatFeatureFlagBits2KHR::eVideoDecodeDpb : return "VideoDecodeDpb";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
      case FormatFeatureFlagBits2KHR::eAccelerationStructureVertexBuffer : return "AccelerationStructureVertexBuffer";
      case FormatFeatureFlagBits2KHR::eFragmentDensityMapEXT : return "FragmentDensityMapEXT";
      case FormatFeatureFlagBits2KHR::eFragmentShadingRateAttachment : return "FragmentShadingRateAttachment";
#if defined( VK_ENABLE_BETA_EXTENSIONS )
      case FormatFeatureFlagBits2KHR::eVideoEncodeInput : return "VideoEncodeInput";
      case FormatFeatureFlagBits2KHR::eVideoEncodeDpb : return "VideoEncodeDpb";
#endif /*VK_ENABLE_BETA_EXTENSIONS*/
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }

#if defined( VK_USE_PLATFORM_FUCHSIA )
  //=== VK_FUCHSIA_buffer_collection ===


  VULKAN_HPP_INLINE std::string to_string( ImageConstraintsInfoFlagBitsFUCHSIA value )
  {
    switch ( value )
    {
      case ImageConstraintsInfoFlagBitsFUCHSIA::eCpuReadRarely : return "CpuReadRarely";
      case ImageConstraintsInfoFlagBitsFUCHSIA::eCpuReadOften : return "CpuReadOften";
      case ImageConstraintsInfoFlagBitsFUCHSIA::eCpuWriteRarely : return "CpuWriteRarely";
      case ImageConstraintsInfoFlagBitsFUCHSIA::eCpuWriteOften : return "CpuWriteOften";
      case ImageConstraintsInfoFlagBitsFUCHSIA::eProtectedOptional : return "ProtectedOptional";
      default: return "invalid ( " + VULKAN_HPP_NAMESPACE::toHexString( static_cast<uint32_t>( value ) ) + " )";
    }

  }


  VULKAN_HPP_INLINE std::string to_string( ImageFormatConstraintsFlagBitsFUCHSIA )
  {
    return "(void)";
  }
#endif /*VK_USE_PLATFORM_FUCHSIA*/

#if defined( VK_USE_PLATFORM_SCREEN_QNX )
  //=== VK_QNX_screen_surface ===


  VULKAN_HPP_INLINE std::string to_string( ScreenSurfaceCreateFlagBitsQNX )
  {
    return "(void)";
  }
#endif /*VK_USE_PLATFORM_SCREEN_QNX*/


} // namespace VULKAN_HPP_NAMESPACE
#endif
