// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "InputServices.h"

#include <EffectiveViewportChangedEventArgs.h>
#include <string>

// Apps usually tend to have a few entries in the sizeChangedQueue and
// sometimes up to a dozen. The value of 24 is a conservative estimate to
// allocate space for 24 entries in the stack vector to avoid running over
// and having to allocate on the heap. 
static constexpr size_t c_sizeChangedVectorSize = 24;

// SizeChangedQueueItems
//------------------------------------------------------------------------
CLayoutManager::SizeChangedQueueItem::SizeChangedQueueItem(_In_ CUIElement* pElement, _In_ const XSIZEF& oldSize) noexcept
    : m_pElement(pElement), m_oldSize(oldSize)
{
    // Keep a ref on the element, and protect it from GC while it's on the queue
    m_pElement->AddRef();
    m_pElement->PegManagedPeer();
}

CLayoutManager::SizeChangedQueueItem::~SizeChangedQueueItem()
{
    if (m_pElement)
    {
        m_pElement->UnpegManagedPeer();
        ReleaseInterface(m_pElement);
    }
}

CLayoutManager::SizeChangedQueueItem::SizeChangedQueueItem(
    SizeChangedQueueItem&& other) noexcept
{
    m_pElement = other.m_pElement;
    m_oldSize = other.m_oldSize;

    other.m_pElement = nullptr;
}

CLayoutManager::SizeChangedQueueItem& CLayoutManager::SizeChangedQueueItem::operator=(
    SizeChangedQueueItem&& other) noexcept
{
    if (this != &other)
    {
        m_pElement = other.m_pElement;
        m_oldSize = other.m_oldSize;

        other.m_pElement = nullptr;
    }

    return *this;
}

CLayoutManager::CLayoutManager(_In_ CCoreServices* pCoreServices, _In_ VisualTree* pVisualTree)
{
    XCP_WEAK(&m_pCoreServices);
    m_pCoreServices = pCoreServices;
    m_previousPluginWidth = 0;
    m_previousPluginHeight = 0;
    m_pLastExceptionElement = NULL;
    m_pTransitioningElement = NULL;
    m_cMeasuresOnStack = 0;
    m_cArrangesOnStack = 0;
    m_firePostLayoutEvents = FALSE;
    m_isInUpdateLayout = FALSE;
    m_arrangeRect.X = m_arrangeRect.Y = m_arrangeRect.Width = m_arrangeRect.Height = 0;
    m_bUIAClientsListeningToProperty = FALSE;
    m_errorOcurredDuringLayout = FALSE;
    m_bManagedPeerReferenceToLastExceptionElementTaken = FALSE;
    m_isInLayoutTransitionPhase = FALSE;
    m_allowTransitionsToRun = TRUE;
    m_pVisualTree = pVisualTree;
    m_layoutCounter = 2;
    m_isInTransitionRealization = FALSE;
    m_nLayoutUpdatedSubscriberCounter = 0;
    m_isInNonClippingTree = false;
    // reserve so we don't end up copying
    m_elementRectPool.reserve(16);
}

CLayoutManager::~CLayoutManager()
{
    m_elementsWithDeferredTransitions.clear();

//#error Need to un-peg managed peer also?  Or does whole thing go down with a FAIL_FAST when this is non-NULL anyway?
    ReleaseInterface(m_pLastExceptionElement);
}

//------------------------------------------------------------------------
//
//  Method:   CLayoutManager::SetLastExceptionElement
//
//  Synopsis:
//      Sets the last exception element.
//
//------------------------------------------------------------------------

void CLayoutManager::SetLastExceptionElement(_In_ CUIElement *pLastExceptionElement)
{
    if (m_pLastExceptionElement != pLastExceptionElement)
    {
        // Release the managed peer reference to the old layout exception element
        if (m_pLastExceptionElement != NULL && m_bManagedPeerReferenceToLastExceptionElementTaken)
        {
            m_pLastExceptionElement->UnpegManagedPeerNoRef();
            m_bManagedPeerReferenceToLastExceptionElementTaken = FALSE;
        }

        // Release the reference to the old layout exception element
        ReleaseInterface(m_pLastExceptionElement);

        // Update the layout exception element
        m_pLastExceptionElement = pLastExceptionElement;

        if (m_pLastExceptionElement != NULL)
        {
            // Add reference to layout exception element
            AddRefInterface(m_pLastExceptionElement);

            m_pLastExceptionElement->PegManagedPeerNoRef();
            m_bManagedPeerReferenceToLastExceptionElementTaken = TRUE;
            m_errorOcurredDuringLayout = TRUE;
        }
        else
        {
            ClearErrorOccurredDuringLayout();
        }
    }

    // Update the core's global last exception element, which is written to by multiple layout managers
    // if there are multiple visual trees.
    static_cast<CCoreServices *>(m_pCoreServices)->SetLastLayoutExceptionElement(pLastExceptionElement);
}

bool CLayoutManager::GetRequiresMeasure() const
{
    if (m_pVisualTree)
    {
        return m_pVisualTree->GetRootVisual()->GetRequiresMeasure();
    }
    else
    {
        return false;
    }
}

bool CLayoutManager::GetRequiresLayout() const
{
    if (m_pVisualTree)
    {
        return m_pVisualTree->GetRootVisual()->GetRequiresLayout();
    }
    else
    {
        return false;
    }
}

_Check_return_
HRESULT
CLayoutManager::EnterMeasure(_In_ CUIElement* pElement)
{
    SetLastExceptionElement(NULL);

    if (++m_cMeasuresOnStack > MaxLayoutDepth)
        IFC_RETURN(E_FAIL);

    m_firePostLayoutEvents = TRUE;

    return S_OK;
}

_Check_return_
HRESULT
CLayoutManager::ExitMeasure(_In_ CUIElement* pElement)
{
    --m_cMeasuresOnStack;

    return S_OK;
}

_Check_return_
HRESULT
CLayoutManager::EnterArrange(_In_ CUIElement* pElement)
{
    SetLastExceptionElement(NULL);

    if (++m_cArrangesOnStack > MaxLayoutDepth)
        IFC_RETURN(E_FAIL);

    m_firePostLayoutEvents = TRUE;

    return S_OK;
}

_Check_return_
HRESULT
CLayoutManager::ExitArrange(_In_ CUIElement* pElement)
{
    --m_cArrangesOnStack;

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CLayoutManager::UpdateLayout
//
//  Synopsis:   This is the main layout loop
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CLayoutManager::UpdateLayout()
{
    return UpdateLayout(m_previousPluginWidth, m_previousPluginHeight);
}

_Check_return_
HRESULT
CLayoutManager::UpdateLayout(XUINT32 controlWidth, XUINT32 controlHeight)
{
    HRESULT hr = S_OK;
    CUIElement* pRoot = NULL;

    if (m_pVisualTree)
    {
        pRoot = (CUIElement*)m_pVisualTree->GetRootVisual();
    }

    if (!pRoot)
        RRETURN(S_OK);

    // Anything to do?
    XUINT32 fPluginSizeChanged = controlWidth != m_previousPluginWidth || controlHeight != m_previousPluginHeight;
    if (!fPluginSizeChanged && !pRoot->GetRequiresLayout()
        && !pRoot->GetIsViewportDirtyOrOnViewportDirtyPath())
        RRETURN(S_OK);

    // Prevent reentrancy, just to be safe
    if (m_isInUpdateLayout || m_cMeasuresOnStack != 0 || m_cArrangesOnStack != 0)
        RRETURN(S_OK);

    m_isInUpdateLayout = TRUE;

    TraceLayoutBegin();

    XUINT32 count = MaxLayoutIterations;
    std::wstring extraInfoEntries[WarningLayoutIterations];

    // UIAutomation needs to fire update events for each element.  Don't do it if nobody is listening.
    m_bUIAClientsListeningToProperty = (static_cast<CCoreServices*>(m_pCoreServices))->UIAClientsAreListening(UIAXcp::AEPropertyChanged) == S_OK;

    while (count--)
    {
        const unsigned int extraInfoIndex = count < WarningLayoutIterations ? WarningLayoutIterations - count - 1 : 0;

        if (count < WarningLayoutIterations)
        {
            // Turn on stack trace logging for potential inclusion in crash dump.
            m_layoutCycleWarningContextsCountdown = static_cast<int>(count);

            extraInfoEntries[extraInfoIndex].assign(L"Layout Iteration Countdown: ");
            extraInfoEntries[extraInfoIndex].append(std::to_wstring(count));
            extraInfoEntries[extraInfoIndex].append(L".");
        }

        if (fPluginSizeChanged || pRoot->GetRequiresMeasure())
        {
            if (fPluginSizeChanged)
            {
                m_arrangeRect.Width = (XFLOAT) controlWidth;
                m_arrangeRect.Height = (XFLOAT) controlHeight;
            }

            TraceMeasureBegin();
            auto scopeGuard = wil::scope_exit([&]
            {
                TraceMeasureEnd();
            });

            if (StoreLayoutCycleWarningContexts())
            {
                extraInfoEntries[extraInfoIndex].append(L" Launching Measure Pass.");
            }

            IFC(pRoot->Measure(m_arrangeRect.Size()));

            if(fPluginSizeChanged)
            {
                fPluginSizeChanged = FALSE;
                m_previousPluginWidth = controlWidth;
                m_previousPluginHeight = controlHeight;
            }

            ASSERT(!pRoot->GetRequiresMeasure());
        }
        else if (pRoot->GetRequiresArrange())
        {
            TraceArrangeBegin();
            auto scopeGuard = wil::scope_exit([&]
            {
                TraceArrangeEnd();
            });

            if (StoreLayoutCycleWarningContexts())
            {
                extraInfoEntries[extraInfoIndex].append(L" Launching Arrange Pass.");
            }

            IFC(pRoot->Arrange(m_arrangeRect));

            ASSERT(!pRoot->GetRequiresArrange() || pRoot->GetRequiresMeasure());
        }
        else if (pRoot->GetIsViewportDirtyOrOnViewportDirtyPath())
        {
            TraceFireEffectiveViewportChangedBegin();
            auto scopeGuard = wil::scope_exit([&]
            {
                TraceFireEffectiveViewportChangedEnd();
            });

            if (StoreLayoutCycleWarningContexts())
            {
                extraInfoEntries[extraInfoIndex].append(L" Launching EffectiveViewport Pass.");
            }

            m_horizontalViewports.clear();
            m_verticalViewports.clear();
            m_transformsToViewports.clear();
            
            // Even though the root visual is not a viewport per se,
            // we use the original arrange rect to clamp effective viewports.
            // The exception is if we have XAML islands, in which case they'll
            // add their own size rects to clamp effective viewports.
            if (m_pCoreServices->GetInitializationType() != InitializationType::IslandsOnly)
            {
                m_horizontalViewports.emplace_back(m_arrangeRect.X, m_arrangeRect.Width);
                m_verticalViewports.emplace_back(m_arrangeRect.Y, m_arrangeRect.Height);
            }

            IFC(pRoot->EffectiveViewportWalk(
                pRoot->GetIsViewportDirty(),
                m_transformsToViewports,
                m_horizontalViewports,
                m_verticalViewports));
            RaiseEffectiveViewportChangedEvents();
        }
        else // Fire any layout events
        {
            // Fire any size changed events
            if (!m_sizeChangedQueue.empty())
            {
                TraceFireSizeChangedBegin();
                auto scopeGuard = wil::scope_exit([&]
                {
                    TraceFireSizeChangedEnd();
                });

                if (StoreLayoutCycleWarningContexts())
                {
                    extraInfoEntries[extraInfoIndex].append(L" Raising SizeChanged Events.");
                }

                RaiseSizeChangedEvents();
            }

            if (pRoot->GetRequiresLayout()
                || pRoot->GetIsViewportDirtyOrOnViewportDirtyPath())
                continue;

            // Fire layout updated events
            if (m_nLayoutUpdatedSubscriberCounter != 0)
            {
                TraceFireLayoutUpdatedBegin();
                auto scopeGuard = wil::scope_exit([&]
                {
                    TraceFireLayoutUpdatedEnd();
                });

                if (StoreLayoutCycleWarningContexts())
                {
                    extraInfoEntries[extraInfoIndex].append(L" Raising LayoutUpdated Events.");
                }

                IGNOREHR(FxCallbacks::JoltHelper_RaiseEvent(
                    /* target */ NULL,
                    DirectUI::ManagedEvent::ManagedEventLayoutUpdated,
                    /* pArgs */ NULL));
            }

            if (!pRoot->GetRequiresLayout()
                && !pRoot->GetIsViewportDirtyOrOnViewportDirtyPath())
                break;
        }
    }

    if (StoreLayoutCycleWarningContexts())
    {
        m_layoutCycleWarningContextsCountdown = -1;

        if (count == -1)
        {
            std::vector<std::wstring> extraInfo;

            for (unsigned int extraInfoEntry = 0; extraInfoEntry < WarningLayoutIterations; extraInfoEntry++)
            {
                extraInfo.push_back(std::move(extraInfoEntries[extraInfoEntry]));
            }

            IFC_EXTRA_INFO(AgError(AG_E_LAYOUT_CYCLE), &extraInfo);
        }
        else
        {
            // No layout cycle condition hit after all. Clear the potentially recorded WarningRecords.
            // Only the instances of type WarningContextType::LayoutCycle are discarded.
            ClearWarningContexts(WarningContextLog::WarningContextType::LayoutCycle);
        }
    }

    IFC(ProcessElementInsertions());

Cleanup:
    // This queue should be empty but if we are exiting due to an error or too many iterations then
    // we should just throw everything away.
    m_sizeChangedQueue.clear();
    
    m_isInUpdateLayout = FALSE;

    TraceLayoutEnd();

    // Firing a UIAutomation automation properties change check
    if (m_bUIAClientsListeningToProperty)
    {
        CUIElement* pVisualRoot = static_cast<CUIElement*>(m_pVisualTree ? m_pVisualTree->GetPublicRootVisual() : m_pCoreServices->getVisualRoot());
        if (pVisualRoot)
        {
            TraceCheckAutomaticAutomationChangesBegin();
            HRESULT hr2 = pVisualRoot->CheckAutomaticAutomationChanges();
            TraceCheckAutomaticAutomationChangesEnd();
            hr = FAILED(hr) ? hr : hr2;
        }

        CPopupRoot* pPopupRoot = m_pVisualTree->GetPopupRoot();
        if (pPopupRoot)
        {
            TraceCheckAutomaticAutomationChangesPopupBegin();
            HRESULT hr2 = pPopupRoot->CheckAutomaticAutomationChanges();
            TraceCheckAutomaticAutomationChangesPopupEnd();
            hr = FAILED(hr) ? hr : hr2;
        }
    }

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CLayoutManager::EnqueueForSizeChanged
//
//  Synopsis:   Create a SizeChangedQueueItem and add it to the queue
//              to be fired later.
//
//-------------------------------------------------------------------------
void CLayoutManager::EnqueueForSizeChanged(_In_ CUIElement* pElement, _In_ const XSIZEF& oldSize)
{
    if (!pElement->GetSizeChanged() && pElement->GetWantsSizeChanged())
    {
        if (StoreLayoutCycleWarningContexts())
        {
            // Recording element's size change with the old size.
            std::vector<std::wstring> warningInfo;

            std::wstring oldSizeEntry(L"oldSize: ");
            oldSizeEntry.append(std::to_wstring(oldSize.width));
            oldSizeEntry.append(L"x");
            oldSizeEntry.append(std::to_wstring(oldSize.height));
            warningInfo.push_back(std::move(oldSizeEntry));

            pElement->StoreLayoutCycleWarningContext(warningInfo, this);
        }

        m_sizeChangedQueue.emplace_back(pElement, oldSize);
    }
}

//-------------------------------------------------------------------------
//
//  Function:   CLayoutManager::RaiseSizeChangedEvents
//
//  Synopsis:   Fire all of the accumulated SizeChanged events
//
//-------------------------------------------------------------------------
void CLayoutManager::RaiseSizeChangedEvents()
{
    Jupiter::stack_vector<SizeChangedQueueItem, c_sizeChangedVectorSize> tmp;

    // While raising the events, more size changed requests might get queued
    // into m_sizeChangedQueue. So we repeatedly move items into a tmp vector
    // and raise the events until sizeChangedQueue gets empty.
    while (!m_sizeChangedQueue.empty())
    {
        // There's no guarantee that the collection won't change as a side
        // effect of raising the event on an item, so we use a temp stack vector
        // of the items to prevent using an invalid iterator.
        tmp.m_vector.reserve(m_sizeChangedQueue.size()); // avoid reallocations

        // Note the move instead of copy here to avoid doing an extra
        // addref/release on the item. Also note that the order in which
        // we call the event is reversed.
        std::for_each(m_sizeChangedQueue.rbegin(),
            m_sizeChangedQueue.rend(),
            [&tmp](auto& item)
            {
                tmp.m_vector.push_back(std::move(item));
            });

        m_sizeChangedQueue.clear();

        for (auto& item : tmp.m_vector)
        {
            TraceIndividualSizeChangedBegin();

            if (auto layoutStorage = item.m_pElement->GetLayoutStorage())
            {
                xref_ptr<CSizeChangedEventArgs> args = make_xref<CSizeChangedEventArgs>(item.m_oldSize, item.m_pElement->RenderSize);
                // AddRef for args is done on the managed side

                IGNOREHR(FxCallbacks::JoltHelper_RaiseEvent(
                    item.m_pElement,
                    DirectUI::ManagedEvent::ManagedEventSizeChanged,
                    args));
            }

            TraceIndividualSizeChangedEnd(UINT64(item.m_pElement));
        }

        tmp.m_vector.clear();
    }
}

_Check_return_ HRESULT CLayoutManager::EnqueueForEffectiveViewportChanged(
    _In_ CFrameworkElement* element,
    const XRECTF effectiveViewport,
    const XRECTF maxViewport,
    const DOUBLE bringIntoViewDistanceX,
    const DOUBLE bringIntoViewDistanceY)
{
    IFC_RETURN(element->PegManagedPeer());

    m_effectiveViewportChangedQueue.emplace_back(
            element,
            effectiveViewport,
            maxViewport,
            bringIntoViewDistanceX,
            bringIntoViewDistanceY);

    return S_OK;
}

void CLayoutManager::RaiseEffectiveViewportChangedEvents()
{
    CEventManager *eventManager = m_pCoreServices->GetEventManager();
    ASSERT(eventManager);

    for (auto& item : m_effectiveViewportChangedQueue)
    {
        CFrameworkElement* element = item.GetElement();
        if (element->ShouldRaiseEvent(KnownEventIndex::FrameworkElement_EffectiveViewportChanged))
        {
            CREATEPARAMETERS cp(m_pCoreServices);

            xref_ptr<CEffectiveViewportChangedEventArgs> args;
            args.init(new CEffectiveViewportChangedEventArgs(
                item.GetEffectiveViewport(),
                item.GetMaxViewport(),
                item.GetBringIntoViewDistanceX(),
                item.GetBringIntoViewDistanceY()));

            eventManager->Raise(
                EventHandle(KnownEventIndex::FrameworkElement_EffectiveViewportChanged),
                TRUE /* bRefire */,
                element,
                args,
                TRUE /* fRaiseSync */);

            element->UnpegManagedPeer();
        }
    }

    m_effectiveViewportChangedQueue.clear();
}


#ifdef DEBUG
void CLayoutManager::DumpTree()
{
    CUIElement* pRoot = NULL;

    if (m_pVisualTree)
    {
        pRoot = (CUIElement*)m_pVisualTree->GetRootVisual();
    }

    if (!pRoot)
        return;

    DumpTree(pRoot, 0);
}

void CLayoutManager::DumpTree(CUIElement* pNode, int depth)
{
    {
        XStringBuilder bufferBuilder;

        if (FAILED(bufferBuilder.Initialize()))
        {
            return;
        }

        const MetaDataType2* pInfo = pNode->GetClassInformation();

        for (int i = 0; i < depth; ++i)
        {
            IGNORERESULT(bufferBuilder.Append(L"\t", 1));
        }

        IGNORERESULT(bufferBuilder.Append(pInfo->GetName()));
        IGNORERESULT(bufferBuilder.Append(L"\t", 1));

        if (pNode->GetIsAncestorDirty())
        {
            IGNORERESULT(bufferBuilder.Append(L"A", 1));
        }

        if (pNode->GetIsOnMeasureDirtyPath())
        {
            IGNORERESULT(bufferBuilder.Append(L"P", 1));
        }

        if (pNode->GetIsMeasureDirty())
        {
            IGNORERESULT(bufferBuilder.Append(L"M", 1));
        }

        gps->DebugOutputSz(bufferBuilder.GetBuffer());
    }

    CCollection* pChildren = pNode->GetChildren();

    if (pChildren)
    {
        for (XUINT32 index = 0; index < pChildren->GetCount(); ++index)
        {
            CUIElement* pChild = (CUIElement*) pChildren->GetItemWithAddRef(index);
            DumpTree(pChild, depth + 1);
            ReleaseInterface(pChild);
        }
    }
}
#endif

//-------------------------------------------------------------------------
//  Synopsis:   Indicates an element should be transitioned.
//
//  Called when registering transitions on layouttransitionstorage.
//  Note: we do not normally unregister. We'll just leave the pTarget in
//  and remove it during realization phase.
//-------------------------------------------------------------------------
void CLayoutManager::RegisterElementForDeferrredTransition(_In_ CUIElement* pTarget, _In_ LayoutTransitionStorage* pStorage)
{
    if (!pStorage->m_bRegisteredInLayoutManager)
    {
        m_elementsWithDeferredTransitions.emplace_back(pTarget);
        pStorage->m_bRegisteredInLayoutManager = true;
    }
}

//-------------------------------------------------------------------------
//  Synopsis:   Indicates an element should not be transitioned.
//
//  We only clear from this vector if the UIElement is actively being disposed.
//  Otherwise we will leave this UIElement in, and during realization phase
//  it will just be skipped (perf optimization).
//-------------------------------------------------------------------------
void CLayoutManager::UnregisterElementForDeferredTransition(_In_ CUIElement* pTarget, _In_ LayoutTransitionStorage* pStorage)
{
    if (pStorage->m_bRegisteredInLayoutManager && m_elementsWithDeferredTransitions.size() > 0)
    {
        auto toErase = std::find_if(
            m_elementsWithDeferredTransitions.begin(),
            m_elementsWithDeferredTransitions.end(), 
            [pTarget](const auto& item){ return item.get() == pTarget; });

        if(toErase != m_elementsWithDeferredTransitions.end())
        {
            m_elementsWithDeferredTransitions.erase(toErase);
            pStorage->m_bRegisteredInLayoutManager = false;
        }

        ASSERT(std::find_if(m_elementsWithDeferredTransitions.begin(), 
            m_elementsWithDeferredTransitions.end(), 
            [pTarget](const auto& item){ return item.get() == pTarget; }) == m_elementsWithDeferredTransitions.end() ,
            L"should not have found an element registered multiple times in the deferred transitions list");
    }

    ASSERT(!pStorage->m_bRegisteredInLayoutManager);
}

_Check_return_ 
HRESULT 
CLayoutManager::GetPosition(_In_ CUIElement* element, _In_ Jupiter::stack_vector<std::pair<CUIElement*, XRECTF>, 16>& cachedPositions, _Out_ XRECTF* position)
{
    auto foundPosition = std::find_if(
        cachedPositions.m_vector.begin(),
        cachedPositions.m_vector.end(),
        [element](auto const& item) { return item.first == element;  } );
    if (foundPosition != cachedPositions.m_vector.end())
    {
        *position = foundPosition->second;
    }
    else if (element)
    {
        xref_ptr<ITransformer> transformer;
        IFC_RETURN(element->TransformToRoot(transformer.ReleaseAndGetAddressOf()));
        IFC_RETURN(CTransformer::TransformBounds(transformer.get(), position, position));
        cachedPositions.m_vector.emplace_back(element, *position);
    }

    return S_OK;
}

ElementRectPairVector* CLayoutManager::GetOrCreateCachedElementRects(_In_ CTransition* transition, _In_ Jupiter::stack_vector<std::pair<xref_ptr<CTransition>, std::unique_ptr<ElementRectPairVector>>, 16>& cachedGroups)
{
    auto foundGroupItem = std::find_if(
                cachedGroups.m_vector.begin(),
                cachedGroups.m_vector.end(),
                [transition](auto const& item) { return item.first == transition; }); 
    if (foundGroupItem != cachedGroups.m_vector.end())
    {
        return foundGroupItem->second.get();
    }
    else
    {
        std::unique_ptr<ElementRectPairVector> elementRects;
        if(!m_elementRectPool.empty())
        {
            elementRects = std::move(m_elementRectPool.back());
            m_elementRectPool.pop_back(); 
        }
        else
        {
            elementRects = std::make_unique<ElementRectPairVector>();
            elementRects->reserve(8);
        }

        auto result = elementRects.get();
        cachedGroups.m_vector.emplace_back(transition, std::move(elementRects));

        return result;
    }
}

//-------------------------------------------------------------------------
//
//  Synopsis:   Create all the layouttransitions that have been registered.
//
//  Groups all the elements (and their layoutrects) per layouttransition,
//  raise the starting event and then setup the transition.
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CLayoutManager::RealizeRegisteredLayoutTransitions()
{
    HRESULT hr = S_OK;
    // arrays we send to managed to match up with starting events
    // note indices all need to match up
    Jupiter::stack_vector<XRECTF, 16> rectVector;    // represents the layoutRect of all elements being animated by this layoutmanager
    Jupiter::stack_vector<CUIElement*, 16> elementVector;    // represents the elements being animated
    Jupiter::stack_vector<XFLOAT, 16> delayVector;       // stagger durations, marshalled for perf as longs

    m_isInTransitionRealization = TRUE;

    // we group all layouttransitions per registering layouttransition object
    Jupiter::stack_vector<std::pair<xref_ptr<CTransition>, std::unique_ptr<ElementRectPairVector>>, 16> groups;
    // optimize getting parent positions. Most element will have the same parent.
    Jupiter::stack_vector<std::pair<CUIElement*, XRECTF>, 16> parentPositions;
    // keep a list of targets that we are setting up transitions for
    Jupiter::stack_vector<xref_ptr<CUIElement>, 16> elementsTransitioned;

    TraceRealizeTransitionBegin();

    // fill the groups dictionary with layoutslots
    for(const auto& element: m_elementsWithDeferredTransitions)
    {
        LayoutTransitionStorage* pStorage = element->GetLayoutTransitionStorage();
        IFCEXPECT(pStorage);
        ASSERT(!pStorage->m_registeredTransitions.empty()); // should have unregistered with deferredtransitions though.       

        if (pStorage->GetTrigger() != DirectUI::TransitionTrigger::NoTrigger)
        {
            // get the layoutslot as we will animate it
            XRECTF parentPosition = {};
            CUIElement* pParent = do_pointer_cast<CUIElement>(element->GetParentInternal(false));
            IFC(GetPosition(pParent, parentPositions, &parentPosition));

            XRECTF layoutSlot  = {  
                parentPosition.X + pStorage->m_transformStart.GetDx(),
                parentPosition.Y + pStorage->m_transformStart.GetDy(),
                pStorage->m_sizeStart.width,
                pStorage->m_sizeStart.height};
            // fill the groups
            for(const auto& transition : pStorage->m_registeredTransitions)
            {    
                auto elementRects = GetOrCreateCachedElementRects(transition.get(), groups);
                // finally store the combination of the layoutrect and the element for later use
                elementRects->emplace_back(element.get(), layoutSlot);
            }
        }

        // the call to unregister will remove from deferredstorage as well, which will be slow
        // since we _know_ we will clear out the deferred vector, lets skip that
        pStorage->m_bRegisteredInLayoutManager = false;

        pStorage->UnregisterElementForTransitions(element.get());   // note how the trigger remains untouched.
    }

    // stage two:
    // ** StartingEvent** call into managed if needed once, with all this information
    //    This will let the managed side spit out several sync events for each element..  NOTE: currently removed, until we bring back layouttransition type
    // ** Staggering ** call into managed if needed once per layouttransition where individual calls are
    //    made to the easingfunction
    for(const auto& groupItem: groups.m_vector)
    {
        CTransition* transition = groupItem.first;
        auto& elementRects =  groupItem.second;
        size_t elementCount = elementRects->size();

        delayVector.m_vector.clear();
        delayVector.m_vector.resize(elementCount, 0);

        ASSERT(elementRects);
        ASSERT(elementCount > 0);
        
        // handle staggering
        if (transition->m_pStaggerFunction && transition->GetIsStaggeringEnabled())
        {
            rectVector.m_vector.clear();
            elementVector.m_vector.clear();
            for(const auto& pElementRectPair: *elementRects)
            {
                // fill the arrays
                elementVector.m_vector.emplace_back(pElementRectPair.first);
                rectVector.m_vector.emplace_back(pElementRectPair.second);
            }

            IFC(CStaggerFunctionBase::GetTransitionDelayValues(
                transition->m_pStaggerFunction,
                static_cast<XUINT32>(elementCount),
                elementVector.m_vector.data(),
                rectVector.m_vector.data(),
                delayVector.m_vector.data())); // delayVector can take elementCount items
        }

        // now do the actual work of calling setuptransition for each element
        // passing in the storyboard that we might have gotten from the aggregated starting event.
        for(XUINT32 i=0; i < elementCount; i++)
        {
            const auto& pElementRectPair = (*elementRects)[i];
            CUIElement *transitionedElement = pElementRectPair.first;
            // puts the target in a list so that we can later validate that storyboards were setup
            elementsTransitioned.m_vector.emplace_back(transitionedElement);
            
            // will create and call begin on storyboards
            IFC(CTransition::SetupTransition(transitionedElement, transition, delayVector.m_vector[i]));
        }
    }

    // iterate over elements that have been called for SetupTransition and make sure they are
    // actually with storyboard
    for (const auto transitionTarget : elementsTransitioned.m_vector)
    {
        // if they did not give a storyboard, we should cancel the transition immediately
        IFC(CTransition::ValidateTransitionWasSetup(transitionTarget));
    }

Cleanup:
    // Now that we are done with the element rects vector, we cache it in pool member vector
    // so that it can be reused. This way we avoid repeated allocations of these vectors only 
    // to throw them away after this method. 
    for(auto& groupItem: groups.m_vector)
    {
        groupItem.second->clear();
        m_elementRectPool.push_back(std::move(groupItem.second));
    }
    
    m_isInTransitionRealization = FALSE;
    m_elementsWithDeferredTransitions.clear();
    TraceRealizeTransitionEnd();
    
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:   Walks the do tree to visit all layouttransition dirty elements.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CLayoutManager::TransitionLayout()
{
    HRESULT hr = S_OK;
    CUIElement* pRoot = NULL;

    if (m_pVisualTree)
    {
        pRoot = do_pointer_cast<CUIElement>(m_pVisualTree->GetRootVisual());
    }

    if(!pRoot)
    {
        RRETURN(S_OK);
    }

    ASSERT(!m_isInLayoutTransitionPhase);

    m_isInLayoutTransitionPhase = TRUE;

    IFC(pRoot->TransitionLayout(pRoot, m_arrangeRect));

Cleanup:
    m_isInLayoutTransitionPhase = FALSE;
    RRETURN(hr);

}

//-------------------------------------------------------------------------
//
//  Synopsis:   Advance the layout counter. See: GetLayoutCounter, GetNextLayoutCounter.
//
//-------------------------------------------------------------------------
void
CLayoutManager::IncrementLayoutCounter()
{
    m_layoutCounter = GetNextLayoutCounter();
}

void
CLayoutManager::IncreaseLayoutUpdatedSubscriberCounter()
{
    m_nLayoutUpdatedSubscriberCounter++;
}

void
CLayoutManager::DecreaseLayoutUpdatedSubscriberCounter()
{
    ASSERT(m_nLayoutUpdatedSubscriberCounter);
    m_nLayoutUpdatedSubscriberCounter--;
}

void CLayoutManager::PushCurrentLayoutElement(CUIElement* element)
{
    m_layoutElementStack.push_front(element);
}

CUIElement* CLayoutManager::PopCurrentLayoutElement()
{
    auto toPop = m_layoutElementStack.front();
    m_layoutElementStack.pop_front();

    return toPop;
}

void CLayoutManager::PropagateAncestorDirtyFromCurrentLayoutElement(CUIElement* targetAncestor)
{
    for (auto element : m_layoutElementStack)
    {
        if (element == targetAncestor)
        {
            return; // found the target, so we're done
        }
        else
        {
            element->SetIsAncestorDirty(TRUE);
        }
    }

    // should only end up here if the target wasn't found
    ASSERT(false);
}

void CLayoutManager::EnqueueElementInsertion(
    _In_ CDependencyObject* realizedElement)
{
    ASSERT(std::find(
        m_deferredRealizedElementInsertions.begin(),
        m_deferredRealizedElementInsertions.end(),
        realizedElement) == m_deferredRealizedElementInsertions.end());

    m_deferredRealizedElementInsertions.emplace_back(realizedElement);
}

_Check_return_ HRESULT CLayoutManager::ProcessElementInsertions()
{
    while (!m_deferredRealizedElementInsertions.empty())
    {
        xref_ptr<CDependencyObject> current;
        std::swap(current, m_deferredRealizedElementInsertions.front());
        m_deferredRealizedElementInsertions.pop_front();

        CDeferredElement* proxy = nullptr;
        IFC_RETURN(current->GetRealizingProxy(&proxy));

        if (proxy)
        {
            // Element could have been redeferred before insertion, in such case realizing proxy would be cleared.
            // In this scenario, ignore insertion and drop reference to realized object.
            IFC_RETURN(proxy->FinalizeElementInsertion(current.get()));
        }
    }

    return S_OK;
}
