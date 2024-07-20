// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Private
{
    class AutomationHelper
    {
    public:
        enum AutomationPropertyEnum
        {
            EmptyProperty,
            NameProperty,
            IsSelectedProperty,
            NotSupported
        };

        static
        _Check_return_ HRESULT RaiseEventIfListener(
            _In_ xaml::IUIElement* pUIElement,
            _In_ xaml::Automation::Peers::AutomationEvents eventId);

        static
        _Check_return_ HRESULT SetAutomationFocusIfListener(
            _In_ xaml::IUIElement* pUIElement);

        static
        _Check_return_ HRESULT RaisePropertyChanged(
            _In_ xaml::IUIElement* pUIElement,
            _In_ xaml::Automation::IAutomationProperty* pAutomationProperty,
            _In_ IInspectable* pOldValue,
            _In_ IInspectable* pNewValue,
            _In_ BOOLEAN checkIfListenerExists = FALSE);

        static
        _Check_return_ HRESULT RaisePropertyChangedIfListener(
            _In_ xaml::IUIElement* pUIElement,
            _In_ xaml::Automation::IAutomationProperty* pAutomationProperty,
            _In_ IInspectable* pOldValue,
            _In_ IInspectable* pNewValue);

        template<typename T>
        static
        _Check_return_ HRESULT RaisePropertyChanged(
            _In_ xaml::IUIElement* pUIElement,
            _In_ xaml::Automation::IAutomationProperty* pAutomationProperty,
            _In_ T oldValue,
            _In_ T newValue,
            _In_ BOOLEAN checkIfListenerExists = FALSE)
        {
            HRESULT hr = S_OK;
            wrl::ComPtr<IInspectable> spOldValue;
            wrl::ComPtr<IInspectable> spNewValue;

            IFC(Private::ValueBoxer::CreatePropertyValue<T>(oldValue, &spOldValue));
            IFC(Private::ValueBoxer::CreatePropertyValue<T>(newValue, &spNewValue));

            IFC(RaisePropertyChanged(
                pUIElement,
                pAutomationProperty,
                spOldValue.Get(),
                spNewValue.Get(),
                checkIfListenerExists));

        Cleanup:
            RRETURN(hr);
        }

        static
        _Check_return_ HRESULT RaiseTextEditTextChanged(
            _In_ xaml::IUIElement* pUIElement,
            _In_ xaml::Automation::AutomationTextEditChangeType pAutomationProperty,
            _In_ IInspectable* pPChangedData,
            _In_ BOOLEAN checkIfListenerExists = FALSE );

        static
        _Check_return_ HRESULT GetPlainText(
            _In_ IInspectable* pObject,
            _Out_ HSTRING* returnValue);

        static
        _Check_return_ HRESULT ConvertPropertyToEnum(
            _In_opt_ xaml::Automation::IAutomationProperty* pAutomationProperty,
            _Out_ AutomationPropertyEnum* pEnumProperty);

        static
        _Check_return_ HRESULT CreatePeerForElement(
            _In_ xaml::IUIElement* pUIElement,
            _Outptr_result_maybenull_ xaml::Automation::Peers::IAutomationPeer** ppReturnValue);

        static
        _Check_return_ HRESULT SetElementAutomationName(
            _In_ xaml::IDependencyObject* pDO,
            _In_ HSTRING string);

        static
        _Check_return_ HRESULT SetElementAutomationId(
            _In_ xaml::IDependencyObject* pDO,
            _In_ HSTRING string);

        static
        _Check_return_ HRESULT CopyAutomationProperties(
            _In_ xaml::IDependencyObject* pDOSource,
            _In_ xaml::IDependencyObject* pDOTarget);

        template <typename T>
        static
        _Check_return_ HRESULT GetOwnerAsInternalPtrNoRef(
            _In_ IInspectable* peer,
            _Outptr_result_maybenull_ T** ppOwnerNoRef)
        {
            HRESULT hr = S_OK;
            wrl::ComPtr<xaml::IUIElement> spOwnerAsUIElement;
            wrl::ComPtr<xaml::Automation::Peers::IFrameworkElementAutomationPeer> spThisAsFEAP;

            *ppOwnerNoRef = nullptr;

            IFC(peer->QueryInterface(
                __uuidof(xaml::Automation::Peers::IFrameworkElementAutomationPeer),
                &spThisAsFEAP));

            IFC(spThisAsFEAP->get_Owner(&spOwnerAsUIElement));
            if (spOwnerAsUIElement.Get())
            {
                wrl::ComPtr<T> spOwner;
                IFC(spOwnerAsUIElement.As(&spOwner));

                // No ref is passed back to the caller.
                *ppOwnerNoRef = spOwner.Get();
            }

        Cleanup:
            RRETURN(hr);
        }

        static
        _Check_return_ HRESULT ListenerExistsHelper(
            _In_ xaml::Automation::Peers::AutomationEvents eventId,
            _Out_ BOOLEAN* pReturnValue);
    };
}
