// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//    CDirectManipulationFrameInfoProvider class used for listening to
//    DirectManipulation feedback on the compositor thread.

#include "precomp.h"

// Uncomment to get DirectManipulation debug traces
// #define DMC_DBG

//------------------------------------------------------------------------
//
//  Method:   CDirectManipulationFrameInfoProvider::SetDMService
//
//  Synopsis:
//    Sets the owning CDirectManipulationService instance that is interested
//    in handling the DirectManipulation feedback.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
CDirectManipulationFrameInfoProvider::SetDMService(_In_ CDirectManipulationService* pDMService)
{
    HRESULT hr = S_OK;

    IFCPTR(pDMService);
    m_pDMService = pDMService;

Cleanup:
    RRETURN(hr);
}


// IDirectManipulationFrameInfoProvider implementation

//------------------------------------------------------------------------
//
//  Method:   CDirectManipulationFrameInfoProvider::GetNextFrameInfo
//
//  Synopsis:
//    Called when DM needs to be given the time the next frame is going to
//    be shown on the screen.
//
//------------------------------------------------------------------------
IFACEMETHODIMP
CDirectManipulationFrameInfoProvider::GetNextFrameInfo(
    _Out_ XUINT64* pTime,
    _Out_ XUINT64* pProcessTime,
    _Out_ XUINT64* pCompositionTime)
{
    HRESULT hr = S_OK;

#ifdef DMC_DBG
    IGNOREHR(gps->DebugOutputSzNoEndl(L"DMC: CDirectManipulationFrameInfoProvider::GetNextFrameInfo entry. ThreadID=%d\r\n", GetCurrentThreadId()));
#endif // DMC_DBG

    ASSERT(m_pDMService);

    IFCPTR(pTime);
    *pTime = 0;
    IFCPTR(pProcessTime);
    *pProcessTime = 0;
    IFCPTR(pCompositionTime);
    *pCompositionTime = m_pDMService->GetDeltaCompositionTime();

Cleanup:
    RRETURN(hr);
}
