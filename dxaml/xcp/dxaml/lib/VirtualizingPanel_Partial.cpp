// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "VirtualizingPanel.g.h"
#include "ListViewBase.g.h"
#include "ItemContainerGenerator.g.h"
#include "DispatcherTimer.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// The offset at which off screen elements are arranged.
const XFLOAT VirtualizingPanel::ExtraContainerArrangeOffset = 50000.0f;

VirtualizingPanel::VirtualizingPanel() :
    m_bGeneratorHooked(FALSE),
    m_isGeneratingNewContainers(FALSE),
    m_itemsHostValidatedTick(0),
    m_bShouldMeasureBuffers(FALSE),
    m_IndexToEnsureInView(-1),
    m_fillBufferTimerTickToken(),
    m_LastSetAvailableSize(),
    m_LastSetChildLayoutSlotSize()
{
}

VirtualizingPanel::~VirtualizingPanel()
{
    auto spFillBuffersTimer = m_tpFillBuffersTimer.GetSafeReference();
    if (spFillBuffersTimer)
    {
        ASSERT(m_fillBufferTimerTickToken.value);
        IGNOREHR(spFillBuffersTimer->remove_Tick(m_fillBufferTimerTickToken));
        IGNOREHR(spFillBuffersTimer->Stop());
    }
}

// The generator associated with this panel.
_Check_return_
HRESULT
VirtualizingPanel::GetItemContainerGenerator(
    _Outptr_ xaml_controls::IItemContainerGenerator** pValue,
    _In_opt_ xaml_controls::IItemsControl* pItemsControlHint)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
    ctl::ComPtr<xaml_primitives::IItemsChangedEventHandler> spItemsChangedHandler;

    IFCPTR(pValue);
    *pValue = nullptr;
    IFC(get_Generator(&spGenerator, pItemsControlHint));
    if (spGenerator)
    {
        if (!m_bGeneratorHooked)
        {
            spItemsChangedHandler.Attach(
                new ClassMemberEventHandler<
                VirtualizingPanel,
                xaml_controls::IVirtualizingPanel,
                xaml_primitives::IItemsChangedEventHandler,
                IInspectable,
                xaml_primitives::IItemsChangedEventArgs>(this, &VirtualizingPanel::OnItemsChangedHandler));

            IFC(spGenerator->add_ItemsChanged(spItemsChangedHandler.Get(), &m_ItemsChangedToken));
            IFC(spGenerator->RemoveAll());
        }

        m_bGeneratorHooked = TRUE;
        IFC(spGenerator.CopyTo(pValue));
    }

Cleanup:
    RRETURN(hr);
}

// public api version
_Check_return_ HRESULT VirtualizingPanel::get_ItemContainerGeneratorImpl(
    _Outptr_ xaml_controls::IItemContainerGenerator** pValue)
{
    RRETURN(GetItemContainerGenerator(pValue, NULL));
}

// we don't wont to depent on ItemContainerGenerator type
// thus we allow to create any type to use as generator for item's container
_Check_return_
HRESULT
VirtualizingPanel::get_Generator(
    _Outptr_ xaml_controls::IItemContainerGenerator** pValue,
    _In_opt_ xaml_controls::IItemsControl* pItemsControlHint)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IItemsControl> spOwner;
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;

    IFCPTR(pValue);

    // this method is on a very hot path and getting the ItemsHost is very expensive.
    if (pItemsControlHint)
    {
        // fast path is giving out hints
        spOwner = pItemsControlHint;
    }
    else
    {
        // have to do a slow lookup.
        IFC(ItemsControl::GetItemsOwner(this, &spOwner));
    }

    // TODO: Check owner is NULL
    // Report Error VirtualizingPanel_ItemsControlNotFound
    // Bug#95997
    IFCEXPECT(spOwner);

    IFC(spOwner->get_ItemContainerGenerator(&spGenerator));
    IFC(spGenerator.CopyTo(pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
VirtualizingPanel::OnItemsChangedHandler(
    _In_ IInspectable* sender,
    _In_ xaml_primitives::IItemsChangedEventArgs* args)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsItemHost = FALSE;

    // OnItemsChangedHandler can only be raised from the ItemsContainerGenerator
    // thus if we're not an ItemsHost anymore then there's nothing for this panel
    // todo
    IFC(get_IsItemsHost(&bIsItemHost));
    if (!bIsItemHost)
    {
        goto Cleanup;
    }

    IFC(OnItemsChangedInternal(sender, args));
    IFC(InvalidateMeasure());

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
VirtualizingPanel::OnItemsChangedInternal(
    _In_ IInspectable* sender,
    _In_ xaml_primitives::IItemsChangedEventArgs* args)
{
    int intAction;
    IFC_RETURN(args->get_Action(&intAction));
    const auto action = static_cast<wfc::CollectionChange>(intAction);

    switch (action)
    {
        case wfc::CollectionChange_Reset:
        {
            ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
            XUINT32 nChildrenCount = 0;

            IFC_RETURN(get_Children(&spChildren));

            IFC_RETURN(spChildren->get_Size(&nChildrenCount));
            if (nChildrenCount > 0)
            {
                ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
                IFC_RETURN(spChildren->Clear());
                IFC_RETURN(get_Generator(&spGenerator));
                IFC_RETURN(spGenerator->RemoveAll());
                IFC_RETURN(OnClearChildrenInternal());
            }

            // reset the startup loading optimization
            m_bShouldMeasureBuffers = FALSE;
        }
        break;

        default:
            break;
    }

    IFC_RETURN(OnItemsChangedProtected(sender, args));

    return S_OK;
}

_Check_return_ HRESULT VirtualizingPanel::AddInternalChildImpl(
    _In_ xaml::IUIElement* child)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

    IFC(get_Children(&spChildren));
    IFC(spChildren->Append(child));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT VirtualizingPanel::InsertInternalChildImpl(
    _In_ INT index,
    _In_ xaml::IUIElement* child)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

    IFC(get_Children(&spChildren));
    IFC(spChildren->InsertAt(index, child));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT VirtualizingPanel::RemoveInternalChildRangeImpl(_In_ INT index, _In_ INT range)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

    IFC(get_Children(&spChildren));
    for (INT i = index + range; i > index; )
    {
        IFC(spChildren->RemoveAt(--i));
    }

Cleanup:
    RRETURN(hr);
}

// Called when the Items collection associated with the containing ItemsControl changes.
_Check_return_ HRESULT VirtualizingPanel::OnItemsChangedImpl(
    _In_ IInspectable* sender,
    _In_ xaml_primitives::IItemsChangedEventArgs* args)
{
    // should be overridden by concrete class.
    RRETURN(S_OK);
}

// Called when the UI collection of children is cleared by the base Panel class.
_Check_return_ HRESULT VirtualizingPanel::OnClearChildrenImpl()
{
    // should be overridden by concrete class.
    RRETURN(S_OK);
}

// Generates the item at the specified index and calls BringIntoView on it.
_Check_return_ HRESULT VirtualizingPanel::BringIndexIntoViewImpl(_In_ INT index)
{
    // should be overridden by concrete class.
    RRETURN(S_OK);
}

_Check_return_ HRESULT VirtualizingPanel::OnClearChildrenInternal()
{
    RRETURN(OnClearChildrenProtected());
}

// Get the index of the item from group view.
// Returns the group index + fractional position within the group.
_Check_return_ HRESULT VirtualizingPanel::GetIndexInGroupView(
    _In_ UINT index,
    _Out_ DOUBLE& groupIndex)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
    UINT totalItemsTraversed = 0;
    UINT nCurrentGroupItemCount = 0;
    groupIndex = index;

    IFC(get_ItemContainerGenerator(&spGenerator));
    if(spGenerator.Cast<ItemContainerGenerator>()->m_bIsGrouping)
    {
        ctl::ComPtr<wfc::IVector<IInspectable*>> spView;

        groupIndex = 0;
        IFC(spGenerator.Cast<ItemContainerGenerator>()->get_View(&spView));
        do
        {
            ctl::ComPtr<IInspectable> spCurrentItem;

            IFC(spView->GetAt(static_cast<UINT>(groupIndex), &spCurrentItem));
            IFC(ItemContainerGenerator::GetItemsCount(spCurrentItem.Get(), nCurrentGroupItemCount));
            totalItemsTraversed += nCurrentGroupItemCount;
            groupIndex++;
        }
        while(totalItemsTraversed < index);

        groupIndex--;
        if (nCurrentGroupItemCount > 0)
        {
            groupIndex += (DOUBLE)(index - totalItemsTraversed + nCurrentGroupItemCount)/nCurrentGroupItemCount;
        }
    }

Cleanup:
    RRETURN(hr);
}

// Buffers are not filled during the first measure pass to faster startup.
// Instead we dispatch InvalidateMeasure to trigger filling buffers after first layout is complete.
_Check_return_ HRESULT VirtualizingPanel::SetFillBuffersTimer()
{
    HRESULT hr = S_OK;

    if (!m_bShouldMeasureBuffers)
    {
        m_bShouldMeasureBuffers = TRUE;

        if (m_tpFillBuffersTimer)
        {
            IFC(m_tpFillBuffersTimer->Stop());
        }
        else
        {
            ctl::ComPtr<DispatcherTimer> spNewDispatcherTimer;
            ctl::ComPtr<wf::IEventHandler<IInspectable*>> spFillBufferTimerTickEventHandler;

            IFC(ctl::make<DispatcherTimer>(&spNewDispatcherTimer));

            spFillBufferTimerTickEventHandler.Attach(
                new ClassMemberEventHandler<
                    VirtualizingPanel,
                    xaml_controls::IVirtualizingPanel,
                    wf::IEventHandler<IInspectable*>,
                    IInspectable,
                    IInspectable>(this, &VirtualizingPanel::FillBuffers));

            IFC(spNewDispatcherTimer->add_Tick(spFillBufferTimerTickEventHandler.Get(), &m_fillBufferTimerTickToken));

            wf::TimeSpan showDurationTimeSpan = { 0 };
            IFC(spNewDispatcherTimer->put_Interval(showDurationTimeSpan));

            SetPtrValue(m_tpFillBuffersTimer, spNewDispatcherTimer);
            IFCPTR(m_tpFillBuffersTimer.Get());
        }

        IFC(m_tpFillBuffersTimer->Start());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT VirtualizingPanel::FillBuffers(
    _In_opt_ IInspectable* pUnused1,
    _In_opt_ IInspectable* pUnused2)
{
    HRESULT hr = S_OK;

    IFC(m_tpFillBuffersTimer->Stop());
    IFC(InvalidateMeasure());

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
VirtualizingPanel::SetupItemBoundsClip()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IListViewBase> spListViewBase = NULL;
    ctl::ComPtr<xaml_controls::IItemsControl> spItemsControl = NULL;

    IFC(ItemsControl::GetItemsOwner(this, &spItemsControl));
    if(spItemsControl.Get() != NULL)
    {
        spListViewBase = spItemsControl.AsOrNull<xaml_controls::IListViewBase>();
        if(spListViewBase.Get() != NULL)
        {
            IFC(spListViewBase.Cast<ListViewBase>()->UpdateClip());
        }
    }

Cleanup:
    RRETURN(hr);
}


