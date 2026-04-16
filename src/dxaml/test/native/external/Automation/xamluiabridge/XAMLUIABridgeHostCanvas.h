// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "pch.h"
#include <UIAutomationHelper.h>

namespace Tests { namespace Native { namespace External { namespace Automation { namespace XamlUIABridge {

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    public ref class XamlUIABridgeHostCanvasAutomationPeer sealed : public Microsoft::UI::Xaml::Automation::Peers::AutomationPeer, public Microsoft::UI::Xaml::Automation::Provider::ICustomNavigationProvider
    {
    public:
        XamlUIABridgeHostCanvasAutomationPeer() : Microsoft::UI::Xaml::Automation::Peers::AutomationPeer()
        {
        }

        virtual ~XamlUIABridgeHostCanvasAutomationPeer()
        {
        }

        void Init(Object^ firstChild)
        {
            m_FirstChild = firstChild;
        }

        void DeInit()
        {
            m_FirstChild = nullptr;
        }

    protected:
        Object^ NavigateCore(Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection direction) override;
        Object^ GetElementFromPointCore(::Windows::Foundation::Point pointInWindowCoordinates) override;
        Object^ GetFocusedElementCore() override;
        bool IsKeyboardFocusableCore() override;
        Object^ GetPatternCore(Microsoft::UI::Xaml::Automation::Peers::PatternInterface patternInterface) override;

    public:
        // ICustomNavigationProvider
        virtual Object^ NavigateCustom(Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection direction);

    private:
        static Object^ DrillForElementFromPoint(::Windows::Foundation::Point point, bool isAP, Object^ element);
        static Object^ DrillForElementFromPointHelper(::Windows::Foundation::Point point, Object^ objectUnderDrill);
        Object^ m_FirstChild;
    };

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    public ref class HostedAutomationPeer sealed : public Microsoft::UI::Xaml::Automation::Peers::AutomationPeer, public Microsoft::UI::Xaml::Automation::Provider::ICustomNavigationProvider
    {
    public:
        HostedAutomationPeer()
        {}
        
        virtual ~HostedAutomationPeer()
        {
        }

        void Init(Object^ parentNode, Object^ prevSibling, Object^ nextSibling, Object^ firstChild, Object^ lastChild, ::Windows::Foundation::Rect rec, Platform::String^ name, Platform::String^ automationId)
        {
            m_Parent = parentNode;
            m_PreviousSibling = prevSibling;
            m_NextSibling = nextSibling;
            m_FirstChild = firstChild;
            m_LastChild = lastChild;
            m_rect = rec;
            m_Name = name;
            m_AutomationId = automationId;
        }

        void DeInit()
        {
            m_Parent = nullptr;
            m_PreviousSibling = nullptr;
            m_NextSibling = nullptr;
            m_FirstChild = nullptr;
            m_LastChild = nullptr;
        }

        bool DoesContainPoint(::Windows::Foundation::Point point)
        {
            return m_rect.Contains(point);
        }

        ::Windows::Foundation::Rect GetBoundingRectangleCore() override
        {
            return m_rect;
        }

        Platform::String^ GetNameCore() override
        {
            return m_Name;
        }

        Platform::String^ GetAutomationIdCore() override
        {
            return m_AutomationId;
        }

        bool IsOffscreenCore() override
        {
            return true;
        }

        bool IsEnabledCore() override
        {
            return true;
        }

        bool IsControlElementCore() override
        {
            return true;
        }

        bool IsContentElementCore() override
        {
            return true;
        }

        Object^ NavigateCore(Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection direction) override
        {
            switch (direction)
            {
            case Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection::FirstChild:
                return m_FirstChild;
            case Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection::LastChild:
                return m_LastChild;
            case Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection::NextSibling:
                return m_NextSibling;
            case Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection::Parent:
                return m_Parent;
            case Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection::PreviousSibling:
                return m_PreviousSibling;
            default:
                break;
            }
            return nullptr;
        }

        Object^ GetPatternCore(Microsoft::UI::Xaml::Automation::Peers::PatternInterface patternInterface) override
        {
            if (patternInterface == Microsoft::UI::Xaml::Automation::Peers::PatternInterface::CustomNavigation)
            {
                return this;
            }
            else
            {
                return AutomationPeer::GetPatternCore(patternInterface);
            }
        }

        // ICustomNavigationProvider
        virtual Object^ NavigateCustom(Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection direction)
        {
            return NavigateCore(direction);
        }


    private:
        Object^ m_Parent;
        Object^ m_PreviousSibling;
        Object^ m_NextSibling;
        Object^ m_FirstChild;
        Object^ m_LastChild;
        ::Windows::Foundation::Rect m_rect;
        Platform::String^ m_Name;
        Platform::String^ m_AutomationId;
    };

    class HostedIRawElementProviderSimple : public IInspectable, public IRawElementProviderSimple2, public IRawElementProviderFragment, public IInvokeProvider, public ICustomNavigationProvider
    {
    public:
        HostedIRawElementProviderSimple(){
            m_cRef = 1;
        }

        void Init(double left, double top, double right, double bottom, long controlType, IInspectable* firstChild, IInspectable* lastChild, IInspectable* nextSibling, IInspectable* prevSibling, IInspectable* parent, IInspectable* host, const wchar_t* pName)
        {
            m_left = left;
            m_right = right;
            m_top = top;
            m_bottom = bottom;
            m_spFirstChild = firstChild;
            m_spLastChild = lastChild;
            m_spNextSibling = nextSibling;
            m_spPreviousSibling = prevSibling;
            m_spParent = parent;
            m_ControlType = controlType;
            m_spHost = host;
            m_name = pName;
        }

        void DeInit()
        {
            m_spFirstChild = nullptr;
            m_spLastChild = nullptr;
            m_spNextSibling = nullptr;
            m_spPreviousSibling = nullptr;
            m_spParent = nullptr;
            m_spHost = nullptr;
        }

        // IInspectable methods
        virtual HRESULT STDMETHODCALLTYPE GetIids(
            __RPC__out ULONG *iidCount,
            __RPC__deref_out_ecount_full_opt(*iidCount) IID **iids)
        {
            IID *pResult = NULL;
            ULONG count = 0;

            static IID const * const localIIDs[] =
            {
                &IID_IUnknown,
                &IID_IInspectable,
                &IID_IRawElementProviderSimple,
                &IID_IRawElementProviderSimple2,
                &IID_IRawElementProviderFragment
            };

            count = _countof(localIIDs);

            pResult = reinterpret_cast<IID *>(CoTaskMemAlloc(sizeof(IID)* count));
            WEX::Common::Throw::IfNull(pResult);

            for (UINT current = 0; current < count; current++)
            {
                pResult[current] = *(localIIDs[current]);
            }
            *iids = pResult;
            *iidCount = count;

            return S_OK;
        }

        virtual HRESULT STDMETHODCALLTYPE GetRuntimeClassName(
            __RPC__deref_out_opt HSTRING *className)
        {
            LogThrow_IfFailed(WindowsCreateString(L"Windows.Foundation.IReference`1<XamlUIABridge.HostedIRawElementProviderSimple>", 45 + 33, className));

            return S_OK;
        }

        virtual HRESULT STDMETHODCALLTYPE GetTrustLevel(
            __RPC__out TrustLevel *trustLevel)
        {
            *trustLevel = BaseTrust;

            return S_OK;
        }

        bool DoesContainPoint(::Windows::Foundation::Point point)
        {
            if (m_left <= point.X && point.X <= m_right && m_top <= point.Y && point.Y <= m_bottom)
            {
                return true;
            }
            return false;
        }

        IInspectable* GetNextSiblingNoRef()
        {
            return m_spNextSibling.Get();
        }

        IInspectable* GetFirstChildNoRef()
        {
            return m_spFirstChild.Get();
        }

        ULONG STDMETHODCALLTYPE AddRef();
        ULONG STDMETHODCALLTYPE Release();
        HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void**);

        // IRawElementProviderSimple methods
        HRESULT STDMETHODCALLTYPE get_ProviderOptions(_Out_ ProviderOptions * pRetVal);
        HRESULT STDMETHODCALLTYPE GetPatternProvider(_In_ PATTERNID patternId, _Out_ IUnknown ** pRetVal);
        HRESULT STDMETHODCALLTYPE GetPropertyValue(_In_ PROPERTYID propertyId, _Out_ VARIANT * pRetVal);
        HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(_Out_ IRawElementProviderSimple ** pRetVal);

        // IRawElementProviderSimple2 methods
        HRESULT STDMETHODCALLTYPE ShowContextMenu();

        // IRawElementProviderFragment methods
        HRESULT STDMETHODCALLTYPE get_BoundingRectangle(_Out_ UiaRect * pRetVal);
        HRESULT STDMETHODCALLTYPE get_FragmentRoot(_Out_ IRawElementProviderFragmentRoot** pRetVal);
        HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots(_Out_ SAFEARRAY **pRetVal);
        HRESULT STDMETHODCALLTYPE GetRuntimeId(_Out_ SAFEARRAY ** pRetVal);
        HRESULT STDMETHODCALLTYPE Navigate(_In_ NavigateDirection direction, _Out_ IRawElementProviderFragment ** pRetVal);
        HRESULT STDMETHODCALLTYPE SetFocus();

        //IInvokeProvider definition
        HRESULT STDMETHODCALLTYPE Invoke();

        //ICustomNavigation definition
        HRESULT STDMETHODCALLTYPE Navigate(enum NavigateDirection direction, _Outptr_result_maybenull_ IRawElementProviderSimple **pRetVal);

    private:
        static SAFEARRAY* GenerateRuntimeId()
        {
            Microsoft::UI::Xaml::Automation::Peers::RawElementProviderRuntimeId rId = Microsoft::UI::Xaml::Automation::Peers::AutomationPeer::GenerateRawElementProviderRuntimeId();
            Microsoft::UI::Xaml::Tests::Common::AutoSafeArray<VT_I4> runtimeIdsAutoSafeArray(static_cast<ULONG>(2));

            runtimeIdsAutoSafeArray.AddElement((void*)(&(rId.Part1)));
            runtimeIdsAutoSafeArray.AddElement((void*)(&(rId.Part2)));


            return runtimeIdsAutoSafeArray.Detach();
        }
        void CreateBoundingRectangleProperty(_Out_ VARIANT *pOutput);
        void GetClickablePoint(_Out_ VARIANT *pVariant);

        double m_left;
        double m_top;
        double m_right;
        double m_bottom;
        long m_ControlType;
        Microsoft::WRL::ComPtr<IInspectable> m_spFirstChild;
        Microsoft::WRL::ComPtr<IInspectable> m_spLastChild;
        Microsoft::WRL::ComPtr<IInspectable> m_spPreviousSibling;
        Microsoft::WRL::ComPtr<IInspectable> m_spNextSibling;
        Microsoft::WRL::ComPtr<IInspectable> m_spParent;
        Microsoft::WRL::ComPtr<IInspectable> m_spHost;
        Microsoft::UI::Xaml::Tests::Common::AutoSafeArray<VT_I4> m_runtimeIdsAutoSafeArray;
        ULONG m_cRef;
        const wchar_t* m_name;
    };

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class XamlUIABridgeHostCanvas sealed: xaml_controls::Canvas
    {
    public:
        XamlUIABridgeHostCanvas()
        {
            m_pAP = nullptr;
        }

        virtual ~XamlUIABridgeHostCanvas()
        {
            // DeInit all UIA tree members as they were tightly coupled creating circular reference.
            m_NativeNode5->DeInit();
            m_pAPNodeD->DeInit();
            m_pAPNodeC->DeInit();
            m_pAPNodeB->DeInit();
            m_NativeNode4->DeInit();
            m_NativeNode3->DeInit();
            m_pAPNodeA->DeInit();
            m_NativeNode2->DeInit();
            m_RootNative->DeInit();
            m_pAP->DeInit();
        }

    protected:
        virtual ::Microsoft::UI::Xaml::Automation::Peers::AutomationPeer^ OnCreateAutomationPeer() override;

    private:
        XamlUIABridgeHostCanvasAutomationPeer^ m_pAP;
        Microsoft::WRL::ComPtr<HostedIRawElementProviderSimple> m_RootNative;
        Microsoft::WRL::ComPtr<HostedIRawElementProviderSimple> m_NativeNode2;
        Microsoft::WRL::ComPtr<HostedIRawElementProviderSimple> m_NativeNode3;
        Microsoft::WRL::ComPtr<HostedIRawElementProviderSimple> m_NativeNode4;
        Microsoft::WRL::ComPtr<HostedIRawElementProviderSimple> m_NativeNode5;
        HostedAutomationPeer^ m_pAPNodeA;
        HostedAutomationPeer^ m_pAPNodeB;
        HostedAutomationPeer^ m_pAPNodeC;
        HostedAutomationPeer^ m_pAPNodeD;
    };
} } } } }
