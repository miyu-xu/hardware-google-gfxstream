from .common.codegen import CodeGen, VulkanWrapperGenerator
from .common.vulkantypes import \
        VulkanAPI, makeVulkanTypeSimple, iterateVulkanType
from .common.vulkantypes import EXCLUDED_APIS
from .common.vulkantypes import HANDLE_TYPES

import copy
import re

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

HANDWRITTEN_ENTRY_POINTS = [
    # Instance-level special-handling
    "vkCreateInstance",
    "vkDestroyInstance",
    "vkGetInstanceProcAddr",
    "vkEnumerateInstanceExtensionProperties",
    # Device-level special handling
    "vkGetDeviceProcAddr",
    "vkEnumeratePhysicalDevices",
    # Need manual object alloc+init (i.e. vk_zalloc() + vk_device_init())
    "vkCreateDevice",
    "vkDestroyDevice",
    "vkGetDeviceQueue",
    "vkGetDeviceQueue2",
    # Special cases to handle array create/destroy
    "vkAllocateCommandBuffers",
    "vkFreeCommandBuffers",
    "vkAllocateDescriptorSets",
    "vkFreeDescriptorSets",
    # Mesa objects have a create (i.e. vk_buffer_create)
    # but params dont't line up in create() call
    "vkCreateImageView",
    "vkCreateImageWithRequirementsGOOGLE",
    "vkCreateBufferWithRequirementsGOOGLE",

    # TODO: Arrays in the compoundType ...
    "vkQueueSubmit",
    "vkQueueBindSparse",
    "vkCreatePipelineLayout",
    "vkUpdateDescriptorSets",
    "vkCreateFramebuffer",
    "vkWaitSemaphores",
    "vkQueueSubmitAsyncGOOGLE",
    "vkQueueBindSparseAsyncGOOGLE",
    "vkQueueCommitDescriptorSetUpdatesGOOGLE",
    "vkCreateGraphicsPipelines",
    "vkCreateComputePipelines",
    # Compound type output
    "vkEnumeratePhysicalDeviceGroups",
]

# TODO: handles with no equivalent gfxstream objects (yet).
#  Might need some special handling.
HANDLES_DONT_TRANSLATE = {
    "VkSurfaceKHR",
}

# Handles that have an equivalent mesa create call (i.e. vk_image_create())
HANDLES_MESA_CREATE = {
    "VkDeviceMemory",
    "VkQueryPool",
    "VkBuffer",
    "VkBufferView",
    "VkImage",
    "VkImageView",
    "VkSampler",
}

def is_cmdbuf_dispatch(api):
    return "VkCommandBuffer" == api.parameters[0].typeName

def is_queue_dispatch(api):
    return "VkQueue" == api.parameters[0].typeName

def getCreateParam(api):
    for param in api.parameters:
        if param.isCreatedBy(api):
            return param
    return None

def getDestroyParam(api):
    for param in api.parameters:
        if param.isDestroyedBy(api):
            return param
    return None

# i.e. VkQueryPool --> vk_query_pool
def typeNameToMesaType(typeName):
    vkTypeNameRegex = "(?<=[a-z])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])"
    words = re.split(vkTypeNameRegex, typeName)
    outputType = "vk"
    for word in words[1:]:
        outputType += "_"
        outputType += word.lower()
    return outputType

def typeNameToBaseName(typeName):
    return typeNameToMesaType(typeName)[len("vk_"):]

def paramNameToObjectName(paramName):
    return "gfxstream_%s" % paramName

def typeNameToObjectType(typeName):
    return "gfxstream_vk_%s" % typeNameToBaseName(typeName)

def mesaObjectHasCreate(typeName):
    return typeName in HANDLES_MESA_CREATE

ALLOCATOR_TYPE_NAME = "VkAllocationCallbacks"
MESA_ALLOCATOR_PARAM_NAME = "pMesaAllocator"
def isAllocatorParam(param):
    return (param.pointerIndirectionLevels == 1
            and param.isConst
            and param.typeName == ALLOCATOR_TYPE_NAME)

def isArrayParam(param):
    return (1 == param.pointerIndirectionLevels
            and param.isConst
            and "len" in param.attribs)

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

        def isCompoundType(typeName):
            return typeInfo.isCompoundType(typeName)

        def handleTranslationRequired(typeName):
            return typeName in HANDLE_TYPES and typeName not in HANDLES_DONT_TRANSLATE

        def translationRequired(typeName):
            if isCompoundType(typeName):
                struct = typeInfo.structs[typeName]
                for member in struct.members:
                    if handleTranslationRequired(member.typeName):
                        return True
                return False
            else:
                return handleTranslationRequired(typeName)

        def genMesaAllocatorParam(cgen, api):
            primaryObjectName = paramNameToObjectName(api.parameters[0].paramName)
            defaultAllocator = "&%s->vk.alloc" % primaryObjectName
            inAllocator = None
            for param in api.parameters:
                if isAllocatorParam(param):
                    inAllocator = param.paramName
                    break
            if inAllocator:
                cgen.stmt("const %s *%s = %s ? %s : %s" %
                          (ALLOCATOR_TYPE_NAME, MESA_ALLOCATOR_PARAM_NAME, inAllocator, inAllocator, defaultAllocator))
                outAllocator = MESA_ALLOCATOR_PARAM_NAME
            else:
                outAllocator = defaultAllocator
            return outAllocator

        def genDestroyGfxstreamObjects(cgen, api):
            destroyParam = getDestroyParam(api)
            if not destroyParam:
                return
            objectName = paramNameToObjectName(destroyParam.paramName)
            mesaAllocator = genMesaAllocatorParam(cgen, api)
            if not mesaObjectHasCreate(destroyParam.typeName):
                # call vk_free() directly
                mesaObjectDestroy = "(void *)%s" % objectName
                cgen.funcCall(
                    None,
                    "vk_free",
                    [mesaAllocator, mesaObjectDestroy]
                )
            else:
                baseName = typeNameToBaseName(destroyParam.typeName)
                # TODO: hasFinish?
                # finishCallParam = ("&%s" % typeNameToObjectName(typeName))
                # cgen.funcCall(
                #     None,
                #     "vk_%s_finish" % (baseName),
                #     [finishCallParam]
                # )
                # objectName for destroy always at the back
                mesaObjectPrimary = "&%s->vk" % paramNameToObjectName(api.parameters[0].paramName)
                mesaObjectDestroy = "&%s->vk" % objectName
                cgen.funcCall(
                    None,
                    "vk_%s_destroy" % (baseName),
                    [mesaObjectPrimary, mesaAllocator, mesaObjectDestroy]
                )

        # Alloc/create gfxstream_vk_* object
        def genCreateGfxstreamObjects(cgen, api):
            createParam = getCreateParam(api)
            if not createParam:
                return False
            objectType = "struct %s" % typeNameToObjectType(createParam.typeName)
            mesaAllocator = genMesaAllocatorParam(cgen, api)
            callLhs = "%s *%s" % (objectType, paramNameToObjectName(createParam.paramName))
            if not mesaObjectHasCreate(createParam.typeName):
                # Call vk_zalloc directly
                cgen.funcCall(
                    callLhs,
                    "(%s *)vk_zalloc" % objectType,
                    # TODO: Always 8-byte align and SCOPE_OBJECT ?
                    [mesaAllocator, ("sizeof(%s)" % objectType), "8", "VK_SYSTEM_ALLOCATION_SCOPE_OBJECT"]
                )
            else:
                def getMesaCreateParams(api):
                    createParam = getCreateParam(api)
                    outParams = copy.deepcopy(api.parameters)
                    for p in outParams:
                        if isAllocatorParam(p):
                            p.paramName = MESA_ALLOCATOR_PARAM_NAME
                        elif p.paramName == createParam.paramName:
                            outParams.remove(p)
                        elif p.typeName in HANDLE_TYPES:
                            # Cast handle to the mesa type
                            p.paramName = ("(%s*)%s" % (typeNameToMesaType(p.typeName), paramNameToObjectName(p.paramName)))
                    return outParams
                # Mod params for the vk_%s_create() call i..e vk_buffer_create()
                modParams = getMesaCreateParams(api)
                cgen.funcCall(
                    callLhs,
                    "(%s *)vk_%s_create" % (objectType, typeNameToBaseName(createParam.typeName)),
                    [p.paramName for p in modParams] + ["sizeof(%s)" % objectType]
                )
            retVar = api.getRetVarExpr()
            if retVar:
                retTypeName = api.getRetTypeExpr()
                # ex: vkCreateBuffer_VkResult_return = gfxstream_buffer ? VK_SUCCESS : VK_ERROR_OUT_OF_HOST_MEMORY;
                cgen.stmt("%s = %s ? %s : %s" % 
                          (retVar, paramNameToObjectName(createParam.paramName), SUCCESS_VAL[retTypeName][0], "VK_ERROR_OUT_OF_HOST_MEMORY"))
            # An object was created
            return True

        def genGetGfxstreamHandles(cgen, api):
            createParam = getCreateParam(api)
            for param in api.parameters:
                if not handleTranslationRequired(param.typeName):
                    continue
                elif isArrayParam(param):
                    continue
                elif param != createParam:
                    if param.pointerIndirectionLevels > 0:
                        print("Error: don't know how to handle pointerIndirectionLevels > 1 for API %s (param %s)" % (api.name, param.paramName))
                        raise
                    cgen.stmt("VK_FROM_HANDLE(%s, %s, %s)" % (typeNameToObjectType(param.typeName), paramNameToObjectName(param.paramName), param.paramName))

        def genInternalObjectArray(cgen, param, countParamName):
            internalArray = "internal_%s" % param.paramName
            cgen.stmt("std::vector<%s> %s(%s)" % (param.typeName, internalArray, countParamName))
            cgen.beginFor("uint32_t i = 0", "i < %s" % countParamName, "++i")
            if isCompoundType(param.typeName):
                cgen.stmt("%s[i] = %s[i]" % (internalArray, param.paramName))
                struct = typeInfo.structs[param.typeName]
                for member in struct.members:
                    if translationRequired(member.typeName):
                        if isArrayParam(member):
                            print("ERROR: Array of handles in compoundType: %s, API: %s" % (member.typeName, api.name))
                            raise
                        elif isCompoundType(member.typeName):
                            print("ERROR: Nested compoundType: %s, in compoundType: %s (for API: %s)" % (member.typeName, param.typeName, api.name))
                            raise
                        cgen.stmt("VK_FROM_HANDLE(%s, %s, %s[i].%s)" % (typeNameToObjectType(member.typeName), paramNameToObjectName(member.paramName), internalArray, member.paramName))
                        cgen.stmt("%s[i].%s = %s->internal_object" % (internalArray, member.paramName, paramNameToObjectName(member.paramName)))
            else:
                cgen.stmt("VK_FROM_HANDLE(%s, %s, %s[i])" % (typeNameToObjectType(param.typeName), paramNameToObjectName(param.paramName), param.paramName))
                cgen.stmt("%s[i] = %s->internal_object" % (internalArray, paramNameToObjectName(param.paramName)))
            cgen.endFor()
            return "%s.data()" % internalArray

        def genInternalCompoundType(cgen, param):
            if 1 != param.pointerIndirectionLevels or not param.isConst:
                print("Error: Compound types expected to be const pointers (API: %s, paramName: %s)" % (api.name, param.paramName))
                raise
            countParamName = "1"
            if "len" in param.attribs:
                countParamName = param.attribs["len"]
            return genInternalObjectArray(cgen, param, countParamName)

        # Translate params into params needed for gfxstream-internal
        #  encoder/resource-tracker calls
        def getEncoderOrResourceTrackerParams(api):
            createParam = getCreateParam(api)
            outParams = copy.deepcopy(api.parameters)
            for param in outParams:
                if not translationRequired(param.typeName):
                    continue
                elif isCompoundType(param.typeName):
                    if not param.possiblyOutput():
                        param.paramName = genInternalCompoundType(cgen, param)
                    else:
                        print("ERROR: CompoundType output for API %s (param %s)" % (api.name, param.paramName))
                        raise
                elif isArrayParam(param):
                    countParamName = param.attribs["len"]
                    param.paramName = genInternalObjectArray(cgen, param, countParamName)
                elif 0 == param.pointerIndirectionLevels:
                    param.paramName = ("%s" % paramNameToObjectName(param.paramName)) + "->internal_object"
                elif createParam and param.paramName == createParam.paramName:
                    param.paramName = ("&%s" % paramNameToObjectName(param.paramName)) + "->internal_object"
                else:
                    print("Error: don't know how to handle API: %s, param: %s" % (api.name, param))
                    raise
            return outParams

        def genEncoderOrResourceTrackerCall(cgen, api, declareResources=True):
            if is_cmdbuf_dispatch(api):
                cgen.stmt("auto vkEnc = gfxstream::vk::ResourceTracker::getCommandBufferEncoder(commandBuffer)")
            elif is_queue_dispatch(api):
                cgen.stmt("auto vkEnc = gfxstream::vk::ResourceTracker::getQueueEncoder(queue)")
            else:
                cgen.stmt("auto vkEnc = gfxstream::vk::ResourceTracker::getThreadLocalEncoder()")
            callLhs = None
            retTypeName = api.getRetTypeExpr()
            if retTypeName != "void":
                callLhs = api.getRetVarExpr()

            # Get parameter list modded for gfxstream-internal call
            parameters = getEncoderOrResourceTrackerParams(api)
            if name in RESOURCE_TRACKER_ENTRIES:
                if declareResources:
                    cgen.stmt("auto resources = gfxstream::vk::ResourceTracker::get()")
                cgen.funcCall(
                    callLhs, "resources->" + "on_" + api.name,
                    ["vkEnc"] + SUCCESS_VAL.get(retTypeName, []) + \
                    [p.paramName for p in parameters])
            else:
                cgen.funcCall(
                    callLhs, "vkEnc->" + api.name, [p.paramName for p in parameters] + ["true /* do lock */"])

            if name in POSTPROCESSES:
                cgen.line(POSTPROCESSES[name])

        def genReturnExpression(cgen, api):
            retTypeName = api.getRetTypeExpr()
            # Set the createParam output, if applicable
            createParam = getCreateParam(api)
            if createParam:
                if 1 != createParam.pointerIndirectionLevels:
                    print("Error: don't know how to handle pointerIndirectionLevels != 1 in return for API %s (createParam %s)" % api.name, createParam.paramName)
                    raise
                # ex: *pBuffer = gfxstream_vk_buffer_to_handle(gfxstream_buffer)
                cgen.funcCall(
                    "*%s" % createParam.paramName,
                    "%s_to_handle" % typeNameToObjectType(createParam.typeName),
                    [paramNameToObjectName(createParam.paramName)]
                )

            if retTypeName != "void":
                cgen.stmt("return %s" % api.getRetVarExpr())

        def genGfxstreamEntry(cgen, api, declareResources=True):
            cgen.stmt("AEMU_SCOPED_TRACE(\"%s\")" % api.name)
            # Translate handles
            genGetGfxstreamHandles(cgen, api)
            # declare returnVar
            retTypeName = api.getRetTypeExpr()
            retVar = api.getRetVarExpr()
            if retVar:
                cgen.stmt("%s %s = (%s)0" % (retTypeName, retVar, retTypeName))
            # Translation/creation of objects
            createdObject = genCreateGfxstreamObjects(cgen, api)
            # Make encoder/resource-tracker call
            if retVar and createdObject:
                cgen.beginIf("%s == %s" % (SUCCESS_VAL[retTypeName][0], retVar))
            else:
                cgen.beginBlock()            
            genEncoderOrResourceTrackerCall(cgen, api)
            cgen.endBlock()
            # Destroy gfxstream objects
            genDestroyGfxstreamObjects(cgen, api)
            # Set output / return variables
            genReturnExpression(cgen, api)

        api_entry = api.withModifiedName("gfxstream_vk_" + api.name[2:])
        if api.name not in HANDWRITTEN_ENTRY_POINTS:
            cgen.line(self.cgen.makeFuncProto(api_entry))
            cgen.beginBlock()
            genGfxstreamEntry(cgen, api)
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
