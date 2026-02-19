-- WalnutExternal.lua

VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"
IncludeDir["glm"] = "../vendor/glm"
IncludeDir["CEF"] = "../vendor/cef"
IncludeDir["IconFontCppHeaders"] = "../vendor/IconFontCppHeaders"
IncludeDir["SQLite"] = "../vendor/sqlite"

LibraryDir = {}
LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"
LibraryDir["CEF"] = "../vendor/cef/Release"
LibraryDir["CEF_Debug"] = "../vendor/cef/Debug"

Library = {}
Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"
Library["CEF"] = "%{LibraryDir.CEF}/libcef.lib"
Library["CEF_Debug"] = "%{LibraryDir.CEF_Debug}/libcef.lib"

group "Dependencies"
   include "vendor/imgui"
   include "vendor/glfw"
   include "vendor/cef"
   include "vendor/sqlite"
group ""

group "Core"
include "Walnut"
group ""