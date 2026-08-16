#ifndef DISPLAY_VK_H
#define DISPLAY_VK_H

#include <atomic>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

#include "BorrowedImage.h"
#include "CompositorVk.h"
#include "Display.h"
#include "DisplaySurfaceVk.h"
#include "Hwc2.h"
#include "SwapChainStateVk.h"
#include "aemu/base/synchronization/Lock.h"
#include "goldfish_vk_dispatch.h"

// The DisplayVk class holds the Vulkan and other states required to draw a
// frame in a host window.

namespace gfxstream {
namespace vk {

class DisplayVk : public gfxstream::Display {
   public:
    DisplayVk(const VulkanDispatch&, VkPhysicalDevice, uint32_t swapChainQueueFamilyIndex,
              uint32_t compositorQueueFamilyIndex, VkDevice, VkQueue compositorVkQueue,
              std::shared_ptr<android::base::Lock> compositorVkQueueLock, VkQueue swapChainVkQueue,
              std::shared_ptr<android::base::Lock> swapChainVkQueueLock);
    ~DisplayVk();

    PostResult post(const BorrowedImageInfo* info);

    // Runs on PostWorker and creates the swapchain for the latest committed surface generation
    // without requiring a new guest frame.
    bool recreateSwapchainIfNeeded();

    // Safe to query from the native window thread. DisplaySurface metadata can already contain a
    // requested Win32 extent while PostWorker still presents the previous swapchain generation.
    bool isSwapchainCurrentForSurface() const;

    void drainQueues();

   protected:
    void bindToSurfaceImpl(gfxstream::DisplaySurface* surface) override;
    void surfaceUpdated(gfxstream::DisplaySurface* surface) override;
    void unbindFromSurfaceImpl() override;

   private:
    void destroySwapchain();
    bool recreateSwapchain(uint64_t surfaceGeneration);

    // The success component of the result is false when the swapchain is no longer valid and
    // bindToSurface() needs to be called again. When the success component is true, the waitable
    // component of the returned result is a future that will complete when the GPU side of work
    // completes. The caller is responsible to guarantee the synchronization and the layout of
    // ColorBufferCompositionInfo::m_vkImage is VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL.
    PostResult postImpl(const BorrowedImageInfo* info);

    VkFormatFeatureFlags getFormatFeatures(VkFormat, VkImageTiling);
    bool canPost(const VkImageCreateInfo&);

    const VulkanDispatch& m_vk;
    VkPhysicalDevice m_vkPhysicalDevice;
    uint32_t m_swapChainQueueFamilyIndex;
    uint32_t m_compositorQueueFamilyIndex;
    VkDevice m_vkDevice;
    VkQueue m_compositorVkQueue;
    std::shared_ptr<android::base::Lock> m_compositorVkQueueLock;
    VkQueue m_swapChainVkQueue;
    std::shared_ptr<android::base::Lock> m_swapChainVkQueueLock;
    VkCommandPool m_vkCommandPool;

    class PostResource {
       public:
        const VkFence m_swapchainImageReleaseFence;
        const VkSemaphore m_swapchainImageAcquireSemaphore;
        const VkSemaphore m_swapchainImageReleaseSemaphore;
        const VkCommandBuffer m_vkCommandBuffer;
        static std::shared_ptr<PostResource> create(const VulkanDispatch&, VkDevice, VkCommandPool);
        ~PostResource();
        DISALLOW_COPY_ASSIGN_AND_MOVE(PostResource);

       private:
        PostResource(const VulkanDispatch&, VkDevice, VkCommandPool,
                     VkFence swapchainImageReleaseFence, VkSemaphore swapchainImageAcquireSemaphore,
                     VkSemaphore swapchainImageReleaseSemaphore, VkCommandBuffer);
        const VulkanDispatch& m_vk;
        const VkDevice m_vkDevice;
        const VkCommandPool m_vkCommandPool;
    };

    std::deque<std::shared_ptr<PostResource>> m_freePostResources;
    std::vector<std::optional<std::shared_future<std::shared_ptr<PostResource>>>>
        m_postResourceFutures;
    int m_inFlightFrameIndex;

    class ImageBorrowResource {
       public:
        const VkFence m_completeFence;
        const VkCommandBuffer m_vkCommandBuffer;
        static std::unique_ptr<ImageBorrowResource> create(const VulkanDispatch&, VkDevice,
                                                           VkCommandPool);
        ~ImageBorrowResource();
        DISALLOW_COPY_ASSIGN_AND_MOVE(ImageBorrowResource);

       private:
        ImageBorrowResource(const VulkanDispatch&, VkDevice, VkCommandPool, VkFence,
                            VkCommandBuffer);
        const VulkanDispatch& m_vk;
        const VkDevice m_vkDevice;
        const VkCommandPool m_vkCommandPool;
    };
    std::vector<std::unique_ptr<ImageBorrowResource>> m_imageBorrowResources;

    std::unique_ptr<SwapChainStateVk> m_swapChainStateVk;
    // Keep the previously presented Win32 swapchain alive while the replacement receives its
    // first frame. DWM can retain the old presentation instead of exposing an empty surface.
    std::unique_ptr<SwapChainStateVk> m_retiredSwapChainStateVk;
    // DisplaySurface notifications run on the native window thread while post() runs on the
    // PostWorker. A plain bool loses runtime resize/rotation updates under that cross-thread
    // access, leaving DisplayVk on the previous extent even though the HWND already changed.
    std::atomic<bool> m_needToRecreateSwapChain{true};
    std::atomic<bool> m_surfaceQueuesAlreadyDrained{false};
    // Monotonic cross-thread surface state. A bool alone can lose a resize when surfaceUpdated()
    // sets it while PostWorker is completing an older recreation and subsequently clears it.
    std::atomic<uint64_t> m_surfaceGeneration{1};
    std::atomic<uint64_t> m_appliedSurfaceGeneration{0};
    std::atomic<uint32_t> m_appliedSwapchainWidth{0};
    std::atomic<uint32_t> m_appliedSwapchainHeight{0};

    std::unordered_map<VkFormat, VkFormatProperties> m_vkFormatProperties;
};

}  // namespace vk
}  // namespace gfxstream

#endif
