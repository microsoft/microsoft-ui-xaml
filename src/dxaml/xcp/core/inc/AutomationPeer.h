// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#define DEFAULT_AP_STRING_SIZE      128
#define MAX_AP_STRING_SIZE          2048
#define AP_BULK_CHILDREN_LIMIT      20

// Must be kept in sync with AutomationPeer.cs (GetAutomationPeerChildren method)
#define GET_AUTOMATION_PEER_CHILDREN_COUNT 0
#define GET_AUTOMATION_PEER_CHILDREN_CHILDREN 1

#include "UIAEnums.h"
#include "UIAStructs.h"

#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <UIAutomationCore.h>

class CAutomationPeerCollection;
class CAutomationPeerAnnotationCollection;
class CCoreServices;
class CUIAWindow;
class CFocusManager;

MIDL_INTERFACE("865F5B88-6506-4E64-A4C5-4B7723650731")
IAutomationPeerHwndInterop : public IUnknown
{
public:
    virtual /* [propput] */ HRESULT STDMETHODCALLTYPE GetRawElementProviderSimple(
        /* [retval][out] */ __RPC__deref_out_opt IRawElementProviderSimple** value) = 0;

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE IsCorrectPeerForHwnd(
                                                 HWND hwnd,
        /* [retval][out] */ __RPC__deref_out_opt bool * value) = 0;

};

class CAutomationPeer : public CDependencyObject
{
public:
    // Destructor
    ~CAutomationPeer() override;

    // Creation method
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate)
    {
        HRESULT hr = S_OK;
        CAutomationPeer *pObject = nullptr;

        IFCEXPECT(pCreate);

        pObject = new CAutomationPeer(pCreate->m_pCore, pCreate->m_value);
        IFC(ValidateAndInit(pObject, ppObject));

        // On success we've transferred ownership

        pObject = nullptr;

    Cleanup:
        delete pObject;
        return hr;
    }

    _Check_return_ HRESULT InitInstance() final;

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CAutomationPeer>::Index;
    }

    IUIAWrapper* GetUIAWrapper();
    void SetUIAWrapper(IUIAWrapper* pUIAWrapper);

    virtual CAutomationPeer* GetAPParent();
    CAutomationPeer* GetLogicalAPParent();

    HRESULT Navigate(_In_ UIAXcp::AutomationNavigationDirection direction, _Outptr_result_maybenull_ CAutomationPeer** ppReturnAP, _Outptr_result_maybenull_ IUnknown** ppReturnIREPFAsUnk);
    XINT32 GetChildren(CAutomationPeer ***pChildrenAP);
    XINT32 GetRootChildren(CAutomationPeer ***pChildrenAP);
    IUIAProvider* GetPattern(_In_ UIAXcp::APPatternInterface ePattern);
    IUIATextRangeProvider* GetTextRangePattern(_In_ CDependencyObject* pInteropObject);

    HRESULT GetAcceleratorKey(_Out_ xstring_ptr* pstrRetVal);
    HRESULT GetAccessKey(_Out_ xstring_ptr* pstrRetVal);
    HRESULT GetAutomationControlType(_Out_ UIAXcp::APAutomationControlType *pRetVal);
    HRESULT GetAutomationId(_Out_ xstring_ptr* pstrRetVal);
    HRESULT GetBoundingRectangle(_Out_ XRECTF* pRetVal);
    HRESULT GetClassName(_Out_ xstring_ptr* pstrRetVal);
    HRESULT GetClickablePoint(_Out_ XPOINTF* pRetVal);
    HRESULT GetHelpText(_Out_ xstring_ptr* pstrRetVal);
    HRESULT GetItemStatus(_Out_ xstring_ptr* pstrRetVal);
    HRESULT GetItemType(_Out_ xstring_ptr* pstrRetVal);
    HRESULT GetLabeledBy(_Outptr_ CAutomationPeer** pRetVal);
    HRESULT GetLocalizedControlType(_Out_ xstring_ptr* pstrRetVal);
    HRESULT GetName(_Out_ xstring_ptr* pstrRetVal);
    HRESULT GetOrientation(_Out_ UIAXcp::OrientationType* pRetVal);
    HRESULT GetLiveSetting(_Out_ UIAXcp::LiveSetting* pRetVal);
    HRESULT GetControlledPeers(_Outptr_ CAutomationPeerCollection **ppRetVal);
    HRESULT HasKeyboardFocus(_Out_ XINT32* pRetVal);
    HRESULT IsContentElement(_Out_ XINT32* pRetVal);
    HRESULT IsControlElement(_Out_ XINT32* pRetVal);
    HRESULT IsEnabled(_Out_ XINT32* pRetVal);
    HRESULT IsKeyboardFocusable(_Out_ XINT32* pRetVal);
    HRESULT IsOffscreen(_Out_ XINT32* pRetVal);
    HRESULT IsPassword(_Out_ XINT32* pRetVal);
    HRESULT IsRequiredForForm(_Out_ XINT32* pRetVal);
    HRESULT GetElementFromPoint(_In_ XPOINTF *pLocation, _Outptr_result_maybenull_ CAutomationPeer** ppReturnAP, _Outptr_result_maybenull_ IUnknown** ppReturnIREPFAsUnk);
    HRESULT GetFocusedElement(_Outptr_result_maybenull_ CAutomationPeer** ppReturnAP, _Outptr_result_maybenull_ IUnknown** ppReturnIREPFAsUnk);
    void InvalidatePeer();
    HRESULT ShowContextMenu();
    HRESULT GetPositionInSet(_Out_ XINT32* pRetVal);
    HRESULT GetSizeOfSet(_Out_ XINT32* pRetVal);
    HRESULT GetLevel(_Out_ XINT32* pRetVal);
    HRESULT GetAnnotations(_Outptr_ CAutomationPeerAnnotationCollection** ppRetVal);
    HRESULT GetLandmarkType(_Out_ UIAXcp::AutomationLandmarkType* pRetVal);
    HRESULT GetLocalizedLandmarkType(_Out_ xstring_ptr* pstrRetVal);
    HRESULT IsPeripheral(_Out_ XINT32* pRetVal);
    HRESULT IsDataValidForForm(_Out_ XINT32* pRetVal);
    HRESULT GetFullDescription(_Out_ xstring_ptr* pstrRetVal);
    HRESULT GetDescribedBy(_Outptr_ CAutomationPeerCollection **ppRetVal);
    HRESULT GetFlowsTo(_Outptr_ CAutomationPeerCollection **ppRetVal);
    HRESULT GetFlowsFrom(_Outptr_ CAutomationPeerCollection **ppRetVal);
    HRESULT GetCulture(_Out_ XINT32* pRetVal);
    HRESULT GetHeadingLevel(_Out_ UIAXcp::AutomationHeadingLevel* pRetVal);
    HRESULT IsDialog(_Out_ XINT32* returnValue);

    bool ListenerExists(_In_ UIAXcp::APAutomationEvents eventId);
    void RaiseAutomationEvent(_In_ UIAXcp::APAutomationEvents eventId);
    void RaisePropertyChangedEvent(_In_ UIAXcp::APAutomationProperties eAutomationProperty,
                                   _In_ const CValue& oldValue,
                                   _In_ const CValue& newValue);
    void RaiseTextEditTextChangedEvent(_In_ UIAXcp::AutomationTextEditChangeType nAutomationTextEditChangeType, _In_ CValue *pChangedData);
    void RaiseNotificationEvent(
        UIAXcp::AutomationNotificationKind notificationKind,
        UIAXcp::AutomationNotificationProcessing notificationProcessing,
        _In_opt_ xstring_ptr displayString,
        _In_ xstring_ptr activityId);

    void SetFocus();

    bool FindCachedPattern(_In_ UIAXcp::APPatternInterface,
                           _In_ CDependencyObject* pPatternObject,
                           _Outptr_ IUIAProvider** ppPatternProvider);

// Interface to override

    virtual XINT32 GetRootChildrenCore(CAutomationPeer ***pChildrenAP);

    HRESULT GetAutomationIdHelper(_Out_ xstring_ptr* pstrRetVal);
    virtual HRESULT HasKeyboardFocusHelper(_Out_ BOOLEAN* pRetVal);
    virtual HRESULT IsEnabledHelper(_Out_ BOOLEAN* pRetVal);
    virtual HRESULT IsKeyboardFocusableHelper(_Out_ BOOLEAN* pRetVal);
    virtual HRESULT IsOffscreenHelper(bool ignoreClippingOnScrollContentPresenters, _Out_ BOOLEAN* pRetVal);
    virtual HRESULT SetFocusHelper();
    void SetAutomationFocusHelper();
    virtual HRESULT ShowContextMenuHelper();
    bool HasParent();
    virtual HRESULT GetCultureHelper(_Out_ int* returnValue);

// Internal stuff
    void RaiseAutomaticPropertyChanges(bool firePropertyChangedEvents);

    int GetRuntimeId();

    void InvalidateUIAWrapper();
    void InvalidateOwner();
    void NotifyManagedUIElementIsDead();
    void Deinit();
    CAutomationPeer* GetAPEventsSource();

    void SetAPParent(CAutomationPeer* pAP)
    {
        if (pAP)
        {
            AddRefInterface(pAP);
        }

        ReleaseInterface(m_pAPParent);
        m_pAPParent = pAP;
    }

    void SetAPEventsSource(CAutomationPeer* pAP)
    {
        m_pAPEventsSource = pAP;
    }

    virtual bool IsHostedWindowlessElementProvider()
    {
        return false;
    }
    virtual bool GetPreventKeyboardDisplayOnProgrammaticFocus()
    {
        return false;
    }
    virtual void* GetWindowlessRawElementProviderSimple()
    {
        return nullptr;
    }

    virtual void* GetUnwrappedPattern(_In_ XINT32 patternID)
    {
        return nullptr;
    }

    virtual bool IsHostedWindowlessElementFromPointProvider()
    {
        return false;
    }

    _Check_return_ HRESULT ThrowElementNotAvailableError();

    CDependencyObject* GetRootNoRef() const;

    CDependencyObject* GetDONoRef() const { return m_pDO; }

protected:
    CAutomationPeer(_In_ CCoreServices *pCore, _In_ CValue &value)
        : CDependencyObject(pCore)
        , m_pDO(value.As<valueObject>())
    {
        ASSERT(GetContext());
    }

    HRESULT GetAutomationPeerStringValueFromManaged(_In_ UIAXcp::APAutomationProperties app, _Out_ xstring_ptr* pstrString);
    HRESULT GetAutomationPeerIntValueFromManaged(_In_ UIAXcp::APAutomationProperties app, _Out_ XINT32* pRetVal);
    HRESULT GetAutomationPeerPointValueFromManaged(_In_ UIAXcp::APAutomationProperties app, _Out_ XPOINTF* pPoint);
    HRESULT GetAutomationPeerRectValueFromManaged(_In_ UIAXcp::APAutomationProperties app, _Out_ XRECTF* pRect);
    HRESULT GetAutomationPeerAPValueFromManaged(_In_ UIAXcp::APAutomationProperties app, _Out_ CAutomationPeer** pAP);
    HRESULT GetAutomationPeerDOValueFromManaged(_In_ UIAXcp::APAutomationProperties app, _Outptr_ CDependencyObject** pDO);

    void ClearChildrenList();

    CFocusManager* GetFocusManagerNoRef() const;

    CXcpList<IUIAProvider>* m_pPatternsList     = nullptr;
    IUIAWrapper* m_pUIAWrapper                  = nullptr;
    CAutomationPeer* m_pAPParent                = nullptr;
    CAutomationPeer* m_pAPEventsSource          = nullptr;

    // m_pDO usually refers to the element peer of the AutomationPeer, but not always. In some cases m_pDO points to the parent.
    // See comment in CAutomationPeer::GetRootNoRef
    CDependencyObject* m_pDO                    = nullptr;

    // Property Change Values
    xstring_ptr strCurrentItemStatus;
    xstring_ptr strCurrentName;
    XINT32 nCurrentIsOffscreen                  = FALSE;
    XINT32 nCurrentIsEnabled                    = TRUE;

    // Members related to the StructureChanged automation event.
    int m_runtimeId  = 0;
};
