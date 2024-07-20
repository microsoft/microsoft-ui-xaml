// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "VisualTreeHelper.h"
#include "AutomationProperties.h"
#include "TextBoxPlaceholderTextHelper.h"
#include "ContentPresenter.g.h"
#include "ContentControl.g.h"
#include "TextBlock.g.h"
#include "TextBoxBase.h"

using namespace DirectUI;

 _Check_return_ HRESULT GetTextBlockFromOwner(_In_ ctl::ComPtr<xaml::IUIElement>& spOwner,
                                              _In_ bool considerCollapsedTextBlocks,
                                              _Outptr_ xaml::IDependencyObject** textBlock)
{
     *textBlock = nullptr;

    ctl::ComPtr<xaml::IDependencyObject> spTextBlock;
    ctl::ComPtr<xaml::IUIElement> spPlaceholderTextPresenter;
    IFC_RETURN(spOwner.Cast<Control>()->GetTemplatePart<xaml::IUIElement>(STR_LEN_PAIR(L"PlaceholderTextContentPresenter"), spPlaceholderTextPresenter.ReleaseAndGetAddressOf()));

    if (!spPlaceholderTextPresenter.Get())
    {
        // No-op if the PlaceholderTextContentPresenter template part does not exist
        return S_OK;
    }

    if (!considerCollapsedTextBlocks)
    {
        xaml::Visibility visibility = xaml::Visibility_Collapsed;
        IFC_RETURN(spPlaceholderTextPresenter->get_Visibility(&visibility));

        if (visibility == xaml::Visibility_Collapsed) { return S_OK; }
    }

    //If the placeholder text is a TextBlock instead of a ContentControl, return that textblock
    if (spPlaceholderTextPresenter.AsOrNull<xaml_controls::ITextBlock>())
    {
        spPlaceholderTextPresenter.CopyTo(textBlock);
        return S_OK;
    }

    int childrenCount = 0;
    IFC_RETURN(VisualTreeHelper::GetChildrenCountStatic(spPlaceholderTextPresenter.Cast<UIElement>(), &childrenCount));

    for(int childIndex = 0; childIndex < childrenCount; childIndex++)
    {
        IFC_RETURN(VisualTreeHelper::GetChildStatic(spPlaceholderTextPresenter.Cast<UIElement>(), childIndex, &spTextBlock));


        if (spTextBlock.Get() && spTextBlock.Cast<UIElement>()->GetHandle()->OfTypeByIndex<KnownTypeIndex::ContentPresenter>())
        {
            // In this scenario, the placeholder text is a ContentControl instead of a ContentPresenter.
            // If there is actual placeholder text, the ContentControl's first child will be the presenter.
            // As a result meaning we will need to call GetChildStatic again, looking for the '0th' child.
            // If there is no placeholder text, the content presenter will have no children.
            const int presenterIndex = 0;
            int contentPresenterChildrenCount = 0;

            IFC_RETURN(VisualTreeHelper::GetChildrenCountStatic(spTextBlock.Cast<UIElement>(), &contentPresenterChildrenCount));
            if (contentPresenterChildrenCount == 0)
            {
                return S_OK;
            }

            ctl::ComPtr<xaml::IDependencyObject> childAsDO;
            IFC_RETURN(VisualTreeHelper::GetChildStatic(spTextBlock.Cast<DependencyObject>(), presenterIndex, &childAsDO));
            spTextBlock = childAsDO;
        }

        if (spTextBlock.AsOrNull<xaml_controls::ITextBlock>() != nullptr)
        {
            spTextBlock.CopyTo(textBlock);
            return S_OK;
        }
    }

    return S_OK;
}


_Check_return_ HRESULT TextBoxPlaceholderTextHelper::SetupPlaceholderTextBlockDescribedBy(
    _In_ ctl::ComPtr<xaml::IUIElement> spOwner)
{
    ctl::ComPtr<xaml::IDependencyObject> textBlock;
    IFC_RETURN(GetTextBlockFromOwner(spOwner, false /*considerCollapsedTextBlocks*/, &textBlock));

    if (textBlock.Get())
    {
        //If the placeholder text is not visible, do not add it to the DescribedBy list
        xaml::Visibility placeholderTextVisibility;
        IFC_RETURN(textBlock.Cast<UIElement>()->get_Visibility(&placeholderTextVisibility));
        if (placeholderTextVisibility == xaml::Visibility::Visibility_Collapsed)
        {
            return S_OK;
        }

        ctl::ComPtr<wfc::IVector<xaml::DependencyObject*>> describedByList;
        IFC_RETURN(DirectUI::AutomationProperties::GetDescribedByStatic(spOwner.Cast<UIElement>(), &describedByList));

        unsigned int index = 0;
        boolean found;

        IFC_RETURN(describedByList->IndexOf(textBlock.Get(), &index, &found));

        if (found) { return S_OK; }

        IFC_RETURN(describedByList->Append(textBlock.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT TextBoxPlaceholderTextHelper::UpdatePlaceholderTextPresenterVisibility(
    _In_ xaml::IUIElement* textBox,
    _In_ xaml::IUIElement* placeholderTextAsUIElement,
    _In_ bool isEnabled)
{
    if (isEnabled && ShouldMakePlaceholderTextVisible(placeholderTextAsUIElement, textBox))
    {
        IFC_RETURN(placeholderTextAsUIElement->put_Visibility(xaml::Visibility_Visible));

        wrl::ComPtr<xaml_automation::IAutomationPropertiesStatics> spAutomationPropertiesStatic;

        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_AutomationProperties).Get(),
            &spAutomationPropertiesStatic));

        wrl::ComPtr<xaml::IDependencyObject> spPlaceholderTextAsDO;
        IFC_RETURN(placeholderTextAsUIElement->QueryInterface(IID_PPV_ARGS(&spPlaceholderTextAsDO)));

        // To ensure that the placeholder text cannot be focused through narrator, we should set the AccessibilityView to Raw
        IFC_RETURN(spAutomationPropertiesStatic->SetAccessibilityView(spPlaceholderTextAsDO.Get(), xaml_automation_peers::AccessibilityView::AccessibilityView_Raw));
    }
    else
    {
        IFC_RETURN(ClearPlaceholderTextBlockDescribedBy(textBox));
        IFC_RETURN(placeholderTextAsUIElement->put_Visibility(xaml::Visibility_Collapsed));
    }

    return S_OK;
}

_Check_return_ HRESULT TextBoxPlaceholderTextHelper::ClearPlaceholderTextBlockDescribedBy(
    _In_ xaml::IUIElement* textBox)
{
    ctl::ComPtr<xaml::IUIElement> spOwner(textBox);

    ctl::ComPtr<xaml::IDependencyObject> textBlock;
    IFC_RETURN(GetTextBlockFromOwner(spOwner, true /*considerCollapsedTextBlocks*/, &textBlock));

    // ShouldCollapsePlaceholderText returns true only if the placeholder text actually has content and
    // the DescribedBy list is not empty.
    if (textBlock.Get() && ShouldCollapsePlaceholderText(textBlock.AsOrNull<xaml::IUIElement>().Get(), textBox))
    {
        ctl::ComPtr<xaml::IDependencyObject> placeholderTextPresenterAsDO;
        textBlock.As(&placeholderTextPresenterAsDO);

        ctl::ComPtr<wfc::IVector<xaml::DependencyObject*>> describedByList;
        //GetDescribedByStatic creates a list if none exists.
        IFC_RETURN(DirectUI::AutomationProperties::GetDescribedByStatic(spOwner.Cast<UIElement>(), &describedByList));

        unsigned int index = 0;
        boolean found;

        IFC_RETURN(describedByList->IndexOf(textBlock.Get(), &index, &found));

        if (found)
        {
            IFC_RETURN(describedByList->RemoveAt(index));
        }
    }

    return S_OK;
}

bool TextBoxPlaceholderTextHelper::ShouldMakePlaceholderTextVisible(
    _In_opt_ xaml::IUIElement* placeholderTextAsUIElement,
    _In_opt_ xaml::IUIElement* textControlAsUIElement)
{
    if (!placeholderTextAsUIElement || !textControlAsUIElement)
    {
        return false;
    }

    ctl::ComPtr<xaml::IUIElement> spTextControlAsUIElement = textControlAsUIElement;
    CTextBoxBase* textControl = do_pointer_cast<CTextBoxBase>(spTextControlAsUIElement.Cast<UIElement>()->GetHandle());

    //If the containing text control is not empty, we should never make placeholder text visible.
    if (!textControl->IsEmpty())
    {
        return false;
    }

    ctl::ComPtr<xaml::IUIElement> spPlaceholderTextAsUIElement = placeholderTextAsUIElement;
    ctl::ComPtr<xaml_controls::ITextBlock> spPlaceholderTextAsTextBlock =
        spPlaceholderTextAsUIElement.AsOrNull<xaml_controls::ITextBlock>();

    // If the placeholder text is a TextBlock, not a ContentControl or ContentPresenter,
    // we only want to set visibility to visible if placeholder text actually exists.
    if (spPlaceholderTextAsTextBlock.Get())
    {
        HSTRING text;
        IFCFAILFAST(spPlaceholderTextAsTextBlock.Cast<TextBlock>()->get_Text(&text));

        return (text != nullptr);
    }

    return true;
}

bool TextBoxPlaceholderTextHelper::ShouldCollapsePlaceholderText(
    _In_opt_ xaml::IUIElement* placeholderTextAsUIElement,
    _In_opt_ xaml::IUIElement* textControlAsUIElement)
{
    if (!placeholderTextAsUIElement || !textControlAsUIElement)
    {
        return false;
    }

    //If the DescribedBy list does not exist, do not change placeholder visibility
    ctl::ComPtr<xaml::IUIElement> spTextControlAsUIElement(textControlAsUIElement);
    ctl::ComPtr<wfc::IVector<xaml::DependencyObject*>> describedByList;

    if (FAILED(spTextControlAsUIElement.Cast<UIElement>()->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_DescribedBy, describedByList.GetAddressOf())) ||
        !describedByList)
    {
        return false;
    }

    //If the containing text control is not empty, we should collapse placeholder text.
    CTextBoxBase* textControl = do_pointer_cast<CTextBoxBase>(spTextControlAsUIElement.Cast<UIElement>()->GetHandle());
    if (textControl->IsEmpty())
    {

        ctl::ComPtr<xaml::IUIElement> spPlaceholderTextAsUIElement = placeholderTextAsUIElement;
        ctl::ComPtr<xaml_controls::ITextBlock> spPlaceholderTextAsTextBlock =
            spPlaceholderTextAsUIElement.AsOrNull<xaml_controls::ITextBlock>();

        // If the placeholder text is a TextBlock, not a ContentControl or ContentPresenter,
        // we only want to set visibility to collapsed if placeholder text actually exists.
        if (spPlaceholderTextAsTextBlock.Get())
        {
            HSTRING text;
            IFCFAILFAST(spPlaceholderTextAsTextBlock.Cast<TextBlock>()->get_Text(&text));

            return (text != nullptr);
        }
    }

    return true;
}
