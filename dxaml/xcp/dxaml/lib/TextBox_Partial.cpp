// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextBox.g.h"
#include "TextBoxAutomationPeer.g.h"
#include "TextBoxTextChangingEventArgs.g.h"
#include "ButtonBase.g.h"
#include "DoubleAnimation.g.h"
#include "Storyboard.g.h"
#include "ScrollViewer.g.h"
#include "SizeChangedEventArgs.g.h"
#include "IsEnabledChangedEventArgs.g.h"
#include "CharacterReceivedRoutedEventArgs.g.h"
#include "TextAlternativesAsyncOperation.h"
#include <XamlTraceLogging.h>
#include "AutomationProperties.h"
#include "TextBoxPlaceholderTextHelper.h"
#include "TextControlHelper.h"
#include "TextBoxBeforeTextChangingEventArgs.g.h"
#include "localizedResource.h"
#include "PointerRoutedEventArgs.g.h"
#include "DoubleTappedRoutedEventArgs.g.h"
#include "TappedRoutedEventArgs.g.h"
#include "RightTappedRoutedEventArgs.g.h"
#include "HoldingRoutedEventArgs.g.h"
#include "ManipulationStartedRoutedEventArgs.g.h"
#include "ManipulationCompletedRoutedEventArgs.g.h"
#include "ContextRequestedEventArgs.g.h"
#include "TextBoxView.h"

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace std::placeholders;

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a new instance.
//
//---------------------------------------------------------------------------
TextBox::TextBox()
    : m_isInitializing(TRUE),
      m_isAnimatingHeight(FALSE),
      m_storyboardCompletedToken()
{
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Releases resources.
//
//---------------------------------------------------------------------------
TextBox::~TextBox()
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//
//      override base implmentation of DisconnectFrameworkPeer so
//      that the code get chances to release the embedded objects
//      and event handlers.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
TextBox::DisconnectFrameworkPeerCore()
{
    HRESULT hr = S_OK;

    ReleaseTemplateParts();

    IFC(TextBoxGenerated::DisconnectFrameworkPeerCore());

Cleanup:

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP TextBox::OnPointerEntered(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(TextBoxGenerated::OnPointerEntered(pArgs));
    IFC_RETURN(ValidateEvent(static_cast<PointerRoutedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_PointerEntered));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP TextBox::OnPointerExited(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(TextBoxGenerated::OnPointerExited(pArgs));
    IFC_RETURN(ValidateEvent(static_cast<PointerRoutedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_PointerExited));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP TextBox::OnPointerPressed(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(TextBoxGenerated::OnPointerPressed(pArgs));
    IFC_RETURN(ValidateEvent(static_cast<PointerRoutedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_PointerPressed));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP TextBox::OnPointerMoved(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(TextBoxGenerated::OnPointerMoved(pArgs));
    IFC_RETURN(ValidateEvent(static_cast<PointerRoutedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_PointerMoved));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP TextBox::OnPointerReleased(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(TextBoxGenerated::OnPointerReleased(pArgs));
    IFC_RETURN(ValidateEvent(static_cast<PointerRoutedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_PointerReleased));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP TextBox::OnPointerCaptureLost(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(TextBoxGenerated::OnPointerCaptureLost(pArgs));
    IFC_RETURN(ValidateEvent(static_cast<PointerRoutedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_PointerCaptureLost));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP TextBox::OnPointerCanceled(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(TextBoxGenerated::OnPointerCanceled(pArgs));
    IFC_RETURN(ValidateEvent(static_cast<PointerRoutedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_PointerCanceled));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP TextBox::OnDoubleTapped(_In_ xaml_input::IDoubleTappedRoutedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(TextBoxGenerated::OnDoubleTapped(pArgs));
    IFC_RETURN(ValidateEvent(static_cast<DoubleTappedRoutedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_DoubleTapped));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP TextBox::OnTapped(_In_ xaml_input::ITappedRoutedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(TextBoxGenerated::OnTapped(pArgs));
    IFC_RETURN(ValidateEvent(static_cast<TappedRoutedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_Tapped));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP TextBox::OnRightTapped(_In_ xaml_input::IRightTappedRoutedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(TextBoxGenerated::OnRightTapped(pArgs));
    IFC_RETURN(ValidateEvent(static_cast<RightTappedRoutedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_RightTapped));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP TextBox::OnHolding(_In_ xaml_input::IHoldingRoutedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(TextBoxGenerated::OnHolding(pArgs));
    IFC_RETURN(ValidateEvent(static_cast<HoldingRoutedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_Holding));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP TextBox::OnManipulationStarted(_In_ xaml_input::IManipulationStartedRoutedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(TextBoxGenerated::OnManipulationStarted(pArgs));
    IFC_RETURN(ValidateEvent(static_cast<ManipulationStartedRoutedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_ManipulationStarted));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP TextBox::OnManipulationCompleted(_In_ xaml_input::IManipulationCompletedRoutedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(TextBoxGenerated::OnManipulationCompleted(pArgs));
    IFC_RETURN(ValidateEvent(static_cast<ManipulationCompletedRoutedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_ManipulationCompleted));
    }

    return S_OK;
}

_Check_return_ HRESULT TextBox::ValidateEvent(_In_ xaml::IRoutedEventArgs* pEventArgs, _Out_ bool* pIsValidEvent)
{
    *pIsValidEvent = true;

    ctl::ComPtr<IInspectable> spOriginalSource;
    ctl::ComPtr<IFrameworkElement> spFrameworkElement;

    BOOLEAN bSourceIsDeleteButton = FALSE;

    IFC_RETURN(pEventArgs->get_OriginalSource(&spOriginalSource));
    spFrameworkElement = spOriginalSource.AsOrNull<IFrameworkElement>();

    if (spFrameworkElement)
    {
        if (m_tpDeleteButton)
        {
            IFC_RETURN(m_tpDeleteButton.Cast<ButtonBase>()->IsAncestorOf(static_cast<FrameworkElement *>(spFrameworkElement.Get()), &bSourceIsDeleteButton));
        }
    }

    // Disallow event on deleteButton
    *pIsValidEvent = !bSourceIsDeleteButton;

    return S_OK;
}

_Check_return_ HRESULT TextBox::OnContextRequestedImpl(
    _In_ xaml_input::IContextRequestedEventArgs* pArgs)
{
    bool isValidEvent = false;

    IFC_RETURN(ValidateEvent(static_cast<ContextRequestedEventArgs*>(pArgs), &isValidEvent));

    if (isValidEvent)
    {
        IFC_RETURN(ShowContextFlyout(pArgs, this));
    }

    return S_OK;
}
//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP TextBox::OnKeyUp(_In_ IKeyRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFC(TextBoxGenerated::OnKeyUp(pArgs));
    IFC(RaiseNative(pArgs, KnownEventIndex::UIElement_KeyUp));
Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP TextBox::OnKeyDown(_In_ IKeyRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFC(TextBoxGenerated::OnKeyDown(pArgs));
    IFC(RaiseNative(pArgs, KnownEventIndex::UIElement_KeyDown));
Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP TextBox::OnGotFocus(_In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFC(TextBoxGenerated::OnGotFocus(pArgs));
    IFC(RaiseNative(pArgs, KnownEventIndex::UIElement_GotFocus));
Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP TextBox::OnLostFocus(_In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFC(TextBoxGenerated::OnLostFocus(pArgs));
    IFC(RaiseNative(pArgs, KnownEventIndex::UIElement_LostFocus));
Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBox::OnCharacterReceivedImpl(_In_ xaml_input::ICharacterReceivedRoutedEventArgs* pArgs)
{
    IFC_RETURN(RaiseNative(ctl::as_iinspectable(pArgs), KnownEventIndex::UIElement_CharacterReceived));
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, raises event in the core layer
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBox::OnInheritedPropertyChanged(_In_ IInspectable* pArgs)
{
    HRESULT hr = S_OK;
    IFC(TextBoxGenerated::OnInheritedPropertyChanged(pArgs));
    IFC(RaiseNative(pArgs, KnownEventIndex::Control_InheritedPropertyChanged));
Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IsEnabled property changed override, raises event in the core layer
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBox::OnIsEnabledChanged(_In_ IsEnabledChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFC(TextBoxGenerated::OnIsEnabledChanged(pArgs));
    IFC(RaiseNative(ctl::iinspectable_cast(pArgs), KnownEventIndex::Control_IsEnabledChanged));
Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Prepare handlers.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBox::Initialize()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::ISizeChangedEventHandler> spSizeChangedHandler;
    EventRegistrationToken sizeChangedToken;

    IFC(TextBoxGenerated::Initialize());

    spSizeChangedHandler.Attach(
        new ClassMemberEventHandler<
            TextBox,
            ITextBox,
            xaml::ISizeChangedEventHandler,
            IInspectable,
            xaml::ISizeChangedEventArgs>(this, &TextBox::OnSizeChanged, true /* subscribingToSelf */ ));
    IFC(add_SizeChanged(spSizeChangedHandler.Get(), &sizeChangedToken));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      SizeChanged Handler
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBox::OnSizeChanged(
    _In_ IInspectable* pSender,
    _In_ xaml::ISizeChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    if (m_isAnimatingHeight)
    {
        goto Cleanup;
    }

    wf::Size previousSize;
    wf::Size newSize;

    // Get the sizes and set the animation values.
    IFC(pArgs->get_PreviousSize(&previousSize));
    IFC(pArgs->get_NewSize(&newSize));

    // Do not animate if the TextBox is setting its initial height.
    if (previousSize.Height == 0)
    {
        goto Cleanup;
    }

    // Make sure that we are not currently animating and that we do not animate a subpixel
    // Height change.
    if(DoubleUtil::Abs(newSize.Height - previousSize.Height) >= 1)
    {
        // Skip the animation if this control does not have focus.
        ctl::ComPtr<DependencyObject> spFocused;
        IFC(GetFocusedElement(&spFocused));
        if (spFocused.Get() != static_cast<DependencyObject*>(this))
        {
            goto Cleanup;
        }

        // Skip the animation if the current height is different from NaN.
        ctl::ComPtr<IInspectable> spSender(pSender);
        ctl::ComPtr<IFrameworkElement> spSenderAsFE;
        IFC(spSender.As(&spSenderAsFE));
        DOUBLE startHeight = 0.0;
        IFC(spSenderAsFE->get_Height(&startHeight));
        if (!DoubleUtil::IsNaN(startHeight))
        {
            goto Cleanup;
        }

        // Skip the animation when TextWrapping is turned off and AcceptsReturn is false.
        xaml::TextWrapping textWrapping = xaml::TextWrapping_NoWrap;
        IFC(get_TextWrapping(&textWrapping));
        BOOLEAN acceptsReturn = false;
        IFC(get_AcceptsReturn(&acceptsReturn));
        if (textWrapping == xaml::TextWrapping_NoWrap && !acceptsReturn)
        {
            goto Cleanup;
        }

        // Variables.
        ctl::ComPtr<IActivationFactory> spActivationFactory;
        ctl::ComPtr<xaml_animation::IStoryboardStatics> spStoryboardStatics;
        ctl::ComPtr<xaml::IDependencyObject> spSenderAsDO;

        ctl::ComPtr<Storyboard> spStoryboard;
        ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> spTimelines;
        ctl::ComPtr<DoubleAnimation> spDoubleAnimation;

        ctl::ComPtr<IInspectable> spFrom;
        ctl::ComPtr<IInspectable> spTo;
        ctl::ComPtr<wf::IReference<DOUBLE>> spFromReference;
        ctl::ComPtr<wf::IReference<DOUBLE>> spToReference;

        wrl_wrappers::HStringReference strPropertyName(L"Height");

        // Set the duration for the animation.
        INT64 durationTime = 1000000L; // 100 ms.
        xaml::Duration duration = { { durationTime }, xaml::DurationType::DurationType_TimeSpan };

        // Get the statics
        spActivationFactory.Attach(ctl::ActivationFactoryCreator<StoryboardFactory>::CreateActivationFactory());
        IFC(spActivationFactory.As(&spStoryboardStatics));

        // Create the animation and the storyboard.
        IFC(ctl::make(&spDoubleAnimation));
        IFC(ctl::make(&spStoryboard));

        // Enable dependent animation, as we are animating height.
        IFC(spDoubleAnimation->put_EnableDependentAnimation(TRUE));
        IFC(spDoubleAnimation->put_Duration(duration));

        // Add the from/to properties to the animation.
        IFC(PropertyValue::CreateFromDouble(previousSize.Height, &spFrom));
        IFC(spFrom.As(&spFromReference));
        IFC(spDoubleAnimation->put_From(spFromReference.Get()));
        IFC(PropertyValue::CreateFromDouble(newSize.Height, &spTo));
        IFC(spTo.As(&spToReference));
        IFC(spDoubleAnimation->put_To(spToReference.Get()));

        // Set the targets.
        IFC(spSender.As(&spSenderAsDO));
        IFC(spStoryboardStatics->SetTargetProperty(spDoubleAnimation.Get(), strPropertyName.Get()));
        IFC(spStoryboardStatics->SetTarget(spDoubleAnimation.Get(), spSenderAsDO.Get()));

        // Add the double animation to the storyboard.
        IFC(spStoryboard->get_Children(&spTimelines));
        IFC(spTimelines->Append(spDoubleAnimation.Get()));

        // Add a completed handler.
        IFC(spStoryboard->add_Completed(Microsoft::WRL::Callback<wf::IEventHandler<IInspectable*>>(
            [this, spSenderAsFE](IInspectable* pSender, IInspectable* pArgs)
            {
                RRETURN(this->OnHeightAnimationCompleted(pSender, spSenderAsFE.Get()));
            }).Get(), &m_storyboardCompletedToken));

        // Start the storyboard and block animation events.
        IFC(spStoryboard->Begin());
        m_isAnimatingHeight = TRUE;

        // Prevent the caret from being brought into view until the animation is complete.
        static_cast<CTextBox*>(GetHandle())->DisableEnsureRectVisible();

        // Prevent this event from continuing to bubble up because we are animating from the
        // "old" value.
        IFC(static_cast<SizeChangedEventArgs*>(pArgs)->SetHandled(TRUE));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT TextBox::OnHeightAnimationCompleted(IInspectable* pSender, IFrameworkElement* pSenderAsFE)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spSenderAsI(pSender);
    ctl::ComPtr<ITimeline> spSenderAsIT;
    CTextBox *pTextBoxCore = static_cast<CTextBox*>(GetHandle());

    // Set the height to NaN, as the "completion height" will
    // force the textbox to keep its current size.  This allows
    // the textbox to expand.
    IFC(pSenderAsFE->put_Height(DoubleUtil::NaN));

    IFC(spSenderAsI.As(&spSenderAsIT));
    IFC(spSenderAsIT->remove_Completed(m_storyboardCompletedToken));

    m_storyboardCompletedToken = EventRegistrationToken();

    IFC(pTextBoxCore->GetView()->UpdateCaretElement());
    IFC(pTextBoxCore->RaisePendingBringLastVisibleRectIntoView(TRUE /* forceIntoView */, false /* focusChanged */));

Cleanup:
    // Allow height to animate on a new size change (we
    // didn't get this event from the animation size change).
    this->m_isAnimatingHeight = FALSE;

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Passes control to the core layer.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBox::RaiseNative(
    _In_ IInspectable* pArgs,
    _In_ KnownEventIndex nDelegate)
{
    return RaiseNative(this, pArgs, nDelegate);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Passes control to the core layer.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBox::RaiseNative(
    _In_ ControlGenerated* pTextBoxBase,
    _In_ IInspectable* pArgs,
    _In_ KnownEventIndex nDelegate)
{
    ctl::ComPtr<EventArgs> spArgs;
    IFCPTR_RETURN(pArgs);

    IFC_RETURN(ctl::do_query_interface(spArgs, pArgs));
    xref_ptr<CEventArgs> coreArgs;
    coreArgs.attach(spArgs->GetCorePeer());

    IFC_RETURN(CoreImports::Control_Raise(
        static_cast<CControl*>(pTextBoxBase->GetHandle()),
        coreArgs,
        nDelegate));

    return S_OK;
}

IFACEMETHODIMP TextBox::add_CandidateWindowBoundsChanged(_In_ wf::ITypedEventHandler<xaml_controls::TextBox*, xaml_controls::CandidateWindowBoundsChangedEventArgs*>* pValue, _Out_ EventRegistrationToken* ptToken)
{
    // This may fail when text services are not functioning
    // TODO: http://task.ms/1887013
    IGNOREHR(static_cast<CTextBoxBase*>(GetHandle())->EnableCandidateWindowBoundsTracking(EventHandle(KnownEventIndex::TextBox_CandidateWindowBoundsChanged)));
    return TextBoxGenerated::add_CandidateWindowBoundsChanged(pValue, ptToken);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Delegate private interface property implementation of
//      Desktop context menu (used by TextBox) to core layer.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBox::get_IsDesktopPopupMenuEnabledImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;

    IFC(get_IsCoreDesktopPopupMenuEnabled(pValue));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Delegate private interface property implementation of
//      Desktop context menu (used by TextBox) to core layer.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBox::put_IsDesktopPopupMenuEnabledImpl(_In_ BOOLEAN value)
{
    HRESULT hr = S_OK;

    IFC(put_IsCoreDesktopPopupMenuEnabled(value));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT TextBox::UndoImpl()
{
    IFC_RETURN(static_cast<CTextBox*>(GetHandle())->Undo());
    return S_OK;
}

_Check_return_ HRESULT TextBox::RedoImpl()
{
    IFC_RETURN(static_cast<CTextBox*>(GetHandle())->Redo());
    return S_OK;
}

_Check_return_ HRESULT TextBox::PasteFromClipboardImpl()
{
    IFC_RETURN(static_cast<CTextBox*>(GetHandle())->Paste());
    return S_OK;
}

_Check_return_ HRESULT TextBox::CopySelectionToClipboardImpl()
{
    IFC_RETURN(static_cast<CTextBox*>(GetHandle())->Copy());
    return S_OK;
}

_Check_return_ HRESULT TextBox::CutSelectionToClipboardImpl()
{
    IFC_RETURN(static_cast<CTextBox*>(GetHandle())->Cut());
    return S_OK;
}

_Check_return_ HRESULT TextBox::ClearUndoRedoHistoryImpl()
{
    IFC_RETURN(static_cast<CTextBox*>(GetHandle())->ClearUndoRedoHistory());
    return S_OK;
}

_Check_return_ HRESULT TextBox::ForceEditFocusLossImpl()
{
    IFC_RETURN(static_cast<CTextBox*>(GetHandle())->ForceEditFocusLoss());

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Callback from core, adds a Click event listener on the template's
//      DeleteButton.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBox::OnApplyTemplateHandler(_In_ CDependencyObject* pNativeTextBox)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spPeer;
    ctl::ComPtr<IDependencyObject> spObject;
    ctl::ComPtr<IUIElement> spPlaceholderTextPresenter;
    TextBox* pTextBoxNoRef = NULL;
    EventRegistrationToken deleteButtonClickToken;
    EventRegistrationToken deleteButtonSizeChangedToken;
    HSTRING pTextBoxText = NULL;

    //
    // Get the framework peer.
    //

    IFC(DXamlCore::GetCurrent()->GetPeer(pNativeTextBox, &spPeer));
    pTextBoxNoRef = spPeer.Cast<TextBox>();

    //
    // Cleanup any state from a previous template.
    //

    pTextBoxNoRef->ReleaseTemplateParts();

    //
    // Find the DeleteButton in the current template.
    //

    IFC(pTextBoxNoRef->GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"DeleteButton")).Get(), &spObject));
    IFC(pTextBoxNoRef->GetTemplatePart<IUIElement>(STR_LEN_PAIR(L"PlaceholderTextContentPresenter"), spPlaceholderTextPresenter.ReleaseAndGetAddressOf()));

    pTextBoxNoRef->SetDeleteButton(spObject.Get());
    pTextBoxNoRef->SetPlaceholderTextPresenter(spPlaceholderTextPresenter.Get());

    if (pTextBoxNoRef->m_tpDeleteButton.Get() != NULL) // NULL when removed from default template.
    {
        ctl::ComPtr<xaml::IRoutedEventHandler> spDeleteButtonClickHandler;
        ctl::ComPtr<xaml::ISizeChangedEventHandler> spDeleteButtonSizeChangedHandler;
        wrl_wrappers::HString strAutomationName;

        //
        // Attach the Click event handler.
        //

        spDeleteButtonClickHandler.Attach(
            new ClassMemberEventHandler
                <TextBox, ITextBox, xaml::IRoutedEventHandler,  IInspectable, IRoutedEventArgs>
                (pTextBoxNoRef, &TextBox::OnDeleteButtonClick));

        IFC(pTextBoxNoRef->m_tpDeleteButton.Cast<ButtonBase>()->add_Click(spDeleteButtonClickHandler.Get(), &deleteButtonClickToken));
        static_cast<CFrameworkElement*>(pTextBoxNoRef->m_tpDeleteButton.Cast<ButtonBase>()->GetHandle())->SetCursor(MouseCursorArrow);
        //
        // Attach the SizeChanged event handler.
        //

        spDeleteButtonSizeChangedHandler.Attach(
            new ClassMemberEventHandler
                <TextBox, ITextBox, xaml::ISizeChangedEventHandler, IInspectable, ISizeChangedEventArgs>
                (pTextBoxNoRef, &TextBox::OnDeleteButtonSizeChanged));

        IFC(pTextBoxNoRef->m_tpDeleteButton.Cast<ButtonBase>()->add_SizeChanged(spDeleteButtonSizeChangedHandler.Get(), &deleteButtonSizeChangedToken));

        // Set the AutomationProperties.Name for the DeleteButton.
        IFC(DirectUI::AutomationProperties::GetNameStatic(pTextBoxNoRef->m_tpDeleteButton.Cast<ButtonBase>(), strAutomationName.ReleaseAndGetAddressOf()));

        if(strAutomationName.Get() == nullptr)
        {
            IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_TEXTBOX_DELETE, strAutomationName.ReleaseAndGetAddressOf()));
            IFC(DirectUI::AutomationProperties::SetNameStatic(pTextBoxNoRef->m_tpDeleteButton.Cast<ButtonBase>(), strAutomationName));
        }
    }

    IFC(pTextBoxNoRef->UpdateHeaderPresenterVisibility());

    pTextBoxNoRef->get_Text(&pTextBoxText);
    IFC(pTextBoxNoRef->UpdatePlaceholderTextPresenterVisibility(pTextBoxText == NULL));

Cleanup:
    if (pTextBoxNoRef)
    {
        pTextBoxNoRef->m_isInitializing = FALSE;
    }
    DELETE_STRING(pTextBoxText);

    RRETURN(hr);
}

_Check_return_ HRESULT TextBox::OnTextChangingHandler(_In_ CTextBox* const pNativeTextBox, _In_ bool fTextChanged)
{
    ctl::ComPtr<DependencyObject> peer;
    IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(pNativeTextBox, &peer));

    ctl::ComPtr<ITextBox> control;
    IFC_RETURN(peer.As(&control));
    IFCPTR_RETURN(control);

    ctl::ComPtr<TextBox> peerAsTextBox;
    IFC_RETURN(peer.As(&peerAsTextBox));

    TextBox::TextChangingEventSourceType* eventSource = nullptr;
    IFC_RETURN(control.Cast<TextBox>()->GetTextChangingEventSourceNoRef(&eventSource));
    if (!peerAsTextBox->m_textChangingEventArgs.Get())
    {
        IFC_RETURN(ctl::make(&(peerAsTextBox->m_textChangingEventArgs)));
    }

    peerAsTextBox->m_textChangingEventArgs->put_IsContentChanging(fTextChanged);

    IFC_RETURN(eventSource->Raise(control.Get(), peerAsTextBox->m_textChangingEventArgs.Get()));

    return S_OK;
}

_Check_return_ HRESULT TextBox::OnContextMenuOpeningHandler(_In_ CDependencyObject* const pNativeTextBox, _In_ XFLOAT cursorLeft, _In_ XFLOAT cursorTop, _Out_ bool& handled)
{
    return (TextControlHelper::OnContextMenuOpeningHandler<ITextBox, TextBox>(pNativeTextBox, cursorLeft, cursorTop, handled));
}

_Check_return_ HRESULT TextBox::QueueUpdateSelectionFlyoutVisibility(_In_ CDependencyObject *pNativeTextBox)
{
    return TextControlHelper::QueueUpdateSelectionFlyoutVisibility<TextBox>(pNativeTextBox);
}

_Check_return_ HRESULT TextBox::UpdateSelectionFlyoutVisibility()
{
    return TextControlHelper::UpdateSelectionFlyoutVisibility<CTextBox>(this);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Callback from core, toggles the Visibility of Placeholder Text
//      whenever text is changed.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBox::ShowPlaceholderTextHandler(_In_ CDependencyObject* pNativeTextBox, _In_ bool isEnabled)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spPeer;
    TextBox* pTextBoxNoRef = NULL;

    //
    // Get the framework peer.
    //

    IFC(DXamlCore::GetCurrent()->GetPeer(pNativeTextBox, &spPeer));
    pTextBoxNoRef = spPeer.Cast<TextBox>();

    IFC(pTextBoxNoRef->UpdatePlaceholderTextPresenterVisibility(isEnabled));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Callback from core, retrieves the value of ScrollViewer.BringIntoViewOnFocusChange.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBox::GetBringIntoViewOnFocusChange(_In_ CDependencyObject *pNativeTextBox, _Out_ bool *pBringIntoViewOnFocusChange)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spPeer;
    BOOLEAN bringIntoViewOnFocusChange = FALSE;

    IFCPTR(pBringIntoViewOnFocusChange);
    *pBringIntoViewOnFocusChange = FALSE;

    //
    // Get the framework peer.
    //

    IFC(DXamlCore::GetCurrent()->GetPeer(pNativeTextBox, &spPeer));
    IFC(ScrollViewerFactory::GetBringIntoViewOnFocusChangeStatic(spPeer.Get(), &bringIntoViewOnFocusChange));

    *pBringIntoViewOnFocusChange = !!bringIntoViewOnFocusChange;

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a TextBoxAutomationPeer to represent the TextBox.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP TextBox::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::ITextBoxAutomationPeer> spTextBoxAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::ITextBoxAutomationPeerFactory> spTextBoxAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::TextBoxAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spTextBoxAPFactory));

    IFC(spTextBoxAPFactory.Cast<TextBoxAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spTextBoxAutomationPeer));
    IFC(spTextBoxAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a plain text string to provide a default AutomationProperties.Name
//        in the absence of an explicitly defined one
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBox::GetPlainText(_Out_ HSTRING* strPlainText)
{
    ctl::ComPtr<IInspectable> spHeader;
    *strPlainText = nullptr;
    XUINT32 pLength = 0;

    IFC_RETURN(get_Header(&spHeader));

    if (spHeader != nullptr)
    {
        IFC_RETURN(FrameworkElement::GetStringFromObject(spHeader.Get(), strPlainText));
        pLength = ::WindowsGetStringLen(*strPlainText); 
    }

    if (pLength == 0)
    {
        IFC_RETURN(get_PlaceholderText(strPlainText));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called whenever the DeleteButton is clicked.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBox::OnDeleteButtonClick(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pArgs
    )
{
    //Log "DeleteButton" Click Event
    TraceLoggingWrite(g_hTraceProvider,
        "TextBox_Clear_Event",
        TraceLoggingLevel(WINEVENT_LEVEL_LOG_ALWAYS),
        TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage),
        TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY),
        TraceLoggingValue("OnDeleteButtonClick"));

    // Just pass along the news to the core CTextBox.
    CTextBox* pTextBox = static_cast<CTextBox* >(GetHandle());
    RRETURN(CoreImports::TextBox_OnDeleteButtonClick(pTextBox));
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called whenever the DeleteButton's size is changed.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBox::OnDeleteButtonSizeChanged(
    _In_ IInspectable* pSender,
    _In_ xaml::ISizeChangedEventArgs* pArgs
    )
{
    HRESULT hr = S_OK;

    // Height of the DeleteButton is changing when the height of the TextBox is changing.
    // In order to maintain square button, set the width whenever height is changed.
    if (m_tpDeleteButton.Get() != NULL)
    {
        wf::Size newDeleteButtonSize;
        DOUBLE deleteButtonWidth;

        IFC(pArgs->get_NewSize(&newDeleteButtonSize));
        IFC(m_tpDeleteButton.Cast<ButtonBase>()->get_Width(&deleteButtonWidth));

        if (newDeleteButtonSize.Height != deleteButtonWidth)
        {
            IFC(m_tpDeleteButton.Cast<ButtonBase>()->put_Width(newDeleteButtonSize.Height));
        }
    }

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Releases resources associated with the current template.
//
//---------------------------------------------------------------------------
void TextBox::ReleaseTemplateParts()
{
    m_tpDeleteButton.Clear();
    m_tpHeaderPresenter.Clear();
    m_tpPlaceholderTextPresenter.Clear();
    m_requiredHeaderPresenter.Clear();
}

_Check_return_ HRESULT TextBox::ShouldHeaderBeVisible(_Out_ bool* visible)
{
    ctl::ComPtr<xaml::IDataTemplate> spHeaderTemplate;
    ctl::ComPtr<IInspectable> spHeaderValue;

    *visible = false;

    IFC_RETURN(get_HeaderTemplate(&spHeaderTemplate));
    IFC_RETURN(get_Header(&spHeaderValue));

    if (spHeaderTemplate)
    {
        *visible = true;
    }
    else if (spHeaderValue)
    {
        ctl::ComPtr<xaml::IUIElement> spHeaderValueAsUIElement;
        HRESULT hr = spHeaderValue.As<IUIElement>(&spHeaderValueAsUIElement);

        *visible = true;

        if (hr == E_NOINTERFACE)   // not UI Element
        {
            wf::PropertyType type = wf::PropertyType_Empty;

            IFC_RETURN(ctl::do_get_property_type(spHeaderValue.Get(), &type));

            if (type == wf::PropertyType_String)
            {
                wrl_wrappers::HString strHeaderText;

                // If this fails it means spHeaderValue isn't the type we want, so we dont
                // have to deal with the HR.
                IGNOREHR(IValueBoxer::UnboxValue(spHeaderValue.Get(), strHeaderText.ReleaseAndGetAddressOf()));

                if (strHeaderText == nullptr)
                {
                    *visible = false; // special case: if header property is set to "", make its visual state collapsed
                }
            }
        }
        else
        {
            IFC_RETURN(hr);
        }
    }

    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the visibility of the Header property. If Header and Header
//      Template are not set, it should collapse the property.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextBox::UpdateHeaderPresenterVisibility()
{
    bool visible = false;

    IFC_RETURN(ShouldHeaderBeVisible(&visible));

    IFC_RETURN(ConditionallyGetTemplatePartAndUpdateVisibility(
        XSTRING_PTR_EPHEMERAL(L"HeaderContentPresenter"),
        visible,
        m_tpHeaderPresenter));

    IFC_RETURN(ConditionallyGetTemplatePartAndUpdateVisibility(
        XSTRING_PTR_EPHEMERAL(L"RequiredHeaderPresenter"),
        visible && IsValueRequired(this),
        m_requiredHeaderPresenter));
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Updates PlaceholderText visibility whenever text is updated
//
//---------------------------------------------------------------------------
_Check_return_
HRESULT
TextBox::UpdatePlaceholderTextPresenterVisibility(_In_ bool isEnabled)
{
    if (m_tpPlaceholderTextPresenter.Get())
    {
        IFC_RETURN(TextBoxPlaceholderTextHelper::UpdatePlaceholderTextPresenterVisibility(
            this, m_tpPlaceholderTextPresenter.Get(), isEnabled));
    }
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Handles the custom property changed event and calls OnPropertyChanged2
//      Methods.
//
//---------------------------------------------------------------------------
_Check_return_
HRESULT
TextBox::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(TextBoxGenerated::OnPropertyChanged2(args));

    // We will ignore property changes during initialization and take care of them when we have a template.
    if (!m_isInitializing)
    {
        switch(args.m_pDP->GetIndex())
        {
            case KnownPropertyIndex::TextBox_Header:
            case KnownPropertyIndex::TextBox_HeaderTemplate:
                IFC_RETURN(UpdateHeaderPresenterVisibility());
                static_cast<CTextBox*>(GetHandle())->InvalidateView();
                break;
            case KnownPropertyIndex::TextBox_PlaceholderText:
                //UpdatePlaceholder visibility here
                IFC_RETURN(UpdatePlaceholderTextPresenterVisibility(
                    TextBoxPlaceholderTextHelper::ShouldMakePlaceholderTextVisible(
                        m_tpPlaceholderTextPresenter.Get(), this)));
                break;
            case KnownPropertyIndex::TextBox_Text:
            {
                wrl_wrappers::HString text;
                IFC_RETURN(get_Text(text.GetAddressOf()));
                IFC_RETURN(InvokeValidationCommand(this, text.Get()));
                break;
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT TextBox::GetLinguisticAlternativesAsyncImpl(
    _Outptr_ wf::IAsyncOperation<wfc::IVectorView<HSTRING>*>** ppReturnValue)
{
    HRESULT hr = S_OK;
    Microsoft::WRL::ComPtr<TextAlternativesOperation> spLoadMoreItemsOperation;

    IFC(Microsoft::WRL::MakeAndInitialize<TextAlternativesOperation>(&spLoadMoreItemsOperation));
    IFC(spLoadMoreItemsOperation->Init(static_cast<CTextBoxBase*>(GetHandle())));
    IFC(spLoadMoreItemsOperation->Start());

    IFC(spLoadMoreItemsOperation.CopyTo(ppReturnValue));

Cleanup:
    RRETURN(hr);
}

// static
_Check_return_ HRESULT TextBox::OnBeforeTextChangingHandler(_In_ CTextBox* const nativeTextBox, _In_ xstring_ptr* newString, _Out_ BOOLEAN* wasCanceled)
{
    ctl::ComPtr<DependencyObject> peer;

    IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(nativeTextBox, &peer));

    if (!peer)
    {
        // There is no need to fire the PasswordBoxChanging event if there is no peer, since no one is listening
        return S_FALSE;
    }

    ctl::ComPtr<ITextBox> textBox;
    IFC_RETURN(peer.As(&textBox));
    IFCPTR_RETURN(textBox);

    ctl::ComPtr<TextBox> peerAsTextBox;
    IFC_RETURN(peer.As(&peerAsTextBox));
    IFCPTR_RETURN(peerAsTextBox);

    if (!peerAsTextBox->m_beforeTextChangingEventArgs)
    {
        IFC_RETURN(ctl::make(&(peerAsTextBox->m_beforeTextChangingEventArgs)));
    }

    xruntime_string_ptr strPromoted;
    IFC_RETURN(newString->Promote(&strPromoted));

    IFC_RETURN(peerAsTextBox->m_beforeTextChangingEventArgs->put_NewText(strPromoted.GetHSTRING()));
    IFC_RETURN(peerAsTextBox->m_beforeTextChangingEventArgs->put_Cancel(FALSE));

    TextBox::BeforeTextChangingEventSourceType* eventSource = nullptr;
    IFC_RETURN(textBox.Cast<TextBox>()->GetBeforeTextChangingEventSourceNoRef(&eventSource));
    IFC_RETURN(eventSource->Raise(textBox.Get(), peerAsTextBox->m_beforeTextChangingEventArgs.Get()));

    IFC_RETURN(peerAsTextBox->m_beforeTextChangingEventArgs->get_Cancel(wasCanceled));

    return S_OK;
}

_Check_return_ HRESULT TextBox::OnSelectionChangingHandler(
    _In_ CDependencyObject* const nativeTextBox,
    _In_ long selectionStart,
    _In_ long selectionLength,
    _Out_ BOOLEAN* wasCanceled)
{
    *wasCanceled = FALSE;
    ctl::ComPtr<DependencyObject> peer;

    IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(nativeTextBox, &peer));

    if (!peer)
    {
        return S_OK;
    }

    ctl::ComPtr<ITextBox> textBox;
    IFC_RETURN(peer.As(&textBox));
    IFCPTR_RETURN(textBox);

    ctl::ComPtr<TextBox> peerAsTextBox;
    IFC_RETURN(peer.As(&peerAsTextBox));
    IFCPTR_RETURN(peerAsTextBox);

    if (!peerAsTextBox->m_selectionChangingEventArgs)
    {
        IFC_RETURN(ctl::make(&(peerAsTextBox->m_selectionChangingEventArgs)));
    }

    IFC_RETURN(peerAsTextBox->m_selectionChangingEventArgs->put_SelectionStart(selectionStart));
    IFC_RETURN(peerAsTextBox->m_selectionChangingEventArgs->put_SelectionLength(selectionLength));
    IFC_RETURN(peerAsTextBox->m_selectionChangingEventArgs->put_Cancel(FALSE));

    TextBox::SelectionChangingEventSourceType* eventSource = nullptr;
    IFC_RETURN(textBox.Cast<TextBox>()->GetSelectionChangingEventSourceNoRef(&eventSource));
    IFC_RETURN(eventSource->Raise(textBox.Get(), peerAsTextBox->m_selectionChangingEventArgs.Get()));

    IFC_RETURN(peerAsTextBox->m_selectionChangingEventArgs->get_Cancel(wasCanceled));

    return S_OK;
}
