// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// This is only used for internal unit testing - this XamlDiagnosticsTap.dll
// is different from the one loadeed by XamlDiagnostics::Launch, which VS provides.

#include "precomp.h"
#include "XamlDiagnosticsTap.h"
#include "wil\resource.h"
#include "throw.h"
#include "wexstring.h"
#include <memory>
#include <functional>
#include <map>
#include "wrl\event.h"
#include "wexexception.h"
#include "log.h"

using namespace WEX::Common;

#define LOG_ERROR(fmt, ...) WEX::Logging::Log::Error(WEX::Common::String().Format(fmt, __VA_ARGS__))

typedef std::function<HRESULT()> DispatcherMethod;

static bool s_multiWindowMode = false;

class
    XamlDiagnosticsTap :
    public wrl::RuntimeClass<
    wrl::RuntimeClassFlags<wrl::ClassicCom>,
    IXamlDiagnosticsTap,
    wrl::FtmBase>
{
public:

#pragma region Constructors

    XamlDiagnosticsTap();

    HRESULT RuntimeClassInitialize(_In_ IXamlDiagnostics* xamlDiag);

#pragma endregion

    // IVisualTreeService Interface

    STDMETHOD(AdviseVisualTreeChange)(
        _In_ IVisualTreeServiceCallback* pCallback,
        _In_ bool supportMultipleWindow) override;

    STDMETHOD(UnadviseVisualTreeChange)(
        _In_ IVisualTreeServiceCallback* pCallback) override;

    STDMETHOD(GetEnums)(
        _Out_ unsigned int* pCount,
        _Deref_post_opt_count_(*pCount) EnumType** ppEnums) override;

    STDMETHOD(CreateInstance)(
        _In_ const std::wstring& typeName,
        _In_ const std::wstring& value,
        _Out_ InstanceHandle* pInstanceHandle) override;

    STDMETHOD(CreateInstance)(
        _In_ const std::wstring& typeName,
        _Out_ InstanceHandle* pInstanceHandle) override;

    STDMETHOD_(InstanceHandle, CreateInstance)(
        _In_ const std::wstring& typeName,
        _In_ IInspectable* dispatcher) override;

    STDMETHOD_(InstanceHandle, CreateInstance)(
        _In_ const std::wstring& typeName,
        _In_ const std::wstring& value,
        _In_ IInspectable* dispatcher) override;

    STDMETHOD_(wil::unique_propertychainvalue, GetPropertyChainValue)(
        _In_ InstanceHandle handleToObject,
        _In_ const std::wstring& propertyName,
        _In_ BaseValueSource valueSource = BaseValueSource::BaseValueSourceDefault) override;

    STDMETHOD_(wil::unique_propertychainsource, GetPropertyChainSource)(
        _In_ InstanceHandle handleToObject,
        _In_ BaseValueSource valueSource,
        _In_ unsigned int expectedSourceCount = 0) override;

    STDMETHOD_(wil::unique_propertychainsource, GetSourceForProperty)(
        _In_ InstanceHandle handleToObject,
        _In_ const std::wstring& propertyName,
        _In_ BaseValueSource valueSource = BaseValueSourceDefault,
        _In_ bool overridden = false) override;

    STDMETHOD_(std::wstring, ConvertEnumValue)(
        _In_ const std::wstring& enumName,
        _In_ int enumValue) override;

    STDMETHOD(SetProperty)(
        _In_ InstanceHandle instanceHandle,
        _In_ InstanceHandle value,
        _In_ unsigned int propertyIndex) override;

    STDMETHOD(ClearProperty)(
        _In_ InstanceHandle instanceHandle,
        _In_ unsigned int propertyIndex) override;

    STDMETHOD(GetCollectionCount)(
        _In_ InstanceHandle instanceHandle,
        _Out_ unsigned int* pCollectionSize) override;

    STDMETHOD(GetCollectionElements)(
        _In_ InstanceHandle instanceHandle,
        _In_ unsigned int startIndex,
        _Inout_ unsigned int* pElementCount,
        _Deref_post_opt_count_(*pElementCount) CollectionElementValue** ppElementValues) override;

    STDMETHOD(AddChild)(
        _In_ InstanceHandle parent,
        _In_ InstanceHandle child,
        _In_ unsigned int index) override;

    STDMETHOD(RemoveChild)(
        _In_ InstanceHandle parent,
        _In_ unsigned int index) override;

    STDMETHOD(ClearChildren)(
        _In_ InstanceHandle parent) override;

    //IVisualTreeService2 Interface
    STDMETHOD(GetPropertyIndex)(
        _In_ InstanceHandle object,
        _In_ const std::wstring& propertyName,
        _Out_ unsigned int* propertyIndex) override;

    STDMETHOD(GetProperty)(
        _In_ InstanceHandle object,
        _In_ unsigned int propertyIndex,
        _Out_ InstanceHandle* pValue) override;

    STDMETHOD(ReplaceResource)(
        _In_ InstanceHandle resourceDictionary,
        _In_ InstanceHandle key,
        _In_ InstanceHandle newValue) override;

    STDMETHOD(RenderTargetBitmap)(
        _In_ InstanceHandle handle,
        _In_ RenderTargetBitmapOptions options,
        _In_ unsigned int maxPixelWidth,
        _In_ unsigned int maxPixelHeight,
        _Outptr_ IBitmapData** ppBitmapData) override;

    STDMETHOD(UpdateMainDispatcherQueue)() override;

    // IXamlDiagnostics Interface

    STDMETHOD(GetDispatcher)(
        _Outptr_ IInspectable** ppDispatcher) override;

    STDMETHOD(GetUiLayer)(
        _Outptr_ IInspectable** ppLayer) override;

    STDMETHOD(GetApplication)(
        _Outptr_ IInspectable** ppApplication) override;

    STDMETHOD(GetIInspectableFromHandle)(
        _In_ InstanceHandle instanceHandle,
        _Outptr_ IInspectable** ppInstance) override;

    STDMETHOD(GetHandleFromIInspectable)(
        _In_ IInspectable* pInstance,
        _Out_ InstanceHandle* pHandle) override;

    STDMETHOD(HitTest)(
        _In_ RECT rect,
        _Out_ unsigned int* pCount,
        _Deref_post_opt_count_(*pCount) InstanceHandle** ppInstanceHandles) override;

    STDMETHOD(RegisterInstance)(
        _In_ IInspectable* pInstance,
        _Out_ InstanceHandle* pInstanceHandle) override;

    // IXamlDiagnosticsTestHooks Interface
    STDMETHOD_(void, UnregisterInstance)(
        _In_ InstanceHandle propertyHandle) override;

    STDMETHOD_(void, ResolveResource)(
        _In_ InstanceHandle resourceContext,
        _In_ const std::wstring& resourceKey,
        _In_ unsigned int propertyIndex,
        _In_ ResourceType resourceType) override;

    STDMETHOD_(InstanceHandle, GetDictionaryItem)(
        _In_ InstanceHandle resourceContext,
        _In_ const std::wstring& resourceKey) override;

    STDMETHOD_(InstanceHandle, GetImplicitStyle)(
        _In_ InstanceHandle resourceContext,
        _In_ const std::wstring& resourceKey) override;

    STDMETHOD_(void, AddDictionaryItem)(
        _In_ InstanceHandle dictionaryHandle,
        _In_ InstanceHandle item,
        _In_ const std::wstring& resourceKey) override;

    STDMETHOD_(void, AddDictionaryItem)(
        _In_ InstanceHandle dictionaryHandle,
        _In_ InstanceHandle implicitStyleHandle) override;

    STDMETHOD_(void, RemoveDictionaryItem)(
        _In_ InstanceHandle dictionaryHandle,
        _In_ const std::wstring& resourceKey) override;

    STDMETHOD_(void, RemoveDictionaryItem)(
        _In_ InstanceHandle dictionaryHandle,
        _In_ InstanceHandle implicitStyleHandle) override;

private:

    HRESULT RunOnUIThread(
            _In_ msy::IDispatcherQueue* pDispatcherQueue,
            _In_ const DispatcherMethod& function);

    HRESULT GetPropertyValuesChainImpl(
        _In_ InstanceHandle object,
        _Out_ CoTaskMemPtr<wil::unique_propertychainsource>& sources,
        _Out_ unsigned int* sourceCount,
        _Out_ CoTaskMemPtr<wil::unique_propertychainvalue>& values,
        _Out_ unsigned int* valueCount
    );

    std::wstring BaseValueSourceToString(
        _In_ BaseValueSource source);

    void GetCurrentDispatcherQueue(_In_ InstanceHandle object);

    InstanceHandle CreateInstanceImpl(
        _In_ const std::wstring& typeName,
        _In_ msy::IDispatcherQueue* dispatcherQueue);

    InstanceHandle CreateInstanceImpl(
        _In_ const std::wstring& typeName,
        _In_ const std::wstring& value,
        _In_ msy::IDispatcherQueue* dispatcherQueue);

    wrl::ComPtr<IXamlDiagnostics> m_xamlDiagnostics;
    wrl::ComPtr<IVisualTreeService3> m_visualTreeService;

    DWORD m_mainDispatcherThreadId = 0;
    wrl::ComPtr<msy::IDispatcherQueue> m_mainDispatcherQueue;
    wrl::ComPtr<msy::IDispatcherQueue> m_currentDispatcherQueue;
};

#pragma region Constructors
HRESULT
XamlDiagnosticsTap_CreateInstance(
    _In_ IXamlDiagnostics* xamlDiagnostics,
    _COM_Outptr_ IXamlDiagnosticsTap** ppInstance
    ) try
{
    wrl::ComPtr<IXamlDiagnosticsTap> tap;
    Throw::IfFailed(wrl::MakeAndInitialize<XamlDiagnosticsTap>(&tap, xamlDiagnostics));
    *ppInstance = tap.Detach();
    return S_OK;
}
CATCH_RETURN()

HRESULT
XamlDiagnosticsTap::RuntimeClassInitialize(
    _In_ IXamlDiagnostics * xamlDiag) try
{
    m_xamlDiagnostics = xamlDiag;
    Throw::IfFailed(m_xamlDiagnostics.As(&m_visualTreeService));
    Throw::IfFailed(UpdateMainDispatcherQueue());

    return S_OK;
}
CATCH_RETURN()

XamlDiagnosticsTap::XamlDiagnosticsTap()
{
}

HRESULT XamlDiagnosticsTap::UpdateMainDispatcherQueue()
{
    HRESULT hr = S_OK;
    m_mainDispatcherThreadId = GetCurrentThreadId();
    wrl::ComPtr<msy::IDispatcherQueueStatics> queueStatics;
    hr = wf::GetActivationFactory(
        wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Dispatching_DispatcherQueue).Get(),
        &queueStatics);
    if (SUCCEEDED(hr))
    {
        hr = queueStatics->GetForCurrentThread(&m_mainDispatcherQueue);
    }

    return hr;
}
#pragma region One-to-one wrappers

STDMETHODIMP
XamlDiagnosticsTap::AdviseVisualTreeChange(
    _In_ IVisualTreeServiceCallback* pCallback,
    _In_ bool supportMultipleWindow)
{
    s_multiWindowMode = supportMultipleWindow;
    return m_visualTreeService->AdviseVisualTreeChange(pCallback);
}

STDMETHODIMP
XamlDiagnosticsTap::UnadviseVisualTreeChange(
    _In_ IVisualTreeServiceCallback* pCallback)
{
    return m_visualTreeService->UnadviseVisualTreeChange(pCallback);
}

STDMETHODIMP
XamlDiagnosticsTap::GetEnums(
    _Out_ unsigned int* pCount,
    _Deref_post_opt_count_(*pCount) EnumType** ppEnums)
{
    return RunOnUIThread(m_mainDispatcherQueue.Get(), [&]() {
        return m_visualTreeService->GetEnums(pCount, ppEnums);
    });
}

STDMETHODIMP
XamlDiagnosticsTap::CreateInstance(
    _In_ const std::wstring& typeName,
    _In_ const std::wstring& value,
    _Out_ InstanceHandle* pInstanceHandle)
{
    try
    {
        *pInstanceHandle = CreateInstanceImpl(typeName, value, m_mainDispatcherQueue.Get());
    }
    catch (const WEX::Common::Exception& ex)
    {
        return ex.ErrorCode();
    }

    return S_OK;
}

STDMETHODIMP
XamlDiagnosticsTap::CreateInstance(
    _In_ const std::wstring& typeName,
    _Out_ InstanceHandle* pInstanceHandle)
{
    try
    {
        *pInstanceHandle = CreateInstanceImpl(typeName, m_mainDispatcherQueue.Get());
    }
    catch (const WEX::Common::Exception& ex)
    {
        return ex.ErrorCode();
    }

    return S_OK;
}

STDMETHODIMP_(InstanceHandle)
XamlDiagnosticsTap::CreateInstance(
    _In_ const std::wstring& typeName,
    _In_ IInspectable* dispatcherQueueAsInsp)
{
    Throw::IfNull(dispatcherQueueAsInsp);
    wrl::ComPtr<msy::IDispatcherQueue> dispatcherQueue;
    Throw::IfFailed(dispatcherQueueAsInsp->QueryInterface<msy::IDispatcherQueue>(&dispatcherQueue));
    return CreateInstanceImpl(typeName, dispatcherQueue.Get());
}

STDMETHODIMP_(InstanceHandle)
XamlDiagnosticsTap::CreateInstance(
    _In_ const std::wstring& typeName,
    _In_ const std::wstring& value,
    _In_ IInspectable* dispatcherQueueAsInsp)
{
    Throw::IfNull(dispatcherQueueAsInsp);
    wrl::ComPtr<msy::IDispatcherQueue> dispatcherQueue;
    Throw::IfFailed(dispatcherQueueAsInsp->QueryInterface<msy::IDispatcherQueue>(&dispatcherQueue));
    return CreateInstanceImpl(typeName, value, dispatcherQueue.Get());
}

InstanceHandle
XamlDiagnosticsTap::CreateInstanceImpl(
    _In_ const std::wstring& typeName,
    _In_ msy::IDispatcherQueue* dispatcherQueue)
{
    bstr_t type(typeName.c_str());

    InstanceHandle handle = 0;
    Throw::IfFailed(RunOnUIThread(dispatcherQueue, [&]() {
        return m_visualTreeService->CreateInstance(type, nullptr, &handle);
    }), WEX::Common::String().Format(L"Failed to create type %s", typeName.c_str()));

    return handle;
}

InstanceHandle
XamlDiagnosticsTap::CreateInstanceImpl(
    _In_ const std::wstring& typeName,
    _In_ const std::wstring& value,
    _In_ msy::IDispatcherQueue* dispatcherQueue)
{
    bstr_t type(typeName.c_str());
    bstr_t val(value.c_str());

    InstanceHandle handle = 0;
    Throw::IfFailed(RunOnUIThread(dispatcherQueue, [&]() {
        return m_visualTreeService->CreateInstance(type, val, &handle);
    }), WEX::Common::String().Format(L"Failed to create type %s with value %s", typeName.c_str(), value.c_str()));

    return handle;
}

STDMETHODIMP_(wil::unique_propertychainvalue)
XamlDiagnosticsTap::GetPropertyChainValue(
    _In_ InstanceHandle handleToObject,
    _In_ const std::wstring& propertyName,
    _In_ BaseValueSource valueSource)
{
    unsigned int chainSourceCount = 0;
    CoTaskMemPtr<wil::unique_propertychainsource> spChainSources;
    unsigned int chainPropertyCount = 0;
    CoTaskMemPtr<wil::unique_propertychainvalue> spChainValues;

    Throw::IfFailed(GetPropertyValuesChainImpl(handleToObject, spChainSources, &chainSourceCount, spChainValues, &chainPropertyCount),
        WEX::Common::String(L"Failed to get ").Append(propertyName.c_str()));


    for (size_t k = 0; k < chainPropertyCount; k++)
    {
        if (propertyName == spChainValues[k].PropertyName &&
            spChainSources[spChainValues[k].PropertyChainIndex].Source == valueSource)
        {
            // set the return value and success but don't break, we need to release
            // the rest of the resources
            wil::unique_propertychainvalue returnValue = std::move(spChainValues[k]);
            return returnValue;
        }
    }

    WEX::Common::Throw::Exception(E_NOTFOUND, WEX::Common::String().Format(L"Property %s with BaseValueSource %s was not found", propertyName.c_str(), BaseValueSourceToString(valueSource).c_str()));
    return wil::unique_propertychainvalue();
}

STDMETHODIMP_(wil::unique_propertychainsource)
XamlDiagnosticsTap::GetPropertyChainSource(
    _In_ InstanceHandle handleToObject,
    _In_ BaseValueSource valueSource,
    _In_ unsigned int expectedSourceCount)
{
    unsigned int chainSourceCount = 0;
    CoTaskMemPtr<wil::unique_propertychainsource> spChainSources;
    unsigned int chainPropertyCount = 0;
    CoTaskMemPtr<wil::unique_propertychainvalue> spChainValues;

    Throw::IfFailed(GetPropertyValuesChainImpl(handleToObject, spChainSources, &chainSourceCount, spChainValues, &chainPropertyCount),
        L"Failed getting property chain to evaluate sources");

    WEX::Common::Throw::If(expectedSourceCount != 0 && expectedSourceCount != chainSourceCount, E_UNEXPECTED, L"Unexpected number of sources");

    for (size_t k = 0; k < chainSourceCount; k++)
    {
        if (spChainSources[k].Source == valueSource)
        {
            wil::unique_propertychainsource returnSource = std::move(spChainSources[k]);
            return returnSource;
        }
    }

    Throw::IfFailed(E_NOTFOUND, WEX::Common::String().Format(L"%s not found", BaseValueSourceToString(valueSource).c_str()));
    return wil::unique_propertychainsource();
}

STDMETHODIMP_(wil::unique_propertychainsource)
XamlDiagnosticsTap::GetSourceForProperty(
    _In_ InstanceHandle handleToObject,
    _In_ const std::wstring& propertyName,
    _In_ BaseValueSource valueSource,
    _In_ bool overridden )
{
    unsigned int chainSourceCount = 0;
    CoTaskMemPtr<wil::unique_propertychainsource> spChainSources;
    unsigned int chainPropertyCount = 0;
    CoTaskMemPtr<wil::unique_propertychainvalue> spChainValues;

    Throw::IfFailed(GetPropertyValuesChainImpl(handleToObject, spChainSources, &chainSourceCount, spChainValues, &chainPropertyCount),
        L"Failed getting property chain to evaluate sources");

    for (unsigned int k = 0; k < chainPropertyCount; k++)
    {
        if (propertyName == spChainValues[k].PropertyName &&
            spChainSources[spChainValues[k].PropertyChainIndex].Source == valueSource)
        {
            if (overridden && !spChainValues[k].Overridden)
            {
                continue;
            }
            else
            {
                wil::unique_propertychainsource returnSource = std::move(spChainSources[spChainValues[k].PropertyChainIndex]);
                return returnSource;
            }
        }
    }

    WEX::Common::Throw::Exception(E_NOTFOUND, WEX::Common::String().Format(L"%s not found for %s", BaseValueSourceToString(valueSource).c_str(), propertyName.c_str()));
    return wil::unique_propertychainsource();
}

HRESULT XamlDiagnosticsTap::GetPropertyValuesChainImpl(
    _In_ InstanceHandle handleToObject,
    _Out_ CoTaskMemPtr<wil::unique_propertychainsource>& sources,
    _Out_ unsigned int* sourceCount,
    _Out_ CoTaskMemPtr<wil::unique_propertychainvalue>& values,
    _Out_ unsigned int* valueCount
)
{
    GetCurrentDispatcherQueue(handleToObject);

    if (m_currentDispatcherQueue == nullptr)
    {
        LOG_ERROR(L"Current DispatcherQueue is null");
        return E_FAIL;
    }

    return RunOnUIThread(m_currentDispatcherQueue.Get(), [&]() {
        // This reinterpret cast is ok because with wil::unique_propertychainvalue/source objects have the same binary layout as PropertyChainValue/Source
        return m_visualTreeService->GetPropertyValuesChain(handleToObject, sourceCount,
            reinterpret_cast<PropertyChainSource**>(&sources), valueCount, reinterpret_cast<PropertyChainValue**>(&values));
    });
}

STDMETHODIMP_(std::wstring)
XamlDiagnosticsTap::ConvertEnumValue(
    _In_ const std::wstring& enumName,
    _In_ int enumValue)
{
    unsigned int enumCount = 0;
    CoTaskMemPtr<EnumType> spEnums;

    auto cleanupArray = wil::scope_exit([&enumCount, &spEnums]()
    {
        for (size_t k = 0; k < enumCount; k++)
        {
            SafeArrayDestroy(spEnums[k].ValueInts);
            SafeArrayDestroy(spEnums[k].ValueStrings);
        }
    });

    Throw::IfFailed(GetEnums(&enumCount, &spEnums));
    WEX::Common::Throw::If(enumCount == 0u, E_UNEXPECTED, L"Unexpected number of enums returned from XAML runtime");

    for (size_t k = 0; k < enumCount; k++)
    {
        // When we found the enum we want
        if (enumName == spEnums[k].Name)
        {
            LONG enumValAsLong = static_cast<LONG>(enumValue);

            // Let's allocate a string of length 20, I can't imagine needing anything larger but this can always be changed.
            auto sizeOfString = SafeArrayGetElemsize(spEnums[k].ValueStrings);
            wil::unique_bstr returnString(SysAllocStringByteLen(nullptr, sizeOfString));
            Throw::IfFailed(SafeArrayGetElement(spEnums[k].ValueStrings, &enumValAsLong, returnString.addressof()));
            return std::wstring(returnString.get());
        }
    }
    WEX::Common::Throw::Exception(E_NOTFOUND, WEX::Common::String().Format(L"Enum %s does not have a value that matches %d",enumName.c_str(), enumValue));
    return std::wstring();
}

STDMETHODIMP
XamlDiagnosticsTap::SetProperty(
    _In_ InstanceHandle instanceHandle,
    _In_ InstanceHandle value,
    _In_ unsigned int propertyIndex)
{
    GetCurrentDispatcherQueue(instanceHandle);

    if (m_currentDispatcherQueue == nullptr)
    {
        LOG_ERROR(L"Current DispatcherQueue is null");
        return E_FAIL;
    }

    return RunOnUIThread(m_currentDispatcherQueue.Get(), [&]() {
        return m_visualTreeService->SetProperty(instanceHandle, value, propertyIndex);
    });
}

STDMETHODIMP
XamlDiagnosticsTap::ClearProperty(
    _In_ InstanceHandle instanceHandle,
    _In_ unsigned int propertyIndex)
{
    GetCurrentDispatcherQueue(instanceHandle);

    if (m_currentDispatcherQueue == nullptr)
    {
        LOG_ERROR(L"Current DispatcherQueue is null");
        return E_FAIL;
    }

    return RunOnUIThread(m_currentDispatcherQueue.Get(), [&]() {
        return m_visualTreeService->ClearProperty(instanceHandle, propertyIndex);
    });
}

STDMETHODIMP
XamlDiagnosticsTap::GetCollectionCount(
    _In_ InstanceHandle instanceHandle,
    _Out_ unsigned int* pCollectionSize)
{
    GetCurrentDispatcherQueue(instanceHandle);

    if (m_currentDispatcherQueue == nullptr)
    {
        LOG_ERROR(L"Current DispatcherQueue is null");
        return E_FAIL;
    }

    return RunOnUIThread(m_currentDispatcherQueue.Get(), [&]() {
        return m_visualTreeService->GetCollectionCount(instanceHandle, pCollectionSize);
    });
}

STDMETHODIMP
XamlDiagnosticsTap::GetCollectionElements(
    _In_ InstanceHandle instanceHandle,
    _In_ unsigned int startIndex,
    _Inout_ unsigned int* pElementCount,
    _Deref_post_opt_count_(*pElementCount) CollectionElementValue** ppElementValues)
{
    GetCurrentDispatcherQueue(instanceHandle);

    if (m_currentDispatcherQueue == nullptr)
    {
        LOG_ERROR(L"Current DispatcherQueue is null");
        return E_FAIL;
    }

    return RunOnUIThread(m_currentDispatcherQueue.Get(), [&]() {
        return m_visualTreeService->GetCollectionElements(instanceHandle, startIndex, pElementCount, ppElementValues);
    });
}

STDMETHODIMP
XamlDiagnosticsTap::AddChild(
    _In_ InstanceHandle parent,
    _In_ InstanceHandle child,
    _In_ unsigned int index)
{
    GetCurrentDispatcherQueue(parent);

    if (m_currentDispatcherQueue == nullptr)
    {
        LOG_ERROR(L"Current DispatcherQueue is null");
        return E_FAIL;
    }

    return RunOnUIThread(m_currentDispatcherQueue.Get(), [&]() {
        return m_visualTreeService->AddChild(parent, child, index);
    });
}

STDMETHODIMP
XamlDiagnosticsTap::RemoveChild(
    _In_ InstanceHandle parent,
    _In_ unsigned int index)
{
    GetCurrentDispatcherQueue(parent);

    if (m_currentDispatcherQueue == nullptr)
    {
        LOG_ERROR(L"Current DispatcherQueue is null");
        return E_FAIL;
    }

    return RunOnUIThread(m_currentDispatcherQueue.Get(), [&]() {
        return m_visualTreeService->RemoveChild(parent, index);
    });
}

STDMETHODIMP
XamlDiagnosticsTap::ClearChildren(
    _In_ InstanceHandle parent)
{
    GetCurrentDispatcherQueue(parent);

    if (m_currentDispatcherQueue == nullptr)
    {
        LOG_ERROR(L"Current DispatcherQueue is null");
        return E_FAIL;
    }

    return RunOnUIThread(m_currentDispatcherQueue.Get(), [&]() {
        return m_visualTreeService->ClearChildren(parent);
    });
}

STDMETHODIMP
XamlDiagnosticsTap::GetPropertyIndex(
    _In_ InstanceHandle object,
    _In_ const std::wstring& propertyName,
    _Out_ unsigned int* pPropertyIndex)
{
    GetCurrentDispatcherQueue(object);

    if (m_currentDispatcherQueue == nullptr)
    {
        LOG_ERROR(L"Current DispatcherQueue is null");
        return E_FAIL;
    }

    return RunOnUIThread(m_currentDispatcherQueue.Get(), [&]() {
        return m_visualTreeService->GetPropertyIndex(object, propertyName.c_str(), pPropertyIndex);
    });
}

STDMETHODIMP
XamlDiagnosticsTap::GetProperty(
    _In_ InstanceHandle object,
    _In_ unsigned int propertyIndex,
    _Out_ InstanceHandle* pValue)
{
    GetCurrentDispatcherQueue(object);

    if (m_currentDispatcherQueue == nullptr)
    {
        LOG_ERROR(L"Current DispatcherQueue is null");
        return E_FAIL;
    }

    return RunOnUIThread(m_currentDispatcherQueue.Get(), [&]() {
        return m_visualTreeService->GetProperty(object, propertyIndex, pValue);
    });
}

STDMETHODIMP
XamlDiagnosticsTap::ReplaceResource(
    _In_ InstanceHandle resourceDictionary,
    _In_ InstanceHandle key,
    _In_ InstanceHandle newValue)
{
    GetCurrentDispatcherQueue(resourceDictionary);

    if (m_currentDispatcherQueue == nullptr)
    {
        LOG_ERROR(L"Current DispatcherQueue is null");
        return E_FAIL;
    }

    return RunOnUIThread(m_currentDispatcherQueue.Get(), [&]() {
        return m_visualTreeService->ReplaceResource(resourceDictionary, key, newValue);
    });
}

STDMETHODIMP
XamlDiagnosticsTap::RenderTargetBitmap(
    _In_ InstanceHandle handle,
    _In_ RenderTargetBitmapOptions options,
    _In_ unsigned int maxPixelWidth,
    _In_ unsigned int maxPixelHeight,
    _Outptr_ IBitmapData** ppBitmapData)
{
    GetCurrentDispatcherQueue(handle);

    if (m_currentDispatcherQueue == nullptr)
    {
        LOG_ERROR(L"Current DispatcherQueue is null");
        return E_FAIL;
    }

    return RunOnUIThread(m_currentDispatcherQueue.Get(), [&]() {
        return m_visualTreeService->RenderTargetBitmap(handle, options, maxPixelWidth, maxPixelHeight, ppBitmapData);
    });
}

STDMETHODIMP
XamlDiagnosticsTap::GetDispatcher(
    _Outptr_ IInspectable** ppDispatcher)
{
    return m_xamlDiagnostics->GetDispatcher(ppDispatcher);
}

STDMETHODIMP
XamlDiagnosticsTap::GetUiLayer(
    _Outptr_ IInspectable** ppLayer)
{
    // TODO: Add a test that ensures we can get different UI layers for other UI thread's.
    return RunOnUIThread(m_mainDispatcherQueue.Get(), [&]() {
        return m_xamlDiagnostics->GetUiLayer(ppLayer);
    });
}

STDMETHODIMP
XamlDiagnosticsTap::GetApplication(
    _Outptr_ IInspectable** ppApplication)
{
    return RunOnUIThread(m_mainDispatcherQueue.Get(), [&]() {
        return m_xamlDiagnostics->GetApplication(ppApplication);
    });
}

STDMETHODIMP
XamlDiagnosticsTap::GetIInspectableFromHandle(
    _In_ InstanceHandle instanceHandle,
    _Outptr_ IInspectable** ppInstance)
{
    return m_xamlDiagnostics->GetIInspectableFromHandle(instanceHandle, ppInstance);
}

STDMETHODIMP
XamlDiagnosticsTap::GetHandleFromIInspectable(
    _In_ IInspectable* pInstance,
    _Out_ InstanceHandle* pHandle)
{
    return m_xamlDiagnostics->GetHandleFromIInspectable(pInstance, pHandle);
}

STDMETHODIMP
XamlDiagnosticsTap::HitTest(
    _In_ RECT rect,
    _Inout_ unsigned int* pCount,
    _Deref_post_opt_count_(*pCount) InstanceHandle** ppInstanceHandles)
{
    // TODO: Add a test that ensures we can hit test other UI thread's.
    return RunOnUIThread(m_mainDispatcherQueue.Get(), [&]() {
        return m_xamlDiagnostics->HitTest(rect, pCount, ppInstanceHandles);
    });
}

STDMETHODIMP
XamlDiagnosticsTap::RegisterInstance(
    _In_ IInspectable* pInstance,
    _Out_ InstanceHandle* pInstanceHandle)
{
    return m_xamlDiagnostics->RegisterInstance(pInstance, pInstanceHandle);
}

STDMETHODIMP_(void)
XamlDiagnosticsTap::UnregisterInstance(
    _In_ InstanceHandle handle)
{
     wrl::ComPtr<IXamlDiagnosticsTestHooks> testHooks;
    // Previous versions of xaml diagnostics don't have this interface
    if (SUCCEEDED(m_xamlDiagnostics.As(&testHooks)))
    {
        Throw::IfFailed(testHooks->UnregisterInstance(handle));
    }
}

std::wstring
XamlDiagnosticsTap::BaseValueSourceToString(
    _In_ BaseValueSource source)
{
    // Only worry about converting the few that we really use today
    switch (source)
    {
    case BaseValueSourceDefault:
            return L"BaseValueSourceDefault";
    case BaseValueSourceBuiltInStyle:
        return L"BaseValueSourceBuiltInStyle";
    case BaseValueSourceStyle:
        return L"BaseValueSourceStyle";
    case BaseValueSourceLocal:
        return L"BaseValueSourceLocal";
    case Animation:
    case BaseValueSourceVisualState:
        return L"BaseValueSource::Animation";
    case BaseValueSourceUnknown:
    default:
        return L"BaseValueSourceUnknown";
    }
}

template<typename TDelegateInterface, typename TCallback>
wrl::ComPtr<TDelegateInterface> MakeAgileCallback(TCallback callback) noexcept
{
    return wrl::Callback<wrl::Implements<wrl::RuntimeClassFlags<::Microsoft::WRL::ClassicCom>, TDelegateInterface, wrl::FtmBase>>(callback);
}

HRESULT XamlDiagnosticsTap::RunOnUIThread(
    _In_ msy::IDispatcherQueue* pDispatcherQueue,
    _In_ const DispatcherMethod& function)
{
    if (m_mainDispatcherThreadId == ::GetCurrentThreadId())
    {
        return (function());
    }

    boolean enqueued;
    HRESULT hr = S_OK;
    HRESULT innerHr = S_OK;

    wil::unique_handle hCompletedEvent(CreateEvent(
        nullptr /* lpEventAttributes */,
        TRUE /* bManualReset */,
        FALSE /* bInitialState */,
        nullptr /* lpName */));

    hr = pDispatcherQueue->TryEnqueue(
        MakeAgileCallback<msy::IDispatcherQueueHandler>([&]() mutable -> HRESULT
    {
        innerHr = function();
        return !!SetEvent(hCompletedEvent.get()) ? S_OK : E_FAIL;
     }).Get(), &enqueued);

    if (SUCCEEDED(hr))
    {
        if (WaitForSingleObject(hCompletedEvent.get(), INFINITE /* dwMilliseconds */) == WAIT_OBJECT_0)
        {
            return innerHr;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    return hr;
}

void XamlDiagnosticsTap::GetCurrentDispatcherQueue(_In_ InstanceHandle object)
{
    wrl::ComPtr<IXamlDiagnosticsTestHooks> testHooks;
    // Previous versions of xaml diagnostics don't have this interface
    if (SUCCEEDED(m_xamlDiagnostics.As(&testHooks)))
    {
        Throw::IfFailed(testHooks->TryGetDispatcherQueueForObject(object, &m_currentDispatcherQueue));
    }

    if (m_currentDispatcherQueue == nullptr) // fallback to main dispatcher queue
    {
        m_currentDispatcherQueue = m_mainDispatcherQueue;
    }
}

STDMETHODIMP_(void)
XamlDiagnosticsTap::ResolveResource(
    _In_ InstanceHandle resourceContext,
    _In_ const std::wstring& resourceName,
    _In_ unsigned int propertyIndex,
    _In_ ResourceType resourceType)
{
    GetCurrentDispatcherQueue(resourceContext);

    Throw::IfFailed(RunOnUIThread(m_currentDispatcherQueue.Get(), [&]() {
        return m_visualTreeService->ResolveResource(resourceContext, resourceName.c_str(), resourceType, propertyIndex);
    }), WEX::Common::String().Format(L"Failed to resolve resource %s", resourceName.c_str()));

}

STDMETHODIMP_(InstanceHandle)
XamlDiagnosticsTap::GetDictionaryItem(
    _In_ InstanceHandle resourceDictionary,
    _In_ const std::wstring& resourceName)
{
    InstanceHandle handle = 0;

    GetCurrentDispatcherQueue(resourceDictionary);

    Throw::IfFailed(RunOnUIThread(m_currentDispatcherQueue.Get(), [&]() {
        return m_visualTreeService->GetDictionaryItem(resourceDictionary, resourceName.c_str(), FALSE, &handle);
    }), WEX::Common::String().Format(L"Failed to get resource %s", resourceName.c_str()));

    return handle;
}


STDMETHODIMP_(InstanceHandle)
XamlDiagnosticsTap::GetImplicitStyle(
    _In_ InstanceHandle resourceDictionary,
    _In_ const std::wstring& resourceKey)
{
   InstanceHandle handle = 0;
   GetCurrentDispatcherQueue(resourceDictionary);

   Throw::IfFailed(RunOnUIThread(m_currentDispatcherQueue.Get(), [&]() {
       return m_visualTreeService->GetDictionaryItem(resourceDictionary, resourceKey.c_str(), TRUE, &handle);
    }), WEX::Common::String().Format(L"Failed to get resource %s", resourceKey.c_str()));

    return handle;
}

STDMETHODIMP_(void)
XamlDiagnosticsTap::AddDictionaryItem(
    _In_ InstanceHandle dictionaryHandle,
    _In_ InstanceHandle item,
    _In_ const std::wstring& resourceKey)
{
    GetCurrentDispatcherQueue(dictionaryHandle);

    InstanceHandle keyHandle = CreateInstanceImpl(L"Windows.Foundation.String", resourceKey, m_currentDispatcherQueue.Get());

    Throw::IfFailed(RunOnUIThread(m_currentDispatcherQueue.Get(), [&]() {
        return m_visualTreeService->AddDictionaryItem(dictionaryHandle, keyHandle, item);
    }), WEX::Common::String().Format(L"Failed to add resource %s", resourceKey.c_str()));
}

STDMETHODIMP_(void)
XamlDiagnosticsTap::AddDictionaryItem(
    _In_ InstanceHandle dictionaryHandle,
    _In_ InstanceHandle implicitStyleKey)
{
    GetCurrentDispatcherQueue(dictionaryHandle);

    Throw::IfFailed(RunOnUIThread(m_currentDispatcherQueue.Get(), [&]() {
        return m_visualTreeService->AddDictionaryItem(dictionaryHandle, implicitStyleKey, implicitStyleKey);
    }), WEX::Common::String().Format(L"Failed to add implicit style resource"));
}

STDMETHODIMP_(void)
XamlDiagnosticsTap::RemoveDictionaryItem(
    _In_ InstanceHandle dictionaryHandle,
    _In_ const std::wstring& resourceKey)
{
    GetCurrentDispatcherQueue(dictionaryHandle);

    InstanceHandle keyHandle = CreateInstanceImpl(L"Windows.Foundation.String", resourceKey, m_currentDispatcherQueue.Get());

    Throw::IfFailed(RunOnUIThread(m_currentDispatcherQueue.Get(), [&]() {
        return m_visualTreeService->RemoveDictionaryItem(dictionaryHandle, keyHandle);
    }), WEX::Common::String().Format(L"Failed to remove resource %s", resourceKey.c_str()));
}


STDMETHODIMP_(void)
XamlDiagnosticsTap::RemoveDictionaryItem(
    _In_ InstanceHandle dictionaryHandle,
    _In_ InstanceHandle implicitStyleHandle)
{
    GetCurrentDispatcherQueue(dictionaryHandle);

    Throw::IfFailed(RunOnUIThread(m_currentDispatcherQueue.Get(), [&]() {
        return m_visualTreeService->RemoveDictionaryItem(dictionaryHandle, implicitStyleHandle);
    }), WEX::Common::String().Format(L"Failed to remove implicit style resource"));
}
#pragma endregion

