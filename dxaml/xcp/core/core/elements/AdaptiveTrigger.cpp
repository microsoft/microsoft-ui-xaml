// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AdaptiveTrigger.h"
#include "QualifierFactory.h"

_Check_return_ HRESULT CAdaptiveTrigger::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    // Call to base first
    IFC_RETURN(CDependencyObject::OnPropertyChanged(args));

    XFLOAT previousWidth = m_minWindowWidth;
    XFLOAT previousHeight = m_minWindowHeight;

    // If the 'Triggered' state has changed, re-evaluate triggers
    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::AdaptiveTrigger_MinWindowWidth:
        {
            previousWidth = args.m_pOldValue->AsFloat();
            break;
        }
        case KnownPropertyIndex::AdaptiveTrigger_MinWindowHeight:
        {
            previousHeight = args.m_pOldValue->AsFloat();
            break;
        }
        default:
        {
            return S_OK;
        }
    }

    for (auto& pVariantMap : m_owningVariantMaps)
    {
        if (auto map = pVariantMap.lock())
        {
            IFC_RETURN(map->Replace(
                xref_ptr<CDependencyObject>(this),
                QualifierFactory::Create(static_cast<int>(previousWidth), static_cast<int>(previousHeight)),
                QualifierFactory::Create(static_cast<int>(m_minWindowWidth), static_cast<int>(m_minWindowHeight))));
            IFC_RETURN(map->Evaluate());
        }
    }

    return S_OK;
}
