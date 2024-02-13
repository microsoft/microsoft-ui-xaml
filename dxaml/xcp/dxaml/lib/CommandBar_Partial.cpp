// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CommandBar.g.h"
#include "ItemCollection.g.h"
#include "AppBarButton.g.h"
#include "AppBarToggleButton.g.h"
#include "ButtonBase.g.h"
#include "CommandBarElementCollection.g.h"
#include "CommandBarOverflowPresenter.g.h"
#include "CommandBarTemplateSettings.g.h"
#include "ItemsControl.g.h"
#include "ItemsPresenter.g.h"
#include "Page.g.h"
#include "Window.g.h"
#include "KeyRoutedEventArgs.g.h"
#include "RoutedEventArgs.h"
#include "InputPointEventArgs.h"
#include "InputServices.h"
#include "ResourceDictionary.g.h"
#include "DoubleUtil.h"
#include "DynamicOverflowItemsChangingEventArgs.g.h"
#include "XamlRoot.g.h"
#include <DeferredElementStateChange.h>
#include "XboxUtility.h"
#include "Callback.h"
#include "AppBarButtonHelpers.h"
#include "ApplicationBarService.g.h"
#include "ElevationHelper.h"
#include "focusmgr.h"
#include <PhoneImports.h>
#include "ThemeShadow.h"
#include "AutomationProperties.h"
#include "AccessKeyStringBuilder.h"
#include "ElementSoundPlayerService_Partial.h"
#include "CompositeTransform.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace std::placeholders;

CommandBar::~CommandBar()
{
    if (m_primaryCommandsChangedEventHandler)
    {
        auto primaryCommands = m_tpPrimaryCommands.GetSafeReference();
        if (primaryCommands)
        {
            VERIFYHR(m_primaryCommandsChangedEventHandler.DetachEventHandler(ctl::iinspectable_cast(primaryCommands.Get())));
        }
    }

    if (m_secondaryCommandsChangedEventHandler)
    {
        auto secondaryCommands = m_tpSecondaryCommands.GetSafeReference();
        if (secondaryCommands)
        {
            VERIFYHR(m_secondaryCommandsChangedEventHandler.DetachEventHandler(ctl::iinspectable_cast(secondaryCommands.Get())));
        }
    }

    if (m_secondaryItemsControlLoadedEventHandler)
    {
        auto secondaryItemsControl = m_tpSecondaryItemsControlPart.GetSafeReference();
        if (secondaryItemsControl)
        {
            VERIFYHR(m_secondaryItemsControlLoadedEventHandler.DetachEventHandler(ctl::iinspectable_cast(secondaryItemsControl.Get())));
        }
    }
}

_Check_return_ HRESULT
CommandBar::PrepareState()
{
    IFC_RETURN(__super::PrepareState());

    ctl::ComPtr<CommandBarElementCollection> spCollection;

    // Create our primary & secondary command collections
    IFC_RETURN(ctl::make(&spCollection));
    spCollection->Init(false /*notifyCollectionChanging*/);
    IFC_RETURN(CoreImports::Collection_SetOwner(static_cast<CCollection*>(spCollection->GetHandle()), this->GetHandle()));
    SetPtrValue(m_tpPrimaryCommands, spCollection.Get());

    IFC_RETURN(ctl::make(&spCollection));
    spCollection->Init(true /*notifyCollectionChanging*/);
    IFC_RETURN(CoreImports::Collection_SetOwner(static_cast<CCollection*>(spCollection->GetHandle()), this->GetHandle()));
    SetPtrValue(m_tpSecondaryCommands, spCollection.Get());

    // Set the value for our collection properties so that they are in the
    // effective value map and get processed during EnterImpl.
    IFC_RETURN(SetValueInternal(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::CommandBar_PrimaryCommands),
        ctl::as_iinspectable(m_tpPrimaryCommands.Get()), TRUE /*fAllowReadOnly*/));

    IFC_RETURN(SetValueInternal(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::CommandBar_SecondaryCommands),
        ctl::as_iinspectable(m_tpSecondaryCommands.Get()), TRUE /*fAllowReadOnly*/));

    IFC_RETURN(m_unloadedEventHandler.AttachEventHandler(this, std::bind(&CommandBar::OnUnloaded, this, _1, _2)));
    IFC_RETURN(m_primaryCommandsChangedEventHandler.AttachEventHandler(m_tpPrimaryCommands.Get(), std::bind(&CommandBar::OnPrimaryCommandsChanged, this, _1, _2)));
    IFC_RETURN(m_secondaryCommandsChangedEventHandler.AttachEventHandler(m_tpSecondaryCommands.Get(), std::bind(&CommandBar::OnSecondaryCommandsChanged, this, _1, _2)));

    ctl::ComPtr<TrackerCollection<xaml_controls::ICommandBarElement*>> primaryCommandsInPreviousTransition;
    ctl::ComPtr<TrackerCollection<xaml_controls::ICommandBarElement*>> primaryCommandsInTransition;
    ctl::ComPtr<AppBarSeparator> appBarSeparatorInOverflow;

    // Create the dynamic primary and secondary collections
    IFC_RETURN(ctl::make(&spCollection));
    spCollection->Init(false /*notifyCollectionChanging*/);
    IFC_RETURN(CoreImports::Collection_SetOwner(static_cast<CCollection*>(spCollection->GetHandle()), this->GetHandle()));
    SetPtrValue(m_tpDynamicPrimaryCommands, spCollection.Get());

    IFC_RETURN(ctl::make(&spCollection));
    spCollection->Init(false /*notifyCollectionChanging*/);
    IFC_RETURN(CoreImports::Collection_SetOwner(static_cast<CCollection*>(spCollection->GetHandle()), this->GetHandle()));
    SetPtrValue(m_tpDynamicSecondaryCommands, spCollection.Get());

    // Create the previous and current transition collections
    IFC_RETURN(ctl::make(&primaryCommandsInPreviousTransition));
    SetPtrValue(m_tpPrimaryCommandsInPreviousTransition, std::move(primaryCommandsInPreviousTransition));

    IFC_RETURN(ctl::make(&primaryCommandsInTransition));
    SetPtrValue(m_tpPrimaryCommandsInTransition, std::move(primaryCommandsInTransition));

    // Create the AppBarSeparator in the overflow when the primary commands is moved into the overflow
    IFC_RETURN(ctl::make(&appBarSeparatorInOverflow));
    SetPtrValue(m_tpAppBarSeparatorInOverflow, appBarSeparatorInOverflow);

    ctl::ComPtr<CommandBarTemplateSettings> templateSettings;
    IFC_RETURN(ctl::make(&templateSettings));
    IFC_RETURN(put_CommandBarTemplateSettings(templateSettings.Get()));

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::DisconnectFrameworkPeerCore()
{
    //
    // DXAML Structure
    // ---------------
    //   DXAML::CommandBar (this)   -------------->   DXAML::CommandBarElement
    //   |  |                           |
    //   |  |                           +--------->   DXAML::CommandBarElement
    //   |  V
    //   |  DXAML::CommandBarElementCollection (m_tpPrimaryCommands)
    //   V
    //   DXAML::CommandBarElementCollection (m_tpSecondaryCommands)
    //
    // CORE Structure
    // --------------
    //   Core::CControl (this)              < - - +         < - - +
    //            |                               :               :
    //            V                               :               :
    //   Core::CCommandBarElementCollection   - - + (m_pOwner)    :
    //      |                  |                                  :
    //      V                  V                                  :
    //   Core::CControl   Core::CControl      - - - - - - - - - - + (m_pParent)
    //
    // To clear the m_pParent association of the CommandBarElement, we have to clear the
    // Core::CCommandBarElementCollection's children, which calls SetParent(NULL) on each of its
    // children. Once this association to CommandBar is broken, we can safely destroy CommandBar.
    //

    // clear the children in the m_tpPrimaryCommands
    if (m_tpPrimaryCommands.GetAsCoreDO() != nullptr)
    {
        IFC_RETURN(CoreImports::Collection_Clear(static_cast<CCollection*>(m_tpPrimaryCommands.GetAsCoreDO())));
        IFC_RETURN(CoreImports::Collection_SetOwner(static_cast<CCollection*>(m_tpPrimaryCommands.GetAsCoreDO()), nullptr));
    }

    // clear the children in the m_tpSecondaryCommands
    if (m_tpSecondaryCommands.GetAsCoreDO() != nullptr)
    {
        IFC_RETURN(CoreImports::Collection_Clear(static_cast<CCollection*>(m_tpSecondaryCommands.GetAsCoreDO())));
        IFC_RETURN(CoreImports::Collection_SetOwner(static_cast<CCollection*>(m_tpSecondaryCommands.GetAsCoreDO()), nullptr));
    }

    // Clear the children in the m_tpDynamicPrimaryCommands
    if (m_tpDynamicPrimaryCommands.GetAsCoreDO() != nullptr)
    {
        IFC_RETURN(CoreImports::Collection_Clear(static_cast<CCollection*>(m_tpDynamicPrimaryCommands.GetAsCoreDO())));
        IFC_RETURN(CoreImports::Collection_SetOwner(static_cast<CCollection*>(m_tpDynamicPrimaryCommands.GetAsCoreDO()), nullptr));
    }

    // Clear the children in the m_tpDynamicSecondaryCommands
    if (m_tpDynamicSecondaryCommands.GetAsCoreDO() != nullptr)
    {
        IFC_RETURN(CoreImports::Collection_Clear(static_cast<CCollection*>(m_tpDynamicSecondaryCommands.GetAsCoreDO())));
        IFC_RETURN(CoreImports::Collection_SetOwner(static_cast<CCollection*>(m_tpDynamicSecondaryCommands.GetAsCoreDO()), nullptr));
    }

    IFC_RETURN(__super::DisconnectFrameworkPeerCore());

    return S_OK;
}

_Check_return_
HRESULT
CommandBar::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(__super::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::CommandBar_DefaultLabelPosition:
        IFC_RETURN(PropagateDefaultLabelPosition());
        IFC_RETURN(UpdateVisualState());
        break;

    case KnownPropertyIndex::CommandBar_IsDynamicOverflowEnabled:
        if (m_isDynamicOverflowEnabled != !!args.m_pNewValue->AsBool())
        {
            m_isDynamicOverflowEnabled = !!args.m_pNewValue->AsBool();

            IFC_RETURN(ResetDynamicCommands());
            IFC_RETURN(InvalidateMeasure());
            IFC_RETURN(UpdateVisualState());
        }
        break;

    case KnownPropertyIndex::AppBar_ClosedDisplayMode:
    case KnownPropertyIndex::CommandBar_OverflowButtonVisibility:
        IFC_RETURN(UpdateTemplateSettings());
        break;

    case KnownPropertyIndex::UIElement_Visibility:
        // We should not restore focus if visibility has changed because
        // this means we got collapsed and lost focus.
        ResetCommandBarElementFocus();
        break;

    case KnownPropertyIndex::UIElement_AccessKey:
        IFC_RETURN(SetAccessKeyAutomationPropertyOnExpandButton());
        break;
    }

    return S_OK;
}

_Check_return_
HRESULT
CommandBar::SetAccessKeyAutomationPropertyOnExpandButton()
{
    // If CommandBar has an AccessKey, we display the KeyTip on the expand button.
    // We also update the expand buttons AccessKey AutomationProperty so that Narrator will read the accesskey of the CommandBar
    // when the expand button has focus. This is necessary because the CommandBar itself typically does not recieve focus and so
    // Narrator will not read its AccessKey.
    if (m_tpExpandButton)
    {
        ctl::ComPtr<DependencyObject> spThisAsDO = this;
        wrl_wrappers::HString accessKeyStr;
        IFC_RETURN(AccessKeyStringBuilder::GetAccessKeyMessageFromElement(spThisAsDO, accessKeyStr.GetAddressOf()));

        ctl::ComPtr<DependencyObject> expandButtonAsDO = m_tpExpandButton.AsOrNull<DependencyObject>();
        IFC_RETURN(AutomationProperties::SetAccessKeyStatic(expandButtonAsDO.Get(), accessKeyStr.Get()));

    }

    return S_OK;
}

IFACEMETHODIMP
CommandBar::OnApplyTemplate()
{
    if (m_tpSecondaryItemsControlPart)
    {
        DetachHandler(m_secondaryItemsControlLoadedEventHandler, m_tpSecondaryItemsControlPart);
    }

    if (m_tpOverflowContentRoot)
    {
        DetachHandler(m_overflowContentSizeChangedEventHandler, m_tpOverflowContentRoot);
    }

    if (m_tpOverflowPresenterItemsPresenter)
    {
        DetachHandler(m_overflowPresenterItemsPresenterKeyDownEventHandler, m_tpOverflowPresenterItemsPresenter);
    }

    // Clear our previous template parts.
    m_tpContentControl.Clear();
    m_tpOverflowContentRoot.Clear();
    m_tpOverflowPopup.Clear();
    m_tpOverflowPresenterItemsPresenter.Clear();
    m_tpWindowedPopupPadding.Clear();

    if (m_tpExpandButton)
    {
        if (m_tpSecondaryItemsControlPart)
        {
            BOOLEAN isAKScope = FALSE;
            IFC_RETURN(m_tpExpandButton.Cast<Button>()->get_IsAccessKeyScope(&isAKScope));

            if (isAKScope)
            {
                IFC_RETURN(m_accessKeyInvokedEventHandler.DetachEventHandler(ctl::as_iinspectable(this)));
            }
        }

        UpdateOverflowButtonAutomationSetNumbers(0 /*sizeOfSet*/);
    }

    IFC_RETURN(__super::OnApplyTemplate());

    ctl::ComPtr<xaml::IFrameworkElement> overflowContentRoot;
    IFC_RETURN(GetTemplatePart<xaml::IFrameworkElement>(STR_LEN_PAIR(L"OverflowContentRoot"), overflowContentRoot.ReleaseAndGetAddressOf()));
    SetPtrValue(m_tpOverflowContentRoot, overflowContentRoot.Get());

    if (m_tpSecondaryItemsControlPart)
    {
        IFC_RETURN(m_secondaryItemsControlLoadedEventHandler.AttachEventHandler(
            m_tpSecondaryItemsControlPart.Cast<CommandBarOverflowPresenter>(),
            std::bind(&CommandBar::OnSecondaryItemsControlLoaded, this, _1, _2)
            ));

        // Apply a shadow
        if (CThemeShadow::IsDropShadowMode())
        {
            ctl::ComPtr<xaml::IFrameworkElement> outerOverflowContentRoot;
            IFC_RETURN(GetTemplatePart<xaml::IFrameworkElement>(STR_LEN_PAIR(L"OuterOverflowContentRoot"), outerOverflowContentRoot.ReleaseAndGetAddressOf()));
            if (outerOverflowContentRoot)
            {
                // WinUI code will apply the shadow to the OuterOverflowContentRoot if it's present (specifically in CommandBarFlyout),
                // but we still need to set the flag to fade in the shadow here.
                ctl::ComPtr<FrameworkElement> wrapperFE(outerOverflowContentRoot.AsOrNull<FrameworkElement>());
                CUIElement* wrapperUIE = static_cast<CUIElement*>(wrapperFE->GetHandle());
                wrapperUIE->SetShouldFadeInDropShadow(true);
            }
        }
        else
        {
            IFC_RETURN(ApplyElevationEffect(m_tpSecondaryItemsControlPart.AsOrNull<IUIElement>().Get()));
        }
    }

    ctl::ComPtr<xaml::IFrameworkElement> contentControl;
    IFC_RETURN(GetTemplatePart<xaml::IFrameworkElement>(STR_LEN_PAIR(L"ContentControl"), contentControl.ReleaseAndGetAddressOf()));
    SetPtrValue(m_tpContentControl, contentControl.Get());

    if (m_tpOverflowContentRoot)
    {
        IFC_RETURN(m_overflowContentSizeChangedEventHandler.AttachEventHandler(m_tpOverflowContentRoot.Get(), std::bind(&CommandBar::OnOverflowContentRootSizeChanged, this, _1, _2)));
    }

    ctl::ComPtr<xaml_primitives::IPopup> overflowPopup;
    IFC_RETURN(GetTemplatePart<xaml_primitives::IPopup>(STR_LEN_PAIR(L"OverflowPopup"), overflowPopup.ReleaseAndGetAddressOf()));
    SetPtrValue(m_tpOverflowPopup, overflowPopup.Get());

    if (m_tpOverflowPopup)
    {
        IFC_RETURN(m_tpOverflowPopup.Cast<Popup>()->put_IsSubMenu(TRUE));
    }

    ctl::ComPtr<xaml::IFrameworkElement> windowedPopupPadding;
    IFC_RETURN(GetTemplatePart<xaml::IFrameworkElement>(STR_LEN_PAIR(L"WindowedPopupPadding"), windowedPopupPadding.ReleaseAndGetAddressOf()));
    SetPtrValue(m_tpWindowedPopupPadding, windowedPopupPadding.Get());

    // Query overflow menu min/max width from resource dictionary.
    ctl::ComPtr<xaml::IResourceDictionary> resources;
    IFC_RETURN(get_Resources(&resources));
    resources.Cast<ResourceDictionary>()->TryLookupBoxedValue(wrl_wrappers::HStringReference(L"CommandBarOverflowMinWidth").Get(), &m_overflowContentMinWidth);
    resources.Cast<ResourceDictionary>()->TryLookupBoxedValue(wrl_wrappers::HStringReference(L"CommandBarOverflowTouchMinWidth").Get(), &m_overflowContentTouchMinWidth);
    resources.Cast<ResourceDictionary>()->TryLookupBoxedValue(wrl_wrappers::HStringReference(L"CommandBarOverflowMaxWidth").Get(), &m_overflowContentMaxWidth);

    // We set CommandBarTemplateSettings.OverflowContentMaxWidth immediately, rather than waiting for the CommandBar to open before setting it.
    // If we don't initialize it here, it will default to 0, meaning the overflow will stay at size 0,0 and will never fire the SizeChanged event.
    // The SizeChanged event is what triggers the call to UpdateTemplateSettings after the CommandBar opens.
    ctl::ComPtr<xaml_primitives::ICommandBarTemplateSettings> templateSettings;
    IFC_RETURN(get_CommandBarTemplateSettings(&templateSettings));
    IFC_RETURN(templateSettings.Cast<CommandBarTemplateSettings>()->put_OverflowContentMaxWidth(m_overflowContentMaxWidth));

    // Configure our template part items controls by setting their items source's to
    // the correct items vector.
    IFC_RETURN(ConfigureItemsControls());

    BOOLEAN isOpen = FALSE;
    IFC_RETURN(get_IsOpen(&isOpen));

    // Put the primary commands into compact mode if not open.
    if (!isOpen)
    {
        IFC_RETURN(SetCompactMode(TRUE));
    }

    // Inform the secondary AppBarButtons whether or not any secondary AppBarToggleButtons exist.
    IFC_RETURN(SetOverflowStyleParams());

    IFC_RETURN(PropagateDefaultLabelPosition());

    UINT32 secondaryItemCount = 0;
    IFC_RETURN(m_tpDynamicSecondaryCommands.Get()->get_Size(&secondaryItemCount));
    for (UINT32 i = 0; i < secondaryItemCount; ++i)
    {
        IFC_RETURN(SetOverflowStyleAndInputModeOnSecondaryCommand(i, true));
    }

    //Enabling Keytips and AccessKeys in CommandBar secondary commands
    if (m_tpExpandButton && m_tpSecondaryItemsControlPart)
    {
        BOOLEAN isAKScope = FALSE;
        IFC_RETURN(m_tpExpandButton.Cast<Button>()->get_IsAccessKeyScope(&isAKScope));

        if (isAKScope)
        {
            IFC_RETURN(m_tpSecondaryItemsControlPart.Cast<CommandBarOverflowPresenter>()->put_AccessKeyScopeOwner(
                m_tpExpandButton.AsOrNull<IDependencyObject>().Get()));

            IFC_RETURN(m_accessKeyInvokedEventHandler.AttachEventHandler(
                this,
                std::bind(&CommandBar::OnAccessKeyInvoked, this, _1, _2)
            ));
        }
    }

    IFC_RETURN(SetAccessKeyAutomationPropertyOnExpandButton());

    IFC_RETURN(UpdateVisualState());

    return S_OK;
}

IFACEMETHODIMP
CommandBar::MeasureOverride(_In_ wf::Size availableSize, _Out_ wf::Size* returnValue)
{
    // These should have been created during initialization.
    IFCEXPECT_RETURN(m_tpDynamicPrimaryCommands.Get());
    IFCEXPECT_RETURN(m_tpDynamicSecondaryCommands.Get());

    if (m_isDynamicOverflowEnabled)
    {
        IFC_RETURN(MeasureOverrideForDynamicOverflow(availableSize, returnValue));
    }
    else
    {
        IFC_RETURN(__super::MeasureOverride(availableSize, returnValue));
    }

    return S_OK;
}

IFACEMETHODIMP
CommandBar::ArrangeOverride(
    _In_ wf::Size arrangeSize,
    _Out_ wf::Size* returnValue)
{
    IFC_RETURN(__super::ArrangeOverride(arrangeSize, returnValue));

    // We need to wait until the measure pass is done and the new command containers
    // are generated before we restore focus. Note that the measure pass will measure
    // both the CommandBar and the secondary commands' popup. The latter is why we
    // can't call RestoreCommandBarElementFocus at the end of CommandBar::MeasureOverride.
    IFC_RETURN(RestoreCommandBarElementFocus());
    return S_OK;
}

_Check_return_ HRESULT
CommandBar::MeasureOverrideForDynamicOverflow(
    _In_ wf::Size availableSize,
    _Out_ wf::Size* returnValue
    )
{
    wf::Size contentRootDesiredSize = {};
    wf::Size availablePrimarySize = {};

    ASSERT(m_isDynamicOverflowEnabled);

    IFC_RETURN(__super::MeasureOverride(availableSize, returnValue));

    if (m_tpPrimaryItemsControlPart && m_tpContentRoot)
    {
        // Get the available sizes for the primary commands.
        IFC_RETURN(m_tpPrimaryItemsControlPart.AsOrNull<IUIElement>()->get_DesiredSize(&availablePrimarySize));

        IFC_RETURN(m_tpContentRoot.AsOrNull<IUIElement>()->Measure({ XFLOAT_INF, XFLOAT_INF }));
        IFC_RETURN(m_tpContentRoot.AsOrNull<IUIElement>()->get_DesiredSize(&contentRootDesiredSize));

        // Check the available width whether it needs to move the primary commands into overflow or
        // restore the primary commands from overflow to the primary commands collection
        if (contentRootDesiredSize.Width > availableSize.Width)
        {
            UINT32 itemsCount = 0;

            IFC_RETURN(m_tpDynamicPrimaryCommands->get_Size(&itemsCount));

            if (itemsCount > 0)
            {
                UINT32 primaryCommandsCountInTransition = 0;
                wf::Size primaryDesiredSize = {};

                // Get the desired sizes for our primary commands.
                IFC_RETURN(m_tpPrimaryItemsControlPart.AsOrNull<IUIElement>()->Measure({ XFLOAT_INF, XFLOAT_INF }));
                IFC_RETURN(m_tpPrimaryItemsControlPart.AsOrNull<IUIElement>()->get_DesiredSize(&primaryDesiredSize));

                IFC_RETURN(FindMovablePrimaryCommands(
                    availablePrimarySize.Width,
                    primaryDesiredSize.Width,
                    &primaryCommandsCountInTransition));

                if (primaryCommandsCountInTransition > 0)
                {
                    IFC_RETURN(TrimPrimaryCommandSeparatorInOverflow(&primaryCommandsCountInTransition));

                    if (primaryCommandsCountInTransition > 0)
                    {
                        if (!m_hasAlreadyFiredOverflowChangingEvent)
                        {
                            IFC_RETURN(FireDynamicOverflowItemsChangingEvent(false /* isForceToRestore */));
                        }

                        // Update the transited  primary command min width that will be used for restoring the
                        // primary commands from the overflow into the primary commands collection
                        IFC_RETURN(UpdatePrimaryCommandElementMinWidthInOverflow());

                        IFC_RETURN(MoveTransitionPrimaryCommandsIntoOverflow(primaryCommandsCountInTransition));

                        // Update the overflow alignment for having toggle button
                        IFC_RETURN(SetOverflowStyleParams());

                        // Save the current transition to compare with the next coming transition
                        IFC_RETURN(SaveMovedPrimaryCommandsIntoPreviousTransitionCollection());
                    }

                    // At this point, we'll have modified our primary and secondary command collections, which
                    // impacts our visual state.  We should update our visual state to ensure that it's current.
                    IFC_RETURN(UpdateVisualState());
                }
            }
        }
        else if (m_lastAvailableWidth < availableSize.Width &&
                 m_SecondaryCommandStartIndex > 0 &&
                 m_restorablePrimaryCommandMinWidth >= 0)
        {
            UINT32 restorableMinCount = 0;
            double restorablePrimaryCommandMinWidth = m_restorablePrimaryCommandMinWidth;

            double availableWidthToRestore = availableSize.Width - contentRootDesiredSize.Width;

            IFC_RETURN(GetRestorablePrimaryCommandsMinimumCount(&restorableMinCount));
            if (restorableMinCount > 0)
            {
                restorablePrimaryCommandMinWidth *= restorableMinCount;
            }

            if (availableWidthToRestore > restorablePrimaryCommandMinWidth)
            {
                IFC_RETURN(FireDynamicOverflowItemsChangingEvent(true /* isForceToRestore */));
                m_hasAlreadyFiredOverflowChangingEvent = true;

                // There is the restorable primary commands from the overflow into the primary command collection.
                // Reset the dynamic primary and secondary commands collection then recalculate the primary commands
                // items control to fit the primary command with the current CommandBar available width.
                IFC_RETURN(ResetDynamicCommands());
                IFC_RETURN(SaveMovedPrimaryCommandsIntoPreviousTransitionCollection());
                IFC_RETURN(m_tpPrimaryItemsControlPart.Cast<ItemsControl>()->InvalidateMeasure());
                IFC_RETURN(SetOverflowStyleParams());

                // At this point, we'll have modified our primary and secondary command collections, which
                // impacts our visual state.  We should update our visual state to ensure that it's current.
                IFC_RETURN(UpdateVisualState());
            }
        }
    }

    m_hasAlreadyFiredOverflowChangingEvent = false;
    m_lastAvailableWidth = availableSize.Width;

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::ChangeVisualState(bool useTransitions)
{
    IFC_RETURN(__super::ChangeVisualState(useTransitions));

    BOOLEAN ignored = FALSE;

    // AvailableCommandsStates
    {
        bool hasVisiblePrimaryElements = false;
        IFC_RETURN(HasVisibleElements(m_tpDynamicPrimaryCommands.Get(), &hasVisiblePrimaryElements));

        bool hasVisibleSecondaryElements = false;
        IFC_RETURN(HasVisibleElements(m_tpDynamicSecondaryCommands.Get(), &hasVisibleSecondaryElements));

        const wchar_t* state = (hasVisiblePrimaryElements && hasVisibleSecondaryElements ?
            L"BothCommands" : (!hasVisibleSecondaryElements ? L"PrimaryCommandsOnly" : L"SecondaryCommandsOnly"));
        IFC_RETURN(GoToState(useTransitions, state, &ignored));
    }

    if (m_isDynamicOverflowEnabled)
    {
        IFC_RETURN(GoToState(useTransitions, L"DynamicOverflowEnabled", &ignored));
    }
    else
    {
        IFC_RETURN(GoToState(useTransitions, L"DynamicOverflowDisabled", &ignored));
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::ConfigureItemsControls()
{
    IFC_RETURN(ResetDynamicCommands());

    // The wrapping collections have a somewhat unusual reference pattern, which should be
    // documented here to avoid having it accidentally perturbed in the future in a way that
    // could lead to problems.  In short, the wrapping collections hold a reference to
    // the collections they wrap, but the wrapped collections don't hold any reference
    // in the reverse direction - aside from these tracker pointers, the only control
    // that holds a reference to the wrapping collections is the ItemsControl that
    // they're given to.  As a result, without these tracker pointers, the ItemsControl,
    // when it is deleted, would in turn delete the wrapping collection, since nothing else
    // holds a reference to it.  However, this is a problem since the wrapping collection adds
    // an event handler to the wrapped collection's VectorChanged event, which can lead to
    // the wrapped collection calling a method on the deleted wrapping collection
    // if we don't keep it alive after the ItemsControl is deleted.
    // The simplest way to keep it alive is just to have another reference to it,
    // which is what these tracker pointers do (until the CommandBar is deleted, at which point
    // that last reference will be removed and they'll be properly cleaned up).
    m_tpWrappedPrimaryCommands.Clear();
    m_tpWrappedSecondaryCommands.Clear();

    if (m_tpPrimaryItemsControlPart)
    {
        ctl::ComPtr<wfc::IVector<xaml_controls::ICommandBarElement*>> spVector;
        ctl::ComPtr<IterableWrappedObservableCollection<xaml_controls::ICommandBarElement>> spWrappedCollection;

        IFC_RETURN(m_tpDynamicPrimaryCommands.As(&spVector))

        // Set the primary items control source.
        IFC_RETURN(ctl::make(&spWrappedCollection));
        IFC_RETURN(spWrappedCollection->SetWrappedCollection(spVector.Get()));
        IFC_RETURN(m_tpPrimaryItemsControlPart->put_ItemsSource(ctl::as_iinspectable(spWrappedCollection.Get())));
        SetPtrValue(m_tpWrappedPrimaryCommands, spWrappedCollection.Get());
    }

    if (m_tpSecondaryItemsControlPart)
    {
        ctl::ComPtr<wfc::IVector<xaml_controls::ICommandBarElement*>> spVector;
        ctl::ComPtr<IterableWrappedObservableCollection<xaml_controls::ICommandBarElement>> spWrappedCollection;

        IFC_RETURN(m_tpDynamicSecondaryCommands.As(&spVector))

        // Set the secondary items control source.
        IFC_RETURN(ctl::make(&spWrappedCollection));
        IFC_RETURN(spWrappedCollection->SetWrappedCollection(spVector.Get()));
        IFC_RETURN(m_tpSecondaryItemsControlPart->put_ItemsSource(ctl::as_iinspectable(spWrappedCollection.Get())));
        SetPtrValue(m_tpWrappedSecondaryCommands, spWrappedCollection.Get());
    }

    return S_OK;
}

// Handles KeyDown events for left and right keys to control shifting focus
// through the primary and secondary items.
IFACEMETHODIMP
CommandBar::OnKeyDown(_In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    IFCPTR_RETURN(pArgs);
    IFC_RETURN(__super::OnKeyDown(pArgs));

    BOOLEAN isHandled = FALSE;
    IFC_RETURN(pArgs->get_Handled(&isHandled));

    // Ignore already handled events
    if (isHandled)
    {
        return S_OK;
    }

    auto key = wsy::VirtualKey_None;
    IFC_RETURN(static_cast<KeyRoutedEventArgs*>(pArgs)->get_Key(&key));

    auto originalKey = wsy::VirtualKey_None;
    IFC_RETURN(static_cast<KeyRoutedEventArgs*>(pArgs)->get_OriginalKey(&originalKey));

    // Determine whether this is a gamepad key event and whether it should be handled
    // by this method.  We only handle gamepad key events here if the menu is open
    // since it needs to by-pass the default gamepad navigation behavior to be
    // able to navigate into the overflow menu.
    bool isGamepadNavigationEvent = false;
    bool shouldHandleGamepadNavigationEvent = false;
    if (XboxUtility::IsGamepadNavigationDirection(originalKey))
    {
        isGamepadNavigationEvent = true;

        BOOLEAN isOpen = FALSE;
        IFC_RETURN(get_IsOpen(&isOpen));
        shouldHandleGamepadNavigationEvent = !!isOpen;
    }

    // Always handle non-gamepad events.  Only handle gamepad events when the menu is open.
    if (!isGamepadNavigationEvent || shouldHandleGamepadNavigationEvent)
    {
        bool wasHandled = false;
        switch (key)
        {
            case wsy::VirtualKey_Right:
            case wsy::VirtualKey_Left:
                IFC_RETURN(OnLeftRightKeyDown(key == wsy::VirtualKey_Left, &wasHandled));
                break;

            case wsy::VirtualKey_Up:
            case wsy::VirtualKey_Down:
                // Do not support focus wrapping for gamepad navigation because it can
                // trap the user which makes navigation difficult.
                IFC_RETURN(OnUpDownKeyDown(key == wsy::VirtualKey_Up, !isGamepadNavigationEvent, &wasHandled));
                break;
        }

        IFC_RETURN(pArgs->put_Handled(wasHandled ? TRUE : FALSE));
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::OnLeftRightKeyDown(bool isLeftKey, _Out_ bool* wasHandled)
{
    *wasHandled = false;

    // If the ALT key is not pressed, don't shift focus.
    auto modifierKeys = wsy::VirtualKeyModifiers_None;
    IFC_RETURN(CoreImports::Input_GetKeyboardModifiers(&modifierKeys));
    if ((modifierKeys & wsy::VirtualKeyModifiers_Menu) != 0)
    {
        return S_OK;
    }

    bool moveToRight = true;
    auto flowDirection = xaml::FlowDirection_LeftToRight;

    // The direction that we'll shift through our lists is based on flow direction.
    IFC_RETURN(get_FlowDirection(&flowDirection));

    moveToRight =
        (flowDirection == xaml::FlowDirection_LeftToRight && !isLeftKey)
        || (flowDirection == xaml::FlowDirection_RightToLeft && isLeftKey);

    IFC_RETURN(ShiftFocusHorizontally(moveToRight));

    *wasHandled = true;

    return S_OK;
}

// If focus is on the expand button, then pressing up or down will
// move focus to the overflow menu.
_Check_return_ HRESULT
CommandBar::OnUpDownKeyDown(bool isUpKey, bool allowFocusWrap, _Out_ bool* wasHandled)
{
    *wasHandled = false;

    if (!m_tpExpandButton)
    {
        return S_OK;
    }

    // If the ALT key is pressed, don't shift focus.
    auto modifierKeys = wsy::VirtualKeyModifiers_None;
    IFC_RETURN(CoreImports::Input_GetKeyboardModifiers(&modifierKeys));
    if ((modifierKeys & wsy::VirtualKeyModifiers_Menu) != 0)
    {
        return S_OK;
    }

    // Only handle the up/down keys when focus is on the more/expand button.
    ctl::ComPtr<DependencyObject> focusedElement;
    IFC_RETURN(GetFocusedElement(&focusedElement));
    if (ctl::are_equal(m_tpExpandButton.Get(), focusedElement.Get()))
    {
        BOOLEAN isOpen = FALSE;
        IFC_RETURN(get_IsOpen(&isOpen));
        if (isOpen)
        {
            bool overflowOpensUp;
            IFC_RETURN(GetShouldOpenUp(&overflowOpensUp));

            if (isUpKey)
            {
                // We go to the last focusable element in the overflow if wrapping is allowed OR
                // if the overflow opens up.
                if (allowFocusWrap || overflowOpensUp)
                {
                    // Focus the last element.
                    IFC_RETURN(SetFocusedElementInOverflow(false /* focusFirstElement */, wasHandled));
                }
            }
            else
            {
                if (allowFocusWrap || !overflowOpensUp)
                {
                    IFC_RETURN(SetFocusedElementInOverflow(true /* focusFirstElement */, wasHandled));
                }
            }
        }
        else
        {
            // Open the overflow menu and configure one of its items to get focus depending
            // on whether we are navigating up or down.
            m_overflowInitialFocusItem = (isUpKey ? OverflowInitialFocusItem::LastItem : OverflowInitialFocusItem::FirstItem);

            // Pressing up or down on the more/expand button opens the overflow.
            IFC_RETURN(put_IsOpen(TRUE));
            *wasHandled = true;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::ShiftFocusVerticallyInOverflow(bool topToBottom, bool allowFocusWrap)
{
    ctl::ComPtr<DependencyObject> focusedElement;
    IFC_RETURN(GetFocusedElement(&focusedElement));

    xref_ptr<CDependencyObject> referenceElement;
    if (topToBottom)
    {
        IFC_RETURN(CoreImports::FocusManager_GetLastFocusableElement(
            m_tpOverflowPresenterItemsPresenter.Cast<ItemsPresenter>()->GetHandle(),
            referenceElement.ReleaseAndGetAddressOf())
            );
    }
    else
    {
        IFC_RETURN(CoreImports::FocusManager_GetFirstFocusableElement(
            m_tpOverflowPresenterItemsPresenter.Cast<ItemsPresenter>()->GetHandle(),
            referenceElement.ReleaseAndGetAddressOf())
            );
    }

    if (focusedElement->GetHandle() == referenceElement)
    {
        bool overflowOpensUp;
        IFC_RETURN(GetShouldOpenUp(&overflowOpensUp));

        // We go to the expand button if wrapping is allowed OR one of the following is true:
        // a. Up key is pressed and the overflow opens down.
        // b. Down key is pressed and the overflow opens up.
        if (allowFocusWrap || !(overflowOpensUp ^ topToBottom))
        {
            //Set the focus back to the Expand Button
            BOOLEAN ignored = FALSE;
            IFC_RETURN(DependencyObject::SetFocusedElement(
                m_tpExpandButton.Cast<ButtonBase>(),
                xaml::FocusState_Keyboard,
                FALSE /*animateIfBringIntoView*/,
                &ignored)
                );
            IFC_RETURN(ElementSoundPlayerService::RequestInteractionSoundForElementStatic(xaml::ElementSoundKind_Focus, this));
        }
    }
    else
    {
        bool ignored = false;

        CFocusManager* focusManager = VisualTree::GetFocusManagerForElement(GetHandle());
        IFC_RETURN(focusManager->TryMoveFocus(topToBottom ? FocusNavigationDirection::Down : FocusNavigationDirection::Up, &ignored));
        IFC_RETURN(ElementSoundPlayerService::RequestInteractionSoundForElementStatic(xaml::ElementSoundKind_Focus, this));
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::HandleTabKeyPressedInOverflow(bool isShiftKeyPressed, _Out_ bool* wasHandled)
{
    *wasHandled = false;

    auto overflowNavigationMode = xaml_input::KeyboardNavigationMode_Local;
    if (m_tpSecondaryItemsControlPart)
    {
        IFC_RETURN(m_tpSecondaryItemsControlPart.Cast<ItemsControl>()->get_TabNavigation(&overflowNavigationMode));
    }

    // If the overflow's tab navigation mode is set to once, always leave
    // on tab key presses.
    bool shouldFocusLeaveOverflow = (overflowNavigationMode == xaml_input::KeyboardNavigationMode_Once);

    // Otherwise, determine whether focus should leave the overflow menu based on whether
    // focus is currently on the first/last item depending on direction.
    if (!shouldFocusLeaveOverflow)
    {
        ctl::ComPtr<DependencyObject> focusedElement;
        IFC_RETURN(GetFocusedElement(&focusedElement));

        xref_ptr<CDependencyObject> referenceElement;
        if (isShiftKeyPressed)
        {
            IFC_RETURN(CoreImports::FocusManager_GetFirstFocusableElement(
                m_tpOverflowPresenterItemsPresenter.Cast<ItemsPresenter>()->GetHandle(),
                referenceElement.ReleaseAndGetAddressOf())
                );
        }
        else
        {
            IFC_RETURN(CoreImports::FocusManager_GetLastFocusableElement(
                m_tpOverflowPresenterItemsPresenter.Cast<ItemsPresenter>()->GetHandle(),
                referenceElement.ReleaseAndGetAddressOf())
                );
        }

        // Only return focus to the bar if we're at either end of the
        // menu and moving focus would cause us to wrap around.
        shouldFocusLeaveOverflow = (focusedElement->GetHandle() == referenceElement);
    }

    if (shouldFocusLeaveOverflow)
    {
        xref_ptr<CDependencyObject> focusCandidate;
        bool shouldMoveFocusOutsideOfCommandBar = false;

        if (isShiftKeyPressed)
        {
            // Backwards navigation out of the overflow menu always focuses the last focusable element
            // in the bar.
            IFC_RETURN(CoreImports::FocusManager_GetLastFocusableElement(GetHandle(), focusCandidate.ReleaseAndGetAddressOf()));
        }
        else
        {
            // Determine whether we should allow the focus to move outside of the CommandBar.
            // We should allow the focus to move out in the sticky case only.
            BOOLEAN isSticky = FALSE;
            IFC_RETURN(get_IsSticky(&isSticky));

            shouldMoveFocusOutsideOfCommandBar = !!isSticky;

            // If we should move outside of the CommandBar, then focus the last item in the CommandBar so that
            // we can later move focus again to focus the next element outside of the CommandBar.
            // Otherwise, we'll cycle focus back to the first item in the CommandBar.
            IFC_RETURN(shouldMoveFocusOutsideOfCommandBar ?
                CoreImports::FocusManager_GetLastFocusableElement(GetHandle(), focusCandidate.ReleaseAndGetAddressOf()) :
                CoreImports::FocusManager_GetFirstFocusableElement(GetHandle(), focusCandidate.ReleaseAndGetAddressOf())
                );
        }

        if (focusCandidate)
        {
            ctl::ComPtr<DependencyObject> focusCandidatePeer;
            IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(focusCandidate, &focusCandidatePeer));

            BOOLEAN ignored = FALSE;
            IFC_RETURN(DependencyObject::SetFocusedElement(
                focusCandidatePeer.Get(),
                xaml::FocusState_Keyboard,
                FALSE /*animateIfBringIntoView*/,
                &ignored,
                true /*isProcessingTab*/,
                isShiftKeyPressed)
                );

            if (shouldMoveFocusOutsideOfCommandBar)
            {
                // Make sure we don't try to override the following move focus operation
                // otherwise we'll end up moving focus back into the overflow menu.
                m_skipProcessTabStopOverride = true;

                bool ignored2 = false;
                CFocusManager* focusManager = VisualTree::GetFocusManagerForElement(GetHandle());
                IFC_RETURN(focusManager->TryMoveFocus(FocusNavigationDirection::Next, &ignored2));
                IFC_RETURN(ElementSoundPlayerService::RequestInteractionSoundForElementStatic(xaml::ElementSoundKind_Focus, this));

                m_skipProcessTabStopOverride = false;
            }
        }

        *wasHandled = true;
    }

    return S_OK;
}

_Check_return_ HRESULT CommandBar::SetFocusedElementInOverflow(bool focusFirstElement, _Out_ bool* wasFocusSet)
{
    *wasFocusSet = false;

    if (!m_tpOverflowPresenterItemsPresenter)
    {
        return S_OK;
    }

    xref_ptr<CDependencyObject> focusableElement;
    if (focusFirstElement)
    {
        IFC_RETURN(CoreImports::FocusManager_GetFirstFocusableElement(
            m_tpOverflowPresenterItemsPresenter.Cast<ItemsPresenter>()->GetHandle(),
            focusableElement.ReleaseAndGetAddressOf())
        );
    }
    else
    {
        IFC_RETURN(CoreImports::FocusManager_GetLastFocusableElement(
            m_tpOverflowPresenterItemsPresenter.Cast<ItemsPresenter>()->GetHandle(),
            focusableElement.ReleaseAndGetAddressOf())
        );
    }

    if (focusableElement)
    {
        ctl::ComPtr<DependencyObject> focusableElementPeer;
        IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(focusableElement, &focusableElementPeer));

        BOOLEAN wasFocusUpdated = FALSE;
        IFC_RETURN(DependencyObject::SetFocusedElement(
            focusableElementPeer.Get(),
            xaml::FocusState_Keyboard,
            FALSE /*animateIfBringIntoView*/,
            &wasFocusUpdated)
        );

        *wasFocusSet = !!wasFocusUpdated;
    }

    return S_OK;
}

// Handle the cases where focus is currently in the CommandBar and the user hits Tab. If focus
// is currently on the last focusable element in the bar and the user hits tab, we move the
// focus onto the first focusable element in overflow menu rather than letting it go out of
// the control.
_Check_return_ HRESULT
CommandBar::ProcessTabStopOverride(
    _In_opt_ DependencyObject* pFocusedElement,
    _In_opt_ DependencyObject* pCandidateTabStopElement,
    const bool isBackward,
    const bool didCycleFocusAtRootVisualScope,
    _Outptr_ DependencyObject** ppNewTabStop,
    _Out_ BOOLEAN* pIsTabStopOverridden
    )
{
    // Give the AppBar code a chance to override the candidate.
    IFC_RETURN(__super::ProcessTabStopOverride(pFocusedElement, pCandidateTabStopElement, isBackward, didCycleFocusAtRootVisualScope, ppNewTabStop, pIsTabStopOverridden));

    // This method is only interested in the forward navigation case, so bail out early if otherwise.
    if (isBackward)
    {
        return S_OK;
    }

    if (m_skipProcessTabStopOverride)
    {
        return S_OK;
    }

    BOOLEAN isOpen = FALSE;
    IFC_RETURN(get_IsOpen(&isOpen));

    if (isOpen && m_tpOverflowPresenterItemsPresenter)
    {
        xref_ptr<CDependencyObject> lastFocusableElement;
        IFC_RETURN(CoreImports::FocusManager_GetLastFocusableElement(GetHandle(), lastFocusableElement.ReleaseAndGetAddressOf()));

        // Move focus to the overflow menu when tabbing forwards and we're moving off
        // of the last focusable element in the bar.
        if (pFocusedElement != nullptr && pFocusedElement->GetHandle() == lastFocusableElement)
        {
            xref_ptr<CDependencyObject> newTabStop;

            IFC_RETURN(CoreImports::FocusManager_GetFirstFocusableElement(m_tpOverflowPresenterItemsPresenter.Cast<ItemsPresenter>()->GetHandle(), newTabStop.ReleaseAndGetAddressOf()));

            // If we found a candidate, then query its corresponding peer.
            if (newTabStop)
            {
                // If the AppBar overrode the tab stop, then we need to release its candidate otherwise
                // we'll leak.
                if (*pIsTabStopOverridden)
                {
                    ASSERT(*ppNewTabStop != nullptr);
                    ctl::release_interface(*ppNewTabStop);
                }

                IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(newTabStop, ppNewTabStop));
                *pIsTabStopOverridden = TRUE;
            }
        }
    }

    return S_OK;
}

// Handle the cases where focus is not currently in the CommandBar and the user hits Shift-Tab
// to focus the control. If focus is moving onto the last focusable element in the bar, then we
// instead move it onto the last focusable element in the overflow menu.
// We also end up handling the case where the AppBar::ProcessTabStopOverride() implementation
// tries to wrap focus back to the last element in the bar from the first element.  In that
// situation, we also override it to move focus into the overflow menu instead.
_Check_return_ HRESULT
CommandBar::ProcessCandidateTabStopOverride(
    _In_opt_ DependencyObject* pFocusedElement,
    _In_ DependencyObject* pCandidateTabStopElement,
    _In_opt_ DependencyObject* pOverriddenCandidateTabStopElement,
    const bool isBackward,
    _Outptr_ DependencyObject** ppNewTabStop,
    _Out_ BOOLEAN* pIsCandidateTabStopOverridden)
{
    // This method is only interested in the backward navigation case, so bail out early if otherwise.
    if (!isBackward)
    {
        return S_OK;
    }

    BOOLEAN isOpen = FALSE;
    IFC_RETURN(get_IsOpen(&isOpen));

    if (isOpen && m_tpOverflowPresenterItemsPresenter)
    {
        xref_ptr<CDependencyObject> lastFocusableElement;
        IFC_RETURN(CoreImports::FocusManager_GetLastFocusableElement(GetHandle(), lastFocusableElement.ReleaseAndGetAddressOf()));

        // Move focus to the overflow menu when tabbing backwards and we're moving onto
        // the last focusable element in the bar.
        if (pCandidateTabStopElement != nullptr && pCandidateTabStopElement->GetHandle() == lastFocusableElement)
        {
            xref_ptr<CDependencyObject> newTabStop;

            // When overriding focus to go into the overflow menu, since TabNavigation==Once means that focus
            // only moves into a particular tree once, if that is set then the element we'll focus
            // is the first overflow item.  If it is any other value, we focus the last element since
            // this method is only applicable during backward navigation.
            {
                auto overflowNavigationMode = xaml_input::KeyboardNavigationMode_Local;
                if (m_tpSecondaryItemsControlPart)
                {
                    IFC_RETURN(m_tpSecondaryItemsControlPart.Cast<ItemsControl>()->get_TabNavigation(&overflowNavigationMode));
                }

                if (overflowNavigationMode == xaml_input::KeyboardNavigationMode_Once)
                {
                    IFC_RETURN(CoreImports::FocusManager_GetFirstFocusableElement(m_tpOverflowPresenterItemsPresenter.Cast<ItemsPresenter>()->GetHandle(), newTabStop.ReleaseAndGetAddressOf()));
                }
                else
                {
                    IFC_RETURN(CoreImports::FocusManager_GetLastFocusableElement(m_tpOverflowPresenterItemsPresenter.Cast<ItemsPresenter>()->GetHandle(), newTabStop.ReleaseAndGetAddressOf()));
                }
            }

            // If we found a candidate, then query its corresponding peer.
            if (newTabStop)
            {
                IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(newTabStop, ppNewTabStop));
                *pIsCandidateTabStopOverridden = TRUE;
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::ShiftFocusHorizontally(bool moveToRight)
{
    // Determine whether we should shift focus horizontally.
    if (m_tpContentControl)
    {
        ctl::ComPtr<DependencyObject> focusedElement;
        IFC_RETURN(GetFocusedElement(&focusedElement));

        // Don't do it if focus is in the custom content area.
        BOOLEAN isChildOfContentControl = FALSE;
        IFC_RETURN(m_tpContentControl.Cast<FrameworkElement>()->IsAncestorOf(focusedElement.Get(), &isChildOfContentControl));
        if (isChildOfContentControl)
        {
            // Bail out.
            return S_OK;
        }

        // Don't wrap focus, so if we're at either end don't shift focus anymore.
        xref_ptr<CDependencyObject> referenceElement;

        if (moveToRight)
        {
            if (m_tpExpandButton)
            {
                referenceElement = m_tpExpandButton.Cast<ButtonBase>()->GetHandle();
            }

            if (!referenceElement && m_tpPrimaryItemsControlPart)
            {
                IFC_RETURN(CoreImports::FocusManager_GetLastFocusableElement(
                    m_tpPrimaryItemsControlPart.Cast<ItemsControl>()->GetHandle(),
                    referenceElement.ReleaseAndGetAddressOf())
                    );
            }
        }
        else
        {
            if (m_tpPrimaryItemsControlPart)
            {
                IFC_RETURN(CoreImports::FocusManager_GetFirstFocusableElement(
                    m_tpPrimaryItemsControlPart.Cast<ItemsControl>()->GetHandle(),
                    referenceElement.ReleaseAndGetAddressOf())
                    );
            }

            if (!referenceElement && m_tpExpandButton)
            {
                referenceElement = m_tpExpandButton.Cast<ButtonBase>()->GetHandle();
            }
        }

        if (focusedElement->GetHandle() == referenceElement)
        {
            // Bail out.
            return S_OK;
        }
    }

    bool focusChanged = false;
    CFocusManager* focusManager = VisualTree::GetFocusManagerForElement(GetHandle());
    IFC_RETURN(focusManager->TryMoveFocus(moveToRight ? FocusNavigationDirection::Right : FocusNavigationDirection::Left, &focusChanged));
    IFC_RETURN(ElementSoundPlayerService::RequestInteractionSoundForElementStatic(xaml::ElementSoundKind_Focus, this));

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::SetCompactMode(_In_ bool isCompact)
{
    ctl::ComPtr<xaml_controls::ICommandBarElement> element;

    IFCEXPECT_RETURN(m_tpDynamicPrimaryCommands.Get());
    IFCEXPECT_RETURN(m_tpDynamicSecondaryCommands.Get());

    // Set the compact mode on the primary items.
    {
        UINT32 primaryItemsCount = 0;
        IFC_RETURN(m_tpDynamicPrimaryCommands.Get()->get_Size(&primaryItemsCount));

        for (UINT32 i = 0; i < primaryItemsCount; ++i)
        {
            IFC_RETURN(m_tpDynamicPrimaryCommands.Get()->GetAt(i, &element));
            IFC_RETURN(element->put_IsCompact(isCompact ? TRUE : FALSE));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::OnOpeningImpl(_In_ IInspectable* pArgs)
{
    IFC_RETURN(__super::OnOpeningImpl(pArgs));
    IFC_RETURN(SetCompactMode(false));

    if (m_tpOverflowPopup)
    {
        IFC_RETURN(m_tpOverflowPopup->put_IsOpen(TRUE));
    }

    IFC_RETURN(UpdateInputDeviceTypeUsedToOpen());

    // After we call OnOpeningImpl, we'll make a call to UpdateVisualState which
    // uses the height of the overflow content root to determine whether it should
    // open up or down.  To make sure it is up-to-date for that call, we update
    // the menu's layout here.
    if (m_tpOverflowContentRoot)
    {
        IFC_RETURN(m_tpOverflowContentRoot.Cast<FrameworkElement>()->UpdateLayout());
    }

    if (CThemeShadow::IsDropShadowMode())
    {
        // For the CommandBar control, there will be a wrapper around the overflow flyout. The shadow should be applied
        // to this wrapper. We'll also be clearing this shadow right before the element closes.
        ctl::ComPtr<xaml::IFrameworkElement> secondaryItemsControlWrapper;
        IFC_RETURN(GetTemplatePart<xaml::IFrameworkElement>(STR_LEN_PAIR(L"SecondaryItemsControlShadowWrapper"), secondaryItemsControlWrapper.ReleaseAndGetAddressOf()));
        if (secondaryItemsControlWrapper)
        {
            IFC_RETURN(ApplyElevationEffect(secondaryItemsControlWrapper.AsOrNull<IUIElement>().Get()));

            m_canShadowBeAnimatedByEntranceAnimation = false;
            if (m_tpOverflowPopup.Cast<Popup>()->IsWindowed())
            {
                //
                // We want this CommandBarOverflow popup's shadow to animate together with the popup contents (and system
                // backdrop). In order to do that, the animation must target high up in the popup tree, higher than we can
                // reach via from the UIElement tree.
                //
                // Popups have a mechanism for such animations - they have a secret TransitionTarget inside, and the
                // transform of that TransitionTarget will be placed near the root of the popup's Visual tree, above the
                // content, shadow, and system backdrop Visuals. This is where we have to place the transform targeted by
                // the entrance animation. We can get to the TransitionTarget through the CPopup, and we have an animating
                // CompositeTransform in our overflow popup's ResourceDictionary, so we just hook that animating
                // CompositeTransform up to the CPopup's secret TransitionTarget.
                //
                // Note that retemplated CommandBars may not have a named CompositeTransform in Popup.Resources, in which
                // case we have to fall back to a shadow that fades in after a delay.
                //
                xref_ptr<CTransitionTarget> popupTransitionTarget = static_cast<CPopup*>(m_tpOverflowPopup.Cast<Popup>()->GetHandle())->EnsureTransitionTarget();

                ctl::ComPtr<xaml::IResourceDictionary> popupResources;
                IFC_RETURN(m_tpOverflowPopup.Cast<Popup>()->get_Resources(&popupResources));

                if (popupResources)
                {
                    ctl::ComPtr<wfc::IMap<IInspectable*, IInspectable*>> resourceMap;
                    IFC_RETURN(popupResources.As(&resourceMap));

                    ctl::ComPtr<IInspectable> resourceKey;
                    IFC_RETURN(PropertyValue::CreateFromString(wrl_wrappers::HStringReference(L"OverflowContentTransform").Get(), &resourceKey));

                    BOOLEAN hasKey = FALSE;
                    IFC_RETURN(resourceMap->HasKey(resourceKey.Get(), &hasKey));
                    if (hasKey)
                    {
                        ctl::ComPtr<IInspectable> resource;
                        IFC_RETURN(resourceMap->Lookup(resourceKey.Get(), &resource));

                        ctl::ComPtr<xaml_media::ICompositeTransform> compositeTransform;
                        if (SUCCEEDED(resource.As(&compositeTransform)))
                        {
                            CCompositeTransform* compositeTransformNoRef = static_cast<CCompositeTransform*>(compositeTransform.Cast<CompositeTransform>()->GetHandle());
                            popupTransitionTarget->ReplaceTransform(compositeTransformNoRef);

                            m_canShadowBeAnimatedByEntranceAnimation = true;
                        }
                    }
                }
            }

            if (!m_canShadowBeAnimatedByEntranceAnimation)
            {
                // Fade in the shadow, otherwise the shadow will be in place and visible as the flyout animates into place.
                ctl::ComPtr<FrameworkElement> wrapperFE(secondaryItemsControlWrapper.AsOrNull<FrameworkElement>());
                CUIElement* wrapperUIE = static_cast<CUIElement*>(wrapperFE->GetHandle());
                wrapperUIE->SetShouldFadeInDropShadow(true);
            }
        }
        else
        {
            // If we don't find the special wrapper element to hold a CommandBar's drop shadow, default back to the OverflowContentRoot.
            // generic.xaml seems to have two CommandBar styles - one with reveal and one without. The one with reveal happens to have
            // the WindowedPopupPadding rectangle element that this wrapper element was added to avoid. The one without reveal doesn't
            // have the WindowedPopupPadding, in which case we can simply apply the shadow to OverflowContentRoot.
            //
            // We also perform a check for OuterOverflowContentRoot/V2 because that element is a signifier for CommandBarFlyout, in which case,
            // we shouldn't apply a shadow to OverflowContentRoot because there's already one on OuterOverflow from the OnApplyTemplate call.
            ctl::ComPtr<xaml::IFrameworkElement> outerOverflowContentRoot;
            IFC_RETURN(GetTemplatePart<xaml::IFrameworkElement>(STR_LEN_PAIR(L"OuterOverflowContentRoot"), outerOverflowContentRoot.ReleaseAndGetAddressOf()));
            ctl::ComPtr<xaml::IFrameworkElement> outerOverflowContentRootV2;
            IFC_RETURN(GetTemplatePart<xaml::IFrameworkElement>(STR_LEN_PAIR(L"OuterOverflowContentRootV2"), outerOverflowContentRootV2.ReleaseAndGetAddressOf()));
            if (!outerOverflowContentRoot && !outerOverflowContentRootV2 && m_tpOverflowContentRoot)
            {
                IFC_RETURN(ApplyElevationEffect(m_tpOverflowContentRoot.AsOrNull<IUIElement>().Get()));

                ctl::ComPtr<FrameworkElement> wrapperFE(m_tpOverflowContentRoot.AsOrNull<FrameworkElement>());
                CUIElement* wrapperUIE = static_cast<CUIElement*>(wrapperFE->GetHandle());
                wrapperUIE->SetShouldFadeInDropShadow(true);
            }
        }
    }

    // If the CommandBar was opened using the expand button, we want to give focus to the first secondary item
    // in order to provide a unified accessible experience.
    if (m_tpOverflowPopup && m_tpSecondaryItemsControlPart && m_openedWithExpandButton)
    {
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> items;
        ctl::ComPtr<wfc::IIterator<IInspectable*>> firstItemIterator;

        IFC_RETURN(m_tpSecondaryItemsControlPart->get_Items(&items));
        IFC_RETURN(items.Cast<ItemCollection>()->First(&firstItemIterator));

        if (firstItemIterator)
        {
            // Since the menu is being opened with the Expand Button, then we want to set focus using the
            // same focus state as was used to interact with the expand button itself.
            CContentRoot* contentRoot = VisualTree::GetContentRootForElement(GetHandle());
            auto focusState = static_cast<xaml::FocusState>(CFocusManager::GetFocusStateFromInputDeviceType(contentRoot->GetInputManager().GetLastInputDeviceType()));

            BOOLEAN hasCurrent = FALSE;
            BOOLEAN succeeded = FALSE;
            IFC_RETURN(firstItemIterator->get_HasCurrent(&hasCurrent));

            while (hasCurrent && !succeeded)
            {
                ctl::ComPtr<IInspectable> firstItem;
                IFC_RETURN(firstItemIterator->get_Current(&firstItem));
                ctl::ComPtr<IControl> itemAsControl = firstItem.AsOrNull<IControl>();
                if (itemAsControl)
                {
                    IFC_RETURN(itemAsControl.Cast<Control>()->Focus(focusState, &succeeded));
                }
                IFC_RETURN(firstItemIterator->MoveNext(&hasCurrent));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::OnClosingImpl(_In_ IInspectable* pArgs)
{
    IFC_RETURN(__super::OnClosingImpl(pArgs));
    IFC_RETURN(CloseSubMenus());

    if (CThemeShadow::IsDropShadowMode())
    {
        ctl::ComPtr<xaml::IFrameworkElement> secondaryItemsControlWrapper;
        IFC_RETURN(GetTemplatePart<xaml::IFrameworkElement>(STR_LEN_PAIR(L"SecondaryItemsControlShadowWrapper"), secondaryItemsControlWrapper.ReleaseAndGetAddressOf()));
        if (secondaryItemsControlWrapper)
        {
            if (m_canShadowBeAnimatedByEntranceAnimation)
            {
                // Do nothing. This version of CommandBar animates the shadow along with the flyout.
            }
            else
            {
                IFC_RETURN(ClearElevationEffect(secondaryItemsControlWrapper.AsOrNull<IUIElement>().Get()));
            }
        }
        else
        {
            ctl::ComPtr<xaml::IFrameworkElement> outerOverflowContentRoot;
            IFC_RETURN(GetTemplatePart<xaml::IFrameworkElement>(STR_LEN_PAIR(L"OuterOverflowContentRoot"), outerOverflowContentRoot.ReleaseAndGetAddressOf()));
            if (!outerOverflowContentRoot && m_tpOverflowContentRoot)
            {
                IFC_RETURN(ClearElevationEffect(m_tpOverflowContentRoot.AsOrNull<IUIElement>().Get()));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::OnClosedImpl(_In_ IInspectable* pArgs)
{
    IFC_RETURN(SetCompactMode(true));

    if (m_tpOverflowPopup)
    {
        IFC_RETURN(m_tpOverflowPopup->put_IsOpen(FALSE));
    }

    // We need to call this last, rather than first (the usual pattern),
    // because this raises an event that apps can use to set IsOpen = true.
    // If that happened before the above, then we would effectively
    // see a sequence of Open -> Open -> Close, instead of
    // Open -> Close -> Open, which gets us into a state where
    // CompactMode is still set to true even when the CommandBar is open,
    // hiding AppBarButton labels and the overflow popup.
    IFC_RETURN(__super::OnClosedImpl(pArgs));

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::EnterImpl(
    _In_ bool bLive,
    _In_ bool bSkipNameRegistration,
    _In_ bool bCoercedIsEnabled,
    _In_ bool bUseLayoutRounding
    )
{
    IFC_RETURN(__super::EnterImpl(bLive, bSkipNameRegistration, bCoercedIsEnabled, bUseLayoutRounding));

    ResetCommandBarElementFocus();

    return S_OK;
}

_Check_return_ HRESULT
CommandBarFactory::GetCurrentBottomCommandBarImpl(_In_ xaml::IXamlRoot* xamlRoot, _Outptr_ xaml_controls::ICommandBar** returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IApplicationBarService> applicationBarService;
    ctl::ComPtr<AppBar> topAppBar;
    ctl::ComPtr<AppBar> bottomAppBar;
    ctl::ComPtr<xaml_controls::ICommandBar> commandBar;
    ctl::ComPtr<xaml::IXamlRoot> ixamlRoot{xamlRoot};
    BOOLEAN isAnyLightDismiss = FALSE;

    IFCPTR(returnValue);
    *returnValue = nullptr;

    IFCEXPECT(ixamlRoot);
    if (ixamlRoot)
    {
        IFC(ixamlRoot.Cast<XamlRoot>()->GetApplicationBarService(applicationBarService));
    }

    if (applicationBarService)
    {
        IFC(applicationBarService->GetTopAndBottomOpenAppBars(&topAppBar, &bottomAppBar, &isAnyLightDismiss));
    }

    if (bottomAppBar && SUCCEEDED(bottomAppBar.As(&commandBar)))
    {
        IFC(commandBar.CopyTo(returnValue));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
CommandBar::SetOverflowStyleAndInputModeOnSecondaryCommand(UINT32 index, bool isItemInOverflow)
{
    ctl::ComPtr<xaml_controls::ICommandBarElement> element;

    IFC_RETURN(m_tpDynamicSecondaryCommands.Get()->GetAt(index, &element));

    if (element)
    {
        IFC_RETURN(SetOverflowStyleUsage(element.Get(), isItemInOverflow));
    }

    DirectUI::InputDeviceType inputType = isItemInOverflow ? m_inputDeviceTypeUsedToOpen : DirectUI::InputDeviceType::None;
    IFC_RETURN(SetInputModeOnSecondaryCommand(index, inputType));

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::SetOverflowStyleUsage(_In_ xaml_controls::ICommandBarElement* element, bool isItemInOverflow)
{
    ctl::ComPtr<xaml_controls::ICommandBarElement> elementAsCommandBar(element);

    auto elementAsOverflow = elementAsCommandBar.AsOrNull<xaml_controls::ICommandBarOverflowElement>();
    if (elementAsOverflow)
    {
        IFC_RETURN(elementAsOverflow->put_UseOverflowStyle(isItemInOverflow));
    }

    return S_OK;
}

_Check_return_ HRESULT CommandBar::UpdateInputDeviceTypeUsedToOpen()
{
    CContentRoot* contentRoot = VisualTree::GetContentRootForElement(GetHandle());
    m_inputDeviceTypeUsedToOpen = contentRoot->GetInputManager().GetLastInputDeviceType();

    UINT32 itemCount = 0;
    IFC_RETURN(m_tpDynamicSecondaryCommands.Get()->get_Size(&itemCount));
    for (UINT32 i = 0; i < itemCount; ++i)
    {
        IFC_RETURN(SetInputModeOnSecondaryCommand(i, m_inputDeviceTypeUsedToOpen));
    }

    return S_OK;
}

_Check_return_ HRESULT CommandBar::SetInputModeOnSecondaryCommand(UINT32 index, DirectUI::InputDeviceType inputType)
{
    ctl::ComPtr<xaml_controls::ICommandBarElement> spElement;
    ctl::ComPtr<xaml_controls::IAppBarButton> spElementAsAppBarButton;
    ctl::ComPtr<xaml_controls::IAppBarToggleButton> spElementAsAppBarToggleButton;

    IFC_RETURN(m_tpDynamicSecondaryCommands.Get()->GetAt(index, &spElement));

    if (spElement)
    {
        // Only AppBarButton and AppBarToggleButton support SetInputMode.
        // We ignore other items such as AppBarSeparator.
        spElementAsAppBarToggleButton = spElement.AsOrNull<xaml_controls::IAppBarToggleButton>();
        if (spElementAsAppBarToggleButton)
        {
            static_cast<AppBarToggleButton*>(spElementAsAppBarToggleButton.Get())->SetInputMode(inputType);
        }

        spElementAsAppBarButton = spElement.AsOrNull<xaml_controls::IAppBarButton>();
        if (spElementAsAppBarButton)
        {
            static_cast<AppBarButton*>(spElementAsAppBarButton.Get())->SetInputMode(inputType);
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::SetOverflowStyleParams()
{
    // We need to check to see if there are any AppBarToggleButtons in the list of secondary commands
    // and inform any AppBarButtons of that fact either way to ensure that
    // their text is always aligned with those of AppBarToggleButtons
    // There's no easy way to get specifically when AppBarToggleButtons are added or removed
    // since ItemRemoved doesn't provide a pointer to the removed item,
    // so we'll just iterate through the whole vector each time it changes to see
    // how many toggle buttons we have. We do a similar check for Icons to ensure
    // space is provided and all items are aligned.
    // The vector will basically never have more than ~10 items in it,
    // so the running time of this loop will be trivial.
    bool hasAppBarToggleButtons = false;
    bool hasAppBarIcons = false;
    bool hasAppBarAcceleratorText = false;

    UINT32 itemCount = 0;
    IFC_RETURN(m_tpDynamicSecondaryCommands.Get()->get_Size(&itemCount));
    for (UINT32 i = 0; i < itemCount; ++i)
    {
        ctl::ComPtr<xaml_controls::ICommandBarElement> element;
        IFC_RETURN(m_tpDynamicSecondaryCommands.Get()->GetAt(i, &element));

        if (element)
        {
            ctl::ComPtr<xaml_controls::IAppBarButton> elementAsAppBarButton;
            ctl::ComPtr<xaml_controls::IAppBarToggleButton> elementAsAppBarToggleButton;
            ctl::ComPtr<xaml_controls::IIconElement> icon;
            wrl_wrappers::HString acceleratorText;

            elementAsAppBarButton = element.AsOrNull<xaml_controls::IAppBarButton>();
            if (elementAsAppBarButton)
            {
                IFC_RETURN(elementAsAppBarButton.Cast<AppBarButton>()->get_Icon(&icon));
                hasAppBarIcons = hasAppBarIcons || icon;

                IFC_RETURN(elementAsAppBarButton.Cast<AppBarButton>()->get_KeyboardAcceleratorTextOverride(acceleratorText.ReleaseAndGetAddressOf()));
                hasAppBarAcceleratorText = hasAppBarAcceleratorText || !WindowsIsStringEmpty(acceleratorText.Get());
            }
            else
            {
                elementAsAppBarToggleButton = element.AsOrNull<xaml_controls::IAppBarToggleButton>();
                if (elementAsAppBarToggleButton)
                {
                    hasAppBarToggleButtons = true;

                    IFC_RETURN(elementAsAppBarToggleButton.Cast<AppBarToggleButton>()->get_Icon(&icon));
                    hasAppBarIcons = hasAppBarIcons || icon;

                    IFC_RETURN(elementAsAppBarToggleButton.Cast<AppBarToggleButton>()->get_KeyboardAcceleratorTextOverride(acceleratorText.ReleaseAndGetAddressOf()));
                    hasAppBarAcceleratorText = hasAppBarAcceleratorText || !WindowsIsStringEmpty(acceleratorText.Get());
                }
            }

            if (hasAppBarIcons && hasAppBarToggleButtons && hasAppBarAcceleratorText)
            {
                break;
            }
        }
    }

    for (UINT32 i = 0; i < itemCount; ++i)
    {
        ctl::ComPtr<xaml_controls::ICommandBarElement> element;
        IFC_RETURN(m_tpDynamicSecondaryCommands.Get()->GetAt(i, &element));

        if (element)
        {
            ctl::ComPtr<xaml_controls::IAppBarButton> elementAsAppBarButton;
            ctl::ComPtr<xaml_controls::IAppBarToggleButton> elementAsAppBarToggleButton;

            elementAsAppBarButton = element.AsOrNull<xaml_controls::IAppBarButton>();
            if (elementAsAppBarButton)
            {
                IFC_RETURN(elementAsAppBarButton.Cast<AppBarButton>()->SetOverflowStyleParams(hasAppBarIcons, hasAppBarToggleButtons, hasAppBarAcceleratorText));
            }
            else
            {
                elementAsAppBarToggleButton = element.AsOrNull<xaml_controls::IAppBarToggleButton>();
                if (elementAsAppBarToggleButton)
                {
                    IFC_RETURN(elementAsAppBarToggleButton.Cast<AppBarToggleButton>()->SetOverflowStyleParams(hasAppBarIcons, hasAppBarAcceleratorText));
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::PropagateDefaultLabelPosition()
{
    UINT32 primaryItemCount = 0;
    IFC_RETURN(m_tpDynamicPrimaryCommands.Get()->get_Size(&primaryItemCount));

    for (UINT32 i = 0; i < primaryItemCount; ++i)
    {
        ctl::ComPtr<xaml_controls::ICommandBarElement> spElement;
        IFC_RETURN(m_tpDynamicPrimaryCommands.Get()->GetAt(i, &spElement));

        if (spElement)
        {
            IFC_RETURN(PropagateDefaultLabelPositionToElement(spElement.Get()));
        }
    }

    UINT32 secondaryItemCount = 0;
    IFC_RETURN(m_tpDynamicSecondaryCommands.Get()->get_Size(&secondaryItemCount));

    for (UINT32 i = 0; i < secondaryItemCount; ++i)
    {
        ctl::ComPtr<xaml_controls::ICommandBarElement> spElement;
        IFC_RETURN(m_tpDynamicSecondaryCommands.Get()->GetAt(i, &spElement));

        if (spElement)
        {
            IFC_RETURN(PropagateDefaultLabelPositionToElement(spElement.Get()));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::PropagateDefaultLabelPositionToElement(xaml_controls::ICommandBarElement *element)
{
    ctl::ComPtr<xaml_controls::ICommandBarElement> spElement(element);

    auto spElementAsLabeledElement = spElement.AsOrNull<xaml_controls::ICommandBarLabeledElement>();
    if (spElementAsLabeledElement)
    {
        xaml_controls::CommandBarDefaultLabelPosition defaultLabelPosition;
        IFC_RETURN(get_DefaultLabelPosition(&defaultLabelPosition))
        IFC_RETURN(spElementAsLabeledElement->SetDefaultLabelPosition(defaultLabelPosition));
    }

    return S_OK;
}

// hasLabelAtPosition is set to True when there is a Visible Dynamic Primary Command with a Label at the provided Bottom or Right position.
_Check_return_ HRESULT
CommandBar::HasLabelAtPosition(_In_ xaml_controls::CommandBarDefaultLabelPosition labelPosition, _Out_ bool* hasLabelAtPosition)
{
    ASSERT(labelPosition == xaml_controls::CommandBarDefaultLabelPosition_Bottom || labelPosition == xaml_controls::CommandBarDefaultLabelPosition_Right);

    *hasLabelAtPosition = false;

    xaml_controls::CommandBarDefaultLabelPosition defaultLabelPosition{};

    IFC_RETURN(get_DefaultLabelPosition(&defaultLabelPosition));

    if (defaultLabelPosition == labelPosition)
    {
        UINT32 primaryItemsCount = 0;

        IFC_RETURN(m_tpDynamicPrimaryCommands.Get()->get_Size(&primaryItemsCount));

        for (UINT32 i = 0; i < primaryItemsCount; ++i)
        {
            ctl::ComPtr<xaml_controls::ICommandBarElement> element;

            IFC_RETURN(m_tpDynamicPrimaryCommands.Get()->GetAt(i, &element));

            const auto elementAsUIE = element.AsOrNull<xaml::IUIElement>();

            if (elementAsUIE)
            {
                xaml::Visibility visibility{};

                IFC_RETURN(elementAsUIE->get_Visibility(&visibility));
                if (visibility == xaml::Visibility_Collapsed)
                {
                    continue;
                }
            }

            const auto elementAsLabeledElement = element.AsOrNull<xaml_controls::ICommandBarLabeledElement>();

            if (elementAsLabeledElement)
            {
                BOOLEAN hasBottomOrRightLabel{};

                if (labelPosition == xaml_controls::CommandBarDefaultLabelPosition_Bottom)
                {
                    IFC_RETURN(elementAsLabeledElement->GetHasBottomLabel(&hasBottomOrRightLabel));
                }
                else
                {
                    IFC_RETURN(elementAsLabeledElement->GetHasRightLabel(&hasBottomOrRightLabel));
                }

                if (hasBottomOrRightLabel)
                {
                    *hasLabelAtPosition = true;
                    break;
                }
            }
        }
    }

    return S_OK;
}

// Used to *reset* overflow style state on items that are leaving
// the secondary items vector.
_Check_return_ HRESULT
CommandBar::NotifyElementVectorChanging(
    _In_ CommandBarElementCollection* pElementCollection,
    _In_ wfc::CollectionChange change,
    _In_ UINT32 changeIndex
    )
{
    // Assume that we get this notification only for secondary commands collection.
    ASSERT(pElementCollection == m_tpSecondaryCommands.Get());

    IFC_RETURN(SetOverflowStyleParams());

    if (change == wfc::CollectionChange_ItemRemoved ||
        change == wfc::CollectionChange_ItemChanged)
    {
        IFC_RETURN(SetOverflowStyleAndInputModeOnSecondaryCommand(changeIndex, false));
    }
    else if (change == wfc::CollectionChange_Reset)
    {
        UINT32 itemCount = 0;
        IFC_RETURN(m_tpSecondaryCommands->get_Size(&itemCount));
        for (UINT32 i = 0; i < itemCount; ++i)
        {
            IFC_RETURN(SetOverflowStyleAndInputModeOnSecondaryCommand(i, false));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::OnUnloaded(_In_ IInspectable* /*pSender*/, _In_ xaml::IRoutedEventArgs* /*pArgs*/)
{
    // Make sure our popup is closed.
    if (m_tpOverflowPopup)
    {
        IFC_RETURN(m_tpOverflowPopup->put_IsOpen(FALSE));
    }

    BOOLEAN isOpen = FALSE;

    // Handles the correct closing for the commandbar even when we don't hit OnClosedImpl when navigating between cached pages.
    IFC_RETURN(get_IsOpen(&isOpen));

    if (!isOpen)
    {
        IFC_RETURN(SetCompactMode(true));
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::OnPrimaryCommandsChanged(
    _In_ wfc::IObservableVector<xaml_controls::ICommandBarElement*>* /*pSender*/,
    _In_ wfc::IVectorChangedEventArgs* pArgs)
{
    IFC_RETURN(ResetDynamicCommands());

    BOOLEAN isOpen = FALSE;
    IFC_RETURN(get_IsOpen(&isOpen));

    BOOLEAN shouldBeCompact = !isOpen;

    wfc::CollectionChange change = wfc::CollectionChange_Reset;
    IFC_RETURN(pArgs->get_CollectionChange(&change));

    UINT32 changeIndex = 0;
    IFC_RETURN(pArgs->get_Index(&changeIndex));

    if (change == wfc::CollectionChange_ItemInserted ||
        change == wfc::CollectionChange_ItemChanged)
    {
        ctl::ComPtr<xaml_controls::ICommandBarElement> element;
        IFC_RETURN(m_tpDynamicPrimaryCommands.Get()->GetAt(changeIndex, &element));
        IFC_RETURN(element->put_IsCompact(shouldBeCompact));
        IFC_RETURN(PropagateDefaultLabelPositionToElement(element.Get()));
    }
    else if (change == wfc::CollectionChange_Reset)
    {
        IFC_RETURN(SetCompactMode(!!shouldBeCompact));

        UINT32 itemCount = 0;
        IFC_RETURN(m_tpDynamicPrimaryCommands.Get()->get_Size(&itemCount));
        for (UINT32 i = 0; i < itemCount; ++i)
        {
            ctl::ComPtr<xaml_controls::ICommandBarElement> element;
            IFC_RETURN(m_tpDynamicPrimaryCommands.Get()->GetAt(i, &element));
            IFC_RETURN(PropagateDefaultLabelPositionToElement(element.Get()));
        }
    }

    IFC_RETURN(InvalidateMeasure());

    IFC_RETURN(UpdateVisualState());

    return S_OK;
}

// Used to *set* overflow style state on items that are entering
// the secondary items vector.
_Check_return_ HRESULT
CommandBar::OnSecondaryCommandsChanged(
    _In_ wfc::IObservableVector<xaml_controls::ICommandBarElement*>* /*pSender*/,
    _In_ wfc::IVectorChangedEventArgs* pArgs
    )
{
    IFC_RETURN(ResetDynamicCommands());

    wfc::CollectionChange change = wfc::CollectionChange_Reset;
    IFC_RETURN(pArgs->get_CollectionChange(&change));

    UINT32 changeIndex = 0;
    IFC_RETURN(pArgs->get_Index(&changeIndex));

    IFC_RETURN(SetOverflowStyleParams());

    if (change == wfc::CollectionChange_ItemInserted ||
        change == wfc::CollectionChange_ItemChanged)
    {
        ctl::ComPtr<xaml_controls::ICommandBarElement> element;
        IFC_RETURN(m_tpDynamicSecondaryCommands.Get()->GetAt(changeIndex, &element));
        IFC_RETURN(SetOverflowStyleAndInputModeOnSecondaryCommand(changeIndex, true));
        IFC_RETURN(PropagateDefaultLabelPositionToElement(element.Get()));
    }
    else if (change == wfc::CollectionChange_Reset)
    {
        UINT32 itemCount = 0;
        IFC_RETURN(m_tpDynamicSecondaryCommands.Get()->get_Size(&itemCount));
        for (UINT32 i = 0; i < itemCount; ++i)
        {
            ctl::ComPtr<xaml_controls::ICommandBarElement> element;
            IFC_RETURN(m_tpDynamicSecondaryCommands.Get()->GetAt(i, &element));
            IFC_RETURN(SetOverflowStyleAndInputModeOnSecondaryCommand(i, true));
            IFC_RETURN(PropagateDefaultLabelPositionToElement(element.Get()));
        }
    }

    IFC_RETURN(InvalidateMeasure());
    IFC_RETURN(UpdateVisualState());
    IFC_RETURN(UpdateTemplateSettings());

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::OnSecondaryItemsControlLoaded(_In_ IInspectable* /*pSender*/, _In_ xaml::IRoutedEventArgs* /*pArgs*/)
{
    // Hook-up a key-down handler to the overflow presenter's items presenter to override the
    // default arrow key behavior of the ScrollViewer, which scrolls the view.  We instead
    // want arrow keys to shift focus up/down.
    if (!m_tpOverflowPresenterItemsPresenter)
    {
        ctl::ComPtr<xaml_controls::IItemsPresenter> overflowPresenterItemsPresenter;
        IFC_RETURN(m_tpSecondaryItemsControlPart.Cast<CommandBarOverflowPresenter>()->GetTemplatePart<xaml_controls::IItemsPresenter>(
            STR_LEN_PAIR(L"ItemsPresenter"),
            overflowPresenterItemsPresenter.ReleaseAndGetAddressOf())
            );
        SetPtrValue(m_tpOverflowPresenterItemsPresenter, overflowPresenterItemsPresenter.Get());

        if (m_tpOverflowPresenterItemsPresenter)
        {
            IFC_RETURN(m_overflowPresenterItemsPresenterKeyDownEventHandler.AttachEventHandler(
                m_tpOverflowPresenterItemsPresenter.Cast<ItemsPresenter>(),
                std::bind(&CommandBar::OnOverflowContentKeyDown, this, _1, _2)
                ));
        }
    }

    // Set focus to a particular item if the overflow was opened by arrowing
    // up/down while focus was on the more button.
    if (m_overflowInitialFocusItem != OverflowInitialFocusItem::None)
    {
        bool ignored = false;
        IFC_RETURN(SetFocusedElementInOverflow(m_overflowInitialFocusItem == OverflowInitialFocusItem::FirstItem, &ignored));

        m_overflowInitialFocusItem = OverflowInitialFocusItem::None;
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::OnOverflowContentRootSizeChanged(_In_ IInspectable* /*sender*/, _In_ xaml::ISizeChangedEventArgs* /*args*/)
{
    if (ctl::is<xaml_controls::ICommandBarOverflowPresenter>(m_tpSecondaryItemsControlPart.Get()))
    {
        bool shouldUseFullWidth = false;
        IFC_RETURN(GetShouldOverflowOpenInFullWidth(&shouldUseFullWidth));

        bool shouldOpenUp;
        IFC_RETURN(GetShouldOpenUp(&shouldOpenUp));

        IFC_RETURN(m_tpSecondaryItemsControlPart.Cast<CommandBarOverflowPresenter>()->SetDisplayModeState(shouldUseFullWidth, shouldOpenUp));
    }

    return UpdateTemplateSettings();
}

_Check_return_ HRESULT
CommandBar::TryDismissCommandBarOverflow()
{
    BOOLEAN isSticky = FALSE;
    IFC_RETURN(get_IsSticky(&isSticky));

    if (!isSticky)
    {
        IFC_RETURN(put_IsOpen(FALSE));
    }

    //In either case (sticky v/s non-sticky) focus should go to the expand button
    IFC_RETURN(RestoreFocusToExpandButton());

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::OnOverflowContentKeyDown(IInspectable* /*pSender*/, xaml_input::IKeyRoutedEventArgs* pArgs)
{
    auto key = wsy::VirtualKey_None;
    auto originalKey = wsy::VirtualKey_None;

    IFC_RETURN(pArgs->get_Key(&key));
    IFC_RETURN(static_cast<KeyRoutedEventArgs*>(pArgs)->get_OriginalKey(&originalKey));

    bool wasHandledLocally = false;

    switch (originalKey)
    {
        case wsy::VirtualKey_GamepadB:
        case wsy::VirtualKey_Escape:
        {
            IFC_RETURN(TryDismissCommandBarOverflow());
            wasHandledLocally = true;
            break;
        }

        case wsy::VirtualKey_GamepadDPadLeft:
        case wsy::VirtualKey_GamepadDPadRight:
        case wsy::VirtualKey_GamepadLeftThumbstickLeft:
        case wsy::VirtualKey_GamepadLeftThumbstickRight:
        case wsy::VirtualKey_Left:
        case wsy::VirtualKey_Right:
        {
            //Mark these keys handled so that Auto-focus doesn't break the focus trap
            wasHandledLocally = true;
            break;
        }

        case wsy::VirtualKey_GamepadDPadUp:
        case wsy::VirtualKey_GamepadLeftThumbstickUp:
        case wsy::VirtualKey_GamepadDPadDown:
        case wsy::VirtualKey_GamepadLeftThumbstickDown:
        {
            IFC_RETURN(ShiftFocusVerticallyInOverflow( key == wsy::VirtualKey_Down ||
                    key == wsy::VirtualKey_GamepadDPadDown ||
                    key == wsy::VirtualKey_GamepadLeftThumbstickDown,
                    false /* allowFocusWrap */));
            wasHandledLocally = true;
            break;
        }

        case wsy::VirtualKey_Up:
        case wsy::VirtualKey_Down:
        {
            IFC_RETURN(ShiftFocusVerticallyInOverflow(key == wsy::VirtualKey_Down));
            wasHandledLocally = true;
            break;
        }

        case wsy::VirtualKey_Tab:
        {
            auto modifierKeys = wsy::VirtualKeyModifiers_None;
            IFC_RETURN(CoreImports::Input_GetKeyboardModifiers(&modifierKeys));

            IFC_RETURN(HandleTabKeyPressedInOverflow((modifierKeys & wsy::VirtualKeyModifiers_Shift) != 0, &wasHandledLocally));
            break;
        }
    }

    if (wasHandledLocally)
    {
        IFC_RETURN(pArgs->put_Handled(TRUE));
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::OnCommandExecutionStatic(_In_ xaml_controls::ICommandBarElement* element)
{
    ctl::ComPtr<xaml_controls::ICommandBar> parentCmdBar;
    IFC_RETURN(FindParentCommandBarForElement(element, &parentCmdBar));

    if (parentCmdBar)
    {
        IFC_RETURN(parentCmdBar.Cast<CommandBar>()->put_IsOpen(FALSE));
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::OnCommandBarElementVisibilityChanged(_In_ xaml_controls::ICommandBarElement* element)
{
    ctl::ComPtr<xaml_controls::ICommandBar> parentCmdBar;
    IFC_RETURN(FindParentCommandBarForElement(element, &parentCmdBar));

    if (parentCmdBar)
    {
        IFC_RETURN(parentCmdBar.Cast<CommandBar>()->UpdateVisualState());
    }

    return S_OK;
}

_Check_return_ HRESULT CommandBar::ContainsElement(_In_ DependencyObject* pElement, _Out_ bool *pContainsElement)
{
    BOOLEAN isAncestorOfElement = FALSE;

    *pContainsElement = false;

    IFC_RETURN(IsAncestorOf(pElement, &isAncestorOfElement));

    if (!isAncestorOfElement && m_tpOverflowContentRoot)
    {
        IFC_RETURN(m_tpOverflowContentRoot.Cast<FrameworkElement>()->IsAncestorOf(pElement, &isAncestorOfElement));
    }

    *pContainsElement = !!isAncestorOfElement;

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::RestoreFocusToExpandButton()
{
    if (m_tpExpandButton)
    {
        ctl::ComPtr<DependencyObject> focusedElement;
        IFC_RETURN(GetFocusedElement(&focusedElement));

        if (focusedElement)
        {
            BOOLEAN isOverflowPopupAncestorOfElement = FALSE;
            if (m_tpOverflowContentRoot)
            {
                IFC_RETURN(m_tpOverflowContentRoot.Cast<FrameworkElement>()->IsAncestorOf(focusedElement.Get(), &isOverflowPopupAncestorOfElement));
            }

            // Focus is in the overflow menu, so restore it to the expand button now that we're
            // closing.
            if (isOverflowPopupAncestorOfElement)
            {
                auto focusState = xaml::FocusState_Unfocused;
                IFC_RETURN(GetFocusState(focusedElement.Get(), &focusState));

                BOOLEAN ignored = FALSE;
                IFC_RETURN(DependencyObject::SetFocusedElement(m_tpExpandButton.Cast<ButtonBase>(), focusState, FALSE /*animateIfBringIntoView*/, &ignored));
            }
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
CommandBar::RestoreSavedFocusImpl(_In_opt_ DependencyObject* savedFocusedElement, xaml::FocusState savedFocusState)
{
    // If we did save focus from a previous element when opening, then defer to the AppBar's
    // implemenation to restore it.  The CommandBar handles the case where there was no
    // saved element and restores focus to the expand button if it was previously in
    // the overflow menu.
    return (savedFocusedElement ? __super::RestoreSavedFocusImpl(savedFocusedElement, savedFocusState)
        : RestoreFocusToExpandButton());
}

//Returns true if the currently focused element
//is a child of CommandBar OR lives in m_tpOverflowContentRoot's sub-tree
_Check_return_ HRESULT
CommandBar::HasFocus(_Out_ BOOLEAN *hasFocus)
{
    IFCPTR_RETURN(hasFocus);
    *hasFocus = FALSE;

    ctl::ComPtr<DependencyObject> focusedElement;
    IFC_RETURN(GetFocusedElement(&focusedElement));

    bool containsElement = false;
    IFC_RETURN(ContainsElement(focusedElement.Get(), &containsElement));

    *hasFocus = containsElement;

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::UpdateTemplateSettings()
{
    ctl::ComPtr<xaml_primitives::ICommandBarTemplateSettings> templateSettings;
    IFC_RETURN(get_CommandBarTemplateSettings(&templateSettings));

    ctl::ComPtr<xaml_primitives::IAppBarTemplateSettings> appBarTemplateSettings;
    IFC_RETURN(get_TemplateSettings(&appBarTemplateSettings));

    BOOLEAN isOpen = FALSE;
    IFC_RETURN(get_IsOpen(&isOpen));
    if (isOpen)
    {
        double contentHeight = GetContentHeight();
        IFC_RETURN(templateSettings.Cast<CommandBarTemplateSettings>()->put_ContentHeight(contentHeight));

        wf::Rect visibleBounds = {};
        wf::Rect availableBounds = {};

        IFC_RETURN(DXamlCore::GetCurrent()->GetVisibleContentBoundsForElement(GetHandle(), &visibleBounds));

        if (m_tpOverflowPopup && m_tpOverflowPopup.Cast<Popup>()->IsWindowed())
        {
            ctl::ComPtr<xaml_media::IGeneralTransform> transform;
            IFC_RETURN(TransformToVisual(nullptr, &transform));

            wf::Point topLeftPoint = { 0, 0 };
            IFC_RETURN(transform->TransformPoint(topLeftPoint, &topLeftPoint));

            IFC_RETURN(DXamlCore::GetCurrent()->CalculateAvailableMonitorRect(this, topLeftPoint, &availableBounds));
        }
        else
        {
            availableBounds = visibleBounds;
        }

        bool shouldUseFullWidth = false;
        IFC_RETURN(GetShouldOverflowOpenInFullWidth(&shouldUseFullWidth));

        double overflowContentMaxWidth = m_overflowContentMaxWidth;
        double overflowContentMinWidth;
        if (shouldUseFullWidth)
        {
            overflowContentMinWidth = visibleBounds.Width;
            overflowContentMaxWidth = visibleBounds.Width;
        }
        else if (m_inputDeviceTypeUsedToOpen == DirectUI::InputDeviceType::Touch ||
                 m_inputDeviceTypeUsedToOpen == DirectUI::InputDeviceType::GamepadOrRemote)
        {
            overflowContentMinWidth = m_overflowContentTouchMinWidth;
        }
        else
        {
            overflowContentMinWidth = m_overflowContentMinWidth;
        }

        IFC_RETURN(templateSettings.Cast<CommandBarTemplateSettings>()->put_OverflowContentMinWidth(overflowContentMinWidth));
        IFC_RETURN(templateSettings.Cast<CommandBarTemplateSettings>()->put_OverflowContentMaxWidth(overflowContentMaxWidth));

        double overflowContentMaxHeight = availableBounds.Height * 0.5;

        // When we're using a windowed popup, we add an extra margin to provide
        // enough space to do translate transforms that our animations need.
        // We'll add its height to the max height.
        if (m_tpWindowedPopupPadding)
        {
            DOUBLE windowedPopupPaddingHeight;
            IFC_RETURN(m_tpWindowedPopupPadding->get_ActualHeight(&windowedPopupPaddingHeight));
            overflowContentMaxHeight += windowedPopupPaddingHeight;
        }

        IFC_RETURN(templateSettings.Cast<CommandBarTemplateSettings>()->put_OverflowContentMaxHeight(overflowContentMaxHeight));

        wf::Size overflowContentSize = {};
        IFC_RETURN(GetOverflowContentSize(&overflowContentSize));

        IFC_RETURN(templateSettings.Cast<CommandBarTemplateSettings>()->put_OverflowContentClipRect({ 0, 0, overflowContentSize.Width, overflowContentSize.Height - (m_tpWindowedPopupPadding ? static_cast<float>(contentHeight) : 0) }));

        DOUBLE compactVerticalDelta;
        IFC_RETURN(appBarTemplateSettings->get_CompactVerticalDelta(&compactVerticalDelta));

        DOUBLE minimalVerticalDelta;
        IFC_RETURN(appBarTemplateSettings->get_MinimalVerticalDelta(&minimalVerticalDelta));

        DOUBLE hiddenVerticalDelta;
        IFC_RETURN(appBarTemplateSettings->get_HiddenVerticalDelta(&hiddenVerticalDelta));

        IFC_RETURN(templateSettings.Cast<CommandBarTemplateSettings>()->put_OverflowContentCompactYTranslation(-overflowContentSize.Height + compactVerticalDelta));
        IFC_RETURN(templateSettings.Cast<CommandBarTemplateSettings>()->put_OverflowContentMinimalYTranslation(-overflowContentSize.Height + minimalVerticalDelta));
        IFC_RETURN(templateSettings.Cast<CommandBarTemplateSettings>()->put_OverflowContentHiddenYTranslation(-overflowContentSize.Height + hiddenVerticalDelta));

        double contentHeightForAnimation = overflowContentSize.Height;

        // If the overflow popup is windowed, we'll have already accounted for the size of the
        // main content in terms of transformation, so we'll only animate the remainder.
        if (m_tpOverflowPopup && m_tpOverflowPopup.Cast<Popup>()->IsWindowed())
        {
            auto closedDisplayMode = xaml_controls::AppBarClosedDisplayMode_Hidden;
            IFC_RETURN(get_ClosedDisplayMode(&closedDisplayMode));

            switch (closedDisplayMode)
            {
            case xaml_controls::AppBarClosedDisplayMode_Compact:
                contentHeightForAnimation -= GetCompactHeight();
                break;

            case xaml_controls::AppBarClosedDisplayMode_Minimal:
                contentHeightForAnimation -= GetMinimalHeight();
                break;

            case xaml_controls::AppBarClosedDisplayMode_Hidden:
            default:
                // The hidden height is zero, so nothing to do here.
                break;
            }
        }

        IFC_RETURN(templateSettings.Cast<CommandBarTemplateSettings>()->put_OverflowContentHeight(contentHeightForAnimation));
        IFC_RETURN(templateSettings.Cast<CommandBarTemplateSettings>()->put_NegativeOverflowContentHeight(-contentHeightForAnimation));

        double overflowContentHorizontalOffset = 0.0;
        IFC_RETURN(CalculateOverflowContentHorizontalOffset(overflowContentSize, availableBounds, &overflowContentHorizontalOffset));
        IFC_RETURN(templateSettings.Cast<CommandBarTemplateSettings>()->put_OverflowContentHorizontalOffset(overflowContentHorizontalOffset));
    }

    IFC_RETURN(__super::UpdateTemplateSettings());

    xaml_controls::CommandBarOverflowButtonVisibility overflowButtonVisibility;
    IFC_RETURN(get_OverflowButtonVisibility(&overflowButtonVisibility));

    bool shouldShowOverflowButton = false;

    if (overflowButtonVisibility == xaml_controls::CommandBarOverflowButtonVisibility_Visible)
    {
        shouldShowOverflowButton = true;
    }
    else if (overflowButtonVisibility == xaml_controls::CommandBarOverflowButtonVisibility_Auto)
    {
        // In the auto case, we should show the overflow button in one of four circumstances:
        // - when we have at least one element in the secondary items collection, 
        // - or when there is a delta between the compact height and the height of the CommandBar, 
        // - or when there is a Visible ICommandBarLabeledElement Primary Command with a Bottom Label,
        // - or when our closed display mode is something other than compact.
        UINT32 secondaryItemsCount = 0;

        IFC_RETURN(m_tpDynamicSecondaryCommands.Get()->get_Size(&secondaryItemsCount));

        if (secondaryItemsCount > 0)
        {
            shouldShowOverflowButton = true;
        }
        else
        {
            auto closedDisplayMode = xaml_controls::AppBarClosedDisplayMode_Compact;
            IFC_RETURN(get_ClosedDisplayMode(&closedDisplayMode));

            if (closedDisplayMode != xaml_controls::AppBarClosedDisplayMode_Compact)
            {
                shouldShowOverflowButton = true;
            }
            else
            {
                DOUBLE compactVerticalDelta;
                IFC_RETURN(appBarTemplateSettings->get_CompactVerticalDelta(&compactVerticalDelta));

                if (!DoubleUtil::IsZero(compactVerticalDelta))
                {
                    shouldShowOverflowButton = true;
                }
                else
                {
                    bool hasBottomLabel{};

                    IFC_RETURN(HasLabelAtPosition(xaml_controls::CommandBarDefaultLabelPosition_Bottom, &hasBottomLabel));

                    if (hasBottomLabel)
                    {
                        shouldShowOverflowButton = true;
                    }
                }
            }
        }
    }

    IFC_RETURN(templateSettings.Cast<CommandBarTemplateSettings>()->put_EffectiveOverflowButtonVisibility(
        shouldShowOverflowButton ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));

    double maxAppBarKeyboardAcceleratorTextWidth = 0;

    UINT32 itemCount = 0;
    IFC_RETURN(m_tpDynamicSecondaryCommands.Get()->get_Size(&itemCount));
    for (UINT32 i = 0; i < itemCount; ++i)
    {
        ctl::ComPtr<xaml_controls::ICommandBarElement> element;
        IFC_RETURN(m_tpDynamicSecondaryCommands.Get()->GetAt(i, &element));

        if (element)
        {
            wf::Size desiredSize = {};
            double desiredWidth = 0;

            ctl::ComPtr<xaml_controls::IAppBarButton> elementAsAppBarButton;
            ctl::ComPtr<xaml_controls::IAppBarToggleButton> elementAsAppBarToggleButton;

            elementAsAppBarButton = element.AsOrNull<xaml_controls::IAppBarButton>();
            if (elementAsAppBarButton)
            {
                AppBarButtonHelpers::GetKeyboardAcceleratorTextDesiredSize(elementAsAppBarButton.Cast<AppBarButton>(), &desiredSize);
            }
            else
            {
                elementAsAppBarToggleButton = element.AsOrNull<xaml_controls::IAppBarToggleButton>();
                if (elementAsAppBarToggleButton)
                {
                    AppBarButtonHelpers::GetKeyboardAcceleratorTextDesiredSize(elementAsAppBarToggleButton.Cast<AppBarToggleButton>(), &desiredSize);
                }
            }

            desiredWidth = static_cast<double>(desiredSize.Width);

            if (desiredWidth > maxAppBarKeyboardAcceleratorTextWidth)
            {
                maxAppBarKeyboardAcceleratorTextWidth = desiredWidth;
            }
        }
    }

    for (UINT32 i = 0; i < itemCount; ++i)
    {
        ctl::ComPtr<xaml_controls::ICommandBarElement> element;
        IFC_RETURN(m_tpDynamicSecondaryCommands.Get()->GetAt(i, &element));

        if (element)
        {
            ctl::ComPtr<xaml_controls::IAppBarButton> elementAsAppBarButton;
            ctl::ComPtr<xaml_controls::IAppBarToggleButton> elementAsAppBarToggleButton;

            elementAsAppBarButton = element.AsOrNull<xaml_controls::IAppBarButton>();
            if (elementAsAppBarButton)
            {
                AppBarButtonHelpers::UpdateTemplateSettings(elementAsAppBarButton.Cast<AppBarButton>(), maxAppBarKeyboardAcceleratorTextWidth);
            }
            else
            {
                elementAsAppBarToggleButton = element.AsOrNull<xaml_controls::IAppBarToggleButton>();
                if (elementAsAppBarToggleButton)
                {
                    AppBarButtonHelpers::UpdateTemplateSettings(elementAsAppBarToggleButton.Cast<AppBarToggleButton>(), maxAppBarKeyboardAcceleratorTextWidth);
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CommandBar::GetShouldOverflowOpenInFullWidth(_Out_ bool* shouldOpenInFullWidth)
{
    wf::Rect visibleBounds = {};
    IFC_RETURN(DXamlCore::GetCurrent()->GetVisibleContentBoundsForElement(GetHandle(), &visibleBounds));

    *shouldOpenInFullWidth = visibleBounds.Width <= m_overflowContentMaxWidth;

    return S_OK;
}

bool CommandBar::HasPrimaryCommands()
{
    if (m_tpPrimaryCommands && !static_cast<CDOCollection*>(m_tpPrimaryCommands.GetAsCoreDO())->empty())
    {
        return true;
    }

    if (m_tpDynamicPrimaryCommands && !static_cast<CDOCollection*>(m_tpDynamicPrimaryCommands.GetAsCoreDO())->empty())
    {
        return true;
    }

    return false;
}

//
// When a CommandBar opens, there are two things that change:
// 1. The primary commands (if any) expand. This can open upwards or downwards.
// 2. The overflow commands (if any) open a (sometimes windowed) popup. This can also open upwards or downwards.
//
// This function checks that there is room for both things.
//
_Check_return_ HRESULT
CommandBar::GetShouldOpenUp(bool* shouldOpenUp)
{
    // Bottom appbars always open up.  All other appbars by default open down.
    *shouldOpenUp = (GetMode() == AppBarMode_Bottom);

    if (GetMode() == AppBarMode_Inline)
    {
        bool appBarHasSpaceToOpenDown = true;
        bool overflowPopupHasSpaceToOpenDown = true;

        // If there are primary commands, then the inline AppBar will open along with the overflow popup. Check whether
        // there's space for the AppBar to open downwards. If there are no primary commands then the overflow popup
        // alone decides whether we can open downwards.
        if (HasPrimaryCommands())
        {
            IFC_RETURN(HasSpaceForAppBarToOpenDown(&appBarHasSpaceToOpenDown));
        }

        IFC_RETURN(HasSpaceForOverflowPopupToOpenDown(&overflowPopupHasSpaceToOpenDown));

        // Since we open down by default, we'll open up only if we *don't* have space in the down direction for either component.
        *shouldOpenUp = !appBarHasSpaceToOpenDown || !overflowPopupHasSpaceToOpenDown;
    }

    return S_OK;
}

// hasRightLabelDynamicPrimaryCommand is set to True when there is a Visible Dynamic Primary Command that is an ICommandBarLabeledElement with a Right Label.
_Check_return_ HRESULT CommandBar::HasRightLabelDynamicPrimaryCommand(_Out_ bool* hasRightLabelDynamicPrimaryCommand)
{
    IFC_RETURN(HasLabelAtPosition(xaml_controls::CommandBarDefaultLabelPosition_Right, hasRightLabelDynamicPrimaryCommand));

    return S_OK;
}


// hasNonLabeledDynamicPrimaryCommand is set to True when there is a Visible Dynamic Primary Command that is not an AppBarSeparator, 
// and not an ICommandBarLabeledElement (i.e. it is something like an AppBarElementContainer).
_Check_return_ HRESULT CommandBar::HasNonLabeledDynamicPrimaryCommand(_Out_ bool* hasNonLabeledDynamicPrimaryCommand)
{
    *hasNonLabeledDynamicPrimaryCommand = false;

    UINT32 primaryItemsCount = 0;

    IFC_RETURN(m_tpDynamicPrimaryCommands.Get()->get_Size(&primaryItemsCount));

    for (UINT32 i = 0; i < primaryItemsCount; ++i)
    {
        ctl::ComPtr<xaml_controls::ICommandBarElement> element;

        IFC_RETURN(m_tpDynamicPrimaryCommands.Get()->GetAt(i, &element));

        const auto elementAsUIE = element.AsOrNull<xaml::IUIElement>();

        if (elementAsUIE)
        {
            xaml::Visibility visibility{};

            IFC_RETURN(elementAsUIE->get_Visibility(&visibility));
            if (visibility == xaml::Visibility_Collapsed)
            {
                continue;
            }
        }

        const auto elementAsLabeledElement = element.AsOrNull<xaml_controls::ICommandBarLabeledElement>();

        if (elementAsLabeledElement)
        {
            continue;
        }

        const auto elementAsSeparator = element.AsOrNull<xaml_controls::IAppBarSeparator>();

        if (elementAsSeparator)
        {
            continue;
        }
        
        *hasNonLabeledDynamicPrimaryCommand = true;
        break;
    }

    return S_OK;
}

_Check_return_ HRESULT CommandBar::HasSpaceForOverflowPopupToOpenDown(bool* hasSpace)
{
    ASSERT(GetMode() == AppBarMode_Inline);

    ctl::ComPtr<xaml_media::IGeneralTransform> transform;
    IFC_RETURN(TransformToVisual(nullptr, &transform));

    ctl::ComPtr<xaml_primitives::ICommandBarTemplateSettings> templateSettings;
    IFC_RETURN(get_CommandBarTemplateSettings(&templateSettings));

    double overflowContentHeight = 0.0;
    IFC_RETURN(templateSettings->get_OverflowContentHeight(&overflowContentHeight));

    // When opening down, the overflow popup is lined up with the bottom of the expanded AppBar.
    double combinedHeights = GetContentHeight() + overflowContentHeight;

    // We open windowed if our popup is windowed.
    const bool opensWindowed = m_tpOverflowPopup && !!m_tpOverflowPopup.Cast<Popup>()->IsWindowed();

    // Transform the bottom of the overflow popup to Xaml island coordinates
    wf::Point bottomOfOverflowPopup = {};
    IFC_RETURN(transform->TransformPoint({ 0, (float)(combinedHeights) }, &bottomOfOverflowPopup));

    wf::Rect layoutBounds = {};

    if (opensWindowed)
    {
        wf::Point topLeftPoint = { 0, 0 };
        IFC_RETURN(transform->TransformPoint(topLeftPoint, &topLeftPoint));

        IFC_RETURN(DXamlCore::GetCurrent()->CalculateAvailableMonitorRect(this, topLeftPoint, &layoutBounds));
    }
    else
    {
        wf::Rect windowBounds = {};
        IFC_RETURN(DXamlCore::GetCurrent()->GetContentBoundsForElement(GetHandle(), &windowBounds));
        IFC_RETURN(DXamlCore::GetCurrent()->GetContentLayoutBoundsForElement(GetHandle(), &layoutBounds));

        // Convert the layout bounds X/Y offsets from screen coordinates into window coordinates.
        layoutBounds.X -= windowBounds.X;
        layoutBounds.Y -= windowBounds.Y;
    }

    *hasSpace = (bottomOfOverflowPopup.Y <= layoutBounds.Y + layoutBounds.Height);
    return S_OK;
}

_Check_return_ HRESULT
CommandBar::GetOverflowContentSize(_Out_ wf::Size* overfowContentSize)
{
    overfowContentSize->Width = 0;
    overfowContentSize->Height = 0;

    if (m_tpOverflowContentRoot)
    {
        bool hasVisibleSecondaryElements;
        IFC_RETURN(HasVisibleElements(m_tpDynamicSecondaryCommands.Get(), &hasVisibleSecondaryElements));

        // Only measure the overflow content control if it should be visible, otherwise we will be
        // unnecessarily expanding the content control template underneath it during the measure pass.
        if (hasVisibleSecondaryElements)
        {
            IFC_RETURN(m_tpOverflowContentRoot.Cast<FrameworkElement>()->Measure({ XFLOAT_INF, XFLOAT_INF }));
            IFC_RETURN(m_tpOverflowContentRoot.Cast<FrameworkElement>()->get_DesiredSize(overfowContentSize));
        }
    }

    return S_OK;
}

// Calculate OverflowContentHorizontalOffset.
// By default align the menu with the right edge of the CommandBar.
// If there is not enough space to flow to the right, align it with the left edge.
// If it still can't fit by aligning left, then align with the edge of the screen.
_Check_return_ HRESULT
CommandBar::CalculateOverflowContentHorizontalOffset(
    wf::Size overflowContentSize,
    wf::Rect visibleBounds,
    _Out_ double* horizontalOffset
    )
{
    double offset = 0.0;

    ctl::ComPtr<xaml_media::IGeneralTransform> transform;
    IFC_RETURN(TransformToVisual(nullptr, &transform));

    wf::Point offsetFromRoot = {};
    IFC_RETURN(transform->TransformPoint({ 0, 0 }, &offsetFromRoot));

    double actualWidth = 0.0;
    IFC_RETURN(get_ActualWidth(&actualWidth));

    // Try to align with the right edge by default.
    offset = actualWidth - overflowContentSize.Width;
    if (offset < 0)
    {
        // If the offset is negative, then the overflow menu is bigger than the CommandBar
        // so we have to make sure it doesn't flow off the edge of the window.

        auto flowDirection = xaml::FlowDirection_LeftToRight;
        IFC_RETURN(get_FlowDirection(&flowDirection));

        // For LTR, if the sum of the offsets is negative, then the menu is flowing over
        // the edge of the window.
        // For RTL, test the difference of the offsets to see if it's greater than
        // the window width.
        if (((flowDirection == xaml::FlowDirection_LeftToRight) && ((offsetFromRoot.X + offset) < 0))
            || ((flowDirection == xaml::FlowDirection_RightToLeft) && ((offsetFromRoot.X - offset) > visibleBounds.Width)))
        {
            // If we can align it with the left of the bar, then do that.
            // Otherwise, align it with the left edge of the window.
            if (((flowDirection == xaml::FlowDirection_LeftToRight) && (offsetFromRoot.X + overflowContentSize.Width <= visibleBounds.Width))
                || ((flowDirection == xaml::FlowDirection_RightToLeft) && (offsetFromRoot.X - overflowContentSize.Width >= 0)))
            {
                // We can fit it by aligning left, so do that by setting the offset to 0.
                offset = 0;
            }
            else
            {
                // Align to the window edge.
                offset = (flowDirection == xaml::FlowDirection_LeftToRight ? -offsetFromRoot.X : offsetFromRoot.X - visibleBounds.Width);
            }
        }
    }

    *horizontalOffset = offset;

    return S_OK;
}

static _Check_return_ HRESULT DoCollectionOperation(
    _In_ CommandBarElementCollection* collection,
    _In_ DeferredElementStateChange state,
    _In_ UINT32 collectionIndex,
    _In_ CDependencyObject* realizedElement)
{
    ctl::ComPtr<xaml_controls::ICommandBarElement> realizedElementAsICBE;
    ctl::ComPtr<DependencyObject> realizedElementDO;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(realizedElement, &realizedElementDO));
    IFC_RETURN(realizedElementDO.As(&realizedElementAsICBE));

    switch (state)
    {
        case DeferredElementStateChange::Realized:
            IFC_RETURN(collection->InsertAt(collectionIndex, realizedElementAsICBE.Get()));
            break;

        case DeferredElementStateChange::Deferred:
            {
                UINT index = 0;
                BOOLEAN found = FALSE;

                IFC_RETURN(collection->IndexOf(
                    realizedElementAsICBE.Get(),
                    &index,
                    &found));

                if (found)
                {
                    IFC_RETURN(collection->RemoveAt(index));
                }
                // if not found, it's ok.  It might have been removed programatically.
            }
            break;

        default:
            ASSERT(false);
    }

    return S_OK;
}

_Check_return_ HRESULT CommandBar::NotifyDeferredElementStateChanged(
    _In_ KnownPropertyIndex propertyIndex,
    _In_ DeferredElementStateChange state,
    _In_ UINT32 collectionIndex,
    _In_ CDependencyObject* realizedElement)
{
    switch (propertyIndex)
    {
        case KnownPropertyIndex::CommandBar_PrimaryCommands:
            IFC_RETURN(DoCollectionOperation(
                m_tpPrimaryCommands.Get(),
                state,
                collectionIndex,
                realizedElement));
            break;

        case KnownPropertyIndex::CommandBar_SecondaryCommands:
            IFC_RETURN(DoCollectionOperation(
                m_tpSecondaryCommands.Get(),
                state,
                collectionIndex,
                realizedElement));
            break;

        default:
            // Should not be calling framework for any other properties
            ASSERT(false);
            break;
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::HasVisibleElements(_In_ CommandBarElementCollection* collection, bool* hasVisibleElements)
{
    *hasVisibleElements = false;

    UINT32 size = 0;
    IFC_RETURN(collection->get_Size(&size));
    for (UINT32 i = 0; i < size; ++i)
    {
        ctl::ComPtr<xaml_controls::ICommandBarElement> element;
        IFC_RETURN(collection->GetAt(i, &element));
        if (element)
        {
            auto elementAsUIE = ctl::query_interface_cast<xaml::IUIElement>(element.Get());
            if (elementAsUIE)
            {
                auto visibility = xaml::Visibility_Collapsed;
                IFC_RETURN(elementAsUIE->get_Visibility(&visibility));
                if (visibility == xaml::Visibility_Visible)
                {
                    *hasVisibleElements = true;
                    break;
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::FindParentCommandBarForElement(
    _In_ xaml_controls::ICommandBarElement* element,
    _Outptr_ xaml_controls::ICommandBar** parentCmdBar
    )
{
    *parentCmdBar = nullptr;
    ctl::ComPtr<xaml::IDependencyObject> elementDO = ctl::query_interface_cast<xaml::IDependencyObject>(element);
    ASSERT(elementDO);

    ctl::ComPtr<xaml_controls::ICommandBar> parentCommandBar;
    ctl::ComPtr<xaml_controls::IItemsControl> itemsControl;
    IFC_RETURN(ItemsControl::ItemsControlFromItemContainer(elementDO.Get(), &itemsControl));
    if (itemsControl)
    {
        ctl::ComPtr<DependencyObject> templatedParent;
        IFC_RETURN(itemsControl.Cast<ItemsControl>()->get_TemplatedParent(&templatedParent));
        parentCommandBar = templatedParent.AsOrNull<xaml_controls::ICommandBar>();
    }

    // If an element is collapsed initially, it isn't placed in the visual tree of its ItemsControl,
    // meaning that its parent will instead be its logical parent.  To account for that circumstance,
    // we'll explicitly walk the tree to find the parent command bar if we haven't found it yet.
    if (!parentCommandBar)
    {
        CDependencyObject* currentElement = elementDO.Cast<DependencyObject>()->GetHandle();

        while (currentElement)
        {
            if (currentElement->GetTypeIndex() == KnownTypeIndex::CommandBar)
            {
                ctl::ComPtr<DependencyObject> peer;

                IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(currentElement, &peer));
                IFC_RETURN(peer.As(&parentCommandBar));
                break;
            }

            currentElement = currentElement->GetParent();
        }
    }

    if (parentCommandBar)
    {
        *parentCmdBar = parentCommandBar.Detach();
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::FindMovablePrimaryCommands(
    _In_ double availablePrimaryCommandsWidth,
    _In_ double primaryItemsControlDesiredWidth,
    _Out_ UINT32* primaryCommandsCountInTransition)
{
    bool canProcessDynamicOverflowOrder = false;

    // Find the movable primary commands by looking the DynamicOverflowOrder property.
    IFC_RETURN(FindMovablePrimaryCommandsFromOrderSet(
        availablePrimaryCommandsWidth,
        primaryItemsControlDesiredWidth,
        primaryCommandsCountInTransition,
        canProcessDynamicOverflowOrder));

    // Find the movable primary commands as the default behavior that move the right most command from the near the More options button
    if (!canProcessDynamicOverflowOrder)
    {
        // Get the movable primary commands transition candidates from the primary commands collection to
        // the overflow collection. The dynamic overflow moving order is based on the right most commands
        // in the near the "More options" button.

        UINT32 dynamicPrimaryCount = 0;

        IFC_RETURN(m_tpDynamicPrimaryCommands->get_Size(&dynamicPrimaryCount));

        ASSERT(dynamicPrimaryCount > 0);

        UINT32 moveStartIndex = dynamicPrimaryCount - 1;

        *primaryCommandsCountInTransition = 0;

        // Find out the move starting index from the right most primary command element
        for (UINT32 i = moveStartIndex; i > 0; i--)
        {
            ctl::ComPtr<xaml_controls::ICommandBarElement> element;
            ctl::ComPtr<xaml::IUIElement> elementAsUiE;
            wf::Size elementDesiredSize = {};

            IFC_RETURN(m_tpDynamicPrimaryCommands->GetAt(i, &element));

            if (element)
            {
                IFC_RETURN(element.As(&elementAsUiE));
                IFC_RETURN(elementAsUiE->get_DesiredSize(&elementDesiredSize));
            }

            primaryItemsControlDesiredWidth -= elementDesiredSize.Width;

            if (primaryItemsControlDesiredWidth < availablePrimaryCommandsWidth)
            {
                break;
            }

            moveStartIndex--;
        }

        // Insert the movable primary commands candidate into the transition commands collection
        IFC_RETURN(m_tpPrimaryCommandsInTransition->Clear());
        for (UINT32 i = moveStartIndex; i < dynamicPrimaryCount; i++)
        {
            ctl::ComPtr<xaml_controls::ICommandBarElement> element;

            IFC_RETURN(m_tpDynamicPrimaryCommands->GetAt(i, &element));
            IFC_RETURN(m_tpPrimaryCommandsInTransition->InsertAt((*primaryCommandsCountInTransition)++, element.Get()));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::FindMovablePrimaryCommandsFromOrderSet(
    _In_ double availablePrimaryCommandsWidth,
    _In_ double primaryItemsControlDesiredWidth,
    _Out_ UINT32* primaryCommandsCountInTransition,
    _Inout_ bool& canProcessDynamicOverflowOrder)
{
    bool shouldFindMovableOrderSet = false;
    UINT32 dynamicPrimaryCount = 0;
    INT32 dynamicOverflowOrder = 0;
    INT32 firstMovableOrder = 0;
    INT32 previousFirstMovableOrder = 0;

    canProcessDynamicOverflowOrder = false;

    // Find the movable primary command by looking DynamicOverflowOrder property.
    //
    // If the DynamicOverflowOrder property is set,
    //      1. Find the first movable order command that isn't zero value set.
    //      2. Find the all movable primary commands that has the same order value.
    //      3. Find the movable separators if it needs to move with moving primary command.
    //      4. Keep look for the next movable order set in primary commands
    //      5. Look the default movable commands if it still need to move more primary commands to overflow.
    do
    {
        bool isSetMovableCandidateOrder = false;

        shouldFindMovableOrderSet = false;

        IFC_RETURN(m_tpDynamicPrimaryCommands->get_Size(&dynamicPrimaryCount));

        // Find the first movable order set on the dynamic primary commands
        for (UINT32 i = 0; i < dynamicPrimaryCount; i++)
        {
            ctl::ComPtr<xaml_controls::ICommandBarElement> element;

            IFC_RETURN(m_tpDynamicPrimaryCommands->GetAt(i, &element));

            if (element)
            {
                IFC_RETURN(element->get_DynamicOverflowOrder(&dynamicOverflowOrder));

                if (dynamicOverflowOrder > 0 && dynamicOverflowOrder > previousFirstMovableOrder)
                {
                    if (!isSetMovableCandidateOrder)
                    {
                        firstMovableOrder = dynamicOverflowOrder;
                        isSetMovableCandidateOrder = true;
                    }
                    else
                    {
                        firstMovableOrder = (INT32)DoubleUtil::Min(dynamicOverflowOrder, firstMovableOrder);
                    }
                    shouldFindMovableOrderSet = true;
                }
            }
        }

        // Find the first movable commands that has the same order value
        if (shouldFindMovableOrderSet && firstMovableOrder > previousFirstMovableOrder)
        {
            if (!canProcessDynamicOverflowOrder)
            {
                canProcessDynamicOverflowOrder = true;
                IFC_RETURN(m_tpPrimaryCommandsInTransition->Clear());
            }

            for (UINT32 i = 0; i < dynamicPrimaryCount; i++)
            {
                ctl::ComPtr<xaml_controls::ICommandBarElement> element;

                IFC_RETURN(m_tpDynamicPrimaryCommands->GetAt(i, &element));

                if (element)
                {

                    IFC_RETURN(element->get_DynamicOverflowOrder(&dynamicOverflowOrder));

                    if (dynamicOverflowOrder > 0 && dynamicOverflowOrder == firstMovableOrder)
                    {
                        IFC_RETURN(InsertPrimaryCommandToPrimaryCommandsInTransition(i, primaryCommandsCountInTransition, &primaryItemsControlDesiredWidth));

                        // Find the movable separators in backward direction by moving the primary command together
                        if (i > 0)
                        {
                            IFC_RETURN(FindMovableSeparatorsInBackwardDirection(i, primaryCommandsCountInTransition, &primaryItemsControlDesiredWidth));
                        }

                        // Find the movable separators in forward direction by moving the primary command together
                        if (i < dynamicPrimaryCount - 1)
                        {
                            IFC_RETURN(FindMovableSeparatorsInForwardDirection(i, primaryCommandsCountInTransition, &primaryItemsControlDesiredWidth));
                        }

                        if (primaryItemsControlDesiredWidth < availablePrimaryCommandsWidth)
                        {
                            shouldFindMovableOrderSet = false;
                        }
                    }
                }
            }
        }

        previousFirstMovableOrder = firstMovableOrder;

        // Keep looking for the next movable order set primary commands
    } while (shouldFindMovableOrderSet != false);

    if (canProcessDynamicOverflowOrder && primaryItemsControlDesiredWidth > availablePrimaryCommandsWidth)
    {
        // Keep looking for the movable primary commands that doesn't set the order
        for (UINT32 i = dynamicPrimaryCount; i > 0; i--)
        {
            ctl::ComPtr<xaml_controls::ICommandBarElement> element;

            IFC_RETURN(m_tpDynamicPrimaryCommands->GetAt(i - 1, &element));

            if (element)
            {
                IFC_RETURN(element->get_DynamicOverflowOrder(&dynamicOverflowOrder));

                if (dynamicOverflowOrder == 0)
                {
                    IFC_RETURN(InsertPrimaryCommandToPrimaryCommandsInTransition(i - 1, primaryCommandsCountInTransition, &primaryItemsControlDesiredWidth));

                    if (primaryItemsControlDesiredWidth < availablePrimaryCommandsWidth)
                    {
                        break;
                    }
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::InsertPrimaryCommandToPrimaryCommandsInTransition(
    _In_ UINT32 indexMovingPrimaryCommand,
    _Inout_ UINT32* primaryCommandsCountInTransition,
    _Inout_ double* primaryItemsControlDesiredWidth)
{
    ctl::ComPtr<xaml_controls::ICommandBarElement> element;
    ctl::ComPtr<xaml::IUIElement> elementAsUiE;
    wf::Size elementDesiredSize = {};

    IFC_RETURN(m_tpDynamicPrimaryCommands->GetAt(indexMovingPrimaryCommand, &element));

    if (element)
    {
        IFC_RETURN(m_tpPrimaryCommandsInTransition->InsertAt((*primaryCommandsCountInTransition)++, element.Get()));

        IFC_RETURN(element.As(&elementAsUiE));
        IFC_RETURN(elementAsUiE->get_DesiredSize(&elementDesiredSize));

        *(primaryItemsControlDesiredWidth) -= elementDesiredSize.Width;
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::UpdateOverflowButtonAutomationSetNumbers(INT sizeOfSet)
{
    ASSERT(m_tpExpandButton);

    m_automationSizeOfSet = sizeOfSet;

    auto expandButtonAsDO = m_tpExpandButton.AsOrNull<DependencyObject>();

    IFC_RETURN(expandButtonAsDO->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_SizeOfSet, sizeOfSet));
    IFC_RETURN(expandButtonAsDO->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_PositionInSet, sizeOfSet));

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::UpdatePrimaryCommandElementMinWidthInOverflow()
{
    wf::Size primaryCommandDesiredSize = {};
    UINT32 primaryCommandsCountInTransition = 0;

    // Update the primary command min width that will be used for finding the restorable
    // primary commands from the overflow into the primary commands collection whenever
    // the CommandBar has more available width.

    IFC_RETURN(m_tpPrimaryCommandsInTransition->get_Size(&primaryCommandsCountInTransition));

    ASSERT(primaryCommandsCountInTransition > 0);

    m_restorablePrimaryCommandMinWidth = -1;

    for (UINT32 i = 0; i < primaryCommandsCountInTransition; i++)
    {
        ctl::ComPtr<xaml_controls::ICommandBarElement> transitionPrimaryElement;

        IFC_RETURN(m_tpPrimaryCommandsInTransition->GetAt(i, &transitionPrimaryElement));

        if (transitionPrimaryElement)
        {
            auto elementAsSeparator = transitionPrimaryElement.AsOrNull<xaml_controls::IAppBarSeparator>();

            if (elementAsSeparator == nullptr)
            {
                ctl::ComPtr<xaml::IUIElement> elementAsUiE;

                IFC_RETURN(transitionPrimaryElement.As(&elementAsUiE));
                IFC_RETURN(elementAsUiE->get_DesiredSize(&primaryCommandDesiredSize));

                if (m_restorablePrimaryCommandMinWidth == -1)
                {
                    m_restorablePrimaryCommandMinWidth = primaryCommandDesiredSize.Width;
                }
                else
                {
                    m_restorablePrimaryCommandMinWidth = DoubleUtil::Min(m_restorablePrimaryCommandMinWidth, primaryCommandDesiredSize.Width);
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::MoveTransitionPrimaryCommandsIntoOverflow(
    _In_ UINT32 primaryCommandsCountInTransition)
{
    BOOLEAN isFound = FALSE;
    UINT32 primaryIndexForTransitionCommand = 0;

    ASSERT(primaryCommandsCountInTransition > 0);

    if (m_SecondaryCommandStartIndex == 0)
    {
        bool hasVisibleSecondaryElements = false;

        IFC_RETURN(HasVisibleElements(m_tpDynamicSecondaryCommands.Get(), &hasVisibleSecondaryElements));

        if (hasVisibleSecondaryElements)
        {
            // Add the AppBarSeparator between the transited primary command and existing secondary command in the overflow
            IFC_RETURN(SetOverflowStyleUsage(m_tpAppBarSeparatorInOverflow.Get(), true /*isItemInOverflow*/));
            IFC_RETURN(m_tpDynamicSecondaryCommands->InsertAt(m_SecondaryCommandStartIndex, m_tpAppBarSeparatorInOverflow.Get()));
            IFC_RETURN(SetInputModeOnSecondaryCommand(m_SecondaryCommandStartIndex++, m_inputDeviceTypeUsedToOpen));
            m_hasAppBarSeparatorInOverflow = true;
        }
    }

    // Rearrange the transition primary commands between the coming transition and the existing primary commands in overflow
    for (UINT32 i = 0; i < primaryCommandsCountInTransition; i++)
    {
        ctl::ComPtr<xaml_controls::ICommandBarElement> transitionPrimaryElement;

        IFC_RETURN(m_tpPrimaryCommandsInTransition->GetAt(0, &transitionPrimaryElement));

        // Peg the command element to ensure the element life time during the primary command moving from the dynamic
        // command into the overflow collection
        if (auto pegged = ctl::try_make_autopeg(transitionPrimaryElement.AsOrNull<IUIElement>().Cast<UIElement>()))
        {
            IFC_RETURN(m_tpPrimaryCommandsInTransition->RemoveAt(0));

            IFC_RETURN(m_tpDynamicPrimaryCommands->IndexOf(transitionPrimaryElement.Get(), &primaryIndexForTransitionCommand, &isFound));
            ASSERT(isFound);
            IFC_RETURN(m_tpDynamicPrimaryCommands->RemoveAt(primaryIndexForTransitionCommand))

            IFC_RETURN(InsertTransitionPrimaryCommandIntoOverflow(transitionPrimaryElement.Get()));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::InsertTransitionPrimaryCommandIntoOverflow(
    _In_ xaml_controls::ICommandBarElement* transitionPrimaryElement)
{
    UINT32 primaryIndexForTransitionCommand = 0;
    UINT32 primaryIndexForExistPrimaryCommand = 0;
    UINT32 indexForMovedPrimaryCommand = 0;
    BOOLEAN isFound = false;
    bool isInserted = false;

    ASSERT(m_isDynamicOverflowEnabled);

    if (m_hasAppBarSeparatorInOverflow)
    {
        indexForMovedPrimaryCommand = m_SecondaryCommandStartIndex - 1;
    }
    else
    {
        indexForMovedPrimaryCommand = m_SecondaryCommandStartIndex;
    }

    // Insert the transition primary command into the overflow collection in order of the original primary index

    IFC_RETURN(m_tpPrimaryCommands->IndexOf(transitionPrimaryElement, &primaryIndexForTransitionCommand, &isFound));
    ASSERT(isFound);

    // Note that the UseOverflowStyle property is set to True before the ICommandBarOverflowElement is inserted into
    // the SecondaryCommands ItemsControl. This is to guarantee that ItemsControl::PrepareItemContainer gets called after
    // the style was reset in AppBarButton::UpdateInternalStyles. Otherwise the ItemsControl::ApplyItemContainerStyle call
    // is ineffective.
    for (UINT32 i = 0; i < indexForMovedPrimaryCommand; i++)
    {
        ctl::ComPtr<xaml_controls::ICommandBarElement> existPrimaryElement;

        IFC_RETURN(m_tpDynamicSecondaryCommands->GetAt(i, &existPrimaryElement));

        IFC_RETURN(m_tpPrimaryCommands->IndexOf(existPrimaryElement.Get(), &primaryIndexForExistPrimaryCommand, &isFound));
        ASSERT(isFound);

        if (primaryIndexForTransitionCommand < primaryIndexForExistPrimaryCommand)
        {
            IFC_RETURN(SetOverflowStyleUsage(transitionPrimaryElement, true /*isItemInOverflow*/));
            IFC_RETURN(m_tpDynamicSecondaryCommands->InsertAt(i, transitionPrimaryElement));
            IFC_RETURN(SetInputModeOnSecondaryCommand(i, m_inputDeviceTypeUsedToOpen));

            m_SecondaryCommandStartIndex++;
            isInserted = true;
            break;
        }
    }

    if (!isInserted)
    {
        IFC_RETURN(SetOverflowStyleUsage(transitionPrimaryElement, true /*isItemInOverflow*/));
        IFC_RETURN(m_tpDynamicSecondaryCommands->InsertAt(indexForMovedPrimaryCommand, transitionPrimaryElement));
        IFC_RETURN(SetInputModeOnSecondaryCommand(indexForMovedPrimaryCommand, m_inputDeviceTypeUsedToOpen));

        m_SecondaryCommandStartIndex++;
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::ResetDynamicCommands()
{
    UINT32 primaryItemsCount = 0;
    UINT32 secondaryItemsCount = 0;

    IFC_RETURN(StoreFocusedCommandBarElement());

    IFC_RETURN(m_tpPrimaryCommands->get_Size(&primaryItemsCount));
    IFC_RETURN(m_tpSecondaryCommands->get_Size(&secondaryItemsCount));

    // Reset any primary commands currently in the overflow back to the non-overflow style.
    for (UINT32 i = 0; i < m_SecondaryCommandStartIndex; ++i)
    {
        IFC_RETURN(SetOverflowStyleAndInputModeOnSecondaryCommand(i, false));
    }

    // Remove the dynamic primary commands from the overflow collection and insert back to
    // the dynamic primary commands collection to make the work around bug#6428591
    for (UINT32 i = 0; i < m_SecondaryCommandStartIndex; ++i)
    {
        ctl::ComPtr<xaml_controls::ICommandBarElement> primaryElement;

        IFC_RETURN(m_tpDynamicSecondaryCommands->GetAt(0, &primaryElement));

        // Remove the moved primary command into the overflow immediately and
        // insert back to the primary commands to make a work around for bug#6428591
        IFC_RETURN(m_tpDynamicSecondaryCommands->RemoveAt(0));
        IFC_RETURN(m_tpDynamicPrimaryCommands->InsertAt(0, primaryElement.Get()));
    }

    // Reset the secondary command start index
    m_SecondaryCommandStartIndex = 0;

    m_hasAppBarSeparatorInOverflow = false;

    // Populate the dynamic primary collection with our primary items by default.
    IFC_RETURN(m_tpDynamicPrimaryCommands->Clear());
    for (UINT32 i = 0; i < primaryItemsCount; ++i)
    {
        ctl::ComPtr<xaml_controls::ICommandBarElement> primaryElement;

        IFC_RETURN(m_tpPrimaryCommands->GetAt(i, &primaryElement));
        IFC_RETURN(m_tpDynamicPrimaryCommands->InsertAt(i, primaryElement.Get()));
    }

    // Populate the dynamic secondary collection with our secondary items by default.
    IFC_RETURN(m_tpDynamicSecondaryCommands->Clear());
    for (UINT32 i = 0; i < secondaryItemsCount; ++i)
    {
        ctl::ComPtr<xaml_controls::ICommandBarElement> secondaryElement;

        IFC_RETURN(m_tpSecondaryCommands->GetAt(i, &secondaryElement));
        IFC_RETURN(SetOverflowStyleUsage(secondaryElement.Get(), true /*isItemInOverflow*/));
        IFC_RETURN(m_tpDynamicSecondaryCommands->InsertAt(i, secondaryElement.Get()));
        IFC_RETURN(SetInputModeOnSecondaryCommand(i, m_inputDeviceTypeUsedToOpen));
    }

    IFC_RETURN(UpdateTemplateSettings());

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::SaveMovedPrimaryCommandsIntoPreviousTransitionCollection()
{
    UINT32 dynamicSecondaryItemsCount = 0;
    UINT32 secondaryItemsCount = 0;
    UINT32 primaryCountInOverflow = 0;

    IFC_RETURN(m_tpDynamicSecondaryCommands->get_Size(&dynamicSecondaryItemsCount));
    IFC_RETURN(m_tpSecondaryCommands->get_Size(&secondaryItemsCount));

    ASSERT(dynamicSecondaryItemsCount >= secondaryItemsCount);
    primaryCountInOverflow = dynamicSecondaryItemsCount - secondaryItemsCount;

    IFC_RETURN(m_tpPrimaryCommandsInPreviousTransition->Clear());

    for (UINT32 i = 0; i < primaryCountInOverflow; i++)
    {
        ctl::ComPtr<xaml_controls::ICommandBarElement> primaryElement;

        IFC_RETURN(m_tpDynamicSecondaryCommands->GetAt(i, &primaryElement));
        IFC_RETURN(m_tpPrimaryCommandsInPreviousTransition->InsertAt(i, primaryElement.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::FireDynamicOverflowItemsChangingEvent(
    _In_ bool isForceToRestore)
{
    ctl::ComPtr<DynamicOverflowItemsChangingEventArgs> args;

    bool isAdding = false;
    UINT32 previousTransitionCount = 0;
    UINT32 currentTransitionCount = 0;
    UINT32 samePrimaryCount = 0;

    IFC_RETURN(ctl::make(&args));

    if (!isForceToRestore)
    {
        IFC_RETURN(m_tpPrimaryCommandsInPreviousTransition->get_Size(&previousTransitionCount));
        IFC_RETURN(m_tpPrimaryCommandsInTransition->get_Size(&currentTransitionCount));

        for (UINT32 i = 0; i < previousTransitionCount; i++)
        {
            ctl::ComPtr<xaml_controls::ICommandBarElement> primaryElementInPreviousTransition;

            IFC_RETURN(m_tpPrimaryCommandsInPreviousTransition->GetAt(i, &primaryElementInPreviousTransition));

            for (UINT32 j = 0; j < currentTransitionCount; j++)
            {
                ctl::ComPtr<xaml_controls::ICommandBarElement> primaryElementInTransition;

                IFC_RETURN(m_tpPrimaryCommandsInTransition->GetAt(j, &primaryElementInTransition));

                if (primaryElementInTransition && ctl::are_equal(primaryElementInTransition.Get(), primaryElementInPreviousTransition.Get()))
                {
                    samePrimaryCount++;
                }
            }
        }

        isAdding = (currentTransitionCount - samePrimaryCount) > 0 ? true : false;
    }

    IFC_RETURN(args.Get()->put_Action(isAdding ?
        xaml_controls::CommandBarDynamicOverflowAction_AddingToOverflow :
        xaml_controls::CommandBarDynamicOverflowAction_RemovingFromOverflow));

    DynamicOverflowItemsChangingEventSourceType* eventSource = nullptr;
    IFC_RETURN(CommandBarGenerated::GetDynamicOverflowItemsChangingEventSourceNoRef(&eventSource));

    // Fire the dynamic overflow items changing event
    IFC_RETURN(eventSource->Raise(this, args.Get()));

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::IsCommandBarElementInOverflow(
    _In_ xaml_controls::ICommandBarElement* element,
    _Out_ BOOLEAN* isInOverflow)
{
    ctl::ComPtr<xaml_controls::ICommandBar> parentCmdBar;

    *isInOverflow = false;

    IFC_RETURN(FindParentCommandBarForElement(element, &parentCmdBar));

    if (parentCmdBar)
    {
        IFC_RETURN(parentCmdBar.Cast<CommandBar>()->IsElementInOverflow(element, isInOverflow));
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::IsElementInOverflow(
    _In_ xaml_controls::ICommandBarElement* element,
    _Out_ BOOLEAN* isInOverflow)
{
    UINT32 itemsCount = 0;

    *isInOverflow = false;

    IFC_RETURN(m_tpDynamicSecondaryCommands.Get()->get_Size(&itemsCount));

    for (UINT32 i = 0; i < itemsCount; ++i)
    {
        ctl::ComPtr<xaml_controls::ICommandBarElement> elementInOverflow;

        IFC_RETURN(m_tpDynamicSecondaryCommands.Get()->GetAt(i, &elementInOverflow));

        if (elementInOverflow && ctl::are_equal(elementInOverflow.Get(), element))
        {
            *isInOverflow = true;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT 
CommandBar::IsOverflowButtonVisible(
    _Out_ bool* isOverflowButtonVisible)
{
    *isOverflowButtonVisible = false;

    if (m_tpExpandButton)
    {
        ctl::ComPtr<xaml_primitives::ICommandBarTemplateSettings> templateSettings;

        IFC_RETURN(get_CommandBarTemplateSettings(&templateSettings));

        xaml::Visibility effectiveOverflowButtonVisibility;

        IFC_RETURN(templateSettings.Cast<CommandBarTemplateSettings>()->get_EffectiveOverflowButtonVisibility(&effectiveOverflowButtonVisibility));

        *isOverflowButtonVisible = effectiveOverflowButtonVisibility == xaml::Visibility_Visible;
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::GetPositionInSetStatic(
    _In_ xaml_controls::ICommandBarElement* element,
    _Out_ INT* positionInSet)
{
    *positionInSet = -1;

    ctl::ComPtr<xaml_controls::ICommandBar> parentCommandBar;
    IFC_RETURN(FindParentCommandBarForElement(element, &parentCommandBar));

    if (parentCommandBar)
    {
        IFC_RETURN(parentCommandBar.Cast<CommandBar>()->GetPositionInSet(element, positionInSet));
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::GetPositionInSet(
    _In_ xaml_controls::ICommandBarElement* element,
    _Out_ INT* positionInSet)
{
    *positionInSet = -1;

    // The UIA position in set for a CommandBar element depends on two things:
    // which set the element belongs to (primary or secondary), and how many
    // interactable elements there are in that set.  We'll ignore separators
    // and collapsed elements for the purposes of this count since those are not UIA stops.
    // To accomplish this, we'll go through first the primary and then secondary
    // commands, and if we find the element we're looking for,
    // we'll return the number of interactable elements we've found
    // prior to and including it, which is its UIA position in set.
    UINT32 itemsCount = 0;
    IFC_RETURN(m_tpDynamicPrimaryCommands.Get()->get_Size(&itemsCount));

    INT interactableElementCount = 0;

    for (UINT32 i = 0; i < itemsCount; ++i)
    {
        ctl::ComPtr<xaml_controls::ICommandBarElement> currentElement;
        IFC_RETURN(m_tpDynamicPrimaryCommands.Get()->GetAt(i, &currentElement));

        auto itemAsUIE = currentElement.AsOrNull<xaml::IUIElement>();
        auto visibility = xaml::Visibility_Collapsed;

        if (itemAsUIE)
        {
            IFC_RETURN(itemAsUIE->get_Visibility(&visibility));
        }

        if (visibility != xaml::Visibility_Visible)
        {
            continue;
        }

        auto itemAsButton = currentElement.AsOrNull<xaml_controls::IAppBarButton>();
        auto itemAsToggleButton = currentElement.AsOrNull<xaml_controls::IAppBarToggleButton>();

        if (itemAsButton || itemAsToggleButton)
        {
            interactableElementCount++;
        }

        if (ctl::are_equal(currentElement.Get(), element))
        {
            *positionInSet = interactableElementCount;
            return S_OK;
        }
    }

    itemsCount = 0;
    IFC_RETURN(m_tpDynamicSecondaryCommands.Get()->get_Size(&itemsCount));

    interactableElementCount = 0;

    for (UINT32 i = 0; i < itemsCount; ++i)
    {
        ctl::ComPtr<xaml_controls::ICommandBarElement> currentElement;
        IFC_RETURN(m_tpDynamicSecondaryCommands.Get()->GetAt(i, &currentElement));

        auto itemAsUIE = currentElement.AsOrNull<xaml::IUIElement>();
        auto visibility = xaml::Visibility_Collapsed;

        if (itemAsUIE)
        {
            IFC_RETURN(itemAsUIE->get_Visibility(&visibility));
        }

        if (visibility != xaml::Visibility_Visible)
        {
            continue;
        }

        auto itemAsButton = currentElement.AsOrNull<xaml_controls::IAppBarButton>();
        auto itemAsToggleButton = currentElement.AsOrNull<xaml_controls::IAppBarToggleButton>();

        if (itemAsButton || itemAsToggleButton)
        {
            interactableElementCount++;
        }

        if (ctl::are_equal(currentElement.Get(), element))
        {
            *positionInSet = interactableElementCount;
            return S_OK;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::GetSizeOfSetStatic(
    _In_ xaml_controls::ICommandBarElement* element,
    _Out_ INT* sizeOfSet)
{
    *sizeOfSet = -1;

    ctl::ComPtr<xaml_controls::ICommandBar> parentCommandBar;
    IFC_RETURN(FindParentCommandBarForElement(element, &parentCommandBar));

    if (parentCommandBar)
    {
        IFC_RETURN(parentCommandBar.Cast<CommandBar>()->GetSizeOfSet(element, sizeOfSet));
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::GetSizeOfSet(
    _In_ xaml_controls::ICommandBarElement* element,
    _Out_ INT* sizeOfSet)
{
    *sizeOfSet = -1;

    // The UIA size in set for a CommandBar element depends on two things:
    // which set the element belongs to (primary or secondary), and how many
    // interactable elements there are in that set.  We'll ignore separators
    // for the purposes of this count since those are not UIA stops.
    // To accomplish this, we'll go through first the primary and then secondary
    // commands, and if we find the element we're looking for,
    // we'll return the number of interactable elements that are in its set.
    UINT32 itemsCount = 0;
    IFC_RETURN(m_tpDynamicPrimaryCommands.Get()->get_Size(&itemsCount));

    bool itemFound = false;
    INT interactableElementCount = 0;

    for (UINT32 i = 0; i < itemsCount; ++i)
    {
        ctl::ComPtr<xaml_controls::ICommandBarElement> currentElement;
        IFC_RETURN(m_tpDynamicPrimaryCommands.Get()->GetAt(i, &currentElement));

        auto itemAsUIE = currentElement.AsOrNull<xaml::IUIElement>();
        auto visibility = xaml::Visibility_Collapsed;

        if (itemAsUIE)
        {
            IFC_RETURN(itemAsUIE->get_Visibility(&visibility));
        }

        if (visibility != xaml::Visibility_Visible)
        {
            continue;
        }

        auto itemAsButton = currentElement.AsOrNull<xaml_controls::IAppBarButton>();
        auto itemAsToggleButton = currentElement.AsOrNull<xaml_controls::IAppBarToggleButton>();

        if (itemAsButton || itemAsToggleButton)
        {
            interactableElementCount++;
        }

        if (ctl::are_equal(currentElement.Get(), element))
        {
            itemFound = true;
        }
    }

    if (itemFound)
    {
        bool isOverflowButtonVisible{ false };

        IFC_RETURN(IsOverflowButtonVisible(&isOverflowButtonVisible));

        if (isOverflowButtonVisible)
        {
            // When the ExpandButton is visible, it participates in the set size.
            interactableElementCount++;

            if (interactableElementCount != m_automationSizeOfSet)
            {
                // Set size has changed since the last update.
                UpdateOverflowButtonAutomationSetNumbers(interactableElementCount /*sizeOfSet*/);
            }
        }

        *sizeOfSet = interactableElementCount;
        return S_OK;
    }

    itemsCount = 0;
    IFC_RETURN(m_tpDynamicSecondaryCommands.Get()->get_Size(&itemsCount));

    interactableElementCount = 0;

    for (UINT32 i = 0; i < itemsCount; ++i)
    {
        ctl::ComPtr<xaml_controls::ICommandBarElement> currentElement;
        IFC_RETURN(m_tpDynamicSecondaryCommands.Get()->GetAt(i, &currentElement));

        auto itemAsUIE = currentElement.AsOrNull<xaml::IUIElement>();
        auto visibility = xaml::Visibility_Collapsed;

        if (itemAsUIE)
        {
            IFC_RETURN(itemAsUIE->get_Visibility(&visibility));
        }

        if (visibility != xaml::Visibility_Visible)
        {
            continue;
        }

        auto itemAsButton = currentElement.AsOrNull<xaml_controls::IAppBarButton>();
        auto itemAsToggleButton = currentElement.AsOrNull<xaml_controls::IAppBarToggleButton>();

        if (itemAsButton || itemAsToggleButton)
        {
            interactableElementCount++;
        }

        if (ctl::are_equal(currentElement.Get(), element))
        {
            itemFound = true;
        }
    }

    if (itemFound)
    {
        *sizeOfSet = interactableElementCount;
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::TrimPrimaryCommandSeparatorInOverflow(
    _Inout_ UINT32* primaryCommandsCountInTransition)
{
    // Remove the primary AppBarSeparators that doesn't allow to move into overflow collection

    ASSERT(*primaryCommandsCountInTransition > 0);

    for (UINT32 i = *primaryCommandsCountInTransition; i > 0; i--)
    {
        ctl::ComPtr<xaml_controls::ICommandBarElement> transitionPrimaryElement;

        IFC_RETURN(m_tpPrimaryCommandsInTransition->GetAt(i - 1, &transitionPrimaryElement));

        auto elementAsSeparator = transitionPrimaryElement.AsOrNull<xaml_controls::IAppBarSeparator>();

        if (elementAsSeparator)
        {
            UINT32 primaryIndexForTransitionCommand;
            BOOLEAN isFound = false;

            IFC_RETURN(m_tpDynamicPrimaryCommands->IndexOf(transitionPrimaryElement.Get(), &primaryIndexForTransitionCommand, &isFound));
            if (isFound)
            {
                IFC_RETURN(m_tpDynamicPrimaryCommands->RemoveAt(primaryIndexForTransitionCommand))
            }
            IFC_RETURN(m_tpPrimaryCommandsInTransition->RemoveAt(i - 1));
            (*primaryCommandsCountInTransition)--;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::IsAppBarSeparatorInDynamicPrimaryCommands(
    _In_ UINT32 index,
    _Out_ bool* isAppBarSeparator)
{
    ctl::ComPtr<xaml_controls::ICommandBarElement> primaryElement;

    *isAppBarSeparator = false;

    IFC_RETURN(m_tpDynamicPrimaryCommands->GetAt(index, &primaryElement));
    if (primaryElement)
    {
        auto elementAsSeparator = primaryElement.AsOrNull<xaml_controls::IAppBarSeparator>();
        if (elementAsSeparator)
        {
            *isAppBarSeparator = true;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::FindMovableSeparatorsInBackwardDirection(
    _In_ UINT32 movingPrimaryCommandIndex,
    _Inout_ UINT32* primaryCommandsCountInTransition,
    _Inout_ double* primaryItemsControlDesiredWidth)
{
    if (movingPrimaryCommandIndex > 0)
    {
        bool isAppBarSeparator = false;

        // Find the separators in backward direction that need to be away with moving the primary command element

        IFC_RETURN(IsAppBarSeparatorInDynamicPrimaryCommands(movingPrimaryCommandIndex - 1, &isAppBarSeparator));

        if (isAppBarSeparator)
        {
            bool hasNonSeparator = false;
            INT32 indexNonSeparator = 0;
            INT32 indexMovingBackward = movingPrimaryCommandIndex - 1;

            IFC_RETURN(FindNonSeparatorInDynamicPrimaryCommands(
                false /* isForward */,
                indexMovingBackward,
                &hasNonSeparator,
                &indexNonSeparator));

            if (!hasNonSeparator)
            {
                while (indexMovingBackward >= 0)
                {
                    IFC_RETURN(InsertSeparatorToPrimaryCommandsInTransition(indexMovingBackward, primaryCommandsCountInTransition, primaryItemsControlDesiredWidth));
                    indexMovingBackward--;
                }
            }
            else
            {
                while (indexMovingBackward > indexNonSeparator && indexMovingBackward - indexNonSeparator > 1)
                {
                    IFC_RETURN(InsertSeparatorToPrimaryCommandsInTransition(indexMovingBackward, primaryCommandsCountInTransition, primaryItemsControlDesiredWidth));
                    indexMovingBackward--;
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::FindMovableSeparatorsInForwardDirection(
    _In_ UINT32 movingPrimaryCommandIndex,
    _Inout_ UINT32* primaryCommandsCountInTransition,
    _Inout_ double* primaryItemsControlDesiredWidth)
{
    bool isAppBarSeparator = false;

    // Find the separators in forward direction that need to be away with moving the primary command element

    IFC_RETURN(IsAppBarSeparatorInDynamicPrimaryCommands(movingPrimaryCommandIndex + 1, &isAppBarSeparator));

    if (isAppBarSeparator)
    {
        bool hasNonSeparator = false;
        UINT32 dynamicPrimaryCount = 0;
        INT32 indexNonSeparator = 0;
        INT32 indexMovingForward = movingPrimaryCommandIndex + 1;

        IFC_RETURN(FindNonSeparatorInDynamicPrimaryCommands(
            true /* isForward */,
            indexMovingForward,
            &hasNonSeparator,
            &indexNonSeparator));

        IFC_RETURN(m_tpDynamicPrimaryCommands->get_Size(&dynamicPrimaryCount));

        if (!hasNonSeparator)
        {
            while ((UINT32)indexMovingForward < dynamicPrimaryCount)
            {
                IFC_RETURN(InsertSeparatorToPrimaryCommandsInTransition(indexMovingForward, primaryCommandsCountInTransition, primaryItemsControlDesiredWidth));
                indexMovingForward++;
            }
        }
        else
        {
            while (indexMovingForward < indexNonSeparator && indexNonSeparator - indexMovingForward > 1)
            {
                IFC_RETURN(InsertSeparatorToPrimaryCommandsInTransition(indexMovingForward, primaryCommandsCountInTransition, primaryItemsControlDesiredWidth));
                indexMovingForward++;
            }
        }

        // Move the separator at the next index of moving primary command at 0 index
        if (movingPrimaryCommandIndex == 0)
        {
            IFC_RETURN(InsertSeparatorToPrimaryCommandsInTransition(movingPrimaryCommandIndex + 1, primaryCommandsCountInTransition, primaryItemsControlDesiredWidth));
        }

        // Move the separator at the next index with moving primary command
        if (movingPrimaryCommandIndex > 0)
        {
            IFC_RETURN(IsAppBarSeparatorInDynamicPrimaryCommands(movingPrimaryCommandIndex - 1, &isAppBarSeparator));
            if (isAppBarSeparator)
            {
                IFC_RETURN(InsertSeparatorToPrimaryCommandsInTransition(movingPrimaryCommandIndex + 1, primaryCommandsCountInTransition, primaryItemsControlDesiredWidth));

                if (!hasNonSeparator)
                {
                    IFC_RETURN(InsertSeparatorToPrimaryCommandsInTransition(movingPrimaryCommandIndex - 1, primaryCommandsCountInTransition, primaryItemsControlDesiredWidth));
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::FindNonSeparatorInDynamicPrimaryCommands(
    _In_ bool isForward,
    _In_ INT32 indexMoving,
    _Out_ bool* hasNonSeparator,
    _Out_ INT32* indexNonSeparator)
{
    UINT32 dynamicPrimaryCount = 0;

    *hasNonSeparator = FALSE;
    *indexNonSeparator = 0;

    IFC_RETURN(m_tpDynamicPrimaryCommands->get_Size(&dynamicPrimaryCount));

    // Find the non-separator command element index in proper direction
    while (isForward ? ((UINT32)indexMoving < dynamicPrimaryCount) : (indexMoving >= 0))
    {
        ctl::ComPtr<xaml_controls::ICommandBarElement> primaryElement;

        IFC_RETURN(m_tpDynamicPrimaryCommands->GetAt(indexMoving, &primaryElement));

        auto elementAsSeparator = primaryElement.AsOrNull<xaml_controls::IAppBarSeparator>();

        if (elementAsSeparator == nullptr)
        {
            *hasNonSeparator = true;
            *indexNonSeparator = indexMoving;
            break;
        }

        isForward ? indexMoving++ : indexMoving--;
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::InsertSeparatorToPrimaryCommandsInTransition(
    _In_ UINT32 indexMovingSeparator,
    _Inout_ UINT32* primaryCommandsCountInTransition,
    _Inout_ double* primaryItemsControlDesiredWidth)
{
    ctl::ComPtr<xaml_controls::ICommandBarElement> element;

    IFC_RETURN(m_tpDynamicPrimaryCommands->GetAt(indexMovingSeparator, &element));

    auto elementAsSeparator = element.AsOrNull<xaml_controls::IAppBarSeparator>();
    if (elementAsSeparator)
    {
        BOOLEAN isFound = false;
        UINT32 separatorIndexInTransition = 0;

        IFC_RETURN(m_tpPrimaryCommandsInTransition->IndexOf(element.Get(), &separatorIndexInTransition, &isFound));
        if (!isFound)
        {
            ctl::ComPtr<xaml::IUIElement> elementAsUiE;
            wf::Size elementDesiredSize = {};

            IFC_RETURN(m_tpPrimaryCommandsInTransition->InsertAt((*primaryCommandsCountInTransition)++, element.Get()));

            IFC_RETURN(element.As(&elementAsUiE));
            IFC_RETURN(elementAsUiE->get_DesiredSize(&elementDesiredSize));

            *(primaryItemsControlDesiredWidth) -= elementDesiredSize.Width;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::GetRestorablePrimaryCommandsMinimumCount(
    _Out_ UINT32* restorableMinCount)
{
    INT32 dynamicOverflowOrder = 0;
    INT32 firstRestorableOrder = 0;

    *restorableMinCount = 0;

    if (m_SecondaryCommandStartIndex > 1)
    {
        for (UINT32 i = 0; i < m_SecondaryCommandStartIndex - 1; ++i)
        {
            ctl::ComPtr<xaml_controls::ICommandBarElement> primaryElement;

            IFC_RETURN(m_tpDynamicSecondaryCommands->GetAt(i, &primaryElement));

            if (primaryElement)
            {
                IFC_RETURN(primaryElement->get_DynamicOverflowOrder(&dynamicOverflowOrder));
                if (dynamicOverflowOrder > 0)
                {
                    firstRestorableOrder = (INT32)DoubleUtil::Max(dynamicOverflowOrder, firstRestorableOrder);
                }
            }
        }

        if (firstRestorableOrder > 0)
        {
            for (UINT32 i = 0; i < m_SecondaryCommandStartIndex - 1; ++i)
            {
                ctl::ComPtr<xaml_controls::ICommandBarElement> primaryElement;

                IFC_RETURN(m_tpDynamicSecondaryCommands->GetAt(i, &primaryElement));

                if (primaryElement)
                {
                    IFC_RETURN(primaryElement->get_DynamicOverflowOrder(&dynamicOverflowOrder));
                    if (dynamicOverflowOrder > 0 && dynamicOverflowOrder == firstRestorableOrder)
                    {
                        // Retrieve the restorable primary commands that has the same dynamic overflow order
                        (*restorableMinCount)++;
                    }
                }
            }
        }
        else
        {
            // Restore the primary command one by one from the overflow to the primary commands
            *restorableMinCount = 1;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::StoreFocusedCommandBarElement()
{
    ctl::ComPtr<DependencyObject> focusedElement;
    IFC_RETURN(GetFocusedElement(&focusedElement));

    if (focusedElement)
    {
        ctl::ComPtr<xaml_controls::IItemsControl> itemsControl;
        IFC_RETURN(ItemsControl::ItemsControlFromItemContainer(focusedElement.Get(), &itemsControl));

        if (itemsControl &&
            (itemsControl.Get() == m_tpPrimaryItemsControlPart.Get() ||
                itemsControl.Get() == m_tpSecondaryItemsControlPart.Cast<CommandBarOverflowPresenter>()))
        {
            ctl::ComPtr<IInspectable> item;
            IFC_RETURN(itemsControl.Cast<ItemsControl>()->ItemFromContainer(focusedElement.Get(), &item));

            ctl::ComPtr<xaml_controls::ICommandBarElement> element;
            IFC_RETURN(item.As(&element));
            SetPtrValue(m_focusedElementPriorToCollectionOrSizeChange, element.Get());
            IFC_RETURN(GetFocusState(focusedElement.Get(), &m_focusStatePriorToCollectionOrSizeChange));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CommandBar::RestoreCommandBarElementFocus()
{
    auto element = m_focusedElementPriorToCollectionOrSizeChange.Get();
    if (element)
    {
        ctl::ComPtr<xaml_controls::IItemsControl> itemsControl;
        unsigned elementIndex;
        BOOLEAN found;

        IFC_RETURN(m_tpDynamicPrimaryCommands.Get()->IndexOf(element, &elementIndex, &found));
        if (found)
        {
            itemsControl = m_tpPrimaryItemsControlPart.Get();
        }
        else
        {
            IFC_RETURN(m_tpDynamicSecondaryCommands.Get()->IndexOf(element, &elementIndex, &found));
            if (found)
            {
                itemsControl = m_tpSecondaryItemsControlPart.Cast<CommandBarOverflowPresenter>();
            }
        }

        if (itemsControl)
        {
            ctl::ComPtr<xaml::IDependencyObject> container;
            IFC_RETURN(itemsControl.Cast<ItemsControl>()->ContainerFromItem(element, &container));

            if (container)
            {
                ctl::ComPtr<xaml::IUIElement> containerAsUIE;
                IFC_RETURN(container.As(&containerAsUIE));

                BOOLEAN ignored;
                IFC_RETURN(containerAsUIE->Focus(m_focusStatePriorToCollectionOrSizeChange, &ignored));
            }
        }

        ResetCommandBarElementFocus();
    }

    return S_OK;
}

_Check_return_ HRESULT CommandBar::OnAccessKeyInvoked(_In_ IUIElement* /*pSender*/,
    _In_ xaml_input::IAccessKeyInvokedEventArgs* /*pArgs*/)
{
    if (m_tpOverflowPopup != nullptr)
    {
        IFC_RETURN(m_overflowPopupOpenedEventHandler.AttachEventHandler(
            m_tpOverflowPopup.Get(),
            std::bind(&CommandBar::OnOverflowPopupOpened, this, _1, _2)));
    }

    return S_OK;
}

_Check_return_ HRESULT  CommandBar::OnOverflowPopupOpened(_In_ IInspectable* pSender, _In_ IInspectable* pArgs)
{
    auto detachEventHandler = wil::scope_exit([&]
    {
        IFCFAILFAST(m_overflowPopupOpenedEventHandler.DetachEventHandler(m_tpOverflowPopup.Get()));
    });

    if (m_tpOverflowPopup)
    {
        if (m_tpSecondaryItemsControlPart)
        {
            ctl::ComPtr<wfc::IObservableVector<IInspectable*>> items;
            ctl::ComPtr<wfc::IIterator<IInspectable*>> firstItemIterator;

            IFC_RETURN(m_tpSecondaryItemsControlPart->get_Items(&items));
            IFC_RETURN(items.Cast<ItemCollection>()->First(&firstItemIterator));

            if (firstItemIterator)
            {
                BOOLEAN succeeded = FALSE;
                BOOLEAN hasCurrent = FALSE;
                IFC_RETURN(firstItemIterator->get_HasCurrent(&hasCurrent));

                while (hasCurrent && !succeeded)
                {
                    ctl::ComPtr<IInspectable> firstItem;
                    IFC_RETURN(firstItemIterator->get_Current(&firstItem));
                    ctl::ComPtr<IControl> itemAsControl = firstItem.AsOrNull<IControl>();
                    if (itemAsControl)
                    {
                        IFC_RETURN(itemAsControl.Cast<Control>()->Focus(xaml::FocusState_Keyboard, &succeeded));
                        if (succeeded)
                        {
                            auto contentRoot = VisualTree::GetContentRootForElement(GetHandle());
                            IFC_RETURN(contentRoot->GetAKExport().UpdateScope());
                        }
                    }
                    IFC_RETURN(firstItemIterator->MoveNext(&hasCurrent));
                }
            }
        }
    }

    return S_OK;
}

void CommandBar::ResetCommandBarElementFocus()
{
    m_focusedElementPriorToCollectionOrSizeChange.Clear();
    m_focusStatePriorToCollectionOrSizeChange = {};
}

_Check_return_ HRESULT
CommandBar::GetFocusState(_In_ xaml::IDependencyObject* focusedElement, _Out_ xaml::FocusState* focusState)
{
    *focusState = xaml::FocusState_Programmatic;
    auto focusedControl = ctl::ComPtr<xaml::IDependencyObject>(focusedElement).AsOrNull<Control>();
    if (focusedControl)
    {
        IFC_RETURN(focusedControl->get_FocusState(focusState));
    }

    // MSFT 20505680. Although it shouldn't be possible, we are seeing scenarios where the FocusState of the focused element is
    // Unfocused. Workaround this issue by using the real FocusState off the FocusManager.
    if (focusedControl == nullptr || *focusState == xaml::FocusState_Unfocused)
    {
        if (CFocusManager* focusManager = VisualTree::GetFocusManagerForElement(GetHandle()))
        {
            *focusState = static_cast<xaml::FocusState>(focusManager->GetRealFocusStateForFocusedElement());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CommandBar::CloseSubMenus(_In_opt_ ISubMenuOwner* pMenuToLeaveOpen, bool closeOnDelay)
{
    ctl::ComPtr<wfc::IObservableVector<ICommandBarElement*>> primaryCommands;
    ctl::ComPtr<wfc::IObservableVector<ICommandBarElement*>> secondaryCommands;
    IFC_RETURN(get_PrimaryCommands(&primaryCommands));
    IFC_RETURN(get_SecondaryCommands(&secondaryCommands));

    ctl::ComPtr<wfc::IVector<ICommandBarElement*>> primaryCommandsAsVector;
    ctl::ComPtr<wfc::IVector<ICommandBarElement*>> secondaryCommandsAsVector;
    IFC_RETURN(primaryCommands.As(&primaryCommandsAsVector));
    IFC_RETURN(secondaryCommands.As(&secondaryCommandsAsVector));

    UINT primaryCommandCount = 0;
    UINT secondaryCommandCount = 0;
    IFC_RETURN(primaryCommandsAsVector->get_Size(&primaryCommandCount));
    IFC_RETURN(secondaryCommandsAsVector->get_Size(&secondaryCommandCount));

    for (UINT i = 0; i < primaryCommandCount; i++)
    {
        ctl::ComPtr<ICommandBarElement> element;

        IFC_RETURN(primaryCommandsAsVector->GetAt(i, &element));
        IFC_RETURN(CloseSubMenu(element.AsOrNull<ISubMenuOwner>(), pMenuToLeaveOpen, closeOnDelay));
    }

    for (UINT i = 0; i < secondaryCommandCount; i++)
    {
        ctl::ComPtr<ICommandBarElement> element;

        IFC_RETURN(secondaryCommandsAsVector->GetAt(i, &element));
        IFC_RETURN(CloseSubMenu(element.AsOrNull<ISubMenuOwner>(), pMenuToLeaveOpen, closeOnDelay));
    }

    return S_OK;
}

_Check_return_ HRESULT CommandBar::CloseSubMenu(
    _In_opt_ ctl::ComPtr<ISubMenuOwner> const& menu,
    _In_opt_ ISubMenuOwner* menuToLeaveOpen,
    bool closeOnDelay)
{
    if (menu && (!menuToLeaveOpen || !ctl::are_equal(menuToLeaveOpen, menu.Get())))
    {
        BOOLEAN isSubMenuOpen = FALSE;
        IFC_RETURN(menu->get_IsSubMenuOpen(&isSubMenuOpen));

        if (isSubMenuOpen)
        {
            if (closeOnDelay)
            {
                IFC_RETURN(menu->DelayCloseSubMenu());
            }
            else
            {
                IFC_RETURN(menu->CloseSubMenuTree());
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CommandBar::get_ParentMenuImpl(_Outptr_result_maybenull_ IMenu** ppValue)
{
    *ppValue = nullptr;
    return S_OK;
}

_Check_return_ HRESULT CommandBar::put_ParentMenuImpl(_In_opt_ IMenu* pValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CommandBar::CloseImpl()
{
    IFC_RETURN(put_IsOpen(FALSE));
    return S_OK;
}