// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    class __declspec(uuid("becd0ea2-dfe9-4e84-a7ac-0d88852c8d19")) DependencyPropertyHandle :
        public ctl::SupportErrorInfo,
        public xaml::IDependencyProperty
    {
        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.DependencyProperty");

        BEGIN_INTERFACE_MAP(DependencyPropertyHandle, ctl::SupportErrorInfo)
            INTERFACE_ENTRY(DependencyPropertyHandle, xaml::IDependencyProperty)
        END_INTERFACE_MAP(DependencyPropertyHandle, ctl::SupportErrorInfo)

    public:
        DependencyPropertyHandle() : m_pDP(nullptr)
        {
        }

        _Check_return_ HRESULT Initialize(_In_ const CDependencyProperty* pDP)
        {
            m_pDP = pDP;
            RRETURN(S_OK);
        }

        IFACEMETHOD(GetMetadata)(
            _In_ wxaml_interop::TypeName type,
            _Outptr_ xaml::IPropertyMetadata** ppMetadata) override;

        // Returns a fully initialized DP.
        const CDependencyProperty* GetDP();

    protected:
        _Check_return_ HRESULT QueryInterfaceImpl(
            _In_ REFIID riid,
            _Out_ void** ppObject) override;

        using ctl::ComBase::Initialize;

    private:
        const CDependencyProperty* m_pDP;
    };

    class DependencyPropertyFactory :
        public ctl::AbstractActivationFactory,
        public xaml::IDependencyPropertyStatics
    {
        BEGIN_INTERFACE_MAP(DependencyPropertyFactory, ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(DependencyPropertyFactory, xaml::IDependencyPropertyStatics)
        END_INTERFACE_MAP(DependencyPropertyFactory, ctl::AbstractActivationFactory)

    public:
        IFACEMETHOD(get_UnsetValue)(_Out_ IInspectable** ppValue);

        IFACEMETHOD(Register)(
            _In_ HSTRING hName,
            _In_ wxaml_interop::TypeName propertyType,
            _In_ wxaml_interop::TypeName ownerType,
            _In_opt_ xaml::IPropertyMetadata* pDefaultMetadata,
            _Outptr_ xaml::IDependencyProperty** ppProperty) override
        {
            RRETURN(RegisterStatic(
                hName,
                propertyType,
                ownerType,
                pDefaultMetadata,
                ppProperty));
        }

        IFACEMETHOD(RegisterAttached)(
            _In_ HSTRING hName,
            _In_ wxaml_interop::TypeName propertyType,
            _In_ wxaml_interop::TypeName ownerType,
            _In_opt_ xaml::IPropertyMetadata* pDefaultMetadata,
            _Outptr_ xaml::IDependencyProperty** ppProperty) override
        {
            RRETURN(RegisterAttachedStatic(
                hName,
                propertyType,
                ownerType,
                pDefaultMetadata,
                ppProperty));
        }

        static _Check_return_ HRESULT RegisterStatic(
            _In_ HSTRING hName,
            _In_ wxaml_interop::TypeName propertyType,
            _In_ wxaml_interop::TypeName ownerType,
            _In_opt_ xaml::IPropertyMetadata* pDefaultMetadata,
            _Outptr_ xaml::IDependencyProperty** ppProperty);

        static _Check_return_ HRESULT RegisterAttachedStatic(
            _In_ HSTRING hName,
            _In_ wxaml_interop::TypeName propertyType,
            _In_ wxaml_interop::TypeName ownerType,
            _In_opt_ xaml::IPropertyMetadata* pDefaultMetadata,
            _Outptr_ xaml::IDependencyProperty** ppProperty);

        static _Check_return_ HRESULT GetUnsetValue(
            _Out_ IInspectable** ppValue);

        static _Check_return_ HRESULT IsUnsetValue(
            _In_ IInspectable* pValue,
            _Out_ BOOLEAN& isUnsetValue);

    protected:
        DependencyPropertyFactory() {}

        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Out_ void** ppObject) override;

        _Check_return_ HRESULT CheckActivationAllowed() override;

    private:
        static _Check_return_ HRESULT Register(
            _In_ BOOLEAN bIsAttached,
            _In_ HSTRING hName,
            _In_ wxaml_interop::TypeName propertyType,
            _In_ wxaml_interop::TypeName ownerType,
            _In_opt_ xaml::IPropertyMetadata* pDefaultMetadata,
            _In_ BOOLEAN bIsReadOnly,
            _Outptr_ xaml::IDependencyProperty** ppProperty);
    };
}
