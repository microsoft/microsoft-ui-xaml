// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines the basic class that specifies all of the parameters
//      that compose a binding beteween a DependencyObject and a source.
//      The manually written override will ensure that once the Binding
//      is used in a binding expression that it will be immutable.

#include "precomp.h"
#include "Binding.g.h"
#include "PropertyPath.g.h"
#include "PropertyPathParser.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

Binding::~Binding()
{
    delete m_pPropertyPathParser;
}

_Check_return_ HRESULT Binding::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(BindingGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::Binding_Path:
        if (!GetHandle()->ParserOwnsParent())
        {
            IFC(UpdatePropertyPathParser());
        }
        break;
    case KnownPropertyIndex::Binding_Converter:
        m_fHasConverter = (!args.m_pNewValue->IsNullOrUnset());
        break;
    case KnownPropertyIndex::Binding_Mode:
        m_fHasExplicitMode = TRUE;
        break;
    }

Cleanup:

    RRETURN(hr);
}

// IBinding interface 
IFACEMETHODIMP Binding::get_Converter(_Outptr_ xaml_data::IValueConverter** pValue)
{
    if (m_fHasConverter)
    {
        RRETURN(BindingGenerated::get_Converter(pValue));
    }

    *pValue = nullptr;
    RRETURN(S_OK);
}

IFACEMETHODIMP Binding::get_Mode(_Out_ xaml_data::BindingMode* pValue)
{
    if (m_fHasExplicitMode)
    {
        RRETURN(BindingGenerated::get_Mode(pValue));
    }

    *pValue = xaml_data::BindingMode_OneWay;
    RRETURN(S_OK);
}

_Check_return_ HRESULT Binding::put_PathImpl(_In_ xaml::IPropertyPath *pValue)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strPath;

    if (m_fIsFrozen)
    {
        return ErrorHelper::OriginateErrorUsingResourceID(E_FAIL,ERROR_OBJECT_IS_FROZEN);
    }

    if (pValue)
    {
        IFC(pValue->get_Path(strPath.GetAddressOf()));
        IFC(put_PathFast(strPath.Get()));
    }
    else
    {
        IFC(put_PathFast(NULL));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Binding::get_PathImpl(_In_ xaml::IPropertyPath **ppValue)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strPath;
    ctl::ComPtr<PropertyPath> spPropertyPath;
    
    IFC(get_PathFast(strPath.GetAddressOf()));
    if (strPath != NULL)
    {
        IFC(PropertyPath::CreateInstance(strPath.Get(), &spPropertyPath));
        *ppValue = spPropertyPath.Detach();
    }
    else
    {
        *ppValue = NULL;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Binding::put_PathFast(_In_ HSTRING hValue)
{
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::Binding_Path, hValue));
}

_Check_return_ HRESULT Binding::get_PathFast(_Outptr_ HSTRING* phValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Binding_Path, phValue));
}

IFACEMETHODIMP Binding::put_Mode(_In_ xaml_data::BindingMode value)
{
    if (m_fIsFrozen)
    {
        return ErrorHelper::OriginateErrorUsingResourceID(E_FAIL,ERROR_OBJECT_IS_FROZEN);
    }

    return BindingGenerated::put_Mode(value);
}

IFACEMETHODIMP Binding::put_Source(_In_ IInspectable* value)
{
    if (m_fIsFrozen)
    {
        return ErrorHelper::OriginateErrorUsingResourceID(E_FAIL,ERROR_OBJECT_IS_FROZEN);
    }

    return BindingGenerated::put_Source(value);
}

IFACEMETHODIMP Binding::put_RelativeSource(_In_ xaml_data::IRelativeSource* value)
{
    if (m_fIsFrozen)
    {
        return ErrorHelper::OriginateErrorUsingResourceID(E_FAIL,ERROR_OBJECT_IS_FROZEN);
    }

    return BindingGenerated::put_RelativeSource(value);
}

IFACEMETHODIMP Binding::put_ElementName(_In_ HSTRING value)
{
    if (m_fIsFrozen)
    {
        return ErrorHelper::OriginateErrorUsingResourceID(E_FAIL,ERROR_OBJECT_IS_FROZEN);
    }

    return BindingGenerated::put_ElementName(value);
}

IFACEMETHODIMP Binding::put_Converter(_In_ xaml_data::IValueConverter* value)
{
    if (m_fIsFrozen)
    {
        return ErrorHelper::OriginateErrorUsingResourceID(E_FAIL,ERROR_OBJECT_IS_FROZEN);
    }

    return BindingGenerated::put_Converter(value);
}

IFACEMETHODIMP Binding::put_ConverterParameter(_In_ IInspectable* value)
{
    if (m_fIsFrozen)
    {
        return ErrorHelper::OriginateErrorUsingResourceID(E_FAIL,ERROR_OBJECT_IS_FROZEN);
    }

    return BindingGenerated::put_ConverterParameter(value);
}

IFACEMETHODIMP Binding::put_ConverterLanguage(_In_ HSTRING value)
{
    if (m_fIsFrozen)
    {
        return ErrorHelper::OriginateErrorUsingResourceID(E_FAIL,ERROR_OBJECT_IS_FROZEN);
    }

    return BindingGenerated::put_ConverterLanguage(value);
}

IFACEMETHODIMP Binding::put_FallbackValue(_In_ IInspectable* value)
{
    if (m_fIsFrozen)
    {
        return ErrorHelper::OriginateErrorUsingResourceID(E_FAIL,ERROR_OBJECT_IS_FROZEN);
    }

    return BindingGenerated::put_FallbackValue(value);
}

IFACEMETHODIMP Binding::put_TargetNullValue(_In_ IInspectable* value)
{
    if (m_fIsFrozen)
    {
        return ErrorHelper::OriginateErrorUsingResourceID(E_FAIL,ERROR_OBJECT_IS_FROZEN);
    }

    return BindingGenerated::put_TargetNullValue(value);
}

IFACEMETHODIMP Binding::put_UpdateSourceTrigger(_In_ xaml_data::UpdateSourceTrigger value)
{
    if (m_fIsFrozen)
    {
        return ErrorHelper::OriginateErrorUsingResourceID(E_FAIL,ERROR_OBJECT_IS_FROZEN);
    }

    return BindingGenerated::put_UpdateSourceTrigger(value);
}

// ISupportInitialize interface
_Check_return_ HRESULT
Binding::BeginInitImpl()
{
    RRETURN(S_OK);
}

_Check_return_ HRESULT
Binding::EndInitImpl(_In_opt_ DirectUI::XamlServiceProviderContext* context)
{
    IFC_RETURN(EnsurePropertyPathParser(context));
    return S_OK;
}

_Check_return_ 
HRESULT 
Binding::UpdatePropertyPathParser()
{
    HRESULT hr = S_OK;
    HSTRING hPath = NULL;
    LPCWSTR szNewPath = NULL;

    // Get rid of the old property path
    delete m_pPropertyPathParser;
    m_pPropertyPathParser = NULL;

    IFC(GetPathString(&hPath));

    szNewPath = WindowsGetStringRawBuffer(hPath, NULL);

    m_pPropertyPathParser = new PropertyPathParser();
    IFC(m_pPropertyPathParser->SetSource(szNewPath, nullptr));

Cleanup:
    DELETE_STRING(hPath);
    RRETURN(hr);
}

_Check_return_ 
HRESULT 
Binding::EnsurePropertyPathParser(_In_opt_ DirectUI::XamlServiceProviderContext* context)
{
    if (m_pPropertyPathParser == nullptr)
    {
        m_pPropertyPathParser = new PropertyPathParser();

        wrl_wrappers::HString strPath;
        IFC_RETURN(GetPathString(strPath.GetAddressOf()));
        LPCWSTR szPath = strPath.GetRawBuffer(nullptr);

        IFC_RETURN(m_pPropertyPathParser->SetSource(szPath, context));
    }

    return S_OK;
}


_Check_return_ 
HRESULT 
Binding::GetPropertyPathParser(_Outptr_ PropertyPathParser **ppPropertyPath)
{
    IFC_RETURN(EnsurePropertyPathParser(/* context */ nullptr));
    *ppPropertyPath = m_pPropertyPathParser;

    return S_OK;
}

_Check_return_ 
HRESULT 
Binding::GetPathString(_Out_ HSTRING *phPath)
{
    RRETURN(get_PathFast(phPath));
}

_Check_return_ 
HRESULT 
Binding::SetPathString(_In_ HSTRING hPath)
{
    RRETURN(put_PathFast(hPath));
}

_Check_return_ 
HRESULT 
Binding::SetPathString(_In_ xaml_data::IBinding *pBinding, _In_ HSTRING hPath)
{
    HRESULT hr = S_OK;
    Binding *pBindingInstance = static_cast<Binding *>(pBinding);

    IFC(pBindingInstance->SetPathString(hPath));

Cleanup:

    RRETURN(hr);
}
