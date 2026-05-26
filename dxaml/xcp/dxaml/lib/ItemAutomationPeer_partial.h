// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the ItemAutomationPeer
    PARTIAL_CLASS(ItemAutomationPeer)
    {
        public:
            // Initializes a new instance of the ItemAutomationPeer class.
            ItemAutomationPeer();
            ~ItemAutomationPeer() override;

            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue) override;
            IFACEMETHOD(GetAcceleratorKeyCore)(_Out_ HSTRING* returnValue) override;
            IFACEMETHOD(GetAccessKeyCore)(_Out_ HSTRING* returnValue) override;
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
            IFACEMETHOD(GetLocalizedControlTypeCore)(_Out_ HSTRING* returnValue) override;
            IFACEMETHOD(GetNameCore)(_Out_ HSTRING* returnValue) override;
            IFACEMETHOD(GetOrientationCore)(_Out_ xaml_automation_peers::AutomationOrientation* returnValue) override;
            IFACEMETHOD(GetLiveSettingCore)(_Out_ xaml_automation_peers::AutomationLiveSetting* returnValue) override;
            _Check_return_ HRESULT GetControlledPeersCoreImpl(_Outptr_ wfc::IVectorView<xaml_automation_peers::AutomationPeer*>** returnValue) final;
            _Check_return_ HRESULT GetElementFromPointCoreImpl(_In_ wf::Point point, _Outptr_ IInspectable** ppReturnValue) final;
            _Check_return_ HRESULT GetFocusedElementCoreImpl(_Outptr_ IInspectable** ppReturnValue) final;
            IFACEMETHOD(HasKeyboardFocusCore)(_Out_ BOOLEAN* returnValue) override;
            IFACEMETHOD(IsContentElementCore)(_Out_ BOOLEAN* returnValue) override;
            IFACEMETHOD(IsControlElementCore)(_Out_ BOOLEAN* returnValue) override;
            IFACEMETHOD(IsEnabledCore)(_Out_ BOOLEAN* returnValue) override;
            IFACEMETHOD(IsKeyboardFocusableCore)(_Out_ BOOLEAN* returnValue) override;
            IFACEMETHOD(IsOffscreenCore)(_Out_ BOOLEAN* returnValue) override;
            IFACEMETHOD(IsPasswordCore)(_Out_ BOOLEAN* returnValue) override;
            IFACEMETHOD(IsRequiredForFormCore)(_Out_ BOOLEAN* returnValue) override;
            IFACEMETHOD(SetFocusCore)() override;
            _Check_return_ HRESULT ShowContextMenuCoreImpl() final;
            _Check_return_ HRESULT GetPositionInSetCoreImpl(_Out_ INT* pReturnValue) final;
            _Check_return_ HRESULT GetSizeOfSetCoreImpl(_Out_ INT* pReturnValue) final;
            _Check_return_ HRESULT GetLevelCoreImpl(_Out_ INT* pReturnValue) final;
            _Check_return_ HRESULT GetAnnotationsCoreImpl(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeerAnnotation*>** returnValue) final;
            _Check_return_ HRESULT GetLandmarkTypeCoreImpl(_Out_ xaml_automation_peers::AutomationLandmarkType* returnValue) final;
            _Check_return_ HRESULT GetLocalizedLandmarkTypeCoreImpl(_Out_ HSTRING* returnValue) final;
            _Check_return_ HRESULT GetCultureCoreImpl(_Out_ INT* returnValue) final;

            IFACEMETHOD(get_Item)(_Outptr_ IInspectable** pValue) override;
            IFACEMETHOD(get_ItemsControlAutomationPeer)(_Outptr_ xaml_automation_peers::IItemsControlAutomationPeer** ppItemsControlAutomationPeer) override;
            _Check_return_ HRESULT put_Item(_In_ IInspectable* pItem);
            _Check_return_ HRESULT put_Parent(_In_ xaml_automation_peers::IItemsControlAutomationPeer* pItemsControlAutomationPeer);
            _Check_return_ HRESULT GetContainer(_Outptr_ xaml::IUIElement** ppContainer);
            _Check_return_ HRESULT GetContainerPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppContainerPeer, _Outptr_opt_ xaml::IUIElement** ppContainer = nullptr);
            void ReleaseItemAndParent();
            void ReleaseEventsSourceLink();
            _Check_return_ HRESULT  RemoveItemAutomationPeerFromItemsControlStorage();

            // IVirtualizedItemProvider
            _Check_return_ HRESULT RealizeImpl();

            _Check_return_ HRESULT NotifyNoUIAClientObjectToOwner() override;
            _Check_return_ HRESULT RaiseAutomationIsSelectedChanged(BOOLEAN isSelected);

        protected:

        private:
            _Check_return_ HRESULT GetItemAsStringFromPropertyValue(
                _In_ wf::IPropertyValue* pItemAsPropertValue,
                _Out_ HSTRING* returnValue);
            _Check_return_ HRESULT ThrowElementNotAvailableException();

        private:
            TrackerPtr<IInspectable> m_tpItem;

            // Keep a weak ref to the items control; we don't want to keep it alive, and using a weak reference (rather than an
            // uncounted raw pointer) prevents problems during the time period between GC and finalization if the
            // owner is a CLR object.
            ctl::WeakRefPtr m_wpItemsControlAutomationPeer;
    };
}
