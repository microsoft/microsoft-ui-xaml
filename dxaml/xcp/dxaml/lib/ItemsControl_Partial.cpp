// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ItemsControl.g.h"
#include "ItemsControlAutomationPeer.g.h"
#include "ItemCollection.g.h"
#include "ModernCollectionBasePanel.g.h"
#include "ItemContainerGenerator.g.h"
#include "GroupItem.g.h"
#include "VirtualizingPanel.g.h"
#include "ContentPresenter.g.h"
#include "ContentControl.g.h"
#include "DisplayMemberTemplate.g.h"
#include "VirtualizingStackPanel.g.h"
#include "ItemsPresenter.g.h"
#include "SelectorItem.g.h"
#include "PropertyChangedParamsHelper.h"
#include <DeferredElementStateChange.h>
#include "VisualTreeHelper.h"
#include "ValidationErrorsObservableVectorWrapper.h"

//uncomment to get transition context output
//#define TRANSITION_CONTEXT_DBG

using namespace DirectUI;
using namespace DirectUISynonyms;

using namespace xaml_animation;

ItemsControl::ItemsControl()
    : m_elementCountAddedThisLayoutTick(0)
    , m_elementCountRemovedThisLayoutTick(0)
    , m_elementCountReorderedThisLayoutTick(0)
    , m_resetItemsThisLayoutTick(FALSE)
    , m_loadedTick(loadCounterToken)
    , m_previousTickCounterId(0)
    , m_themeTransitionContext(ThemeTransitionContext::Entrance)
    , m_isVirtualizingPropertySetByPanel(FALSE)
    , m_isGrouping(FALSE)
    , m_dataSelectorRecyclingContext(this)
{
    memset(&m_ItemsChangedToken, 0, sizeof(EventRegistrationToken));
    memset(&m_GroupStyleChangedToken, 0, sizeof(EventRegistrationToken));
}

ItemsControl::~ItemsControl()
{
    m_tpItems.Clear();

    auto spGenerator = m_tpGenerator.GetSafeReference();
    if (spGenerator)
    {
        IGNOREHR(spGenerator->remove_ItemsChanged(m_ItemsChangedToken));
    }

    if (auto peg = m_tpGroupStyle.TryMakeAutoPeg())
    {
        IGNOREHR(m_tpGroupStyle->remove_VectorChanged(m_GroupStyleChangedToken));
        m_tpGroupStyle.Clear();
    }

    m_tpDisplayMemberTemplate.Clear();
}

// Prepares object's state.
_Check_return_ HRESULT ItemsControl::PrepareState()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ObservableTrackerCollection<xaml_controls::GroupStyle*>> spGroupStyle;

    IFC(ItemsControlGenerated::PrepareState());

    IFC(ctl::make(&spGroupStyle));
    SetPtrValue(m_tpGroupStyle, spGroupStyle);
    IFC(SetValueByKnownIndex(KnownPropertyIndex::ItemsControl_GroupStyle, ctl::iinspectable_cast(spGroupStyle.Get())));

Cleanup:
    RRETURN(hr);
}

// Supports the IGeneratorHost interface.
_Check_return_
    HRESULT
    ItemsControl::QueryInterfaceImpl(
    _In_ REFIID iid,
    _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(IGeneratorHost)))
    {
        *ppObject = static_cast<IGeneratorHost*>(this);
    }
    else
    {
        RRETURN(ItemsControlGenerated::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

_Check_return_ HRESULT ItemsControl::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spOldValue;
    ctl::ComPtr<IInspectable> spNewValue;

    IFC(ItemsControlGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::ItemsControl_ItemsSource:
        {
            IFC(PropertyChangedParamsHelper::GetObjects(args, &spOldValue, &spNewValue));

            // Double check that the value really changed. Eventually we want to make sure that CDependencyObject::NotifyPropertyChanged
            // properly checks whether two EORs/IInspectables are equal.
            bool areEqual = false;
            IFC(PropertyValue::AreEqual(spOldValue.Get(), spNewValue.Get(), &areEqual));
            if (!areEqual)
            {
                IFC(OnItemsSourceChanged(spNewValue.Get()));
            }

            ItemsControl::TraceVirtualizationEnabledByModernPanel();
        }
        break;
    case KnownPropertyIndex::ItemsControl_ItemContainerStyle:
        {
            IFC(PropertyChangedParamsHelper::GetObjects(args, &spOldValue, &spNewValue));

            // Double check that the value really changed. Eventually we want to make sure that CDependencyObject::NotifyPropertyChanged
            // properly checks whether two EORs/IInspectables are equal.
            bool areEqual = false;
            IFC(PropertyValue::AreEqual(spOldValue.Get(), spNewValue.Get(), &areEqual));
            if (!areEqual)
            {
                IFC(OnItemContainerStyleChanged(spOldValue.Get(), spNewValue.Get()));
            }
        }
        break;
    case KnownPropertyIndex::ItemsControl_ItemContainerStyleSelector:
        {
            IFC(PropertyChangedParamsHelper::GetObjects(args, &spOldValue, &spNewValue));

            // Double check that the value really changed. Eventually we want to make sure that CDependencyObject::NotifyPropertyChanged
            // properly checks whether two EORs/IInspectables are equal.
            bool areEqual = false;
            IFC(PropertyValue::AreEqual(spOldValue.Get(), spNewValue.Get(), &areEqual));
            if (!areEqual)
            {
                IFC(OnItemContainerStyleSelectorChanged(spOldValue.Get(), spNewValue.Get()));
            }
        }
        break;
    case KnownPropertyIndex::ItemsControl_ItemTemplate:
        IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, /* pTargetType */ nullptr, &spOldValue));
        IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, /* pTargetType */ nullptr, &spNewValue));
        IFC(OnItemTemplateChanged(spOldValue.Get(), spNewValue.Get()));
        break;
    case KnownPropertyIndex::ItemsControl_ItemTemplateSelector:
        {
            IFC(PropertyChangedParamsHelper::GetObjects(args, &spOldValue, &spNewValue));

            // Double check that the value really changed. Eventually we want to make sure that CDependencyObject::NotifyPropertyChanged
            // properly checks whether two EORs/IInspectables are equal.
            bool areEqual = false;
            IFC(PropertyValue::AreEqual(spOldValue.Get(), spNewValue.Get(), &areEqual));
            if (!areEqual)
            {
                IFC(OnItemTemplateSelectorChanged(spOldValue.Get(), spNewValue.Get()));
            }
        }
        break;
    case KnownPropertyIndex::ItemsControl_GroupStyleSelector:
        IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, /* pTargetType */ nullptr, &spOldValue));
        IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, /* pTargetType */ nullptr, &spNewValue));
        IFC(OnGroupStyleSelectorChanged(spOldValue.Get(), spNewValue.Get()));
        break;
    case KnownPropertyIndex::ItemsControl_ItemsPanel:
        IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, /* pTargetType */ nullptr, &spOldValue));
        IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, /* pTargetType */ nullptr, &spNewValue));
        IFC(OnItemsPanelChanged(spOldValue.Get(), spNewValue.Get()));
        ItemsControl::TraceVirtualizationEnabledByModernPanel();
        break;
    case KnownPropertyIndex::ItemsControl_ItemsHost:
        ItemsControl::TraceVirtualizationEnabledByModernPanel();
        break;
    }

Cleanup:
    RRETURN(hr);
}

//  When the ItemsSourceProperty is set or cleared, we update the internal _itemsSourceAsList,
//  and notify the core ItemsControl.
_Check_return_
    HRESULT
    ItemsControl::OnItemsSourceChanged(
    _In_ IInspectable* pNewValue)
{
    ctl::ComPtr<wfc::IIterable<IInspectable*>> spSource;
    ctl::ComPtr<IInspectable> newValue(pNewValue);
    // Give all the existing visual children a chance to clean up in ClearContainerForItemOverride().
    IFC_RETURN(ClearContainers(/*bHostIsReplaced*/ FALSE));

    if (auto spBindableIterable = newValue.AsOrNull<IBindableIterable>())
    {
        spSource.Attach(reinterpret_cast<wfc::IIterable<IInspectable *>*>(spBindableIterable.Detach()));
    }
    else if (auto validationErrorsCollection = newValue.AsOrNull<ValidationErrorsCollection>())
    {
        // We want to create our own special wrapper here because we don't want to get an IBindableIterable
        // back from the CLR. This ensures that ItemsSource="{Binding Path=(Validation.Errors)}" works the
        // same in native apps and the CLR.
        spSource = ValidationErrorsObservableVectorWrapper::CreateInstance(validationErrorsCollection.Get());
    }
    else
    {
        spSource = newValue.AsOrNull<wfc::IIterable<IInspectable*>>();
    }

    // If we still haven't been able to get a usable source, and this is a CLR application, try to get
    // a wrapper around the input collection.
    if( !spSource && pNewValue != nullptr )
    {
        ctl::ComPtr<IInspectable> spWrapper;
        spWrapper.Attach(ReferenceTrackerManager::GetTrackerTarget(pNewValue));
        if( spWrapper )
        {
            if (auto spBindableIterable = spWrapper.AsOrNull<IBindableIterable>())
            {
                spSource.Attach(reinterpret_cast<wfc::IIterable<IInspectable *>*>(spBindableIterable.Detach()));
            }
        }
    }

    // Update ItemCollection with new ItemsSource reference.
    // May be a null, which means clearing. But must be a IEnumerable derived instance.

    if(pNewValue != nullptr && !spSource)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
    IFC_RETURN(get_Items(&spItems));
    IFC_RETURN(spItems.Cast<ItemCollection>()->UpdateItemsSourceList(spSource.Get()));

    return S_OK;
}

_Check_return_
    HRESULT
    ItemsControl::SetItemCollectionStatic(
    _In_ CItemCollection* pNativeItemCollection,
    _In_ CItemsControl* pNativeItemsControl)
{
    ctl::ComPtr<ItemCollection> spItemCollection;
    ctl::ComPtr<ItemsControl> spItemsControl;

    ctl::ComPtr<DependencyObject> spTemp;

    IFCEXPECT_RETURN(pNativeItemCollection);
    IFCEXPECT_RETURN(pNativeItemsControl);

    // Get the peers
    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFC_RETURN(pCore->GetPeer(pNativeItemCollection, &spTemp));
    spItemCollection = spTemp.Cast<ItemCollection>();
    IFCEXPECT_RETURN(spItemCollection);
    spTemp.Reset();

    IFC_RETURN(pCore->GetPeer(pNativeItemsControl, &spTemp));
    spItemsControl = spTemp.Cast<ItemsControl>();
    IFCEXPECT_RETURN(spItemsControl);
    IFC_RETURN(spItemCollection->Init(spItemsControl.Get()));
    spTemp.Reset();

    IFC_RETURN(spItemsControl->SetItemCollection(spItemCollection.Get()));

    return S_OK;
}

_Check_return_
    HRESULT
    ItemsControl::SetItemCollection( _In_ DirectUI::ItemCollection* pItemCollection )
{
    HRESULT hr = S_OK;

    ctl::ComPtr<wfc::VectorChangedEventHandler<xaml_controls::GroupStyle*>> spGroupStyleVectorChangedHandler;

    SetPtrValue(m_tpItems, pItemCollection);

    // this tells old style grouping to hook into groupeddatacollection
    IFC(InitializeItemContainerGenerator());

    IFC(m_epItemCollectionVectorChangedHandler.AttachEventHandler(m_tpItems.Get(),
        [this] (wfc::IObservableVector<IInspectable*>* pSender, wfc::IVectorChangedEventArgs* pArgs)
    {
        return this->OnItemsChangedProtected(pArgs);
    }));

    spGroupStyleVectorChangedHandler.Attach(
        new ClassMemberEventHandler<
        ItemsControl,
        IItemsControl,
        wfc::VectorChangedEventHandler<xaml_controls::GroupStyle*>,
        wfc::IObservableVector<xaml_controls::GroupStyle*>,
        wfc::IVectorChangedEventArgs>(this, &ItemsControl::OnGroupStyleChanged));

    IFC(m_tpGroupStyle->add_VectorChanged(spGroupStyleVectorChangedHandler.Get(), &m_GroupStyleChangedToken));

Cleanup:
    RRETURN(hr);
}

// If not using a modern panel, create an ICG appropriate for the grouping scenario
// and hook up its event handlers.
// If using a modern panel, we null out the ICG and store an IICM pointer
_Check_return_ HRESULT ItemsControl::InitializeItemContainerGenerator()
{
    HRESULT hr = S_OK;

    if (m_tpItems)
    {
        ctl::ComPtr<IPanel> spItemsHost;
        ctl::ComPtr<ICustomGeneratorItemsHost> spCustomItemsHost;

        BOOLEAN itemsHostInvalid = FALSE;
        IFC(get_ItemsHost(&spItemsHost));
        IFC(get_IsItemsHostInvalid(&itemsHostInvalid));
        spCustomItemsHost = spItemsHost.AsOrNull<ICustomGeneratorItemsHost>();

        // We should only be doing this work if we don't already have an ICG,
        // or if a ModernCollectionPanel is being swapped in or out
        ASSERT(!m_tpGenerator || spCustomItemsHost);

        if (spCustomItemsHost && !itemsHostInvalid)
        {
            // Our panel is going to do its thing implementing IICM.
            // Set the IICM property and ensure the IICG property is null
            ctl::ComPtr<IItemContainerMapping> spMapping;
            ctl::ComPtr<IGroupHeaderMapping> spGroupMapping;

            IFC(spCustomItemsHost->GetItemContainerMapping(&spMapping));
            SetPtrValue(m_tpMapping, spMapping);

            IFC(spCustomItemsHost->GetGroupHeaderMapping(&spGroupMapping));
            SetPtrValue(m_tpGroupMapping, spGroupMapping);

            IFC(spCustomItemsHost->RegisterItemsHost(this));

            if (m_tpGenerator)
            {
                IFC(m_tpGenerator->remove_ItemsChanged(m_ItemsChangedToken));
                m_ItemsChangedToken = EventRegistrationToken();
                IFC(m_tpGenerator.Cast<ItemContainerGenerator>()->DetachHost());
                m_tpGenerator.Clear();
            }
        }
        else if (!m_tpGenerator)
        {
            // We are not using a modern panel, so hook up all that plumbing
            ctl::ComPtr<DependencyObject> spTemplatedParent;
            ctl::ComPtr<IGroupItem> spGroupItem;
            ctl::ComPtr<ItemContainerGenerator> spGenerator;
            ctl::ComPtr<IItemsChangedEventHandler> spGeneratorItemsChangedHandler;

            IFC(get_TemplatedParent(&spTemplatedParent));
            spGroupItem = spTemplatedParent.AsOrNull<IGroupItem>();
            if (!spGroupItem ||
                !(spGroupItem.Cast<GroupItem>()->m_tpGenerator))
            {
                IFC(ItemContainerGenerator::CreateGenerator(this, &spGenerator));
                SetPtrValue(m_tpGenerator, spGenerator);
            }
            else
            {
                SetPtrValue(m_tpGenerator, spGroupItem.Cast<GroupItem>()->m_tpGenerator.Get());
            }
            SetPtrValueWithQI(m_tpMapping, m_tpGenerator.Get());
            m_tpGroupMapping.Clear();

            spGeneratorItemsChangedHandler.Attach(
                new ClassMemberEventHandler<
                ItemsControl,
                IItemsControl,
                IItemsChangedEventHandler,
                IInspectable,
                IItemsChangedEventArgs>(this, &ItemsControl::OnGeneratorItemsChanged));

            IFC(m_tpGenerator->add_ItemsChanged(spGeneratorItemsChangedHandler.Get(), &m_ItemsChangedToken));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControl::GetItemContainerMapping(_Outptr_ IItemContainerMapping** ppReturnValue)
{
    RRETURN(m_tpMapping.CopyTo(ppReturnValue));
}

_Check_return_ HRESULT ItemsControl::GetGroupHeaderMapping(_Outptr_ DirectUI::IGroupHeaderMapping** ppReturnValue)
{
    RRETURN(m_tpGroupMapping.CopyTo(ppReturnValue));
}

_Check_return_
    HRESULT
    ItemsControl::ClearVisualChildren(
    _In_ CItemsControl* pNativeItemsControl,
    _In_ bool bHostIsReplaced)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ItemsControl> spItemsControl;

    ctl::ComPtr<DependencyObject> spTemp;

    IFCEXPECT(pNativeItemsControl);

    // Get the peer
    IFC(DXamlCore::GetCurrent()->GetPeer(pNativeItemsControl, &spTemp));
    spItemsControl = spTemp.Cast<ItemsControl>();
    IFCEXPECT(spItemsControl);
    spTemp.Reset();

    IFC(spItemsControl->ClearContainers(!!bHostIsReplaced));

Cleanup:
    RRETURN(hr);
}

_Check_return_
    HRESULT
    ItemsControl::DisplayMemberPathChanged(
    _In_ CItemsControl* pNativeItemsControl)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ItemsControl> spItemsControl;

    ctl::ComPtr<DependencyObject> spTemp;

    IFCEXPECT(pNativeItemsControl);

    // Get the peer
    IFC(DXamlCore::GetCurrent()->GetPeer(pNativeItemsControl, &spTemp));
    spItemsControl = spTemp.Cast<ItemsControl>();
    IFCEXPECT(spItemsControl);
    spTemp.Reset();

    spItemsControl->m_tpDisplayMemberTemplate.Clear();

Cleanup:
    RRETURN(hr);
}

_Check_return_
    HRESULT
    ItemsControl::RecreateVisualChildren(
    _In_ CItemsControl* pNativeItemsControl)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ItemsControl> spItemsControl;

    ctl::ComPtr<DependencyObject> spTemp;

    IFCEXPECT(pNativeItemsControl);

    // Get the peer
    IFC(DXamlCore::GetCurrent()->GetPeer(pNativeItemsControl, &spTemp));
    spItemsControl = spTemp.Cast<ItemsControl>();
    IFCEXPECT(spItemsControl);
    spTemp.Reset();

    spItemsControl->m_tpItemsHost.Clear();
    IFC(spItemsControl->AddContainers());

Cleanup:
    RRETURN(hr);
}

_Check_return_
    HRESULT
    ItemsControl::NotifyAllItemsAdded(
    _In_ CItemsControl* pNativeItemsControl)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ItemsControl> spItemsControl;

    ctl::ComPtr<DependencyObject> spTemp;

    IFCEXPECT(pNativeItemsControl);

    // Get the peer
    IFC(DXamlCore::GetCurrent()->GetPeer(pNativeItemsControl, &spTemp));
    spItemsControl = spTemp.Cast<ItemsControl>();
    IFCEXPECT(spItemsControl);
    spTemp.Reset();

    IFCEXPECT(spItemsControl->m_tpItems);
    IFC(spItemsControl->m_tpItems->NotifyCollectionReady());

Cleanup:
    RRETURN(hr);
}

// This code gets called from Automation Provider e.g. ItemAutomationPeer and must not be called from any
// internal APIs in the control itself. It basically returns the Container for the DataItem in case it exist.
_Check_return_
    HRESULT
    ItemsControl::UIA_GetContainerForDataItemOverride(
    _In_opt_ IInspectable* pItem,
    _In_ INT itemIndex,
    _Outptr_ xaml::IUIElement** ppContainer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spContainerAsDO;
    BOOLEAN isOwnContainer = FALSE;

    *ppContainer = nullptr;

    if (pItem != nullptr)
    {
        IFC(IsItemItsOwnContainer(pItem, &isOwnContainer));

        if(isOwnContainer)
        {
            IFC(ctl::do_query_interface(*ppContainer, pItem));
        }
    }

    if (!isOwnContainer)
    {
        // TODO: once UIA is handling null items propertly by passing indices (bug 366772) call to UIA_GetContainerForDataItemOverride
        // should set the itemIndex parameter appropriately.  For now, to fix the drag and drop of null items scenario this
        // behavior is special cased and old code is executed (ContainerFromItem) by passing -1.

        if (itemIndex != -1)
        {
            IFC(ContainerFromIndex(itemIndex, &spContainerAsDO));
        }
        else
        {
            IFC(ContainerFromItem(pItem, &spContainerAsDO));
        }

        if(spContainerAsDO)
        {
            IFC(spContainerAsDO.CopyTo(ppContainer));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
    HRESULT
    ItemsControl::ClearContainers(
    _In_ BOOLEAN bHostIsReplaced)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spItemsHost;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    ctl::ComPtr<IVirtualizingPanel> spVirtualizingPanel;
    ctl::ComPtr<ICustomGeneratorItemsHost> spCustomItemsHost;

    IFC(put_IsItemsHostInvalid(TRUE));

    IFC(get_ItemsHost(&spItemsHost));

    if (!spItemsHost)
    {
        goto Cleanup;
    }

    spVirtualizingPanel = spItemsHost.AsOrNull<IVirtualizingPanel>();
    spCustomItemsHost = spItemsHost.AsOrNull<ICustomGeneratorItemsHost>();

    if (!spVirtualizingPanel && !spCustomItemsHost)
    {
        IFC(spItemsHost->get_Children(&spChildren));
        IFC(spChildren->Clear());
    }
    else
    {
        if (spCustomItemsHost)
        {
            // I believe that since our panel takes care of the ICG responsibilities, that both cases go hand in hand...
            IFC(spCustomItemsHost->Refresh());
            if (bHostIsReplaced)
            {
                IFC(spCustomItemsHost->DisconnectItemsHost());
                IFC(InitializeItemContainerGenerator());
            }
        }
        else
        {
            if (bHostIsReplaced)
            {
                IFC(spVirtualizingPanel.Cast<VirtualizingPanel>()->get_Children(&spChildren));
                IFC(spChildren->Clear());
                IFC(spVirtualizingPanel.Cast<VirtualizingPanel>()->OnClearChildrenInternal());
            }
            else
            {
                UINT nCount = 0;
                if (m_tpItems)
                {
                    IFC(m_tpItems->get_Size(&nCount));
                }
                if (nCount > 0)
                {
                    IFC(m_tpGenerator.Cast<ItemContainerGenerator>()->Refresh());
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
    HRESULT
    ItemsControl::AddContainers()
{
    HRESULT hr = S_OK;
    UINT nCount = 0;
    GeneratorPosition position = {-1, 0};
    ctl::ComPtr<xaml::IDependencyObject> spDO;
    ctl::ComPtr<IPanel> spItemsHost;
    ctl::ComPtr<IVirtualizingPanel> spVirtualizingItemsHost;
    ctl::ComPtr<IItemContainerGenerator> spItemContainerGenerator;
    BOOLEAN bItemsHostInvalid = FALSE;

    if (!m_tpItems)
    {
        goto Cleanup;
    }

    IFC(get_IsItemsHostInvalid(&bItemsHostInvalid));
    if (bItemsHostInvalid)
    {
        goto Cleanup;
    }

    IFC(get_ItemsHost(&spItemsHost));

    if (spItemsHost)
    {
        IFC(OnItemsHostAvailable());
    }


    if (!spItemsHost)
    {
        goto Cleanup;
    }

    spVirtualizingItemsHost = spItemsHost.AsOrNull<IVirtualizingPanel>();

    if (spVirtualizingItemsHost)
    {
        INT32 tick = 0;
        VirtualizingPanel* pVirtualizingPanel = spVirtualizingItemsHost.Cast<VirtualizingPanel>();
        IFC(CoreImports::LayoutManager_GetLayoutTickForTransition(static_cast<CUIElement*>(spItemsHost.Cast<Panel>()->GetHandle()), (XINT16*)&tick));
        pVirtualizingPanel->SetItemsHostValidatedTick(tick);    // communicate the tick in which this host got validated (first time on screen).
        goto Cleanup;
    }

    IFC(m_tpItems->get_Size(&nCount));
    if (nCount == 0)
    {
        goto Cleanup;
    }

    // Don't do this ICG work if our panel has a custom container generator.
    if (ctl::is<DirectUI::ICustomGeneratorItemsHost>(spItemsHost))
    {
        goto Cleanup;
    }

    IFC(get_ItemContainerGenerator(&spItemContainerGenerator));
    IFC(m_tpGenerator->StartAt(position, xaml_primitives::GeneratorDirection::GeneratorDirection_Forward, TRUE));

    for (UINT nIndex = 0; nIndex < nCount ; ++nIndex)
    {
        BOOLEAN isNewlyRealized = FALSE;
        IFC(m_tpGenerator->GenerateNext(&isNewlyRealized, &spDO));
        if (spDO)
        {
            IFC(AddVisualChild(nIndex, spDO.Get(), isNewlyRealized));
            spDO.Reset();
        }
        else
        {
            // reach end of CollectionViewGroup(s)
            break;
        }
    }

    IFC(m_tpGenerator->Stop());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControl::RefreshContainers()
{
    HRESULT hr = S_OK;

    if (m_tpGenerator)
    {
        IFC(m_tpGenerator.Cast<ItemContainerGenerator>()->Refresh());
    }
    else
    {
        ctl::ComPtr<IPanel> spItemsHost;
        IFC(get_ItemsHost(&spItemsHost));

        ctl::ComPtr<ICustomGeneratorItemsHost> spCustomItemsHost = spItemsHost.AsOrNull<ICustomGeneratorItemsHost>();
        if (spCustomItemsHost)
        {
            IFC(spCustomItemsHost->Refresh());
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControl::IsItemItsOwnContainerOverrideImpl(
    _In_ IInspectable* item,
    _Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;

    IFCEXPECT(returnValue);
    *returnValue = !!ctl::value_is<IUIElement>(item);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControl::GetContainerForItemOverrideImpl(
    _Outptr_ xaml::IDependencyObject** returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ContentPresenter> spContentPresenter;

    IFCEXPECT(returnValue);
    IFC(ctl::make(&spContentPresenter));

    IFC(spContentPresenter.MoveTo(returnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControl::GetHeaderForGroupOverrideImpl(_Outptr_ xaml::IDependencyObject** returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ContentControl> spContentControl;

    IFCEXPECT(returnValue);
    IFC(ctl::make(&spContentControl));

    IFC(spContentControl.MoveTo(returnValue));

Cleanup:
    RRETURN(hr);
}

// Cleans up the given container previously set up in PrepareContainerForItemOverride.
// Will remove ItemContainerStyle if the container is not internally generated, and had this style previously set.
_Check_return_ HRESULT ItemsControl::ClearContainerForItemOverrideImpl(
    _In_ xaml::IDependencyObject* element,
    _In_ IInspectable* item)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IDependencyObject> spContainer;
    ctl::ComPtr<IUIElement> spContainerAsUIE;
    DependencyObject* pElement = NULL;
    ContentControl* pContentControl = NULL;
    ContentPresenter* pContentPresenter = NULL;

    IFCPTR(element);

    spContainer = element;
    spContainerAsUIE = spContainer.AsOrNull<IUIElement>();
    if (spContainerAsUIE)
    {
        BOOLEAN isGeneratedContainer = spContainerAsUIE.Cast<UIElement>()->GetIsGeneratedContainer();

        // For perf, only clear the style if we didn't generate the container.
        // Since we own the container if we generated it, we can get away with this.
        if (!isGeneratedContainer)
        {
            BOOLEAN isStyleSetFromItemsControl = FALSE;
            ctl::ComPtr<IFrameworkElement> spContainerAsFE;

            IFC(spContainer.As<IFrameworkElement>(&spContainerAsFE));
            isStyleSetFromItemsControl = spContainerAsFE.Cast<FrameworkElement>()->GetIsStyleSetFromItemsControl();
            if (isStyleSetFromItemsControl)
            {
                // if Style was formerly set from ItemContainerStyle, clear it
                IFC(spContainerAsFE.Cast<FrameworkElement>()->ClearValue(
                    MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::FrameworkElement_Style)));
                spContainerAsFE.Cast<FrameworkElement>()->SetIsStyleSetFromItemsControl(FALSE);
            }
        }
        else if (ctl::is<IFrameworkElement>(item))
        {
            // In cases where we are virtualizing and recycling containers, and the items are
            // FrameworkElements but are not their own containers, we need to clear the parent
            // association of the item with the ContentPresenter/ContentControl in the
            // generated container before that item can be reinserted into a different
            // container, which is common when recycling.
            pElement = static_cast<DependencyObject*>(element);
            if (ctl::is<IContentPresenter>(pElement))
            {
                pContentPresenter = static_cast<ContentPresenter*>(pElement);
                IFC(pContentPresenter->put_ContentTemplate(NULL));
                IFC(pContentPresenter->put_ContentTemplateSelector(NULL));
            }
            else if (ctl::is<IContentControl>(pElement))
            {
                pContentControl = static_cast<ContentControl*>(pElement);
                IFC(pContentControl->put_ContentTemplate(NULL));
                IFC(pContentControl->put_ContentTemplateSelector(NULL));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ IFACEMETHODIMP
ItemsControl::ClearGroupContainerForGroup(
    _In_ xaml::IDependencyObject* pContainer,
    _In_opt_ xaml_data::ICollectionViewGroup* pItem )
{
    RRETURN(S_OK);
}

// For certain situations (such as drag/drop), the host may hang onto a container for an extended
// period of time. That particular container shouldn't ever be recycled as long as it's being used.
// This method asks whether or not the given container is eligible for recycling.
_Check_return_ IFACEMETHODIMP
ItemsControl::CanRecycleContainer(
    _In_ xaml::IDependencyObject* pContainer,
    _Out_ BOOLEAN* pCanRecycleContainer)
{
    *pCanRecycleContainer = TRUE;
    RRETURN(S_OK);
}

_Check_return_ IFACEMETHODIMP
ItemsControl::SuggestContainerForContainerFromItemLookup(
    _Outptr_ xaml::IDependencyObject** ppContainer)
{
    // itemscontrol has no clue
    *ppContainer = nullptr;
    RRETURN(S_OK);
}

_Check_return_ HRESULT ItemsControl::PrepareContainerForItemOverrideImpl(
    _In_ xaml::IDependencyObject* element,
    _In_ IInspectable* item)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spItemAsDO;
    DependencyObject* pElement = NULL;
    ContentControl* pContentControl = NULL;
    ContentPresenter* pContentPresenter = NULL;
    ctl::ComPtr<IDataTemplate> spItemTemplate;
    ctl::ComPtr<IDataTemplateSelector> spItemTemplateSelector;
    wrl_wrappers::HString strDisplayMemberPath;
    ctl::ComPtr<DisplayMemberTemplate> spDisplayMemberTemplate;
    bool skipExpensivePrepareWork = false;

    BOOLEAN deferredBehavior = FALSE;

    IFCPTR(element);
    spItemAsDO = ctl::query_interface_cast<xaml::IDependencyObject>(item);

    // ListViewBase allows us to defer the costly SetContent
    IFC(IsPrepareContainerForItemDeferred(element, &deferredBehavior));

    // pItemAsDO == element happens typically when you have containers specified directly in the content of XAML.  In this case we don't want to
    // mess with the content or the template, the user is driving things.
    // In either case, we want to set the ItemContainerStyle if
    // we have one and the user didn't set a local item style.
    if (spItemAsDO.Get() != element)
    {
        ctl::ComPtr<xaml::IDependencyObject> spContainer = element;
        ctl::ComPtr<xaml::IUIElement> spContainerAsUIE = spContainer.AsOrNull<xaml::IUIElement>();
        VirtualizationInformation *pVirtualizationInformation = spContainerAsUIE ? spContainerAsUIE.Cast<UIElement>()->GetVirtualizationInformation() : nullptr;
        skipExpensivePrepareWork = pVirtualizationInformation != nullptr && pVirtualizationInformation->GetWantsToSkipContainerPreparation();

        pElement = static_cast<DependencyObject*>(element);

        if (!ctl::is<IContentPresenter>(pElement))
        {
            if (!ctl::is<IContentControl>(pElement))
            {
                // We only know how to initialize ContentPresenters and ContentControls
                goto Cleanup;
            }
            else
            {
                pContentControl = static_cast<ContentControl*>(pElement);
            }
        }
        else
        {
            pContentPresenter = static_cast<ContentPresenter*>(pElement);
        }

        IFC(get_ItemTemplate(&spItemTemplate));
        IFC(get_ItemTemplateSelector(&spItemTemplateSelector));
        IFC(get_DisplayMemberPath(strDisplayMemberPath.GetAddressOf()));

        // TODO: Report Error Resx.ItemsControls_CannotSetItemTemplateAndDisplayMemberPathSimultaneously
        // Bug#95997
        IFCEXPECT(!(spItemTemplate || spItemTemplateSelector) || !strDisplayMemberPath.Get());

        // In this case there is no DataTemplate the item itself will simply be added to the visual tree.
        if (!ctl::is<IUIElement>(spItemAsDO) && !spItemTemplate && !spItemTemplateSelector)
        {
            // All the content presenters can share a single copy of this default template.
            // This hardcoded template will expand to a TextBlock with a databinding to the specified path.
            if (!m_tpDisplayMemberTemplate)
            {
                IFC(ctl::make(&spDisplayMemberTemplate));
                IFC(spDisplayMemberTemplate->put_DisplayMemberPath(strDisplayMemberPath.Get()));
                SetPtrValue(m_tpDisplayMemberTemplate, spDisplayMemberTemplate);
            }
            IFC(m_tpDisplayMemberTemplate.As(&spItemTemplate));
        }

        bool isContainerFromTemplateRoot = pVirtualizationInformation != nullptr && pVirtualizationInformation->GetIsContainerFromTemplateRoot();

        if (pContentPresenter)
        {
            ASSERT(!deferredBehavior);

            if (isContainerFromTemplateRoot)
            {
                // set data context
                IFC(static_cast<FrameworkElement*>(pContentPresenter)->put_DataContext(item));
            }
            else
            {
                IFC(pContentPresenter->put_Content(item));
                IFC(pContentPresenter->put_ContentTemplate(spItemTemplate.Get()));
                IFC(pContentPresenter->put_ContentTemplateSelector(spItemTemplateSelector.Get()));
            }
        }
        else
        {
            // we are not setting content anymore during prepare, except through the quirk (used in determining the deferral)
            // We are going to set the content when raising the sync version of the ContainerContentChanging event
            // It will be done when the user doesn't 'handle' that event
            ctl::ComPtr<IPanel> spItemsHost;

            IFC(get_ItemsHost(&spItemsHost));

            if (isContainerFromTemplateRoot)
            {
                // set data context
                IFC(static_cast<FrameworkElement*>(pContentControl)->put_DataContext(item));
            }
            else
            {
                // moderncollectionbasepanel will call SetupContainerContentChangingAfterPrepare after prepare
                // and set the content there.
                // if moderncollectionbasepanel works with old (non-tree builder controls) then we need to set content here.
                if (!spItemsHost || !ctl::is<ICustomGeneratorItemsHost>(spItemsHost) || !ctl::is<ITreeBuilder>(this))
                {
                    IFC(pContentControl->put_Content(item));
                }
            }

            if (ctl::ComPtr<SelectorItem> spContainerAsSelectorItem = ctl::query_interface_cast<SelectorItem>(element))
            {
                // specify whether this container is a ui placeholder according to us
                spContainerAsSelectorItem->SetIsUIPlaceholder(!!deferredBehavior);
            }

            if (!skipExpensivePrepareWork && !isContainerFromTemplateRoot)
            {
                IFC(pContentControl->put_ContentTemplate(spItemTemplate.Get()));
                IFC(pContentControl->put_ContentTemplateSelector(spItemTemplateSelector.Get()));
            }
        }
    }

    if (!skipExpensivePrepareWork)
    {
        // apply the ItemContainer style (if any, and if appropriate).
        IFC(ApplyItemContainerStyle(element, item));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
    HRESULT
    ItemsControl::NotifyOfSourceChanged(
    _In_ wfc::IObservableVector<IInspectable*>* pSender,
    _In_ wfc::IVectorChangedEventArgs* e)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<IPanel> spItemsHost;
    ctl::ComPtr<ICustomGeneratorItemsHost> spCustomItemsHost;

    IFC(get_ItemsHost(&spItemsHost));

    // this is callback from ItemsCollection. By this point generator must exist.
    // todo: for now I've kept this method out of ICG2, maybe that should be moved in there at some point.
    if (spItemsHost)
    {
        spCustomItemsHost = spItemsHost.AsOrNull<ICustomGeneratorItemsHost>();
    }

    if (spCustomItemsHost)
    {
        // new style panels: the panel implements internal IICG2 interface
        IFC(spCustomItemsHost->NotifyOfItemsChanging(e));
    }
    else
    {
        // old style panels: m_tpGenerator is pointing to an ICG implementations
        IFC(m_tpGenerator.Cast<ItemContainerGenerator>()->NotifyOfSourceChanged(pSender, e));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
    HRESULT
    ItemsControl::OnItemsChanged(
    _In_ wfc::IVectorChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    wfc::CollectionChange action = wfc::CollectionChange_Reset;

    ctl::ComPtr<IPanel> spItemsHost;
    IFC(get_ItemsHost(&spItemsHost));

    IFCPTR(pArgs);
    IFC(UpdateTickContextCounters());

    IFC(pArgs->get_CollectionChange(&action));
    switch (action)
    {
    case wfc::CollectionChange_Reset:
        {
            m_resetItemsThisLayoutTick = TRUE;
            // load should no longer come into play:
            m_loadedTick = 0;
            break;
        }
    case wfc::CollectionChange_ItemInserted:
        {
            m_elementCountAddedThisLayoutTick += 1;
            break;
        }
    case wfc::CollectionChange_ItemRemoved:
        {
            m_elementCountRemovedThisLayoutTick += 1;
            break;
        }
    }

    if (spItemsHost)
    {
        ctl::ComPtr<ICustomGeneratorItemsHost> spCustomItemsHost = spItemsHost.AsOrNull<ICustomGeneratorItemsHost>();
        if (spCustomItemsHost)
        {
            // new style panels: the panel implements internal IICG2 interface
            IFC(spCustomItemsHost->NotifyOfItemsChanged(pArgs));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControl::OnItemsChangedImpl(_In_ IInspectable* e)
{
    // Don't override this. The method exist temporary until we have IDL limitation for IVectorChangedEventArgs
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVectorChangedEventArgs> spArgs;
    IFC(ctl::do_query_interface(spArgs, e));
    IFC(OnItemsChanged(spArgs.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControl::OnItemContainerStyleChangedImpl(
    _In_ IStyle* oldItemContainerStyle,
    _In_ IStyle* newItemContainerStyle)
{
    RRETURN(RefreshContainers());
}

_Check_return_
    HRESULT
    ItemsControl::OnItemContainerStyleChanged(
    _In_ IInspectable* pOldValue,
    _In_ IInspectable* pNewValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IStyle> spOldItemContainerStyle;
    ctl::ComPtr<IStyle> spNewItemContainerStyle;

    spOldItemContainerStyle = ctl::query_interface_cast<IStyle>(pOldValue);
    spNewItemContainerStyle = ctl::query_interface_cast<IStyle>(pNewValue);

    IFC(OnItemContainerStyleChangedProtected(spOldItemContainerStyle.Get(), spNewItemContainerStyle.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControl::OnItemContainerStyleSelectorChangedImpl(
    _In_ IStyleSelector* oldItemContainerStyleSelector,
    _In_ IStyleSelector* newItemContainerStyleSelector)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IStyle> spContainerStyle;
    IFC(get_ItemContainerStyle(&spContainerStyle));
    if(!spContainerStyle)
    {
        IFC(RefreshContainers());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
    HRESULT
    ItemsControl::OnItemContainerStyleSelectorChanged(
    _In_ IInspectable* pOldValue,
    _In_ IInspectable* pNewValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IStyleSelector> spOldItemContainerStyleSelector;
    ctl::ComPtr<IStyleSelector> spNewItemContainerStyleSelector;

    spOldItemContainerStyleSelector = ctl::query_interface_cast<IStyleSelector>(pOldValue);
    spNewItemContainerStyleSelector = ctl::query_interface_cast<IStyleSelector>(pNewValue);

    IFC(OnItemContainerStyleSelectorChangedProtected(spOldItemContainerStyleSelector.Get(), spNewItemContainerStyleSelector.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControl::OnItemTemplateChangedImpl(
    _In_ IDataTemplate* oldItemTemplate,
    _In_ IDataTemplate* newItemTemplate)
{
    RRETURN(RefreshContainers());
}

_Check_return_
    HRESULT
    ItemsControl::OnItemTemplateChanged(
    _In_ IInspectable* pOldValue,
    _In_ IInspectable* pNewValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IDataTemplate> spOldItemTemplate;
    ctl::ComPtr<IDataTemplate> spNewItemTemplate;

    spOldItemTemplate = ctl::query_interface_cast<IDataTemplate>(pOldValue);
    spNewItemTemplate = ctl::query_interface_cast<IDataTemplate>(pNewValue);

    IFC(OnItemTemplateChangedProtected(spOldItemTemplate.Get(), spNewItemTemplate.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControl::OnItemTemplateSelectorChangedImpl(
    _In_ IDataTemplateSelector* oldItemTemplateSelector,
    _In_ IDataTemplateSelector* newItemTemplateSelector)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IDataTemplate> spItemTemplate;
    IFC(get_ItemTemplate(&spItemTemplate));
    if (!spItemTemplate)
    {
        IFC(RefreshContainers());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
    HRESULT
    ItemsControl::OnItemTemplateSelectorChanged(
    _In_ IInspectable* pOldValue,
    _In_ IInspectable* pNewValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IDataTemplateSelector> spOldItemTemplateSelector;
    ctl::ComPtr<IDataTemplateSelector> spNewItemTemplateSelector;

    spOldItemTemplateSelector = ctl::query_interface_cast<IDataTemplateSelector>(pOldValue);
    spNewItemTemplateSelector = ctl::query_interface_cast<IDataTemplateSelector>(pNewValue);

    IFC(OnItemTemplateSelectorChangedProtected(spOldItemTemplateSelector.Get(), spNewItemTemplateSelector.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControl::OnGroupStyleSelectorChangedImpl(
    _In_ IGroupStyleSelector* oldGroupStyleSelector,
    _In_ IGroupStyleSelector* newGroupStyleSelector)
{
    RRETURN(RefreshContainers());
}

_Check_return_
    HRESULT
    ItemsControl::OnGroupStyleSelectorChanged(
    _In_ IInspectable* pOldValue,
    _In_ IInspectable* pNewValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IGroupStyleSelector> spOldGroupStyleSelector;
    ctl::ComPtr<IGroupStyleSelector> spNewGroupStyleSelector;

    spOldGroupStyleSelector = ctl::query_interface_cast<IGroupStyleSelector>(pOldValue);
    spNewGroupStyleSelector = ctl::query_interface_cast<IGroupStyleSelector>(pNewValue);

    IFC(OnGroupStyleSelectorChangedProtected(spOldGroupStyleSelector.Get(), spNewGroupStyleSelector.Get()));

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
//  OnItemsPanelChanged
//
//  ItemsPanel property changed handler
//
//-----------------------------------------------------------------------------

_Check_return_
    HRESULT
    ItemsControl::OnItemsPanelChanged(
    _In_ IInspectable* pOldValue,
    _In_ IInspectable* pNewValue)
{
    HRESULT hr = S_OK;

    // Clear IsVirtualizing which was set by the old ItemsPanel. If needed, the
    // new panel will set it on the ItemsControl in the panel's Measure
    if (m_isVirtualizingPropertySetByPanel)
    {
        IFC(VirtualizingStackPanelFactory::SetIsVirtualizingStatic(this, FALSE));
        m_isVirtualizingPropertySetByPanel = FALSE;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
    HRESULT
    ItemsControl::OnGeneratorItemsChanged(
    _In_ IInspectable* pSender,
    _In_ IItemsChangedEventArgs* e)
{
    ctl::ComPtr<IPanel> spItemsHost;
    GeneratorPosition position = {-1, 0};
    BOOLEAN bItemsHostInvalid = FALSE;

    IFC_RETURN(get_IsItemsHostInvalid(&bItemsHostInvalid));
    IFC_RETURN(get_ItemsHost(&spItemsHost));

    if (!spItemsHost || bItemsHostInvalid || ctl::is<IVirtualizingPanel>(spItemsHost))
    {
        return S_OK;
    }

    int32_t intAction;
    IFC_RETURN(e->get_Action(&intAction));
    IFC_RETURN(e->get_Position(&position));

    switch (static_cast<DirectUI::CollectionChange>(intAction))
    {
    case CollectionChange::ItemInserted:
        IFC_RETURN(AddContainerForPosition(position));
        break;
    case CollectionChange::ItemRemoved:
        IFC_RETURN(RemoveContainerForPosition(position));
        break;
    case CollectionChange::ItemChanged:
        IFC_RETURN(RemoveContainerForPosition(position));
        IFC_RETURN(AddContainerForPosition(position));
        break;
    case CollectionChange::Reset:
        IFC_RETURN(ClearContainers(/*bHostIsReplaced*/ false));
        break;
    default:
        break;
    }

    return S_OK;
}

_Check_return_
    HRESULT
    ItemsControl::OnGroupStyleChanged(
    _In_ wfc::IObservableVector<xaml_controls::GroupStyle*>* pSender,
    _In_ wfc::IVectorChangedEventArgs* e)
{
    RRETURN(RefreshContainers());
}

_Check_return_
    HRESULT
    ItemsControl::AddContainerForPosition(
    _In_ GeneratorPosition position)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spDO;
    ctl::ComPtr<IItemContainerGenerator> spItemContainerGenerator;
    BOOLEAN isNewlyRealized = FALSE;
    INT nIndex = 0;

    IFC(get_ItemContainerGenerator(&spItemContainerGenerator));

    IFC(m_tpGenerator->StartAt(position, xaml_primitives::GeneratorDirection::GeneratorDirection_Forward, TRUE));
    IFC(m_tpGenerator->GenerateNext(&isNewlyRealized, &spDO));
    IFC(m_tpGenerator->Stop());

    // if spDO is NULL, which means we reached the end of the items (because of a group)
    if (spDO)
    {
        IFC(m_tpGenerator->IndexFromGeneratorPosition(position, &nIndex));
        IFC(AddVisualChild(nIndex, spDO.Get(), isNewlyRealized));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
    HRESULT
    ItemsControl::RemoveContainerForPosition(
    _In_ GeneratorPosition position)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spItemsHost;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

    IFC(get_ItemsHost(&spItemsHost));
    IFCEXPECT(spItemsHost);

    IFC(spItemsHost->get_Children(&spChildren));
    IFC(spChildren->RemoveAt(position.Index));

Cleanup:
    RRETURN(hr);
}

_Check_return_
    HRESULT
    ItemsControl::AddVisualChild(
    _In_ UINT nContainerIndex,
    _In_ xaml::IDependencyObject* pContainer,
    _In_ BOOLEAN bNeedPrepareContainer)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<IPanel> spItemsHost;
    ctl::ComPtr<IUIElement> spUIElement;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    UINT nChildrenCount = 0;

    IFCEXPECT(pContainer);
    IFC(ctl::do_query_interface(spUIElement, pContainer));

    IFC(get_ItemsHost(&spItemsHost));
    IFC(spItemsHost->get_Children(&spChildren));

    IFC(spChildren->get_Size(&nChildrenCount));
    if (nContainerIndex < nChildrenCount)
    {
        IFC(spChildren->InsertAt(nContainerIndex, spUIElement.Get()));
    }
    else
    {
        IFC(spChildren->Append(spUIElement.Get()));
    }

    if (bNeedPrepareContainer)
    {
        ctl::ComPtr<IItemContainerGenerator> spItemContainerGenerator;

        IFC(get_ItemContainerGenerator(&spItemContainerGenerator));
        IFC(m_tpGenerator->PrepareItemContainer(pContainer));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
    HRESULT
    ItemsControl::GetItemsOwner(
    _In_ xaml::IDependencyObject* element,
    _Outptr_result_maybenull_ IItemsControl** returnValue)
{
    HRESULT hr = S_OK;
    IFC(GetItemsOwner(element, false, returnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_
    HRESULT
    ItemsControl::GetItemsOwner(
    _In_ xaml::IDependencyObject* element,
    _In_ bool ignoreGrouping,
    _Outptr_result_maybenull_ IItemsControl** returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IItemsControl> spItemsControl;
    ctl::ComPtr<xaml::IDependencyObject> spElementAsDO;
    ctl::ComPtr<xaml::IDependencyObject> spParentElementAsDO;
    ctl::ComPtr<Panel> spPanel;
    BOOLEAN bIsItemHost = FALSE;
    ctl::ComPtr<ItemsPresenter> spItemsPresenter;
    ctl::ComPtr<IGroupItem> spGroupItem;
    ctl::ComPtr<DependencyObject> spTemplatedParent;

    *returnValue = nullptr;

    if (!element)
    {
        goto Cleanup;
    }

    spPanel = ctl::query_interface_cast<Panel>(element);
    if (!spPanel)
    {
        goto Cleanup;
    }

    IFC(spPanel->get_IsItemsHost(&bIsItemHost));
    if (!bIsItemHost)
    {
        goto Cleanup;
    }

    if (!ignoreGrouping)
    {
        IFC(spPanel->GetItemsOwner(&spItemsControl));
    }

    if (!spItemsControl)
    {
        // see if element was generated for an ItemsPresenter
        IFC(spPanel->get_TemplatedParent(&spTemplatedParent));
        if (ctl::is<IItemsPresenter>(spTemplatedParent))
        {
            // here we should have ItemsPresenter, if we don't we don't know
            // whose panel it is
            spItemsPresenter = spTemplatedParent.Cast<ItemsPresenter>();
            spTemplatedParent.Reset();

            // if so use the element whose style beget the ItemsPresenter

            IFC(spItemsPresenter->get_TemplatedParent(&spTemplatedParent));

            // if we have the presenter which has not ItemsControl as templated parent
            // we should try to find it as first ancestor.
            if (!ctl::is<IItemsControl>(spTemplatedParent))
            {
                spElementAsDO = spItemsPresenter;
                spItemsPresenter.Reset();
                while (spElementAsDO  && !(ctl::is<IItemsControl>(spElementAsDO)))
                {
                    IFC(VisualTreeHelper::TryGetParentStatic(spElementAsDO.Get(), &spParentElementAsDO));
                    spElementAsDO = spParentElementAsDO;
                }

                spParentElementAsDO.Reset();
                IFC(spElementAsDO.As(&spItemsControl));
            }
            else
            {
                spItemsControl = static_cast<IItemsControl*>(spTemplatedParent.Cast<ItemsControl>());
                spTemplatedParent.Reset();

                if(!ignoreGrouping)
                {
                    IFC(spItemsControl.Cast<ItemsControl>()->get_TemplatedParent(&spTemplatedParent));
                    spGroupItem = spTemplatedParent.AsOrNull<IGroupItem>();
                    if (spGroupItem &&
                        spGroupItem.Cast<GroupItem>()->m_tpGenerator)
                    {
                        spItemsControl.Reset();
                        IFC(VisualTreeHelper::TryGetParentStatic(spGroupItem.Cast<GroupItem>(), &spParentElementAsDO));
                        IFC(GetItemsOwner(spParentElementAsDO.Get(), &spItemsControl));
                    }
                }
            }

            if (spItemsControl && !ignoreGrouping)
            {
                IFC(spPanel->SetItemsOwner(spItemsControl.Get()));
            }
        }
    }

    IFC(spItemsControl.MoveTo(returnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControlFactory::GetItemsOwnerImpl(
    _In_ xaml::IDependencyObject* element,
    _Outptr_ IItemsControl** returnValue)
{
    RRETURN(ItemsControl::GetItemsOwner(element, returnValue));
}

_Check_return_ HRESULT ItemsControl::get_ItemContainerGeneratorImpl(
    _Outptr_ IItemContainerGenerator** pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);
    if (!m_tpGenerator && !m_tpMapping)
    {
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
        IFCEXPECT(!m_tpItems);
        // Accessing the Items property will cause the m_tpItems and the m_tpGenerator to be created
        // by invoking SetItemCollection from native side.
        IFC(get_Items(&spItems));
    }

    IFCEXPECT(m_tpItems);
    IFCEXPECT(m_tpMapping); // The ICG can be null if using a modern panel, but the mapping should be set as a minimum
    IFC(m_tpGenerator.CopyTo(pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
    ItemsControl::get_ItemsHost(_Outptr_ xaml_controls::IPanel** pValue)
{
    HRESULT hr = S_OK;

    if (m_tpItemsHost && !static_cast<CItemsControl*>(GetHandle())->m_bItemsHostInvalid)
    {
        IFC(m_tpItemsHost.CopyTo(pValue));
    }
    else
    {
        IFC(ItemsControlGenerated::get_ItemsHost(pValue));
        if (*pValue)
        {
            SetPtrValue(m_tpItemsHost, *pValue);
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
    ItemsControl::get_IsItemsHostInvalid(_Out_ BOOLEAN* pValue)
{
    *pValue = !!static_cast<CItemsControl*>(GetHandle())->m_bItemsHostInvalid;
    RRETURN(S_OK);
}

// Returns the ItemsPresenter, if found, that is the templated parent of the ItemsHost.
_Check_return_ HRESULT
    ItemsControl::get_ItemsPresenter(
    _Outptr_opt_ xaml_controls::IItemsPresenter** ppItemsPresenter)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IPanel> spItemsHost;

    *ppItemsPresenter = nullptr;

    IFC(get_ItemsHost(&spItemsHost));
    if (spItemsHost)
    {
        ctl::ComPtr<DependencyObject> spTemplatedParent;
        ctl::ComPtr<IItemsPresenter> spItemsPresenter;

        IFC(spItemsHost.Cast<Panel>()->get_TemplatedParent(&spTemplatedParent));
        spItemsPresenter = spTemplatedParent.AsOrNull<IItemsPresenter>();
        IFC(spItemsPresenter.MoveTo(ppItemsPresenter));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
    ItemsControl::get_Items(
    _Outptr_ wfc::IObservableVector<IInspectable*>** pValue)
{
    HRESULT hr = S_OK;
    IFCPTR(pValue);

    if (!m_tpItems)
    {
        IFC(ItemsControlGenerated::get_Items(pValue));
    }
    else
    {
        IFC(m_tpItems.CopyTo(pValue));
    }

    IFCEXPECT(m_tpItems);

Cleanup:
    RRETURN(hr);
}

_Check_return_ IFACEMETHODIMP
ItemsControl::get_View(
    _Outptr_ wfc::IVector<IInspectable*>** ppView)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spObservableVector;

    IFCPTR(ppView);
    IFC(get_Items(&spObservableVector));
    IFC(spObservableVector.CopyTo(ppView));

Cleanup:
    RRETURN(hr);
}

_Check_return_ IFACEMETHODIMP
ItemsControl::get_CollectionView(
    _Outptr_ ICollectionView** ppCollectionView)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spSource;
    ctl::ComPtr<ICollectionView> spResult;

    IFCPTR(ppCollectionView);
    IFC(get_ItemsSource(&spSource));

    spResult = spSource.AsOrNull<ICollectionView>();
    IFC(spResult.MoveTo(ppCollectionView));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControl::get_GroupStyleImpl(_Outptr_ wfc::IObservableVector<xaml_controls::GroupStyle*>** pValue)
{
    HRESULT hr = S_OK;
    IFCPTR(pValue);
    IFC(m_tpGroupStyle.CopyTo(pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_
    HRESULT
    ItemsControl::IsItemItsOwnContainer(
    _In_ IInspectable* pItem,
    _Out_ BOOLEAN* pIsOwnContainer)
{
    RRETURN(IsItemItsOwnContainerOverrideProtected(pItem, pIsOwnContainer));
}

_Check_return_
    HRESULT
    ItemsControl::GetContainerForItem(
    _In_ IInspectable* pItem,
    _In_opt_ xaml::IDependencyObject* pRecycledContainer,
    _Outptr_ xaml::IDependencyObject** ppContainer)
{
    HRESULT hr = S_OK;
    BOOLEAN bItemsIsContainer = FALSE;
    ctl::ComPtr<xaml::IDependencyObject> spContainer;
    ctl::ComPtr<IContentControl> spContentControl;
    ctl::ComPtr<IUIElement> spContainerAsUIE;

    IFC(IsItemItsOwnContainer(pItem, &bItemsIsContainer));

    if (bItemsIsContainer)
    {
        // Or, if the item is its own container, just use it as is.

        // TODO: Report Error ItemsControls_ItemsControlItemsMustBeDerivativeOfUIElement
        // Bug#95997
        IFCEXPECT(ctl::value_is<IUIElement>(pItem));
        spContainer = ctl::query_interface_cast<xaml::IDependencyObject>(pItem);

        // TODO: Check that if we have ItemsSource items then ItemTemplate property is set
        // Report Error ItemsControls_ItemsControlItemsMustNotBeAUIElementWhenItemTemplateIsSet
        // Bug#95997
    }
    else
    {
        // Otherwise create a visual to render the item (a ContentPresenter by default)
        // or use the passed in recycledContainer.

        if (pRecycledContainer && ctl::is<IUIElement>(pRecycledContainer))
        {
            spContainer = pRecycledContainer;
        }
        else
        {
            IFC(GetContainerForItemOverrideProtected(&spContainer));
        }

        // Report Error Resx.ItemsControls_ItemsControlGetContainerForItemOverrideMustReturnDerivativeOfUIElement
        // Bug#95997
        IFCEXPECT(spContainer && ctl::is<IUIElement>(spContainer));

        spContentControl = spContainer.AsOrNull<IContentControl>();
        if (spContentControl)
        {
            IFC(spContentControl.Cast<ContentControl>()->SetContentIsNotLogical());
        }

        // Record that this container was generated. We need this so we can skip un-setting the container's Style in
        // CleanupContainerForItem.
        IFC(spContainer.As(&spContainerAsUIE));
        spContainerAsUIE.Cast<UIElement>()->SetIsGeneratedContainer(true);
    }

    IFC(spContainer.MoveTo(ppContainer));

Cleanup:
    RRETURN(hr);
}

_Check_return_ IFACEMETHODIMP
ItemsControl::GetHeaderForGroup(
    _In_ IInspectable* pGroup,
    _Outptr_ xaml::IDependencyObject** ppContainer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spContainer;
    ctl::ComPtr<IContentControl> spContentControl;

    // we do not support passing in your own groups yet

    IFC(GetHeaderForGroupOverrideImpl(&spContainer));

    spContentControl = spContainer.AsOrNull<IContentControl>();

    IFC(spContentControl.Cast<ContentControl>()->SetContentIsNotLogical());

    // Record that this container was generated. We need this so we can skip un-setting the container's Style in
    // CleanupContainerForItem.
    spContentControl.Cast<ContentControl>()->SetIsGeneratedContainer(true);

    IFC(spContainer.MoveTo(ppContainer));

Cleanup:
    RRETURN(hr);
}

_Check_return_ IFACEMETHODIMP
ItemsControl::PrepareItemContainer(
    _In_ xaml::IDependencyObject* pContainer,
    _In_ IInspectable* pItem)
{
    TracePrepareContainerBegin();
    HRESULT hr = S_OK;
    ctl::ComPtr<IGroupItem> spGroupItem;

    // GroupItems are special - their information comes from a different place
    spGroupItem = ctl::query_interface_cast<IGroupItem>(pContainer);
    if (spGroupItem)
    {
        IFC(spGroupItem.Cast<GroupItem>()->PrepareItemContainer(pItem, this));
    }
    else
    {
        ctl::ComPtr<xaml::IDependencyObject> spContainer = pContainer;
        ctl::ComPtr<xaml::IUIElement> spContainerAsUIE = spContainer.AsOrNull<xaml::IUIElement>();
        VirtualizationInformation *pVirtualizationInformation = spContainerAsUIE ? spContainerAsUIE.Cast<UIElement>()->GetVirtualizationInformation() : nullptr;

        IFC(PreProcessContentPreparation(pContainer, pItem));

        if (pVirtualizationInformation == nullptr || !pVirtualizationInformation->GetMaySkipPreparation())
        {
            IFC(PrepareContainerForItemOverrideProtected(pContainer, pItem));

            if (pVirtualizationInformation != nullptr)
            {
                pVirtualizationInformation->SetIsPrepared(true);
            }
        }
    }

Cleanup:
    TracePrepareContainerEnd();
    RRETURN(hr);
}

_Check_return_ IFACEMETHODIMP
ItemsControl::ClearContainerForItem(
    _In_ xaml::IDependencyObject* pContainer,
    _In_ IInspectable* pItem)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IGroupItem> spGroupItem;

    // GroupItems are special - their information comes from a different place
    spGroupItem = ctl::query_interface_cast<IGroupItem>(pContainer);
    if (spGroupItem)
    {
        IFC(spGroupItem.Cast<GroupItem>()->ClearContainerForItem(pItem));
    }
    else
    {
        ctl::ComPtr<xaml::IDependencyObject> spContainer = pContainer;
        ctl::ComPtr<xaml::IUIElement> spContainerAsUIE = spContainer.AsOrNull<xaml::IUIElement>();
        VirtualizationInformation *pVirtualizationInformation = spContainerAsUIE ? spContainerAsUIE.Cast<UIElement>()->GetVirtualizationInformation() : nullptr;

        IFC(ClearContainerForItemOverrideProtected(pContainer, pItem));

        if (pVirtualizationInformation != nullptr)
        {
            pVirtualizationInformation->SetIsPrepared(false);
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ IFACEMETHODIMP
ItemsControl::IsHostForItemContainer(
    _In_ xaml::IDependencyObject* pContainer,
    _Out_ BOOLEAN* pIsHost)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IItemsControl> spItemsControl;
    BOOLEAN hasParent = FALSE;

    IFCPTR(pIsHost);
    *pIsHost = FALSE;

    // If ItemsControlFromItemContainer can determine who owns the element,
    // use its decision.
    IFC(ItemsControlFromItemContainer(pContainer, &spItemsControl));

    if (spItemsControl)
    {
        *pIsHost = (spItemsControl.Get() == static_cast<IItemsControl*>(this));
        goto Cleanup;
    }

    // If the element is in my items view, and if it can be its own ItemContainer,
    // it's mine.  Contains may be expensive, so we avoid calling it in cases
    // where we already know the answer - namely when the element has a
    // logical parent (ItemsControlFromItemContainer handles this case).  This
    // leaves only those cases where the element belongs to my items
    // without having a logical parent (e.g. via ItemsSource) and without
    // having been generated yet. HasItem indicates if anything has been generated.

    if (ctl::is<IFrameworkElement>(pContainer))
    {
        IFC(static_cast<FrameworkElement*>(static_cast<DependencyObject*>(pContainer))->HasParent(&hasParent));
    }

    if (!hasParent)
    {
        IFC(IsItemItsOwnContainer(pContainer, pIsHost));
        if (*pIsHost)
        {
            UINT nCount = 0;
            if (m_tpItems)
            {
                IFC(m_tpItems->get_Size(&nCount));
            }
            *pIsHost = nCount > 0;
            if (*pIsHost)
            {
                IFC(m_tpItems->IndexOf(pContainer, &nCount, pIsHost));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Basically gets the Item From the Container, the Container could be the Item Itself.
_Check_return_
    HRESULT
    ItemsControl::GetItemOrContainerFromContainer(
    _In_ IDependencyObject* pContainer,
    _Outptr_ IInspectable** ppItem)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spItem;
    BOOLEAN isUnsetValue = FALSE;

    IFCPTR(ppItem);
    IFCEXPECT(pContainer);
    IFC(ItemFromContainer(pContainer, &spItem));

    IFC(DependencyPropertyFactory::IsUnsetValue(spItem.Get(), isUnsetValue));

    if ( isUnsetValue )
    {
        spItem.Reset();
    }

    IFC(spItem.MoveTo(ppItem));

Cleanup:
    RRETURN(hr);
}

_Check_return_ IFACEMETHODIMP
ItemsControl::GetGroupStyle(
    _In_opt_ ICollectionViewGroup* pGroup,
    _In_ UINT level,
    _Out_ IGroupStyle** ppGroupStyle)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IGroupStyleSelector> spGroupStyleSelector;
    ctl::ComPtr<IGroupStyle> spGroupStyle;
    ctl::ComPtr<wfc::IObservableVector<xaml_controls::GroupStyle*>> spGroupStyleCollection;
    ctl::ComPtr<wfc::IVector<xaml_controls::GroupStyle*>> spGroupStyleVector;

    IFC(get_GroupStyleSelector(&spGroupStyleSelector));

    // a. Use global selector
    if (spGroupStyleSelector)
    {
        IFC(spGroupStyleSelector->SelectGroupStyle(pGroup, level, &spGroupStyle));
    }

    // b. lookup in GroupStyle list
    if (!spGroupStyle)
    {
        UINT nStyleCount = 0;
        IFC(get_GroupStyle(&spGroupStyleCollection));
        IFC(spGroupStyleCollection.As(&spGroupStyleVector));
        IFC(spGroupStyleVector->get_Size(&nStyleCount))

            // use last entry for all higher levels
            if (nStyleCount > 0)
            {
                IFC(spGroupStyleVector->GetAt(MIN(nStyleCount - 1, level), &spGroupStyle));
            }
    }

    IFC(spGroupStyle.MoveTo(ppGroupStyle));

Cleanup:
    RRETURN(hr);
}

_Check_return_
    HRESULT
    ItemsControl::put_IsGrouping(_In_ BOOLEAN isGrouping)
{
    HRESULT hr = S_OK;

    // Cache the value. This is safe because the DP is read-only, so the only way
    // to set this value is internally through this code-path.
    if (isGrouping != m_isGrouping)
    {
        IFC(ItemsControlGenerated::put_IsGrouping(isGrouping));
        m_isGrouping = isGrouping;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ IFACEMETHODIMP
ItemsControl::SetIsGrouping(_In_ BOOLEAN isGrouping)
{
    RRETURN(put_IsGrouping(isGrouping));
}

_Check_return_ IFACEMETHODIMP
ItemsControl::PrepareGroupContainer(
    _In_ xaml::IDependencyObject* pContainer,
    _In_ xaml_data::ICollectionViewGroup* pGroup)
{
    // todo: should not have spContentControl necessarily. See cr feedback
    HRESULT hr = S_OK;
    ctl::ComPtr<IContentControl> spContentControl;
    ctl::ComPtr<IFrameworkElement> spContainerAsFE;
    ctl::ComPtr<IGroupStyle> spGroupStyle;
    ctl::ComPtr<IInspectable> spGroupData;
    ctl::ComPtr<IDataTemplate> spHeaderTemplate;
    ctl::ComPtr<IDataTemplateSelector> spHeaderTemplateSelector;

    spContentControl = ctl::query_interface_cast<IContentControl>(pContainer);

    IFC(GetGroupStyle(pGroup, 0, &spGroupStyle));

    // ordering here is important. First set content, so that the right content is sent
    // to the selector.

    // content
    IFC(pGroup->get_Group(&spGroupData));
    IFC(spContentControl->put_Content(spGroupData.Get()));

    // template
    IFC(spGroupStyle->get_HeaderTemplate(&spHeaderTemplate));
    IFC(spContentControl->put_ContentTemplate(spHeaderTemplate.Get()));

    // template selector
    IFC(spGroupStyle->get_HeaderTemplateSelector(&spHeaderTemplateSelector));
    IFC(spContentControl->put_ContentTemplateSelector(spHeaderTemplateSelector.Get()));

    // style
    spContainerAsFE = spContentControl.AsOrNull<IFrameworkElement>();
    if (spContainerAsFE)
    {
        ctl::ComPtr<xaml::IStyle> spHeaderStyle;
        BOOLEAN isUnsetValue = FALSE;

        const CDependencyProperty* pStyleProperty = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::FrameworkElement_Style);
        IFC(static_cast<DependencyObject*>(pContainer)->ReadLocalValue(pStyleProperty, &spHeaderStyle));
        IFC(DependencyPropertyFactory::IsUnsetValue(spHeaderStyle.Get(), isUnsetValue));

        if (isUnsetValue || spContainerAsFE.Cast<FrameworkElement>()->GetIsStyleSetFromItemsControl())
        {
            IFC(spGroupStyle->get_HeaderContainerStyle(&spHeaderStyle));

            // If we have been given a style to set, let's do it
            if (spHeaderStyle)
            {
                IFC(spContainerAsFE->put_Style(spHeaderStyle.Get()));
                spContainerAsFE.Cast<FrameworkElement>()->SetIsStyleSetFromItemsControl(TRUE);
            }
            else
            {
                // If we don't have one, clear the property and let the default/implicit take over
                IFC(static_cast<DependencyObject*>(pContainer)->ClearValue(pStyleProperty));
                spContainerAsFE.Cast<FrameworkElement>()->SetIsStyleSetFromItemsControl(FALSE);
            }
        }
    }

Cleanup:
    RRETURN(hr);
}


_Check_return_
    HRESULT
    ItemsControl::ItemsControlFromItemContainer(
    _In_ xaml::IDependencyObject* container,
    _Outptr_ IItemsControl** returnValue)
{
    HRESULT hr = S_OK;

    IFC(ItemsControlFromItemContainer(container, false, returnValue));
Cleanup:
    RRETURN(hr);
}

_Check_return_
    HRESULT
    ItemsControl::ItemsControlFromItemContainer(
    _In_ xaml::IDependencyObject* container,
    _In_ bool ignoreGrouping,
    _Outptr_ IItemsControl** returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spParent;
    ctl::ComPtr<IItemsControl> spItemsControl;

    IFCPTR(returnValue);
    *returnValue = NULL;

    if (!ctl::is<IUIElement>(container))
    {
        goto Cleanup;
    }

    if (ctl::is<IFrameworkElement>(container))
    {
        // First test logical parent.
        // If container is item of ItemsControl then logical parent will not be null.
        IFC(static_cast<FrameworkElement*>(static_cast<DependencyObject*>(container))->TryGetParent(&spParent));
    }

    if (ctl::is<IItemsControl>(spParent))
    {
        BOOLEAN bElementIsContainer = FALSE;
        IFC(spParent.As(&spItemsControl));

        // this is the right ItemsControl as long as the item
        // is (or is eligible to be) its own container
        IFC((spItemsControl.Cast<ItemsControl>())->IsItemItsOwnContainer(container, &bElementIsContainer));
        if (bElementIsContainer)
        {
            // if item is eligible to be its own container
            // then return ItemsControl
            IFC(spItemsControl.MoveTo(returnValue));
        }
        // if item is not eligible to be its own container
        // then we return null for ItemsControl because we test
        // the DependencyObject which is actually just an item of ItemsControl
    }
    else
    {
        // if logical parent is null or it is not ItemsControl then test visual parent
        spParent.Reset();
        IFC(VisualTreeHelper::TryGetParentStatic(container, &spParent));
        IFC(GetItemsOwner(spParent.Get(), ignoreGrouping, &spItemsControl));
        IFC(spItemsControl.MoveTo(returnValue));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControlFactory::ItemsControlFromItemContainerImpl(
    _In_ xaml::IDependencyObject* container,
    _Outptr_ IItemsControl** returnValue)
{
    RRETURN(ItemsControl::ItemsControlFromItemContainer(container, returnValue));
}

_Check_return_
    HRESULT
    ItemsControl::GetItemCount(
    _Out_ UINT& itemCount)
{
    HRESULT hr = S_OK;
    itemCount = 0;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItemsAsI;
    IFC(get_Items(&spItemsAsI));
    IFC(spItemsAsI.Cast<ItemCollection>()->get_Size(&itemCount));

Cleanup:
    RRETURN(hr);
}

_Check_return_
    HRESULT
    ItemsControl::OnItemsHostAvailable()
{
    HRESULT hr = S_OK;

    ctl::ComPtr<IPanel> spItemsHost;
    IFC(get_ItemsHost(&spItemsHost));

#ifdef ITEMSCONTROL_DBG
    WCHAR szTrace[256];
    IFCEXPECT(swprintf_s(szTrace, 256, L"ItemHost Available") >= 0);
    Trace(szTrace);
#endif

    // Different panel types can have different ICG setups, so we'd better make sure we have the right one
    if (ctl::is<ICustomGeneratorItemsHost>(spItemsHost))
    {
        IFC(InitializeItemContainerGenerator());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
    HRESULT
    ItemsControl::get_ItemsPanelRootImpl(_Outptr_ xaml_controls::IPanel** ppItemsHost)
{
    HRESULT hr = S_OK;

    *ppItemsHost = nullptr;

    BOOLEAN itemsHostInvalid = FALSE;
    IFC(get_IsItemsHostInvalid(&itemsHostInvalid));

    if (!itemsHostInvalid)
    {
        ctl::ComPtr<IPanel> spItemsHost;
        IFC(get_ItemsHost(&spItemsHost));
        IFC(spItemsHost.MoveTo(ppItemsHost));
    }

Cleanup:
    RRETURN(hr);
}

// Calls ChangeVisualState on all child SelectorItems (including items inside a GroupItem),
// with optimizations for the virtualization provided by IOrientedVirtualizingPanel.
_Check_return_ HRESULT ItemsControl::ChangeSelectorItemsVisualState(
    _In_ bool bUseTransitions)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<IPanel> spItemsHost;
    ctl::ComPtr<xaml_primitives::IOrientedVirtualizingPanel> spItemsHostAsIOrientedPanel;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    UINT childCount = 0;

    IFC(get_ItemsHost(&spItemsHost));

    if (spItemsHost)
    {
        spItemsHostAsIOrientedPanel = spItemsHost.AsOrNull<xaml_primitives::IOrientedVirtualizingPanel>();

        if (spItemsHostAsIOrientedPanel)
        {
            IFC(spItemsHostAsIOrientedPanel.Cast<OrientedVirtualizingPanel>()->get_RealizedChildren(&spChildren));
        }
        else
        {
            IFC(spItemsHost->get_Children(&spChildren));
        }

        if (spChildren)
        {
            IFC(spChildren->get_Size(&childCount));
            for (UINT i = 0; i < childCount; i++)
            {
                ctl::ComPtr<IUIElement> spCurrentChild;
                ctl::ComPtr<IGroupItem> spCurrentChildAsGroupItem;
                IFC(spChildren->GetAt(i, &spCurrentChild));

                if (!spCurrentChild)
                {
                    continue;
                }

                spCurrentChildAsGroupItem = spCurrentChild.AsOrNull<IGroupItem>();
                if (spCurrentChildAsGroupItem)
                {
                    IFC(spCurrentChildAsGroupItem.Cast<GroupItem>()->ChangeSelectorItemsVisualState(TRUE));
                }
                else
                {
                    ctl::ComPtr<ISelectorItem> spCurrentChildAsSelectorItem;
                    spCurrentChildAsSelectorItem = spCurrentChild.AsOrNull<ISelectorItem>();
                    if (spCurrentChildAsSelectorItem != NULL)
                    {
                        IFC(spCurrentChildAsSelectorItem.Cast<SelectorItem>()->ChangeVisualState(TRUE));
                    }
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Compute the TransitionContext
_Check_return_ HRESULT ItemsControl::GetCurrentTransitionContext(
    _Out_ ThemeTransitionContext* returnValue)
{
    IFCPTR_RETURN(returnValue);
    ThemeTransitionContext themeTransitionContext = ThemeTransitionContext::None;

    if (m_resetItemsThisLayoutTick)
    {
        // if content is changed, the colelction is reset or dataSource is changed, this animation runs
        themeTransitionContext = ThemeTransitionContext::ContentTransition;
    }
    // Reorder Ticks checks are above add/remove because if there is reorder, it will update the ItemsSource
    // which eventually updates m_elementCountAddedThisLayoutTick/m_elementCountRemovedThisLayoutTick
    else if (m_elementCountReorderedThisLayoutTick == 1)
    {
        // if single reorder happens
        themeTransitionContext = ThemeTransitionContext::SingleReorderList;
    }
    else if (m_elementCountReorderedThisLayoutTick > 1)
    {
        // if multiple reorder happens
        themeTransitionContext = ThemeTransitionContext::MultipleReorderList;
    }
    else if (m_elementCountAddedThisLayoutTick == 1 && m_elementCountRemovedThisLayoutTick == 0)
    {
        // if only 1 item gets added or removed
        themeTransitionContext = ThemeTransitionContext::SingleAddList;
    }
    else if (m_elementCountAddedThisLayoutTick == 0 && m_elementCountRemovedThisLayoutTick == 1)
    {
        themeTransitionContext = ThemeTransitionContext::SingleDeleteList;
    }
    // If there is Add and Delete ticks, we will consider as MultipleAppList since there is no separate Animation for Add/Delete.
    else if (m_elementCountAddedThisLayoutTick > 0)
    {
        // if multiple items gets added/removed
        themeTransitionContext = ThemeTransitionContext::MultipleAddList;
    }
    else if (m_elementCountAddedThisLayoutTick == 0 && m_elementCountRemovedThisLayoutTick > 0)
    {
        // if multiple items gets added/removed
        themeTransitionContext = ThemeTransitionContext::MultipleDeleteList;
    }

    *returnValue = themeTransitionContext;

    return S_OK;
}

// This method returns information regarding recent activities on ItemsControl Control,
// Activities are whether the items get added/removed/reordered or content changed.
// This method returns the above information as AnimationContext which is used by PVL
// to identify which animation to run
_Check_return_ HRESULT ItemsControl::GetCurrentTransitionContext(
    _In_ INT layoutTickId,
    _Out_ ThemeTransitionContext* returnValue)
{
    HRESULT hr = S_OK;
    ThemeTransitionContext themeTransitionContext = ThemeTransitionContext::None;

    IFCPTR(returnValue);

    // If new ID, just use currently calculated TransitionContext
    if (layoutTickId != m_previousTickCounterId)
    {
        IFC(UpdateTickContextCounters());

        // loaded is an async event, so the tick might not come in until later
        if (m_loadedTick == loadCounterToken)
        {
            m_loadedTick = layoutTickId;
        }

        // If layoutTick is same as the one during Load, call EntraceTransition
        if (m_loadedTick == layoutTickId)
        {
            // Entrance Transition should happen only on First time, this animation runs only first time
            themeTransitionContext = ThemeTransitionContext::Entrance;
        }
        else
        {
            IFC(GetCurrentTransitionContext(&themeTransitionContext));
        }
        m_themeTransitionContext = themeTransitionContext;
    }

#ifdef TRANSITION_CONTEXT_DBG
    //Trace out the value
    if (m_themeTransitionContext == ThemeTransitionContext::SingleAddList)
    {
        Trace(L"ThemeTransitionContext::SingleAddList");
    }
    else if (m_themeTransitionContext == ThemeTransitionContext::SingleDeleteList)
    {
        Trace(L"ThemeTransitionContext::SingleDeleteList");
    }
    else if (m_themeTransitionContext == ThemeTransitionContext::SingleReorderList)
    {
        Trace(L"ThemeTransitionContext::SingleReorderList");
    }
    else if (m_themeTransitionContext == ThemeTransitionContext::MultipleAddList)
    {
        Trace(L"ThemeTransitionContext::MultipleAddList");
    }
    else if (m_themeTransitionContext == ThemeTransitionContext::MultipleReorderList)
    {
        Trace(L"ThemeTransitionContext::MultipleReorderList");
    }
    else if (m_themeTransitionContext == ThemeTransitionContext::Entrance)
    {
        Trace(L"ThemeTransitionContext::Entrance");
    }
    else if (m_themeTransitionContext == ThemeTransitionContext::ContentTransition)
    {
        Trace(L"ThemeTransitionContext::ContentTransition");
    }
    else if (m_themeTransitionContext == ThemeTransitionContext::MultipleDeleteList)
    {
        Trace(L"ThemeTransitionContext::MultipleDeleteList");
    }

    WCHAR szValue[32];
    IFCEXPECT(swprintf_s(szValue, 32, L"%d", id) >= 0);
    Trace(szValue);
#endif

    // Store current id to use in the next cycle
    m_previousTickCounterId = layoutTickId;
    *returnValue = m_themeTransitionContext;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControl::UpdateTickContextCounters()
{
    HRESULT hr = S_OK;
    XINT16 currentTickCounterId = 0;

    IFC(CoreImports::LayoutManager_GetLayoutTickForTransition(static_cast<CUIElement*>(GetHandle()), &currentTickCounterId));

    // as we store one tick less from current we should adjust comparison.
    if (currentTickCounterId - 1 != m_previousTickCounterId)
    {
        m_elementCountAddedThisLayoutTick = 0;
        m_elementCountRemovedThisLayoutTick = 0;
        m_elementCountReorderedThisLayoutTick = 0;
        m_resetItemsThisLayoutTick = FALSE;
    }

    // We should not store current id before we get GetCurrentTransitionContext call.
    // Otherwise we won't re-calculate a transition context.
    m_previousTickCounterId = currentTickCounterId - 1;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControl::GetDropOffsetToRoot(
    _Out_ wf::Point* returnValue)
{
    RRETURN(S_OK);
}

_Check_return_ HRESULT ItemsControl::OnItemsReordered(_In_ UINT nCount)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spPanel;

    IFC(UpdateTickContextCounters());
    m_elementCountReorderedThisLayoutTick += nCount;

    IFC(get_ItemsHost(&spPanel));

    {
        ctl::ComPtr<ICustomGeneratorItemsHost> spCustomItemsHost = spPanel.AsOrNull<ICustomGeneratorItemsHost>();
        if (spCustomItemsHost)
        {
            IFC(spCustomItemsHost->NotifyOfItemsReordered(nCount));
        }
    }

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
//  SetVirtualizationStateByPanel
//
//  Called by a Virtualizing Panel to force IsVirtualizing. Note that
//  IsVirtualizing is a read-only property, so cannot be set by the app.
//
//-----------------------------------------------------------------------------

_Check_return_ HRESULT ItemsControl::SetVirtualizationStateByPanel()
{
    HRESULT hr = S_OK;
    BOOLEAN isVirtualizing = FALSE;

    IFC(VirtualizingStackPanelFactory::GetIsVirtualizingStatic(this, &isVirtualizing));
    if (!isVirtualizing)
    {
        IFC(VirtualizingStackPanelFactory::SetIsVirtualizingStatic(this, TRUE));
        m_isVirtualizingPropertySetByPanel = TRUE;
    }

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
//  OnDisconnectVisualChildren
//
//  During a DisconnectVisualChildrenRecursive tree walk, also clear the Items property.
//
//-----------------------------------------------------------------------------

IFACEMETHODIMP
    ItemsControl::OnDisconnectVisualChildren()
{
    HRESULT hr = S_OK;

    if( m_tpItems )
    {
        IFC( m_tpItems->DisconnectVisualChildrenRecursive() );
    }

    IFC( ItemsControlGenerated::OnDisconnectVisualChildren() );

Cleanup:

    RRETURN(hr);
}

// Applies ItemContainerStyle to the given container and item, if appropriate.
// If the Style is applied and the container is not internally-generated, the Style
// will be cleared on ClearContainerForItemOverride.
_Check_return_ HRESULT
    ItemsControl::ApplyItemContainerStyle(
    _In_ xaml::IDependencyObject* pContainer,
    _In_ IInspectable* pItem)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IDependencyObject> spContainer;
    ctl::ComPtr<IFrameworkElement> spContainerAsFE;

    IFCPTR(pContainer);
    spContainer = pContainer;

    spContainerAsFE = spContainer.AsOrNull<IFrameworkElement>();

    if (spContainerAsFE)
    {
        ctl::ComPtr<xaml::IStyle> spLocalStyle;
        BOOLEAN isStyleSetFromItemsControl = FALSE;
        BOOLEAN isUnsetValue = FALSE;

        const CDependencyProperty* pStyleProperty = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::FrameworkElement_Style);
        IFC(static_cast<DependencyObject*>(pContainer)->ReadLocalValue(pStyleProperty, &spLocalStyle));
        IFC(DependencyPropertyFactory::IsUnsetValue(spLocalStyle.Get(), isUnsetValue));

        isStyleSetFromItemsControl = spContainerAsFE.Cast<FrameworkElement>()->GetIsStyleSetFromItemsControl();
        if ( isUnsetValue || isStyleSetFromItemsControl)
        {
            ctl::ComPtr<xaml::IStyle> spItemContainerStyle;

            // ItemsControl's ItemContainerStyle has first stab
            IFC(get_ItemContainerStyle(&spItemContainerStyle));
            // apply the style, if found
            if (spItemContainerStyle)
            {
                IFC(spContainerAsFE->put_Style(spItemContainerStyle.Get()));
                spContainerAsFE.Cast<FrameworkElement>()->SetIsStyleSetFromItemsControl(TRUE);
            }
            else
            {
                ctl::ComPtr<IStyleSelector> spItemContainerStyleSelector;
                ctl::ComPtr<IStyle> spStyle;

                // Next, try to use the ItemContainerStyleSelector
                IFC(get_ItemContainerStyleSelector(&spItemContainerStyleSelector));
                if (spItemContainerStyleSelector)
                {
                    IFC(spItemContainerStyleSelector->SelectStyle(pItem, pContainer, &spStyle));
                }

                if (spStyle)
                {
                    IFC(spContainerAsFE->put_Style(spStyle.Get()));
                    spContainerAsFE.Cast<FrameworkElement>()->SetIsStyleSetFromItemsControl(TRUE);
                }
                else
                {
                    // if Style was formerly set from ItemContainerStyle, clear it
                    IFC(spContainerAsFE.Cast<FrameworkElement>()->ClearValue(pStyleProperty));
                    spContainerAsFE.Cast<FrameworkElement>()->SetIsStyleSetFromItemsControl(FALSE);
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Helper to find out GroupItemIndex from GroupItem
_Check_return_ HRESULT
    ItemsControl::GetGroupItemIndex(
    _In_ IInspectable* groupItem,
    _Out_ UINT* pGroupItemIndex,
    _Out_ BOOLEAN* pFound)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<IInspectable*>> spView;
    BOOLEAN isGrouping = FALSE;

    IFCPTR(pFound);
    IFCPTR(pGroupItemIndex);
    *pFound = FALSE;
    *pGroupItemIndex = 0;

    // If groupItem is nullptr, return found= false
    if(groupItem == nullptr)
    {
        goto Cleanup;
    }

    IFC(get_IsGrouping(&isGrouping));
    if(isGrouping)
    {
        ctl::ComPtr<IPanel> spPanel;

        IFC(get_ItemsHost(&spPanel));

        if (ctl::is<ICustomGeneratorItemsHost>(spPanel))
        {
            // If we have a modern panel, there are no nested ItemsControls, so we can get our collection view directly
            // We are probably going through a few extra steps to match ICG's IVector<IInspectable> view, but
            // I'd rather not risk breaking backwards compatibility on old apps
            ctl::ComPtr<ICollectionView> spCollectionView;
            IFC(get_CollectionView(&spCollectionView));
            if (spCollectionView)
            {
                ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spObservableGroups;
                IFC(spCollectionView->get_CollectionGroups(&spObservableGroups));
                if (spObservableGroups)
                {
                    IFC(spObservableGroups.As(&spView));
                }
            }
        }
        else
        {
            ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
            IFC(get_ItemContainerGenerator(&spGenerator));
            if (spGenerator)
            {
                // Only works for legacy panels. ModernCollectionPanels don't have an ICG or GroupItems
                IFC(spGenerator.Cast<ItemContainerGenerator>()->get_View(&spView));
            }
        }

        if (spView)
        {
            ctl::ComPtr<IInspectable> spItemGroup;
            ctl::ComPtr<xaml_data::ICollectionViewGroup> spGroup;
            ctl::ComPtr<IInspectable> spGroupData;

            // Go through all CollectionViewGroups, get Group from it and check whether it is given group or not
            UINT numGroups = 0;
            IFC(spView->get_Size(&numGroups));
            for (UINT index = 0; index < numGroups; ++index)
            {
                IFC(spView->GetAt(index, &spItemGroup));
                spGroup = spItemGroup.AsOrNull<xaml_data::ICollectionViewGroup>();
                IFC(spGroup->get_Group(&spGroupData));

                bool areEqual = false;
                IFC(PropertyValue::AreEqual(spGroupData.Get(), groupItem, &areEqual));
                if (areEqual)
                {
                    *pFound = TRUE;
                    *pGroupItemIndex = index;
                    break;
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControl::ItemFromContainerImpl(
    _In_ xaml::IDependencyObject* container,
    _Outptr_ IInspectable** returnValue)
{
    HRESULT hr = S_OK;

    if (!m_tpMapping)
    {
        // Accessing the Items property will hook up the plumbing by invoking SetItemCollection from the native side
        ASSERT(!m_tpItems);
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
        IFC(get_Items(&spItems));
    }
    IFC(m_tpMapping->ItemFromContainer(container, returnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControl::ContainerFromItemImpl(
    _In_opt_ IInspectable* item,
    _Outptr_ xaml::IDependencyObject** returnValue)
{
    HRESULT hr = S_OK;

    if (!m_tpMapping)
    {
        // Accessing the Items property will hook up the plumbing by invoking SetItemCollection from the native side
        ASSERT(!m_tpItems);
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
        IFC(get_Items(&spItems));
    }
    IFC(m_tpMapping->ContainerFromItem(item, returnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControl::IndexFromContainerImpl(
    _In_ xaml::IDependencyObject* container,
    _Out_ INT* returnValue)
{
    HRESULT hr = S_OK;

    if (!m_tpMapping)
    {
        // Accessing the Items property will hook up the plumbing by invoking SetItemCollection from the native side
        ASSERT(!m_tpItems);
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
        IFC(get_Items(&spItems));
    }
    IFC(m_tpMapping->IndexFromContainer(container, returnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControl::ContainerFromIndexImpl(
    _In_ INT index,
    _Outptr_ xaml::IDependencyObject** returnValue)
{
    HRESULT hr = S_OK;

    if (!m_tpMapping)
    {
        // Accessing the Items property will hook up the plumbing by invoking SetItemCollection from the native side
        ASSERT(!m_tpItems);
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
        IFC(get_Items(&spItems));
    }
    IFC(m_tpMapping->ContainerFromIndex(index, returnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControl::GroupHeaderContainerFromItemContainerImpl(
    _In_ xaml::IDependencyObject* pItemContainer,
    _Outptr_result_maybenull_ xaml::IDependencyObject** ppReturnValue)
{
    HRESULT hr = S_OK;

    // ICG doesn't implement IGHM interface, so we need to manually dispatch to the right handler
    if (!m_tpGroupMapping && !m_tpGenerator)
    {
        // Accessing the Items property will hook up the plumbing by invoking SetItemCollection from the native side
        ASSERT(!m_tpItems);
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
        IFC(get_Items(&spItems));
    }

    if (m_tpGroupMapping)
    {
        IFC(m_tpGroupMapping->GroupHeaderContainerFromItemContainer(pItemContainer, ppReturnValue));
    }
    else
    {
        IFC(E_UNEXPECTED);
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControl::GroupFromHeaderImpl(_In_ xaml::IDependencyObject* header, _Outptr_ IInspectable** returnValue)
{
    HRESULT hr = S_OK;

    // ICG doesn't implement IGHM interface, so we need to manually dispatch to the right handler
    if (!m_tpGroupMapping && !m_tpGenerator)
    {
        // Accessing the Items property will hook up the plumbing by invoking SetItemCollection from the native side
        ASSERT(!m_tpItems);
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
        IFC(get_Items(&spItems));
    }
    if (m_tpGroupMapping)
    {
        IFC(m_tpGroupMapping->GroupFromHeader(header, returnValue));
    }
    else
    {
        IFC(E_UNEXPECTED);
    }

Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT ItemsControl::HeaderFromGroupImpl(_In_ IInspectable* group, _Outptr_ xaml::IDependencyObject** returnValue)
{
    HRESULT hr = S_OK;

    // ICG doesn't implement IGHM interface, so we need to manually dispatch to the right handler
    if (!m_tpGroupMapping && !m_tpGenerator)
    {
        // Accessing the Items property will hook up the plumbing by invoking SetItemCollection from the native side
        ASSERT(!m_tpItems);
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
        IFC(get_Items(&spItems));
    }

    if (m_tpGroupMapping)
    {
        IFC(m_tpGroupMapping->HeaderFromGroup(group, returnValue));
    }
    else
    {
        IFC(E_UNEXPECTED);
    }

Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT ItemsControl::IndexFromHeaderImpl(_In_ xaml::IDependencyObject* header, _In_ BOOLEAN excludeHiddenEmptyGroups, _Out_ INT* returnValue)
{
    HRESULT hr = S_OK;

    // ICG doesn't implement IGHM interface, so we need to manually dispatch to the right handler
    if (!m_tpGroupMapping && !m_tpGenerator)
    {
        // Accessing the Items property will hook up the plumbing by invoking SetItemCollection from the native side
        ASSERT(!m_tpItems);
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
        IFC(get_Items(&spItems));
    }

    if (m_tpGroupMapping)
    {
        IFC(m_tpGroupMapping->IndexFromHeader(header, excludeHiddenEmptyGroups, returnValue));
    }
    else
    {
        IFC(E_UNEXPECTED);
    }

Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT ItemsControl::HeaderFromIndexImpl(_In_ INT index, _Outptr_ xaml::IDependencyObject** returnValue)
{
    HRESULT hr = S_OK;

    // ICG doesn't implement IGHM interface, so we need to manually dispatch to the right handler
    if (!m_tpGroupMapping && !m_tpGenerator)
    {
        // Accessing the Items property will hook up the plumbing by invoking SetItemCollection from the native side
        ASSERT(!m_tpItems);
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
        IFC(get_Items(&spItems));
    }
    if (m_tpGroupMapping)
    {
        IFC(m_tpGroupMapping->HeaderFromIndex(index, returnValue));
    }
    else
    {
        IFC(m_tpGenerator.Cast<ItemContainerGenerator>()->ContainerFromGroupIndex(index, returnValue));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControl::PrepareForItemRecyclingImpl(_In_opt_ IInspectable* item)
{
    RRETURN(m_dataSelectorRecyclingContext.PrepareForItemRecycling(item));
}

_Check_return_ HRESULT ItemsControl::IsCompatibleImpl(_In_ xaml::IUIElement* candidate, _Out_ BOOLEAN* returnValue)
{
    RRETURN(m_dataSelectorRecyclingContext.IsCompatible(candidate, returnValue));
}

_Check_return_ HRESULT ItemsControl::StopRecyclingImpl()
{
    RRETURN(m_dataSelectorRecyclingContext.StopRecycling());
}

_Check_return_ HRESULT ItemsControl::get_SelectedContainerImpl(_Outptr_ xaml::IUIElement** value)
{
    RRETURN(m_dataSelectorRecyclingContext.get_SelectedContainer(value));
}

_Check_return_ HRESULT ItemsControl::put_SelectedContainerImpl(_In_opt_ xaml::IUIElement* value)
{
    RRETURN(m_dataSelectorRecyclingContext.put_SelectedContainer(value));
}

_Check_return_ HRESULT ItemsControl::ConfigureSelectedContainerImpl(_In_ xaml::IUIElement* container)
{
    RRETURN(m_dataSelectorRecyclingContext.ConfigureSelectedContainer(container));
}

_Check_return_ HRESULT ItemsControl::GetRecyclingContext(_Outptr_ DirectUI::IContainerRecyclingContext** ppReturnValue)
{
    HRESULT hr = S_OK;

    *ppReturnValue = nullptr;

    // Return ItemsControl's IContainerRecyclingContext interface only if it has a ItemTemplateSelector implementing IDataTemplateSelector and NO ItemTemplate
    if (!static_cast<CItemsControl*>(GetHandle())->m_pItemTemplate)
    {
        ctl::ComPtr<xaml_controls::IDataTemplateSelector> spDataTemplateSelector;

        IFC(get_ItemTemplateSelector(&spDataTemplateSelector));
        if (spDataTemplateSelector)
        {
            m_dataSelectorRecyclingContext.SetDataTemplateSelector(spDataTemplateSelector.Get());
            if (spDataTemplateSelector)
            {
                IFC(ctl::do_query_interface(*ppReturnValue, this));
            }
            else
            {
                *ppReturnValue = nullptr;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

ItemsControl::DataTemplateSelectorRecyclingContext::DataTemplateSelectorRecyclingContext(_In_ ItemsControl* pOwner)
    : m_pOwner(pOwner)
{
}

ItemsControl::DataTemplateSelectorRecyclingContext::~DataTemplateSelectorRecyclingContext()
{
}

void ItemsControl::DataTemplateSelectorRecyclingContext::SetDataTemplateSelector(_In_opt_ xaml_controls::IDataTemplateSelector* const pSelector)
{
    m_pOwner->SetPtrValue(m_tpSelector, pSelector);
}

_Check_return_ HRESULT ItemsControl::DataTemplateSelectorRecyclingContext::get_SelectedContainer(_Outptr_ xaml::IUIElement** value)
{
    HRESULT hr = S_OK;

    IFC(m_tpSelectedContainer.CopyTo(value));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControl::DataTemplateSelectorRecyclingContext::put_SelectedContainer(_In_ xaml::IUIElement* value)
{
    // Store the selection
    m_pOwner->SetPtrValue(m_tpSelectedContainer, value);
    return S_OK;
}

// Store the selected data template in the VirtualizationInformation associated
// to the UIElement in order to avoid calling another time DataTemplateSelector::SelectTemplate
_Check_return_ HRESULT ItemsControl::DataTemplateSelectorRecyclingContext::ConfigureSelectedContainer(_In_ xaml::IUIElement* container)
{
    HRESULT hr = S_OK;
    VirtualizationInformation *pVirtualizationInformation = nullptr;

    // Make sure that the UIEmement has a virtualization information associated
    IFC(static_cast<UIElement*>(container)->InitVirtualizationInformation());

    // And get the corresponding pointer
    // if InitVirtualizationInformation succeeded, we are ensured that GetVirtualizationInformation
    // will not return NULL
    pVirtualizationInformation = static_cast<UIElement*>(container)->GetVirtualizationInformation();

    // m_tpSelectedTemplate.Get() should not be NULL but it is not a problem if it was:
    // it would simply mean that ContentControl will call DataTemplateSelector::SelectTemplate
    // which would be fine in this situation
    pVirtualizationInformation->SetSelectedTemplate(m_tpSelectedTemplate.Get());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControl::DataTemplateSelectorRecyclingContext::PrepareForItemRecycling(_In_opt_ IInspectable* item)
{
    HRESULT hr = S_OK;

    m_pOwner->SetPtrValue(m_tpDataItem, item);
    m_tpSelectedContainer.Clear();
    m_tpSelectedTemplate.Clear();

    // We should never be called without a TemplateSelector being set but we check anyway
    // As the rest of the code is already resilient to the absence of a selected template
    if (m_tpSelector)
    {
        ctl::ComPtr<xaml::IDataTemplate> spDataTemplate;

        IFC(m_tpSelector->SelectTemplateForItem(item, &spDataTemplate));
        m_pOwner->SetPtrValue(m_tpSelectedTemplate, spDataTemplate.Get());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControl::DataTemplateSelectorRecyclingContext::StopRecycling()
{
    HRESULT hr = S_OK;

    m_tpDataItem.Clear();
    m_tpSelectedContainer.Clear();
    m_tpSelectedTemplate.Clear();

    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControl::DataTemplateSelectorRecyclingContext::IsCompatible(_In_ xaml::IUIElement* candidate, _Out_ BOOLEAN* pReturnValue)
{
    HRESULT hr = S_OK;
    BOOLEAN returnValue = TRUE;

    // If we don't have a selectedTemplate, all containers are considered as "compatible"
    if (m_tpSelectedTemplate.Get())
    {
        ctl::ComPtr<xaml::IUIElement> spCandidate = candidate;
        VirtualizationInformation *pVirtualizationInformation = spCandidate.Cast<UIElement>()->GetVirtualizationInformation();
        if (pVirtualizationInformation == nullptr)
        {
            // The container has never been virtualized : we fallback asking the container for its SelectedDataTemplate
            ctl::ComPtr<xaml_controls::IContentControl> spContentControl;
            ctl::ComPtr<xaml::IDataTemplate> spSelectedDataTemplate;
            IFC(ctl::do_query_interface(spContentControl, candidate));
            IFC(spContentControl.Cast<ContentControl>()->get_SelectedContentTemplate(&spSelectedDataTemplate));
            returnValue = (spSelectedDataTemplate.Get() == m_tpSelectedTemplate.Get());
        }
        else
        {
            returnValue = (pVirtualizationInformation->GetSelectedTemplate().Get() == m_tpSelectedTemplate.Get());
        }
    }
    *pReturnValue = returnValue;
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControl::NotifyDeferredElementStateChanged(
    _In_ KnownPropertyIndex propertyIndex,
    _In_ DeferredElementStateChange state,
    _In_ UINT32 collectionIndex,
    _In_ CDependencyObject* realizedElement)
{
    if (propertyIndex == KnownPropertyIndex::ItemsControl_Items)
    {
        ctl::ComPtr<IInspectable> realizedElementAsIInspectable;
        ctl::ComPtr<DependencyObject> realizedElementDO;
        IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(realizedElement, &realizedElementDO));
        IFC_RETURN(realizedElementDO.As(&realizedElementAsIInspectable));

        switch (state)
        {
            case DeferredElementStateChange::Realized:
                IFC_RETURN(m_tpItems->InsertAt(collectionIndex, realizedElementAsIInspectable.Get()));
                break;

            case DeferredElementStateChange::Deferred:
                {
                    UINT index = 0;
                    BOOLEAN found = FALSE;

                    IFC_RETURN(m_tpItems->IndexOf(
                        realizedElementAsIInspectable.Get(),
                        &index,
                        &found));

                    if (found)
                    {
                        IFC_RETURN(m_tpItems->RemoveAt(index));
                    }
                    // if not found, it's ok.  It might have been removed programatically.
                }
                break;

            default:
                ASSERT(false);
        }
    }
    else
    {
        // Should not be calling framework for any other properties
        ASSERT(false);
    }

    return S_OK;
}

void ItemsControl::TraceVirtualizationEnabledByModernPanel()
{
     if (EventEnabledVirtualizationIsEnabledByModernPanelUsageInfo())
    {
        ctl::ComPtr<IPanel> spItemsHost;
        if (SUCCEEDED(get_ItemsHost(&spItemsHost)) && spItemsHost)
        {
            xstring_ptr panelTypeName = static_cast<Panel*>(spItemsHost.Get())->GetHandle()->GetClassName();
            xstring_ptr elementName = GetHandle()->m_strName;
            xstring_ptr elementTypeName = GetHandle()->GetClassName();
            TraceVirtualizationIsEnabledByModernPanelUsageInfo1(
                ctl::is<IModernCollectionBasePanel>(spItemsHost) || ctl::is<IVirtualizingPanel>(spItemsHost),
                reinterpret_cast<UINT64>(GetHandle()),
                elementName.GetBuffer(),
                elementTypeName.GetBuffer(),
                panelTypeName.GetBuffer()
                );
        }
    }
}
