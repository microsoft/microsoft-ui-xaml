// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "HyperLinkButton.g.h"
#include "HyperlinkButtonAutomationPeer.g.h"
#include "ContentPresenter.g.h"
#include "TextBlock.g.h"
#include "Launcher.h"
#include <FrameworkTheming.h>
#include <Hyperlink.h>

using namespace DirectUI;

const WCHAR HyperlinkButton::c_ContentPresenterName[] = L"ContentPresenter";
const WCHAR HyperlinkButton::c_ContentPresenterLegacyName[] = L"Text";

// Initializes a new instance of the HyperlinkButton class.
HyperlinkButton::HyperlinkButton()
{
}

// Deconstructor
HyperlinkButton::~HyperlinkButton()
{
}

_Check_return_ HRESULT
HyperlinkButton::Initialize()
{
    IFC_RETURN(HyperlinkButtonGenerated::Initialize());
    IFC_RETURN(static_cast<CFrameworkElement*>(GetHandle())->SetCursor(MouseCursorHand));

    return S_OK;
}

_Check_return_ HRESULT
HyperlinkButton::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(HyperlinkButtonGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::ContentControl_Content:
        case KnownPropertyIndex::ContentControl_ContentTemplate:
            // We need to call InvalidateMeasure() to cover the case
            // where the new content has the same size as the old content,
            // to make sure MeasureOverride() is called again and the
            // text content is underlined.
            IFC_RETURN(InvalidateMeasure());
    }

    return S_OK;
}

// Change to the correct visual state.
_Check_return_ HRESULT HyperlinkButton::ChangeVisualState(
    // true to use transitions when updating the visual state, false
    // to snap directly to the new visual state.
    _In_ bool bUseTransitions)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsEnabled = FALSE;
    BOOLEAN bIsPressed = FALSE;
    BOOLEAN bIsPointerOver = FALSE;
    BOOLEAN bIgnored = FALSE;
    xaml::FocusState focusState = xaml::FocusState_Unfocused;

    IFC(get_IsEnabled(&bIsEnabled));
    IFC(get_IsPressed(&bIsPressed));
    IFC(get_IsPointerOver(&bIsPointerOver));
    IFC(get_FocusState(&focusState));

    // Update the Interaction state group
    if (!bIsEnabled)
    {
        IFC(GoToState(bUseTransitions, L"Disabled", &bIgnored));
    }
    else if (bIsPressed)
    {
        IFC(GoToState(bUseTransitions, L"Pressed", &bIgnored));
    }
    else if (bIsPointerOver)
    {
        IFC(GoToState(bUseTransitions, L"PointerOver", &bIgnored));
    }
    else
    {
        IFC(GoToState(bUseTransitions, L"Normal", &bIgnored));
    }

    // Update the Focus group
    if (xaml::FocusState_Unfocused != focusState && bIsEnabled)
    {
        if (xaml::FocusState_Pointer == focusState) 
        {
            IFC(GoToState(bUseTransitions, L"PointerFocused", &bIgnored));
        }
        else
        {
            IFC(GoToState(bUseTransitions, L"Focused", &bIgnored));
        }
    }
    else
    {
        IFC(GoToState(bUseTransitions, L"Unfocused", &bIgnored));
    }

Cleanup:
    RRETURN(hr);
}

// Create HyperlinkButtonAutomationPeer to represent the HyperlinkButton.
IFACEMETHODIMP HyperlinkButton::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IHyperlinkButtonAutomationPeer> spHyperlinkButtonAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IHyperlinkButtonAutomationPeerFactory> spHyperlinkButtonAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;
    
    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::HyperlinkButtonAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spHyperlinkButtonAPFactory));

    IFC(spHyperlinkButtonAPFactory.Cast<HyperlinkButtonAutomationPeerFactory>()->CreateInstanceWithOwner(this, 
        NULL, 
        &spInner, 
        &spHyperlinkButtonAutomationPeer));
    IFC(spHyperlinkButtonAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP HyperlinkButton::MeasureOverride(_In_ wf::Size availableSize, _Out_ wf::Size* returnValue)
{
    IFC_RETURN(HyperlinkButtonGenerated::MeasureOverride(availableSize, returnValue));

    // Note that this gets correctly invoked when the theme changes.
    IFC_RETURN(UpdateContentPresenterTextUnderline());

    // We don't want to override the foreground when BackPlate is active to allow hyperlink color changes.
    IFC_RETURN(SetHyperlinkForegroundOverrideForBackPlate());

    return S_OK;
}

// Raise the Click routed event. Navigates to URI if needed.
_Check_return_ HRESULT HyperlinkButton::OnClick()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;
    BOOLEAN bAutomationListener = FALSE;
    ctl::ComPtr<wf::IUriRuntimeClass> spUri;

    IFC(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_InvokePatternOnInvoked, &bAutomationListener));
    if (bAutomationListener)
    {
        IFC(GetOrCreateAutomationPeer(&spAutomationPeer));
        if(spAutomationPeer)
        {
            IFC(spAutomationPeer->RaiseAutomationEvent(xaml_automation_peers::AutomationEvents_InvokePatternOnInvoked));
        }
    }

    IFC(HyperlinkButtonGenerated::OnClick());

    // If NavigateUri is set, launch it in external app.
    IFC(get_NavigateUri(&spUri));
    if (spUri.Get())
    {
        IFC(Launcher::TryInvokeLauncher(spUri.Get()));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
HyperlinkButton::UpdateContentPresenterTextUnderline()
{
    ctl::ComPtr<xaml::IDependencyObject> contentPresenterPart;
    DependencyObject* contentPresenterPartDO = NULL;
    CContentPresenter* contentPresenter = NULL;

    IFC_RETURN(GetTemplateChild(wrl_wrappers::HStringReference(c_ContentPresenterName).Get(), &contentPresenterPart));

    if (contentPresenterPart)
    {
        contentPresenterPartDO = static_cast<DependencyObject*>(contentPresenterPart.Get());
        if (ctl::is<xaml_controls::IContentPresenter>(contentPresenterPartDO))
        {
            contentPresenter = static_cast<CContentPresenter*>(static_cast<ContentPresenter*>(contentPresenterPartDO)->GetHandle());
        }

        // If the ContentPresenter in the default template is present, and the TextBlock is one which is
        // generated in ContentPresenter code behind as default, only then do we proceed with the underline.
        if (contentPresenter && contentPresenter->IsUsingDefaultTemplate())
        {
            ctl::ComPtr<xaml::IUIElement> contentTemplateRootAsIUIE;
            ctl::ComPtr<xaml_controls::ITextBlock> contentTemplateRootAsITextBlock;

            IFC_RETURN(this->get_ContentTemplateRoot(&contentTemplateRootAsIUIE));
            contentTemplateRootAsITextBlock = contentTemplateRootAsIUIE.AsOrNull<xaml_controls::ITextBlock>();
            if (contentTemplateRootAsITextBlock)
            {
                // In 21H1 and later, when the HyperlinkUnderlineVisible resource is set to False, the underline is only visible in HighContrast mode.
                CCoreServices* core = static_cast<CCoreServices*>(DXamlCore::GetCurrent()->GetHandle());
                bool underlineVisibleResourceDirective = CHyperlink::UnderlineVisibleResourceDirective(core);
                bool underline = underlineVisibleResourceDirective || core->GetFrameworkTheming()->HasHighContrastTheme();

                IFC_RETURN(contentTemplateRootAsITextBlock.Cast<TextBlock>()->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_TextDecorations, underline ? DirectUI::TextDecorations::Underline : DirectUI::TextDecorations::None));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
HyperlinkButton::SetHyperlinkForegroundOverrideForBackPlate()
{
    ctl::ComPtr<xaml::IDependencyObject> contentPresenterPart;

    IFC_RETURN(GetTemplateChild(wrl_wrappers::HStringReference(c_ContentPresenterName).Get(), &contentPresenterPart));

    // If HyperLinkButton is not using the current template try to get the ContentPresenter by its name in TextBlockButtonStyle.
    // TextBlockButtonStyle isn't normally used with HyperlinkButton. Today it is sometimes used to remove the underline on the HyperlinkButton.
    // The style originally existed just to support some of the VS templates that shipped with Win8.
    if (!contentPresenterPart)
    {
        IFC_RETURN(GetTemplateChild(wrl_wrappers::HStringReference(c_ContentPresenterLegacyName).Get(), &contentPresenterPart));
    }

    if (contentPresenterPart)
    {
        DependencyObject* contentPresenterPartDO = static_cast<DependencyObject*>(contentPresenterPart.Get());

        if (ctl::is<xaml_controls::IContentPresenter>(contentPresenterPartDO))
        {
            CContentPresenter* contentPresenter = static_cast<CContentPresenter*>(contentPresenterPartDO->GetHandle());

            if (contentPresenter)
            {
                SetHyperlinkForegroundOverrideForBackPlateRecursive(contentPresenter);
            }
        }
    }

    return S_OK;
}

void HyperlinkButton::SetHyperlinkForegroundOverrideForBackPlateRecursive(_In_ CUIElement* pElement)
{
    auto pChildren = pElement->GetChildren();
    if (pChildren)
    {
        for (auto& child : *pChildren)
        {
            auto childUIE = checked_cast<CUIElement>(child);
            if (childUIE != nullptr)
            {
                auto childTextBlock = do_pointer_cast<CTextBlock>(childUIE);
                if (childTextBlock)
                {
                    childTextBlock->SetUseHyperlinkForegroundOnBackPlate(true);
                }
                else
                {
                    SetHyperlinkForegroundOverrideForBackPlateRecursive(childUIE);
                }
            }
        }
    }
}