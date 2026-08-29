// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "Value.h"
#include "PropertyValueWrapper.h"

using namespace DirectUI;
using namespace ::Windows::UI::Xaml::Tests;

_Check_return_ HRESULT PropertyValue::CreateEmpty(_Outptr_ IInspectable **ppValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT PropertyValue::CreateFromBoolean(_In_ BOOLEAN bValue, _Outptr_ IInspectable **ppValue)
{
    IFC_RETURN(PropertyValueWrapper::GetPropertyValueStatics()->CreateBoolean(bValue, ppValue));
    return S_OK;
}

_Check_return_ HRESULT PropertyValue::CreateFromUInt8(_In_ BYTE nValue, _Outptr_ IInspectable **ppValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT PropertyValue::CreateFromInt16(_In_ INT16 nValue, _Outptr_ IInspectable **ppValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT PropertyValue::CreateFromUInt16(_In_ UINT16 nValue, _Outptr_ IInspectable **ppValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT PropertyValue::CreateFromInt32(_In_ INT32 nValue, _Outptr_ IInspectable **ppValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT PropertyValue::CreateFromUInt32(_In_ UINT32 nValue, _Outptr_ IInspectable **ppValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT PropertyValue::CreateFromInt64(_In_ INT64 nValue, _Outptr_ IInspectable **ppValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT PropertyValue::CreateFromUInt64(_In_ UINT64 nValue, _Outptr_ IInspectable **ppValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT PropertyValue::CreateFromDouble(_In_ XDOUBLE nValue, _Outptr_ IInspectable **ppValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT PropertyValue::CreateFromSingle(_In_ XFLOAT nValue, _Outptr_ IInspectable **ppValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT PropertyValue::CreateFromString(_In_opt_ HSTRING hString, _Outptr_ IInspectable **ppValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT PropertyValue::CreateFromChar16(_In_ WCHAR chValue, _Outptr_ IInspectable **ppValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT PropertyValue::CreateFromTimeSpan(_In_ wf::TimeSpan value, _Outptr_ IInspectable **ppValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT PropertyValue::CreateFromDateTime(_In_ wf::DateTime value, _Outptr_ IInspectable **ppValue)
{
    IFC_RETURN(PropertyValueWrapper::GetPropertyValueStatics()->CreateDateTime(value, ppValue));
    return S_OK;
}

_Check_return_ HRESULT PropertyValue::CreateFromPoint(_In_ wf::Point value, _Outptr_ IInspectable **ppValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT PropertyValue::CreateFromSize(_In_ wf::Size value, _Outptr_ IInspectable **ppValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT PropertyValue::CreateFromRect(_In_ wf::Rect value, _Outptr_ IInspectable **ppValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT PropertyValue::CreateFromGuid(_In_ GUID value, _Outptr_ IInspectable **ppValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT PropertyValue::CreateFromTextRange(_In_ xaml_docs::TextRange value, _Outptr_ IInspectable **ppValue)
{
    return E_NOTIMPL;
}
