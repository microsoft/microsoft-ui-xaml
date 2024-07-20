// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <wil\resource.h>
#include <string>
#include "XamlOM.WinUI.Private.h"
#include <vector>
#include <map>
#include "wilhelper.h"

template<class T>
class CoTaskMemPtr
{
public:
    CoTaskMemPtr() : m_ptr(nullptr) {}
    CoTaskMemPtr(T* ptr) : m_ptr(ptr) {}
    CoTaskMemPtr(const CoTaskMemPtr<T>& other) = delete;
    CoTaskMemPtr(CoTaskMemPtr<T>&& other)
    {
        m_ptr = other.Detach();
    }

    ~CoTaskMemPtr()
    {
        CoTaskMemFree(m_ptr);
    }

    T& operator[](size_t index)
    {
        return m_ptr[index];
    }

    T** operator&()
    {
        return &m_ptr;
    }

    T* Get()
    {
        return m_ptr;
    }

    T* Detach()
    {
        T* ptr = m_ptr;
        m_ptr = nullptr;
        return ptr;
    }

private:
    T* m_ptr;
};

interface __declspec(uuid("{e6755030-f33e-4864-931e-06368e30a84a}"))
    IXamlDiagnosticsTap
    : public IUnknown
{
    STDMETHOD(AdviseVisualTreeChange)(
        _In_ IVisualTreeServiceCallback* pCallback,
        _In_ bool supportMultipleWindow = true) = 0;
    STDMETHOD(UnadviseVisualTreeChange)(
        _In_ IVisualTreeServiceCallback* pCallback) = 0;

    // Uses the main dispatcher
    STDMETHOD(GetEnums)(
        _Out_ unsigned int* pCount,
        _Deref_post_opt_count_(*pCount) EnumType** ppEnums) = 0;

    // Deprecated: These CreateInstance methods will use the main dispatcher. To create
    // an instance on a different dispatcher, use the newer CreateInstance methods
    // that take the dispatcher. Test code should use the newer methods that pass the
    // dispatcher down.
    STDMETHOD(CreateInstance)(
        _In_ const std::wstring& typeName,
        _Out_ InstanceHandle* pInstanceHandle) = 0;

    STDMETHOD(CreateInstance)(
        _In_ const std::wstring& typeName,
        _In_ const std::wstring& value,
        _Out_ InstanceHandle* pInstanceHandle) = 0;

    // Creates the instance on the thread related to the dispatcher passed in
    STDMETHOD_(InstanceHandle, CreateInstance)(
        _In_ const std::wstring& typeName,
        _In_ IInspectable* dispatcher) = 0;

    STDMETHOD_(InstanceHandle, CreateInstance)(
        _In_ const std::wstring& typeName,
        _In_ const std::wstring& value,
        _In_ IInspectable* dispatcher) = 0;

    // The GetProperty* methods will use the dispatcher for the object passed in, if it's a DO.
    // If it isn't a DO, it will use the last dispatcher, assuming that someone is drilling down
    // into the LPE.
    STDMETHOD_(wil::unique_propertychainvalue, GetPropertyChainValue)(
        _In_ InstanceHandle handleToObject,
        _In_ const std::wstring& propertyName,
        _In_ BaseValueSource valueSource = BaseValueSource::BaseValueSourceDefault) = 0;

    STDMETHOD_(wil::unique_propertychainsource, GetPropertyChainSource)(
        _In_ InstanceHandle handleToObject,
        _In_ BaseValueSource valueSource,
        _In_ unsigned int expectedSourceCount = 0) = 0;

    STDMETHOD_(wil::unique_propertychainsource, GetSourceForProperty)(
        _In_ InstanceHandle handleToObject, 
        _In_ const std::wstring& propertyName,
        _In_ BaseValueSource valueSource = BaseValueSourceDefault, 
        _In_ bool overridden = false) = 0;

    STDMETHOD_(std::wstring, ConvertEnumValue)(
        _In_ const std::wstring& enumName,
        _In_ int enumValue) = 0;
    
    // The following collection methods use the the dispatcher for the object passed in, if it's a DO.
    // If it isn't a DO, it will use the last dispatcher, assuming that someone is drilling down
    // into the LPE.
    STDMETHOD(SetProperty)(
        _In_ InstanceHandle instanceHandle,
        _In_ InstanceHandle value,
        _In_ unsigned int propertyIndex) = 0;

    STDMETHOD(ClearProperty)(
        _In_ InstanceHandle instanceHandle,
        _In_ unsigned int propertyIndex) = 0;

    STDMETHOD(GetCollectionCount)(
        _In_ InstanceHandle instanceHandle,
        _Out_ unsigned int* pCollectionSize) = 0;

    STDMETHOD(GetCollectionElements)(
        _In_ InstanceHandle instanceHandle,
        _In_ unsigned int startIndex,
        _Inout_ unsigned int* pElementCount,
        _Deref_post_opt_count_(*pElementCount) CollectionElementValue** ppElementValues) = 0;

    STDMETHOD(AddChild)(
        _In_ InstanceHandle parent,
        _In_ InstanceHandle child,
        _In_ unsigned int index) = 0;

    STDMETHOD(RemoveChild)(
        _In_ InstanceHandle parent,
        _In_ unsigned int index) = 0;

    STDMETHOD(ClearChildren)(
        _In_ InstanceHandle parent) = 0;

    //IVisualTreeService2 Interface
    STDMETHOD(GetPropertyIndex)(
        _In_ InstanceHandle object,
        _In_ const std::wstring& propertyName,
        _Out_ unsigned int* propertyIndex) = 0;

    STDMETHOD(GetProperty)(
        _In_ InstanceHandle object,
        _In_ unsigned int propertyIndex,
        _Out_ InstanceHandle* pValue) = 0;

    STDMETHOD(ReplaceResource)(
        _In_ InstanceHandle resourceDictionary,
        _In_ InstanceHandle key,
        _In_ InstanceHandle newValue) = 0;

    STDMETHOD(RenderTargetBitmap)(
        _In_ InstanceHandle handle,
        _In_ RenderTargetBitmapOptions options,
        _In_ unsigned int maxPixelWidth,
        _In_ unsigned int maxPixelHeight,
        _Outptr_ IBitmapData** ppBitmapData) = 0;

    STDMETHOD(UpdateMainDispatcherQueue)() = 0;


    // IXamlDiagnostics Interface
    // Returns the main dispatcher
    STDMETHOD(GetDispatcher)(
        _Outptr_ IInspectable** ppDispatcher) = 0;

    // Returns the UI layer for the thread called on
    STDMETHOD(GetUiLayer)(
        _Outptr_ IInspectable** ppLayer) = 0;

    STDMETHOD(GetApplication)(
        _Outptr_ IInspectable** ppApplication) = 0;

    // Thread agnostic
    STDMETHOD(GetIInspectableFromHandle)(
        _In_ InstanceHandle instanceHandle,
        _Outptr_ IInspectable** ppInstance) = 0;

    STDMETHOD(GetHandleFromIInspectable)(
        _In_ IInspectable* pInstance,
        _Out_ InstanceHandle* pHandle) = 0;

    // Needs to be called from the correct thread
    STDMETHOD(HitTest)(
        _In_ RECT rect,
        _Out_ unsigned int* pCount,
        _Deref_post_opt_count_(*pCount) InstanceHandle** ppInstanceHandles) = 0;

    // Thread agnostic
    STDMETHOD(RegisterInstance)(
        _In_ IInspectable* pInstance,
        _Out_ InstanceHandle* pInstanceHandle) = 0;

    // IXamlDiagnosticsTestHooks.
    STDMETHOD_(void, UnregisterInstance)(_In_ InstanceHandle handle) = 0;

    STDMETHOD_(void, ResolveResource)(
        _In_ InstanceHandle resourceContext,
        _In_ const std::wstring& resourceKey,
        _In_ unsigned int propertyIndex,
        _In_ ResourceType resourceType) = 0;

    STDMETHOD_(InstanceHandle, GetDictionaryItem)(
        _In_ InstanceHandle resourceContext,
        _In_ const std::wstring& resourceKey) = 0;
                      
    STDMETHOD_(InstanceHandle, GetImplicitStyle)(
        _In_ InstanceHandle resourceContext,
        _In_ const std::wstring& resourceKey) = 0;
        
    STDMETHOD_(void, AddDictionaryItem)(
        _In_ InstanceHandle dictionaryHandle,
        _In_ InstanceHandle item,
        _In_ const std::wstring& resourceKey) = 0;

    STDMETHOD_(void, AddDictionaryItem)(
        _In_ InstanceHandle dictionaryHandle,
        _In_ InstanceHandle implicitStyleHandle) = 0;
        
    STDMETHOD_(void, RemoveDictionaryItem)(
        _In_ InstanceHandle dictionaryHandle,
        _In_ const std::wstring& resourceKey) = 0;

    STDMETHOD_(void, RemoveDictionaryItem)(
        _In_ InstanceHandle dictionaryHandle,
        _In_ InstanceHandle implicitStyleHandle) = 0;
};

HRESULT XamlDiagnosticsTap_CreateInstance(
    _In_ IXamlDiagnostics* xamlDiagnostics,
    _COM_Outptr_ IXamlDiagnosticsTap** ppInstance
    );
