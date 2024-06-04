// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      A default implementation of ICustomProperty, which can be used to describe internal
//      properties on non-DOs.

#pragma once

namespace DirectUI
{
    class __declspec(novtable) CustomProperty:
        public xaml_data::ICustomProperty,
        public ctl::WeakReferenceSource
    {
        typedef std::function<HRESULT (IInspectable*, IInspectable**)> GetValueFunction;

        BEGIN_INTERFACE_MAP(CustomProperty, ctl::WeakReferenceSource)
            INTERFACE_ENTRY(CustomProperty, xaml_data::ICustomProperty)
        END_INTERFACE_MAP(CustomProperty, ctl::WeakReferenceSource)

    public:

        static _Check_return_ HRESULT CreateObjectProperty(
            _In_ HSTRING hName,
            _In_ GetValueFunction getValueFunc,
            _Outptr_ xaml_data::ICustomProperty** ppProperty);

        static _Check_return_ HRESULT CreateInt32Property(
            _In_ HSTRING hName,
            _In_ GetValueFunction getValueFunc,
            _Outptr_ xaml_data::ICustomProperty** ppProperty);

        static _Check_return_ HRESULT CreateBooleanProperty(
            _In_ HSTRING hName,
            _In_ GetValueFunction getValueFunc,
            _Outptr_ xaml_data::ICustomProperty** ppProperty);

        IFACEMETHODIMP get_Name(_Out_ HSTRING* phValue) override
        {
            RRETURN(WindowsDuplicateString(m_hName, phValue));
        }

        IFACEMETHODIMP get_CanRead(_Out_ BOOLEAN* pbValue) override
        {
            *pbValue = TRUE;
            RRETURN(S_OK);
        }

        IFACEMETHODIMP get_CanWrite(_Out_ BOOLEAN* pbValue) override
        {
            *pbValue = FALSE;
            RRETURN(S_OK);
        }

        IFACEMETHODIMP GetValue(_In_ IInspectable* pTarget, _Outptr_ IInspectable** ppResult) override
        {
            RRETURN(m_funcGetValue(pTarget, ppResult));
        }

        IFACEMETHODIMP SetValue(_In_ IInspectable* pTarget, _In_ IInspectable* pValue) override
        {
            RRETURN(E_NOTIMPL);
        }

        IFACEMETHODIMP GetIndexedValue(_In_ IInspectable* pTarget, _In_ IInspectable* pIndex, _Outptr_ IInspectable** ppResult) override
        {
            RRETURN(E_NOTIMPL);
        }

        IFACEMETHODIMP SetIndexedValue(_In_ IInspectable* pTarget, _In_ IInspectable* pValue, _In_ IInspectable* pIndex) override
        {
            RRETURN(E_NOTIMPL);
        }

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(xaml_data::ICustomProperty)))
            {
                *ppObject = static_cast<xaml_data::ICustomProperty*>(this);
            }
            else
            {
                RRETURN(ctl::WeakReferenceSource::QueryInterfaceImpl(iid, ppObject));
            }

            AddRefOuter();
            RRETURN(S_OK);
        }

        // This class is marked novtable, so must not be instantiated directly.
        CustomProperty() = default;
    private:
        HSTRING m_hName{};
        GetValueFunction m_funcGetValue;
    };

    class __declspec(novtable) CustomProperty_Object: public CustomProperty
    {
    public:
        IFACEMETHODIMP get_Type(_Out_ wxaml_interop::TypeName* pValue) override
        {
            HRESULT hr = S_OK;
            wxaml_interop::TypeName typeName = { };

            typeName.Kind = wxaml_interop::TypeKind_Primitive;
            IFC(WindowsCreateString(STR_LEN_PAIR(L"Object"), &typeName.Name));

            *pValue = typeName;

        Cleanup:
            RRETURN(hr);
        }
    protected:
        // This class is marked novtable, so must not be instantiated directly.
        CustomProperty_Object() = default;
    };

    class __declspec(novtable) CustomProperty_Int32: public CustomProperty
    {
    public:
        IFACEMETHODIMP get_Type(_Out_ wxaml_interop::TypeName* pValue) override
        {
            HRESULT hr = S_OK;
            wxaml_interop::TypeName typeName = { };

            typeName.Kind = wxaml_interop::TypeKind_Primitive;
            IFC(WindowsCreateString(STR_LEN_PAIR(L"Int32"), &typeName.Name));

            *pValue = typeName;

        Cleanup:
            RRETURN(hr);
        }

    protected:
        // This class is marked novtable, so must not be instantiated directly.
        CustomProperty_Int32() = default;
    };

    class __declspec(novtable) CustomProperty_Boolean: public CustomProperty
    {
    public:
        IFACEMETHODIMP get_Type(_Out_ wxaml_interop::TypeName* pValue) override
        {
            HRESULT hr = S_OK;
            wxaml_interop::TypeName typeName = { };

            typeName.Kind = wxaml_interop::TypeKind_Primitive;
            IFC(WindowsCreateString(STR_LEN_PAIR(L"Boolean"), &typeName.Name));

            *pValue = typeName;

        Cleanup:
            RRETURN(hr);
        }

    protected:
        // This class is marked novtable, so must not be instantiated directly.
        CustomProperty_Boolean() = default;
    };
}
