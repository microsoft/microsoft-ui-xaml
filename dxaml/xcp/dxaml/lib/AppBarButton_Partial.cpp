// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AppBarButton.g.h"
#include "AppBarButtonAutomationPeer.g.h"
#include "AppBarButtonTemplateSettings.g.h"
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
#include "PropertyChangedParamsHelper.h"
#include "FlyoutBase.g.h"
#include "FlyoutShowOptions.g.h"
#include "Window.g.h"
#include "XamlRoot.g.h"
#include "ThemeWalkResourceCache.h"

#include <limits>

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace ::Windows::Internal;

template<>
KnownPropertyIndex AppBarButtonHelpers::GetIsCompactDependencyProperty<AppBarButton>()
{
    return KnownPropertyIndex::AppBarButton_IsCompact;
}

template<>
KnownPropertyIndex AppBarButtonHelpers::GetUseOverflowStyleDependencyProperty<AppBarButton>()
{
    return KnownPropertyIndex::AppBarButton_UseOverflowStyle;
}

template<>
KnownPropertyIndex AppBarButtonHelpers::GetLabelPositionDependencyProperty<AppBarButton>()
{
    return KnownPropertyIndex::AppBarButton_LabelPosition;
}

template<>
KnownPropertyIndex AppBarButtonHelpers::GetLabelDependencyProperty<AppBarButton>()
{
    return KnownPropertyIndex::AppBarButton_Label;
}

template<>
KnownPropertyIndex AppBarButtonHelpers::GetIconDependencyProperty<AppBarButton>()
{
    return KnownPropertyIndex::AppBarButton_Icon;
}

template<>
KnownPropertyIndex AppBarButtonHelpers::GetKeyboardAcceleratorTextDependencyProperty<AppBarButton>()
{
    return KnownPropertyIndex::AppBarButton_KeyboardAcceleratorTextOverride;
}

AppBarButton::AppBarButton()
    : m_isWithToggleButtons(false)
    , m_isWithIcons(false)
    , m_inputDeviceTypeUsedToOpenOverflow(DirectUI::InputDeviceType::None)
    , m_isTemplateApplied(false)
    , m_ownsToolTip(true)
{
}

_Check_return_
HRESULT AppBarButton::SetOverflowStyleParams(_In_ bool hasIcons, _In_ bool hasToggleButtons, _In_ bool hasKeyboardAcceleratorText)
{
    bool updateState = false;

    if (m_isWithIcons != hasIcons)
    {
        m_isWithIcons = hasIcons;
        updateState = true;
    }
    if (m_isWithToggleButtons != hasToggleButtons)
    {
        m_isWithToggleButtons = hasToggleButtons;
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
HRESULT AppBarButton::SetDefaultLabelPositionImpl(_In_ xaml_controls::CommandBarDefaultLabelPosition defaultLabelPosition)
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
HRESULT AppBarButton::GetHasBottomLabelImpl(_Out_ BOOLEAN *hasBottomLabel)
{
    xaml_controls::CommandBarDefaultLabelPosition effectiveLabelPosition;
    HSTRING label;

    IFC_RETURN(GetEffectiveLabelPosition(&effectiveLabelPosition));
    IFC_RETURN(get_Label(&label));

    *hasBottomLabel =
        effectiveLabelPosition == xaml_controls::CommandBarDefaultLabelPosition_Bottom &&
        label != nullptr;

    return S_OK;
}

IFACEMETHODIMP AppBarButton::OnPointerEntered(_In_ xaml_input::IPointerRoutedEventArgs* args)
{
    IFC_RETURN(AppBarButtonGenerated::OnPointerEntered(args));

    BOOLEAN isInOverflow = FALSE;
    IFC_RETURN(get_IsInOverflow(&isInOverflow));

    if (isInOverflow && m_menuHelper)
    {
        IFC_RETURN(m_menuHelper->OnPointerEntered(args));
    }

    IFC_RETURN(AppBarButtonHelpers::CloseSubMenusOnPointerEntered<AppBarButton>(this, this));
    return S_OK;
}

IFACEMETHODIMP AppBarButton::OnPointerExited(_In_ xaml_input::IPointerRoutedEventArgs* args)
{
    IFC_RETURN(AppBarButtonGenerated::OnPointerExited(args));

    BOOLEAN isInOverflow = FALSE;
    IFC_RETURN(get_IsInOverflow(&isInOverflow));

    if (isInOverflow && m_menuHelper)
    {
        IFC_RETURN(m_menuHelper->OnPointerExited(args, false /* parentIsSubMenu */));
    }

    return S_OK;
}

IFACEMETHODIMP AppBarButton::OnKeyDown(_In_ xaml_input::IKeyRoutedEventArgs* args)
{
    IFC_RETURN(AppBarButtonGenerated::OnKeyDown(args));

    BOOLEAN isInOverflow = FALSE;
    IFC_RETURN(get_IsInOverflow(&isInOverflow));

    if (isInOverflow && m_menuHelper)
    {
        IFC_RETURN(m_menuHelper->OnKeyDown(args));
    }

    return S_OK;
}

IFACEMETHODIMP AppBarButton::OnKeyUp(_In_ xaml_input::IKeyRoutedEventArgs* args)
{
    IFC_RETURN(AppBarButtonGenerated::OnKeyUp(args));

    BOOLEAN isInOverflow = FALSE;
    IFC_RETURN(get_IsInOverflow(&isInOverflow));

    if (isInOverflow && m_menuHelper)
    {
        IFC_RETURN(m_menuHelper->OnKeyUp(args));
    }

    return S_OK;
}

_Check_return_
HRESULT AppBarButton::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(AppBarButtonGenerated::OnPropertyChanged2(args));

    if (args.m_pDP->GetIndex() == KnownPropertyIndex::Button_Flyout)
    {
        ctl::ComPtr<IInspectable> oldFlyoutAsI;
        ctl::ComPtr<IInspectable> newFlyoutAsI;
        IFC_RETURN(PropertyChangedParamsHelper::GetObjects(args, &oldFlyoutAsI, &newFlyoutAsI));

        ctl::ComPtr<IFlyoutBase> oldFlyout;
        ctl::ComPtr<IFlyoutBase> newFlyout;
        IFC_RETURN(oldFlyoutAsI.As(&oldFlyout));
        IFC_RETURN(newFlyoutAsI.As(&newFlyout));

        if (oldFlyout)
        {
            IFC_RETURN(m_flyoutOpenedHandler.DetachEventHandler(oldFlyout.Get()));
            IFC_RETURN(m_flyoutClosedHandler.DetachEventHandler(oldFlyout.Get()));

            m_menuHelper.Clear();
        }

        if (newFlyout)
        {
            ctl::ComPtr<CascadingMenuHelper> menuHelper;
            IFC_RETURN(ctl::make(this, &menuHelper));
            SetPtrValue(m_menuHelper, menuHelper.Get());

            ctl::WeakRefPtr wrThis;
            IFC_RETURN(ctl::AsWeak(this, &wrThis));

            auto flyoutOpenStateChangedHandler = [wrThis](bool isOpen) mutable
            {
                ctl::ComPtr<AppBarButton> appBarButton = wrThis.AsOrNull<AppBarButton>();
                if (appBarButton)
                {
                    appBarButton->m_isFlyoutClosing = false;
                    IFC_RETURN(appBarButton->UpdateVisualState());
                }
                return S_OK;
            };

            auto openedHandler = [flyoutOpenStateChangedHandler](IInspectable* pSender, IInspectable* pArgs) mutable
            {
                IFC_RETURN(flyoutOpenStateChangedHandler(true));
                return S_OK;
            };

            auto closedHandler = [flyoutOpenStateChangedHandler](IInspectable* pSender, IInspectable* pArgs) mutable
            {
                IFC_RETURN(flyoutOpenStateChangedHandler(false));
                return S_OK;
            };

            IFC_RETURN(m_flyoutOpenedHandler.AttachEventHandler(newFlyout.Get(), openedHandler));
            IFC_RETURN(m_flyoutClosedHandler.AttachEventHandler(newFlyout.Get(), closedHandler));
        }
    }

    IFC_RETURN(AppBarButtonHelpers::OnPropertyChanged<AppBarButton>(this, args));
    return S_OK;
}

// After template is applied, set the initial view state
// (FullSize or Compact) based on the value of our
// IsCompact property
IFACEMETHODIMP AppBarButton::OnApplyTemplate()
{
    IFC_RETURN(AppBarButtonHelpers::OnBeforeApplyTemplate<AppBarButton>(this));
    IFC_RETURN(AppBarButtonGenerated::OnApplyTemplate());
    IFC_RETURN(AppBarButtonHelpers::OnApplyTemplate<AppBarButton>(this));
    return S_OK;
}

// Sets the visual state to "Compact" or "FullSize" based on the value
// of our IsCompact property.
_Check_return_ HRESULT AppBarButton::ChangeVisualState(_In_ bool useTransitions)
{
    BOOLEAN ignored = FALSE;
    BOOLEAN useOverflowStyle = FALSE;

    // From explorer.exe profiling, this spares ~2700 resource lookups for "AppBarButtonForegroundDisabled" when opening
    // a new File Explorer window.
    // There may be other VSM transitions where we can cache theme resources for some savings.
    auto endOnExit = DXamlCore::GetCurrent()->GetHandle()->GetThemeWalkResourceCache()->BeginCachingThemeResources();

    IFC_RETURN(AppBarButtonGenerated::ChangeVisualState(useTransitions));
    IFC_RETURN(get_UseOverflowStyle(&useOverflowStyle));

    if (useOverflowStyle)
    {
        if (m_isWithToggleButtons && m_isWithIcons)
        {
            IFC_RETURN(GoToState(useTransitions, L"OverflowWithToggleButtonsAndMenuIcons", &ignored));
        }
        else if (m_isWithToggleButtons)
        {
            IFC_RETURN(GoToState(useTransitions, L"OverflowWithToggleButtons", &ignored));
        }
        else if (m_isWithIcons)
        {
            IFC_RETURN(GoToState(useTransitions, L"OverflowWithMenuIcons", &ignored));
        }
        else
        {
            IFC_RETURN(GoToState(useTransitions, L"Overflow", &ignored));
        }

        if (m_isWithIcons)
        {
            BOOLEAN isSubMenuOpen = FALSE;
            BOOLEAN isEnabled = FALSE;
            BOOLEAN isPressed = FALSE;
            BOOLEAN isPointerOver = FALSE;

            IFC_RETURN(get_IsSubMenuOpen(&isSubMenuOpen));
            IFC_RETURN(get_IsEnabled(&isEnabled));
            IFC_RETURN(get_IsPressed(&isPressed));
            IFC_RETURN(get_IsPointerOver(&isPointerOver));

            if (isSubMenuOpen && !m_isFlyoutClosing)
            {
                IFC_RETURN(GoToState(useTransitions, L"OverflowSubMenuOpened", &ignored));
            }
            else if (isPressed)
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

    ctl::ComPtr<xaml_primitives::IFlyoutBase> flyout;
    IFC_RETURN(get_Flyout(&flyout));

    if (flyout)
    {
        IFC_RETURN(GoToState(useTransitions, L"HasFlyout", &ignored));
    }
    else
    {
        IFC_RETURN(GoToState(useTransitions, L"NoFlyout", &ignored));
    }

    IFC_RETURN(AppBarButtonHelpers::ChangeCommonVisualStates<AppBarButton>(this, useTransitions));
    return S_OK;
}

// Create AppBarButtonAutomationPeer to represent the AppBarButton.
IFACEMETHODIMP AppBarButton::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IAppBarButtonAutomationPeer> spAppBarButtonAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IAppBarButtonAutomationPeerFactory> spAppBarButtonAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::AppBarButtonAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spAppBarButtonAPFactory));

    IFC(spAppBarButtonAPFactory.Cast<AppBarButtonAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spAppBarButtonAutomationPeer));
    IFC(spAppBarButtonAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
AppBarButton::OnClick()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_primitives::IFlyoutBase> spFlyoutBase;

    // Don't execute the logic on CommandBar to close the secondary
    // commands popup when we have a flyout associated with this button.
    IFC(get_Flyout(&spFlyoutBase));
    if (!spFlyoutBase)
    {
        IFC(CommandBar::OnCommandExecutionStatic(this));
    }

    IFC(AppBarButtonGenerated::OnClick());

Cleanup:
    return hr;
}

_Check_return_ HRESULT
AppBarButton::OnVisibilityChanged()
{
    IFC_RETURN(__super::OnVisibilityChanged());
    return CommandBar::OnCommandBarElementVisibilityChanged(this);
}

_Check_return_ HRESULT
AppBarButton::OnCommandChanged(_In_  IInspectable* pOldValue, _In_ IInspectable* pNewValue)
{
    IFC_RETURN(AppBarButtonGenerated::OnCommandChanged(pOldValue, pNewValue));
    IFC_RETURN(AppBarButtonHelpers::OnCommandChanged<AppBarButton>(this, pOldValue, pNewValue));
    return S_OK;
}

_Check_return_ HRESULT
AppBarButton::OpenAssociatedFlyout()
{
    BOOLEAN isInOverflow = FALSE;
    IFC_RETURN(get_IsInOverflow(&isInOverflow));

    // If we call OpenSubMenu, that causes the menu not to have a light-dismiss layer, as it assumes
    // that its parent will have one.  That's not the case for AppBarButtons that aren't in overflow,
    // so we'll just open the flyout normally in this circumstance.
    if (m_menuHelper && isInOverflow)
    {
        IFC_RETURN(m_menuHelper->OpenSubMenu());
    }
    else
    {
        IFC_RETURN(AppBarButtonGenerated::OpenAssociatedFlyout());
    }

    return S_OK;
}

_Check_return_ HRESULT AppBarButton::GetEffectiveLabelPosition(_Out_ xaml_controls::CommandBarDefaultLabelPosition *effectiveLabelPosition)
{
    xaml_controls::CommandBarLabelPosition labelPosition;
    IFC_RETURN(get_LabelPosition(&labelPosition));

    *effectiveLabelPosition = static_cast<xaml_controls::CommandBarDefaultLabelPosition>(labelPosition == xaml_controls::CommandBarLabelPosition_Collapsed ? xaml_controls::CommandBarDefaultLabelPosition_Collapsed : m_defaultLabelPosition);
    return S_OK;
}

_Check_return_ HRESULT AppBarButton::UpdateInternalStyles()
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
    // only play auto width animation when the width is not overrided by local/animation setting
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

    IFC_RETURN(AppBarButtonHelpers::UpdateToolTip<AppBarButton>(this));
    return S_OK;
}

_Check_return_ HRESULT
AppBarButton::get_IsInOverflowImpl(_Out_ BOOLEAN* pValue)
{
    return CommandBar::IsCommandBarElementInOverflow(this, pValue);
}

_Check_return_ HRESULT
AppBarButton::CreateStoryboardForWidthAdjustmentsForLabelOnRightStyle(
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
AppBarButton::StartAnimationForWidthAdjustments()
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
AppBarButton::StopAnimationForWidthAdjustments()
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

_Check_return_ HRESULT
AppBarButton::get_KeyboardAcceleratorTextOverrideImpl(_Out_ HSTRING* pValue)
{
    IFC_RETURN(AppBarButtonHelpers::GetKeyboardAcceleratorText<AppBarButton>(this, pValue));
    return S_OK;
}

_Check_return_ HRESULT
AppBarButton::put_KeyboardAcceleratorTextOverrideImpl(_In_opt_ HSTRING value)
{
    IFC_RETURN(AppBarButtonHelpers::PutKeyboardAcceleratorText<AppBarButton>(this, value));
    return S_OK;
}

_Check_return_ HRESULT
AppBarButton::get_IsSubMenuOpenImpl(_Out_ BOOLEAN* pValue)
{
    *pValue = FALSE;

    ctl::ComPtr<IFlyoutBase> flyout;
    IFC_RETURN(get_Flyout(&flyout));

    if (flyout)
    {
        IFC_RETURN(flyout.Cast<FlyoutBase>()->get_IsOpen(pValue));
    }

    return S_OK;
}

_Check_return_ HRESULT
AppBarButton::get_ParentOwnerImpl(_Outptr_ ISubMenuOwner** ppValue)
{
    *ppValue = NULL;
    return S_OK;
}

_Check_return_ HRESULT
AppBarButton::put_ParentOwnerImpl(_In_ ISubMenuOwner* pValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT
AppBarButton::SetSubMenuDirectionImpl(BOOLEAN isSubMenuDirectionUp)
{
    return S_OK;
}

_Check_return_ HRESULT
AppBarButton::PrepareSubMenuImpl()
{
    ctl::ComPtr<IFlyoutBase> flyout;
    IFC_RETURN(get_Flyout(&flyout));

    if (!flyout)
    {
        return S_OK;
    }

    ctl::ComPtr<DependencyObject> flyoutAsDO;
    IFC_RETURN(flyout.As(&flyoutAsDO));

    if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(flyoutAsDO.Get()))
    {
        ctl::ComPtr<IUIElement> rootVisual;
        IFC_RETURN(xamlRoot->get_ContentImpl(&rootVisual));
        IFC_RETURN(flyout.Cast<FlyoutBase>()->put_OverlayInputPassThroughElement(rootVisual.Cast<UIElement>()));
    }

    if (const auto flyoutAsMenu = flyout.AsOrNull<IMenu>())
    {
        ctl::ComPtr<ICommandBar> parentCommandBar;
        IFC_RETURN(CommandBar::FindParentCommandBarForElement(this, &parentCommandBar));

        if (parentCommandBar)
        {
            IFC_RETURN(flyoutAsMenu->put_ParentMenu(parentCommandBar.Cast<CommandBar>()));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
AppBarButton::OpenSubMenuImpl(wf::Point position)
{
    ctl::ComPtr<IFlyoutBase> flyout;
    IFC_RETURN(get_Flyout(&flyout));

    if (flyout)
    {
        ctl::ComPtr<FlyoutShowOptions> showOptions;
        IFC_RETURN(ctl::make(&showOptions));

        BOOLEAN isInOverflow = FALSE;
        IFC_RETURN(get_IsInOverflow(&isInOverflow));

        // We shouldn't be doing anything special to open flyouts for AppBarButtons
        // that are not in overflow.
        ASSERT(isInOverflow);

        IFC_RETURN(showOptions->put_Placement(xaml_primitives::FlyoutPlacementMode_RightEdgeAlignedTop));

        // In order to avoid the sub-menu from showing on top of the menu, the FlyoutShowOptions.ExclusionRect property is set to the width
        // of the AppBarButton minus the small overlap on the left & right edges. The exclusion height is set to the height of the button.
        double itemWidth = 0;
        double itemHeight = 0;
        IFC_RETURN(get_ActualWidth(&itemWidth));
        IFC_RETURN(get_ActualHeight(&itemHeight));

        float overlap = static_cast<float>(itemWidth) - position.X;
        wf::Rect exclusionRect = {};

        exclusionRect.X = overlap;
        exclusionRect.Width = position.X - overlap;
        exclusionRect.Height = static_cast<float>(itemHeight);

        ctl::ComPtr<wf::IReference<wf::Rect>> targetExclusionRect;
        IFC_RETURN(PropertyValue::CreateTypedReference<wf::Rect>(exclusionRect, &targetExclusionRect));
        IFC_RETURN(showOptions->put_ExclusionRect(targetExclusionRect.Get()));

        ctl::ComPtr<wf::IReference<wf::Point>> targetPosition;
        IFC_RETURN(PropertyValue::CreateTypedReference<wf::Point>(position, &targetPosition));
        IFC_RETURN(showOptions->put_Position(targetPosition.Get()));

        ctl::ComPtr<IFlyoutShowOptions> showOptionsAsI;
        IFC_RETURN(showOptions.As(&showOptionsAsI));
        IFC_RETURN(flyout.Cast<FlyoutBase>()->ShowAtWithOptions(this, showOptionsAsI.Get()));

        Control* flyoutPresenterNoRef = static_cast<Control*>(flyout.Cast<FlyoutBase>()->GetPresenter());

        ASSERT(m_menuHelper);
        IFC_RETURN(m_menuHelper->SetSubMenuPresenter(flyoutPresenterNoRef));

        BOOLEAN focusUpdated = FALSE;
        IFC_RETURN(DependencyObject::SetFocusedElement(
            flyoutPresenterNoRef,
            xaml::FocusState_Programmatic,
            FALSE /*animateIfBringIntoView*/,
            &focusUpdated));
    }

    m_lastFlyoutPosition = position;
    return S_OK;
}

_Check_return_ HRESULT
AppBarButton::PositionSubMenuImpl(wf::Point position)
{
    IFC_RETURN(CloseSubMenu());

    if (position.X == -std::numeric_limits<float>::infinity())
    {
        position.X = m_lastFlyoutPosition.X;
    }

    if (position.Y == -std::numeric_limits<float>::infinity())
    {
        position.Y = m_lastFlyoutPosition.Y;
    }

    IFC_RETURN(OpenSubMenu(position));
    return S_OK;
}

_Check_return_ HRESULT
AppBarButton::ClosePeerSubMenusImpl()
{
    ctl::ComPtr<ICommandBar> parentCommandBar;
    IFC_RETURN(CommandBar::FindParentCommandBarForElement(this, &parentCommandBar));

    if (parentCommandBar)
    {
        IFC_RETURN(parentCommandBar.Cast<CommandBar>()->CloseSubMenus(this));
    }

    return S_OK;
}

_Check_return_ HRESULT
AppBarButton::CloseSubMenuImpl()
{
    ctl::ComPtr<IFlyoutBase> flyout;
    IFC_RETURN(get_Flyout(&flyout));

    if (flyout)
    {
        // If the sub-menu flyout has the current keyboard focus, then we'll
        // move keyboard focus to this button in order to avoid having a period
        // where XAML keyboard focus is nowhere.
        ctl::ComPtr<DependencyObject> focusedElement;
        IFC_RETURN(GetFocusedElement(&focusedElement));

        if(focusedElement != nullptr)
        {
            CUIElement* focusedElementCoreAsUIE = do_pointer_cast<CUIElement>(focusedElement->GetHandle());

            if (focusedElementCoreAsUIE != nullptr &&
                CPopup::GetClosestFlyoutAncestor(focusedElementCoreAsUIE) == flyout.Cast<FlyoutBase>()->GetHandle())
            {
                BOOLEAN ignored;
                IFC_RETURN(Focus(xaml::FocusState_Programmatic, &ignored));
            }
        }

        IFC_RETURN(flyout.Cast<FlyoutBase>()->Hide());

        // The Closing event is raised after the fade-out animation completes,
        // whereas we want to stop showing the sub-menu open state as soon
        // as we know we're moving out of it.  So we'll manually update the
        // visual state here.
        m_isFlyoutClosing = true;
        IFC_RETURN(UpdateVisualState());
    }

    return S_OK;
}

_Check_return_ HRESULT
AppBarButton::CloseSubMenuTreeImpl()
{
    if (m_menuHelper)
    {
        IFC_RETURN(m_menuHelper->CloseChildSubMenus());
    }

    return S_OK;
}

_Check_return_ HRESULT
AppBarButton::DelayCloseSubMenuImpl()
{
    if (m_menuHelper)
    {
        IFC_RETURN(m_menuHelper->DelayCloseSubMenu());
    }
    return S_OK;
}

_Check_return_ HRESULT
AppBarButton::CancelCloseSubMenuImpl()
{
    if (m_menuHelper)
    {
        IFC_RETURN(m_menuHelper->CancelCloseSubMenu());
    }
    return S_OK;
}

_Check_return_ HRESULT
AppBarButton::RaiseAutomationPeerExpandCollapseImpl(
    _In_ BOOLEAN isOpen)
{
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;
    BOOLEAN isListener = FALSE;

    IFC_RETURN(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_PropertyChanged, &isListener));
    if (isListener)
    {
        IFC_RETURN(GetOrCreateAutomationPeer(&spAutomationPeer));
        if (spAutomationPeer)
        {
            IFC_RETURN(spAutomationPeer.Cast<AppBarButtonAutomationPeer>()->RaiseExpandCollapseAutomationEvent(isOpen));
        }
    }
    return S_OK;
}
