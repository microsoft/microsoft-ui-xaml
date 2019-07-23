// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ResourceAccessor.h"
#include "common.h"

PCWSTR ResourceAccessor::c_resourceLoc{ L"Microsoft.UI.Xaml/Resources" };

#ifdef BUILD_WINDOWS

#include "ResourceHelper.h"

winrt::hstring ResourceAccessor::GetLocalizedStringResource(int resourceId)
{
    // On the Windows side, we get localized strings from a MUI file instead of from a PRI file.
    // To accomplish this, we have a windows specific header MuiHelper.h that defines this method,
    // meaning that this function won't be found in the Git repository.
    return GetLocalizedStringResourceFromMui(resourceId);
}

winrt::LoadedImageSurface ResourceAccessor::GetImageSurface(int assetId, winrt::Size imageSize)
{

    // On the Windows side, we get images from the DLL file instead of from a PRI file.
    // To accomplish this, we have a Windows-specific header MuiHelper.h that defines this method,
    // meaning that this function won't be found in the WinUI repository.
    winrt::array_view<const byte> imageArrayView = GetImageBytesFromDll(assetId);
    winrt::InMemoryRandomAccessStream imageStream = SharedHelpers::CreateStreamFromBytes(imageArrayView);
    return winrt::LoadedImageSurface::StartLoadFromStream(imageStream, imageSize);
}

#else

winrt::ResourceMap ResourceAccessor::GetResourceMap()
{
    auto packageResourceMap = []() {
        if (SharedHelpers::IsInFrameworkPackage())
        {
            winrt::hstring packageName{ MUXCONTROLS_PACKAGE_NAME };
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

winrt::LoadedImageSurface ResourceAccessor::GetImageSurface(const wstring_view &assetName, winrt::Size imageSize)
{
    auto imageUri = [assetName]() {
        if (SharedHelpers::IsInFrameworkPackage())
        {
            return winrt::Uri{ std::wstring(L"ms-resource://" MUXCONTROLS_PACKAGE_NAME "/Files/Microsoft.UI.Xaml/Assets/") + std::wstring(assetName.data()) + std::wstring(L".png")  };
        }
        else
        {
            return winrt::Uri{ std::wstring(L"ms-resource:///Files/Microsoft.UI.Xaml/Assets/") + std::wstring(assetName.data()) + std::wstring(L".png") };
        }
    }();
    return winrt::LoadedImageSurface::StartLoadFromUri(imageUri, imageSize);
}

#endif
