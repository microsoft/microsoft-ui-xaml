// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <Thickness.h>
#include <StringConversions.h>
#include <DOPointerCast.h>

// Creates an instance of a 2D Thickness object.
_Check_return_ HRESULT CThickness::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate)
{
    HRESULT hr = S_OK;

    ASSERT(ppObject);
    ASSERT(pCreate);

    CThickness *_this = new CThickness(pCreate->m_pCore);

    if (pCreate->m_value.GetType() == valueObject)
    {
        CThickness* thickness = checked_cast<CThickness>(pCreate->m_value);

        if (thickness)
        {
            _this->m_thickness = thickness->m_thickness;
        }
    }
    else if (pCreate->m_value.GetType() == valueString)
    {
        // parser needs to understand how to go from string to thickness
        // Call the type converter. Must go in descending order of number of parameters
        IFC(ThicknessFromString(pCreate->m_value.AsString(), &_this->m_thickness));
    }
    else if (pCreate->m_value.GetType() == valueThickness)
    {
        _this->m_thickness = *pCreate->m_value.AsThickness();
    }
    else
    {
        ASSERT(pCreate->m_value.GetType() == valueAny);
    }

    // Return the object to the caller

    *ppObject = static_cast<CDependencyObject *>(_this);
    _this = NULL;

Cleanup:
    ReleaseInterface(_this);
    return hr;
}

// Allow multiple association.
bool CThickness::DoesAllowMultipleAssociation() const
{
    return true;
}

KnownTypeIndex CThickness::GetTypeIndex() const
{
    return DependencyObjectTraits<CThickness>::Index;
}
