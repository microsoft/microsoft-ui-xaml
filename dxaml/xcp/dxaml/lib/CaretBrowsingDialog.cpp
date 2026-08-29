// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ContentDialog.g.h"
#include "ContentDialogMetadata.h"
#include "CaretBrowsingDialog.h"
#include "TextBlock.g.h"
#include "WrlHelper.h"
#include "localizedResource.h"
#include "XamlRoot_Partial.h"
#include <CaretBrowsingGlobal.h>


using namespace DirectUI;
using namespace Microsoft::WRL::Wrappers;

namespace DirectUI
{

bool IsTextBlockControl(_In_ CDependencyObject* pDO)
{
    return (pDO->OfTypeByIndex<KnownTypeIndex::TextBlock>()
        || pDO->OfTypeByIndex<KnownTypeIndex::RichTextBlock>()
        || pDO->OfTypeByIndex<KnownTypeIndex::RichTextBlockOverflow>());
}
    
// Returns true if the given element or a descendent is a textblock with IsTextSelectionEnabled="true".
bool SearchForSelectableTextBlock(_In_ CUIElement* const pElement)
{
    if (IsTextBlockControl(pElement) && CTextCore::IsTextSelectionEnabled(pElement))
    {
        return true;
    }

    // Recurse over all children to check the subtree
    CUIElementCollection* pCollection = pElement->CanHaveChildren() ? pElement->GetChildren() : nullptr;
    if (!pCollection)
    {
        return false;
    }
    for (auto& pDOChild : *pCollection)
    {
        auto pChild = do_pointer_cast<CUIElement>(pDOChild);
        if (pChild && SearchForSelectableTextBlock(pChild))
        {
            return true;
        }
    }

    return false;
}
    
void PopCaretBrowsingContentDialog(_In_ IInspectable* xamlRootInsp)
{
    ctl::ComPtr<IInspectable> contentRootInsp(xamlRootInsp);

    // First, check if there is already an open ContentDialog. If there is, attempting to show
    // this dialog will fail with E_ASYNC_OPERATION_NOT_STARTED because ContentDialog only allows
    // one to be showing at a time per xaml root.  So if there is one already open, just bail -- the user will
    // need to close that dialog first and can then try again to enable/disable Caret Browsing

    ctl::ComPtr<ContentDialogMetadata> metadata;
    const auto xamlRoot = contentRootInsp.AsOrNull<XamlRoot>();
    IFCFAILFAST(xamlRoot->GetContentDialogMetadata(&metadata));
    
    bool hasOpenDialog = false;
    IGNOREHR(metadata->HasOpenDialog(nullptr, &hasOpenDialog));
    if (hasOpenDialog)
    {
        return; // a dialog is already showing, so we can't show our dialog now
    }

    // Don't show the dialog if there is no selectable text, since there is no
    // need for caret browsing in that scenario.
    ctl::ComPtr<xaml::IUIElement> rootElement;
    if (FAILED(xamlRoot->get_Content(&rootElement)) || !rootElement)
    {
        return;
    }
    bool hasSelectableText = SearchForSelectableTextBlock(rootElement.Cast<UIElement>()->GetHandle());
    if (!hasSelectableText)
    {
        return;
    }

    bool caretBrowsingEnabled = GetCaretBrowsingModeEnable();
    ctl::ComPtr<IInspectable> dialogInspectable;
    IFCFAILFAST(ActivationAPI::ActivateInstance(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::ContentDialog), &dialogInspectable));
    ctl::ComPtr<xaml_controls::IContentDialog> contentDialogInstance;
    IFCFAILFAST(dialogInspectable.As(&contentDialogInstance));

    // content
    CValue val;
    ctl::ComPtr<IInspectable> contentObject;
    ctl::ComPtr<xaml::IDependencyObject> pDO;
    const WCHAR* mystring;
    XUINT32 cStrXaml;
    const WCHAR turnOffString[] = (
        L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
        L"</StackPanel>"
        );
    const WCHAR turnOnString[] = (
        L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
        L"    <TextBlock x:Name='content' TextWrapping='WrapWholeWords'></TextBlock>"
        L"    <TextBlock></TextBlock>" // this empty textblock is used for vertical spacing
        L"    <CheckBox x:Name='notPopAgainCheckBox'/> "
        L"</StackPanel>"
        );
    if (caretBrowsingEnabled)
    {
        mystring = turnOffString;
        cStrXaml = ARRAYSIZE(turnOffString) - 1;  //CreateFromXaml expects a size that doesn't include the null terminator
    }
    else
    {
        mystring = turnOnString;
        cStrXaml = ARRAYSIZE(turnOnString) - 1;  //CreateFromXaml expects a size that doesn't include the null terminator
    }

    IFCFAILFAST(CoreImports::Host_CreateFromXaml(
        DXamlCore::GetCurrent()->GetHandle(),
        cStrXaml,
        mystring,
        /* bCreateNameScope */ TRUE,
        /* bRequireDefaultNamespace */ TRUE,
        /* bExpandTemplatesDuringParse */ FALSE,
        &val));

    IFCFAILFAST(CValueBoxer::UnboxObjectValue(&val, /* pTargetType */ NULL, &contentObject));

    // During the parse, the only thing keeping the peer for the root
    // object alive was the peer table. Now that we have another
    // reference to it ("pDO"), we can release on behalf of the parser.
    IFCFAILFAST(contentObject.As(&pDO));
    if (pDO)
    {
        static_cast<DependencyObject *>(pDO.Get())->UnpegNoRef();
    }

    ctl::ComPtr<xaml_controls::IContentControl> contentcontrol;
    IFCFAILFAST(contentDialogInstance.As(&contentcontrol));
    IFCFAILFAST(contentcontrol->put_Content(contentObject.Get()));

    ctl::ComPtr<xaml::IFrameworkElement> dialogRoot;
    IFCFAILFAST(contentObject.As(&dialogRoot));

    //checkbox with textblock, which is only in the "Turn on?" dialog
    ctl::ComPtr<xaml_primitives::IToggleButton> toggleButton;
    if (!caretBrowsingEnabled)
    {
        ctl::ComPtr<IInspectable> checkboxInspectable;
        IFCFAILFAST(dialogRoot->FindName(HStringReference(L"notPopAgainCheckBox").Get(), &checkboxInspectable));
        IFCFAILFAST(checkboxInspectable.As(&toggleButton));
        ctl::ComPtr<IInspectable> checkBoxTextBlockInspectable;
        IFCFAILFAST(ActivationAPI::ActivateInstance(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::TextBlock), &checkBoxTextBlockInspectable));
        ctl::ComPtr<xaml_controls::ITextBlock> checkBoxTextBlock;
        IFCFAILFAST(checkBoxTextBlockInspectable.As(&checkBoxTextBlock));
        HString checkboxStr;
        IFCFAILFAST(DXamlCore::GetCurrent()->GetLocalizedResourceString(TEXT_CARETBROWSING_DIALOG_CHECKBOX, checkboxStr.ReleaseAndGetAddressOf()));
        IFCFAILFAST(checkBoxTextBlock->put_Text(checkboxStr.Get()));
        ctl::ComPtr<xaml_controls::IContentControl> checkboxContentcontrol;
        IFCFAILFAST(checkboxInspectable.As(&checkboxContentcontrol));
        IFCFAILFAST(checkboxContentcontrol->put_Content(checkBoxTextBlockInspectable.Get()));
    }

    //title
    HString titleStr;
    if (caretBrowsingEnabled)
    {
        IFCFAILFAST(DXamlCore::GetCurrent()->GetLocalizedResourceString(TEXT_CARETBROWSING_DIALOG_TURNOFF_TITLE, titleStr.ReleaseAndGetAddressOf()));
    }
    else
    {
        IFCFAILFAST(DXamlCore::GetCurrent()->GetLocalizedResourceString(TEXT_CARETBROWSING_DIALOG_TURNON_TITLE, titleStr.ReleaseAndGetAddressOf()));
    }

    ctl::ComPtr<TextBlock> titleTextBlock;
    ctl::ComPtr<IInspectable> titleTextBlockInspectable;
    IFCFAILFAST(ctl::make(&titleTextBlock));
    IFCFAILFAST(titleTextBlock->put_Text(titleStr.Get()));
    IFCFAILFAST(titleTextBlock.As(&titleTextBlockInspectable));
    IFCFAILFAST(contentDialogInstance->put_Title(titleTextBlockInspectable.Get()));

    if (!caretBrowsingEnabled)
    {
        //content textblock
        ctl::ComPtr<IInspectable> contentTBInspectable;
        IFCFAILFAST(dialogRoot->FindName(HStringReference(L"content").Get(), &contentTBInspectable));
        ctl::ComPtr<xaml_controls::ITextBlock> textBlockContent;
        IFCFAILFAST(contentTBInspectable.As(&textBlockContent));
        HString contentStr;
        IFCFAILFAST(DXamlCore::GetCurrent()->GetLocalizedResourceString(TEXT_CARETBROWSING_DIALOG_CONTENT, contentStr.ReleaseAndGetAddressOf()));
        IFCFAILFAST(textBlockContent->put_Text(contentStr.Get()));
    }

    //button
    HString primaryButtonStr;
    HString secondaryButtonStr;
    IFCFAILFAST(DXamlCore::GetCurrent()->GetLocalizedResourceString(TEXT_CARETBROWSING_DIALOG_YES, primaryButtonStr.ReleaseAndGetAddressOf()));
    IFCFAILFAST(DXamlCore::GetCurrent()->GetLocalizedResourceString(TEXT_CARETBROWSING_DIALOG_NO, secondaryButtonStr.ReleaseAndGetAddressOf()));

    IFCFAILFAST(contentDialogInstance->put_PrimaryButtonText(primaryButtonStr.Get()));
    IFCFAILFAST(contentDialogInstance->put_SecondaryButtonText(secondaryButtonStr.Get()));

    EventRegistrationToken primaryButtonClickToken;
    auto primaryBtnCallback = WRLHelper::MakeCallback<wf::ITypedEventHandler<xaml_controls::ContentDialog*, xaml_controls::ContentDialogButtonClickEventArgs*>>(
        [toggleButton](xaml_controls::IContentDialog* sender, xaml_controls::IContentDialogButtonClickEventArgs* pEventArgs) -> HRESULT
    {
        if (GetCaretBrowsingModeEnable())
        {
            SetCaretBrowsingModeEnable(false);
        }
        else
        {
            SetCaretBrowsingModeEnable(true);
        }
        if (toggleButton) // only the "Turn on?" dialog has the checkbox
        {
            ctl::ComPtr<wf::IReference<bool>> isCheckedReference;
            IFCFAILFAST(toggleButton->get_IsChecked(&isCheckedReference));
            if (isCheckedReference)
            {
                BOOLEAN isChecked = false;
                IFCFAILFAST(isCheckedReference->get_Value(&isChecked));
                if (isChecked)
                {
                    SetCaretBrowsingDialogNotPopAgain(true);
                }
            }
        }
        return S_OK;
    });
    IFCFAILFAST(contentDialogInstance->add_PrimaryButtonClick(primaryBtnCallback.Get(), &primaryButtonClickToken));

    if (!caretBrowsingEnabled)
    {
        EventRegistrationToken secondaryButtonClickToken;
        auto secondaryBtnCallback = WRLHelper::MakeCallback<wf::ITypedEventHandler<xaml_controls::ContentDialog*, xaml_controls::ContentDialogButtonClickEventArgs*>>(
            [toggleButton](xaml_controls::IContentDialog* sender, xaml_controls::IContentDialogButtonClickEventArgs* pEventArgs) -> HRESULT
        {
            ctl::ComPtr<wf::IReference<bool>> isCheckedReference;
            IFCFAILFAST(toggleButton->get_IsChecked(&isCheckedReference));
            if (isCheckedReference)
            {
                BOOLEAN isChecked = false;
                IFCFAILFAST(isCheckedReference->get_Value(&isChecked));
                if (isChecked)
                {
                    // The user selected to *not* enable caret browsing, and to never be asked to
                    // enable again.  Permanently disable F7.
                    SetCaretBrowsingDialogNotPopAgain(true);
                    SetCaretBrowsingF7Disabled(true);
                }
            }
            return S_OK;
        });
        IFCFAILFAST(contentDialogInstance->add_SecondaryButtonClick(secondaryBtnCallback.Get(), &secondaryButtonClickToken));
    }

    ctl::ComPtr<xaml_controls::IContentDialog> contentDialogACR;
    IFCFAILFAST(contentDialogInstance.As(&contentDialogACR));

    ctl::ComPtr<xaml::IUIElement> uiElement;
    IFCFAILFAST(contentDialogACR.As(&uiElement));
    IFCFAILFAST(uiElement->put_XamlRoot(xamlRoot.Get()));

    ctl::ComPtr<wf::IAsyncOperation<xaml_controls::ContentDialogResult>> asyncOperationContentDialog;
    IFCFAILFAST(contentDialogInstance->ShowAsync(&asyncOperationContentDialog));
}

}
