// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines the basic class that specifies all of the parameters
//      that compose a binding beteween a DependencyObject and a source.
//      The manually written override will ensure that once the Binding
//      is used in a binding expression that it will be immutable.

#pragma once

#include "Binding.g.h"

namespace DirectUI
{
    class PropertyPathParser;
    
    PARTIAL_CLASS(Binding)
    {
    protected:

        Binding(): 
            m_fIsFrozen(FALSE),
            m_fHasConverter(FALSE),
            m_fHasExplicitMode(FALSE),
            m_pPropertyPathParser(NULL)
        { }

        ~Binding() override;

    public:

        // MarkupExtension implementation.
        _Check_return_ HRESULT ProvideValue(_In_ xaml::IXamlServiceProvider* pServiceProvider, _Outptr_ IInspectable** ppValue, _Out_ KnownTypeIndex* peTypeIndex) override
        {
            UNREFERENCED_PARAMETER(pServiceProvider);

            *ppValue = ctl::iinspectable_cast(this);
            AddRefOuter();
            *peTypeIndex = KnownTypeIndex::Binding;
            RRETURN(S_OK);
        }

        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        IFACEMETHOD(get_Converter)(_Outptr_ xaml_data::IValueConverter** pValue) override;
        IFACEMETHOD(get_Mode)(_Out_ xaml_data::BindingMode* pValue) override;

        // Overrides for the property setters
        IFACEMETHOD(put_Mode)(_In_ xaml_data::BindingMode value) override;
        IFACEMETHOD(put_Source)(_In_ IInspectable* value) override;
        IFACEMETHOD(put_RelativeSource)(_In_ xaml_data::IRelativeSource* value) override;
        IFACEMETHOD(put_ElementName)(_In_ HSTRING value) override;
        IFACEMETHOD(put_Converter)(_In_ xaml_data::IValueConverter* value) override;
        IFACEMETHOD(put_ConverterParameter)(_In_ IInspectable* value) override;
        IFACEMETHOD(put_ConverterLanguage)(_In_ HSTRING value) override;
        IFACEMETHOD(put_FallbackValue)(_In_ IInspectable* value) override;
        IFACEMETHOD(put_TargetNullValue)(_In_ IInspectable* value) override;
        IFACEMETHOD(put_UpdateSourceTrigger)(_In_ xaml_data::UpdateSourceTrigger value) override;

        // Internal method to get to the property path object
        _Check_return_ HRESULT GetPropertyPathParser(_Outptr_ PropertyPathParser **ppPropertyPath);

        _Check_return_ HRESULT EnsurePropertyPathParser(_In_opt_ XamlServiceProviderContext* context);
        _Check_return_ HRESULT UpdatePropertyPathParser();

        // Internal methods to get the path strings
        _Check_return_ HRESULT GetPathString(_Out_ HSTRING *phPath);
        _Check_return_ HRESULT SetPathString(_In_ HSTRING hPath);

        static _Check_return_ HRESULT SetPathString(_In_ xaml_data::IBinding *pBinding, _In_ HSTRING hPath);

        _Check_return_ HRESULT put_PathImpl(_In_ xaml::IPropertyPath *pValue);
        _Check_return_ HRESULT get_PathImpl(_Outptr_ xaml::IPropertyPath **ppValue);

        // Customized methods.
        _Check_return_ HRESULT BeginInitImpl();
        _Check_return_ HRESULT EndInitImpl(_In_opt_ XamlServiceProviderContext* context);

    private:
        _Check_return_ HRESULT put_PathFast(_In_ HSTRING hValue);
        _Check_return_ HRESULT get_PathFast(_Outptr_ HSTRING* phValue);

        void Freeze()
        { m_fIsFrozen = TRUE; }

        BOOLEAN m_fHasConverter : 1;
        BOOLEAN m_fHasExplicitMode : 1;
        BOOLEAN m_fIsFrozen : 1;
        BOOLEAN __padding : 1;

        PropertyPathParser *m_pPropertyPathParser;

        friend class BindingExpression;
    };
}
