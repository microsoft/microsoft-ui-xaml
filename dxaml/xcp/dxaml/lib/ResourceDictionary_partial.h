// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ResourceDictionary.g.h"

namespace Diagnostics
{
    class DiagnosticsInterop;
}

namespace DirectUI
{
    // TODO: Remove this comment
    class ResourceKeyValuePair:
        public wfc::IKeyValuePair<IInspectable*, IInspectable*>,
        public ctl::WeakReferenceSource
    {
        BEGIN_INTERFACE_MAP(ResourceKeyValuePair, ctl::SupportErrorInfo)
            INTERFACE_ENTRY(ResourceKeyValuePair, wfc::IKeyValuePair<IInspectable * COMMA IInspectable *>)
        END_INTERFACE_MAP(ResourceKeyValuePair, ctl::SupportErrorInfo)

    protected:
        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(wfc::IKeyValuePair<IInspectable*, IInspectable*>)))
            {
                *ppObject = static_cast<wfc::IKeyValuePair<IInspectable*, IInspectable*>*>(this);
            }
            else
            {
                RRETURN(ctl::SupportErrorInfo::QueryInterfaceImpl(iid, ppObject));
            }

            AddRefOuter();
            RRETURN(S_OK);
        }

    public:
        ResourceKeyValuePair()
        {
        }

        ~ResourceKeyValuePair() override
        {
        }

        IFACEMETHODIMP get_Key(_Out_ IInspectable** key) override
        {
            HRESULT hr = S_OK;

            IFCPTR(key);
            *key = m_tpKey.Get();
            AddRefInterface(m_tpKey.Get());

        Cleanup:
            RRETURN(hr);
        }

        IFACEMETHODIMP get_Value(_Out_ IInspectable** value) override
        {
            HRESULT hr = S_OK;

            IFCPTR(value);
            *value = m_tpValue.Get();
            AddRefInterface(m_tpValue.Get());

        Cleanup:
            RRETURN(hr);
        }

        void put_KeyValuePair(
            _In_ IInspectable* const key,
            _In_ IInspectable* const value)
        {
            SetPtrValue(m_tpKey, key);
            SetPtrValue(m_tpValue, value);
        }

    private:
        TrackerPtr<IInspectable> m_tpKey;
        TrackerPtr<IInspectable> m_tpValue;
    };

    PARTIAL_CLASS(ResourceDictionary),
        public wfc::IVector<wfc::IKeyValuePair<IInspectable*, IInspectable*>*>,
        public wfc::IIterable<wfc::IKeyValuePair<IInspectable*, IInspectable*>*>,
        public wfc::IVectorView<wfc::IKeyValuePair<IInspectable*, IInspectable*>*>,
        public wfc::IMapView<IInspectable*, IInspectable*>
    {
        friend class Diagnostics::DiagnosticsInterop;
    public:
        // Constructors/destructors.
        ResourceDictionary();

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(wfc::IVector<wfc::IKeyValuePair<IInspectable*, IInspectable*>*>)))
            {
                *ppObject = static_cast<wfc::IVector<wfc::IKeyValuePair<IInspectable*, IInspectable*>*>*>(this);
            }
            else if (InlineIsEqualGUID(iid, __uuidof(wfc::IIterable<wfc::IKeyValuePair<IInspectable*, IInspectable*>*>)))
            {
                *ppObject = static_cast<wfc::IIterable<wfc::IKeyValuePair<IInspectable*, IInspectable*>*>*>(this);
            }
            else if (InlineIsEqualGUID(iid, __uuidof(wfc::IVectorView<wfc::IKeyValuePair<IInspectable*, IInspectable*>*>)))
            {
                *ppObject = static_cast<wfc::IVectorView<wfc::IKeyValuePair<IInspectable*, IInspectable*>*>*>(this);
            }
            else if (InlineIsEqualGUID(iid, __uuidof(wfc::IMapView<IInspectable*, IInspectable*>)))
            {
                *ppObject = static_cast<wfc::IMapView<IInspectable*, IInspectable*>*>(this);
            }
            else
            {
                return ResourceDictionaryGenerated::QueryInterfaceImpl(iid, ppObject);
            }

            AddRefOuter();
            return S_OK;
        }

    public:
        // IMap<IInspectable*, IInspectable*> members.
        IFACEMETHOD(Lookup)(_In_opt_ IInspectable* key, _Outptr_ IInspectable **ppValue) override;
        IFACEMETHOD(HasKey)(_In_opt_ IInspectable* key, _Out_ boolean *found) override;
        IFACEMETHOD(Insert)(_In_opt_ IInspectable* key, _In_opt_ IInspectable* value, _Out_ boolean *replaced) override;
        IFACEMETHOD(Remove)(_In_opt_ IInspectable* key) override;
        IFACEMETHOD(GetView)(_Outptr_result_maybenull_ wfc::IMapView<IInspectable*, IInspectable*> **view) override;

        // IIterable<T> implementation
        IFACEMETHOD(First)(_Outptr_result_maybenull_ wfc::IIterator<wfc::IKeyValuePair<IInspectable*, IInspectable*>*> **first) override;


        // IVector<IKeyValuePair<IInspectable*, IInspectable*>*> members.
        IFACEMETHOD(get_Size)(_Out_ UINT *size) override;
        IFACEMETHOD(Clear)() override;

        // IIterable<IKeyValuePair<IInspectable*, IInspectable*>*> members.
        IFACEMETHOD(GetAt)(_In_opt_ UINT index, _Out_ wfc::IKeyValuePair<IInspectable*, IInspectable*>** item) override;
        IFACEMETHOD(GetView)(_Outptr_result_maybenull_ wfc::IVectorView<wfc::IKeyValuePair<IInspectable*, IInspectable*>*>** view) override;
        IFACEMETHOD(IndexOf)(_In_opt_ wfc::IKeyValuePair<IInspectable*, IInspectable*>* value, _Out_ UINT *index, _Out_ BOOLEAN *found) override;
        // TODO Bug# 105831
        // Code coverage note: unimplemented method purely for supporting an interface that is exposed via QueryInterface (IVector), but is not officially part of the public API.
        IFACEMETHODIMP SetAt(_In_ UINT index, _In_opt_ wfc::IKeyValuePair<IInspectable*, IInspectable*>* item) override { return E_NOTIMPL; }
        // Code coverage note: unimplemented method purely for supporting an interface that is exposed via QueryInterface (IVector), but is not officially part of the public API.
        IFACEMETHODIMP InsertAt(_In_ UINT index, _In_opt_ wfc::IKeyValuePair<IInspectable*, IInspectable*>* item) override { return E_NOTIMPL; }
        IFACEMETHOD(RemoveAt)(_In_ UINT index) override;
        IFACEMETHOD(Append)(_In_opt_ wfc::IKeyValuePair<IInspectable*, IInspectable*>* item) override;
        IFACEMETHOD(RemoveAtEnd)() override;

        // TODO: Implement IMapView members.
        // Code coverage note: unimplemented method purely for supporting IMapView.
        IFACEMETHODIMP Split(_Outptr_result_maybenull_ wfc::IMapView<IInspectable*,IInspectable*> **firstPartition,_Outptr_result_maybenull_ wfc::IMapView<IInspectable*,IInspectable*> **secondPartition) override { return E_NOTIMPL; }

    public:
        // Tree change notifications
        _Check_return_ HRESULT OnParentUpdated(
            _In_opt_ CDependencyObject* pOldParentCore,
            _In_opt_ CDependencyObject* pNewParentCore,
            _In_ bool isNewParentAlive) override;

        template<typename T>
        _Check_return_ HRESULT TryLookupBoxedValue(_In_ HSTRING resourceName, _Inout_ T* pResourceValue)
        {
            ASSERT(pResourceValue);
            ctl::ComPtr<IInspectable> boxedResourceKey;
            ctl::ComPtr<IInspectable> boxedResource;

            IFC_RETURN(PropertyValue::CreateFromString(resourceName, &boxedResourceKey));
            if (SUCCEEDED(LookupCore(boxedResourceKey.Get(), FALSE /* originateErrorOnLookupFailure */, &boxedResource)))
            {
                IFCEXPECT_RETURN(boxedResource);
                auto resourceAsIReference = ctl::query_interface_cast<wf::IReference<T>>(boxedResource.Get());
                if (resourceAsIReference)
                {
                    IFC_RETURN(resourceAsIReference->get_Value(pResourceValue));
                }
            }

            return S_OK;
        }

        _Check_return_ HRESULT GetItem(
            _In_ const xstring_ptr_view& strKey,
            _In_ bool checkLocalOnly,
            _In_ bool isImplicitStyle,
            _Out_ CValue* pValue);

    private:
        _Check_return_ HRESULT TryGetKeyAsString(
            _In_ IInspectable* pKey,
            _Out_ BOOLEAN* pbIsImplicitStyle,
            _Out_ BOOLEAN* pbIsImplicitDataTemplate,
            _Out_ xstring_ptr* pstrKey);

        _Check_return_ HRESULT GetKeyAt(
            _In_ UINT index,
            _Out_ IInspectable** pKey);

       _Check_return_  HRESULT LookupCore(
            _In_opt_ IInspectable* key,
            _In_ bool originateErrorOnLookupFailure,
            _Outptr_ IInspectable **ppValue);

        _Check_return_ HRESULT TryGetItemCore(
            const xstring_ptr_view& key,
            bool isImplicitStyle,
            _COM_Outptr_ IInspectable **item);

       static _Check_return_ HRESULT GetStrKeyAsInspectable(
           _In_ const xstring_ptr_view& strKey,
           _In_ bool keyIsType,
           _Outptr_ IInspectable** pKey);
    };
}

