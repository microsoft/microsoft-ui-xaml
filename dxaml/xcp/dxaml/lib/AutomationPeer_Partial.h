// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "AutomationPeer.g.h"

namespace Automation
{
    class CValue;
}

namespace DirectUI
{
    class UIElement;

    // Represents the AutomationPeer
    PARTIAL_CLASS(AutomationPeer)
    {
        public:
            _Check_return_ HRESULT GetChildrenImpl(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** returnValue);
            _Check_return_ HRESULT GetControlledPeersImpl(_Outptr_ wfc::IVectorView<xaml_automation_peers::AutomationPeer*>** returnValue);
            _Check_return_ HRESULT SetParentImpl(_In_ xaml_automation_peers::IAutomationPeer* peer);
            _Check_return_ HRESULT ShowContextMenuImpl();
            _Check_return_ HRESULT GetPeerFromPointImpl(
                _In_ wf::Point point,
                _Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);
            _Check_return_ HRESULT GetParentImpl(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);
            _Check_return_ HRESULT InvalidatePeerImpl();

            _Check_return_ HRESULT RaiseAutomationEventImpl(_In_ xaml_automation_peers::AutomationEvents eventId);
            _Check_return_ HRESULT RaiseStructureChangedEventImpl(
                _In_ xaml_automation_peers::AutomationStructureChangeType structureChangeType,
                _In_opt_ xaml_automation_peers::IAutomationPeer* pChild);
            _Check_return_ HRESULT RaisePropertyChangedEventImpl(
                _In_ xaml_automation::IAutomationProperty* pAutomationProperty,
                _In_ IInspectable* pOldValue,
                _In_ IInspectable* pNewValue);
            _Check_return_ HRESULT RaisePropertyChangedEvent(
                _In_ xaml_automation::IAutomationProperty* pAutomationProperty,
                _In_ const CValue& oldValue,
                _In_ const CValue& newValue);

            _Check_return_ HRESULT RaiseTextEditTextChangedEventImpl(
                _In_ xaml_automation::AutomationTextEditChangeType pAutomationProperty,
                _In_ wfc::IVectorView<HSTRING>* pChangedData);

            _Check_return_ HRESULT RaiseNotificationEventImpl(
                xaml_automation_peers::AutomationNotificationKind notificationKind,
                xaml_automation_peers::AutomationNotificationProcessing notificationProcessing,
                _In_opt_ HSTRING displayString,
                _In_ HSTRING activityId);

            _Check_return_ HRESULT GetPatternCoreImpl(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue);
            _Check_return_ HRESULT GetAcceleratorKeyCoreImpl(_Out_ HSTRING* returnValue);
            _Check_return_ HRESULT GetAccessKeyCoreImpl(_Out_ HSTRING* returnValue);
            _Check_return_ HRESULT GetAutomationControlTypeCoreImpl(_Out_ xaml_automation_peers::AutomationControlType* returnValue);
            _Check_return_ HRESULT GetAutomationIdCoreImpl(_Out_ HSTRING* returnValue);
            _Check_return_ HRESULT GetBoundingRectangleCoreImpl(_Out_ wf::Rect* returnValue);
            _Check_return_ HRESULT GetChildrenCoreImpl(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** returnValue);
            _Check_return_ HRESULT NavigateCoreImpl(_In_ xaml_automation_peers::AutomationNavigationDirection direction, _Outptr_ IInspectable** pReturnValue);
            _Check_return_ HRESULT GetClassNameCoreImpl(_Out_ HSTRING* returnValue);
            _Check_return_ HRESULT GetClickablePointCoreImpl(_Out_ wf::Point* returnValue);
            _Check_return_ HRESULT GetHelpTextCoreImpl(_Out_ HSTRING* returnValue);
            _Check_return_ HRESULT GetItemStatusCoreImpl(_Out_ HSTRING* returnValue);
            _Check_return_ HRESULT GetItemTypeCoreImpl(_Out_ HSTRING* returnValue);
            _Check_return_ HRESULT GetLabeledByCoreImpl(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);
            _Check_return_ HRESULT GetLocalizedControlTypeCoreImpl(_Out_ HSTRING* returnValue);
            _Check_return_ HRESULT GetNameCoreImpl(_Out_ HSTRING* returnValue);
            _Check_return_ HRESULT GetOrientationCoreImpl(_Out_ xaml_automation_peers::AutomationOrientation* returnValue);
            _Check_return_ HRESULT GetLiveSettingCoreImpl(_Out_ xaml_automation_peers::AutomationLiveSetting* returnValue);
            virtual _Check_return_ HRESULT GetControlledPeersCoreImpl(_Outptr_ wfc::IVectorView<xaml_automation_peers::AutomationPeer*>** returnValue);
            virtual _Check_return_ HRESULT GetPositionInSetCoreImpl(_Out_ INT* returnValue);
            virtual _Check_return_ HRESULT GetSizeOfSetCoreImpl(_Out_ INT* returnValue);
            virtual _Check_return_ HRESULT GetLevelCoreImpl(_Out_ INT* returnValue);
            virtual _Check_return_ HRESULT GetAnnotationsCoreImpl(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeerAnnotation*>** returnValue);
            virtual _Check_return_ HRESULT GetLandmarkTypeCoreImpl(_Out_ xaml_automation_peers::AutomationLandmarkType* returnValue);
            virtual _Check_return_ HRESULT GetLocalizedLandmarkTypeCoreImpl(_Out_ HSTRING* returnValue);
            _Check_return_ HRESULT HasKeyboardFocusCoreImpl(_Out_ BOOLEAN* returnValue);
            _Check_return_ HRESULT IsContentElementCoreImpl(_Out_ BOOLEAN* returnValue);
            _Check_return_ HRESULT IsControlElementCoreImpl(_Out_ BOOLEAN* returnValue);
            _Check_return_ HRESULT IsEnabledCoreImpl(_Out_ BOOLEAN* returnValue);
            _Check_return_ HRESULT IsKeyboardFocusableCoreImpl(_Out_ BOOLEAN* returnValue);
            _Check_return_ HRESULT IsOffscreenCoreImpl(_Out_ BOOLEAN* returnValue);
            _Check_return_ HRESULT IsPasswordCoreImpl(_Out_ BOOLEAN* returnValue);
            _Check_return_ HRESULT IsRequiredForFormCoreImpl(_Out_ BOOLEAN* returnValue);
            _Check_return_ HRESULT SetFocusCoreImpl();
            _Check_return_ HRESULT SetAutomationFocusImpl();
            virtual _Check_return_ HRESULT ShowContextMenuCoreImpl();
            virtual _Check_return_ HRESULT IsPeripheralCoreImpl(_Out_ BOOLEAN* returnValue);
            virtual _Check_return_ HRESULT IsDataValidForFormCoreImpl(_Out_ BOOLEAN* returnValue);
            virtual _Check_return_ HRESULT GetFullDescriptionCoreImpl(_Out_ HSTRING* returnValue);
            virtual _Check_return_ HRESULT GetDescribedByCoreImpl(_Outptr_ wfc::IIterable<xaml_automation_peers::AutomationPeer*>** returnValue);
            virtual _Check_return_ HRESULT GetFlowsToCoreImpl(_Outptr_ wfc::IIterable<xaml_automation_peers::AutomationPeer*>** returnValue);
            virtual _Check_return_ HRESULT GetFlowsFromCoreImpl(_Outptr_ wfc::IIterable<xaml_automation_peers::AutomationPeer*>** returnValue);
            virtual _Check_return_ HRESULT GetCultureCoreImpl(_Out_ INT* returnValue);
            virtual _Check_return_ HRESULT GetHeadingLevelCoreImpl(_Out_ xaml_automation_peers::AutomationHeadingLevel* returnValue);
            virtual _Check_return_ HRESULT IsDialogCoreImpl(_Out_ BOOLEAN* returnValue);

            IFACEMETHOD(get_EventsSource)(_Outptr_ xaml_automation_peers::IAutomationPeer** pValue) override;
            IFACEMETHOD(put_EventsSource)(_In_ xaml_automation_peers::IAutomationPeer* value) override;

            _Check_return_ HRESULT GetPeerFromPointCoreImpl(
                _In_ wf::Point point,
                _Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);
            virtual _Check_return_ HRESULT GetElementFromPointCoreImpl(
                _In_ wf::Point point,
                _Outptr_ IInspectable** pReturnValue);
            virtual _Check_return_ HRESULT GetFocusedElementCoreImpl(_Outptr_ IInspectable** pReturnValue);

            _Check_return_ HRESULT PeerFromProviderImpl(
                _In_ xaml_automation::Provider::IIRawElementProviderSimple* pProvider,
                _Outptr_ xaml_automation_peers::IAutomationPeer** ppReturnValue);
            _Check_return_ HRESULT ProviderFromPeerImpl(
                _In_ xaml_automation_peers::IAutomationPeer* pAutomationPeer,
                _Outptr_ xaml_automation::Provider::IIRawElementProviderSimple** ppReturnValue);

            virtual _Check_return_ HRESULT NotifyNoUIAClientObjectToOwner();
            virtual void NotifyManagedUIElementIsDead();
            virtual _Check_return_ HRESULT GenerateAutomationPeerEventsSource(_In_ xaml_automation_peers::IAutomationPeer* pAPParent);

            _Check_return_ HRESULT GetFlowsFrom(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);
            _Check_return_ HRESULT GetFlowsTo(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);

            _Check_return_ HRESULT RaisePropertyChangedEventById(_In_ UIAXcp::APAutomationProperties propertyId, _In_ HSTRING oldValue, _In_ HSTRING newValue);
            _Check_return_ HRESULT RaisePropertyChangedEventById(_In_ UIAXcp::APAutomationProperties propertyId, _In_ BOOLEAN oldValue, _In_ BOOLEAN newValue);

            // UIA callback
            static _Check_return_ HRESULT GetAutomationPeerStringValue(
                _In_ CDependencyObject* nativeTarget,
                _In_ UIAXcp::APAutomationProperties eProperty,
                _Out_writes_z_(*pcText) WCHAR* psText,
                _Inout_ XINT32* pcText);

            static _Check_return_ HRESULT GetAutomationPeerIntValue(
                _In_ CDependencyObject* nativeTarget,
                _In_ UIAXcp::APAutomationProperties eProperty,
                _Inout_ XINT32* pcReturnValue);

            static _Check_return_ HRESULT GetAutomationPeerPointValue(
                _In_ CDependencyObject* nativeTarget,
                _In_ UIAXcp::APAutomationProperties eProperty,
                _Out_ XPOINTF* pReturnPoint);

            static _Check_return_ HRESULT GetAutomationPeerRectValue(
                _In_ CDependencyObject* nativeTarget,
                _In_ UIAXcp::APAutomationProperties eProperty,
                _Out_ XRECTF* pReturnRect);

            static _Check_return_ HRESULT GetAutomationPeerAPValue(
                _In_ CDependencyObject* nativeTarget,
                _In_ UIAXcp::APAutomationProperties eProperty,
                _Inout_ ::CDependencyObject** ppReturnAP);

            static _Check_return_ HRESULT GetAutomationPeerDOValue(
                _In_ CDependencyObject* nativeTarget,
                _In_ UIAXcp::APAutomationProperties eProperty,
                _Outptr_ ::CDependencyObject** ppReturnDO);

            static _Check_return_ HRESULT CallAutomationPeerMethod(
                _In_ CDependencyObject* nativeTarget,
                _In_ XINT32 CallAutomationPeerMethod);

            static _Check_return_ HRESULT Navigate(
                _In_ CDependencyObject* nativeTarget,
                _In_ UIAXcp::AutomationNavigationDirection direction,
                _Outptr_result_maybenull_ ::CDependencyObject** ppReturnAPAsDO,
                _Outptr_result_maybenull_ IUnknown** ppReturnIREPFAsUnk);

            static _Check_return_ HRESULT GetAutomationPeerChildren(
                _In_ CDependencyObject* nativeTarget,
                _In_ XUINT32 CallAutomationPeerMethod,
                _Inout_ XINT32* pcReturnAPChildren,
                __deref_inout_ecount(*pcReturnAPChildren) ::CDependencyObject*** pppReturnAPChildren);

            static _Check_return_ HRESULT GetPattern(
                _In_ CDependencyObject* nativeTarget,
                _Outptr_ CDependencyObject** nativeInterface,
                _In_ UIAXcp::APPatternInterface ePatternInterface);

            static _Check_return_ HRESULT GetElementFromPoint(
                _In_ CDependencyObject* nativeTarget,
                _In_ const CValue& param,
                _Outptr_result_maybenull_ ::CDependencyObject** ppReturnAPAsDO,
                _Outptr_result_maybenull_ IUnknown** ppReturnIREPFAsUnk);

            static _Check_return_ HRESULT GetFocusedElement(
                _In_ CDependencyObject* nativeTarget,
                _Outptr_result_maybenull_ ::CDependencyObject** ppReturnAPAsDO,
                _Outptr_result_maybenull_ IUnknown** ppReturnIREPFAsUnk);

            static _Check_return_ HRESULT UIATextRangeInvoke(
                _In_ CDependencyObject* nativeTarget,
                _In_ XINT32 eFunction,
                _In_ XINT32 cParams,
                _In_ void* pvParams,
                _Out_ Automation::CValue* pReturnVal) noexcept;

            static _Check_return_ HRESULT UIAPatternInvoke(
                _In_ CDependencyObject* nativeTarget,
                _In_ UIAXcp::APPatternInterface ePatternInterface,
                _In_ XINT32 eFunction,
                _In_ XINT32 cParams,
                _In_ void* pvParams,
                _Out_ Automation::CValue* pReturnVal) noexcept;

            static _Check_return_ HRESULT NotifyNoUIAClientObjectForAP(_In_ CDependencyObject* nativeTarget);

            static _Check_return_ HRESULT GenerateAutomationPeerEventsSource(_In_ CDependencyObject* nativeTarget, _In_ CDependencyObject* nativeTargetParent);

            // Internal static methods
            static _Check_return_ HRESULT ListenerExistsHelper(
                _In_ xaml_automation_peers::AutomationEvents eventId,
                _Out_ BOOLEAN* returnValue);

            static BOOLEAN ArePropertyChangedListeners();

            static _Check_return_ HRESULT RaiseEventIfListener(
                _In_ UIElement* pUie,
                _In_ xaml_automation_peers::AutomationEvents eventId);

            static _Check_return_ HRESULT PeerFromProviderStatic(_In_ xaml_automation::Provider::IIRawElementProviderSimple* pProvider,
                _Outptr_ xaml_automation_peers::IAutomationPeer** ppReturnValue);
            static _Check_return_ HRESULT ProviderFromPeerStatic(_In_ xaml_automation_peers::IAutomationPeer* pAutomationPeer,
                _Outptr_ xaml_automation::Provider::IIRawElementProviderSimple** ppReturnValue);
            static _Check_return_ HRESULT GetTrimmedKeyboardAcceleratorTextOverrideStatic(_In_ wrl_wrappers::HString& keyboardAcceleratorTextOverride,
                _Out_ HSTRING* returnValue);

        protected:
            // Initializes a new instance of the AutomationPeer class.
            AutomationPeer();
            ~AutomationPeer() override;
            HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        private:
            TrackerPtr<xaml_automation_peers::IAutomationPeer> m_tpEventsSource;

            static void RetrieveNativeNodeOrAPFromIInspectable(
                _In_ IInspectable* pAccessibleNode,
                _Outptr_result_maybenull_ ::CDependencyObject** ppReturnAPAsDO,
                _Outptr_result_maybenull_ IUnknown** ppReturnIREPFAsUnk);

            static _Check_return_ HRESULT GetAutomationPropertyFromUIAXcpEnum(_In_ UIAXcp::APAutomationProperties eProperty, _Outptr_ xaml_automation::IAutomationProperty** ppAutomationProperty);
            static _Check_return_ HRESULT GetPropertyValueFromCValue(_In_ UIAXcp::APAutomationProperties eProperty, _In_ Automation::CValue value, _Outptr_ IInspectable** ppInspectable);
            static _Check_return_ HRESULT GetStringFromCValue(_In_ Automation::CValue* pBox, _Outptr_ HSTRING* phValue);
            static _Check_return_ HRESULT SetCValueFromString(_In_ Automation::CValue* pBox, _In_opt_ HSTRING value);
            static _Check_return_ HRESULT BoxArrayOfHStrings(Automation::CValue* pReturnVal, XINT32 nLength, HSTRING* phsArray);
            static _Check_return_ HRESULT BoxArrayOfRawElementProviderSimple(Automation::CValue* pReturnVal, XINT32 nLength, xaml_automation::Provider::IIRawElementProviderSimple** prepsArray);
            static _Check_return_ HRESULT GetAutomationPeerDOValueFromIterable(_In_ CDependencyObject* nativeTarget, _In_ UIAXcp::APAutomationProperties eProperty, _Outptr_::CDependencyObject** ppReturnDO);
    };
}

namespace AutomationPeerHelper
{
    template <typename TInterface, typename TAutomationPeerInterface, typename T, typename TAutomationPeer, typename TAutomationPeerFactory>
    static _Check_return_ HRESULT CreateInstanceWithOwnerImplForAutomationPeerFactory(
        _In_ TAutomationPeerFactory *factory,
        _In_ TInterface* owner,
        _In_opt_ IInspectable* pOuter,
        _Outptr_ IInspectable** ppInner,
        _Outptr_ TAutomationPeerInterface** ppInstance)
    {
        HRESULT hr = S_OK;
        TAutomationPeerInterface* pInstance = NULL;
        IInspectable* pInner = NULL;
        xaml::IUIElement* ownerAsUIE = NULL;

        IFCPTR(ppInstance);
        IFCEXPECT(pOuter == NULL || ppInner != NULL);
        IFCPTR(owner);
        IFC(ctl::do_query_interface(ownerAsUIE, owner));

        IFC(factory->ActivateInstance(pOuter,
            static_cast<T*>(owner)->GetHandle(),
            &pInner));
        IFC(ctl::do_query_interface(pInstance, pInner));
        IFC(static_cast<TAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

        if (ppInner)
        {
            *ppInner = pInner;
            pInner = NULL;
        }

        *ppInstance = pInstance;
        pInstance = NULL;

    Cleanup:
        ReleaseInterface(ownerAsUIE);
        ReleaseInterface(pInstance);
        ReleaseInterface(pInner);
        RRETURN(hr);
    }
}

// To make AutomationPeer API public for a control, Each control need to implement Control##AutomationPeerFactory::CreateInstanceWithOwnerImpl in Control##AutomationPeer_Partial.cpp
// It's hard to debug with macro only code, so this macro just forwards the function call to a template function.
#define IMPLEMENT_CONTROL_AUTOMATIONPEERFACTORY_CREATEINSTANCE(Control) \
_Check_return_ HRESULT Control##AutomationPeerFactory::CreateInstanceWithOwnerImpl( \
    _In_ xaml_controls::I##Control* owner, \
    _In_opt_ IInspectable* pOuter, \
    _Outptr_ IInspectable** ppInner, \
    _Outptr_ xaml_automation_peers::I##Control##AutomationPeer** ppInstance) \
{ \
    return AutomationPeerHelper::CreateInstanceWithOwnerImplForAutomationPeerFactory< \
        typename xaml_controls::I##Control, \
        typename xaml_automation_peers::I##Control##AutomationPeer, \
        typename Control, \
        typename Control##AutomationPeer, \
        typename Control##AutomationPeerFactory>(this, owner, pOuter, ppInner, ppInstance); \
}