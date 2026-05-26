// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CalendarDatePickerDateChangedEventArgs.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT CalendarDatePickerDateChangedEventArgs::get_NewDateImpl(_Out_ wf::IReference<wf::DateTime>** ppValue)
{
    return m_tpNewDate.CopyTo(ppValue);
}
_Check_return_ HRESULT CalendarDatePickerDateChangedEventArgs::put_NewDateImpl(_In_ wf::IReference<wf::DateTime>* pValue)
{
    SetPtrValue(m_tpNewDate, pValue);
    return S_OK;
}
_Check_return_ HRESULT CalendarDatePickerDateChangedEventArgs::get_OldDateImpl(_Out_ wf::IReference<wf::DateTime>** ppValue)
{
    return m_tpOldDate.CopyTo(ppValue);
}
_Check_return_ HRESULT CalendarDatePickerDateChangedEventArgs::put_OldDateImpl(_In_ wf::IReference<wf::DateTime>* pValue)
{
    SetPtrValue(m_tpOldDate, pValue);
    return S_OK;
}