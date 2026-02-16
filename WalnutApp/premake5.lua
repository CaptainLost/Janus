project "WalnutApp"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"
   targetdir "bin/%{cfg.buildcfg}"
   staticruntime "off"

   files { "src/**.h", "src/**.cpp", "app.manifest" }

   includedirs
   {
      "../vendor/imgui",
      "../vendor/glfw/include",

      "../Walnut/src",

      "%{IncludeDir.VulkanSDK}",
      "%{IncludeDir.glm}",
      "%{IncludeDir.CEF}",
   }

    links
    {
        "Walnut",
    }

   defines
   {
      "USING_CEF_SHARED",
      "IMGUI_DEFINE_MATH_OPERATORS",
      "NOMINMAX",
      "WIN32_LEAN_AND_MEAN",
   }

   targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
   objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

   filter "system:windows"
      systemversion "latest"
      defines { "WL_PLATFORM_WINDOWS" }

   filter "configurations:Debug"
      defines { "WL_DEBUG" }
      runtime "Debug"
      symbols "On"
      links
      {
         "%{Library.CEF_Debug}",
      }
      postbuildcommands
      {
         '{COPY} "../vendor/cef/Debug/*.dll" "%{cfg.targetdir}"',
         '{COPY} "../vendor/cef/Debug/*.bin" "%{cfg.targetdir}"',
         '{COPY} "../vendor/cef/Resources/*.*" "%{cfg.targetdir}"',
         '{MKDIR} "%{cfg.targetdir}/locales"',
         '{COPY} "../vendor/cef/Resources/locales" "%{cfg.targetdir}/locales"',
      }

   filter "configurations:Release"
      defines { "WL_RELEASE" }
      runtime "Release"
      optimize "On"
      symbols "On"
      links
      {
         "%{Library.CEF}",
      }
      postbuildcommands
      {
         '{COPY} "../vendor/cef/Release/*.dll" "%{cfg.targetdir}"',
         '{COPY} "../vendor/cef/Release/*.bin" "%{cfg.targetdir}"',
         '{COPY} "../vendor/cef/Resources/*.*" "%{cfg.targetdir}"',
         '{MKDIR} "%{cfg.targetdir}/locales"',
         '{COPY} "../vendor/cef/Resources/locales" "%{cfg.targetdir}/locales"',
      }

   filter "configurations:Dist"
      kind "WindowedApp"
      defines { "WL_DIST" }
      runtime "Release"
      optimize "On"
      symbols "Off"
      links
      {
         "%{Library.CEF}",
      }
      postbuildcommands
      {
         '{COPY} "../vendor/cef/Release/*.dll" "%{cfg.targetdir}"',
         '{COPY} "../vendor/cef/Release/*.bin" "%{cfg.targetdir}"',
         '{COPY} "../vendor/cef/Resources/*.*" "%{cfg.targetdir}"',
         '{MKDIR} "%{cfg.targetdir}/locales"',
         '{COPY} "../vendor/cef/Resources/locales" "%{cfg.targetdir}/locales"',
      }