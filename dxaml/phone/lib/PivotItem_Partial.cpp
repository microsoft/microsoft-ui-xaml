// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "PivotItem_Partial.h"
#include "PivotItemAutomationPeer_Partial.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{
    PivotItem::PivotItem()
        : m_isContentVisibilityPending(false)
        , m_isKeyboardFocusPending(false)
        , m_pendingContentVisibility(xaml::Visibility_Visible)
        , m_lastMeasureAvailableSize()
        , m_pivotItemAccessKeyInvokedToken()
    {}

    _Check_return_ HRESULT
    PivotItem::InitializeImpl(_In_opt_ IInspectable* outer)
    {
        wrl::ComPtr<xaml_controls::IContentControlFactory> spInnerFactory;
        wrl::ComPtr<xaml_controls::IContentControl> spDelegatingInnerInstance;
        wrl::ComPtr<IInspectable> spNonDelegatingInnerInspectable;
        IInspectable* aggregateOuter = outer ? outer : static_cast<IPivotItem*>(this);

        IFC_RETURN(PivotItemGenerated::InitializeImpl());

        IFC_RETURN(wf::GetActivationFactory(
               wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_ContentControl).Get(),
               &spInnerFactory));

        IFC_RETURN(spInnerFactory->CreateInstance(
               aggregateOuter,
               &spNonDelegatingInnerInspectable,
               &spDelegatingInnerInstance));

        IFC_RETURN(SetComposableBasePointers(
               spNonDelegatingInnerInspectable.Get(),
               spInnerFactory.Get()));

        // Care must be taken with any initialization after this point, as the outer object is not
        // finished fully initialized and any IInspectable operations on spDelegatingInner will
        // be delegated to the outer.

        IFC_RETURN(Private::SetDefaultStyleKey(
               spNonDelegatingInnerInspectable.Get(),
               L"Microsoft.UI.Xaml.Controls.PivotItem"));

        return S_OK;
    }

    _Check_return_ HRESULT PivotItem::MeasureOverrideImpl(
        _In_ wf::Size availableSize,
        _Out_ wf::Size* desiredSize) /*override*/
    {
        IFC_RETURN(PivotItemGenerated::MeasureOverrideImpl(availableSize, desiredSize));

        if (m_isKeyboardFocusPending)
        {
            IFC_RETURN(SetKeyboardFocus(false /* postponeUntilNextMeasure */));
        }

        m_lastMeasureAvailableSize = availableSize;
        return S_OK;
    }

    _Check_return_ HRESULT PivotItem::OnPivotItemAccessKeyInvoked(
        _In_ IUIElement* sender,
        _In_ xaml_input::IAccessKeyInvokedEventArgs* args)
    {
        UNREFERENCED_PARAMETER(sender);

        bool isHandled = false;
        IFC_RETURN(OnProgramaticHeaderItemTapped(&isHandled));
        if (isHandled)
        {
            IFC_RETURN(args->put_Handled(true));
        }
        return S_OK;
    }

    _Check_return_ HRESULT PivotItem::OnKeyboardAcceleratorInvokedImpl(
        _In_ xaml_input::IKeyboardAcceleratorInvokedEventArgs* args)
    {
        bool isHandled = false;
        IFC_RETURN(OnProgramaticHeaderItemTapped(&isHandled));
        if (isHandled)
        {
            IFC_RETURN(args->put_Handled(true));
        }
        return S_OK;
    }

    _Check_return_ HRESULT PivotItem::OnProgramaticHeaderItemTapped(bool *isHandled)
    {
        if (m_wpParent)
        {
            wrl::ComPtr<xaml_controls::IPivot> spIParent;
            IFC_RETURN(m_wpParent.As(&spIParent));
            //Get the index of pivot item
            wrl::ComPtr<xaml_controls::Pivot> spParent = static_cast<Pivot*>(spIParent.Get());
            INT newIdx = -1;
            BOOLEAN isValid = TRUE;
            IFC_RETURN(spParent->ValidateSelectedItem(static_cast<IPivotItem*>(this), &newIdx, &isValid));

            //If index is valid, call OnHeaderItemTapped to re-render the Pivot
            if (isValid && newIdx >= 0)
            {
                IFC_RETURN(spParent->OnHeaderItemTapped(newIdx, false /* shouldPlaySound */));
                *isHandled = true;
            }
        }

        return S_OK;
    }

    _Check_return_ HRESULT
    PivotItem::OnApplyTemplateImpl()
    {
        if(m_isContentVisibilityPending)
        {
            IFC_RETURN(SetContentVisibility(m_pendingContentVisibility));
            m_isContentVisibilityPending = false;
        }

        // Add an event handler for AccessKeyInvoked event on pivot items.
        // This event is automatically removed when the
        // UIElement's destructor is called.
        wrl::ComPtr<xaml::IUIElement> thisAsUE;
        IFC_RETURN(QueryInterface(__uuidof(xaml::IUIElement), &thisAsUE));

        if (m_pivotItemAccessKeyInvokedToken.value == 0)
        {
            IFC_RETURN(thisAsUE->add_AccessKeyInvoked(
                wrl::Callback<wf::ITypedEventHandler<UIElement*, xaml_input::AccessKeyInvokedEventArgs*>>
                (this, &PivotItem::OnPivotItemAccessKeyInvoked).Get(),
                &m_pivotItemAccessKeyInvokedToken));
        }

        return S_OK;
    }

    _Check_return_ HRESULT
    PivotItem::OnCreateAutomationPeerImpl(
        _Outptr_ xaml::Automation::Peers::IAutomationPeer **returnValue)
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<xaml_controls::PivotItem> spThis(this);
        wrl::ComPtr<xaml_controls::IPivotItem> spThisAsIPivotItem;
        wrl::ComPtr<xaml_automation_peers::PivotItemAutomationPeer> spPivotItemAutomationPeer;

        IFC(spThis.As(&spThisAsIPivotItem));
        IFC(wrl::MakeAndInitialize<xaml_automation_peers::PivotItemAutomationPeer>
                (&spPivotItemAutomationPeer, spThisAsIPivotItem.Get()));

        IFC(spPivotItemAutomationPeer.CopyTo(returnValue));

    Cleanup:
        RRETURN(hr);
    }

    void
    PivotItem::SetParent(
        _In_ wrl::ComPtr<xaml_controls::IPivot> spParent)
    {
        if (nullptr != spParent)
        {
            spParent.AsWeak(&m_wpParent);
        }
    }

    wrl::WeakRef
    PivotItem::GetParent()
    {
        return m_wpParent;
    }

    // Adds the element to the internal list of slide-in elements.
    _Check_return_ HRESULT
    PivotItem::RegisterSlideInElementNoRef(
        _In_ xaml::IFrameworkElement* pElement)
    {
        HRESULT hr = S_OK;

        // We will unregister the element if it gets removed from the visual tree.
        // So this list will contain elements that are always available for slide-in.
        ASSERT(pElement);
        m_slideInElementsNoRef.push_back(pElement);

        RRETURN(hr); //RRETURN_REMOVAL
    }

    // Removes the element from the internal list of slide-in elements.
    _Check_return_ HRESULT
    PivotItem::UnregisterSlideInElementNoRef(
        _In_ xaml::IFrameworkElement* pElement)
    {
        HRESULT hr = S_OK;

        m_slideInElementsNoRef.remove(pElement);

        RRETURN(hr); //RRETURN_REMOVAL
    }

    _Check_return_ HRESULT
    PivotItem::GetContentVisibility(
        _Out_ xaml::Visibility* visibility)
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<xaml::IUIElement> spChild;

        IFC(GetFirstChild(&spChild));

        if(spChild)
        {
            IFC(spChild->get_Visibility(visibility));
        }
        else
        {
            *visibility = xaml::Visibility_Collapsed;
        }

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT
    PivotItem::SetContentVisibility(
        _In_ xaml::Visibility visibility)
    {
        wrl::ComPtr<xaml::IUIElement> spChild;

        IFC_RETURN(GetFirstChild(&spChild));

        if (spChild)
        {
            IFC_RETURN(spChild->put_Visibility(visibility));
        }
        else
        {
            m_isContentVisibilityPending = true;
            m_pendingContentVisibility = visibility;
        }

        return S_OK;
    }

    _Check_return_ HRESULT
    PivotItem::TrySetContentIsEnabled(
        _In_ bool isEnabled, _Out_ BOOLEAN* successful)
    {
        *successful = FALSE;

        wrl::ComPtr<xaml::IUIElement> spChild;

        IFC_RETURN(GetFirstChild(&spChild));

        if (!m_spUIElementStaticsPrivate)
        {
            IFCFAILFAST(wf::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_UIElement).Get(),
                &m_spUIElementStaticsPrivate));
        }

        if (spChild)
        {
            //By default spChild is a grid, which is not a control so we have to go through UIElement's internal IsEnabled setter
            IFC_RETURN(m_spUIElementStaticsPrivate->InternalPutIsEnabled(spChild.Get(), isEnabled));
            *successful = TRUE;
        }

        return S_OK;
    }

    _Check_return_ HRESULT
    PivotItem::RealizeContent()
    {
        wrl::ComPtr<xaml_controls::IContentControl> thisAsContentControl;
        wrl::ComPtr<xaml::IUIElement> contentTemplateRoot;

        IFC_RETURN(QueryInterface(__uuidof(xaml_controls::IContentControl), &thisAsContentControl));
        IFC_RETURN(thisAsContentControl->get_ContentTemplateRoot(&contentTemplateRoot))

        if (!contentTemplateRoot)
        {
            wrl::ComPtr<xaml::IUIElement> child;

            // If we don't yet have a content template root, then we'll measure the child
            // (if we have one) in order to realize our item template.
            // Since this isn't part of any actual layout pass, this won't affect layout -
            // we'll re-measure with our actual bounds once the child is actually added
            // to the visual tree before anything is displayed.
            IFC_RETURN(GetFirstChild(&child));

            if (child)
            {
                IFC_RETURN(child->Measure(m_lastMeasureAvailableSize));
            }
        }

        return S_OK;
    }

    _Check_return_ HRESULT
    PivotItem::GetFirstChild(
        _Outptr_result_maybenull_ IUIElement** ppFirstChild)
    {
        HRESULT hr = S_OK;
        INT childrenCount = 0;
        wrl::ComPtr<xaml::IDependencyObject> spThisAsDO;
        wrl::ComPtr<xaml_media::IVisualTreeHelperStatics> spVisualTreeHelperStatics;

        *ppFirstChild = nullptr;
        IFC(QueryInterface(
            __uuidof(xaml::IDependencyObject),
            &spThisAsDO));

        IFC(wf::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_VisualTreeHelper).Get(),
                &spVisualTreeHelperStatics));

        IFC(spVisualTreeHelperStatics->GetChildrenCount(spThisAsDO.Get(), &childrenCount));

        if(childrenCount > 0)
        {
            wrl::ComPtr<xaml::IDependencyObject> spChildAsDO;

            ASSERT(childrenCount == 1);

            IFC(spVisualTreeHelperStatics->GetChild(spThisAsDO.Get(), 0 /* childIndex */, &spChildAsDO));
            IFC(spChildAsDO.CopyTo(ppFirstChild));
        }

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT PivotItem::SetKeyboardFocus(_In_ bool postponeUntilNextMeasure)
    {
        m_isKeyboardFocusPending = postponeUntilNextMeasure;

        if (postponeUntilNextMeasure)
        {
            wrl::ComPtr<xaml::IUIElement> thisAsUE;
            IFC_RETURN(QueryInterface(__uuidof(xaml::IUIElement), &thisAsUE));
            IFC_RETURN(thisAsUE->InvalidateMeasure());
        }
        else
        {
            BOOLEAN didFocus;
            wrl::ComPtr<xaml::IUIElement> thisAsUIE;
            IFC_RETURN(QueryInterface(__uuidof(xaml::IUIElement), &thisAsUIE));
            IFC_RETURN(thisAsUIE->Focus(xaml::FocusState_Keyboard, &didFocus));
        }

        return S_OK;
    }

    _Check_return_ HRESULT PivotItem::HasFocusedElement(_Out_ bool* hasFocusedElement)
    {
        wrl::ComPtr<IInspectable> focusedElement;

        *hasFocusedElement = false;

        wrl::ComPtr<xaml::IUIElement> thisAsUIE;
        IFC_RETURN(this->QueryInterface(IID_PPV_ARGS(&thisAsUIE)));

        wrl::ComPtr<xaml::IXamlRoot> xamlRoot;
        IFC_RETURN(thisAsUIE->get_XamlRoot(&xamlRoot));

        if (xamlRoot)
        {
            wrl::ComPtr<xaml_input::IFocusManagerStatics> spFocusManager;
            IFC_RETURN(wf::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Input_FocusManager).Get(),
                &spFocusManager));

            IFC_RETURN(spFocusManager->GetFocusedElementWithRoot(xamlRoot.Get(), &focusedElement));

            if (focusedElement)
            {
                wrl::ComPtr<xaml::IDependencyObject> focusedDO;

                IFC_RETURN(focusedElement.As(&focusedDO));
                IFC_RETURN(HasElement(focusedDO.Get(), hasFocusedElement));
            }
        }

        return S_OK;
    }

    _Check_return_ HRESULT PivotItem::HasElement(_In_ xaml::IDependencyObject* element, _Out_ bool* hasElement)
    {
        wrl::ComPtr<xaml::IUIElement> child;
        wrl::ComPtr<xaml::IDependencyObject> childAsDO;
        wrl::ComPtr<xaml::IDependencyObject> currentDO(element);
        wrl::ComPtr<xaml_media::IVisualTreeHelperStatics> vthStatics;
        bool isFound = false;

        *hasElement = false;

        IFC_RETURN(GetFirstChild(&child));

        if (!child)
        {
            return S_OK;
        }

        IFC_RETURN(child.As(&childAsDO));

        if (!childAsDO)
        {
            return S_OK;
        }

        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_VisualTreeHelper).Get(),
            &vthStatics));

        while (currentDO && !isFound)
        {
            if (currentDO == childAsDO)
            {
                isFound = true;
            }
            else
            {
                wrl::ComPtr<xaml::IDependencyObject> parent;
                IFC_RETURN(vthStatics->GetParent(currentDO.Get(), &parent));
                currentDO = std::move(parent);
            }
        }

        *hasElement = isFound;

        return S_OK;
    }

} } } } XAML_ABI_NAMESPACE_END