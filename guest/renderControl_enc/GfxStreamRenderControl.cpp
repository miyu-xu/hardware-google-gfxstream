/*
 * Copyright 2024 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include "GfxStreamRenderControl.h"

#include "GfxStreamRenderControlConnection.h"

int32_t renderControlInit(GfxStreamConnectionManager* mgr, void* vkInfo) {
    auto stream = mgr->getStream();
    uint64_t puid = stream->processPipeInit();

    auto rcConnection = std::make_unique<GfxStreamRenderControlConnection>(stream);
    ExtendedRCEncoderContext* rcEnc = (ExtendedRCEncoderContext*)rcConnection->getEncoder();
    gfxstream::guest::ChecksumCalculator* calc = rcConnection->getCheckSumHelper();

    rcEnc->setChecksumHelper(calc);
    rcEnc->queryAndSetSyncImpl();
    rcEnc->queryAndSetDmaImpl();
    rcEnc->queryAndSetGLESMaxVersion();
    rcEnc->queryAndSetHostCompositionImpl();
    rcEnc->queryAndSetDirectMemSupport();
    rcEnc->queryAndSetVulkanSupport();
    rcEnc->queryAndSetDeferredVulkanCommandsSupport();
    rcEnc->queryAndSetVulkanNullOptionalStringsSupport();
    rcEnc->queryAndSetVulkanCreateResourcesWithRequirementsSupport();
    rcEnc->queryAndSetVulkanIgnoredHandles();
    rcEnc->queryAndSetYUVCache();
    rcEnc->queryAndSetAsyncUnmapBuffer();
    rcEnc->queryAndSetVirtioGpuNext();
    rcEnc->queryHasSharedSlotsHostMemoryAllocator();
    rcEnc->queryAndSetVulkanFreeMemorySync();
    rcEnc->queryAndSetVirtioGpuNativeSync();
    rcEnc->queryAndSetVulkanShaderFloat16Int8Support();
    rcEnc->queryAndSetVulkanAsyncQueueSubmitSupport();
    rcEnc->queryAndSetHostSideTracingSupport();
    rcEnc->queryAndSetAsyncFrameCommands();
    rcEnc->queryAndSetVulkanQueueSubmitWithCommandsSupport();
    rcEnc->queryAndSetVulkanBatchedDescriptorSetUpdateSupport();
    rcEnc->queryAndSetSyncBufferData();
    rcEnc->queryAndSetVulkanAsyncQsri();
    rcEnc->queryAndSetReadColorBufferDma();
    rcEnc->queryAndSetHWCMultiConfigs();
    rcEnc->queryAndSetVulkanAuxCommandBufferMemory();
    rcEnc->queryVersion();

    rcEnc->rcSetPuid(rcEnc, puid);

    rcEnc->setVulkanFeatureInfo(vkInfo);

    return 0;
}
