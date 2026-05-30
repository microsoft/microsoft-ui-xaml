#[[====================================================================================================================

    Microsoft.WindowsAppSDK.WinUI

    CMake configuration file for Microsoft.WindowsAppSDK.WinUI package.

        find_package(Microsoft.WindowsAppSDK.WinUI CONFIG REQUIRED)

        target_link_libraries(<your-target>
            PRIVATE
                Microsoft.WindowsAppSDK.WinUI_SelfContained  # or _Framework
        )

    License:
        See the LICENSE file in the package root for more information.
====================================================================================================================]]#
include_guard()

if(CMAKE_VERSION VERSION_LESS 3.31)
    message(FATAL_ERROR "Microsoft.WindowsAppSDK.WinUI requires at least CMake 3.31, but CMake ${CMAKE_VERSION} is in use.")
endif()


find_package(Microsoft.WindowsAppSDK.Foundation CONFIG QUIET)
find_package(Microsoft.WindowsAppSDK.InteractiveExperiences CONFIG QUIET)

# WebView2 WinMD reference: needed for CppWinRT projection type resolution.
#
# NOTE: This is a minimal integration for WinUI's CppWinRT projection dependency only.
# The WebView2 NuGet also ships native headers (build/native/include) and the
# WebView2Loader library (lib/WebView2LoaderStatic.lib, runtimes/.../WebView2Loader.dll)
# which are NOT exposed here. Direct WebView2 C/C++ API usage is not yet supported
# through this target — a more complete WebView2 cmake config would be needed for that.
if(NOT TARGET Microsoft.Web.WebView2)
    get_property(_WEBVIEW2_LOCATION GLOBAL PROPERTY NUGET_LOCATION-MICROSOFT_WEB_WEBVIEW2)
    if(NOT _WEBVIEW2_LOCATION)
        message(FATAL_ERROR "Microsoft.Web.WebView2 package location not found. "
            "Ensure Microsoft.Web.WebView2 is listed in add_nuget_packages(). "
            "Please see https://github.com/mschofie/NuGetCMakePackage/blob/develop/.github/copilot-instructions.md")
    endif()
    add_library(Microsoft.Web.WebView2 INTERFACE)
    set_target_properties(Microsoft.Web.WebView2 PROPERTIES
        INTERFACE_CPPWINRT_REFS "${_WEBVIEW2_LOCATION}/lib/Microsoft.Web.WebView2.Core.winmd"
    )
endif()

block(SCOPE_FOR VARIABLES)
    wasdk_detect_platform()

    # NuGet package root: this file lives at build/cmake/PACKAGE_NAME_LOWER-config.cmake
    set(PACKAGE_LOCATION "${CMAKE_CURRENT_LIST_DIR}/../..")

    #[[====================================================================================================================
        Target: Microsoft.WindowsAppSDK.WinUI
    ====================================================================================================================]]#
    set(_WINUI_PROJECTION_DEPS Microsoft.Windows.CppWinRT)
    if(TARGET Microsoft.WindowsAppSDK.Foundation)
        list(APPEND _WINUI_PROJECTION_DEPS Microsoft.WindowsAppSDK.Foundation)
    endif()
    if(TARGET Microsoft.WindowsAppSDK.InteractiveExperiences)
        list(APPEND _WINUI_PROJECTION_DEPS Microsoft.WindowsAppSDK.InteractiveExperiences)
    endif()
    if(TARGET Microsoft.Web.WebView2)
        list(APPEND _WINUI_PROJECTION_DEPS Microsoft.Web.WebView2)
    endif()

    file(GLOB _WINMD_FILES "${PACKAGE_LOCATION}/metadata/*.winmd")
    add_cppwinrt_projection(Microsoft.WindowsAppSDK.WinUI
        INPUTS ${_WINMD_FILES}
        OPTIMIZE
        DEPS ${_WINUI_PROJECTION_DEPS}
    )

    target_include_directories(Microsoft.WindowsAppSDK.WinUI
        INTERFACE
            ${PACKAGE_LOCATION}/include
    )

    #[[====================================================================================================================
        Target: Microsoft.WindowsAppSDK.WinUI_SelfContained
    ====================================================================================================================]]#
    set(FRAMEWORK_PATH "${PACKAGE_LOCATION}/runtimes-framework/win-${PLATFORM_IDENTIFIER}/native")
    file(GLOB FRAMEWORK_DLLS "${FRAMEWORK_PATH}/*.dll")
    file(GLOB FRAMEWORK_EXTRA "${FRAMEWORK_PATH}/*.pri")

    add_library(Microsoft.WindowsAppSDK.WinUI_SelfContained INTERFACE)

    wasdk_transform_appxfragment(
        Microsoft.WindowsAppSDK.WinUI_SelfContained
        "${PACKAGE_LOCATION}/runtimes-framework/package.appxfragment"
        "${CMAKE_BINARY_DIR}/__manifests/Microsoft.WindowsAppSDK.WinUI.manifest"
    )

    target_link_libraries(Microsoft.WindowsAppSDK.WinUI_SelfContained
        INTERFACE
            Microsoft.WindowsAppSDK.WinUI
    )

    # Store runtime DLLs for self-contained deployment copy
    set(_WINUI_ALL_RUNTIME_FILES ${FRAMEWORK_DLLS} ${FRAMEWORK_EXTRA})
    set_property(TARGET Microsoft.WindowsAppSDK.WinUI_SelfContained
        PROPERTY INTERFACE_SELFCONTAINED_RUNTIME_DLLS "${_WINUI_ALL_RUNTIME_FILES}")

    #[[====================================================================================================================
        Target: Microsoft.WindowsAppSDK.WinUI_Framework
    ====================================================================================================================]]#
    add_library(Microsoft.WindowsAppSDK.WinUI_Framework INTERFACE)

    target_link_libraries(Microsoft.WindowsAppSDK.WinUI_Framework
        INTERFACE
            Microsoft.WindowsAppSDK.WinUI
    )

endblock()
