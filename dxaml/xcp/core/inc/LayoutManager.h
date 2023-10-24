// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <FeatureFlags.h>
#include <uielement.h>
#include <vector>

class CTransition;
class LayoutTransitionStorage;

typedef std::pair<CUIElement*, XRECTF> ElementRectPair;
typedef std::vector<ElementRectPair> ElementRectPairVector;

class CLayoutManager
{
    friend class LayoutTransitionStorage;
public:
    CLayoutManager(_In_ CCoreServices* pCoreServices, _In_ VisualTree* pVisualTree);
    ~CLayoutManager();

    _Check_return_ HRESULT UpdateLayout(XUINT32 controlWidth, XUINT32 controlHeight);
    _Check_return_ HRESULT UpdateLayout();

    void PushCurrentLayoutElement(CUIElement* element);
    CUIElement* PopCurrentLayoutElement();
    // Set the AncestorDirty flag on elements on the layout stack, starting from the
    // current element and until the target ancestor is reached (but don't set the flag
    // on the target itself).
    // It's assumed to be an error if the target is not reached before the bottom
    // of the stack.
    void PropagateAncestorDirtyFromCurrentLayoutElement(CUIElement* targetAncestor);

    XCP_FORCEINLINE CUIElement* GetTransitioningElement() { return m_pTransitioningElement; }
    XCP_FORCEINLINE void SetTransitioningElement(_In_ CUIElement* pElement) { m_pTransitioningElement = pElement; }

    bool GetRequiresMeasure() const;
    bool GetRequiresLayout() const;

    // Returns True when a layout cycle crash is imminent.
    // At that point CUIElement::StoreLayoutCycleWarningContexts() returns True, and CUIElement::StoreLayoutCycleWarningContext(...)
    // calls StoreWarningContext which generate stowed exceptions and additional info in the memory dump.
    bool StoreLayoutCycleWarningContexts() const
    {
        return m_layoutCycleWarningContextsCountdown != -1;
    }

    // Used by CUIElement::StoreLayoutCycleWarningContexts() to include the layout cycle countdown in the WarningContexts recorded
    // in stowed exceptions prior to a layout cycle crash.
    int LayoutCycleWarningContextsCountdown() const
    {
        return m_layoutCycleWarningContextsCountdown;
    }

    _Check_return_ HRESULT EnterMeasure(_In_ CUIElement* pElement);
    _Check_return_ HRESULT ExitMeasure(_In_ CUIElement* pElement);

    _Check_return_ HRESULT EnterArrange(_In_ CUIElement* pElement);
    _Check_return_ HRESULT ExitArrange(_In_ CUIElement* pElement);

    void SetLastExceptionElement(_In_ CUIElement* pElement);
    CUIElement* GetLastExceptionElement() { return m_pLastExceptionElement; }

    void IncreaseLayoutUpdatedSubscriberCounter();
    void DecreaseLayoutUpdatedSubscriberCounter();

    void EnqueueForSizeChanged(_In_ CUIElement* pElement, _In_ const XSIZEF& oldSize);

    _Check_return_ HRESULT EnqueueForEffectiveViewportChanged(
        _In_ CFrameworkElement* element,
        const XRECTF effectiveViewport,
        const XRECTF maxViewport,
        const DOUBLE bringIntoViewDistanceX,
        const DOUBLE bringIntoViewDistanceY);

    _Check_return_ XUINT32 GetPreviousPluginWidth() { return m_previousPluginWidth; }

    _Check_return_ XUINT32 GetPreviousPluginHeight() { return m_previousPluginHeight; }

    _Check_return_ XUINT32 GetUIAClientsListeningToProperty() { return m_bUIAClientsListeningToProperty; }

    _Check_return_ bool DidErrorOccurDuringLayout() { return m_errorOcurredDuringLayout; }
    void ClearErrorOccurredDuringLayout() { m_errorOcurredDuringLayout = FALSE; }

    // indicates the LT walk
    bool GetIsInLayoutTransitionPhase() { return m_isInLayoutTransitionPhase; }
    bool GetIsInLayoutRealizationPhase() { return m_isInTransitionRealization ; }

    // indicates whether we want to ignore availablesize restrictions on desiredsize
    bool GetIsInNonClippingTree() { return m_isInNonClippingTree; }
    void SetIsInNonClippingTree(bool notClipping) { m_isInNonClippingTree = notClipping; }

    _Check_return_ HRESULT GetPosition(_In_ CUIElement* element, _In_ Jupiter::stack_vector<ElementRectPair, 16>& cachedPositions, _Out_ XRECTF* position);
    ElementRectPairVector* GetOrCreateCachedElementRects(_In_ CTransition* transition, _In_ Jupiter::stack_vector<std::pair<xref_ptr<CTransition>, std::unique_ptr<ElementRectPairVector>>, 16>& cachedGroups);
    // Create all the transitions that have been deferred up until now
    _Check_return_ HRESULT RealizeRegisteredLayoutTransitions();
    // start a transition walk (similar to measure and arrange)
    _Check_return_ HRESULT TransitionLayout();
    
    XCP_FORCEINLINE XUINT16 GetLayoutCounter() { return m_layoutCounter; }
    XCP_FORCEINLINE XUINT16 GetNextLayoutCounter() { return m_layoutCounter == XUINT16_MAX ? 2 : m_layoutCounter + 1;}  // using 2 so we have room to always subtract one for setting enter to have already happened (virtualization)
    void IncrementLayoutCounter();

    // This whole flag 'allow transitions to run' is currently just used by virtualizing panels.
    // It should be interpreted as 'we are correcting layout after a manipulation'. 
    XCP_FORCEINLINE bool GetAllowTransitionsToRunUnderCurrentSubtree() { return m_allowTransitionsToRun; }
    XCP_FORCEINLINE void SetAllowTransitionsToRunUnderCurrentSubtree(bool bAllow) { m_allowTransitionsToRun = bAllow; }

#ifdef DEBUG
    // These are diagnostic methods for inspecting the layout properties of the tree.
    void DumpTree();
    void DumpTree(CUIElement* pNode, int depth=0);
#endif

    void EnqueueElementInsertion(
        _In_ CDependencyObject* realizedElement);

    // Layout control constants. These are somewhat arbitrary magic numbers.
public:
    static const XUINT32 MaxLayoutDepth = 250;
    static const XUINT32 MaxLayoutIterations = 250;

    // When decreasing iteration 'count' in CLayoutManager::UpdateLayout reaches this low number, debugging callstacks are recorded and added to the crash dump in the event it underflows.
    static constexpr const unsigned int WarningLayoutIterations = 8;

private:
    void RaiseSizeChangedEvents();
    void RaiseEffectiveViewportChangedEvents();
    void RegisterElementForDeferrredTransition(_In_ CUIElement* pTarget, _In_ LayoutTransitionStorage* pStorage);
    void UnregisterElementForDeferredTransition(_In_ CUIElement* pTarget, _In_ LayoutTransitionStorage* pStorage);

    _Check_return_ HRESULT ProcessElementInsertions();

private:
    // stack of elements going through measure/arrange
    // elements are pushed/popped from the front
    std::deque<CUIElement*> m_layoutElementStack;

    std::deque<xref_ptr<CDependencyObject>>  m_deferredRealizedElementInsertions;

    CCoreServices*  m_pCoreServices;
    VisualTree*     m_pVisualTree;
    CUIElement*     m_pTransitioningElement; // weak ref; needed during transition phase
    CUIElement*     m_pLastExceptionElement; 

    XINT32          m_cArrangesOnStack;
    XINT32          m_cMeasuresOnStack;

    XUINT32         m_firePostLayoutEvents;

    XUINT32         m_previousPluginWidth;
    XUINT32         m_previousPluginHeight;

    // Default value is -1, which indicates that no layout cycle crash is imminent.
    // Value is set in CLayoutManager::UpdateLayout between WarningLayoutIterations-1 and 0 when the layout iteration gets close to the 250 limit and a layout cycle crash may be imminent.
    // 0 is used for the last iteration before the AG_E_LAYOUT_CYCLE error.
    int             m_layoutCycleWarningContextsCountdown{ -1 };

    bool            m_isInUpdateLayout;
    bool            m_isInTransitionRealization;

    bool            m_isInNonClippingTree;  // some trees do not want to clip if their desiredsize is greater than their availablesize

    XRECTF          m_arrangeRect;

    XUINT32         m_nLayoutUpdatedSubscriberCounter;
    bool           m_bManagedPeerReferenceToLastExceptionElementTaken;

    XUINT16         m_layoutCounter    : 16; // increased at the start of each layout. Is used when determining
                                             // the transition an element should trigger (load versus layout versus unload).

    // This queue contains the elements that changed size in the layout pass
    // They are queued for firing after no Measures or Arranges are on the stack.
    struct SizeChangedQueueItem
    {
        SizeChangedQueueItem(_In_ CUIElement* pElement, _In_ const XSIZEF& oldSize) noexcept;
        ~SizeChangedQueueItem();

        SizeChangedQueueItem(const SizeChangedQueueItem&) = delete;
        SizeChangedQueueItem& operator=(const SizeChangedQueueItem&) = delete;

        SizeChangedQueueItem(SizeChangedQueueItem&& other) noexcept;
        SizeChangedQueueItem& operator=(SizeChangedQueueItem&& other) noexcept;

        CUIElement* m_pElement{ nullptr };
        XSIZEF m_oldSize{};
    };

    std::vector<SizeChangedQueueItem> m_sizeChangedQueue;

    class EffectiveViewportChangedQueueItem
    {
    public:
        EffectiveViewportChangedQueueItem(
            _In_ CFrameworkElement* element,
            XRECTF effectiveViewport,
            XRECTF maxViewport,
            DOUBLE bringIntoViewDistanceX,
            DOUBLE bringIntoViewDistanceY)
            : m_element(element)
            , m_effectiveViewport(effectiveViewport)
            , m_maxViewport(maxViewport)
            , m_bringIntoViewDistanceX(bringIntoViewDistanceX)
            , m_bringIntoViewDistanceY(bringIntoViewDistanceY)
        {
        }

        xref_ptr<CFrameworkElement> GetElement() const { return m_element; }
        XRECTF GetEffectiveViewport() const { return m_effectiveViewport; }
        XRECTF GetMaxViewport() const { return m_maxViewport; }
        DOUBLE GetBringIntoViewDistanceX() const { return m_bringIntoViewDistanceX; }
        DOUBLE GetBringIntoViewDistanceY() const { return m_bringIntoViewDistanceY; }

    private:
        xref_ptr<CFrameworkElement> m_element;
        XRECTF m_effectiveViewport;
        XRECTF m_maxViewport;
        DOUBLE m_bringIntoViewDistanceX;
        DOUBLE m_bringIntoViewDistanceY;
    };

    std::vector<EffectiveViewportChangedQueueItem> m_effectiveViewportChangedQueue;
    std::vector<CUIElement::TransformToPreviousViewport> m_transformsToViewports;
    std::vector<CUIElement::UnidimensionalViewportInformation> m_horizontalViewports;
    std::vector<CUIElement::UnidimensionalViewportInformation> m_verticalViewports;

    XUINT32         m_bUIAClientsListeningToProperty;
    bool           m_errorOcurredDuringLayout;
    bool           m_allowTransitionsToRun;

    bool           m_isInLayoutTransitionPhase;           // indicates the layouttransition walk, similar to arrange and measure walks
    std::vector<xref_ptr<CUIElement>> m_elementsWithDeferredTransitions;        // list of elements that have registered to want to create one ore more transitions.
                                                           // the elements keep their own list of registered transitions
    std::vector<std::unique_ptr<ElementRectPairVector>> m_elementRectPool;
};
