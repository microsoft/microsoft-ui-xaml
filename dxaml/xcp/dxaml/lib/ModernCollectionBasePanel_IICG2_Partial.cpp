// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ModernCollectionBasePanel.g.h"
#include "IContainerRecyclingContext.g.h"
#include "ScrollViewer.g.h"
#include "VisualTreeHelper.h"
#include "DataTemplate.g.h"

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

using namespace DirectUI;
using namespace DirectUISynonyms;

using xaml_controls::LayoutReference;

//#define MCBP_DEBUG

_Check_return_ HRESULT ModernCollectionBasePanel::ReloadUnloadedElements()
{
    HRESULT hr = S_OK;
    UINT nSize = 0;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

    IFC(m_tpUnloadedElements->get_Size(&nSize));

    if (nSize == 0)
    {
        goto Cleanup;
    }

    IFC(m_cacheManager.GetChildren(&spChildren));

    // we have 2 pass intentionally.
    // First pass will check existing children and if any exist in unloaded elements we will just remove it from unloaded
    for (INT i = nSize - 1; i >= 0; --i)
    {
        ctl::ComPtr<xaml::IUIElement> spContainer;
        UINT unused = 0;
        BOOLEAN found = FALSE;

        IFC(m_tpUnloadedElements->GetAt(i, &spContainer));
        IFC(spChildren->IndexOf(spContainer.Get(), &unused, &found));
        if (found)
        {
            IFC(m_tpUnloadedElements->RemoveAt(i));
        }
    }

    IFC(m_tpUnloadedElements->get_Size(&nSize));

    // Second pass we move all elements which are not in visual tree to the visual tree
    // It allows us to not increase size of children collection for IndexOf operation.
    for (UINT i = 0; i < nSize; ++i)
    {
        ctl::ComPtr<xaml::IUIElement> spContainer;

        IFC(m_tpUnloadedElements->GetAt(0, &spContainer));
        IFCFAILFAST(spChildren->Append(spContainer.Get()));
        IFC(m_tpUnloadedElements->RemoveAt(0));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::GetRootOfItemTemplateAsContainer(const ctl::ComPtr<IGeneratorHost>& host, const ctl::ComPtr<IInspectable>& item, bool queryForInspecting, ctl::ComPtr<xaml::IUIElement>& container)
{
    container = nullptr;

    // Scope this feature only to ListViewBase.
    // This bug tracks making it work correctly in the ItemsControl general case:
    // Bug 17815731: Using DataTemplate as Container feature does not work for ItemsControl (breaks binding)
    ctl::ComPtr<xaml_controls::IListViewBase> listViewBase = host.AsOrNull<xaml_controls::IListViewBase>();
    if(listViewBase)
    {
        // Check if the ItemTemplate includes a container. If so use that. This allows us to provide
        // a container as part of the ItemTemplate.
        ctl::ComPtr<xaml_controls::IItemsControl> itemsControl = host.AsOrNull<xaml_controls::IItemsControl>();
        if (itemsControl)
        {
            // Get the template from either ItemsControl.ItemTemplate or ItemsControl.ItemTemplateSelector
            ctl::ComPtr<xaml::IDataTemplate> selectedTemplate;
            IFC_RETURN(itemsControl->get_ItemTemplate(&selectedTemplate));
            if (!selectedTemplate)
            {
                // Check TemplateSelector
                ctl::ComPtr<xaml_controls::IDataTemplateSelector> itemTemplateSelector;
                IFC_RETURN(itemsControl->get_ItemTemplateSelector(&itemTemplateSelector));
                if (itemTemplateSelector)
                {
                    IFC_RETURN(itemTemplateSelector->SelectTemplateForItem(item.Get(), &selectedTemplate));
                }
            }

            if (selectedTemplate)
            {
                auto templateCore = static_cast<CDataTemplate*>(static_cast<DirectUI::DataTemplate*>(selectedTemplate.Get())->GetHandle());
                if (templateCore)
                {
                    xref_ptr<CDependencyObject> expandedTemplateCore;
                    CDependencyObject* expandedTemplateCoreNoRef = nullptr;
                    if (queryForInspecting)
                    {
                        // Query for an instance of the template. We may or may not use it, so the QueryContentNoRef call caches the instance in CDataTemplate to be used later.
                        IFC_RETURN(templateCore->QueryContentNoRef(&expandedTemplateCoreNoRef));
                    }
                    else
                    {
                        // We are going to use the instance, so call LoadContent so that the
                        // cached instance in CDataTemplate will be cleared.
                        // LoadContent returns a CDependencyObject that we must release, so we store it in a smart pointer.
                        // The QueryContentNoRef call above returns a raw pointer to a cached object that the template owns, so we do not want to release it.
                        IFC_RETURN(templateCore->LoadContent(expandedTemplateCore.ReleaseAndGetAddressOf()));
                        expandedTemplateCoreNoRef = expandedTemplateCore.get();
                    }

                    if (expandedTemplateCoreNoRef)
                    {
                        ctl::ComPtr<xaml::IDependencyObject> expandedTemplate;
                        IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(expandedTemplateCoreNoRef, expandedTemplate.GetAddressOf()));
                        BOOLEAN isOwnContainer = FALSE;
                        IFC_RETURN(host->IsItemItsOwnContainer(expandedTemplate.Cast<IInspectable>(), &isOwnContainer));
                        if (isOwnContainer)
                        {
                            IFC_RETURN(expandedTemplate.As(&container));
                        }
                        else
                        {
                            // not its own container.
                            container = nullptr;
                        }
                    }
                }
            }
        }
    }

    return S_OK;
}

// Creates or recycles a container for the given index.
_Check_return_ HRESULT ModernCollectionBasePanel::GenerateContainerAtIndexImpl(_In_ INT indexInItemCollection, _Outptr_ xaml::IUIElement** ppReturnValue ) noexcept
{
    HRESULT hr = S_OK;
    TraceGenerateMCContainerBegin(indexInItemCollection);

    ctl::ComPtr<IGeneratorHost> spIHost;
    ctl::ComPtr<IInspectable> spItem;
    ctl::ComPtr<xaml::IUIElement> spContainer;
    ctl::ComPtr<xaml::IUIElement> spPinnedContainer;
    ctl::ComPtr<xaml::IUIElement> spContainerFromItemTemplate;
    bool foundPinnedContainer = FALSE;
    bool shouldSkipContainerPreparation = false;
    BOOLEAN isOwnContainer = FALSE;

    *ppReturnValue = nullptr;

    // Increment container requested count for telemetry purposes - only done if cache potential reached so we are counting use during panning and not initial setup
    if (m_windowState.cachePotentialReached)
    {
        ++m_containerRequestedCount;
    }

    IFC(m_cacheManager.GetItemsHost(&spIHost));

    // get content for the container
    ASSERT(0 <= indexInItemCollection && indexInItemCollection < m_cacheManager.GetTotalItemCount());
    IFC(m_cacheManager.GetItem(indexInItemCollection, &spItem));

    // We need to know if this item/container is pinned.
    IFC(m_containerManager.GetContainerFromPinnedContainers(indexInItemCollection, &spPinnedContainer));
    foundPinnedContainer = spPinnedContainer != nullptr;

    // Try several approaches to getting a container for this item.
    // Different approaches yield varying degrees of preparedness
    // for a container.

    // Query to see if we need to skip container preparation
    IFC(GetRootOfItemTemplateAsContainer(spIHost, spItem, true /* queryOnly */, spContainerFromItemTemplate));

    if (foundPinnedContainer) // Do we have the container in the pinned collection?
    {
        // The container was pinned. We're done, just return it.
        // Pinned containers are already linked and prepared, unless we're in the process
        // of preparing them.
        spContainer = spPinnedContainer;
    }
    else
    {
        IFC(spIHost->IsItemItsOwnContainer(spItem.Get(), &isOwnContainer));
        if (isOwnContainer) // Is the item a container in its own right?
        {
            IFC(spItem.As(&spContainer));
            IFC(spContainer.Cast<UIElement>()->InitVirtualizationInformation());
            SetElementIsGenerated(spContainer, FALSE);
        }
        else
        {
            // Container wasn't pinned. Get a container some other way.
            // We need to link and prepare it as well, since it was previously
            // unlinked and cleared (or was never prepared and linked at all).
            ctl::ComPtr<IContainerRecyclingContext> spRecyclingContext;
            ctl::ComPtr<xaml::IUIElement> spSuggestedContainer;
            ctl::ComPtr<xaml_primitives::ISelectorItem> spContainerAsSelectorItem;
            BOOLEAN shouldRaiseChoosingItemContainer = false;
            BOOLEAN choosingItemContainerIsContainerPrepared = FALSE;

            IFC(m_cacheManager.GetContainerRecyclingContext(indexInItemCollection, &spRecyclingContext));

            if (spRecyclingContext)
            {
                IFC(spRecyclingContext->get_SelectedContainer(&spSuggestedContainer));
                if (spSuggestedContainer)
                {
                    ASSERT(GetElementIsGenerated(spSuggestedContainer));
                    ASSERT(!GetElementIsHeader(spSuggestedContainer));

                    IFC(RemoveFromQueue(spSuggestedContainer.Get()));
                    IFC(spRecyclingContext->ConfigureSelectedContainer(spSuggestedContainer.Get()));
                }
                else
                {
                    // Check the queue Length
                    if (m_recycleQueue.size() >= QueueLengthBeforeFallback)
                    {
                        // Lets recycle a container even if it does not have the right template
                        spSuggestedContainer = std::move(m_recycleQueue.back()).AsOrNull<IUIElement>();
                        m_recycleQueue.pop_back();
                        ASSERT(GetElementIsGenerated(spSuggestedContainer));
                        ASSERT(!GetElementIsHeader(spSuggestedContainer));
                        IFC(spRecyclingContext->ConfigureSelectedContainer(spSuggestedContainer.Get()));
                        IFC(spRecyclingContext->put_SelectedContainer(nullptr));
                    }
                }
            }
            else if (!m_recycleQueue.empty()) // Do we have containers ready in the recycle queue?
            {
                spSuggestedContainer = std::move(m_recycleQueue.back()).AsOrNull<IUIElement>();
                m_recycleQueue.pop_back();
                ASSERT(GetElementIsGenerated(spSuggestedContainer));
                ASSERT(!GetElementIsHeader(spSuggestedContainer));
            }

            if (spSuggestedContainer)
            {
                spContainerAsSelectorItem = spSuggestedContainer.AsOrNull<xaml_primitives::ISelectorItem>();
            }

            IFC(spIHost->ShouldRaiseChoosingItemContainer(&shouldRaiseChoosingItemContainer));

            if (shouldRaiseChoosingItemContainer)
            {
                if (!m_trChoosingItemContainerEventArgs)
                {
                    ctl::ComPtr<ChoosingItemContainerEventArgs> spChoosingItemContainerEventArgs;
                    ctl::make(&spChoosingItemContainerEventArgs);

                    SetPtrValue(m_trChoosingItemContainerEventArgs, spChoosingItemContainerEventArgs.Get());
                }

                IFC(m_trChoosingItemContainerEventArgs->put_ItemIndex(indexInItemCollection));
                IFC(m_trChoosingItemContainerEventArgs->put_Item(spItem.Get()));
                IFC(m_trChoosingItemContainerEventArgs->put_ItemContainer(spContainerAsSelectorItem.Get()));
                IFC(m_trChoosingItemContainerEventArgs->put_IsContainerPrepared(false));

                // First prompt the application to provide a container for this item via ChoosingItemContainer.
                IFC(spIHost->RaiseChoosingItemContainer(m_trChoosingItemContainerEventArgs.Get()));

                IFC(m_trChoosingItemContainerEventArgs->get_ItemContainer(&spContainerAsSelectorItem));
                IFC(m_trChoosingItemContainerEventArgs->get_IsContainerPrepared(&choosingItemContainerIsContainerPrepared));

                // NOTE: We still need to prepare this container via the items host (see PrepareContainerViaItemsHost),
                // unless ChoosingItemContainer was marked as handled, in which case we can just reuse it as-is.
                shouldSkipContainerPreparation = !!choosingItemContainerIsContainerPrepared;
            }

            if (spContainerAsSelectorItem)
            {
                IFC(spContainerAsSelectorItem.As<xaml::IUIElement>(&spContainer));
            }

            // If we suggested a container and if we're not going to use it,
            // then we should put that container back into the recycle queue.
            if (spContainerAsSelectorItem && spContainerAsSelectorItem != spSuggestedContainer)
            {
                IFC(spContainer.Cast<UIElement>()->InitVirtualizationInformation());

                // put back the original one
                if (spSuggestedContainer)
                {
                    ctl::WeakRefPtr wrContainer;
                    // and make a note that the app did not like this container
                    // this will make sure that we keep trying to recycle if the recycle queue only has elements that are
                    // rejected
                    auto p_virtualizationinfo = GetVirtualizationInformationFromElement(spSuggestedContainer);
                    p_virtualizationinfo->SetWasRejectedAsAContainerByApp(true);

                    IFC(spSuggestedContainer.AsWeak(&wrContainer));
                    m_recycleQueue.push_back(std::move(wrContainer));
                }

                // ofcourse, the container that we got could have been from the recycle queue itself
                // in fact, we really hope it is!
                IFC(RemoveFromQueue(spContainer.Get(), false /*expectContainerToBePresent*/));

                SetElementIsGenerated(spContainer, TRUE);

                // Set a local value of null to block data context propagation.
                IFC(spContainer.AsOrNull<xaml::IFrameworkElement>()->put_DataContext(nullptr));

                if (spRecyclingContext)
                {
                    IFC(spRecyclingContext->ConfigureSelectedContainer(spContainer.Get()));
                    IFC(spRecyclingContext->put_SelectedContainer(nullptr));
                }
            }
            // Otherwise, if ChoosingItemContainer didn't give us a container, then we'll need to get one a different way,
            // either by directly using the suggested container or by creating a new container if no container was suggested.
            else if (!spContainerAsSelectorItem)
            {
                if (spSuggestedContainer)
                {
                    spContainer = spSuggestedContainer;
                }
                else
                {
                    // Increment new container count for telemetry purposes - only done if cache potential reached so we are counting use during panning and not initial setup
                    if (m_windowState.cachePotentialReached)
                    {
                        ++m_containerCreatedCount;
                    }

                    // If root of ItemTemplate is a container. use that.
                    if (spContainerFromItemTemplate)
                    {
                        // false to queryOnly argument will call LoadContent to clear the cached entry in CDataTemplate since we are
                        // going to consume that instance now.
                        IFC(GetRootOfItemTemplateAsContainer(spIHost, spItem, false /* queryOnly */, spContainer));
                    }
                    else
                    {
                        // have to actually create the container from scratch. This is an expensive operation
                        ctl::ComPtr<xaml::IDependencyObject> spContainerAsDO;
                        IFC(spIHost->GetContainerForItem(spItem.Get(), nullptr, &spContainerAsDO));
                        IFC(spContainerAsDO.As<xaml::IUIElement>(&spContainer));
                    }

                    IFC(spContainer.Cast<UIElement>()->InitVirtualizationInformation());
                    SetElementIsGenerated(spContainer, TRUE);

                    // set a local value of null to block datacontext propagation
                    IFC(spContainer.AsOrNull<xaml::IFrameworkElement>()->put_DataContext(nullptr));

                    if (spRecyclingContext)
                    {
                        IFC(spRecyclingContext->ConfigureSelectedContainer(spContainer.Get()));
                        IFC(spRecyclingContext->put_SelectedContainer(nullptr));
                    }

                }
            }
        }

        // to avoid re-parenting we should mark the recycled container that it did't leave the tree in current layout cycle.
        IFC(CoreImports::UIElement_SetIsLeaving(static_cast<CUIElement*>(spContainer.Cast<UIElement>()->GetHandle()), FALSE));

        // make sure it has a valid virtualizationinformation section.
        IFC(spContainer.Cast<UIElement>()->InitVirtualizationInformation());

        SetElementIsHeader(spContainer, FALSE);

        IFC(LinkContainerToItem(spContainer.Get(), spItem.Get()));
    }

    SetElementEmptySizeInGarbageSection(spContainer);

    if (spContainerFromItemTemplate && !isOwnContainer)
    {
        // If root of ItemTemplate is a container, and the item itself is not a container,
        // Skip container preparation. Otherwise we will end up setting the ContentTemplate
        // on the container which we do not want here. This is tracked by a flag in VirtualizationInformation.
        SetIsContainerFromTemplateRoot(spContainer, true /* isFromTemplateRoot */);
    }

    SetElementWantsToSkipContainerPreparation(spContainer, shouldSkipContainerPreparation);

    // Item Containers have special rules regarding implicit show/hide animations.
    // Set a flag so that this case can be properly detected.
    static_cast<CUIElement*>(spContainer.Cast<UIElement>()->GetHandle())->SetIsItemContainer(true);

    IFC(spContainer.MoveTo(ppReturnValue));

Cleanup:
    TraceGenerateMCContainerEnd();
    RRETURN(hr);
}

// Delegate the preparation of an container to the items host.
// Note that caller has to guarantee that the container is already in the visual tree since this is going to do a measure
_Check_return_ HRESULT ModernCollectionBasePanel::PrepareContainerViaItemsHost(
    _In_ const INT indexInItemCollection,
    _In_ wf::Size measureSize,
    _In_ const ctl::ComPtr<xaml::IUIElement>& spContainer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IGeneratorHost> spIHost;

    IFC(m_cacheManager.GetItemsHost(&spIHost));

    if (!GetElementIsPrepared(spContainer))
    {
        ctl::ComPtr<IInspectable> spItem = GetItemFromElement(spContainer);

        // Temporarily hold it so that lookups will work during during preparation.
        m_containerManager.HoldForPrepare(spContainer, xaml_controls::ElementType_ItemContainer, indexInItemCollection);

        IFC(spIHost->PrepareItemContainer(spContainer.Cast<UIElement>(), spItem.Get()));

        // after a prepare we are guaranteeing that we are going to raise the ContainerContentChanging event
        // when setting content, we need to guarantee the contenttemplateroot has been applied, and thus we need to trigger
        // a measure. For that to not incur overhead because of later measures, we need to be accurate in the size that
        // we measure with (if you are not invalid and the size you get measured with hasn't changed, it should no-op)
        IFC(spIHost->SetupContainerContentChangingAfterPrepare(spContainer.Cast<UIElement>(), spItem.Get(), indexInItemCollection, measureSize));

        // make the container a candidate for gamedpad/2D focus.
        IFC(spContainer.Cast<UIElement>()->put_IsGamepadFocusCandidate(TRUE));

        m_containerManager.ResetAfterPrepare();
    }
    else
    {
        // We don't want to prepare and re-setup CCC/CIC events for pinned container, but we should at least ensure that our items host
        // will be registered for work on the next BuildTree pass to pick up what might be a changed visible/cache range

        // MCBP doesn't need to know about CCC/CIC events, also the non treebuild controls don't need to create the CCC/CIC event args
        // let the Host decide if it needs the CCC/CIC args.
        IFC(spIHost->RegisterWorkForContainer(spContainer.Get()));
    }

Cleanup:
    RRETURN(hr);

}

// Delegate the preparation of an container to the items host.
// Note that caller has to guarantee that the container is already in the visual tree since this is going to set the style
_Check_return_ HRESULT ModernCollectionBasePanel::PrepareHeaderViaItemsHost(
    _In_ const INT indexInGroupCollection,
    _In_ wf::Size, // Not using this parameter for now.
    _In_ const ctl::ComPtr<xaml::IUIElement>& spHeader)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IGeneratorHost> spIHost;
    ctl::ComPtr<ICollectionViewGroup> spCurrentGroup;

    IFC(m_cacheManager.GetItemsHost(&spIHost));

    ASSERT(0 <= indexInGroupCollection && indexInGroupCollection < m_cacheManager.GetTotalGroupCount());
    IFC(m_cacheManager.GetGroup(indexInGroupCollection, &spCurrentGroup));

    // Temporarily hold it so that lookups will work during during preparation.
    m_containerManager.HoldForPrepare(spHeader, xaml_controls::ElementType_GroupHeader, indexInGroupCollection);

    IFC(spIHost->PrepareGroupContainer(spHeader.Cast<UIElement>(), spCurrentGroup.Get()));

    m_containerManager.ResetAfterPrepare();

Cleanup:
    RRETURN(hr);
}
// Creates or recycles a header for the given index.
_Check_return_ HRESULT ModernCollectionBasePanel::GenerateHeaderAtGroupIndexImpl( _In_ INT index, _Outptr_ xaml::IUIElement** ppReturnValue )
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IGeneratorHost> spIHost;
    ctl::ComPtr<xaml::IUIElement> spHeader;

    ctl::ComPtr<xaml::IDependencyObject> spHeaderAsDO;
    ctl::ComPtr<ICollectionViewGroup> spCurrentGroup;
    bool foundPinnedHeader = FALSE;
    BOOLEAN shouldRaiseChoosingGroupHeaderContainer = FALSE;

    // this implementation works with our own panels

    *ppReturnValue = nullptr;

    IFC(m_cacheManager.GetItemsHost(&spIHost));

    ASSERT(0 <= index && index < m_cacheManager.GetTotalGroupCount());
    IFC(m_cacheManager.GetGroup(index, &spCurrentGroup));

    // do we have it in the pinned collection?
    IFC(m_containerManager.GetHeaderFromPinnedHeaders(index, &spHeader));
    foundPinnedHeader = spHeader != nullptr;

    // Use the pinned header if one was found, like GenerateContainerAtIndexImpl uses pinned item containers when found.
    if (!spHeader)
    {
        if (!m_recycleHeaderQueue.empty())
        {
            // great, I can get it from the queue
            spHeader = std::move(m_recycleHeaderQueue.back()).AsOrNull<IUIElement>();
            ASSERT(GetElementIsGenerated(spHeader));
            m_recycleHeaderQueue.pop_back();
        }

        IFC(spIHost->ShouldRaiseChoosingGroupHeaderContainer(&shouldRaiseChoosingGroupHeaderContainer));

        if (shouldRaiseChoosingGroupHeaderContainer)
        {
            ctl::ComPtr<xaml_controls::IListViewBaseHeaderItem> spHeaderObtained;
            ctl::ComPtr<IUIElement> spHeaderObtainedAsUIElement;
            ctl::ComPtr<IInspectable> spGroupData;

            // Provide a chance for the app to give us a container
            // For improved performance, we just create on event args and resuse it every time we raise the event
            if (!m_trChoosingGroupHeaderContainerEventArgs)
            {
                ctl::ComPtr<ChoosingGroupHeaderContainerEventArgs> spChoosingGroupHeaderContainerEventArgs;
                ctl::make(&spChoosingGroupHeaderContainerEventArgs);

                SetPtrValue(m_trChoosingGroupHeaderContainerEventArgs, spChoosingGroupHeaderContainerEventArgs.Get());
            }

            IFC(spCurrentGroup->get_Group(&spGroupData));
            IFC(m_trChoosingGroupHeaderContainerEventArgs->put_Group(spGroupData.Get()));
            IFC(m_trChoosingGroupHeaderContainerEventArgs->put_GroupIndex(index));
            IFC(m_trChoosingGroupHeaderContainerEventArgs->put_GroupHeaderContainer(spHeader.AsOrNull<xaml_controls::IListViewBaseHeaderItem>().Get()));

            // prompt the application to provide a header container through ChoosingGroupHeaderContainer
            IFC(spIHost->RaiseChoosingGroupHeaderContainer(m_trChoosingGroupHeaderContainerEventArgs.Get()));
            IFC(m_trChoosingGroupHeaderContainerEventArgs->get_GroupHeaderContainer(&spHeaderObtained));

            if (spHeader != nullptr && spHeaderObtained == nullptr)
            {
                // We suggested a container but got back null from ChoosingGroupHeaderContainer
                IFC(E_FAIL);
            }

            // If ChoosingGroupHeaderContainer gave us a container (that is not what we suggested), use that
            // If it gave back null, create a new one
            // If it gave back what we suggested, then just do what we would normally do (like the event didn't exist)
            if (spHeaderObtained != nullptr && spHeader != spHeaderObtained)
            {
                // ChoosingGroupHeaderContainer gave us a container that was not the one we gave it.
                // If we suggested a header container put that back into the recycle queue
                if (spHeader)
                {
                    ctl::WeakRefPtr wrContainer;
                    IFC(spHeader.AsWeak(&wrContainer));
                    m_recycleHeaderQueue.push_back(std::move(wrContainer));
                }

                // Check if the header returned is in the recycle queue. If so, remove it
                spHeaderObtainedAsUIElement = spHeaderObtained.AsOrNull<UIElement>();
                BOOLEAN wasInRecycleQueue = FALSE;
                IFC(RemoveFromHeaderRecycleQueue(spHeaderObtainedAsUIElement.Get(), &wasInRecycleQueue));

                if (!wasInRecycleQueue)
                {
                    // Check if the header is already realized. If so, throw an exception
                    BOOLEAN hasParent = FALSE;
                    IFC(VisualTreeHelper::HasParentStatic(static_cast<UIElement*>(spHeaderObtainedAsUIElement.Get()), &hasParent));
                    if (hasParent)
                    {
                        IFC(ErrorHelper::OriginateErrorUsingResourceID(E_INVALID_OPERATION, ERROR_LISTVIEWBASE_GROUPHEADERCONTAINER_ALREADY_IN_USE));
                    }
                }

                // Got back a valid container, use that
                spHeader = spHeaderObtainedAsUIElement;
                IFC(spHeader.Cast<UIElement>()->InitVirtualizationInformation());
                SetElementIsGenerated(spHeader, TRUE);
            }
        }

        if (!spHeader)
        {
            // create a new group header
            IFC(spIHost->GetHeaderForGroup(spCurrentGroup.Get(), &spHeaderAsDO));
            IFC(spHeaderAsDO.As<IUIElement>(&spHeader));
            IFC(spHeader.Cast<UIElement>()->InitVirtualizationInformation());
            SetElementIsGenerated(spHeader, TRUE);

            IFC(spHeader.AsOrNull<xaml::IFrameworkElement>()->put_DataContext(nullptr));
        }
    }

    SetElementIsHeader(spHeader, TRUE);
    SetElementEmptySizeInGarbageSection(spHeader);

    // put it in the pinned containers, so that lookups will work during the Link and Prepare phases
    if (!foundPinnedHeader)
    {
        IFC(m_containerManager.RegisterPinnedHeader(index, spHeader));

        // if we are pinned, we've already been linked and prepared
        // per definition we have a matched group
        // todo: assert this?
        IFC(LinkHeaderToGroup(spHeader.Get(), spCurrentGroup.Cast<IInspectable>()));

        // possibly remove it from the pinned containers
        IFC(m_containerManager.UnregisterPinnedHeader(index));
    }

    // Item Containers/Headers have special rules regarding implicit show/hide animations.
    // Set a flag so that this case can be properly detected.
    static_cast<CUIElement*>(spHeader.Cast<UIElement>()->GetHandle())->SetIsItemContainer(true);

    IFC(spHeader.MoveTo(ppReturnValue));

Cleanup:
    RRETURN(hr);
}

// Is our container recycle queue empty?
_Check_return_ HRESULT DirectUI::ModernCollectionBasePanel::GetContainerRecycleQueueEmptyImpl( _Out_ BOOLEAN* pReturnValue )
{
    *pReturnValue = m_recycleQueue.empty();
    RRETURN(S_OK);
}

// Makes sure that whatever container we think is focused is still focused.
// Otherwise, recycle it.
_Check_return_ HRESULT ModernCollectionBasePanel::VerifyStoredFocusedContainer()
{
    HRESULT hr = S_OK;

    BOOLEAN stillHasFocus = FALSE;
    IFC(StoredFocusedContainerStillHasFocus(&stillHasFocus));
    if (!stillHasFocus && !m_containerManager.m_isScrollIntoViewPending)
    {
        IFC(ReleaseStoredFocusedContainer());
    }

Cleanup:
    RRETURN(hr);
}

// Does the previously stored focused container still have focus?
_Check_return_ HRESULT ModernCollectionBasePanel::StoredFocusedContainerStillHasFocus(_Out_ BOOLEAN* pHasFocus)
{
    HRESULT hr = S_OK;

    BOOLEAN hasFocus = FALSE;
    ctl::ComPtr<IUIElement> spFocusedContainer;
    INT itemIndex = -1;

    IFC(m_containerManager.GetFocusedContainer(&itemIndex, &spFocusedContainer));

    if (spFocusedContainer)
    {
        ctl::ComPtr<xaml::IFrameworkElement> spFE;
        spFE = spFocusedContainer.AsOrNull<xaml::IFrameworkElement>();

        if (spFE)
        {
            IFC(spFE.Cast<FrameworkElement>()->HasFocus(&hasFocus));

        }
    }

    *pHasFocus = hasFocus;

Cleanup:
    RRETURN(hr);
}

// If we have a stored focused container, release it back into the recycle queue.
_Check_return_ HRESULT ModernCollectionBasePanel::ReleaseStoredFocusedContainer()
{
    HRESULT hr = S_OK;

    ctl::ComPtr<IUIElement> spFocusedContainer;
    INT itemIndex = -1;

    IFC(m_containerManager.GetFocusedContainer(&itemIndex, &spFocusedContainer));

    if (spFocusedContainer)
    {
        // We can potentially re-use this container
        // Do not remove yet, just reset the reference to focused element, the removal will happen later.
        IFC(m_containerManager.ResetFocusedContainer(FALSE));

        // would not have made it into the recycle queue if it was recycled, but won't
        // be in valid range either
        if (!m_containerManager.IsValidContainerIndexWithinBounds(m_containerManager.GetValidContainerIndexFromItemIndex(itemIndex)))
        {
            // container is not in the valid range, so we can now really put it in the recyclequeue. Before this it was being
            // held onto by direct storage.

            // ICG2 will not actually recycle if an element is pinned. This makes sense, since the concept of pinning
            // is that something should not be recycled.
            // However, something being focused also counts as being pinned.
            // So we have to do the recycling ourselves. I'd rather do that than add an ugly parameter to recyclecontainer.

            // find out if it is still pinned, even after resetting the focused container
            if (!m_containerManager.GetIsContainerPinned(spFocusedContainer))
            {
                // now temporarily pin it, so that Clears know how to map index to container
                IFC(m_containerManager.SetFocusedContainer(itemIndex, spFocusedContainer));

                IFC(RecycleLinkedContainer(spFocusedContainer.Get()));

                // and finally clear it out again
                // This time remove the non-generated container from the tree
                IFC(m_containerManager.ResetFocusedContainer(TRUE));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Call UnlinkContainerFromItem and ClearContainerForItem on the given container. If the item isn't
// its own container, return the container to the recycle queue.
// For pinned items, this is a no-op.
_Check_return_ HRESULT ModernCollectionBasePanel::RecycleContainerImpl( _In_ xaml::IUIElement* container)
{
    HRESULT hr = S_OK;
    BOOLEAN recycled = FALSE;
    IFC(TryRecycleContainerImpl(container, &recycled));

Cleanup:
    RRETURN(hr);
}

// Call UnlinkContainerFromItem and ClearContainerForItem on the given container. If the item isn't
// its own container, return the container to the recycle queue.
// For pinned items, this is a no-op.
_Check_return_ HRESULT ModernCollectionBasePanel::TryRecycleContainerImpl(_In_ xaml::IUIElement* container, _Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;
    BOOLEAN recycled = FALSE;
    ctl::ComPtr<xaml::IUIElement> spContainer = container;

    // pinned containers do not get recycled
    if (!m_containerManager.GetIsContainerPinned(spContainer))
    {
        IFC(RecycleLinkedContainer(spContainer.Get()));
        recycled = TRUE;
    }

    *returnValue = recycled;
Cleanup:
    RRETURN(hr);
}


// Prepare the container for recycling by unlinking and clearing, then recycle the container.
// (recycling means different things depending on if the container is generated or not).
// Generated containers are left in the tree and reused for another item, while
// nongenerated containers are removed from the tree entirely (and will be re-added when the
// virtualization window is appropriate for them).
// Linked containers are containers which have had LinkContainerToItem and PrepareContainerForItem
// called on them.
_Check_return_ HRESULT ModernCollectionBasePanel::RecycleLinkedContainer(_In_ xaml::IUIElement* container)
{
    ctl::ComPtr<IGeneratorHost> spIHost;
    ctl::ComPtr<IInspectable> spItem;
    BOOLEAN isOwnContainer = FALSE;

    ctl::ComPtr<xaml::IUIElement> spContainer = container;
    auto p_virtualizationInformation = GetVirtualizationInformationFromElement(spContainer);
    // make a note that the user has not yet dismissed this container perse
    p_virtualizationInformation->SetWasRejectedAsAContainerByApp(false);
    // rejecting happens after we use this container as a suggestion to the CIC event.

    IFC_RETURN(m_cacheManager.GetItemsHost(&spIHost));

    IFC_RETURN(UnlinkContainerFromItem(spContainer.Get(), &spItem));

    // Items which are their own containers are not eligible for the recycle queue.
    IFC_RETURN(spIHost->IsItemItsOwnContainer(spItem.Get(), &isOwnContainer));

    // this calls clear on the container, this is expensive
    // However, we 'defer' the actual Clear to idle time when this panel is in a listview.
    // In fact, if a prepare comes along before we've had time to perform the clear, we will
    // skip the whole clear logic.
    IFC_RETURN(spIHost->ClearContainerForItem(spContainer.Cast<UIElement>(), spItem.Get()));

    // make the container not focusable by gamepad/2D auto focus. We do not
    // want a recycled container to get focused. This usually ends up causing
    // focus to jump when the container gets reused.
    IFC_RETURN(spContainer.Cast<UIElement>()->put_IsGamepadFocusCandidate(FALSE));

    TraceVirtualizedItemRemovedInfo((UINT64)spContainer.Cast<UIElement>()->GetHandle());

    // Only containers that are not items are eligible for recycling.
    // Ideally the item should specify what "recycling" means for itself,
    // (i.e., generated containers are pushed into the recycle queue and left in-tree,
    // while nongenerated containers are no-opped here).
    // For now this knowledge is rolled in here.
    if (!isOwnContainer)
    {
        ctl::WeakRefPtr wrContainer;

        ASSERT(GetElementIsGenerated(spContainer));

        IFC_RETURN(CoreImports::UIElement_CancelTransition(static_cast<CUIElement*>(spContainer.Cast<UIElement>()->GetHandle())));

        IFC_RETURN(spContainer.AsWeak(&wrContainer));
        m_recycleQueue.push_back(std::move(wrContainer));

        // Make sure we are not recycling a tracked element. If we are then
        // leave a trace breadcrumb and keep going. We are likely to hit a crash
        // shortly, but atleast the dump will have this callstack in the stowed
        // exceptions to figure out what caused this situation.
        if (m_viewportBehavior.isTracking &&
            m_viewportBehavior.type == xaml_controls::ElementType_ItemContainer)
        {
            ctl::ComPtr<IUIElement> trackedElement;
            IFC_RETURN(GetTrackedElement(&trackedElement));
            if (trackedElement.Get() == container)
            {
                TraceGuardFailure(L"TrackedContainerRecycled");
            }
        }
    }

    return S_OK;
}

// Calls RecycleContainer on all realized containers.
_Check_return_ HRESULT ModernCollectionBasePanel::RecycleAllContainersImpl()
{
    HRESULT hr = S_OK;

    for (INT32 validContainerIndex = 0; validContainerIndex < m_containerManager.GetValidContainerCount(); ++validContainerIndex)
    {
        ctl::ComPtr<IUIElement> spContainer;

        IFC(m_containerManager.GetContainerAtValidIndex(validContainerIndex, &spContainer));

        if (spContainer)
        {
            IFC(RecycleContainerImpl(spContainer.Get()));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Is our header recycle queue empty?
_Check_return_ HRESULT DirectUI::ModernCollectionBasePanel::GetHeaderRecycleQueueEmptyImpl( _Out_ BOOLEAN* pReturnValue )
{
    *pReturnValue = m_recycleHeaderQueue.empty();
    RRETURN(S_OK);
}

// Call UnlinkHeaderFromGroup and ClearContainerForGroup on the given header and return it to
// the recycle queue (except for pinned headers, for which this is a no-op).
_Check_return_ HRESULT ModernCollectionBasePanel::RecycleHeaderImpl( _In_ xaml::IUIElement* header )
{
    ctl::ComPtr<IGeneratorHost> spIHost;
    ctl::ComPtr<IInspectable> spItem;

    // move this container into the recycle queue
    ctl::ComPtr<xaml::IUIElement> spHeader = header;

    IFC_RETURN(m_cacheManager.GetItemsHost(&spIHost));

    // pinned headers do not get recycled
    if (!m_containerManager.GetIsHeaderPinned(spHeader))
    {
        ctl::WeakRefPtr wrHeader;
        IFC_RETURN(UnlinkHeaderFromGroup(spHeader.Get(), &spItem));
        IFC_RETURN(spIHost->ClearGroupContainerForGroup(spHeader.Cast<UIElement>(), spItem.AsOrNull<ICollectionViewGroup>().Get()));

        IFC_RETURN(spHeader.AsWeak(&wrHeader));
        ASSERT(GetElementIsGenerated(spHeader));
        m_recycleHeaderQueue.push_back(std::move(wrHeader));

        // Make sure we are not recycling a tracked element. If we are then
        // leave a trace breadcrumb and keep going. We are likely to hit a crash
        // shortly, but at least the dump will have this callstack in the stowed
        // exceptions to figure out what caused this situation.
        if (m_viewportBehavior.isTracking &&
            m_viewportBehavior.type == xaml_controls::ElementType_GroupHeader)
        {
            ctl::ComPtr<IUIElement> trackedElement;
            IFC_RETURN(GetTrackedElement(&trackedElement));
            if (trackedElement.Get() == header)
            {
                TraceGuardFailure(L"TrackedHeaderRecycled");
            }
        }

        // Protected by Apiset : TRUE only for Vertical Orientation + Headerplacement!=Left
        if (m_bUseStickyHeaders)
        {
            // Remove the parametric curves as the header reusing the container will have different curves
            RemoveStickyHeader(spHeader);
        }
    }

    return S_OK;
}

// Calls RecycleHeaderImpl on all realized headers.
_Check_return_ HRESULT ModernCollectionBasePanel::RecycleAllHeadersImpl()
{
    HRESULT hr = S_OK;

    for (INT32 validHeaderIndex = 0; validHeaderIndex < m_containerManager.GetValidHeaderCount(); ++validHeaderIndex)
    {
        ctl::ComPtr<IUIElement> spHeader;
        IFC(m_containerManager.GetHeaderAtValidIndex(validHeaderIndex, &spHeader));
        if (spHeader)
        {
            IFC(RecycleHeaderImpl(spHeader.Get()));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Associates the item to the container.
_Check_return_ HRESULT ModernCollectionBasePanel::LinkContainerToItem(_In_ const ctl::ComPtr<xaml::IUIElement>& spContainer, _In_ const ctl::ComPtr<IInspectable>& spItem)
{
    HRESULT hr = S_OK;

    // todo: validate the old code: it cast pItem to a DO first. Why?

    // linking the item to the container
    IFC(SetItemForElement(spContainer, spItem));

    // todo: decide if we still want to set datacontext here

Cleanup:
    RRETURN(hr);
}

// Associates the group to the header.
_Check_return_ HRESULT ModernCollectionBasePanel::LinkHeaderToGroup( _In_ const ctl::ComPtr<xaml::IUIElement>& spHeader, _In_ const ctl::ComPtr<IInspectable>& spItem )
{
    HRESULT hr = S_OK;

    // linking the item to the container
    IFC(SetItemForElement(spHeader, spItem));

Cleanup:
    RRETURN(hr);
}

// Clears the item from the container.
_Check_return_
    HRESULT
    ModernCollectionBasePanel::UnlinkContainerFromItem(_In_ const ctl::ComPtr<xaml::IUIElement>& spContainer, _Out_ ctl::ComPtr<IInspectable>* pspItem)
{
    HRESULT hr = S_OK;

    *pspItem = nullptr;

    // If we're part of a graph that got GC'ed, then don't worry about doing the following
    // cleanup. All of these objects will be removed from memory soon anyway.
    auto peggedContainer = ctl::try_make_autopeg(spContainer.Cast<UIElement>());

    if (peggedContainer)
    {
        UIElement::VirtualizationInformation* p_virtualizationInformation = GetVirtualizationInformationFromElement(spContainer);
        *pspItem = p_virtualizationInformation->GetItem();
        IFC(p_virtualizationInformation->SetItem(nullptr));
    }

Cleanup:
    RRETURN(hr);
}

// Clears the group from the header.
_Check_return_ HRESULT ModernCollectionBasePanel::UnlinkHeaderFromGroup(_In_ const ctl::ComPtr<xaml::IUIElement>& spHeader, _Out_ ctl::ComPtr<IInspectable>* pspItem )
{
    HRESULT hr = S_OK;

    *pspItem = nullptr;

    // If we're part of a graph that got GC'ed, then don't worry about doing the following
    // cleanup. All of these objects will be removed from memory soon anyway.
    auto peggedHeader = ctl::try_make_autopeg(spHeader.Cast<UIElement>());

    if (peggedHeader)
    {
        UIElement::VirtualizationInformation* p_virtualizationInformation = GetVirtualizationInformationFromElement(spHeader);
        *pspItem = p_virtualizationInformation->GetItem();
        IFC(p_virtualizationInformation->SetItem(nullptr));
    }

Cleanup:
    RRETURN(hr);
}

// naming confusion: for long we have shipped with a Refresh that did the work of what I would consider
// a reset. This has seeped into public api, and therefore I can't rectify that. So understand these definitions:
// *******    Refresh:   destructive, time consuming. We run this too often, but it is the way it is.
// *******    Reset:     maintain state as much as possible. We can't trust our concept of indices anymore and want to redo them
_Check_return_ HRESULT ModernCollectionBasePanel::RefreshImpl()
{
    HRESULT hr = S_OK;

    m_inCollectionChange = true;
    m_refreshPendingLayout = true;
    if (m_layoutInProgress)
    {
        TraceGuardFailure(L"RefreshDuringLayout");
    }

    // We should not be tracking anything at this stage.
    m_viewportBehavior.Reset();

    auto spScrollViewer = m_wrScrollViewer.AsOrNull<IScrollViewer>();

    // Note: there is a bit of internal overlap between ICG2 and our panel...


    // since this is a refresh, we really want to get rid of _all_ state, which means we should
    // be left with no focused container and other pinned containers even. Let us make sure they
    // are all gone
    m_containerManager.ClearPinAndFocus();

    // Recycle everything to give them a chance to call ClearContainerForItemOverride
    // (if separated, this code will be under IICG2::Refresh()
    IFC(RecycleAllHeaders());
    IFC(RecycleAllContainers());

    m_recycleQueue.clear();
    m_recycleHeaderQueue.clear();

    // Now we clear out everything regarding our children collection
    {
        ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

        IFC(m_cacheManager.ResetGroupCache());
        IFC(m_containerManager.Refresh());
        IFC(get_Children(&spChildren));
        IFC(spChildren->Clear());
    }

    IFC(ResetCacheBuffers());

    // also clear out any tracked element for orientation
    m_tpFirstVisibleElementBeforeOrientationChange.Clear();
    m_typeOfFirstVisibleElementBeforeOrientationChange = xaml_controls::ElementType_ItemContainer;
    m_indexOfFirstVisibleElementBeforeOrientationChange = -1;


    // reset all the caches that we know for sure are going to be invalid now
    m_firstCacheIndexBase = -1;
    m_firstVisibleIndexBase = -1;
    m_lastVisibleIndexBase = -1;
    m_lastCacheIndexBase = -1;
    m_firstCacheGroupIndexBase = -1;
    m_firstVisibleGroupIndexBase = -1;
    m_lastVisibleGroupIndexBase = -1;
    m_lastCacheGroupIndexBase = -1;

    IFC(UpdateGroupStyle());

    // TODO: Decide which element we want to scroll to in the new view

    // We should be done with all collection change related work at this point.
    // The ChangeViewWithOptionalAnimation call below might, in turn, call UpdateLayout
    // if the viewport is in manipulation; and we don't want to trigger the layout
    // guard during layout.
    m_inCollectionChange = false;

    // We either don't have an element to target, or targeting an element is meaningless
    // because we don't yet have a valid window. Just go to 0,0 in the SV so the header is visible.
    if (spScrollViewer)
    {
        // For now, we assume we aren't in a nested SV.
        ctl::ComPtr<IInspectable> spHorizontalOffset;
        ctl::ComPtr<IInspectable> spVerticalOffset;
        ctl::ComPtr<wf::IReference<DOUBLE>> spHorizontalOffsetReference;
        ctl::ComPtr<wf::IReference<DOUBLE>> spVerticalOffsetReference;
        BOOLEAN handled;

        IFC(PropertyValue::CreateFromDouble(0, &spHorizontalOffset));
        IFC(spHorizontalOffset.As(&spHorizontalOffsetReference));
        IFC(PropertyValue::CreateFromDouble(0, &spVerticalOffset));
        IFC(spVerticalOffset.As(&spVerticalOffsetReference));

        IFC(spScrollViewer->ChangeViewWithOptionalAnimation(
            spHorizontalOffsetReference.Get(),
            spVerticalOffsetReference.Get(),
            nullptr /*zoom */,
            TRUE /*disable animation */,
            &handled));
    }

    IFC(InvalidateMeasure());

Cleanup:
    RRETURN(hr);
}

// being called from ItemsControl when ItemCollection calls into it for a NotifyOfSourceChanged
// this should only be called for items.
_Check_return_ HRESULT
ModernCollectionBasePanel::NotifyOfItemsChangingImpl(
    _In_ wfc::IVectorChangedEventArgs* e)
{
    HRESULT hr = S_OK;
    wfc::CollectionChange action;
    UINT nIndex = 0;

    if (m_layoutInProgress)
    {
        TraceGuardFailure(L"ItemsChangingDuringLayout");
    }

    if (m_inCollectionChange)
    {
        // We are already in the middle of a collection change.
        // we are headed for a crash. Fail here preemptively
        IFC(E_UNEXPECTED);
    }

    m_inCollectionChange = true;

    IFC(e->get_CollectionChange(&action));

    IFC(m_transitionContextManager.NotifyOfItemsChanging(action));

    switch (action)
    {
    case wfc::CollectionChange_ItemInserted:
        IFC(e->get_Index(&nIndex));
        IFC(OnItemAdded(static_cast<INT>(nIndex)));
        break;

    case wfc::CollectionChange_Reset:
    {
        if (m_cacheManager.IsGrouping() && m_cacheManager.GetTotalGroupCount() > 0 && !m_cacheManager.ShouldInterpretResetAsFullReset())
        {
            // can be that we are getting this reset but we have already refreshed the collection
            // because we've also had a NotifyOfGroupChanged with a reset

            // in case of grouping the reset event which is comming from CVS implementation has the group index
            // which needs to be used.
            // Note: In custom implementation of ICollectionView customer can decide to use WinRT Vector
            // WinRT vector fail if on Reset action we are reading index. We are workarounding this error by ignoring it.
            // Thus we still get benefit of using index of group where Reset happens provided by build-in implementation of our CVS.
            hr = e->get_Index(&nIndex);
            if (hr == E_FAIL)
            {
                nIndex = 0;
                hr = S_OK;
            }
            IFC(hr);

            IFC(OnGroupReset(nIndex));
        }
        else
        {
            IFC(OnCollectionReset());
        }
        break;
    }
    case wfc::CollectionChange_ItemChanged:
    case wfc::CollectionChange_ItemRemoved:
        // we do not react to removed before others have processed the removal
        break;

    default:
        // Impossible to get here:
        __assume(0);
    }

Cleanup:
    m_inCollectionChange = false;
    RRETURN(hr);
}

_Check_return_ HRESULT
ModernCollectionBasePanel::NotifyOfItemsChangedImpl(
    _In_ wfc::IVectorChangedEventArgs* e)
{
    HRESULT hr = S_OK;
    wfc::CollectionChange action;
    UINT nIndex = 0;

    if (m_layoutInProgress)
    {
        TraceGuardFailure(L"ItemsChangedDuringLayout");
    }

    if (m_inCollectionChange)
    {
        // We are already in the middle of a collection change.
        // we are headed for a crash. Fail here preemptively
        IFC(E_UNEXPECTED);
    }

    m_inCollectionChange = true;

    IFC(e->get_CollectionChange(&action));

    IFC(m_transitionContextManager.NotifyOfItemsChanged(action));

    switch (action)
    {
    case wfc::CollectionChange_ItemInserted:
    case wfc::CollectionChange_Reset:
        // we only respond to removed
        break;
    case wfc::CollectionChange_ItemRemoved:
        IFC(e->get_Index(&nIndex));
        IFC(OnItemRemoved(static_cast<INT>(nIndex)));
        break;
    case wfc::CollectionChange_ItemChanged:
        IFC(e->get_Index(&nIndex));
        IFC(OnDataElementReplaced(xaml_controls::ElementType_ItemContainer, static_cast<INT>(nIndex)));
        break;
    default:
        // Impossible to get here:
        __assume(0);
    }

    IFC(OnCollectionChangeProcessed());

Cleanup:
    m_inCollectionChange = false;
    RRETURN(hr);
}

// The hookup here is strange. We follow the old ICG's pattern of hooking up ourselves
// even though I think in the future it is better if IC just calls into us. For now, IC
// does not get notifications of group changes and we subscribe ourselves in RegisterItemsHost.
_Check_return_ HRESULT ModernCollectionBasePanel::NotifyOfGroupsChanged(
    _In_ wfc::IObservableVector<IInspectable*>* pSender,
    _In_ wfc::IVectorChangedEventArgs* e)
{
    HRESULT hr = S_OK;
    wfc::CollectionChange action;
    UINT nIndex = 0;

    if (m_layoutInProgress)
    {
        TraceGuardFailure(L"GroupsChangedDuringLayout");
    }

    if (m_inCollectionChange)
    {
        // We are already in the middle of a collection change.
        // we are headed for a crash. Fail here preemptively
        IFC(E_UNEXPECTED);
    }

    m_inCollectionChange = true;

    IFC(e->get_CollectionChange(&action));

    IFC(m_transitionContextManager.NotifyOfGroupsChanged(action));

    if (m_cacheManager.ShouldInterpretResetAsFullReset())
    {
        // cvs is rebuilding its groups and raising events, we need to ignore until we
        // get a item reset event
        goto Cleanup;
    }

    switch (action)
    {
    case wfc::CollectionChange_ItemInserted:
        IFC(e->get_Index(&nIndex));
        IFC(OnGroupAdded(static_cast<INT>(nIndex)));
        break;

    case wfc::CollectionChange_ItemRemoved:
        IFC(e->get_Index(&nIndex));
        IFC(OnGroupRemoved(static_cast<INT>(nIndex)));
        break;

    case wfc::CollectionChange_ItemChanged:
        IFC(e->get_Index(&nIndex));
        IFC(OnDataElementReplaced(xaml_controls::ElementType_GroupHeader, static_cast<INT>(nIndex)));
        break;

    case wfc::CollectionChange_Reset:
        // we are getting a reset on the group collection. In CVS implementations, this is going to be
        // followed by a reset on the (flat) item collection. After discussing with the feature team
        // we have decided it is very hard to do a reset here and now and then somehow know how to interpret the
        // flat reset that is coming in.
        // However, the flat reset has no way of indicating the 'full-ness' of the reset, since the args.index is
        // specified as a uint, so they cannot do a -1. That reset also tends to be raised if individual groups have been
        // reset, in which case the args.index _does_ make sense. So, we're setting a boolean here, that we will clear in
        // OnItemCollectionChanged event handler.
        // This is not perfect, and will fall down if consumers do not implement grouping per guidelines. Tough luck, it is
        // impossible to change the eventing ordering at this point in the release.
        // We should follow up with a drastic amount of samples
        m_cacheManager.RegisterInterpretResetAsFullReset();
        break;

    default:
        // Impossible to get here:
        __assume(0);
    }

    IFC(OnCollectionChangeProcessed());

Cleanup:
    m_inCollectionChange = false;
    RRETURN(hr);
}

// item added in the collection
_Check_return_ HRESULT ModernCollectionBasePanel::OnItemAdded(_In_ INT nIndex)
{
    HRESULT hr = S_OK;

#ifdef MCBP_DEBUG
    WCHAR szTrace[256];
    IFCEXPECT(swprintf_s(szTrace, 256, L"Item added, index %d", nIndex) >= 0);
    Trace(szTrace);
#endif

    // todo: fill in bug number. We need to remove this if condition when they fix their code.
    // 5/7/2015. VoiceRecorder app actually gives us the wrong indices when raising events. We do not want to RI
    // a change that will break them, but are working with them to fix their code. A very scoped change to understand
    // their scenario and regress to the old reset code path.
    if (m_cacheManager.GetTotalItemCount() < nIndex)
    {
#ifdef MCBP_DEBUG
        WCHAR szTrace[256];
        IFCEXPECT(swprintf_s(szTrace, 256, L"Going into regressed OnItemAdded path because index of add %d is bigger then total item count", nIndex) >= 0);
        Trace(szTrace);
#endif
        IFC(m_cacheManager.ResetGroupCache());
    }
    else
    {
        // caches are no longer valid
        // if we are in 'group filling mode', the groupAdded event has already taken care of the correct
        // renewal of the cache
        if (m_containerManager.m_unlinkedFillers.empty())
        {
            // so we only care if we are not filling out a groupAdd
            IFC(m_cacheManager.RenewCacheAfterMutation(nIndex, -1, -1, -1));
        }
    }

    {
        // sets up caches to make iterating over groups much faster, caches children collection, host, etc.
        auto strongCache = m_cacheManager.CacheStrongRefs(&hr); // Releases when it goes out of scope at the end of method
        IFC(hr);

        // super special case: if we added a group, we might be in 'group filling mode'.
        // GroupedDataCollection gives us back inserts going backward, so we need to recognize this behavior and correct
        // our perceived index
        if (!m_containerManager.m_unlinkedFillers.empty())
        {
            if (m_containerManager.m_unlinkedFillers.back() == nIndex)
            {
                // yep, we can always expect that it starts from high indices to low
                nIndex = m_containerManager.m_unlinkedFillers.front();
                // and remove it
                m_containerManager.m_unlinkedFillers.pop_back();
            }
            else
            {
                // well, that means there should not be any unlinkedfillers
                IFCEXPECT(m_containerManager.m_unlinkedFillers.empty());
            }
        }

        IFC(BeginTrackingOnCollectionChange(false /* isGroupChange */, nIndex, wfc::CollectionChange_ItemInserted));

        // while indexes are still correct, let the pinned containers know
        // and update our mapping indices
        IFC(m_containerManager.OnItemAdded(nIndex));

        // should we insert a sentinel? Do so if the container would be added inside the valid range
        // If we only have a single valid container, there is no need for a sentinel, as all the mapping will be
        // accurate, as the new item lies outside the realized block. We only care about insertions *inside* the block
        if (m_containerManager.GetValidContainerCount() > 1 &&
            m_containerManager.GetItemIndexFromValidIndex(0) < nIndex &&
            nIndex <= m_containerManager.GetItemIndexFromValidIndex(m_containerManager.GetValidContainerCount() - 1))
        {
            IFC(m_containerManager.InsertSentinelContainer(nIndex));
        }

        // If this insertion happened before our collection, we won't insert a sentinel, but we do need to shift the location
        // of our containers appropriately. Find the first container, and use its size to perform the shift
        // When tracking, we will shift the tracked element instead.
        if (m_containerManager.SizeOfContainerVisualSection() > 0 &&
            nIndex < m_containerManager.GetItemIndexFromValidIndex(0) &&
            !m_viewportBehavior.isTracking)
        {
            ctl::ComPtr<IUIElement> spFirstContainer;
            wf::Size desiredSize;
            wf::Rect newLocation;
            LayoutReference referenceInformation = CreateDefaultLayoutReference();

            IFC(m_containerManager.GetContainerAtValidIndex(0, &spFirstContainer));
            IFCEXPECT(ElementHasVirtualizationInformation(spFirstContainer));

            IFC(spFirstContainer->get_DesiredSize(&desiredSize));
            referenceInformation.ReferenceIsHeader = FALSE;
            referenceInformation.RelativeLocation = xaml_controls::ReferenceIdentity_BeforeMe;
            referenceInformation.ReferenceBounds = GetBoundsFromElement(spFirstContainer);

            IFC(m_spLayoutStrategy->GetElementBounds(
                xaml_controls::ElementType_ItemContainer,
                nIndex,
                desiredSize,
                referenceInformation,
                m_windowState.GetRealizationWindow(),
                &newLocation));

            SetBoundsForElement(spFirstContainer, newLocation);
        }

        // always re-do measure even if outside of the valid window (for the extents)
        IFC(InvalidateMeasure());
    }

Cleanup:
#ifdef DBG
    m_debugIsInTemporaryInvalidState = FALSE;
#endif
    RRETURN(hr);
}

// item removed in the collection
_Check_return_ HRESULT ModernCollectionBasePanel::OnItemRemoved(INT nIndex, bool itemRemovedForGroupReset /* = false */)
{
    HRESULT hr = S_OK;

#ifdef MCBP_DEBUG
    WCHAR szTrace[256];
    IFCEXPECT(swprintf_s(szTrace, 256, L"Item removed, index %d", nIndex) >= 0);
    Trace(szTrace);
#endif

    // caches are no longer valid
    IFC(m_cacheManager.RenewCacheAfterMutation(-1, nIndex, -1, -1));

    {
        ctl::ComPtr<IUIElement> spPinnedContainer;
        // sets up caches to make iterating over groups much faster, caches children collection, host, etc.
        auto strongCache = m_cacheManager.CacheStrongRefs(&hr); // Releases when it goes out of scope at the end of method
        IFC(hr);

        IFC(BeginTrackingOnCollectionChange(false /* isGroupChange */, nIndex, wfc::CollectionChange_ItemRemoved));

        // get the container from the pinned collection
        IFC(m_containerManager.GetContainerFromPinnedContainers(nIndex, &spPinnedContainer));

        // while indexes are still correct, let the pinned containers know
        IFC(m_containerManager.OnItemRemoved(nIndex));

        if (m_containerManager.IsValidContainerIndexWithinBounds(m_containerManager.GetValidContainerIndexFromItemIndex(nIndex)))
        {
            ctl::ComPtr<IUIElement> spOldContainer;
            bool isTransitioning = false;
            TransitionTrigger trigger = TransitionTrigger::Layout;
            INT validIndex = m_containerManager.GetValidContainerIndexFromItemIndex(nIndex);
            IFC(m_containerManager.GetContainerAtValidIndex(validIndex, &spOldContainer));

#ifdef DBG
            m_debugIsInTemporaryInvalidState = TRUE;
#endif

            // recognize that EnsureRecycleContainer does more work: it will ensure the recycling of headers
            // before a group whose containers are partially virtualized.
            // However, we cannot call Ensure because its core functionality is different: it will try
            // and recycle if something is outside of bounds. (It will potentially recycle not the first container
            // (if there was a focused container in between), but that is not the case since we already removed
            // this container from the pinned containers etc.).

            // so removing a first or last container might leave us with a group that has more than 0 items, but no
            // valid containers. This can happen if this container is the last container of group at the front
            // or the first container of a group at the back. In that case we need to recycle the group as well to
            // ensure our contiguousness

            IFC(m_containerManager.RemoveFromValidContainers(validIndex, TRUE /* isForItemRemoval */));
            if (spOldContainer)
            {
                IFC(CoreImports::UIElement_GetHasTransition(static_cast<CUIElement*>(spOldContainer.Cast<UIElement>()->GetHandle()), &isTransitioning, &trigger));
                if (!isTransitioning || trigger != TransitionTrigger::Unload)
                {
                    // unloading queue is needed only for callback from transition completion routine.
                    IFC(FinishUnlinkingContainer(spOldContainer.Cast<UIElement>(), FALSE /* useUnloadingQueue */));
                }
                else
                {
                    ctl::ComPtr<DeferredModernPanelUnlinkActionPayload> spPayload;

                    IFC(ctl::make(&spPayload));
                    spPayload->InitializePayload(this);

                    IFC(spOldContainer.Cast<UIElement>()->SetValueInternal(
                        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ItemContainerGenerator_DeferredUnlinkingPayload), spPayload.Get(), /* fAllowReadOnly */ TRUE));

                    IFC(CoreImports::UIElement_SetDeferUnlinkContainer(static_cast<CUIElement*>(spOldContainer.Cast<UIElement>()->GetHandle())));
                    IFC(m_unloadingElements->Append(spOldContainer.Get()));
                }
            }
#ifdef DBG
            m_debugIsInTemporaryInvalidState = FALSE;
#endif

            if (m_cacheManager.IsGrouping() && itemRemovedForGroupReset == false)
            {
                INT32 indexOfGroup = -1;
                INT32 indexInsideGroup = -1;
                INT32 itemCountInGroup = -1;

                if (m_cacheManager.WasGrouping())
                {
                    IFC(m_cacheManager.GetGroupHistoryInformationFromItemIndex(nIndex,
                        &indexOfGroup,
                        &indexInsideGroup,
                        &itemCountInGroup));
                }

                if (validIndex == 0 || validIndex == m_containerManager.GetValidContainerCount())   // notice that since we removed, the current validContainerCount is already subtracted by 1
                {
                    // notice how we are using information that was gathered before we reset the caches.
                    // The item removed is the first (or) last item in the window. Note that we do not
                    // do this for items that are in the middle of the window. So if there is a group in the middle
                    // with hides if empty is true and its last item was removed, we do not recycle the header until
                    // the next generate.

                    if (validIndex == 0 && indexInsideGroup == itemCountInGroup - 1 && itemCountInGroup > 1
                        && m_containerManager.GetValidHeaderIndexFromGroupIndex(indexOfGroup) == 0)
                    {
                        // we have just removed the last container in a non-empty group
                        // that means that we should have a header. Based on contiguous promises, that header
                        // should be the first one
                        // H - Header
                        // C - Container
                        // _ - Unrealized
                        // X - Deleted
                        // R - Header that needs to be recycled
                        //  Window  H _ _ C H C C C
                        //          R     X
                        IFC(EnsureRecycleHeaderCandidate(TRUE, TRUE /* always recycle */));
                    }

                    if (validIndex != 0 && indexInsideGroup == 0 && itemCountInGroup > 1
                        && m_containerManager.GetValidHeaderIndexFromGroupIndex(indexOfGroup) == m_containerManager.GetValidHeaderCount() - 1)
                    {
                        // we have just removed the first container of a non-empty group
                        // Window   H C C C H C _ _ _
                        //                  R X
                        IFC(EnsureRecycleHeaderCandidate(FALSE, TRUE /* always recycle */));
                    }

                    if (m_cacheManager.GetHidesIfEmpty() && itemCountInGroup == 1)
                    {
                        // hidesIfEmpty is true and
                        // we have removed the only item in the group
                        // pre-emptively recycle the header as well
                        BOOLEAN fromFront = validIndex == 0;
                        IFC(EnsureRecycleHeaderCandidate(fromFront, TRUE /* always recycle */));
                    }
                }
            }
        }
        else
        {
            // we removed something in front of the valid range
            if (nIndex < m_containerManager.GetItemIndexFromValidIndex(0))
            {
                m_containerManager.UpdateItemIndexForFirstValidContainer(m_containerManager.GetItemIndexFromValidIndex(0) - 1);
            }

            if (spPinnedContainer)
            {
                // we removed a pinned container that is not in the valid range
                IFC(RecycleContainer(spPinnedContainer.Get()));
            }
        }

        if (nIndex <= m_containerManager.GetItemIndexFromValidIndex(0) && m_containerManager.GetValidContainerCount() > 0)
        {
            INT32 indexInGroup = -1;

            if (m_cacheManager.IsGrouping())
            {
                IFC(m_cacheManager.GetGroupInformationFromItemIndex(
                    m_containerManager.GetItemIndexFromValidIndex(0),
                    nullptr,
                    &indexInGroup,
                    nullptr));
            }

            // the first container is potentially located incorrectly
            // redo its layout based on himself!
            // you would think we can just take the removed containers location, but we also need to
            // redo work if there was no old container, so this container is really all we can use.
            // When tracking, we will shift the tracked element instead.

            // ungrouped logic:
            // take the (potentially new) first containers location and act as though it is the location
            // of the next container, then 'generateContainer' for this new first container.

            // grouped logic: there is always going to be a header before the item (on which to base forward positions on
            // during measure), so we only care about correcting if we are not at groupInIndex 0

            if ((!m_cacheManager.IsGrouping() || indexInGroup > 0) && !m_viewportBehavior.isTracking)
            {
                LayoutReference referenceInformation = CreateDefaultLayoutReference();
                ctl::ComPtr<IUIElement> spFirstContainer;
                wf::Size containerDesiredSize;
                wf::Rect newLocation;

                IFC(m_containerManager.GetContainerAtValidIndex(0, &spFirstContainer));
                IFCEXPECT(ElementHasVirtualizationInformation(spFirstContainer));

                IFC(spFirstContainer->get_DesiredSize(&containerDesiredSize));

                referenceInformation.ReferenceIsHeader = FALSE;
                referenceInformation.RelativeLocation = xaml_controls::ReferenceIdentity_AfterMe;
                referenceInformation.ReferenceBounds = GetBoundsFromElement(spFirstContainer);

                IFC(m_spLayoutStrategy->GetElementBounds(
                    xaml_controls::ElementType_ItemContainer,
                    m_containerManager.GetItemIndexFromValidIndex(0),
                    containerDesiredSize,
                    referenceInformation,
                    m_windowState.GetRealizationWindow(),
                    &newLocation));

                SetBoundsForElement(spFirstContainer, newLocation);
            }

        }

        // always re-do measure even if outside of the valid window (for the extents)
        IFC(InvalidateMeasure());
    }

Cleanup:
#ifdef DBG
    m_debugIsInTemporaryInvalidState = FALSE;
#endif
    RRETURN(hr);
}

// Group added to the collection.
_Check_return_ HRESULT ModernCollectionBasePanel::OnGroupAdded(_In_ INT nIndex)
{
    HRESULT hr = S_OK;

#ifdef MCBP_DEBUG
    WCHAR szTrace[256];
    IFCEXPECT(swprintf_s(szTrace, 256, L"Group added, index %d", nIndex) >= 0);
    Trace(szTrace);
#endif

    // caches are no longer valid
    IFC(m_cacheManager.RenewCacheAfterMutation(-1, -1, nIndex, -1));

    {
        // sets up caches to make iterating over groups much faster, caches children collection, host, etc.
        auto strongCache = m_cacheManager.CacheStrongRefs(&hr);
        IFC(hr);

        IFC(BeginTrackingOnCollectionChange(true /* isGroupChange */, nIndex, wfc::CollectionChange_ItemInserted));

        // while indexes are still correct, let the pinned containers know
        IFC(m_containerManager.OnGroupAdded(nIndex));

        // should we insert a sentinel? Do so if the header would be added inside the valid range
        // If we only have a single valid header, there is no need for a sentinel, as all the mapping will be
        // accurate, as the new item lies outside the realized block. We only care about insertions *inside* the block
        if (m_containerManager.GetValidHeaderCount() > 1 &&
            m_containerManager.GetGroupIndexFromValidIndex(0) < nIndex &&
            nIndex <= m_containerManager.GetGroupIndexFromValidIndex(m_containerManager.GetValidHeaderCount()-1))
        {
            IFC(m_containerManager.InsertSentinelHeader(nIndex));
        }

        // If this insertion happened before our collection, we won't insert a sentinel, but we do need to shift the location
        // of our headers appropriately. Find the first header, and use its size to perform the shift
        // When tracking, we will shift the tracked element instead.
        if (m_containerManager.SizeOfHeaderVisualSection() > 0 &&
            nIndex < m_containerManager.GetGroupIndexFromValidIndex(0) &&
            !m_viewportBehavior.isTracking)
        {
            ctl::ComPtr<IUIElement> spFirstHeader;
            wf::Size desiredSize;
            wf::Rect newLocation;
            LayoutReference referenceInformation = CreateDefaultLayoutReference();

            IFC(m_containerManager.GetHeaderAtValidIndex(0, &spFirstHeader));
            IFCEXPECT(ElementHasVirtualizationInformation(spFirstHeader));

            IFC(spFirstHeader->get_DesiredSize(&desiredSize));
            referenceInformation.ReferenceIsHeader = TRUE;
            referenceInformation.RelativeLocation = xaml_controls::ReferenceIdentity_BeforeMe;
            referenceInformation.ReferenceBounds = GetBoundsFromElement(spFirstHeader);

            const INT32 layoutIndex = m_cacheManager.DataIndexToLayoutIndex(xaml_controls::ElementType_GroupHeader, nIndex);
            IFC(m_spLayoutStrategy->GetElementBounds(
                xaml_controls::ElementType_GroupHeader,
                layoutIndex,
                desiredSize,
                referenceInformation,
                m_windowState.GetRealizationWindow(),
                &newLocation));

            SetBoundsForElement(spFirstHeader, newLocation);
        }

        // always re-do measure even if outside of the valid window (for the extents)
        IFC(InvalidateMeasure());
    }

    IFC(CreateUnlinkedFillersForGroup(nIndex));

Cleanup:
#ifdef DBG
    m_debugIsInTemporaryInvalidState = FALSE;
#endif
    RRETURN(hr);
}

// Group removed in the collection
_Check_return_ HRESULT ModernCollectionBasePanel::OnGroupRemoved(_In_ INT nIndex)
{
    HRESULT hr = S_OK;

#ifdef MCBP_DEBUG
    WCHAR szTrace[256];
    IFCEXPECT(swprintf_s(szTrace, 256, L"Group Removed, index %d", nIndex) >= 0);
    Trace(szTrace);
#endif

    // caches are no longer valid
    IFC(m_cacheManager.RenewCacheAfterMutation(-1, -1, -1, nIndex));

    {
        ctl::ComPtr<IUIElement> spPinnedHeader;

        // sets up caches to make iterating over groups much faster, caches children collection, host, etc.
        auto strongCache = m_cacheManager.CacheStrongRefs(&hr); // Releases when it goes out of scope at the end of method
        IFC(hr);

        IFC(BeginTrackingOnCollectionChange(true /* isGroupChange */, nIndex, wfc::CollectionChange_ItemRemoved));

        // get the container from the pinned collection
        IFC(m_containerManager.GetHeaderFromPinnedHeaders(nIndex, &spPinnedHeader));

        // while indexes are still correct, let the pinned containers know
        IFC(m_containerManager.OnGroupRemoved(nIndex));

        if (m_containerManager.IsValidHeaderIndexWithinBounds(m_containerManager.GetValidHeaderIndexFromGroupIndex(nIndex)))
        {
            ctl::ComPtr<IUIElement> spOldHeader;
            INT validIndex = m_containerManager.GetValidHeaderIndexFromGroupIndex(nIndex);

#ifdef DBG
            m_debugIsInTemporaryInvalidState = TRUE;
#endif

            IFC(m_containerManager.GetHeaderAtValidIndex(validIndex, &spOldHeader));

            IFC(m_containerManager.RemoveFromValidHeaders(validIndex, TRUE /* isForGroupRemoval */));

            if (spOldHeader)
            {
                IFC(RecycleHeader(spOldHeader.Get()));
            }

#ifdef DBG
            m_debugIsInTemporaryInvalidState = FALSE;
#endif
        }
        else
        {
            if (spPinnedHeader)
            {
                // we removed a pinned container that is not in the valid range
                IFC(RecycleHeader(spPinnedHeader.Get()));
            }

            // we removed something in front of the valid range
            if (nIndex < m_containerManager.GetGroupIndexFromValidIndex(0))
            {
                m_containerManager.UpdateGroupIndexForFirstValidHeader(m_containerManager.GetGroupIndexFromValidIndex(0) -1 );
            }
        }


        if (nIndex <= m_containerManager.GetGroupIndexFromValidIndex(0) && m_containerManager.GetValidHeaderCount() > 0)
        {
            // When tracking, we will shift the tracked element instead.
            if(!m_viewportBehavior.isTracking)
            {
                // the first container is potentially located incorrectly
                // redo its layout based on himself!
                // you would think we can just take the removed containers location, but we also need to
                // redo work if there was no old container, so this container is really all we have
                // to use.
                LayoutReference referenceInformation = CreateDefaultLayoutReference();
                ctl::ComPtr<IUIElement> spFirstHeader;
                UIElement::VirtualizationInformation* p_virtualizationInformation;
                wf::Size desiredSize;
                wf::Rect newLocation;

                // we'll use the first container as a reference
                IFC(m_containerManager.GetHeaderAtValidIndex(0, &spFirstHeader));
                p_virtualizationInformation = GetVirtualizationInformationFromElement(spFirstHeader);
                IFC(spFirstHeader->get_DesiredSize(&desiredSize));

                referenceInformation.ReferenceIsHeader = TRUE;
                referenceInformation.RelativeLocation = xaml_controls::ReferenceIdentity_AfterMe;
                referenceInformation.ReferenceBounds = p_virtualizationInformation->GetBounds();

                const INT32 layoutIndex = m_cacheManager.DataIndexToLayoutIndex(
                    xaml_controls::ElementType_GroupHeader,
                    m_containerManager.GetGroupIndexFromValidIndex(0));

                IFC(m_spLayoutStrategy->GetElementBounds(
                    xaml_controls::ElementType_GroupHeader,
                    layoutIndex,
                    desiredSize,
                    referenceInformation,
                    m_windowState.GetRealizationWindow(),
                    &newLocation));

                p_virtualizationInformation->SetBounds(newLocation);
            }
        }

        // always re-do measure even if outside of the valid window (for the extents)
        IFC(InvalidateMeasure());
    }

Cleanup:
#ifdef DBG
    m_debugIsInTemporaryInvalidState = FALSE;
#endif
    RRETURN(hr);
}

// Group reset in the collection
_Check_return_ HRESULT ModernCollectionBasePanel::OnGroupReset(_In_ INT groupIndex)
{
    HRESULT hr = S_OK;
    INT startItemIndex = 0;
    INT itemCountInGroup = 0;

#ifdef MCBP_DEBUG
    WCHAR szTrace[256];
    IFCEXPECT(swprintf_s(szTrace, 256, L"Group reset, index %d", groupIndex) >= 0);
    Trace(szTrace);
#endif

    m_containerManager.SetIndexOfGroupBeingReset(groupIndex);

    IFC(m_cacheManager.GetGroupInformationFromGroupIndex(groupIndex, &startItemIndex, &itemCountInGroup));

    for(INT i = 0; i < itemCountInGroup; ++i)
    {
        IFC(OnItemRemoved(startItemIndex, true /* itemRemovedForGroupReset */));
    }

    // Ensures that the group header at the end of the valid range gets recycled if it's the header
    // for the current group (i.e. groupIndex).
    if (m_cacheManager.GetHidesIfEmpty() &&
        m_containerManager.GetValidHeaderCount() > 0)
    {
        if (m_containerManager.GetGroupIndexFromValidIndex(0) == groupIndex)
        {
            IFC(EnsureRecycleHeaderCandidate(true /* fromFront */, true /* alwaysRecycle */));
        }
        else if (m_containerManager.GetGroupIndexFromValidIndex(m_containerManager.GetValidHeaderCount() - 1) == groupIndex)
        {
            IFC(EnsureRecycleHeaderCandidate(false /* fromFront */, true /* alwaysRecycle */));
        }
    }

    // Group resets don't generate a set of item additions, so we need to clear out any fillers
    m_containerManager.m_unlinkedFillers.clear();

    // Reset the group cache so that the cache manager is aware
    // of the new items in the group that just got reset.
    IFC(m_cacheManager.ResetGroupCache());

    {
        auto strongCache = m_cacheManager.CacheStrongRefs(&hr);
        IFC(hr);

        INT newItemCountInGroup = 0;
        IFC(m_cacheManager.GetGroupInformationFromGroupIndex(groupIndex, nullptr, &newItemCountInGroup));

        for (INT i = 0; i < newItemCountInGroup; ++i)
        {
            IFC(OnItemAdded(startItemIndex));
        }

        IFC(InvalidateMeasure());
    }

Cleanup:
    m_containerManager.InvalidateIndexOfGroupBeingReset();
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::OnDataElementReplaced(_In_ xaml_controls::ElementType type, _In_ INT index)
{
    ctl::ComPtr<IUIElement> spOldElement;

    // the cache only actually changes when a group has been replaced.
    // tell the cache manager that the groups have really changed
    if (type == xaml_controls::ElementType::ElementType_GroupHeader)
    {
        IFC_RETURN(m_cacheManager.ResetGroupCache());
    }

    {
        // Use the interface mapping method, as it will also retrieve pinned elements
        ctl::ComPtr<IDependencyObject> spOldElementAsDO;
        IFC_RETURN(m_containerManager.Interface_ElementFromDataIndex(type, index, &spOldElementAsDO));
        spOldElement = spOldElementAsDO.AsOrNull<IUIElement>();
    }

    const BOOLEAN wasFocused = m_containerManager.GetIsIndexFocused(type, index);
    const BOOLEAN wasPinned = m_containerManager.GetIsIndexPinned(type, index);

    if (wasFocused)
    {
        IFC_RETURN(m_containerManager.ResetFocusedContainer(TRUE));
    }
    if (wasPinned)
    {
        IFC_RETURN(m_containerManager.UnregisterPinnedElement(type, index));
    }

    if (spOldElement || wasFocused || wasPinned)
    {
        // If we want animations to run, we can't just throw new content into the UI element
        // So, we remove/recycle the old one, and generate a new replacement
        // If this element was realized, let's get the old bounds and generate a new element at the same location
        ctl::ComPtr<IUIElement> spNewElement;
        HRESULT hr = S_OK;

        auto strongCache = m_cacheManager.CacheStrongRefs(&hr);
        IFC_RETURN(hr);
        IFC_RETURN(BeginTrackingOnCollectionChange(type == xaml_controls::ElementType_GroupHeader /* isGroupChange */, index, wfc::CollectionChange_ItemChanged));

        const wf::Rect oldElementBounds = GetBoundsFromElement(spOldElement);

        switch(type)
        {
        case xaml_controls::ElementType_GroupHeader:
            {
                IFC_RETURN(GenerateHeaderAtGroupIndexImpl(index, &spNewElement));
            }
            break;

        case xaml_controls::ElementType_ItemContainer:
            {
                IFC_RETURN(GenerateContainerAtIndexImpl(index, &spNewElement));
            }
            break;

        default:
            ASSERT(FALSE, L"Invalid type!");
            break;
        }

        // This will update the replaced element in both the realized range and the pinned collection
        IFC_RETURN(m_containerManager.OnDataItemReplaced(type, index, spNewElement.Get()));

        SetBoundsForElement(spNewElement, oldElementBounds);

        switch(type)
        {
        case xaml_controls::ElementType_GroupHeader:
            {
                wf::Size measureSize;
                IFC_RETURN(RecycleHeaderImpl(spOldElement.Get()));
                IFC_RETURN(m_spLayoutStrategy->GetElementMeasureSize(xaml_controls::ElementType_GroupHeader, index, m_windowState.GetRealizationWindow(), &measureSize));
                IFC_RETURN(PrepareHeaderViaItemsHost(index, measureSize, spNewElement))
            }

            // Protected by Apiset : TRUE only for Vertical Orientation + Headerplacement!=Left
            if (m_bUseStickyHeaders)
            {
                // The best positioning we can do here is the replaced header (it will be updated later if incorrect)
                IFC_RETURN(ConfigureStickyHeader(spNewElement, index, GetBoundsFromElement(spOldElement)));
            }
            break;

        case xaml_controls::ElementType_ItemContainer:
            {
                wf::Size measureSize;
                IFC_RETURN(RecycleContainerImpl(spOldElement.Get()));
                IFC_RETURN(m_spLayoutStrategy->GetElementMeasureSize(xaml_controls::ElementType_ItemContainer, index, m_windowState.GetRealizationWindow(), &measureSize));
                IFC_RETURN(PrepareContainerViaItemsHost(index, measureSize, spNewElement));
            }
            break;

        default:
            ASSERT(FALSE, L"Invalid type!");
            break;
        }

        // Reregister the focus/pin state
        if (wasFocused)
        {
            IFC_RETURN(m_containerManager.SetFocusedContainer(index, spNewElement));
        }
        if (wasPinned)
        {
            // Element could already be pinned by PrepareViaItemsHost, so check again
            if (!m_containerManager.GetIsIndexPinned(type, index))
            {
                IFC_RETURN(m_containerManager.RegisterPinnedElement(type, index, spNewElement));
            }
        }
    }

    if (type == xaml_controls::ElementType_GroupHeader)
    {
        HRESULT hr = S_OK;
        auto strongCache = m_cacheManager.CacheStrongRefs(&hr);
        IFC_RETURN(hr);
        IFC_RETURN(CreateUnlinkedFillersForGroup(index));
    }

    IFC_RETURN(InvalidateMeasure());

    return S_OK;
}

// naming confusion: for long we have shipped with a Refresh that did the work of what I would consider
// a reset. This has seeped into public api, and therefore I can't rectify that. So understand these definitions:
// *******    Refresh:   destructive, time consuming
// *******    Reset:     maintain state as much as possible. We can't trust our concept of indices anymore and want to redo them
_Check_return_ HRESULT ModernCollectionBasePanel::OnCollectionReset()
{
    HRESULT hr = S_OK;

    // we shall call this method from OnItemsReset

    auto spScrollViewer = m_wrScrollViewer.AsOrNull<IScrollViewer>();
    INT containerIndexToStartGeneratingFrom = -1;
    INT headerIndexToStartGeneratingFrom = -1;
    DOUBLE offsetFromTop = 0.0;

    m_inCollectionChange = true;
    if (m_layoutInProgress)
    {
        TraceGuardFailure(L"ResetDuringLayout");
    }

    // logic during refresh:
    // 1. store items from pinnedcontainers and first visible container
    // 2. iterate over them and ask for new indices
    // 3. recycle elements (raising CCC) including pinnedcontainers, but do not clear out visual tree
    // 4. reset our caches
    // 5. start generation, but make sure we use the correct containers for the correct indices


    // we want to keep track of pinned containers, focus container and first visible container

    // re-evaluate

    // no longer trust this
    IGNOREHR(m_cacheManager.ResetGroupCache());

    if (IsItemsHostRegistered())
    {
        auto strongCache = m_cacheManager.CacheStrongRefs(&hr); // Releases when it goes out of scope at the end of block
        IFC(hr);

        // getting to our first visible container

        // if we ever get a a new elementtype, we probably need to update this code
        ASSERT(ElementType_Count == 2);
        for (int typeindex = 0; typeindex < ElementType_Count && containerIndexToStartGeneratingFrom == -1; ++typeindex)
        {
            xaml_controls::ElementType elementType = static_cast<xaml_controls::ElementType>(typeindex);

            wf::Rect adjustedWindowForStickyHeaders = m_windowState.GetVisibleWindow();
            if (elementType == xaml_controls::ElementType::ElementType_ItemContainer && m_bUseStickyHeaders)
            {
                adjustedWindowForStickyHeaders.Y += m_lastVisibleWindowClippingHeight;
                adjustedWindowForStickyHeaders.Height -= m_lastVisibleWindowClippingHeight;
            }

            // we also care about the first visible container or (if no containers available) header
            // ordering is always correct (since we are sorted by dataindex and our layouts are supposed to be in order)
            // this means that the first container we find is the only one we care about
            for (int validIndex = 0; validIndex < m_containerManager.GetValidElementCount(elementType); ++validIndex)
            {
                wf::Rect bounds = {};
                ctl::ComPtr<IUIElement> spElement;

                IFC(m_containerManager.GetElementAtValidIndex(elementType, validIndex, &spElement));

                // Ignores any sentinel.
                if (GetElementIsSentinel(spElement))
                {
                    continue;
                }

                bounds = GetBoundsFromElement(spElement);
                // We don't want elements to overlap over one line.

                bounds.*SizeFromRectInVirtualizingDirection() -= EdgeOverlayDisambiguationDelta;

                if (!RectUtil::AreDisjoint(bounds, adjustedWindowForStickyHeaders))
                {
                    bool isGroupHeader = false;
                    bool found = false;
                    unsigned int newItemIndex = 0;
                    ctl::ComPtr<IInspectable> spDataItem;

                    IFC(m_containerManager.Interface_DataItemFromElement(elementType, spElement.AsOrNull<IDependencyObject>().Get(), &spDataItem));

                    IFC(m_cacheManager.GetIndex(spDataItem.Get(), &newItemIndex, &isGroupHeader, &found));
                    if (found)
                    {
                        // we can only do the scroll if we find a new index. If this did not happen, we're out of luck
                        if (isGroupHeader)
                        {
                            headerIndexToStartGeneratingFrom = newItemIndex;
                        }
                        else
                        {
                            containerIndexToStartGeneratingFrom = newItemIndex;
                        }
                        offsetFromTop = adjustedWindowForStickyHeaders.*PointFromRectInVirtualizingDirection() - bounds.*PointFromRectInVirtualizingDirection();
                        break;
                    }
                }
            }
        }

        // We should not be tracking anything at this stage. Note that
        // we need to reset the tracked element before recycling all the containers.
        m_viewportBehavior.Reset();

        // great. we have scribbled away the first visible container index
        // now recycle all, note how this will conveniently save the focused container for us during recycling so the
        // ordering is important here (this comes before m_containerManager.Reset)
        IFC(RecycleAllContainersAndHeaders());

        // tell containerManager to reset. This will tell him to update all the indices of both the focused and the pinned elements
        IFC(m_containerManager.Reset());

        // also clear out any tracked element for orientation
        m_tpFirstVisibleElementBeforeOrientationChange.Clear();
        m_typeOfFirstVisibleElementBeforeOrientationChange = xaml_controls::ElementType_ItemContainer;
        m_indexOfFirstVisibleElementBeforeOrientationChange = -1;


        // let's start with the right container
        if (spScrollViewer)
        {

            if (containerIndexToStartGeneratingFrom > -1)
            {
                IFC(ScrollItemIntoView(
                    containerIndexToStartGeneratingFrom,
                    xaml_controls::ScrollIntoViewAlignment_Leading,
                    offsetFromTop,
                    false /* forceSynchronous */));
            }
            else if (headerIndexToStartGeneratingFrom > -1)
            {
                IFC(ScrollGroupHeaderIntoView(
                    headerIndexToStartGeneratingFrom,
                    xaml_controls::ScrollIntoViewAlignment_Leading,
                    offsetFromTop,
                    false /* forceSynchronous */));
            }
            else
            {
                // For now, we assume we aren't in a nested SV.
                ctl::ComPtr<IInspectable> spHorizontalOffset;
                ctl::ComPtr<IInspectable> spVerticalOffset;
                ctl::ComPtr<wf::IReference<DOUBLE>> spHorizontalOffsetReference;
                ctl::ComPtr<wf::IReference<DOUBLE>> spVerticalOffsetReference;
                BOOLEAN handled;

                IFC(PropertyValue::CreateFromDouble(0, &spHorizontalOffset));
                IFC(spHorizontalOffset.As(&spHorizontalOffsetReference));
                IFC(PropertyValue::CreateFromDouble(0, &spVerticalOffset));
                IFC(spVerticalOffset.As(&spVerticalOffsetReference));

                IFC(spScrollViewer->ChangeViewWithOptionalAnimation(
                    spHorizontalOffsetReference.Get(),
                    spVerticalOffsetReference.Get(),
                    nullptr /*zoom */,
                    TRUE /*disable animation */,
                    &handled));
            }
        }

        IFC(UpdateGroupStyle());
    }

    IFC(InvalidateMeasure());

Cleanup:
    m_inCollectionChange = false;
    RRETURN(hr);
}

// When groups are inserted, we get subsequent item insertions, but they arrive in reverse order!
// This could be ugly if we don't correct for it.
// For instance, if we are looking at group 0, items 0-10, and a new group comes in with 10 items,
// we'll see a group header come in, but then we'll see an insertion at index 10, which will look to us
// like we're inserting something right before the old index 10. Things will get messed up.
_Check_return_ HRESULT ModernCollectionBasePanel::CreateUnlinkedFillersForGroup(_In_ INT nIndex)
{
    HRESULT hr = S_OK;

    INT32 startItemIndex = -1;
    INT32 countInGroup = -1;        // fill them up again

    // get the groupsize of the group just inserted, this allows us to insert dummies
    IFC(m_cacheManager.GetGroupInformationFromGroupIndex(nIndex, &startItemIndex, &countInGroup));

    m_containerManager.m_unlinkedFillers.clear();
    m_containerManager.m_unlinkedFillers.reserve(countInGroup);

    while(countInGroup > 0)
    {
        m_containerManager.m_unlinkedFillers.push_back(startItemIndex);
        ++startItemIndex;
        --countInGroup;
    }

Cleanup:
    RRETURN(hr);
}

void
ModernCollectionBasePanel::DeferredModernPanelUnlinkActionPayload::InitializePayload(_In_ ModernCollectionBasePanel* const pOwner)
{
    SetPtrValue(m_tpOwner, pOwner);
}

_Check_return_ HRESULT
ModernCollectionBasePanel::DeferredModernPanelUnlinkActionPayload::Execute(_In_ UIElement* pContainer)
{
    // unloading queue is needed only for callback from transition completion routine.
    RRETURN(m_tpOwner->FinishUnlinkingContainer(pContainer, TRUE /* useUnloadingQueue */));
}

// Callback for finishing unlinking from DeferredModernPanelUnlinkActionPayload when layout transition is complete.
// Transition completion routine should use unloaded queue to avoid adding elements to the visual tree after the Layout completes.
_Check_return_ HRESULT ModernCollectionBasePanel::FinishUnlinkingContainer(_In_ UIElement* pContainer, _In_ BOOLEAN useUnloadingQueue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spContainer;

    IFC(RecycleContainer(pContainer));

    // only add back to the tree if we actually put this guy into the recycle queue

    {
        // use the implicit knowledge that we will have pushed to the back
        auto it = std::find_if(m_recycleQueue.rbegin(), m_recycleQueue.rend(),
            [&spContainer, &pContainer](ctl::WeakRefPtr& arg)
        {
            spContainer = arg.AsOrNull<IUIElement>();
            return spContainer.Get() == pContainer;
        });

        if (it != m_recycleQueue.rend())
        {
            if (useUnloadingQueue)
            {
                IFC(m_tpUnloadedElements->Append(pContainer));
            }
            else
            {
                ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
                IFC(m_cacheManager.GetChildren(&spChildren));
                IFC(spChildren->Append(pContainer));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::FindRecyclingCandidateImpl(
    _In_ INT index,
    _Out_ BOOLEAN* pHasMatchingCandidate)
{
    HRESULT hr = S_OK;
    BOOLEAN hasMatchingCandidate = FALSE;

    // If queue is empty, no need to go further
    if (m_recycleQueue.size() > 0)
    {
        ctl::ComPtr<IContainerRecyclingContext> spRecyclingContext;
        IFC(m_cacheManager.GetContainerRecyclingContext(index, &spRecyclingContext));

        if (!spRecyclingContext)
        {
            // if a candidate was previously rejected (suggested in the ChoosingItemContainer event but
            // not used by the app) we should not propose it again. In those cases they just stay alive in the
            // recycle queue. This means we will try to recycle another container. That is great and we will make
            // sure that that is the one that is suggested to the app. However, if we cannot find another candidate to
            // recycle, since the queue is not empty, we will just suggest a container from the queue and that is
            // perfect.
            ctl::ComPtr<IUIElement> spContainer;

            hasMatchingCandidate = FALSE;

            auto it = std::find_if(m_recycleQueue.rbegin(), m_recycleQueue.rend(),
                [&spContainer](ctl::WeakRefPtr& arg)
            {
                BOOLEAN denied = FALSE;
                spContainer = arg.AsOrNull<IUIElement>();
                if (spContainer)
                {
                    auto p_virtualizationInformation = GetVirtualizationInformationFromElement(spContainer);
                    denied = p_virtualizationInformation->GetWasRejectedAsAContainerByApp();

                }
                return !denied;
            });

            if (it != m_recycleQueue.rend())
            {
                hasMatchingCandidate = TRUE;
            }

        }
        else
        {
            ctl::ComPtr<IUIElement> spContainer;
            auto it = std::find_if(m_recycleQueue.rbegin(), m_recycleQueue.rend(),
                [spRecyclingContext, &spContainer](ctl::WeakRefPtr& arg)
            {
                BOOLEAN compatible = FALSE;
                spContainer = arg.AsOrNull<IUIElement>();
                if (spContainer)
                {
                    VERIFYHR(spRecyclingContext->IsCompatible(spContainer.Get(), &compatible));
                }
                return compatible;
            });

            if (it != m_recycleQueue.rend())
            {
                IFC(spRecyclingContext->put_SelectedContainer(spContainer.Get()));
                hasMatchingCandidate = TRUE;
            }
        }
    }

    *pHasMatchingCandidate = hasMatchingCandidate;
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::GetQueueLengthImpl(_Out_ UINT* returnValue)
{
    // returnValue has been verified in GetQueueLength
    *returnValue = m_recycleQueue.size();
    return S_OK;
}

_Check_return_ HRESULT ModernCollectionBasePanel::RemoveFromQueue(_In_ xaml::IUIElement* pContainer, _In_ bool expectContainerToBePresent)
{
    HRESULT hr = S_OK;
    BOOL found = FALSE;
    if (m_recycleQueue.size() > 0)
    {
        auto it = std::find_if(m_recycleQueue.rbegin(),
                               m_recycleQueue.rend(),
                               [pContainer](ctl::WeakRefPtr& arg) { return arg.AsOrNull<IUIElement>().Get() == pContainer; });

        if (it != m_recycleQueue.rend())
        {
            // Found it
            m_recycleQueue.erase(--it.base());
            found = TRUE;
        }
    }

    ASSERT(!expectContainerToBePresent || found);

    RRETURN(hr);//RRETURN_REMOVAL
}

_Check_return_ HRESULT ModernCollectionBasePanel::RemoveFromHeaderRecycleQueue(_In_ xaml::IUIElement* pContainer, _Out_ BOOLEAN* wasInQueue)
{
    HRESULT hr = S_OK;
    *wasInQueue = FALSE;
    if (m_recycleHeaderQueue.size() > 0)
    {
        auto it = std::find_if(m_recycleHeaderQueue.rbegin(),
            m_recycleHeaderQueue.rend(),
            [pContainer](ctl::WeakRefPtr& arg) { return arg.AsOrNull<IUIElement>().Get() == pContainer; });

        if (it != m_recycleHeaderQueue.rend())
        {
            // Found it
            m_recycleHeaderQueue.erase(--it.base());
            *wasInQueue = TRUE;
        }
    }

    RRETURN(hr);//RRETURN_REMOVAL
}
