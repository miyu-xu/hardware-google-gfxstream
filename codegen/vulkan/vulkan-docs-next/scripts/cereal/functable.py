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
    # Instance/device/physical-device special-handling, dispatch tables, etc..
    "vkCreateInstance",
    "vkDestroyInstance",
    "vkGetInstanceProcAddr",
    "vkEnumerateInstanceExtensionProperties",
    "vkGetDeviceProcAddr",
    "vkEnumeratePhysicalDevices",
    "vkCreateDevice",
    "vkDestroyDevice",
    # Manual vk_*_init() call w/ special params
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
    # Compound type output
    "vkEnumeratePhysicalDeviceGroups",
    # Handle types in nested compoundTypes
    "vkCreateGraphicsPipelines",
    "vkCreateComputePipelines",
    "vkUpdateDescriptorSets",
    "vkQueueCommitDescriptorSetUpdatesGOOGLE",

    # TODO: Add handwritten implementations ...
    "vkQueueBindSparse",
    "vkQueueBindSparseAsyncGOOGLE",
]

# TODO: handles with no equivalent gfxstream objects (yet).
#  Might need some special handling.
HANDLES_DONT_TRANSLATE = {
    "VkSurfaceKHR",
}

# Handles whose gfxstream object have non-base-object vk_ structs
HANDLES_MESA_VK = {
    # Handwritten handlers (added here for completeness)
    "VkInstance",
    "VkPhysicalDevice",
    "VkDevice",
    "VkQueue",
    "VkCommandBuffer",
    # Auto-generated creation/destroy
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

def typeNameToVkObjectType(typeName):
    return "VK_OBJECT_TYPE_%s" % typeNameToBaseName(typeName).upper()

def typeNameToObjectType(typeName):
    return "gfxstream_vk_%s" % typeNameToBaseName(typeName)

def hasMesaVkObject(typeName):
    return typeName in HANDLES_MESA_VK

def isAllocatorParam(param):
    ALLOCATOR_TYPE_NAME = "VkAllocationCallbacks"
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

        def genDestroyGfxstreamObjects(cgen, api):
            destroyParam = getDestroyParam(api)
            if not destroyParam:
                return
            objectName = paramNameToObjectName(destroyParam.paramName)
            allocatorParam = "NULL"
            for p in api.parameters:
                if isAllocatorParam(p):
                    allocatorParam = p.paramName
            if not hasMesaVkObject(destroyParam.typeName):
                deviceParam = api.parameters[0]
                if "VkDevice" != deviceParam.typeName:
                    print("ERROR: Unhandled non-VkDevice parameters[0]: %s (for API: %s)" %(deviceParam.typeName, api.name))
                    raise
                # call vk_object_free() directly
                mesaObjectDestroy = "(void *)%s" % objectName
                cgen.funcCall(
                    None,
                    "vk_object_free",
                    ["&%s->vk" % paramNameToObjectName(deviceParam.paramName), allocatorParam, mesaObjectDestroy]
                )
            else:
                baseName = typeNameToBaseName(destroyParam.typeName)
                # objectName for destroy always at the back
                mesaObjectPrimary = "&%s->vk" % paramNameToObjectName(api.parameters[0].paramName)
                mesaObjectDestroy = "&%s->vk" % objectName
                cgen.funcCall(
                    None,
                    "vk_%s_destroy" % (baseName),
                    [mesaObjectPrimary, allocatorParam, mesaObjectDestroy]
                )

        def genMesaObjectAlloc(cgen, api, allocCallLhs):
            deviceParam = api.parameters[0]
            if "VkDevice" != deviceParam.typeName:
                print("ERROR: Unhandled non-VkDevice parameters[0]: %s (for API: %s)" %(deviceParam.typeName, api.name))
                raise
            allocatorParam = "NULL"
            for p in api.parameters:
                if isAllocatorParam(p):
                    allocatorParam = p.paramName
            createParam = getCreateParam(api)
            objectType = typeNameToObjectType(createParam.typeName)
            # Call vk_object_zalloc directly
            cgen.funcCall(
                allocCallLhs,
                "(%s *)vk_object_zalloc" % objectType,
                ["&%s->vk" % paramNameToObjectName(deviceParam.paramName), allocatorParam, ("sizeof(%s)" % objectType), typeNameToVkObjectType(createParam.typeName)]
            )

        def genMesaObjectCreate(cgen, api, createCallLhs):
            createParam = getCreateParam(api)
            objectType = "struct %s" % typeNameToObjectType(createParam.typeName)
            modParams = copy.deepcopy(api.parameters)
            # Mod params for the vk_%s_create() call i..e vk_buffer_create()
            for p in modParams:
                if p.paramName == createParam.paramName:
                    modParams.remove(p)
                elif p.typeName in HANDLE_TYPES:
                    # Cast handle to the mesa type
                    p.paramName = ("(%s*)%s" % (typeNameToMesaType(p.typeName), paramNameToObjectName(p.paramName)))
            cgen.funcCall(
                createCallLhs,
                "(%s *)vk_%s_create" % (objectType, typeNameToBaseName(createParam.typeName)),
                [p.paramName for p in modParams] + ["sizeof(%s)" % objectType]
            )

        # Alloc/create gfxstream_vk_* object
        def genCreateGfxstreamObjects(cgen, api):
            createParam = getCreateParam(api)
            if not createParam:
                return False
            objectType = "struct %s" % typeNameToObjectType(createParam.typeName)
            callLhs = "%s *%s" % (objectType, paramNameToObjectName(createParam.paramName))
            if hasMesaVkObject(createParam.typeName):
                genMesaObjectCreate(cgen, api, callLhs)
            else:
                genMesaObjectAlloc(cgen, api, callLhs)

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
                        print("ERROR: Unhandled pointerIndirectionLevels > 1 for API %s (param %s)" % (api.name, param.paramName))
                        raise
                    cgen.stmt("VK_FROM_HANDLE(%s, %s, %s)" % (typeNameToObjectType(param.typeName), paramNameToObjectName(param.paramName), param.paramName))

        def internalNestedArrayName(paramName):
            return "internal_nested_%s" % paramName

        def genInternalNestedArray(cgen, param, internalArrayName, nestedParam):
            cgen.line("/* %s::%s */" % (param.typeName, nestedParam.paramName))
            countParam = "%s[i].%s" % (internalArrayName, nestedParam.attribs["len"])
            cgen.stmt("%s[i].reserve(%s)" % (internalNestedArrayName(nestedParam.paramName), countParam))
            cgen.beginFor("uint32_t j = 0", "j < %s" % countParam, "++j")
            gfxstreamObjectName = paramNameToObjectName(nestedParam.paramName)
            cgen.stmt("VK_FROM_HANDLE(%s, %s, %s[i].%s[j])" % (typeNameToObjectType(nestedParam.typeName), gfxstreamObjectName, internalArrayName, nestedParam.paramName))
            cgen.stmt("%s[i][j] = %s->internal_object" % (internalNestedArrayName(nestedParam.paramName), gfxstreamObjectName))
            cgen.endFor()
            cgen.stmt("%s[i].%s = %s[i].data()" % (internalArrayName, nestedParam.paramName, internalNestedArrayName(nestedParam.paramName)))

        def genInternalArrayDeclarations(cgen, param, countParamName):
            internalArray = "internal_%s" % param.paramName
            cgen.stmt("std::vector<%s> %s(%s)" % (param.typeName, internalArray, countParamName))
            if isCompoundType(param.typeName):
                for member in typeInfo.structs[param.typeName].members:
                    if translationRequired(member.typeName):
                        if isCompoundType(member.typeName):
                            print("ERROR: Unhandled handleType in nested compoundType: %s, in compoundType: %s (for API: %s)" % (member.typeName, param.typeName, api.name))
                            raise
                        elif isArrayParam(member):
                            cgen.stmt("std::vector<std::vector<%s>> %s(%s)" % (member.typeName, internalNestedArrayName(member.paramName), countParamName))
            return internalArray

        def genInternalArray(cgen, param, countParamName):
            internalArray = genInternalArrayDeclarations(cgen, param, countParamName)
            # Main loop to translate internal array
            cgen.beginFor("uint32_t i = 0", "i < %s" % countParamName, "++i")
            if isCompoundType(param.typeName):
                cgen.stmt("%s[i] = %s[i]" % (internalArray, param.paramName))
                for member in typeInfo.structs[param.typeName].members:
                    if translationRequired(member.typeName):
                        if isArrayParam(member):
                            genInternalNestedArray(cgen, param, internalArray, member)
                        else:
                            cgen.line("/* %s::%s */" % (param.typeName, member.paramName))
                            cgen.stmt("VK_FROM_HANDLE(%s, %s, %s[i].%s)" % (typeNameToObjectType(member.typeName), paramNameToObjectName(member.paramName), internalArray, member.paramName))
                            cgen.stmt("%s[i].%s = %s->internal_object" % (internalArray, member.paramName, paramNameToObjectName(member.paramName)))
            else:
                cgen.stmt("VK_FROM_HANDLE(%s, %s, %s[i])" % (typeNameToObjectType(param.typeName), paramNameToObjectName(param.paramName), param.paramName))
                cgen.stmt("%s[i] = %s->internal_object" % (internalArray, paramNameToObjectName(param.paramName)))
            cgen.endFor()
            return "%s.data()" % internalArray

        # Translate params into params needed for gfxstream-internal
        #  encoder/resource-tracker calls
        def getEncoderOrResourceTrackerParams(api):
            createParam = getCreateParam(api)
            outParams = copy.deepcopy(api.parameters)
            for param in outParams:
                if not translationRequired(param.typeName):
                    continue
                elif isCompoundType(param.typeName):
                    if param.possiblyOutput():
                        print("ERROR: Unhandled CompoundType output for API %s (param %s)" % (api.name, param.paramName))
                        raise
                    if 1 != param.pointerIndirectionLevels or not param.isConst:
                        print("ERROR: Compound type input is not 'const <type>*' (API: %s, paramName: %s)" % (api.name, param.paramName))
                        raise
                    countParamName = "1"
                    if "len" in param.attribs:
                        countParamName = param.attribs["len"]
                    param.paramName = genInternalArray(cgen, param, countParamName)
                elif isArrayParam(param):
                    countParamName = param.attribs["len"]
                    param.paramName = genInternalArray(cgen, param, countParamName)
                elif 0 == param.pointerIndirectionLevels:
                    param.paramName = ("%s" % paramNameToObjectName(param.paramName)) + "->internal_object"
                elif createParam and param.paramName == createParam.paramName:
                    param.paramName = ("&%s" % paramNameToObjectName(param.paramName)) + "->internal_object"
                else:
                    print("ERROR: Unknown handling for param: %s (API: %s)" % (param, api.name))
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
                    print("ERROR: Unhandled pointerIndirectionLevels != 1 in return for API %s (createParam %s)" % api.name, createParam.paramName)
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
