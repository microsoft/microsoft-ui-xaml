// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AutoSuggestBox.g.h"
#include "AutoSuggestBox_Partial.h"
#include "ListView_Partial.h"
#include "AutoSuggestBoxTextChangedEventArgs.g.h"
#include "AutoSuggestBoxTextChangedEventArgs_Partial.h"
#include "AutoSuggestBoxSuggestionChosenEventArgs.g.h"
#include "AutoSuggestBoxQuerySubmittedEventArgs.g.h"
#include "AutoSuggestBoxAutomationPeer.g.h"
#include "IconElement.g.h"
#include "KeyRoutedEventArgs.g.h"
#include "SymbolIcon.g.h"
#include "TextBox.g.h"
#include "ButtonBase.g.h"
#include "ListViewBase.g.h"
#include "Window.g.h"
#include "ScrollViewer.g.h"
#include "NamespaceAliases.h"
#include "KeyRoutedEventArgs.g.h"
#include "Rectangle.g.h"
#include "Grid.g.h"
#include "Popup.g.h"
#include "PopupRoot.g.h"
#include "CompositeTransform.g.h"
#include "FullWindowMediaRoot.g.h"
#include "XboxUtility.h"
#include <LightDismissOverlayHelper.h>
#include "PropertyPath.h"
#include "PropertyPathParser.h"
#include "VisualTreeHelper.h"
#include "Callback.h"
#include "AutomationProperties.h"
#include <XamlOneCoreTransforms.h>
#include "localizedResource.h"
#include "RectUtil.h"
#include <WRLHelper.h>
#include "ElevationHelper.h"
#include <windows.ui.viewmanagement.h>

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace std::placeholders;

const INT64 AutoSuggestBox::s_textChangedEventTimerDuration = 1500000L;    // 150ms

const unsigned int AutoSuggestBox::s_minSuggestionListHeight = 178; // 178 pixels = 44 pixels * 4 items + 2 pixels

//#define DBG_TRACE(...) TRACE(TraceAlways, __VA_ARGS__)
#define DBG_TRACE(...)

const WCHAR AutoSuggestBox::c_TextBoxName[] = L"TextBox";
const WCHAR AutoSuggestBox::c_TextBoxQueryButtonName[] = L"QueryButton";
const WCHAR AutoSuggestBox::c_SuggestionsPopupName[] = L"SuggestionsPopup";
const WCHAR AutoSuggestBox::c_SuggestionsListName[] = L"SuggestionsList";
const WCHAR AutoSuggestBox::c_SuggestionsContainerName[] = L"SuggestionsContainer";
const WCHAR AutoSuggestBox::c_UpwardTransformName[] = L"UpwardTransform";
const WCHAR AutoSuggestBox::c_TextBoxScrollViewerName[] = L"ContentElement";
const WCHAR AutoSuggestBox::c_VisualStateLandscape[] = L"Landscape";
const WCHAR AutoSuggestBox::c_VisualStatePortrait[] = L"Portrait";
const WCHAR AutoSuggestBox::c_ListItemOrderTransformName[] = L"ListItemOrderTransform";
const WCHAR AutoSuggestBox::c_LayoutRootName[] = L"LayoutRoot";
const WCHAR AutoSuggestBox::c_RequiredHeaderName[] = L"RequiredHeaderPresenter";

bool AutoSuggestBox::m_sSipIsOpen = false;

AutoSuggestBox::AutoSuggestBox(){}

AutoSuggestBox::~AutoSuggestBox()
{
    if (m_tpInputPane)
    {
        if (m_sipEvents[0].value != 0)
        {
            VERIFYHR(m_tpInputPane->remove_Showing(m_sipEvents[0]));
        }
        if (m_sipEvents[1].value != 0)
        {
            VERIFYHR(m_tpInputPane->remove_Hiding(m_sipEvents[1]));
        }
    }

    // Should have been clean up in the unloaded handler.
    ASSERT(!m_isOverlayVisible);
}

//------------------------------------------------------------------------------
// AutoSuggestBox PrepareState
//
// Prepares this control by attaching needed event handlers
//------------------------------------------------------------------------------
_Check_return_ HRESULT
    AutoSuggestBox::PrepareState()
{
    wrl::ComPtr<wuv::IInputPaneStatics> spInputPaneStatics;
    wrl::ComPtr<wuv::IInputPane> spInputPane;
    wrl::ComPtr<xaml::IDispatcherTimer> spTextChangedEventTimer;
    wf::TimeSpan interval;

    IFC_RETURN(AutoSuggestBoxGenerated::PrepareState());

    // If we're in the context of XAML islands, then we don't want to use InputPane -
    // that requires a UIContext instance, which is not supported in WinUI 3.
    if (DXamlCore::GetCurrent()->GetHandle()->GetInitializationType() != InitializationType::IslandsOnly)
    {
        // Acquire an instance to IInputPane. It is used to listen on
        // SIP events and queries that type for the size occupied by
        // that SIP when it is visible, so we can position this
        // control correctly.

        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_InputPane).Get(),
            &spInputPaneStatics));

        IFC_RETURN(spInputPaneStatics->GetForCurrentView(&spInputPane)); //Task 23548475 Get correct input pane without UIContext.

        SetPtrValue(m_tpInputPane, spInputPane.Get());

        // listen on visibility changes:
        IFC_RETURN(spInputPane->add_Showing(
            wrl::Callback<InputPaneVisibilityEventHandler>(this, &AutoSuggestBox::OnSipShowing).Get(),
            &m_sipEvents[0]));

        IFC_RETURN(spInputPane->add_Hiding(
            wrl::Callback<InputPaneVisibilityEventHandler>(this, &AutoSuggestBox::OnSipHiding).Get(),
            &m_sipEvents[1]));
    }

    IFC_RETURN(wf::ActivateInstance(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_DispatcherTimer).Get(),
        &spTextChangedEventTimer));
    SetPtrValue(m_tpTextChangedEventTimer, spTextChangedEventTimer.Get());

    interval.Duration = s_textChangedEventTimerDuration;
    IFC_RETURN(spTextChangedEventTimer->put_Interval(interval));

    IFC_RETURN(m_epUnloadedEventHandler.AttachEventHandler(
        this,
        std::bind(&AutoSuggestBox::OnUnloaded, this, _1, _2)));

    IFC_RETURN(m_epSizeChangedEventHandler.AttachEventHandler(
        this,
        std::bind(&AutoSuggestBox::OnSizeChanged, this, _1, _2)));

    return S_OK;
}

IFACEMETHODIMP AutoSuggestBox::put_IsSuggestionListOpen(_In_ BOOLEAN value)
{
    IFC_RETURN(AutoSuggestBoxGenerated::put_IsSuggestionListOpen(value));

    // When we programmatically set IsSuggestionListOpen to true, we want to take focus.
    if (value)
    {
        BOOLEAN succeeded = FALSE;
        IFC_RETURN(Focus(xaml::FocusState_Programmatic, &succeeded));
    }

    return S_OK;
}

IFACEMETHODIMP
AutoSuggestBox::OnApplyTemplate()
{
    ctl::ComPtr<xaml_controls::ITextBox> spTextBoxPart;
    ctl::ComPtr<xaml_primitives::ISelector> spSuggestionsPart;
    ctl::ComPtr<xaml_primitives::IPopup> spPopupPart;
    ctl::ComPtr<xaml::IFrameworkElement> spSuggestionsContainerPart;
    ctl::ComPtr<xaml_media::ITranslateTransform> spUpwardTransformPart;
    ctl::ComPtr<xaml_media::ITransform> spListItemOrderTransformPart;
    ctl::ComPtr<xaml_controls::IGrid> layoutRoot;
    ctl::ComPtr<xaml::IUIElement> requiredHeaderPart;
    m_wkRootScrollViewer.Reset();

    // unwire old template part (if existing)
    if (m_tpSuggestionsPart)
    {
        ctl::ComPtr<xaml_controls::IListViewBase> spListViewPart;

        IFC_RETURN(m_tpSuggestionsPart.As(&spListViewPart));
        if (spListViewPart)
        {
            IFC_RETURN(m_epListViewItemClickEventHandler.DetachEventHandler(spListViewPart.Get()));
            IFC_RETURN(m_epListViewContainerContentChangingEventHandler.DetachEventHandler(spListViewPart.Get()));
        }

        IFC_RETURN(m_epSuggestionSelectionChangedEventHandler.DetachEventHandler(m_tpSuggestionsPart.Get()));
        IFC_RETURN(m_suggestionListKeyDownEventHandler.DetachEventHandler(m_tpSuggestionsPart.Get()));
    }

    if (m_tpPopupPart)
    {
        IFC_RETURN(m_epPopupOpenedEventHandler.DetachEventHandler(m_tpPopupPart.Get()));
    }

    if (m_tpSuggestionsContainerPart)
    {
        IFC_RETURN(m_epSuggestionsContainerLoadedEventHandler.DetachEventHandler(m_tpSuggestionsContainerPart.Get()));
    }

    if (m_tpTextBoxPart)
    {
        IFC_RETURN(m_epTextBoxTextChangedEventHandler.DetachEventHandler(iinspectable_cast(m_tpTextBoxPart.Cast<TextBox>())));
        IFC_RETURN(m_epTextBoxLoadedEventHandler.DetachEventHandler(iinspectable_cast(m_tpTextBoxPart.Cast<TextBox>())));
        IFC_RETURN(m_epTextBoxCandidateWindowBoundsChangedEventHandler.DetachEventHandler(m_tpTextBoxPart.Get()));
    }

    if (m_tpTextBoxQueryButtonPart)
    {
        IFC_RETURN(ClearTextBoxQueryButtonIcon());
        IFC_RETURN(m_epQueryButtonClickEventHandler.DetachEventHandler(m_tpTextBoxQueryButtonPart.Get()));
    }

    m_tpSuggestionsPart.Clear();
    m_tpPopupPart.Clear();
    m_tpSuggestionsContainerPart.Clear();
    m_tpTextBoxPart.Clear();
    m_tpUpwardTransformPart.Clear();
    m_tpListItemOrderTransformPart.Clear();
    m_tpLayoutRootPart.Clear();
     m_requiredHeaderPresenterPart.Clear();

    IFC_RETURN(AutoSuggestBoxGenerated::OnApplyTemplate());

    IFC_RETURN(HookToRootScrollViewer());

    IFC_RETURN(GetTemplatePart<ITextBox>(STR_LEN_PAIR(c_TextBoxName), spTextBoxPart.ReleaseAndGetAddressOf()));
    IFC_RETURN(GetTemplatePart<ISelector>(STR_LEN_PAIR(c_SuggestionsListName), spSuggestionsPart.ReleaseAndGetAddressOf()));
    IFC_RETURN(GetTemplatePart<IPopup>(STR_LEN_PAIR(c_SuggestionsPopupName), spPopupPart.ReleaseAndGetAddressOf()));
    IFC_RETURN(GetTemplatePart<IFrameworkElement>(STR_LEN_PAIR(c_SuggestionsContainerName), spSuggestionsContainerPart.ReleaseAndGetAddressOf()));
    IFC_RETURN(GetTemplatePart<ITranslateTransform>(STR_LEN_PAIR(c_UpwardTransformName), spUpwardTransformPart.ReleaseAndGetAddressOf()));
    IFC_RETURN(GetTemplatePart<ITransform>(STR_LEN_PAIR(c_ListItemOrderTransformName), spListItemOrderTransformPart.ReleaseAndGetAddressOf()));
    IFC_RETURN(GetTemplatePart<IGrid>(STR_LEN_PAIR(c_LayoutRootName), layoutRoot.ReleaseAndGetAddressOf()));

    if (IsValueRequired(this))
    {
        IFC_RETURN(GetTemplatePart<IUIElement>(STR_LEN_PAIR(c_RequiredHeaderName), requiredHeaderPart.ReleaseAndGetAddressOf()));
    }
    SetPtrValue(m_requiredHeaderPresenterPart, requiredHeaderPart.Get());

    SetPtrValue(m_tpTextBoxPart, spTextBoxPart.Get());
    SetPtrValue(m_tpSuggestionsPart, spSuggestionsPart.Get());
    SetPtrValue(m_tpPopupPart, spPopupPart.Get());
    SetPtrValue(m_tpSuggestionsContainerPart, spSuggestionsContainerPart.Get());
    SetPtrValue(m_tpUpwardTransformPart, spUpwardTransformPart.Get());
    SetPtrValue(m_tpListItemOrderTransformPart, spListItemOrderTransformPart.Get());
    SetPtrValue(m_tpLayoutRootPart, layoutRoot.Get());

    if (m_tpTextBoxPart)
    {
        wrl_wrappers::HString originalText;

        IFC_RETURN(m_epTextBoxTextChangedEventHandler.AttachEventHandler(
            m_tpTextBoxPart.Cast<TextBox>(),
            std::bind(&AutoSuggestBox::OnTextBoxTextChanged, this, _1, _2)));

        // added code to initialize text box
        // retrieving original text set by user in the "Text" field
        // updating the text box text with the original text
        IFC_RETURN(get_Text(originalText.GetAddressOf()));
        IFC_RETURN(UpdateTextBoxText(originalText, xaml_controls::AutoSuggestionBoxTextChangeReason_ProgrammaticChange));

        IFC_RETURN(m_epTextBoxLoadedEventHandler.AttachEventHandler(
            m_tpTextBoxPart.Cast<TextBox>(),
            std::bind(&AutoSuggestBox::OnTextBoxLoaded, this, _1, _2)));

        IFC_RETURN(m_epTextBoxUnloadedEventHandler.AttachEventHandler(
            m_tpTextBoxPart.Cast<TextBox>(),
            std::bind(&AutoSuggestBox::OnTextBoxUnloaded, this, _1, _2)));

        IFC_RETURN(m_epTextBoxCandidateWindowBoundsChangedEventHandler.AttachEventHandler(
            m_tpTextBoxPart.Cast<TextBox>(),
            std::bind(&AutoSuggestBox::OnTextBoxCandidateWindowBoundsChanged, this, _1, _2)));

        // Add the automation name from the group to the edit box.
        wrl_wrappers::HString automationName;
        IFC_RETURN(AutomationProperties::GetNameStatic(this, automationName.ReleaseAndGetAddressOf()));
        if (automationName.Get() != nullptr)
        {
            IFC_RETURN(AutomationProperties::SetNameStatic(m_tpTextBoxPart.Cast<TextBox>(), automationName));
        }

        // Pass our validation context and command onto the editable textbox
        ctl::ComPtr<xaml_controls::IInputValidationContext> context;
        IFC_RETURN(get_ValidationContext(&context));
        ctl::ComPtr<xaml_controls::IInputValidationControl> textBoxValidation;
        IFC_RETURN(spTextBoxPart.As(&textBoxValidation));
        IFC_RETURN(textBoxValidation->put_ValidationContext(context.Get()));

        ctl::ComPtr<xaml_controls::IInputValidationCommand> command;
        IFC_RETURN(get_ValidationCommand(&command));
        ctl::ComPtr<xaml_controls::IInputValidationControl2> textBoxValidation2;
        IFC_RETURN(spTextBoxPart.As(&textBoxValidation2));
        IFC_RETURN(textBoxValidation2->put_ValidationCommand(command.Get()));
    }

    if (m_tpSuggestionsPart)
    {
        ctl::ComPtr<xaml_controls::IListViewBase> spListViewPart;

        IFC_RETURN(UpdateSuggestionListItemsSource());

        IFC_RETURN(m_tpSuggestionsPart.As(&spListViewPart));

        if (spListViewPart)
        {
            IFC_RETURN(m_epListViewItemClickEventHandler.AttachEventHandler(
                spListViewPart.Get(),
                std::bind(&AutoSuggestBox::OnListViewItemClick, this, _1, _2)));

            IFC_RETURN(m_epListViewContainerContentChangingEventHandler.AttachEventHandler(
                spListViewPart.Get(),
                std::bind(&AutoSuggestBox::OnListViewContainerContentChanging, this, _1, _2)));
        }

        IFC_RETURN(m_epSuggestionSelectionChangedEventHandler.AttachEventHandler(
            m_tpSuggestionsPart.Get(),
            std::bind(&AutoSuggestBox::OnSuggestionSelectionChanged, this, _1, _2)));

        IFC_RETURN(m_suggestionListKeyDownEventHandler.AttachEventHandler(
            m_tpSuggestionsPart.AsOrNull<IUIElement>().Get(),
               std::bind(&AutoSuggestBox::OnSuggestionListKeyDown, this, _1, _2)));

        ctl::ComPtr<ListView> spListView = m_tpSuggestionsPart.AsOrNull<ListView>();
        if (spListView)
        {
            spListView->SetAllowItemFocusFromUIA(false);
        }
    }

    if (m_tpPopupPart)
    {
        IFC_RETURN(m_epPopupOpenedEventHandler.AttachEventHandler(
            m_tpPopupPart.Get(),
            std::bind(&AutoSuggestBox::OnPopupOpened, this, _1, _2)));
    }

    if (m_tpSuggestionsContainerPart)
    {
        IFC_RETURN(m_epSuggestionsContainerLoadedEventHandler.AttachEventHandler(
            m_tpSuggestionsContainerPart.Get(),
            std::bind(&AutoSuggestBox::OnSuggestionsContainerLoaded, this, _1, _2)));
    }

    if (m_tpTextChangedEventTimer)
    {
        IFC_RETURN(m_epTextChangedEventTimerTickEventHandler.AttachEventHandler(
            m_tpTextChangedEventTimer.Get(),
            std::bind(&AutoSuggestBox::OnTextChangedEventTimerTick, this, _1, _2)));
    }

    IFC_RETURN(ReevaluateIsOverlayVisible());

    return S_OK;
}

_Check_return_ HRESULT AutoSuggestBox::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(AutoSuggestBoxGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::AutoSuggestBox_Text:
        {
            wrl_wrappers::HString strQueryText;

            IFC_RETURN(CValueBoxer::UnboxValue(args.m_pNewValue, strQueryText.GetAddressOf()));
            IFC_RETURN(UpdateTextBoxText(strQueryText.Get(), xaml_controls::AutoSuggestionBoxTextChangeReason_ProgrammaticChange));
            break;
        }

        case KnownPropertyIndex::AutoSuggestBox_QueryIcon:
        {
            IFC_RETURN(SetTextBoxQueryButtonIcon());
            break;
        }

        case KnownPropertyIndex::AutoSuggestBox_IsSuggestionListOpen:
        {
            BOOLEAN isOpen;

            IFC_RETURN(CValueBoxer::UnboxValue(args.m_pNewValue, &isOpen));

            // Only proceed if there is at least one island that's still alive. Otherwise we can crash when opening the
            // windowed popup when it tries to get its island in CPopup::EnsureWindowForWindowedPopup to check that the
            // popup didn't move between main Xaml islands.
            // Note: Tests running in UWP mode don't need this check, so count them as having islands.
            CCoreServices* core = DXamlCore::GetCurrent()->GetHandle();
            if (core->GetInitializationType() != InitializationType::IslandsOnly || core->HasXamlIslandRoots())
            {
                if (m_tpPopupPart)
                {
                    IFC_RETURN(m_tpPopupPart->put_IsOpen(isOpen));

                    if (!isOpen)
                    {
                        // In the desktop window, the focus moves into the AutoSuggestBox when the suggestion
                        // popup list is closed so m_suppressSuggestionListVisibility flag ensures the popup
                        // close when the AutoSuggestBox got the focus by closing the popup.
                        m_suppressSuggestionListVisibility = true;

                        // We should ensure that no element in the suggestion list is selected
                        // when the popup isn't open, since otherwise that opens up the possibility
                        // of interacting with the suggestion list even when it's closed.
                        m_ignoreSelectionChanges = true;
                        IFC_RETURN(m_tpSuggestionsPart.AsOrNull<ISelector>()->put_SelectedIndex(-1));
                        m_ignoreSelectionChanges = false;
                    }
                }

                IFC_RETURN(ReevaluateIsOverlayVisible());

                IFC_RETURN(SetCurrentControlledPeer(isOpen ? ControlledPeer::SuggestionsList : ControlledPeer::None));
            }

            break;
        }

        case KnownPropertyIndex::AutoSuggestBox_LightDismissOverlayMode:
        {
            IFC_RETURN(ReevaluateIsOverlayVisible());
            break;
        }

        case KnownPropertyIndex::AutoSuggestBox_TextMemberPath:
        {
            // TextMemberPath updated, release existing PropertyPathListener
            m_spPropertyPathListener = nullptr;
            break;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
AutoSuggestBox::OnUnloaded(_In_ IInspectable* /*pSender*/, _In_ xaml::IRoutedEventArgs* /*pArgs*/)
{
    if (!IsInLiveTree() && m_isOverlayVisible)
    {
        m_isOverlayVisible = false;
        IFC_RETURN(TeardownOverlayState());
    }

    return S_OK;
}

_Check_return_ HRESULT
AutoSuggestBox::OnTextBoxTextChanged(_In_ IInspectable*, _In_ xaml_controls::ITextChangedEventArgs* )
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strQueryText;
    ctl::ComPtr<AutoSuggestBoxTextChangedEventArgs> spEventArgs;

    m_textChangedCounter++;

    IFC(ctl::make(&spEventArgs));

    spEventArgs->SetCounter(m_textChangedCounter);
    IFC(spEventArgs->SetOwner(this));

    IFC(spEventArgs->put_Reason(m_textChangeReason));

    SetPtrValue(m_tpTextChangedEventArgs, static_cast<IAutoSuggestBoxTextChangedEventArgs*>(spEventArgs.Get()));

    IFC(m_tpTextChangedEventTimer->Stop());
    IFC(m_tpTextChangedEventTimer->Start());

    IFC(m_tpTextBoxPart->get_Text(strQueryText.GetAddressOf()));

    IFC(UpdateText(strQueryText.Get()));

    if (!m_ignoreTextChanges)
    {
        if (m_textChangeReason == xaml_controls::AutoSuggestionBoxTextChangeReason_UserInput)
        {
            m_userTypedText.Duplicate(strQueryText);
            // make sure the suggestion list is shown when user inputs
            IFC(UpdateSuggestionListVisibility());
        }

        if (m_tpSuggestionsPart)
        {
            INT32 selectedIndex = 0;

            IFC(m_tpSuggestionsPart.AsOrNull<ISelector>()->get_SelectedIndex(&selectedIndex));

            if (-1 != selectedIndex)
            {
                m_ignoreSelectionChanges = true;
                IFC(m_tpSuggestionsPart.AsOrNull<ISelector>()->put_SelectedIndex(-1));
                m_ignoreSelectionChanges = false;
            }
        }
    }

Cleanup:
    m_textChangeReason = xaml_controls::AutoSuggestionBoxTextChangeReason_UserInput;

    RRETURN(hr);
}

_Check_return_ HRESULT
AutoSuggestBox::OnTextBoxCandidateWindowBoundsChanged(
    _In_ xaml_controls::ITextBox* /*pSender*/,
    _In_ xaml_controls::ICandidateWindowBoundsChangedEventArgs* pArgs)
{
    wf::Rect candidateWindowBounds;

    IFC_RETURN(pArgs->get_Bounds(&candidateWindowBounds));

    // do nothing if the candidate windows bound did not change
    if (RectUtil::AreEqual(m_candidateWindowBoundsRect,candidateWindowBounds))
    {
        return S_OK;
    }

    m_candidateWindowBoundsRect = candidateWindowBounds;

    // When the candidate window bounds change, there are three things that we need to do,
    // since this changes the rect in which we need to draw the suggestion list:
    //
    // 1. Adjust the available height for and position of the suggestion list;
    // 2. Adjust the size of the suggestion list; and
    // 3. Adjust the position of the suggestion list.
    //
    // #1 must be done in a different way depending on whether or not the SIP is currently open,
    // hence the if statement.  The others are the same regardless.

    if (m_tpInputPane && m_sSipIsOpen)
    {
        wf::Rect sipOverlayArea;

        IFC_RETURN(m_tpInputPane->get_OccludedRect(&sipOverlayArea));
        IFC_RETURN(AlignmentHelper(sipOverlayArea));
    }
    else
    {
        IFC_RETURN(MaximizeSuggestionAreaWithoutInputPane());
    }

    IFC_RETURN(UpdateSuggestionListPosition());
    IFC_RETURN(UpdateSuggestionListSize());

    return S_OK;
}

//------------------------------------------------------------------------------
// Handler of the SizeChanged event.
//------------------------------------------------------------------------------
_Check_return_ HRESULT
AutoSuggestBox::OnSizeChanged(
    _In_ IInspectable* /*pSender*/,
    _In_ xaml::ISizeChangedEventArgs* /*pArgs*/)
{
    HRESULT hr = S_OK;

    if (m_tpSuggestionsContainerPart)
    {
        DOUBLE actualWidth = 0.;
        wrl::ComPtr<xaml::IFrameworkElement> spThisAsFE = this;

        IFC(spThisAsFE->get_ActualWidth(&actualWidth));
        IFC(m_tpSuggestionsContainerPart->put_Width(actualWidth));
    }

Cleanup:
    RRETURN(hr);
}

// This event handler is only for Gamepad or Remote cases, where Focus and Selection are tied.
// This is never invoked for Keyboard cases since Focus stays on the TextBox but Selection moves
// down the ListView so the key down events go to OnKeyDown event handler.
_Check_return_ HRESULT
AutoSuggestBox::OnSuggestionListKeyDown(
    _In_ IInspectable* pSender,
    _In_ xaml::Input::IKeyRoutedEventArgs* pArgs)
{
    auto key = wsy::VirtualKey_None;
    IFC_RETURN(pArgs->get_Key(&key));

    m_inputDeviceTypeUsed = DirectUI::InputDeviceType::GamepadOrRemote;

    boolean wasHandledLocally = false;

    switch (key)
    {
    case wsy::VirtualKey_Left:
    case wsy::VirtualKey_Right:
        // Since the SuggestionList is open, we don't allow horizontal movement.
        wasHandledLocally = true;
        break;

    case wsy::VirtualKey_Up:
    case wsy::VirtualKey_Down:
    {
        int selectedIndex = 0;
        IFC_RETURN(m_tpSuggestionsPart.AsOrNull<ISelector>()->get_SelectedIndex(&selectedIndex));

        UINT count = 0;
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
        IFC_RETURN(get_Items(&spItems));
        IFC_RETURN(spItems.AsOrNull<wfc::IVector<IInspectable*>>()->get_Size(&count));
        int lastIndex = count - 1;

        // If we are already at the Suggestion that is adjacent to the TextBox, then set SelectedIndex to -1.
        if ((selectedIndex == 0 && !IsSuggestionListVectorReversed()) ||
            (selectedIndex == lastIndex && IsSuggestionListVectorReversed()))
        {
            IFC_RETURN(UpdateTextBoxText(m_userTypedText.Get(), xaml_controls::AutoSuggestionBoxTextChangeReason_ProgrammaticChange));
            IFC_RETURN(m_tpSuggestionsPart.AsOrNull<ISelector>()->put_SelectedIndex(-1));

            BOOLEAN succeeded = FALSE;
            IFC_RETURN(m_tpTextBoxPart.AsOrNull<IUIElement>()->Focus(xaml::FocusState_Keyboard, &succeeded));
        }

        wasHandledLocally = true;
        break;
    }

    case wsy::VirtualKey_Space:
    case wsy::VirtualKey_Enter:
    {
        ctl::ComPtr<IInspectable> spSelectedItem;
        IFC_RETURN(m_tpSuggestionsPart->get_SelectedItem(&spSelectedItem));
        IFC_RETURN(SubmitQuery(spSelectedItem.Get()));
        IFC_RETURN(put_IsSuggestionListOpen(FALSE));
        wasHandledLocally = true;
    }
        break;

    case wsy::VirtualKey_Escape:
        // Reset the text in the TextBox to what the user had typed.
        IFC_RETURN(UpdateTextBoxText(m_userTypedText.Get(), xaml_controls::AutoSuggestionBoxTextChangeReason_ProgrammaticChange));
        // Close the suggestion list.
        IFC_RETURN(put_IsSuggestionListOpen(FALSE));
        // Return the focus to the TextBox.
        BOOLEAN succeeded = FALSE;
        IFC_RETURN(m_tpTextBoxPart.AsOrNull<IUIElement>()->Focus(xaml::FocusState_Keyboard, &succeeded));

        wasHandledLocally = true;
    }

    if (wasHandledLocally)
    {
        IFC_RETURN(pArgs->put_Handled(TRUE));
    }

    return S_OK;
}

//------------------------------------------------------------------------------
// Handler of the suggestions Popup's Opened event.
//
// Updates the suggestions list's position as its position can be changed before
// the previous Open operation.
//------------------------------------------------------------------------------
_Check_return_ HRESULT
AutoSuggestBox::OnPopupOpened(
    _In_ IInspectable* /*pSender*/,
    _In_ IInspectable* /*pArgs*/)
{
    TraceLoggingActivity<g_hTraceProvider, MICROSOFT_KEYWORD_TELEMETRY> traceLoggingActivity;

    // Telemetry marker for suggestion list opening popup.
    TraceLoggingWriteStart(traceLoggingActivity,
        "ASBSuggestionListOpened");

    // Bail out early if the popup has been unloaded already.
    // It's possible for this async Popup.Opened event to fire after the island that contains the Popup is already
    // closed.  If we continue on in this state, VisualTree::GetForElementNoRef() may return null, leading to failures.
    BOOLEAN isLoaded {};
    IFC_RETURN(m_tpPopupPart.AsOrNull<IFrameworkElement>()->get_IsLoaded(&isLoaded));
    if (!isLoaded)
    {
        return S_OK;
    }

    // Apply a shadow effect to the popup's immediate child
    IFC_RETURN(ApplyElevationEffect(m_tpPopupPart.AsOrNull<IUIElement>().Get()));

    IFC_RETURN(UpdateSuggestionListPosition());

    return S_OK;
}

//------------------------------------------------------------------------------
// Handler of the suggestions container's Loaded event.
//
// Sets the position of the suggestions list as soon as the container is loaded.
//------------------------------------------------------------------------------
_Check_return_ HRESULT
AutoSuggestBox::OnSuggestionsContainerLoaded(
    _In_ IInspectable* /*pSender*/,
    _In_ xaml::IRoutedEventArgs* /*pArgs*/ )
{
    IFC_RETURN(UpdateSuggestionListPosition());
    IFC_RETURN(UpdateSuggestionListSize());

    return S_OK;
}

//------------------------------------------------------------------------------
// Handler of the text box's Loaded event.
//
// Retrieves the query button if it exists and attaches a handler to its Click event.
//------------------------------------------------------------------------------
_Check_return_ HRESULT
AutoSuggestBox::OnTextBoxLoaded(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    ctl::ComPtr<xaml_primitives::IButtonBase> spTextBoxQueryButtonPart;

    IFC_RETURN(m_tpTextBoxPart.Cast<TextBox>()->GetTemplatePart<IButtonBase>(STR_LEN_PAIR(c_TextBoxQueryButtonName), spTextBoxQueryButtonPart.ReleaseAndGetAddressOf()));

    SetPtrValue(m_tpTextBoxQueryButtonPart, spTextBoxQueryButtonPart.Get());

    if (m_tpTextBoxQueryButtonPart)
    {
        IFC_RETURN(SetTextBoxQueryButtonIcon());
        if (!m_epQueryButtonClickEventHandler)
        {
            IFC_RETURN(m_epQueryButtonClickEventHandler.AttachEventHandler(
                m_tpTextBoxQueryButtonPart.Get(),
                std::bind(&AutoSuggestBox::OnTextBoxQueryButtonClick, this, _1, _2)));
        }
        // Update query button's AutomationProperties.Name to "Search" by default
        wrl_wrappers::HString automationName;
        IFC_RETURN(AutomationProperties::GetNameStatic(m_tpTextBoxQueryButtonPart.Cast<ButtonBase>(), automationName.ReleaseAndGetAddressOf()));
        if (automationName.Get() == nullptr)
        {
            IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AUTOSUGGESTBOX_QUERY, automationName.ReleaseAndGetAddressOf()));
            IFC_RETURN(DirectUI::AutomationProperties::SetNameStatic(m_tpTextBoxQueryButtonPart.Cast<ButtonBase>(), automationName));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------------
// Handler of the text box's Unloaded event.
//
// Removes the handler to the query button
//------------------------------------------------------------------------------
_Check_return_ HRESULT
AutoSuggestBox::OnTextBoxUnloaded(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    // Checking IsActive because Unloaded is async and we might have reloaded before this fires
    if (m_tpTextBoxQueryButtonPart && m_epQueryButtonClickEventHandler && !GetHandle()->IsActive())
    {
        IFC_RETURN(m_epQueryButtonClickEventHandler.DetachEventHandler(m_tpTextBoxQueryButtonPart.Get()));
    }

    return S_OK;
}

//------------------------------------------------------------------------------
// Handler of the query button's Click event.
//
// Raises the QuerySubmitted event.
//------------------------------------------------------------------------------
_Check_return_ HRESULT
AutoSuggestBox::OnTextBoxQueryButtonClick(
    _In_ IInspectable* /*pSender*/,
    _In_ xaml::IRoutedEventArgs* /*pArgs*/)
{
    IFC_RETURN(ProgrammaticSubmitQuery());

    return S_OK;
}

_Check_return_ HRESULT AutoSuggestBox::ProgrammaticSubmitQuery()
{
    // Clicking the query button should always submit the query solely with the text
    // in the TextBox, and should ignore any selected item in the suggestion list.
    // To ensure that, we'll deselect any item in the suggestion list that might be selected
    // before submitting the query.
    m_ignoreSelectionChanges = true;
    auto cleanupGuard = wil::scope_exit([&]
    {
        m_ignoreSelectionChanges = false;
    });

    IFC_RETURN(m_tpSuggestionsPart.AsOrNull<ISelector>()->put_SelectedIndex(-1));
    cleanupGuard.release();

    IFC_RETURN(SubmitQuery(nullptr));

    return S_OK;
}

//------------------------------------------------------------------------------
// Sets the value of QueryButton.Content to the current value of QueryIcon.
//------------------------------------------------------------------------------
_Check_return_ HRESULT
AutoSuggestBox::SetTextBoxQueryButtonIcon()
{
    HRESULT hr = S_OK;

    if (m_tpTextBoxQueryButtonPart)
    {
        static_cast<CFrameworkElement*>(m_tpTextBoxQueryButtonPart.Cast<ButtonBase>()->GetHandle())->SetCursor(MouseCursorArrow);
        ctl::ComPtr<xaml_controls::IIconElement> spQueryIcon;

        IFC(get_QueryIcon(&spQueryIcon));

        if (spQueryIcon)
        {
            ctl::ComPtr<xaml_controls::ISymbolIcon> spQueryIconAsSymbolIcon = spQueryIcon.AsOrNull<xaml_controls::ISymbolIcon>();

            if (spQueryIconAsSymbolIcon)
            {
                // Setting FontSize to zero prevents SymbolIcon from setting a static FontSize on it's child TextBlock,
                // allowing the binding to AutoSuggestBoxIconFontSize to be inherited properly.
                spQueryIconAsSymbolIcon.Cast<SymbolIcon>()->SetFontSize(0);
            }

            IFC(m_tpTextBoxQueryButtonPart.Cast<ButtonBase>()->put_Visibility(xaml::Visibility_Visible));
            IFC(m_tpTextBoxQueryButtonPart.Cast<ButtonBase>()->put_Content(ctl::iinspectable_cast(spQueryIcon.Cast<IconElement>())));
        }
        else
        {
            IFC(m_tpTextBoxQueryButtonPart.Cast<ButtonBase>()->put_Visibility(xaml::Visibility_Collapsed));
        }
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------------
// Sets the value of QueryButton.Content to the current value of QueryIcon.
//------------------------------------------------------------------------------
_Check_return_ HRESULT
AutoSuggestBox::ClearTextBoxQueryButtonIcon()
{
    HRESULT hr = S_OK;

    if (m_tpTextBoxQueryButtonPart)
    {
        IFC(m_tpTextBoxQueryButtonPart.Cast<ButtonBase>()->put_Content(nullptr));
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------------
// Raises the QuerySubmitted event using the current content of the TextBox.
//------------------------------------------------------------------------------
_Check_return_ HRESULT
AutoSuggestBox::SubmitQuery(_In_opt_ IInspectable* pChosenSuggestion)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strQueryText;
    ctl::ComPtr<AutoSuggestBoxQuerySubmittedEventArgs> spEventArgs;
    QuerySubmittedEventSourceType* pEventSource = nullptr;

    IFC(ctl::make(&spEventArgs));

    IFC(m_tpTextBoxPart->get_Text(strQueryText.GetAddressOf()));
    IFC(spEventArgs->put_QueryText(strQueryText.Get()));

    IFC(spEventArgs->put_ChosenSuggestion(pChosenSuggestion));

    IFC(GetQuerySubmittedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(this, spEventArgs.Get()));

    IFC(put_IsSuggestionListOpen(FALSE));

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------------
// AutoSuggestBox OnLostFocus event handler
//
// The suggestions drop-down will never be displayed when this control loses focus.
//------------------------------------------------------------------------------
IFACEMETHODIMP AutoSuggestBox::OnLostFocus(_In_ xaml::IRoutedEventArgs*)
{
    if (!m_keepFocus)
    {
        m_hasFocus = false;

        if (m_isSipVisible)
        {
            // checking to see if the focus went to a new element that contains a textbox
            // in this case, we leave it up to the new element to handle scrolling
            ctl::ComPtr<DependencyObject> spFocusedElement;
            IFC_RETURN(GetFocusedElement(&spFocusedElement));
            if (spFocusedElement)
            {
                ctl::ComPtr<xaml_controls::ITextBox> spFocusedElementAsTextBox;
                IGNOREHR(spFocusedElement.As(&spFocusedElementAsTextBox));
                if (!spFocusedElementAsTextBox)
                {
                    // This is for the case when the focus is changed to something that is not ASB or textbox
                    IFC_RETURN(OnSipHidingInternal());
                }
            }
            else
            {
                // This is for the case when focus is lost and no other control is in focus
                IFC_RETURN(OnSipHidingInternal());
            }
        }
        m_scrollActions.clear();
    }

    // When Gamepad or Remote is used, we move the focus from the AutoSuggestBox TextBox to the ListView.
    // But in other cases (when using keyboard), where we get OnLostFocus, we want to make sure to close suggestions list.
    // Note that Left/Right keys are handled by OnKeyDown and OnSuggestionListKeyDown handlers so the only time we are here
    // is when we "Tab" out of, or the focus is moved programmatically off of, the AutoSuggestBox TextBox.
    if (m_inputDeviceTypeUsed != DirectUI::InputDeviceType::GamepadOrRemote)
    {
        IFC_RETURN(put_IsSuggestionListOpen(FALSE));
    }

    return S_OK;
}

//------------------------------------------------------------------------------
// AutoSuggestBox OnGotFocus event handler
//
// The suggestions drop-down should be displayed if items are present and the textbox has text
//------------------------------------------------------------------------------
IFACEMETHODIMP AutoSuggestBox::OnGotFocus(_In_ xaml::IRoutedEventArgs*)
{
    HRESULT hr = S_OK;

    if (!m_keepFocus)
    {
        BOOLEAN isTextBoxFocused = FALSE;

        IFC(IsTextBoxFocusedElement(&isTextBoxFocused));
        if (isTextBoxFocused)
        {
            wrl_wrappers::HString strText;

            // this code handles the case when the control receives focus from another control
            // that was using the sip. In this case, OnSipShowing is not guaranteed to be called
            // hence, we align the control to the top or bottom depending on its position at the time
            // it received focus
            if (m_tpInputPane && m_sSipIsOpen)
            {
                wf::Rect sipOverlayArea;

                IFC(m_tpInputPane->get_OccludedRect(&sipOverlayArea));
                IFC(AlignmentHelper(sipOverlayArea));
            }

            // Expands the suggestion list if there is already text in the textbox
            IFC(m_tpTextBoxPart->get_Text(strText.GetAddressOf()));

            if (!strText.IsEmpty() && !m_suppressSuggestionListVisibility)
            {
                IFC(UpdateSuggestionListVisibility());
                m_suppressSuggestionListVisibility = false;
            }
        }
    }
    else
    {
        // making sure the ASB is aligned to where it should be
        IFC(ApplyScrollActions(TRUE));
    }

Cleanup:
    m_keepFocus = false;

    RRETURN(hr);
}

//------------------------------------------------------------------------------
// AutoSuggestBox OnKeyDown event handler
//
// Handle the proper KeyDown event to process Key_Down, KeyUp, Key_Enter and
// Key_Escape.
//
//  Key_Down/Key_Up is for navigating the suggestionlist.
//  Key_Enter is for choosing the current selection if there is a proper select item.
//   otherwise, do nothing.
//  Key_Escape is for closing the suggestion list or clear the current text.
//
//------------------------------------------------------------------------------
IFACEMETHODIMP
AutoSuggestBox::OnKeyDown(
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    auto key = wsy::VirtualKey_None;
    IFC_RETURN(pArgs->get_Key(&key));

    auto originalKey = wsy::VirtualKey_None;
    IFC_RETURN(static_cast<KeyRoutedEventArgs*>(pArgs)->get_OriginalKey(&originalKey));

    m_inputDeviceTypeUsed = XboxUtility::IsGamepadNavigationInput(originalKey) ? DirectUI::InputDeviceType::GamepadOrRemote : DirectUI::InputDeviceType::Keyboard;

    IFC_RETURN(AutoSuggestBoxGenerated::OnKeyDown(pArgs));

    if (m_tpSuggestionsPart)
    {
        boolean wasHandledLocally = false;

        int selectedIndex = 0;
        IFC_RETURN(m_tpSuggestionsPart.AsOrNull<ISelector>()->get_SelectedIndex(&selectedIndex));

        BOOLEAN isSuggestionListOpen = FALSE;
        IFC_RETURN(get_IsSuggestionListOpen(&isSuggestionListOpen));

        switch (key)
        {
        case wsy::VirtualKey_Left:
        case wsy::VirtualKey_Right:
            if (isSuggestionListOpen && m_inputDeviceTypeUsed == DirectUI::InputDeviceType::GamepadOrRemote)
            {
                // If SuggestionList is open, we don't allow horizontal movement
                // when using Gamepad or Remote.
                wasHandledLocally = true;
            }
            break;

        case wsy::VirtualKey_Up:
        case wsy::VirtualKey_Down:
            // If the suggestion list isn't open, we shouldn't be able to keyboard through it.
            if (isSuggestionListOpen)
            {
                ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
                IFC_RETURN(get_Items(&spItems));

                UINT count = 0;
                IFC_RETURN(spItems.AsOrNull<wfc::IVector<IInspectable*>>()->get_Size(&count));
                if (count > 0)
                {
                    int lastIndex = count - 1;

                    wsy::VirtualKeyModifiers modifiers = wsy::VirtualKeyModifiers_None;
                    IFC_RETURN(GetKeyboardModifiers(&modifiers));
                    const bool isForward = ShouldMoveIndexForwardForKey(key, modifiers);

                    IFC_RETURN(m_tpSuggestionsPart.AsOrNull<ISelector>()->get_SelectedIndex(&selectedIndex));

                    // The meaning of this conditional is really hard to parse by itself, so here's
                    // a much simpler way of saying the same thing:
                    //
                    // "If we only have one item (lastIndex == 0), and if it's already selected (selectedIndex == 0),
                    // then the arrow keys should do nothing."
                    //
                    // Basically, if lastIndex != 0, then since count > 0, lastIndex must be greater than 0.
                    // Conversely, if lastIndex == 0, then selectedIndex can only be equal to -1 (nothing selected)
                    // or 0 (the first and only item is selected).
                    //
                    // Note that lastIndex means "the index of the last item in the suggestion list", not
                    // "the previous index".
                    //
                    if (selectedIndex != 0 || lastIndex != 0)
                    {
                        // The following conditional was written to satisfy the table below which identifies the indices we
                        // need to go to for the different types of AutoSuggestBoxes (Suggestion List below, Suggestion List
                        // above with new implementation where vector is reversed, Suggestion List above with old implementation).
                        // Note that the "/NA" indicates that the index does not move when the input is coming from Gamepad/Remote
                        // because there is no looping behavior.
                        //
                        //                        ArrowKey  isForward   VectorReversed  indexToGoTo
                        // ASB_SuggestionsBelow     Down        1           0               0
                        //                          Up          0           0               lastIndex/NA
                        // ASB_SuggestionsAbove     Down        1           1               0/NA
                        //                (New)     Up          0           1               lastIndex
                        // ASB_SuggestionsAbove     Down        0           0               lastIndex/NA
                        //                (Old)     Up          1           0               0
                        if (selectedIndex == -1)
                        {
                            if (isForward)
                            {
                                if (!(IsSuggestionListVectorReversed() && m_inputDeviceTypeUsed == DirectUI::InputDeviceType::GamepadOrRemote))
                                {
                                    IFC_RETURN(m_tpSuggestionsPart.AsOrNull<ISelector>()->put_SelectedIndex(0));
                                }
                            }
                            else
                            {
                                if (!(!IsSuggestionListVectorReversed() && m_inputDeviceTypeUsed == DirectUI::InputDeviceType::GamepadOrRemote))
                                {
                                    IFC_RETURN(m_tpSuggestionsPart.AsOrNull<ISelector>()->put_SelectedIndex(lastIndex));
                                }
                            }
                        }
                        else if (selectedIndex == 0)
                        {
                            if (isForward)
                            {
                                IFC_RETURN(m_tpSuggestionsPart.AsOrNull<ISelector>()->put_SelectedIndex(selectedIndex + 1));
                            }
                            else
                            {
                                IFC_RETURN(UpdateTextBoxText(m_userTypedText.Get(), xaml_controls::AutoSuggestionBoxTextChangeReason_ProgrammaticChange));
                                IFC_RETURN(m_tpSuggestionsPart.AsOrNull<ISelector>()->put_SelectedIndex(-1));
                            }
                        }
                        else if (selectedIndex == lastIndex)
                        {
                            if (isForward)
                            {
                                IFC_RETURN(UpdateTextBoxText(m_userTypedText.Get(), xaml_controls::AutoSuggestionBoxTextChangeReason_ProgrammaticChange));
                                IFC_RETURN(m_tpSuggestionsPart.AsOrNull<ISelector>()->put_SelectedIndex(-1));
                            }
                            else
                            {
                                IFC_RETURN(m_tpSuggestionsPart.AsOrNull<ISelector>()->put_SelectedIndex(lastIndex - 1));
                            }
                        }
                        else
                        {
                            IFC_RETURN(m_tpSuggestionsPart.AsOrNull<ISelector>()->put_SelectedIndex(isForward ? selectedIndex + 1 : selectedIndex - 1));
                        }
                    }
                }
                wasHandledLocally = true;
            }
            break;

        case wsy::VirtualKey_Tab:
            // Only close the suggestion list and reset the user text with Tab if the suggestion
            // list is open.
            if (!isSuggestionListOpen)
            {
                break;
            }
        case wsy::VirtualKey_Escape:
        {
            // Reset the text in the TextBox to what the user had typed.
            IFC_RETURN(UpdateTextBoxText(m_userTypedText.Get(), xaml_controls::AutoSuggestionBoxTextChangeReason_ProgrammaticChange));
            // Close the suggestion list.
            IFC_RETURN(put_IsSuggestionListOpen(FALSE));
            // Return the focus to the TextBox.
            BOOLEAN succeeded = FALSE;
            IFC_RETURN(m_tpTextBoxPart.AsOrNull<IUIElement>()->Focus(xaml::FocusState_Keyboard, &succeeded));

            // Handle the key for Escape, but not for tab so that the default tab processing can take place.
            wasHandledLocally = (key != wsy::VirtualKey_Tab);
            break;
        }

        case wsy::VirtualKey_Enter:
            ctl::ComPtr<IInspectable> spSelectedItem;

            // If the AutoSuggestBox supports QueryIcon, then pressing the Enter key
            // will submit the current query - we'll already have set the text in the TextBox
            // in OnSuggestionSelectionChanged().
            IFC_RETURN(m_tpSuggestionsPart->get_SelectedItem(&spSelectedItem));
            IFC_RETURN(SubmitQuery(spSelectedItem.Get()));

            IFC_RETURN(put_IsSuggestionListOpen(FALSE));
            wasHandledLocally = true;
            break;
        }

        if (wasHandledLocally)
        {
            IFC_RETURN(pArgs->put_Handled(TRUE));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------------
// AutoSuggestBox IsFocusedElement event handler
//
// Queries the focused element from the focus manager to see if it's the textbox
//------------------------------------------------------------------------------
_Check_return_ HRESULT
    AutoSuggestBox::IsTextBoxFocusedElement(_Out_ BOOLEAN* pFocused)
{
    HRESULT hr = S_OK;

    *pFocused = FALSE;

    if (m_tpTextBoxPart)
    {
        ctl::ComPtr<DependencyObject> spFocusedElement;

        IFC(GetFocusedElement(&spFocusedElement));
        if (spFocusedElement)
        {
            ctl::ComPtr<xaml_controls::ITextBox> spFocusedElementAsTextBox;

            IGNOREHR(spFocusedElement.As(&spFocusedElementAsTextBox));
            if (spFocusedElementAsTextBox && spFocusedElementAsTextBox.Get() == m_tpTextBoxPart.Get())
            {
                *pFocused = TRUE;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------------
// AutoSuggestBox OnSipShowing
//
// This event handler will be called by the global InputPane when the SIP will
// be showing up from the bottom of the screen, here we try to scroll AutoSuggestBox
// to top or bottom with minimum visual glitch
//------------------------------------------------------------------------------
_Check_return_ HRESULT
    AutoSuggestBox::OnSipShowing(_In_ wuv::IInputPane*, _In_ wuv::IInputPaneVisibilityEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    // setting the static value to true
    m_sSipIsOpen = true;

    // the check here is to ensure that the arguments received are not null hence crashing our application
    // in some stress tests, null was encountered
    if (pArgs)
    {
        IFC(OnSipShowingInternal(pArgs));
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------------
// AutoSuggestBox OnSipHiding
//
// This event handler will be called by the global InputPane when the SIP is
// hiding. It reverts the scroll actions that are applied to bring the ASB
// to its original position.
//------------------------------------------------------------------------------
_Check_return_ HRESULT
    AutoSuggestBox::OnSipHiding(_In_ wuv::IInputPane*, _In_ wuv::IInputPaneVisibilityEventArgs*)
{
    // setting the static value to false
    m_sSipIsOpen = false;

    return ReevaluateIsOverlayVisible();
}

//------------------------------------------------------------------------------
// Updates the suggestion list's size based on the available space and the
// MaxSuggestionListHeight property.
//------------------------------------------------------------------------------
_Check_return_ HRESULT
    AutoSuggestBox::UpdateSuggestionListSize()
{
    HRESULT hr = S_OK;

    if (m_tpSuggestionsContainerPart)
    {
        DOUBLE maxSuggestionListHeight = 0.;
        DOUBLE actualWidth = 0.;
        ctl::ComPtr<xaml::IFrameworkElement> spThisAsFElement = this;

        IFC(get_MaxSuggestionListHeight(&maxSuggestionListHeight));

        // if the user specifies a negative value for the maxsuggestionlistsize, we use the available size
        if ((m_availableSuggestionHeight > 0 && maxSuggestionListHeight > m_availableSuggestionHeight) || maxSuggestionListHeight < 0)
        {
            maxSuggestionListHeight = m_availableSuggestionHeight;
        }

        IFC(m_tpSuggestionsContainerPart->put_MaxHeight(maxSuggestionListHeight));

        IFC(spThisAsFElement->get_ActualWidth(&actualWidth));
        IFC(m_tpSuggestionsContainerPart->put_Width(actualWidth));
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------------
// Opens the suggestion list if there is at least one item in the items collection.
//------------------------------------------------------------------------------
_Check_return_ HRESULT
    AutoSuggestBox::UpdateSuggestionListVisibility()
{
    HRESULT hr = S_OK;

    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItemsReference;
    ctl::ComPtr<wfc::IVector<IInspectable*>> spSuggestionsCollectionReference;
    BOOLEAN isOpen = FALSE;

    // if the suggestion container exists, we are retrieving its maxsuggestionlistheight
    DOUBLE maxHeight = 0.;
    if (m_tpSuggestionsContainerPart)
    {
        IFC(m_tpSuggestionsContainerPart->get_MaxHeight(&maxHeight));
    }

    IFC(get_Items(&spItemsReference));
    IFC(spItemsReference.As(&spSuggestionsCollectionReference));

    if (spSuggestionsCollectionReference && maxHeight > 0.)
    {
        UINT count = 0;
        IFC(spSuggestionsCollectionReference->get_Size(&count));

        // the suggestion list is only open when the maxsuggestionlistheight is greater than zero
        // and the count of elements in the list is positive
        if (count > 0)
        {
            isOpen = TRUE;
        }
    }

    // We don't want to necessarily take focus in this case, since we probably already
    // have focus somewhere internal to the AutoSuggestBox, so we'll bypass the custom
    // setter for IsSuggestionListOpen that takes focus.
    IFC(AutoSuggestBoxGenerated::put_IsSuggestionListOpen(isOpen));


Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------------
// Positions the suggestion list based on the value specified in the TextBoxPosition
//------------------------------------------------------------------------------
_Check_return_ HRESULT
    AutoSuggestBox::UpdateSuggestionListPosition()
{
    // Don't call this function while processing a collection change.  When a collection change happens,
    // ASB's OnItemsChanged gets called before the inner ListView's OnItemsChanged.  So if ASB calls UpdateLayout
    // on the ListView during its OnItemsChanged, ListView will try to measure itself when it has a stale
    // view of the collection.  Don't let this happen.
#if DBG
    ASSERT(!m_handlingCollectionChange);
#endif

    if (!m_isSipVisible)
    {
        IFC_RETURN(MaximizeSuggestionAreaWithoutInputPane());
    }

    if (m_tpPopupPart && m_tpTextBoxPart && m_tpSuggestionsContainerPart)
    {
        ctl::ComPtr<xaml::IUIElement> spThisAsUI = this;
        ctl::ComPtr<xaml::IDependencyObject> spTextBoxScrollViewerAsDO;

        DOUBLE width = 0.;
        DOUBLE height = 0.;
        DOUBLE translateX = 0.;
        DOUBLE translateY = 0.;
        DOUBLE scaleY = 1.;

        double candidateWindowXOffset = 0.0;
        double candidateWindowYOffset = 0.0;
        xaml::Thickness margin;

        xaml::Thickness suggestionListMargin = {};
        if (m_tpSuggestionsPart != nullptr)
        {
            ctl::ComPtr<xaml::IFrameworkElement> spSuggestionAsFE;
            IFC_RETURN(m_tpSuggestionsPart.As(&spSuggestionAsFE));
            IFC_RETURN(spSuggestionAsFE->get_Margin(&suggestionListMargin));
        }

        ctl::ComPtr<TextBox> spTextBoxPeer;
        IFC_RETURN(m_tpTextBoxPart.As(&spTextBoxPeer));

        // scroll viewer location
        // getting the ScrollViewer (child of the textbox)
        // we want to align the popup to the ScrollViewer part of the textbox
        // after getting the ScrollViewer, we find its position relative to the AutoSuggestBox
        // if the ScrollViewer is not present, we align to the textbox itself
        IFC_RETURN(m_tpTextBoxPart.AsOrNull<IControlProtected>()->GetTemplateChild(
            wrl_wrappers::HStringReference(c_TextBoxScrollViewerName).Get(), &spTextBoxScrollViewerAsDO));

        if (!spTextBoxScrollViewerAsDO)
        {
            ctl::ComPtr<xaml_media::IGeneralTransform> spTransform;
            wf::Point textBoxLocation = { 0,0 };

            IFC_RETURN(m_tpTextBoxPart.AsOrNull<IUIElement>()->TransformToVisual(spThisAsUI.Get(), &spTransform));
            IFC_RETURN(spTransform->TransformPoint(textBoxLocation, &textBoxLocation));
            translateY = textBoxLocation.Y;
        }
        else
        {
            ctl::ComPtr<xaml::IUIElement> spTextBoxScrollViewerAsUI;
            ctl::ComPtr<xaml_media::IGeneralTransform> spTransform;
            wf::Point scrollViewerLocation = {0,0};

            IFC_RETURN(spTextBoxScrollViewerAsDO.As(&spTextBoxScrollViewerAsUI));
            IFC_RETURN(spTextBoxScrollViewerAsUI->TransformToVisual(spThisAsUI.Get(), &spTransform));
            IFC_RETURN(spTransform->TransformPoint(scrollViewerLocation, &scrollViewerLocation));
            translateY = scrollViewerLocation.Y;
        }

        // We need move the popup up (popup's bottom align to textbox) when textbox is at bottom position.
        if (m_suggestionListPosition == SuggestionListPosition::Above)
        {
            IFC_RETURN(m_tpSuggestionsContainerPart.AsOrNull<IUIElement>()->UpdateLayout());
            IFC_RETURN(m_tpSuggestionsContainerPart.AsOrNull<IFrameworkElement>()->get_ActualHeight(&height));

            translateY -= height;

            if (IsSuggestionListVerticallyMirrored())
            {
                scaleY = -1.;
            }
        }
        else if (m_suggestionListPosition == SuggestionListPosition::Below)
        {
             // If the text box has an active handwritingView or if the ScrollViewer isn't present, get the height of the
            // textbox/handwritingVirew itself. Otherweise add the ScrollViewer's height.
            if (!spTextBoxScrollViewerAsDO)
            {
                IFC_RETURN(GetActualTextBoxSize(width, height));
                translateY += height;
                // bring up the suggestion list to avoid gap caused by margin
                translateY -= suggestionListMargin.Top;
            }
            else
            {
                ctl::ComPtr<xaml::IFrameworkElement> spTextBoxScrollViewerAsFE;
                // ScrollViewer height
                IFC_RETURN(spTextBoxScrollViewerAsDO.As(&spTextBoxScrollViewerAsFE));
                IFC_RETURN(spTextBoxScrollViewerAsFE->get_ActualHeight(&height));

                translateY += height;
            }
        }

        IFC_RETURN(GetCandidateWindowPopupAdjustment(
            false /* ignoreSuggestionListPosition */,
            &candidateWindowXOffset,
            &candidateWindowYOffset));

        if (m_tpUpwardTransformPart)
        {
            IFC_RETURN(m_tpUpwardTransformPart->put_X(translateX + candidateWindowXOffset));
            IFC_RETURN(m_tpUpwardTransformPart->put_Y(translateY + candidateWindowYOffset));
        }
        else
        {
            IFC_RETURN(m_tpPopupPart->put_HorizontalOffset(translateX + candidateWindowXOffset));
            IFC_RETURN(m_tpPopupPart->put_VerticalOffset(translateY + candidateWindowYOffset));
        }

        // If we've moved the suggestions list popup over in the x-direction, we still want the
        // right side of the popup to be in the same place, so we add the offset to its right margin as well.
        IFC_RETURN(m_tpSuggestionsContainerPart->get_Margin(&margin));
        margin.Right = candidateWindowXOffset;
        IFC_RETURN(m_tpSuggestionsContainerPart->put_Margin(margin));

        if (IsSuggestionListVerticallyMirrored())
        {
            ctl::ComPtr<xaml_media::IScaleTransform> scaleTransform(m_tpListItemOrderTransformPart.AsOrNull<xaml_media::IScaleTransform>().Get());

            if (scaleTransform)
            {
                IFC_RETURN(scaleTransform->put_ScaleY(scaleY));
            }
        }
        else
        {
            IFC_RETURN(UpdateSuggestionListItemsSource());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT AutoSuggestBox::ScrollLastItemIntoView()
{
    ctl::ComPtr<IListViewBase> listViewBase;
    if (SUCCEEDED(m_tpSuggestionsPart.As(&listViewBase)))
    {
        unsigned int size = 0;
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> items;
        ctl::ComPtr<wfc::IVector<IInspectable*>> vector;

        IFC_RETURN(listViewBase.AsOrNull<IItemsControl>()->get_Items(&items));
        IFC_RETURN(items.As(&vector));
        IFC_RETURN(vector->get_Size(&size));

        if (size > 1)
        {
            ctl::ComPtr<IInspectable> lastItem;
            IFC_RETURN(vector->GetAt(size - 1, &lastItem));

            IFC_RETURN(listViewBase->ScrollIntoView(lastItem.Get()));
        }
    }
    return S_OK;
}


// Called when ItemsSource is set, or when the ItemsSource collection chanages
IFACEMETHODIMP AutoSuggestBox::OnItemsChanged(_In_ IInspectable* e)
{
    HRESULT hr = S_OK;
    BOOLEAN isTextBoxFocused = FALSE;
    BOOLEAN bAutomationListener = FALSE;

#if DBG
    const bool wasHandlingCollectionChange = m_handlingCollectionChange;
    m_handlingCollectionChange = true;
#endif

    IFC(OnItemsChangedImpl(e));

    IFC(IsTextBoxFocusedElement(&isTextBoxFocused));
    if (isTextBoxFocused)
    {
        // Defer the update until after change notification is fully processed.
        // UpdateSuggestionListPosition must not be called synchronously while handling
        // the change notification (see comment in UpdateSuggestionListPosition).
        // This also has the benefit of doing just one update for a batch of change notifications
        // if we get a bunch at the same time.
        if (!m_deferringUpdate)
        {
            ctl::WeakRefPtr wpThis;
            IFC(ctl::AsWeak(this, &wpThis));

            IFC(DXamlCore::GetCurrent()->GetXamlDispatcherNoRef()->RunAsync(
                MakeCallback<ctl::WeakRefPtr,ctl::WeakRefPtr>(&AutoSuggestBox::ProcessDeferredUpdateStatic, wpThis)));
            m_deferringUpdate = true;
        }

        // We should immediately update visibility, however, since we want the value of
        // IsSuggestionListOpen to be properly updated by the time this returns.
        IFC(UpdateSuggestionListVisibility());
    }

    IFC(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_LayoutInvalidated, &bAutomationListener));

    if (bAutomationListener)
    {
        ctl::ComPtr<UIElement> spListPart;
        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;

        if (m_tpSuggestionsPart)
        {
            IFC(m_tpSuggestionsPart.As(&spListPart));
            if (spListPart)
            {
                IFC(spListPart->GetOrCreateAutomationPeer(&spAutomationPeer));
                if (spAutomationPeer)
                {
                    IFC(spAutomationPeer->RaiseAutomationEvent(xaml_automation_peers::AutomationEvents_LayoutInvalidated));
                }
            }
        }
    }

Cleanup:
#if DBG
    m_handlingCollectionChange = wasHandlingCollectionChange;
#endif
    RRETURN(hr);
};

_Check_return_ HRESULT
AutoSuggestBox::ProcessDeferredUpdateStatic(ctl::WeakRefPtr wpThis)
{
    auto localThis = wpThis.AsOrNull<AutoSuggestBox>();
    if (localThis)
    {
        return localThis->ProcessDeferredUpdate();
    }
    return S_OK;
}

_Check_return_ HRESULT
AutoSuggestBox::ProcessDeferredUpdate()
{
    if (m_deferringUpdate)
    {
        m_deferringUpdate = false;
        IFC_RETURN(UpdateSuggestionListPosition());
        IFC_RETURN(UpdateSuggestionListSize());
        IFC_RETURN(UpdateSuggestionListVisibility());
    }
    return S_OK;
}

_Check_return_ HRESULT
AutoSuggestBox::UpdateSuggestionListItemsSource()
{
    if (m_tpSuggestionsPart && !m_tpListItemOrderTransformPart)
    {
        // If we have a m_tpListItemOrderTransformPart, we implement SuggestionListPosition::Above
        // by applying a scale transform.  Also, in the win8.1 template where we do have a
        // m_tpListItemOrderTransformPart, the suggestion list's ItemsSource is bound to the
        // ASB's ItemsSource, so no need to update it.

        if (m_suggestionListPosition == SuggestionListPosition::Above)
        {
            ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spObservable;

            IFC_RETURN(get_Items(&spObservable));

            if (spObservable)
            {
                if (!m_spReversedVector || !m_spReversedVector->IsBoundTo(spObservable.Get()))
                {
                    m_spReversedVector  = wrl::Make<ReversedVector>();
                    m_spReversedVector->SetSource(spObservable.Get());

                    IFC_RETURN(m_tpSuggestionsPart.AsOrNull<IItemsControl>()->put_ItemsSource(
                        static_cast<wfc::IVector<IInspectable*>*>(m_spReversedVector.Get())));
                    IFC_RETURN(ScrollLastItemIntoView());
                }
                return S_OK;
            }
        }

        // We can't reverse the vector, fall back to propagating ItemsSource from ASB to the suggestion list
        m_spReversedVector = nullptr;

        ctl::ComPtr<IInspectable> spItemsSource;
        IFC_RETURN(get_ItemsSource(&spItemsSource));
        IFC_RETURN(m_tpSuggestionsPart.AsOrNull<IItemsControl>()->put_ItemsSource(spItemsSource.Get()));
    }
    return S_OK;
}

_Check_return_ HRESULT
    AutoSuggestBox::TryGetSuggestionValue(_In_ IInspectable* object, _In_opt_ PropertyPathListener* pathListener, _Out_ HSTRING* value)
{
    HRESULT hr = S_OK;

    if(object == nullptr)
    {
        RRETURN(hr);
    }

    ASSERT(value != nullptr);

    ctl::ComPtr<IInspectable> spBoxedValue;
    ctl::ComPtr<IInspectable> spObject(object);
    ctl::ComPtr<xaml_data::ICustomPropertyProvider> spObjectPropertyAccessor;
    ctl::ComPtr<wf::IStringable> spString;

    if( SUCCEEDED(spObject.As(&spObjectPropertyAccessor)) )
    {
        if (pathListener != nullptr)
        {
            // Our caller has provided us with a PropertyPathListener. By setting the source of the listener, we can pull a value out.
            // This is our boxedValue, which we effectively ToString below.
            IFC_RETURN(pathListener->SetSource(spObject.Get()));
            IFC_RETURN(pathListener->GetValue(&spBoxedValue));
        }
        else
        {
            // "value" property not specified, but this object implements
            // ICustomPropertyProvider. Call .ToString on the object:
            IFC(spObjectPropertyAccessor->GetStringRepresentation(value));
            goto Cleanup;
        }
    }
    else
    {
        spBoxedValue = spObject; // the object itself is the value string, unbox it.
    }

    // calling the ToString function on items that can be represented by a string
    if (spBoxedValue != nullptr && SUCCEEDED(spBoxedValue.As(&spString)))
    {
        IFC(spString->ToString(value));
    }
    else
    {
        IFC(FrameworkElement::GetStringFromObject(object, value));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
    AutoSuggestBox::SetCurrentControlledPeer(
    _In_ ControlledPeer peer)
{
    HRESULT hr = S_OK;

    if (m_tpTextBoxPart)
    {
        BOOLEAN bAutomationListener = FALSE;
        ctl::ComPtr<xaml::IUIElement> spPeer;
        ctl::ComPtr<xaml::IDependencyObject> spTextBoxPartAsDO;
        ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spControlledPeers;
        wrl::ComPtr<xaml_automation::IAutomationPropertiesStatics> spAutomationPropertiesStatic;

        IFC(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_AutomationProperties).Get(),
            &spAutomationPropertiesStatic));

        switch (peer)
        {
        case ControlledPeer::None:
            // Leave spPeer as nullptr.
            break;

        case ControlledPeer::SuggestionsList:
            if (m_tpSuggestionsPart)
            {
                IFC(m_tpSuggestionsPart.As(&spPeer));
            }
            break;

        default:
            break;
        }

        IFC(m_tpTextBoxPart.As(&spTextBoxPartAsDO));

        spAutomationPropertiesStatic->GetControlledPeers(spTextBoxPartAsDO.Get(), &spControlledPeers);

        IFC(spControlledPeers->Clear());
        if (spPeer)
        {
            IFC(spControlledPeers->Append(spPeer.Get()));
        }

        IFC(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_PropertyChanged, &bAutomationListener));

        if (bAutomationListener)
        {
            ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;

            IFC(m_tpTextBoxPart.AsOrNull<UIElement>()->GetOrCreateAutomationPeer(&spAutomationPeer));

            if (spAutomationPeer)
            {
                XHANDLE handle = spAutomationPeer.Cast<AutomationPeer>()->GetHandle();
                if (handle)
                {
                    IFC(CoreImports::AutomationRaiseAutomationPropertyChanged(static_cast<CAutomationPeer*>(handle), UIAXcp::APAutomationProperties::APControlledPeersProperty, CValue(), CValue()));
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT AutoSuggestBox::HookToRootScrollViewer()
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml::IFrameworkElement> spRSViewerAsFE;
    wrl::ComPtr<xaml::IDependencyObject> spCurrentAsDO = this;
    wrl::ComPtr<xaml::IDependencyObject> spParentAsDO;
    wrl::ComPtr<xaml_controls::IScrollViewer> spRootScrollViewer;

    spRSViewerAsFE.Reset();

    while (spCurrentAsDO)
    {
        IFC(VisualTreeHelper::GetParentStatic(spCurrentAsDO.Get(), &spParentAsDO));

        if (spParentAsDO)
        {
            wrl::ComPtr<xaml::IFrameworkElement> spParentAsFE;

            IFC(spParentAsDO.As(&spParentAsFE));
            spCurrentAsDO = spParentAsDO;

            // checking to see if the element is of type rootScrollViewer
            // using IFC will cause the application to throw an exception
            hr = spParentAsFE.As(&spRootScrollViewer);
            if (hr == S_OK)
            {
                spRSViewerAsFE = spParentAsFE;
            }
            else
                if (hr != E_NOINTERFACE)
                {
                goto Cleanup;
                }
        }
        else
        {
            break;
        }
    }

    if (spRSViewerAsFE)
    {
        IFC(spRSViewerAsFE.AsWeak(&m_wkRootScrollViewer));
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------------
// AutoSuggestBox ChangeVisualState
//
// Applies the necessary visual state
//------------------------------------------------------------------------------
_Check_return_ HRESULT
    AutoSuggestBox::ChangeVisualState()
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml::Controls::IControl> spThisAsControl = this;
    BOOLEAN succeeded = FALSE;

    if (m_displayOrientation == XamlDisplay::Orientation::Landscape ||
        m_displayOrientation == XamlDisplay::Orientation::LandscapeFlipped)
    {
        IFC(VisualStateManager::GoToState(
                spThisAsControl.Get(),
                wrl_wrappers::HStringReference(c_VisualStateLandscape).Get(),
                true,
                &succeeded));
    }
    else
    if (m_displayOrientation == XamlDisplay::Orientation::Portrait ||
        m_displayOrientation == XamlDisplay::Orientation::PortraitFlipped)
    {
        IFC(VisualStateManager::GoToState(spThisAsControl.Get(),
                wrl_wrappers::HStringReference(c_VisualStatePortrait).Get(),
                true,
                &succeeded));
    }
    checked_cast<CControl>(GetHandle())->EnsureValidationVisuals();
Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------------
// AutoSuggestBox AlignmentHelper
//
// Performs the alignment to the top or bottom depending on the location of the control
//------------------------------------------------------------------------------
_Check_return_ HRESULT
    AutoSuggestBox::AlignmentHelper(
        _In_ wf::Rect sipOverlay)
{
    HRESULT hr = S_OK;

    // query the ScrollViewer.BringIntoViewOnFocusChange property
    // if it's set to false, the control should not move
    ctl::ComPtr<xaml::IDependencyObject> spThisAsDO = this;
    BOOLEAN bringIntoViewOnFocusChange = TRUE;

    // In case when the app is not occluded by the SIP, we should just calculate the max suggestion area same way as if SIP is not deployed.
    // Otherwise sipOverlay.Y is 0 and it will erroneously throw the calculation off, it is same as if SIP is deployed at the top of the app.
    if (sipOverlay.Y == 0)
    {
        IFC(MaximizeSuggestionAreaWithoutInputPane());
        goto Cleanup;
    }

    if (!m_wkRootScrollViewer)
    {
        goto Cleanup;
    }

    IFC(ScrollViewerFactory::GetBringIntoViewOnFocusChangeStatic(spThisAsDO.Get(), &bringIntoViewOnFocusChange));

    if (bringIntoViewOnFocusChange)
    {
        if (m_scrollActions.empty())
        {
            wrl::ComPtr<xaml::IUIElement> spRootScrollViewerAsUIElement;

            IFC(m_wkRootScrollViewer.As(&spRootScrollViewerAsUIElement));
            if (spRootScrollViewerAsUIElement)
            {
                DOUBLE actualTextBoxHeight = 0.;
                wf::Point point = { 0, 0 };
                wf::Rect layoutBounds = {};
                IFC(GetAdjustedLayoutBounds(layoutBounds));

                // getting the position with respect to the root ScrollViewer
                IFC(TransformPoint(spRootScrollViewerAsUIElement.Get(), &point));

                double actualTextBoxWidth = 0.0;
                IFC(GetActualTextBoxSize(actualTextBoxWidth, actualTextBoxHeight));

                const DOUBLE bottomY = point.Y + actualTextBoxHeight;

                // updates the visual state of the ASB depending on the phone orientation
                IFC(ChangeVisualState());

                IFC(MaximizeSuggestionArea(point.Y, bottomY, sipOverlay.Y, layoutBounds));
            }
        }
        else
        {
            IFC(ApplyScrollActions(TRUE));
        }

        IFC(UpdateSuggestionListPosition());
        IFC(UpdateSuggestionListSize());
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------------
// AutoSuggestBox MaximizeSuggestionArea
//
// Maximizes the suggestion list area if the AutoMaximizeSuggestionArea is enabled.
//------------------------------------------------------------------------------
_Check_return_ HRESULT
    AutoSuggestBox::MaximizeSuggestionArea(
    _In_ DOUBLE topY,
    _In_ DOUBLE bottomY,
    _In_ DOUBLE sipOverlayY,
    _In_ wf::Rect layoutBounds)
{
    HRESULT hr = S_OK;
    DOUBLE deltaTop = 0.;
    DOUBLE deltaBottom = 0.;
    BOOLEAN autoMaximizeSuggestionArea = TRUE;
    double candidateWindowYOffset = 0.0;

    DBG_TRACE(L"DBASB[0x%p]: topY: %f, bottomY: %f, sipOverlayAreaY: %f, windowsBoundsHeight",
        this, topY, bottomY, sipOverlayY, windowsBoundsHeight);

    IFC(get_AutoMaximizeSuggestionArea(&autoMaximizeSuggestionArea));

    IFC(GetCandidateWindowPopupAdjustment(
        true /* ignoreSuggestionListPosition */,
        nullptr,
        &candidateWindowYOffset));

    // distance from top of asb (or candidate window, whichever is higher) to bottom of system chrome
    deltaTop = topY + (candidateWindowYOffset < 0 ? candidateWindowYOffset : 0) - layoutBounds.Y;
    // distance from bottom of asb (or candidate window, whichever is lower) to top of sip
    deltaBottom = sipOverlayY - (bottomY + (candidateWindowYOffset > 0 ? candidateWindowYOffset : 0));

    if (deltaBottom < 0)
    {
        DOUBLE actualTextBoxHeight = bottomY - topY;

        // Scrolls the textbox up above the SIP if it is covered by the SIP, put the suggestions
        // list on top of the ASB and maximizes its height based on the available space left.
        deltaBottom *= -1;
        IFC(Scroll(deltaBottom));
        m_suggestionListPosition = SuggestionListPosition::Above;

        // scroll function changes the deltaBottom to 0 if the ASB reached its location
        // otherwise it will contain the remaining distance to the desired destination
        // subtracting the positive deltabottom value (bottomY - sipOverlayY)
        // then subtracting deltaBottom which will either contain 0 or the remaining distance
        m_availableSuggestionHeight =  sipOverlayY - (deltaBottom + actualTextBoxHeight + layoutBounds.Y);
    }
    else if (autoMaximizeSuggestionArea &&
             (deltaTop < s_minSuggestionListHeight && deltaBottom < s_minSuggestionListHeight))
    {
        // Scrolls the textbox to the top of the page, this makes use of ScrollableArea
        // of the RootScrollViewer if needed. Put the suggestions list to the bottom of the
        // ASB and maximizes its height based on the available space, the height of the
        // suggestion list shouldn't go beyond the SIP area.

        DOUBLE actualTextBoxHeight = bottomY - topY;

        IFC(Scroll(deltaTop));

        // in case we cannot scroll all the way to the top due to ScrollViewer scrollableheight restrictions,
        // we maximize the suggestions list position and size
        // deltaTop will only be greater than zero in case we couldn't scroll all the way to the top
        if (deltaTop > 0.)
        {
            topY = deltaTop + layoutBounds.Y;
            bottomY = topY + actualTextBoxHeight;

            deltaBottom = sipOverlayY - bottomY;

            if (deltaTop < deltaBottom)
            {
                m_suggestionListPosition = SuggestionListPosition::Below;
                m_availableSuggestionHeight = abs(deltaBottom);
            }
            else
            {
                m_suggestionListPosition = SuggestionListPosition::Above;
                m_availableSuggestionHeight = deltaTop;
            }
        }
        else
        {
            m_suggestionListPosition = SuggestionListPosition::Below;
            m_availableSuggestionHeight = sipOverlayY - actualTextBoxHeight - layoutBounds.Y;
        }
    }
    else
    {
        if (deltaTop < deltaBottom)
        {
            m_suggestionListPosition = SuggestionListPosition::Below;
            m_availableSuggestionHeight = abs(deltaBottom);
        }
        else
        {
            m_suggestionListPosition = SuggestionListPosition::Above;
            m_availableSuggestionHeight = deltaTop;
        }
    }

Cleanup:
    // Set availabeHeight to zero if there are no space left on the screen, this can happen
    // for instance when the ASB has a great height and when the device is in landscape orientation
    if (m_availableSuggestionHeight < 0)
    {
        m_availableSuggestionHeight = 0;
    }
    RRETURN(hr);
}

_Check_return_ HRESULT
AutoSuggestBox::MaximizeSuggestionAreaWithoutInputPane()
{
    auto scopeGuard = wil::scope_exit([&]
    {
        // Set availableHeight to zero if there are no space left on the screen, this can happen
        // for instance when the ASB has a great height and when the device is in landscape orientation
        if (m_availableSuggestionHeight < 0)
        {
            m_availableSuggestionHeight = 0;
        }
    });

    double topY;
    double bottomY;
    double deltaTop = 0.;
    double deltaBottom = 0.;
    wf::Rect layoutBounds = {};
    BOOLEAN autoMaximizeSuggestionArea = TRUE;
    wrl::ComPtr<xaml::IUIElement> spRootScrollViewerAsUIElement;
    double candidateWindowYOffset = 0.0;
    double actualTextBoxWidth = 0.0;
    double actualTextBoxHeight = 0.0;
    wf::Point point = { 0, 0 };

    IFC_RETURN(m_wkRootScrollViewer.As(&spRootScrollViewerAsUIElement));
    if (spRootScrollViewerAsUIElement)
    {
        // getting the position with respect to the root ScrollViewer
        IFC_RETURN(TransformPoint(spRootScrollViewerAsUIElement.Get(), &point));
    }


    // Instead of determining the alignment of the autosuggest popup using the popup layout bounds, use the adjusted layout
    // bounds of the text box in the case where the autosuggest popup is nullptr. If a windowed popup is created later,
    // the UpdateSuggestionListPosition function will run again and correct the invalid previous alignment.
    if ((m_tpPopupPart != nullptr) && (m_tpPopupPart.Cast<Popup>()->IsWindowed()))
    {
        IFC_RETURN(DXamlCore::GetCurrent()->CalculateAvailableMonitorRect(m_tpPopupPart.Cast<Popup>(), point, &layoutBounds));
    }
    else
    {
        IFC_RETURN(GetAdjustedLayoutBounds(layoutBounds));
    }

    IFC_RETURN(GetActualTextBoxSize(actualTextBoxWidth, actualTextBoxHeight));

    topY = point.Y;
    bottomY = point.Y + actualTextBoxHeight;

    IFC_RETURN(get_AutoMaximizeSuggestionArea(&autoMaximizeSuggestionArea));

    IFC_RETURN(GetCandidateWindowPopupAdjustment(
        true /* ignoreSuggestionListPosition */,
        nullptr,
        &candidateWindowYOffset));

    // distance from top of asb (or candidate window, whichever is higher) to bottom of system chrome
    deltaTop = topY + (candidateWindowYOffset < 0 ? candidateWindowYOffset : 0) - layoutBounds.Y;

    // distance from bottom of asb (or candidate window, whichever is lower) to the bottom of the layout bounds
    deltaBottom = layoutBounds.Height - (bottomY + (candidateWindowYOffset > 0 ? candidateWindowYOffset : 0));

    if (deltaTop < deltaBottom)
    {
        m_suggestionListPosition = SuggestionListPosition::Below;
        m_availableSuggestionHeight = abs(deltaBottom);
    }
    else
    {
        m_suggestionListPosition = SuggestionListPosition::Above;
        m_availableSuggestionHeight = deltaTop;
    }

    return S_OK;
}

//------------------------------------------------------------------------------
// Walks up the visual tree and find all ScrollViewers, try to scroll them up or down to see
// if we can let the ASB hit the desired position (Top or Bottom)
//------------------------------------------------------------------------------
_Check_return_ HRESULT
    AutoSuggestBox::Scroll(_Inout_ DOUBLE& totalOffset)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spCurrentAsDO = this;
    ctl::ComPtr<xaml::IDependencyObject> spParentAsDO;

    DOUBLE previousYLocation = 0.;

    ASSERT(m_scrollActions.size() == 0);

    do
    {
        IFC(VisualTreeHelper::GetParentStatic(spCurrentAsDO.Get(), &spParentAsDO));

        if (spParentAsDO)
        {
            ctl::ComPtr<xaml_controls::IScrollViewer> spScrollViewer;

            // checking to see if the element is of type ScrollViewer
            // using IFC will cause the application to throw an exception
            hr = spParentAsDO.As(&spScrollViewer);
            if (hr == S_OK)
            {
                ctl::ComPtr<xaml::IUIElement> spScrollViewerAsUIE;
                wf::Point asbLocation = {0, 0};
                DOUBLE partialOffset = 0.;

                IFC(spScrollViewer.As(&spScrollViewerAsUIE));
                IFC(TransformPoint(spScrollViewerAsUIE.Get(), &asbLocation));

                asbLocation.Y -= (float)previousYLocation;
                partialOffset = asbLocation.Y;

                // checking to see if the ASB's position within the ScrollViewer is less than the total offset
                // this means that the ASB will scroll out of the ScrollViewer's viewport
                // in this case, we scroll by the ASB's position and let the parent ScrollViewer handle the rest of the move
                if (asbLocation.Y < totalOffset)
                {
                    IFC(PushScrollAction(spScrollViewer.Get(), partialOffset));

                    totalOffset -= asbLocation.Y - partialOffset;
                }
                else
                {
                    IFC(PushScrollAction(spScrollViewer.Get(), totalOffset));
                }

                // if ASB cannot scroll by the partial offset value (ScrollViewer height restrictions)
                // the difference is added to previous location so that the parent ScrollViewer handles the rest of the move
                previousYLocation = asbLocation.Y - (float)partialOffset;
            }
            else
            if (hr != E_NOINTERFACE)
            {
                goto Cleanup;
            }

            spCurrentAsDO = spParentAsDO;
        }

    } while (spParentAsDO);

    IFC(ApplyScrollActions(TRUE /* hasNewScrollActions */));

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------------
// Push scroll actions for the given ScrollViewer to the internal ScrollAction vector.
//------------------------------------------------------------------------------
_Check_return_ HRESULT
    AutoSuggestBox::PushScrollAction(
    _In_ xaml_controls::IScrollViewer* pScrollViewer,
    _Inout_ DOUBLE& targetOffset)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_controls::IScrollViewer> spScrollViewer(pScrollViewer);
    DOUBLE verticalOffset = 0.;
    DOUBLE scrollableHeight = 0.;
    ScrollAction action;

    ASSERT(spScrollViewer);

    IFC(spScrollViewer->get_VerticalOffset(&verticalOffset));
    IFC(spScrollViewer->get_ScrollableHeight(&scrollableHeight));

    IFC(spScrollViewer.AsWeak(&action.wkScrollViewer));

    action.initial = verticalOffset;
    if (targetOffset + verticalOffset > scrollableHeight)
    {
        action.target = scrollableHeight;
        targetOffset -= scrollableHeight - verticalOffset;
    }
    else
    {
        action.target = targetOffset + verticalOffset;

        if (action.target < 0.)
        {
            action.target = 0.;
            targetOffset += verticalOffset;
        }
        else
        {
            targetOffset = 0.;
        }
    }

    m_scrollActions.push_back(action);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT AutoSuggestBox::ApplyScrollActions(_In_ BOOLEAN hasNewScrollActions)
{
    HRESULT hr = S_OK;

    if (!m_wkRootScrollViewer)
    {
        goto Cleanup;
    }

    for (auto iter = m_scrollActions.begin(); iter != m_scrollActions.end(); ++iter)
    {
        wrl::ComPtr<xaml_controls::IScrollViewer> spScrollViewer;

        IFC(iter->wkScrollViewer.As(&spScrollViewer));
        if (spScrollViewer)
        {
            DOUBLE offset = iter->target;

            if (!hasNewScrollActions)
            {
                offset = iter->initial;
            }

            if (iter->wkScrollViewer.Get() == m_wkRootScrollViewer.Get())
            {
                // potential bug on RootScrolViewer, there is a visual glitch on the RootScrollViewer
                // when ChangeViewWithOptionalAnimation is used
                IFC(spScrollViewer->ScrollToVerticalOffset(offset));
            }
            else
            {
                BOOLEAN returnValueIgnored;
                wrl::ComPtr<IInspectable> spVerticalOffsetAsInspectable;
                wrl::ComPtr<wf::IReference<DOUBLE>> spVerticalOffset;

                IFC(PropertyValue::CreateFromDouble(offset, &spVerticalOffsetAsInspectable));
                IFC(spVerticalOffsetAsInspectable.As(&spVerticalOffset));

                IFC(spScrollViewer->ChangeViewWithOptionalAnimation(
                        nullptr,                   // horizontalOffset
                        spVerticalOffset.Get(),    // verticalOffset
                        nullptr,                   // zoomFactor
                        FALSE,                     // disableAnimation
                        &returnValueIgnored));
            }
        }
    }

Cleanup:
    if (!hasNewScrollActions)
    {
        m_scrollActions.clear();
    }
    RRETURN(hr);
}

_Check_return_ HRESULT AutoSuggestBox::OnTextChangedEventTimerTick(
    _In_ IInspectable* /*pSender*/,
    _In_ IInspectable* /*pArgs*/)
{
    HRESULT hr = S_OK;

    IFC(m_tpTextChangedEventTimer->Stop());

    if (m_tpTextChangedEventArgs)
    {
        TextChangedEventSourceType* pEventSource = nullptr;
        IFC(GetTextChangedEventSourceNoRef(&pEventSource));
        IFC(pEventSource->Raise(this, m_tpTextChangedEventArgs.Get()));
    }

    // We expect apps to modify the ItemsSource in the TextChangedEvent (raised above).
    // If that happened. we'll have a deferred update at this point, let's just process
    // it now so we don't have to wait for the next tick.
    IFC(ProcessDeferredUpdate());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT AutoSuggestBox::OnSuggestionSelectionChanged(
    _In_ IInspectable* /*pSender*/,
    _In_ xaml_controls::ISelectionChangedEventArgs* /*pArgs*/)
{
    ctl::ComPtr<IInspectable> spSelectedItem;
    ctl::ComPtr<IListViewBase> suggestionsPartAsLVB;
    TraceLoggingActivity<g_hTraceProvider, MICROSOFT_KEYWORD_TELEMETRY> traceLoggingActivity;

    IFC_RETURN(m_tpSuggestionsPart->get_SelectedItem(&spSelectedItem));

    // ASB handles keyboard navigation on behalf of the suggestion box.
    // Consequently, the latter won't scroll its viewport to follow the selected item.
    // We have to do that ourselves explicitly.
    {
        ctl::ComPtr<IInspectable> scrollToItem = spSelectedItem.Get();

        // We fallback on the first item in order to bring the viewport to the beginning.
        if (!scrollToItem)
        {
            unsigned itemsCount;
            ctl::ComPtr<wfc::IObservableVector<IInspectable*>> items;
            IFC_RETURN(get_Items(&items));
            IFC_RETURN(items.AsOrNull<wfc::IVector<IInspectable*>>()->get_Size(&itemsCount));

            if (itemsCount > 0)
            {
                IFC_RETURN(items.AsOrNull<wfc::IVector<IInspectable*>>()->GetAt(0, &scrollToItem));
            }
        }

        if (scrollToItem)
        {
            if (SUCCEEDED(m_tpSuggestionsPart.As(&suggestionsPartAsLVB)))
            {
                IFC_RETURN(suggestionsPartAsLVB->ScrollIntoView(scrollToItem.Get()));
            }
        }
    }

    if (m_ignoreSelectionChanges)
    {
        // Ignore the selection change if the change is trigged by the TextBoxText changed event.
        return S_OK;
    }

    // Telemetry marker for suggestion selection changed.
    TraceLoggingWriteStart(traceLoggingActivity,
        "ASBSuggestionSelectionChanged");

    // The only time we'll get here is when we're keyboarding through the suggestion list.
    // In this case, we're going to be updating the TextBox's text
    // as the user does that, so we should not be responding to TextChanged
    // events in the TextBox, as they shouldn't be affecting anything.
    // However, TextChanged is an asynchronous event, so we can't just
    // set a boolean value to true at the start of this method
    // and then set it back to false at the end of this method,
    // because the TextChanged will come in after the end of this method.
    // To get around this fact, we'll leverage the fact that, by the time
    // this method returns, the TextChanged events will be added to the
    // event queue.  We'll post a callback to change m_ignoreTextChanges
    // back to false once all of the TextChanged events have been raised.
    m_ignoreTextChanges = true;

    if (spSelectedItem)
    {
        BOOLEAN updateTextOnSelect = FALSE;
        ctl::ComPtr<AutoSuggestBoxSuggestionChosenEventArgs> spEventArgs;

        IFC_RETURN(get_UpdateTextOnSelect(&updateTextOnSelect));
        if (updateTextOnSelect)
        {
            wrl_wrappers::HString strTextMemberPath;
            IFC_RETURN(get_TextMemberPath(strTextMemberPath.GetAddressOf()));
            if (!m_spPropertyPathListener && !strTextMemberPath.IsEmpty())
            {
                auto pPropertyPathParser = std::unique_ptr<PropertyPathParser>(new PropertyPathParser());
                IFC_RETURN(pPropertyPathParser->SetSource(WindowsGetStringRawBuffer(strTextMemberPath.Get(), nullptr), nullptr /* context */));
                IFC_RETURN(ctl::make<PropertyPathListener>(nullptr /* pOwner */, pPropertyPathParser.get(), false /* fListenToChanges*/, false /* fUseWeakReferenceForSource */, &m_spPropertyPathListener));
            }

            wrl_wrappers::HString strSelectedItem;
            IFC_RETURN(TryGetSuggestionValue(spSelectedItem.Get(), m_spPropertyPathListener.Get(), strSelectedItem.GetAddressOf()));
            IFC_RETURN(UpdateTextBoxText(strSelectedItem.Get(), xaml_controls::AutoSuggestionBoxTextChangeReason_SuggestionChosen));
        }

        // If the item was selected using Gamepad or Remote, move the focus to the selected item.
        if (m_inputDeviceTypeUsed == DirectUI::InputDeviceType::GamepadOrRemote)
        {
            ctl::ComPtr<xaml::IDependencyObject> selectedItemDO;
            IFC_RETURN(suggestionsPartAsLVB.Cast<ListViewBase>()->ContainerFromItem(spSelectedItem.Get(), &selectedItemDO));
            BOOLEAN succeeded = false;
            IFC_RETURN(selectedItemDO.AsOrNull<IUIElement>()->Focus(xaml::FocusState_Keyboard, &succeeded));
        }

        IFC_RETURN(ctl::make(&spEventArgs));
        IFC_RETURN(spEventArgs->put_SelectedItem(spSelectedItem.Get()));
        SuggestionChosenEventSourceType* pEventSource = nullptr;
        IFC_RETURN(GetSuggestionChosenEventSourceNoRef(&pEventSource));

        IFC_RETURN(pEventSource->Raise(this, spEventArgs.Get()));
    }

    // At this point everything that's going to post a TextChanged event
    // to the event queue has done so, so we'll schedule a callback
    // to reset the value of m_ignoreTextChanges to false once they've
    // all been raised.
    IFC_RETURN(DXamlCore::GetCurrent()->GetXamlDispatcherNoRef()->RunAsync(
        MakeCallback(
            this,
            &AutoSuggestBox::ResetIgnoreTextChanges)
        ));

    return S_OK;
}

_Check_return_ HRESULT AutoSuggestBox::OnListViewItemClick(
    _In_ IInspectable* /*pSender*/,
    _In_ xaml_controls::IItemClickEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spClickedItem;

    IFC(pArgs->get_ClickedItem(&spClickedItem));

    // When an suggestion is clicked, we want to raise QuerySubmitted using that
    // as the chosen suggestion.  However, clicking on an item may additionally raise
    // SelectionChanged, which will set the value of the TextBox and raise SuggestionChosen,
    // both of which we want to occur before we raise QuerySubmitted.
    // To account for this, we'll register a callback that will cause us to call SubmitQuery
    // after everything else has happened.
    IFC(DXamlCore::GetCurrent()->GetXamlDispatcherNoRef()->RunAsync(
        MakeCallback(
            this,
            &AutoSuggestBox::SubmitQuery,
            spClickedItem.Get())
        ));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT AutoSuggestBox::OnListViewContainerContentChanging(
    _In_ xaml_controls::IListViewBase*,
    _In_ xaml_controls::IContainerContentChangingEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    if (m_tpListItemOrderTransformPart)
    {
        ctl::ComPtr<xaml_primitives::ISelectorItem> spContainer;

        IFC(pArgs->get_ItemContainer(&spContainer));
        if (spContainer)
        {
            ctl::ComPtr<xaml::IUIElement> spContainerAsUI;

            IFC(spContainer.As(&spContainerAsUI));
            if (spContainerAsUI)
            {
                wf::Point origin = {0.5, 0.5};

                IFC(spContainerAsUI->put_RenderTransformOrigin(origin));
                IFC(spContainerAsUI->put_RenderTransform(m_tpListItemOrderTransformPart.Get()));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------------
// Transforms coordinates to the target element's space.
//------------------------------------------------------------------------------
_Check_return_ HRESULT
    AutoSuggestBox::TransformPoint(
        _In_ xaml::IUIElement* pTargetElement,
        _Inout_ wf::Point *pPoint)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IUIElement> spThisAsUIElement = this;

    ctl::ComPtr<xaml_media::IGeneralTransform> spGeneralTransform;
    wf::Point inPoint = *pPoint;

    ASSERT(pTargetElement);

    IFC(spThisAsUIElement->TransformToVisual(pTargetElement, &spGeneralTransform));
    IFC(spGeneralTransform->TransformPoint(inPoint, pPoint));

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------------
// Internal helper function to handle the Hiding event from InputPane.
//------------------------------------------------------------------------------
_Check_return_ HRESULT
    AutoSuggestBox::OnSipHidingInternal()
{
    HRESULT hr = S_OK;

    m_isSipVisible = false;

    // scroll all ScrollViewers back if we changed.
    IFC(ApplyScrollActions(FALSE /* hasNewScrollActions */));

    // potential bug in RootScrollViewer: when SIP is hiding, the viewport of RootScrollViewer will be restored to
    // the screen size and the content of RootScrollViewer will not able to scroll, in this case, the vertical offset
    // should be reset to 0, however it doesn't.
    // wrong vertical offset will cause suggestionlist in wrong positon next time when SIP is showing.
    if (m_wkRootScrollViewer)
    {
        wrl::ComPtr<xaml_controls::IScrollViewer> spRootScrollViewer;

        IFC(m_wkRootScrollViewer.As(&spRootScrollViewer));

        if (spRootScrollViewer)
        {
            DOUBLE verticalOffset = 0.;
            IFC(spRootScrollViewer->get_VerticalOffset(&verticalOffset));
            if (verticalOffset != 0.)
            {
                IFC(spRootScrollViewer->ScrollToVerticalOffset(0.));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------------
// Internal helper function to handle the Showing event from InputPane.
// The InputPane.Showing event could be called multiple times and the order
// of the internal and public Sip events are not guaranted.
//------------------------------------------------------------------------------
_Check_return_ HRESULT
    AutoSuggestBox::OnSipShowingInternal(
    _In_ wuv::IInputPaneVisibilityEventArgs* pArgs)
{
    if (m_tpTextBoxPart)
    {
        wrl::ComPtr<wuv::IInputPaneVisibilityEventArgs> spSipArgs(pArgs);
        wf::Rect sipOverlayArea = {};
        static BOOLEAN s_fDeferredShowing = FALSE;
        BOOLEAN isTextBoxFocused = FALSE;

        // Hold a reference to the eventarg
        SetPtrValue(m_tpSipArgs, spSipArgs.Get());

        IFC_RETURN(pArgs->get_OccludedRect(&sipOverlayArea));

        IFC_RETURN(IsTextBoxFocusedElement(&isTextBoxFocused));
        if (isTextBoxFocused)
        {
            m_hasFocus = true;

            auto currentOrientation = XamlDisplay::Orientation::None;
            IFC_RETURN(XamlDisplay::GetDisplayOrientation(GetHandle(), currentOrientation));
            if (currentOrientation != m_displayOrientation)
            {
                m_displayOrientation = currentOrientation;
                if (m_scrollActions.size() != 0)
                {
                    IFC_RETURN(OnSipHidingInternal());
                }
            }
        }

        if (!m_isSipVisible && m_hasFocus && m_wkRootScrollViewer)
        {
            wrl::ComPtr<xaml_controls::IScrollViewer> spRootScrollViewer;

            IFC_RETURN(m_wkRootScrollViewer.As(&spRootScrollViewer));

            if (spRootScrollViewer)
            {
                DOUBLE scrollableHeight = 0.;

                IFC_RETURN(spRootScrollViewer->get_ScrollableHeight(&scrollableHeight));
                if (scrollableHeight == 0. && !s_fDeferredShowing)
                {
                    // Wait for next OnSIPShowing event as the RootScrollViewer is not adjusted yet.
                    DBG_TRACE(L"DBASB[0x%p]: RootScrollViewer not yet adjusted", this);

                    // There is no guarantee that the Jupiter (InputPane::Showing) will gets call
                    // first, the native side invokes the Windows.UI.ViewManagement.InputPane.Showing/Hiding
                    // separately from the Jupiter internal events. RootScrollViewer will get
                    // notified about the InputPane state (through the callback NotifyInputPaneStateChange)
                    // when Jupiter gets the Showing event. Defer the SipEvent if the RootScrollViewer
                    // has not yet been notified about the SIP state change.

                    wrl::ComPtr<msy::IDispatcherQueueStatics> spDispatcherQueueStatics;
                    wrl::ComPtr<msy::IDispatcherQueue> spDispatcherQueue;
                    boolean enqueued;
                    wrl::ComPtr<IAutoSuggestBox> spThis = this;
                    wrl::WeakRef wrThis;

                    IFC_RETURN(spThis.AsWeak(&wrThis));

                    IFC_RETURN(wf::GetActivationFactory(
                        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Dispatching_DispatcherQueue).Get(),
                        &spDispatcherQueueStatics));

                    IFC_RETURN(spDispatcherQueueStatics->GetForCurrentThread(&spDispatcherQueue));

                    IFC_RETURN(spDispatcherQueue->TryEnqueue(
                            WRLHelper::MakeAgileCallback<msy::IDispatcherQueueHandler>([wrThis] () mutable {
                                wrl::ComPtr<IAutoSuggestBox> spThis;
                                IFC_RETURN(wrThis.As(&spThis));

                                AutoSuggestBox* asb = static_cast<AutoSuggestBox*>(spThis.Get());

                                ASSERT(spThis);
                                ASSERT(asb);

                                IFC_RETURN(asb->OnSipShowingInternal(asb->m_tpSipArgs.Get()));

                                // clearing the placeholder sip arguments stored in the asb after being used
                                asb->m_tpSipArgs.Clear();

                                return S_OK;
                            }).Get(),
                            &enqueued));

                    IFCEXPECT_RETURN(enqueued);

                    s_fDeferredShowing = TRUE;

                    return S_OK;
                }

                wrl_wrappers::HString strText;

                m_isSipVisible = true;
                s_fDeferredShowing = FALSE;

                IFC_RETURN(AlignmentHelper(sipOverlayArea));

                // Expands the suggestion list if there is already text in the textbox
                IFC_RETURN(m_tpTextBoxPart->get_Text(strText.GetAddressOf()));
                if (!strText.IsEmpty())
                {
                    IFC_RETURN(UpdateSuggestionListVisibility());
                }
            }
        }
    }

    IFC_RETURN(ReevaluateIsOverlayVisible());

    return S_OK;
}

_Check_return_ HRESULT AutoSuggestBox::UpdateText(_In_ HSTRING value)
{
    wrl_wrappers::HString strText;

    IFC_RETURN(get_Text(strText.GetAddressOf()));
    if (value != strText)
    {
        IFC_RETURN(put_Text(value));
    }

    IFC_RETURN(InvokeValidationCommand(this, strText.Get()));

    return S_OK;
}

_Check_return_ HRESULT
AutoSuggestBox::UpdateTextBoxText(
    _In_ HSTRING value,
    _In_ xaml_controls::AutoSuggestionBoxTextChangeReason reason)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strText;

    if (m_tpTextBoxPart)
    {
        IFC(m_tpTextBoxPart->get_Text(strText.GetAddressOf()));
        if (value != strText)
        {
            // when TextBox text is changing, we need to let user know the reason so user can
            // respond correctly.
            // however the TextChanged event is raised asynchronously so we need to reset the
            // reason in the TextChangedEvent.
            // here we are sure the TextChangedEvent will be raised because the old content
            // and new content are different.
            m_textChangeReason = reason;
            IFC(m_tpTextBoxPart->put_Text(value));
            IFC(m_tpTextBoxPart->put_SelectionStart(::WindowsGetStringLen(value)));
        }
    }

Cleanup:
    RRETURN(hr);
}

#pragma region IUIElementOverrides methods

IFACEMETHODIMP AutoSuggestBox::OnCreateAutomationPeer(
    _Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IAutoSuggestBoxAutomationPeer> spAutoSuggestBoxAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IAutoSuggestBoxAutomationPeerFactory> spAutoSuggestBoxAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;

    IFCPTR(ppAutomationPeer);
    *ppAutomationPeer = NULL;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::AutoSuggestBoxAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spAutoSuggestBoxAPFactory));

    IFC(spAutoSuggestBoxAPFactory.Cast<AutoSuggestBoxAutomationPeerFactory>()->CreateInstanceWithOwner(
            this,
            &spAutoSuggestBoxAutomationPeer));

    IFC(spAutoSuggestBoxAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

#pragma endregion

_Check_return_ HRESULT
AutoSuggestBox::GetPlainText(
    _Out_ HSTRING* strPlainText)
{
    ctl::ComPtr<IInspectable> spHeader;
    *strPlainText = nullptr;

    IFC_RETURN(get_Header(&spHeader));

    if (spHeader != nullptr)
    {
        IFC_RETURN(FrameworkElement::GetStringFromObject(spHeader.Get(), strPlainText));
    }

    return S_OK;
}

_Check_return_ HRESULT
AutoSuggestBox::GetCandidateWindowPopupAdjustment(
    _In_ bool ignoreSuggestionListPosition,
    _Out_opt_ double *pXOffset,
    _Out_opt_ double *pYOffset)
{
    HRESULT hr = S_OK;
    double xOffset = 0.0;
    double yOffset = 0.0;
    double textBoxWidth = 0.0;
    double textBoxHeight = 0.0;
    bool shouldOffsetInXDirection = false;

    IFC(GetActualTextBoxSize(textBoxWidth, textBoxHeight));

    // There are two cases in which we need to return a nonzero offset:
    // either in the case where the candidate window is appearing above the top
    // of the TextBox, or the case where the candidate window stretches below
    // the bottom of the TextBox.  In either case, we only care if the candidate window
    // height and width are not both zero, since if they are then the candidate window
    // isn't actually being shown.
    //
    // Once we determine that we do need to return a nonzero offset, the next question
    // is whether the suggestion list is being displayed in the position in question
    // (i.e., either above or below the AutoSuggestBox).  If it's not, then we'll return
    // offsets that are all zero, since we don't need to do anything,
    // unless we were instructed to ignore the suggestion list position
    // (used when we're setting the suggestion list position).
    //
    // Finally, if we do need to offset, we then see whether to offset in the x-direction or the y-direction.
    // The general heuristic used is that if the candidate window spans more than half
    // of the width of the TextBox, then we'll offset in the y-direction, since
    // otherwise the popup will be squished into an unacceptably small width.
    // Otherwise, we'll offset in the x-direction and apply a margin
    // that will cause the popup to appear side-by-side with the candidate window.
    shouldOffsetInXDirection = (m_candidateWindowBoundsRect.X + m_candidateWindowBoundsRect.Width) < (textBoxWidth / 2);

    if (m_candidateWindowBoundsRect.Y < 0 && m_candidateWindowBoundsRect.Height > 0 &&
        (m_suggestionListPosition == SuggestionListPosition::Above || ignoreSuggestionListPosition))
    {
        if (shouldOffsetInXDirection)
        {
            xOffset = m_candidateWindowBoundsRect.X + m_candidateWindowBoundsRect.Width;
        }
        else
        {
            yOffset = m_candidateWindowBoundsRect.Y;
        }
    }
    else if (m_candidateWindowBoundsRect.Y + m_candidateWindowBoundsRect.Height > textBoxHeight &&
        (m_suggestionListPosition == SuggestionListPosition::Below || ignoreSuggestionListPosition))
    {
        if (shouldOffsetInXDirection)
        {
            xOffset = m_candidateWindowBoundsRect.X + m_candidateWindowBoundsRect.Width;
        }
        else
        {
            // m_candidateWindowBoundsRect.Y - textBoxHeight gets us the starting point of the
            // candidate window with respect to the lower bound of the TextBox's height,
            // and then from there we add on m_candidateWindowBoundsRect.Height in order to ensure
            // that the popup is flush with the bottom of the candidate window.
            yOffset = m_candidateWindowBoundsRect.Y - textBoxHeight + m_candidateWindowBoundsRect.Height;
        }
    }

    if (pXOffset != nullptr)
    {
        *pXOffset = xOffset;
    }

    if (pYOffset != nullptr)
    {
        *pYOffset = yOffset;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
AutoSuggestBox::ReevaluateIsOverlayVisible()
{
    if (!IsInLiveTree())
    {
        return S_OK;
    }

    bool isOverlayVisible = false;
    IFC_RETURN(LightDismissOverlayHelper::ResolveIsOverlayVisibleForControl(this, &isOverlayVisible));

    BOOLEAN isSuggestionListOpen = FALSE;
    IFC_RETURN(get_IsSuggestionListOpen(&isSuggestionListOpen));

    isOverlayVisible &= !!isSuggestionListOpen;  // Overlay should only be visible when the suggestion list is.
    isOverlayVisible &= !m_sSipIsOpen;           // Except if the SIP is also visible.

    if (isOverlayVisible != m_isOverlayVisible)
    {
        m_isOverlayVisible = isOverlayVisible;

        if (m_isOverlayVisible)
        {
            IFC_RETURN(SetupOverlayState());
        }
        else
        {
            IFC_RETURN(TeardownOverlayState());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
AutoSuggestBox::SetupOverlayState()
{
    ASSERT(m_isOverlayVisible);
    ASSERT(!m_layoutUpdatedEventHandler);

    if (m_tpLayoutRootPart)
    {
        // Create our overlay element if necessary.
        if (!m_overlayElement)
        {
            ctl::ComPtr<Rectangle> rectangle;
            IFC_RETURN(ctl::make(&rectangle));
            IFC_RETURN(rectangle->put_Width(1));
            IFC_RETURN(rectangle->put_Height(1));
            IFC_RETURN(rectangle->put_IsHitTestVisible(FALSE));

            // Create a theme resource for the overlay brush.
            {
                auto core = DXamlCore::GetCurrent()->GetHandle();
                auto dictionary = core->GetThemeResources();

                xstring_ptr themeBrush;
                IFC_RETURN(xstring_ptr::CloneBuffer(L"AutoSuggestBoxLightDismissOverlayBackground", &themeBrush));

                CDependencyObject* initialValueNoRef = nullptr;
                IFC_RETURN(dictionary->GetKeyNoRef(themeBrush, &initialValueNoRef));

                CREATEPARAMETERS cp(core);
                xref_ptr<CThemeResourceExtension> themeResourceExtension;
                IFC_RETURN(CThemeResourceExtension::Create(
                    reinterpret_cast<CDependencyObject **>(themeResourceExtension.ReleaseAndGetAddressOf()),
                    &cp));

                themeResourceExtension->m_strResourceKey = themeBrush;

                IFC_RETURN(themeResourceExtension->SetInitialValueAndTargetDictionary(initialValueNoRef, dictionary));

                IFC_RETURN(themeResourceExtension->SetThemeResourceBinding(
                    rectangle->GetHandle(),
                    DirectUI::MetadataAPI::GetPropertyByIndex(KnownPropertyIndex::Shape_Fill))
                    );
            }

            IFC_RETURN(SetPtrValueWithQI(m_overlayElement, rectangle.Get()));
        }

        // Add our overlay element to our layout root panel.
        ctl::ComPtr<wfc::IVector<xaml::UIElement*>> layoutRootChildren;
        IFC_RETURN(m_tpLayoutRootPart.Cast<Grid>()->get_Children(&layoutRootChildren));
        IFC_RETURN(layoutRootChildren->InsertAt(0, m_overlayElement.Cast<FrameworkElement>()));
    }

    IFC_RETURN(CreateLTEs());

    IFC_RETURN(m_layoutUpdatedEventHandler.AttachEventHandler(
        this,
        [this](_In_ IInspectable* /*sender*/, _In_ IInspectable* /*args*/)
        {
            if (m_isOverlayVisible)
            {
                IFC_RETURN(PositionLTEs());
            }
            return S_OK;
        }
        ));

    return S_OK;
}

_Check_return_ HRESULT
AutoSuggestBox::TeardownOverlayState()
{
    ASSERT(!m_isOverlayVisible);
    ASSERT(m_layoutUpdatedEventHandler);

    IFC_RETURN(DestroyLTEs());

    // Remove our light-dismiss element from our layout root panel.
    if (m_tpLayoutRootPart)
    {
        ctl::ComPtr<wfc::IVector<xaml::UIElement*>> layoutRootChildren;
        IFC_RETURN(m_tpLayoutRootPart.Cast<Grid>()->get_Children(&layoutRootChildren));

        UINT32 indexOfOverlayElement = 0;
        BOOLEAN wasFound = FALSE;
        IFC_RETURN(layoutRootChildren->IndexOf(m_overlayElement.Cast<FrameworkElement>(), &indexOfOverlayElement, &wasFound));
        ASSERT(wasFound);
        IFC_RETURN(layoutRootChildren->RemoveAt(indexOfOverlayElement));
    }

    IFC_RETURN(m_layoutUpdatedEventHandler.DetachEventHandler(ctl::iinspectable_cast(this)));

    return S_OK;
}

_Check_return_ HRESULT
AutoSuggestBox::CreateLTEs()
{
    ASSERT(!m_layoutTransition);
    ASSERT(!m_overlayLayoutTransition);
    ASSERT(!m_parentElementForLTEs);

    // If we're under the PopupRoot or FullWindowMediaRoot, then we'll explicitly set
    // our LTE's parent to make sure the LTE doesn't get placed under the TransitionRoot,
    // which is lower in z-order than these other roots.
    if (ShouldUseParentedLTE())
    {
        ctl::ComPtr<xaml::IDependencyObject> parent;
        IFC_RETURN(VisualTreeHelper::GetParentStatic(this, &parent));
        IFCEXPECT_RETURN(parent);

        IFC_RETURN(SetPtrValueWithQI(m_parentElementForLTEs, parent.Get()));
    }

    xref_ptr<CUIElement>    spNativeLTE;
    ctl::ComPtr<DependencyObject>   spNativeLTEAsDO;

    if (m_overlayElement)
    {
        // Create an LTE for our overlay element.
        IFC_RETURN(CoreImports::LayoutTransitionElement_Create(
            DXamlCore::GetCurrent()->GetHandle(),
            m_overlayElement.Cast<FrameworkElement>()->GetHandle(),
            m_parentElementForLTEs ? m_parentElementForLTEs.Cast<UIElement>()->GetHandle() : nullptr,
            false /*isAbsolutelyPositioned*/,
            spNativeLTE.ReleaseAndGetAddressOf()
            ));

        // Configure the overlay LTE with a rendertransform that we'll use to position/size it.
        {
            IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(spNativeLTE, KnownTypeIndex::UIElement, &spNativeLTEAsDO));
            IFC_RETURN(SetPtrValueWithQI(m_overlayLayoutTransition, spNativeLTEAsDO.Get()));

            ctl::ComPtr<CompositeTransform> compositeTransform;
            IFC_RETURN(ctl::make(&compositeTransform));

            IFC_RETURN(m_overlayLayoutTransition.Cast<UIElement>()->put_RenderTransform(compositeTransform.Get()));
        }
    }

    IFC_RETURN(CoreImports::LayoutTransitionElement_Create(
        DXamlCore::GetCurrent()->GetHandle(),
        GetHandle(),
        m_parentElementForLTEs ? m_parentElementForLTEs.Cast<UIElement>()->GetHandle() : nullptr,
        false /*isAbsolutelyPositioned*/,
        spNativeLTE.ReleaseAndGetAddressOf()
    ));
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(spNativeLTE, KnownTypeIndex::UIElement, &spNativeLTEAsDO));
    IFC_RETURN(SetPtrValueWithQI(m_layoutTransition, spNativeLTEAsDO.Get()));

    IFC_RETURN(PositionLTEs());

    return S_OK;
}

_Check_return_ HRESULT
AutoSuggestBox::PositionLTEs()
{
    ASSERT(m_layoutTransition);

    ctl::ComPtr<xaml::IDependencyObject> parentDO;
    ctl::ComPtr<xaml::IUIElement> parent;

    IFC_RETURN(VisualTreeHelper::GetParentStatic(this, &parentDO));

    // If we don't have a parent, then there's nothing for us to do.
    if (parentDO)
    {
        IFC_RETURN(parentDO.As(&parent));

        ctl::ComPtr<xaml_media::IGeneralTransform> transform;
        IFC_RETURN(TransformToVisual(parent.Cast<UIElement>(), &transform));

        wf::Point offset = {};
        IFC_RETURN(transform->TransformPoint({ 0, 0 }, &offset));

        IFC_RETURN(CoreImports::LayoutTransitionElement_SetDestinationOffset(m_layoutTransition.Cast<UIElement>()->GetHandle(), offset.X, offset.Y));
    }

    // Since AutoSuggestBox's suggestion list does not dismiss on window resize, we have to make sure
    // we update the overlay element's size.
    if (m_overlayLayoutTransition)
    {
        ctl::ComPtr<xaml_media::ITransform> transform;
        IFC_RETURN(m_overlayLayoutTransition.Cast<UIElement>()->get_RenderTransform(&transform));

        ctl::ComPtr<xaml_media::ICompositeTransform> compositeTransform;
        IFC_RETURN(transform.As(&compositeTransform));

        wf::Rect windowBounds = {};
        IFC_RETURN(DXamlCore::GetCurrent()->GetContentBoundsForElement(GetHandle(), &windowBounds));

        IFC_RETURN(compositeTransform->put_ScaleX(windowBounds.Width));
        IFC_RETURN(compositeTransform->put_ScaleY(windowBounds.Height));

        ctl::ComPtr<xaml_media::IGeneralTransform> transformToVisual;
        IFC_RETURN(TransformToVisual(nullptr, &transformToVisual));

        wf::Point offsetFromRoot = {};
        IFC_RETURN(transformToVisual->TransformPoint({ 0, 0 }, &offsetFromRoot));

        auto flowDirection = xaml::FlowDirection_LeftToRight;
        IFC_RETURN(get_FlowDirection(&flowDirection));

        // Translate the light-dismiss layer so that it is positioned at the top-left corner of the window (for LTR cases)
        // or the top-right corner of the window (for RTL cases).
        // TransformToVisual(nullptr) will return an offset relative to the top-left corner of the window regardless of
        // flow direction, so for RTL cases subtract the window width from the returned offset.x value to make it relative
        // to the right edge of the window.
        IFC_RETURN(compositeTransform->put_TranslateX(flowDirection == xaml::FlowDirection_LeftToRight ? -offsetFromRoot.X : offsetFromRoot.X - windowBounds.Width));
        IFC_RETURN(compositeTransform->put_TranslateY(-offsetFromRoot.Y));
    }

    return S_OK;
}

_Check_return_ HRESULT
AutoSuggestBox::DestroyLTEs()
{
    ASSERT(m_layoutTransition);

    IFC_RETURN(CoreImports::LayoutTransitionElement_Destroy(
        DXamlCore::GetCurrent()->GetHandle(),
        GetHandle(),
        m_parentElementForLTEs ? m_parentElementForLTEs.Cast<UIElement>()->GetHandle() : nullptr,
        m_layoutTransition.Cast<UIElement>()->GetHandle()
        ));

    m_layoutTransition.Clear();

    if (m_overlayLayoutTransition)
    {
        // Destroy our light-dismiss element's LTE.
        IFC_RETURN(CoreImports::LayoutTransitionElement_Destroy(
            DXamlCore::GetCurrent()->GetHandle(),
            m_overlayElement.Cast<FrameworkElement>()->GetHandle(),
            m_parentElementForLTEs ? m_parentElementForLTEs.Cast<UIElement>()->GetHandle() : nullptr,
            m_overlayLayoutTransition.Cast<UIElement>()->GetHandle()
            ));

        m_overlayLayoutTransition.Clear();
    }

    m_parentElementForLTEs.Clear();

    return S_OK;
}

bool
AutoSuggestBox::ShouldUseParentedLTE()
{
    ctl::ComPtr<xaml::IDependencyObject> rootDO;
    if (SUCCEEDED(VisualTreeHelper::GetRootStatic(this, &rootDO)) && rootDO)
    {
        ctl::ComPtr<PopupRoot> popupRoot;
        ctl::ComPtr<FullWindowMediaRoot> mediaRoot;

        if (SUCCEEDED(rootDO.As(&popupRoot)) && popupRoot)
        {
            return true;
        }
        else if (SUCCEEDED(rootDO.As(&mediaRoot)) && mediaRoot)
        {
            return true;
        }
    }

    return false;
}

_Check_return_ HRESULT AutoSuggestBox::GetAdjustedLayoutBounds (_Out_ wf::Rect &layoutBounds) const
{
    IFC_RETURN(DXamlCore::GetCurrent()->GetContentLayoutBoundsForElement(GetHandle(), &layoutBounds));

    // TODO: 12949603 -- re-enable this in XamlOneCoreTransforms mode using OneCore-friendly APIs
    // It's disabled today because ClientToScreen deals in screen coordinates, which isn't allowed in strict mode.
    // AutoSuggestBox effectively acts as though the client window is always at the very top of the screen.
    if (!XamlOneCoreTransforms::IsEnabled())
    {
        wf::Point point = { 0, 0 };
        DXamlCore::GetCurrent()->ClientToScreen(&point);
        layoutBounds.Y -= point.Y;
    }

    return S_OK;
}

_Check_return_ HRESULT AutoSuggestBox::GetActualTextBoxSize(_Out_ double& actualWidth, _Out_ double& actualHeight) const
{
    if (m_tpTextBoxPart)
    {
        ctl::ComPtr<TextBox> spTextBoxPeer;
        IFC_RETURN(m_tpTextBoxPart.As(&spTextBoxPeer));
        float fWidth, fHeight;
        static_cast<CTextBoxBase*>(spTextBoxPeer->GetHandle())->GetActualSize(fWidth, fHeight);
        actualWidth = static_cast<double>(fWidth);
        actualHeight = static_cast<double>(fHeight);
    }
    else
    {
        actualWidth = 0;
        actualHeight = 0;
    }

    return S_OK;
}

/*static*/
_Check_return_ HRESULT AutoSuggestBox::OnInkingFunctionButtonClicked(
    _In_ CDependencyObject* pAutoSuggestBox)
{
    ctl::ComPtr<DependencyObject> spAbsPeer;
    ctl::ComPtr<AutoSuggestBox> spAbs;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(pAutoSuggestBox, &spAbsPeer));
    IFC_RETURN(spAbsPeer.As(&spAbs));
    IFC_RETURN(spAbs->ProgrammaticSubmitQuery());

    return S_OK;
}

