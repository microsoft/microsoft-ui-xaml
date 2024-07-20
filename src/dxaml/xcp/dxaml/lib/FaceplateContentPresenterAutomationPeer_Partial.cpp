// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FaceplateContentPresenterAutomationPeer_partial.h"
#include "ItemsControl.g.h"
#include "ItemCollection.g.h"
#include "Selector.g.h"
#include "ContentPresenter.g.h"
#include "VisualTreeHelper.h"
#include "AutomationProperties.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// We are following ComboBoxItemAutomationPeer APIs here
IFACEMETHODIMP FaceplateContentPresenterAutomationPeer::GetClassNameCore(_Out_ HSTRING* pReturnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"ComboBoxItem")).CopyTo(pReturnValue));

Cleanup:
    RRETURN(hr);
}

// We are following ComboBoxItemAutomationPeer APIs here
IFACEMETHODIMP FaceplateContentPresenterAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    *pReturnValue = xaml_automation_peers::AutomationControlType_ListItem;
    RRETURN(S_OK);
}

// Faceplate gets used for selected Item in ComboBox that has not yet been expanded. In this case
// the actual container for Item doesn't exist and we need to fetch PositionInSet and SizeOfSet directly.
_Check_return_ HRESULT FaceplateContentPresenterAutomationPeer::GetPositionInSetCoreImpl(_Out_ INT* pReturnValue)
{
    IFCPTR_RETURN(pReturnValue);
    *pReturnValue = -1;

    ctl::ComPtr<xaml::IUIElement> spFaceplateAsUie;
    ctl::ComPtr<IFrameworkElement> spFaceplateAsIFE;

    IFC_RETURN(get_Owner(&spFaceplateAsUie));
    spFaceplateAsIFE = spFaceplateAsUie.AsOrNull<IFrameworkElement>();

    if (spFaceplateAsIFE != nullptr)
    {
        ctl::ComPtr<DependencyObject> spTemplatedParent;
        ctl::ComPtr<ISelector> spTemplatedParentAsISelector;

        IFC_RETURN(spFaceplateAsIFE.Cast<FrameworkElement>()->get_TemplatedParent(&spTemplatedParent));
        spTemplatedParentAsISelector = spTemplatedParent.AsOrNull<ISelector>();

        if (spTemplatedParentAsISelector != nullptr)
        {
            IFC_RETURN(spTemplatedParentAsISelector.Cast<Selector>()->get_SelectedIndex(pReturnValue));

            if (*pReturnValue > -1)
            {
                *pReturnValue += 1; // Position is one more than index itself. -1 is returned when no selection is found.
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT FaceplateContentPresenterAutomationPeer::GetSizeOfSetCoreImpl(_Out_ INT* pReturnValue)
{
    IFCPTR_RETURN(pReturnValue);
    *pReturnValue = -1;

    ctl::ComPtr<xaml::IUIElement> spFaceplateAsUie;
    ctl::ComPtr<IFrameworkElement> spFaceplateAsIFE;

    IFC_RETURN(get_Owner(&spFaceplateAsUie));
    spFaceplateAsIFE = spFaceplateAsUie.AsOrNull<IFrameworkElement>();

    if (spFaceplateAsIFE != nullptr)
    {
        ctl::ComPtr<DependencyObject> spTemplatedParent;
        ctl::ComPtr<IItemsControl> spTemplatedParentAsIIC;

        IFC_RETURN(spFaceplateAsIFE.Cast<FrameworkElement>()->get_TemplatedParent(&spTemplatedParent));
        spTemplatedParentAsIIC = spTemplatedParent.AsOrNull<IItemsControl>();

        if (spTemplatedParentAsIIC != nullptr)
        {
            ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
            IFC_RETURN(spTemplatedParentAsIIC.Cast<ItemsControl>()->get_Items(&spItems));

            if (spItems != nullptr)
            {
                UINT count = 0;
                IFC_RETURN(spItems.Cast<ItemCollection>()->get_Size(&count));
                *pReturnValue = static_cast<INT>(count);
            }
        }
    }

    return S_OK;
}

IFACEMETHODIMP FaceplateContentPresenterAutomationPeer::GetNameCore(_Out_ HSTRING* returnValue)
{
    *returnValue = nullptr;

    ctl::ComPtr<xaml::IUIElement> contentPresenterAsUIE;

    wrl_wrappers::HString stringRepresentation;

    IFC_RETURN(get_Owner(&contentPresenterAsUIE));

    if (contentPresenterAsUIE)
    {
        // Check whether the ContentPresenter's content has an automation name
        INT childCount = 0;
        IFC_RETURN(VisualTreeHelper::GetChildrenCountStatic(contentPresenterAsUIE.Cast<UIElement>(), &childCount));
        if (childCount > 0)
        {
            ctl::ComPtr<IDependencyObject> contentPresenterContent;
            // The first child is the content of the ContentPresenter.
            IFC_RETURN(VisualTreeHelper::GetChildStatic(contentPresenterAsUIE.Cast<UIElement>(), 0, &contentPresenterContent));

            // Get the AutomationProperties.Name field of the Content if it exists.
            IFC_RETURN(AutomationProperties::GetNameStatic(contentPresenterContent.Cast<UIElement>(), stringRepresentation.ReleaseAndGetAddressOf()));
        }

        // Fallback: use GetPlainText on the ContentPresenter
        if (stringRepresentation.IsEmpty())
        {
            auto contentPresenterAsFE = contentPresenterAsUIE.AsOrNull<xaml::IFrameworkElement>();

            if (contentPresenterAsFE)
            {
                IFC_RETURN(contentPresenterAsFE.Cast<FrameworkElement>()->GetPlainText(stringRepresentation.ReleaseAndGetAddressOf()));
            }
        }

        // Fallback: use GetTextBlockText on the ContentPresenter
        if (stringRepresentation.IsEmpty())
        {
            ctl::ComPtr<ContentPresenter> contentPresenterAsCP = contentPresenterAsUIE.AsOrNull<ContentPresenter>();

            if (contentPresenterAsCP)
            {
                auto cp = do_pointer_cast<CContentPresenter>(contentPresenterAsCP->GetHandle());
                if (cp->HasDataBoundTextBlockText())
                {
                    // Make sure the databound ContentPresenter's TextBlock.Text is up-to-date before
                    // accessing its value since this FaceplateContentPresenterAutomationPeer::GetNameCore
                    // may be the result of an AutomationEvents_SelectionItemPatternOnElementSelected event
                    // that occurs before the DataContext change propagation.
                    BOOLEAN addedVisuals;
                    IFC_RETURN(cp->InvokeApplyTemplate(&addedVisuals));
                }
                IFC_RETURN(cp->GetTextBlockText(stringRepresentation.ReleaseAndGetAddressOf()));
            }
        }
    }

    IFC_RETURN(stringRepresentation.CopyTo(returnValue));

    return S_OK;
}
