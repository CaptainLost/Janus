#pragma once

#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>

namespace Walnut {

	inline void check_vk_result(VkResult err)
	{
		if (err == 0)
			return;
		fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
		if (err < 0)
			abort();
	}

}
