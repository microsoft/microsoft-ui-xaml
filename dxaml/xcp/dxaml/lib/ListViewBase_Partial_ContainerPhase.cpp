// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ListViewBase.g.h"
#include "SelectorItem.g.h"
#include "ContainerContentChangingEventArgs.g.h"
#include "Panel.g.h"
#include "ModernCollectionBasePanel.g.h"
#include "ScrollViewer.g.h"
#include <DependencyLocator.h>
#include "XamlTraceLogging.h"
#include "XamlTraceSession.h"
#include "XamlBindingHelper.g.h"
#include "BuildTreeService.g.h"
#include "BudgetManager.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Work around disruptive max/min macros
#undef max
#undef min

_Check_return_ HRESULT ListViewBase::SetDesiredContainerUpdateDurationImpl(_In_ wf::TimeSpan duration)
{
    // duration is expressed in 100-nanosecond units
    // I will need ms.
    m_budget = static_cast<UINT>(duration.Duration) / 10000;
    RRETURN(S_OK);
}

// Determines whether we want to defer the prepare of this container
// In the case of ListViewBase, this is determined by external API
_Check_return_ HRESULT ListViewBase::IsPrepareContainerForItemDeferred(_In_ xaml::IDependencyObject* element, _Out_ BOOLEAN* pDefer)
{
    HRESULT hr = S_OK;
    UIElement::VirtualizationInformation* pVirtualizationInformation = nullptr;
    BOOLEAN defer = FALSE;

    // listview supports deferring setting the content.
    // is this a new style container? We can know by looking at the virtualizationInformation struct which is
    // a ModernCollectionBase concept

    ASSERT(ctl::is<IUIElement>(element));

    // m_disableScrollingPlaceholders may be set to temporarily disable display of placeholders.
    // This is typically the case when switching between views of SemanticZoom on phone
    if (!m_disableScrollingPlaceholders)
    {
        // doing the static_casts to not pay for the QI
        pVirtualizationInformation = static_cast<UIElement*>(static_cast<DependencyObject*>(element))->GetVirtualizationInformation();

        if (pVirtualizationInformation)
        {
            auto item = pVirtualizationInformation->GetItem();

            if (!ctl::is<IUIElement>(item))
            {
                ctl::ComPtr<IUIElement> spElement;
                ctl::ComPtr<IPanel> spPanel;
                ctl::ComPtr<IChildTransitionContextProvider> spTCP;
                BOOLEAN isIgnoringTransitions = FALSE;

                IFC(get_ItemsHost(&spPanel));

                // If we are ignoring transitions then it means we are in the middle of scrolling.
                // In this case always show scrolling placeholders.
                IFC(spPanel.Cast<Panel>()->get_IsIgnoringTransitions(&isIgnoringTransitions));

                if (!isIgnoringTransitions)
                {
                    IFC(ctl::do_query_interface<IUIElement>(spElement, element));
                    spTCP = spPanel.AsOrNull<IChildTransitionContextProvider>();

                    if (spTCP)
                    {
                        INT layoutTick = 0;
                        ThemeTransitionContext tt;

                        IFC(CoreImports::LayoutManager_GetLayoutTickForTransition(static_cast<CUIElement*>(static_cast<UIElement*>(static_cast<DependencyObject*>(element))->GetHandle()), (XINT16*)&layoutTick));

                        IFC(spTCP->GetChildTransitionContext(spElement.Get(), layoutTick, &tt));

                        if (tt != ThemeTransitionContext::None)
                        {
                            goto Cleanup;
                        }
                    }
                }

                // we're going to switch behavior on the existence of this struct.
                IFC(get_ShowsScrollingPlaceholders(&defer));
            }
        }
    }

Cleanup:
    *pDefer = defer;
    RRETURN(hr);
}

// Called after prepare. This will raise the event and put us into the queue
_Check_return_ IFACEMETHODIMP
ListViewBase::SetupContainerContentChangingAfterPrepare(
    _In_ xaml::IDependencyObject* container,
    _In_ IInspectable* item,
    _In_ INT itemIndex,
    _In_ wf::Size measureSize)
{
    TraceSetupCCCBegin(itemIndex);
    ctl::ComPtr<xaml::IDependencyObject> itemAsDO;

    // this is being called by modern panels after the prepare has occurred.
    // we will setup information that we will need during the lifetime of this container

    ctl::ComPtr<ISelectorItem> containerAsISI;
    ctl::ComPtr<xaml_controls::IContainerContentChangingEventArgs> args;
    ctl::ComPtr<ContainerContentChangingEventArgs> argsConcrete;
    ctl::ComPtr<IUIElement> templateRoot;
    BOOLEAN handled = FALSE;
    bool measureContentAfterCode = false;

    // raw pointer, since we're not a refcounted object
    UIElement::VirtualizationInformation* virtualizationInformation = nullptr;
    BOOLEAN showingPlaceholder = FALSE;
    itemAsDO = ctl::query_interface_cast<xaml::IDependencyObject>(item);

    // If the item is an element, there is no reason to do any of this fancy work
    // see the if condition in prepare
    if (itemAsDO.Get() != container)
    {
        containerAsISI = ctl::query_interface_cast<ISelectorItem>(container);
        if (containerAsISI)
        {
            // is this a new style container? We can know by looking at the virtualizationInformation struct which is
            // a ModernCollectionBase concept
            virtualizationInformation = containerAsISI.AsOrNull<IUIElement>().Cast<UIElement>()->GetVirtualizationInformation();
            if (virtualizationInformation)
            {
                // we're going to switch behavior on the existence of this struct.

                // it is somewhat expensive to call ShowsScrollingPlaceholders and we have already called it
                // in ItemsControl::PrepareContainerForItemOverride. The result there has been to
                // set us as UIPlaceholder, so instead of doing this call:
                // IFC(get_ShowsScrollingPlaceholders(&deferredBehavior));
                // we will do this call:
                showingPlaceholder = containerAsISI.Cast<SelectorItem>()->GetIsUIPlaceholder();
                IFC_RETURN(virtualizationInformation->GetBuildTreeArgs(&args));
                argsConcrete = args.Cast<ContainerContentChangingEventArgs>();
            }
        }

        ASSERT(virtualizationInformation);
        ASSERT(argsConcrete);

        // store the size we would measure with
        virtualizationInformation->SetMeasureSize(measureSize);

        // initialize values in the args
        IFC_RETURN(argsConcrete->put_WantsCallBack(FALSE));              // let them explicitly call-out if they want it
        IFC_RETURN(argsConcrete->put_Item(item));                       // there is now a hard ref
        IFC_RETURN(argsConcrete->put_InRecycleQueue(FALSE));
        IFC_RETURN(argsConcrete->put_Phase(0));
        IFC_RETURN(argsConcrete->put_ItemIndex(itemIndex));              // note how this is passed in, it is very expensive to calculate on the fly
        IFC_RETURN(argsConcrete->put_Handled(FALSE));
        argsConcrete->SetBindingPhase(ContainerContentChangingEventArgs::NoBindingNeededToken);                   // start off with no bindingphase needed

        // force measure. This will be no-op since content has not been set/changed
        // but we need it for the contenttemplateroot
        IFC_RETURN(containerAsISI.Cast<SelectorItem>()->Measure(measureSize));
        const bool isContainerFromTemplateRoot = virtualizationInformation->GetIsContainerFromTemplateRoot();

        // raise the event. This is the synchronous version. we will raise it 'async' as well when we have time
        // but we guarantee calling it after prepare
        if (ShouldRaiseEvent(KnownEventIndex::ListViewBase_ContainerContentChanging))
        {
            ContainerContentChangingEventSourceType* pEventSource = nullptr;
            ctl::ComPtr<ListViewBase> thisAsLVB = this;
            IFC_RETURN(GetContainerContentChangingEventSourceNoRef(&pEventSource));

            IFC_RETURN(pEventSource->Raise(thisAsLVB.AsOrNull<xaml_controls::IListViewBase>().Get(), argsConcrete.Get()));
            IFC_RETURN(argsConcrete->get_Handled(&handled));

            if (handled)
            {
                // never set the content any more
                // the placeholder will be removed before the callback if there is one
                IFC_RETURN(argsConcrete->put_ContentShouldBeSet(FALSE));
            }
            else
            {
                if (showingPlaceholder)
                {
                    IFC_RETURN(argsConcrete->put_ContentShouldBeSet(TRUE));
                }
                else
                {
                    // default behavior of setting the content should be performed
                    // this means that bindings and so on will work by default, but some big cost is being paid
                    if (!isContainerFromTemplateRoot)
                    {
                        IFC_RETURN(containerAsISI.Cast<SelectorItem>()->put_Content(item));
                    }

                    IFC_RETURN(argsConcrete->put_ContentShouldBeSet(FALSE));
                    measureContentAfterCode = true;
                }
            }
        }
        else
        {
            // app code did not hook the event
            if (showingPlaceholder)
            {
                // if we were showing placeholders we need to set the content
                // before we do the (implicit) callback
                IFC_RETURN(argsConcrete->put_ContentShouldBeSet(TRUE));
            }
            else
            {
                // app code did not hook the event and is not showing placeholders
                // we should set content now
                IFC_RETURN(argsConcrete->put_ContentShouldBeSet(FALSE));
                if (!isContainerFromTemplateRoot)
                {
                    IFC_RETURN(containerAsISI.Cast<SelectorItem>()->put_Content(item));
                }

                measureContentAfterCode = true;
            }
        }

        // above we did the user code callbacks, now deal with the x:bind facility
        // xbind will put an extension on the root of the datatemplate tree
        if (isContainerFromTemplateRoot)
        {
            // If root of the template is the container, the container itself will have the extension
            templateRoot = containerAsISI.AsOrNull<IUIElement>();

            // We just want to pass along data context in this case, so that bindings continue to work.
            if (auto containerAsFE = containerAsISI.AsOrNull<xaml::IFrameworkElement>())
            {
                IFC_RETURN(containerAsFE->put_DataContext(item));
            }

            // We do not want to set content ever if container is from item template.
            IFC_RETURN(argsConcrete->put_ContentShouldBeSet(FALSE));
        }
        else
        {
            IFC_RETURN(containerAsISI.Cast<SelectorItem>()->get_ContentTemplateRoot(&templateRoot));
        }

        if (templateRoot)
        {
            ctl::ComPtr<IDataTemplateComponent> templateComponent;
            IFC_RETURN(DirectUI::XamlBindingHelperFactory::GetDataTemplateComponentStatic(templateRoot.AsOrNull<IDependencyObject>().Get(), &templateComponent));
            if (templateComponent)
            {
                // There is a DataTemplateComponent, use that.
                INT requestedPhase = -1;
                IFC_RETURN(templateComponent->ProcessBindings(item, itemIndex, 0 /* phase */, &requestedPhase));
                // the public api on ProcessBinding for an extension is to not register for the callback like the app will do,
                // but return the phase it wants to be called back.
                argsConcrete->SetBindingPhase(requestedPhase);
                measureContentAfterCode = true;
            }
            else
            {
                // Fallback to using DataTemplateExtension to be compatible with apps built using older sdks.
                ctl::ComPtr<IDataTemplateExtension> extension;
                IFC_RETURN(DirectUI::DataTemplateFactory::GetExtensionInstanceStatic(templateRoot.AsOrNull<IFrameworkElement>().Get(), &extension));
                if (extension)
                {
                    // yep, there is an extension
                    HRESULT hr2 = S_OK;
                    INT requestedPhase = -1;
                    UINT nextAppCallback = 0;
                    // app usage might have changed the phase
                    IFC_RETURN(argsConcrete->get_Phase(&nextAppCallback));
                    IFC_RETURN(argsConcrete->put_Phase(0));  // working on phase 0

                    hr2 = extension->ProcessBindings(args.Get(), &requestedPhase);

                    // put back what the app expected
                    IFC_RETURN(argsConcrete->put_Phase(nextAppCallback));

                    // the public api on ProcessBinding for an extension is to not register for the callback like the app will do,
                    // but return the phase it wants to be called back.
                    argsConcrete->SetBindingPhase(requestedPhase);

                    IFC_RETURN(hr2);

                    measureContentAfterCode = true;
                }
            }
        }

        if (measureContentAfterCode)
        {
            // need to measure, so we keep the budget as fair as possible
            IFC_RETURN(containerAsISI.Cast<SelectorItem>()->Measure(measureSize));
            IFC_RETURN(containerAsISI.Cast<SelectorItem>()->UpdateVisualState(TRUE /* bUseTransitions */));
        }

        TraceVirtualizedItemAddedInfo(
            (UINT64)containerAsISI.Cast<SelectorItem>()->GetHandle(),
            (UINT64)(m_tpScrollViewer.Get() != nullptr ? m_tpScrollViewer.Cast<ScrollViewer>()->GetHandle() : nullptr),
            itemIndex,
            showingPlaceholder
        );

        IFC_RETURN(RegisterWorkFromArgs(argsConcrete.Get()));
    }

    TraceSetupCCCEnd();
    return S_OK;
}

_Check_return_ IFACEMETHODIMP ListViewBase::RegisterWorkForContainer(
    _In_ xaml::IUIElement* pContainer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IContainerContentChangingEventArgs> spArgs;

    IFC(static_cast<UIElement*>(pContainer)->GetVirtualizationInformation()->GetBuildTreeArgs(&spArgs));
    if (spArgs)
    {
        IFC(RegisterWorkFromArgs(spArgs.Get()));
    }

Cleanup:
    return hr;
}

// Register for work from the BuildTree service based on the given args
_Check_return_ IFACEMETHODIMP
ListViewBase::RegisterWorkFromArgs(
    _In_ IContainerContentChangingEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    BOOLEAN wantsAppCallback = FALSE;
    bool wantsxbindingCallback = false;
    ctl::ComPtr<ISelectorItem> spContainer;
    auto const pConcreteArgsNoRef = static_cast<ContainerContentChangingEventArgs*>(pArgs);

    IFC(pConcreteArgsNoRef->get_WantsCallBack(&wantsAppCallback));
    wantsxbindingCallback = pConcreteArgsNoRef->GetWantsBindingPhaseCallback();
    IFC(pConcreteArgsNoRef->get_ItemContainer(&spContainer));

    // we are going to want to be called back if:
    // 1. we are still showing the placeholder
    // 2. app code registered to be called back
    // 3. binding code registered to be called back
    if (spContainer.Cast<SelectorItem>()->GetIsUIPlaceholder() || wantsAppCallback || wantsxbindingCallback)
    {
        INT64 phase = 1;

        // we will to the lowest phase of the set that the app and the binding has asked us to go to
        if (wantsAppCallback)
        {
            UINT phaseArgs = 0;
            IFC(pConcreteArgsNoRef->get_Phase(&phaseArgs));
            phase = phaseArgs;
        }

        // default behavior is that this has elevated us to phase 1.
        // the static cast is fine here

        // put in the public phase that the user has requested (phase)
        IFC(pConcreteArgsNoRef->put_Phase(static_cast<INT>(phase)));

        // keep this state on the listviewbase
        if (m_lowestPhaseInQueue == -1)
        {
            // there was nothing registered
            m_lowestPhaseInQueue = phase;

            // that means we need to register ourselves with the buildtreeservice so that
            // we can get called back to do some work
            if (!m_isRegisteredForCallbacks)
            {
                ctl::ComPtr<BuildTreeService> spBuildTree;
                IFC(DXamlCore::GetCurrent()->GetBuildTreeService(spBuildTree));
                IFC(spBuildTree->RegisterWork(this));
            }

            ASSERT(m_isRegisteredForCallbacks);
        }
        else
        {
            m_lowestPhaseInQueue = std::min(m_lowestPhaseInQueue, phase);
        }

        // could be that the user wanted phase 5, but there is a x:bind for phase 2
        if (wantsxbindingCallback)
        {
            m_lowestPhaseInQueue = std::min(m_lowestPhaseInQueue, static_cast<INT64>(pConcreteArgsNoRef->GetBindingPhase()));
        }
    }
    else
    {
        // well, app code doesn't want a callback, and we don't need one (no placeholder mode)
        // so cleanup the args
        IFC(pConcreteArgsNoRef->ResetLifetime());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBase::RegisterIfWorkPending()
{
    if (m_lowestPhaseInQueue != -1 && !m_isRegisteredForCallbacks)
    {
        ctl::ComPtr<BuildTreeService> spBuildTree;
        IFC_RETURN(DXamlCore::GetCurrent()->GetBuildTreeService(spBuildTree));
        IFC_RETURN(spBuildTree->RegisterWork(this));
    }

    return S_OK;
}

_Check_return_ HRESULT ListViewBase::IsBuildTreeSuspendedImpl(_Out_ BOOLEAN* pReturnValue)
{
    *pReturnValue = GetHandle()->IsCollapsed() || !GetHandle()->AreAllAncestorsVisible();
    return S_OK;
}

// the async version of doWork that is being called by NWDrawTree
_Check_return_ HRESULT ListViewBase::BuildTreeImpl(_Out_ BOOLEAN *workLeft) noexcept
{
    INT timeElapsedInMS = 0;
    ctl::ComPtr<BudgetManager> budget;
    UINT containersToClearCount = 0;
    ctl::ComPtr<IPanel> itemsHost;
    ctl::ComPtr<ICustomGeneratorItemsHost> customGeneratorPanel;

    *workLeft = TRUE;

    IFC_RETURN(DXamlCore::GetCurrent()->GetBudgetManager(budget));
    IFC_RETURN(budget->GetElapsedMilliSecondsSinceLastUITick(&timeElapsedInMS));

    if (static_cast<UINT>(timeElapsedInMS) <= m_budget)
    {
        IFC_RETURN(get_ItemsHost(&itemsHost));
        if (itemsHost)
        {
            customGeneratorPanel = itemsHost.AsOrNull<ICustomGeneratorItemsHost>();
        }

        // we are going to do different types of work:
        // 1. process incremental visualization
        // 2. process deferred clear container work
        // 3. process extra cache

        // at this point, cache indices are set correctly
        // We might be going several passes over the containers. Currently we are not keeping those containers
        // in a particular datastructure, but we are re-using the children collection on our moderncollectionbasepanel
        // We also have a nice hint (m_lowestPhaseInQueue) that tells us what phase to look out for. While we are doing our
        // walk, we're going to build up a structure that allows us to do the second walks much faster.
        // When we are done, we'll throw it away.

        // we do not want to do incremental loading when we are not in the live tree.
        const bool isInLiveTree = IsInLiveTree() && itemsHost && itemsHost.Cast<Panel>()->IsInLiveTree();

        // 1. process incremental visualization
        if (m_lowestPhaseInQueue > -1 && customGeneratorPanel && isInLiveTree)
        {
            // A block structure has been considered, but we do expect to continuously mutate the phase on containers, which would have
            // cost us perf while reflecting that in the blocks. Instead, I keep an array of the size of the amount of containers i'm interested in.
            // The idea is that walking through that multiple times is still pretty darn fast.

            // Do not hold on spContainersIterator outside of this frame. Its state won't be valid outside of it.
            ctl::ComPtr<IContainerContentChangingIterator> containersIterator;
            IFC_RETURN(customGeneratorPanel->GetContainersForIncrementalVisualization(&containersIterator));

            UINT containersCount = 0;
            if (containersIterator)
            {
                IFC_RETURN(containersIterator->get_Size(&containersCount));
                if (containersCount == 0)
                {
                    // Nothing to do.
                    m_lowestPhaseInQueue = -1;
                }
            }
            else
            {
                // Nothing to do.
                m_lowestPhaseInQueue = -1;
            }

            if (m_lowestPhaseInQueue != -1)
            {
                // trying to find containers in this phase
                INT64 processingPhase = m_lowestPhaseInQueue;

                // when we are done with a full iteration, will be looking for the nextlowest phase
                // note: INT64 here so we can set it to a max that is out of range of the public phase property which is INT
                // max is used to indicate there is nothing left to work through.
                INT64 nextLowest = std::numeric_limits<INT64>::max();

                // the array that we keep iterating until we're done
                // -1 means, not fetched yet
                std::vector<INT64> lookup;
                lookup.assign(containersCount, -1);

                ASSERT(containersCount > 0);
                UINT currentPositionInVector = -1;
                auto ProcessCurrentPosition =
                    [&containersIterator,
                    &currentPositionInVector,
                    &containersCount,
                    &processingPhase,
                    &nextLowest]()
                {
                    BOOLEAN thereIsMore;
                    HRESULT hr = containersIterator->MoveNext(&thereIsMore);
                    currentPositionInVector = (currentPositionInVector + 1) % containersCount;

                    // we increase the phase when we loop back to the beginning.
                    if (SUCCEEDED(hr) && !thereIsMore)
                    {
                        processingPhase = nextLowest;
                        // when increasing, signal using max() that we can stop after this iteration.
                        // this will be undone by the loop, resetting it to the actual value
                        nextLowest = std::numeric_limits<INT64>::max();

                        hr = containersIterator->Reset();
                        if (SUCCEEDED(hr))
                        {
                            hr = containersIterator->MoveNext(&thereIsMore);
                        }
                    }
                    return hr;
                };

                // initialize before going into the loop
                IFC_RETURN(ProcessCurrentPosition());

                while (processingPhase != std::numeric_limits<INT64>::max() &&
                    static_cast<UINT>(timeElapsedInMS) < m_budget)
                {
                    INT64 phase = 0;
                    ctl::ComPtr<xaml::IDependencyObject> container;
                    UIElement::VirtualizationInformation* virtualizationInformation = nullptr;
                    ctl::ComPtr<xaml_controls::IContainerContentChangingEventArgs> args;
                    ctl::ComPtr<ContainerContentChangingEventArgs> argsConcrete;
                    // always update the current position when done, except when the phase requested is lower than current phase
                    bool shouldUpdateCurrentPosition = true;

                    // what is the phase?
                    phase = lookup[currentPositionInVector];
                    if (phase == -1)
                    {
                        // not initialized yet
                        UINT argsPhase = 0;
                        IFC_RETURN(containersIterator->get_Current(&container));
                        if (!container)
                        {
                            // this is possible when mutations have occurred to the collection but we
                            // cannot be reached through layout. This is very hard to figure out, so we just harden
                            // the code here to deal with nulls.
                            IFC_RETURN(ProcessCurrentPosition());
                            continue;
                        }
                        ASSERT(ctl::is<ISelectorItem>(container), L"this should be a listviewitem or gridviewitem");

                        virtualizationInformation = container.AsOrNull<IUIElement>().Cast<UIElement>()->GetVirtualizationInformation();
                        IFC_RETURN(virtualizationInformation->GetBuildTreeArgs(&args));

                        argsPhase = args.Cast<ContainerContentChangingEventArgs>()->GetLowestWantedPhase();
                        phase = argsPhase;  // fits easily

                        lookup[currentPositionInVector] = phase;
                    }

                    if (!args)
                    {
                        // we might have skipped getting the args, let's do that now.
                        IFC_RETURN(containersIterator->get_Current(&container));

                        // no need to check for null here, because we already did that in previous block

                        virtualizationInformation = container.AsOrNull<IUIElement>().Cast<UIElement>()->GetVirtualizationInformation();
                        IFC_RETURN(virtualizationInformation->GetBuildTreeArgs(&args));
                    }

                    // guaranteed to have spArgs now
                    argsConcrete = args.Cast<ContainerContentChangingEventArgs>();

                    if (phase == processingPhase)
                    {
                        // processing this guy
                        BOOLEAN wantsAppCallback = FALSE;
                        wf::Size measureSize = {};
                        UINT appCallbackPhase = 0;
                        INT bindingCallbackPhase = 0;

                        ctl::ComPtr<wf::ITypedEventHandler<xaml_controls::ListViewBase*, xaml_controls::ContainerContentChangingEventArgs*>> callback;

                        // guaranteed to have pVirtualizationInformation by now
                        ASSERT(virtualizationInformation);

                        measureSize = virtualizationInformation->GetMeasureSize();

                        // get out of placeholder mode
                        if (container.AsOrNull<ISelectorItem>().Cast<SelectorItem>()->GetIsUIPlaceholder())
                        {
                            BOOLEAN contentShouldBeSet = FALSE;
                            IFC_RETURN(argsConcrete->get_ContentShouldBeSet(&contentShouldBeSet));

                            // get out of placeholder mode
                            container.Cast<SelectorItem>()->SetIsUIPlaceholder(FALSE);

                            if (contentShouldBeSet)
                            {
                                ctl::ComPtr<IInspectable> item;
                                IFC_RETURN(args->get_Item(&item));

                                // the below is now very expensive
                                IFC_RETURN(container.Cast<ContentControl>()->put_Content(item.Get()));
                                IFC_RETURN(argsConcrete->put_ContentShouldBeSet(FALSE));
                            }

                            IFC_RETURN(container.Cast<SelectorItem>()->ChangeVisualState(TRUE));
                            IFC_RETURN(container.Cast<SelectorItem>()->Measure(measureSize));
                        }

                        // do we need to raise this for the binding system or for app callback (or both)
                        IFC_RETURN(argsConcrete->get_Phase(&appCallbackPhase));
                        bindingCallbackPhase = argsConcrete->GetBindingPhase();

                        if (appCallbackPhase == processingPhase || bindingCallbackPhase == processingPhase)
                        {
                            // we are going to call someone back, maybe both

                            if (appCallbackPhase == processingPhase)
                            {
                                // did we store a callback
                                IFC_RETURN(argsConcrete->get_Callback(&callback));
                            }

                            INT itemIndex = 0;
                            IFC_RETURN(IndexFromContainer(container.Get(), &itemIndex));

                            // make sure args are correct. The itemindex might have changed
                            // note that we keep the item and container set since 'setupAfterPrepare'
                            // and we will only clean them when the app doesn't want any more callbacks
                            IFC_RETURN(argsConcrete->put_ItemIndex(itemIndex));

                            // they might not want a callback now, but we need to remember that we
                            // might want one after the binding phase
                            IFC_RETURN(argsConcrete->get_WantsCallBack(&wantsAppCallback));

                            // raise event
                            if (callback.Get())
                            {
                                IFC_RETURN(argsConcrete->put_WantsCallBack(FALSE));
                                // clear out the delegate
                                IFC_RETURN(argsConcrete->put_Callback(nullptr));
                                IFC_RETURN(argsConcrete->put_Handled(FALSE));

                                IFC_RETURN(callback->Invoke(this, args.Get()));

                                // the invoke will cause them to call RegisterCallback which will overwrite the delegate (fine)
                                // and set the boolean below to true
                                IFC_RETURN(argsConcrete->get_WantsCallBack(&wantsAppCallback));
                            }

                            if (bindingCallbackPhase == processingPhase)
                            {
                                ctl::ComPtr<IUIElement> templateRoot;
                                IFC_RETURN(container.Cast<SelectorItem>()->get_ContentTemplateRoot(&templateRoot));
                                if (templateRoot)
                                {
                                    ctl::ComPtr<IDataTemplateComponent> templateComponent;
                                    IFC_RETURN(DirectUI::XamlBindingHelperFactory::GetDataTemplateComponentStatic(templateRoot.AsOrNull<IDependencyObject>().Get(), &templateComponent));
                                    if (templateComponent)
                                    {
                                        // we have a template component, lets use that
                                        ctl::ComPtr<IInspectable> item;
                                        INT32 requestedPhase = -1;

                                        IFC_RETURN(args->get_Item(&item));
                                        IFC_RETURN(args->get_ItemIndex(&itemIndex));

                                        IFC_RETURN(templateComponent->ProcessBindings(item.Get(), itemIndex, static_cast<INT>(processingPhase), &requestedPhase));

                                        // the public api on ProcessBinding for an extension is to not register for the callback like the app will do,
                                        // but return the phase it wants to be called back.
                                        argsConcrete->SetBindingPhase(requestedPhase);
                                    }
                                    else
                                    {
                                        // check if we have an extension for apps compiled with older sdk.
                                        ctl::ComPtr<IDataTemplateExtension> extension;
                                        IFC_RETURN(DirectUI::DataTemplateFactory::GetExtensionInstanceStatic(templateRoot.AsOrNull<IFrameworkElement>().Get(), &extension));
                                        if (extension)
                                        {
                                            HRESULT hr2 = S_OK;
                                            UINT nextAppCallback = 0;
                                            // app usage might have changed the phase
                                            IFC_RETURN(argsConcrete->get_Phase(&nextAppCallback));
                                            IFC_RETURN(argsConcrete->put_Phase(static_cast<UINT>(processingPhase)));

                                            // yep, there is an extension
                                            INT requestedPhase = -1;
                                            hr2 = extension->ProcessBindings(args.Get(), &requestedPhase);

                                            // put back what the app expected
                                            IFC_RETURN(argsConcrete->put_Phase(nextAppCallback));

                                            IFC_RETURN(hr2);

                                            // the public api on ProcessBinding for an extension is to not register for the callback like the app will do,
                                            // but return the phase it wants to be called back.
                                            argsConcrete->SetBindingPhase(requestedPhase);
                                        }
                                    }
                                }
                            }
                        }



                        TraceVirtualizedItemUpdatedInfo(
                            (UINT64)container.Cast<DependencyObject>()->GetHandle(),
                            static_cast<UINT>(phase)
                        );

                        // always make sure we go out of placeholder mode independent of setting content
                        if (container.Cast<SelectorItem>()->GetIsUIPlaceholder())
                        {
                            container.Cast<SelectorItem>()->SetIsUIPlaceholder(FALSE);
                            IFC_RETURN(container.Cast<SelectorItem>()->UpdateVisualState(TRUE /* bUseTransitions */));
                        }

                        // the user might have changed elements. In order to keep the budget fair, we need to try and incur
                        // most of the cost right now.
                        IFC_RETURN(container.Cast<SelectorItem>()->Measure(measureSize));

                        // register callback
                        if (wantsAppCallback || argsConcrete->GetWantsBindingPhaseCallback())
                        {
                            UINT phaseFromArgs = argsConcrete->GetLowestWantedPhase();
                            phase = phaseFromArgs;
                            lookup[currentPositionInVector] = phase;

                            // if the appcode requested a phase that is lower than the current processing phase, it is kind of weird
                            if (phase < processingPhase)
                            {
                                // after we change the processingphase, our next lowest is going to be the current phase (we didn't finish it yet)
                                nextLowest = processingPhase;

                                // change our processing phase to the requested phase. It is going to be the one we work on next
                                processingPhase = phase;
                                m_lowestPhaseInQueue = processingPhase;

                                // the pointer is pointing to the current container which is great
                                shouldUpdateCurrentPosition = false;
                            }
                            else
                            {
                                // update the next lowest to the best of our current understanding
                                nextLowest = std::min(nextLowest, static_cast<INT64>(phase));
                            }
                        }
                        else
                        {
                            // won't be called again for the lifetime of this container
                            IFC_RETURN(argsConcrete->ResetLifetime());

                            // we do not have to update the next lowest. We are still processing this phase and will
                            // continue to do so (procesingPhase is still valid).
                        }
                    }
                    else
                    {
                        // if we hit a container that is registered for a callback (so he wants to iterate over phases)
                        // but is currently at a different phase, we need to make sure that the next lowest is set.
                        BOOLEAN wantsAppCallback = FALSE;
                        IFC_RETURN(argsConcrete->get_WantsCallBack(&wantsAppCallback));

                        if (wantsAppCallback || argsConcrete->GetWantsBindingPhaseCallback())
                        {
                            ASSERT(phase > processingPhase);
                            // update the next lowest, now that we have seen a phase that is higher than our current processing phase
                            nextLowest = std::min(nextLowest, static_cast<INT64>(phase));
                        }
                    }

                    // updates the current position in the correct direction
                    if (shouldUpdateCurrentPosition)
                    {
                        IFC_RETURN(ProcessCurrentPosition());
                    }
                    // updates the time
                    IFC_RETURN(budget->GetElapsedMilliSecondsSinceLastUITick(&timeElapsedInMS));
                }

                if (processingPhase == std::numeric_limits<INT64>::max())
                {
                    // nothing left to process
                    m_lowestPhaseInQueue = -1;
                }
                else
                {
                    // we broke out of the loop for some other reason (policy)
                    // should be safe at this point
                    ASSERT(processingPhase < std::numeric_limits<INT>::max());
                    m_lowestPhaseInQueue = static_cast<INT>(processingPhase);
                }
            }

            // cache count of contianers that were processed
            m_lastIncrementalVisualizationContainerCount = containersCount;
        }

        // 2. Clear containers
        IFC_RETURN(m_toBeClearedContainers->get_Size(&containersToClearCount));
        for (UINT toClearIndex = containersToClearCount - 1; containersToClearCount > 0; --toClearIndex)
        {
            ctl::ComPtr<IUIElement> container;
            ctl::ComPtr<IInspectable> item;

            if (!budget)
            {
                IFC_RETURN(DXamlCore::GetCurrent()->GetBudgetManager(budget));
            }

            IFC_RETURN(budget->GetElapsedMilliSecondsSinceLastUITick(&timeElapsedInMS));

            if (static_cast<UINT>(timeElapsedInMS) > m_budget)
            {
                break;
            }


            // we stored a pair of container and item in two separate collections (because of tracking)
            IFC_RETURN(m_toBeClearedContainers->GetAt(toClearIndex, &container));
            IFC_RETURN(m_toBeClearedItems->GetAt(toClearIndex, &item));
            IFC_RETURN(m_toBeClearedContainers->RemoveAtEnd());
            IFC_RETURN(m_toBeClearedItems->RemoveAtEnd());


            // execute the deferred work
            // apparently we were not going to reuse this container immediately again, so
            // let's do the work now
            IFC_RETURN(ListViewBaseGenerated::ClearContainerForItemOverride(container.AsOrNull<IDependencyObject>().Get(), item.Get()));


            if (toClearIndex == 0)
            {
                // UINT
                break;
            }
        }

        IFC_RETURN(m_toBeClearedContainers->get_Size(&containersToClearCount));

        // we have work left if we still have containers that need to finish their phases and we are still in the
        // live tree (or)
        // when we have containers that need to be cleared. Note that we will still clear containers even after
        // being taken out of the live tree.
        // If we are taken out of the live tree while still having phasing work, we need to make sure that the
        // next time we entre the tree, the phasing work continues. OnLoaded call does a check and register's work
        // for this case.
        *workLeft = (m_lowestPhaseInQueue != -1 && isInLiveTree) || containersToClearCount > 0;
    }

    return S_OK;
}

_Check_return_ HRESULT ListViewBase::ShutDownDeferredWorkImpl()
{
    HRESULT hr = S_OK;

    ctl::ComPtr<IPanel> spItemsHost;
    ctl::ComPtr<ICustomGeneratorItemsHost> spCustomGeneratorPanel;
    UINT containersToClearCount = 0;

    IFC(get_ItemsHost(&spItemsHost));
    if (spItemsHost)
    {
        spCustomGeneratorPanel = spItemsHost.AsOrNull<ICustomGeneratorItemsHost>();
    }

    if (spCustomGeneratorPanel)
    {
        // go through everyone that might have work registered for a prepare
        ctl::ComPtr<IContainerContentChangingIterator> spContainersIterator;
        IFC(spCustomGeneratorPanel->GetContainersForIncrementalVisualization(&spContainersIterator));

        if (spContainersIterator)
        {
            BOOLEAN thereIsMore;
            IFC(spContainersIterator->MoveNext(&thereIsMore));

            while (thereIsMore)
            {
                ctl::ComPtr<xaml::IDependencyObject> spContainer;
                IFC(spContainersIterator->get_Current(&spContainer));

                if (spContainer)
                {
                    ctl::ComPtr<xaml_controls::IContainerContentChangingEventArgs> spArgs;
                    UIElement::VirtualizationInformation* pVirtualizationInformation =
                        spContainer.AsOrNull<IUIElement>().Cast<UIElement>()->GetVirtualizationInformation();

                    IFC(pVirtualizationInformation->GetBuildTreeArgs(&spArgs));
                    IFC(spArgs.Cast<ContainerContentChangingEventArgs>()->ResetLifetime());

                }
                else
                {
                    // apparently a sentinel. This should not occur, however, during shutdown we could
                    // run into this since measure might not have been processed yet
                }


                IFC(spContainersIterator->MoveNext(&thereIsMore));
            }
        }
    }

    IFC(m_toBeClearedContainers->get_Size(&containersToClearCount));

    // go to all the containers waiting to clear
    for (UINT currentIndex = 0; currentIndex < containersToClearCount; ++currentIndex)
    {
        UIElement::VirtualizationInformation* pVirtualizationInformation = nullptr;
        ctl::ComPtr<IUIElement> spContainer;
        ctl::ComPtr<xaml_controls::IContainerContentChangingEventArgs> spArgs;

        IFC(m_toBeClearedContainers->GetAt(currentIndex, &spContainer));

        pVirtualizationInformation = spContainer.Cast<UIElement>()->GetVirtualizationInformation();

        IFC(pVirtualizationInformation->GetBuildTreeArgs(&spArgs));

        IFC(spArgs.Cast<ContainerContentChangingEventArgs>()->ResetLifetime());
    }

    IFC(m_toBeClearedContainers->Clear());
    IFC(m_toBeClearedItems->Clear());

Cleanup:
    RRETURN(hr);
}

_Check_return_ IFACEMETHODIMP
ListViewBase::ShouldRaiseChoosingItemContainer(
    _Out_ BOOLEAN *pShouldRaiseChoosingItemContainer)
{
    HRESULT hr = S_OK;
    ChoosingItemContainerEventSourceType* pEventSource = nullptr;

    *pShouldRaiseChoosingItemContainer = false;

    IFC(GetChoosingItemContainerEventSourceNoRef(&pEventSource));

    *pShouldRaiseChoosingItemContainer = pEventSource->HasHandlers() && ShouldRaiseEvent(KnownEventIndex::ListViewBase_ChoosingItemContainer);

Cleanup:
    RRETURN(hr);
}

_Check_return_ IFACEMETHODIMP
ListViewBase::ShouldRaiseChoosingGroupHeaderContainer(
_Out_ BOOLEAN *pShouldRaiseChoosingGroupHeaderContainer)
{
    HRESULT hr = S_OK;
    ChoosingGroupHeaderContainerEventSourceType* pEventSource = nullptr;

    *pShouldRaiseChoosingGroupHeaderContainer = false;

    IFC(GetChoosingGroupHeaderContainerEventSourceNoRef(&pEventSource));

    *pShouldRaiseChoosingGroupHeaderContainer = pEventSource->HasHandlers() && ShouldRaiseEvent(KnownEventIndex::ListViewBase_ChoosingGroupHeaderContainer);

Cleanup:
    RRETURN(hr);
}

_Check_return_ IFACEMETHODIMP
ListViewBase::RaiseChoosingItemContainer(
    _In_ xaml_controls::IChoosingItemContainerEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    ASSERT(ShouldRaiseEvent(KnownEventIndex::ListViewBase_ChoosingItemContainer));

    ChoosingItemContainerEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<ListViewBase> spThis = this;
    IFC(GetChoosingItemContainerEventSourceNoRef(&pEventSource));

    IFC(pEventSource->Raise(spThis.AsOrNull<xaml_controls::IListViewBase>().Get(), pArgs));

Cleanup:
    RRETURN(hr);
}

_Check_return_ IFACEMETHODIMP
ListViewBase::RaiseChoosingGroupHeaderContainer(
_In_ xaml_controls::IChoosingGroupHeaderContainerEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    ASSERT(ShouldRaiseEvent(KnownEventIndex::ListViewBase_ChoosingGroupHeaderContainer));

    ChoosingGroupHeaderContainerEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<ListViewBase> spThis = this;
    IFC(GetChoosingGroupHeaderContainerEventSourceNoRef(&pEventSource));

    IFC(pEventSource->Raise(spThis.AsOrNull<xaml_controls::IListViewBase>().Get(), pArgs));

Cleanup:
    RRETURN(hr);
}

_Check_return_ IFACEMETHODIMP
ListViewBase::RaiseContainerContentChangingOnRecycle(
    _In_ xaml::IUIElement* container,
    _In_ IInspectable* item)
{
    ctl::ComPtr<xaml_controls::IContainerContentChangingEventArgs> args;
    ctl::ComPtr<ContainerContentChangingEventArgs> argsConcrete;
    UIElement::VirtualizationInformation* virtualizationInformation = ctl::ComPtr<xaml::IUIElement>(container).Cast<UIElement>()->GetVirtualizationInformation();

    if (virtualizationInformation)
    {
        IFC_RETURN(virtualizationInformation->GetBuildTreeArgs(&args));
        argsConcrete = args.Cast<ContainerContentChangingEventArgs>();

        ctl::ComPtr<IUIElement> templateRoot;
        BOOLEAN wantAppCallback = FALSE;

        ContainerContentChangingEventSourceType* pEventSource = nullptr;
        IFC_RETURN(GetContainerContentChangingEventSourceNoRef(&pEventSource));

        IFC_RETURN(argsConcrete->put_InRecycleQueue(TRUE));
        IFC_RETURN(argsConcrete->put_Phase(0));
        IFC_RETURN(argsConcrete->put_WantsCallBack(FALSE));
        IFC_RETURN(argsConcrete->put_Callback(nullptr));
        IFC_RETURN(argsConcrete->put_Item(item));
        argsConcrete->SetBindingPhase(ContainerContentChangingEventArgs::NoBindingNeededToken);


        if (ShouldRaiseEvent(KnownEventIndex::ListViewBase_ContainerContentChanging))
        {
            IFC_RETURN(pEventSource->Raise(this, args.Get()));
            IFC_RETURN(args.Cast<ContainerContentChangingEventArgs>()->get_WantsCallBack(&wantAppCallback));
        }

        IFC_RETURN(ctl::ComPtr<xaml::IUIElement>(container).Cast<ContentControl>()->get_ContentTemplateRoot(&templateRoot));
        if (templateRoot)
        {
            ctl::ComPtr<IDataTemplateComponent> templateComponent;
            IFC_RETURN(DirectUI::XamlBindingHelperFactory::GetDataTemplateComponentStatic(templateRoot.AsOrNull<IDependencyObject>().Get(), &templateComponent));
            if (templateComponent)
            {
                IFC_RETURN(templateComponent->Recycle());
            }
            else
            {
                ctl::ComPtr<IDataTemplateExtension> extension;
                IFC_RETURN(DirectUI::DataTemplateFactory::GetExtensionInstanceStatic(templateRoot.AsOrNull<IFrameworkElement>().Get(), &extension));
                // raise for binding
                if (extension)
                {
                    IFC_RETURN(extension->ResetTemplate());
                }
            }
        }

        if (wantAppCallback)
        {
            if (m_lowestPhaseInQueue == -1)
            {
                UINT phaseArgs = 0;
                IFC_RETURN(args.Cast<ContainerContentChangingEventArgs>()->get_Phase(&phaseArgs));

                // there was nothing registered
                m_lowestPhaseInQueue = phaseArgs;

                // that means we need to register ourselves with the buildtreeservice so that
                // we can get called back to do some work
                if (!m_isRegisteredForCallbacks)
                {
                    ctl::ComPtr<BuildTreeService> buildTree;
                    IFC_RETURN(DXamlCore::GetCurrent()->GetBuildTreeService(buildTree));
                    IFC_RETURN(buildTree->RegisterWork(this));
                }
            }
        }
        else
        {
            IFC_RETURN(argsConcrete->ResetLifetime());
        }
    }

    return S_OK;
}
