// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "StateTriggerBase.h"


_Check_return_ HRESULT CStateTriggerBase::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    // Call to base first
    IFC_RETURN(CDependencyObject::OnPropertyChanged(args));

    //  If trigger state has changed, re-evaluate triggers
    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::StateTriggerBase_TriggerState:
        {
            m_triggerState = args.m_pNewValue->AsBool();
            IFC_RETURN(EvaluateStateTriggers());
            break;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CStateTriggerBase::EvaluateStateTriggers()
{
    for(auto& pVariantMap : m_owningVariantMaps)
    {
        if(auto map = pVariantMap.lock())
        {
            IFC_RETURN(map->Evaluate());
        }
    }

    return S_OK;
}
