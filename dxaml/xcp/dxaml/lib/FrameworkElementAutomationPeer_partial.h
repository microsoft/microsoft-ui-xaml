// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "FrameworkElementAutomationPeer.g.h"
#include "ScrollItemAdapter.g.h"

namespace DirectUI
{
    // Represents the FrameworkElementAutomationPeer
    PARTIAL_CLASS(FrameworkElementAutomationPeer)
    {
        public:
            // Initializes a new instance of the AutomationPeer class.
            FrameworkElementAutomationPeer();
            ~FrameworkElementAutomationPeer() override;

            IFACEMETHOD(GetAcceleratorKeyCore)(_Out_ HSTRING* returnValue) override;
            IFACEMETHOD(GetAccessKeyCore)(_Out_ HSTRING* returnValue) override;
            IFACEMETHOD(GetAutomationControlType)(_Out_ xaml_automation_peers::AutomationControlType * returnValue) override;
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* returnValue) override;
            IFACEMETHOD(GetAutomationIdCore)(_Out_ HSTRING* returnValue) override;
            IFACEMETHOD(GetBoundingRectangleCore)(_Out_ wf::Rect* returnValue) override;
            IFACEMETHOD(GetChildrenCore)(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** returnValue) override;
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue) override;
            IFACEMETHOD(GetClickablePointCore)(_Out_ wf::Point* returnValue) override;
            IFACEMETHOD(GetHelpTextCore)(_Out_ HSTRING* returnValue) override;
            IFACEMETHOD(GetItemStatusCore)(_Out_ HSTRING* returnValue) override;
            IFACEMETHOD(GetItemTypeCore)(_Out_ HSTRING* returnValue) override;
            IFACEMETHOD(GetLabeledByCore)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue) override;
            IFACEMETHOD(GetLocalizedControlType)(_Out_ HSTRING* returnValue) override;
            IFACEMETHOD(GetLocalizedControlTypeCore)(_Out_ HSTRING* returnValue) override;
            IFACEMETHOD(IsRequiredForFormCore)(_Out_ BOOLEAN* returnValue) override;
            IFACEMETHOD(GetNameCore)(_Out_ HSTRING* returnValue) override;
            IFACEMETHOD(GetLiveSettingCore)(_Out_ xaml_automation_peers::AutomationLiveSetting* returnValue) override;
            _Check_return_ HRESULT GetControlledPeersCoreImpl(_Outptr_ wfc::IVectorView<xaml_automation_peers::AutomationPeer*>** returnValue) override;
            _Check_return_ HRESULT GetPositionInSetCoreImpl(_Out_ INT* returnValue) override;
            _Check_return_ HRESULT GetSizeOfSetCoreImpl(_Out_ INT* returnValue) override;
            _Check_return_ HRESULT GetLevelCoreImpl(_Out_ INT* returnValue) override;
            _Check_return_ HRESULT GetAnnotationsCoreImpl(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeerAnnotation*>** returnValue) override;
            _Check_return_ HRESULT GetLandmarkTypeCoreImpl(_Out_ xaml_automation_peers::AutomationLandmarkType* returnValue) override;
            _Check_return_ HRESULT GetLocalizedLandmarkTypeCoreImpl(_Out_ HSTRING* returnValue) override;
            IFACEMETHOD(IsContentElementCore)(_Out_ BOOLEAN* returnValue) override;
            IFACEMETHOD(IsControlElementCore)(_Out_ BOOLEAN* returnValue) override;
            IFACEMETHOD(IsEnabledCore)(_Out_ BOOLEAN* returnValue) override;
            IFACEMETHOD(IsKeyboardFocusableCore)(_Out_ BOOLEAN* returnValue) override;
            IFACEMETHOD(IsOffscreenCore)(_Out_ BOOLEAN* returnValue) override;
            IFACEMETHOD(SetFocusCore)() override;
            _Check_return_ HRESULT ShowContextMenuCoreImpl() override;
            _Check_return_ HRESULT IsPeripheralCoreImpl(_Out_ BOOLEAN* returnValue) final;
            _Check_return_ HRESULT IsDataValidForFormCoreImpl(_Out_ BOOLEAN* returnValue) final;
            _Check_return_ HRESULT GetFullDescriptionCoreImpl(_Out_ HSTRING* returnValue) override;
            _Check_return_ HRESULT GetDescribedByCoreImpl(_Outptr_ wfc::IIterable<xaml_automation_peers::AutomationPeer*>** returnValue) override;
            _Check_return_ HRESULT GetFlowsToCoreImpl(_Outptr_ wfc::IIterable<xaml_automation_peers::AutomationPeer*>** returnValue) override;
            _Check_return_ HRESULT GetFlowsFromCoreImpl(_Outptr_ wfc::IIterable<xaml_automation_peers::AutomationPeer*>** returnValue) override;
            _Check_return_ HRESULT GetHeadingLevelCoreImpl(_Out_ xaml_automation_peers::AutomationHeadingLevel* returnValue) override;
            _Check_return_ HRESULT IsDialogCoreImpl(_Out_ BOOLEAN* returnValue) override;

            IFACEMETHOD(get_Owner)(_Outptr_ xaml::IUIElement** pValue) override;

            _Check_return_ HRESULT put_Owner(xaml::IUIElement* pOwner);

            _Check_return_ HRESULT GetDefaultPattern(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue);

            void SetControlType(_In_ xaml_automation_peers::AutomationControlType controlType);
            _Check_return_ HRESULT SetLocalizedControlType(_In_ HSTRING localizedControlType);
            _Check_return_ HRESULT SetClassName(_In_ HSTRING className);

            void NotifyManagedUIElementIsDead() override;

            virtual _Check_return_ HRESULT ChildIsAcceptable(
                _In_ xaml::IUIElement* pElement,
                _Out_ BOOLEAN* bchildIsAcceptable);

            _Check_return_ HRESULT GetAutomationPeerChildren(
                _In_ xaml::IUIElement* pElement,
                _In_ wfc::IVector<xaml_automation_peers::AutomationPeer*>* pAPChildren);

            _Check_return_ HRESULT GetAutomationPeersForChildrenOfElementImpl(
                _In_ xaml::IUIElement* pElement,
                _Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** ppReturnValue);

        private:

            // Keep a weak ref to the owner; we don't want to keep it alive, and using a weak reference (rather than an
            // uncounted raw pointer) prevents problems during the time period between GC and finalization if the
            // owner is a CLR object.
            ctl::WeakRefPtr m_wpOwner;
            wrl_wrappers::HString m_LocalizedControlType;
            wrl_wrappers::HString m_ClassName;
            ctl::ComPtr<DirectUI::ScrollItemAdapter> m_spScrollItemAdapter;

        protected:
            _Check_return_ HRESULT GetAutomationPeerCollection(_In_ UIAXcp::APAutomationProperties eProperty, _Outptr_ wfc::IIterable<xaml_automation_peers::AutomationPeer*>** returnValue);
            
            xaml_automation_peers::AutomationControlType m_ControlType;
    };
}
