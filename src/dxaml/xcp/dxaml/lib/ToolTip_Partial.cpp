// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ToolTip.g.h"
#include "ToolTipAutomationPeer.g.h"
#include "Popup.g.h"
#include "PopupRoot.g.h"
#include "Binding.g.h"
#include "ToolTipTemplateSettings.g.h"
#include "ToolTipService.g.h"
#include "VisualStateGroup.g.h"
#include "Storyboard.g.h"
#include "Window.g.h"
#include "VisualTreeHelper.h"
#include "TextElement.g.h"
#include "XboxUtility.h"
#include "ElevationHelper.h"
#include "ThemeShadow.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

ToolTip::ToolTip()
{
    m_pToolTipServicePlacementModeOverride = NULL;
    m_bIsPopupPositioned = FALSE;
    m_bClosing = FALSE;
    m_bInputEventsHookedUp = FALSE;
    m_bIsOpenAsAutomaticToolTip = FALSE;
    m_bCallPerformPlacementAtNextPopupOpen = FALSE;
    m_inputMode = AutomaticToolTipInputMode::None;
    m_isSliderThumbToolTip = FALSE;
}

ToolTip::~ToolTip()
{
    VERIFYHR(UnhookOwnerLayoutChangedEvent());
    VERIFYHR(UnhookFromXamlIslandRoot());

    delete m_pToolTipServicePlacementModeOverride;
    m_pToolTipServicePlacementModeOverride = NULL;
}

_Check_return_ HRESULT ToolTip::SetOwner(
    _In_opt_ xaml::IDependencyObject* pNewOwner)
{
    IFC_RETURN(UnhookOwnerLayoutChangedEvent());

    m_wrOwner = nullptr;

    if (pNewOwner)
    {
        IFC_RETURN(ctl::AsWeak(pNewOwner, &m_wrOwner));
    }

    return S_OK;
}


_Check_return_ HRESULT ToolTip::GetContainer(
    _Outptr_ xaml::IFrameworkElement** container)
{
    ctl::ComPtr<IFrameworkElement> spContainer;

    IFC_RETURN(m_wrContainer.As(&spContainer));
    *container = spContainer.Detach();

    return S_OK;
}

_Check_return_ HRESULT ToolTip::SetContainer(
    _In_opt_ xaml::IFrameworkElement* pNewContainer)
{
    ctl::ComPtr<IInspectable> spContainerDataContext;

    m_wrContainer = nullptr;

    if (pNewContainer)
    {
        IFC_RETURN(ctl::AsWeak(pNewContainer, &m_wrContainer));
        IFC_RETURN(pNewContainer->get_DataContext(&spContainerDataContext));
    }

    // If pNewContainer is NULL, we'll clear the DataContext here.
    IFC_RETURN(this->put_DataContext(spContainerDataContext.Get()));

    return S_OK;
}

// Prepares object's state
_Check_return_ HRESULT ToolTip::Initialize()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ISizeChangedEventHandler> spSizeChangedHandler;
    EventRegistrationToken sizeChangedToken;

    IFC(ToolTipGenerated::Initialize());

    spSizeChangedHandler.Attach(
        new ClassMemberEventHandler<
            ToolTip,
            IToolTip,
            ISizeChangedEventHandler,
            IInspectable,
            ISizeChangedEventArgs>(this, &ToolTip::OnToolTipSizeChanged, true /* subscribingToSelf */ ));
    IFC(add_SizeChanged(spSizeChangedHandler.Get(), &sizeChangedToken));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ToolTip::HookupParentPopup(
    _Outptr_ IPopup** ppPopup)
{
    ctl::ComPtr<Popup> spPopup;
    ctl::ComPtr<IFrameworkElement> spTarget;
    ctl::ComPtr<wf::IEventHandler<IInspectable*>> spPopupOpenedEventHandler;
    ctl::ComPtr<wf::IEventHandler<IInspectable*>> spPopupClosedEventHandler;
    ctl::ComPtr<xaml_data::IBindingBase> spFlowDirectionBindingBase;
    BOOLEAN bIsEnabled = FALSE;
    EventRegistrationToken popupOpenedToken;
    EventRegistrationToken popupClosedToken;

    IFCPTR_RETURN(ppPopup);
    *ppPopup = nullptr;

    IFC_RETURN(put_IsTabStop(FALSE));
    IFC_RETURN(put_IsHitTestVisible(FALSE));

    IFC_RETURN(GetTarget(&spTarget));

    IFC_RETURN(ctl::make<Popup>(&spPopup));

    // Set the Popup's ToolTip owner : This is a hint required by XamlIslandRoots
    // ToolTips use parentless popups.  ToolTip targets point to an element in the
    // XamlIslandRoot's tree and can be walked to find the root for that XamlIslandRoot.
    // The popup can then be rooted under the correct PopupRoot so it displays
    // relative to the island position.
    auto popup = do_pointer_cast<CPopup>(spPopup->GetHandle());
    if(popup)
    {
        popup->m_toolTipOwnerWeakRef = xref::get_weakref(static_cast<FrameworkElement*>(spTarget.Get())->GetHandle());
    }

    // ToolTip sets the location to the out of Xaml Window by using the windowed Popup in Threshold Windows
    if (CPopup::DoesPlatformSupportWindowedPopup(DXamlCore::GetCurrent()->GetHandle()) &&
        !static_cast<CPopup*>(spPopup.Cast<Popup>()->GetHandle())->IsWindowed())
    {
        // Set the windowed popup to support the render popup at the out of Xaml window
        IFC_RETURN(static_cast<CPopup*>(spPopup.Cast<Popup>()->GetHandle())->SetIsWindowed());
        ASSERT(static_cast<CPopup*>(spPopup.Cast<Popup>()->GetHandle())->IsWindowed());
    }

    // Don't show the Popup for disabled ToolTips.
    IFC_RETURN(get_IsEnabled(&bIsEnabled));
    if (!bIsEnabled)
    {
        IFC_RETURN(spPopup->put_Opacity(0));
    }

    if (spTarget)
    {
        ctl::ComPtr<Binding> spFlowDirectionBinding;
        wrl_wrappers::HString strLanguage;

        // Propagate FlowDirection from placement target to popup
        IFC_RETURN(ctl::make<Binding>(&spFlowDirectionBinding));

        IFC_RETURN(spFlowDirectionBinding->put_Source(spTarget.Get()));
        IFC_RETURN(Binding::SetPathString(spFlowDirectionBinding.Get(), wrl_wrappers::HStringReference(STR_LEN_PAIR(L"FlowDirection")).Get()));

        IFC_RETURN(spFlowDirectionBinding.As(&spFlowDirectionBindingBase));

        IFC_RETURN(spPopup->SetBinding(
            MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::FrameworkElement_FlowDirection),
            spFlowDirectionBindingBase.Get()));

        // Propagate Language from placement target to popup
        IFC_RETURN(spTarget->get_Language(strLanguage.GetAddressOf()));
        IFC_RETURN(spPopup->put_Language(strLanguage));
    }

    // Listening to the Opened and Closed events lets us guarantee that
    // the popup is actually opened when we perform those functions.

    spPopupOpenedEventHandler.Attach(
        new ClassMemberEventHandler<
            ToolTip,
            xaml_controls::IToolTip,
            wf::IEventHandler<IInspectable*>,
            IInspectable,
            IInspectable>(this, &ToolTip::OnPopupOpened));
    IFC_RETURN(spPopup->add_Opened(spPopupOpenedEventHandler.Get(), &popupOpenedToken));

    spPopupClosedEventHandler.Attach(
        new ClassMemberEventHandler<
            ToolTip,
            xaml_controls::IToolTip,
            wf::IEventHandler<IInspectable*>,
            IInspectable,
            IInspectable>(this, &ToolTip::OnPopupClosed));
    IFC_RETURN(spPopup->add_Closed(spPopupClosedEventHandler.Get(), &popupClosedToken));

    IFC_RETURN(spPopup.Cast<Popup>()->SetOwner(this));
    IFC_RETURN(spPopup.MoveTo(ppPopup));

    return S_OK;
}

// Handle the custom property changed event and call the OnPropertyChanged2 functions.
_Check_return_ HRESULT ToolTip::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(ToolTipGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::ToolTip_IsOpen:
            {
                BOOLEAN isOpen = !!args.m_pNewValue->AsBool();
                IFC_RETURN(OnIsOpenChanged(isOpen));
                break;
            }
        case KnownPropertyIndex::ToolTip_VerticalOffset:
        case KnownPropertyIndex::ToolTip_HorizontalOffset:
        case KnownPropertyIndex::ToolTip_PlacementRect:
            {
                IFC_RETURN(OnPlacementCriteriaChanged());
                break;
            }
    }

    return S_OK;
}

// Sets up instances that are expected on this type.
_Check_return_ HRESULT ToolTip::PrepareState()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ToolTipTemplateSettings> spTemplateSettings;

    IFC(ToolTipGenerated::PrepareState());
    IFC(ctl::make<ToolTipTemplateSettings>(&spTemplateSettings));
    IFC(put_TemplateSettings(spTemplateSettings.Get()));

Cleanup:
    RRETURN(hr);
}

// Create ToolTipAutomationPeer to represent the ToolTip.
IFACEMETHODIMP ToolTip::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<ToolTipAutomationPeer> spAutomationPeer;
    IFC(ActivationAPI::ActivateAutomationInstance(KnownTypeIndex::ToolTipAutomationPeer, GetHandle(), spAutomationPeer.GetAddressOf()));
    IFC(spAutomationPeer->put_Owner(this));
    *ppAutomationPeer = spAutomationPeer.Detach();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ToolTip::OnIsOpenChanged(
    _In_ BOOLEAN bIsOpen)
{
    HRESULT hr = S_OK;

    ToolTipServiceMetadata* pToolTipServiceMetadata = NULL;

    IFC(DXamlCore::GetCurrent()->GetToolTipServiceMetadata(pToolTipServiceMetadata));

    if (bIsOpen)
    {
        ctl::ComPtr<IPopup> spPopup;

        m_bIsOpenAsAutomaticToolTip = ToolTipService::s_bOpeningAutomaticToolTip;
        if (m_bIsOpenAsAutomaticToolTip)
        {
            ctl::ComPtr<IToolTip> spThisAsIToolTip;

            // If there is another ToolTip currently playing its closing animation, destroy it.
            if (pToolTipServiceMetadata->m_tpCurrentPopup)
            {
                ctl::ComPtr<IUIElement> spPopupChild;

                IFC(pToolTipServiceMetadata->m_tpCurrentPopup.Get()->get_Child(&spPopupChild));
                if (spPopupChild)
                {
                    ctl::ComPtr<IToolTip> spPreviousToolTip;

                    IFC(spPopupChild.As(&spPreviousToolTip));
                    IFC(spPreviousToolTip.Cast<ToolTip>()->ForceFinishClosing(NULL, NULL));
                }
                pToolTipServiceMetadata->m_tpCurrentPopup.Clear();
            }

            // Sometimes the current ToolTip is not fully closed by this point
            // we need to ensure that it is as so it is not still in the live tree
            // when we try to open it below
            // We should only try to do this if the popup is still alive
            // The only known issue so far with this is Slider Thumb tooltips, those
            // are opened automatically by the Slider control during the PointerPressed
            // event to show the current value.
            if (m_bClosing)
            {
                ctl::ComPtr<IPopup> spResolvedPopup;
                IFC(m_wrPopup.As(&spResolvedPopup));
                if (spResolvedPopup)
                {
                    IFC(ForceFinishClosing(NULL, NULL));
                }
            }

            ASSERT(!m_wrTargetOverride);
            ASSERT(!m_pToolTipServicePlacementModeOverride);
            ASSERT(pToolTipServiceMetadata->m_tpOwner != nullptr);

            if (pToolTipServiceMetadata->m_tpCloseTimer)
            {
                IFC(pToolTipServiceMetadata->m_tpCloseTimer->Stop());
            }

            if (pToolTipServiceMetadata->m_tpSafeZoneCheckTimer)
            {
                IFC(pToolTipServiceMetadata->m_tpSafeZoneCheckTimer->Stop());
            }

            IFC(ctl::do_query_interface(spThisAsIToolTip, this));
            pToolTipServiceMetadata->SetCurrentToolTip(spThisAsIToolTip.Get());

            IFC(SetPlacementOverrides(pToolTipServiceMetadata->m_tpContainer.Get()));

            IFC(HookupParentPopup(&spPopup));
            ASSERT(!pToolTipServiceMetadata->m_tpCurrentPopup);
            pToolTipServiceMetadata->SetCurrentPopup(spPopup.Get());
            IFC(spPopup.AsWeak(&m_wrPopup));

            if (pToolTipServiceMetadata->m_tpContainer)
            {
                IFC(static_cast<FrameworkElement*>(pToolTipServiceMetadata->m_tpContainer.Get())->SetHasOpenToolTip(TRUE));
            }
        }
        else
        {
            ctl::ComPtr<IFrameworkElement> spContainer;

            IFC(m_wrContainer.As(&spContainer));
            IFC(SetPlacementOverrides(spContainer.Get()));

            IFC(m_wrPopup.As(&spPopup));
            if (spPopup)
            {
                // Make sure to finish closing if we are currently closing the Popup.
                IFC(ForceFinishClosing(NULL, NULL));

                // Since the Popup closes/opens asynchronously, we may not get a SizeChanged notification
                // in the correct order, so we have to explicitly call PerformPlacement() the next time the
                // Popup is opened.
                m_bCallPerformPlacementAtNextPopupOpen = TRUE;
            }
            else
            {
                IFC(HookupParentPopup(&spPopup));
                IFC(spPopup.AsWeak(&m_wrPopup));
            }
        }

        IFC(ForwardOwnerThemePropertyToToolTip());

        IFC(OpenPopup());

        // Since ToolTip is kept open, need to hook up with CoreWindow or XamlIslandRoot to handle PointerMove and Key event
        IFC(HookupOwnerLayoutChangedEvent());
        IFC(HookupXamlIslandRoot());
        IGNOREHR(UpdateOwnersBoundary()); // it's OK if boundary can't be got
    }
    else
    {
        IFC(UnhookOwnerLayoutChangedEvent());
        IFC(UnhookFromXamlIslandRoot());

        // When IsOpen is set to True, we wait for the Popup to be created and opened before updating state.
        // When IsOpen is set to False, we go to the Closed state immediately and close the Popup after
        // we have finished transitioning to the Closed state.
        IFC(UpdateVisualState());

        delete m_pToolTipServicePlacementModeOverride;
        m_pToolTipServicePlacementModeOverride = NULL;
        m_wrTargetOverride = nullptr;

        if (m_bIsOpenAsAutomaticToolTip)
        {
            pToolTipServiceMetadata->m_tpCurrentToolTip.Clear();
            pToolTipServiceMetadata->m_tpCurrentPopup.Clear();
            if (pToolTipServiceMetadata->m_tpContainer)
            {
                IFC(static_cast<FrameworkElement*>(pToolTipServiceMetadata->m_tpContainer.Get())->SetHasOpenToolTip(FALSE));
            }

            m_bIsOpenAsAutomaticToolTip = FALSE;
        }

        m_bCallPerformPlacementAtNextPopupOpen = FALSE;
        m_inputMode = AutomaticToolTipInputMode::None;
    }

Cleanup:
    RRETURN(hr);
}

// Apply a template to the ToolTip.
IFACEMETHODIMP ToolTip::OnApplyTemplate()
{
    ctl::ComPtr<IDependencyObject> spChild;
    ctl::ComPtr<IFrameworkElement> spChildAsFE;
    INT childCount = 0;
    UINT nGroupCount = 0;
    UINT nStateCount = 0;
    EventRegistrationToken closedStateStoryboardCompletedToken;
    BOOLEAN bClosedStateHandled = FALSE;

    IFC_RETURN(ToolTipGenerated::OnApplyTemplate());

    // This hunk of code traverses the ToolTip's VSM states and finds its Closed state.
    // If this state is found, we add a Completed event handler to the Closed state's Storyboard.
    // When we finish going to the Closed state, then we need to close the ToolTip's Popup.
    // If no Closed state is found, we close the ToolTip's Popup immediately.
    // This logic allows us to achieve a fade-out effect for ToolTip.
    IFC_RETURN(VisualTreeHelper::GetChildrenCountStatic(this, &childCount));
    if (childCount)
    {
        IFC_RETURN(VisualTreeHelper::GetChildStatic(this, 0, &spChild));
        spChildAsFE = spChild.AsOrNull<IFrameworkElement>();

        if (spChildAsFE)
        {
            ctl::ComPtr<VisualStateManagerFactory> spFactory;
            ctl::ComPtr<wfc::IVector<xaml::VisualStateGroup*>> spChildVisualStateGroups;

            IFC_RETURN(ctl::make<VisualStateManagerFactory>(&spFactory));
            IFC_RETURN(spFactory->GetVisualStateGroups(spChildAsFE.Get(), &spChildVisualStateGroups));

            if (spChildVisualStateGroups)
            {
                wrl_wrappers::HString strStateName;
                wrl_wrappers::HStringReference strClosedStateName(STR_LEN_PAIR(L"Closed"));

                // TODO: Clean this up to use the following call instead of manually iterating the groups:
                // IFC_RETURN(VisualStateManager::TryGetState(spChildVisualStateGroups.Get(), hsClosedStateName, &spGroup, &spState, &bResult));

                IFC_RETURN(spChildVisualStateGroups->get_Size(&nGroupCount));
                for (UINT i = 0; i < nGroupCount; ++i)
                {
                    ctl::ComPtr<IVisualStateGroup> spGroup;
                    ctl::ComPtr<wfc::IVector<xaml::VisualState*>> spVisualStates;

                    IFC_RETURN(spChildVisualStateGroups->GetAt(i, &spGroup));
                    IFC_RETURN(spGroup.Cast<VisualStateGroup>()->get_States(&spVisualStates));
                    if (spVisualStates)
                    {
                        IFC_RETURN(spVisualStates->get_Size(&nStateCount));
                        for (UINT j = 0; j < nStateCount; ++j)
                        {
                            ctl::ComPtr<IVisualState> spState;

                            IFC_RETURN(spVisualStates->GetAt(j, &spState));

                            IFC_RETURN(spState->get_Name(strStateName.ReleaseAndGetAddressOf()));

                            if (strStateName == strClosedStateName)
                            {
                                ctl::ComPtr<IStoryboard> spStoryboard;
                                ctl::ComPtr<wf::IEventHandler<IInspectable*>> spClosedStateStoryboardCompletedHandler;

                                // Add handler close the Tooltip's Popup after its Closed state has completed.

                                spClosedStateStoryboardCompletedHandler.Attach(
                                    new ClassMemberEventHandler<
                                        ToolTip,
                                        IToolTip,
                                        wf::IEventHandler<IInspectable*>,
                                        IInspectable,
                                        IInspectable>(this, &ToolTip::ForceFinishClosing));
                                IFC_RETURN(spState->get_Storyboard(&spStoryboard));
                                if (spStoryboard)
                                {
                                    IFC_RETURN(spStoryboard.Cast<Storyboard>()->add_Completed(
                                        spClosedStateStoryboardCompletedHandler.Get(), &closedStateStoryboardCompletedToken));
                                }

                                bClosedStateHandled = TRUE;
                                break;
                            }
                        }

                        if (bClosedStateHandled)
                        {
                            break;
                        }
                    }
                }
            }
        }
    }

    // TODO: 105819 - Dynamic Timeline needs to clear and recreate children collection when its inheritance context changes
    // When the bug is fixed, uncomment UpdateVisualState() and delete the explicit GoToState call.
    //IFC_RETURN(UpdateVisualState(FALSE));
    BOOLEAN bIgnored = FALSE;
    IFC_RETURN(GoToState(FALSE, L"Opened", &bIgnored));

    return S_OK;
}

_Check_return_ HRESULT ToolTip::SetPlacementOverrides(
    _In_ IFrameworkElement* pInputTargetOverride)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ToolTipServiceFactory> spFactory;
    ctl::ComPtr<xaml::IDependencyObject> spOwnerAsIDO;
    ctl::ComPtr<IUIElement> spOwnerPlacementTarget;
    ctl::ComPtr<IFrameworkElement> spOwnerPlacementTargetAsFE;
    ctl::ComPtr<IFrameworkElement> spTargetOverride;

    xaml_primitives::PlacementMode ownerPlacement = DefaultPlacementMode;

    IFCPTR(pInputTargetOverride);

    IFC(ctl::make<ToolTipServiceFactory>(&spFactory));
    IFC(ctl::do_query_interface(spOwnerAsIDO, pInputTargetOverride));

    // The override values below will be cleared in ToolTip when the ToolTip closes.

    // ToolTipService overrides any Placement and PlacementTarget that have been set on the ToolTip.
    IFC(spFactory->GetPlacementTarget(spOwnerAsIDO.Get(), &spOwnerPlacementTarget));
    spOwnerPlacementTargetAsFE = spOwnerPlacementTarget.AsOrNull<IFrameworkElement>();
    if (spOwnerPlacementTargetAsFE)
    {
        spTargetOverride = spOwnerPlacementTargetAsFE;
    }
    else
    {
        spTargetOverride = pInputTargetOverride;
    }

    // Since we don't have coercion, we can't override the PlacementTarget like WPF does.
    // Instead, we use m_wrTargetOverride for this purpose.
    ASSERT(spTargetOverride.Get());
    if (spTargetOverride)
    {
        IFC(spTargetOverride.AsWeak(&m_wrTargetOverride));
    }

    // We need to tell if the Placement property has been set already.
    IFC(spFactory->GetPlacement(spOwnerAsIDO.Get(), &ownerPlacement));
    if (ownerPlacement != DefaultPlacementMode)
    {
        delete m_pToolTipServicePlacementModeOverride;
        m_pToolTipServicePlacementModeOverride = new xaml_primitives::PlacementMode(ownerPlacement);
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ToolTip::OnPlacementCriteriaChanged()
{
    IFC_RETURN(PerformPlacementInternal());
    return S_OK;
}

_Check_return_ HRESULT ToolTip::OpenPopup()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPopup> spPopup;
    ctl::ComPtr<IInspectable> spNewDataContext;
    BOOLEAN bTemplateApplied = FALSE;
    ctl::ComPtr<IControlTemplate> spTemplate;

    IFC(m_wrPopup.As(&spPopup));
    ASSERT(spPopup.Get(), L"popup from weak reference expected to be non-null");
    IFCPTR(spPopup);

    VisualTree* targetVisualTree = VisualTree::GetForElementNoRef(spPopup.Cast<Popup>()->GetHandle());

    // If the visual tree is null, there are two circumstances that might lead to this:
    // either we haven't yet parented the popup to a visual tree, in which case we can fall back
    // to using the visual tree of the tool tip target; or we may have gotten into the state
    // where we're cleaning up and no longer have a visual tree associated with the popup at all,
    // in which case we'll just do nothing.
    if (targetVisualTree == nullptr)
    {
        ctl::ComPtr<xaml::IDependencyObject> owner;

        if (SUCCEEDED(m_wrOwner.As(&owner)) && owner)
        {
            targetVisualTree = VisualTree::GetForElementNoRef(owner.Cast<DependencyObject>()->GetHandle());
        }

        if (targetVisualTree == nullptr)
        {
            goto Cleanup;
        }
    }

    IFC(spPopup->put_Child(this));

    IFC(get_DataContext(&spNewDataContext));
    IFC(spPopup.Cast<Popup>()->put_DataContext(spNewDataContext.Get()));

    ToolTipService::EnsureHandlersAttachedToRootElement(targetVisualTree);

    BOOLEAN popupIsOpen = FALSE;
    IFC(spPopup->get_IsOpen(&popupIsOpen));
    ASSERT(!popupIsOpen);
    IFC(spPopup->put_IsOpen(TRUE));

    // Count the number of times that we've opened the popup. Rapidly closing and reopening the popup can put the ToolTip
    // in an inconsistent state when the Popup.Opened event gets delayed in being delivered, so we wait until that final
    // Popup.Opened event in this scenario. See comment for m_pendingPopupOpenEventCount in header for details.
    m_pendingPopupOpenEventCount++;

    IFC(get_Template(&spTemplate));
    if (spTemplate)
    {
        IFC(ApplyTemplate(&bTemplateApplied));

        // Expected structure:
        //  CPopupRoot
        //    CToolTip
        //      CContentPresenter "LayoutRoot"
        //
        //  We apply shadow on ContentPresenter to capture FadeInThemeAnimation/FadeOutThemeAnimation

        ctl::ComPtr<IDependencyObject> contentPresenterAsDO;
        IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"LayoutRoot").Get(), &contentPresenterAsDO));

        if (contentPresenterAsDO)
        {
            ctl::ComPtr<IUIElement> contentPresenterAsUIE;
            IFC(contentPresenterAsDO.As(&contentPresenterAsUIE));

            if (CThemeShadow::IsDropShadowMode())
            {
                // Under drop shadows, ToolTip has a smaller shadow than normal
                IFC(ApplyElevationEffect(contentPresenterAsUIE.Get(), 0 /* depth */, 16 /* baseElevation */));
            }
            else
            {
                IFC(ApplyElevationEffect(contentPresenterAsUIE.Get()));
            }
        }
    }
    else
    {
        // In phone scenarios we do no offer a default template. So instead of having an open popup that doesn't display anything,
        // close it.
        IFC(spPopup->put_IsOpen(FALSE));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ToolTip::Close()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPopup> spPopup;
    ctl::ComPtr<IControlTemplate> spTemplate;

    IFC(UnhookOwnerLayoutChangedEvent());
    IFC(UnhookFromXamlIslandRoot());

    IFC(get_Template(&spTemplate));
    //If we don't have a template, we never opened the popup in the first place.
    if (spTemplate)
    {
        IFC(m_wrPopup.As(&spPopup));

        // Fix for Bug 2146297: The popup may already be null by the time ToolTip::Close() is called.
        if (spPopup)
        {
            // Though cleanup of the Popup will release the ToolTip, it's possible this ToolTip might try to open again
            // with a different Popup before that cleanup happens.  I've seen something to this effect happening, and found
            // that clearing Popup.Child here prevents this problem.
            IFC(spPopup.Cast<Popup>()->put_Child(NULL));
            IFC(spPopup->put_IsOpen(FALSE));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ToolTip::GetTarget(_Outptr_result_maybenull_ IFrameworkElement** ppTarget)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IFrameworkElement> spTarget;

    // If the ToolTipService is opening the ToolTip, then its owner is the placement target,
    // regardless of what the PlacementTarget has been set to.
    IFC(m_wrTargetOverride.As(&spTarget));
    if (!spTarget)
    {
        ctl::ComPtr<IUIElement> spTargetAsUIElement;

        IFC(get_PlacementTarget(&spTargetAsUIElement));
        IFC(spTargetAsUIElement.As(&spTarget));
    }

    IFC(spTarget.MoveTo(ppTarget));

Cleanup:
    RRETURN(hr);
}

// Sets the location of the ToolTip's Popup.
//
// Slider "Disambiguation UI" ToolTips need special handling since they need to remain centered
// over the sliding Thumb, which has not yet rendered in its new position.  Therefore, we pass
// the new target rect to handle this case.
_Check_return_ HRESULT ToolTip::PerformPlacement(
    _In_opt_ RECT* pTargetRect)
{
    HRESULT hr = S_OK;
    BOOLEAN isOpen;
    ctl::ComPtr<IPopup> spPopup;

    // It is possible for this function to be called even though the ToolTip is closed.
    // This can happen if the ToolTip gets closed before it has had a chance to layout and complete its opening sequence.
    // If this happens, we don't want to continue opening, as that could result in a "closed" ToolTip that is still visible on the screen.
    IFC(get_IsOpen(&isOpen));

    IFC(m_wrPopup.As(&spPopup));

    if (spPopup && isOpen)
    {
        if (static_cast<CPopup*>(spPopup.Cast<Popup>()->GetHandle())->IsWindowed())
        {
            // Sets the location of the ToolTip's Popup out of the Xaml window.
            IFC(PerformPlacementWithWindowedPopup(pTargetRect));
        }
        else
        {
            // Sets the location of the ToolTip's Popup within the Xaml window.
            IFC(PerformPlacementWithPopup(pTargetRect));
        }

        // If PerformPlacementWithPopup/PerformPlacementWithWindowedPopup fail to position the Popup, they will set ToolTip.IsOpen=False.
        // If this happens, we don't want to continue opening the ToolTip.
        IFC(get_IsOpen(&isOpen));

        if (!m_bIsPopupPositioned && isOpen)
        {
            m_bIsPopupPositioned = TRUE;

            // TODO: 105819 - Dynamic Timeline needs to clear and recreate children collection when its inheritance context changes
            // When the bug is fixed, uncomment UpdateVisualState() and delete the explicit GoToState call.
            //IFC(UpdateVisualState());
            BOOLEAN bIgnored = FALSE;
            IFC(GoToState(FALSE, L"Closed", &bIgnored));
            IFC(GoToState(TRUE, L"Opened", &bIgnored));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Sets the location of the ToolTip's Popup within the Xaml window.
_Check_return_ HRESULT ToolTip::PerformPlacementWithPopup(
    _In_opt_ RECT* pTargetRect) noexcept
{
    HRESULT hr = S_OK;
    BOOLEAN isOpen = FALSE;
    BOOLEAN isEnabled = FALSE;
    DOUBLE tooltipActualWidth = 0;
    DOUBLE tooltipActualHeight = 0;
    DOUBLE horizontalOffset = 0;
    DOUBLE verticalOffset = 0;
    xaml_primitives::PlacementMode placement = DefaultPlacementMode;

    // Make sure we can actually place the ToolTip.  The size should be > 0, and
    // IsOpen and IsEnabled should be true.
    IFC_RETURN(get_IsOpen(&isOpen));
    IFC_RETURN(get_IsEnabled(&isEnabled));
    IFC_RETURN(get_ActualWidth(&tooltipActualWidth));
    IFC_RETURN(get_ActualHeight(&tooltipActualHeight));
    if (isOpen == FALSE ||
        isEnabled == FALSE ||
        !DoubleUtil::GreaterThan(tooltipActualWidth, 0) ||
        !DoubleUtil::GreaterThan(tooltipActualHeight, 0))
    {
        return hr;
    }

    IFC_RETURN(get_HorizontalOffset(&horizontalOffset));
    IFC_RETURN(get_VerticalOffset(&verticalOffset));

    // If the ToolTipService is opening the ToolTip, then its Placement is used,
    // regardless of what the ToolTip.Placement has been set to.
    if (m_pToolTipServicePlacementModeOverride)
    {
        placement = *m_pToolTipServicePlacementModeOverride;
    }
    else
    {
        IFC_RETURN(get_Placement(&placement));
    }


    RECT dimentions{ static_cast<LONG>(horizontalOffset), static_cast<LONG>(verticalOffset), static_cast<LONG>(tooltipActualWidth), static_cast<LONG>(tooltipActualHeight) };

    // PlacementMode.Mouse only makes sense for automatic ToolTips opened by touch or mouse.
    if (placement == xaml_primitives::PlacementMode_Mouse &&
        (m_inputMode == AutomaticToolTipInputMode::Touch || m_inputMode == AutomaticToolTipInputMode::Mouse))
    {
        IFC_RETURN(PerformMousePlacementWithPopup(&dimentions, placement));
    }
    else
    {
        IFC_RETURN(PerformNonMousePlacementWithPopup(pTargetRect, &dimentions, placement));
    }

    return hr;
}

// Gets cached PointerPoint from XamlIslandRoot or CoreWindow
wrl::ComPtr<ixp::IPointerPoint> ToolTip::GetCurrentPointFromRootOrCoreWindow(
    _In_ const ctl::ComPtr<IFrameworkElement>& spTarget)
{
    wrl::ComPtr<ixp::IPointerPoint> currentPointerPoint = nullptr;
    CUIElement* handle = nullptr;

    // The current point is cached on either the CXamlIslandRoot or a windowed CPopup. We need an element to start
    // searching from. Use the ToolTip target element bu default, because that's probably where the mouse moved to
    // show the ToolTip in the first place.
    ctl::ComPtr<DirectUI::UIElement> targetAsUIE;
    IFCFAILFAST(ctl::do_query_interface(targetAsUIE, spTarget.Get()));
    if (targetAsUIE->GetHandle()->IsActive())
    {
        handle = targetAsUIE->GetHandle();
    }
    else
    {
        // Target element isn't in the tree, so use the ToolTip itself instead
        handle = GetHandle();
    }

    // Now walk up from the search element until we hit either a windowed popup or a XamlIslandRoot. The XamlIslandRoot
    // can be quickly found via the VisualTree, but the windowed popup requires a manual walk up the element tree. As
    // an optimization, only look for popups if there are open popups anywhere in the tree.
    VisualTree* visualTree = VisualTree::GetForElementNoRef(handle);
    if (visualTree)
    {
        CPopup* ancestorWindowedPopup = nullptr;
        if (visualTree->GetPopupRoot()->HasOpenOrUnloadingPopups())
        {
            ancestorWindowedPopup = handle->GetFirstAncestorPopup(true /* windowedOnly */);
        }

        if (ancestorWindowedPopup)
        {
            // Get cached point from windowed popup. Note that this point has already been transformed by the difference
            // between the windowed popup and the root of the Xaml tree. See WindowedPopupInputSiteAdapter's
            // SetTransformFromContentRoot, OnPointerMessage, and GetTransformedPointerPoint methods.
            currentPointerPoint = ancestorWindowedPopup->GetPreviousPointerPoint();
        }
        else
        {
            CXamlIslandRoot* root = visualTree->GetXamlIslandRootForElement(handle);
            if (root)
            {
                // Get cached point from CXamlIslandRoot.
                currentPointerPoint = root->GetPreviousPointerPoint();
            }
            else
            {
                // There are no islands, so we're in a CoreWindow in a UWP. Get the cached point from JupiterWindow's
                // InputSiteAdapter.
                CJupiterWindow* jupiterWindow = DirectUI::DXamlServices::GetCurrentJupiterWindow();
                currentPointerPoint = jupiterWindow->GetInputSiteAdapterPointerPoint();
            }
        }
    }

    return currentPointerPoint;
}

// Sets the location of the ToolTip's Popup within the Xaml window.
_Check_return_ HRESULT ToolTip::PerformMousePlacementWithPopup(
    _In_opt_ RECT* pDimentions, _In_ xaml_primitives::PlacementMode placement) noexcept
{
    HRESULT hr = S_OK;

    DOUBLE tooltipActualWidth = pDimentions->right;
    DOUBLE tooltipActualHeight = pDimentions->bottom;
    DOUBLE horizontalOffset = pDimentions->left;
    DOUBLE verticalOffset = pDimentions->top;

    XUINT32 screenWidth = 0;
    XUINT32 screenHeight = 0;
    BOOLEAN bIsRTL = FALSE;

    ctl::ComPtr<IFrameworkElement> spTarget;
    wrl::ComPtr<ixp::IPointerPoint> spCurrentPointerPoint;
    ctl::ComPtr<IPopup> spPopup;

    DOUBLE maxX = 0.0;
    DOUBLE maxY = 0.0;
    DOUBLE left = 0.0;
    DOUBLE top = 0.0;
    wf::Rect toolTipRect = {};
    wf::Rect intersectionRect = {};

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFC_RETURN(CoreImports::Host_GetActualWidth(static_cast<CCoreServices*>(pCore->GetHandle()), &screenWidth));
    // TODO: Fix this with HMON changes (#27504100)
    if (screenWidth == 0)
    {
        screenWidth = 65535;
    }
    IFC_RETURN(CoreImports::Host_GetActualHeight(static_cast<CCoreServices*>(pCore->GetHandle()), &screenHeight));
    if (screenHeight == 0)
    {
        screenHeight = 65535;
    }

    IFC_RETURN(GetTarget(&spTarget));

    ASSERT(spTarget.Get(), L"pTarget expected to be non-null in ToolTip_Partial::PerformPlacement()");
    if (spTarget)
    {
        xaml::FlowDirection targetFlowDirection = xaml::FlowDirection_LeftToRight;
        IFC_RETURN(spTarget->get_FlowDirection(&targetFlowDirection));
        bIsRTL = targetFlowDirection == xaml::FlowDirection_RightToLeft;

        // We should not do placement if the target is no longer in the live tree.
        if (!spTarget.Cast<FrameworkElement>()->IsInLiveTree())
        {
            return hr;
        }
    }

    IFC_RETURN(m_wrPopup.As(&spPopup));
    ASSERT(spPopup.Get(), L"popup from weak reference expected to be non-null");
    IFCPTR_RETURN(spPopup);

    ASSERT(m_bIsOpenAsAutomaticToolTip, L"m_bIsOpenAsAutomaticToolTip expected to be true for PlacementMode.Mouse");

    wf::Point lastPointerEnteredPoint = {};

    spCurrentPointerPoint = GetCurrentPointFromRootOrCoreWindow(spTarget);
    if (!spCurrentPointerPoint)
    {
        IFC_RETURN(put_IsOpen(FALSE));

        return S_OK;
    }

    IFC_RETURN(spCurrentPointerPoint->get_Position(&lastPointerEnteredPoint));

    left = lastPointerEnteredPoint.X;
    top = lastPointerEnteredPoint.Y;

    // If we are in RTL mode, then flip the X coordinate around so that it appears to be in LTR mode. That
    // means all of the LTR logic will still work.
    if (bIsRTL)
    {
        left = screenWidth - left;
    }

    IFC_RETURN(MovePointToPointerToolTipShowPosition(left, top, placement));

    // align ToolTip with the bottom left corner of mouse bounding rectangle
    top += m_mousePlacementVerticalOffset + verticalOffset;
    left += horizontalOffset;

    // pessimistic check of top value - can be 0 only if TextBlock().FontSize == 0
    top = DoubleUtil::Max(TOOLTIP_TOLERANCE, top);

    // left can be less then TOOLTIP_tolerance if user put mouse pointer on the border of object
    left = DoubleUtil::Max(TOOLTIP_TOLERANCE, left);

    maxX = screenWidth;
    maxY = screenHeight;

    toolTipRect.X = static_cast<FLOAT>(left);
    toolTipRect.Y = static_cast<FLOAT>(top);
    toolTipRect.Width = static_cast<FLOAT>(tooltipActualWidth);
    toolTipRect.Height = static_cast<FLOAT>(tooltipActualHeight);

    intersectionRect.Width = static_cast<FLOAT>(maxX);
    intersectionRect.Height = static_cast<FLOAT>(maxY);

    IFC_RETURN(RectUtil::Intersect(intersectionRect, toolTipRect));
    if ((DoubleUtil::Abs(intersectionRect.Width - toolTipRect.Width) < TOOLTIP_TOLERANCE) &&
        (DoubleUtil::Abs(intersectionRect.Height - toolTipRect.Height) < TOOLTIP_TOLERANCE))
    {
        // The placement algorithm operates in LTR mode (with transformed data if it
        // is really in RTL mode), so we also need to transform the X value it returns.
        if (bIsRTL)
        {
            left = screenWidth - left;
        }

        // ToolTip is completely inside the plug-in
        IFC_RETURN(spPopup->put_VerticalOffset(top));
        IFC_RETURN(spPopup->put_HorizontalOffset(left));
    }
    else
    {
        if (top + toolTipRect.Height > maxY)
        {
            // If the lower edge of the plug-in obscures the ToolTip,
            // it repositions itself to align with the upper edge of the bounding box of the mouse.
            top = maxY - toolTipRect.Height - TOOLTIP_TOLERANCE;
        }

        if (top < 0)
        {
            // If the upper edge of Plug-in obscures the ToolTip,
            // the control repositions itself to align with the upper edge.
            // align with the top of the plug-in
            top = 0;
        }

        if (left + toolTipRect.Width > maxX)
        {
            // If the right edge obscures the ToolTip,
            // it opens in the opposite direction from the obscuring edge.
            left = maxX - toolTipRect.Width - TOOLTIP_TOLERANCE;
        }

        if (left < 0)
        {
            // If the left edge obscures the ToolTip,
            // it then aligns with the obscuring screen edge
            left = 0;
        }

        // if right/bottom doesn't fit into the plug-in bounds, clip the ToolTip
        {
            RECT clipCalculationsRect{ static_cast<LONG>(left), static_cast<LONG>(top), static_cast<LONG>(toolTipRect.Width), static_cast<LONG>(toolTipRect.Height) };
            IFC_RETURN(CalculateTooltipClip(&clipCalculationsRect, maxX, maxY));
        }

        // The placement algorithm operates in LTR mode (with transformed data if it
        // is really in RTL mode), so we also need to transform the X value it returns.

        if (bIsRTL)
        {
            left = screenWidth - left;
        }

        // position the parent Popup
        IFC_RETURN(spPopup->put_VerticalOffset(top + verticalOffset));
        IFC_RETURN(spPopup->put_HorizontalOffset(left + horizontalOffset));
    }

    return hr;
}

// Sets the location of the ToolTip's Popup within the Xaml window.
_Check_return_ HRESULT ToolTip::PerformNonMousePlacementWithPopup(
    _In_opt_ RECT* pTargetRect, _In_opt_ RECT* pDimentions, _In_ xaml_primitives::PlacementMode placement) noexcept
{
    HRESULT hr = S_OK;

    DOUBLE tooltipActualWidth = pDimentions->right;
    DOUBLE tooltipActualHeight = pDimentions->bottom;
    DOUBLE horizontalOffset = pDimentions->left;
    DOUBLE verticalOffset = pDimentions->top;

    BOOLEAN bIsRTL = FALSE;

    ctl::ComPtr<IFrameworkElement> spTarget;
    wrl::ComPtr<ixp::IPointerPoint> spCurrentPointerPoint;
    ctl::ComPtr<IPopup> spPopup;

    xaml_primitives::PlacementMode placementChosen = DefaultPlacementMode;
    ctl::ComPtr<xaml_media::IGeneralTransform> spTransformToRoot;

    wf::Rect visibleRect = {};
    wf::Rect windowRect = {};
    ToolTipPositioning::CConstraint constraint;
    SIZE szFlyout = {};
    DOUBLE targetActualWidth = 0.0;
    DOUBLE targetActualHeight = 0.0;
    wf::Point origin = {};
    wf::Point targetTopLeft = {};
    RECT rcDockTo = {};
    RECT rcResult = {};
    BOOLEAN isPropertyLocal = FALSE;

    IFC_RETURN(GetTarget(&spTarget));

    ASSERT(spTarget.Get(), L"pTarget expected to be non-null in ToolTip_Partial::PerformPlacement()");
    if (spTarget)
    {
        xaml::FlowDirection targetFlowDirection = xaml::FlowDirection_LeftToRight;

        IFC_RETURN(spTarget->get_FlowDirection(&targetFlowDirection));
        bIsRTL = targetFlowDirection == xaml::FlowDirection_RightToLeft;

        // We should not do placement if the target is no longer in the live tree.
        if (!spTarget.Cast<FrameworkElement>()->IsInLiveTree())
        {
            return hr;
        }
    }

    IFC_RETURN(m_wrPopup.As(&spPopup));
    ASSERT(spPopup.Get(), L"popup from weak reference expected to be non-null");
    IFCPTR_RETURN(spPopup);

    // For ToolTips opened by keyboard focus, PlacementMode_Mouse doesn't make any sense.
    // Fall back to the default - PlacementMode_Top.
    if (placement == xaml_primitives::PlacementMode_Mouse)
    {
        placement = xaml_primitives::PlacementMode_Top;
    }

    if (!spTarget)
    {
        return hr;
    }

    IFC_RETURN(DXamlCore::GetCurrent()->GetContentLayoutBoundsForElement(GetHandle(), &visibleRect));
    constraint.SetRect(
        static_cast<INT>(visibleRect.X),
        static_cast<INT>(visibleRect.Y),
        static_cast<INT>(visibleRect.X + visibleRect.Width),
        static_cast<INT>(visibleRect.Y + visibleRect.Height));

    IFC_RETURN(DXamlCore::GetCurrent()->GetContentBoundsForElement(GetHandle(), &windowRect));
    origin.X = windowRect.X;
    origin.Y = windowRect.Y;

    szFlyout.cx = static_cast<LONG>(tooltipActualWidth);
    szFlyout.cy = static_cast<LONG>(tooltipActualHeight);

    IFC_RETURN(spTarget->get_ActualWidth(&targetActualWidth));
    IFC_RETURN(spTarget->get_ActualHeight(&targetActualHeight));
    // Task 23865114: Move windowed popups to lifted input
    // Windowed popups are disabled for the time being, and ToolTips are now opening with unwindowed popups, which
    // can hit these asserts (e.g. ColorPicker tests from MUXC). Ignore these for now.
//    ASSERT(DoubleUtil::GreaterThan(targetActualWidth, 0.0));
//    ASSERT(DoubleUtil::GreaterThan(targetActualHeight, 0.0));

    wf::Rect placementRect = {};
    IFC_RETURN(GetPlacementRectInWindowCoordinates(&placementRect));

    bool getDockToRectFromTargetElement = false;

    if (pTargetRect)
    {
        // Slider case - position ToolTip over Thumb rect
        rcDockTo = *pTargetRect;
    }
    else if (!RectUtil::GetIsEmpty(placementRect))
    {
        rcDockTo.left = static_cast<LONG>(placementRect.X);
        rcDockTo.top = static_cast<LONG>(placementRect.Y);
        rcDockTo.right = static_cast<LONG>(placementRect.X + placementRect.Width);
        rcDockTo.bottom = static_cast<LONG>(placementRect.Y + placementRect.Height);
    }
    else if (!m_isSliderThumbToolTip &&
        (AutomaticToolTipInputMode::Touch == m_inputMode || AutomaticToolTipInputMode::Mouse == m_inputMode))
    {
        wf::Point lastPointerEnteredPoint = {};

        spCurrentPointerPoint = GetCurrentPointFromRootOrCoreWindow(spTarget);

        if (!spCurrentPointerPoint)
        {
            // If we don't have a cached PointerPoint, we fall back to getting the placement location from the target element.
            getDockToRectFromTargetElement = true;
        }
        else
        {
            IFC_RETURN(spCurrentPointerPoint->get_Position(&lastPointerEnteredPoint));
            rcDockTo.left = rcDockTo.right = static_cast<LONG>(lastPointerEnteredPoint.X);
            rcDockTo.top = rcDockTo.bottom = static_cast<LONG>(lastPointerEnteredPoint.Y);

            // For touch, we need to account for the fact that the context menu hint has a vertical offset.
            if (AutomaticToolTipInputMode::Touch == m_inputMode)
            {
                rcDockTo.top += CONTEXT_MENU_HINT_VERTICAL_OFFSET;
                rcDockTo.bottom += CONTEXT_MENU_HINT_VERTICAL_OFFSET;
            }
        }
    }
    else
    {
        getDockToRectFromTargetElement = true;
    }

    if(getDockToRectFromTargetElement)
    {
        IFC_RETURN(spTarget.Cast<FrameworkElement>()->TransformToVisual(NULL, &spTransformToRoot));
        IFC_RETURN(spTransformToRoot->TransformPoint(targetTopLeft, &targetTopLeft));
        // targetTopLeft.X should be the left edge of the target, so adjust for RTL by
        // subtracting the width of the target.
        if (bIsRTL)
        {
            targetTopLeft.X -= static_cast<FLOAT>(targetActualWidth);
        }

        rcDockTo.left = static_cast<LONG>(targetTopLeft.X);
        rcDockTo.top = static_cast<LONG>(targetTopLeft.Y);
        rcDockTo.right = static_cast<LONG>(targetTopLeft.X + targetActualWidth);
        rcDockTo.bottom = static_cast<LONG>(targetTopLeft.Y + targetActualHeight);
    }

    OffsetRect(&rcDockTo, static_cast<INT>(origin.X), static_cast<INT>(origin.Y));

    // If horizontal & vertical offset are not specified, use the system defaults.
    IFC_RETURN(IsPropertyLocal(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ToolTip_HorizontalOffset),
        &isPropertyLocal));

    if (!isPropertyLocal)
    {
        switch (m_inputMode)
        {
        case AutomaticToolTipInputMode::Keyboard:
            horizontalOffset = DEFAULT_KEYBOARD_OFFSET;
            break;
        case AutomaticToolTipInputMode::Mouse:
            horizontalOffset = DEFAULT_MOUSE_OFFSET;
            break;
        case AutomaticToolTipInputMode::Touch:
            horizontalOffset = DEFAULT_TOUCH_OFFSET;
            break;
        }
    }

    IFC_RETURN(IsPropertyLocal(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ToolTip_VerticalOffset),
        &isPropertyLocal));

    if (!isPropertyLocal)
    {
        switch (m_inputMode)
        {
        case AutomaticToolTipInputMode::Keyboard:
            verticalOffset = DEFAULT_KEYBOARD_OFFSET;
            break;
        case AutomaticToolTipInputMode::Mouse:
            verticalOffset = DEFAULT_MOUSE_OFFSET;
            break;
        case AutomaticToolTipInputMode::Touch:
            verticalOffset = DEFAULT_TOUCH_OFFSET;
            break;
        }
    }

    // Reverse Left/Right placement for RTL, because the target is flipped.
    if (bIsRTL)
    {
        if (placement == xaml_primitives::PlacementMode_Right)
        {
            placement = xaml_primitives::PlacementMode_Left;
        }
        else if (placement == xaml_primitives::PlacementMode_Left)
        {
            placement = xaml_primitives::PlacementMode_Right;
        }
    }

    // To honor horizontal/vertical offset, inflate the placement target by these offsets before calling into
    // the Windows popup positioning logic.
    InflateRect(&rcDockTo, static_cast<INT>(horizontalOffset), static_cast<INT>(verticalOffset));
    rcResult = ToolTipPositioning::QueryRelativePosition(
        constraint,
        szFlyout,
        rcDockTo,
        placement,
        ToolTipPositioning::POPUP_SNAPPING::PS_NONE,
        &placementChosen);

    // if right/bottom doesn't fit into the plug-in bounds, clip the ToolTip
    {
        RECT clipCalculationsRect{ static_cast<LONG>(0.0), static_cast<LONG>(0.0), static_cast<LONG>(tooltipActualWidth), static_cast<LONG>(tooltipActualHeight) };
        IFC_RETURN(CalculateTooltipClip(&clipCalculationsRect, visibleRect.Width - visibleRect.X, visibleRect.Height - visibleRect.Y));
    }

    // Position tooltip by setting popup's offsets
    IFC_RETURN(spPopup->put_VerticalOffset(rcResult.top - origin.Y));
    // ToolTipPositioning::QueryRelativePosition is used to position in LTR and RTL. In LTR, the horizontal
    // offset is  in rcResult.left. In RTL, the horizontal offset is in rcResult.right because the
    // popup is flipped.
    IFC_RETURN(spPopup->put_HorizontalOffset((bIsRTL) ? rcResult.right - origin.X : rcResult.left - origin.X));

    // There used to be a setting of FromVerticalOffset and FromHorizontalOffset on the ToolTipTemplateSettings
    // gotten from get_TemplateSettings here.  However, this is no longer done because the ToolTip animation
    // is now a FadeIn/FadeOut which doesn't use any FromHorizontalOffset/FromVerticalOffset.  This leaves
    // ToolTipTemplateSettings and all associated code in a basically non-used and deprecated state - but as of
    // now, we can't make any breaking changes.  So, to preserve max compatibility, the ToolTipTemplateSettings
    // is still around as a class/interface/property of the tooltip for now.  It should be removed (or at least
    // the ToolTip-specific-ness of the Tooltip's template settings should be removed) as soon as it's ok to
    // make a breaking change.

    return hr;
}


_Check_return_ HRESULT ToolTip::CalculateTooltipClip(
    _In_opt_ RECT* toolTipRect, DOUBLE maxX, DOUBLE maxY) noexcept
{
    wf::Size clipSize = {};
    DOUBLE dX = 0.0;
    DOUBLE dY = 0.0;

    dX = toolTipRect->left + toolTipRect->right - maxX;
    dY = toolTipRect->top + toolTipRect->bottom - maxY;

    if ((dX >= 0) || (dY >= 0))
    {
        dX = DoubleUtil::Max(0, dX);
        dY = DoubleUtil::Max(0, dY);
        clipSize.Width = static_cast<FLOAT>(DoubleUtil::Max(0, toolTipRect->right - dX));
        clipSize.Height = static_cast<FLOAT>(DoubleUtil::Max(0, toolTipRect->bottom - dY));
        IFC_RETURN(PerformClipping(clipSize));
    }

    return S_OK;
}

// Sets the location of the ToolTip's Popup out of the Xaml window.
_Check_return_ HRESULT ToolTip::PerformPlacementWithWindowedPopup(
    _In_opt_ RECT* pTargetRect)
{
    HRESULT hr = S_OK;
    bool bIsRTL = FALSE;
    BOOLEAN isOpen = FALSE;
    BOOLEAN isEnabled = FALSE;
    DOUBLE tooltipActualWidth = 0;
    DOUBLE tooltipActualHeight = 0;
    DOUBLE horizontalOffset = 0;
    DOUBLE verticalOffset = 0;
    xaml_primitives::PlacementMode placement = DefaultPlacementMode;
    xaml::FlowDirection targetFlowDirection = xaml::FlowDirection_LeftToRight;
    wf::Point currentPoint = {};
    wf::Rect availableMonitorRect = {};
    ctl::ComPtr<IFrameworkElement> spTarget;

    // Make sure we can actually place the ToolTip.  The size should be > 0, and
    // IsOpen and IsEnabled should be true.
    IFC(get_IsOpen(&isOpen));
    IFC(get_IsEnabled(&isEnabled));
    IFC(get_ActualWidth(&tooltipActualWidth));
    IFC(get_ActualHeight(&tooltipActualHeight));

    if (isOpen == FALSE ||
        isEnabled == FALSE ||
        !DoubleUtil::GreaterThan(tooltipActualWidth, 0) ||
        !DoubleUtil::GreaterThan(tooltipActualHeight, 0))
    {
        goto Cleanup;
    }

    // Get the horizontal/vertical offset between the target and ToolTip
    IFC(get_HorizontalOffset(&horizontalOffset));
    IFC(get_VerticalOffset(&verticalOffset));

    // If the ToolTipService is opening the ToolTip, then its Placement is used,
    // regardless of what the ToolTip.Placement has been set to.
    if (m_pToolTipServicePlacementModeOverride)
    {
        placement = *m_pToolTipServicePlacementModeOverride;
    }
    else
    {
        IFC(get_Placement(&placement));
    }

    IFC(GetTarget(&spTarget));

    ASSERT(spTarget.Get(), L"pTarget expected to be non-null in ToolTip_Partial::PerformPlacement()");
    if (spTarget)
    {
        IFC(spTarget->get_FlowDirection(&targetFlowDirection));
        bIsRTL = targetFlowDirection == xaml::FlowDirection_RightToLeft;

        // We should not do placement if the target is no longer in the live tree.
        if (!spTarget.Cast<FrameworkElement>()->IsInLiveTree())
        {
            goto Cleanup;
        }
    }

    // Get the current position to find the nearest available monitor bounds
    if ((!m_isSliderThumbToolTip) &&
        (AutomaticToolTipInputMode::Touch == m_inputMode || AutomaticToolTipInputMode::Mouse == m_inputMode))
    {
        wf::Point lastPointerEnteredPoint = {};
        wrl::ComPtr<ixp::IPointerPoint> spCurrentPointerPoint;

        spCurrentPointerPoint = GetCurrentPointFromRootOrCoreWindow(spTarget);
        if (!spCurrentPointerPoint)
        {
            // We can hit a case where the most recent mouse event went to a hWnd which is now closed
            // (e.g. a different ToolTip that is now closed). See bug MSFT:2303578 for details.
            // We fall back to a point cached by ToolTipService whenever the CoreWindow/Island doesn't have a cached point.
            lastPointerEnteredPoint = ToolTipService::s_lastPointerEnteredPoint;
        }
        else
        {
            IFC(hr);
            IFC(spCurrentPointerPoint->get_Position(&lastPointerEnteredPoint));

            // Deliverable 21236263: Support windowed popups
            // When Xaml had windowed popups, we would adjust for the difference between the popup hwnd and the main tree's
            // hwnd here. That adjustment will be needed again once windowed popups are brought back.
        }

        currentPoint = lastPointerEnteredPoint;
    }
    else
    {
        wf::Point targetLeftTop = {};
        ctl::ComPtr<xaml_media::IGeneralTransform> spTransformToRoot;

        IFC(spTarget.Cast<FrameworkElement>()->TransformToVisual(NULL, &spTransformToRoot));
        IFC(spTransformToRoot->TransformPoint(targetLeftTop, &targetLeftTop));

        currentPoint = targetLeftTop;
    }

    // Get the available monitor bounds to render the Windowed Popup ToolTip
    IFC(DXamlCore::GetCurrent()->CalculateAvailableMonitorRect(this, currentPoint, &availableMonitorRect));

    // PlacementMode.Mouse only makes sense for automatic ToolTips opened by touch or mouse.
    if (placement == xaml_primitives::PlacementMode_Mouse &&
        (m_inputMode == AutomaticToolTipInputMode::Touch || m_inputMode == AutomaticToolTipInputMode::Mouse))
    {
        // Set the ToolTip with using Windowed Popup for the mouse placement
        IFC(PerformMousePlacementWithWindowedPopup(
                bIsRTL,
                availableMonitorRect,
                currentPoint,
                tooltipActualWidth,
                tooltipActualHeight,
                horizontalOffset,
                verticalOffset,
                placement));
    }
    else
    {
        // For ToolTips opened by keyboard focus, PlacementMode_Mouse doesn't make any sense.
        // Fall back to the default - PlacementMode_Top.
        if (placement == xaml_primitives::PlacementMode_Mouse)
        {
            placement = xaml_primitives::PlacementMode_Top;
        }

        // Sets the directional(Left, Top, Right and Bottom) placement with the Windowed Popup.
        IFC(PerformDirectionalPlacementWithWindowedPopup(
                spTarget.Get(),
                bIsRTL,
                availableMonitorRect,
                currentPoint,
                tooltipActualWidth,
                tooltipActualHeight,
                horizontalOffset,
                verticalOffset,
                placement,
                pTargetRect));
    }

Cleanup:
    RRETURN(hr);
}

// Sets the mouse placement with the Windowed Popup.
_Check_return_ HRESULT ToolTip::PerformMousePlacementWithWindowedPopup(
    _In_ bool bIsRTL,
    _In_ wf::Rect availableMonitorRect,
    _In_ wf::Point currentPoint,
    _In_ DOUBLE tooltipActualWidth,
    _In_ DOUBLE tooltipActualHeight,
    _In_ DOUBLE horizontalOffset,
    _In_ DOUBLE verticalOffset,
    _In_ xaml_primitives::PlacementMode placement)
{
    DOUBLE left = 0;
    DOUBLE top = 0;
    wf::Point pointRTL = {};
    wf::Rect toolTipRect = {};
    wf::Rect intersectionRect = {};
    ctl::ComPtr<IPopup> spPopup;

    IFC_RETURN(m_wrPopup.As(&spPopup));
    ASSERT(spPopup.Get(), L"popup from weak reference expected to be non-null");
    IFCPTR_RETURN(spPopup);

    ASSERT(m_bIsOpenAsAutomaticToolTip, L"m_bIsOpenAsAutomaticToolTip expected to be true for PlacementMode.Mouse");

    // Set the current point with the initial left/top
    left = currentPoint.X;
    top = currentPoint.Y;

    // If we are in RTL mode, then flip the X coordinate around so that it appears to be in LTR mode. That
    // means all of the LTR logic will still work.
    if (bIsRTL)
    {
        pointRTL = currentPoint;
        ConvertToRTLPoint(&pointRTL, availableMonitorRect.Width);
        left = pointRTL.X;
    }

    IFC_RETURN(MovePointToPointerToolTipShowPosition(left, top, placement));

    top += m_mousePlacementVerticalOffset + verticalOffset;
    left += horizontalOffset;

    // pessimistic check of top value - can be 0 only if TextBlock().FontSize == 0
    top = DoubleUtil::Max(TOOLTIP_TOLERANCE, top);

    const float minX = availableMonitorRect.X;
    const float minY = availableMonitorRect.Y;
    const float maxX = availableMonitorRect.X + availableMonitorRect.Width;
    const float maxY = availableMonitorRect.Y + availableMonitorRect.Height;

    toolTipRect.X = static_cast<FLOAT>(left);
    toolTipRect.Y = static_cast<FLOAT>(top);
    toolTipRect.Width = static_cast<FLOAT>(tooltipActualWidth);
    toolTipRect.Height = static_cast<FLOAT>(tooltipActualHeight);

    intersectionRect.X = availableMonitorRect.X;
    intersectionRect.Y = availableMonitorRect.Y;
    intersectionRect.Width = maxX;
    intersectionRect.Height = maxY;

    IFC_RETURN(RectUtil::Intersect(intersectionRect, toolTipRect));

    if ((DoubleUtil::Abs(intersectionRect.Width - toolTipRect.Width) < TOOLTIP_TOLERANCE) &&
        (DoubleUtil::Abs(intersectionRect.Height - toolTipRect.Height) < TOOLTIP_TOLERANCE))
    {
        // The placement algorithm operates in LTR mode (with transformed data if it
        // is really in RTL mode), so we also need to transform the X value it returns.
        if (bIsRTL)
        {
            pointRTL.X = static_cast<FLOAT>(left);
            ConvertToRTLPoint(&pointRTL, availableMonitorRect.Width);
            left = pointRTL.X;
        }

        // ToolTip is completely inside the Xaml window
        IFC_RETURN(spPopup->put_VerticalOffset(top));
        IFC_RETURN(spPopup->put_HorizontalOffset(left));
    }
    else
    {
        if (toolTipRect.Y + toolTipRect.Height > maxY)
        {
            // If the lower edge of the plug-in obscures the ToolTip,
            // it repositions itself to align with the upper edge of the bounding box of the mouse.
            top += maxY - (toolTipRect.Y + toolTipRect.Height) - TOOLTIP_TOLERANCE;
        }

        if (toolTipRect.Y < minY)
        {
            // If the upper edge of Plug-in obscures the ToolTip,
            // the control repositions itself to align with the upper edge.
            // align with the top of the plug-in
            top = minY;
        }

        if (toolTipRect.X + toolTipRect.Width > maxX)
        {
            // If the right edge obscures the ToolTip,
            // it opens in the opposite direction from the obscuring edge.
            left += maxX - (toolTipRect.X + toolTipRect.Width) - TOOLTIP_TOLERANCE;
        }

        if (toolTipRect.X < minX)
        {
            // If the left edge obscures the ToolTip,
            // it then aligns with the obscuring screen edge
            left = minX;
        }

        // if right/bottom doesn't fit into the plug-in bounds, clip the ToolTip
        {
            RECT clipCalculationsRect{ static_cast<LONG>(0.0), static_cast<LONG>(0.0), static_cast<LONG>(toolTipRect.Width), static_cast<LONG>(toolTipRect.Height) };
            IFC_RETURN(CalculateTooltipClip(&clipCalculationsRect, maxX, maxY));
        }

        // The placement algorithm operates in LTR mode (with transformed data if it
        // is really in RTL mode), so we also need to transform the X value it returns.
        if (bIsRTL)
        {
            pointRTL.X = static_cast<FLOAT>(left);
            ConvertToRTLPoint(&pointRTL, availableMonitorRect.Width);
            left = pointRTL.X;
        }

        // position the parent Popup
        IFC_RETURN(spPopup->put_VerticalOffset(top + verticalOffset));
        IFC_RETURN(spPopup->put_HorizontalOffset(left + horizontalOffset));
    }

    return S_OK;
}

// Convert the LTR point to RTL client point
void ToolTip::ConvertToRTLPoint(
    _Inout_ wf::Point* pDipPoint,
    _In_ FLOAT ScreenWidth)
{
    pDipPoint->X = ScreenWidth - pDipPoint->X;
}

// Sets the directional(Left, Top, Right and Bottom) placement with the Windowed Popup.
_Check_return_ HRESULT ToolTip::PerformDirectionalPlacementWithWindowedPopup(
    _In_ xaml::IFrameworkElement* pTarget,
    _In_ bool bIsRTL,
    _In_ wf::Rect availableMonitorRect,
    _In_ wf::Point currentPoint,
    _In_ DOUBLE tooltipActualWidth,
    _In_ DOUBLE tooltipActualHeight,
    _In_ DOUBLE horizontalOffset,
    _In_ DOUBLE verticalOffset,
    _In_ xaml_primitives::PlacementMode placement,
    _In_opt_ RECT* pTargetRect)
{
    ToolTipPositioning::CConstraint constraint;
    SIZE szFlyout = {};
    DOUBLE targetActualWidth = 0;
    DOUBLE targetActualHeight = 0;
    RECT rcDockTo = {};
    RECT rcResult = {};
    wf::Point popupLeftTop = {};
    BOOLEAN isPropertyLocal = FALSE;
    ctl::ComPtr<IPopup> spPopup;
    xaml_primitives::PlacementMode placementChosen = DefaultPlacementMode;

    IFC_RETURN(m_wrPopup.As(&spPopup));
    ASSERT(spPopup.Get(), L"popup from weak reference expected to be non-null");
    IFCPTR_RETURN(spPopup);

    if (!pTarget)
    {
        return S_OK;
    }

    // Set the constraint rect with the available monitor rect
    constraint.SetRect(
        static_cast<INT>(availableMonitorRect.X),
        static_cast<INT>(availableMonitorRect.Y),
        static_cast<INT>(availableMonitorRect.X + availableMonitorRect.Width),
        static_cast<INT>(availableMonitorRect.Y + availableMonitorRect.Height));

    szFlyout.cx = static_cast<LONG>(tooltipActualWidth);
    szFlyout.cy = static_cast<LONG>(tooltipActualHeight);

    IFC_RETURN(pTarget->get_ActualWidth(&targetActualWidth));
    IFC_RETURN(pTarget->get_ActualHeight(&targetActualHeight));

//    TODO:  RS1 bug 4775797:  These asserts are firing in Microsoft Edge but appear benign, commenting out for now.
//    ASSERT(DoubleUtil::GreaterThan(targetActualWidth, 0));
//    ASSERT(DoubleUtil::GreaterThan(targetActualHeight, 0));

    wf::Rect placementRect = {};
    IFC_RETURN(GetPlacementRectInWindowCoordinates(&placementRect));

    if (!m_isSliderThumbToolTip &&
        (AutomaticToolTipInputMode::Touch == m_inputMode || AutomaticToolTipInputMode::Mouse == m_inputMode))
    {
        IFC_RETURN(MovePointToPointerToolTipShowPosition(currentPoint, placement));

        rcDockTo.left = static_cast<LONG>(currentPoint.X);
        rcDockTo.top = static_cast<LONG>(currentPoint.Y);
        rcDockTo.right = static_cast<LONG>(currentPoint.X);
        rcDockTo.bottom = static_cast<LONG>(currentPoint.Y);

        // For touch, we need to account for the fact that the context menu hint has a vertical offset.
        if (AutomaticToolTipInputMode::Touch == m_inputMode)
        {
            rcDockTo.top += CONTEXT_MENU_HINT_VERTICAL_OFFSET;
            rcDockTo.bottom += CONTEXT_MENU_HINT_VERTICAL_OFFSET;
        }
    }
    else if (pTargetRect)
    {
        // Slider case - position ToolTip over Thumb rect
        // TextElement case - position ToolTip over text element if the input mode is not mouse or touch.
        rcDockTo = *pTargetRect;
    }
    else if (!RectUtil::GetIsEmpty(placementRect))
    {
        rcDockTo.left = static_cast<LONG>(placementRect.X);
        rcDockTo.top = static_cast<LONG>(placementRect.Y);
        rcDockTo.right = static_cast<LONG>(placementRect.X + placementRect.Width);
        rcDockTo.bottom = static_cast<LONG>(placementRect.Y + placementRect.Height);
    }
    else
    {
        // currentPoint.X should be the left edge of the target, so adjust for RTL by
        // subtracting the width of the target.
        if (bIsRTL)
        {
            currentPoint.X -= static_cast<FLOAT>(targetActualWidth);
        }

        rcDockTo.left = static_cast<LONG>(currentPoint.X);
        rcDockTo.top = static_cast<LONG>(currentPoint.Y);
        rcDockTo.right = static_cast<LONG>(currentPoint.X + targetActualWidth);
        rcDockTo.bottom = static_cast<LONG>(currentPoint.Y + targetActualHeight);
    }

    // If horizontal & vertical offset are not specified, use the system defaults.
    IFC_RETURN(IsPropertyLocal(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ToolTip_HorizontalOffset),
        &isPropertyLocal));

    if (!isPropertyLocal)
    {
        switch (m_inputMode)
        {
        case AutomaticToolTipInputMode::Keyboard:
            horizontalOffset = DEFAULT_KEYBOARD_OFFSET;
            break;
        case AutomaticToolTipInputMode::Mouse:
            horizontalOffset = DEFAULT_MOUSE_OFFSET;
            break;
        case AutomaticToolTipInputMode::Touch:
            horizontalOffset = DEFAULT_TOUCH_OFFSET;
            break;
        }
    }

    IFC_RETURN(IsPropertyLocal(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ToolTip_VerticalOffset),
        &isPropertyLocal));

    if (!isPropertyLocal)
    {
        switch (m_inputMode)
        {
        case AutomaticToolTipInputMode::Keyboard:
            verticalOffset = DEFAULT_KEYBOARD_OFFSET;
            break;
        case AutomaticToolTipInputMode::Mouse:
            verticalOffset = DEFAULT_MOUSE_OFFSET;
            break;
        case AutomaticToolTipInputMode::Touch:
            verticalOffset = DEFAULT_TOUCH_OFFSET;
            break;
        }
    }

    // Reverse Left/Right placement for RTL, because the target is flipped.
    if (bIsRTL)
    {
        if (placement == xaml_primitives::PlacementMode_Right)
        {
            placement = xaml_primitives::PlacementMode_Left;
        }
        else if (placement == xaml_primitives::PlacementMode_Left)
        {
            placement = xaml_primitives::PlacementMode_Right;
        }
    }

    // To honor horizontal/vertical offset, inflate the placement target by these offsets before calling into
    // the Windows popup positioning logic.
    InflateRect(&rcDockTo, static_cast<INT>(horizontalOffset), static_cast<INT>(verticalOffset));

    rcResult = ToolTipPositioning::QueryRelativePosition(
        constraint,
        szFlyout,
        rcDockTo,
        placement,
        ToolTipPositioning::POPUP_SNAPPING::PS_NONE,
        &placementChosen);

    // ToolTipPositioning::QueryRelativePosition is used to position in LTR and RTL. In LTR, the horizontal
    // offset is  in rcResult.left. In RTL, the horizontal offset is in rcResult.right because the
    // popup is flipped.
    popupLeftTop.X = static_cast<XFLOAT>((bIsRTL) ? rcResult.right : rcResult.left);
    popupLeftTop.Y = static_cast<XFLOAT>(rcResult.top);

    // Position tooltip by setting popup's offsets
    IFC_RETURN(spPopup->put_HorizontalOffset(popupLeftTop.X));
    IFC_RETURN(spPopup->put_VerticalOffset(popupLeftTop.Y));

    return S_OK;
}

_Check_return_ HRESULT ToolTip::MovePointToPointerToolTipShowPosition(
    _Inout_ wf::Point& point,
    xaml_primitives::PlacementMode placement)
{
    // If the point is inside the placement rect, move it out of the placement rect.
    wf::Rect placementRect = {};
    IFC_RETURN(GetPlacementRectInWindowCoordinates(&placementRect));

    if (!RectUtil::GetIsEmpty(placementRect) && RectUtil::Contains(placementRect, point))
    {
        switch (placement)
        {
        case xaml_primitives::PlacementMode_Left:
            point.X = placementRect.X;
            break;
        case xaml_primitives::PlacementMode_Right:
            point.X = placementRect.X + placementRect.Width;
            break;
        case xaml_primitives::PlacementMode_Top:
        case xaml_primitives::PlacementMode_Mouse:
            point.Y = placementRect.Y;
            break;
        case xaml_primitives::PlacementMode_Bottom:
            point.Y = placementRect.Y + placementRect.Height;
            break;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ToolTip::MovePointToPointerToolTipShowPosition(
    _Inout_ DOUBLE& left,
    _Inout_ DOUBLE& top,
    xaml_primitives::PlacementMode placement)
{
    wf::Point point = { static_cast<float>(left), static_cast<float>(top) };

    IFC_RETURN(MovePointToPointerToolTipShowPosition(point, placement));

    left = point.X;
    top = point.Y;

    return S_OK;
}

_Check_return_ HRESULT ToolTip::GetPlacementRectInWindowCoordinates(
    _Out_ wf::Rect* placementRect)
{
    wf::Rect placementRectLocal = RectUtil::CreateEmptyRect();

    ctl::ComPtr<wf::IReference<wf::Rect>> placementRectReference;
    IFCFAILFAST(get_PlacementRect(&placementRectReference));

    if (placementRectReference)
    {
        IFC_RETURN(placementRectReference->get_Value(&placementRectLocal));

        ctl::ComPtr<IFrameworkElement> target;
        IFC_RETURN(GetTarget(&target));

        if (target)
        {
            ctl::ComPtr<IGeneralTransform> transformToRoot;
            IFC_RETURN(target.Cast<FrameworkElement>()->TransformToVisual(nullptr, &transformToRoot));
            IFC_RETURN(transformToRoot->TransformBounds(placementRectLocal, &placementRectLocal));
        }
    }

    *placementRect = placementRectLocal;
    return S_OK;
}

// If the owner is a TextElement, we'll get its bounding rect and use that as our placement target rect.
// Otherwise, we'll use the default rect derived from the target.
_Check_return_ HRESULT ToolTip::PerformPlacementInternal()
{
    ctl::ComPtr<xaml::IDependencyObject> owner;
    ctl::ComPtr<xaml_docs::ITextElement> ownerAsTextElement;

    IFC_RETURN(m_wrOwner.As(&owner));
    ownerAsTextElement = owner.AsOrNull<xaml_docs::ITextElement>();

    if (ownerAsTextElement)
    {
        CCoreServices *pCore = static_cast<CCoreServices*>(DXamlCore::GetCurrent()->GetHandle());

        XRECTF boundingRect;
        IFC_RETURN(pCore->GetTextElementBoundingRect(ownerAsTextElement.Cast<TextElement>()->GetHandle(), &boundingRect));

        RECT rectDockTo =
        {
            static_cast<long>(boundingRect.X),
            static_cast<long>(boundingRect.Y),
            static_cast<long>(boundingRect.X + boundingRect.Width),
            static_cast<long>(boundingRect.Y + boundingRect.Height)
        };
        IFC_RETURN(PerformPlacement(&rectDockTo));
    }
    else
    {
        IFC_RETURN(PerformPlacement());
    }

    return S_OK;
}

// Handle the SizeChanged event.
_Check_return_ HRESULT ToolTip::OnToolTipSizeChanged(
    _In_ IInspectable* pSender,
    _In_ ISizeChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(PerformPlacementInternal());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ToolTip::OnPopupOpened(
    _In_opt_ IInspectable* pUnused1,
    _In_opt_ IInspectable* pUnused2)
{
    HRESULT hr = S_OK;
    BOOLEAN bAutomationListener = FALSE;

    IFC(OnOpened());

    IFC(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_ToolTipOpened, &bAutomationListener));
    if (bAutomationListener)
    {
        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;

        IFC(GetOrCreateAutomationPeer(&spAutomationPeer));
        if(spAutomationPeer)
        {
            IFC(spAutomationPeer->RaiseAutomationEvent(xaml_automation_peers::AutomationEvents_ToolTipOpened));
        }
    }

    // If we've recently closed and reopened this ToolTip, then don't do anything until we receive the final Popup.Opened
    // event. See comment for m_pendingPopupOpenEventCount in header for details.
    m_pendingPopupOpenEventCount--;
    if (m_pendingPopupOpenEventCount == 0 && m_bCallPerformPlacementAtNextPopupOpen)
    {
        m_bCallPerformPlacementAtNextPopupOpen = FALSE;
        IFC(PerformPlacementInternal());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ToolTip::OnPopupClosed(
    _In_opt_ IInspectable* pUnused1,
    _In_opt_ IInspectable* pUnused2)
{
    HRESULT hr = S_OK;
    BOOLEAN bAutomationListener = FALSE;

    IFC(OnClosed());

    IFC(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_ToolTipClosed, &bAutomationListener));
    if (bAutomationListener)
    {
        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;
        IFC(GetOrCreateAutomationPeer(&spAutomationPeer));
        if(spAutomationPeer)
        {
            IFC(spAutomationPeer->RaiseAutomationEvent(xaml_automation_peers::AutomationEvents_ToolTipClosed));
        }
    }

    m_bIsPopupPositioned = FALSE;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ToolTip::OnOpened()
{
    HRESULT hr = S_OK;
    OpenedEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<RoutedEventArgs> spArgs;

    // Create the args
    IFC(ctl::make<RoutedEventArgs>(&spArgs));
    IFC(spArgs->put_OriginalSource(ctl::as_iinspectable(this)));

    // Raise the event
    IFC(GetOpenedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), spArgs.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ToolTip::OnClosed()
{
    HRESULT hr = S_OK;
    ClosedEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<RoutedEventArgs> spArgs;

    // Create the args
    IFC(ctl::make<RoutedEventArgs>(&spArgs));
    IFC(spArgs->put_OriginalSource(ctl::as_iinspectable(this)));

    // Raise the event
    IFC(GetClosedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), spArgs.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ToolTip::PerformClipping(
    _In_ wf::Size size)
{
    HRESULT hr = S_OK;
    INT childCount = 0;
    DOUBLE childActualWidth = 0.0;
    DOUBLE childActualHeight = 0.0;

    // By default a tooltip has only 1 child (border).

    IFC(VisualTreeHelper::GetChildrenCountStatic(this, &childCount));
    if (childCount)
    {
        ctl::ComPtr<IDependencyObject> spChildAsDO;
        ctl::ComPtr<IFrameworkElement> spChildAsFE;

        IFC(VisualTreeHelper::GetChildStatic(this, 0, &spChildAsDO));
        spChildAsFE = spChildAsDO.AsOrNull<IFrameworkElement>();
        if (spChildAsFE)
        {
            IFC(spChildAsFE->get_ActualWidth(&childActualWidth));
            if (size.Width < childActualWidth)
            {
                IFC(spChildAsFE->put_Width(size.Width));
            }

            IFC(spChildAsFE->get_ActualHeight(&childActualHeight));
            if (size.Height < childActualHeight)
            {
                IFC(spChildAsFE->put_Height(size.Height));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ToolTip::OnRootVisualSizeChanged()
{
    HRESULT hr = S_OK;

    IFC(PerformPlacementInternal());

Cleanup:
    RRETURN(hr);
}

// If we are in the process of animating to the Closed state, then closes the ToolTip's Popup.
// Else, does nothing.
_Check_return_ HRESULT ToolTip::ForceFinishClosing(
    _In_opt_ IInspectable* pUnused1,
    _In_opt_ IInspectable* pUnused2)
{
    HRESULT hr = S_OK;

    // Avoid closing the current Popup if it's already been done i.e. by another ToolTip trying to open.
    if (m_bClosing)
    {
        IFC(Close());
        m_bClosing = FALSE;
    }

Cleanup:
    RRETURN(hr);
}

// Called when the IsEnabled property changes.
_Check_return_ HRESULT ToolTip::OnIsEnabledChanged(_In_ IsEnabledChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsOpen = FALSE;
    BOOLEAN bIsEnabled = FALSE;

    IFC(get_IsOpen(&bIsOpen));
    if (bIsOpen)
    {
        ctl::ComPtr<IPopup> spPopup;

        IFC(m_wrPopup.As(&spPopup));
        ASSERT(spPopup.Get(), L"popup from weak reference expected to be non-null");
        IFCPTR(spPopup);

        IFC(get_IsEnabled(&bIsEnabled));
        if (bIsEnabled)
        {
            IFC(PerformPlacementInternal());
        }

        // Make the ToolTip visible if IsEnabled=True, or hidden otherwise.
        IFC(spPopup.Cast<Popup>()->put_Opacity(bIsEnabled ? 1 : 0));
    }

Cleanup:
    RRETURN(hr);
}

// Change to the correct visual state for the ToolTip.
_Check_return_ HRESULT ToolTip::ChangeVisualState(
    // true to use transitions when updating the visual state, false
    // to snap directly to the new visual state.
    _In_ bool bUseTransitions)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsOpen = FALSE;
    BOOLEAN bWentToState = FALSE;

    IFC(get_IsOpen(&bIsOpen));

    if (bIsOpen && m_bIsPopupPositioned)
    {
        IFC(GoToState(bUseTransitions, L"Opened", &bWentToState));
    }
    else
    {
        bool alreadyInClosedState = false;
        ctl::ComPtr<IVisualStateGroup> spVisualStateGroup;
        ctl::ComPtr<IVisualState> spClosedVisualState;
        ctl::ComPtr<IVisualState> spCurrentVisualState;
        BOOLEAN foundState = FALSE;

        IFC(VisualStateManager::TryGetState(this, L"Closed", &spVisualStateGroup, &spClosedVisualState, &foundState));

        if (foundState && spVisualStateGroup && spClosedVisualState)
        {
            IFC(spVisualStateGroup->get_CurrentState(&spCurrentVisualState));
            alreadyInClosedState = spCurrentVisualState == spClosedVisualState;
        }

        m_bClosing = TRUE;

        IFC(GoToState(bUseTransitions, L"Closed", &bWentToState));

        if (!bWentToState || alreadyInClosedState)
        {
            // We could not go to the "Closed" state. This could happen if there is no Closed state or if we were already in
            // the Closed state.
            // Ordinarily, the Storyboard Completed event will trigger the call to Close(), but if this is not going to happen,
            // we need to call it directly from here.
            m_bClosing = FALSE;
            IFC(Close());
        }
    }

Cleanup:
    RRETURN(hr);
}

// Removes the "automatic" flag and clears associated fields.
//
// For Slider, the Thumb ToolTip may be opened as an automatic ToolTip by pointer hover.  However, if
// we click on the Thumb and start to drag, we don't want the ToolTip to disappear after several seconds.
// Thus, we remove the automatic flag and keep the ToolTip open for Slider to handle.
_Check_return_ HRESULT ToolTip::RemoveAutomaticStatusFromOpenToolTip()
{
    HRESULT hr = S_OK;
    BOOLEAN isOpen = FALSE;

    IFC(get_IsOpen(&isOpen));

    if (isOpen && m_bIsOpenAsAutomaticToolTip)
    {
        ToolTipServiceMetadata* pToolTipServiceMetadata = NULL;

        IFC(DXamlCore::GetCurrent()->GetToolTipServiceMetadata(pToolTipServiceMetadata));

        if (pToolTipServiceMetadata->m_tpCloseTimer)
        {
            IFC(pToolTipServiceMetadata->m_tpCloseTimer->Stop());
        }

        if (pToolTipServiceMetadata->m_tpSafeZoneCheckTimer)
        {
            IFC(pToolTipServiceMetadata->m_tpSafeZoneCheckTimer->Stop());
        }

        ASSERT(pToolTipServiceMetadata->m_tpCurrentPopup != nullptr);
        ASSERT(pToolTipServiceMetadata->m_tpCurrentToolTip.Get() != nullptr);
        ASSERT(static_cast<ToolTip*>(pToolTipServiceMetadata->m_tpCurrentToolTip.Get()) == this);

        m_bIsOpenAsAutomaticToolTip = FALSE;

        pToolTipServiceMetadata->m_tpCurrentToolTip.Clear();
        if (pToolTipServiceMetadata->m_tpContainer)
        {
            IFC(static_cast<FrameworkElement*>(pToolTipServiceMetadata->m_tpContainer.Get())->SetHasOpenToolTip(FALSE));
            pToolTipServiceMetadata->m_tpContainer.Clear();
        }
        pToolTipServiceMetadata->m_tpLastEnterSource.Clear();
        pToolTipServiceMetadata->m_tpCurrentPopup.Clear();
    }

Cleanup:
    RRETURN(hr);
}
_Check_return_ HRESULT ToolTip::RepositionPopup()
{
    IFC_RETURN(PerformPlacementInternal());
    return S_OK;
}

_Check_return_ HRESULT
ToolTip::HandlePointInSafeZone(const POINT& position)
{
    if (auto visualTree = VisualTree::GetForElementNoRef(GetHandle()))
    {
        // Start from GetCursorPos point, which need to be converted logical point
        auto logicalPoint = visualTree->ScreenPhysicalToClientLogical(position);
        IFC_RETURN(HandlePointInSafeZone(logicalPoint));
    }
    return S_OK;
}

_Check_return_ HRESULT
ToolTip::HandlePointInSafeZone(const wf::Point& point)
{
    ctl::ComPtr<xaml::IDependencyObject> owner;
    IFC_RETURN(m_wrOwner.As(&owner));
    if (owner)
    {
        IFC_RETURN(ToolTipService::HandleToolTipSafeZone(point, this, owner));
    }
    return S_OK;
}

bool
ToolTip::IsOwnerPositionChanged()
{
    ctl::ComPtr<xaml::IDependencyObject> owner;
    if (SUCCEEDED(m_wrOwner.As(&owner)))
    {
        XRECTF_RB ownerBounds = {};
        if (SUCCEEDED(ToolTipService::GetToolTipOwnersBoundary(owner, &ownerBounds)))
        {
            if ((fabs(ownerBounds.bottom - m_ownerBounds.bottom) > 0.5) ||
                (fabs(ownerBounds.top - m_ownerBounds.top) > 0.5) ||
                (fabs(ownerBounds.left - m_ownerBounds.left) > 0.5) ||
                (fabs(ownerBounds.right - m_ownerBounds.right) > 0.5))
            {
                return true;
            }
        }
    }
    return false;
}

bool
ToolTip::IsControlKeyOnly(
    _In_ wsy::VirtualKey key)
{
    wsy::VirtualKeyModifiers modifiers = wsy::VirtualKeyModifiers_None;
    if (key == wsy::VirtualKey_Control && SUCCEEDED(CoreImports::Input_GetKeyboardModifiers(&modifiers)))
    {
        return (!IsFlagSet(modifiers, wsy::VirtualKeyModifiers_Menu) &&
                !IsFlagSet(modifiers, wsy::VirtualKeyModifiers_Shift) &&
                !IsFlagSet(modifiers, wsy::VirtualKeyModifiers_Windows));
    }

    return false;
}

_Check_return_ HRESULT ToolTip::UpdateOwnersBoundary()
{
    ctl::ComPtr<xaml::IDependencyObject> owner;
    IFC_RETURN(m_wrOwner.As(&owner));
    IFC_RETURN(ToolTipService::GetToolTipOwnersBoundary(owner, &m_ownerBounds));
    return S_OK;
}

// Hooks up the CoreWindow's or XamlIslandRoot's PointerMoved event so the ToolTip can be automatically
// closed if it's out of safe zone.
_Check_return_ HRESULT ToolTip::HookupXamlIslandRoot()
{
    ctl::ComPtr<UIElement> islandRootElement;
    IFC_RETURN(GetXamlIslandRootElement(&islandRootElement));

    if (islandRootElement)
    {
        IFC_RETURN(AddXamlIslandRootHandler(islandRootElement.Get()));
    }
    return S_OK;
}

_Check_return_ HRESULT ToolTip::UnhookFromXamlIslandRoot()
{
    IFC_RETURN(RemoveXamlIslandRootHandler());
    return S_OK;
}

_Check_return_ HRESULT ToolTip::HookupOwnerLayoutChangedEvent()
{
    ctl::ComPtr<xaml::IDependencyObject> owner;
    if (SUCCEEDED(m_wrOwner.As(&owner)))
    {
        if (auto ownerAsFE = owner.AsOrNull<xaml::IFrameworkElement>())
        {
            ctl::WeakRefPtr wrThis;
            IFC_RETURN(ctl::AsWeak(this, &wrThis));

            IFC_RETURN(ownerAsFE->add_LayoutUpdated(
                wrl::Callback<wf::IEventHandler<IInspectable*>>(
                    [wrThis](IInspectable* /*sender*/, IInspectable* /*args*/) mutable -> HRESULT
                    {
                        ctl::ComPtr<ToolTip> toolTip;
                        IFC_RETURN(wrThis.As<ToolTip>(&toolTip));

                        if (toolTip)
                        {
                            BOOLEAN hasParent;
                            IFC_RETURN(toolTip->HasParent(&hasParent));
                            if (hasParent && toolTip->IsOwnerPositionChanged())
                            {
                                ToolTipServiceMetadata* pToolTipServiceMetadataNoRef = nullptr;
                                IFC_RETURN(DXamlCore::GetCurrent()->GetToolTipServiceMetadata(pToolTipServiceMetadataNoRef));

                                auto current = pToolTipServiceMetadataNoRef->m_tpCurrentToolTip.Get();
                                if (current && ctl::are_equal(ctl::iinspectable_cast(current), ctl::iinspectable_cast(toolTip.Get())))
                                {
                                    ToolTipService::CancelAutomaticToolTip();
                                }
                            }
                        }
                        return S_OK;
                    }).Get(),
                &m_ownerLayoutUpdatedToken));
        }
    }
    return S_OK;
}

_Check_return_ HRESULT ToolTip::UnhookOwnerLayoutChangedEvent()
{
    if (m_ownerLayoutUpdatedToken.value)
    {
        ctl::ComPtr<xaml::IDependencyObject> owner;
        if (SUCCEEDED(m_wrOwner.As(&owner)))
        {
            if (auto ownerAsFE = owner.AsOrNull<xaml::IFrameworkElement>())
            {
                IFC_RETURN(ownerAsFE->remove_LayoutUpdated(m_ownerLayoutUpdatedToken));
                m_ownerLayoutUpdatedToken.value = 0;
            }
        }
    }
    return S_OK;
}

_Check_return_ HRESULT ToolTip::AddXamlIslandRootHandler(UIElement* rootElement)
{
    if (!m_xamlIslandRootPointerMovedHandler)
    {
        ctl::WeakRefPtr wrThis;
        IFC_RETURN(ctl::AsWeak(this, &wrThis));

        m_xamlIslandRootPointerMovedHandler.Attach(wrl::Callback<xaml_input::IPointerEventHandler>(
            [wrThis](IInspectable*, xaml_input::IPointerRoutedEventArgs* args) mutable -> HRESULT
            {
                ctl::ComPtr<ToolTip> toolTip;
                IFC_RETURN(wrThis.As<ToolTip>(&toolTip));

                if (toolTip)
                {
                    ctl::ComPtr<ixp::IPointerPoint> pointerPoint;

                    IFC_RETURN(args->GetCurrentPoint(nullptr, &pointerPoint));
                    IFCPTR_RETURN(pointerPoint);

                    wf::Point cursor = {};
                    IFC_RETURN(pointerPoint->get_Position(&cursor));

                    IFC_RETURN(toolTip->HandlePointInSafeZone(cursor));
                }
                return S_OK;
            }
        ).Detach());

        PointerMovedEventSourceType* pointerMovedEventSource = nullptr;
        IFC_RETURN(rootElement->GetPointerMovedEventSourceNoRef(&pointerMovedEventSource));
        IFC_RETURN(pointerMovedEventSource->AddHandler(m_xamlIslandRootPointerMovedHandler.Get(), TRUE /* handledEventsToo */));
    }

    if (!m_xamlIslandRootKeyDownHandler)
    {
        ctl::WeakRefPtr wrThis;
        IFC_RETURN(ctl::AsWeak(this, &wrThis));

        m_xamlIslandRootKeyDownHandler.Attach(wrl::Callback<xaml_input::IKeyEventHandler>(
            [wrThis](IInspectable*, xaml_input::IKeyRoutedEventArgs* args) mutable -> HRESULT
            {
                ctl::ComPtr<ToolTip> toolTip;
                IFC_RETURN(wrThis.As<ToolTip>(&toolTip));

                if (toolTip)
                {
                    wsy::VirtualKey key = wsy::VirtualKey_None;
                    IFC_RETURN(args->get_Key(&key));
                    toolTip->m_lastKeyDownIsControlOnly = IsControlKeyOnly(key);
                }
                return S_OK;
            }
        ).Detach());

        KeyDownEventSourceType* keyDownEventSource = nullptr;
        IFC_RETURN(rootElement->GetKeyDownEventSourceNoRef(&keyDownEventSource));
        IFC_RETURN(keyDownEventSource->AddHandler(m_xamlIslandRootKeyDownHandler.Get(), TRUE /* handledEventsToo */));
    }

    if (!m_xamlIslandRootKeyUpHandler)
    {
        ctl::WeakRefPtr wrThis;
        IFC_RETURN(ctl::AsWeak(this, &wrThis));

        m_xamlIslandRootKeyUpHandler.Attach(wrl::Callback<xaml_input::IKeyEventHandler>(
            [wrThis](IInspectable*, xaml_input::IKeyRoutedEventArgs* args) mutable -> HRESULT
            {
                ctl::ComPtr<ToolTip> toolTip;
                IFC_RETURN(wrThis.As<ToolTip>(&toolTip));

                if (toolTip)
                {
                    wsy::VirtualKey key = wsy::VirtualKey_None;
                    IFC_RETURN(args->get_Key(&key));
                    if (toolTip->m_lastKeyDownIsControlOnly && IsControlKeyOnly(key))
                    {
                        IGNOREHR(ToolTipService::CloseToolTipInternal(nullptr));
                    }
                }
                return S_OK;
            }
        ).Detach());

        KeyUpEventSourceType* keyUpEventSource = nullptr;
        IFC_RETURN(rootElement->GetKeyUpEventSourceNoRef(&keyUpEventSource));
        IFC_RETURN(keyUpEventSource->AddHandler(m_xamlIslandRootKeyUpHandler.Get(), TRUE /* handledEventsToo */));
    }

    return S_OK;
}

// Unhooks the CoreWindow's PointerMoved event.
_Check_return_ HRESULT ToolTip::RemoveXamlIslandRootHandler()
{
    if (m_xamlIslandRootKeyUpHandler || m_xamlIslandRootKeyDownHandler || m_xamlIslandRootPointerMovedHandler)
    {
        ctl::ComPtr<UIElement> rootElement;
        IFC_RETURN(GetXamlIslandRootElement(&rootElement));

        if (rootElement)
        {
            if (m_xamlIslandRootPointerMovedHandler)
            {
                PointerMovedEventSourceType* pointerMovedEventSource = nullptr;
                IFC_RETURN(rootElement->GetPointerMovedEventSourceNoRef(&pointerMovedEventSource));
                IFC_RETURN(pointerMovedEventSource->RemoveHandler(m_xamlIslandRootPointerMovedHandler.Get()));
                m_xamlIslandRootPointerMovedHandler = nullptr;
            }
            if (m_xamlIslandRootKeyDownHandler)
            {
                KeyDownEventSourceType* keyDownEventSource = nullptr;
                IFC_RETURN(rootElement->GetKeyDownEventSourceNoRef(&keyDownEventSource));
                IFC_RETURN(keyDownEventSource->RemoveHandler(m_xamlIslandRootKeyDownHandler.Get()));
                m_xamlIslandRootKeyDownHandler = nullptr;
            }
            if (m_xamlIslandRootKeyUpHandler)
            {
                KeyUpEventSourceType* keyUpEventSource = nullptr;
                IFC_RETURN(rootElement->GetKeyUpEventSourceNoRef(&keyUpEventSource));
                IFC_RETURN(keyUpEventSource->RemoveHandler(m_xamlIslandRootKeyUpHandler.Get()));
                m_xamlIslandRootKeyUpHandler = nullptr;
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ToolTip::GetXamlIslandRootElement(_Outptr_ UIElement** rootElement)
{
    *rootElement = nullptr;

    ctl::ComPtr<xaml::IDependencyObject> owner;
    IFC_RETURN(m_wrOwner.As(&owner));

    if (owner)
    {
        CContentRoot* contentRoot = VisualTree::GetContentRootForElement(static_cast<DependencyObject*>(owner.Get())->GetHandle());
        if (contentRoot && contentRoot->GetXamlIslandRootNoRef())
        {
            auto rootNoRef = contentRoot->GetVisualTreeNoRef()->GetRootElementNoRef();
            if (rootNoRef)
            {
                ctl::ComPtr<DependencyObject> peer;
                IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(rootNoRef, &peer));

                ctl::ComPtr<UIElement> peerAsUIE = peer.AsOrNull<UIElement>();

                *rootElement = peerAsUIE.Detach();
            }
        }
    }
    return S_OK;
}

_Check_return_ HRESULT ToolTip::ForwardOwnerThemePropertyToToolTip()
{
    const CDependencyProperty* requestedThemeProperty = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::FrameworkElement_RequestedTheme);

    // We'll only override the requested theme on the ToolTip if its value hasn't been explicitly set.
    // Otherwise, we'll abide by its existing value.
    if (GetHandle()->IsPropertyDefault(requestedThemeProperty) ||
        m_isToolTipRequestedThemeOverridden)
    {
        xaml::ElementTheme currentToolTipTheme;
        xaml::ElementTheme requestedTheme = xaml::ElementTheme_Default;
        ctl::ComPtr<IDependencyObject> spCurrent;
        ctl::ComPtr<IDependencyObject> spParent;
        ctl::ComPtr<IFrameworkElement> spCurrentAsFE;

        IFC_RETURN(get_RequestedTheme(&currentToolTipTheme));

        // Walk up the tree from the placement target until we find an element with a RequestedTheme.
        IFC_RETURN(m_wrOwner.As(&spCurrent));
        while (spCurrent)
        {
            if (auto spCurrentAsFE2 = spCurrent.AsOrNull<xaml::IFrameworkElement>())
            {
                IFC_RETURN(spCurrent.CopyTo(spCurrentAsFE.ReleaseAndGetAddressOf()));

                IFC_RETURN(spCurrentAsFE2->get_RequestedTheme(&requestedTheme));
                if (requestedTheme != xaml::ElementTheme_Default)
                {
                    break;
                }
            }
            else if (auto textElement = spCurrent.AsOrNull<TextElement>())
            {
                auto parent = static_cast<CTextElement*>(textElement->GetHandle())->GetContainingFrameworkElement();
                if (parent)
                {
                    ctl::ComPtr<DependencyObject> containingDO;
                    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(parent, &containingDO));
                    spCurrent = containingDO;
                    continue;
                }
                else
                {
                    return S_OK;
                }
            }
            else
            {
                return S_OK;
            }

            IFC_RETURN(VisualTreeHelper::GetParentStatic(spCurrent.Get(), spParent.ReleaseAndGetAddressOf()));
            if (spParent.AsOrNull<DirectUI::PopupRoot>())
            {
                // If the target is in a Popup and the Popup is in the Visual Tree, we want to inherit the theme
                // from that Popup's parent. Otherwise we will get the App's theme, which might not be what
                // is expected.
                if (spCurrentAsFE)
                {
                    IFC_RETURN(spCurrentAsFE->get_Parent(&spParent));
                }
            }

            spCurrent = spParent;
        }

        if (requestedTheme != currentToolTipTheme)
        {
            IFC_RETURN(put_RequestedTheme(requestedTheme));
            m_isToolTipRequestedThemeOverridden = true;
        }
    }

    return S_OK;
}
