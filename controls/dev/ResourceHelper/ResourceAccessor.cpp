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

winrt::Microsoft::Windows::ApplicationModel::Resources::ResourceManager  ResourceAccessor::m_resourceManagerWinRT{ nullptr };


winrt::Microsoft::Windows::ApplicationModel::Resources::ResourceMap ResourceAccessor::GetResourceMap()
{
    return ResourceAccessor::GetResourceManager().MainResourceMap().GetSubtree(ResourceAccessor::c_resourceLoc);
}

winrt::Microsoft::Windows::ApplicationModel::Resources::ResourceManager ResourceAccessor::GetResourceManagerImpl()
{
    winrt::hstring frameworkInstallLocation;
    if (m_resourceManagerWinRT == nullptr)
    {
        if (SharedHelpers::IsInFrameworkPackage(frameworkInstallLocation))
        {
            ResourceAccessor::m_resourceManagerWinRT = winrt::Microsoft::Windows::ApplicationModel::Resources::ResourceManager(frameworkInstallLocation + L"\\resources.pri");
        }
        else
        {
            ResourceAccessor:: m_resourceManagerWinRT = winrt::Microsoft::Windows::ApplicationModel::Resources::ResourceManager();
        }
    }
    return m_resourceManagerWinRT;
}

winrt::Microsoft::Windows::ApplicationModel::Resources::ResourceManager ResourceAccessor::GetResourceManager()
{
    auto static resourceManager = ResourceAccessor::GetResourceManagerImpl();
    return resourceManager;
}

winrt::Microsoft::Windows::ApplicationModel::Resources::ResourceContext ResourceAccessor::GetResourceContext()
{
    auto static m_resourceContextWinRT = ResourceAccessor::GetResourceManager().CreateResourceContext(); 
    return m_resourceContextWinRT;
}


winrt::hstring ResourceAccessor::GetLocalizedStringResource(const wstring_view& resourceName)
{
    static auto mrt_lifted_resourceMap = GetResourceMap();
    static auto mrt_lifted_resourceContext = GetResourceContext();
    return mrt_lifted_resourceMap.GetValue(resourceName, mrt_lifted_resourceContext).ValueAsString();
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

winrt::LoadedImageSurface ResourceAccessor::GetImageSurface(const wstring_view& assetName, winrt::Size imageSize)
{
    auto imageUri = winrt::Uri{ std::wstring(L"ms-resource:///Files/Microsoft.UI.Xaml/Assets/") + std::wstring(assetName.data()) + std::wstring(L".png") };
    return winrt::LoadedImageSurface::StartLoadFromUri(imageUri, imageSize);
}

winrt::IInspectable ResourceAccessor::ResourceLookup(const winrt::Control& control, const winrt::IInspectable& key)
{
    return control.Resources().HasKey(key) ? control.Resources().Lookup(key) : winrt::Application::Current().Resources().TryLookup(key);
}
