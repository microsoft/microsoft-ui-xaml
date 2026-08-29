// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Hub.g.h"
#include "HubAutomationPeer.g.h"
#include "HubSection.g.h"
#include "HubSectionCollection.g.h"
#include "DispatcherTimer.g.h"
#include "ModernCollectionBasePanel.g.h"
#include "HubSectionHeaderClickEventArgs.g.h"
#include "StackPanel.g.h"
#include "SectionsInViewChangedEventArgs.g.h"
#include "IScrollInfo.g.h"
#include "ScrollViewer.g.h"
#include "Storyboard.g.h"
#include "SecondaryContentRelationship.g.h"
#include "Window.g.h"
#include "BitmapImage.g.h"
#include "VisualTreeHelper.h"
#include "RootScale.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// The FocusState we use to focus the .Header of the DefaultSection after scrolling it into view during load.
// While Programmatic seems like the logical value to use here, Programmatic focus shows the same dashed focus rect
// on controls as Keyboard focus, and we don't want that to ever show for touch users.  So, Pointer is the way to go.
const xaml::FocusState Hub::FocusStateForDefaultSection = xaml::FocusState::FocusState_Pointer;

Hub::Hub()
    : m_hubHeaderHeight (0.)
    , m_semanticZoomCompletedFocusState(xaml::FocusState_Programmatic)
    , m_destinationIndexForSemanticZoom(-1)
    , m_isPanelLoaded(FALSE)
{
}

Hub::~Hub()
{
    VERIFYHR(DetachHandler(m_epHeaderSizeChangedHandler, m_tpHeaderHostPart));
    VERIFYHR(DetachHandler(m_epPanelLoadedHandler, m_tpPanel));
    VERIFYHR(DetachHandler(m_epSemanticZoomScrollIntoViewTimerTickHandler, m_tpSemanticZoomScrollIntoViewTimer));
}

// Prepares object's state
_Check_return_ HRESULT
Hub::PrepareState()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<HubSectionCollection> spSections;
    ctl::ComPtr<ReadOnlyObservableTrackerCollection<IInspectable*>> spSectionHeaders;
    ctl::ComPtr<ReadOnlyTrackerCollection<xaml_controls::HubSection*>> spSectionsInView;
    ctl::ComPtr<DispatcherTimer> spSemanticZoomScrollIntoViewTimer;
    wf::TimeSpan zeroDurationTimeSpan;

    IFC(HubGenerated::PrepareState());

    IFC(ctl::make(&spSections));
    // like ItemsControl and ItemCollection, the owner of HubSectionCollection should be Hub.
    IFC(CoreImports::Collection_SetOwner(static_cast<CCollection*>(spSections->GetHandle()), GetHandle()));

    SetPtrValue(m_tpSections, spSections);
    IFC(put_Sections(spSections.Get()));

    IFC(ctl::make(&spSectionHeaders));
    SetPtrValue(m_tpSectionHeaders, spSectionHeaders);

    IFC(ctl::make(&spSectionsInView));
    SetPtrValue(m_tpSectionsInView, spSectionsInView);

    IFC(ctl::make<DispatcherTimer>(&spSemanticZoomScrollIntoViewTimer));
    SetPtrValue(m_tpSemanticZoomScrollIntoViewTimer, spSemanticZoomScrollIntoViewTimer);

    zeroDurationTimeSpan.Duration = 0;
    IFC(m_tpSemanticZoomScrollIntoViewTimer->put_Interval(zeroDurationTimeSpan));
    IFC(m_epSemanticZoomScrollIntoViewTimerTickHandler.AttachEventHandler(m_tpSemanticZoomScrollIntoViewTimer.Cast<DispatcherTimer>(),
        [this](IInspectable *pSender, IInspectable *pArgs)
    {
        RRETURN(OnSemanticZoomScrollIntoViewTimerTick());
    }));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Hub::DisconnectFrameworkPeerCore()
{
    HRESULT hr = S_OK;

    //
    // DXAML Structure
    // ---------------
    //   DXAML::Hub (this)   -------------->   DXAML::HubSection
    //      |                    |
    //      |                    +--------->   DXAML::HubSection
    //      V
    //   DXAML::HubSectionCollection (m_tpSections)
    //
    // CORE Structure
    // --------------
    //   Core::CControl (this)              < - - +         < - - +
    //            |                               :               :
    //            V                               :               :
    //   Core::CHubSectionCollection          - - + (m_pOwner)    :
    //      |                  |                                  :
    //      V                  V                                  :
    //   Core::CControl   Core::CControl      - - - - - - - - - - + (m_pParent)
    //
    // To clear the m_pParent association of the HubSections, we have to clear the
    // CHubSectionCollection's children, which calls SetParent(NULL) on each of its
    // children. Once this association to Hub is broken, we can safely destroy Hub.
    //

    // clear the children in the HubSectionCollection
    // clear the children in the m_tpPrimaryCommands
    if (m_tpSections.GetAsCoreDO() != nullptr)
    {
        IFC(CoreImports::Collection_Clear(static_cast<CCollection*>(m_tpSections.GetAsCoreDO())));
        IFC(CoreImports::Collection_SetOwner(static_cast<CCollection*>(m_tpSections.GetAsCoreDO()), nullptr));
    }

    IFC(HubGenerated::DisconnectFrameworkPeerCore());

Cleanup:
    RRETURN(hr);
}

// Override the GetDefaultValue method to return the default values
// for Hub dependency properties.
_Check_return_ HRESULT
Hub::GetDefaultValue2(
    _In_ const CDependencyProperty* pDP,
    _Out_ CValue* pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pDP);
    IFCPTR(pValue);

    switch (pDP->GetIndex())
    {
    case KnownPropertyIndex::Hub_SectionHeaders:
        pValue->SetIInspectableAddRef(ctl::iinspectable_cast(m_tpSectionHeaders.Get()));
        break;
    default:
        IFC(HubGenerated::GetDefaultValue2(pDP, pValue));
        break;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Hub::OnCollectionChanged(
    _In_ XUINT32 nCollectionChangeType,
    _In_ XUINT32 nIndex)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<VectorChangedEventArgs> spArg;
    UINT32 nIndexInPanel = nIndex;

    if (EventEnabledHubSectionCountInfo() && m_tpSections)
    {
        UINT nNewSectionsCount = 0;
        if (SUCCEEDED(m_tpSections->get_Size(&nNewSectionsCount)))
        {
            TraceHubSectionCountInfo(nNewSectionsCount);
        }
    }

    IFC(HubGenerated::OnCollectionChanged(nCollectionChangeType, nIndex));

    IFC(ctl::make(&spArg));
    IFC(spArg->put_CollectionChange(static_cast<wfc::CollectionChange>(nCollectionChangeType)));
    IFC(spArg->put_Index(nIndexInPanel));

    if (m_tpPanel && ctl::is<IModernCollectionBasePanel>(m_tpPanel.Get()))
    {
        IFC(m_tpPanel.Cast<ModernCollectionBasePanel>()->NotifyOfItemsChanging(spArg.Get()));
    }

    switch (nCollectionChangeType)
    {
    case wfc::CollectionChange_Reset:
        {
            if (m_tpPanel && !ctl::is<IModernCollectionBasePanel>(m_tpPanel.Get()))
            {
                ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
                ctl::ComPtr<wfc::IIterator<xaml_controls::HubSection*>> spIterator;
                BOOLEAN hasCurrent = FALSE;

                IFC(m_tpSections.Cast<HubSectionCollection>()->First(&spIterator));
                IFC(spIterator->get_HasCurrent(&hasCurrent));

                while (hasCurrent)
                {
                    ctl::ComPtr<xaml_controls::IHubSection> spSection;

                    IFC(spIterator->get_Current(&spSection));
                    IFC(spSection.Cast<HubSection>()->SetParentHub(nullptr));
                    IFC(spIterator->MoveNext(&hasCurrent));
                }

                IFC(m_tpPanel->get_Children(&spChildren));
                IFC(spChildren->Clear());
            }

            IFC(m_tpSectionHeaders->InternalClear());
        }
        break;
    case wfc::CollectionChange_ItemInserted:
        {
            ctl::ComPtr<xaml_controls::IHubSection> spSection;
            ctl::ComPtr<IInspectable> spHeader;

            IFC(m_tpSections.Cast<HubSectionCollection>()->GetAt(nIndex, &spSection));

            if (m_tpPanel && !ctl::is<IModernCollectionBasePanel>(m_tpPanel.Get()))
            {
                ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

                IFC(spSection.Cast<HubSection>()->SetParentHub(this));
                IFC(m_tpPanel->get_Children(&spChildren));
                IFC(spChildren->InsertAt(nIndexInPanel, spSection.Cast<HubSection>()));
            }

            IFC(spSection->get_Header(&spHeader));
            IFC(m_tpSectionHeaders->InternalInsertAt(nIndex, spHeader.Get()));
        }
        break;
    case wfc::CollectionChange_ItemRemoved:
        {
            if (m_tpPanel && !ctl::is<IModernCollectionBasePanel>(m_tpPanel.Get()))
            {
                ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
                ctl::ComPtr<IUIElement> spChild;

                IFC(m_tpPanel->get_Children(&spChildren));

                IFC(spChildren->GetAt(nIndexInPanel, &spChild));
                if (ctl::is<IHubSection>(spChild.Get()))
                {
                    IFC(spChild.Cast<HubSection>()->SetParentHub(nullptr));
                    IFC(spChildren->RemoveAt(nIndexInPanel));
                }
                else
                {
                    ASSERT(FALSE);  // We are only supporting HubSections as the panel children.
                }
            }
            IFC(m_tpSectionHeaders->InternalRemoveAt(nIndex));
        }
        break;
    case wfc::CollectionChange_ItemChanged:
        {
            ctl::ComPtr<xaml_controls::IHubSection> spNewSection;
            ctl::ComPtr<IInspectable> spHeader;

            if (m_tpPanel && !ctl::is<IModernCollectionBasePanel>(m_tpPanel.Get()))
            {
                ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
                ctl::ComPtr<IUIElement> spChild;
                ctl::ComPtr<IHubSection> spOldSection;

                IFC(m_tpPanel->get_Children(&spChildren));
                IFC(spChildren->GetAt(nIndexInPanel, &spChild));
                IFC(ctl::do_query_interface(spOldSection, spChild.Get()));

                IFC(spOldSection.Cast<HubSection>()->SetParentHub(nullptr));
                IFC(spChildren->RemoveAt(nIndexInPanel));

                IFC(m_tpSections.Cast<HubSectionCollection>()->GetAt(nIndex, &spNewSection));
                IFC(spNewSection.Cast<HubSection>()->SetParentHub(this));
                IFC(spChildren->InsertAt(nIndexInPanel, spNewSection.Cast<HubSection>()));
            }
            else
            {
                IFC(m_tpSections.Cast<HubSectionCollection>()->GetAt(nIndex, &spNewSection));
            }

            IFC(m_tpSectionHeaders->InternalRemoveAt(nIndex));

            IFC(spNewSection->get_Header(&spHeader));
            IFC(m_tpSectionHeaders->InternalInsertAt(nIndex, spHeader.Get()));
        }
        break;
    default:
        IFC(E_FAIL);
        break;
    }

    if (m_tpPanel && ctl::is<IModernCollectionBasePanel>(m_tpPanel.Get()))
    {
        IFC(m_tpPanel.Cast<ModernCollectionBasePanel>()->NotifyOfItemsChanged(spArg.Get()));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
Hub::OnApplyTemplate()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IFrameworkElement> spHeaderHostPart;
    ctl::ComPtr<xaml_controls::IPanel> spPanel;
    ctl::ComPtr<xaml_controls::IScrollViewer> spScrollViewer;

    IFC(DetachHandler(m_epHeaderSizeChangedHandler, m_tpHeaderHostPart));
    m_tpHeaderHostPart.Clear();

    //in case Hub gets dynamicly retemplated, clear the height so HubSections can get chance to re-position themselves.
    SetHubHeaderHeight(0.);

    if (m_tpPanel)
    {
        if (ctl::is<IModernCollectionBasePanel>(m_tpPanel.Get()))
        {
            // This will clean up the panel and clear the children
            IFC(m_tpPanel.Cast<ModernCollectionBasePanel>()->DisconnectItemsHost());
            IFC(m_tpPanel.Cast<ModernCollectionBasePanel>()->remove_VisibleIndicesUpdated(m_visibleIndicesUpdatedToken));
        }
        else
        {
            ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
            ctl::ComPtr<wfc::IIterator<xaml_controls::HubSection*>> spIterator;
            BOOLEAN hasCurrent = FALSE;

            IFC(m_tpSections.Cast<HubSectionCollection>()->First(&spIterator));
            IFC(spIterator->get_HasCurrent(&hasCurrent));
            while (hasCurrent)
            {
                ctl::ComPtr<xaml_controls::IHubSection> spSection;
                IFC(spIterator->get_Current(&spSection));
                IFC(spSection.Cast<HubSection>()->SetParentHub(nullptr));
                IFC(spIterator->MoveNext(&hasCurrent));
            }

            IFC(m_tpPanel->get_Children(&spChildren));
            IFC(spChildren->Clear());
        }

        IFC(DetachHandler(m_epPanelLoadedHandler, m_tpPanel));
        IFC(DetachHandler(m_epPanelUnloadedHandler, m_tpPanel));
    }

    // TODO: Test dynamic retemplating.
    m_tpPanel.Clear();

    IFC(HubGenerated::OnApplyTemplate());

    IFC(GetTemplatePart<IFrameworkElement>(STR_LEN_PAIR(L"HeaderHost"), spHeaderHostPart.ReleaseAndGetAddressOf()));
    SetPtrValueWithQIOrNull(m_tpHeaderHostPart, spHeaderHostPart.Get());
    if (m_tpHeaderHostPart)
    {
        IFC(m_epHeaderSizeChangedHandler.AttachEventHandler(m_tpHeaderHostPart.Get(),
            [this](IInspectable *pSender, ISizeChangedEventArgs *pArgs)
        {
            RRETURN(OnHeaderSizeChanged(pSender, pArgs));
        }));
    }

    IFC(GetTemplatePart<IPanel>(STR_LEN_PAIR(L"Panel"), spPanel.ReleaseAndGetAddressOf()));
    SetPtrValueWithQIOrNull(m_tpPanel, spPanel.Get());

    if (m_tpPanel)
    {
        KnownTypeIndex indexPanel = m_tpPanel.Cast<Panel>()->GetTypeIndex();

        switch (indexPanel)
        {
        case KnownTypeIndex::WrapGrid:
        case KnownTypeIndex::VariableSizedWrapGrid:
        case KnownTypeIndex::CarouselPanel:
        case KnownTypeIndex::VirtualizingStackPanel:
            // Throw an exception when using a built-in panel type that we don't wish to support for Hub.
            IFC(ErrorHelper::OriginateErrorUsingResourceID(E_FAIL, ERROR_INCORRECT_PANEL_FOR_CONTROL));
            break;

            // ItemsStackPanel scenario, P0 for Hub.  Allow ItemsWrapGrid as well though we don't have a real
            // scenario for it and don't test it nearly as thoroughly, but no reason it shouldn't work here.
        case KnownTypeIndex::ItemsStackPanel:
        case KnownTypeIndex::ItemsWrapGrid:
            {
                ctl::ComPtr<wf::IEventHandler<IInspectable*>> spVisibleIndicesChangedHandler;

                IFC(m_tpPanel.Cast<ModernCollectionBasePanel>()->RegisterItemsHost(this));

                spVisibleIndicesChangedHandler.Attach(
                    new ClassMemberEventHandler<
                    Hub,
                    xaml_controls::IHub,
                    wf::IEventHandler<IInspectable*>,
                    IInspectable,
                    IInspectable>(this, &Hub::OnPanelVisibleIndicesChanged));
                IFC(m_tpPanel.Cast<ModernCollectionBasePanel>()->add_VisibleIndicesUpdated(spVisibleIndicesChangedHandler.Get(), &m_visibleIndicesUpdatedToken));
            }
            break;

            // StackPanel scenario, P1 for Hub.  Don't block other custom panels.
        default:
            {
                ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
                ctl::ComPtr<wfc::IIterator<IInspectable*>> spIterator;
                BOOLEAN hasCurrent = FALSE;

                IFC(m_tpPanel->get_Children(&spChildren));

                IFC(m_tpSections.Cast<HubSectionCollection>()->First(&spIterator));
                IFC(spIterator->get_HasCurrent(&hasCurrent));
                while (hasCurrent)
                {
                    ctl::ComPtr<IInspectable> spSectionAsII;
                    ctl::ComPtr<xaml_controls::IHubSection> spSection;

                    IFC(spIterator->get_Current(&spSectionAsII));
                    IFC(spSectionAsII.As<IHubSection>(&spSection));
                    IFC(spChildren->Append(spSection.Cast<HubSection>()));
                    IFC(spSection.Cast<HubSection>()->SetParentHub(this));
                    IFC(spIterator->MoveNext(&hasCurrent));
                }
            }
        }

        IFC(m_epPanelLoadedHandler.AttachEventHandler(m_tpPanel.Cast<Panel>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            RRETURN(OnPanelLoaded(pSender, pArgs));
        }));

        IFC(m_epPanelUnloadedHandler.AttachEventHandler(m_tpPanel.Cast<Panel>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            m_isPanelLoaded = FALSE;
            RRETURN(S_OK);
        }));
    }

    IFC(GetTemplatePart<IScrollViewer>(STR_LEN_PAIR(L"ScrollViewer"), spScrollViewer.ReleaseAndGetAddressOf()));
    SetPtrValueWithQIOrNull(m_tpScrollViewer, spScrollViewer.Get());

    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// Handle the custom property changed event and call the OnPropertyChanged2 methods.
_Check_return_ HRESULT
Hub::OnPropertyChanged2(
    _In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(HubGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::Hub_Orientation:
        IFC(OnOrientationChanged());
        break;

    case KnownPropertyIndex::Hub_Header:
    case KnownPropertyIndex::Hub_HeaderTemplate:
        IFC(UpdateVisualState());
        break;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Hub::OnOrientationChanged()
{
    HRESULT hr = S_OK;

    IFC(UpdateVisualState());
    IFC(RefreshHubSectionPlaceholderHeights());

Cleanup:
    RRETURN(hr);
}

// Change to the correct visual state for the Hub.
_Check_return_ HRESULT
Hub::ChangeVisualState(
    // true to use transitions when updating the visual state, false
    // to snap directly to the new visual state.
    _In_ bool bUseTransitions)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;
    BOOLEAN bIgnored = FALSE;

    IFC(get_Orientation(&orientation));

    // OrientationStates
    if (orientation == xaml_controls::Orientation_Vertical)
    {
        IFC(GoToState(bUseTransitions, L"Vertical", &bIgnored));
    }
    else
    {
        IFC(GoToState(bUseTransitions, L"Horizontal", &bIgnored));
    }

    // Evaluates Hub.Header and Hub.HeaderTemplate and sets the HeaderHost template part's IsHitTestVisible property accordingly.
    // If there is only string text in the header, we disable hit testing so that touching down on the HeaderHost still scrolls.
    // If there is a UIElement set as the header, we allow hit testing.  Any TextBlock's here that should not interfere with scrolling
    // must be manually set to IsHitTestVisible="False" by the customer.  Remember, HeaderHost overlays the ScrollViewer in z-order
    // and events on HeaderHost never route to the ScrollViewer since they are siblings.
    if (m_tpHeaderHostPart)
    {
        ctl::ComPtr<IDataTemplate> spHeaderTemplate;
        BOOLEAN hitTestVisible = TRUE;

        IFC(get_HeaderTemplate(&spHeaderTemplate));
        if (!spHeaderTemplate)
        {
            ctl::ComPtr<IInspectable> spHeader;

            IFC(get_Header(&spHeader));
            if (spHeader && !ctl::is<IUIElement>(spHeader.Get()))
            {
                // Only set IsHitTestVisible to FALSE on the HeaderHost for the case where the header is a non-null non-UIElement.
                // This matches the case for which ContentPresenter creates an internal TextBlock to display its Content.
                hitTestVisible = FALSE;
            }
        }

        IFC(m_tpHeaderHostPart.Cast<FrameworkElement>()->put_IsHitTestVisible(hitTestVisible));
    }

Cleanup:
    RRETURN(hr);
}

// Handler for the Loaded event on the m_tpPanel template part.
// Scrolls the DefaultSection into view (if DefaultSectionIndex is set) and makes sure its header gets focus.
_Check_return_ HRESULT
Hub::OnPanelLoaded(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN deferredScrollToRequestProcessed = FALSE;

    m_isPanelLoaded = TRUE;

    // check if we have any deferred scroll to request
    IFC(ProcessDeferredScrollToRequest(&deferredScrollToRequestProcessed));

    if (!deferredScrollToRequestProcessed)
    {
        // if wrapping is enabled, the default section is already the first section in Panel.
        INT defaultSectionIndex = -1;

        // If DefaultSectionIndex has been set to some value >= 0, then scroll the corresponding section into view and focus its header.
        IFC(get_DefaultSectionIndex(&defaultSectionIndex));
        if (defaultSectionIndex >= 0)
        {
            UINT size = 0;

            IFC(m_tpSections.Cast<HubSectionCollection>()->get_Size(&size));
            if (defaultSectionIndex < static_cast<INT>(size))
            {
                ctl::ComPtr<IHubSection> spSection;

                IFC(m_tpSections.Cast<HubSectionCollection>()->GetAt(defaultSectionIndex, &spSection));
                ASSERT(spSection);

                IFC(ScrollToSection(spSection.Get()));
                IFC(spSection.Cast<HubSection>()->TakeFocusOnLoaded(FocusStateForDefaultSection, FALSE /* semanticZoomMode */));
            }
            else
            {
                // We're going to make you crash because you specified DefaultSectionIndex greater
                // than or equal to the number of sections.
                IFC(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_HUB_DEFAULT_SECTION_INDEX_OUT_OF_RANGE));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Create HubAutomationPeer to represent the Hub.
IFACEMETHODIMP Hub::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IHubAutomationPeer> spHubAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IHubAutomationPeerFactory> spHubAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::HubAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spHubAPFactory));

    IFC(spHubAPFactory.Cast<HubAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spHubAutomationPeer));
    IFC(spHubAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

// Returns the FrameworkElement template part that hosts the Hub's Header.
_Check_return_ HRESULT Hub::GetHeaderHostPart(_Outptr_result_maybenull_ IFrameworkElement** ppHeaderHost)
{
    HRESULT hr = S_OK;

    IFCPTR(ppHeaderHost);
    *ppHeaderHost = NULL;

    if (m_tpHeaderHostPart)
    {
        IFC(m_tpHeaderHostPart.CopyTo(ppHeaderHost));
    }

Cleanup:
    RRETURN(hr);
}

// Returns the ScrollViewer template part that hosts the ScrollViewer.
_Check_return_ HRESULT Hub::GetScrollViewerPart(_Outptr_result_maybenull_ IScrollViewer** ppScrollViewer)
{
    HRESULT hr = S_OK;

    IFCPTR(ppScrollViewer);
    *ppScrollViewer = NULL;

    if (m_tpScrollViewer)
    {
        IFC(m_tpScrollViewer.CopyTo(ppScrollViewer));
    }

Cleanup:
    RRETURN(hr);
}

// Returns the Panel template part that hosts the HubSections.
_Check_return_ HRESULT Hub::GetPanelPart(_Outptr_result_maybenull_ xaml_controls::IPanel** ppPanel)
{
    HRESULT hr = S_OK;

    IFCPTR(ppPanel);
    *ppPanel = NULL;

    IFC(m_tpPanel.CopyTo(ppPanel));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Hub::OnHeaderSizeChanged(_In_ IInspectable* pSender, _In_ ISizeChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    DOUBLE headerHeight = 0.;

    IFC(m_tpHeaderHostPart->get_ActualHeight(&headerHeight));
    if (headerHeight != GetHubHeaderHeight())
    {
        SetHubHeaderHeight(headerHeight);
        IFC(RefreshHubSectionPlaceholderHeights());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Hub::RefreshHubSectionPlaceholderHeights()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IIterator<xaml_controls::HubSection*>> spIterator;
    BOOLEAN hasCurrent = FALSE;

    IFC(m_tpSections.Cast<HubSectionCollection>()->First(&spIterator));
    IFC(spIterator->get_HasCurrent(&hasCurrent));
    while (hasCurrent)
    {
        ctl::ComPtr<xaml_controls::IHubSection> spSection;

        IFC(spIterator->get_Current(&spSection));
        IFC(spSection.Cast<HubSection>()->RefreshHubHeaderPlaceholderHeight());
        IFC(spIterator->MoveNext(&hasCurrent));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Hub::RaiseSectionHeaderClick(
    _In_ HubSection* pSection)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<HubSectionHeaderClickEventArgs> spArgs;
    SectionHeaderClickEventSourceType* pEventSource = nullptr;

    IFC(ctl::make(&spArgs));
    ASSERT(pSection);
    IFC(spArgs->put_Section(pSection));

    IFC(HubGenerated::GetSectionHeaderClickEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), spArgs.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Hub::get_SectionsInViewImpl(
    _Outptr_ wfc::IVector<xaml_controls::HubSection*>** pValue)
{
    HRESULT hr = S_OK;

    IFC(m_tpSectionsInView.CopyTo(pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Hub::ScrollToSectionImpl(
    _In_ xaml_controls::IHubSection* section)
{
    HRESULT hr = S_OK;

    // either Panel is not loaded, or OnApplyTemplate is not called (the Panel template part is not set yet)
    // cache the request and we'll process this request when Panel is loaded.
    if (!m_isPanelLoaded)
    {
        SetPtrValue(m_tpDeferredScrollToSection, section);
    }
    else
    {
        UINT index = 0;
        BOOLEAN found = FALSE;
        ctl::ComPtr<wfc::IVector<IInspectable*>> spSections;

        ASSERT(m_tpPanel);
        IFC(m_tpSections.As(&spSections));

        IFC(spSections->IndexOf(section, &index, &found));
        if (found)
        {
            if (ctl::is<IModernCollectionBasePanel>(m_tpPanel.Get()))
            {
                IFC(m_tpPanel.Cast<ModernCollectionBasePanel>()->ScrollItemIntoView(index, xaml_controls::ScrollIntoViewAlignment_Leading, 0.0 /* offset */, TRUE /* forceSynchronous */));
            }
            else if (ctl::is<IStackPanel>(m_tpPanel.Get()))
            {
                ctl::ComPtr<IDependencyObject> spParent;
                ctl::ComPtr<IScrollInfo> spScrollInfo;

                IFC(VisualTreeHelper::GetParentStatic(m_tpPanel.Cast<StackPanel>(), &spParent));
                spScrollInfo = spParent.AsOrNull<IScrollInfo>();
                if (spScrollInfo)
                {
                    xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;
                    DOUBLE totalItemSize = 0;

                    IFC(m_tpPanel.Cast<StackPanel>()->get_Orientation(&orientation));

                    {
#ifdef DBG
                        UINT sectionsCount = 0;
                        IFC(spSections->get_Size(&sectionsCount));
                        ASSERT(index < sectionsCount);
#endif
                    }
                    // Accumulate the sizes in the orientation direction of the sections in front of the target section.
                    for (UINT i = 0; i < index; ++i)
                    {
                        ctl::ComPtr<IInspectable> spSectionAsII;
                        ctl::ComPtr<IHubSection> spSection;
                        wf::Size itemSize = {};

                        IFC(spSections->GetAt(i, &spSectionAsII));
                        IFC(spSectionAsII.As(&spSection));
                        IFC(spSection.Cast<HubSection>()->get_DesiredSize(&itemSize));

                        if (orientation == xaml_controls::Orientation_Vertical)
                        {
                            totalItemSize += itemSize.Height;
                        }
                        else
                        {
                            totalItemSize += itemSize.Width;
                        }
                    }

                    if (orientation == xaml_controls::Orientation_Vertical)
                    {
                        BOOLEAN canScroll = FALSE;

                        IFC(spScrollInfo->get_CanVerticallyScroll(&canScroll));
                        if (canScroll)
                        {
                            IFC(spScrollInfo->SetVerticalOffset(totalItemSize));
                        }
                    }
                    else
                    {
                        BOOLEAN canScroll = FALSE;

                        IFC(spScrollInfo->get_CanHorizontallyScroll(&canScroll));
                        if (canScroll)
                        {
                            IFC(spScrollInfo->SetHorizontalOffset(totalItemSize));
                        }
                    }
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Hub::get_SectionHeadersImpl(
    _Outptr_ wfc::IObservableVector<IInspectable*>** pValue)
{
    HRESULT hr = S_OK;

    IFC(m_tpSectionHeaders.CopyTo(pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Hub::OnPanelVisibleIndicesChanged(IInspectable*, IInspectable*)
{
    HRESULT hr = S_OK;

    ASSERT(m_tpPanel && ctl::is<IModernCollectionBasePanel>(m_tpPanel.Get()));

    // Now really update the collection
    IFC(UpdateSectionsInView());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Hub::UpdateSectionsInView()
{
    std::map<UINT, ctl::ComPtr<xaml_controls::IHubSection>> sectionsInViewMap;
    ctl::ComPtr<ReadOnlyTrackerCollection<xaml_controls::HubSection*>> spSectionsInViewNew;
    ctl::ComPtr<ReadOnlyTrackerCollection<xaml_controls::HubSection*>> spSectionsAddedToView;
    INT firstVisibleIndex = -1;
    INT lastVisibleIndex = -1;

    // All sections will be treated as leaving the view unless we find them in the new range of indices.
    ctl::ComPtr<ReadOnlyTrackerCollection<xaml_controls::HubSection*>> spSectionsRemovedFromView = m_tpSectionsInView.Get();

    // Vector tracking the sections that are newly added and weren't in view before.
    IFC_RETURN(ctl::make(&spSectionsAddedToView));

    if (m_tpPanel && ctl::is<IModernCollectionBasePanel>(m_tpPanel.Get()))
    {
        IFC_RETURN(m_tpPanel.Cast<ModernCollectionBasePanel>()->get_FirstVisibleIndexBase(&firstVisibleIndex));
        IFC_RETURN(m_tpPanel.Cast<ModernCollectionBasePanel>()->get_LastVisibleIndexBase(&lastVisibleIndex));

        // Now, append the sections the panel thinks are visible
        for (INT i = firstVisibleIndex; i >= 0 && i <= lastVisibleIndex; ++i)
        {
            ctl::ComPtr<IDependencyObject> spContainer;
            ctl::ComPtr<IHubSection> spSection;

            IFC_RETURN(m_tpPanel.Cast<ModernCollectionBasePanel>()->ContainerFromIndex(i, &spContainer));
            IFC_RETURN(spContainer.As(&spSection));
            if (spSection)
            {
                UINT index = 0;
                BOOLEAN found = FALSE;
                IFC_RETURN(m_tpSections.Cast<HubSectionCollection>()->IndexOf(spSection.Get(), &index, &found));
                ASSERT(found);
                sectionsInViewMap[index] = spSection;
            }
        }
    }

    // Now that all sections have been added to the maps, we can prepare the final sectionsInView collection
    // We have to start with the section visible on the left edge of the viewport
    // As the sectionsInViewMap is ordered (map) and not too large, we can afford to a linear walk
    // Note that the map is ordered by the index relative to the default section
    // Which basically means that we will only skip the possible sections in peek through area
    UINT skipped = 0;
    bool isSkipping = true;
    DOUBLE viewportOffset = 0.;

    if (m_tpScrollViewer)
    {
        IFC_RETURN(m_tpScrollViewer->get_HorizontalOffset(&viewportOffset));
    }
    IFC_RETURN(ctl::make(&spSectionsInViewNew));
    for (auto it = begin(sectionsInViewMap); it != end(sectionsInViewMap); ++it)
    {
        HubSection *pSection = it->second.Cast<HubSection>();
        if (isSkipping)
        {
            DOUBLE sectionWidth = 0.;
            IFC_RETURN(pSection->get_ActualWidth(&sectionWidth));
            if (sectionWidth > viewportOffset)
            {
                isSkipping = false;
                IFC_RETURN(spSectionsInViewNew->InternalAppend(pSection));
            }
            else
            {
                ++skipped;
            }
        }
        else
        {
            IFC_RETURN(spSectionsInViewNew->InternalAppend(pSection));
        }
    }
    // Let's add the skipped sections
    if (skipped > 0)
    {
        for (auto it = begin(sectionsInViewMap); (skipped-- > 0) && (it != end(sectionsInViewMap)); ++it)
        {
            IFC_RETURN(spSectionsInViewNew->InternalAppend(it->second.Get()));
        }
    }

    // Now, we can walk the list of in-view sections, and see which ones were added/removed
    UINT sectionsInViewCount;
    IFC_RETURN(spSectionsInViewNew->get_Size(&sectionsInViewCount));
    for (UINT index = 0; index < sectionsInViewCount; ++index)
    {
        ctl::ComPtr<IHubSection> spSection;
        IFC_RETURN(spSectionsInViewNew->GetAt(index, &spSection));

        // See if the Section was already in the SectionsInView range.
        UINT oldIndex;
        BOOLEAN found;
        IFC_RETURN(spSectionsRemovedFromView->IndexOf(spSection.Get(), &oldIndex, &found));

        if (found)
        {
            // If it was found, then it is remaining in view.
            IFC_RETURN(spSectionsRemovedFromView->InternalRemoveAt(oldIndex));
        }
        else
        {
            // If it wasn't found, then it is being brought into view.
            IFC_RETURN(spSectionsAddedToView->InternalAppend(spSection.Get()));
        }
    }

    SetPtrValue(m_tpSectionsInView, spSectionsInViewNew);

    IFC_RETURN(RaiseSectionsInViewChanged(spSectionsRemovedFromView.Get(), spSectionsAddedToView.Get()));

    return S_OK;
}

_Check_return_ HRESULT
Hub::RaiseSectionsInViewChanged(
    _In_ wfc::IVector<xaml_controls::HubSection*>* pSectionsRemoved,
    _In_ wfc::IVector<xaml_controls::HubSection*>* pSectionsAdded)
{
    HRESULT hr = S_OK;
    UINT addedCount = 0;
    UINT removedCount = 0;

    IFC(pSectionsAdded->get_Size(&addedCount));
    IFC(pSectionsRemoved->get_Size(&removedCount));

    if (addedCount > 0 || removedCount > 0)
    {
        ctl::ComPtr<SectionsInViewChangedEventArgs> spArgs;
        SectionsInViewChangedEventSourceType* pEventSource = nullptr;

        IFC(ctl::make<SectionsInViewChangedEventArgs>(&spArgs));

        IFC(spArgs->put_RemovedSections(pSectionsRemoved));
        IFC(spArgs->put_AddedSections(pSectionsAdded));

        IFC(HubGenerated::GetSectionsInViewChangedEventSourceNoRef(&pEventSource));
        IFC(pEventSource->Raise(ctl::as_iinspectable(this), spArgs.Get()));
    }

Cleanup:
    RRETURN(hr);
}

// if user calls ScrollToSection before Panel is loaded, cache the request and handle the request in PanelLoaded event (or when EntranceAnimation is done for Phone)
_Check_return_ HRESULT Hub::ProcessDeferredScrollToRequest(_Out_ BOOLEAN* pProcessed)
{
    HRESULT hr = S_OK;

    *pProcessed = FALSE;

    if (m_tpDeferredScrollToSection)
    {
        UINT index = 0;
        BOOLEAN found = FALSE;

        IFC(m_tpSections->IndexOf(m_tpDeferredScrollToSection.Get(), &index, &found));
        if (found)
        {
            IFC(ScrollToSection(m_tpDeferredScrollToSection.Get()));
            *pProcessed = TRUE;
        }
        m_tpDeferredScrollToSection.Clear();
    }

Cleanup:
    RRETURN(hr);
}