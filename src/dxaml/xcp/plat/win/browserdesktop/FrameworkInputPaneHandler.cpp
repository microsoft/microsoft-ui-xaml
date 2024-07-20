// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"


_Check_return_ HRESULT
CFrameworkInputPaneHandler::Showing(
    _In_ RECT *pOccludedRectangle,
    _In_ BOOL ensureFocusedElementInView)
{
    HRESULT hr = S_OK;
    XRECTF OccludedBounds;

    IFCPTR(pOccludedRectangle);

    OccludedBounds.X      = static_cast<XFLOAT>(pOccludedRectangle->left);
    OccludedBounds.Y      = static_cast<XFLOAT>(pOccludedRectangle->top);
    OccludedBounds.Width  = static_cast<XFLOAT>(pOccludedRectangle->right - pOccludedRectangle->left);
    OccludedBounds.Height = static_cast<XFLOAT>(pOccludedRectangle->bottom - pOccludedRectangle->top);

    ASSERT(m_pInputPaneHandler);
    IFC(m_pInputPaneHandler->Showing(&OccludedBounds, ensureFocusedElementInView));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
CFrameworkInputPaneHandler::Hiding(
    _In_ BOOL ensureFocusedElementInView)
{
    ASSERT(m_pInputPaneHandler);
    RRETURN(m_pInputPaneHandler->Hiding(ensureFocusedElementInView));
}

_Check_return_ HRESULT
CFrameworkInputPaneHandler::HidingWithEditFocusRemoval(
    _In_ BOOL ensureFocusedElementInView)
{
    ASSERT(m_pInputPaneHandler);
    IFC_RETURN(m_pInputPaneHandler->Hiding(ensureFocusedElementInView));
    IFC_RETURN(m_pInputPaneHandler->NotifyEditFocusRemoval());

    return S_OK;
}

_Check_return_ HRESULT
CFrameworkInputPaneHandler::HidingWithEditControlNotify(
    _In_ BOOL ensureFocusedElementInView)
{
    ASSERT(m_pInputPaneHandler);
    IFC_RETURN(m_pInputPaneHandler->Hiding(ensureFocusedElementInView));
    IFC_RETURN(m_pInputPaneHandler->NotifyEditControlInputPaneHiding());

    return S_OK;
}

_Check_return_ HRESULT
CFrameworkInputPaneHandler::SetInputPaneHandler(
    _In_ IXcpInputPaneHandler* pInputPaneHandler)
{
    HRESULT hr = S_OK;

    IFCPTR(pInputPaneHandler);

    m_pInputPaneHandler = pInputPaneHandler;

Cleanup:
    RRETURN(hr);
}
