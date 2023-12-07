// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CalendarViewGeneratorHost.h"
#include "CalendarView.g.h"
#include "CalendarViewItem.g.h"
#include "CalendarViewDayItem_Partial.h"
#include "CalendarViewDayItemChangingEventArgs.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Work around disruptive max/min macros
#undef max
#undef min

// DataVirtualization - to avoid creating large number of calendar items,
// we only implement GetAt and get_Size.

// IVector<IInspectable*> implementation
IFACEMETHODIMP CalendarViewGeneratorHost::GetAt(_In_ UINT index, _Outptr_ IInspectable** item)
{
    HRESULT hr = S_OK;
    wf::DateTime date;

    IFC(GetDateAt(index, &date));

    IFC(PropertyValue::CreateFromDateTime(date, item));

Cleanup:
    return hr;
}

IFACEMETHODIMP CalendarViewGeneratorHost::get_Size(_Out_ UINT* value)
{
    *value = m_size;
    return S_OK;
}

IFACEMETHODIMP CalendarViewGeneratorHost::GetView(_Outptr_result_maybenull_ wfc::IVectorView<IInspectable*>** view)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVectorView<IInspectable*>> spResult;

    IFC(CheckThread());
    ARG_VALIDRETURNPOINTER(view);

    IFC(ctl::ComObject<TrackerView<IInspectable*>>::CreateInstance(spResult.ReleaseAndGetAddressOf()));
    spResult.Cast<TrackerView<IInspectable*>>()->SetCollection(this);

    *view = spResult.Detach();

Cleanup:

    return hr;
}

IFACEMETHODIMP CalendarViewGeneratorHost::IndexOf(_In_opt_ IInspectable* value, _Out_ UINT* index, _Out_ BOOLEAN* found)
{
    return E_NOTIMPL;
}

IFACEMETHODIMP CalendarViewGeneratorHost::SetAt(_In_ UINT index, _In_opt_ IInspectable* item)
{
    return E_NOTIMPL;
}

IFACEMETHODIMP CalendarViewGeneratorHost::InsertAt(_In_ UINT index, _In_ IInspectable* item)
{
    return E_NOTIMPL;
}

IFACEMETHODIMP CalendarViewGeneratorHost::RemoveAt(_In_ UINT index)
{
    return E_NOTIMPL;
}

IFACEMETHODIMP CalendarViewGeneratorHost::Append(_In_opt_ IInspectable* item)
{
    return E_NOTIMPL;
}

IFACEMETHODIMP CalendarViewGeneratorHost::RemoveAtEnd()
{
    return E_NOTIMPL;
}

IFACEMETHODIMP CalendarViewGeneratorHost::Clear()
{
    return E_NOTIMPL;
}
