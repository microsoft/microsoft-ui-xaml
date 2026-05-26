// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AppBarToggleButton.g.h"
#include "AppBarToggleButtonAutomationPeer.g.h"
#include "AppBarToggleButtonTemplateSettings.g.h"
#include "AppBarButtonHelpers.h"
#include "CommandBar.g.h"
#include "DependencyPropertyChangedEventArgs.g.h"
#include "Style.g.h"
#include "Setter.g.h"
#include "SetterBaseCollection.g.h"
#include "Storyboard.g.h"
#include "TimelineCollection.g.h"
#include "ObjectAnimationUsingKeyFrames.g.h"
#include "ObjectKeyFrameCollection.g.h"
#include "DiscreteObjectKeyFrame.g.h"

#include <limits>

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace ::Windows::Internal;

template<>
KnownPropertyIndex AppBarButtonHelpers::GetIsCompactDependencyProperty<AppBarToggleButton>()
{
    return KnownPropertyIndex::AppBarToggleButton_IsCompact;
}

template<>
KnownPropertyIndex AppBarButtonHelpers::GetUseOverflowStyleDependencyProperty<AppBarToggleButton>()
{
    return KnownPropertyIndex::AppBarToggleButton_UseOverflowStyle;
}

template<>
KnownPropertyIndex AppBarButtonHelpers::GetLabelPositionDependencyProperty<AppBarToggleButton>()
{
    return KnownPropertyIndex::AppBarToggleButton_LabelPosition;
}

template<>
KnownPropertyIndex AppBarButtonHelpers::GetLabelDependencyProperty<AppBarToggleButton>()
{
    return KnownPropertyIndex::AppBarToggleButton_Label;
}

template<>
KnownPropertyIndex AppBarButtonHelpers::GetIconDependencyProperty<AppBarToggleButton>()
{
    return KnownPropertyIndex::AppBarToggleButton_Icon;
}

template<>
KnownPropertyIndex AppBarButtonHelpers::GetKeyboardAcceleratorTextDependencyProperty<AppBarToggleButton>()
{
    return KnownPropertyIndex::AppBarToggleButton_KeyboardAcceleratorTextOverride;
}

AppBarToggleButton::AppBarToggleButton()
    : m_inputDeviceTypeUsedToOpenOverflow(DirectUI::InputDeviceType::None)
    , m_isTemplateApplied(false)
    , m_isWithIcons(false)
    , m_ownsToolTip(true)
{
}

_Check_return_
HRESULT AppBarToggleButton::SetOverflowStyleParams(_In_ bool hasIcons, _In_ bool hasKeyboardAcceleratorText)
{
    bool updateState = false;

    if (m_isWithIcons != hasIcons)
    {
        m_isWithIcons = hasIcons;
        updateState = true;
    }
    if (m_isWithKeyboardAcceleratorText != hasKeyboardAcceleratorText)
    {
        m_isWithKeyboardAcceleratorText = hasKeyboardAcceleratorText;
        updateState = true;
    }
    if (updateState)
    {
        IFC_RETURN(UpdateVisualState());
    }
    return S_OK;
}

_Check_return_
HRESULT AppBarToggleButton::SetDefaultLabelPositionImpl(_In_ xaml_controls::CommandBarDefaultLabelPosition defaultLabelPosition)
{
    if (m_defaultLabelPosition != defaultLabelPosition)
    {
        m_defaultLabelPosition = defaultLabelPosition;
        IFC_RETURN(UpdateInternalStyles());
        IFC_RETURN(UpdateVisualState());
    }

    return S_OK;
}

_Check_return_
HRESULT AppBarToggleButton::GetHasBottomLabelImpl(_Out_ BOOLEAN* hasBottomLabel)
{
    IFC_RETURN(GetHasLabelAtPosition(xaml_controls::CommandBarDefaultLabelPosition_Bottom, hasBottomLabel));

    return S_OK;
}

_Check_return_
HRESULT AppBarToggleButton::GetHasRightLabelImpl(_Out_ BOOLEAN* hasRightLabel)
{
    IFC_RETURN(GetHasLabelAtPosition(xaml_controls::CommandBarDefaultLabelPosition_Right, hasRightLabel));

    return S_OK;
}

_Check_return_
HRESULT AppBarToggleButton::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(AppBarToggleButtonGenerated::OnPropertyChanged2(args));
    IFC_RETURN(AppBarButtonHelpers::OnPropertyChanged<AppBarToggleButton>(this, args));
    return S_OK;
}

// After template is applied, set the initial view state
// (FullSize or Compact) based on the value of our
// IsCompact property
IFACEMETHODIMP AppBarToggleButton::OnApplyTemplate()
{
    IFC_RETURN(AppBarButtonHelpers::OnBeforeApplyTemplate<AppBarToggleButton>(this));
    IFC_RETURN(AppBarToggleButtonGenerated::OnApplyTemplate());
    IFC_RETURN(AppBarButtonHelpers::OnApplyTemplate<AppBarToggleButton>(this));
    return S_OK;
}

IFACEMETHODIMP AppBarToggleButton::OnPointerEntered(_In_ xaml_input::IPointerRoutedEventArgs* args)
{
    IFC_RETURN(AppBarToggleButtonGenerated::OnPointerEntered(args));
    IFC_RETURN(AppBarButtonHelpers::CloseSubMenusOnPointerEntered<AppBarToggleButton>(this, nullptr));
    return S_OK;
}

// Sets the visual state to "Compact" or "FullSize" based on the value
// of our IsCompact property.
_Check_return_
    HRESULT AppBarToggleButton::ChangeVisualState(_In_ bool useTransitions)
{
    BOOLEAN ignored = FALSE;
    BOOLEAN useOverflowStyle = FALSE;

    IFC_RETURN(AppBarToggleButtonGenerated::ChangeVisualState(useTransitions));
    IFC_RETURN(get_UseOverflowStyle(&useOverflowStyle));

    if (useOverflowStyle)
    {
        if (m_isWithIcons)
        {
            IFC_RETURN(GoToState(useTransitions, L"OverflowWithMenuIcons", &ignored));
        }
        else
        {
            IFC_RETURN(GoToState(useTransitions, L"Overflow", &ignored));
        }

        {
            BOOLEAN isEnabled = FALSE;
            BOOLEAN isPressed = FALSE;
            BOOLEAN isPointerOver = FALSE;
            ctl::ComPtr<wf::IReference<bool>> isCheckedReference;
            BOOLEAN isChecked = FALSE;

            IFC_RETURN(get_IsEnabled(&isEnabled));
            IFC_RETURN(get_IsPressed(&isPressed));
            IFC_RETURN(get_IsPointerOver(&isPointerOver));

            IFC_RETURN(this->get_IsChecked(&isCheckedReference));
            if (isCheckedReference)
            {
                IFC_RETURN(isCheckedReference->get_Value(&isChecked));
            }

            if (isChecked)
            {
                if (isPressed)
                {
                    IFC_RETURN(GoToState(useTransitions, L"OverflowCheckedPressed", &ignored));
                }
                else if (isPointerOver)
                {
                    IFC_RETURN(GoToState(useTransitions, L"OverflowCheckedPointerOver", &ignored));
                }
                else if (isEnabled)
                {
                    IFC_RETURN(GoToState(useTransitions, L"OverflowChecked", &ignored));
                }
            }
            else
            {
                if (isPressed)
                {
                    IFC_RETURN(GoToState(useTransitions, L"OverflowPressed", &ignored));
                }
                else if (isPointerOver)
                {
                    IFC_RETURN(GoToState(useTransitions, L"OverflowPointerOver", &ignored));
                }
                else if (isEnabled)
                {
                    IFC_RETURN(GoToState(useTransitions, L"OverflowNormal", &ignored));
                }
            }
        }
    }

    IFC_RETURN(AppBarButtonHelpers::ChangeCommonVisualStates<AppBarToggleButton>(this, useTransitions));
    return S_OK;
}

// Create AppBarToggleButtonAutomationPeer to represent the AppBarToggleButton.
IFACEMETHODIMP AppBarToggleButton::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IAppBarToggleButtonAutomationPeer> spAppBarToggleButtonAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IAppBarToggleButtonAutomationPeerFactory> spAppBarToggleButtonAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::AppBarToggleButtonAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spAppBarToggleButtonAPFactory));

    IFC(spAppBarToggleButtonAPFactory.Cast<AppBarToggleButtonAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spAppBarToggleButtonAutomationPeer));
    IFC(spAppBarToggleButtonAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
AppBarToggleButton::OnClick()
{
    HRESULT hr = S_OK;

    IFC(CommandBar::OnCommandExecutionStatic(this));
    IFC(AppBarToggleButtonGenerated::OnClick());

Cleanup:
    return hr;
}

_Check_return_ HRESULT
AppBarToggleButton::OnVisibilityChanged()
{
    IFC_RETURN(__super::OnVisibilityChanged());
    return CommandBar::OnCommandBarElementVisibilityChanged(this);
}

_Check_return_ HRESULT
AppBarToggleButton::OnCommandChanged(_In_  IInspectable* pOldValue, _In_ IInspectable* pNewValue)
{
    IFC_RETURN(AppBarToggleButtonGenerated::OnCommandChanged(pOldValue, pNewValue));
    IFC_RETURN(AppBarButtonHelpers::OnCommandChanged<AppBarToggleButton>(this, pOldValue, pNewValue));
    return S_OK;
}

_Check_return_ HRESULT AppBarToggleButton::GetHasLabelAtPosition(_In_ xaml_controls::CommandBarDefaultLabelPosition labelPosition, _Out_ BOOLEAN* hasLabelAtPosition)
{
    *hasLabelAtPosition = FALSE;

    xaml_controls::CommandBarDefaultLabelPosition effectiveLabelPosition;

    IFC_RETURN(GetEffectiveLabelPosition(&effectiveLabelPosition));

    if (effectiveLabelPosition != labelPosition)
    {
        return S_OK;
    }

    HSTRING label;

    IFC_RETURN(get_Label(&label));

    *hasLabelAtPosition = label != nullptr;

    return S_OK;
}

_Check_return_ HRESULT AppBarToggleButton::GetEffectiveLabelPosition(_Out_ xaml_controls::CommandBarDefaultLabelPosition* effectiveLabelPosition)
{
    xaml_controls::CommandBarLabelPosition labelPosition;
    IFC_RETURN(get_LabelPosition(&labelPosition));

    *effectiveLabelPosition = static_cast<xaml_controls::CommandBarDefaultLabelPosition>(labelPosition == xaml_controls::CommandBarLabelPosition_Collapsed ? xaml_controls::CommandBarDefaultLabelPosition_Collapsed : m_defaultLabelPosition);
    return S_OK;
}

_Check_return_ HRESULT AppBarToggleButton::UpdateInternalStyles()
{
    // If the template isn't applied yet, we'll early-out,
    // because we won't have the style to apply from the
    // template yet.
    if (!m_isTemplateApplied)
    {
        return S_OK;
    }

    xaml_controls::CommandBarDefaultLabelPosition effectiveLabelPosition;
    BOOLEAN useOverflowStyle;

    IFC_RETURN(GetEffectiveLabelPosition(&effectiveLabelPosition));
    IFC_RETURN(get_UseOverflowStyle(&useOverflowStyle));

    const bool shouldHaveLabelOnRightStyleSet = effectiveLabelPosition == xaml_controls::CommandBarDefaultLabelPosition_Right && !useOverflowStyle;

    // Apply/UnApply auto width animation if needed
    // only play auto width animation when the width is not overridden by local/animation setting
    // and when LabelOnRightStyle is not set. LabelOnRightStyle take high priority than animation.
    if (shouldHaveLabelOnRightStyleSet
        && !GetHandle()->HasLocalOrModifierValue(MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::FrameworkElement_Width)))
    {
        // Apply our width adjustments using a storyboard so that we don't stomp over template or user
        // provided values.  When we stop the storyboard, it will restore the previous values.
        if (!m_widthAdjustmentsForLabelOnRightStyleStoryboard) {
            ctl::ComPtr<xaml_animation::IStoryboard> storyboard;
            IFC_RETURN(CreateStoryboardForWidthAdjustmentsForLabelOnRightStyle(&storyboard));
            IFC_RETURN(SetPtrValueWithQI(m_widthAdjustmentsForLabelOnRightStyleStoryboard, storyboard.Get()));
        }

        IFC_RETURN(StartAnimationForWidthAdjustments());
    }
    else if (!shouldHaveLabelOnRightStyleSet && m_widthAdjustmentsForLabelOnRightStyleStoryboard)
    {
        IFC_RETURN(StopAnimationForWidthAdjustments());
    }

    IFC_RETURN(AppBarButtonHelpers::UpdateToolTip<AppBarToggleButton>(this));
    return S_OK;
}

_Check_return_ HRESULT
AppBarToggleButton::get_IsInOverflowImpl(_Out_ BOOLEAN* pValue)
{
    return CommandBar::IsCommandBarElementInOverflow(this, pValue);
}

_Check_return_ HRESULT
AppBarToggleButton::CreateStoryboardForWidthAdjustmentsForLabelOnRightStyle(
    _Out_ xaml_animation::IStoryboard** storyboard)
{
    ctl::ComPtr<Storyboard> storyboardLocal;
    IFC_RETURN(ctl::make(&storyboardLocal));

    ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> storyboardChildren;
    IFC_RETURN(storyboardLocal->get_Children(&storyboardChildren));

    ctl::ComPtr<ObjectAnimationUsingKeyFrames> objectAnimation;
    IFC_RETURN(ctl::make(&objectAnimation));

    IFC_RETURN(CoreImports::Storyboard_SetTarget(static_cast<CTimeline*>(objectAnimation->GetHandle()), GetHandle()));
    IFC_RETURN(StoryboardFactory::SetTargetPropertyStatic(objectAnimation.Get(), wrl_wrappers::HStringReference(L"Width").Get()));

    ctl::ComPtr<wfc::IVector<xaml_animation::ObjectKeyFrame*>> objectKeyFrames;
    IFC_RETURN(objectAnimation->get_KeyFrames(&objectKeyFrames));

    ctl::ComPtr<DiscreteObjectKeyFrame> discreteObjectKeyFrame;
    IFC_RETURN(ctl::make(&discreteObjectKeyFrame));

    xaml_animation::KeyTime keyTime = {};
    keyTime.TimeSpan.Duration = 0;

    ctl::ComPtr<IInspectable> value;
    IFC_RETURN(IValueBoxer::BoxValue(&value, DoubleUtil::NaN));

    IFC_RETURN(discreteObjectKeyFrame->put_KeyTime(keyTime));
    IFC_RETURN(discreteObjectKeyFrame->put_Value(value.Get()));

    IFC_RETURN(objectKeyFrames->Append(discreteObjectKeyFrame.Cast<DiscreteObjectKeyFrame>()));
    IFC_RETURN(storyboardChildren->Append(objectAnimation.Cast<ObjectAnimationUsingKeyFrames>()));

    *storyboard = storyboardLocal.Detach();

    return S_OK;
}

_Check_return_ HRESULT
AppBarToggleButton::StartAnimationForWidthAdjustments()
{
    if (m_widthAdjustmentsForLabelOnRightStyleStoryboard)
    {
        IFC_RETURN(StopAnimationForWidthAdjustments());
        IFC_RETURN(m_widthAdjustmentsForLabelOnRightStyleStoryboard->Begin());
        IFC_RETURN(m_widthAdjustmentsForLabelOnRightStyleStoryboard->SkipToFill());
    }
    return S_OK;
}

_Check_return_ HRESULT
AppBarToggleButton::StopAnimationForWidthAdjustments()
{
    if (m_widthAdjustmentsForLabelOnRightStyleStoryboard)
    {
        xaml_animation::ClockState currentState;
        IFC_RETURN(m_widthAdjustmentsForLabelOnRightStyleStoryboard->GetCurrentState(&currentState));
        if (currentState == xaml_animation::ClockState_Active
            || currentState == xaml_animation::ClockState_Filling)
        {
            IFC_RETURN(m_widthAdjustmentsForLabelOnRightStyleStoryboard->Stop());
        }
    }
    return S_OK;
}

_Check_return_ HRESULT AppBarToggleButton::get_KeyboardAcceleratorTextOverrideImpl(_Out_ HSTRING* pValue)
{
    IFC_RETURN(AppBarButtonHelpers::GetKeyboardAcceleratorText<AppBarToggleButton>(this, pValue));
    return S_OK;
}

_Check_return_ HRESULT AppBarToggleButton::put_KeyboardAcceleratorTextOverrideImpl(_In_opt_ HSTRING value)
{
    IFC_RETURN(AppBarButtonHelpers::PutKeyboardAcceleratorText<AppBarToggleButton>(this, value));
    return S_OK;
}

