// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "StateTrigger.h"

_Check_return_ HRESULT
CStateTrigger::IsActive(
    _In_ CDependencyObject* pObject,
    _In_ XUINT32 cArgs,
    _In_reads_(cArgs) CValue* pArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Inout_ CValue* pResult)
{
    CStateTrigger* pThis = do_pointer_cast<CStateTrigger>(pObject);
    if(!(pResult && pObject && pThis))
    {
        return E_INVALIDARG;
    }

    if (cArgs == 0)
    {
        // GetValue
        pResult->SetBool(pThis->m_triggerState);
    }
    else
    {
        // SetValue
        if (!pArgs) return E_INVALIDARG;
        IFC_RETURN((pArgs->GetType() == valueBool) ? S_OK : E_INVALIDARG);
        pThis->m_triggerState = pArgs->AsBool();
        IFC_RETURN(pThis->EvaluateStateTriggers());
    }

    return S_OK;
}
