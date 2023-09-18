from .common.codegen import CodeGen, VulkanWrapperGenerator
from .common.vulkantypes import \
        VulkanAPI, makeVulkanTypeSimple, iterateVulkanType
from .common.vulkantypes import EXCLUDED_APIS

RESOURCE_TRACKER_ENTRIES = [
    "vkEnumerateInstanceExtensionProperties",
    "vkEnumerateDeviceExtensionProperties",
    "vkEnumeratePhysicalDevices",
    "vkAllocateMemory",
    "vkFreeMemory",
    "vkCreateImage",
    "vkDestroyImage",
    "vkGetImageMemoryRequirements",
    "vkGetImageMemoryRequirements2",
    "vkGetImageMemoryRequirements2KHR",
    "vkBindImageMemory",
    "vkBindImageMemory2",
    "vkBindImageMemory2KHR",
    "vkCreateBuffer",
    "vkDestroyBuffer",
    "vkGetBufferMemoryRequirements",
    "vkGetBufferMemoryRequirements2",
    "vkGetBufferMemoryRequirements2KHR",
    "vkBindBufferMemory",
    "vkBindBufferMemory2",
    "vkBindBufferMemory2KHR",
    "vkCreateSemaphore",
    "vkDestroySemaphore",
    "vkQueueSubmit",
    "vkQueueSubmit2",
    "vkQueueWaitIdle",
    "vkImportSemaphoreFdKHR",
    "vkGetSemaphoreFdKHR",
    # Warning: These need to be defined in vk.xml (currently no-op) {
    "vkGetMemoryFuchsiaHandleKHR",
    "vkGetMemoryFuchsiaHandlePropertiesKHR",
    "vkGetSemaphoreFuchsiaHandleKHR",
    "vkImportSemaphoreFuchsiaHandleKHR",
    # } end Warning: These need to be defined in vk.xml (currently no-op)
    "vkGetAndroidHardwareBufferPropertiesANDROID",
    "vkGetMemoryAndroidHardwareBufferANDROID",
    "vkCreateSamplerYcbcrConversion",
    "vkDestroySamplerYcbcrConversion",
    "vkCreateSamplerYcbcrConversionKHR",
    "vkDestroySamplerYcbcrConversionKHR",
    "vkUpdateDescriptorSetWithTemplate",
    "vkGetPhysicalDeviceImageFormatProperties2",
    "vkGetPhysicalDeviceImageFormatProperties2KHR",
    "vkBeginCommandBuffer",
    "vkEndCommandBuffer",
    "vkResetCommandBuffer",
    "vkCreateImageView",
    "vkCreateSampler",
    "vkGetPhysicalDeviceExternalFenceProperties",
    "vkGetPhysicalDeviceExternalFencePropertiesKHR",
    "vkGetPhysicalDeviceExternalBufferProperties",
    "vkGetPhysicalDeviceExternalBufferPropertiesKHR",
    "vkCreateFence",
    "vkResetFences",
    "vkImportFenceFdKHR",
    "vkGetFenceFdKHR",
    "vkWaitForFences",
    "vkCreateDescriptorPool",
    "vkDestroyDescriptorPool",
    "vkResetDescriptorPool",
    "vkAllocateDescriptorSets",
    "vkFreeDescriptorSets",
    "vkCreateDescriptorSetLayout",
    "vkUpdateDescriptorSets",
    "vkCmdExecuteCommands",
    "vkCmdBindDescriptorSets",
    "vkDestroyDescriptorSetLayout",
    "vkAllocateCommandBuffers",
    "vkQueueSignalReleaseImageANDROID",
    "vkCmdPipelineBarrier",
    "vkCreateGraphicsPipelines",
    # Fuchsia
    "vkGetMemoryZirconHandleFUCHSIA",
    "vkGetMemoryZirconHandlePropertiesFUCHSIA",
    "vkGetSemaphoreZirconHandleFUCHSIA",
    "vkImportSemaphoreZirconHandleFUCHSIA",
    "vkCreateBufferCollectionFUCHSIA",
    "vkDestroyBufferCollectionFUCHSIA",
    "vkSetBufferCollectionImageConstraintsFUCHSIA",
    "vkSetBufferCollectionBufferConstraintsFUCHSIA",
    "vkGetBufferCollectionPropertiesFUCHSIA",
]

SPECIAL_ENTRIES = [
    "vkCreateInstance",
    "vkDestroyInstance",
    "vkGetInstanceProcAddr",
    "vkEnumerateInstanceExtensionProperties",
]


SUCCESS_VAL = {
    "VkResult" : ["VK_SUCCESS"],
}

POSTPROCESSES = {
    "vkResetCommandPool" : """if (vkResetCommandPool_VkResult_return == VK_SUCCESS) {
        gfxstream::vk::ResourceTracker::get()->resetCommandPoolStagingInfo(commandPool);
    }""",
    "vkAllocateCommandBuffers" : """if (vkAllocateCommandBuffers_VkResult_return == VK_SUCCESS) {
        gfxstream::vk::ResourceTracker::get()->addToCommandPool(pAllocateInfo->commandPool, pAllocateInfo->commandBufferCount, pCommandBuffers);
    }""",
}

def is_cmdbuf_dispatch(api):
    return "VkCommandBuffer" == api.parameters[0].typeName

def is_queue_dispatch(api):
    return "VkQueue" == api.parameters[0].typeName

class VulkanFuncTable(VulkanWrapperGenerator):
    def __init__(self, module, typeInfo):
        VulkanWrapperGenerator.__init__(self, module, typeInfo)
        self.typeInfo = typeInfo
        self.cgen = CodeGen()
        self.entries = []
        self.entryFeatures = []
        self.cmdToFeatureType = {}
        self.feature = None
        self.featureType = None

    def onBegin(self,):
        cgen = self.cgen
        self.module.appendImpl(cgen.swapCode())
        pass

    def onBeginFeature(self, featureName, featureType):
        self.feature = featureName
        self.featureType = featureType

    def onEndFeature(self):
        self.feature = None
        self.featureType = None

    def onFeatureNewCmd(self, name):
        self.cmdToFeatureType[name] = self.featureType

    def onGenCmd(self, cmdinfo, name, alias):
        typeInfo = self.typeInfo
        cgen = self.cgen
        api = typeInfo.apis[name]
        self.entries.append(api)
        self.entryFeatures.append(self.feature)

        def genEncoderOrResourceTrackerCall(cgen, api, declareResources=True):
            cgen.stmt("AEMU_SCOPED_TRACE(\"%s\")" % api.name)

            if is_cmdbuf_dispatch(api):
                cgen.stmt("auto vkEnc = gfxstream::vk::ResourceTracker::getCommandBufferEncoder(commandBuffer)")
            elif is_queue_dispatch(api):
                cgen.stmt("auto vkEnc = gfxstream::vk::ResourceTracker::getQueueEncoder(queue)")
            else:
                cgen.stmt("auto vkEnc = gfxstream::vk::ResourceTracker::getThreadLocalEncoder()")
            callLhs = None
            retTypeName = api.getRetTypeExpr()
            if retTypeName != "void":
                retVar = api.getRetVarExpr()
                cgen.stmt("%s %s = (%s)0" % (retTypeName, retVar, retTypeName))
                callLhs = retVar

            if name in RESOURCE_TRACKER_ENTRIES:
                if declareResources:
                    cgen.stmt("auto resources = gfxstream::vk::ResourceTracker::get()")
                cgen.funcCall(
                    callLhs, "resources->" + "on_" + api.name,
                    ["vkEnc"] + SUCCESS_VAL.get(retTypeName, []) + \
                    [p.paramName for p in api.parameters])
            else:
                cgen.funcCall(
                    callLhs, "vkEnc->" + api.name, [p.paramName for p in api.parameters] + ["true /* do lock */"])

            if name in POSTPROCESSES:
                cgen.line(POSTPROCESSES[name])

            if retTypeName != "void":
                cgen.stmt("return %s" % retVar)


        api_entry = api.withModifiedName("gfxstream_vk_" + api.name[2:])

        if api.name not in SPECIAL_ENTRIES:
            cgen.line(self.cgen.makeFuncProto(api_entry))
            cgen.beginBlock()
            genEncoderOrResourceTrackerCall(cgen, api)
            cgen.endBlock()
            self.module.appendImpl(cgen.swapCode())

    def onEnd(self,):
        pass

    def isDeviceDispatch(self, api):
        # TODO(230793667): improve the heuristic and just use "cmdToFeatureType"
        return (len(api.parameters) > 0 and
            "VkDevice" == api.parameters[0].typeName) or (
            "VkCommandBuffer" == api.parameters[0].typeName and
            self.cmdToFeatureType.get(api.name, "") == "device")
