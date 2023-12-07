// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Pointer.h"
#include "PointerCollection.h"
#include "PointerEventArgs.h"
#include <Microsoft.DirectManipulation.h>
#include "DragEventArgs.h"
#include "EventMgr.h"
#include <DMDeferredRelease.h>
#include <unordered_map>
#include <vector_map.h>
#include "Timer.h"
#include "TimeSpan.h"
#include "KeyTipManager.h"
#include "InteractionManager.h"
#include "FrameworkInputViewHandler.h"
#include "TextInputProducerHelper.h"

enum MouseCursor : uint8_t;

// Jupiter bug 103446 - change from a UI tick count to a millisecond count.
// DirectManipulation containers raise notifications for the input manager in case a DM
// characteristic changes during the 200 UI ticks following an input message processing by DM.
#define UITicksThresholdForNotifications 200

class CKeyEventArgs;
class CDMCrossSlideViewport;
class CDMContent;
class CDMViewport;
class CDMCViewport;
class VisualTree;
class CPointer;
class CContentRoot;
class DirectManipulationServiceSharedState;

class CInputManager;


class CDragDropState
{
public:
    CDragDropState(
        _In_ CCoreServices* coreServices,
        _In_ CEventManager* eventManager)
        : m_coreServicesNoRef(coreServices)
        , m_eventManager(eventManager)
    {
    }

    ~CDragDropState()
    {
        VERIFYHR(ClearCache(true /*clearDragEnterEventArgsToo*/));
    }

    _Check_return_ HRESULT CacheInitialState(
        _In_ DragMsg* dragMsg,
        _In_ XPOINTF pointDrag,
        _In_opt_ CDependencyObject* oldDragDO,
        _In_opt_ CDependencyObject* newDragDO,
        _In_ IInspectable* winRtDragInfo,
        _In_opt_ IInspectable* dragDropAsyncOperation,
        _In_ DirectUI::DataPackageOperation acceptedOperation);

    _Check_return_ HRESULT RaiseEvents(_Out_opt_ DirectUI::DataPackageOperation* acceptedOperation);

    _Check_return_ HRESULT RaiseDragLeaveEvents();

    _Check_return_ HRESULT RaiseDragEnterEvents();

    _Check_return_ HRESULT RaiseDragOverOrDropEvents();

    _Check_return_ HRESULT ClearCache(bool clearDragEnterEventArgsToo);

    void ResetEnterStack();

    bool IsDeferred() const {
        return m_dragDO != nullptr;
    }

    void SetAcceptedOperation(DirectUI::DataPackageOperation acceptedOperation)
    {
        m_acceptedOperation = acceptedOperation;
    }

private:
    // We are owned by the input manager, which holds a noref pointer
    // to core services, so we can hold a noref pointer as well.  This
    // prevents a circular reference of Core Services -> Input Manager ->
    // CDragDropState -> Core Services.
    CCoreServices* m_coreServicesNoRef;

    xref_ptr<CDependencyObject> m_dragDO;
    xref_ptr<CDependencyObject> m_nextDragDO;
    xref_ptr<CDragEventArgs> m_dragEnterEventArgs;
    xref_ptr<CDragEventArgs> m_dragEventArgs;
    xref_ptr<CEventManager> m_eventManager;
    xref_ptr<IInspectable> m_winRtDragInfo;
    xref_ptr<IInspectable> m_dragDropAsyncOperation;
    DirectUI::DragDropMessageType m_dragType;
    DirectUI::DragDropMessageType m_nextDragType;
    DirectUI::DataPackageOperation m_acceptedOperation = DirectUI::DataPackageOperation::DataPackageOperation_None;
    BOOLEAN m_handled = FALSE;
    XPOINTF m_dragPoint;

    // When the tree is changed during D&D operations, we need to be able to clear
    // some flags on the DO on which we have set them
    std::vector<xref::weakref_ptr<CDependencyObject>> m_enterStack;
};

class CPointerState
{
    public:
        CPointerState(
            XHANDLE hWindow,
            XINT32 pointerId,
            XPointerInputType pointerInputType
            )
        {
            m_hWindow = hWindow;
            m_pointerId = pointerId;
            m_pointerInputType = pointerInputType;
            m_bPointerDown = false;
            m_ptLastPosition.x = 0;
            m_ptLastPosition.y = 0;
            m_pPointerEnterDO = NULL;
            m_pPointerCaptureDO = NULL;
            m_pPointer = NULL;
            m_bPointerCanceled = false;
            m_bPointerCaptureDenied = false;
        }
        ~CPointerState()
        {
            Reset();
        }
        CPointerState(const CPointerState&) = delete;
        CPointerState(CPointerState&& other) = delete;

        void Reset()
        {
            IGNOREHR(SetEnterDO(nullptr));
            IGNOREHR(SetCaptureDO(nullptr));
            IGNOREHR(SetCapturePointer(nullptr));

        }

        HRESULT SetEnterDO(CDependencyObject *pdo)
        {
            if (m_pPointerEnterDO)
            {
                m_pPointerEnterDO->UnpegManagedPeer(true /* isShutdownException */);
                ReleaseInterface(m_pPointerEnterDO);
            }
            m_pPointerEnterDO = pdo;
            if (m_pPointerEnterDO)
            {
                m_pPointerEnterDO->AddRef();

                // Use app-specific behavior.
                bool isPeerAvailable = false;
                bool pegged = false;
                IFC_RETURN(m_pPointerEnterDO->TryEnsureManagedPeer(
                    &isPeerAvailable,
                    false, // fPegNoRef
                    true,  // fPegRef
                    true, // isShutdownException
                    &pegged));
            }
            return S_OK;
        }
        CDependencyObject *GetEnterDO()
        {
            return m_pPointerEnterDO;
        }

        HRESULT SetCaptureDO(CDependencyObject *pdo)
        {
            if (m_pPointerCaptureDO)
            {
                m_pPointerCaptureDO->UnpegManagedPeer(TRUE /* isShutdownException */);
                ReleaseInterface(m_pPointerCaptureDO);
            }
            m_pPointerCaptureDO = pdo;
            if (m_pPointerCaptureDO)
            {
                m_pPointerCaptureDO->AddRef();
                IFC_RETURN(m_pPointerCaptureDO->PegManagedPeer( TRUE /* isShutdownException */ ));
            }
            return S_OK;
        }
        CDependencyObject *GetCaptureDO()
        {
            return m_pPointerCaptureDO;
        }

        HRESULT SetCapturePointer(CPointer* pPointer)
        {
            ReplaceInterface(m_pPointer, pPointer);
            RRETURN(S_OK);
        }
        CPointer* GetCapturePointer()
        {
            return m_pPointer;
        }

        void SetLastPosition(XPOINTF pointLast)
        {
            m_ptLastPosition.x = pointLast.x;
            m_ptLastPosition.y = pointLast.y;
        }
        XPOINTF GetLastPosition()
        {
            return m_ptLastPosition;
        }

        void SetPointerDown(bool bPointerDown)
        {
            m_bPointerDown = bPointerDown;
        }
        bool IsPointerDown()
        {
            return m_bPointerDown;
        }

        void SetWindowHandle(_In_ XHANDLE hWindow)
        {
            m_hWindow = hWindow;
        }
        XHANDLE GetWindowHandle()
        {
            return m_hWindow;
        }

        XINT32 GetPointerId()
        {
            return m_pointerId;
        }

        XPointerInputType GetPointerInputType()
        {
            return m_pointerInputType;
        }

        void SetPointerCanceled(bool bPointerCanceled)
        {
            m_bPointerCanceled = bPointerCanceled;
        }
        bool IsPointerCanceled()
        {
            return m_bPointerCanceled;
        }

        void SetPointerCaptureDenied(bool bPointerCaptureDenied)
        {
            m_bPointerCaptureDenied = bPointerCaptureDenied;
        }
        bool IsPointerCaptureDenied()
        {
            return m_bPointerCaptureDenied;
        }

    public:
        CDependencyObject*      m_pPointerEnterDO;
        CDependencyObject*      m_pPointerCaptureDO;
        CPointer*               m_pPointer;
        XHANDLE                 m_hWindow;
        XINT32                  m_pointerId;
        XPOINTF                 m_ptLastPosition;
        XPointerInputType       m_pointerInputType;
        bool                    m_bPointerDown;
        bool                    m_bPointerCanceled;
        bool                    m_bPointerCaptureDenied;
};

class CPointerExitedState
{
public:
    CPointerExitedState(_In_ XINT32 pointerId)
    {
        m_pointerId = pointerId;
        m_pPointerEnteredDO = NULL;
        m_pPointerExitedDO = NULL;
    }

    ~CPointerExitedState()
    {
        IGNOREHR(SetEnteredDO(NULL));
        IGNOREHR(SetExitedDO(NULL));
        for (auto& peggedPointerExitedDOWeakRef : m_peggedPointerExitedDOs)
        {
            xref_ptr<CDependencyObject> peggedPointerExitedDO = peggedPointerExitedDOWeakRef.lock();
            if (peggedPointerExitedDO)
            {
                peggedPointerExitedDO->UnpegManagedPeer(TRUE /* isShutdownException */);
            }
        }
    }

    HRESULT SetEnteredDO(_In_opt_ CDependencyObject *pdo)
    {
        if(m_pPointerEnteredDO)
        {
            m_pPointerEnteredDO->UnpegManagedPeer(TRUE /* isShutdownException */);
            ReleaseInterface(m_pPointerEnteredDO);
        }
        m_pPointerEnteredDO = pdo;
        if(m_pPointerEnteredDO)
        {
            m_pPointerEnteredDO->AddRef();
            IFC_RETURN(m_pPointerEnteredDO->PegManagedPeer( TRUE /* isShutdownException */ ));
        }
        return S_OK;
    }
    CDependencyObject *GetEnteredDONoRef()
    {
        return m_pPointerEnteredDO;
    }

    HRESULT SetExitedDO(_In_opt_ CDependencyObject *pdo)
    {
        if(m_pPointerExitedDO)
        {
            m_pPointerExitedDO->UnpegManagedPeer(TRUE /* isShutdownException */);
            ReleaseInterface(m_pPointerExitedDO);
        }
        m_pPointerExitedDO = pdo;
        if(m_pPointerExitedDO)
        {
            m_pPointerExitedDO->AddRef();
            IFC_RETURN(m_pPointerExitedDO->PegManagedPeer( TRUE /* isShutdownException */ ));
        }
        return S_OK;
    }
    CDependencyObject *GetExitedDONoRef()
    {
        return m_pPointerExitedDO;
    }

    XINT32 GetPointerId()
    {
        return m_pointerId;
    }

    std::vector<xref::weakref_ptr<CDependencyObject>>* GetPeggedPointerExitedDOs()
    {
        return &m_peggedPointerExitedDOs;
    }

public:
    XINT32                          m_pointerId;
    CDependencyObject*              m_pPointerEnteredDO;
    CDependencyObject*              m_pPointerExitedDO;
    std::vector<xref::weakref_ptr<CDependencyObject>>  m_peggedPointerExitedDOs;
};

struct PointerExitedStateKey
{
    XINT32                          m_pointerId;
    CDependencyObject*              m_pointerEnteredDO;
};

// Provider for the Pointer Exited State Key
template<>
struct DataStructureFunctionProvider<PointerExitedStateKey>
{
    static XUINT32 Hash(const PointerExitedStateKey& data)
    {
        return DataStructureFunctionProvider<void*>::Hash(data.m_pointerEnteredDO) + data.m_pointerId;
    }

    static bool AreEqual(const PointerExitedStateKey& lhs, const PointerExitedStateKey& rhs)
    {
        return lhs.m_pointerId == rhs.m_pointerId && lhs.m_pointerEnteredDO == rhs.m_pointerEnteredDO;
    }
};

//
//      Synchronous Input events are fired without taking the reentrancy guard,
//      to allow the application's input event handlers to call API like the
//      following, which pump messages and cause reentrancy:
//          CoreDispatcher.ProcessEvents
//          CoWaitForMultipleHandles
//
//      For example, tick or other input events may be pumped while InputManager
//      is in a synchronous callout. So InputManager needs to be hardened against
//      reentrancy. It should ensure that objects are alive and that state is
//      re-validated after these callouts return.
//
class CInputServices final
{
public:
    friend class CInputManager;

    XUINT32 AddRef();
    XUINT32 Release();

    void SetCoreWindow(_In_ wuc::ICoreWindow* pCoreWindow);
    wuc::ICoreWindow* GetCoreWindow() const;

    void SetApplicationHwnd(_In_ HWND hWnd);

    void SetLastInputDeviceType(XPointerInputType deviceType)
    {
        m_lastInputDeviceType = deviceType;
    }

    _Check_return_ HRESULT ProcessInput(
        _In_ InputMessage *pMsg,
        _In_ CContentRoot* contentRoot,
        _Out_ XINT32 *handled);

    _Check_return_ HRESULT ProcessTouchInteractionCallback(
        _In_ const xref_ptr<CUIElement> &element,
        _In_ TouchInteractionMsg *message);

    void ProcessManipulationInertiaInteraction();

    // Register/UnRegister DM Viewports on PointerDown/Up
    _Check_return_ HRESULT RegisterDMViewportsOnPointerDown(_In_ XUINT32 pointerId, _In_ bool fIsForDMHitTest, _In_ CUIElement* pPointedElement);
    _Check_return_ HRESULT UnRegisterDMViewportsOnPointerUp(_In_ XUINT32 pointerId);

    // Called when the focus is changed from the focus manager
    static BOOL IsTextEditableControl(_In_ const CDependencyObject* const pObject);

    void NotifyWindowDestroyed(_In_ XHANDLE hDestroyedWindow);

    // Returns True when:
    // - the provided UIElement is the manipulated element of a DManip viewport
    // - that viewport belongs to a regular ScrollViewer or a touch-manipulatable root ScrollViewer
    bool RequiresViewportInteraction(_In_ CUIElement* manipulatedElement);

    // Called on each UI thread tick to process possible DirectManipulation updates
    _Check_return_ HRESULT ProcessUIThreadTick();

    // Called after the UI thread tick has processed a frame.
    _Check_return_ HRESULT OnPostUIThreadTick();

    // Called by UIElement_SetIsDirectManipulationContainer when an UIElement declares itself as a CUIDMContainer
    // supporter. Initializes the provided element or puts it into a waiting queue for future initialization.
    _Check_return_ HRESULT RegisterDirectManipulationContainer(
        _In_ CUIElement* pDMContainer,
        _In_ bool fIsDirectManipulationContainer);

    // Called by UIElement_SetIsDirectManipulationCrossSlideContainer when an UIElement declares itself as an element that
    // requires a cross-slide DM viewport in order to get a chance to recognize its own gestures/manipulations.
    // fIsDirectManipulationCrossSlideContainer is False when the element no longer needs a cross-slide viewport.
    _Check_return_ HRESULT RegisterDirectManipulationCrossSlideContainer(
        _In_ CUIElement* pDMCrossSlideContainer,
        _In_ bool fIsDirectManipulationCrossSlideContainer);

    // Called by UIElement_DirectManipulationCrossSlideContainerCompleted when the provided UIElement no longer requires a cross-slide
    // viewport. This may be because a DM manipulation needs to start for the owning ScrollViewer.
    _Check_return_ HRESULT DirectManipulationCrossSlideContainerCompleted(
        _In_ CUIElement* pDMCrossSlideContainer,
        _In_opt_ CDMCrossSlideViewport* pCrossSlideViewport);

    // Called by CInputManagerDMViewportEventHandler to forward DirectManipulation's notification of viewport status change
    _Check_return_ HRESULT ProcessDirectManipulationViewportStatusUpdate(
        _In_ CDMViewport* pViewport,
        _In_ XDMViewportStatus oldStatus,
        _In_ XDMViewportStatus newStatus);

    // Called by CInputManagerDMViewportEventHandler to forward DirectManipulation's notification of viewport interaction type change
    _Check_return_ HRESULT ProcessDirectManipulationViewportInteractionTypeUpdate(
        _In_ CDMViewport* pViewport,
        _In_ XDMViewportInteractionType newInteractionType);

    // Called by ProcessDirectManipulationViewportChanges during a UI thread tick to raise potentially queued up DirectManipulationStarted
    // and DirectManipulationCompleted events for the provided viewport.
    _Check_return_ HRESULT RaiseQueuedDirectManipulationStateChanges(
        _In_ CDMViewport* pViewport,
        _In_ bool isValuesChangePreProcessing,
        _In_ bool isValuesChangePostProcessing);

    // Called by CInputManagerDMViewportEventHandler to forward DirectManipulation's notification of dragging status changed
    _Check_return_ HRESULT ProcessDirectManipulationViewportDraggingStatusChange(
        _In_ CDMCrossSlideViewport* pCrossSlideViewport,
        _In_ DIRECTMANIPULATION_DRAG_DROP_STATUS current,
        _In_ DIRECTMANIPULATION_DRAG_DROP_STATUS previous);

    // Called by the UI thread in ::NWDraw to become aware of new DirectManipulation-driven manipulations.
    // Or all existing manipulations when fReturnAllActiveViewports is True.
    _Check_return_ HRESULT GetDirectManipulationViewports(
        _In_ bool fReturnAllActiveViewports,
        _Out_ xvector<CDMCViewport*>& compositorViewports);

    // Called by the UI thread's ::NWDraw to become aware of DirectManipulation-driven viewports that have
    // no longer a manipulation without the compositor thread being capable of knowing it.
    _Check_return_ HRESULT GetOldDirectManipulationViewport(
        _Out_ IObject** ppCompositorViewport);

    // Discards the potential rejection cross-slide viewports associated with the provided element and its subtree.
    _Check_return_ HRESULT DiscardRejectionViewportsInSubTree(
        _In_ CUIElement* pElement);

    // Stops the provided inertial viewport.
    _Check_return_ HRESULT StopInertialViewport(
        _In_ CDMViewport* pViewport,
        _In_ bool restrictToKnownInertiaEnd,
        _Out_opt_ bool* pHandled);

    // Cancels all ongoing DManip-based manipulations involving the provided element and its ancestors.
    _Check_return_ HRESULT CancelDirectManipulations(
        _In_ CUIElement* pElement,
        _Out_ bool* pfHandled);

    _Check_return_ HRESULT TryStartDirectManipulation(
        _In_ CPointer* pValue,
        _Out_ bool* pHandled);

    // Called when the container is being destroyed and the manipulation handler needs
    // to release all references related to it.
    _Check_return_ HRESULT NotifyReleaseManipulationContainer(
        _In_ CUIElement* pDMContainer);

    // Called when the container's ability to manipulate elements has changed
    _Check_return_ HRESULT NotifyCanManipulateElements(
        _In_ CUIElement* pDMContainer,
        _In_ bool fCanManipulateElementsByTouch,
        _In_ bool fCanManipulateElementsNonTouch,
        _In_ bool fCanManipulateElementsWithBringIntoViewport);

    // Called when:
    //  - originally, when CUIDMContainer.put_Handler is called in order to declare the existing manipulated elements.
    //  - afterwards, whenever the list of manipulated elements has changed.
    // pOldManipulatableElement == NULL && pNewManipulatableElement != NULL ==> a new manipulated element is available
    // pOldManipulatableElement != NULL && pNewManipulatableElement == NULL ==> an old manipulated element is gone
    // pOldManipulatableElement != NULL && pNewManipulatableElement != NULL ==> an old manipulated element was replaced with another one
    _Check_return_ HRESULT NotifyManipulatableElementChanged(
        _In_ CUIElement* pDMContainer,
        _In_opt_ CUIElement* pOldManipulatableElement,
        _In_opt_ CUIElement* pNewManipulatableElement);

    // Notification from a DM container that a secondary content was added.
    // A new CDMContent instance is created if there is an existing CDMViewport,
    // and the DManip service registers the secondary content if available.
    _Check_return_ HRESULT NotifySecondaryContentAdded(
        _In_ CUIElement* pDMContainer,
        _In_opt_ CUIElement* pManipulatableElement,
        _In_ CUIElement* pContentElement,
        _In_ XDMContentType contentType);

    _Check_return_ HRESULT PrepareSecondaryContentRelationshipForCurveUpdate(
        _In_ CSecondaryContentRelationship *pSecondaryContentRelationship);

    // Creates secondary content based on a given secondary content relationship.
    _Check_return_ HRESULT ApplySecondaryContentRelationship(
        _In_ CSecondaryContentRelationship *pSecondaryContentRelationship);

    // Creates secondary content based on the secondary content relationships
    // that couldn't be applied immediately.
    _Check_return_ HRESULT ApplySecondaryContentRelationships();

    // Notification from a DM container that a secondary content was
    // removed. The associated CDMContent instance is deleted and the
    // DManip service unregisters the secondary content.
    _Check_return_ HRESULT NotifySecondaryContentRemoved(
        _In_ CUIElement* pDMContainer,
        _In_opt_ CUIElement* pManipulatableElement,
        _In_ CUIElement* pContentElement);

    // Removes secondary content added for a given secondary content relationship.
    _Check_return_ HRESULT RemoveSecondaryContentRelationship(
        _In_ CSecondaryContentRelationship *pSecondaryContentRelationship);

    // Reconfigure a Secondary Content Relationship taking into account primary content's location change
    _Check_return_ HRESULT UpdateSecondaryContentRelationshipOffsets(
        _In_ IPALDirectManipulationService* pDirectManipulationService,
        _In_ CDMViewport* pViewport,
        _In_ CDMContent* pContent,
        _In_ bool fIsForClip,
        _In_ XFLOAT contentOffsetX,
        _In_ XFLOAT contentContentsOffsetY);

    // Reconfigure all Secondary Content Relationships of type Custom and Descendant
    // if the primary content's location has changed
    _Check_return_ HRESULT UpdateSecondaryContentsOffsets(
        _In_ CUIElement* dmContainer,
        _In_ CUIElement* manipulatedElement,
        _In_ CDMViewport* viewport);

    // Updates the associated dependency properties for secondary content relationships
    // assigned to the given viewport.
    _Check_return_ HRESULT UpdateSecondaryContentRelationshipDependencyProperties(
        _In_ CDMViewport *pViewport);

    // Called when one or more viewport characteristics have changed.
    _Check_return_ HRESULT NotifyViewportChanged(
        _In_ CUIElement* pDMContainer,
        _In_ CUIElement* pManipulatedElement,
        _In_ bool fInManipulation,
        _In_ bool fBoundsChanged,
        _In_ bool fTouchConfigurationChanged,
        _In_ bool fNonTouchConfigurationChanged,
        _In_ bool fConfigurationsChanged,
        _In_ bool fChainedMotionTypesChanged,
        _In_ bool fHorizontalOverpanModeChanged,
        _In_ bool fVerticalOverpanModeChanged,
        _Out_ bool* pfConfigurationsUpdated);

    // Called when one or more viewport characteristics need to be pushed
    // to DM through the DM service.
    _Check_return_ HRESULT UpdateManipulationViewport(
        _In_ CUIElement* pDMContainer,
        _In_ CUIElement* pManipulatedElement,
        _In_ bool fUpdateBounds,
        _In_ bool fUpdateInputTransform,
        _In_ bool fUpdateTouchConfiguration,
        _In_ bool fUpdateNonTouchConfiguration,
        _In_ bool fUpdateConfigurations,
        _In_ bool fUpdateChainedMotionTypes,
        _In_ bool fActivateTouchConfiguration,
        _In_ bool fActivateNonTouchConfiguration,
        _In_ bool fActivateBringIntoViewConfiguration,
        _In_ bool fUpdateHorizontalOverpanMode,
        _In_ bool fUpdateVerticalOverpanMode,
        _Out_opt_ bool* pfConfigurationsUpdated);

    // Called when one or more primary content characteristics have changed.
    _Check_return_ HRESULT NotifyPrimaryContentChanged(
        _In_ CUIElement* pDMContainer,
        _In_ CUIElement* pManipulatedElement,
        _In_ bool fInManipulation,
        _In_ bool fLayoutRefreshed,
        _In_ bool fBoundsChanged,
        _In_ bool fHorizontalAlignmentChanged,
        _In_ bool fVerticalAlignmentChanged,
        _In_ bool fZoomFactorBoundaryChanged);

    // Called when one or more primary content characteristics need to be
    // pushed to DM through the DM service.
    // *pCancelOperation is set to True when the provided DManip container
    // becomes non-manipulatable during the update.
    _Check_return_ HRESULT UpdateManipulationPrimaryContent(
        _In_ CUIElement* pDMContainer,
        _In_ CUIElement* pManipulatedElement,
        _In_ bool fUpdateLayoutRefreshed,
        _In_ bool fUpdateBounds,
        _In_ bool fUpdateHorizontalAlignment,
        _In_ bool fUpdateVerticalAlignment,
        _In_ bool fUpdateZoomFactorBoundary,
        _Out_opt_ bool* pCancelOperation = nullptr);

    // Called when one or more primary content transform characteristics have changed.
    _Check_return_ HRESULT NotifyPrimaryContentTransformChanged(
        _In_ CUIElement* pDMContainer,
        _In_ CUIElement* pManipulatedElement,
        _In_ bool fInManipulation,
        _In_ bool fTranslationXChanged,
        _In_ bool fTranslationYChanged,
        _In_ bool fZoomFactorChanged);

    // Called when one or more primary content characteristics need to be
    // pushed to DM through the DM service.
    _Check_return_ HRESULT UpdateManipulationPrimaryContentTransform(
        _In_ CUIElement* pDMContainer,
        _In_ CUIElement* pManipulatedElement,
        _In_ bool fInManipulation,
        _In_ bool fUpdateTranslationX,
        _In_ bool fUpdateTranslationY,
        _In_ bool fUpdateZoomFactor);

    // Called when the snap points for the provided motion type have changed.
    _Check_return_ HRESULT NotifySnapPointsChanged(
        _In_ CUIElement* pDMContainer,
        _In_ CUIElement* pManipulatedElement,
        _In_ XDMMotionTypes motionType);

    // Called when the snap points for the provided motion type need to be
    // pushed to DM through the DM service.
    _Check_return_ HRESULT UpdateManipulationSnapPoints(
        _In_ CUIElement* pDMContainer,
        _In_ CUIElement* pManipulatedElement,
        _In_ XDMMotionTypes motionType);

    // Called when an alignment for the primary content has changed.
    _Check_return_ HRESULT NotifyManipulatedElementAlignmentChanged(
        _In_ CUIElement* pManipulatedElement,
        _In_ bool fIsForHorizontalAlignment,
        _In_ bool fIsForStretchAlignment,
        _In_ bool fIsStretchAlignmentTreatedAsNear);

    // Retrieves the current Stretch-to-Top/Left alignment overriding status from
    // the ScrollViewer control owning the provided manipulated element.
    _Check_return_ HRESULT IsStretchAlignmentTreatedAsNear(
        _In_ CUIElement* pManipulatedElement,
        _In_ bool isForHorizontalAlignment,
        _Out_ bool* pIsStretchAlignmentTreatedAsNear);

    // Called when the DM container needs access to the latest primary content transform.
    _Check_return_ HRESULT GetPrimaryContentTransform(
        _In_ CUIElement* pDMContainer,
        _In_ CUIElement* pManipulatedElement,
        _In_ bool fForBringIntoViewport,
        _Out_ XFLOAT& translationX,
        _Out_ XFLOAT& translationY,
        _Out_ XFLOAT& uncompressedZoomFactorm,
        _Out_ XFLOAT& zoomFactorX,
        _Out_ XFLOAT& zoomFactorY);

    // Called when the DM container wants to bring the specified bounds of
    // the manipulated element into the viewport. If animate is True, a DM
    // animation is used.
    _Check_return_ HRESULT BringIntoViewport(
        _In_ CUIElement* pDMContainer,
        _In_ CUIElement* pManipulatedElement,
        _In_ XRECTF& bounds,
        _In_ XFLOAT translateX,
        _In_ XFLOAT translateY,
        _In_ XFLOAT zoomFactor,
        _In_ bool fTransformIsValid,
        _In_ bool fSkipDuringTouchContact,
        _In_ bool fSkipAnimationWhileRunning,
        _In_ bool fAnimate,
        _In_ bool fApplyAsManip,
        _In_ bool fIsForMakeVisible,
        _Out_ bool* pfHandled);

    // Retrieve the DManip transform directly from DManip Compositor
    _Check_return_ HRESULT GetDirectManipulationCompositorTransform(
        _In_ CUIElement* pManipulatedElement,
        _In_ TransformRetrievalOptions transformRetrievalOptions,
        _Out_ BOOL& fTransformSet,
        _Out_ FLOAT& translationX,
        _Out_ FLOAT& translationY,
        _Out_ FLOAT& uncompressedZoomFactor,
        _Out_ FLOAT& zoomFactorX,
        _Out_ FLOAT& zoomFactorY);

    // Retrieve the clip content's DManip transform directly from DManip
    _Check_return_ HRESULT GetDirectManipulationClipContentTransform(
        _In_ CUIElement* pClipContentElement,
        _In_ IPALDirectManipulationService* pDirectManipulationService,
        _In_ IObject* pCompositorClipContent,
        _Out_ FLOAT& translationX,
        _Out_ FLOAT& translationY,
        _Out_ FLOAT& zoomFactorX,
        _Out_ FLOAT& zoomFactorY);

    // Retrieve the end-of-inertia transform from the DManip service.
    // pIsInertiaEndTransformValid is set to false when no transform could be retrieved.
    // The returned transform accounts for the possible non-zero content offsets.
    _Check_return_ HRESULT GetDirectManipulationContentInertiaEndTransform(
        _In_ IPALDirectManipulationService* pDirectManipulationService,
        _In_ CDMViewport* pViewport,
        _Out_ FLOAT* pInertiaEndTranslationX,
        _Out_ FLOAT* pInertiaEndTranslationY,
        _Out_ FLOAT* pInertiaEndZoomFactor,
        _Out_ bool* pIsInertiaEndTransformValid);

    // If pCandidateElement is primary or secondary content, returns the DManip service and content for the element, otherwise nullptrs.
    _Check_return_ HRESULT GetDirectManipulationServiceAndContent(
        _In_ CUIElement* pCandidateElement,
        _Outptr_result_maybenull_ IPALDirectManipulationService** ppDMService,
        _Outptr_result_maybenull_ IObject** ppCompositorContent,
        _Outptr_result_maybenull_ IObject** ppCompositorClipContent,
        _Out_ XDMContentType* pDMContentType,
        _Out_ float* pContentOffsetX,
        _Out_ float* pContentOffsetY);

    _Check_return_ _Maybenull_ CUIElement* GetPrimaryContentDMContainer(
        _In_ CUIElement* manipulatedElement) const;

    _Check_return_ _Maybenull_ CUIElement* GetPrimaryContentManipulatedElement(
        _In_ CUIElement* dmContainerElement) const;

    // Returns True when the provided element is:
    // - declared manipulatable for DirectManipulation
    // - the primary content of a viewport
    _Check_return_ HRESULT IsManipulatablePrimaryContent(
        _In_ CUIElement* pUIElement,
        _Out_ bool* pIsManipulatablePrimaryContent);

    // Returns a vector of secondary contents associated with the provided manipulated primary content.
    _Check_return_ HRESULT GetDirectManipulationContents(
        _In_ CUIElement* pManipulatedElement,
        _Out_ xvector<CDMContent*>** ppContents);

    // Returns a vector of secondary clip contents associated with the provided manipulated primary content.
    _Check_return_ HRESULT GetDirectManipulationClipContents(
        _In_ CUIElement* pManipulatedElement,
        _Out_ xvector<CDMContent*>** ppClipContents);

    // Returns manipulated element's viewport status
    _Check_return_ HRESULT GetDirectManipulationViewportStatus(
        _In_ CUIElement* pManipulatedElement,
        _Out_ XDMViewportStatus* pStatus);

    // Called when the ongoing DirectManipulation for the provided viewport needs to be completed.
    _Check_return_ HRESULT CompleteDirectManipulation(_In_ CDMViewport* pViewport, _In_ bool fDisableViewport);

    // Called when the DM container wants the InputManager to process the current
    // input message, by forwarding it to DirectManipulation.
    // The fHandled flag must be set to True if the message was handled.
    // Forwards an input message to DirectManipulation for processing. Key
    // messages that DM is interested in are: mouse wheel messages, arrow keys
    // page up / page down, home / end, ctrl -/+ keys.
    _Check_return_ HRESULT ProcessInputMessageWithDirectManipulation(
            _In_ CUIElement* pDMContainer,
            _In_ CUIElement* pManipulatedElement,
            _In_ bool ignoreFlowDirection,
            _In_ CContentRoot* contentRoot,
            _Out_ bool& fHandled);

    // Stops the viewport associated with the provided DManip container and
    // manipulated element if it's in inertia phase.
    _Check_return_ HRESULT StopInertialViewport(
        _In_ CUIElement* pDMContainer,
        _In_ CUIElement* pManipulatedElement,
        _Out_ bool* pHandled);

    // Initiate a constant-velocity pan on the given
    // container.
    _Check_return_ HRESULT SetConstantVelocities(
        _In_ CUIElement* pDMContainer,
        _In_ CUIElement* pManipulatedElement,
        _In_ XFLOAT panXVelocity,
        _In_ XFLOAT panYVelocity);

    _Check_return_ HRESULT SetCursor(MouseCursor eMouseCursor,
                                     _In_opt_ mui::IInputCursor* inputCursor,
                                     _In_ wrl::ComPtr<mui::IInputPointerSource> inputPointerSource);

    _Check_return_ HRESULT UpdateCursor(_In_ CDependencyObject* pVisualInTargetIsland, _In_ XINT32 bUnset = FALSE);

    void Reset();
    void ResetCrossSlideService();
    void DestroyPointerObjects();

    _Check_return_ HRESULT ObjectLeavingTree(_In_ CDependencyObject *pObject);
    _Check_return_ HRESULT CleanPointerElementObject(_In_ CDependencyObject *pObject);
    _Check_return_ HRESULT ProcessPointerExitedEventByPointerEnteredElementStateChange(
        _In_ CDependencyObject* pElementDO,
        _In_opt_ std::shared_ptr<CPointerState> pointerState = nullptr);

    void DestroyInteractionEngine(_In_ CUIElement* pDestroyElement);
    _Check_return_ HRESULT RaiseRightTappedEvent(_In_ CDependencyObject* pElement, _In_ DirectUI::PointerDeviceType deviceType);

    XUINT64 GetFirstPointerUpQPCSinceLastFrame()
    {
        return m_qpcFirstPointerUpSinceLastFrame;
    }

    void SetFirstPointerUpSinceLastFrame(_In_ XUINT64 value)
    {
        m_qpcFirstPointerUpSinceLastFrame = value;
    }

    CCoreServices* GetCoreServicesNoRef()
    {
        return m_pCoreService;
    }

    // Get the secondary content transform from DMService
    _Check_return_ HRESULT GetSecondaryContentTransform(
        _In_ CUIElement* pDMContainer,
        _In_ CUIElement* pSecondaryContent,
        _Out_ XFLOAT& translationX,
        _Out_ XFLOAT& translationY,
        _Out_ XFLOAT& uncompressedZoomFactor,
        _Out_ XFLOAT& zoomFactorX,
        _Out_ XFLOAT& zoomFactorY);

    HKL GetInputLanguage() const
    {
        return m_inputLang;
    }

    _Check_return_ HRESULT ProcessDeferredReleaseQueue(_In_opt_ IDirectManipulationViewport* pViewport);

    // Returns the Viewport interaction for the given element
    void GetViewportInteraction(
        _In_ CUIElement* element,
        _In_ IUnknown* compositor,
        _Outptr_ IUnknown** interaction);

    void CreateViewportInteractionForManipulatableElement(
        _In_ CUIElement* manipulatedElement,
        _In_ IUnknown* compositor,
        _Outptr_ IUnknown** interaction);

    _Check_return_ HRESULT ResetDManipCompositors();
    void ResetSharedDManipCompositor();

    bool IsUsingInternalTextInputProducer() const
    {
        // This flag indicates if XAML is receiving it's key events from a UWP CoreWindow
        return m_textInputProducerHelper.IsValid();
    }

    TextInputProducerHelper& GetTextInputProducerHelper()
    {
        return m_textInputProducerHelper;
    }

    KeyTipManager& GetKeyTipManager()
    {
        if (!m_keyTipManager)
        {
            m_keyTipManager.attach(new KeyTipManager());
        }
        return *m_keyTipManager;
    }

    void StopInteraction(
        _In_ CUIElement* pContactElement,
        _In_ bool bCallbackForManipulationCompleted);

    static _Check_return_ HRESULT GetPointerInfoFromPointerPoint(
            _In_ ixp::IPointerPoint* pointerPoint,
            _Out_ PointerInfo* pointerInfoResult);

    static UINT GetMessageFromPointerCaptureLostArgs(_In_ ixp::IPointerEventArgs* args);

    CInputServices(_In_ CCoreServices *pCoreService);
    ~CInputServices();

    static _Check_return_ HRESULT ConvertTransformPointToGlobal(_In_ CUIElement *pUIElement, _Inout_ XPOINTF * ppt);

    xchainedmap<XUINT32, std::shared_ptr<CPointerState>>& GetMapPointerState() { return m_mapPointerState; }
    xchainedmap<PointerExitedStateKey, CPointerExitedState*>& GetMapPointerExitedState() { return m_mapPointerExitedState; }
    xchainedmap<XUINT32, CUIElement*>& GetMapInteraction() { return m_mapInteraction; }

    _Check_return_ HRESULT RemovePointerIdFromInteractionElement(_In_ XUINT32 pointerId);
    _Check_return_ HRESULT RemoveEntryFromPointerDownTrackerMap(_In_ UINT32 pointerId);
    _Check_return_ HRESULT AddPointerIdToInteractionElement(_In_ XUINT32 pointerId, _In_ CUIElement *pContactElement);
    _Check_return_ HRESULT AddEntryToPointerDownTrackerMap(_In_ UINT32 pointerId, _In_ CUIElement* pElement);

    _Check_return_ bool IsInputPointerNodeDirty(_In_ CDependencyObject* pElement, _In_ UINT32 uiPointerId) const;
    void SetInputPointerNodeDirty(_In_ CDependencyObject* pElement, _In_ UINT32 uiPointerId, _In_ bool bValue);
    void SetPointerEnter(_In_ CDependencyObject* pElement, _In_ UINT32 uiPointerId, _In_ bool bValue);
    _Check_return_ bool HasPointerEnter(_In_ CDependencyObject* pElement, _In_ UINT32 uiPointerId) const;
    void RemovePointerEnter(_In_ CDependencyObject* pElement, _In_ UINT32 uiPointerId);
    void RemoveInputPointerNodeDirty(_In_ CDependencyObject* pElement, _In_ UINT32 uiPointerId);

    // Returns the CDMViewport instance associated with the provided DManip container,
    // irrespective of its manipulatable element. Only non-unregistering viewports qualify.
    _Maybenull_ CDMViewport* GetViewport(_In_ CUIElement* pDMContainer) const;

    HWND GetHwnd() const { return static_cast<HWND>(m_hWnd); }

#ifdef DM_DEBUG
    bool IsDMInfoTracingEnabled() const { return m_fIsDMInfoTracingEnabled; }
#endif // DM_DEBUG

    _Check_return_ HRESULT ProcessInteractionPointerMessages(
            _In_ XUINT32 pointerId,
            _In_ InputMessage *pMsg);

    _Check_return_ HRESULT CleanPointerProcessingState(
        _In_ CContentRoot* contentRoot,
        _In_ InputMessage *pMsg,
        _In_ XUINT32 pointerId,
        _In_ std::shared_ptr<CPointerState> pointerState,
        _In_opt_ CDependencyObject* pDOContact);

    // Discards all rejection cross-slide viewports associated with the
    // provided object and all its ancestors.
    _Check_return_ HRESULT DiscardRejectionViewportsInParentChain(
            _In_ CDependencyObject* pDO);

    CInteractionManager& GetInteractionManager() { return m_interactionManager; }

    // Returns the DM service for the provided DM container
    _Check_return_ HRESULT EnsureHwndForDManipService(_In_ CUIElement* pDMContainer, HWND hwnd);

private:
    void Init(_In_ CCoreServices *pCoreService);

    // Various functions to process input for different kinds of devices
    //

    static bool IsGamepadOrRemote(const wsy::VirtualKey originalKeyCode);
    XPointerInputType m_lastInputDeviceType = XcpPointerInputTypeMouse;

    _Check_return_ HRESULT ProcessGestureInput(
            _In_ CDependencyObject *pElement,
            _In_ TouchInteractionMsg *pMsg);
    _Check_return_ HRESULT ProcessManipulationStartedInput(
            _In_ CDependencyObject *pElement,
            _In_ TouchInteractionMsg *pMsg);
    _Check_return_ HRESULT ProcessManipulationDeltaInput(
            _In_ CDependencyObject *pElement,
            _In_ TouchInteractionMsg *pMsg);
    _Check_return_ HRESULT ProcessManipulationCompletedInput(
            _In_ CDependencyObject *pElement,
            _In_ TouchInteractionMsg *pMsg);

    _Check_return_ HRESULT ProcessDirectManipulationPointerHitTest(
            _In_ InputMessage *pMsg,
            _In_ CContentRoot* contentRoot,
            _Out_ XINT32 *pfHandled);
    _Check_return_ HRESULT ProcessEnterLeave(
            _In_ CDependencyObject *pNewElement,
            _In_opt_ CEventArgs *pArgs,
            _In_ XINT32 bSkipLeave);

    _Check_return_ HRESULT ProcessCaptureEnterLeave(
            _In_ CDependencyObject *pNewElement,
            _In_opt_ CEventArgs *pArgs);

    _Check_return_ HRESULT RaiseManipulationStartingEvent(
            _In_ CUIElement* pElement,
            _Inout_ DirectUI::ManipulationModes* puiGestureSettings,
            _Inout_ XPOINTF* pPivotCenter,
            _Inout_ XFLOAT* pfPivotRadius);

    _Check_return_ HRESULT RaiseManipulationInertiaStartingEvent(
            _In_ CUIElement* pElement,
            _In_ TouchInteractionMsg *pMsg);

    _Check_return_ HRESULT GetManipulationElement(
            _In_ CUIElement* pContactElement,
            _Out_ CUIElement** ppManipulationElement);

    // Pointer and Gesture/Manipulation methods

    _Check_return_ HRESULT IsInteractionSupported(
            _In_ CUIElement *pContactElement,
            _In_ CUIElement **ppInteractionElement,
            _Out_ bool *pbInteractionSupported);

    _Check_return_ HRESULT ProcessPointerMessagesWithInteractionEngine(
            _In_ XUINT32 pointerId,
            _In_ InputMessage *pMsg,
            _In_ CUIElement *pInteractionElement,
            _In_opt_ CUIElement *pManipulationElement,
            _In_ bool bIgnoreManipulationElement,
            _In_ ElementGestureTracker *pInteraction,
            _In_ bool bForceDisableGesture);

    void RaiseManipulationInertiaProcessingEvent();

    _Check_return_ HRESULT CreatePointerCaptureLostEventArgs(
            _In_ CDependencyObject *pSenderObject,
            _In_ XPOINTF pointLast,
            _In_ CPointer* pPointer,
            _Out_ CPointerEventArgs **ppPointerEventArgs);

    float ConvertPixelsToDips(_In_ float scale, _In_ float pixelValue);
    XPOINTF ConvertPixelsToDips(_In_ float scale, _In_ XPOINTF pixelValue);
    float ConvertDipsToPixels(_In_ float scale, _In_ float dipsValue);

    _Check_return_ HRESULT ConvertTransformPointToLocal(_In_ CUIElement *pUIElement, _Inout_ XPOINTF * ppt);

    _Check_return_ HRESULT RaiseDelayedPointerUpEvent(
        _In_opt_ TouchInteractionMsg* pMsg,
        _In_ CDependencyObject* pElement);

    _Check_return_ HRESULT RaiseRightTappedUnhandledEvent(
        _In_ TouchInteractionMsg* pMsg,
        _In_ CDependencyObject* pElement);

    bool EnsureIslandMouseCaptureReleased();

    //-----------------------------------------------------------------------------
    // DirectManipulation Interaction Methods
    //-----------------------------------------------------------------------------

    // Debugging method that checks if a UIElement is a child of
    // the manipulated element for the provided viewport.
    bool IsElementInViewport(
        _In_ CUIElement* pUIElement,
        _In_ CDMViewport* pViewport);

    static bool IsViewportActive(XDMViewportStatus status)
    {
        return status == XcpDMViewportRunning || status == XcpDMViewportInertia || status == XcpDMViewportSuspended || status == XcpDMViewportAutoRunning;
    }

    bool CanDMContainerInitialize() const
    {
        return m_hWnd != nullptr && m_hWnd != INVALID_HANDLE_VALUE;
    }

    bool CanDMContainerInitialize(_In_ CUIElement* const dmContainer) const
    {
        return dmContainer->CanDMContainerInitialize();
    }

    // Creates a CUIDMContainer and CUIDMContainerHandler instance for the provided element and sets them up for future usage.
    _Check_return_ HRESULT InitializeDirectManipulationContainer(
        _In_ CUIElement* pDMContainer);

    // Creates and initializes a DM PAL service to handle the cross-slide viewport for the provided element.
    _Check_return_ HRESULT InitializeDirectManipulationCrossSlideContainer(
        _In_ CUIElement* pDMCrossSlideContainer);

    // Updates the parent viewport configuration or combined parent viewports
    // configurations for the cross-slide viewports that still need to be started
    // through a call to OnDirectManipulationCrossSlideContainerStart. The
    // active configuration of the provided viewport is used.
    _Check_return_ HRESULT UpdateCrossSlideViewportConfigurations(
        _In_ XUINT32 pointerId,
        _In_ CDMViewport* pViewport,
        _Out_ bool* pfContactFailure);

    // Notifies the provided element that a cross-slide viewport was created on its
    // behalf. The element is then supposed to call DirectManipulationCrossSlideContainerCompleted
    // when a DM manipulation is recognized.
    _Check_return_ HRESULT StartDirectManipulationCrossSlideContainer(
        _In_ CUIElement* pDMCrossSlideContainer,
        _In_ XDMConfigurations parentViewportConfiguration,
        _In_ XDMConfigurations parentViewportsCombinedConfigurations);

    // Loops through all existing cross-slide viewports and checks which ones need a call to
    // OnDirectManipulationCrossSlideContainerStart to kick-start the manipulation recognition.
    _Check_return_ HRESULT StartDirectManipulationCrossSlideContainers();

    // Loops through all existing cross-slide viewports and checks which ones
    // were meant to be started via a call to OnDirectManipulationCrossSlideContainerStart.
    // Discard those cross-slide viewports instead.
    _Check_return_ HRESULT CompleteDirectManipulationCrossSlideContainers();

    // Creates a CUIDMContainer and CUIDMContainerHandler instance for all elements in the waiting queue.
    _Check_return_ HRESULT InitializeDirectManipulationContainers();

    // Caches the initial primary content transformation values for the provided viewport.
    _Check_return_ HRESULT InitializeDirectManipulationViewportValues(
            _In_ CDMViewport* pViewport,
            _In_opt_ IPALDirectManipulationService* pDirectManipulationService,
            _In_ XFLOAT initialTranslationX,
            _In_ XFLOAT initialTranslationY,
            _In_ XFLOAT initialZoomFactor);

    // Allocates the m_pViewports xvector if needed
    _Check_return_ HRESULT EnsureViewports();

    // Allocates the m_pCrossSlideViewports xvector if needed
    _Check_return_ HRESULT EnsureCrossSlideViewports();

    // Returns the existing cross-slide viewport for the provided element, NULL otherwise.
    _Check_return_ HRESULT GetCrossSlideViewport(
            _In_ CUIElement* pDMCrossSlideContainer,
            _Outptr_ CDMCrossSlideViewport** ppCrossSlideViewport);

    // Returns the CDMViewport and CDMContent instances associated with the provided secondary content element.
    _Check_return_ HRESULT GetViewportForContentElement(
            _In_ CUIElement* pContentElement,
            _Outptr_ CDMViewport** ppViewport,
            _Outptr_ CDMContent** ppContent);

    // Returns the CDMViewport instance associated with the provided manipulated element (i.e. DM content)
    _Check_return_ HRESULT GetViewport(
            _In_opt_ CUIElement* pDMContainer,
            _In_ CUIElement* pManipulatedElement,
            _Outptr_ CDMViewport** ppViewport);

    // Returns the viewport for pCandidateElement if this element is primary or secondary content
    _Check_return_ HRESULT GetViewportForPrimaryOrSecondaryContent(
        _In_ CUIElement* pCandidateElement,
        _Outptr_result_maybenull_ CDMViewport** ppViewport);

    // Searches a viewport's primary and secondary content for the given pCandidateElement as its manipulatable element
    bool SearchViewportForPrimaryOrSecondaryContent(
        _In_ CDMViewport* pViewport,
        _In_ CUIElement* pCandidateElement);

    // Returns the CDMContent instance associated with the provided secondary content relationship.
    _Check_return_ HRESULT GetContentForContentElement(
            _In_ CUIElement *pContentElement,
            _In_ bool targetClip,
            _Outptr_ CDMContent** ppContent);

    // Filters out the chained motions that are not included in the provided configuration.
    _Check_return_ HRESULT FilterChainedMotions(
            _In_ XDMConfigurations configuration,
            _Inout_ XDMMotionTypes& chainedMotionTypes);

    // Checks if the provided pViewport viewport has a child viewport with chaining turned on.
    // fCheckState == TRUE:  looking for a child viewport in the ManipulationStarting,
    // ManipulationStarted or ManipulationDelta state.
    // fCheckState == FALSE: looking for a child viewport in the XcpDMViewportReady status.
    _Check_return_ HRESULT HasChainingChildViewport(
            _In_ CDMViewport* pViewport,
            _In_ bool fCheckState,
            _Out_ bool& fHasChainingChildViewport);

    // Completes the manipulation of any parent viewport with an active state.
    _Check_return_ HRESULT CompleteParentViewports(
            _In_ CDMViewport* pViewport);

    // Allocates the m_pDMServices xchainedmap if needed
    _Check_return_ HRESULT EnsureDMServices();

    // Returns the DM service for the provided DM container
    _Check_return_ HRESULT GetDMService(
            _In_ CUIElement* pDMContainer,
            _Outptr_ IPALDirectManipulationService** ppDirectManipulationService);

    // Loops through all the cross-slide viewports and removes the provided pointerId
    // if recorded. Unregisters the viewports with no remaining contacts.
    _Check_return_ HRESULT UnregisterCrossSlideViewportContactId(
            _In_ XUINT32 pointerId);

    // Called to unregister all contact IDs and complete the current manipulation for the provided viewport.
    // When fReleaseAllContactsAndDisableViewport is set, the DManip DisableViewport and ReleaseAllContacts methods are also called.
    _Check_return_ HRESULT UnregisterContactIds(
            _In_ CDMViewport* pViewport,
            _In_ bool fReleaseAllContactsAndDisableViewport);

    // Removes the provided contact Id from the pExclusiveViewport viewport
    // or all viewports when pExclusiveViewport is NULL.
    _Check_return_ HRESULT UnregisterContactId(
            _In_ XUINT32 pointerId,
            _In_opt_ CDMViewport* pExclusiveViewport,
            _In_ bool fCompleteManipulation);

    // Unregisters a cross-slide viewport with DM, discards the DM PAL service,
    // and removes it from the m_pCrossSlideViewports hashtable.
    _Check_return_ HRESULT UnregisterCrossSlideViewport(
            _In_ CDMCrossSlideViewport* pCrossSlideViewport);

    // Unregisters the viewport with the DM PAL service.
    // Removes the viewport from the internal m_pViewports xvector
    _Check_return_ HRESULT UnregisterViewport(
            _In_ CDMViewport* pViewport);

    // Enables the provided viewport. Sets the viewport's NeedsRunningStatusRemoval
    // flag if enabling caused a transitional Running status.
    _Check_return_ HRESULT EnableViewport(
            _In_ IPALDirectManipulationService* pDirectManipulationService,
            _In_ CDMViewport* pViewport);

    // Set the bounds of the given viewport
    _Check_return_ HRESULT SetViewportBounds(
            _In_ IPALDirectManipulationService* directManipulationService,
            _In_ CDMViewport* viewport,
            _In_ const XRECTF& bounds) const;

    // Creates viewports as needed, for the provided contact Id
    _Check_return_ HRESULT InitializeDirectManipulationForPointerId(
            _In_ XUINT32 pointerId,
            _In_ bool fIsForDMHitTest,
            _In_ CUIElement* pPointedElement,
            _Out_ bool* pContactSuccess);

    // Initiates a DirectManipulation manip for the provided CUIDMContainer
    // if the associated viewport is ready.
    _Check_return_ HRESULT SetDirectManipulationContact(
            _In_ XUINT32 pointerId,
            _In_ bool fIsForDMHitTest,
            _In_ CDependencyObject* pPointedElement,
            _In_ CUIElement* pChildElement,
            _In_ CUIElement* pDMContainer,
            _Out_ bool* pfContactFailure);

    // Checks if the provided element requires a cross-slide or rejection cross-slide viewport,
    // and creates it if needed.
    _Check_return_ HRESULT SetDirectManipulationCrossSlideContainer(
            _In_ XUINT32 pointerId,
            _In_ CUIElement* pElement,
            _Out_ bool* pfContactFailure);

    // Creates and sets up a PAL service and a cross-slide viewport for the element
    // if needed. Records the contact on the viewport.
    _Check_return_ HRESULT SetDirectManipulationCrossSlideContainer(
            _In_ XUINT32 pointerId,
            _In_ CUIElement* pDMCrossSlideContainer,
            _In_ XDMConfigurations configuration,
            _Out_ bool* pfContactFailure);

    // Calls SetContact for the provided pointerId and cross-slide viewport and registers that pointerId.
    _Check_return_ HRESULT AddCrossSlideViewportContactId(
            _In_ XUINT32 pointerId,
            _In_ CDMCrossSlideViewport* pCrossSlideViewport,
            _Out_ bool* pfContactFailure);

    // Configures a DM viewport either on pointer down or when an input
    // message is forwarded to DM. Configures the viewport bounds,
    // chaining mode, primary content, configuration mode, snap points
    // and accesses the initial transformation values.
    // *pCancelOperation is set to True when the provided DManip container
    // becomes non-manipulatable during the setup.
    _Check_return_ HRESULT SetupDirectManipulationViewport(
            _In_ CUIElement* pDMContainer,
            _In_ CUIElement* pManipulatedElement,
            _In_ CDMViewport* pViewport,
            _In_ CUIDMContainer* pDirectManipulationContainer,
            _In_ IPALDirectManipulationService* pDirectManipulationService,
            _In_ bool fIsForTouch,
            _In_ bool fIsForBringIntoViewport,
            _In_ bool fIsForAnimatedBringIntoViewport,
            _In_ bool fIsForFullSetup,
            _Out_opt_ bool* pCancelOperation = nullptr);

    // Disables the viewport associated to the provided DM container
    // and manipulated element
    _Check_return_ HRESULT DisableViewport(
            _In_ CUIElement* pDMContainer,
            _In_ CUIElement* pManipulatedElement);

    // Returns the IXcpDirectManipulationViewportEventHandler implementation that is used
    // to forward the DirectManipulation notifications
    _Check_return_ HRESULT GetDirectManipulationViewportEventHandler(
            _Outptr_ IXcpDirectManipulationViewportEventHandler** ppDirectManipulationViewportEventHandler);

    // Deletes the potential viewports remaining and the m_pViewports xvector
    _Check_return_ HRESULT DeleteDMViewports();

    // Deletes the potential viewports remaining and the m_pCrossSlideViewports xvector
    _Check_return_ HRESULT DeleteDMCrossSlideViewports();

    // Deletes the secondary content relationships to be applied and the m_pSecondaryContentRelationshipsToBeApplied xvector
    _Check_return_ HRESULT DeleteSecondaryContentRelationshipsToBeApplied();

    // Deletes the IPALDirectManipulationService implementations associated to CUIDMContainer
    // implementations
    _Check_return_ HRESULT DeleteDMServices();

    // Releases the CUIElement objects stored in the m_pDMContainersNeedingInitialization
    // vector, and deletes that vector.
    void DeleteDMContainersNeedingInitialization();

    // Mark the manipulated element of the provided viewport dirty so that HWCompTreeNode::SetElementData gets called for it
    // and a new DMData structure gets used on the compositor. Do the same for its potential secondary contents.
    _Check_return_ HRESULT DirtyDirectManipulationTransforms(
            _In_ CDMViewport* pViewport);

    // Called after a constant velocity pan completes on the UI thread and
    // the new content bounds need to be pushed to DManip.
    _Check_return_ HRESULT UpdateContentBoundsAfterConstantVelocityPan(
            _In_ CDMViewport* pViewport);

    // Notification from a DM container that a secondary content was added.
    // A new CDMContent instance is created if there is an existing CDMViewport,
    // and the DManip service registers the secondary content if available.
    _Check_return_ HRESULT NotifySecondaryContentAdded(
            _In_ CUIElement* pDMContainer,
            _In_opt_ CUIElement* pManipulatableElement,
            _In_ CUIElement* pContentElement,
            _In_ XDMContentType contentType,
            _In_ XUINT32 cDefinitions,
            _In_reads_(cDefinitions) CParametricCurveDefinition *pDefinitions,
            _In_opt_ CSecondaryContentRelationship *pSecondaryContentRelationship);

    // Deletes the CDMContent instance associated with the provided CUIElement,
    // and unregisters the secondary content on the DManip service side.
    _Check_return_ HRESULT RemoveSecondaryContent(
            _In_ CUIElement* pContentElement,
            _In_opt_ CDMContent* pContent,
            _In_opt_ CDMViewport* pViewport,
            _In_opt_ IPALDirectManipulationService* pDirectManipulationService);

    // Deletes the CDMContent instance associated with the provided CUIElement,
    // and unregisters the secondary clip content on the DManip service side.
    _Check_return_ HRESULT RemoveSecondaryClipContent(
            _In_ CUIElement* pContentElement,
            _In_opt_ CDMContent* pContent,
            _In_opt_ CDMViewport* pViewport,
            _In_opt_ IPALDirectManipulationService* pDirectManipulationService);

    // Stops the ongoing DM manipulations for the provided DM container
    // when fCancelManipulations is True, for the viewports for which
    // the activate configuration corresponds to a disabled manipulability.
    // Attempts the activate or deactivate the DM manager associated with
    // the provided DM container according to the three fCanManipulateElements*** parameters.
    _Check_return_ HRESULT UpdateDirectManipulationManagerActivation(
            _In_ CUIElement* pDMContainer,
            _In_ bool fCancelManipulations,
            _In_ bool fCanManipulateElementsByTouch,
            _In_ bool fCanManipulateElementsNonTouch,
            _In_ bool fCanManipulateElementsWithBringIntoViewport,
            _In_ bool fRefreshViewportStatus);

    // Attempts the activate or deactivate the DM manager associated with
    // the provided DM container according to the manipulability of its elements,
    // the active status of its viewports, and the active status of the application.
    _Check_return_ HRESULT UpdateDirectManipulationManagerActivation(
            _In_ CUIElement* pDMContainer,
            _In_ bool fRefreshViewportStatus);

    // Pushes configuration changes to DM through the DM Service.
    _Check_return_ HRESULT UpdateManipulationConfigurations(
            _In_ IPALDirectManipulationService* pDirectManipulationService,
            _In_ CDMViewport* pViewport,
            _In_ XUINT32 cConfigurations,
            _In_reads_(cConfigurations) XDMConfigurations* pConfigurations,
            _Out_opt_ bool* pfConfigurationsUpdated);

    // Stores touchConfiguration in the provided CDMViewport.
    // Pushes the new active configuration to DM through the DM Service if one is provided.
    _Check_return_ HRESULT UpdateManipulationTouchConfiguration(
            _In_opt_ IPALDirectManipulationService* pDirectManipulationService,
            _In_ CDMViewport* pViewport,
            _In_ XDMConfigurations touchConfiguration);

    // Stores nonTouchConfiguration in the provided CDMViewport.
    // Pushes the new non-touch configuration to DM through the DM Service if one is provided.
    _Check_return_ HRESULT UpdateManipulationNonTouchConfiguration(
            _In_opt_ IPALDirectManipulationService* pDirectManipulationService,
            _In_ CDMViewport* pViewport,
            _In_ XDMConfigurations nonTouchConfiguration);

    // Fetches and stores the horizontal and vertical XDMOverpanModes for the viewport.
    // Recreates the DM curves to define the overpan with compression effect.
    _Check_return_ HRESULT UpdateManipulationOverpanModes(
            _In_ IPALDirectManipulationService* pDirectManipulationService,
            _In_ CDMViewport* pViewport,
            _In_ bool fIsStartingNewManipulation);

    // Stores bringIntoViewportConfiguration in the provided CDMViewport.
    // Pushes the bring-into-viewport configuration to DM through the DM Service.
    _Check_return_ HRESULT UpdateBringIntoViewportConfiguration(
            _In_ IPALDirectManipulationService* pDirectManipulationService,
            _In_ CDMViewport* pViewport,
            _In_ XDMConfigurations bringIntoViewportConfiguration);

    // Pushes the viewport's touch configuration to DM through the DM Service.
    // Marks the viewport as expecting an interaction completion notification
    // and queues up a manipulation recognition notification.
    _Check_return_ HRESULT SwitchToTouchConfiguration(
            _In_ IPALDirectManipulationService* pDirectManipulationService,
            _In_ CDMViewport* pViewport);

    // Called at each UI tick to handle any potential viewport status or transform updates.
    _Check_return_ HRESULT ProcessDirectManipulationViewportChanges();

    // Handles any potential viewport status or transform updates.
    _Check_return_ HRESULT ProcessDirectManipulationViewportChanges(
            _In_ CDMViewport* pViewport);

    // Called by OnPostUIThreadTick at each UI tick to stop any viewport that may
    // be in inertia phase while its manipulated element has no composition peer.
    _Check_return_ HRESULT StopInertialViewportsWithoutCompositorPeer();

    // Called during OnPostUIThreadTick at each UI tick to stop the provided viewport in
    // case it is in inertia phase while its manipulated element has no composition peer.
    _Check_return_ HRESULT StopInertialViewportWithoutCompositorPeer(
            _In_ CDMViewport* pViewport);

    // Called on a UI tick when the status of the provided viewport changed.
    _Check_return_ HRESULT ProcessDirectManipulationViewportStatusUpdate(
            _In_ CDMViewport* pViewport,
            _In_ XDMViewportStatus oldStatus,
            _In_ XDMViewportStatus newStatus,
            _In_ bool fIsValuesChangePreProcessing,
            _Out_opt_ bool* pfIgnoreStatusChange,
            _Out_opt_ bool* pfManipulationCompleted);

    // Called on a UI tick to process a status change related to constant velocity pan.
    _Check_return_ HRESULT ProcessConstantVelocityViewportStatusUpdate(
            _In_ CDMViewport* pViewport,
            _In_ XDMViewportStatus currentStatus);

    // Called when a new transform for a viewport's primary content is
    // available. Caches the new transform.
    _Check_return_ HRESULT ProcessDirectManipulationViewportValuesUpdate(
            _In_ CDMViewport* pViewport,
            _In_ bool fWasInertial,
            _In_ bool fIsInertial,
            _In_ bool fIsLastDelta);

    // Updates the secondary contents' CDMContent transform values.
    _Check_return_ HRESULT ProcessDirectManipulationSecondaryContentsUpdate(
            _In_ CDMViewport* pViewport,
            _In_ IPALDirectManipulationService* pDirectManipulationService);

    // Called when a viewport may have to be submitted to the compositor.
    // In the affirmative, the translation adjustments are cached for
    // future consumption in GetNewDirectManipulationViewport.
    _Check_return_ HRESULT DeclareNewViewportForCompositor(
            _In_ CDMViewport* pViewport,
            _In_ CUIDMContainer* pDirectManipulationContainer);

    // Called when the input manager wants to tell a DM container that it
    // is interested in knowing any DM-influencing change, or is no longer interested.
    _Check_return_ HRESULT SetDirectManipulationHandlerWantsNotifications(
            _In_ CDMViewport* pViewport,
            _In_ CUIElement* pManipulatedElement,
            _In_ CUIDMContainer* pDirectManipulationContainer,
            _In_ bool fWantsNotifications);

    // Called on each UI tick to update the flag on each DM container that
    // determines whether the input manager is interested in knowing any
    // DM-influencing change.
    _Check_return_ HRESULT RefreshDirectManipulationHandlerWantsNotifications();

    // Makes sure the compositor renders an additional frame
    _Check_return_ HRESULT RequestAdditionalFrame();

    // Process WM_INPUTLANGCHANGE
    _Check_return_ HRESULT ProcessInputLanguageChange(
            _In_ InputMessage* pMsg,
            _In_ CContentRoot* contentRoot,
            _Out_ XINT32* pHandled);

    // Process WM_MOVE
    _Check_return_ HRESULT ProcessWindowMove(_In_ InputMessage* pMsg, _In_ CContentRoot* contentRoot);

#ifdef DM_DEBUG
    // Refreshes the values of m_fIsDMInfoTracingEnabled and m_fIsDMVerboseInfoTracingEnabled
    void EvaluateInfoTracingStatuses();
#endif // DM_DEBUG


    _Check_return_ HRESULT PrepareCompositionNodesForBringIntoView(_In_ CDMViewport* pViewport);

    _Check_return_ HRESULT HitTestWithLightDismissAwareness(
        _Inout_ xref_ptr<CDependencyObject>& contactDO,
        XPOINTF contactPoint,
        _In_ MessageMap message,
        _In_opt_ PointerInfo *pointerInfo,
        _In_ CDependencyObject* hitTestRoot);

    int m_ref = 1;

    // Site pointers
    CCoreServices *m_pCoreService;
    CEventManager *m_pEventManager;
    VisualTree* m_pVisualTree = nullptr;

    // Mouse related structures
    // Last Mouse Hittested element
    CDependencyObject * m_pTextCompositionTargetDO;

    // Last stylus location, this is relevant to faking
    // mouse move messages during on capture changes.
    XPOINTF     m_ptStylusPosLast;
    XFLOAT      m_fStylusPressureFactorLast;
    XUINT32     m_bStylusInvertedOnDown;

    XUINT64 m_qpcFirstPointerUpSinceLastFrame;

    xchainedmap<XUINT32, CUIElement*> m_mapInteraction;
    typedef std::map<UINT32, xref_ptr<CUIElement>> PointerDownTrackerMap;
    PointerDownTrackerMap m_mapPointerDownTracker;
    xchainedmap<CUIElement*, CUIElement*> m_mapManipulationContainer;

    // Use a shared_ptr here to ensure that when we get an element out of the map, the CPointer won't be deleted out
    // from underneath us.  (this can happen in re-entrancy scenarios, where the app pumps messages during a callback)
    xchainedmap<XUINT32, std::shared_ptr<CPointerState>> m_mapPointerState;

    std::unordered_map<CDependencyObject*, containers::vector_map<UINT32, bool>> m_mapPointerEnterFromElement;
    std::unordered_map<CDependencyObject*, containers::vector_map<UINT32, bool>> m_mapPointerNodeDirtyFromElement;

    xchainedmap<PointerExitedStateKey, CPointerExitedState*> m_mapPointerExitedState;

    CInteractionManager m_interactionManager;
    xref_ptr<CXamlIslandRoot> m_mouseCaptureIslandRoot;

    HKL m_inputLang;
    XHANDLE m_hCoreWindow = nullptr;

    //-----------------------------------------------------------------------------
    // DirectManipulation Interaction Storage
    //-----------------------------------------------------------------------------

    // Registered DM viewports
    xvector<CDMViewport*>* m_pViewports;
    xvector<CDMCrossSlideViewport*>* m_pCrossSlideViewports;
    xvector<CSecondaryContentRelationship*>* m_pSecondaryContentRelationshipsToBeApplied;

    xchainedmap<CUIElement*, IPALDirectManipulationService*>* m_pDMServices;
    IPALDirectManipulationService* m_pDMCrossSlideService; // Common DM manager used to handle all cross-slide viewports for cross-slide support
    std::shared_ptr<DirectManipulationServiceSharedState> m_DMServiceSharedState;

    XUINT32 m_cCrossSlideContainers; // Number of registered cross-slide containers.
    XHANDLE m_hWnd;     // Window handle used to initialize DirectManipulation managers before any pointer message is processed.
    std::unique_ptr<std::vector < xref::weakref_ptr<CUIElement>>> m_pDMContainersNeedingInitialization;

#ifdef DM_DEBUG
    bool m_fIsDMInfoTracingEnabled : 1;
    bool m_fIsDMVerboseInfoTracingEnabled : 1;
#endif // DM_DEBUG

    // for deciding whether or not DMViewportHandler needs to be registered
    // on a CrossSlide viewport. True if at least one UIElement is draggable.
    bool m_shouldRegisterDMViewportCallback = false;

    // Holds on to DM content/shared transform for sticky headers as we synchronize changing their curves
    std::vector<DMDeferredRelease> m_vecDeferredRelease;

    // This is used to disable text insertion when a KeyDown event gets handled
    TextInputProducerHelper m_textInputProducerHelper;

    xref_ptr<KeyTipManager> m_keyTipManager;

    XPOINTF m_ptLastPressedPosition;

    wrl::ComPtr<mui::IInputSystemCursorStatics> m_inputSystemCursorStatics;
};

//------------------------------------------------------------------------
//
//  Class:  CUIElementHitTestDisabler
//
//  Synopsis: Used to disable the UI element's participation in hit-testing
//            in a way that ensures its participation is re-enabled
//            even if we encounter a failure that causes us to early-return.
//
//------------------------------------------------------------------------
class CUIElementHitTestDisabler final
{
public:
    CUIElementHitTestDisabler(_In_ CUIElement *uiElement)
        : m_uiElement(uiElement)
    {
        if (auto popupRoot = do_pointer_cast<CPopupRoot>(m_uiElement.get()))
        {
            popupRoot->SetIsRootHitTestingSuppressed(true);
        }
        else
        {
            m_uiElement->SetIsHitTestingSuppressed(true);
        }
    }

    ~CUIElementHitTestDisabler()
    {
        if (auto popupRoot = do_pointer_cast<CPopupRoot>(m_uiElement.get()))
        {
            popupRoot->SetIsRootHitTestingSuppressed(false);
        }
        else
        {
            m_uiElement->SetIsHitTestingSuppressed(false);
        }

        m_uiElement = nullptr;
    }

private:
    xref_ptr<CUIElement> m_uiElement;
};
