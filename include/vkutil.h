#ifdef USE_VOLK
#include <volk.h>
#include <volk.c>
#else
#include <vulkan/vulkan.h>
#endif

static const char *vulkan_strerror(VkResult err)
{
#define ERR_STR(r) case VK_ ##r: return #r

	switch (err) {
		ERR_STR(SUCCESS);
		ERR_STR(NOT_READY);
		ERR_STR(TIMEOUT);
		ERR_STR(EVENT_SET);
		ERR_STR(EVENT_RESET);
		ERR_STR(INCOMPLETE);
		ERR_STR(SUBOPTIMAL_KHR);
		ERR_STR(ERROR_OUT_OF_HOST_MEMORY);
		ERR_STR(ERROR_OUT_OF_DEVICE_MEMORY);
		ERR_STR(ERROR_INITIALIZATION_FAILED);
		ERR_STR(ERROR_DEVICE_LOST);
		ERR_STR(ERROR_MEMORY_MAP_FAILED);
		ERR_STR(ERROR_LAYER_NOT_PRESENT);
		ERR_STR(ERROR_EXTENSION_NOT_PRESENT);
		ERR_STR(ERROR_FEATURE_NOT_PRESENT);
		ERR_STR(ERROR_INCOMPATIBLE_DRIVER);
		ERR_STR(ERROR_TOO_MANY_OBJECTS);
		ERR_STR(ERROR_FORMAT_NOT_SUPPORTED);
		ERR_STR(ERROR_SURFACE_LOST_KHR);
		ERR_STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
		ERR_STR(ERROR_OUT_OF_DATE_KHR);
		ERR_STR(ERROR_FRAGMENTED_POOL);
		ERR_STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
		ERR_STR(ERROR_VALIDATION_FAILED_EXT);
		ERR_STR(ERROR_INVALID_EXTERNAL_HANDLE);
		ERR_STR(ERROR_OUT_OF_POOL_MEMORY);
		ERR_STR(ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);

	default:
		return "<unknown>";
	}

#undef ERR_STR
}
// Macro to check and display Vulkan return results
#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		printf("Fatal : VkResult is \" %s \" in %s at line %d\n", vulkan_strerror(res), __FILE__, __LINE__); \
		exit(0); \
	}																									\
}

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
						VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	}
	return -1;
}
