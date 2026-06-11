// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CColor.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <DOPointerCast.h>

CColor::CColor(_In_ CCoreServices *pCore)
    : CDependencyObject(pCore)
    , m_rgb(0)
{
}

_Check_return_ HRESULT
CColor::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate)
{
    ASSERT(ppObject);
    ASSERT(pCreate);

    *ppObject = nullptr;

    //auto result = make_xref<CColor>(pCreate->m_pCore);
    CColor* result = new CColor(pCreate->m_pCore);

    // There is only one kind of color converter and we're usually using the one
    // stored in the local store.
    if (pCreate->m_value.GetType() == valueObject)
    {
        CColor* color = checked_cast<CColor>(pCreate->m_value);

        if (color)
        {
            result->m_rgb = color->m_rgb;
        }
    }
    else if (pCreate->m_value.GetType() == valueString)
    {
        IFC_RETURN(ColorFromString(pCreate->m_value.AsString(), &result->m_rgb));
    }
    else if (pCreate->m_value.GetType() == valueColor)
    {
        result->m_rgb = pCreate->m_value.AsColor();
    }
    else
    {
        // Currently only support, string, object and the default
        ASSERT(pCreate->m_value.GetType() == valueAny);
    }

    // Return the object to the caller
    //*ppObject = result.detach();
    *ppObject = result;
    return S_OK;
}

KnownTypeIndex CColor::GetTypeIndex() const
{
    return DependencyObjectTraits<CColor>::Index;
}
