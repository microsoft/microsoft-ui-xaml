// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Constructor.
//
//------------------------------------------------------------------------------
WindowsPresentTarget::WindowsPresentTarget(
    XUINT32 width,
    XUINT32 height,
    _In_opt_ XHANDLE hTargetWindow,
    _In_opt_ IViewObjectPresentNotifySite *pIPresentNotifySite
    ) : m_width(width)
      , m_height(height)
      , m_hTargetWindow(reinterpret_cast<HWND>(hTargetWindow))
      , m_pIPresentNotifySite(pIPresentNotifySite)
{
    AddRefInterface(m_pIPresentNotifySite);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Destructor.
//
//------------------------------------------------------------------------------
WindowsPresentTarget::~WindowsPresentTarget()
{
    ReleaseInterface(m_pIPresentNotifySite);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      A helper for the static factory methods.
//
//------------------------------------------------------------------------------
/* static */ _Check_return_ HRESULT
WindowsPresentTarget::CreateWorker(
    XUINT32 width,
    XUINT32 height,
    _In_opt_ XHANDLE hTargetWindow,
    _In_opt_ IViewObjectPresentNotifySite *pIPresentNotifySite,
    _Outptr_ WindowsPresentTarget **ppPresentTarget
    )
{
    HRESULT hr = S_OK;

    WindowsPresentTarget *pPresentTarget =
        new WindowsPresentTarget(
            width,
            height,
            hTargetWindow,
            pIPresentNotifySite);


    *ppPresentTarget = pPresentTarget;

    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Statis factory method for creating windowed present targets.
//
//------------------------------------------------------------------------------
/* static */ _Check_return_ HRESULT
WindowsPresentTarget::CreateWindowedPresentTarget(
    XUINT32 width,
    XUINT32 height,
    _In_ XHANDLE hTargetWindow,
    _Outptr_ WindowsPresentTarget **ppPresentTarget
    )
{
    RRETURN(CreateWorker(
        width,
        height,
        hTargetWindow,
        NULL /* pIPresentNotifySite */,
        ppPresentTarget));
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Statis factory method for creating composited windowless present
//      targets that became available in Internet Explorer 9.
//
//------------------------------------------------------------------------------
/* static */ _Check_return_ HRESULT
WindowsPresentTarget::CreateCompositedWindowlessPresentTarget(
    XUINT32 width,
    XUINT32 height,
    _In_ IViewObjectPresentNotifySite *pIPresentNotifySite,
    _Outptr_ WindowsPresentTarget **ppPresentTarget
    )
{
    RRETURN(CreateWorker(
        width,
        height,
        NULL /* hTargetWindow */,
        pIPresentNotifySite,
        ppPresentTarget));
}