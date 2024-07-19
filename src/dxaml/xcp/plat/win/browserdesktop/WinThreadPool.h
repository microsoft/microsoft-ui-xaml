// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Definitions for Windows Thread Pool implementation wrappers

#pragma once

//-------------------------------------------------------------------------------
// 
// Abstracts a thread pool.  Current implementation (9/5/2011) always uses the global thread pool,
// but  different cleanup groups.  Same abstraction could use private pools.
//
//-------------------------------------------------------------------------------
class CWinWorkItemFactory :  public CXcpObjectBase<IPALWorkItemFactory>
{
public:
    CWinWorkItemFactory();
    virtual ~CWinWorkItemFactory();
public:
    _Check_return_ HRESULT CreateWorkItem(
        _Outptr_ IPALWorkItem **pNewItem,
        _In_ PALWORKITEMCALLBACK pfn,
        _In_opt_ IObject *pData);

    // Uses the cleanup group functionality of the windows thread pool.
    // Cancels work that has not started and waits for work that has
    _Check_return_ HRESULT CancelAndCleanupAllWork();

    _Check_return_ HRESULT Initialize();

private:  
    // Internals all Windows threadpool specific.  See threadpool docs.
    TP_CALLBACK_ENVIRON m_threadPoolEnvironment;
    PTP_CLEANUP_GROUP m_pCleanupGroup;

public:
    
    #ifdef DBG
    DWORD m_dwWorkItemCounter;
    #endif


    static VOID CALLBACK 
        CleanupGroupCancelCallback(_Inout_opt_ PVOID pObjectContext, _Inout_opt_ PVOID pCleanupContext);    
};
