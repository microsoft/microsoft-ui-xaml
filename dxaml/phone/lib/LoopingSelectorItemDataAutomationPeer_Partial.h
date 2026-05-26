// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Automation { namespace Peers
{

    class LoopingSelectorItemDataAutomationPeer:
        public LoopingSelectorItemDataAutomationPeerGenerated
    {

    public:
        LoopingSelectorItemDataAutomationPeer();

        _Check_return_ HRESULT GetItem(_Outptr_result_maybenull_ IInspectable** ppItem);
        _Check_return_ HRESULT SetItemIndex(_In_ int index);

    protected:
        // IAutomationPeerOverrides
        _Check_return_ HRESULT GetPatternCoreImpl(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue) override;
        _Check_return_ HRESULT GetAcceleratorKeyCoreImpl(_Out_ HSTRING* returnValue) override;
        _Check_return_ HRESULT GetAccessKeyCoreImpl(_Out_ HSTRING* returnValue) override;
        _Check_return_ HRESULT GetAutomationControlTypeCoreImpl(_Out_ xaml_automation_peers::AutomationControlType* returnValue) override;
        _Check_return_ HRESULT GetAutomationIdCoreImpl(_Out_ HSTRING* returnValue) override;
        _Check_return_ HRESULT GetBoundingRectangleCoreImpl(_Out_ wf::Rect* returnValue) override;
        _Check_return_ HRESULT GetChildrenCoreImpl(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** returnValue) override;
        _Check_return_ HRESULT GetClassNameCoreImpl(_Out_ HSTRING* returnValue) override;
        _Check_return_ HRESULT GetClickablePointCoreImpl(_Out_ wf::Point* returnValue) override;
        _Check_return_ HRESULT GetHelpTextCoreImpl(_Out_ HSTRING* returnValue) override;
        _Check_return_ HRESULT GetItemStatusCoreImpl(_Out_ HSTRING* returnValue) override;
        _Check_return_ HRESULT GetItemTypeCoreImpl(_Out_ HSTRING* returnValue) override;
        _Check_return_ HRESULT GetLabeledByCoreImpl(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue) override;
        _Check_return_ HRESULT GetLocalizedControlTypeCoreImpl(_Out_ HSTRING* returnValue) override;
        _Check_return_ HRESULT GetNameCoreImpl(_Out_ HSTRING* returnValue) override;
        _Check_return_ HRESULT GetOrientationCoreImpl(_Out_ xaml_automation_peers::AutomationOrientation* returnValue) override;
        _Check_return_ HRESULT GetLiveSettingCoreImpl(_Out_ xaml_automation_peers::AutomationLiveSetting* returnValue) override;
        _Check_return_ HRESULT GetControlledPeersCoreImpl(_Outptr_ wfc::IVectorView<xaml_automation_peers::AutomationPeer*>** returnValue) override;
        _Check_return_ HRESULT HasKeyboardFocusCoreImpl(_Out_ BOOLEAN* returnValue) override;
        _Check_return_ HRESULT IsContentElementCoreImpl(_Out_ BOOLEAN* returnValue) override;
        _Check_return_ HRESULT IsControlElementCoreImpl(_Out_ BOOLEAN* returnValue) override;
        _Check_return_ HRESULT IsEnabledCoreImpl(_Out_ BOOLEAN* returnValue) override;
        _Check_return_ HRESULT IsKeyboardFocusableCoreImpl(_Out_ BOOLEAN* returnValue) override;
        _Check_return_ HRESULT IsOffscreenCoreImpl(_Out_ BOOLEAN* returnValue) override;
        _Check_return_ HRESULT IsPasswordCoreImpl(_Out_ BOOLEAN* returnValue) override;
        _Check_return_ HRESULT IsRequiredForFormCoreImpl(_Out_ BOOLEAN* returnValue) override;
        _Check_return_ HRESULT SetFocusCoreImpl() override;

        // IAutomationPeerOverrides3
        _Check_return_ HRESULT GetAnnotationsCoreImpl(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeerAnnotation*>** returnValue);
        _Check_return_ HRESULT GetPositionInSetCoreImpl(_Out_ INT* returnValue);
        _Check_return_ HRESULT GetSizeOfSetCoreImpl(_Out_ INT* returnValue);
        _Check_return_ HRESULT GetLevelCoreImpl(_Out_ INT* returnValue);

        // IAutomationPeerOverrides4
        _Check_return_ HRESULT GetLandmarkTypeCoreImpl(_Out_ xaml_automation_peers::AutomationLandmarkType* returnValue);
        _Check_return_ HRESULT GetLocalizedLandmarkTypeCoreImpl(_Out_ HSTRING* returnValue);

    private:

        _Check_return_ HRESULT InitializeImpl(_In_ IInspectable* pItem, _In_ ILoopingSelectorAutomationPeer* pOwner) override;

    public:
        _Check_return_ HRESULT RealizeImpl();

    private:
        _Check_return_ HRESULT ThrowElementNotAvailableException();
        _Check_return_ HRESULT SetParent(_In_opt_ xaml_automation_peers::ILoopingSelectorAutomationPeer* pParent);
        _Check_return_ HRESULT SetItem(_In_ IInspectable* pItem);
        _Check_return_ HRESULT GetContainerAutomationPeer(_Outptr_result_maybenull_ xaml::Automation::Peers::IAutomationPeer** ppContainer);

        Private::TrackerPtr<IInspectable> _tpItem;
        wrl::WeakRef _wrParent;
        int _itemIndex;
    };

} } } } } XAML_ABI_NAMESPACE_END
