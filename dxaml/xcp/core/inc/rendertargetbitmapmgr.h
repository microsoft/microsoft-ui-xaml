// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation
//
//  Abstract:
//
//      Definition of CRenderTargetBitmapManager object
//
//------------------------------------------------------------------------

//------------------------------------------------------------------------
//
//  Class:  CRenderTargetBitmapManager
//
//  Synopsis:
//      CRenderTargetBitmapManager class
//
//------------------------------------------------------------------------

class CCoreServices;
class CRenderTargetBitmap;
class IRenderTargetBitmapGetPixelsAsyncOperation;
struct IRenderTargetElement;
struct IRenderTargetBitmapExecutorControl;

//------------------------------------------------------------------------
//
//  Synopsis:
//      Struct for holding the information needed to manage
//      all the RenderAsync waits.
//
//------------------------------------------------------------------------
struct RenderTargetElementWaitData
{
    // The event upon which we are waiting
    IPALWaitable *m_pWaitHandleNoRef;

    // The end marker for the batch of RTEs waiting
    // on this event in the bigger list of all the RTEs
    // waiting for draw.
    IRenderTargetElement *m_pRenderTargetElementNoRef;

    RenderTargetElementWaitData()
    {
        m_pWaitHandleNoRef = NULL;
        m_pRenderTargetElementNoRef = NULL;
    }
};

//------------------------------------------------------------------------
//
//  Synopsis:
//      Acts as a facade for all the RenderTargetBitmap based operations
//      needed by CCoreServices.
//
//------------------------------------------------------------------------
class CRenderTargetBitmapManager : public CXcpObjectBase<IObject>
{
public:
    static _Check_return_ HRESULT Create(_In_ CCoreServices *pCoreService, _Outptr_ CRenderTargetBitmapManager **ppRenderTargetBitmapManager );
    _Check_return_ HRESULT PickupForRender();
    _Check_return_ HRESULT PreCommit(
        _In_ CWindowRenderTarget* renderTarget);
    _Check_return_ HRESULT RenderElements(
        _In_ HWWalk *pHWWalk,
        _In_ CWindowRenderTarget* pIRenderTarget,
        _Out_ bool *pHasPendingDraws);
    _Check_return_ HRESULT DrawCompTrees(
        _In_ CWindowRenderTarget *pRenderTarget);
    _Check_return_ HRESULT NotifyDrawCompleted(_In_ IPALWaitable *pWaitHandle);
    _Check_return_ HRESULT SubmitGetPixelsWorkItem(
        _In_ IPALByteAccessSurface *pByteAccessSurface,
        _In_ IRenderTargetBitmapGetPixelsAsyncOperation *pAsyncOperation,
        _In_ IRenderTargetBitmapExecutorControl *pRenderTargetBitmapExecutorControl);
    _Check_return_ HRESULT CleanupDeviceRelatedResources(_In_ bool cleanupDComp);
    _Check_return_ HRESULT CheckForLostSurfaceContent();

    _Check_return_ HRESULT OnSetCurrentState(_In_ IRenderTargetElement *pRenderTargetElement);

    bool NeedsSurfaceContentsLost() const { return m_needsSurfaceContentsLost; }
    void ResetNeedsSurfaceContentsLost() { m_needsSurfaceContentsLost = FALSE; }

    // Calculate the sizes of the RTBs to render
    _Check_return_ HRESULT UpdateMetrics();

    int CountElementJobs(_In_ CUIElement* uiElement);

private:

    CRenderTargetBitmapManager(_In_ CCoreServices *pCore);
    ~CRenderTargetBitmapManager() override;

    bool CanPickupForRender(_In_ CUIElement *pElement);
    _Check_return_ HRESULT RemoveRenderTargetElement(_In_ IRenderTargetElement *pItem);
    _Check_return_ HRESULT RemoveFromDrawingLists(_In_ IRenderTargetElement *pItem);

    _Check_return_ HRESULT CleanupDeviceRelatedResourcesHelper(
        bool cleanupDiscardedOnly,
        bool cleanupDComp);
    _Check_return_ HRESULT CleanupDeviceRelatedResourcesOnList(
        _In_opt_ CXcpList<IRenderTargetElement> *pList,
        bool addToPendingList,
        bool cleanList,
        bool setNeedsContentsLost,
        bool cleanupDiscardedOnly,
        bool cleanupDComp);

    _Check_return_ HRESULT CleanupResourcesForBackgroundTask();

    void EnsureWaitList();

    _Ret_notnull_ RenderTargetElementWaitData* AddWaitItem(
        _In_opt_ IRenderTargetElement* renderTargetElement,
        _In_ IPALWaitable* waitableEvent
        );

    _Check_return_ HRESULT SubmitWaitWorkItem(_In_ IPALWaitable* waitableEvent);

    static _Check_return_ HRESULT WorkCallback(_In_opt_ IObject *pData);

    template<class T>
    static void IterateList(
        _In_opt_ CXcpList<IRenderTargetElement>* elementList,
        T&& callbackFunc
        );

    // Following are the global lists for RTBs. RTBs exist in one of the list
    // depending upon their status. When a TDR happens or the hardware
    // resources are to be cleaned up for any reason, we travese these
    // lists and get rid of the surfaces. RTBs can exist with hardware resources
    // even when they are outside the tree. And hence we even have a list
    // (m_pIdleWithResourcesListNoRef) for those which are not actively
    // rendering element tree into them. Of course such RTBs with no
    // hardware resources are not kept in this list.

    // Holds the RTBs for which Render has been requested and
    // is yet to be processed.
    CXcpList<IRenderTargetElement> *m_pPendingListNoRef;

    // Holds the RTBs which are picked up for Render from
    // with in a tick.
    CXcpList<IRenderTargetElement> *m_pRenderingListNoRef;

    // Holds the RTBs which have been submitted to DComp for draw.
    CXcpList<IRenderTargetElement> *m_pDrawingListNoRef;

    // Holds the wait information for RTBs which have been submitted
    // to DComp for draw.
    CXcpList<RenderTargetElementWaitData> *m_pDrawWaitDataList;

    // Holds the RTBs which are idle but have hardware resources.
    CXcpList<IRenderTargetElement> *m_pIdleWithResourcesListNoRef;

    CCoreServices *m_pCoreNoRef;

    bool m_needsSurfaceContentsLost;
}; // CRenderTargetBitmapManager class


