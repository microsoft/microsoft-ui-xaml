// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
// Contains the basic abstraction for doing work in a thread pool

#ifndef __PAL__WORKITEM__
#define __PAL__WORKITEM__

//------------------------------------------------------------------------
//
// Interface: IPALWorkItem
//
// Synopsis:
//  Represents work to do on a threadpool worker thread
// Usage:
// Create an IPALWorkItem by calling CCoreServices::CreateWorkItem, passing
// in a PALWORKTITEMCALLBACK function pointer and optional IObject data.  
//
// Call Submit to put the work into the work queue.  When the work completes, cancelled or the thread pool is shutdown
// the WorkItem will be derefed.  This means you can Fire & Forget work:  create it, submit it and 
// it will get destroyed when it is finished.
// 
// **WARNING** 
// The current implementation does not properly support any usage pattern other than fire-and-forget.
// If you must hold on to a reference to a WorkItem, take care to do the following:
// 1) You must release your reference before process shutdown.  Otherwise the threadpool cleanup code will release your
//    WorkItem out from under you.  (Fixing this properly involves adding critsection locks to WorkItem which would adversely
//    affect performance.)
// 2) If you need to hold a reference long enough to wait on the WorkItem at an arbitrary time, it is advised that you use
//    a critical section and double-NULL check pattern (see WinTextCore::WaitForInitializeWorkItemCompletion() as an example).
//
// If optional pData is sent to the CreateWorkItem call, this will be ref'd by the work item, so if the caller
// does not hold a ref to the pData, it will also be destroyed when the work completes or is cancelled.
//
// The IObject passed in as pData  must implement thread-safe AddRef/Release through CXcpObjectThreadSafeAddRefPolicy
// or some simlar mechanism.
//------------------------------------------------------------------------
struct IPALWorkItem : public IObject
{
    // Submit will add the work to a queue and ref it. If you ReleaseInterface after Submit
    // the object will be cleaned up when the work is completed or cancelled.
    virtual _Check_return_ HRESULT Submit() = 0;
    virtual _Check_return_ HRESULT Wait(bool cancelPending) = 0; 
    virtual IObject *GetDataNoRef() = 0;    
};

typedef HRESULT (__stdcall *PALWORKITEMCALLBACK)(_In_opt_ IObject *pData);


//------------------------------------------------------------------------
//
//  Synopsis:
//       Used to create work that will call back pfn, passing pData to it.
//      The work must be submitted by the caller (IPALWorkItem::Submit)
//      The work item will ref pData and unref when it is destroyed
//      The IObject passed in as pData  must implement thread-safe AddRef/Release through CXcpObjectThreadSafeAddRefPolicy
//      or some simlar mechanism.   
// ------------------------------------------------------------------------

struct IPALWorkItemFactory : public IObject
{
    virtual _Check_return_ HRESULT CreateWorkItem(
        _Outptr_ IPALWorkItem **ppWork,
        _In_ PALWORKITEMCALLBACK pfn,
        _In_opt_ IObject *pData) = 0;  

    virtual _Check_return_ HRESULT CancelAndCleanupAllWork() = 0;

};
#endif //#ifndef __PAL__WORKITEM__
