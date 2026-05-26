// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PropertyChangedParamsHelper.h"

using namespace DirectUI;

_Check_return_ HRESULT PropertyChangedParamsHelper::GetObjects(_In_ const PropertyChangedParams& args, _Out_ IInspectable** ppOldValue, _Out_ IInspectable** ppNewValue)
{
    HRESULT hr = S_OK;

    // Grab the outer IInspectables if they exist for object identity purposes.
    if (args.m_pOldValueOuterNoRef != nullptr)
    {
        CValueBoxer::UnwrapExternalObjectReferenceIfPresent(args.m_pOldValueOuterNoRef, ppOldValue);
    }
    else
    {
        IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, args.m_pDP->GetPropertyType(), ppOldValue));
    }

    if (args.m_pNewValueOuterNoRef != nullptr)
    {
        CValueBoxer::UnwrapExternalObjectReferenceIfPresent(args.m_pNewValueOuterNoRef, ppNewValue);
    }
    else
    {
        IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, args.m_pDP->GetPropertyType(), ppNewValue));
    }

Cleanup:
    return hr;
}
