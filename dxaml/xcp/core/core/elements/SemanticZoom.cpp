// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SemanticZoom.h"

_Check_return_
HRESULT
CSemanticZoom::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params)
{
    if (!params.fSkipNameRegistration)
    {
        CDependencyObject *pZoomOutView = GetZoomOutView();
        CDependencyObject *pZoomInView = GetZoomInView();

        EnterParams newParams(params);
        newParams.fIsLive = FALSE;

        if (pZoomOutView)
        {
            IFC_RETURN(pZoomOutView->Enter(pNamescopeOwner, newParams));
        }
        if (pZoomInView)
        {
            IFC_RETURN(pZoomInView->Enter(pNamescopeOwner, newParams));
        }
    }

    return CControl::EnterImpl(pNamescopeOwner, params);
}

_Check_return_
HRESULT
CSemanticZoom::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params)
{
    if (!params.fSkipNameRegistration)
    {
        CDependencyObject *pZoomOutView = GetZoomOutView();
        CDependencyObject *pZoomInView = GetZoomInView();

        LeaveParams newParams(params);
        newParams.fIsLive = FALSE;

        if (pZoomOutView)
        {
            IFC_RETURN(pZoomOutView->Leave(pNamescopeOwner, newParams));
        }
        if (pZoomInView)
        {
            IFC_RETURN(pZoomInView->Leave(pNamescopeOwner, newParams));
        }
    }

    return CControl::LeaveImpl(pNamescopeOwner, params);
}

