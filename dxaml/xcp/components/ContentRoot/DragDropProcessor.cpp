// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "DragDropProcessor.h"
#include "ContentRoot.h"

#include "paltypes.h"
#include "eventargs.h"

#include "corep.h"

#include "RootVisual.h"

using namespace ContentRootInput;

DragDropProcessor::DragDropProcessor(_In_ CContentRoot& contentRoot)
    : m_contentRoot(contentRoot)
{
    m_dragDropState.reset(new CDragDropState(static_cast<CCoreServices*>(&contentRoot.GetCoreServices()), m_contentRoot.GetCoreServices().GetEventManager()));
}

_Check_return_ HRESULT DragDropProcessor::ProcessDragDrop(
    _In_ DragMsg *pMsg,
    _In_ bool bRequireMouseMoveForDragOver,
    _In_ bool bRaiseSync,
    _Out_ bool *handled)
{
    xref_ptr<CDependencyObject> pDODrag;
    xref_ptr<CDragEventArgs> pDragArgs;
    XPOINTF pointDrag;
    bool doDragOver = false;

    // Initialize handled
    *handled = FALSE;

    IFC_RETURN(CDragEventArgs::Create(&m_contentRoot.GetCoreServices(), pDragArgs.ReleaseAndGetAddressOf()));

    // create a point representing the hittest area
    pointDrag.x = static_cast<XFLOAT>(pMsg->m_xPos);
    pointDrag.y = static_cast<XFLOAT>(pMsg->m_yPos);

    if (pMsg->m_msgID != XCP_DRAGLEAVE)
    {
        IFC_RETURN(m_contentRoot.GetInputManager().GetPointerInputProcessor().HitTestHelper(pointDrag, m_contentRoot.GetVisualTreeNoRef()->GetRootVisual(), pDODrag.ReleaseAndGetAddressOf()));
    }

    // Tell the DragEventArgs object the global position so it can calculate
    //  the local position later in GetPosition().
    pDragArgs->SetGlobalPoint(pointDrag);

    // set the Source value
    IFC_RETURN(pDragArgs->put_Source(pDODrag));

    // Call raise event AND reset handled to false if this is not ours
    *handled = TRUE;

    // compare DO to the existing do we store if it matches then do nothing, else  fire the drag leave on
    // old DO and drag enter on the new DO
    if (m_dragEnterDO.get() != pDODrag)
    {
        IFC_RETURN(ProcessDragEnterLeave(pDODrag, pDragArgs, FALSE, pointDrag, bRaiseSync));
    }

    switch (pMsg->m_msgID)
    {
    case XCP_DROP:
    {
        if (pMsg->m_filePathSize > 0)
        {
            IFC_RETURN(xstring_ptr::CloneBuffer(pMsg->m_filePaths, pMsg->m_filePathSize, &(pDragArgs->m_strFilePaths)));
        }
        pDragArgs->m_bAllowDataAccess = TRUE;
        pDragArgs->SetDragDropEventType(DirectUI::DragDropMessageType::Drop);
        m_contentRoot.GetCoreServices().GetEventManager()->RaiseRoutedEvent(
            EventHandle(KnownEventIndex::UIElement_Drop),
            m_dragEnterDO.get(),
            pDragArgs,
            false /* bIgnoreVisibility */,
            bRaiseSync,
            true /* fInputEvent */);
        m_dragEnterDO = nullptr;
        break;
    }
    case XCP_DRAGOVER:
        // doDragOver is initialized to FALSE
        if (bRequireMouseMoveForDragOver)
        {
            if ((m_xMousePosLast != pMsg->m_xPos)
                || (m_yMousePosLast != pMsg->m_yPos))
            {
                // store new co-ordinates
                m_xMousePosLast = pMsg->m_xPos;
                m_yMousePosLast = pMsg->m_yPos;
                doDragOver = TRUE;
            }
        }
        else
        {
            doDragOver = TRUE;
        }

        if (doDragOver)
        {
            pDragArgs->m_bAllowDataAccess = FALSE;
            pDragArgs->SetDragDropEventType(DirectUI::DragDropMessageType::DragOver);

            m_contentRoot.GetCoreServices().GetEventManager()->RaiseRoutedEvent(
                EventHandle(KnownEventIndex::UIElement_DragOver),
                m_dragEnterDO.get(), pDragArgs,
                false /* bIgnoreVisibility */,
                bRaiseSync,
                true /* fInputEvent */);
        }
        break;
    case XCP_DRAGENTER:
    case XCP_DRAGLEAVE:
        break;
    default:
        *handled = FALSE;
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

_Check_return_ HRESULT DragDropProcessor::ProcessDragEnterLeave(
    _In_ CDependencyObject *pNewElement,
    _In_opt_ CEventArgs *pArgs,
    _In_ XINT32 bSkipLeave,
    _In_ XPOINTF xp,
    _In_ bool bRaiseSync)
{
    CDependencyObject *pVisual = NULL;
    xref_ptr<CDependencyObject> pReleaseVisual;
    xref_ptr<CDragEventArgs> pDragLeaveArgs;
    xref_ptr<CDragEventArgs> pDragEnterArgs;
    BOOLEAN handled = FALSE;

    IFC_RETURN(CDragEventArgs::Create(&m_contentRoot.GetCoreServices(), pDragEnterArgs.ReleaseAndGetAddressOf()));

    // Once you are in the control then we want to go between elements without spamming elements
    // that already have enter with new enter events
    // This is a four pass algorithm that has the complexity of 4 * depth of tree

    // Pass 1: Set the leave and dirty bits on all elements in the parent chain
    pVisual = m_dragEnterDO.get();
    while (pVisual)
    {
        pVisual->SetDragInputNodeDirty(TRUE);
        pVisual->SetDragEnter(FALSE);
        pVisual = pVisual->GetParentInternal();
    }

    // Pass 2 set the enter on the elements up to the tree root
    pVisual = static_cast<CDependencyObject*>(pNewElement);
    while (pVisual)
    {
        pVisual->SetDragEnter(TRUE);
        pVisual = pVisual->GetParentInternal();
    }

    // Pass 3 : now fire events for leave until we hit first element with drag enter set
    pVisual = m_dragEnterDO.get();
    while (pVisual && !pVisual->HasDragEnter())
    {
        pVisual->SetDragInputNodeDirty(FALSE);

        if (static_cast<CUIElement*>(pVisual)->IsHitTestVisible() && !bSkipLeave && !handled)
        {
            IFC_RETURN(CDragEventArgs::Create(&m_contentRoot.GetCoreServices(), pDragLeaveArgs.ReleaseAndGetAddressOf()));
            IFC_RETURN(pDragLeaveArgs->put_Source(pVisual));
            pDragLeaveArgs->SetDragDropEventType(DirectUI::DragDropMessageType::DragLeave);
            pDragLeaveArgs->SetGlobalPoint(xp);
            pDragLeaveArgs->m_bAllowDataAccess = FALSE;

            // AddRef before, and Release after calling out to script so that no
            // tree changes will cause pVisual to be deleted.
            pReleaseVisual = pVisual;

            m_contentRoot.GetCoreServices().GetEventManager()->Raise(
                EventHandle(KnownEventIndex::UIElement_DragLeave),
                true,
                pVisual,
                pDragLeaveArgs,
                bRaiseSync,
                true /* fInputEvent */);

            IFC_RETURN(pDragLeaveArgs->get_Handled(&handled));
        }
        pVisual = pVisual->GetParentInternal();
    }

    // store the new element that can be hittested
    m_dragEnterDO = static_cast<CDependencyObject*>(pNewElement);

    pVisual = m_dragEnterDO.get();

    handled = FALSE;

    // Pass 4: Now walk up the tree
    //          A) firing enter events until we hit the first guy that has dirty bit set
    //             and from there reset the dirty bit
    //          B) Finding the lowest element that has the dragenter set.
    while (pVisual)
    {
        if (pVisual->HasDragEnter()
            && !pVisual->IsDragInputNodeDirty()
            && !handled)
        {
            if (static_cast<CUIElement*>(pVisual)->IsHitTestVisible())
            {
                // AddRef before, and Release after calling out to script so that no
                // tree changes will cause pVisual to be deleted.
                pReleaseVisual = pVisual;
                pDragEnterArgs->SetGlobalPoint(xp);
                pDragEnterArgs->m_bAllowDataAccess = FALSE;
                pDragEnterArgs->SetDragDropEventType(DirectUI::DragDropMessageType::DragEnter);
                m_contentRoot.GetCoreServices().GetEventManager()->Raise(
                    EventHandle(KnownEventIndex::UIElement_DragEnter),
                    true,
                    pVisual,
                    pDragEnterArgs,
                    bRaiseSync,
                    true /* fInputEvent */);

                IFC_RETURN(pDragEnterArgs->get_Handled(&handled));
            }
        }
        else if (pVisual->IsDragInputNodeDirty())
        {
            pVisual->SetDragInputNodeDirty(FALSE);
        }

        pVisual = pVisual->GetParentInternal();
    }

    return S_OK;
}

// Raise drag enter, over, leave, and drop events based on a drag message.
_Check_return_ HRESULT DragDropProcessor::ProcessWinRtDragDrop(
    _In_ DragMsg* msg,
    _In_ IInspectable* winRtDragInfo,
    _In_opt_ IInspectable* dragDropAsyncOperation,
    _Inout_opt_ DirectUI::DataPackageOperation* acceptedOperation,
    _In_opt_ CDependencyObject* hitTestRoot)
{
    if (!m_dragDropState->IsDeferred())
    {
        //initialize the state
        xref_ptr<CDependencyObject> spDODrag;
        XPOINTF pointDrag;

        // create a point representing the hittest area
        pointDrag.x = static_cast<XFLOAT>(msg->m_xPos);
        pointDrag.y = static_cast<XFLOAT>(msg->m_yPos);

        // We don't need to hittest when we are leaving because the leave events only go to
        // DOs that previously had an enter.  In addition, we don't hittest on a drop, relying
        // on the last drag over.  When we drop, we get a drag over and then a drop.  However,
        // because of independent animations, it is possible that the item could have moved
        // between these two messages.  So we will treat them as if they came from the same
        // place.
        if (msg->m_msgID == XCP_DROP)
        {
            spDODrag = m_dragEnterDO;
        }
        else if (msg->m_msgID != XCP_DRAGLEAVE)
        {
            if (nullptr == hitTestRoot)
            {
                hitTestRoot = m_contentRoot.GetVisualTreeNoRef()->GetRootVisual();
            }

            IFC_RETURN(m_contentRoot.GetInputManager().GetPointerInputProcessor().HitTestWithLightDismissAwareness(spDODrag, pointDrag, msg->m_msgID, nullptr /* pointerInfo */, hitTestRoot));
        }

        if (m_dragEnterDO == nullptr && spDODrag == nullptr)
        {
            // When dragging from outside the window, hit-test returns null because the offset is just at the edge of the window
            // in this case, return immediately, DragEnter will be raised when OverAsync calls
            return S_OK;
        }

        // cache the state
        IFC_RETURN(m_dragDropState->CacheInitialState(
            msg,
            pointDrag,
            m_dragEnterDO.get(), // old drag DO
            spDODrag.get(),      // new drag DO
            winRtDragInfo,
            dragDropAsyncOperation,
            (acceptedOperation != nullptr) ? *acceptedOperation : DirectUI::DataPackageOperation::DataPackageOperation_None));

        // store the new element that can be hittested
        m_dragEnterDO = spDODrag;
    }
    else if (acceptedOperation != nullptr)
    {
        // We have to update the accepted operation in case it has been set asynchronously
        m_dragDropState->SetAcceptedOperation(*acceptedOperation);
    }

    IFC_RETURN(m_dragDropState->RaiseEvents(acceptedOperation));

    if (!m_dragDropState->IsDeferred() && ((msg->m_msgID == XCP_DROP) || (msg->m_msgID == XCP_DRAGLEAVE)))
    {
        m_dragEnterDO = nullptr;
    }

    if ((msg->m_msgID == XCP_DRAGLEAVE) || (msg->m_msgID == XCP_DROP))
    {
        m_dragDropState->ResetEnterStack();
    }

    return S_OK;
}
