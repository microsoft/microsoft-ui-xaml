// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TimeSpan.h"
#include <StringConversions.h>
#include <Clock.h>

KnownTypeIndex CTimeSpan::GetTypeIndex() const
{
    return DependencyObjectTraits<CTimeSpan>::Index;
}

_Check_return_ HRESULT CTimeSpan::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate)
{
    HRESULT hr = S_OK;
    CTimeSpan *pTimeSpan = new CTimeSpan(pCreate->m_pCore);

    ASSERT((pCreate->m_value.GetType() == valueString) ||
           (pCreate->m_value.GetType() == valueFloat) ||
           (pCreate->m_value.GetType() == valueTimeSpan) ||
           (pCreate->m_value.GetType() == valueDouble) ||
           (pCreate->m_value.GetType() == valueAny));

    if (pCreate->m_value.GetType() == valueString)
    {
        IFC(pTimeSpan->InitFromString(pCreate->m_value.AsString()));
    }
    // for compatibility with XBF
    else if (pCreate->m_value.GetType() == valueFloat)
    {
        pTimeSpan->m_rTimeSpan = pCreate->m_value.AsFloat();
    }
    else if (pCreate->m_value.GetType() == valueDouble)
    {
        pTimeSpan->m_rTimeSpan = pCreate->m_value.AsDouble();
    }
    else if (pCreate->m_value.GetType() == valueTimeSpan)
    {
        wf::TimeSpan ts = {};
        IFC(pCreate->m_value.GetTimeSpan(ts));

        pTimeSpan->m_rTimeSpan = TimeSpanUtil::ToSeconds(ts);
    }

   *ppObject = pTimeSpan;
   pTimeSpan = NULL;

Cleanup:
    delete pTimeSpan;

    RRETURN(hr);
}

_Check_return_ HRESULT CTimeSpan::InitFromString(
    _In_ const xstring_ptr_view& inString
    )
{
    return TimeSpanFromString(inString, &m_rTimeSpan);
}
