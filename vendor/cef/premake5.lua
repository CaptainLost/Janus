project "CEF"
   kind "StaticLib"
   language "C++"
   cppdialect "C++20"
   staticruntime "off"

   targetdir ("bin/" .. outputdir .. "/%{prj.name}")
   objdir ("../../bin-int/" .. outputdir .. "/%{prj.name}")

   files
   {
      "libcef_dll/wrapper/**.cc",
      "libcef_dll/wrapper/**.h",
      "libcef_dll/base/**.cc",
      "libcef_dll/base/**.h",
      "libcef_dll/cpptoc/**.cc",
      "libcef_dll/cpptoc/**.h",
      "libcef_dll/ctocpp/**.cc",
      "libcef_dll/ctocpp/**.h",
      "libcef_dll/shutdown_checker.cc",
      "libcef_dll/shutdown_checker.h",
      "libcef_dll/transfer_util.cc",
      "libcef_dll/transfer_util.h",
   }

   includedirs
   {
      ".",
   }

   defines
   {
      "USING_CEF_SHARED",
      "WRAPPING_CEF_SHARED",
   }

   filter "system:windows"
      systemversion "latest"
      defines
      {
         "WIN32",
         "_WINDOWS",
         "UNICODE",
         "_UNICODE",
         "NOMINMAX",
         "WIN32_LEAN_AND_MEAN",
      }

   filter "configurations:Debug"
      runtime "Debug"
      symbols "On"
      optimize "Off"

   filter "configurations:Release"
      runtime "Release"
      optimize "On"
      symbols "On"

   filter "configurations:Dist"
      runtime "Release"
      optimize "On"
      symbols "Off"
