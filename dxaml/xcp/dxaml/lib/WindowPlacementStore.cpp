// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "WindowPlacementStore.h"
#include "WindowPlacementValueName.h"

#include <Microsoft.Windows.Storage.h>

using namespace DirectUI;

namespace mws = ABI::Microsoft::Windows::Storage;

namespace WindowPlacementPersistence {

namespace {

// Breadcrumb for the failure paths. Placement failures are silent by design, so
// this is the only local signal that something went wrong.
//
// It deliberately logs the operation and the HRESULT and nothing else. The value
// name embeds a hash of an app-chosen id, and the text is the encoded blob with
// the window's coordinates and monitor name, so neither is safe to trace.
void TracePlacementStoreFailure(_In_z_ const char* operation, HRESULT hr)
{
#if DBG
    char message[128];
    if (SUCCEEDED(StringCchPrintfA(
            message, ARRAYSIZE(message), "WindowPlacement: %s failed with hr=0x%08X\r\n", operation, hr)))
    {
        OutputDebugStringA(message);
    }
#else
    UNREFERENCED_PARAMETER(operation);
    UNREFERENCED_PARAMETER(hr);
#endif
}

// Opens LocalSettings for the current app.
//
// GetDefault fails for an unpackaged app on a Windows App SDK that has no
// unpackaged ApplicationData support. That is an expected outcome, not a bug, so
// the caller treats it as "no store".
_Check_return_ HRESULT GetLocalSettings(_Outptr_ mws::IApplicationDataContainer** localSettings)
{
    *localSettings = nullptr;

    ctl::ComPtr<mws::IApplicationDataStatics> applicationDataStatics;
    IFC_RETURN(ctl::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_Windows_Storage_ApplicationData).Get(),
        &applicationDataStatics));

    ctl::ComPtr<mws::IApplicationData> applicationData;
    IFC_RETURN(applicationDataStatics->GetDefault(&applicationData));
    IFCPTR_RETURN(applicationData.Get());

    ctl::ComPtr<mws::IApplicationDataContainer> settings;
    IFC_RETURN(applicationData->get_LocalSettings(&settings));
    IFCPTR_RETURN(settings.Get());

    *localSettings = settings.Detach();
    return S_OK;
}

// Opens the placement container inside LocalSettings.
//
// `createIfMissing` is false on the read path so that merely starting an app that
// has never saved a placement does not create a settings key.
_Check_return_ HRESULT GetPlacementContainer(
    bool createIfMissing,
    _Outptr_result_maybenull_ mws::IApplicationDataContainer** container)
{
    *container = nullptr;

    ctl::ComPtr<mws::IApplicationDataContainer> localSettings;
    IFC_RETURN(GetLocalSettings(&localSettings));

    // CreateContainer with Existing returns null rather than failing when the
    // container has never been written.
    ctl::ComPtr<mws::IApplicationDataContainer> placementContainer;
    IFC_RETURN(localSettings->CreateContainer(
        wrl_wrappers::HStringReference(c_settingsContainerName).Get(),
        createIfMissing ? mws::ApplicationDataCreateDisposition_Always
                        : mws::ApplicationDataCreateDisposition_Existing,
        &placementContainer));

    *container = placementContainer.Detach();
    return S_OK;
}

// Gets the container's value map. Placement values are plain strings, so this is
// the only collection the store touches.
_Check_return_ HRESULT GetValues(
    _In_ mws::IApplicationDataContainer* container,
    _Outptr_ wfc::IMap<HSTRING, IInspectable*>** values)
{
    *values = nullptr;

    ctl::ComPtr<wfc::IPropertySet> propertySet;
    IFC_RETURN(container->get_Values(&propertySet));
    IFCPTR_RETURN(propertySet.Get());

    ctl::ComPtr<wfc::IMap<HSTRING, IInspectable*>> map;
    IFC_RETURN(propertySet.As(&map));

    *values = map.Detach();
    return S_OK;
}

_Check_return_ HRESULT LoadImpl(_In_ const std::wstring& valueName, _Out_ std::wstring& text)
{
    text.clear();

    ctl::ComPtr<mws::IApplicationDataContainer> container;
    IFC_RETURN(GetPlacementContainer(false /* createIfMissing */, &container));
    if (!container)
    {
        // Nothing has ever been saved for this app.
        return S_FALSE;
    }

    ctl::ComPtr<wfc::IMap<HSTRING, IInspectable*>> values;
    IFC_RETURN(GetValues(container.Get(), &values));

    wrl_wrappers::HStringReference key(valueName.c_str(), static_cast<UINT32>(valueName.size()));

    boolean hasKey = false;
    IFC_RETURN(values->HasKey(key.Get(), &hasKey));
    if (!hasKey)
    {
        return S_FALSE;
    }

    ctl::ComPtr<IInspectable> value;
    IFC_RETURN(values->Lookup(key.Get(), &value));
    if (!value)
    {
        return S_FALSE;
    }

    // A non-string value means someone else wrote to our container. Treat it the
    // same as a corrupt blob: ignore it rather than failing the window.
    ctl::ComPtr<wf::IPropertyValue> propertyValue;
    if (FAILED(value.As(&propertyValue)) || !propertyValue)
    {
        return S_FALSE;
    }

    wf::PropertyType propertyType = wf::PropertyType_Empty;
    IFC_RETURN(propertyValue->get_Type(&propertyType));
    if (propertyType != wf::PropertyType_String)
    {
        return S_FALSE;
    }

    wrl_wrappers::HString storedText;
    IFC_RETURN(propertyValue->GetString(storedText.GetAddressOf()));

    UINT32 length = 0;
    const WCHAR* buffer = storedText.GetRawBuffer(&length);
    if (!buffer || length == 0)
    {
        return S_FALSE;
    }

    text.assign(buffer, length);
    return S_OK;
}

_Check_return_ HRESULT SaveImpl(_In_ const std::wstring& valueName, _In_ const std::wstring& text)
{
    ctl::ComPtr<mws::IApplicationDataContainer> container;
    IFC_RETURN(GetPlacementContainer(true /* createIfMissing */, &container));
    IFCPTR_RETURN(container.Get());

    ctl::ComPtr<wfc::IMap<HSTRING, IInspectable*>> values;
    IFC_RETURN(GetValues(container.Get(), &values));

    ctl::ComPtr<wf::IPropertyValueStatics> propertyValueStatics;
    IFC_RETURN(ctl::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Foundation_PropertyValue).Get(),
        &propertyValueStatics));

    ctl::ComPtr<IInspectable> value;
    IFC_RETURN(propertyValueStatics->CreateString(
        wrl_wrappers::HStringReference(text.c_str(), static_cast<UINT32>(text.size())).Get(),
        &value));

    boolean replaced = false;
    IFC_RETURN(values->Insert(
        wrl_wrappers::HStringReference(valueName.c_str(), static_cast<UINT32>(valueName.size())).Get(),
        value.Get(),
        &replaced));

    return S_OK;
}

_Check_return_ HRESULT RemoveImpl(_In_ const std::wstring& valueName)
{
    ctl::ComPtr<mws::IApplicationDataContainer> container;
    IFC_RETURN(GetPlacementContainer(false /* createIfMissing */, &container));
    if (!container)
    {
        // Never written, so already absent.
        return S_OK;
    }

    ctl::ComPtr<wfc::IMap<HSTRING, IInspectable*>> values;
    IFC_RETURN(GetValues(container.Get(), &values));

    wrl_wrappers::HStringReference key(valueName.c_str(), static_cast<UINT32>(valueName.size()));

    boolean hasKey = false;
    IFC_RETURN(values->HasKey(key.Get(), &hasKey));
    if (!hasKey)
    {
        return S_OK;
    }

    IFC_RETURN(values->Remove(key.Get()));
    return S_OK;
}

} // anonymous namespace

/* static */ bool Store::TryLoad(_In_ const std::wstring& valueName, _Out_ std::wstring& text)
{
    text.clear();

    if (valueName.empty())
    {
        return false;
    }

    const HRESULT hr = LoadImpl(valueName, text);
    if (FAILED(hr))
    {
        TracePlacementStoreFailure("load", hr);
        text.clear();
        return false;
    }

    return hr == S_OK && !text.empty();
}

/* static */ bool Store::TrySave(_In_ const std::wstring& valueName, _In_ const std::wstring& text)
{
    if (valueName.empty() || text.empty())
    {
        return false;
    }

    const HRESULT hr = SaveImpl(valueName, text);
    if (FAILED(hr))
    {
        TracePlacementStoreFailure("save", hr);
        return false;
    }

    return true;
}

/* static */ bool Store::TryRemove(_In_ const std::wstring& valueName)
{
    if (valueName.empty())
    {
        return false;
    }

    const HRESULT hr = RemoveImpl(valueName);
    if (FAILED(hr))
    {
        TracePlacementStoreFailure("remove", hr);
        return false;
    }

    return true;
}

/* static */ bool Store::IsAvailable()
{
    ctl::ComPtr<mws::IApplicationDataContainer> localSettings;
    const HRESULT hr = GetLocalSettings(&localSettings);
    if (FAILED(hr))
    {
        TracePlacementStoreFailure("open", hr);
        return false;
    }

    return localSettings != nullptr;
}

} // namespace WindowPlacementPersistence
