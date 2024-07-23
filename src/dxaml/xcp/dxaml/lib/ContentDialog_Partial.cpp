// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ContentDialog.g.h"
#include <windows.ui.viewmanagement.h>
#include <windows.graphics.display.h>
#include "focusmgr.h"
#include "Button.g.h"
#include "Window.g.h"
#include "Popup.g.h"
#include "Rectangle.g.h"
#include "Border.g.h"
#include "Grid.g.h"
#include "ContentDialogOpenedEventArgs.g.h"
#include "ContentDialogClosedEventArgs.g.h"
#include "ContentDialogClosingEventArgs.g.h"
#include "ContentDialogButtonClickEventArgs.g.h"
#include "KeyRoutedEventArgs.g.h"
#include "PopupThemeTransition.g.h"
#include "TransitionCollection.g.h"
#include "Page.g.h"
#include "ScrollViewer.g.h"
#include "ContentDialogOpenCloseThemeTransition.g.h"
#include <XboxUtility.h>
#include <FrameworkUdk/BackButtonIntegration.h>
#include "KeyboardAcceleratorInvokedEventArgs.g.h"
#include "AutomationProperties.h"
#include "Storyboard.g.h"
#include "TimelineCollection.g.h"
#include <DesignMode.h>
#include "ObjectAnimationUsingKeyFrames.g.h"
#include "ObjectKeyFrameCollection.g.h"
#include "DiscreteObjectKeyFrame.g.h"
#include "ProcessKeyboardAcceleratorEventArgs.g.h"
#include <XamlOneCoreTransforms.h>
#include "InitialFocusSIPSuspender.h"
#include <windows.foundation.metadata.h>
#include "CommandingHelpers.h"
#include "PropertyChangedParamsHelper.h"
#include "ContentDialogMetadata.h"
#include "ElevationHelper.h"
#include "ThemeShadow.h"
#include <XamlRoot_Partial.h>
#include "WindowChrome_Partial.h"
#include "ElementSoundPlayerService_Partial.h"
#include "XamlTelemetry.h"

#undef min
#undef max

using namespace std::placeholders;

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace Microsoft::WRL::Wrappers;

ULONG ContentDialog::z_ulUniqueAsyncActionId = 1;

static const DOUBLE ContentDialog_SIP_Bottom_Margin = 12.0;

ContentDialog::~ContentDialog()
{
    VERIFYHR(DetachEventHandlers());

    auto xamlRoot = XamlRoot::GetForElementStatic(this);
    if (m_xamlRootChangedEventHandler && xamlRoot)
    {
        VERIFYHR(m_xamlRootChangedEventHandler.DetachEventHandler(xamlRoot.Get()));
    }

    if (auto popup = m_tpPopup.GetSafeReference())
    {
        if (m_popupOpenedHandler)
        {
            VERIFYHR(m_popupOpenedHandler.DetachEventHandler(popup.Get()));
        }
    }
}

_Check_return_ HRESULT
ContentDialog::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(__super::OnPropertyChanged2(args));

    // We only react to property changes in a ContentDialog is currently visible or in design mode
    if ((!m_tpCurrentAsyncOperation && !DesignerInterop::GetDesignerMode(DesignerMode::V2Only)) || // or no active ShowAsync operation and not under the designer
        !m_hasPreparedContent || // or content hasn't been prepared
        m_hideInProgress
        )
    {
        return S_OK;
    }

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::ContentDialog_FullSizeDesired:
    case KnownPropertyIndex::ContentDialog_PrimaryButtonText:
    case KnownPropertyIndex::ContentDialog_SecondaryButtonText:
    case KnownPropertyIndex::ContentDialog_CloseButtonText:
    case KnownPropertyIndex::ContentDialog_DefaultButton:
        IFC_RETURN(UpdateVisualState());
        break;

    case KnownPropertyIndex::ContentDialog_IsPrimaryButtonEnabled:
    case KnownPropertyIndex::ContentDialog_IsSecondaryButtonEnabled:
        break;

    case KnownPropertyIndex::ContentDialog_PrimaryButtonCommand:
        {
            ctl::ComPtr<IInspectable> oldCommandAsI;
            ctl::ComPtr<IInspectable> newCommandAsI;
            ctl::ComPtr<ICommand> oldCommand;

            IFC_RETURN(PropertyChangedParamsHelper::GetObjects(args, &oldCommandAsI, &newCommandAsI));
            oldCommand = oldCommandAsI.AsOrNull<ICommand>();

            IFC_RETURN(SetButtonPropertiesFromCommand(xaml_controls::ContentDialogButton_Primary, oldCommand.Get()));
        }
        break;

    case KnownPropertyIndex::ContentDialog_SecondaryButtonCommand:
        {
            ctl::ComPtr<IInspectable> oldCommandAsI;
            ctl::ComPtr<IInspectable> newCommandAsI;
            ctl::ComPtr<ICommand> oldCommand;

            IFC_RETURN(PropertyChangedParamsHelper::GetObjects(args, &oldCommandAsI, &newCommandAsI));
            oldCommand = oldCommandAsI.AsOrNull<ICommand>();

            IFC_RETURN(SetButtonPropertiesFromCommand(xaml_controls::ContentDialogButton_Secondary, oldCommand.Get()));
        }
        break;

    case KnownPropertyIndex::ContentDialog_CloseButtonCommand:
        {
            ctl::ComPtr<IInspectable> oldCommandAsI;
            ctl::ComPtr<IInspectable> newCommandAsI;
            ctl::ComPtr<ICommand> oldCommand;

            IFC_RETURN(PropertyChangedParamsHelper::GetObjects(args, &oldCommandAsI, &newCommandAsI));
            oldCommand = oldCommandAsI.AsOrNull<ICommand>();

            IFC_RETURN(SetButtonPropertiesFromCommand(xaml_controls::ContentDialogButton_Close, oldCommand.Get()));
        }
        break;

    case KnownPropertyIndex::ContentDialog_Title:
    case KnownPropertyIndex::ContentDialog_TitleTemplate:
        IFC_RETURN(UpdateTitleSpaceVisibility());
        IFC_RETURN(SetPopupAutomationProperties());
        break;

    case KnownPropertyIndex::FrameworkElement_FlowDirection:
        if (m_placementMode != PlacementMode::InPlace)
        {
            IFC_RETURN(SizeAndPositionContentInPopup());
        }
        break;

    case KnownPropertyIndex::AutomationProperties_AutomationId:
        IFC_RETURN(SetPopupAutomationProperties());
        break;
    }

    return S_OK;
}

IFACEMETHODIMP ContentDialog::OnApplyTemplate()
{
    if (m_isSmokeLayerATemplatePart)
    {
        m_isSmokeLayerATemplatePart = false;
        m_tpSmokeLayer.Clear();
    }

    IFC_RETURN(DetachEventHandlers());
    m_tpDialogShowingStates.Clear();

    IFC_RETURN(__super::OnApplyTemplate());

    m_isLayoutRootTransplanted = false;

    m_isTemplateApplied = true;

    if (DesignerInterop::GetDesignerMode(DesignerMode::V2Only))
    {
        // In designer mode the dialog is displayed in-place.  Do nothing special
        // to display the popup, but only worry about the contents
        m_isShowing = true;
        m_placementMode = PlacementMode::InPlace;

        IFC_RETURN(PrepareContent());
    }

    // For dialogs that were shown when not in the visual tree, since we couldn't prepare
    // their content during the ShowAsync() call, do it now that it's loaded.
    if (m_placementMode == PlacementMode::EntireControlInPopup)
    {
        IFC_RETURN(PrepareContent());
    }

    // On non-PhoneBlue templates, it is possible to resize the app, in which case we would like to reposition the
    // ContentDialog. FullSize behavior also needs to be checked as the window height might become smaller than
    // the MaxHeight, in which case positioning behavior changes.
    auto xamlRoot = XamlRoot::GetForElementStatic(this);
    if (xamlRoot)
    {
        ctl::WeakRefPtr weakInstance;
        IFC_RETURN(ctl::AsWeak(this, &weakInstance));

        auto handler = [weakInstance](xaml::IXamlRoot* sender, xaml::IXamlRootChangedEventArgs* args) mutable
        {
            if (auto instance = weakInstance.AsOrNull<IContentDialog>())
            {
                IFC_RETURN(instance.Cast<ContentDialog>()->OnXamlRootChanged(sender, args));
            }
            return S_OK;
        };

        IFC_RETURN(m_xamlRootChangedEventHandler.AttachEventHandler(xamlRoot.Get(), handler));
    }

    if (m_tpLayoutRootPart)
    {
        IFC_RETURN(m_epLayoutRootLoadedHandler.AttachEventHandler(
            m_tpLayoutRootPart.AsOrNull<IFrameworkElement>().Get(),
            std::bind(&ContentDialog::OnLayoutRootLoaded, this, _1, _2)));

        IFC_RETURN(m_epLayoutRootKeyDownHandler.AttachEventHandler(
            m_tpLayoutRootPart.AsOrNull<IUIElement>().Get(),
            std::bind(&ContentDialog::OnLayoutRootKeyDown, this, _1, _2)));

        IFC_RETURN(m_epLayoutRootKeyUpHandler.AttachEventHandler(
            m_tpLayoutRootPart.AsOrNull<IUIElement>().Get(),
            std::bind(&ContentDialog::OnLayoutRootKeyUp, this, _1, _2)));

        IFC_RETURN(m_epLayoutRootGotFocusHandler.AttachEventHandler(
            m_tpLayoutRootPart.AsOrNull<IUIElement>().Get(),
            [this](IInspectable*, xaml::IRoutedEventArgs*)
            {
                // Update which command button has the default button visualization.
                return UpdateVisualState();
            }));

        IFC_RETURN(m_epLayoutRootProcessKeyboardAcceleratorsHandler.AttachEventHandler(
            m_tpLayoutRootPart.AsOrNull<IUIElement>().Get(),
            std::bind(&ContentDialog::OnLayoutRootProcessKeyboardAccelerators, this, _1, _2)));
    }

    if (m_tpBackgroundElementPart)
    {
        IFC_RETURN(m_dialogSizeChangedHandler.AttachEventHandler(
            m_tpBackgroundElementPart.AsOrNull<IFrameworkElement>().Get(),
            std::bind(&ContentDialog::OnDialogSizeChanged, this, _1, _2)));
    }

    IFC_RETURN(AttachButtonEvents());

    // In case the commands were set before the template was applied, we won't have responded to the property-change event.
    // We should set the button properties from the commands at this point to ensure they're set properly.
    // If they've already been set before, this will be a no-op, since we check to make sure that the properties
    // are unset before we set them.
    IFC_RETURN(SetButtonPropertiesFromCommand(xaml_controls::ContentDialogButton_Primary));
    IFC_RETURN(SetButtonPropertiesFromCommand(xaml_controls::ContentDialogButton_Secondary));
    IFC_RETURN(SetButtonPropertiesFromCommand(xaml_controls::ContentDialogButton_Close));

    // If the template changes while the dialog is showing, we need to re-attached to the
    // dialog showing states changed event so that we can fire the hiding event at the
    // right time.
    if (m_placementMode == PlacementMode::InPlace && m_isShowing)
    {
        ctl::ComPtr<xaml::IVisualStateGroup> dialogShowingStates;
        IFC_RETURN(GetTemplatePart<xaml::IVisualStateGroup>(STR_LEN_PAIR(L"DialogShowingStates"), dialogShowingStates.ReleaseAndGetAddressOf()));
        if (dialogShowingStates)
        {
            IFC_RETURN(m_dialogShowingStateChangedEventHandler.AttachEventHandler(dialogShowingStates.Get(), std::bind(&ContentDialog::OnDialogShowingStateChanged, this, _1, _2)));
            SetPtrValue(m_tpDialogShowingStates, dialogShowingStates);
        }
    }
     
    // Attempt to set the m_tpSmokeLayer field as a FrameworkElement template part, in Popup placement.
    if (m_tpLayoutRootPart)
    {
        ctl::ComPtr<IFrameworkElement> smokeLayerPartAsFE;

        IFC_RETURN(GetTemplatePart<IFrameworkElement>(STR_LEN_PAIR(L"SmokeLayerBackground"), smokeLayerPartAsFE.ReleaseAndGetAddressOf()));

        if (smokeLayerPartAsFE)
        {
            // Unparent smokeLayerPartAsFE in both Popup and InPlace placements. It can be reparented later in ContentDialog::PrepareSmokeLayer() in Popup placement.
            ctl::ComPtr<wfc::IVector<xaml::UIElement*>> layoutRootChildren;

            IFC_RETURN(m_tpLayoutRootPart.Cast<Grid>()->get_Children(&layoutRootChildren));

            UINT32 indexOfSmokeLayerPart = 0;
            BOOLEAN wasFound = FALSE;

            IFC_RETURN(layoutRootChildren->IndexOf(smokeLayerPartAsFE.AsOrNull<IUIElement>().Get(), &indexOfSmokeLayerPart, &wasFound));

            if (wasFound)
            {
                IFC_RETURN(layoutRootChildren->RemoveAt(indexOfSmokeLayerPart));

                if (m_placementMode == PlacementMode::TransplantedRootInPopup || m_placementMode == PlacementMode::EntireControlInPopup)
                {
                    // When m_tpSmokeLayer was previously set by a PrepareSmokeLayer call while m_tpLayoutRootPart was still unknown,
                    // HostDialogWithinPopup needs to be called again to set up m_tpSmokeLayer, m_tpSmokeLayerPopup and m_tpPopup.
                    // For instance, m_tpSmokeLayer needs to be sized and parented to m_tpSmokeLayerPopup and m_tpPopup needs to
                    // redefine its TransitionCollection. HostDialogWithinPopup will be called in the imminent OnDialogSizeChanged call
                    // where reparenting is allowed again.
                    m_prepareSmokeLayerAndPopup = m_tpSmokeLayer != nullptr;
                    SetPtrValueWithQIOrNull(m_tpSmokeLayer, smokeLayerPartAsFE.Get());

                    m_isSmokeLayerATemplatePart = true;
                }
            }
        }
    }

    IFC_RETURN(UpdateVisualState());

    return S_OK;
}

IFACEMETHODIMP ContentDialog::ArrangeOverride(
    _In_ wf::Size arrangeSize,
    _Out_ wf::Size* returnValue)
{
    if (m_isShowing)
    {
        IFC_RETURN(__super::ArrangeOverride(arrangeSize, returnValue));
    }
    else
    {
        returnValue->Width = 0;
        returnValue->Height = 0;
    }

    return S_OK;
}

_Check_return_ HRESULT ContentDialog::ChangeVisualState(_In_ bool useTransitions)
{
    IFC_RETURN(__super::ChangeVisualState(useTransitions));

    if (!m_isTemplateApplied)
    {
        return S_OK;
    }

    BOOLEAN fullSizeDesired = FALSE;
    IFC_RETURN(get_FullSizeDesired(&fullSizeDesired));

    // ButtonsVisibilityStates
    {
        wrl_wrappers::HString primaryText;
        IFC_RETURN(get_PrimaryButtonText(primaryText.ReleaseAndGetAddressOf()));

        wrl_wrappers::HString secondaryText;
        IFC_RETURN(get_SecondaryButtonText(secondaryText.ReleaseAndGetAddressOf()));

        wrl_wrappers::HString closeText;
        IFC_RETURN(get_CloseButtonText(closeText.ReleaseAndGetAddressOf()));

        bool hasPrimary = !primaryText.IsEmpty();
        bool hasSecondary = !secondaryText.IsEmpty();
        bool hasClose = !closeText.IsEmpty();

        const wchar_t* buttonVisibilityState = L"NoneVisible";
        if (hasPrimary && hasSecondary && hasClose)
        {
            buttonVisibilityState = L"AllVisible";
        }
        else if (hasPrimary && hasSecondary)
        {
            buttonVisibilityState = L"PrimaryAndSecondaryVisible";
        }
        else if (hasPrimary && hasClose)
        {
            buttonVisibilityState = L"PrimaryAndCloseVisible";
        }
        else if (hasSecondary && hasClose)
        {
            buttonVisibilityState = L"SecondaryAndCloseVisible";
        }
        else if (hasPrimary)
        {
            buttonVisibilityState = L"PrimaryVisible";
        }
        else if (hasSecondary)
        {
            buttonVisibilityState = L"SecondaryVisible";
        }
        else if (hasClose)
        {
            buttonVisibilityState = L"CloseVisible";
        }

        BOOLEAN ignored = FALSE;
        IFC_RETURN(GoToState(useTransitions, buttonVisibilityState, &ignored));
    }

    // DefaultButtonStates
    {
        const wchar_t* defaultButtonState = L"NoDefaultButton";

        auto defaultButton = xaml_controls::ContentDialogButton_None;
        IFC_RETURN(get_DefaultButton(&defaultButton));

        if (defaultButton != xaml_controls::ContentDialogButton_None)
        {
            ctl::ComPtr<DependencyObject> focusedElement;
            IFC_RETURN(GetFocusedElement(&focusedElement));

            BOOLEAN isFocusInCommandArea = FALSE;
            IFC_RETURN(m_tpCommandSpacePart.Cast<Grid>()->IsAncestorOf(focusedElement.Get(), &isFocusInCommandArea));

            // If focus is not in the command area, set the default button visualization just based on the property value.
            // If focus is in the command area, set the default button visualization only if it has focus.
            if (defaultButton == xaml_controls::ContentDialogButton_Primary)
            {
                if (!isFocusInCommandArea || ctl::are_equal(m_tpPrimaryButtonPart.Get(), focusedElement.Get()))
                {
                    defaultButtonState = L"PrimaryAsDefaultButton";
                }
            }
            else if (defaultButton == xaml_controls::ContentDialogButton_Secondary)
            {
                if (!isFocusInCommandArea || ctl::are_equal(m_tpSecondaryButtonPart.Get(), focusedElement.Get()))
                {
                    defaultButtonState = L"SecondaryAsDefaultButton";
                }
            }
            else if (defaultButton == xaml_controls::ContentDialogButton_Close)
            {
                if (!isFocusInCommandArea || ctl::are_equal(m_tpCloseButtonPart.Get(), focusedElement.Get()))
                {
                    defaultButtonState = L"CloseAsDefaultButton";
                }
            }
        }

        BOOLEAN ignored = FALSE;
        IFC_RETURN(GoToState(useTransitions, defaultButtonState, &ignored));
    }

    // DialogShowingStates
    if (m_placementMode == PlacementMode::InPlace)
    {
        if (m_tpDialogShowingStates)
        {
            BOOLEAN ignored = FALSE;
            IFC_RETURN(GoToState(true, m_isShowing && !m_hideInProgress ? L"DialogShowing" : L"DialogHidden", &ignored));
        }
        else if (m_tpLayoutRootPart)
        {
            // We don't have a state transition defined so brute force it.
            IFC_RETURN(m_tpLayoutRootPart.AsOrNull<IUIElement>()->put_Visibility(m_isShowing && !m_hideInProgress ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
        }
    }
    else if (m_placementMode != PlacementMode::Undetermined)
    {
        // For ContentDialog's shown in the popup, set the state to always showing since the opened
        // state of the popup effectively controls whether its showing it not.
        BOOLEAN ignored = FALSE;
        IFC_RETURN(GoToState(false, L"DialogShowingWithoutSmokeLayer", &ignored));
    }

    // DialogSizingStates
    {
        BOOLEAN ignored = FALSE;
        IFC_RETURN(GoToState(useTransitions, fullSizeDesired ? L"FullDialogSizing" : L"DefaultDialogSizing", &ignored));
    }

    // DialogBorderStates
    {
        BOOLEAN ignored = FALSE;
        IFC_RETURN(GoToState(useTransitions, L"NoBorder", &ignored));
    }

    return S_OK;
}

_Check_return_ HRESULT
ContentDialog::HideImpl()
{
    if (m_isShowing)
    {
        IFC_RETURN(HideInternal(xaml_controls::ContentDialogResult::ContentDialogResult_None));
    }

    return S_OK;
}

_Check_return_ HRESULT
ContentDialog::HideInternal(_In_ xaml_controls::ContentDialogResult result)
{
    if (m_tpCurrentAsyncOperation && !m_hideInProgress)
    {
        ctl::ComPtr<ContentDialogClosingEventArgs> args;

        IFC_RETURN(EnsureDeferralManagers());

        ULONG deferralGeneration = 0;
        boolean isAlreadyInUse = false;
        IFC_RETURN(m_spClosingDeferralManager->Prepare(&deferralGeneration, &isAlreadyInUse));

        ASSERT(!isAlreadyInUse);

        m_hideInProgress = true;

        bool doFireClosing = false;
        IFC_RETURN(ShouldFireClosing(&doFireClosing));
        if (doFireClosing)
        {
            IFC_RETURN(ctl::make(m_spClosingDeferralManager.Get(), deferralGeneration, &args));

            IFC_RETURN(args->put_Result(result));

            ClosingEventSourceType* pEventSource = nullptr;
            IFC_RETURN(GetClosingEventSourceNoRef(&pEventSource));

            IFC_RETURN(pEventSource->Raise(this, args.Get()));
        }

        ctl::WeakRefPtr wrThis;
        IFC_RETURN(ctl::AsWeak(this, &wrThis));
        IFC_RETURN(m_spClosingDeferralManager->ContinueWith([wrThis, args, result]() mutable
        {
            auto contentDialog = wrThis.AsOrNull<IContentDialog>();
            if (contentDialog)
            {
                BOOLEAN isCanceled = FALSE;
                if (args)
                {
                    IFC_RETURN(args->get_Cancel(&isCanceled));
                }

                IFC_RETURN(contentDialog.Cast<ContentDialog>()->HideAfterDeferralWorker(!!isCanceled, result));
            }
            return S_OK;
        }));
    }

    return S_OK;
}

_Check_return_ HRESULT
ContentDialog::HideAfterDeferralWorker(bool isCanceled, xaml_controls::ContentDialogResult result)
{
    BOOLEAN isPopupOpen = FALSE;
    if (m_placementMode != PlacementMode::InPlace)
    {
        IFC_RETURN(m_tpPopup->get_IsOpen(&isPopupOpen));
    }

    // For the popup hosted scenarios, cancel hiding the dialog only if the popup is still open.  It
    // might not be open if an app manually closed the popup after searching for it in the
    // the visual tree.
    // For the inline scenarios, since there's no popup, always respect the cancel flag.
    if (isCanceled && (isPopupOpen || m_placementMode == PlacementMode::InPlace))
    {
        m_hideInProgress = false;
    }
    else
    {
        // Since we are hiding the dialog, we know the root existed when we showed it.  So if it is not there
        // now, we are shutting down.
        if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(this, false /* createIfNotExist */))
        {
            ctl::ComPtr<ContentDialogMetadata> metadata;
            IFC_RETURN(xamlRoot->GetContentDialogMetadata(&metadata));
#if DBG
            bool isOpen = false;
            IFC_RETURN(metadata->IsOpen(this, &isOpen));
            ASSERT(isOpen);
#endif
            IFC_RETURN(metadata->RemoveOpenDialog(this));
        }


        auto asyncOperationNoRef = static_cast<ContentDialogShowAsyncOperation*>(m_tpCurrentAsyncOperation.Get());
        IFC_RETURN(asyncOperationNoRef->SetResults(result));

        // Try to restore focus back to the original focused element. It is important to
        // do this before the Popup is closed and destroyed, because otherwise, the FocusManager
        // would try to set focus to the first focusable element of the page on Desktop, before
        // getting to this point.
        if (m_spFocusedElementBeforeContentDialogShows)
        {
            auto dependencyObject = m_spFocusedElementBeforeContentDialogShows.AsOrNull<DependencyObject>();
            if (dependencyObject)
            {
                BOOLEAN isFocusSuccessful = FALSE;
                IFC_RETURN(DependencyObject::SetFocusedElement(
                    dependencyObject.Get(),
                    xaml::FocusState_Programmatic,
                    FALSE /*animateIfBringIntoView*/,
                    &isFocusSuccessful));
            }
        }

        if (m_placementMode == PlacementMode::InPlace)
        {
            IFC_RETURN(UpdateVisualState());
        }
        else
        {
            IFC_RETURN(m_popupChildUnloadedEventHandler.AttachEventHandler(
                m_placementMode == PlacementMode::EntireControlInPopup ? this : m_tpLayoutRootPart.AsOrNull<IFrameworkElement>().Get(),
                std::bind(&ContentDialog::OnPopupChildUnloaded, this, _1, _2)));

            // Make these elements non-interactable while the closing transitions plays.
            IFC_RETURN(m_tpSmokeLayer.AsOrNull<IUIElement>()->put_IsHitTestVisible(FALSE));

            if (m_tpLayoutRootPart)
            {
                IFC_RETURN(m_tpLayoutRootPart.Cast<Grid>()->put_IsHitTestVisible(FALSE));
            }

            IFC_RETURN(m_tpPopup->put_IsOpen(FALSE));
        }

        IFC_RETURN(BackButtonIntegration_UnregisterListener(this));

        IFC_RETURN(DirectUI::ElementSoundPlayerService::RequestInteractionSoundForElementStatic(xaml::ElementSoundKind_Hide, this));
    }

    // For the popup hosted scenario, the OnFinishedClosing() method is usually called in the
    // popup's closed event handler, but only if the popup was closed as a result of a hide
    // action.  If the popup was closed via another means (such as the app finding the popup
    // in the tree and closing it manually), then catch that case and call OnFinishedClosing()
    // here.
    if (m_placementMode != PlacementMode::InPlace && !isPopupOpen)
    {
        IFC_RETURN(OnFinishedClosing());
    }

    return S_OK;
}

_Check_return_ HRESULT
ContentDialog::ShowAsyncImpl(_Outptr_ wf::IAsyncOperation<xaml_controls::ContentDialogResult>** returnValue)
{
    return ShowAsyncWithPlacementImpl(xaml_controls::ContentDialogPlacement_Popup, returnValue);
}

_Check_return_ HRESULT
ContentDialog::ShowAsyncWithPlacementImpl(
    xaml_controls::ContentDialogPlacement placement,
    _Outptr_ wf::IAsyncOperation<xaml_controls::ContentDialogResult>** returnValue)
{
    PerfXamlEvent_RAII perfXamlEvent(reinterpret_cast<uint64_t>(this), "ContentDialog::ShowAsync[WithPlacement]", true);

    auto cleanupOnFailure = wil::scope_exit([&]
    {
        m_placementMode = PlacementMode::Undetermined;
        m_tpPopup.Clear();
        m_tpCurrentAsyncOperation.Clear();
    });

    bool ifWasWindowed = m_isWindowed;

    m_isWindowed = placement == xaml_controls::ContentDialogPlacement_UnconstrainedPopup ? TRUE : FALSE;

    if (m_tpPopup && ifWasWindowed != m_isWindowed)
    {
        IFC_RETURN(DiscardPopup());
    }

    // Validate we have a unique, non-ambiguous visualtree on which to place the ContentDialog
    {
        VisualTree* uniqueVisualTreeUnused = nullptr;
        IFC_RETURN(VisualTree::GetUniqueVisualTreeNoRef(GetHandle(), nullptr /*positionReferenceElement*/, nullptr, &uniqueVisualTreeUnused));
    }

    // reset previous focused element
    m_spFocusedElementBeforeContentDialogShows = nullptr;

    // If the control is in the visual tree then we transplant the LayoutRoot, otherwise
    // we place the entire control in the popup.
    if (m_placementMode == PlacementMode::Undetermined)
    {
        m_placementMode = PlacementMode::EntireControlInPopup;

        if (IsInLiveTree())
        {
            // Ensure the template is applied so that we can get to the parts.
            BOOLEAN ignore = FALSE;
            IFC_RETURN(ApplyTemplate(&ignore));

            m_placementMode = placement == xaml_controls::ContentDialogPlacement_InPlace ? PlacementMode::InPlace : PlacementMode::TransplantedRootInPopup;
        
            // In order to actually move to a popup we need the container and layout root parts.  There is no current
            // check for this (using version) and we will crash if you attempt to host a content dialog in a popup with
            // one of these parts missing.  This continues that behavior, but we will fail here instead of at some random
            // point in the future when we attempt to use one of those parts.  
            //
            // Alternately, we could forcibly switch to inplace mode if one of the parts are missing or we could try to 
            // to use the dialogs content as the LayoutRoot and the Dialog itself as the Container.
            IFCCHECK_RETURN(m_placementMode == PlacementMode::InPlace || (m_tpContainerPart && m_tpLayoutRootPart));
        }
    }

    ctl::ComPtr<ContentDialogMetadata> metadata;

    if (const auto xamlRoot = XamlRoot::GetForElementStatic(this).AsOrNull<XamlRoot>())
    {
        IFC_RETURN(xamlRoot->GetContentDialogMetadata(&metadata));
    }
    else
    {
        IFC_RETURN(DirectUI::ErrorHelper::OriginateErrorUsingResourceID(E_UNEXPECTED, ERROR_POPUP_XAMLROOT_NOT_SET));
    }

    // See if there is already an open dialog and return an error if that is the case.
    // For InPlace dialogs, multiple can be shown at the same time, provided that they
    // are each under different parents.
    {
        ctl::ComPtr<xaml::IDependencyObject> parent;
        if (m_placementMode == PlacementMode::InPlace)
        {
            IFC_RETURN(get_Parent(&parent));

            // A dialog can only be shown InPlace if it is in the live tree, so it should have a parent.
            ASSERT(parent);
        }

        bool hasOpenDialog = false;
        IFC_RETURN(metadata->HasOpenDialog(parent.Get(), &hasOpenDialog));

        if (m_tpCurrentAsyncOperation || hasOpenDialog)
        {
            IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_ASYNC_OPERATION_NOT_STARTED, ERROR_CONTENTDIALOG_MULTIPLE_OPEN));
        }
    }

    Microsoft::WRL::ComPtr<ContentDialogShowAsyncOperation> newAsyncOperation;
    IFC_RETURN(Microsoft::WRL::MakeAndInitialize<ContentDialogShowAsyncOperation>(
        &newAsyncOperation,
        ::InterlockedIncrement(&ContentDialog::z_ulUniqueAsyncActionId),
        nullptr));

    IFC_RETURN(newAsyncOperation->StartOperation(this));

    // Set the default result for light-dismiss triggered scenarios.
    IFC_RETURN(newAsyncOperation->SetResults(xaml_controls::ContentDialogResult_None));
    SetPtrValue(m_tpCurrentAsyncOperation, newAsyncOperation.Get());

    m_isShowing = true;
    m_hideInProgress = false;
    m_skipClosingEventOnHide = false;
    m_hasPreparedContent = false;

    if (m_placementMode == PlacementMode::InPlace)
    {
        // If the dialog had previously been open, then this should have been cleared after
        // it finished closing.
        ASSERT(!m_dialogShowingStateChangedEventHandler);

        // This is done here rather than in OnApplyTemplate to avoid some startup perf impact because
        // querying for this part will fault the VSM.
        ctl::ComPtr<xaml::IVisualStateGroup> dialogShowingStates;
        IFC_RETURN(GetTemplatePart<xaml::IVisualStateGroup>(STR_LEN_PAIR(L"DialogShowingStates"), dialogShowingStates.ReleaseAndGetAddressOf()));
        if (dialogShowingStates)
        {
            IFC_RETURN(m_dialogShowingStateChangedEventHandler.AttachEventHandler(dialogShowingStates.Get(), std::bind(&ContentDialog::OnDialogShowingStateChanged, this, _1, _2)));
            SetPtrValue(m_tpDialogShowingStates, dialogShowingStates);
        }

        IFC_RETURN(PrepareContent());
    }
    else
    {
        // This will be set to false once the popup has opened.  If an app calls
        // hide before the popup has opened, we don't fire the closing event.
        m_skipClosingEventOnHide = true;

        // For dialogs that haven't been loaded yet (ones that are being opened in the popup
        // in their entirety), wait until they are loaded before preparing their content.
        // Template version is determined in OnApplyTemplate, so we can test against that
        // to see whether the control has been loaded before ShowAsync() was called.
        if (m_isTemplateApplied)
        {
            IFC_RETURN(PrepareContent());
        }

        IFC_RETURN(HostDialogWithinPopup(false /*wasSmokeLayerFoundAsTemplatePart*/));

        // Defer actually opening the ContentDialog's popup by a tick to enable
        // transitions to play.  They don't play if they target an element that
        // has been removed from and re-inserted back into the tree in the same tick,
        // which is the case for the LayoutRoot part when m_placementMode == TransplantedRootInPopup
        // or for the whole control when m_placementMode == EntireControlInPopup.
        IFC_RETURN(DXamlCore::GetCurrent()->GetXamlDispatcherNoRef()->RunAsync(
            MakeCallback(ctl::ComPtr<ContentDialog>(this), &ContentDialog::DeferredOpenPopup)));
    }

    if (DXamlCore::GetCurrent()->GetHandle()->BackButtonSupported())
    {
        IFC_RETURN(BackButtonIntegration_RegisterListener(this));
    }

    IFC_RETURN(DirectUI::ElementSoundPlayerService::RequestInteractionSoundForElementStatic(xaml::ElementSoundKind_Show, this));

    IFC_RETURN(metadata->AddOpenDialog(this));

    IFC_RETURN(newAsyncOperation.CopyTo(returnValue));

    // Everything completed successfully, so no need to let our scope-exit cleanup object execute.
    cleanupOnFailure.release();

    return S_OK;
}

_Check_return_ HRESULT ContentDialog::OnCommandButtonClicked(
    CommandClickEventSourceType* const clickEventSource,
    const ctl::ComPtr<xaml_input::ICommand>& command,
    const ctl::ComPtr<IInspectable>& commandParameter,
    xaml_controls::ContentDialogResult result
    )
{
    ASSERT(m_tpCurrentAsyncOperation);

    if (!m_hideInProgress)
    {
        IFC_RETURN(EnsureDeferralManagers());

        ULONG deferralGeneration = 0;
        BOOLEAN isAlreadyInUse = FALSE;
        IFC_RETURN(m_spButtonClickDeferralManager->Prepare(&deferralGeneration, &isAlreadyInUse));
        if (isAlreadyInUse)
        {
            return S_OK;
        }

        ctl::ComPtr<ContentDialogButtonClickEventArgs> args;
        IFC_RETURN(ctl::make(m_spButtonClickDeferralManager.Get(), deferralGeneration, &args));

        IFC_RETURN(clickEventSource->Raise(this, args.Get()));

        ctl::WeakRefPtr wrThis;
        IFC_RETURN(ctl::AsWeak(this, &wrThis));
        IFC_RETURN(m_spButtonClickDeferralManager->ContinueWith([wrThis, args, command, commandParameter, result]() mutable
        {
            auto spThis = wrThis.AsOrNull<IContentDialog>();
            if (spThis)
            {
                BOOLEAN isCanceled = FALSE;
                IFC_RETURN(args->get_Cancel(&isCanceled));
                if (!isCanceled)
                {
                    if (command)
                    {
                        // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
                        IFC_RETURN(command->Execute(commandParameter.Get()));
                    }

                    IFC_RETURN(spThis.Cast<ContentDialog>()->HideInternal(result));
                }
            }
            return S_OK;
        }));
    }

    return S_OK;
}

_Check_return_ HRESULT ContentDialog::EnsureDeferralManagers()
{
    HRESULT hr = S_OK;

    if (!m_spClosingDeferralManager)
    {
        ctl::ComPtr<DeferralManager<ContentDialogClosingDeferral>> spClosingDeferralManager;
        IFC(ctl::make(&spClosingDeferralManager));
        IFC(spClosingDeferralManager.MoveTo(m_spClosingDeferralManager.GetAddressOf()));
    }

    if (!m_spButtonClickDeferralManager)
    {
        ctl::ComPtr<DeferralManager<ContentDialogButtonClickDeferral>> spButtonClickDeferralManager;
        IFC(ctl::make(&spButtonClickDeferralManager));
        IFC(spButtonClickDeferralManager.MoveTo(m_spButtonClickDeferralManager.GetAddressOf()));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ContentDialog::ResetAndPrepareContent()
{
    if (m_isTemplateApplied && m_tpCurrentAsyncOperation && m_hasPreparedContent)
    {
        m_hasPreparedContent = false;
        IFC_RETURN(PrepareContent());
    }

    return S_OK;
}

// wasSmokeLayerFoundAsTemplatePart is set to True when this method is called after m_tpSmokeLayer was found as a template part and m_isSmokeLayerATemplatePart was set to True.
_Check_return_ HRESULT
ContentDialog::HostDialogWithinPopup(bool wasSmokeLayerFoundAsTemplatePart)
{
    ASSERT(m_placementMode == PlacementMode::TransplantedRootInPopup || m_placementMode == PlacementMode::EntireControlInPopup);

    IFC_RETURN(PrepareSmokeLayer());
    IFC_RETURN(PreparePopup(wasSmokeLayerFoundAsTemplatePart));
    IFC_RETURN(m_tpPopup->put_ShouldConstrainToRootBounds(!m_isWindowed));

    if (!wasSmokeLayerFoundAsTemplatePart)
    {
        // Make sure the automation name is in-sync with the title.
        IFC_RETURN(SetPopupAutomationProperties());

        if (m_placementMode == PlacementMode::TransplantedRootInPopup)
        {
            if (!m_isLayoutRootTransplanted)
            {
                ctl::ComPtr<IUIElement> spChildElt;

                // We verify some parts of the template here and simply
                // no-op if it looks too odd. (if child isn't the layoutroot)
                IFC_RETURN(m_tpContainerPart->get_Child(&spChildElt));
                if (spChildElt.AsOrNull<IGrid>().Get() == m_tpLayoutRootPart.Get())
                {
                    IFC_RETURN(m_tpContainerPart->put_Child(nullptr));

                    // We place our popup in the children collection so it can receive
                    // all the text property inheritance goodness from ContentControl.
                    IFC_RETURN(m_tpContainerPart->put_Child(m_tpPopup.AsOrNull<IUIElement>().Get()));
                }

                // Even if we have an incorrect template set this value
                // to preserve the logical tests other parts of ContentDialog
                // perform.
                m_isLayoutRootTransplanted = true;
            }
        }
    }

    IFC_RETURN(m_tpPopup->put_Child(m_placementMode == PlacementMode::EntireControlInPopup ? this : m_tpLayoutRootPart.AsOrNull<IUIElement>().Get()));

    return S_OK;
}

_Check_return_ HRESULT
ContentDialog::PreparePopup(bool wasSmokeLayerFoundAsTemplatePart)
{
    if (!m_tpPopup || wasSmokeLayerFoundAsTemplatePart)
    {
        ctl::ComPtr<Popup> popup;

        if (wasSmokeLayerFoundAsTemplatePart)
        {
            popup = m_tpPopup.Cast<DirectUI::Popup>();
        }
        else
        {
            IFC_RETURN(ctl::make(&popup));

            IFC_RETURN(popup->put_IsContentDialog(TRUE));
            IFC_RETURN(m_popupOpenedHandler.AttachEventHandler(popup.Get(), std::bind(&ContentDialog::OnPopupOpened, this, _1, _2)));
        }

        ASSERT(m_tpSmokeLayer);

        // Setup the open / close transitions
        ctl::ComPtr<TransitionCollection> transitionCollection;
        IFC_RETURN(ctl::make(&transitionCollection));
        IFCOOM_RETURN(transitionCollection.Get());

        ctl::ComPtr<ContentDialogOpenCloseThemeTransition> contentDialogOpenCloseTransition;
        IFC_RETURN(ctl::make(&contentDialogOpenCloseTransition));

        IFC_RETURN(contentDialogOpenCloseTransition->SetSmokeLayer(m_tpSmokeLayer.AsOrNull<IUIElement>().Get()));

        ctl::ComPtr<ITransition> openCloseTransition;
        IFC_RETURN(contentDialogOpenCloseTransition.As(&openCloseTransition));
        IFC_RETURN(transitionCollection->Append(openCloseTransition.Get()));

        IFC_RETURN(popup->put_ChildTransitions(transitionCollection.Get()));

        if (!wasSmokeLayerFoundAsTemplatePart)
        {
            // Set IsDialog property to True for popup
            IFC_RETURN(DirectUI::AutomationProperties::SetIsDialogStatic(popup.Cast<UIElement>(), TRUE));

            IFC_RETURN(SetPtrValueWithQI(m_tpPopup, popup.Get()));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ContentDialog::PrepareSmokeLayer()
{
    // Prepare a second popup to show a semi-transparent
    // smoke layer behind the dialog.
    if (!m_tpSmokeLayerPopup)
    {
        ctl::ComPtr<Popup> popupForOverlay;
        IFC_RETURN(ctl::make(&popupForOverlay));
        IFC_RETURN(SetPtrValueWithQI(m_tpSmokeLayerPopup, popupForOverlay.Get()));
    }

    if (!m_tpSmokeLayer || m_isSmokeLayerATemplatePart)
    {
        if (!m_tpSmokeLayer)
        {
            ctl::ComPtr<Rectangle> overlay;
            IFC_RETURN(ctl::make(&overlay));
            IFC_RETURN(SetPtrValueWithQI(m_tpSmokeLayer, overlay.Get()));
        }

        wf::Rect windowBounds{};
        IFC_RETURN(DXamlCore::GetCurrent()->GetContentBoundsForElement(GetHandle(), &windowBounds));

        IFC_RETURN(m_tpSmokeLayer.Cast<FrameworkElement>()->put_Width(windowBounds.Width));
        IFC_RETURN(m_tpSmokeLayer.Cast<FrameworkElement>()->put_Height(windowBounds.Height));

        IFC_RETURN(m_tpSmokeLayerPopup->put_Child(m_tpSmokeLayer.AsOrNull<IUIElement>().Get()));

    }

    if (!m_isSmokeLayerATemplatePart)
    {
        ctl::ComPtr<IBrush> brush;
        IFC_RETURN(CreateSmokeLayerBrush(&brush));
        IFC_RETURN(m_tpSmokeLayer.Cast<Rectangle>()->put_Fill(brush.Get()));
    }

    return S_OK;
}

// only in Desktop Window app when custom titlebar is being used
// disable custom titlebar temporarily when smoke layer is displayed
_Check_return_ HRESULT
ContentDialog::UpdateCanDragStatusWindowChrome(bool dragEnabled)
{
    // check if window chrome exists or not
    // if window chrome exists then this app is running in island or desktop window mode
    auto xamlRoot = XamlRoot::GetForElementStatic(this);
    if (xamlRoot)
    {
        CUIElement* publicRootVisual = xamlRoot.Cast<XamlRoot>()->GetVisualTreeNoRef()->GetPublicRootVisual();
        ctl::ComPtr<DependencyObject> peer;
        IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(publicRootVisual, &peer));
        auto windowChrome = peer.AsOrNull<WindowChrome>();

        if (windowChrome && windowChrome->IsChromeActive())
        {
            windowChrome->UpdateCanDragStatus(dragEnabled);
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
ContentDialog::CreateSmokeLayerBrush(_Outptr_ xaml_media::IBrush** brush)
{
    ASSERT(!m_isSmokeLayerATemplatePart);

    *brush = nullptr;

    ctl::ComPtr<xaml::IResourceDictionary> resourceDictionary;
    IFC_RETURN(get_Resources(&resourceDictionary));

    ctl::ComPtr<wfc::IMap<IInspectable*, IInspectable*>> resourceMap;
    IFC_RETURN(resourceDictionary.As(&resourceMap));

    // Querying this from code-behind does not allow it to change with theme changes (light/dark/HC).
    // We also can't just refresh the brush whenever it opens because resource lookups from
    // code-behind don't take into account the current theme of the control; they instead use
    // the theme dictionary that corresponds to the app's theme.
    // The correct solution for this is to specify the overlay as a template part and use
    // a {ThemeResource} markup extension. That solution has been implemented in a more recent release.
    // See the use of m_isSmokeLayerATemplatePart for details.
    wrl_wrappers::HStringReference themeBrushName(L"SystemControlPageBackgroundMediumAltMediumBrush");

    ctl::ComPtr<IInspectable> resourceKey;
    IFC_RETURN(PropertyValue::CreateFromString(themeBrushName.Get(), &resourceKey));

    BOOLEAN hasKey = FALSE;
    IFC_RETURN(resourceMap->HasKey(resourceKey.Get(), &hasKey));

    if (hasKey)
    {
        ctl::ComPtr<IInspectable> resource;
        IFC_RETURN(resourceMap->Lookup(resourceKey.Get(), &resource));
        IFC_RETURN(resource.CopyTo(brush));
    }

    return S_OK;
}

_Check_return_ HRESULT ContentDialog::DeferredOpenPopup()
{
    // Since opening the popups is deferred by a tick to allow layout transitions to play
    // correctly, we could hit a situation where a dialog is shown and then immediately
    // hidden within the same tick.  In this scenario, we don't want to open the popups
    // after the dialog has been hidden so make sure the dialog is still showing before
    // actually opening them.
    if (m_isShowing)
    {
        VisualTree* tree = nullptr;
        IFC_RETURN(VisualTree::GetUniqueVisualTreeNoRef(GetHandle(), nullptr /*positionReferenceElement*/, nullptr, &tree));

        if (m_tpSmokeLayerPopup)
        {
            static_cast<CPopup*>(m_tpSmokeLayerPopup.Cast<DirectUI::Popup>()->GetHandle())->SetAssociatedVisualTree(tree);
            IFC_RETURN(m_tpSmokeLayerPopup->put_IsOpen(TRUE));
            UpdateCanDragStatusWindowChrome(false); //disable dragging in custom titlebar
        }

        static_cast<CPopup*>(m_tpPopup.Cast<DirectUI::Popup>()->GetHandle())->SetAssociatedVisualTree(tree);
        IFC_RETURN(m_tpPopup->put_IsOpen(TRUE));
    }

    return S_OK;
}

_Check_return_ HRESULT
ContentDialog::PrepareContent()
{
    if (!m_hasPreparedContent)
    {
        if (m_placementMode == PlacementMode::TransplantedRootInPopup)
        {
            // We set the width/height of the container to 0 to ensure that the
            // inverse transform we apply to Popup behaves correctly when in RTL mode,
            // otherwise it would transform from the top right visual point and incorrectly
            // position the popup.
            IFC_RETURN(m_tpContainerPart.AsOrNull<IFrameworkElement>()->put_Width(0.0));
            IFC_RETURN(m_tpContainerPart.AsOrNull<IFrameworkElement>()->put_Height(0.0));
        }
        else if (m_placementMode == PlacementMode::InPlace && m_isLayoutRootTransplanted)
        {
            // If a dialog that had previously been shown in a popup is now being shown in place,
            // restore the layout root that had been transplanted out of the dialog's tree.
            IFC_RETURN(m_tpContainerPart->put_Child(m_tpLayoutRootPart.AsOrNull<IUIElement>().Get()));
            IFC_RETURN(m_tpContainerPart.AsOrNull<IFrameworkElement>()->put_Width(DoubleUtil::NaN));
            IFC_RETURN(m_tpContainerPart.AsOrNull<IFrameworkElement>()->put_Height(DoubleUtil::NaN));

            m_isLayoutRootTransplanted = false;
        }

        IFC_RETURN(UpdateVisualState());
        IFC_RETURN(UpdateTitleSpaceVisibility());

        if (m_placementMode != PlacementMode::InPlace)
        {
            IFC_RETURN(SizeAndPositionContentInPopup());
        }

        // Cast a shadow if we have a background part
        if (m_tpBackgroundElementPart)
        {
            if (CThemeShadow::IsDropShadowMode())
            {
                // Under drop shadows, ContentDialog has a larger shadow than normal
                IFC_RETURN(ApplyElevationEffect(m_tpBackgroundElementPart.AsOrNull<IUIElement>().Get(), 0 /* depth */, 128 /* baseElevation */));
            }
            else
            {
                IFC_RETURN(ApplyElevationEffect(m_tpBackgroundElementPart.AsOrNull<IUIElement>().Get()));
            }
        }

        m_hasPreparedContent = true;
    }

    return S_OK;
}

_Check_return_ HRESULT
ContentDialog::SizeAndPositionContentInPopup()
{
    if (!m_tpPopup)
    {
        return S_OK;
    }

    // Eventually, I think we want to center the popup, rather than make the popup full window size and then
    // center the content.  For now, to handle invalid templates, we just won't position if we don't have
    // the right parts.
    if (!m_tpBackgroundElementPart || !m_tpLayoutRootPart)
    {
        return S_OK;
    }

    DOUBLE xOffset = 0;
    DOUBLE yOffset = 0;

    wf::Rect windowBounds = {};
    IFC_RETURN(DXamlCore::GetCurrent()->GetContentBoundsForElement(GetHandle(), &windowBounds));

    // Get the layout bounds to calculate the ContentDialog position and size
    wf::Rect adjustedLayoutBounds = {};
    IFC_RETURN(DXamlCore::GetCurrent()->GetContentLayoutBoundsForElement(GetHandle(), &adjustedLayoutBounds));

    // If smoke layer has a Y offset (due to being shifted down for making space for window chrome caption buttons)
    // add that to calculations in centering the popup
    xaml::Thickness margin {};
    IFC_RETURN(m_tpSmokeLayer.Cast<Rectangle>()->get_Margin(&margin));
    DOUBLE smokeLayerYOffset = margin.Top;

    double dialogMaxHeight = 0;
    IFC_RETURN(get_MaxHeight(&dialogMaxHeight));

    double dialogMaxWidth = 0;
    IFC_RETURN(get_MaxWidth(&dialogMaxWidth));

    auto flowDirection = xaml::FlowDirection_LeftToRight;
    IFC_RETURN(get_FlowDirection(&flowDirection));

    BOOLEAN fullSizeDesired = FALSE;
    IFC_RETURN(get_FullSizeDesired(&fullSizeDesired));

    ctl::ComPtr<IFrameworkElement> spBackgroundAsFE;
    IFC_RETURN(m_tpBackgroundElementPart.As(&spBackgroundAsFE));

    double popupWidth = 0;
    IFC_RETURN(spBackgroundAsFE->get_ActualWidth(&popupWidth));

    double popupHeight = 0;
    IFC_RETURN(spBackgroundAsFE->get_ActualHeight(&popupHeight));

    // Here we shamelessly contour ourselves to the Layout system and the whims of
    // the implementers of Popup. The following truths must be known to understand
    // this code:
    // - ContentDialog has two modes, LayouRoot and EntireControl. When it is in the visual
    //   tree it cannot add itself to the popup, it's already the child of another UI element.
    //   As a result we transplant a specific part of the control template (LayoutRoot) to the
    //   popup.
    //   When it is not in the visual tree there's no way to apply the control template. We add
    //   the entire control to the visual tree in this scenario.
    // - Popup is added to ContentDialog's children when ContentDialog is in LayoutRoot mode to
    //   allow inherited properties to work.
    // - When Popup has no parent it positions its contents in the top left of the screen. When it
    //   has a parent it positions its contents at the top left of its parent. As a result when we are
    //   in LayoutRoot mode we must apply an inverse transform of the top left of the Popup's parent's
    //   position.
    // - When a Popup has no parent it does not flip in place for RTL mode. As a result we have to apply
    //   a horizontal offset equal to the width of the popup (in our case the screen width) to the Popup
    //   when in EntireControl mode.
    // - When a Popup has a parent it determines it position using the top left corner even in RTL mode.
    //   as a result we make sure the container is sized to zero pixels.
    // - When a parented Popup is in RTL mode its position is determined by its right edge and not the left edge.
    // - The Arrange pass is busted for RTL mode in Popup so don't expect any autosizing
    //   to work correctly. We force feed Popup its Width and Height instead of using Stretch.
    if (m_placementMode == PlacementMode::EntireControlInPopup)
    {
        IFC_RETURN(put_Height(windowBounds.Height));
        IFC_RETURN(put_Width(windowBounds.Width));
    }
    else if (m_tpLayoutRootPart)
    {
        IFC_RETURN(m_tpLayoutRootPart.AsOrNull<IFrameworkElement>()->put_Height(windowBounds.Height - static_cast<float>(smokeLayerYOffset)));
        IFC_RETURN(m_tpLayoutRootPart.AsOrNull<IFrameworkElement>()->put_Width(windowBounds.Width));
    }

    xOffset = (flowDirection == xaml::FlowDirection_LeftToRight ? 0 : adjustedLayoutBounds.Width);

    // When the ContentDialog is in the visual tree, the popup offset has added
    // to it the top-left point of where layout measured and arranged it to.
    // Since we want ContentDialog to be an overlay, we need to subtract off that
    // point in order to ensure the ContentDialog is always being displayed in
    // window coordinates instead of local coordinates.
    if (m_placementMode == PlacementMode::TransplantedRootInPopup)
    {
        wf::Point offsetFromRoot = { 0, 0 };
        ctl::ComPtr<IGeneralTransform> transformToRoot;

        IFC_RETURN(m_tpPopup.Cast<Popup>()->TransformToVisual(nullptr, &transformToRoot));
        IFC_RETURN(transformToRoot->TransformPoint({ 0, 0 }, &offsetFromRoot));

        xOffset =
            flowDirection == xaml::FlowDirection_LeftToRight ?
            (xOffset - offsetFromRoot.X) :
            (xOffset - offsetFromRoot.X) * -1;

        yOffset = yOffset - offsetFromRoot.Y + smokeLayerYOffset;
    }
 
    // Set the ContentDialog left and top position.
    IFC_RETURN(m_tpPopup->put_HorizontalOffset(xOffset));
    IFC_RETURN(m_tpPopup->put_VerticalOffset(yOffset));

    return S_OK;
}

_Check_return_ HRESULT
ContentDialog::GetFocusedElementPosition(
_Out_ wf::Point* focusedPosition)
{
    *focusedPosition = {};

    ctl::ComPtr<IUIElement> focusedElement;
    auto focusedElementAsDO = m_spFocusedElementBeforeContentDialogShows.AsOrNull<IDependencyObject>();

    if (focusedElementAsDO)
    {
        focusedElement = focusedElementAsDO.AsOrNull<IUIElement>();

        if (!focusedElement)
        {
            CDependencyObject* focusedDO = focusedElementAsDO.Cast<DependencyObject>()->GetHandle();
            CTextElement* focusedTextElement = do_pointer_cast<CTextElement>(focusedDO);

            if (focusedTextElement)
            {
                CFrameworkElement* containingFE = focusedTextElement->GetContainingFrameworkElement();

                if (containingFE)
                {
                    ctl::ComPtr<DependencyObject> containingDO;

                    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(containingFE, &containingDO));
                    IFC_RETURN(containingDO.As<IUIElement>(&focusedElement));
                }
            }
        }
    }

    if (focusedElement)
    {
        ctl::ComPtr<IGeneralTransform> transform;

        IFC_RETURN(focusedElement.Get()->TransformToVisual(nullptr, &transform));
        IFC_RETURN(transform->TransformPoint({ 0.f, 0.f }, focusedPosition));
    }

    return S_OK;
}

_Check_return_ HRESULT
ContentDialog::OnPopupOpened(
_In_ IInspectable* /*pSender*/,
_In_ IInspectable* /*pArgs*/)
{
    ctl::ComPtr<IAsyncInfo> spAsyncInfo;
    wf::AsyncStatus status = wf::AsyncStatus::Started;

    IFC_RETURN(m_tpCurrentAsyncOperation.As(&spAsyncInfo));
    IFC_RETURN(spAsyncInfo->get_Status(&status));
    if (status != wf::AsyncStatus::Canceled)
    {
        IFC_RETURN(UpdateVisualState());

        // Now that the popup is opened, allow an app to cancel the hiding the dialog.
        m_skipClosingEventOnHide = false;

        ctl::ComPtr<ContentDialogOpenedEventArgs> spArgs;
        IFC_RETURN(ctl::make(&spArgs));

        OpenedEventSourceType* pEventSource = nullptr;
        IFC_RETURN(GetOpenedEventSourceNoRef(&pEventSource));

        IFC_RETURN(pEventSource->Raise(this, spArgs.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT
ContentDialog::OnPopupChildUnloaded(
    _In_ IInspectable* /*sender*/,
    _In_ xaml::IRoutedEventArgs* /*args*/)
{
    ASSERT(m_placementMode == PlacementMode::EntireControlInPopup || m_placementMode == PlacementMode::TransplantedRootInPopup);

    if (!m_tpCurrentAsyncOperation)
    {
        // If m_tpCurrentAsyncOperation is null, then that means that we've already handled the popup closing.
        // This can happen in the circumstance where we close the popup in the handler for a button click -
        // for example, if we remove the ContentDialog from the visual tree in the handler by navigating
        // to another page, this will occur.
        return S_OK;
    }

    if (m_hideInProgress)
    {
        m_hideInProgress = false;
        IFC_RETURN(OnFinishedClosing());
    }
    else
    {
        // If the app directly closed the popup, go through the closing actions for
        // ContentDialog.
        // This isn't great, as using the Cancel flag on the Closing event args will
        // have no effect.
        m_skipClosingEventOnHide = true;
        IFC_RETURN(HideInternal(xaml_controls::ContentDialogResult_None));
    }

    return S_OK;
}

_Check_return_ HRESULT
ContentDialog::OnBackButtonPressedImpl(_Out_ BOOLEAN* handled)
{
    IFC_RETURN(ExecuteCloseAction());

    *handled = TRUE;

    return S_OK;
}

_Check_return_ HRESULT ContentDialog::OnFinishedClosing()
{
    auto asyncOperationNoRef = m_tpCurrentAsyncOperation.Cast<ContentDialogShowAsyncOperation>();

    m_isShowing = false;

    IFC_RETURN(UpdateVisualState());

    if (m_placementMode != PlacementMode::InPlace)
    {
        // No longer need the handler so detach it.
        IFC_RETURN(m_popupChildUnloadedEventHandler.DetachEventHandler(m_placementMode == PlacementMode::EntireControlInPopup ? this : m_tpLayoutRootPart.AsOrNull<IFrameworkElement>().Get()));

        IFC_RETURN(m_tpSmokeLayerPopup->put_IsOpen(FALSE));
        UpdateCanDragStatusWindowChrome(true); //re-enable dragging in custom titlebar

        // Break circular reference with ContentDialog.
        IFC_RETURN(m_tpPopup->put_Child(nullptr));
    }

    auto result = xaml_controls::ContentDialogResult_None;
    IFC_RETURN(asyncOperationNoRef->GetResults(&result));

    IFC_RETURN(RaiseClosedEvent(result));

    // We use a new deferral manager each time the ContentDialog is shown because the
    // dialog may be forcibly closed and then reshown while a button click deferral is pending.
    // In that case if the deferral is ever completed it no longer has anything to do, and until
    // it is completed its manager will not be able to start any new deferrals.
    m_spButtonClickDeferralManager->Disconnect();
    m_spButtonClickDeferralManager.Reset();

    if (m_dialogShowingStateChangedEventHandler)
    {
        ASSERT(m_placementMode == PlacementMode::InPlace);
        IFC_RETURN(m_dialogShowingStateChangedEventHandler.DetachEventHandler(m_tpDialogShowingStates.Get()));
        m_tpDialogShowingStates.Clear();
    }

    if (m_placementMode != PlacementMode::InPlace)
    {
        // Now that we've finished closing, make these interactable again.  They were temporarily
        // disabled while the closing transition played.
        IFC_RETURN(m_tpSmokeLayer.AsOrNull<IUIElement>()->put_IsHitTestVisible(TRUE));

        if (m_tpLayoutRootPart)
        {
            IFC_RETURN(m_tpLayoutRootPart.Cast<Grid>()->put_IsHitTestVisible(TRUE));
        }
    }

    // Reset this so that it can be re-evaluated the next time the dialog is opened.
    m_placementMode = PlacementMode::Undetermined;

    // We clear the tracker pointer here to allow for callers
    // to perform an additional ShowAsync from the completion
    // handler.
    ctl::ComPtr<wf::IAsyncOperation<xaml_controls::ContentDialogResult>> asyncOperationRef;
    IFC_RETURN(m_tpCurrentAsyncOperation.CopyTo(asyncOperationRef.ReleaseAndGetAddressOf()));

    m_tpCurrentAsyncOperation.Clear();
    asyncOperationNoRef->CoreFireCompletion();

    return S_OK;
}

_Check_return_ HRESULT ContentDialog::AttachButtonEvents()
{
    if (m_tpPrimaryButtonPart && !m_epPrimaryButtonClickHandler)
    {
        IFC_RETURN(m_epPrimaryButtonClickHandler.AttachEventHandler(m_tpPrimaryButtonPart.Get(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
            {
                CommandClickEventSourceType* eventSource = nullptr;
                IFC_RETURN(GetPrimaryButtonClickEventSourceNoRef(&eventSource));

                ctl::ComPtr<xaml_input::ICommand> command;
                IFC_RETURN(get_PrimaryButtonCommand(&command));

                ctl::ComPtr<IInspectable> commandParameter;
                IFC_RETURN(get_PrimaryButtonCommandParameter(&commandParameter));

                IFC_RETURN(OnCommandButtonClicked(eventSource, command, commandParameter, xaml_controls::ContentDialogResult::ContentDialogResult_Primary));

                return S_OK;
            }));
    }

    if (m_tpSecondaryButtonPart && !m_epSecondaryButtonClickHandler)
    {
        IFC_RETURN(m_epSecondaryButtonClickHandler.AttachEventHandler(m_tpSecondaryButtonPart.Get(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
            {
                CommandClickEventSourceType* eventSource = nullptr;
                IFC_RETURN(GetSecondaryButtonClickEventSourceNoRef(&eventSource));

                ctl::ComPtr<xaml_input::ICommand> command;
                IFC_RETURN(get_SecondaryButtonCommand(&command));

                ctl::ComPtr<IInspectable> commandParameter;
                IFC_RETURN(get_SecondaryButtonCommandParameter(&commandParameter));

                IFC_RETURN(OnCommandButtonClicked(eventSource, command, commandParameter, xaml_controls::ContentDialogResult::ContentDialogResult_Secondary));

                return S_OK;
            }));
    }

    if (m_tpCloseButtonPart && !m_epCloseButtonClickHandler)
    {
        IFC_RETURN(m_epCloseButtonClickHandler.AttachEventHandler(m_tpCloseButtonPart.Get(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
            {
                CommandClickEventSourceType* eventSource = nullptr;
                IFC_RETURN(GetCloseButtonClickEventSourceNoRef(&eventSource));

                ctl::ComPtr<xaml_input::ICommand> command;
                IFC_RETURN(get_CloseButtonCommand(&command));

                ctl::ComPtr<IInspectable> commandParameter;
                IFC_RETURN(get_CloseButtonCommandParameter(&commandParameter));

                IFC_RETURN(OnCommandButtonClicked(eventSource, command, commandParameter, xaml_controls::ContentDialogResult::ContentDialogResult_None));

                return S_OK;
            }));
    }

    return S_OK;
}

_Check_return_ HRESULT ContentDialog::GetButtonHelper(xaml_controls::ContentDialogButton buttonType, _Outptr_ xaml_primitives::IButtonBase** button)
{
    *button = nullptr;

    switch (buttonType)
    {
    case xaml_controls::ContentDialogButton_Primary:
        IFC_RETURN(m_tpPrimaryButtonPart.CopyTo(button));
        break;

    case xaml_controls::ContentDialogButton_Secondary:
        IFC_RETURN(m_tpSecondaryButtonPart.CopyTo(button));
        break;

    case xaml_controls::ContentDialogButton_Close:
        IFC_RETURN(m_tpCloseButtonPart.CopyTo(button));
        break;
    }

    return S_OK;
}

_Check_return_ HRESULT ContentDialog::GetDefaultButtonHelper(_Outptr_ xaml_primitives::IButtonBase** button)
{
    *button = nullptr;

    auto defaultButton = xaml_controls::ContentDialogButton_None;
    IFC_RETURN(get_DefaultButton(&defaultButton));

    IFC_RETURN(GetButtonHelper(defaultButton, button));

    return S_OK;
}

_Check_return_ HRESULT
ContentDialog::DetachButtonEvents()
{
    // By assigning our EventPtrs to empty ones, we're effectively clearing them since
    // the next time the old ones get invoked, they'll unregister themselves.
    if (m_epPrimaryButtonClickHandler)
    {
        m_epPrimaryButtonClickHandler = ctl::EventPtr<ButtonBaseClickEventCallback>();
    }

    if (m_epSecondaryButtonClickHandler)
    {
        m_epSecondaryButtonClickHandler = ctl::EventPtr<ButtonBaseClickEventCallback>();
    }

    if (m_epCloseButtonClickHandler)
    {
        m_epCloseButtonClickHandler = ctl::EventPtr<ButtonBaseClickEventCallback>();
    }

    return S_OK;
}

_Check_return_ HRESULT
ContentDialog::UpdateTitleSpaceVisibility()
{
    HRESULT hr = S_OK;

    if (m_tpTitlePart)
    {
        ctl::ComPtr<IInspectable> spTitleAsInspectable;
        ctl::ComPtr<xaml::IDataTemplate> spTitleTemplate;

        IFC(get_Title(&spTitleAsInspectable));
        IFC(get_TitleTemplate(&spTitleTemplate));

        xaml::Visibility visibility = (spTitleAsInspectable || spTitleTemplate) ?
            xaml::Visibility_Visible : xaml::Visibility_Collapsed;

        IFC(m_tpTitlePart.AsOrNull<IUIElement>()->put_Visibility(visibility));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ContentDialog::OnLayoutRootLoaded(
    _In_ IInspectable* sender,
    _In_ xaml::IRoutedEventArgs* args)
{
    // This handler will get executed when the layout root is loaded within the popup and also (in
    // the cases of dialogs defined in markup) when it is put back into the ContentDialog's
    // tree when finishing closing, so make sure to only execute the following code when the dialog
    // is actually showing.
    if (m_isShowing && m_placementMode != PlacementMode::InPlace)
    {
        IFC_RETURN(SetInitialFocusElement());
        IFC_RETURN(SizeAndPositionContentInPopup());
    }

    return S_OK;
}

_Check_return_ HRESULT
ContentDialog::OnLayoutRootProcessKeyboardAccelerators(
    _In_ IUIElement* pSender,
    _In_ xaml_input::IProcessKeyboardAcceleratorEventArgs* pArgs)
{
    // If we're already in the middle of processing accelerators, we don't need to do anything.  See the comment below about
    // how we can get ourselves into this situation.
    if (m_isProcessingKeyboardAccelerators)
    {
        return S_OK;
    }
    ctl::ComPtr<UIElement> pLayoutRoot = m_tpLayoutRootPart.AsOrNull<UIElement>();

    // Even if TryInvokeKeyboardAccelerator fails, we want to always mark the args as handled for ContentDialog to ensure the dialog appears
    // as if it's modal.
    wsy::VirtualKey key = wsy::VirtualKey_None;
    IFCFAILFAST(pArgs->get_Key(&key));
    auto alwaysHandledGuard = wil::scope_exit([&]
    {
        if (key != wsy::VirtualKey_Escape && key != wsy::VirtualKey_Enter)
        {
            // We will not set the property if key is already handled through TryInvokde code flow.
            BOOLEAN bAlreadyHandled = FALSE;
            IFCFAILFAST(pArgs->get_Handled(&bAlreadyHandled));
            if (!bAlreadyHandled)
            {
                // As we are marking each key handled though it's not actually, we will set HandledShouldNotImpedeTextInput
                // property true. InputManager will use this flag to skip SetKeyDownHandled on key, which
                // will allow it as an input to the text box.
                IFCFAILFAST(static_cast<ProcessKeyboardAcceleratorEventArgs*>(pArgs)->put_HandledShouldNotImpedeTextInput(TRUE));
                IFCFAILFAST(pArgs->put_Handled(TRUE));
            }
        }
    });
    if (pLayoutRoot)
    {
        // This handler is being called from the layout root - so we shouldn't just call TryInvokeKeyboardAccelerators on the layout root itself,
        // as that would cause the event on the layout root to be raised again and we'd be in an infinite loop here.
        // We could fix this in two ways - by iterating over all the children of the layout root and calling TryInvoke on those or we could just
        // use this flag to keep us from re-entering this handler.  The flag is simpler, and good enough for now.
        m_isProcessingKeyboardAccelerators = true;
        auto reentrancyGuard = wil::scope_exit([&] { m_isProcessingKeyboardAccelerators = false; });
        IFC_RETURN(pLayoutRoot->TryInvokeKeyboardAccelerator(pArgs));
    }

    return S_OK;
}

_Check_return_ HRESULT
ContentDialog::ShouldFireClosing(_Out_ bool* doFire)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IAsyncInfo> spAsyncInfo;
    wf::AsyncStatus status = wf::AsyncStatus::Started;

    *doFire = false;

    if (m_skipClosingEventOnHide)
    {
        goto Cleanup;
    }

    IFC(m_tpCurrentAsyncOperation.As(&spAsyncInfo));
    IFC(spAsyncInfo->get_Status(&status));
    if (status == wf::AsyncStatus::Canceled)
    {
        // If the async operation was canceled, the dialog is irrevocably being closed, so
        // we shouldn't fire.
        goto Cleanup;
    }

    *doFire = true;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ContentDialog::OnLayoutRootKeyDown(
    _In_ IInspectable* sender,
    _In_ xaml_input::IKeyRoutedEventArgs* args)
{
    return ProcessLayoutRootKey(true /*isKeyDown*/, args);
}

_Check_return_ HRESULT
ContentDialog::OnLayoutRootKeyUp(
    _In_ IInspectable* sender,
    _In_ xaml_input::IKeyRoutedEventArgs* args)
{
    return ProcessLayoutRootKey(false /*isKeyDown*/, args);
}

_Check_return_ HRESULT
ContentDialog::ProcessLayoutRootKey(
    bool isKeyDown,
    _In_ xaml_input::IKeyRoutedEventArgs* args)
{
    auto key = wsy::VirtualKey_None;

    IFCPTR_RETURN(args);
    IFC_RETURN(args->get_Key(&key));

    switch (key)
    {
        case wsy::VirtualKey_Escape:
        {
            auto originalKey = wsy::VirtualKey_None;

            IFC_RETURN(static_cast<KeyRoutedEventArgs*>(args)->get_OriginalKey(&originalKey));

            if ((!isKeyDown && originalKey == wsy::VirtualKey_GamepadB) ||
                (isKeyDown && originalKey == wsy::VirtualKey_Escape))
            {
                IFC_RETURN(ExecuteCloseAction());
                IFC_RETURN(args->put_Handled(TRUE));
            }
            break;
        }
        case wsy::VirtualKey_Enter:
        {
            if (isKeyDown)
            {
                ctl::ComPtr<xaml_primitives::IButtonBase> defaultButton;
                IFC_RETURN(GetDefaultButtonHelper(defaultButton.GetAddressOf()));

                if (defaultButton)
                {
                    BOOLEAN isDefaultButtonEnabled = FALSE;
                    IFC_RETURN(defaultButton.Cast<ButtonBase>()->get_IsEnabled(&isDefaultButtonEnabled));
                    if (isDefaultButtonEnabled)
                    {
                        IFC_RETURN(defaultButton.Cast<ButtonBase>()->ProgrammaticClick());
                        IFC_RETURN(args->put_Handled(TRUE));
                    }
                }
            }
            break;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ContentDialog::DetachEventHandlers()
{
    IFC_RETURN(DetachButtonEvents());

    if (auto layoutRoot = m_tpLayoutRootPart.GetSafeReference())
    {
        if (m_epLayoutRootPointerReleasedHandler)
        {
            IFC_RETURN(m_epLayoutRootPointerReleasedHandler.DetachEventHandler(layoutRoot.Get()));
        }

        if (m_epLayoutRootLoadedHandler)
        {
            IFC_RETURN(m_epLayoutRootLoadedHandler.DetachEventHandler(layoutRoot.Get()));
        }

        if (m_epLayoutRootKeyDownHandler)
        {
            IFC_RETURN(m_epLayoutRootKeyDownHandler.DetachEventHandler(layoutRoot.Get()));
        }

        if (m_epLayoutRootKeyUpHandler)
        {
            IFC_RETURN(m_epLayoutRootKeyUpHandler.DetachEventHandler(layoutRoot.Get()));
        }

        if (m_epLayoutRootGotFocusHandler)
        {
            IFC_RETURN(m_epLayoutRootGotFocusHandler.DetachEventHandler(layoutRoot.Get()));
        }

        if (m_epLayoutRootProcessKeyboardAcceleratorsHandler)
        {
            IFC_RETURN(m_epLayoutRootProcessKeyboardAcceleratorsHandler.DetachEventHandler(layoutRoot.Get()));
        }
    }

    if (auto backgroundElement = m_tpBackgroundElementPart.GetSafeReference())
    {
        if (m_dialogSizeChangedHandler)
        {
            IFC_RETURN(m_dialogSizeChangedHandler.DetachEventHandler(backgroundElement.Get()));
        }
    }

    if (auto dialogShowingStates = m_tpDialogShowingStates.GetSafeReference())
    {
        if (m_dialogShowingStateChangedEventHandler)
        {
            IFC_RETURN(m_dialogShowingStateChangedEventHandler.DetachEventHandler(dialogShowingStates.Get()));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ContentDialog::OnXamlRootChanged(
    _In_ xaml::IXamlRoot* /*sender*/,
    _In_ xaml::IXamlRootChangedEventArgs* args)
{
    wf::Size currentXamlRootSize{};
    if (auto xamlRoot = XamlRoot::GetForElementStatic(this))
    {
        IFC_RETURN(xamlRoot->get_Size(&currentXamlRootSize));
    }
    else
    {
        return E_UNEXPECTED;
    }

    if (m_tpSmokeLayer)
    {
        IFC_RETURN(m_tpSmokeLayer.Cast<Rectangle>()->put_Width(currentXamlRootSize.Width));
        IFC_RETURN(m_tpSmokeLayer.Cast<Rectangle>()->put_Height(currentXamlRootSize.Height));
    }

    if (m_placementMode != PlacementMode::InPlace)
    {
        IFC_RETURN(SizeAndPositionContentInPopup());
    }

    return S_OK;
}

_Check_return_ HRESULT
ContentDialog::OnDialogSizeChanged(
    _In_ IInspectable* /*sender*/,
    _In_ xaml::ISizeChangedEventArgs* /*args*/)
{
    if (m_prepareSmokeLayerAndPopup)
    {
        // HostDialogWithinPopup needs to be called again to set up m_tpSmokeLayerPopup and m_tpPopup with the new m_tpSmokeLayer.
        m_prepareSmokeLayerAndPopup = false;
        IFC_RETURN(HostDialogWithinPopup(true /*wasSmokeLayerFoundAsTemplatePart*/));
    }

    IFC_RETURN(UpdateVisualState());

    IFC_RETURN(ResetAndPrepareContent());

    if (m_placementMode != PlacementMode::InPlace)
    {
        IFC_RETURN(SizeAndPositionContentInPopup());
    }

    return S_OK;
}

_Check_return_ HRESULT ContentDialog::OnDialogShowingStateChanged(
    _In_ IInspectable* /*sender*/,
    _In_ xaml::IVisualStateChangedEventArgs* /*args*/)
{
    ASSERT(m_placementMode == PlacementMode::InPlace);

    // Fire the opened or closed events after visual transitions have played.
    if (m_isShowing && !m_hideInProgress)
    {
        IFC_RETURN(SetInitialFocusElement());
        IFC_RETURN(RaiseOpenedEvent());
    }
    else
    {
        ASSERT(m_hideInProgress);

        m_hideInProgress = false;
        IFC_RETURN(OnFinishedClosing());
    }

    return S_OK;
}

_Check_return_ HRESULT
ContentDialog::GetPlainText(
    _Out_ HSTRING* strPlainText)
{
    ctl::ComPtr<IInspectable> spTitle;
    *strPlainText = nullptr;

    IFC_RETURN(get_Title(&spTitle));

    if (spTitle)
    {
        IFC_RETURN(FrameworkElement::GetStringFromObject(spTitle.Get(), strPlainText));
    }
    else
    {
        // If we have no title, we'll fall back to the default implementation,
        // which retrieves our content as plain text (e.g., if our content is a string,
        // it returns that; if our content is a TextBlock, it returns its Text value, etc.)
        IFC_RETURN(ContentDialogGenerated::GetPlainText(strPlainText));

        // If we get the plain text from the content, then we want to truncate it,
        // in case the resulting automation name is very long.
        IFC_RETURN(Popup::TruncateAutomationName(strPlainText));
    }

    return S_OK;
}

_Check_return_ HRESULT
ContentDialog::SetPopupAutomationProperties()
{
    // Bug 15664046: m_tpPopup is expected to be null in some scenarios, like if the dialog is InPlace.
    if (!m_tpPopup)
    {
        return S_OK;
    }

    // If a Title string exists, make it the default AutomationProperties.Name string
    // for the Popup if the developer does not explicitly set one (see GetPlainText on
    // any FrameworkElement controls).
    wrl_wrappers::HString defaultAutomationName;
    IFC_RETURN(GetPlainText(defaultAutomationName.GetAddressOf()));
    IFC_RETURN(m_tpPopup.Cast<Popup>()->SetDefaultAutomationName(defaultAutomationName.Get()));

    // We'll explicitly pass on AutomationProperties.Name to the popup as well,
    // since GetPlainText is only the default plain-text to return if AutomationProperties.Name
    // is not set.
    wrl_wrappers::HString automationName;
    IFC_RETURN(DirectUI::AutomationProperties::GetNameStatic(this, automationName.GetAddressOf()));
    IFC_RETURN(DirectUI::AutomationProperties::SetNameStatic(m_tpPopup.Cast<Popup>(), automationName.Get()));

    // Update the automation Id as well.
    wrl_wrappers::HString automationId;
    IFC_RETURN(DirectUI::AutomationProperties::GetAutomationIdStatic(this, automationId.GetAddressOf()));
    IFC_RETURN(DirectUI::AutomationProperties::SetAutomationIdStatic(m_tpPopup.Cast<Popup>(), automationId.Get()));

    return S_OK;
}

_Check_return_ HRESULT ContentDialog::SetInitialFocusElement()
{
    BOOLEAN wasFocusSet = FALSE;

    // Save the focused element in order to give focus back to that once the ContentDialog dismisses.
    ctl::ComPtr<DependencyObject> previouslyFocusedObject;
    IFC_RETURN(GetFocusedElement(&previouslyFocusedObject));

    if (previouslyFocusedObject)
    {
        IFC_RETURN(previouslyFocusedObject.AsWeak(&m_spFocusedElementBeforeContentDialogShows));
    }

    // Try to set focus to the first focusable element in the content area.
    if (m_tpContentPart)
    {
        // We need to make sure all the nested templates of the ScrollViewer are expanded because that is where
        // we will find focusable fields within the dialogs content.
        if (m_tpContentScrollViewerPart)
        {
            m_tpContentScrollViewerPart.Cast<ScrollViewer>()->UpdateLayout();
        }

        ctl::ComPtr<IDependencyObject> searchRoot;
        IFC_RETURN(m_tpContentPart.As(&searchRoot));

        xref_ptr<CDependencyObject> firstFocusableDO;
        IFC_RETURN(CoreImports::FocusManager_GetFirstFocusableElement(
            static_cast<CDependencyObject*>(searchRoot.Cast<DependencyObject>()->GetHandle()),
            firstFocusableDO.ReleaseAndGetAddressOf()));

        if (firstFocusableDO)
        {
            ctl::ComPtr<DependencyObject> firstFocusableDOPeer;
            IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(firstFocusableDO, &firstFocusableDOPeer));
            CFocusManager *pFocusManager = VisualTree::GetFocusManagerForElement(GetHandle());

            InitialFocusSIPSuspender setInitalFocusTrue(pFocusManager);
            IFC_RETURN(DependencyObject::SetFocusedElement(firstFocusableDOPeer.Get(),
                xaml::FocusState_Programmatic, FALSE /*animateIfBringIntoView*/, &wasFocusSet));
        }
    }

    // If not set, try to focus the default button.
    if (!wasFocusSet)
    {
        ctl::ComPtr<xaml_primitives::IButtonBase> defaultButton;
        IFC_RETURN(GetDefaultButtonHelper(defaultButton.GetAddressOf()));
        if (defaultButton)
        {
            IFC_RETURN(DependencyObject::SetFocusedElement(defaultButton.Cast<ButtonBase>(),
                xaml::FocusState_Programmatic, FALSE /*animateIfBringIntoView*/, &wasFocusSet));
        }
    }

    // If not set, try to focus the first focusable command button.
    if (!wasFocusSet && m_tpCommandSpacePart)
    {
        ctl::ComPtr<IDependencyObject> searchRoot;
        IFC_RETURN(m_tpCommandSpacePart.As(&searchRoot));

        xref_ptr<CDependencyObject> firstFocusableDO;
        IFC_RETURN(CoreImports::FocusManager_GetFirstFocusableElement(
            static_cast<CDependencyObject*>(searchRoot.Cast<DependencyObject>()->GetHandle()),
            firstFocusableDO.ReleaseAndGetAddressOf()));

        if (firstFocusableDO)
        {
            ctl::ComPtr<DependencyObject> firstFocusableDOPeer;
            IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(firstFocusableDO, &firstFocusableDOPeer));

            IFC_RETURN(DependencyObject::SetFocusedElement(firstFocusableDOPeer.Get(),
                xaml::FocusState_Programmatic, FALSE /*animateIfBringIntoView*/, &wasFocusSet));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ContentDialog::ExecuteCloseAction()
{
    bool didInvokeClose = false;

    wrl_wrappers::HString closeButtonText;
    IFC_RETURN(get_CloseButtonText(closeButtonText.ReleaseAndGetAddressOf()));

    // If we have a clickable close button, then invoke it, otherwise just
    // return a result of None.
    if (!closeButtonText.IsEmpty())
    {
        if (m_tpCloseButtonPart)
        {
            BOOLEAN isCloseButtonEnabled = FALSE;
            IFC_RETURN(m_tpCloseButtonPart.Cast<ButtonBase>()->get_IsEnabled(&isCloseButtonEnabled));
            if (isCloseButtonEnabled)
            {
                IFC_RETURN(m_tpCloseButtonPart.Cast<ButtonBase>()->ProgrammaticClick());
                didInvokeClose = true;
            }
        }
    }

    // If there was no close button to invoke, the just call hide.
    if (!didInvokeClose)
    {
        IFC_RETURN(HideInternal(xaml_controls::ContentDialogResult_None));
    }

    return S_OK;
}

_Check_return_ HRESULT ContentDialog::RaiseOpenedEvent()
{
    ctl::ComPtr<ContentDialogOpenedEventArgs> args;
    IFC_RETURN(ctl::make(&args));

    OpenedEventSourceType* eventSource = nullptr;
    IFC_RETURN(GetOpenedEventSourceNoRef(&eventSource));

    IFC_RETURN(eventSource->Raise(this, args.Get()));

    return S_OK;
}

_Check_return_ HRESULT ContentDialog::RaiseClosedEvent(xaml_controls::ContentDialogResult result)
{
    ctl::ComPtr<ContentDialogClosedEventArgs> args;
    IFC_RETURN(ctl::make(&args));
    IFC_RETURN(args->put_Result(result));

    ClosedEventSourceType* eventSource = nullptr;
    IFC_RETURN(GetClosedEventSourceNoRef(&eventSource));

    IFC_RETURN(eventSource->Raise(this, args.Get()));

    return S_OK;
}

_Check_return_ HRESULT ContentDialog::SetButtonPropertiesFromCommand(xaml_controls::ContentDialogButton buttonType, _In_opt_ ICommand* oldCommand)
{
    KnownPropertyIndex commandPropertyIndex;
    KnownPropertyIndex textPropertyIndex;
    ctl::ComPtr<IButtonBase> button;
    ButtonBase* buttonNoRef;

    ASSERT(buttonType != xaml_controls::ContentDialogButton_None);
    IFC_RETURN(GetButtonHelper(buttonType, &button));

    if (button)
    {
        buttonNoRef = button.Cast<ButtonBase>();

        if (buttonType == xaml_controls::ContentDialogButton_Primary)
        {
            commandPropertyIndex = KnownPropertyIndex::ContentDialog_PrimaryButtonCommand;
            textPropertyIndex = KnownPropertyIndex::ContentDialog_PrimaryButtonText;
        }
        else if (buttonType == xaml_controls::ContentDialogButton_Secondary)
        {
            commandPropertyIndex = KnownPropertyIndex::ContentDialog_SecondaryButtonCommand;
            textPropertyIndex = KnownPropertyIndex::ContentDialog_SecondaryButtonText;
        }
        else /* buttonType == xaml_controls::ContentDialogButton_Close */
        {
            commandPropertyIndex = KnownPropertyIndex::ContentDialog_CloseButtonCommand;
            textPropertyIndex = KnownPropertyIndex::ContentDialog_CloseButtonText;
        }

        if (oldCommand)
        {
            ctl::ComPtr<ICommand> oldCommandComPtr(oldCommand);
            ctl::ComPtr<IXamlUICommand> oldCommandAsUICommand = oldCommandComPtr.AsOrNull<IXamlUICommand>();

            if (oldCommandAsUICommand)
            {
                IFC_RETURN(CommandingHelpers::ClearBindingIfSet(oldCommandAsUICommand.Get(), this, textPropertyIndex));
                IFC_RETURN(CommandingHelpers::ClearBindingIfSet(oldCommandAsUICommand.Get(), buttonNoRef, KnownPropertyIndex::UIElement_KeyboardAccelerators));
                IFC_RETURN(CommandingHelpers::ClearBindingIfSet(oldCommandAsUICommand.Get(), buttonNoRef, KnownPropertyIndex::UIElement_AccessKey));
                IFC_RETURN(CommandingHelpers::ClearBindingIfSet(oldCommandAsUICommand.Get(), buttonNoRef, KnownPropertyIndex::AutomationProperties_HelpText));
                IFC_RETURN(CommandingHelpers::ClearBindingIfSet(oldCommandAsUICommand.Get(), buttonNoRef, KnownPropertyIndex::ToolTipService_ToolTip));
            }
        }

        ctl::ComPtr<IInspectable> newCommandAsI;

        IFC_RETURN(GetValueByKnownIndex(commandPropertyIndex, &newCommandAsI));

        if (newCommandAsI)
        {
            ctl::ComPtr<ICommand> newCommand;

            IFC_RETURN(ctl::do_query_interface(newCommand, newCommandAsI.Get()));
            ctl::ComPtr<IXamlUICommand> newCommandAsUICommand = newCommand.AsOrNull<IXamlUICommand>();

            if (newCommandAsUICommand)
            {
                IFC_RETURN(CommandingHelpers::BindToLabelPropertyIfUnset(newCommandAsUICommand.Get(), this, textPropertyIndex));
                IFC_RETURN(CommandingHelpers::BindToKeyboardAcceleratorsIfUnset(newCommandAsUICommand.Get(), buttonNoRef));
                IFC_RETURN(CommandingHelpers::BindToAccessKeyIfUnset(newCommandAsUICommand.Get(), buttonNoRef));
                IFC_RETURN(CommandingHelpers::BindToDescriptionPropertiesIfUnset(newCommandAsUICommand.Get(), buttonNoRef));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ContentDialog::DiscardPopup()
{
    if (auto popup = m_tpPopup.GetSafeReference())
    {
        if (m_popupOpenedHandler)
        {
            IFC_RETURN(m_popupOpenedHandler.DetachEventHandler(popup.Get()));
        }
    }
    m_tpPopup.Clear();
    return S_OK;
}

_Check_return_ HRESULT ContentDialog::put_XamlRootImpl(_In_ xaml::IXamlRoot* pValue)
{
    IFC_RETURN(__super::put_XamlRootImpl(pValue));

    VisualTree *tree = VisualTree::GetForElementNoRef(GetHandle());

    if (m_tpSmokeLayerPopup)
    {
        static_cast<CPopup*>(m_tpSmokeLayerPopup.Cast<DirectUI::Popup>()->GetHandle())->SetAssociatedVisualTree(tree);
    }
    if (m_tpPopup)
    {
        static_cast<CPopup*>(m_tpPopup.Cast<DirectUI::Popup>()->GetHandle())->SetAssociatedVisualTree(tree);
    }
    return S_OK;
}
