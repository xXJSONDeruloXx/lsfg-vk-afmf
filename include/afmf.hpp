#ifndef AFMF_HPP
#define AFMF_HPP

#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace AFMF {

    ///
    /// Initialize the AFMF library (AMD FidelityFX Motion Frames).
    ///
    /// @throws AFMF::vulkan_error if Vulkan objects fail to initialize.
    ///
    void initialize();

    ///
    /// Create a new AFMF context on a swapchain.
    ///
    /// @param width Width of the input images.
    /// @param height Height of the input images.
    /// @param in0 File descriptor for the first input image.
    /// @param in1 File descriptor for the second input image.
    /// @param outN File descriptor for each output image. This defines the frame generation multiplier.
    /// @return A unique identifier for the created context.
    ///
    /// @throws AFMF::vulkan_error if the context cannot be created.
    ///
    int32_t createContext(uint32_t width, uint32_t height, int in0, int in1,
        const std::vector<int>& outN);

    ///
    /// Present a context with frame interpolation.
    ///
    /// @param id Unique identifier of the context to present.
    /// @param inSem Semaphore to wait on before starting the generation.
    /// @param outSem Semaphores to signal once each output image is ready.
    ///
    /// @throws AFMF::vulkan_error if the context cannot be presented.
    ///
    void presentContext(int32_t id, int inSem, const std::vector<int>& outSem);

    ///
    /// Delete an AFMF context.
    ///
    /// @param id Unique identifier of the context to delete.
    ///
    void deleteContext(int32_t id);

    ///
    /// Deinitialize the AFMF library.
    ///
    void finalize();

    /// Simple exception class for Vulkan errors.
    class vulkan_error : public std::runtime_error {
    public:
        ///
        /// Construct a vulkan_error with a message and a Vulkan result code.
        ///
        /// @param result The Vulkan result code associated with the error.
        /// @param message The error message.
        ///
        explicit vulkan_error(VkResult result, const std::string& message);

        /// Get the Vulkan result code associated with this error.
        [[nodiscard]] VkResult error() const { return this->result; }

        // Trivially copyable, moveable and destructible
        vulkan_error(const vulkan_error&) = default;
        vulkan_error(vulkan_error&&) = default;
        vulkan_error& operator=(const vulkan_error&) = default;
        vulkan_error& operator=(vulkan_error&&) = default;
        ~vulkan_error() noexcept override;
    private:
        VkResult result;
    };

}

#endif // AFMF_HPP
