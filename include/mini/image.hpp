#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <vulkan/vulkan_core.h>

#include <memory>

namespace Mini {

    ///
    /// C++ wrapper class for a Vulkan image.
    ///
    /// This class manages the lifetime of a Vulkan image.
    ///
    class Image {
    public:
        Image() noexcept = default;

        ///
        /// Create the image and export the backing fd
        ///
        /// @param device Vulkan device
        /// @param physicalDevice Vulkan physical device
        /// @param extent Extent of the image in pixels.
        /// @param format Vulkan format of the image
        /// @param usage Usage flags for the image
        /// @param aspectFlags Aspect flags for the image view
        /// @param fd Pointer to an integer where the file descriptor will be stored.
        ///
        /// @throws LSFG::vulkan_error if object creation fails.
        ///
        Image(VkDevice device, VkPhysicalDevice physicalDevice, VkExtent2D extent, VkFormat format,
            VkImageUsageFlags usage, VkImageAspectFlags aspectFlags, int* fd);

        /// Get the Vulkan handle.
        [[nodiscard]] auto handle() const { return *this->image; }
        /// Get the Vulkan device memory handle.
        [[nodiscard]] auto getMemory() const { return *this->memory; }
        /// Get the extent of the image.
        [[nodiscard]] VkExtent2D getExtent() const { return this->extent; }
        /// Get the format of the image.
        [[nodiscard]] VkFormat getFormat() const { return this->format; }
        /// Get the aspect flags of the image.
        [[nodiscard]] VkImageAspectFlags getAspectFlags() const { return this->aspectFlags; }

        /// Trivially copyable, moveable and destructible
        Image(const Image&) noexcept = default;
        Image& operator=(const Image&) noexcept = default;
        Image(Image&&) noexcept = default;
        Image& operator=(Image&&) noexcept = default;
        ~Image() = default;
    private:
        std::shared_ptr<VkImage> image;
        std::shared_ptr<VkDeviceMemory> memory;

        VkExtent2D extent{};
        VkFormat format{};
        VkImageAspectFlags aspectFlags{};
    };

}

#endif // IMAGE_HPP
