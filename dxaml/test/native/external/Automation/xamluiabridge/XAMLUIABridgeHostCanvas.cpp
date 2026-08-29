// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlUIABridgeHostCanvas.h>

using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Tests { namespace Native { namespace External { namespace Automation { namespace XamlUIABridge {

    ::Microsoft::UI::Xaml::Automation::Peers::AutomationPeer^ XamlUIABridgeHostCanvas::OnCreateAutomationPeer()
    {

        // Building the complete custom UIA tree involving XAML Nodes and native UIA nodes here.
        if (m_pAP == nullptr)
        {
            m_pAP = ref new XamlUIABridgeHostCanvasAutomationPeer();

            m_RootNative.Attach(new HostedIRawElementProviderSimple());
            wrl::ComPtr<IInspectable> rootNativeAsInsp;
            m_RootNative.As(&rootNativeAsInsp);

            m_NativeNode2.Attach(new HostedIRawElementProviderSimple());
            wrl::ComPtr<IInspectable> nativeNode2AsInsp;
            m_NativeNode2.As(&nativeNode2AsInsp);

            m_NativeNode3.Attach(new HostedIRawElementProviderSimple());
            wrl::ComPtr<IInspectable> nativeNode3AsInsp;
            m_NativeNode3.As(&nativeNode3AsInsp);

            m_NativeNode4.Attach(new HostedIRawElementProviderSimple());
            wrl::ComPtr<IInspectable> nativeNode4AsInsp;
            m_NativeNode4.As(&nativeNode4AsInsp);

            m_NativeNode5.Attach(new HostedIRawElementProviderSimple());
            wrl::ComPtr<IInspectable> nativeNode5AsInsp;
            m_NativeNode5.As(&nativeNode5AsInsp);

            m_pAPNodeA = ref new HostedAutomationPeer();
            wrl::ComPtr<IInspectable> apNodeAAsInsp;
            (reinterpret_cast<IUnknown*>(m_pAPNodeA))->QueryInterface(__uuidof(IInspectable), &apNodeAAsInsp);

            m_pAPNodeB = ref new HostedAutomationPeer();
            wrl::ComPtr<IInspectable> apNodeBAsInsp;
            (reinterpret_cast<IUnknown*>(m_pAPNodeB))->QueryInterface(__uuidof(IInspectable), &apNodeBAsInsp);

            m_pAPNodeC = ref new HostedAutomationPeer();
            wrl::ComPtr<IInspectable> apNodeCAsInsp;
            (reinterpret_cast<IUnknown*>(m_pAPNodeC))->QueryInterface(__uuidof(IInspectable), &apNodeCAsInsp);

            m_pAPNodeD = ref new HostedAutomationPeer();
            wrl::ComPtr<IInspectable> apNodeDAsInsp;
            (reinterpret_cast<IUnknown*>(m_pAPNodeD))->QueryInterface(__uuidof(IInspectable), &apNodeDAsInsp);

            m_pAP->Init(reinterpret_cast<Object^>(rootNativeAsInsp.Get()));
            wrl::ComPtr<IInspectable> m_pAPAsInsp;
            (reinterpret_cast<IUnknown*>(m_pAP))->QueryInterface(__uuidof(IInspectable), &m_pAPAsInsp);

            // XAML Root Window: CUIAWindow
            // Host Canvas AP: XamlUIABridgeHostCanvasAutomationPeer
            // AP Node: objects of HostedAutomationPeer
            // Native Node: objects of HostedIRawElementProviderSimple implements IREPS, IREPF and are accessible nodes based off UIAutomationCore
            //
            //                                          --XAML Root Window--
            //                                                  |
            //                                           --Host Canvas AP--
            //                                                  |
            //                                          --Root Native Node--
            //                                                  |
            //      ---------------------------------------------------------------------------------------
            //     |                        |                   |                  |                       |
            // --Native Node 2--      --AP Node A--      --Native Node 3--      --NAtive Node 4--     --AP Node B--
            //                              |
            //      -------------------------------------------------
            //     |                        |                        |
            //   --AP Node C          --AP Node D--          --Native Node 5--

            m_RootNative->Init(20, 20, 620, 620, UIA_PaneControlTypeId, nativeNode2AsInsp.Get(), apNodeBAsInsp.Get(), nullptr, nullptr, m_pAPAsInsp.Get(), m_pAPAsInsp.Get(), L"Root Native");
            m_NativeNode2->Init(30, 30, 80, 80, UIA_PaneControlTypeId, nullptr, nullptr, apNodeAAsInsp.Get(), nullptr, rootNativeAsInsp.Get(), m_pAPAsInsp.Get(), L"Native Node 2");
            m_NativeNode3->Init(410, 30, 440, 80, UIA_PaneControlTypeId, nullptr, nullptr, nativeNode4AsInsp.Get(), apNodeAAsInsp.Get(), rootNativeAsInsp.Get(), m_pAPAsInsp.Get(), L"Native Node 3");
            m_NativeNode4->Init(450, 30, 480, 80, UIA_PaneControlTypeId, nullptr, nullptr, apNodeBAsInsp.Get(), nativeNode3AsInsp.Get(), rootNativeAsInsp.Get(), m_pAPAsInsp.Get(), L"Native Node 4");
            m_NativeNode5->Init(320, 120, 480, 180, UIA_PaneControlTypeId, nullptr, nullptr, nullptr, apNodeDAsInsp.Get(), apNodeAAsInsp.Get(), m_pAPAsInsp.Get(), L"Native Node 5");

            m_pAPNodeA->Init(reinterpret_cast<Object^>(rootNativeAsInsp.Get()), reinterpret_cast<Object^>(nativeNode2AsInsp.Get()), reinterpret_cast<Object^>(nativeNode3AsInsp.Get()), m_pAPNodeC, reinterpret_cast<Object^>(nativeNode5AsInsp.Get()), ::Windows::Foundation::Rect(100, 30, 300, 500), ref new Platform::String(L"AP Node A"), ref new Platform::String(L"AP Node A"));
            m_pAPNodeB->Init(reinterpret_cast<Object^>(rootNativeAsInsp.Get()), reinterpret_cast<Object^>(nativeNode4AsInsp.Get()), nullptr, nullptr, nullptr, ::Windows::Foundation::Rect(500, 30, 80, 80), ref new Platform::String(L"AP Node B"), ref new Platform::String(L"AP Node B"));
            m_pAPNodeC->Init(m_pAPNodeA, nullptr, m_pAPNodeD, nullptr, nullptr, ::Windows::Foundation::Rect(120, 120, 60, 60), ref new Platform::String(L"AP Node C"), ref new Platform::String(L"AP Node C"));
            m_pAPNodeD->Init(m_pAPNodeA, m_pAPNodeC, reinterpret_cast<Object^>(nativeNode5AsInsp.Get()), nullptr, nullptr, ::Windows::Foundation::Rect(220, 120, 60, 60), ref new Platform::String(L"AP Node D"), ref new Platform::String(L"AP Node D"));
        }
        return m_pAP;
    }

    // Hosting Canvas just contains Root Native node which hosts the rest of the UIA tree.
    Platform::Object^ XamlUIABridgeHostCanvasAutomationPeer::NavigateCore(Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection direction)
    {
        if (direction == Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection::FirstChild || direction == Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection::LastChild)
        {
            return m_FirstChild;
        }

        return nullptr;
    }

    // Hosting Canvas handles the hit-testing for the complete tree it is hosting underneath it.
    Platform::Object^ XamlUIABridgeHostCanvasAutomationPeer::GetElementFromPointCore(::Windows::Foundation::Point point)
    {
        return XamlUIABridgeHostCanvasAutomationPeer::DrillForElementFromPoint(point, false, m_FirstChild);
    }

    // This function basically searches for the contained point in depth-first.
    Platform::Object^ XamlUIABridgeHostCanvasAutomationPeer::DrillForElementFromPoint(::Windows::Foundation::Point point, bool isAP, Object^ element)
    {
        HostedAutomationPeer^ ap = nullptr;
        wrl::ComPtr<IRawElementProviderSimple> spIREPS;
        wrl::ComPtr<IInspectable> nativeNodeAAsInsp;
        Platform::Object^ objectUnderDrill = nullptr;
        Platform::Object^ result;
        bool isPointContained = false;

        if (isAP)
        {
            ap = static_cast<HostedAutomationPeer^>(element);
            isPointContained = ap->DoesContainPoint(point);

            if (isPointContained)
            {
                objectUnderDrill = ap->Navigate(Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection::FirstChild);
                result = DrillForElementFromPointHelper(point, objectUnderDrill);
                if (result)
                {
                    return result;
                }

                return element;
            }
            else
            {
                return nullptr;
            }
        }
        else
        {
            nativeNodeAAsInsp = reinterpret_cast<IInspectable*>(element);
            isPointContained = (static_cast<HostedIRawElementProviderSimple*>(nativeNodeAAsInsp.Get()))->DoesContainPoint(point);
            if (isPointContained)
            {
                objectUnderDrill = reinterpret_cast<Object^>(static_cast<HostedIRawElementProviderSimple*>(nativeNodeAAsInsp.Get())->GetFirstChildNoRef());
                result = DrillForElementFromPointHelper(point, objectUnderDrill);
                if (result)
                {
                    return result;
                }

                return element;
            }
            else
            {
                return nullptr;
            }
        }
    }

    Platform::Object^ XamlUIABridgeHostCanvasAutomationPeer::DrillForElementFromPointHelper(::Windows::Foundation::Point point, Object^ objectUnderDrill)
    {
        Platform::Object^ result;
        wrl::ComPtr<IRawElementProviderSimple> spIREPS;

        while (objectUnderDrill != nullptr)
        {
            HostedAutomationPeer^ objDrillAP = nullptr;
            (reinterpret_cast<IInspectable*>(objectUnderDrill))->QueryInterface(__uuidof(IRawElementProviderSimple), &spIREPS);
            if (!spIREPS)
            {
                objDrillAP = static_cast<HostedAutomationPeer^>(objectUnderDrill);
                result = XamlUIABridgeHostCanvasAutomationPeer::DrillForElementFromPoint(point, true, objectUnderDrill);
                if (result == nullptr)
                {
                    objectUnderDrill = objDrillAP->Navigate(Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection::NextSibling);
                }
                else
                    return result;
            }
            else
            {
                result = XamlUIABridgeHostCanvasAutomationPeer::DrillForElementFromPoint(point, false, objectUnderDrill);
                if (result == nullptr)
                {
                    HostedIRawElementProviderSimple* hostIREPS = static_cast<HostedIRawElementProviderSimple*>(reinterpret_cast<IInspectable*>(objectUnderDrill));
                    objectUnderDrill = reinterpret_cast<Object^>(hostIREPS->GetNextSiblingNoRef());
                }
                else
                    return result;
            }
        }

        return nullptr;
    }


    // Hosting Canvas handles the focus management for the complete tree it is hosting underneath it.
    // Native Node 3 is designated as dedicated focused element.
    Platform::Object^ XamlUIABridgeHostCanvasAutomationPeer::GetFocusedElementCore()
    {
        wrl::ComPtr<IRawElementProviderFragment> spRootNative;
        LogThrow_IfFailed(reinterpret_cast<IInspectable*>(m_FirstChild)->QueryInterface<IRawElementProviderFragment>(&spRootNative));

        if (spRootNative)
        {
            wrl::ComPtr<IRawElementProviderFragment> spNativeNode2;
            LogThrow_IfFailed(spRootNative->Navigate(NavigateDirection_FirstChild, &spNativeNode2));

            if (spNativeNode2)
            {
                wrl::ComPtr<IRawElementProviderFragment> spAPNodeA;
                LogThrow_IfFailed(spNativeNode2->Navigate(NavigateDirection_NextSibling, &spAPNodeA));

                if (spAPNodeA)
                {
                    wrl::ComPtr<IRawElementProviderFragment> spNativeNode3;
                    LogThrow_IfFailed(spAPNodeA->Navigate(NavigateDirection_NextSibling, &spNativeNode3));

                    if (spNativeNode3)
                    {
                        wrl::ComPtr<IInspectable> spFocusedEleemnt;
                        LogThrow_IfFailed(spNativeNode3.As<IInspectable>(&spFocusedEleemnt));

                        if (spFocusedEleemnt)
                        {
                            return reinterpret_cast<Object^>(spFocusedEleemnt.Get());
                        }
                    }
                }
            }
        }

        return nullptr;
    }

    // This is required for Host Canvas to manage Focus for tree underneath it.
    bool XamlUIABridgeHostCanvasAutomationPeer::IsKeyboardFocusableCore()
    {
        return true;
    }


    Platform::Object^ XamlUIABridgeHostCanvasAutomationPeer::GetPatternCore(Microsoft::UI::Xaml::Automation::Peers::PatternInterface pattern)
    {
        if (pattern == Microsoft::UI::Xaml::Automation::Peers::PatternInterface::CustomNavigation)
        {
            return this;
        }
        else
        {
            return AutomationPeer::GetPatternCore(pattern);
        }
    }

    // ICustomNavigationProvider
    Platform::Object^ XamlUIABridgeHostCanvasAutomationPeer::NavigateCustom(Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection direction)
    {
        return NavigateCore(direction);
    }

    // IUnknown implementation.

    ULONG STDMETHODCALLTYPE HostedIRawElementProviderSimple::AddRef()
    {
        ASSERT(m_cRef != 0);
        return ++m_cRef;
    }

    ULONG STDMETHODCALLTYPE HostedIRawElementProviderSimple::Release()
    {
        ASSERT(m_cRef != 0);
        ULONG refs = --m_cRef;

        if (refs == 0)
        {
            delete this;
        }

        return refs;
    }

    HRESULT STDMETHODCALLTYPE HostedIRawElementProviderSimple::QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface)
    {
        WEX::Common::Throw::IfNull(ppInterface);

        if (riid == __uuidof(IUnknown))
        {
            *ppInterface = static_cast<IUnknown*>(static_cast<IRawElementProviderFragment*>(this));
        }
        else if (riid == __uuidof(IInspectable))
        {
            *ppInterface = static_cast<IInspectable*>(this);
        }
        else if (riid == __uuidof(IRawElementProviderSimple))
        {
            *ppInterface = static_cast<IRawElementProviderSimple*>(this);
        }
        else if (riid == __uuidof(IRawElementProviderSimple2))
        {
            *ppInterface = static_cast<IRawElementProviderSimple2*>(this);
        }
        else if (riid == __uuidof(IRawElementProviderFragment))
        {
            *ppInterface = static_cast<IRawElementProviderFragment*>(this);
        }
        else if (riid == __uuidof(IInvokeProvider))
        {
            *ppInterface = static_cast<IInvokeProvider*>(this);
        }
        else if (riid == __uuidof(ICustomNavigationProvider))
        {
            *ppInterface = static_cast<ICustomNavigationProvider*>(this);
        }
        else
        {
            *ppInterface = NULL;
            return E_NOINTERFACE;
        }

        (static_cast<IUnknown*>(*ppInterface))->AddRef();
        return S_OK;
    }


    // IRawElementProviderSimple implementation

    // Get Provider options.

    HRESULT STDMETHODCALLTYPE HostedIRawElementProviderSimple::get_ProviderOptions(_Out_ ProviderOptions* pRetVal)
    {
        WEX::Common::Throw::IfNull(pRetVal);

        *pRetVal = ProviderOptions_ServerSideProvider | ProviderOptions_UseComThreading;

        return S_OK;
    }

    // Get the object that supports IInvokePattern.

    HRESULT STDMETHODCALLTYPE HostedIRawElementProviderSimple::GetPatternProvider(_In_ PATTERNID patternId, _Out_ IUnknown** ppRetVal)
    {
        WEX::Common::Throw::IfNull(ppRetVal);

        *ppRetVal = NULL;

        if (patternId == UIA_InvokePatternId)
        {
            LogThrow_IfFailed(QueryInterface(__uuidof(IInvokeProvider), reinterpret_cast<void**>(ppRetVal)));
        }
        else if (patternId == UIA_CustomNavigationPatternId)
        {
            LogThrow_IfFailed(QueryInterface(__uuidof(ICustomNavigationProvider), reinterpret_cast<void**>(ppRetVal)));
        }

        return S_OK;
    }

    // Gets custom properties.

    HRESULT STDMETHODCALLTYPE HostedIRawElementProviderSimple::GetPropertyValue(_In_ PROPERTYID propertyId, _Out_ VARIANT* pRetVal)
    {
        WEX::Common::Throw::IfNull(pRetVal);
        pRetVal->vt = VT_EMPTY;

        switch (propertyId)
        {
        case UIA_AutomationIdPropertyId:
            break;

        case UIA_IsEnabledPropertyId:
            pRetVal->vt = VT_BOOL;
            pRetVal->boolVal = VARIANT_TRUE;
            break;

        case UIA_BoundingRectanglePropertyId:
            CreateBoundingRectangleProperty(pRetVal);
            break;

        case UIA_ClickablePointPropertyId:
            GetClickablePoint(pRetVal);
            break;

        case UIA_ControlTypePropertyId:
            pRetVal->vt = VT_I4;
            pRetVal->lVal = m_ControlType;
            break;

        case UIA_IsContentElementPropertyId:
            pRetVal->vt = VT_BOOL;
            pRetVal->boolVal = VARIANT_TRUE;
            break;

        case UIA_IsControlElementPropertyId:
            pRetVal->vt = VT_BOOL;
            pRetVal->boolVal = VARIANT_TRUE;
            break;

        case UIA_IsKeyboardFocusablePropertyId:
            pRetVal->vt = VT_BOOL;
            pRetVal->boolVal = VARIANT_TRUE;
            break;

        case UIA_IsOffscreenPropertyId:
            pRetVal->vt = VT_BOOL;
            pRetVal->boolVal = VARIANT_FALSE;
            break;
        case UIA_ClassNamePropertyId:
            pRetVal->vt = VT_BSTR;
            pRetVal->bstrVal = SysAllocString(L"HostedIRawElementProviderSimple");
            break;
        case UIA_NamePropertyId:
            pRetVal->vt = VT_BSTR;
            pRetVal->bstrVal = SysAllocString(m_name);
            break;
        case UIA_HasKeyboardFocusPropertyId:
            pRetVal->vt = VT_BOOL;
            pRetVal->boolVal = VARIANT_FALSE;
            if (!wcscmp(m_name, L"Native Node 3"))
            {
                pRetVal->boolVal = VARIANT_TRUE;
            }
            break;
        default:
            break;
        }

        return S_OK;
    }

    void HostedIRawElementProviderSimple::CreateBoundingRectangleProperty(
        _Out_ VARIANT *pOutput)
    {
        ::AutoSafeArray<VT_R8> psaBoundingRect(4);

        double rId[] = { m_left, m_top, m_right, m_bottom };

        for (LONG i = 0; (i < 4); i++)
        {
            psaBoundingRect.AddElement((void*)&(rId[i]));
        }

        pOutput->parray = psaBoundingRect.Detach();
        pOutput->vt = VT_ARRAY | VT_R8;
    }

    void HostedIRawElementProviderSimple::GetClickablePoint(_Out_ VARIANT *pVariant)
    {
        ::AutoSafeArray<VT_R8> psaClickablePoint(2);

        long x = static_cast<long>((m_right + m_left) / 2);
        long y = static_cast<long>((m_bottom + m_top) / 2);
        double rId[] = { static_cast<double>(x), static_cast<double>(y) };

        for (LONG i = 0; (i < 2); i++)
        {
            psaClickablePoint.AddElement((void*)&(rId[i]));
        }

        pVariant->parray = psaClickablePoint.Detach();
        pVariant->vt = VT_ARRAY | VT_R8;
    }

    // Gets the UI Automation CUIAWrapper for the host window. This CUIAWrapper supplies most properties.
    HRESULT STDMETHODCALLTYPE HostedIRawElementProviderSimple::get_HostRawElementProvider(_Out_  IRawElementProviderSimple** ppRetVal)
    {
        WEX::Common::Throw::IfNull(ppRetVal);

        *ppRetVal = NULL;
        return S_OK;
    }

    // Sets the focus to this element
    HRESULT STDMETHODCALLTYPE HostedIRawElementProviderSimple::ShowContextMenu()
    {
        return S_OK;
    }

    // IRawElementProviderFragment implementation

    // Gets the bounding rectangle of this element

    HRESULT STDMETHODCALLTYPE HostedIRawElementProviderSimple::get_BoundingRectangle(_Out_ UiaRect * pRetVal)
    {
        WEX::Common::Throw::IfNull(pRetVal);

        pRetVal->left = m_left;
        pRetVal->top = m_top;
        pRetVal->width = m_right - m_left;
        pRetVal->height = m_bottom - m_top;

        return S_OK;
    }

    // Gets the root node of the fragment

    HRESULT STDMETHODCALLTYPE HostedIRawElementProviderSimple::get_FragmentRoot(_Out_ IRawElementProviderFragmentRoot** ppRetVal)
    {
        WEX::Common::Throw::IfNull(ppRetVal);
        wrl::ComPtr<IRawElementProviderFragment> spFrag;
        *ppRetVal = nullptr;

        if (m_spHost)
        {
            LogThrow_IfFailed(m_spHost.As(&spFrag));

            if (spFrag)
            {
                LogThrow_IfFailed(spFrag->get_FragmentRoot(ppRetVal));
            }
        }
        return S_OK;
    }

    // Retrieves an array of root fragments that are embedded in the UI Automation tree rooted at the current element
    // Returns NULL if there is no Automation Framework contained within
    HRESULT STDMETHODCALLTYPE HostedIRawElementProviderSimple::GetEmbeddedFragmentRoots(_Out_ SAFEARRAY **ppRetVal)
    {
        WEX::Common::Throw::IfNull(ppRetVal);
        *ppRetVal = NULL;
        return S_OK;
    }

    // Retrieves the runtime identifier of an element
    HRESULT STDMETHODCALLTYPE HostedIRawElementProviderSimple::GetRuntimeId(_Out_ SAFEARRAY ** ppRetVal)
    {
        WEX::Common::Throw::IfNull(ppRetVal);
        if (m_runtimeIdsAutoSafeArray.Get() == nullptr)
        {
            m_runtimeIdsAutoSafeArray.Attach(HostedIRawElementProviderSimple::GenerateRuntimeId());
        }

        LogThrow_IfFailed(SafeArrayCopy(m_runtimeIdsAutoSafeArray.Get(), ppRetVal));
        return S_OK;
    }

    // Retrieves the UI Automation element in a specified direction within the UI Automation tree
    HRESULT STDMETHODCALLTYPE HostedIRawElementProviderSimple::Navigate(_In_ NavigateDirection direction, _Out_ IRawElementProviderFragment ** ppRetVal)
    {
        WEX::Common::Throw::IfNull(ppRetVal);

        *ppRetVal = NULL;
        wrl::ComPtr<IRawElementProviderFragment> spFrag;

        switch (direction)
        {
        case NavigateDirection_FirstChild:
            if (m_spFirstChild)
            {
                LogThrow_IfFailed(m_spFirstChild.As(&spFrag));
            }
            break;
        case NavigateDirection_LastChild:
            if (m_spLastChild)
            {
                LogThrow_IfFailed(m_spLastChild.As(&spFrag));
            }
            break;
        case NavigateDirection_NextSibling:
            if (m_spNextSibling)
            {
                LogThrow_IfFailed(m_spNextSibling.As(&spFrag));
            }
            break;
        case NavigateDirection_PreviousSibling:
            if (m_spPreviousSibling)
            {
                LogThrow_IfFailed(m_spPreviousSibling.As(&spFrag));
            }
            break;
        case NavigateDirection_Parent:
            if (m_spParent)
            {
                LogThrow_IfFailed(m_spParent.As(&spFrag));
            }
            break;
        }

        *ppRetVal = spFrag.Detach();
        return S_OK;
    }

    // Sets the focus to this element
    HRESULT STDMETHODCALLTYPE HostedIRawElementProviderSimple::SetFocus()
    {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE HostedIRawElementProviderSimple::Invoke()
    {
        if (!wcscmp(m_name, L"Native Node 3"))
        {
            m_name = L"New Native Node 3";

            if (Microsoft::UI::Xaml::Automation::Peers::AutomationPeer::ListenerExists(Microsoft::UI::Xaml::Automation::Peers::AutomationEvents::PropertyChanged))
            {
                ::AutoVariant autoVarNameOld;
                ::AutoVariant autoVarNameNew;
                autoVarNameOld.SetString(L"Native Node 3");
                autoVarNameNew.SetString(m_name);

                LogThrow_IfFailed(UiaRaiseAutomationPropertyChangedEvent(this, UIA_NamePropertyId, *(autoVarNameOld.Storage()), *(autoVarNameNew.Storage())));
            }
        }

        if (Microsoft::UI::Xaml::Automation::Peers::AutomationPeer::ListenerExists(Microsoft::UI::Xaml::Automation::Peers::AutomationEvents::InvokePatternOnInvoked))
        {
            LogThrow_IfFailed(UiaRaiseAutomationEvent(this, UIA_Invoke_InvokedEventId));
        }

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE HostedIRawElementProviderSimple::Navigate(enum NavigateDirection direction, _Outptr_result_maybenull_ IRawElementProviderSimple **pRetVal)
    {
        wrl::ComPtr<IRawElementProviderFragment> spTarget;
        LogThrow_IfFailed(Navigate(direction, &spTarget));
        LogThrow_IfFailed(spTarget.CopyTo(pRetVal));
        return S_OK;
    }


} } } } } // Tests::Native::External::Automation::XamlUIABridge;
