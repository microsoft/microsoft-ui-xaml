// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"

PCWSTR ResourceAccessor::c_resourceLocWinUI{ L"Microsoft.UI.Xaml/Resources" };

#ifdef MUX_EXPERIMENTAL
PCWSTR ResourceAccessor::c_resourceLoc{ L"Microsoft.Experimental.UI.Xaml/Resources" };
#else
PCWSTR ResourceAccessor::c_resourceLoc{ L"Microsoft.UI.Xaml/Resources" };
#endif

winrt::ResourceMap ResourceAccessor::GetResourceMap()
{
    auto packageResourceMap = []() {
        if (SharedHelpers::IsInFrameworkPackage())
        {
            winrt::hstring packageName{ MUXCONTROLS_PACKAGE_NAME };
            return winrt::ResourceManager::Current().AllResourceMaps().Lookup(packageName);
        }
        else if (SharedHelpers::IsInCBSPackage())
        {
            winrt::hstring packageName{ MUXCONTROLS_CBS_PACKAGE_NAME };
            return winrt::ResourceManager::Current().AllResourceMaps().Lookup(packageName);
        }
        else
        {
            return winrt::ResourceManager::Current().MainResourceMap();
        }
    }();

    return packageResourceMap.GetSubtree(ResourceAccessor::c_resourceLoc);
}

winrt::hstring ResourceAccessor::GetLocalizedStringResource(const wstring_view &resourceName)
{
    static winrt::ResourceMap s_resourceMap = GetResourceMap();
    static winrt::ResourceContext s_resourceContext = winrt::ResourceContext::GetForViewIndependentUse();

    return s_resourceMap.GetValue(resourceName, s_resourceContext).ValueAsString();
}

#ifdef MUX_EXPERIMENTAL
winrt::hstring ResourceAccessor::GetLocalizedStringResourceFromWinUI(const wstring_view& resourceName)
{
    static winrt::ResourceMap s_resourceMapWinUI = []() {

        constexpr wchar_t c_winUIPackageNamePrefix[] = L"Microsoft.UI.Xaml.";
        constexpr size_t c_winUIPackageNamePrefixLength = ARRAYSIZE(c_winUIPackageNamePrefix) - 1;

        for (auto resourceMapKvp : winrt::ResourceManager::Current().AllResourceMaps())
        {
            auto resourceMapName = resourceMapKvp.Key();
            if (std::wstring_view(resourceMapName.c_str(), c_winUIPackageNamePrefixLength) == std::wstring_view(c_winUIPackageNamePrefix))
            {
                return resourceMapKvp.Value().GetSubtree(ResourceAccessor::c_resourceLocWinUI);
            }
        }

        // If we don't find a matching framework package, use the app's resources:
        return winrt::ResourceManager::Current().MainResourceMap().GetSubtree(ResourceAccessor::c_resourceLocWinUI);
    }();

    static winrt::ResourceContext s_resourceContext = winrt::ResourceContext::GetForViewIndependentUse();

    return s_resourceMapWinUI.GetValue(resourceName, s_resourceContext).ValueAsString();
}
#endif

winrt::LoadedImageSurface ResourceAccessor::GetImageSurface(const wstring_view &assetName, winrt::Size imageSize)
{
    auto imageUri = [assetName]() {
        if (SharedHelpers::IsInFrameworkPackage())
        {
            return winrt::Uri{ std::wstring(L"ms-resource://" MUXCONTROLS_PACKAGE_NAME "/Files/Microsoft.UI.Xaml/Assets/") + std::wstring(assetName.data()) + std::wstring(L".png")  };
        }
        else if (SharedHelpers::IsInCBSPackage())
        {
            return winrt::Uri{ std::wstring(L"ms-resource://" MUXCONTROLS_CBS_PACKAGE_NAME "/Files/Microsoft.UI.Xaml/Assets/") + std::wstring(assetName.data()) + std::wstring(L".png")  };
        }
        else
        {
            return winrt::Uri{ std::wstring(L"ms-resource:///Files/Microsoft.UI.Xaml/Assets/") + std::wstring(assetName.data()) + std::wstring(L".png") };
        }
    }();
    return winrt::LoadedImageSurface::StartLoadFromUri(imageUri, imageSize);
}

winrt::IInspectable ResourceAccessor::ResourceLookup(const winrt::Control& control, const winrt::IInspectable& key)
{
    return control.Resources().HasKey(key) ? control.Resources().Lookup(key) : winrt::Application::Current().Resources().TryLookup(key);
}
