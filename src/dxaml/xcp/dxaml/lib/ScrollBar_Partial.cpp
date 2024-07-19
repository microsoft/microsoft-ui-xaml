// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ScrollBar.g.h"
#include "ScrollBarAutomationPeer.g.h"
#include "Thumb.g.h"
#include "RepeatButton.g.h"
#include "ScrollEventArgs.g.h"
#include "AutomationProperties.h"
#include "localizedResource.h"
#include "LayoutCycleDebugSettings.h"

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

using namespace DirectUI;
using namespace DirectUISynonyms;

// Initializes a new instance of the ScrollBar class.
ScrollBar::ScrollBar()
    : m_isIgnoringUserInput(FALSE)
    , m_isPointerOver(FALSE)
    , m_suspendVisualStateUpdates(FALSE)
    , m_dragValue(0.0)
    , m_blockIndicators(FALSE)
    , m_isUsingActualSizeAsExtent(false)
{
}

// Prepares object's state
_Check_return_
HRESULT
ScrollBar::Initialize()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::ISizeChangedEventHandler> spSizeChangedHandler;
    EventRegistrationToken sizeChangedToken;

    IFC(ScrollBarGenerated::Initialize());

    spSizeChangedHandler.Attach(
        new ClassMemberEventHandler<
            ScrollBar,
            xaml_primitives::IScrollBar,
            xaml::ISizeChangedEventHandler,
            IInspectable,
            xaml::ISizeChangedEventArgs>(this, &ScrollBar::OnSizeChanged, true /* subscribingToSelf */ ));
    IFC(add_SizeChanged(spSizeChangedHandler.Get(), &sizeChangedToken));

    DXamlCore::GetCurrent()->RegisterForChangeVisualStateOnDynamicScrollbarsSettingChanged(this);

Cleanup:
    RRETURN(hr);
}

ScrollBar::~ScrollBar()
{
    if (auto dxamlCore = DXamlCore::GetCurrent())
    {
        dxamlCore->UnregisterFromDynamicScrollbarsSettingChanged(this);
    }
}

_Check_return_ HRESULT ScrollBar::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(ScrollBarGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::ScrollBar_Orientation:
            IFC(OnOrientationChanged());
            break;
        case KnownPropertyIndex::ScrollBar_IndicatorMode:
            IFC(RefreshTrackLayout());
            break;
        case KnownPropertyIndex::UIElement_Visibility:
            IFC(OnVisibilityChanged());
            break;
    }
Cleanup:
    RRETURN(hr);
}

// Update the visual states when the Visibility property is changed.
_Check_return_ HRESULT
ScrollBar::OnVisibilityChanged()
{
    HRESULT hr = S_OK;
    xaml::Visibility visibility = xaml::Visibility_Collapsed;

    IFC(get_Visibility(&visibility));
    if (xaml::Visibility_Visible != visibility)
    {
        m_isPointerOver = FALSE;
    }

    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// Apply a template to the ScrollBar.
IFACEMETHODIMP ScrollBar::OnApplyTemplate()
{
    HRESULT hr = S_OK;

    wrl_wrappers::HString strAutomationName;
    m_suspendVisualStateUpdates = TRUE;

    ctl::ComPtr<xaml_primitives::IDragStartedEventHandler> spDragStartedHandler;
    ctl::ComPtr<xaml_primitives::IDragDeltaEventHandler> spDragDeltaHandler;
    ctl::ComPtr<xaml_primitives::IDragCompletedEventHandler> spDragCompletedHandler;
    ctl::ComPtr<xaml::IRoutedEventHandler> spLargeIncreaseClickHandler;
    ctl::ComPtr<xaml::IRoutedEventHandler> spLargeDecreaseClickHandler;
    ctl::ComPtr<xaml::IRoutedEventHandler> spSmallIncreaseClickHandler;
    ctl::ComPtr<xaml::IRoutedEventHandler> spSmallDecreaseClickHandler;

    ctl::ComPtr<IFrameworkElement> spElementHorizontalTemplate;
    ctl::ComPtr<IFrameworkElement> spElementVerticalTemplate;
    ctl::ComPtr<IFrameworkElement> spElementHorizontalPanningRoot;
    ctl::ComPtr<IFrameworkElement> spElementHorizontalPanningThumb;
    ctl::ComPtr<IFrameworkElement> spElementVerticalPanningRoot;
    ctl::ComPtr<IFrameworkElement> spElementVerticalPanningThumb;

    ctl::ComPtr<IRepeatButton> spElementHorizontalLargeIncrease;
    ctl::ComPtr<IRepeatButton> spElementHorizontalLargeDecrease;
    ctl::ComPtr<IRepeatButton> spElementHorizontalSmallIncrease;
    ctl::ComPtr<IRepeatButton> spElementHorizontalSmallDecrease;

    ctl::ComPtr<IRepeatButton> spElementVerticalLargeIncrease;
    ctl::ComPtr<IRepeatButton> spElementVerticalLargeDecrease;
    ctl::ComPtr<IRepeatButton> spElementVerticalSmallIncrease;
    ctl::ComPtr<IRepeatButton> spElementVerticalSmallDecrease;

    ctl::ComPtr<IThumb> spElementVerticalThumb;
    ctl::ComPtr<IThumb> spElementHorizontalThumb;

    // Cleanup any existing template parts
    if (m_tpElementHorizontalThumb)
    {
        IFC(m_tpElementHorizontalThumb.Cast<Thumb>()->remove_DragStarted(m_ElementHorizontalThumbDragStartedToken));
        IFC(m_tpElementHorizontalThumb.Cast<Thumb>()->remove_DragDelta(m_ElementHorizontalThumbDragDeltaToken));
        IFC(m_tpElementHorizontalThumb.Cast<Thumb>()->remove_DragCompleted(m_ElementHorizontalThumbDragCompletedToken));
    }
    if (m_tpElementHorizontalLargeDecrease)
    {
        IFC(m_tpElementHorizontalLargeDecrease.Cast<RepeatButton>()->remove_Click(m_ElementHorizontalLargeDecreaseClickToken));
    }
    if (m_tpElementHorizontalLargeIncrease)
    {
        IFC(m_tpElementHorizontalLargeIncrease.Cast<RepeatButton>()->remove_Click(m_ElementHorizontalLargeIncreaseClickToken));
    }
    if (m_tpElementHorizontalSmallDecrease)
    {
        IFC(m_tpElementHorizontalSmallDecrease.Cast<RepeatButton>()->remove_Click(m_ElementHorizontalSmallDecreaseClickToken));
    }
    if (m_tpElementHorizontalSmallIncrease)
    {
        IFC(m_tpElementHorizontalSmallIncrease.Cast<RepeatButton>()->remove_Click(m_ElementHorizontalSmallIncreaseClickToken));
    }
    if (m_tpElementVerticalThumb)
    {
        IFC(m_tpElementVerticalThumb.Cast<Thumb>()->remove_DragStarted(m_ElementVerticalThumbDragStartedToken));
        IFC(m_tpElementVerticalThumb.Cast<Thumb>()->remove_DragDelta(m_ElementVerticalThumbDragDeltaToken));
        IFC(m_tpElementVerticalThumb.Cast<Thumb>()->remove_DragCompleted(m_ElementVerticalThumbDragCompletedToken));
    }
    if (m_tpElementVerticalLargeDecrease)
    {
        IFC(m_tpElementVerticalLargeDecrease.Cast<RepeatButton>()->remove_Click(m_ElementVerticalLargeDecreaseClickToken));
    }
    if (m_tpElementVerticalLargeIncrease)
    {
        IFC(m_tpElementVerticalLargeIncrease.Cast<RepeatButton>()->remove_Click(m_ElementVerticalLargeIncreaseClickToken));
    }
    if (m_tpElementVerticalSmallDecrease)
    {
        IFC(m_tpElementVerticalSmallDecrease.Cast<RepeatButton>()->remove_Click(m_ElementVerticalSmallDecreaseClickToken));
    }
    if (m_tpElementVerticalSmallIncrease)
    {
        IFC(m_tpElementVerticalSmallIncrease.Cast<RepeatButton>()->remove_Click(m_ElementVerticalSmallIncreaseClickToken));
    }
    m_tpElementHorizontalTemplate.Clear();
    m_tpElementHorizontalLargeIncrease.Clear();
    m_tpElementHorizontalLargeDecrease.Clear();
    m_tpElementHorizontalSmallIncrease.Clear();
    m_tpElementHorizontalSmallDecrease.Clear();
    m_tpElementHorizontalThumb.Clear();
    m_tpElementVerticalTemplate.Clear();
    m_tpElementVerticalLargeIncrease.Clear();
    m_tpElementVerticalLargeDecrease.Clear();
    m_tpElementVerticalSmallIncrease.Clear();
    m_tpElementVerticalThumb.Clear();
    m_tpElementVerticalSmallDecrease.Clear();
    m_tpElementHorizontalPanningRoot.Clear();
    m_tpElementHorizontalPanningThumb.Clear();
    m_tpElementVerticalPanningRoot.Clear();
    m_tpElementVerticalPanningThumb.Clear();

    // Apply the template to the base class
    IFC(ScrollBarGenerated::OnApplyTemplate());

    // Get the parts
    IFC(GetTemplateChildHelper<xaml::IFrameworkElement>(STR_LEN_PAIR(L"HorizontalRoot"), spElementHorizontalTemplate.ReleaseAndGetAddressOf()));
    SetPtrValue(m_tpElementHorizontalTemplate, spElementHorizontalTemplate);
    IFC(GetTemplateChildHelper<xaml_primitives::IRepeatButton>(STR_LEN_PAIR(L"HorizontalLargeIncrease"), spElementHorizontalLargeIncrease.ReleaseAndGetAddressOf()));
    SetPtrValue(m_tpElementHorizontalLargeIncrease, spElementHorizontalLargeIncrease);
    if (m_tpElementHorizontalLargeIncrease)
    {
        IFC(DirectUI::AutomationProperties::GetNameStatic(m_tpElementHorizontalLargeIncrease.Cast<RepeatButton>(), strAutomationName.ReleaseAndGetAddressOf()))
        if (strAutomationName.Get() == nullptr)
        {
            IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_SCROLLBAR_HORIZONTALLARGEINCREASE, strAutomationName.ReleaseAndGetAddressOf()));
            IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpElementHorizontalLargeIncrease.Cast<RepeatButton>(), strAutomationName.Get()))
        }
    }
    IFC(GetTemplateChildHelper<xaml_primitives::IRepeatButton>(STR_LEN_PAIR(L"HorizontalSmallIncrease"), spElementHorizontalSmallIncrease.ReleaseAndGetAddressOf()));
    SetPtrValue(m_tpElementHorizontalSmallIncrease, spElementHorizontalSmallIncrease);
    if (m_tpElementHorizontalSmallIncrease)
    {
        IFC(DirectUI::AutomationProperties::GetNameStatic(m_tpElementHorizontalSmallIncrease.Cast<RepeatButton>(), strAutomationName.ReleaseAndGetAddressOf()))
        if (strAutomationName.Get() == nullptr)
        {
            IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_SCROLLBAR_HORIZONTALSMALLINCREASE, strAutomationName.ReleaseAndGetAddressOf()));
            IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpElementHorizontalSmallIncrease.Cast<RepeatButton>(), strAutomationName.Get()))
        }
    }
    IFC(GetTemplateChildHelper<xaml_primitives::IRepeatButton>(STR_LEN_PAIR(L"HorizontalLargeDecrease"), spElementHorizontalLargeDecrease.ReleaseAndGetAddressOf()));
    SetPtrValue(m_tpElementHorizontalLargeDecrease, spElementHorizontalLargeDecrease);
    if (m_tpElementHorizontalLargeDecrease)
    {
        IFC(DirectUI::AutomationProperties::GetNameStatic(m_tpElementHorizontalLargeDecrease.Cast<RepeatButton>(), strAutomationName.ReleaseAndGetAddressOf()))
        if (strAutomationName.Get() == nullptr)
        {
            IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_SCROLLBAR_HORIZONTALLARGEDECREASE, strAutomationName.ReleaseAndGetAddressOf()));
            IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpElementHorizontalLargeDecrease.Cast<RepeatButton>(), strAutomationName.Get()))
        }
    }
    IFC(GetTemplateChildHelper<xaml_primitives::IRepeatButton>(STR_LEN_PAIR(L"HorizontalSmallDecrease"), spElementHorizontalSmallDecrease.ReleaseAndGetAddressOf()));
    SetPtrValue(m_tpElementHorizontalSmallDecrease, spElementHorizontalSmallDecrease);
    if (m_tpElementHorizontalSmallDecrease)
    {
        IFC(DirectUI::AutomationProperties::GetNameStatic(m_tpElementHorizontalSmallDecrease.Cast<RepeatButton>(), strAutomationName.ReleaseAndGetAddressOf()))
        if (strAutomationName.Get() == nullptr)
        {
            IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_SCROLLBAR_HORIZONTALSMALLDECREASE, strAutomationName.ReleaseAndGetAddressOf()));
            IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpElementHorizontalSmallDecrease.Cast<RepeatButton>(), strAutomationName.Get()))
        }
    }
    IFC(GetTemplateChildHelper<xaml_primitives::IThumb>(STR_LEN_PAIR(L"HorizontalThumb"), spElementHorizontalThumb.ReleaseAndGetAddressOf()));
    SetPtrValue(m_tpElementHorizontalThumb, spElementHorizontalThumb);
    if (m_tpElementHorizontalThumb)
    {
        IFC(DirectUI::AutomationProperties::GetNameStatic(m_tpElementHorizontalThumb.Cast<Thumb>(), strAutomationName.ReleaseAndGetAddressOf()))
        if (strAutomationName.Get() == nullptr)
        {
            IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_SCROLLBAR_HORIZONTALTHUMB, strAutomationName.ReleaseAndGetAddressOf()));
            IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpElementHorizontalThumb.Cast<Thumb>(), strAutomationName.Get()))
        }
    }

    IFC(GetTemplateChildHelper<xaml::IFrameworkElement>(STR_LEN_PAIR(L"VerticalRoot"), spElementVerticalTemplate.ReleaseAndGetAddressOf()));
    SetPtrValue(m_tpElementVerticalTemplate, spElementVerticalTemplate);

    IFC(GetTemplateChildHelper<xaml_primitives::IRepeatButton>(STR_LEN_PAIR(L"VerticalLargeIncrease"), spElementVerticalLargeIncrease.ReleaseAndGetAddressOf()));
    SetPtrValue(m_tpElementVerticalLargeIncrease, spElementVerticalLargeIncrease);
    if (m_tpElementVerticalLargeIncrease)
    {
        IFC(DirectUI::AutomationProperties::GetNameStatic(m_tpElementVerticalLargeIncrease.Cast<RepeatButton>(), strAutomationName.ReleaseAndGetAddressOf()))
        if (strAutomationName.Get() == nullptr)
        {
            IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_SCROLLBAR_VERTICALALLARGEINCREASE, strAutomationName.ReleaseAndGetAddressOf()));
            IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpElementVerticalLargeIncrease.Cast<RepeatButton>(), strAutomationName.Get()))
        }
    }
    IFC(GetTemplateChildHelper<xaml_primitives::IRepeatButton>(STR_LEN_PAIR(L"VerticalSmallIncrease"), spElementVerticalSmallIncrease.ReleaseAndGetAddressOf()));
    SetPtrValue(m_tpElementVerticalSmallIncrease, spElementVerticalSmallIncrease);
    if (m_tpElementVerticalSmallIncrease)
    {
        IFC(DirectUI::AutomationProperties::GetNameStatic(m_tpElementVerticalSmallIncrease.Cast<RepeatButton>(), strAutomationName.ReleaseAndGetAddressOf()))
        if (strAutomationName.Get() == nullptr)
        {
            IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_SCROLLBAR_VERTICALSMALLINCREASE, strAutomationName.ReleaseAndGetAddressOf()));
            IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpElementVerticalSmallIncrease.Cast<RepeatButton>(), strAutomationName.Get()))
        }
    }
    IFC(GetTemplateChildHelper<xaml_primitives::IRepeatButton>(STR_LEN_PAIR(L"VerticalLargeDecrease"), spElementVerticalLargeDecrease.ReleaseAndGetAddressOf()));
    SetPtrValue(m_tpElementVerticalLargeDecrease, spElementVerticalLargeDecrease);
    if (m_tpElementVerticalLargeDecrease)
    {
        IFC(DirectUI::AutomationProperties::GetNameStatic(m_tpElementVerticalLargeDecrease.Cast<RepeatButton>(), strAutomationName.ReleaseAndGetAddressOf()))
        if (strAutomationName.Get() == nullptr)
        {
            IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_SCROLLBAR_VERTICALLARGEDECREASE, strAutomationName.ReleaseAndGetAddressOf()));
            IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpElementVerticalLargeDecrease.Cast<RepeatButton>(), strAutomationName.Get()))
        }
    }
    IFC(GetTemplateChildHelper<xaml_primitives::IRepeatButton>(STR_LEN_PAIR(L"VerticalSmallDecrease"), spElementVerticalSmallDecrease.ReleaseAndGetAddressOf()));
    SetPtrValue(m_tpElementVerticalSmallDecrease, spElementVerticalSmallDecrease);
    if (m_tpElementVerticalSmallDecrease)
    {
        IFC(DirectUI::AutomationProperties::GetNameStatic(m_tpElementVerticalSmallDecrease.Cast<RepeatButton>(), strAutomationName.ReleaseAndGetAddressOf()))
        if (strAutomationName.Get() == nullptr)
        {
            IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_SCROLLBAR_VERTICALSMALLDECREASE, strAutomationName.ReleaseAndGetAddressOf()));
            IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpElementVerticalSmallDecrease.Cast<RepeatButton>(), strAutomationName.Get()))
        }
    }
    IFC(GetTemplateChildHelper<xaml_primitives::IThumb>(STR_LEN_PAIR(L"VerticalThumb"), spElementVerticalThumb.ReleaseAndGetAddressOf()));
    SetPtrValue(m_tpElementVerticalThumb, spElementVerticalThumb);
    if (m_tpElementVerticalThumb)
    {
        IFC(DirectUI::AutomationProperties::GetNameStatic(m_tpElementVerticalThumb.Cast<Thumb>(), strAutomationName.ReleaseAndGetAddressOf()))
        if (strAutomationName.Get() == nullptr)
        {
            IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_SCROLLBAR_VERTICALTHUMB, strAutomationName.ReleaseAndGetAddressOf()));
            IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpElementVerticalThumb.Cast<Thumb>(), strAutomationName.Get()))
        }
    }

    IFC(GetTemplateChildHelper<xaml::IFrameworkElement>(STR_LEN_PAIR(L"HorizontalPanningRoot"), spElementHorizontalPanningRoot.ReleaseAndGetAddressOf()));
    SetPtrValue(m_tpElementHorizontalPanningRoot, spElementHorizontalPanningRoot);
    IFC(GetTemplateChildHelper<xaml::IFrameworkElement>(STR_LEN_PAIR(L"HorizontalPanningThumb"), spElementHorizontalPanningThumb.ReleaseAndGetAddressOf()));
    SetPtrValue(m_tpElementHorizontalPanningThumb, spElementHorizontalPanningThumb);
    IFC(GetTemplateChildHelper<xaml::IFrameworkElement>(STR_LEN_PAIR(L"VerticalPanningRoot"), spElementVerticalPanningRoot.ReleaseAndGetAddressOf()));
    SetPtrValue(m_tpElementVerticalPanningRoot, spElementVerticalPanningRoot);
    IFC(GetTemplateChildHelper<xaml::IFrameworkElement>(STR_LEN_PAIR(L"VerticalPanningThumb"), spElementVerticalPanningThumb.ReleaseAndGetAddressOf()));
    SetPtrValue(m_tpElementVerticalPanningThumb, spElementVerticalPanningThumb);

    // Attach the event handlers
    if (m_tpElementHorizontalThumb || m_tpElementVerticalThumb)
    {
        spDragStartedHandler.Attach(
            new ClassMemberEventHandler<
                ScrollBar,
                xaml_primitives::IScrollBar,
                xaml_primitives::IDragStartedEventHandler,
                IInspectable,
                xaml_primitives::IDragStartedEventArgs>(this, &ScrollBar::OnThumbDragStarted));

        spDragDeltaHandler.Attach(
            new ClassMemberEventHandler<
                ScrollBar,
                xaml_primitives::IScrollBar,
                xaml_primitives::IDragDeltaEventHandler,
                IInspectable,
                xaml_primitives::IDragDeltaEventArgs>(this, &ScrollBar::OnThumbDragDelta));

        spDragCompletedHandler.Attach(
            new ClassMemberEventHandler<
                ScrollBar,
                xaml_primitives::IScrollBar,
                xaml_primitives::IDragCompletedEventHandler,
                IInspectable,
                xaml_primitives::IDragCompletedEventArgs>(this, &ScrollBar::OnThumbDragCompleted));

        if (m_tpElementHorizontalThumb)
        {
            IFC(m_tpElementHorizontalThumb.Cast<Thumb>()->add_DragStarted(spDragStartedHandler.Get(), &m_ElementHorizontalThumbDragStartedToken));
            IFC(m_tpElementHorizontalThumb.Cast<Thumb>()->add_DragDelta(spDragDeltaHandler.Get(), &m_ElementHorizontalThumbDragDeltaToken));
            IFC(m_tpElementHorizontalThumb.Cast<Thumb>()->add_DragCompleted(spDragCompletedHandler.Get(), &m_ElementHorizontalThumbDragCompletedToken));
            IFC(m_tpElementHorizontalThumb.Cast<Thumb>()->put_IgnoreTouchInput(TRUE));
        }

        if (m_tpElementVerticalThumb)
        {
            IFC(m_tpElementVerticalThumb.Cast<Thumb>()->add_DragStarted(spDragStartedHandler.Get(), &m_ElementVerticalThumbDragStartedToken));
            IFC(m_tpElementVerticalThumb.Cast<Thumb>()->add_DragDelta(spDragDeltaHandler.Get(), &m_ElementVerticalThumbDragDeltaToken));
            IFC(m_tpElementVerticalThumb.Cast<Thumb>()->add_DragCompleted(spDragCompletedHandler.Get(), &m_ElementVerticalThumbDragCompletedToken));
            IFC(m_tpElementVerticalThumb.Cast<Thumb>()->put_IgnoreTouchInput(TRUE));
        }
    }

    if (m_tpElementHorizontalLargeDecrease || m_tpElementVerticalLargeDecrease)
    {
        spLargeDecreaseClickHandler.Attach(
            new ClassMemberEventHandler<
                ScrollBar,
                xaml_primitives::IScrollBar,
                xaml::IRoutedEventHandler,
                IInspectable,
                xaml::IRoutedEventArgs>(this, &ScrollBar::LargeDecrement));

        if (m_tpElementHorizontalLargeDecrease)
        {
            IFC(m_tpElementHorizontalLargeDecrease.Cast<RepeatButton>()->add_Click(spLargeDecreaseClickHandler.Get(), &m_ElementHorizontalLargeDecreaseClickToken));
            IFC(m_tpElementHorizontalLargeDecrease.Cast<RepeatButton>()->put_IgnoreTouchInput(TRUE));
        }

        if (m_tpElementVerticalLargeDecrease)
        {
            IFC(m_tpElementVerticalLargeDecrease.Cast<RepeatButton>()->add_Click(spLargeDecreaseClickHandler.Get(), &m_ElementVerticalLargeDecreaseClickToken));
            IFC(m_tpElementVerticalLargeDecrease.Cast<RepeatButton>()->put_IgnoreTouchInput(TRUE));
        }
    }

    if (m_tpElementHorizontalLargeIncrease || m_tpElementVerticalLargeIncrease)
    {
        spLargeIncreaseClickHandler.Attach(
            new ClassMemberEventHandler<
                ScrollBar,
                xaml_primitives::IScrollBar,
                xaml::IRoutedEventHandler,
                IInspectable,
                xaml::IRoutedEventArgs>(this, &ScrollBar::LargeIncrement));

        if (m_tpElementHorizontalLargeIncrease)
        {
            IFC(m_tpElementHorizontalLargeIncrease.Cast<RepeatButton>()->add_Click(spLargeIncreaseClickHandler.Get(), &m_ElementHorizontalLargeIncreaseClickToken));
            IFC(m_tpElementHorizontalLargeIncrease.Cast<RepeatButton>()->put_IgnoreTouchInput(TRUE));
        }

        if (m_tpElementVerticalLargeIncrease)
        {
            IFC(m_tpElementVerticalLargeIncrease.Cast<RepeatButton>()->add_Click(spLargeIncreaseClickHandler.Get(), &m_ElementVerticalLargeIncreaseClickToken));
            IFC(m_tpElementVerticalLargeIncrease.Cast<RepeatButton>()->put_IgnoreTouchInput(TRUE));
        }
    }

    if (m_tpElementHorizontalSmallDecrease || m_tpElementVerticalSmallDecrease)
    {
        spSmallDecreaseClickHandler.Attach(
            new ClassMemberEventHandler<
                ScrollBar,
                xaml_primitives::IScrollBar,
                xaml::IRoutedEventHandler,
                IInspectable,
                xaml::IRoutedEventArgs>(this, &ScrollBar::SmallDecrement));

        if (m_tpElementHorizontalSmallDecrease)
        {
            IFC(m_tpElementHorizontalSmallDecrease.Cast<RepeatButton>()->add_Click(spSmallDecreaseClickHandler.Get(), &m_ElementHorizontalSmallDecreaseClickToken));
            IFC(m_tpElementHorizontalSmallDecrease.Cast<RepeatButton>()->put_IgnoreTouchInput(TRUE));
        }

        if (m_tpElementVerticalSmallDecrease)
        {
            IFC(m_tpElementVerticalSmallDecrease.Cast<RepeatButton>()->add_Click(spSmallDecreaseClickHandler.Get(), &m_ElementVerticalSmallDecreaseClickToken));
            IFC(m_tpElementVerticalSmallDecrease.Cast<RepeatButton>()->put_IgnoreTouchInput(TRUE));
        }
    }
    if (m_tpElementHorizontalSmallIncrease || m_tpElementVerticalSmallIncrease)
    {
        spSmallIncreaseClickHandler.Attach(
            new ClassMemberEventHandler<
                ScrollBar,
                xaml_primitives::IScrollBar,
                xaml::IRoutedEventHandler,
                IInspectable,
                xaml::IRoutedEventArgs>(this, &ScrollBar::SmallIncrement));

        if (m_tpElementHorizontalSmallIncrease)
        {
            IFC(m_tpElementHorizontalSmallIncrease.Cast<RepeatButton>()->add_Click(spSmallIncreaseClickHandler.Get(), &m_ElementHorizontalSmallIncreaseClickToken));
            IFC(m_tpElementHorizontalSmallIncrease.Cast<RepeatButton>()->put_IgnoreTouchInput(TRUE));
        }

        if (m_tpElementVerticalSmallIncrease)
        {
            IFC(m_tpElementVerticalSmallIncrease.Cast<RepeatButton>()->add_Click(spSmallIncreaseClickHandler.Get(), &m_ElementVerticalSmallIncreaseClickToken));
            IFC(m_tpElementVerticalSmallIncrease.Cast<RepeatButton>()->put_IgnoreTouchInput(TRUE));
        }
    }

    // Updating states for parts where properties might have been updated
    // through XAML before the template was loaded.
    IFC(UpdateScrollBarVisibility());

    m_suspendVisualStateUpdates = FALSE;
    IFC(ChangeVisualState(false));

Cleanup:
    m_suspendVisualStateUpdates = FALSE;
    RRETURN(hr);
}

// Retrieves a reference to a child template object given its name
template<class TInterface, class TRuntime>
_Check_return_ HRESULT
ScrollBar::GetTemplateChildHelper(
    _In_reads_(cName) WCHAR* pName,
    _In_ size_t cName,
    _Outptr_ TRuntime** ppReference)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spElement;
    wrl_wrappers::HString strName;

    // Validate parameters
    IFCPTR(pName);
    IFCPTR(ppReference);

    // Perform the lookup and cast the result
    IFC(strName.Set(pName, cName));
    IFC(GetTemplateChild(strName.Get(), spElement.ReleaseAndGetAddressOf()));
    IFC(spElement.CopyTo(ppReference));

Cleanup:
    RRETURN(hr);
}


// IsEnabled property changed handler.
_Check_return_ HRESULT ScrollBar::OnIsEnabledChanged(
    _In_ IsEnabledChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isEnabled = FALSE;

    IFC(ScrollBarGenerated::OnIsEnabledChanged(pArgs));

    IFC(get_IsEnabled(&isEnabled));
    if (!isEnabled)
    {
        m_isPointerOver = FALSE;
    }

    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// PointerEnter event handler.
IFACEMETHODIMP ScrollBar::OnPointerEntered(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isDragging = FALSE;

    IFC(ScrollBarGenerated::OnPointerEntered(pArgs));
    m_isPointerOver = TRUE;

    IFC(get_IsDragging(&isDragging));
    if (!isDragging)
    {
        IFC(UpdateVisualState());
    }

Cleanup:
    RRETURN(hr);
}

// PointerExited event handler.
IFACEMETHODIMP ScrollBar::OnPointerExited(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isDragging = FALSE;

    IFC(ScrollBarGenerated::OnPointerExited(pArgs));
    m_isPointerOver = FALSE;

    IFC(get_IsDragging(&isDragging));
    if (!isDragging)
    {
        IFC(UpdateVisualState());
    }

Cleanup:
    RRETURN(hr);
}

// PointerPressed event handler.
IFACEMETHODIMP ScrollBar::OnPointerPressed(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
    ctl::ComPtr<ixp::IPointerPointProperties> spPointerProperties;
    BOOLEAN handled = FALSE;
    BOOLEAN captured = FALSE;
    BOOLEAN bIsLeftButtonPressed = FALSE;

    IFC(ScrollBarGenerated::OnPointerPressed(pArgs));

    IFC(pArgs->get_Handled(&handled));

    IFC(pArgs->GetCurrentPoint(this, &spPointerPoint));
    IFCPTR(spPointerPoint);
    IFC(spPointerPoint->get_Properties(&spPointerProperties));
    IFCPTR(spPointerProperties);
    IFC(spPointerProperties->get_IsLeftButtonPressed(&bIsLeftButtonPressed));

    if (bIsLeftButtonPressed)
    {
        ctl::ComPtr<xaml_input::IPointer> spPointer;
        if (!handled)
        {
            IFC(pArgs->put_Handled(TRUE));
            IFC(pArgs->get_Pointer(&spPointer));
            IFC(CapturePointer(spPointer.Get(), &captured));
        }
    }

Cleanup:
    RRETURN(hr);
}

// PointerReleased event handler.
IFACEMETHODIMP ScrollBar::OnPointerReleased(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN handled = FALSE;

    IFC(ScrollBarGenerated::OnPointerReleased(pArgs));

    IFC(pArgs->get_Handled(&handled));
    if (!handled)
    {
        IFC(pArgs->put_Handled(TRUE));
    }

Cleanup:
    RRETURN(hr);
}

/// PointerCaptureLost event handler.
IFACEMETHODIMP ScrollBar::OnPointerCaptureLost(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(ScrollBarGenerated::OnPointerCaptureLost(pArgs));
    IFC(UpdateVisualState(TRUE));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ScrollBar::OnDoubleTapped(
    _In_ xaml_input::IDoubleTappedRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(ScrollBarGenerated::OnDoubleTapped(pArgs));
    IFC(pArgs->put_Handled(TRUE));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ScrollBar::OnTapped(
    _In_ xaml_input::ITappedRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(ScrollBarGenerated::OnTapped(pArgs));
    IFC(pArgs->put_Handled(TRUE));

Cleanup:
    RRETURN(hr);
}


// Create ScrollBarAutomationPeer to represent the ScrollBar.
IFACEMETHODIMP ScrollBar::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IScrollBarAutomationPeer> spScrollBarAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IScrollBarAutomationPeerFactory> spScrollBarAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::ScrollBarAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spScrollBarAPFactory));

    IFC(spScrollBarAPFactory.Cast<ScrollBarAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spScrollBarAutomationPeer));
    IFC(spScrollBarAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

// Change to the correct visual state for the button.
_Check_return_ HRESULT ScrollBar::ChangeVisualState(
    // true to use transitions when updating the visual state, false
    // to snap directly to the new visual state.
    _In_ bool bUseTransitions)
{
    HRESULT hr = S_OK;
    BOOLEAN isEnabled = FALSE;
    BOOLEAN isIgnored = FALSE;
    BOOLEAN isSuccessful = FALSE;
    xaml_primitives::ScrollingIndicatorMode scrollingIndicator = xaml_primitives::ScrollingIndicatorMode_None;

    if (m_suspendVisualStateUpdates)
    {
        goto Cleanup;
    }

    IFC(ScrollBarGenerated::ChangeVisualState(bUseTransitions));

    IFC(get_IndicatorMode(&scrollingIndicator));

    IFC(get_IsEnabled(&isEnabled));
    if (!isEnabled)
    {
        IFC(GoToState(bUseTransitions, L"Disabled", &isIgnored));
    }
    else if (m_isPointerOver)
    {
        IFC(GoToState(bUseTransitions, L"PointerOver", &isSuccessful));
        //Default to Normal if PointerOver state isn't available.
        if (!isSuccessful)
        {
            IFC(GoToState(bUseTransitions, L"Normal", &isIgnored));
        }
    }
    else
    {
        IFC(GoToState(bUseTransitions, L"Normal", &isIgnored));
    }

    if (!m_blockIndicators && (!IsConscious() || scrollingIndicator == xaml_primitives::ScrollingIndicatorMode_MouseIndicator))
    {
        IFC(GoToState(bUseTransitions, L"MouseIndicator", &isIgnored));
    }
    else if (!m_blockIndicators && scrollingIndicator==xaml_primitives::ScrollingIndicatorMode_TouchIndicator)
    {
        IFC(GoToState(bUseTransitions, L"TouchIndicator", &isSuccessful));
        //Default to MouseActiveState if Panning state isn't available.
        if (!isSuccessful)
        {
            IFC(GoToState(bUseTransitions, L"MouseIndicator", &isIgnored));
        }
    }
    else
    {
        IFC(GoToState(bUseTransitions, L"NoIndicator", &isIgnored));
    }

    // Expanded/Collapsed States were added in RS3 and ExpandedWithoutAnimation/CollapsedWithoutAnimation states
    // were added in RS4. Since Expanded can exist without ExpandedWithoutAnimation (and the same for collapsed)
    // each time we try to transition  to a *WithoutAnimation state we need to check to make sure the transition was
    // successful. If it was not we fallback to the appropriate expanded or collapsed state.
    // UseTransitions is always true since the delay behavior is defined in the transitions when
    // animations are enabled. When animations are disabled, the framework does not run transitions.
    if (!IsConscious())
    {
        IFC(GoToState(true /* useTransitions */, isEnabled ? L"Expanded" : L"Collapsed", &isIgnored));
    }
    else
    {
        isSuccessful = FALSE;
        bool animate = false;
        IGNOREHR(DXamlCore::GetCurrent()->IsAnimationEnabled(&animate));

        if (isEnabled && m_isPointerOver)
        {
            if (!animate)
            {
                IFC(GoToState(true /* useTransitions */, L"ExpandedWithoutAnimation", &isSuccessful));
            }
            if (!isSuccessful)
            {
                IFC(GoToState(true /* useTransitions */, L"Expanded", &isIgnored));
            }
        }
        else
        {
            if (!animate)
            {
                IFC(GoToState(true /* useTransitions */, L"CollapsedWithoutAnimation", &isSuccessful));
            }
            if (!isSuccessful)
            {
                IFC(GoToState(true /* useTransitions */, L"Collapsed", &isIgnored));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Returns the actual length of the ScrollBar in the direction of its orientation.
_Check_return_ HRESULT ScrollBar::GetTrackLength(
    _Out_ DOUBLE* pLength)
{
    HRESULT hr = S_OK;
    DOUBLE length = DoubleUtil::NaN;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;

    IFCPTR(pLength);
    *pLength = length;

    IFC(get_Orientation(&orientation));
    if (orientation == xaml_controls::Orientation_Horizontal)
    {
        IFC(get_ActualWidth(&length));
    }
    else
    {
        IFC(get_ActualHeight(&length));
    }

    //
    // Set the track length as zero which is collapsed state if the length is greater than
    // the current viewport size in case of the layout is dirty with using actual size(Width/Height)
    // as the extent. The invalid track length setting will cause of updating the new layout size
    // on the ScrollViewer and ScrollContentPresenter that will keep to ask ScrollBar for updating
    // the track length continuously which is a layout cycle crash.
    //
    if ((m_isUsingActualSizeAsExtent) &&
        (static_cast<CUIElement*>(GetHandle())->GetIsMeasureDirty() ||
        static_cast<CUIElement*>(GetHandle())->GetIsOnMeasureDirtyPath() ||
        static_cast<CUIElement*>(GetHandle())->GetIsArrangeDirty() ||
        static_cast<CUIElement*>(GetHandle())->GetIsOnArrangeDirtyPath()))
    {
        DOUBLE viewport = DoubleUtil::NaN;

        IFC(get_ViewportSize(&viewport));

        // Return the length as zero because of current length is greater than
        // the viewport that is a layout cycle issue. The valid length and
        // viewport will be updated after complete layout updating.
        if (!DoubleUtil::IsNaN(viewport) && !DoubleUtil::IsNaN(length) && length != 0 && DoubleUtil::GreaterThan(length, viewport))
        {
            *pLength = 0.0f;
            goto Cleanup;
        }
    }

    // Added to consider the case where everything is collapsed.
    *pLength = (DoubleUtil::IsNaN(length)) ? 0.0f : length;

Cleanup:
    RRETURN(hr);
}

// Returns the combined actual length in the direction of its orientation of the ScrollBar's RepeatButtons.
_Check_return_ HRESULT
ScrollBar::GetRepeatButtonsLength(
    _Out_ DOUBLE* pRepeatButtonsLength)
{
    HRESULT hr = S_OK;
    DOUBLE length = 0;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
    xaml::Thickness increaseMargin = {};
    xaml::Thickness decreaseMargin = {};
    DOUBLE smallLength = 0.0;

    IFCPTR(pRepeatButtonsLength);
    *pRepeatButtonsLength = 0;

    IFC(get_Orientation(&orientation));
    if (orientation == xaml_controls::Orientation_Horizontal)
    {
        if (m_tpElementHorizontalSmallDecrease)
        {
            IFC(m_tpElementHorizontalSmallDecrease.Cast<RepeatButton>()->get_ActualWidth(&smallLength));
            IFC(m_tpElementHorizontalSmallDecrease.Cast<RepeatButton>()->get_Margin(&decreaseMargin));
            length = smallLength + decreaseMargin.Left + decreaseMargin.Right;
        }
        if (m_tpElementHorizontalSmallIncrease)
        {
            IFC(m_tpElementHorizontalSmallIncrease.Cast<RepeatButton>()->get_ActualWidth(&smallLength));
            IFC(m_tpElementHorizontalSmallIncrease.Cast<RepeatButton>()->get_Margin(&increaseMargin));
            length += smallLength + increaseMargin.Left + increaseMargin.Right;
        }
    }
    else
    {
        if (m_tpElementVerticalSmallDecrease)
        {
            IFC(m_tpElementVerticalSmallDecrease.Cast<RepeatButton>()->get_ActualHeight(&smallLength));
            IFC(m_tpElementVerticalSmallDecrease.Cast<RepeatButton>()->get_Margin(&decreaseMargin));
            length = smallLength + decreaseMargin.Top + decreaseMargin.Bottom;
        }
        if (m_tpElementVerticalSmallIncrease)
        {
            IFC(m_tpElementVerticalSmallIncrease.Cast<RepeatButton>()->get_ActualHeight(&smallLength));
            IFC(m_tpElementVerticalSmallIncrease.Cast<RepeatButton>()->get_Margin(&increaseMargin));
            length += smallLength + increaseMargin.Top + increaseMargin.Bottom;
        }
    }

    *pRepeatButtonsLength = length;

Cleanup:
    RRETURN(hr);
}

// Called when the Value value changed.
IFACEMETHODIMP ScrollBar::OnValueChanged(
    _In_ DOUBLE oldValue,
    _In_ DOUBLE newValue)
{
    HRESULT hr = S_OK;

    IFC(ScrollBarGenerated::OnValueChanged(oldValue, newValue));
    IFC(UpdateTrackLayout());

Cleanup:
    RRETURN(hr);
}

// Called when the Minimum value changed.
IFACEMETHODIMP ScrollBar::OnMinimumChanged(
    _In_ DOUBLE oldMinimum,
    _In_ DOUBLE newMinimum)
{
    HRESULT hr = S_OK;

    IFC(ScrollBarGenerated::OnMinimumChanged(oldMinimum, newMinimum));
    IFC(UpdateTrackLayout());

Cleanup:
    RRETURN(hr);
}

// Called when the Maximum value changed.
IFACEMETHODIMP ScrollBar::OnMaximumChanged(
    _In_ DOUBLE oldMaximum,
    _In_ DOUBLE newMaximum)
{
    HRESULT hr = S_OK;

    IFC(ScrollBarGenerated::OnMaximumChanged(oldMaximum, newMaximum));
    IFC(UpdateTrackLayout());

Cleanup:
    RRETURN(hr);
}

// Gets a value indicating whether the ScrollBar is currently dragging.
_Check_return_ HRESULT ScrollBar::get_IsDragging(
    _Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;

    IFCPTR(pValue);

    IFC(get_Orientation(&orientation));
    if (orientation == xaml_controls::Orientation_Horizontal && m_tpElementHorizontalThumb)
    {
        IFC(m_tpElementHorizontalThumb.Cast<Thumb>()->get_IsDragging(pValue));
    }
    else if (orientation == xaml_controls::Orientation_Vertical && m_tpElementVerticalThumb)
    {
        IFC(m_tpElementVerticalThumb.Cast<Thumb>()->get_IsDragging(pValue));
    }
    else
    {
        *pValue = FALSE;
    }

Cleanup:
    RRETURN(hr);
}

// Gets a value indicating whether the ScrollBar reacts to user input or not.
_Check_return_ HRESULT ScrollBar::get_IsIgnoringUserInput(
    _Out_ BOOLEAN& value)
{
    value = m_isIgnoringUserInput;
    RRETURN(S_OK);
}

// Sets a value indicating whether the ScrollBar reacts to user input or not.
_Check_return_ HRESULT ScrollBar::put_IsIgnoringUserInput(
    _In_ BOOLEAN value)
{
    m_isIgnoringUserInput = value;
    RRETURN(S_OK);
}

_Check_return_
HRESULT
ScrollBar::get_ElementHorizontalTemplate(
    _Outptr_ xaml::IUIElement** ppElement)
{
    HRESULT hr = S_OK;
    IFCPTR(ppElement);

    IFC(m_tpElementHorizontalTemplate.CopyTo(ppElement));

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ScrollBar::get_ElementVerticalTemplate(
    _Outptr_ xaml::IUIElement** ppElement)
{
    HRESULT hr = S_OK;
    IFCPTR(ppElement);

    IFC(m_tpElementVerticalTemplate.CopyTo(ppElement));

Cleanup:
    RRETURN(hr);
}

// Called whenever the Thumb drag operation is started.
_Check_return_ HRESULT ScrollBar::OnThumbDragStarted(
    _In_ IInspectable* pSender,
    _In_ xaml_primitives::IDragStartedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ThumbDragStartedEventSourceType* pEventSource = nullptr;

    IFC(get_Value(&m_dragValue));

    IFC(GetThumbDragStartedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), pArgs));

Cleanup:
    RRETURN(hr);
}

// Whenever the thumb gets dragged, we handle the event through this function to
// update the current value depending upon the thumb drag delta.
_Check_return_ HRESULT ScrollBar::OnThumbDragDelta(
    _In_ IInspectable* pSender,
    _In_ xaml_primitives::IDragDeltaEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    DOUBLE offset = 0.0;
    DOUBLE zoom = 1.0;
    DOUBLE maximum = 0.0;
    DOUBLE minimum = 0.0;
    DOUBLE trackLength = 0.0;
    DOUBLE repeatButtonsLength = 0.0;
    DOUBLE thumbSize = 0.0;
    DOUBLE change = 0.0;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;

    IFC(get_Maximum(&maximum));
    IFC(get_Minimum(&minimum));

    IFC(get_Orientation(&orientation));
    if (orientation == xaml_controls::Orientation_Horizontal &&
        m_tpElementHorizontalThumb)
    {
        IFC(pArgs->get_HorizontalChange(&change));
        IFC(GetTrackLength(&trackLength));
        IFC(GetRepeatButtonsLength(&repeatButtonsLength));
        trackLength -= repeatButtonsLength;
        IFC(m_tpElementHorizontalThumb.Cast<Thumb>()->get_ActualWidth(&thumbSize));

        offset = (zoom * change) / (trackLength - thumbSize) * (maximum - minimum);
    }
    else if (orientation == xaml_controls::Orientation_Vertical &&
        m_tpElementVerticalThumb)
    {
        IFC(pArgs->get_VerticalChange(&change));
        IFC(GetTrackLength(&trackLength));
        IFC(GetRepeatButtonsLength(&repeatButtonsLength));
        trackLength -= repeatButtonsLength;
        IFC(m_tpElementVerticalThumb.Cast<Thumb>()->get_ActualHeight(&thumbSize));

        offset = (zoom * change) / (trackLength - thumbSize) * (maximum - minimum);
    }

    if (!DoubleUtil::IsNaN(offset) &&
        !DoubleUtil::IsInfinity(offset))
    {
        DOUBLE value = 0.0;
        DOUBLE newValue = 0.0;

        m_dragValue += offset;

        newValue = DoubleUtil::Min(maximum, DoubleUtil::Max(minimum, m_dragValue));
        IFC(get_Value(&value));
        if (newValue != value)
        {
            IFC(put_Value(newValue));
            IFC(RaiseScrollEvent(xaml_primitives::ScrollEventType_ThumbTrack));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Raise the Scroll event when teh Thumb drag is completed.
_Check_return_ HRESULT ScrollBar::OnThumbDragCompleted(
    _In_ IInspectable* pSender,
    _In_ xaml_primitives::IDragCompletedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ThumbDragCompletedEventSourceType* pEventSource = nullptr;

    IFC(RaiseScrollEvent(xaml_primitives::ScrollEventType_EndScroll));

    IFC(GetThumbDragCompletedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), pArgs));

Cleanup:
    RRETURN(hr);
}

// Handle the SizeChanged event.
_Check_return_ HRESULT ScrollBar::OnSizeChanged(
    _In_ IInspectable* pSender,
    _In_ xaml::ISizeChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(UpdateTrackLayout());

Cleanup:
    RRETURN(hr);
}

// Called whenever the SmallDecrement button is clicked.
_Check_return_ HRESULT ScrollBar::SmallDecrement(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    DOUBLE value = 0.0;
    DOUBLE change = 0.0;
    DOUBLE edge = 0.0;
    DOUBLE newValue = 0.0;

    IFC(get_Value(&value));
    IFC(get_SmallChange(&change));
    IFC(get_Minimum(&edge));

    newValue = DoubleUtil::Max(value - change, edge);
    if (newValue != value)
    {
        IFC(put_Value(newValue));
        IFC(RaiseScrollEvent(xaml_primitives::ScrollEventType_SmallDecrement));
    }

Cleanup:
    RRETURN(hr);
}

// Called whenever the SmallIncrement button is clicked.
_Check_return_ HRESULT ScrollBar::SmallIncrement(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    DOUBLE value = 0.0;
    DOUBLE change = 0.0;
    DOUBLE edge = 0.0;
    DOUBLE newValue = 0.0;

    IFC(get_Value(&value));
    IFC(get_SmallChange(&change));
    IFC(get_Maximum(&edge));

    newValue = DoubleUtil::Min(value + change, edge);
    if (newValue != value)
    {
        IFC(put_Value(newValue));
        IFC(RaiseScrollEvent(xaml_primitives::ScrollEventType_SmallIncrement));
    }

Cleanup:
    RRETURN(hr);
}

// Called whenever the LargeDecrement button is clicked.
_Check_return_ HRESULT ScrollBar::LargeDecrement(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    DOUBLE value = 0.0;
    DOUBLE change = 0.0;
    DOUBLE edge = 0.0;
    DOUBLE newValue = 0.0;

    IFC(get_Value(&value));
    IFC(get_LargeChange(&change));
    IFC(get_Minimum(&edge));

    newValue = DoubleUtil::Max(value - change, edge);
    if (newValue != value)
    {
        IFC(put_Value(newValue));
        IFC(RaiseScrollEvent(xaml_primitives::ScrollEventType_LargeDecrement));
    }

Cleanup:
    RRETURN(hr);
}

// Called whenever the LargeIncrement button is clicked.
_Check_return_ HRESULT ScrollBar::LargeIncrement(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    DOUBLE value = 0.0;
    DOUBLE change = 0.0;
    DOUBLE edge = 0.0;
    DOUBLE newValue = 0.0;

    IFC(get_Value(&value));
    IFC(get_LargeChange(&change));
    IFC(get_Maximum(&edge));

    newValue = DoubleUtil::Min(value + change, edge);
    if (newValue != value)
    {
        IFC(put_Value(newValue));
        IFC(RaiseScrollEvent(xaml_primitives::ScrollEventType_LargeIncrement));
    }

Cleanup:
    RRETURN(hr);
}

// This raises the Scroll event, passing in the scrollEventType as a parameter
// to let the handler know what triggered this event.
_Check_return_ HRESULT ScrollBar::RaiseScrollEvent(
    _In_ xaml_primitives::ScrollEventType scrollEventType)
{
    HRESULT hr = S_OK;
    ScrollEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<ScrollEventArgs> spArgs;
    DOUBLE value = 0.0;

    // TODO: Add tracing for small change events

    // Create the args
    IFC(ctl::make<ScrollEventArgs>(&spArgs));
    IFC(spArgs->put_ScrollEventType(scrollEventType));
    IFC(get_Value(&value));
    IFC(spArgs->put_NewValue(value));
    IFC(spArgs->put_OriginalSource(ctl::as_iinspectable(this)));

    // Raise the event
    IFC(GetScrollEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), spArgs.Get()));

Cleanup:
    RRETURN(hr);
}

// Change the template being used to display this control when the orientation
// changes.
_Check_return_ HRESULT ScrollBar::OnOrientationChanged()
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;

    IFC(get_Orientation(&orientation));

    //Set Visible and collapsed based on orientation.
    if (m_tpElementVerticalTemplate)
    {
        IFC(m_tpElementVerticalTemplate.Cast<FrameworkElement>()->put_Visibility(
            orientation == xaml_controls::Orientation_Horizontal ?
                xaml::Visibility_Collapsed :
                xaml::Visibility_Visible));
    }

    if (m_tpElementVerticalPanningRoot)
    {
        IFC(m_tpElementVerticalPanningRoot.Cast<FrameworkElement>()->put_Visibility(
            orientation == xaml_controls::Orientation_Horizontal ?
                xaml::Visibility_Collapsed :
                xaml::Visibility_Visible));
    }

    if (m_tpElementHorizontalTemplate)
    {
        IFC(m_tpElementHorizontalTemplate.Cast<FrameworkElement>()->put_Visibility(
            orientation == xaml_controls::Orientation_Horizontal ?
                xaml::Visibility_Visible :
                xaml::Visibility_Collapsed));
    }

    if (m_tpElementHorizontalPanningRoot)
    {
        IFC(m_tpElementHorizontalPanningRoot.Cast<FrameworkElement>()->put_Visibility(
            orientation == xaml_controls::Orientation_Horizontal ?
                xaml::Visibility_Visible :
                xaml::Visibility_Collapsed));
    }

    IFC(UpdateTrackLayout());

Cleanup:
    RRETURN(hr);
}

// Update track based on panning or mouse activity
_Check_return_ HRESULT ScrollBar::RefreshTrackLayout()
{
    HRESULT hr = S_OK;

    IFC(UpdateTrackLayout());
    IFC(ChangeVisualState(true));

Cleanup:
    RRETURN(hr);
}

//Update scrollbar visibility based on what input device is active and the orientation
//of the ScrollBar.
_Check_return_ HRESULT ScrollBar::UpdateScrollBarVisibility()
{
    HRESULT hr = S_OK;
    IFC(OnOrientationChanged());
    IFC(RefreshTrackLayout());

Cleanup:
    RRETURN(hr);
}


// This method will take the current min, max, and value to
// calculate and layout the current control measurements.
_Check_return_ HRESULT ScrollBar::UpdateTrackLayout()
{
    DOUBLE maximum = 0.0;
    DOUBLE minimum = 0.0;
    DOUBLE value = 0.0;
    DOUBLE multiplier = 1.0;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
    DOUBLE trackLength = 0.0;
    DOUBLE repeatButtonsLength = 0.0;
    DOUBLE mouseIndicatorLength = 0.0;
    DOUBLE touchIndicatorLength = 0.0;
    xaml::Thickness newMargin = {};

    IFC_RETURN(get_Maximum(&maximum));
    IFC_RETURN(get_Minimum(&minimum));
    IFC_RETURN(get_Value(&value));
    IFC_RETURN(get_Orientation(&orientation));
    IFC_RETURN(GetTrackLength(&trackLength));
    IFC_RETURN(UpdateIndicatorLengths(trackLength, &mouseIndicatorLength, &touchIndicatorLength));
    const DOUBLE difference = maximum - minimum;

    //Check to make sure that its not dividing by zero.
    if (difference == 0.0)
    {
        multiplier = 0.0;
    }
    else
    {
        multiplier = (value - minimum) / difference;
    }

    IFC_RETURN(GetRepeatButtonsLength(&repeatButtonsLength));
    const DOUBLE largeDecreaseNewSize = DoubleUtil::Max(0.0, multiplier * (trackLength - repeatButtonsLength - mouseIndicatorLength));
    const DOUBLE indicatorOffset = DoubleUtil::Max(0.0, multiplier * (trackLength - touchIndicatorLength));

    if (orientation == xaml_controls::Orientation_Horizontal &&
        m_tpElementHorizontalLargeDecrease &&
        m_tpElementHorizontalThumb)
    {
        IFC_RETURN(m_tpElementHorizontalLargeDecrease.Cast<RepeatButton>()->put_Width(largeDecreaseNewSize));
    }
    else if (orientation == xaml_controls::Orientation_Vertical &&
        m_tpElementVerticalLargeDecrease &&
        m_tpElementVerticalThumb)
    {
        IFC_RETURN(m_tpElementVerticalLargeDecrease.Cast<RepeatButton>()->put_Height(largeDecreaseNewSize));
    }

    if (orientation == xaml_controls::Orientation_Horizontal &&
        m_tpElementHorizontalPanningRoot)
    {
        IFC_RETURN(m_tpElementHorizontalPanningRoot.Cast<FrameworkElement>()->get_Margin(&newMargin));
        newMargin.Left = indicatorOffset;
        IFC_RETURN(m_tpElementHorizontalPanningRoot.Cast<FrameworkElement>()->put_Margin(newMargin));
    }
    else if (orientation == xaml_controls::Orientation_Vertical &&
        m_tpElementVerticalPanningRoot)
    {
        IFC_RETURN(m_tpElementVerticalPanningRoot.Cast<FrameworkElement>()->get_Margin(&newMargin));
        newMargin.Top = indicatorOffset;
        IFC_RETURN(m_tpElementVerticalPanningRoot.Cast<FrameworkElement>()->put_Margin(newMargin));
    }

    return S_OK;
}

// Based on the ViewportSize, the Track's length, and the Minimum and Maximum
// values, we will calculate the length of the Thumb.
_Check_return_ HRESULT ScrollBar::ConvertViewportSizeToDisplayUnits(
    _In_ DOUBLE trackLength,
    _Out_ DOUBLE* pThumbSize)
{
    HRESULT hr = S_OK;
    DOUBLE maximum = 0.0;
    DOUBLE minimum = 0.0;
    DOUBLE viewport = 0.0;

    IFCPTR(pThumbSize);
    *pThumbSize = 0.0;

    IFC(get_Maximum(&maximum));
    IFC(get_Minimum(&minimum));
    IFC(get_ViewportSize(&viewport));

    double thumbSize = trackLength * viewport / DoubleUtil::Max(1, viewport + maximum - minimum);

    // When UseLayoutRounding is True (default), the thumb size, whether it's for touch or mouse input, needs to be rounded to
    // the nearest value taking into account the rounding step 1.0f / RootScale::GetRasterizationScaleForElement(GetHandle()).
    // Rounding to the nearest whole number would risk a situation where ElementVerticalPanningRoot.Margin.Top + ElementVerticalPanningThumb.Height
    // could grow the ScrollBar when ScrollBar.Value is at its maximum ScrollBar.Maximum.
    bool roundedThumbSizeWithLayoutRound = RoundWithLayoutRound(&thumbSize);

    if (roundedThumbSizeWithLayoutRound)
    {
        *pThumbSize = thumbSize;
    }
    else
    {
        // We need to round to the nearest whole number.
        // In the case where pThumbSize is calculated to have a fractional part of exactly .5,
        // then in UpdateTrackLayout() where we calculate the largeDecreaseNewSize we end up giving
        // largeDecreaseNewSize a size of 0.5 as well, and at this point the grid laying out
        // the mouse portion of the ScrollBar template nudges the increase repeat button 1 px.
        *pThumbSize = DoubleUtil::Round(thumbSize, 0);
    }

Cleanup:
    RRETURN(hr);
}

// This will resize the Thumb, based on calculations with the
// ViewportSize, the Track's length, and the Minimum and Maximum
// values.
_Check_return_ HRESULT
ScrollBar::UpdateIndicatorLengths(
    _In_ DOUBLE trackLength,
    _Out_ DOUBLE* pMouseIndicatorLength,
    _Out_ DOUBLE* pTouchIndicatorLength)
{
    HRESULT hr = S_OK;
    DOUBLE repeatButtonsLength = 0.0;
    DOUBLE trackLengthMinusRepeatButtonsLength = 0.0;
    DOUBLE result = DoubleUtil::NaN;
    DOUBLE maximum = 0.0;
    DOUBLE minimum = 0.0;
    DOUBLE mouseIndicatorSize = 0.0;
    DOUBLE touchIndicatorSize = 0.0;
    DOUBLE mouseMinSize = 0.0;
    DOUBLE touchMinSize = 0.0;
    DOUBLE actualSize = 0.0;
    BOOLEAN hideThumb = DoubleUtil::LessThanOrClose(trackLength, 0.0);
    BOOLEAN mouseIndicatorLengthWasSet = FALSE;
    BOOLEAN touchIndicatorLengthWasSet = FALSE;

    IFCPTR(pMouseIndicatorLength);
    *pMouseIndicatorLength = 0.0;
    IFCPTR(pTouchIndicatorLength);
    *pTouchIndicatorLength = 0.0;

    if (!hideThumb)
    {
        DOUBLE actualSizeMinusRepeatButtonsLength = 0.0;

        xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;

        IFC(get_Orientation(&orientation));
        IFC(get_Maximum(&maximum));
        IFC(get_Minimum(&minimum));
        IFC(GetRepeatButtonsLength(&repeatButtonsLength));
        trackLengthMinusRepeatButtonsLength = trackLength - repeatButtonsLength;
        IFC(ConvertViewportSizeToDisplayUnits(trackLengthMinusRepeatButtonsLength, &mouseIndicatorSize));

        if (orientation == xaml_controls::Orientation_Horizontal &&
            m_tpElementHorizontalThumb)
        {
            if (maximum - minimum != 0)
            {
                IFC(m_tpElementHorizontalThumb.Cast<Thumb>()->get_MinWidth(&mouseMinSize));
                RoundWithLayoutRound(&mouseMinSize);
                result = DoubleUtil::Max(mouseMinSize, mouseIndicatorSize);
            }

            // Hide the thumb if too big
            IFC(get_ActualWidth(&actualSize));
            actualSizeMinusRepeatButtonsLength = actualSize - repeatButtonsLength;
            if (maximum - minimum == 0 || result > actualSizeMinusRepeatButtonsLength || trackLengthMinusRepeatButtonsLength <= mouseMinSize)
            {
                hideThumb = TRUE;
            }
            else
            {
                IFC(m_tpElementHorizontalThumb.Cast<Thumb>()->put_Visibility(xaml::Visibility_Visible));
                IFC(m_tpElementHorizontalThumb.Cast<Thumb>()->put_Width(result));
                mouseIndicatorLengthWasSet = TRUE;
            }
        }
        else if (orientation == xaml_controls::Orientation_Vertical &&
            m_tpElementVerticalThumb)
        {
            if (maximum - minimum != 0)
            {
                IFC(m_tpElementVerticalThumb.Cast<Thumb>()->get_MinHeight(&mouseMinSize));
                RoundWithLayoutRound(&mouseMinSize);
                result = DoubleUtil::Max(mouseMinSize, mouseIndicatorSize);
            }

            // Hide the thumb if too big
            IFC(get_ActualHeight(&actualSize));
            actualSizeMinusRepeatButtonsLength = actualSize - repeatButtonsLength;
            if (maximum - minimum == 0 || result > actualSizeMinusRepeatButtonsLength || trackLengthMinusRepeatButtonsLength <= mouseMinSize)
            {
                hideThumb = TRUE;
            }
            else
            {
                IFC(m_tpElementVerticalThumb.Cast<Thumb>()->put_Visibility(xaml::Visibility_Visible));
                IFC(m_tpElementVerticalThumb.Cast<Thumb>()->put_Height(result));
                mouseIndicatorLengthWasSet = TRUE;
            }
        }

        if (mouseIndicatorLengthWasSet)
        {
            //added to consider the case where everything is collapsed.
            *pMouseIndicatorLength = (DoubleUtil::IsNaN(result)) ? 0.0f : result;
        }

        IFC(ConvertViewportSizeToDisplayUnits(trackLength, &touchIndicatorSize));

        //Do the same for horizontal panning indicator.
        if (orientation == xaml_controls::Orientation_Horizontal &&
            m_tpElementHorizontalPanningThumb)
        {
            if (maximum - minimum != 0)
            {
                IFC(m_tpElementHorizontalPanningThumb.Cast<FrameworkElement>()->get_MinWidth(&touchMinSize));
                RoundWithLayoutRound(&touchMinSize);
                result = DoubleUtil::Max(touchMinSize, touchIndicatorSize);
            }

            // Hide the thumb if too big
            IFC(get_ActualWidth(&actualSize));
            if (maximum - minimum == 0 || result > actualSize || trackLength <= touchMinSize)
            {
                hideThumb = TRUE;
            }
            else
            {
                IFC(m_tpElementHorizontalPanningThumb.Cast<FrameworkElement>()->put_Visibility(xaml::Visibility_Visible));
                IFC(m_tpElementHorizontalPanningThumb.Cast<FrameworkElement>()->put_Width(result));
                touchIndicatorLengthWasSet = TRUE;
            }
        }
        else if (orientation == xaml_controls::Orientation_Vertical &&
            m_tpElementVerticalPanningThumb)
        {
            if (maximum - minimum != 0)
            {
                IFC(m_tpElementVerticalPanningThumb.Cast<FrameworkElement>()->get_MinHeight(&touchMinSize));
                RoundWithLayoutRound(&touchMinSize);
                result = DoubleUtil::Max(touchMinSize, touchIndicatorSize);
            }

            // Hide the thumb if too big
            IFC(get_ActualHeight(&actualSize));
            if (maximum - minimum == 0 || result > actualSize || trackLength <= touchMinSize)
            {
                hideThumb = TRUE;
            }
            else
            {
                IFC(m_tpElementVerticalPanningThumb.Cast<FrameworkElement>()->put_Visibility(xaml::Visibility_Visible));
                IFC(m_tpElementVerticalPanningThumb.Cast<FrameworkElement>()->put_Height(result));
                touchIndicatorLengthWasSet = TRUE;
            }
        }

        if (touchIndicatorLengthWasSet)
        {
            //added to consider the case where everything is collapsed.
            *pTouchIndicatorLength = (DoubleUtil::IsNaN(result)) ? 0.0f : result;
        }
    }

    if (hideThumb)
    {
        if (m_tpElementHorizontalThumb)
        {
            IFC(m_tpElementHorizontalThumb.Cast<Thumb>()->put_Visibility(xaml::Visibility_Collapsed));
        }
        if (m_tpElementVerticalThumb)
        {
            IFC(m_tpElementVerticalThumb.Cast<Thumb>()->put_Visibility(xaml::Visibility_Collapsed));
        }
        if (m_tpElementHorizontalPanningThumb)
        {
            IFC(m_tpElementHorizontalPanningThumb.Cast<FrameworkElement>()->put_Visibility(xaml::Visibility_Collapsed));
        }
        if (m_tpElementVerticalPanningThumb)
        {
            IFC(m_tpElementVerticalPanningThumb.Cast<FrameworkElement>()->put_Visibility(xaml::Visibility_Collapsed));
        }
    }

    StoreLayoutCycleWarningContext(
        trackLength,
        repeatButtonsLength,
        minimum,
        maximum,
        mouseIndicatorSize,
        touchIndicatorSize,
        mouseMinSize,
        touchMinSize,
        actualSize); 

Cleanup:
    RRETURN(hr);
}

// during a SemanticZoomOperation we want to be able to block the scrollbar
// without stomping over the user value
_Check_return_ HRESULT
ScrollBar::BlockIndicatorFromShowing()
{
    HRESULT hr = S_OK;

    if (!m_blockIndicators)
    {
        m_blockIndicators = TRUE;
        IFC(ChangeVisualState(false));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ScrollBar::ResetBlockIndicatorFromShowing()
{

    m_blockIndicators = FALSE;

    // Don't change state; stay in NoIndicator. The next ScrollViewer::ShowIndicators()
    // call will drive our next GoToState() call, with transitions.

    RRETURN(S_OK);
}

_Check_return_ HRESULT
    ScrollBar::AdjustDragValue(DOUBLE delta)
{
    BOOLEAN dragging = FALSE;
    HRESULT hr = S_OK;

    // If somebody is calling this when not dragging, are they confused?
    IFC(get_IsDragging(&dragging));
    ASSERT(dragging);

    m_dragValue += delta;

Cleanup:
    RRETURN(hr);
}

// Rounds the value using CUIElement::LayoutRound when get_UseLayoutRounding returns True.
// Returns True when rounding was performed, and False otherwise.
bool ScrollBar::RoundWithLayoutRound(
    _Inout_ double* value)
{
    BOOLEAN roundValue = FALSE;

    IFCFAILFAST(get_UseLayoutRounding(&roundValue));

    if (roundValue)
    {
        float valueF = static_cast<float>(*value);

        IFCFAILFAST(LayoutRound(valueF, &valueF));

        *value = valueF;
    }

    return roundValue;
}

// Stores some ScrollBar internal sizes in a WarningContext when the layout iterations get close to the 250 limit
// in order to ease layout cycles' debugging (that involve a ScrollBar).
void ScrollBar::StoreLayoutCycleWarningContext(
    double trackLength,
    double repeatButtonsLength,
    double minimum,
    double maximum,
    double mouseIndicatorSize,
    double touchIndicatorSize,
    double mouseMinSize,
    double touchMinSize,
    double actualSize)
{
    CUIElement* scrollBarAsCUIElement = static_cast<CUIElement*>(GetHandle());

    if (!scrollBarAsCUIElement || !scrollBarAsCUIElement->StoreLayoutCycleWarningContexts())
    {
        return;
    }

    std::vector<std::wstring> warningInfo;

    std::wstring lengths(L"trackLength: ");
    lengths.append(std::to_wstring(trackLength));
    lengths.append(L", repeatButtonsLength=");
    lengths.append(std::to_wstring(repeatButtonsLength));
    warningInfo.push_back(std::move(lengths));

    double value, viewport;

    IGNOREHR(get_Value(&value));
    IGNOREHR(get_ViewportSize(&viewport));

    std::wstring values(L"minimum: ");
    values.append(std::to_wstring(minimum));
    values.append(L", maximum: ");
    values.append(std::to_wstring(maximum));
    values.append(L", value: ");
    values.append(std::to_wstring(value));
    values.append(L", viewport: ");
    values.append(std::to_wstring(viewport));
    warningInfo.push_back(std::move(values));

    std::wstring indicatorSizes(L"mouseIndicatorSize: ");
    indicatorSizes.append(std::to_wstring(mouseIndicatorSize));
    indicatorSizes.append(L", touchIndicatorSize: ");
    indicatorSizes.append(std::to_wstring(touchIndicatorSize));
    warningInfo.push_back(std::move(indicatorSizes));

    std::wstring sizes(L"mouseMinSize: ");
    sizes.append(std::to_wstring(mouseMinSize));
    sizes.append(L", touchMinSize: ");
    sizes.append(std::to_wstring(touchMinSize));
    sizes.append(L", actualSize: ");
    sizes.append(std::to_wstring(actualSize));
    warningInfo.push_back(std::move(sizes));

    scrollBarAsCUIElement->StoreLayoutCycleWarningContext(warningInfo, nullptr /*layoutManager*/, DEFAULT_WARNING_FRAMES_TO_SKIP + 1);

    if (LayoutCycleDebugSettings::ShouldDebugBreak(DirectUI::LayoutCycleDebugBreakLevel::Low))
    {
        __debugbreak();
    }
}
