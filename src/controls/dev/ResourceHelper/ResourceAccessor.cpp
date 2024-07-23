// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"

#define LOC_PREFIX L"Microsoft.UI.Xaml"
#define LOC_PREFIX_WINUI L"Microsoft.UI.Xaml"

PCWSTR ResourceAccessor::c_resourceLoc{ LOC_PREFIX L"/Resources" };
PCWSTR ResourceAccessor::c_assetLoc{ L"Files/" LOC_PREFIX L"/Assets"};
PCWSTR ResourceAccessor::c_resourceLocWinUI{ LOC_PREFIX_WINUI L"/Resources" };

winrt::Microsoft::Windows::ApplicationModel::Resources::ResourceManager  ResourceAccessor::m_resourceManagerWinRT{ nullptr };

winrt::Microsoft::Windows::ApplicationModel::Resources::ResourceMap ResourceAccessor::GetAssetMap()
{
    return ResourceAccessor::GetResourceManager().MainResourceMap().GetSubtree(ResourceAccessor::c_assetLoc);
}

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

winrt::LoadedImageSurface ResourceAccessor::GetImageSurface(const wstring_view& assetName, winrt::Size imageSize)
{
    auto imageUri = winrt::Uri{ std::wstring(L"ms-resource:///Files/Microsoft.UI.Xaml/Assets/") + std::wstring(assetName.data()) + std::wstring(L".png") };
    return winrt::LoadedImageSurface::StartLoadFromUri(imageUri, imageSize);
}

winrt::IAsyncOperation<winrt::hstring> ResourceAccessor::GetFileContents(const wstring_view& assetFileName)
{
    static auto mrt_lifted_assetMap = GetAssetMap();
    static auto mrt_lifted_resourceContext = GetResourceContext();
    auto filePath = mrt_lifted_assetMap.GetValue(assetFileName, mrt_lifted_resourceContext).ValueAsString();
    auto file = co_await winrt::StorageFile::GetFileFromPathAsync(filePath);
    auto fileBuffer = co_await winrt::FileIO::ReadBufferAsync(file);
    auto fileReader = winrt::DataReader::FromBuffer(fileBuffer);
    co_return fileReader.ReadString(fileReader.UnconsumedBufferLength());
}

winrt::IInspectable ResourceAccessor::ResourceLookup(const winrt::Control& control, const winrt::IInspectable& key)
{
    return control.Resources().HasKey(key) ? control.Resources().Lookup(key) : winrt::Application::Current().Resources().TryLookup(key);
}
