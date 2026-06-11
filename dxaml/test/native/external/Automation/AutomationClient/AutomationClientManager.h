// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <corewindow.h>
#include <XamlTailored.h>
#include <wrl\module.h>
#include <UIAutomationHelper.h>

#include <asyncinfo.h>


using namespace test_infra;
using namespace ::Windows::UI::Core;
using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationClient {

    // Maybe use this as parameter in most of helper cases instead of ClientManager.
    struct UIAElementInfo
    {
        const WCHAR* m_Name;
        const WCHAR* m_AutomationID;
        const WCHAR* m_ItemStatus;
        CONTROLTYPEID m_cType;
    };

    // This class manages Automation Client related tasks. This could involve finding the Client Automation Objects or
    // dealing with various kinds of tree views. This class will be changed/refactored a lot as we keep on covering
    // UIA scenarios involving various Patterns/Events/Automation objects.
    class AutomationClientManager
    {
    public:
        AutomationClientManager()
            : m_spCurrentUIAutomationElement(nullptr)
            , m_spCacheRequest(nullptr)
            , m_spAutomation(nullptr)
        {
        }

        void GetAutomation(IUIAutomation** ppAutomation)
        {
            WEX::Common::Throw::IfNull(ppAutomation);
            *ppAutomation = nullptr;

            EnsureAutomation();
            LogThrow_IfFailed(m_spAutomation.CopyTo(ppAutomation));
        }

        void SetCurrentUIAutomationElement(IUIAutomationElement* pAutomationElement)
        {
            m_spCurrentUIAutomationElement = pAutomationElement;
        }

        void GetCurrentUIAutomationElement(IUIAutomationElement** ppAutomationElement)
        {
            LogThrow_IfFailed(m_spCurrentUIAutomationElement.CopyTo(ppAutomationElement));
        }

        void GetCurrentCacheRequest(IUIAutomationCacheRequest** ppAutomationCacheReq)
        {
            if (!m_spCacheRequest)
            {
                EnsureAutomation();
                LogThrow_IfFailed(m_spAutomation->CreateCacheRequest(&m_spCacheRequest));
            }
            LogThrow_IfFailed(m_spCacheRequest.CopyTo(ppAutomationCacheReq));
        }

        // Must be called on Non-UI Thread.
        // This functions finds the Client side UIAutomation Object based on the UIAElementInfo provided
        // and Initializes the Client Manager based on settings related to this object.
        static std::shared_ptr<AutomationClientManager> CreateAutomationClientManagerFromInfo(UIAElementInfo info)
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            auto spAutomationClientManager = std::make_shared<AutomationClientManager>();

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                wrl::ComPtr<IUIAutomationElement> spUiaXamlRootElement;
                wrl::ComPtr<IUIAutomationCondition> spUIAutomationNameCondition;

                wrl::ComPtr<IUIAutomation> spAutomation;
                spAutomationClientManager->GetAutomation(&spAutomation);

                Common::AutoVariant autoVar;
                autoVar.SetString(info.m_Name);
                LogThrow_IfFailedWithMessage(spAutomation->CreatePropertyCondition(UIA_NamePropertyId, *(autoVar.Storage()), &spUIAutomationNameCondition), L"AutomationClientManager::CreateAutomationClientManagerFromInfo: Failed in creating PropertyCondition.");

                UIA_HWND handle = GetHandleToUseForUIA();
                LogThrow_IfFailedWithMessage(spAutomation->ElementFromHandle(handle, &spUiaXamlRootElement), L"AutomationClientManager::CreateAutomationClientManagerFromInfo: Failed in retrieving root element.");

                // Maybe Use all Name, AutomationId and ControlType in And Condition to uniquely find the element
                LogThrow_IfFailedWithMessage(spUiaXamlRootElement->FindFirst(TreeScope_Descendants, spUIAutomationNameCondition.Get(), &spUIAutomationElement), L"AutomationClientManager::CreateAutomationClientManagerFromInfo: Failed in finding the required UIA Element.");
            });

            spAutomationClientManager->SetCurrentUIAutomationElement(spUIAutomationElement.Get());

            return spAutomationClientManager;
        }

        // Overload for islands sceanrio, where we want to pass in a specific island hwnd.
        // Make sure to call on the island's UI thread.
        static std::shared_ptr<AutomationClientManager> CreateAutomationClientManagerFromInfo(
            UIAElementInfo info,
            UIA_HWND islandHwnd)
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            auto spAutomationClientManager = std::make_shared<AutomationClientManager>();

            wrl::ComPtr<IUIAutomationElement> spUiaXamlRootElement;
            wrl::ComPtr<IUIAutomationCondition> spUIAutomationNameCondition;

            wrl::ComPtr<IUIAutomation> spAutomation;
            spAutomationClientManager->GetAutomation(&spAutomation);

            Common::AutoVariant autoVar;
            autoVar.SetString(info.m_Name);
            LogThrow_IfFailedWithMessage(spAutomation->CreatePropertyCondition(UIA_NamePropertyId, *(autoVar.Storage()), &spUIAutomationNameCondition), L"AutomationClientManager::CreateAutomationClientManagerFromInfo: Failed in creating PropertyCondition.");

            LogThrow_IfFailedWithMessage(spAutomation->ElementFromHandle(islandHwnd, &spUiaXamlRootElement), L"AutomationClientManager::CreateAutomationClientManagerFromInfo: Failed in retrieving root element.");

            // Maybe Use all Name, AutomationId and ControlType in And Condition to uniquely find the element
            LogThrow_IfFailedWithMessage(spUiaXamlRootElement->FindFirst(TreeScope_Descendants, spUIAutomationNameCondition.Get(), &spUIAutomationElement), L"AutomationClientManager::CreateAutomationClientManagerFromInfo: Failed in finding the required UIA Element.");

            spAutomationClientManager->SetCurrentUIAutomationElement(spUIAutomationElement.Get());

            return spAutomationClientManager;
        }

        static std::shared_ptr<AutomationClientManager> CreateAutomationClientManagerFromInfoId(UIAElementInfo info)
        {
            Common::AutoVariant autoVar;
            wrl::ComPtr<IUIAutomation> spAutomation;
            wrl::ComPtr<IUIAutomationElement> spUiaXamlRootElement;
            wrl::ComPtr<IUIAutomationCondition> spUIAutomationIdCondition;
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            auto spAutomationClientManager = std::make_shared<AutomationClientManager>();

            spAutomationClientManager->GetAutomation(&spAutomation);
            autoVar.SetString(info.m_AutomationID);
            LogThrow_IfFailedWithMessage(spAutomation->CreatePropertyCondition(UIA_AutomationIdPropertyId, *(autoVar.Storage()), &spUIAutomationIdCondition), L"AutomationClientManager::CreateAutomationClientManagerFromInfoId: Failed in creating PropertyCondition.");

            auto handle = GetHandleToUseForUIA();
            LogThrow_IfFailedWithMessage(spAutomation->ElementFromHandle(handle, &spUiaXamlRootElement), L"AutomationClientManager::CreateAutomationClientManagerFromInfoId: Failed in retrieving root element.");

            // Maybe Use all Name, AutomationId and ControlType in And Condition to uniquely find the element
            LogThrow_IfFailedWithMessage(spUiaXamlRootElement->FindFirst(TreeScope_Descendants, spUIAutomationIdCondition.Get(), &spUIAutomationElement), L"AutomationClientManager::CreateAutomationClientManagerFromInfoId: Failed in finding the required UIA Element.");
            spAutomationClientManager->SetCurrentUIAutomationElement(spUIAutomationElement.Get());

            return spAutomationClientManager;
        }

        static std::shared_ptr<AutomationClientManager> CreateAutomationClientManagerFromInfoPopup(HWND hwnd, UIAElementInfo info)
        {
            Common::AutoVariant autoVar;
            wrl::ComPtr<IUIAutomation> spAutomation;
            wrl::ComPtr<IUIAutomationElement> spUiaXamlRootElement;
            wrl::ComPtr<IUIAutomationCondition> spUIAutomationNameCondition;
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            auto spAutomationClientManager = std::make_shared<AutomationClientManager>();

            spAutomationClientManager->GetAutomation(&spAutomation);
            autoVar.SetString(info.m_Name);
            LogThrow_IfFailedWithMessage(spAutomation->CreatePropertyCondition(UIA_NamePropertyId, *(autoVar.Storage()), &spUIAutomationNameCondition), L"AutomationClientManager::CreateAutomationClientManagerFromInfoPopup: Failed in creating PropertyCondition.");

            LogThrow_IfFailedWithMessage(spAutomation->ElementFromHandle(hwnd ? hwnd : GetHandleToUseForUIA(), &spUiaXamlRootElement), L"AutomationClientManager::CreateAutomationClientManagerFromInfoPopup: Failed in retrieving root element.");

            // Maybe Use all Name, AutomationId and ControlType in And Condition to uniquely find the element
            LogThrow_IfFailedWithMessage(spUiaXamlRootElement->FindFirst(TreeScope_Descendants, spUIAutomationNameCondition.Get(), &spUIAutomationElement), L"AutomationClientManager::CreateAutomationClientManagerFromInfoPopup: Failed in finding the required UIA Element.");
            spAutomationClientManager->SetCurrentUIAutomationElement(spUIAutomationElement.Get());

            return spAutomationClientManager;
        }

        static std::shared_ptr<AutomationClientManager> CreateAutomationClientManagerFromWindow()
        {
            auto spAutomationClientManager = std::make_shared<AutomationClientManager>();

            wrl::ComPtr<IUIAutomation> spAutomation;
            spAutomationClientManager->GetAutomation(&spAutomation);

            UIA_HWND handle = GetHandleToUseForUIA();
            wrl::ComPtr<IUIAutomationElement> spWindowElement;
            VERIFY_SUCCEEDED(spAutomation->ElementFromHandle(handle, &spWindowElement));

            spAutomationClientManager->SetCurrentUIAutomationElement(spWindowElement.Get());

            return spAutomationClientManager;

        }

        static std::shared_ptr<AutomationClientManager> CreateAutomationClientManagerFromClassName(const wchar_t* className)
        {
            auto clientManager = std::make_shared<AutomationClientManager>();

            wil::unique_variant classNameVar;
            classNameVar.vt = VT_BSTR;
            classNameVar.bstrVal = ::SysAllocString(className);

            wrl::ComPtr<IUIAutomation> automation;
            clientManager->GetAutomation(&automation);

            wrl::ComPtr<IUIAutomationCondition> condition;
            LogThrow_IfFailedWithMessage(automation->CreatePropertyCondition(UIA_ClassNamePropertyId, classNameVar, &condition), L"AutomationClientManager::CreateAutomationClientManagerFromClassName: Failed in creating PropertyCondition.");

            auto handle = GetHandleToUseForUIA();
            wrl::ComPtr<IUIAutomationElement> rootElement;
            LogThrow_IfFailedWithMessage(automation->ElementFromHandle(handle, &rootElement), L"AutomationClientManager::CreateAutomationClientManagerFromClassName: Failed in retrieving root element.");

            wrl::ComPtr<IUIAutomationElement> element;
            LogThrow_IfFailedWithMessage(rootElement->FindFirst(TreeScope_Descendants, condition.Get(), &element), L"AutomationClientManager::CreateAutomationClientManagerFromClassName: Failed in finding the required UIA Element.");
            clientManager->SetCurrentUIAutomationElement(element.Get());

            return clientManager;
        }

        static std::shared_ptr<AutomationClientManager> CreateAutomationClientManagerFromClassName(const wchar_t* className, UIA_HWND handle)
        {
            auto clientManager = std::make_shared<AutomationClientManager>();

            wil::unique_variant classNameVar;
            classNameVar.vt = VT_BSTR;
            classNameVar.bstrVal = ::SysAllocString(className);

            wrl::ComPtr<IUIAutomation> automation;
            clientManager->GetAutomation(&automation);

            wrl::ComPtr<IUIAutomationCondition> condition;
            LogThrow_IfFailedWithMessage(automation->CreatePropertyCondition(UIA_ClassNamePropertyId, classNameVar, &condition), L"AutomationClientManager::CreateAutomationClientManagerFromClassName: Failed in creating PropertyCondition.");

            wrl::ComPtr<IUIAutomationElement> rootElement;
            LogThrow_IfFailedWithMessage(automation->ElementFromHandle(handle, &rootElement), L"AutomationClientManager::CreateAutomationClientManagerFromClassName: Failed in retrieving root element.");

            wrl::ComPtr<IUIAutomationElement> element;
            LogThrow_IfFailedWithMessage(rootElement->FindFirst(TreeScope_Descendants, condition.Get(), &element), L"AutomationClientManager::CreateAutomationClientManagerFromClassName: Failed in finding the required UIA Element.");
            clientManager->SetCurrentUIAutomationElement(element.Get());

            return clientManager;
        }

        IUIAutomationElement* GetParent(IUIAutomationElement* element)
        {
            WEX::Common::Throw::IfNull(element);
            EnsureAutomation();

            Microsoft::WRL::ComPtr<IUIAutomationTreeWalker> spWalker;
            LogThrow_IfFailed(m_spAutomation->get_ControlViewWalker(&spWalker));

            wrl::ComPtr<IUIAutomationCacheRequest> spCacheReq;
            GetCurrentCacheRequest(&spCacheReq);

            Microsoft::WRL::ComPtr<IUIAutomationElement> spParentNode;
            LogThrow_IfFailed(spWalker->GetParentElementBuildCache(element, spCacheReq.Get(), &spParentNode));

            return spParentNode.Detach();
        }

        IUIAutomationElement* GetPrevious(IUIAutomationElement* element)
        {
            WEX::Common::Throw::IfNull(element);
            EnsureAutomation();

            Microsoft::WRL::ComPtr<IUIAutomationTreeWalker> spWalker;
            LogThrow_IfFailed(m_spAutomation->get_ControlViewWalker(&spWalker));

            wrl::ComPtr<IUIAutomationCacheRequest> spCacheReq;
            GetCurrentCacheRequest(&spCacheReq);

            Microsoft::WRL::ComPtr<IUIAutomationElement> spPrevious;
            LogThrow_IfFailed(spWalker->GetPreviousSiblingElementBuildCache(element, spCacheReq.Get(), &spPrevious));

            return spPrevious.Detach();
        }

        IUIAutomationElement* GetNext(IUIAutomationElement* element)
        {
            WEX::Common::Throw::IfNull(element);
            EnsureAutomation();

            Microsoft::WRL::ComPtr<IUIAutomationTreeWalker> spWalker;
            LogThrow_IfFailed(m_spAutomation->get_ControlViewWalker(&spWalker));

            wrl::ComPtr<IUIAutomationCacheRequest> spCacheReq;
            GetCurrentCacheRequest(&spCacheReq);

            Microsoft::WRL::ComPtr<IUIAutomationElement> spNext;
            LogThrow_IfFailed(spWalker->GetNextSiblingElementBuildCache(element, spCacheReq.Get(), &spNext));

            return spNext.Detach();
        }

        bool IsElementSame(IUIAutomationElement* pElement)
        {
            BOOL fAreEqual = FALSE;
            LogThrow_IfFailed(m_spAutomation->CompareElements(m_spCurrentUIAutomationElement.Get(), pElement, &fAreEqual));
            return !!fAreEqual;
        }

        bool HasChildren(IUIAutomationElement* element)
        {
            WEX::Common::Throw::IfNull(element);
            EnsureAutomation();

            Microsoft::WRL::ComPtr<IUIAutomationTreeWalker> spWalker;
            LogThrow_IfFailed(m_spAutomation->get_ControlViewWalker(&spWalker));

            Microsoft::WRL::ComPtr<IUIAutomationElement> childNode;
            LogThrow_IfFailed(spWalker->GetFirstChildElement(element, &childNode));

            return childNode != nullptr;
        }

        static std::shared_ptr<AutomationClientManager> CreateAutomationClientManagerFromOpenPopup(UIAElementInfo info, xaml::UIElement^ xamlRootReferenceObject, UINT index = 0)
        {
            return AutomationClientManager::CreateAutomationClientManagerFromInfoPopup(GetHwndFromOpenWindowedPopup(xamlRootReferenceObject, index), info);
        }

        static HWND GetHwndFromOpenWindowedPopup(xaml::UIElement^ xamlRootReferenceObject, UINT index = 0)
        {
            HWND hwnd = nullptr;

            RunOnUIThread([&]()
            {
                auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(xamlRootReferenceObject->XamlRoot);
                VERIFY_IS_LESS_THAN(index, popups->Size, L"Index out of bounds of list of open popups.");
                hwnd = GetHwndFromWindowedPopup(popups->GetAt(index));
            });

            return hwnd;
        }

        static HWND GetHwndFromWindowedPopup(xaml_primitives::Popup^ popup)
        {
            return reinterpret_cast<HWND>(TestServices::WindowHelper->Popup_GetWindow(popup));
        }

    private:

        void EnsureAutomation()
        {
            if (m_spAutomation == nullptr)
            {
                LogThrow_IfFailedWithMessage(CoCreateInstance(CLSID_CUIAutomation8, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_spAutomation)), L"AutomationClientManager::GetAutomation: Failed in creating IUIAutomation Object.");
            }
        }

        static UIA_HWND GetHandleToUseForUIA()
        {
            // On Desktop, we use a HWND as a handle for UIA.
            UIA_HWND handle;
            if (!test_infra::TestServices::Utilities->IsDesktop)
            {
                LOG_OUTPUT(L"In-proc UIA testing is currently only supported on Desktop skus.");
                FAIL_FAST();
            }
            else
            {
                handle = reinterpret_cast<UIA_HWND>(TestServices::WindowHelper->GetUIAWindowHandle());
            }
            return handle;
        }

        wrl::ComPtr<IUIAutomation> m_spAutomation;
        wrl::ComPtr<IUIAutomationElement> m_spCurrentUIAutomationElement;
        wrl::ComPtr<IUIAutomationCacheRequest> m_spCacheRequest;
    };
} } } } } } // namespace Microsoft::UI::Xaml::Tests::Automation::AutomationClient

