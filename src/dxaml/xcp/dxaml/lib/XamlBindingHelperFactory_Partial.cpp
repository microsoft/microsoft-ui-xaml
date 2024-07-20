// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlBindingHelper.g.h"
#include "ValueBuffer.h"
#include "DynamicValueConverter.h"
#include "UIElement.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

static _Check_return_ HRESULT SetValueImpl(
    _In_ IInspectable* pDependencyObject,
    _In_ xaml::IDependencyProperty* propertyToSet,
    _In_ const CValue& value)
{
    ctl::ComPtr<DependencyObject> depends;
    IFC_RETURN(ctl::do_query_interface(depends, pDependencyObject));
    const CDependencyProperty* actualDP = static_cast<DependencyPropertyHandle*>(propertyToSet)->GetDP();
    IFC_RETURN(depends->SetValueCore(actualDP, value));
    return S_OK;
}

static _Check_return_ HRESULT SetValueImpl(
    _In_ IInspectable* pDependencyObject,
    _In_ xaml::IDependencyProperty* propertyToSet,
    _In_ IInspectable* value)
{
    ctl::ComPtr<DependencyObject> depends;
    IFC_RETURN(ctl::do_query_interface(depends, pDependencyObject));
    const CDependencyProperty* actualDP = static_cast<DependencyPropertyHandle*>(propertyToSet)->GetDP();
    IFC_RETURN(depends->SetValueCore(actualDP, value));
    return S_OK;
}

_Check_return_ HRESULT XamlBindingHelperFactory::ConvertValueImpl(_In_ wxaml_interop::TypeName type, _In_ IInspectable* pValue, _Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;
    IFCPTR_RETURN(pValue);
    IFCPTR_RETURN(ppReturnValue);

    const CClassInfo* pInfo = nullptr;
    IFC_RETURN(MetadataAPI::GetClassInfoByTypeName(type, &pInfo));

    if (!m_valueConverter)
    {
        IFC_RETURN(DynamicValueConverter::CreateConverter(&m_valueConverter));
    }

    hr = m_valueConverter->Convert(pValue, pInfo, nullptr, ppReturnValue);
    if (FAILED(hr))
    {
        int len = 0;
        wchar_t szBuffer[MAX_PATH] = {};
        wrl_wrappers::HString strErrorString;
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetNonLocalizedErrorString(ERROR_BIND_CANNOT_CONVERT_VALUE_TO_TYPE, strErrorString.GetAddressOf()));
        IFCEXPECT_RETURN((len = swprintf_s(szBuffer, _countof(szBuffer), strErrorString.GetRawBuffer(NULL), pInfo->GetName().GetBuffer())) > 0);

        return ErrorHelper::OriginateError(E_INVALIDARG, len, szBuffer);
    }
    return hr;
}

#if XCP_MONITOR
IFACEMETHODIMP XamlBindingHelperFactory::Close()
{
    m_valueConverter.Reset();
    return S_OK;
}
#endif

_Check_return_ HRESULT XamlBindingHelperFactoryGenerated::SuspendRenderingImpl(_In_ xaml::IUIElement* pTarget)
{
    IFCPTR_RETURN(pTarget);

    static_cast<UIElement*>(pTarget)->GetHandle()->HideElementForSuspendRendering(true);

    return S_OK;
}


_Check_return_ HRESULT XamlBindingHelperFactoryGenerated::ResumeRenderingImpl(_In_ xaml::IUIElement* pTarget)
{
    IFCPTR_RETURN(pTarget);

    static_cast<UIElement*>(pTarget)->GetHandle()->HideElementForSuspendRendering(false);

    return S_OK;
}

_Check_return_ HRESULT XamlBindingHelperFactoryGenerated::ConvertValueImpl(_In_ wxaml_interop::TypeName type, _In_ IInspectable* pValue, _Outptr_ IInspectable** ppReturnValue)
{
    IFC_RETURN(static_cast<XamlBindingHelperFactory*>(this)->ConvertValueImpl(type, pValue, ppReturnValue));
    return S_OK;
}


_Check_return_ HRESULT XamlBindingHelperFactoryGenerated::SetPropertyFromBooleanImpl(_In_ IInspectable* pDependencyObject, _In_ xaml::IDependencyProperty* pPropertyToSet, _In_ BOOLEAN value)
{
    IFCPTR_RETURN(pDependencyObject);
    IFCPTR_RETURN(pPropertyToSet);

    CValue boxedValue;
    IFC_RETURN(CValueBoxer::BoxValue(&boxedValue, value));
    IFC_RETURN(SetValueImpl(pDependencyObject, pPropertyToSet, boxedValue));
    return S_OK;
}

_Check_return_ HRESULT XamlBindingHelperFactoryGenerated::SetPropertyFromByteImpl(_In_ IInspectable* pDependencyObject, _In_ xaml::IDependencyProperty* pPropertyToSet, _In_ BYTE value)
{
    IFCPTR_RETURN(pDependencyObject);
    IFCPTR_RETURN(pPropertyToSet);

    CValue boxedValue;
    IFC_RETURN(CValueBoxer::BoxValue(&boxedValue, value));
    IFC_RETURN(SetValueImpl(pDependencyObject, pPropertyToSet, boxedValue));
    return S_OK;

}

_Check_return_ HRESULT XamlBindingHelperFactoryGenerated::SetPropertyFromInt32Impl(_In_ IInspectable* pDependencyObject, _In_ xaml::IDependencyProperty* pPropertyToSet, _In_ INT value)
{
    IFCPTR_RETURN(pDependencyObject);
    IFCPTR_RETURN(pPropertyToSet);

    CValue boxedValue;
    IFC_RETURN(CValueBoxer::BoxValue(&boxedValue, value));
    IFC_RETURN(SetValueImpl(pDependencyObject, pPropertyToSet, boxedValue));
    return S_OK;
}

_Check_return_ HRESULT XamlBindingHelperFactoryGenerated::SetPropertyFromUInt32Impl(_In_ IInspectable* pDependencyObject, _In_ xaml::IDependencyProperty* pPropertyToSet, _In_ UINT value)
{
    IFCPTR_RETURN(pDependencyObject);
    IFCPTR_RETURN(pPropertyToSet);

    CValue boxedValue;
    IFC_RETURN(CValueBoxer::BoxValue(&boxedValue, static_cast<INT>(value)));
    IFC_RETURN(SetValueImpl(pDependencyObject, pPropertyToSet, boxedValue));
    return S_OK;
}

_Check_return_ HRESULT XamlBindingHelperFactoryGenerated::SetPropertyFromInt64Impl(_In_ IInspectable* pDependencyObject, _In_ xaml::IDependencyProperty* pPropertyToSet, _In_ INT64 value)
{
    IFCPTR_RETURN(pDependencyObject);
    IFCPTR_RETURN(pPropertyToSet);

    CValue boxedValue;
    IFC_RETURN(CValueBoxer::BoxValue(&boxedValue, value));
    IFC_RETURN(SetValueImpl(pDependencyObject, pPropertyToSet, boxedValue));
    return S_OK;
}

_Check_return_ HRESULT XamlBindingHelperFactoryGenerated::SetPropertyFromUInt64Impl(_In_ IInspectable* pDependencyObject, _In_ xaml::IDependencyProperty* pPropertyToSet, _In_ UINT64 value)
{
    IFCPTR_RETURN(pDependencyObject);
    IFCPTR_RETURN(pPropertyToSet);

    CValue boxedValue;
    IFC_RETURN(CValueBoxer::BoxValue(&boxedValue, static_cast<INT64>(value)));
    IFC_RETURN(SetValueImpl(pDependencyObject, pPropertyToSet, boxedValue));
    return S_OK;
}

_Check_return_ HRESULT XamlBindingHelperFactoryGenerated::SetPropertyFromDoubleImpl(_In_ IInspectable* pDependencyObject, _In_ xaml::IDependencyProperty* pPropertyToSet, _In_ DOUBLE value)
{
    IFCPTR_RETURN(pDependencyObject);
    IFCPTR_RETURN(pPropertyToSet);

    CValue boxedValue;
    IFC_RETURN(CValueBoxer::BoxValue(&boxedValue, value));
    IFC_RETURN(SetValueImpl(pDependencyObject, pPropertyToSet, boxedValue));
    return S_OK;
}

_Check_return_ HRESULT XamlBindingHelperFactoryGenerated::SetPropertyFromSingleImpl(_In_ IInspectable* pDependencyObject, _In_ xaml::IDependencyProperty* pPropertyToSet, _In_ FLOAT value)
{
    IFCPTR_RETURN(pDependencyObject);
    IFCPTR_RETURN(pPropertyToSet);

    CValue boxedValue;
    IFC_RETURN(CValueBoxer::BoxValue(&boxedValue, value));
    IFC_RETURN(SetValueImpl(pDependencyObject, pPropertyToSet, boxedValue));
    return S_OK;
}

_Check_return_ HRESULT XamlBindingHelperFactoryGenerated::SetPropertyFromStringImpl(_In_ IInspectable* pDependencyObject, _In_ xaml::IDependencyProperty* pPropertyToSet, _In_ HSTRING value)
{
    IFCPTR_RETURN(pDependencyObject);
    IFCPTR_RETURN(pPropertyToSet);
    IFCPTR_RETURN(value);

    if ((static_cast<DependencyPropertyHandle*>(pPropertyToSet))->GetDP()->GetStorageType() != valueString)
    {
        return E_INVALIDARG;
    }

    CValue boxedValue;
    IFC_RETURN(CValueBoxer::BoxValue(&boxedValue, value));
    IFC_RETURN(SetValueImpl(pDependencyObject, pPropertyToSet, boxedValue));
    return S_OK;
}

_Check_return_ HRESULT XamlBindingHelperFactoryGenerated::SetPropertyFromChar16Impl(_In_ IInspectable* pDependencyObject, _In_ xaml::IDependencyProperty* pPropertyToSet, _In_ WCHAR value)
{
    IFCPTR_RETURN(pDependencyObject);
    IFCPTR_RETURN(pPropertyToSet);

    CValue boxedValue;
    IFC_RETURN(CValueBoxer::BoxValue(&boxedValue, value));
    IFC_RETURN(SetValueImpl(pDependencyObject, pPropertyToSet, boxedValue));
    return S_OK;
}

_Check_return_ HRESULT XamlBindingHelperFactoryGenerated::SetPropertyFromTimeSpanImpl(_In_ IInspectable* pDependencyObject, _In_ xaml::IDependencyProperty* pPropertyToSet, _In_ wf::TimeSpan value)
{
    IFCPTR_RETURN(pDependencyObject);
    IFCPTR_RETURN(pPropertyToSet);

    CValue boxedValue;
    IFC_RETURN(CValueBoxer::BoxValue(&boxedValue, value));
    IFC_RETURN(SetValueImpl(pDependencyObject, pPropertyToSet, boxedValue));
    return S_OK;
}

_Check_return_ HRESULT XamlBindingHelperFactoryGenerated::SetPropertyFromDateTimeImpl(_In_ IInspectable* pDependencyObject, _In_ xaml::IDependencyProperty* pPropertyToSet, _In_ wf::DateTime value)
{
    IFCPTR_RETURN(pDependencyObject);
    IFCPTR_RETURN(pPropertyToSet);

    CValue boxedValue;
    IFC_RETURN(CValueBoxer::BoxValue(&boxedValue, value));
    IFC_RETURN(SetValueImpl(pDependencyObject, pPropertyToSet, boxedValue));
    return S_OK;
}

_Check_return_ HRESULT XamlBindingHelperFactoryGenerated::SetPropertyFromPointImpl(_In_ IInspectable* pDependencyObject, _In_ xaml::IDependencyProperty* pPropertyToSet, _In_ wf::Point value)
{
    IFCPTR_RETURN(pDependencyObject);
    IFCPTR_RETURN(pPropertyToSet);

    CValue boxedValue;
    BoxerBuffer buffer;
    IFC_RETURN(CValueBoxer::BoxValue(&boxedValue, value, &buffer));
    IFC_RETURN(SetValueImpl(pDependencyObject, pPropertyToSet, boxedValue));
    return S_OK;
}

_Check_return_ HRESULT XamlBindingHelperFactoryGenerated::SetPropertyFromSizeImpl(_In_ IInspectable* pDependencyObject, _In_ xaml::IDependencyProperty* pPropertyToSet, _In_ wf::Size value)
{
    IFCPTR_RETURN(pDependencyObject);
    IFCPTR_RETURN(pPropertyToSet);

    CValue boxedValue;
    BoxerBuffer buffer;
    IFC_RETURN(CValueBoxer::BoxValue(&boxedValue, value, &buffer));
    IFC_RETURN(SetValueImpl(pDependencyObject, pPropertyToSet, boxedValue));
    return S_OK;
}

_Check_return_ HRESULT XamlBindingHelperFactoryGenerated::SetPropertyFromRectImpl(_In_ IInspectable* pDependencyObject, _In_ xaml::IDependencyProperty* pPropertyToSet, _In_ wf::Rect value)
{
    IFCPTR_RETURN(pDependencyObject);
    IFCPTR_RETURN(pPropertyToSet);

    CValue boxedValue;
    BoxerBuffer buffer;
    IFC_RETURN(CValueBoxer::BoxValue(&boxedValue, value, &buffer));
    IFC_RETURN(SetValueImpl(pDependencyObject, pPropertyToSet, boxedValue));
    return S_OK;
}

_Check_return_ HRESULT XamlBindingHelperFactoryGenerated::SetPropertyFromUriImpl(_In_ IInspectable* pDependencyObject, _In_ xaml::IDependencyProperty* pPropertyToSet, _In_ wf::IUriRuntimeClass* pValue)
{
    IFCPTR_RETURN(pDependencyObject);
    IFCPTR_RETURN(pPropertyToSet);
    IFCPTR_RETURN(pValue);

    CValue boxedValue;
    IFC_RETURN(CValueBoxer::BoxValue(&boxedValue, pValue));
    IFC_RETURN(SetValueImpl(pDependencyObject, pPropertyToSet, boxedValue));
    return S_OK;
}

_Check_return_ HRESULT XamlBindingHelperFactoryGenerated::SetPropertyFromObjectImpl(_In_ IInspectable* pDependencyObject, _In_ xaml::IDependencyProperty* pPropertyToSet, _In_ IInspectable* pValue)
{
    IFCPTR_RETURN(pDependencyObject);
    IFCPTR_RETURN(pPropertyToSet);

    IFC_RETURN(SetValueImpl(pDependencyObject, pPropertyToSet, pValue));
    return S_OK;
}
