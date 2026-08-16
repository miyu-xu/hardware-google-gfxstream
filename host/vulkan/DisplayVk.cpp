#include "DisplayVk.h"

#if defined(_WIN32)
#include <windows.h>
#endif

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>

#include "host-common/GfxstreamFatalError.h"
#include "host-common/logging.h"
#include "vulkan/HdDisplayTelemetry.h"
#include "vulkan/VkFormatUtils.h"
#include "vulkan/vk_enum_string_helper.h"

namespace gfxstream {
namespace vk {

namespace {

void publishHdAppliedSwapchainViewport(const DisplaySurfaceVk* surface, uint32_t width,
                                       uint32_t height) {
#if defined(_WIN32)
    if (surface == nullptr) return;
    const auto renderWindow = reinterpret_cast<HWND>(surface->getNativeWindow());
    if (renderWindow == nullptr) return;
    constexpr wchar_t kProperty[] = L"HD_GFXSTREAM_APPLIED_VIEWPORT_V1";
    if (width == 0 || height == 0 || width > UINT16_MAX || height > UINT16_MAX) {
        RemovePropW(renderWindow, kProperty);
        const HWND parent = GetParent(renderWindow);
        if (parent != nullptr) RemovePropW(parent, kProperty);
        return;
    }
    const uintptr_t packed = (static_cast<uintptr_t>(height) << 16) | width;
    SetPropW(renderWindow, kProperty, reinterpret_cast<HANDLE>(packed));
    const HWND parent = GetParent(renderWindow);
    if (parent != nullptr) SetPropW(parent, kProperty, reinterpret_cast<HANDLE>(packed));
#else
    (void)surface;
    (void)width;
    (void)height;
#endif
}

uint64_t hdCadenceProbeEpoch(const DisplaySurfaceVk* surface) {
#if defined(_WIN32)
    if (surface == nullptr) return 0;
    const auto renderWindow = reinterpret_cast<HWND>(surface->getNativeWindow());
    if (renderWindow == nullptr) return 0;
    constexpr wchar_t kProperty[] = L"HD_GFXSTREAM_CADENCE_PROBE_V1";
    HANDLE value = GetPropW(renderWindow, kProperty);
    if (value == nullptr) {
        const HWND parent = GetParent(renderWindow);
        if (parent != nullptr) value = GetPropW(parent, kProperty);
    }
    return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(value));
#else
    (void)surface;
    return 0;
#endif
}

}  // namespace

using emugl::ABORT_REASON_OTHER;
using emugl::FatalError;
using gfxstream::vk::formatIsDepthOrStencil;
using gfxstream::vk::formatIsSInt;
using gfxstream::vk::formatIsUInt;
using gfxstream::vk::formatRequiresSamplerYcbcrConversion;

#define DISPLAY_VK_ERROR(fmt, ...)                                                            \
    do {                                                                                      \
        fprintf(stderr, "%s(%s:%d): " fmt "\n", __func__, __FILE__, __LINE__, ##__VA_ARGS__); \
        fflush(stderr);                                                                       \
    } while (0)

#define DISPLAY_VK_ERROR_ONCE(fmt, ...)              \
    do {                                             \
        static bool displayVkInternalLogged = false; \
        if (!displayVkInternalLogged) {              \
            DISPLAY_VK_ERROR(fmt, ##__VA_ARGS__);    \
            displayVkInternalLogged = true;          \
        }                                            \
    } while (0)

namespace {

bool shouldRecreateSwapchain(VkResult result) {
    switch (result) {
        case VK_SUBOPTIMAL_KHR:
        case VK_ERROR_OUT_OF_DATE_KHR:
        // b/217229121: drivers may return VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT in
        // vkQueuePresentKHR even if VK_EXT_full_screen_exclusive is not enabled.
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            return true;

        default:
            return false;
    }
}

const char* presentModeName(VkPresentModeKHR presentMode) {
    switch (presentMode) {
        case VK_PRESENT_MODE_IMMEDIATE_KHR:
            return "immediate";
        case VK_PRESENT_MODE_MAILBOX_KHR:
            return "mailbox";
        case VK_PRESENT_MODE_FIFO_KHR:
            return "fifo";
        default:
            return "other";
    }
}

}  // namespace

DisplayVk::DisplayVk(const VulkanDispatch& vk, VkPhysicalDevice vkPhysicalDevice,
                     uint32_t swapChainQueueFamilyIndex, uint32_t compositorQueueFamilyIndex,
                     VkDevice vkDevice, VkQueue compositorVkQueue,
                     std::shared_ptr<android::base::Lock> compositorVkQueueLock,
                     VkQueue swapChainVkqueue,
                     std::shared_ptr<android::base::Lock> swapChainVkQueueLock)
    : m_vk(vk),
      m_vkPhysicalDevice(vkPhysicalDevice),
      m_swapChainQueueFamilyIndex(swapChainQueueFamilyIndex),
      m_compositorQueueFamilyIndex(compositorQueueFamilyIndex),
      m_vkDevice(vkDevice),
      m_compositorVkQueue(compositorVkQueue),
      m_compositorVkQueueLock(compositorVkQueueLock),
      m_swapChainVkQueue(swapChainVkqueue),
      m_swapChainVkQueueLock(swapChainVkQueueLock),
      m_vkCommandPool(VK_NULL_HANDLE),
      m_swapChainStateVk(nullptr) {
    // TODO(kaiyili): validate the capabilites of the passed in Vulkan
    // components.
    VkCommandPoolCreateInfo commandPoolCi = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = m_compositorQueueFamilyIndex,
    };
    VK_CHECK(m_vk.vkCreateCommandPool(m_vkDevice, &commandPoolCi, nullptr, &m_vkCommandPool));
    constexpr size_t imageBorrowResourcePoolSize = 10;
    for (size_t i = 0; i < imageBorrowResourcePoolSize; i++) {
        m_imageBorrowResources.emplace_back(
            ImageBorrowResource::create(m_vk, m_vkDevice, m_vkCommandPool));
    }
}

DisplayVk::~DisplayVk() {
    destroySwapchain();
    m_imageBorrowResources.clear();
    m_vk.vkDestroyCommandPool(m_vkDevice, m_vkCommandPool, nullptr);
}

void DisplayVk::drainQueues() {
    {
        android::base::AutoLock lock(*m_swapChainVkQueueLock);
        VK_CHECK(vk_util::waitForVkQueueIdleWithRetry(m_vk, m_swapChainVkQueue));
    }
    // We don't assume all VkCommandBuffer submitted to m_compositorVkQueueLock is always followed
    // by another operation on the m_swapChainVkQueue. Therefore, only waiting for the
    // m_swapChainVkQueue is not enough to guarantee all resources used are free to be destroyed.
    {
        android::base::AutoLock lock(*m_compositorVkQueueLock);
        VK_CHECK(vk_util::waitForVkQueueIdleWithRetry(m_vk, m_compositorVkQueue));
    }
}

void DisplayVk::bindToSurfaceImpl(gfxstream::DisplaySurface* surface) {
    m_surfaceGeneration.fetch_add(1, std::memory_order_release);
    m_needToRecreateSwapChain = true;
}

void DisplayVk::surfaceUpdated(gfxstream::DisplaySurface* surface) {
    m_surfaceGeneration.fetch_add(1, std::memory_order_release);
    m_surfaceQueuesAlreadyDrained.store(true, std::memory_order_release);
    m_needToRecreateSwapChain = true;
}

void DisplayVk::unbindFromSurfaceImpl() { destroySwapchain(); }

void DisplayVk::destroySwapchain() {
    const auto* surface = getBoundSurface();
    publishHdAppliedSwapchainViewport(
        surface ? static_cast<const DisplaySurfaceVk*>(surface->getImpl()) : nullptr, 0, 0);
    drainQueues();
    m_freePostResources.clear();
    m_postResourceFutures.clear();
    m_swapChainStateVk.reset();
    m_retiredSwapChainStateVk.reset();
    m_appliedSwapchainWidth.store(0, std::memory_order_release);
    m_appliedSwapchainHeight.store(0, std::memory_order_release);
    m_needToRecreateSwapChain = true;
}

bool DisplayVk::recreateSwapchain(uint64_t surfaceGeneration) {
    // Do not destroy the currently presented Win32 swapchain before its replacement exists.
    // Destroy-first leaves the HWND with no retained image for one or two DWM compositions,
    // which is visible as a black flash on maximize, restore and live resize.
    // FrameBuffer drains both queues before publishing DisplaySurface::updateSize(). Draining a
    // second time from the PostWorker can wait on the very presentation that is performing this
    // recreation. Only self-drain for out-of-date/error retries that were not triggered by a
    // committed surface update.
    if (!m_surfaceQueuesAlreadyDrained.exchange(false, std::memory_order_acq_rel)) {
        drainQueues();
    }
    m_freePostResources.clear();
    m_postResourceFutures.clear();
    m_retiredSwapChainStateVk.reset();
    auto previousSwapchain = std::move(m_swapChainStateVk);

    const auto* surface = getBoundSurface();
    if (!surface) {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
            << "DisplayVk can't create VkSwapchainKHR without a VkSurfaceKHR";
    }
    const auto* surfaceVk = static_cast<const DisplaySurfaceVk*>(surface->getImpl());

    if (!SwapChainStateVk::validateQueueFamilyProperties(
            m_vk, m_vkPhysicalDevice, surfaceVk->getSurface(), m_swapChainQueueFamilyIndex)) {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
            << "DisplayVk can't create VkSwapchainKHR with given VkDevice and VkSurfaceKHR.";
    }
    const uint32_t surfaceWidth = surface->getWidth();
    const uint32_t surfaceHeight = surface->getHeight();
    INFO("Creating swapchain with size %" PRIu32 "x%" PRIu32 ".", surfaceWidth, surfaceHeight);
    auto swapChainCi = SwapChainStateVk::createSwapChainCi(
        m_vk, surfaceVk->getSurface(), m_vkPhysicalDevice, surfaceWidth, surfaceHeight,
        {m_swapChainQueueFamilyIndex, m_compositorQueueFamilyIndex});
    if (!swapChainCi) {
        recordHdSwapchainRecreation(false);
        m_swapChainStateVk = std::move(previousSwapchain);
        return false;
    }
    swapChainCi->mCreateInfo.oldSwapchain = previousSwapchain
                                                ? previousSwapchain->getSwapChain()
                                                : VK_NULL_HANDLE;
    VkFormatProperties formatProps;
    m_vk.vkGetPhysicalDeviceFormatProperties(m_vkPhysicalDevice,
                                             swapChainCi->mCreateInfo.imageFormat, &formatProps);
    if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)) {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
            << "DisplayVk: The image format chosen for present VkImage can't be used as the color "
               "attachment, and therefore can't be used as the render target of CompositorVk.";
    }
    auto replacementSwapchain =
        SwapChainStateVk::createSwapChainVk(m_vk, m_vkDevice, swapChainCi->mCreateInfo);
    if (replacementSwapchain == nullptr) {
        recordHdSwapchainRecreation(false);
        m_swapChainStateVk = std::move(previousSwapchain);
        return false;
    }
    m_swapChainStateVk = std::move(replacementSwapchain);
    // A successful vkCreateSwapchainKHR retires the old chain but does not require its immediate
    // destruction. Retaining it until the next recreation/unbind keeps DWM's last complete image
    // available while postImpl submits the first frame to the replacement.
    m_retiredSwapChainStateVk = std::move(previousSwapchain);
    int numSwapChainImages = m_swapChainStateVk->getVkImages().size();

    m_postResourceFutures.resize(numSwapChainImages, std::nullopt);
    for (uint32_t i = 0; i < numSwapChainImages + 1; ++i) {
        m_freePostResources.emplace_back(PostResource::create(m_vk, m_vkDevice, m_vkCommandPool));
    }

    m_inFlightFrameIndex = 0;
    m_appliedSwapchainWidth.store(surfaceWidth, std::memory_order_relaxed);
    m_appliedSwapchainHeight.store(surfaceHeight, std::memory_order_relaxed);
    m_appliedSurfaceGeneration.store(surfaceGeneration, std::memory_order_release);
    m_needToRecreateSwapChain = false;
    publishHdAppliedSwapchainViewport(surfaceVk, surfaceWidth, surfaceHeight);
    recordHdSwapchainRecreation(true);
    return true;
}

DisplayVk::PostResult DisplayVk::post(const BorrowedImageInfo* sourceImageInfo) {
    auto completedFuture = std::async(std::launch::deferred, [] {}).share();
    completedFuture.wait();

    const auto* surface = getBoundSurface();
    if (!surface) {
        ERR("Trying to present to non-existing surface!");
        return PostResult{
            .success = true,
            .postCompletedWaitable = completedFuture,
        };
    }

    if (!recreateSwapchainIfNeeded()) {
        return PostResult{
            .success = false,
            .postCompletedWaitable = completedFuture,
        };
    }

    auto result = postImpl(sourceImageInfo);
    if (!result.success) {
        m_needToRecreateSwapChain = true;
    }
    return result;
}

bool DisplayVk::recreateSwapchainIfNeeded() {
    const uint64_t surfaceGeneration = m_surfaceGeneration.load(std::memory_order_acquire);
    if (isSwapchainCurrentForSurface()) {
        return true;
    }
    INFO("Recreating swapchain...");
    constexpr const int kMaxRecreateSwapchainRetries = 8;
    int retriesRemaining = kMaxRecreateSwapchainRetries;
    while (retriesRemaining >= 0 && !recreateSwapchain(surfaceGeneration)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        --retriesRemaining;
        INFO("Swapchain recreation failed, retrying...");
    }
    if (retriesRemaining < 0) {
        const auto* surface = getBoundSurface();
        ERR("Failed to create swapchain for transient surface size %" PRIu32 "x%" PRIu32
            "; keeping the VM alive and retrying on a later frame.",
            surface ? surface->getWidth() : 0, surface ? surface->getHeight() : 0);
        return false;
    }
    INFO("Recreating swapchain completed.");
    return true;
}

bool DisplayVk::isSwapchainCurrentForSurface() const {
    const uint64_t surfaceGeneration = m_surfaceGeneration.load(std::memory_order_acquire);
    const bool needRecreate = m_needToRecreateSwapChain.load(std::memory_order_acquire);
    const uint64_t appliedGeneration =
        m_appliedSurfaceGeneration.load(std::memory_order_acquire);
    const auto* surface = getBoundSurface();
    return surface != nullptr && !needRecreate && appliedGeneration == surfaceGeneration &&
           m_appliedSwapchainWidth.load(std::memory_order_acquire) == surface->getWidth() &&
           m_appliedSwapchainHeight.load(std::memory_order_acquire) == surface->getHeight();
}

DisplayVk::PostResult DisplayVk::postImpl(const BorrowedImageInfo* sourceImageInfo) {
    auto completedFuture = std::async(std::launch::deferred, [] {}).share();
    completedFuture.wait();

    // One for acquire, one for release.
    const ImageBorrowResource* imageBorrowResources[2] = {nullptr};
    for (size_t i = 0; i < std::size(imageBorrowResources); i++) {
        auto freeImageBorrowResource =
            std::find_if(m_imageBorrowResources.begin(), m_imageBorrowResources.end(),
                         [this](const std::unique_ptr<ImageBorrowResource>& imageBorrowResource) {
                             VkResult fenceStatus = m_vk.vkGetFenceStatus(
                                 m_vkDevice, imageBorrowResource->m_completeFence);
                             if (fenceStatus == VK_SUCCESS) { return true; }
                             if (fenceStatus == VK_NOT_READY) { return false; }
                             VK_CHECK(fenceStatus);
                             return false;
                         });
        if (freeImageBorrowResource == m_imageBorrowResources.end()) {
            freeImageBorrowResource = m_imageBorrowResources.begin();
            VK_CHECK(m_vk.vkWaitForFences(
                m_vkDevice, 1, &(*freeImageBorrowResource)->m_completeFence, VK_TRUE, UINT64_MAX));
        }
        VK_CHECK(m_vk.vkResetFences(m_vkDevice, 1, &(*freeImageBorrowResource)->m_completeFence));
        imageBorrowResources[i] = freeImageBorrowResource->get();
    }
    // We need to unconditionally acquire and release the image to satisfy the requiremment for the
    // borrowed image.
    const auto* sourceImageInfoVk = static_cast<const BorrowedImageInfoVk*>(sourceImageInfo);
    struct ImageBorrower {
        ImageBorrower(const VulkanDispatch& vk, VkQueue queue,
                      std::shared_ptr<android::base::Lock> queueLock, uint32_t usedQueueFamilyIndex,
                      const BorrowedImageInfoVk& image, const ImageBorrowResource& acquireResource,
                      const ImageBorrowResource& releaseResource)
            : m_vk(vk),
              m_vkQueue(queue),
              m_queueLock(queueLock),
              m_releaseResource(releaseResource) {
            std::vector<VkImageMemoryBarrier> acquireQueueTransferBarriers;
            std::vector<VkImageMemoryBarrier> acquireLayoutTransitionBarriers;
            std::vector<VkImageMemoryBarrier> releaseLayoutTransitionBarriers;
            std::vector<VkImageMemoryBarrier> releaseQueueTransferBarriers;
            addNeededBarriersToUseBorrowedImage(
                image, usedQueueFamilyIndex,
                /*usedInitialImageLayout=*/VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                /*usedFinalImageLayout=*/VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_ACCESS_TRANSFER_READ_BIT, &acquireQueueTransferBarriers,
                &acquireLayoutTransitionBarriers, &releaseLayoutTransitionBarriers,
                &releaseQueueTransferBarriers);

            // Record the acquire commands.
            const VkCommandBufferBeginInfo acquireBeginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            };
            VK_CHECK(
                m_vk.vkBeginCommandBuffer(acquireResource.m_vkCommandBuffer, &acquireBeginInfo));
            if (!acquireQueueTransferBarriers.empty()) {
                m_vk.vkCmdPipelineBarrier(
                    acquireResource.m_vkCommandBuffer,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0, 0, nullptr, 0, nullptr,
                    static_cast<uint32_t>(acquireQueueTransferBarriers.size()),
                    acquireQueueTransferBarriers.data());
            }
            if (!acquireLayoutTransitionBarriers.empty()) {
                m_vk.vkCmdPipelineBarrier(
                    acquireResource.m_vkCommandBuffer,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
                    static_cast<uint32_t>(acquireLayoutTransitionBarriers.size()),
                    acquireLayoutTransitionBarriers.data());
            }
            VK_CHECK(m_vk.vkEndCommandBuffer(acquireResource.m_vkCommandBuffer));

            // Record the release commands.
            const VkCommandBufferBeginInfo releaseBeginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            };
            VK_CHECK(
                m_vk.vkBeginCommandBuffer(releaseResource.m_vkCommandBuffer, &releaseBeginInfo));
            if (!releaseLayoutTransitionBarriers.empty()) {
                m_vk.vkCmdPipelineBarrier(
                    releaseResource.m_vkCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr,
                    static_cast<uint32_t>(releaseLayoutTransitionBarriers.size()),
                    releaseLayoutTransitionBarriers.data());
            }
            if (!releaseQueueTransferBarriers.empty()) {
                m_vk.vkCmdPipelineBarrier(
                    releaseResource.m_vkCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr,
                    static_cast<uint32_t>(releaseQueueTransferBarriers.size()),
                    releaseQueueTransferBarriers.data());
            }
            VK_CHECK(m_vk.vkEndCommandBuffer(releaseResource.m_vkCommandBuffer));

            VkSubmitInfo submitInfo = {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .waitSemaphoreCount = 0,
                .pWaitSemaphores = nullptr,
                .pWaitDstStageMask = nullptr,
                .commandBufferCount = 1,
                .pCommandBuffers = &acquireResource.m_vkCommandBuffer,
                .signalSemaphoreCount = 0,
                .pSignalSemaphores = nullptr,
            };
            // Submit the acquire commands.
            {
                android::base::AutoLock lock(*m_queueLock);
                VK_CHECK(
                    m_vk.vkQueueSubmit(m_vkQueue, 1, &submitInfo, acquireResource.m_completeFence));
            }
        }

        const VulkanDispatch& m_vk;
        const VkQueue m_vkQueue;
        std::shared_ptr<android::base::Lock> m_queueLock;
        const ImageBorrowResource& m_releaseResource;
        ~ImageBorrower() {
            VkSubmitInfo submitInfo = {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .waitSemaphoreCount = 0,
                .pWaitSemaphores = nullptr,
                .pWaitDstStageMask = nullptr,
                .commandBufferCount = 1,
                .pCommandBuffers = &m_releaseResource.m_vkCommandBuffer,
                .signalSemaphoreCount = 0,
                .pSignalSemaphores = nullptr,
            };
            // Submit the release commands.
            {
                android::base::AutoLock lock(*m_queueLock);
                VK_CHECK(m_vk.vkQueueSubmit(m_vkQueue, 1, &submitInfo,
                                            m_releaseResource.m_completeFence));
            }
        }
    } imageBorrower(m_vk, m_compositorVkQueue, m_compositorVkQueueLock,
                    m_compositorQueueFamilyIndex, *sourceImageInfoVk, *imageBorrowResources[0],
                    *imageBorrowResources[1]);

    const auto* surface = getBoundSurface();
    if (!m_swapChainStateVk || !surface) {
        DISPLAY_VK_ERROR("Haven't bound to a surface, can't post ColorBuffer.");
        return PostResult{true, std::move(completedFuture)};
    }

    if (!canPost(sourceImageInfoVk->imageCreateInfo)) {
        DISPLAY_VK_ERROR("Can't post ColorBuffer.");
        return PostResult{true, std::move(completedFuture)};
    }

    const auto hdPresentStarted = std::chrono::steady_clock::now();

    for (auto& postResourceFutureOpt : m_postResourceFutures) {
        if (!postResourceFutureOpt.has_value()) {
            continue;
        }
        auto postResourceFuture = postResourceFutureOpt.value();
        if (!postResourceFuture.valid()) {
            GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
                << "Invalid postResourceFuture in m_postResourceFutures.";
        }
        std::future_status status = postResourceFuture.wait_for(std::chrono::seconds(0));
        if (status == std::future_status::ready) {
            m_freePostResources.emplace_back(postResourceFuture.get());
            postResourceFutureOpt = std::nullopt;
        }
    }
    if (m_freePostResources.empty()) {
        for (auto& postResourceFutureOpt : m_postResourceFutures) {
            if (!postResourceFutureOpt.has_value()) {
                continue;
            }
            m_freePostResources.emplace_back(postResourceFutureOpt.value().get());
            postResourceFutureOpt = std::nullopt;
            break;
        }
    }
    std::shared_ptr<PostResource> postResource = m_freePostResources.front();
    m_freePostResources.pop_front();

    VkSemaphore imageReadySem = postResource->m_swapchainImageAcquireSemaphore;

    uint32_t imageIndex;
    VkResult acquireRes =
        m_vk.vkAcquireNextImageKHR(m_vkDevice, m_swapChainStateVk->getSwapChain(), UINT64_MAX,
                                   imageReadySem, VK_NULL_HANDLE, &imageIndex);
    if (shouldRecreateSwapchain(acquireRes)) {
        recordHdSwapchainOutOfDate();
        return PostResult{false, std::shared_future<void>()};
    }
    VK_CHECK(acquireRes);

    if (m_postResourceFutures[imageIndex].has_value()) {
        m_freePostResources.emplace_back(m_postResourceFutures[imageIndex].value().get());
        m_postResourceFutures[imageIndex] = std::nullopt;
    }

    VkCommandBuffer cmdBuff = postResource->m_vkCommandBuffer;
    VK_CHECK(m_vk.vkResetCommandBuffer(cmdBuff, 0));

    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VK_CHECK(m_vk.vkBeginCommandBuffer(cmdBuff, &beginInfo));

    // Track the swapchain image state explicitly.  In particular, access masks are VkAccessFlags,
    // not pipeline-stage bits.  Intel's Windows Vulkan driver does not reliably make the transfer
    // write visible to presentation when the old, mismatched barrier is used.
    VkImageLayout currentSwapchainLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkAccessFlags currentSwapchainAccess = 0;

    VkImageMemoryBarrier acquireSwapchainImageBarrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = currentSwapchainAccess,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = currentSwapchainLayout,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = m_swapChainStateVk->getVkImages()[imageIndex],
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };
    m_vk.vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                              &acquireSwapchainImageBarrier);
    currentSwapchainLayout = acquireSwapchainImageBarrier.newLayout;
    currentSwapchainAccess = acquireSwapchainImageBarrier.dstAccessMask;

    // Note: The extent used during swapchain creation must be used here and not the
    // current surface's extent as the swapchain may not have been updated after the
    // surface resized. The blit must not try to write outside of the extent of the
    // existing swapchain images.
    const VkExtent2D swapchainImageExtent = m_swapChainStateVk->getImageExtent();
    const VkImageBlit region = {
        .srcSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                           .mipLevel = 0,
                           .baseArrayLayer = 0,
                           .layerCount = 1},
        .srcOffsets = {{0, 0, 0},
                       {static_cast<int32_t>(sourceImageInfoVk->imageCreateInfo.extent.width),
                        static_cast<int32_t>(sourceImageInfoVk->imageCreateInfo.extent.height), 1}},
        .dstSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                           .mipLevel = 0,
                           .baseArrayLayer = 0,
                           .layerCount = 1},
        .dstOffsets = {{0, 0, 0},
                       {static_cast<int32_t>(swapchainImageExtent.width),
                        static_cast<int32_t>(swapchainImageExtent.height), 1}},
    };
    VkFormat displayBufferFormat = sourceImageInfoVk->imageCreateInfo.format;
    VkImageTiling displayBufferTiling = sourceImageInfoVk->imageCreateInfo.tiling;
    VkFilter filter = VK_FILTER_NEAREST;
    VkFormatFeatureFlags displayBufferFormatFeatures =
        getFormatFeatures(displayBufferFormat, displayBufferTiling);
    if (formatIsDepthOrStencil(displayBufferFormat)) {
        DISPLAY_VK_ERROR_ONCE(
            "The format of the display buffer, %s, is a depth/stencil format, we can only use the "
            "VK_FILTER_NEAREST filter according to VUID-vkCmdBlitImage-srcImage-00232.",
            string_VkFormat(displayBufferFormat));
        filter = VK_FILTER_NEAREST;
    } else if (!(displayBufferFormatFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        DISPLAY_VK_ERROR_ONCE(
            "The format of the display buffer, %s, with the tiling, %s, doesn't support "
            "VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT, so we can only use the "
            "VK_FILTER_NEAREST filter according VUID-vkCmdBlitImage-filter-02001. The supported "
            "features are %s.",
            string_VkFormat(displayBufferFormat), string_VkImageTiling(displayBufferTiling),
            string_VkFormatFeatureFlags(displayBufferFormatFeatures).c_str());
        filter = VK_FILTER_NEAREST;
    } else {
        filter = VK_FILTER_LINEAR;
    }
    m_vk.vkCmdBlitImage(cmdBuff, sourceImageInfoVk->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        m_swapChainStateVk->getVkImages()[imageIndex], currentSwapchainLayout, 1,
                        &region, filter);

    VkImageMemoryBarrier releaseSwapchainImageBarrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = currentSwapchainAccess,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .oldLayout = currentSwapchainLayout,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = m_swapChainStateVk->getVkImages()[imageIndex],
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };
    m_vk.vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                              &releaseSwapchainImageBarrier);

    VK_CHECK(m_vk.vkEndCommandBuffer(cmdBuff));

    VkFence postCompleteFence = postResource->m_swapchainImageReleaseFence;
    VK_CHECK(m_vk.vkResetFences(m_vkDevice, 1, &postCompleteFence));
    VkSemaphore postCompleteSemaphore = postResource->m_swapchainImageReleaseSemaphore;
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TRANSFER_BIT};
    VkSubmitInfo submitInfo = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                               .waitSemaphoreCount = 1,
                               .pWaitSemaphores = &imageReadySem,
                               .pWaitDstStageMask = waitStages,
                               .commandBufferCount = 1,
                               .pCommandBuffers = &cmdBuff,
                               .signalSemaphoreCount = 1,
                               .pSignalSemaphores = &postCompleteSemaphore};
    {
        android::base::AutoLock lock(*m_compositorVkQueueLock);
        VK_CHECK(m_vk.vkQueueSubmit(m_compositorVkQueue, 1, &submitInfo, postCompleteFence));
    }
    std::shared_future<std::shared_ptr<PostResource>> postResourceFuture =
        std::async(std::launch::deferred, [postCompleteFence, postResource, this]() mutable {
            VkResult res = m_vk.vkWaitForFences(m_vkDevice, 1, &postCompleteFence, VK_TRUE,
                                                kVkWaitForFencesTimeoutNsecs);
            if (res == VK_SUCCESS) {
                return postResource;
            }
            if (res == VK_TIMEOUT) {
                // Retry. If device lost, hopefully this returns immediately.
                res = m_vk.vkWaitForFences(m_vkDevice, 1, &postCompleteFence, VK_TRUE,
                                           kVkWaitForFencesTimeoutNsecs);
            }
            VK_CHECK(res);
            return postResource;
        }).share();
    m_postResourceFutures[imageIndex] = postResourceFuture;

    auto swapChain = m_swapChainStateVk->getSwapChain();
    VkPresentInfoKHR presentInfo = {.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                                    .waitSemaphoreCount = 1,
                                    .pWaitSemaphores = &postCompleteSemaphore,
                                    .swapchainCount = 1,
                                    .pSwapchains = &swapChain,
                                    .pImageIndices = &imageIndex};
    VkResult presentRes;
    {
        android::base::AutoLock lock(*m_swapChainVkQueueLock);
        presentRes = m_vk.vkQueuePresentKHR(m_swapChainVkQueue, &presentInfo);
    }
    if (shouldRecreateSwapchain(presentRes)) {
        recordHdSwapchainOutOfDate();
        postResourceFuture.wait();
        return PostResult{false, std::shared_future<void>()};
    }
    VK_CHECK(presentRes);
    const auto hostPresentLatencyNs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() -
                                                             hdPresentStarted)
            .count());
    const auto* displaySurface = static_cast<const DisplaySurfaceVk*>(surface->getImpl());
    recordHdSuccessfulPresent(presentModeName(m_swapChainStateVk->getPresentMode()),
                              hostPresentLatencyNs, hdCadenceProbeEpoch(displaySurface),
                              sourceImageInfoVk->imageCreateInfo.extent.width,
                              sourceImageInfoVk->imageCreateInfo.extent.height,
                              swapchainImageExtent.width, swapchainImageExtent.height);
    return PostResult{true, std::async(std::launch::deferred, [postResourceFuture] {
                                // We can't directly wait for the VkFence here, because we
                                // share the VkFences on different frames, but we don't share
                                // the future on different frames. If we directly wait for the
                                // VkFence here, we may wait for a different frame if a new
                                // frame starts to be drawn before this future is waited.
                                postResourceFuture.wait();
                            }).share()};
}

VkFormatFeatureFlags DisplayVk::getFormatFeatures(VkFormat format, VkImageTiling tiling) {
    auto i = m_vkFormatProperties.find(format);
    if (i == m_vkFormatProperties.end()) {
        VkFormatProperties formatProperties;
        m_vk.vkGetPhysicalDeviceFormatProperties(m_vkPhysicalDevice, format, &formatProperties);
        i = m_vkFormatProperties.emplace(format, formatProperties).first;
    }
    const VkFormatProperties& formatProperties = i->second;
    VkFormatFeatureFlags formatFeatures = 0;
    if (tiling == VK_IMAGE_TILING_LINEAR) {
        formatFeatures = formatProperties.linearTilingFeatures;
    } else if (tiling == VK_IMAGE_TILING_OPTIMAL) {
        formatFeatures = formatProperties.optimalTilingFeatures;
    } else {
        DISPLAY_VK_ERROR("Unknown tiling %#" PRIx64 ".", static_cast<uint64_t>(tiling));
    }
    return formatFeatures;
}

bool DisplayVk::canPost(const VkImageCreateInfo& postImageCi) {
    // According to VUID-vkCmdBlitImage-srcImage-01999, the format features of srcImage must contain
    // VK_FORMAT_FEATURE_BLIT_SRC_BIT.
    VkFormatFeatureFlags formatFeatures = getFormatFeatures(postImageCi.format, postImageCi.tiling);
    if (!(formatFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) {
        DISPLAY_VK_ERROR(
            "VK_FORMAT_FEATURE_BLIT_SRC_BLIT is not supported for VkImage with format %s, tilling "
            "%s. Supported features are %s.",
            string_VkFormat(postImageCi.format), string_VkImageTiling(postImageCi.tiling),
            string_VkFormatFeatureFlags(formatFeatures).c_str());
        return false;
    }

    // According to VUID-vkCmdBlitImage-srcImage-06421, srcImage must not use a format that requires
    // a sampler Y’CBCR conversion.
    if (formatRequiresSamplerYcbcrConversion(postImageCi.format)) {
        DISPLAY_VK_ERROR("Format %s requires a sampler Y'CbCr conversion. Can't be used to post.",
                         string_VkFormat(postImageCi.format));
        return false;
    }

    if (!(postImageCi.usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)) {
        // According to VUID-vkCmdBlitImage-srcImage-00219, srcImage must have been created with
        // VK_IMAGE_USAGE_TRANSFER_SRC_BIT usage flag.
        DISPLAY_VK_ERROR(
            "The VkImage is not created with the VK_IMAGE_USAGE_TRANSFER_SRC_BIT usage flag. The "
            "usage flags are %s.",
            string_VkImageUsageFlags(postImageCi.usage).c_str());
        return false;
    }

    VkFormat swapChainFormat = m_swapChainStateVk->getFormat();
    if (formatIsSInt(postImageCi.format) || formatIsSInt(swapChainFormat)) {
        // According to VUID-vkCmdBlitImage-srcImage-00229, if either of srcImage or dstImage was
        // created with a signed integer VkFormat, the other must also have been created with a
        // signed integer VkFormat.
        if (!(formatIsSInt(postImageCi.format) && formatIsSInt(m_swapChainStateVk->getFormat()))) {
            DISPLAY_VK_ERROR(
                "The format(%s) doesn't match with the format of the presentable image(%s): either "
                "of the formats is a signed integer VkFormat, but the other is not.",
                string_VkFormat(postImageCi.format), string_VkFormat(swapChainFormat));
            return false;
        }
    }

    if (formatIsUInt(postImageCi.format) || formatIsUInt(swapChainFormat)) {
        // According to VUID-vkCmdBlitImage-srcImage-00230, if either of srcImage or dstImage was
        // created with an unsigned integer VkFormat, the other must also have been created with an
        // unsigned integer VkFormat.
        if (!(formatIsUInt(postImageCi.format) && formatIsUInt(swapChainFormat))) {
            DISPLAY_VK_ERROR(
                "The format(%s) doesn't match with the format of the presentable image(%s): either "
                "of the formats is an unsigned integer VkFormat, but the other is not.",
                string_VkFormat(postImageCi.format), string_VkFormat(swapChainFormat));
            return false;
        }
    }

    if (formatIsDepthOrStencil(postImageCi.format) || formatIsDepthOrStencil(swapChainFormat)) {
        // According to VUID-vkCmdBlitImage-srcImage-00231, if either of srcImage or dstImage was
        // created with a depth/stencil format, the other must have exactly the same format.
        if (postImageCi.format != swapChainFormat) {
            DISPLAY_VK_ERROR(
                "The format(%s) doesn't match with the format of the presentable image(%s): either "
                "of the formats is a depth/stencil VkFormat, but the other is not the same format.",
                string_VkFormat(postImageCi.format), string_VkFormat(swapChainFormat));
            return false;
        }
    }

    if (postImageCi.samples != VK_SAMPLE_COUNT_1_BIT) {
        // According to VUID-vkCmdBlitImage-srcImage-00233, srcImage must have been created with a
        // samples value of VK_SAMPLE_COUNT_1_BIT.
        DISPLAY_VK_ERROR(
            "The VkImage is not created with the VK_SAMPLE_COUNT_1_BIT samples value. The samples "
            "value is %s.",
            string_VkSampleCountFlagBits(postImageCi.samples));
        return false;
    }
    if (postImageCi.flags & VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT) {
        // According to VUID-vkCmdBlitImage-dstImage-02545, dstImage and srcImage must not have been
        // created with flags containing VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT.
        DISPLAY_VK_ERROR(
            "The VkImage can't be created with flags containing "
            "VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT. The flags are %s.",
            string_VkImageCreateFlags(postImageCi.flags).c_str());
        return false;
    }
    return true;
}

std::shared_ptr<DisplayVk::PostResource> DisplayVk::PostResource::create(
    const VulkanDispatch& vk, VkDevice vkDevice, VkCommandPool vkCommandPool) {
    VkFenceCreateInfo fenceCi = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };
    VkFence fence;
    VK_CHECK(vk.vkCreateFence(vkDevice, &fenceCi, nullptr, &fence));
    VkSemaphore semaphores[2];
    for (uint32_t i = 0; i < std::size(semaphores); i++) {
        VkSemaphoreCreateInfo semaphoreCi = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        VK_CHECK(vk.vkCreateSemaphore(vkDevice, &semaphoreCi, nullptr, &semaphores[i]));
    }
    VkCommandBuffer commandBuffer;
    VkCommandBufferAllocateInfo commandBufferAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vkCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VK_CHECK(vk.vkAllocateCommandBuffers(vkDevice, &commandBufferAllocInfo, &commandBuffer));
    return std::shared_ptr<PostResource>(new PostResource(
        vk, vkDevice, vkCommandPool, fence, semaphores[0], semaphores[1], commandBuffer));
}

DisplayVk::PostResource::~PostResource() {
    m_vk.vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool, 1, &m_vkCommandBuffer);
    m_vk.vkDestroyFence(m_vkDevice, m_swapchainImageReleaseFence, nullptr);
    m_vk.vkDestroySemaphore(m_vkDevice, m_swapchainImageAcquireSemaphore, nullptr);
    m_vk.vkDestroySemaphore(m_vkDevice, m_swapchainImageReleaseSemaphore, nullptr);
}

DisplayVk::PostResource::PostResource(const VulkanDispatch& vk, VkDevice vkDevice,
                                      VkCommandPool vkCommandPool,
                                      VkFence swapchainImageReleaseFence,
                                      VkSemaphore swapchainImageAcquireSemaphore,
                                      VkSemaphore swapchainImageReleaseSemaphore,
                                      VkCommandBuffer vkCommandBuffer)
    : m_swapchainImageReleaseFence(swapchainImageReleaseFence),
      m_swapchainImageAcquireSemaphore(swapchainImageAcquireSemaphore),
      m_swapchainImageReleaseSemaphore(swapchainImageReleaseSemaphore),
      m_vkCommandBuffer(vkCommandBuffer),
      m_vk(vk),
      m_vkDevice(vkDevice),
      m_vkCommandPool(vkCommandPool) {}

std::unique_ptr<DisplayVk::ImageBorrowResource> DisplayVk::ImageBorrowResource::create(
    const VulkanDispatch& vk, VkDevice device, VkCommandPool commandPool) {
    const VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VK_CHECK(vk.vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer));
    const VkFenceCreateInfo fenceCi = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    VkFence fence = VK_NULL_HANDLE;
    VK_CHECK(vk.vkCreateFence(device, &fenceCi, nullptr, &fence));
    return std::unique_ptr<ImageBorrowResource>(
        new ImageBorrowResource(vk, device, commandPool, fence, commandBuffer));
}

DisplayVk::ImageBorrowResource::~ImageBorrowResource() {
    m_vk.vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool, 1, &m_vkCommandBuffer);
}

DisplayVk::ImageBorrowResource::ImageBorrowResource(const VulkanDispatch& vk, VkDevice device,
                                                    VkCommandPool commandPool, VkFence fence,
                                                    VkCommandBuffer commandBuffer)
    : m_completeFence(fence),
      m_vkCommandBuffer(commandBuffer),
      m_vk(vk),
      m_vkDevice(device),
      m_vkCommandPool(commandPool) {}

}  // namespace vk
}  // namespace gfxstream
