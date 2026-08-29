// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "IValueBoxer.h"

using namespace DirectUI;

_Check_return_ HRESULT IValueBoxer::BoxValue(
    _In_ IInspectable** box,
    _In_ BOOLEAN value)
{
    return PropertyValue::CreateFromBoolean(value, box);
}

_Check_return_ HRESULT IValueBoxer::BoxValue(
    _In_ IInspectable** box,
    _In_ INT value)
{
    return PropertyValue::CreateFromInt32(value, box);
}

_Check_return_ HRESULT IValueBoxer::BoxValue(
    _In_ IInspectable** box,
    _In_ UINT value)
{
    return PropertyValue::CreateFromUInt32(value, box);
}

_Check_return_ HRESULT IValueBoxer::BoxValue(
    _In_ IInspectable** box,
    _In_ INT64 value)
{
    return PropertyValue::CreateFromInt64(value, box);
}

_Check_return_ HRESULT IValueBoxer::BoxValue(
    _In_ IInspectable** box,
    _In_ UINT64 value)
{
    return PropertyValue::CreateFromUInt64(value, box);
}

_Check_return_ HRESULT IValueBoxer::BoxValue(
    _In_ IInspectable** box,
    _In_ DOUBLE value)
{
    return PropertyValue::CreateFromDouble(value, box);
}

_Check_return_ HRESULT IValueBoxer::BoxValue(
    _In_ IInspectable** box,
    _In_ FLOAT value)
{
    return PropertyValue::CreateFromSingle(value, box);
}

_Check_return_ HRESULT IValueBoxer::BoxValue(
    _In_ IInspectable** box,
    _In_opt_ HSTRING value)
{
    return PropertyValue::CreateFromString(value, box);
}

_Check_return_ HRESULT IValueBoxer::BoxValue(
    _In_ IInspectable** box,
    _In_ wf::DateTime value)
{
    return PropertyValue::CreateFromDateTime(value, box);
}

_Check_return_ HRESULT IValueBoxer::BoxValue(
    _In_ IInspectable** box,
    _In_ wf::TimeSpan value)
{
    return PropertyValue::CreateFromTimeSpan(value, box);
}

_Check_return_ HRESULT IValueBoxer::BoxValue(
    _In_ IInspectable** box,
    _In_ wf::Point value)
{
    return PropertyValue::CreateFromPoint(value, box);
}

_Check_return_ HRESULT IValueBoxer::BoxValue(
    _In_ IInspectable** box,
    _In_ wf::Size value)
{
    return PropertyValue::CreateFromSize(value, box);
}

_Check_return_ HRESULT IValueBoxer::BoxValue(
    _In_ IInspectable** box,
    _In_ wf::Rect value)
{
    return PropertyValue::CreateFromRect(value, box);
}

_Check_return_ HRESULT IValueBoxer::BoxValue(
    _In_ IInspectable** box,
    _In_ wf::IUriRuntimeClass *value)
{
    IFCPTR_RETURN(box);

    *box = value;
    AddRefInterface(value);

    return S_OK;
}

_Check_return_ HRESULT IValueBoxer::BoxObjectValue(
    _In_ IInspectable** box,
    _In_ IInspectable* value)
{
    *box = value;
    AddRefInterface(value);

    return S_OK;
}

_Check_return_ HRESULT IValueBoxer::UnboxValue(
    _In_ IInspectable* box,
    _Out_ BOOLEAN* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    return ctl::do_get_value(*value, box);
}

_Check_return_ HRESULT IValueBoxer::UnboxValue(
    _In_ IInspectable* box,
    _Out_ INT* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    return ctl::do_get_value(*value, box);
}

_Check_return_ HRESULT IValueBoxer::UnboxValue(
    _In_ IInspectable* box,
    _Out_ UINT* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    return ctl::do_get_value(*value, box);
}

_Check_return_ HRESULT IValueBoxer::UnboxValue(
    _In_ IInspectable* box,
    _Out_ INT64* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    return ctl::do_get_value(*value, box);
}

_Check_return_ HRESULT IValueBoxer::UnboxValue(
    _In_ IInspectable* box,
    _Out_ UINT64* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    return ctl::do_get_value(*value, box);
}

_Check_return_ HRESULT IValueBoxer::UnboxValue(
    _In_ IInspectable* box,
    _Out_ DOUBLE* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    return ctl::do_get_value(*value, box);
}

_Check_return_ HRESULT IValueBoxer::UnboxValue(
    _In_ IInspectable* box,
    _Out_ FLOAT* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    wf::PropertyType type = wf::PropertyType_Empty;
    ctl::ComPtr<wf::IPropertyValue> spBoxAsPV = ctl::query_interface_cast<wf::IPropertyValue>(box);

    IFC_RETURN(spBoxAsPV->get_Type(&type));

    switch (type)
    {
        case wf::PropertyType_Single:
            IFC_RETURN(spBoxAsPV->GetSingle(value));
            break;

        case wf::PropertyType_Double:
            {
                DOUBLE dValue = 0.0;
                IFC_RETURN(spBoxAsPV->GetDouble(&dValue));
                *value = static_cast<FLOAT>(dValue);
            }
            break;

        default:
            IFC_RETURN(E_UNEXPECTED);
            break;
    }

    return S_OK;
}

_Check_return_ HRESULT IValueBoxer::UnboxValue(
    _In_opt_ IInspectable* box,
    _Outptr_ HSTRING* value)
{
    IFCPTR_RETURN(value);

    return ctl::do_get_value(*value, box);
}

_Check_return_ HRESULT IValueBoxer::UnboxValue(
    _In_ IInspectable* box,
    _Out_ wf::DateTime* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    return ctl::do_get_value(*value, box);
}

_Check_return_ HRESULT IValueBoxer::UnboxValue(
    _In_ IInspectable* box,
    _Out_ wf::TimeSpan* value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    return ctl::do_get_value(*value, box);
}

_Check_return_ HRESULT IValueBoxer::UnboxValue(
    _In_ IInspectable* box,
    _Out_ wf::IUriRuntimeClass** value)
{
    IFCPTR_RETURN(box);
    IFCPTR_RETURN(value);

    *value = nullptr;

    return ctl::do_get_value(*value, box);
}

_Check_return_ HRESULT IValueBoxer::UnboxValue(
    _In_opt_ IInspectable* box,
    _Out_ wxaml_interop::TypeName* value)
{
    IFCPTR_RETURN(value);

    if (box)
    {
        ctl::ComPtr<wf::IReference<wxaml_interop::TypeName>> spObjAsRef;

        IFC_RETURN(ctl::do_query_interface(spObjAsRef, box));
        IFC_RETURN(spObjAsRef->get_Value(value));
    }
    else
    {
        value->Name = nullptr;
        value->Kind = wxaml_interop::TypeKind_Primitive;
    }

    return S_OK;
}

_Check_return_ HRESULT IValueBoxer::UnboxObjectValue(
    _In_opt_ IInspectable* box,
    _In_ REFIID riid,
    _Out_ void** value)
{
    IFCPTR_RETURN(value);

    if (box)
    {
        IFC_RETURN(box->QueryInterface(riid, value));
    }
    else
    {
        *value = nullptr;
    }

    return S_OK;
}
