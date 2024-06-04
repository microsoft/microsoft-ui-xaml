// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Method:   ctor
//
//  Synopsis:
//      Creates a new ApplicationSingleton object.
//
//------------------------------------------------------------------------
CWinApplicationSingleton::CWinApplicationSingleton(XHANDLE hMutex)
{
    m_hMutex = hMutex;
}

//------------------------------------------------------------------------
//
//  Method:   Release
//
//  Synopsis:
//      Deconstructs an ApplicationSingleton object.
//
//------------------------------------------------------------------------
CWinApplicationSingleton::~CWinApplicationSingleton()
{
// We don't close the handle because we're haven't added a ref to it.
// User code cannot delete the object without releasing the mutex, just
// make sure of it.
    ASSERT(m_hMutex == NULL);
}

//------------------------------------------------------------------------
//
//  Method:   Create
//
//  Synopsis:
//      Gets a reference to an ApplicationSingleton object.
//
//------------------------------------------------------------------------
#pragma warning(push)
#pragma warning(disable: 6387)  /* *ppSingleton is null only when on the eror path, so safe to suppress the warning */
//_Acquires_lock_(*ppSingleton->m_hMutex)
_Check_return_ HRESULT
CWinApplicationSingleton::Create(_Outptr_ IPALApplicationSingleton **ppSingleton)
{
    XHANDLE hSingleton = NULL;

    *ppSingleton = nullptr;

    static XHANDLE s_hApplicationSingleton = NULL;

    auto guard = wil::scope_exit([&ppSingleton, &hSingleton]()
    {
        // Check if ppSingleton was not assigned but mutex was acquired.
        if ((!*ppSingleton) && (hSingleton))
        {
            ReleaseMutex(hSingleton);
        }
    });


    // Always create a mutex, if we are the first then that will be
    // the singleton
    XHANDLE hMutex = CreateMutex(nullptr, false, nullptr);
    IFCOOMFAILFAST(hMutex);

    // Try to set the mutex in the singleton variable, if we don't
    // have one we'll get nullptr as the original value and we'll have
    // the mutex we created will be the singleton, otherwise, the
    // return value is the singleton
    hSingleton = (XHANDLE)InterlockedCompareExchangePointer(
        &s_hApplicationSingleton,
        hMutex,
        NULL
        );

    // If the singleton was nullptr then we've just set it
    if (hSingleton == NULL)
    {
        hSingleton = hMutex;
        hMutex = NULL;
    }
    else
    {
        CloseHandle(hMutex);
        hMutex = NULL;
    }

    XDWORD dwWaitResult = WaitForSingleObject(hSingleton, INFINITE);
    if (dwWaitResult != WAIT_OBJECT_0)
    {
        // If we don't have the singleton, then don't we won't want to release it.
        hSingleton = NULL;
        IFC_RETURN(E_FAIL);
    }

    CWinApplicationSingleton *pWinSingleton = new CWinApplicationSingleton(hSingleton);
    *ppSingleton = (IPALApplicationSingleton *)pWinSingleton;

    return S_OK;
}
#pragma warning(pop)

//------------------------------------------------------------------------
//
//  Method:   Leave
//
//  Synopsis:
//      Release the hold on the ApplicationSingleton, and destroy the
//    object.
//
//------------------------------------------------------------------------
_Success_(this->m_hMutex != NULL)
_Releases_lock_(this->m_hMutex)
XUINT32
CWinApplicationSingleton::Release()
{

    if (m_hMutex)
    {
        ReleaseMutex(m_hMutex);
        m_hMutex = NULL;
    }

    delete this;

    // Since no refcount is maintained just return 0 after release.
    return 0;
}
