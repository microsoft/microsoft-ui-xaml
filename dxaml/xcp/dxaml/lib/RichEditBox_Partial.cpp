// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RichEditBox.g.h"
#include "TextBox.g.h"
#include "RichEditBoxAutomationPeer.g.h"
#include "RichEditBoxTextChangingEventArgs.g.h"
#include "ContextMenuEventArgs.g.h"
#include "DoubleAnimation.g.h"
#include "Storyboard.g.h"
#include "SizeChangedEventArgs.g.h"
#include "IsEnabledChangedEventArgs.g.h"
#include "CharacterReceivedRoutedEventArgs.g.h"
#include "TextAlternativesAsyncOperation.h"
#include <xstrutil.h>
#include "StaticStore.h"
#include "TextBoxPlaceholderTextHelper.h"
#include "TextControlHelper.h"
#include "Launcher.h"
#include "TextBoxView.h"

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace std::placeholders;

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a new instance.
//
//---------------------------------------------------------------------------
RichEditBox::RichEditBox()
    : m_isInitializing(TRUE),
      m_isAnimatingHeight(FALSE)
{
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Releases resources held by an instance.
//
//---------------------------------------------------------------------------
RichEditBox::~RichEditBox()
{
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP RichEditBox::OnPointerEntered(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFC(RichEditBoxGenerated::OnPointerEntered(pArgs));
    IFC(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_PointerEntered));

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP RichEditBox::OnPointerExited(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFC(RichEditBoxGenerated::OnPointerExited(pArgs));
    IFC(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_PointerExited));

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP RichEditBox::OnPointerPressed(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFC(RichEditBoxGenerated::OnPointerPressed(pArgs));
    IFC(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_PointerPressed));

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP RichEditBox::OnPointerMoved(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFC(RichEditBoxGenerated::OnPointerMoved(pArgs));
    IFC(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_PointerMoved));

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP RichEditBox::OnPointerReleased(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFC(RichEditBoxGenerated::OnPointerReleased(pArgs));
    IFC(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_PointerReleased));

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP RichEditBox::OnPointerCaptureLost(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFC(RichEditBoxGenerated::OnPointerCaptureLost(pArgs));
    IFC(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_PointerCaptureLost));

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP RichEditBox::OnPointerCanceled(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFC(RichEditBoxGenerated::OnPointerCanceled(pArgs));
    IFC(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_PointerCanceled));

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP RichEditBox::OnDoubleTapped(_In_ xaml_input::IDoubleTappedRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFC(RichEditBoxGenerated::OnDoubleTapped(pArgs));
    IFC(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_DoubleTapped));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP RichEditBox::OnTapped(_In_ xaml_input::ITappedRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFC(RichEditBoxGenerated::OnTapped(pArgs));
    IFC(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_Tapped));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP RichEditBox::OnRightTapped(_In_ xaml_input::IRightTappedRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFC(RichEditBoxGenerated::OnRightTapped(pArgs));
    IFC(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_RightTapped));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP RichEditBox::OnHolding(_In_ xaml_input::IHoldingRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFC(RichEditBoxGenerated::OnHolding(pArgs));
    IFC(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_Holding));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP RichEditBox::OnManipulationStarted(_In_ xaml_input::IManipulationStartedRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFC(RichEditBoxGenerated::OnManipulationStarted(pArgs));
    IFC(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_ManipulationStarted));

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP RichEditBox::OnManipulationCompleted(_In_ xaml_input::IManipulationCompletedRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFC(RichEditBoxGenerated::OnManipulationCompleted(pArgs));
    IFC(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_ManipulationCompleted));

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP RichEditBox::OnKeyUp(_In_ IKeyRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(RichEditBoxGenerated::OnKeyUp(pArgs));
    IFC(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_KeyUp));

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP RichEditBox::OnKeyDown(_In_ IKeyRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(RichEditBoxGenerated::OnKeyDown(pArgs));
    IFC(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_KeyDown));

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP RichEditBox::OnGotFocus(_In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(RichEditBoxGenerated::OnGotFocus(pArgs));
    IFC(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_GotFocus));

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP RichEditBox::OnLostFocus(_In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(RichEditBoxGenerated::OnLostFocus(pArgs));
    IFC(TextBox::RaiseNative(this, pArgs, KnownEventIndex::UIElement_LostFocus));

Cleanup:
    return hr;
}


//---------------------------------------------------------------------------
//
//  Synopsis:
//      Create RichEditBoxAutomationPeer to represent the RichEditBox.
//
//---------------------------------------------------------------------------
IFACEMETHODIMP RichEditBox::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IRichEditBoxAutomationPeer> spRichEditBoxAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IRichEditBoxAutomationPeerFactory> spRichEditBoxAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::RichEditBoxAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spRichEditBoxAPFactory));

    IFC(spRichEditBoxAPFactory.Cast<RichEditBoxAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spRichEditBoxAutomationPeer));
    IFC(spRichEditBoxAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a plain text string to provide a default AutomationProperties.Name
//      in the absence of an explicitly defined one
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT RichEditBox::GetPlainText(_Out_ HSTRING* strPlainText)
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
//      Event handler override, passes control to core layer.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT RichEditBox::OnCharacterReceivedImpl(_In_ xaml_input::ICharacterReceivedRoutedEventArgs* pArgs)
{
    IFC_RETURN(TextBox::RaiseNative(this, ctl::as_iinspectable(pArgs), KnownEventIndex::UIElement_CharacterReceived));
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler override, raises event in the core layer
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT RichEditBox::OnInheritedPropertyChanged(_In_ IInspectable* pArgs)
{
    HRESULT hr = S_OK;
    IFC(RichEditBoxGenerated::OnInheritedPropertyChanged(pArgs));
    IFC(TextBox::RaiseNative(this, pArgs, KnownEventIndex::Control_InheritedPropertyChanged));
Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IsEnabled property changed override, raises event in the core layer
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT RichEditBox::OnIsEnabledChanged(_In_ IsEnabledChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFC(RichEditBoxGenerated::OnIsEnabledChanged(pArgs));
    IFC(TextBox::RaiseNative(this, ctl::iinspectable_cast(pArgs), KnownEventIndex::Control_IsEnabledChanged));
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT RichEditBox::get_DocumentImpl(_COM_Outptr_ mut::ITextDocument** returnValue)
{
    *returnValue = nullptr;

    ctl::ComPtr<IUnknown> spDocument;
    IFC_RETURN(CoreImports::RichEditBox_GetDocument(GetHandle(), __uuidof(mut::ITextDocument), &spDocument));

    IFC_RETURN(spDocument.CopyTo(returnValue));

    return S_OK;
}

_Check_return_ HRESULT RichEditBox::get_TextDocumentImpl(_COM_Outptr_ mut::ITextDocument** returnValue)
{
    IFC_RETURN(get_DocumentImpl(returnValue));

    return S_OK;
}

IFACEMETHODIMP RichEditBox::add_CandidateWindowBoundsChanged(_In_ wf::ITypedEventHandler<xaml_controls::RichEditBox*, xaml_controls::CandidateWindowBoundsChangedEventArgs*>* pValue, _Out_ EventRegistrationToken* ptToken)
{
    // This may fail when text services are not functioning
    // TODO: https://task.ms/1887013
    IGNOREHR(static_cast<CTextBoxBase*>(GetHandle())->EnableCandidateWindowBoundsTracking(EventHandle(KnownEventIndex::RichEditBox_CandidateWindowBoundsChanged)));
    return RichEditBoxGenerated::add_CandidateWindowBoundsChanged(pValue, ptToken);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Navigates a hyperlink by invoking the Launcher
//
//------------------------------------------------------------------------
_Check_return_ HRESULT RichEditBox::HandleHyperlinkNavigation(
    _In_reads_(cLinkText) WCHAR* pLinkText,
    XUINT32 cLinkText)
{
    ctl::ComPtr<wf::IUriRuntimeClassFactory> spUriFactory;
    IFC_RETURN(StaticStore::GetUriFactory(&spUriFactory));

    wrl_wrappers::HStringReference uriString(pLinkText);
    ctl::ComPtr<wf::IUriRuntimeClass> spUri;
    IFC_RETURN(spUriFactory->CreateUri(uriString.Get(), &spUri));

    IFC_RETURN(Launcher::TryInvokeLauncher(spUri.Get()));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      OnApplyTemplate callback from core, keeps templated parts around
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT RichEditBox::OnApplyTemplateHandler(_In_ CDependencyObject *pNativeRichEditBox)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spPeer;
    ctl::ComPtr<IUIElement> spPlaceholderTextPresenter;
    RichEditBox* pRichEditBoxNoRef = NULL;
    ctl::ComPtr<mut::ITextRange> spTextRange;
    ctl::ComPtr<mut::ITextDocument> spTextDocument;
    INT32 storyLength = 0;

    //
    // Get the framework peer.
    //

    IFC(DXamlCore::GetCurrent()->GetPeer(pNativeRichEditBox, &spPeer));
    pRichEditBoxNoRef = spPeer.Cast<RichEditBox>();

    //
    // Cleanup any state from a previous template.
    //

    pRichEditBoxNoRef->ReleaseTemplateParts();

    IFC(pRichEditBoxNoRef->GetTemplatePart<IUIElement>(STR_LEN_PAIR(L"PlaceholderTextContentPresenter"), spPlaceholderTextPresenter.ReleaseAndGetAddressOf()));

    pRichEditBoxNoRef->SetPlaceholderTextPresenter(spPlaceholderTextPresenter.Get());

    IFC(pRichEditBoxNoRef->UpdateHeaderPresenterVisibility());

    IFC(pRichEditBoxNoRef->get_Document(&spTextDocument));
    IFC(spTextDocument->GetRange(0, 2, &spTextRange));
    IFC(spTextRange->get_StoryLength(&storyLength));

    // I'm unable to use IsEmpty() on the native peer to check if the RichEditBox is empty, so instead I use
    // the same logic, checking if storyLength is <= 1.
    IFC(pRichEditBoxNoRef->UpdatePlaceholderTextPresenterVisibility(storyLength <= 1));

Cleanup:
    if (pRichEditBoxNoRef)
    {
        pRichEditBoxNoRef->m_isInitializing = FALSE;
    }
    RRETURN(hr);
}

// static
_Check_return_ HRESULT RichEditBox::OnTextChangingHandler(_In_ CDependencyObject *pNativeRichEditBox, _In_ bool fTextChanged)
{
    ctl::ComPtr<DependencyObject> peer;
    IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(pNativeRichEditBox, &peer));

    if (!peer)
    {
        // There is no need to fire the TextChanging event if there is no peer, since no one is listening
        return S_FALSE;
    }

    ctl::ComPtr<IRichEditBox> control;
    IFC_RETURN(peer.As(&control));
    IFCPTR_RETURN(control);

    ctl::ComPtr<RichEditBox> peerAsRichEditBox = control.Cast<RichEditBox>();

    if (!peerAsRichEditBox->m_textChangingEventArgs)
    {
        IFC_RETURN(ctl::make(&(peerAsRichEditBox->m_textChangingEventArgs)));
    }

    peerAsRichEditBox->m_textChangingEventArgs->put_IsContentChanging(fTextChanged);

    RichEditBox::TextChangingEventSourceType* eventSource = nullptr;
    IFC_RETURN(control.Cast<RichEditBox>()->GetTextChangingEventSourceNoRef(&eventSource));
    IFC_RETURN(eventSource->Raise(control.Get(), peerAsRichEditBox->m_textChangingEventArgs.Get()));

    return S_OK;
}

_Check_return_ HRESULT RichEditBox::OnSelectionChangingHandler(
    _In_ CDependencyObject* const nativeRichEditBox,
    _In_ long selectionStart,
    _In_ long selectionLength,
    _Out_ BOOLEAN* wasCanceled)
{
    *wasCanceled = FALSE;

    ctl::ComPtr<DependencyObject> peer;
    IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(nativeRichEditBox, &peer));

    if (!peer)
    {
        return S_OK;
    }

    ctl::ComPtr<IRichEditBox> richEditBox;
    IFC_RETURN(peer.As(&richEditBox));
    IFCPTR_RETURN(richEditBox);

    ctl::ComPtr<RichEditBox> peerAsRichEditBox;
    IFC_RETURN(peer.As(&peerAsRichEditBox));
    IFCPTR_RETURN(peerAsRichEditBox);

    if (!peerAsRichEditBox->m_selectionChangingEventArgs)
    {
        IFC_RETURN(ctl::make(&(peerAsRichEditBox->m_selectionChangingEventArgs)));
    }

    IFC_RETURN(peerAsRichEditBox->m_selectionChangingEventArgs->put_SelectionStart(selectionStart));
    IFC_RETURN(peerAsRichEditBox->m_selectionChangingEventArgs->put_SelectionLength(selectionLength));
    IFC_RETURN(peerAsRichEditBox->m_selectionChangingEventArgs->put_Cancel(FALSE));

    RichEditBox::SelectionChangingEventSourceType* eventSource = nullptr;
    IFC_RETURN(richEditBox.Cast<RichEditBox>()->GetSelectionChangingEventSourceNoRef(&eventSource));
    IFC_RETURN(eventSource->Raise(richEditBox.Get(), peerAsRichEditBox->m_selectionChangingEventArgs.Get()));

    IFC_RETURN(peerAsRichEditBox->m_selectionChangingEventArgs->get_Cancel(wasCanceled));

    return S_OK;
}
//---------------------------------------------------------------------------
//
//  Synopsis:
//      Prepare handlers.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT RichEditBox::Initialize()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::ISizeChangedEventHandler> spSizeChangedHandler;
    EventRegistrationToken sizeChangedToken;

    IFC(RichEditBoxGenerated::Initialize());

    spSizeChangedHandler.Attach(
        new ClassMemberEventHandler<
            RichEditBox,
            IRichEditBox,
            xaml::ISizeChangedEventHandler,
            IInspectable,
            xaml::ISizeChangedEventArgs>(this, &RichEditBox::OnSizeChanged, true /* subscribingToSelf */ ));
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
_Check_return_ HRESULT RichEditBox::OnSizeChanged(
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

    // Do not animate if the RichEditBox is setting its initial height.
    if (previousSize.Height == 0)
    {
        goto Cleanup;
    }

    // Make sure that we are not currently animating and that we do not animate a subpixel
    // Height change.
    if (DoubleUtil::Abs(newSize.Height - previousSize.Height) >= 1)
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
        xaml::Duration duration = {{durationTime}, xaml::DurationType::DurationType_TimeSpan};

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
        static_cast<CRichEditBox*>(GetHandle())->DisableEnsureRectVisible();

        // Prevent this event from continuing to bubble up because we are animating from the
        // "old" value.
        IFC(static_cast<SizeChangedEventArgs*>(pArgs)->SetHandled(TRUE));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT RichEditBox::OnHeightAnimationCompleted(IInspectable* pSender, IFrameworkElement* pSenderAsFE)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spSenderAsI (pSender);
    ctl::ComPtr<ITimeline> spSenderAsIT;
    CRichEditBox *pRichEditBoxCore = static_cast<CRichEditBox*>(GetHandle());

    // Set the height to NaN, as the "completion height" will
    // force the textbox to keep its current size.  This allows
    // the textbox to expand.
    IFC(pSenderAsFE->put_Height(DoubleUtil::NaN));

    IFC(spSenderAsI.As(&spSenderAsIT));
    IFC(spSenderAsIT->remove_Completed(m_storyboardCompletedToken));

    m_storyboardCompletedToken = EventRegistrationToken();

    IFC(pRichEditBoxCore->GetView()->UpdateCaretElement());
    IFC(pRichEditBoxCore->RaisePendingBringLastVisibleRectIntoView(TRUE /* forceIntoView */, false /* focusChanged */));

Cleanup:
    // Allow height to animate on a new size change (we
    // didn't get this event from the animation size change).
    this->m_isAnimatingHeight = FALSE;

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Callback from core, toggles the visiblity of Placeholder Text
//      whenever text is changed.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT RichEditBox::ShowPlaceholderTextHandler(_In_ CDependencyObject* pNativeRichEditBox, _In_ bool isEnabled)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spPeer;
    RichEditBox* pRichEditBoxNoRef = NULL;

    //
    // Get the framework peer.
    //

    IFC(DXamlCore::GetCurrent()->GetPeer(pNativeRichEditBox, &spPeer));
    pRichEditBoxNoRef = spPeer.Cast<RichEditBox>();

    IFC(pRichEditBoxNoRef->UpdatePlaceholderTextPresenterVisibility(isEnabled));

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Releases resources associated with the current template.
//
//---------------------------------------------------------------------------
void RichEditBox::ReleaseTemplateParts()
{
    m_tpHeaderPresenter.Clear();
    m_tpPlaceholderTextPresenter.Clear();
    m_requiredHeaderPresenter.Clear();
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the visibility of the Header property. If Header and Header
//      Template are not set, it should collapse the property.
//
//---------------------------------------------------------------------------
_Check_return_
HRESULT
RichEditBox::UpdateHeaderPresenterVisibility()
{
    ctl::ComPtr<xaml::IDataTemplate> spHeaderTemplate;
    ctl::ComPtr<IInspectable> spHeader;

    IFC_RETURN(get_HeaderTemplate(&spHeaderTemplate));
    IFC_RETURN(get_Header(&spHeader));

    IFC_RETURN(ConditionallyGetTemplatePartAndUpdateVisibility(
        XSTRING_PTR_EPHEMERAL(L"HeaderContentPresenter"),
        (spHeader || spHeaderTemplate),
        m_tpHeaderPresenter));

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
RichEditBox::UpdatePlaceholderTextPresenterVisibility(_In_ bool isEnabled)
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
RichEditBox::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(RichEditBoxGenerated::OnPropertyChanged2(args));

    // We will ignore property changes during initialization and take care of them when we have a template.
    if (!m_isInitializing)
    {
        switch(args.m_pDP->GetIndex())
        {
            case KnownPropertyIndex::RichEditBox_Header:
            case KnownPropertyIndex::RichEditBox_HeaderTemplate:
                IFC_RETURN(UpdateHeaderPresenterVisibility());
                static_cast<CRichEditBox*>(GetHandle())->InvalidateView();
                break;
            case KnownPropertyIndex::RichEditBox_PlaceholderText:
                //UpdatePlaceholder visibility here
                IFC_RETURN(UpdatePlaceholderTextPresenterVisibility(
                    TextBoxPlaceholderTextHelper::ShouldMakePlaceholderTextVisible(
                        m_tpPlaceholderTextPresenter.Get(), this)));
                break;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT RichEditBox::GetLinguisticAlternativesAsyncImpl(_Outptr_ wf::IAsyncOperation<wfc::IVectorView<HSTRING>*>** ppReturnValue)
{
    Microsoft::WRL::ComPtr<TextAlternativesOperation> spLoadMoreItemsOperation;

    IFC_RETURN(Microsoft::WRL::MakeAndInitialize<TextAlternativesOperation>(&spLoadMoreItemsOperation));
    IFC_RETURN(spLoadMoreItemsOperation->Init(static_cast<CTextBoxBase*>(GetHandle())));
    IFC_RETURN(spLoadMoreItemsOperation->Start());

    IFC_RETURN(spLoadMoreItemsOperation.CopyTo(ppReturnValue));

    return S_OK;
}

_Check_return_ HRESULT RichEditBox::OnContextMenuOpeningHandler(_In_ CDependencyObject* const pNativeRichEditBox, _In_ XFLOAT cursorLeft, _In_ XFLOAT cursorTop, _Out_ bool& handled)
{
    return (TextControlHelper::OnContextMenuOpeningHandler<IRichEditBox, RichEditBox>(pNativeRichEditBox, cursorLeft, cursorTop, handled));
}

_Check_return_ HRESULT RichEditBox::QueueUpdateSelectionFlyoutVisibility(_In_ CDependencyObject *pNativeRichEditBox)
{
    return TextControlHelper::QueueUpdateSelectionFlyoutVisibility<RichEditBox>(pNativeRichEditBox);
}

_Check_return_ HRESULT RichEditBox::UpdateSelectionFlyoutVisibility()
{
    return TextControlHelper::UpdateSelectionFlyoutVisibility<CRichEditBox>(this);
}