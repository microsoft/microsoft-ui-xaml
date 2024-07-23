// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

static const XFLOAT LeaveShouldDetermineOpacity = XFLOAT_MAX;

//------------------------------------------------------------------------
// Storage for the snapshot that a LayoutTransition takes between layout
// changes, and storage for the properties that drive the animation.
//------------------------------------------------------------------------
class LayoutTransitionStorage
{
public:
    LayoutTransitionStorage()
        : m_transformStart(true /*initialize*/)
        , m_transformDestination(true /*initialize*/)
    {
        m_sizeStart.height = m_sizeStart.width = m_currentSize.height = m_currentSize.width = m_sizeDestination.height = m_sizeDestination.width = 0.0f;
        m_currentOffset.x = m_currentOffset.y = 0.0f;
        m_arrangeInput.Height = m_arrangeInput.Width = m_arrangeInput.X = m_arrangeInput.Y = 0.0f;
        m_arrangeOutput.Height = m_arrangeOutput.Width = m_arrangeOutput.X = m_arrangeOutput.Y = 0.0f;
        m_opacityCache = LeaveShouldDetermineOpacity;
        m_opacityStart = m_opacityDestination = 1.0f;
        m_scaleStart = 1.0f;
        m_nextGenerationCounter = 0;
        m_nextGenerationOffset.x = m_nextGenerationOffset.y = 0.0f;
        m_nextGenerationSize.width = m_nextGenerationSize.height = 0.0f;

        m_trigger = DirectUI::TransitionTrigger::NoTrigger;
        m_bRegisteredInLayoutManager = false;
    };

    ~LayoutTransitionStorage();

    void RegisterElementForTransitions(_In_ CUIElement* pTarget, _In_ xvector<CTransition*>& transitions, _In_ DirectUI::TransitionTrigger trigger);
    void UnregisterElementForTransitions(_In_ CUIElement* pTarget);
    _Check_return_ HRESULT RegisterStoryboard(_In_ CStoryboard* pStoryboard);
    _Check_return_ HRESULT CleanupStoryboard(_In_ CStoryboard* pStoryboard); // not meant to be used except from a FAILED(hr)
    _Check_return_ HRESULT ClearStoryboards();

    // the primary brush is the LTE providing location and opacity information.
    // it basically means: this is what we consider to be what is on the screen.
    // we only check the first two LTE's and use the one with the highest opacity
    // since that is the one that has focused the users eye.
    // As a secondary, more expensive criteria, we can use the localclip
    _Check_return_ CLayoutTransitionElement* GetPrimaryBrush();

    _Check_return_ HRESULT RegisterBrushRepresentation(_In_ CUIElement* pTarget, _In_ CLayoutTransitionElement* pLT, _In_ DirectUI::TransitionParent transitionParent);
    _Check_return_ HRESULT UnregisterBrushRepresentation(_In_ CUIElement* pTarget, _In_ CLayoutTransitionElement* pLT, _In_ bool removeFromInternalStorage);
    _Check_return_ HRESULT CleanupBrushRepresentations(_In_ CUIElement* pTarget);

    _Check_return_ HRESULT UpdateBrushes(_In_ XRECTF finalRect);

    // translates the internal trigger to a public trigger. Fails if there is no trigger
    _Check_return_ HRESULT GetTriggerForPublicConsumption(_Out_ DirectUI::TransitionTrigger *pTrigger);
    _Check_return_ DirectUI::TransitionTrigger GetTrigger() { return m_trigger; }
    void SetTrigger(DirectUI::TransitionTrigger trigger) { m_trigger = trigger; }

public:
    // snapshot information
    XSIZEF      m_currentSize;
    XPOINTF     m_currentOffset;        // offset to either parent or plugin (it is relative to plugin after lefttree, but before enterimpl)
                                        // these offsets are not kept up to date during animation but are grabbed when layout runs.
    XPOINTF     m_nextGenerationOffset; // offset is updated in arrange, but is only valid on the next layout cycle
    XSIZEF      m_nextGenerationSize;
    XUINT32     m_nextGenerationCounter;// the layout cycle in which the nextgeneration offset becomes valid.

    XFLOAT      m_opacityCache;         // calculated at specific times to be used as a helper to determine a start opacity
    XRECTF      m_arrangeInput;
    XRECTF      m_arrangeOutput;

    // used for the animation
    XSIZEF      m_sizeStart;
    XSIZEF      m_sizeDestination;
    CMILMatrix  m_transformStart;
    CMILMatrix  m_transformDestination;
    XFLOAT      m_opacityStart;
    XFLOAT      m_opacityDestination;
    XFLOAT      m_scaleStart;

    xvector<CStoryboard*> m_transitionStoryboards;  // the active storyboards
    std::vector<xref_ptr<CTransition>> m_registeredTransitions;  // transition instances that have registered to start a transition for this uielement
    bool       m_bRegisteredInLayoutManager;
private:
    DirectUI::TransitionTrigger m_trigger;
    xvector<CLayoutTransitionElement*> m_transitionElements;    // the active LTE instances
};

//------------------------------------------------------------------------
// If a transition is applicable to a visual, it will get notified
// of a change in layout and has the ability to setup a storyboard to
// make that change 'fluid'.
// Note Transition is shareable.
//------------------------------------------------------------------------
class CTransition : public CMultiParentShareableDependencyObject
{
protected:
    CTransition(_In_ CCoreServices *pCore);
    ~CTransition() override;
public:

    DECLARE_CREATE(CTransition);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTransition>::Index;
    }

    // process and take current snapshot data in preparation of a layoutchange
    _Check_return_ static HRESULT OnLayoutChanging(_In_ CUIElement* pTarget, _In_ XRECTF finalRect);
    // process layoutchange and determine action
    _Check_return_ static HRESULT OnLayoutChanged(_In_ CUIElement* pTarget);
    _Check_return_ static HRESULT ProcessLoadTrigger(_In_ CUIElement* pTarget, _In_ LayoutTransitionStorage* pStorage);
    _Check_return_ static HRESULT ProcessLayoutTrigger(_In_ CUIElement* pTarget, _In_ LayoutTransitionStorage* pStorage);
    _Check_return_ static HRESULT ProcessUnloadTrigger(_In_ CUIElement* pTarget, _In_ LayoutTransitionStorage* pStorage, _In_ xvector<CTransition*>& applicableTransitions);

    // do the actual work of creating and starting up a storyboard
    _Check_return_ static HRESULT SetupTransition(_In_ CUIElement* pTarget, _In_ CTransition* pTransition, _In_ XFLOAT staggeredBegin);
    _Check_return_ virtual HRESULT CreateStoryboards(_In_ CUIElement* pTarget, _In_ XFLOAT staggeredBegin, _Out_ XINT32* cStoryboards, _Outptr_result_buffer_(*cStoryboards) CStoryboard*** pppStoryboardArray, _Out_ DirectUI::TransitionParent* pTransitionParent);
    _Check_return_ virtual HRESULT ParticipateInTransitions(_In_ CUIElement* pTarget, _In_ DirectUI::TransitionTrigger trigger, _Out_ bool* pResult);

    // cancels the transitions without modifying any visual properties
    _Check_return_ static HRESULT CancelTransitions(_In_ CUIElement* pTarget);

    // called after layoutmanager sets up transitions
    _Check_return_ static HRESULT ValidateTransitionWasSetup(_In_ CUIElement* pTarget);

    // applicable from this trigger
    _Check_return_ static HRESULT AppendAllTransitionsNoAddRefs(_In_ CUIElement* pTarget, _In_ DirectUI::TransitionTrigger trigger, _Out_ xvector<CTransition*>& applicableTransitions);

    // ==== information about running animations/transitions

    // true if there is an active or filling transition
    bool static HasTransitionAnimations(_In_ CUIElement* pTarget);
    bool static HasTransitionAnimations(_In_ CUIElement* pTarget, _In_ DirectUI::TransitionTrigger trigger);

    // true if there is a transition set on the element (might not have started yet, so may not yet have a storyboard)
    // HasActiveTransition will always be true if HasActiveTransitionAnimations is TRUE. During the time between registration and realization, the inverse is not true.
    bool static HasActiveTransition(_In_ CUIElement* pTarget);
    bool static HasActiveTransition(_In_ CUIElement* pTarget, _In_ DirectUI::TransitionTrigger trigger);

    // true if this element has a transitioncollection or inherits one from a parent.
    // used for speed: no call out is made to managed layer and we just look at the existence of a collection
    bool static HasPossibleTransition(_In_ CUIElement* pTarget);

    // ===== information for the layout cycle

    // updates current location from pTarget to the storage with the always correct information from destinationElement
    void static SetInformationFromAnimation(_In_ CUIElement* pTarget, _In_ LayoutTransitionStorage* pStorage, _In_ XUINT32 nextGenerationCounter);

    // updates a versioned set of information that will become the input on a next layout cycle.
    void static SetNextGenerationInformationFromLayout(_In_ CUIElement* pTarget, _In_ LayoutTransitionStorage* pStorage, _In_ XUINT32 nextGenerationCounter);

    // the layout constraint as it is currently animated
    _Check_return_ HRESULT static GetLayoutSlotDuringTransition(_In_ CUIElement* pTarget, _Inout_ XRECTF* pLayoutSlot);

    // true if this element is disabling transitions for its subtree
    bool static GetAllowsTransitionsToRun(_In_ CUIElement* pTarget);
    // true if this transition is allowing any staggering
    bool GetIsStaggeringEnabled() { return m_isStaggeringEnabled; }
    void SetIsStaggeringEnabled(_In_ bool value) { m_isStaggeringEnabled = value; }

private:
    // ultimately will cancel transitions and also reset visual properties
    static _Check_return_ HRESULT OnTransitionCompleted(
    _In_ CDependencyObject* pSender,
    _In_ CEventArgs* pEventArgs
    );

    bool static HasRunningTransitionAnimations(_In_ CUIElement* pTarget);

public:
    CDependencyObject* m_pStaggerFunction;

private:
    bool m_isStaggeringEnabled = false;
};

//------------------------------------------------------------------------
//
//  Struct:  LayoutTransitionCompletedData
//
//      Simple Data Abstraction containing the objects necessary
//      to complete a layout transition after the animations have completed.
//
//------------------------------------------------------------------------
struct LayoutTransitionCompletedData
{
    CUIElement*        m_pTarget;
    CValue             m_EventListenerToken;

    LayoutTransitionCompletedData(
        _In_ CUIElement* pTarget)
    {
        m_pTarget = pTarget;
        AddRefInterface(m_pTarget);
    }

    ~LayoutTransitionCompletedData()
    {
        ReleaseInterface(m_pTarget);
    }
};
