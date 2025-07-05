#include "context.hpp"
#include "utils.hpp"

#include <afmf.hpp>

#include <vector>
#include <vulkan/vulkan_core.h>

LsContext::LsContext(const Hooks::DeviceInfo& info, VkSwapchainKHR swapchain,
        VkExtent2D extent, const std::vector<VkImage>& swapchainImages)
        : swapchain(swapchain), swapchainImages(swapchainImages),
          extent(extent) {
    // initialize afmf
    int frame_0_fd{};
    this->frame_0 = Mini::Image(
        info.device, info.physicalDevice,
        extent, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        &frame_0_fd);

    int frame_1_fd{};
    this->frame_1 = Mini::Image(
        info.device, info.physicalDevice,
        extent, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        &frame_1_fd);

    std::vector<int> out_n_fds(info.frameGen);
    for (size_t i = 0; i < info.frameGen; ++i)
        this->out_n.emplace_back(
            info.device, info.physicalDevice,
            extent, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            &out_n_fds.at(i));

    this->lsfgCtxId = std::shared_ptr<int32_t>(
        new int32_t(AFMF::createContext(extent.width, extent.height,
            frame_0_fd, frame_1_fd, out_n_fds)),
        [](const int32_t* id) {
            AFMF::deleteContext(*id);
        }
    );

    // prepare render passes
    this->cmdPool = Mini::CommandPool(info.device, info.queue.first);
    for (size_t i = 0; i < 8; i++) {
        auto& pass = this->passInfos.at(i);
        pass.renderSemaphores.resize(info.frameGen);
        pass.acquireSemaphores.resize(info.frameGen);
        pass.postCopyBufs.resize(info.frameGen);
        pass.postCopySemaphores.resize(info.frameGen);
        pass.prevPostCopySemaphores.resize(info.frameGen);
    }
}

VkResult LsContext::present(const Hooks::DeviceInfo& info, const void* pNext, VkQueue queue,
        const std::vector<VkSemaphore>& gameRenderSemaphores, uint32_t presentIdx) {
    auto& pass = this->passInfos.at(this->frameIdx % 8);

    // 1. copy swapchain image to frame_0/frame_1
    int preCopySemaphoreFd{};
    pass.preCopySemaphores.at(0) = Mini::Semaphore(info.device, &preCopySemaphoreFd);
    pass.preCopySemaphores.at(1) = Mini::Semaphore(info.device);
    pass.preCopyBuf = Mini::CommandBuffer(info.device, this->cmdPool);
    pass.preCopyBuf.begin();

    Utils::copyImage(pass.preCopyBuf.handle(),
        this->swapchainImages.at(presentIdx),
        this->frameIdx % 2 == 0 ? this->frame_0.handle() : this->frame_1.handle(),
        this->extent.width, this->extent.height,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        true, false);

    pass.preCopyBuf.end();

    std::vector<VkSemaphore> gameRenderSemaphores2 = gameRenderSemaphores;
    if (this->frameIdx > 0)
        gameRenderSemaphores2.emplace_back(this->passInfos.at((this->frameIdx - 1) % 8)
            .preCopySemaphores.at(1).handle());
    pass.preCopyBuf.submit(info.queue.second,
        gameRenderSemaphores2,
        { pass.preCopySemaphores.at(0).handle(),
          pass.preCopySemaphores.at(1).handle() });

    // 2. render intermediary frames
    std::vector<int> renderSemaphoreFds(info.frameGen);
    for (size_t i = 0; i < info.frameGen; ++i)
        pass.renderSemaphores.at(i) = Mini::Semaphore(info.device, &renderSemaphoreFds.at(i));

    AFMF::presentContext(*this->lsfgCtxId,
        preCopySemaphoreFd,
        renderSemaphoreFds);

    for (size_t i = 0; i < info.frameGen; i++) {
        // 3. acquire next swapchain image
        pass.acquireSemaphores.at(i) = Mini::Semaphore(info.device);
        uint32_t imageIdx{};
        auto res = vkAcquireNextImageKHR(info.device, this->swapchain, UINT64_MAX,
            pass.acquireSemaphores.at(i).handle(), VK_NULL_HANDLE, &imageIdx);
        if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
            throw AFMF::vulkan_error(res, "Failed to acquire next swapchain image");

        // 4. copy output image to swapchain image
        pass.postCopySemaphores.at(i) = Mini::Semaphore(info.device);
        pass.prevPostCopySemaphores.at(i) = Mini::Semaphore(info.device);
        pass.postCopyBufs.at(i) = Mini::CommandBuffer(info.device, this->cmdPool);
        pass.postCopyBufs.at(i).begin();

        Utils::copyImage(pass.postCopyBufs.at(i).handle(),
            this->out_n.at(i).handle(),
            this->swapchainImages.at(imageIdx),
            this->extent.width, this->extent.height,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            false, true);

        pass.postCopyBufs.at(i).end();
        pass.postCopyBufs.at(i).submit(info.queue.second,
            { pass.acquireSemaphores.at(i).handle(),
              pass.renderSemaphores.at(i).handle() },
            { pass.postCopySemaphores.at(i).handle(),
              pass.prevPostCopySemaphores.at(i).handle() });

        // 5. present swapchain image
        std::vector<VkSemaphore> waitSemaphores{ pass.postCopySemaphores.at(i).handle() };
        if (i != 0) waitSemaphores.emplace_back(pass.prevPostCopySemaphores.at(i - 1).handle());

        const VkPresentInfoKHR presentInfo{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = i == 0 ? pNext : nullptr, // only set on first present
            .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
            .pWaitSemaphores = waitSemaphores.data(),
            .swapchainCount = 1,
            .pSwapchains = &this->swapchain,
            .pImageIndices = &imageIdx,
        };
        res = vkQueuePresentKHR(queue, &presentInfo);
        if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
            throw AFMF::vulkan_error(res, "Failed to present swapchain image");
    }

    // 6. present actual next frame
    VkSemaphore lastPrevPostCopySemaphore =
        pass.prevPostCopySemaphores.at(info.frameGen - 1).handle();
    const VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &lastPrevPostCopySemaphore,
        .swapchainCount = 1,
        .pSwapchains = &this->swapchain,
        .pImageIndices = &presentIdx,
    };
    auto res = vkQueuePresentKHR(queue, &presentInfo);
    if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
        throw LSFG::vulkan_error(res, "Failed to present swapchain image");

    this->frameIdx++;
    return res;
}
