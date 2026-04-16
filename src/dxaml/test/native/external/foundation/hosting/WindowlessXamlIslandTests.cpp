// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "WindowlessXamlIslandTests.h"

#include <Microsoft.UI.Content.h>
#include <Microsoft.UI.Dispatching.h>
#include <Microsoft.UI.Dispatching.Interop.h>
#include <Microsoft.UI.Input.h>
#include <Microsoft.UI.Interop.h>
#include <Microsoft.UI.Xaml.h>
#include <XamlTailored.h>
#include "TestEvent.h"
#include "WaitForDebugger.h"
#include <functional>
#include <wrl.h>
#include <windowsnumerics.h>
#include <AutomationClient/AutomationClientManager.h>
#include <UIAutomationHelper.h>
#include <sstream>

using namespace Microsoft::UI;
using namespace Microsoft::UI::Composition;
using namespace Microsoft::UI::Composition::SystemBackdrops;
using namespace Microsoft::UI::Content;
using namespace Microsoft::UI::Dispatching;
using namespace Microsoft::UI::Input;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Documents;
using namespace Microsoft::UI::Xaml::Hosting;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Interop;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace Microsoft::UI::Windowing;
using namespace Windows::Foundation;
using namespace Private::Infrastructure;
using namespace Platform;

namespace wrl = Microsoft::WRL;

#pragma warning(disable:4698)  // Disable warnings about experimental APIs

#define LOG_UIA(...) 
//#define LOG_UIA(...) LOG_OUTPUT(__VA_ARGS__)

#include "ApplicationWithMuxc.h"
#include "XamlIslandTestUtil.h"
#include "TestAutomationNode.h"

// Return a string that describes an automation element.
std::wstring GetElementString(IUIAutomationTreeWalker* walker, IUIAutomationElement* node);

// Tree walker helper functions.
void WalkUiaTreeDepthFirst(std::wstringstream& ss, IUIAutomationTreeWalker* walker, IUIAutomationElement* node, int depth = 0);
void WalkUiaTreeUpward(std::wstringstream& ss, IUIAutomationTreeWalker* walker, IUIAutomationElement* node, const wchar_t* stopNode, int depth = 0);

// Tests if strings are euqal, with debugging messages.  Multiline strings OK.
bool TestStringsEqual(const wchar_t* expected, const std::wstring& observed);

namespace Microsoft::UI::Xaml::Tests::Foundation::Hosting {

struct WindowlessXamlIslandScene
{
    void Create(DispatcherQueue^ dq)
    {
        appWindow = AppWindow::Create();
        appWindow->Title = L"WindowlessXamlIslandTest";
        appWindow->MoveAndResize({ 50, 50, 800, 600 });
        appWindow->AssociateWithDispatcherQueue(dq);
        appWindow->Destroying += ref new TypedEventHandler<AppWindow^, Object^>([&](AppWindow^ sender, Object^ args) {
            // This will cause DispatcherQueue->RunEventLoop to return.
            dq->EnqueueEventLoopExit();
        });

        appWindow->Show();

        const ABI::Microsoft::UI::WindowId windowIdAbi {appWindow->Id.Value};
        VERIFY_SUCCEEDED(ABI::Microsoft::UI::GetWindowFromWindowId(windowIdAbi, &appWindowHwnd));

        auto compositor = ref new Microsoft::UI::Composition::Compositor();

        // The scene is like this:
        //           appWindow (AppWindow)
        //              |
        //          siteBridge (DesktopChildSiteBridge) 
        //             | |                              
        //          rootIsland (ContentIsland)                              <---
        //              |                                                      |
        //             root (ContainerVisual)                                  |
        //            /                \                                       |-- link (ChildSiteLink)
        // spriteVisual (SpriteVisual)  placementVisual (ContainerVisual)   <--|
        //                                   | |                               |
        //                               xamlIsland (XamlIsland)           <----
        
        auto siteBridge = DesktopChildSiteBridge::Create(compositor, appWindow->Id);
        siteBridge->ResizePolicy = ContentSizePolicy::ResizeContentToParentWindow;
        siteBridge->Show();

        ABI::Microsoft::UI::GetWindowFromWindowId({siteBridge->WindowId.Value}, &desktopChildSiteBridgeHwnd);

        auto root = compositor->CreateContainerVisual();
        rootIsland = ContentIsland::Create(root);

        auto spriteVisual = compositor->CreateSpriteVisual();
        spriteVisual->Offset = ::Windows::Foundation::Numerics::float3(20, 20, 0);
        spriteVisual->Size = ::Windows::Foundation::Numerics::float2(1000, 1000);        
        spriteVisual->Brush = compositor->CreateColorBrush(::Windows::UI::ColorHelper::FromArgb(255, 180, 250, 180));
        root->Children->InsertAtTop(spriteVisual);

        auto placementVisual = compositor->CreateContainerVisual();
        placementVisual->Offset = ::Windows::Foundation::Numerics::float3(100, 140, 0);
        placementVisual->Size = ::Windows::Foundation::Numerics::float2(300, 200);
        root->Children->InsertAtTop(placementVisual);

        siteBridge->Connect(rootIsland);

        xamlIsland = ref new XamlIsland();
        auto grid = ref new Grid();
        grid->Background = ref new SolidColorBrush(::Windows::UI::ColorHelper::FromArgb(255, 180, 180, 255));
        auto b = ref new Button();
        b->Content = "I am a xaml button.";
        b->Margin = ThicknessHelper::FromLengths(0, 150.0, 0, 0); // push button downward.
        grid->Children->Append(b);
        xamlIsland->Content = grid;

        link = ChildSiteLink::Create(rootIsland, placementVisual);
        link->ActualSize = ::Windows::Foundation::Numerics::float2(300, 200);
        link->AutomationOption = Microsoft::UI::Content::ContentAutomationOptions::FragmentBased;

        //ixp::InputKeyboardSource^ inputKeyboardSource = ixp::InputKeyboardSource::GetForIsland(rootIsland);
        //inputFocusNavigationHost = ixp::InputFocusNavigationHost::GetForSiteLink(link);

        if (IsTestParameterSet(L"UseNonXamlIsland"))
        {
            // Use a dummy non-Xaml island instead of a Xaml Island
            auto islandRootVisual = compositor->CreateContainerVisual();
            auto dummyIsland = ContentIsland::Create(islandRootVisual);
            auto pointerSource = InputPointerSource::GetForIsland(dummyIsland);
            auto keySource = InputKeyboardSource::GetForIsland(dummyIsland);
            link->Connect(dummyIsland);
        }
        else
        {
            link->Connect(xamlIsland->ContentIsland);
        }

        // Scene Graph setup is complete!


        // Now, set up a UIA tree.  We'll make it like this:
        //
        //         rootNode             <-- FragmentRoot
        //            |
        //          node1
        //      /     |       \
        // node2      |        node3
        //        xamlIsland            <-- Xaml's CUIAWindow implements this
        //            |
        //        Xaml Button           <-- Xaml's CUIAWrapper implements this
        //
        // Here's how they're arranged on the screen (not to scale):
        //  ---------------------------------------------------------------------------------
        //  |  rootNode: 20,20 1000x1000, node1: 20,20 560x380
        //  |  -----------------                              ------------------
        //  |  | node2: 40,40  | ---------------------------- | node3: 410,40  |
        //  |  |        50x300 | | link: 100,140 300x200    | |        50x300  |
        //  |  |               | | xamlIsland: same         | |                |
        //  |  |               | |                          | |                |
        //  |  |               | |                          | |                |
        //  |  ----------------- ---------------------------- ------------------
        //  |
        //  |

        auto rootNode = Microsoft::WRL::Make<TestAutomationNode>();
        rootNode->m_id = 900;
        rootNode->m_name = L"RootNode";
        rootNode->m_island = rootIsland;
        rootNode->m_isRoot = true;        
        rootNode->m_root = rootNode;
        rootNode->SetBoundingRectangle(Rect(20, 20, 1000, 1000));

        auto node1 = Microsoft::WRL::Make<TestAutomationNode>();
        node1->m_id = 901;
        node1->m_name = L"Node1";
        node1->m_island = rootIsland;        
        node1->m_root = rootNode;
        node1->m_parent = rootNode;
        node1->SetBoundingRectangle( Rect(20, 20, 560, 380));
        rootNode->m_children.push_back(node1);        

        auto node2 = Microsoft::WRL::Make<TestAutomationNode>();
        node2->m_id = 902;
        node2->m_name = L"Node2";
        node2->m_island = rootIsland;
        node2->m_root = rootNode;
        node2->m_parent = node1;
        node2->SetBoundingRectangle( Rect(40, 40, 50, 300));
        node1->m_children.push_back(node2);

        link->LocalToParentTransformMatrix = ::Windows::Foundation::Numerics::make_float3x2_translation(100, 140);
        node1->m_children.push_back(CastHatTo<IRawElementProviderFragment>(link->AutomationProvider));

        auto node3 = Microsoft::WRL::Make<TestAutomationNode>();
        node3->m_id = 904;
        node3->m_name = L"Node3";
        node3->m_island = rootIsland;        
        node3->m_root = rootNode;
        node3->m_parent = node1;
        node3->SetBoundingRectangle(Rect(410, 40, 50, 300));
        node1->m_children.push_back(node3);

        automationProviderRequestedToken = rootIsland->AutomationProviderRequested +=
            ref new TypedEventHandler<ContentIsland^, ContentIslandAutomationProviderRequestedEventArgs^>(
                [=](ContentIsland^ sender, ContentIslandAutomationProviderRequestedEventArgs^ args) {
            LOG_UIA(L"[Root provider requested]");
            Microsoft::WRL::ComPtr<IInspectable> insp;
            rootNode.As(&insp);
            args->AutomationProvider = reinterpret_cast<Object^>(insp.Detach());
            args->Handled = true;
        });

        fragmentRootAutomationProviderRequestedToken = link->FragmentRootAutomationProviderRequested +=
            ref new TypedEventHandler<IContentSiteAutomation ^, ContentSiteAutomationProviderRequestedEventArgs ^>(
                [=](IContentSiteAutomation ^ sender, ContentSiteAutomationProviderRequestedEventArgs ^ args) {
            LOG_UIA(L"[Link FragmentRootAutomationProviderRequested]");
            args->AutomationProvider = ToObject(rootNode.Get());
            args->Handled = true;
        });

        parentAutomationProviderRequestedToken = link->ParentAutomationProviderRequested +=
            ref new TypedEventHandler<IContentSiteAutomation ^, ContentSiteAutomationProviderRequestedEventArgs ^>(
                [=](IContentSiteAutomation ^ sender, ContentSiteAutomationProviderRequestedEventArgs ^ args) {
            LOG_UIA(L"[Link ParentAutomationProviderRequested]");
            args->AutomationProvider = ToObject(node1.Get());
            args->Handled = true;
        });

        nextSiblingAutomationProviderRequestedToken = link->NextSiblingAutomationProviderRequested +=
            ref new TypedEventHandler<IContentSiteAutomation ^, ContentSiteAutomationProviderRequestedEventArgs ^>(
                [=](IContentSiteAutomation ^ sender, ContentSiteAutomationProviderRequestedEventArgs ^ args) {
            LOG_UIA(L"[Link NextSiblingAutomationProviderRequested]");
            args->AutomationProvider = ToObject(node3.Get());
            args->Handled = true;
        });

        previousSiblingAutomationProviderRequestedToken = link->PreviousSiblingAutomationProviderRequested +=
            ref new TypedEventHandler<IContentSiteAutomation ^, ContentSiteAutomationProviderRequestedEventArgs ^>(
                [=](IContentSiteAutomation ^ sender, ContentSiteAutomationProviderRequestedEventArgs ^ args) {
            LOG_UIA(L"[Link PreviousSiblingAutomationProviderRequested]");
            args->AutomationProvider = ToObject(node2.Get());
            args->Handled = true;
        });

    }

    void Cleanup()
    {
        rootIsland->AutomationProviderRequested -= automationProviderRequestedToken;
        link->FragmentRootAutomationProviderRequested -= fragmentRootAutomationProviderRequestedToken;
        link->ParentAutomationProviderRequested -= parentAutomationProviderRequestedToken;
        link->NextSiblingAutomationProviderRequested -= nextSiblingAutomationProviderRequestedToken;
        link->PreviousSiblingAutomationProviderRequested -= previousSiblingAutomationProviderRequestedToken;
    }

    AppWindow^ appWindow;
    HWND appWindowHwnd {NULL};
    HWND desktopChildSiteBridgeHwnd {NULL};
    ContentIsland^ rootIsland;
    ChildSiteLink^ link;
    XamlIsland^ xamlIsland;
    ixp::InputFocusNavigationHost^ inputFocusNavigationHost;
    wf::EventRegistrationToken automationProviderRequestedToken;
    wf::EventRegistrationToken fragmentRootAutomationProviderRequestedToken;
    wf::EventRegistrationToken parentAutomationProviderRequestedToken;
    wf::EventRegistrationToken nextSiblingAutomationProviderRequestedToken;
    wf::EventRegistrationToken previousSiblingAutomationProviderRequestedToken;

};

bool WindowlessXamlIslandTests::TestSetup()
{
    // Need a better way to do this. This breaks between every test. Without it, we don't get a chance to attach at all.
    // Do this during ClassSetup?
    WaitForDebugger();
    return true;
}

bool WindowlessXamlIslandTests::TestCleanup()
{
    return true;
}

void WindowlessXamlIslandTests::ValidateUiaTree()
{
    VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_SINGLETHREADED));

    DispatcherQueue^ uiThreadDq;
    Event uiThreadReady;
    WindowlessXamlIslandScene windowlessXamlIslandScene;

    auto uiThread = RunOnNewThread([&]() {
        VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_SINGLETHREADED));
        auto dqc{DispatcherQueueController::CreateOnCurrentThread()};

        auto app { ref new XamlIslandTests_ApplicationWithMuxc()};

        windowlessXamlIslandScene.Create(dqc->DispatcherQueue);

        uiThreadDq = dqc->DispatcherQueue;
        uiThreadReady.Set();

        dqc->DispatcherQueue->RunEventLoop();

        windowlessXamlIslandScene.Cleanup();

        dqc->ShutdownQueue();
        app->Cleanup();

        LOG_OUTPUT(L"  Exiting UI thread...");
    });

    uiThreadReady.WaitForDefault();

    // Make sure the scene is visible.
    ::Sleep(500);

    // Run with "/p:DontTest" to pause if you just want to observe the scene and not run tests.
    if (IsTestParameterSet(L"DontTest"))
    {
        WaitForSingleObjectWithTimeout(uiThread, INFINITE);
        return;
    }

    uiThreadDq->TryEnqueue(
        Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal,
        ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([&](){
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"RootNode";

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo, windowlessXamlIslandScene.appWindowHwnd);

            wrl::ComPtr<IUIAutomation> spUIAutomation;
            spAutomationClientManager->GetAutomation(&spUIAutomation);

            wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
            spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition);

            wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
            spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker);

            wrl::ComPtr<IUIAutomationElement> spRootElement;
            spAutomationClientManager->GetCurrentUIAutomationElement(&spRootElement);

            std::wstringstream ss;
            WalkUiaTreeDepthFirst(ss, spUIAutomationTreeWalker.Get(), spRootElement.Get());

            // There are two UIA trees we accept.
            // The first is the behavior of WinAppSDK 1.7 exp3
            // The second is how we expect to ship 1.7.0 
            // So we can eventually remove the first one.

            constexpr const wchar_t* expectedUiaTree1 = 
L"Node Name=[RootNode] Class=[TestClass] ParentName=[(null)]\n"
L"  Node Name=[(null)] Class=[InputSiteWindowClass] ParentName=[RootNode]\n"
L"  Node Name=[Node1] Class=[TestClass] ParentName=[RootNode]\n"
L"    Node Name=[Node2] Class=[TestClass] ParentName=[Node1]\n"
L"    Node Name=[] Class=[] ParentName=[Node1]\n"
L"      Node Name=[I am a xaml button.] Class=[Button] ParentName=[]\n"
L"        Node Name=[I am a xaml button.] Class=[TextBlock] ParentName=[I am a xaml button.]\n"
L"    Node Name=[Node3] Class=[TestClass] ParentName=[Node1]\n";

            constexpr const wchar_t* expectedUiaTree2 = 
L"Node Name=[RootNode] Class=[TestClass] ParentName=[(null)]\n"
L"  Node Name=[Node1] Class=[TestClass] ParentName=[RootNode]\n"
L"    Node Name=[Node2] Class=[TestClass] ParentName=[Node1]\n"
L"    Node Name=[] Class=[] ParentName=[Node1]\n"
L"      Node Name=[I am a xaml button.] Class=[Button] ParentName=[]\n"
L"        Node Name=[I am a xaml button.] Class=[TextBlock] ParentName=[I am a xaml button.]\n"
L"    Node Name=[Node3] Class=[TestClass] ParentName=[Node1]\n";

            const bool isExpectedUiaTree1 = TestStringsEqual(expectedUiaTree1, ss.str());
            const bool isExpectedUiaTree2 = TestStringsEqual(expectedUiaTree2, ss.str());

            VERIFY_IS_TRUE(isExpectedUiaTree1 || isExpectedUiaTree2);

            LOG_OUTPUT(L"WARNING: The following tests assume DPI 100.");

            auto testElementFromPoint = [=](int x, int y, const wchar_t* expected, bool onlyWarn = false) {
                wrl::ComPtr<IUIAutomationElement> hitTestElement;
                spUIAutomation->ElementFromPoint({x, y}, &hitTestElement);
                LOG_OUTPUT(L"Calling ElementFromPoint for %d,%d:", x, y);
                auto elementStr = GetElementString(spUIAutomationTreeWalker.Get(), hitTestElement.Get());
                LOG_OUTPUT(L"  Expected: %s", expected);
                LOG_OUTPUT(L"  Hit:      %s", elementStr.c_str());
                if (onlyWarn)
                {
                    LOG_OUTPUT(L"WARNING: This is a known issue, continuing.");   
                }
                else
                {
                    VERIFY_IS_TRUE(TestStringsEqual(expected, elementStr));
                }

                std::wstringstream ss;
                WalkUiaTreeUpward(ss, spUIAutomationTreeWalker.Get(), hitTestElement.Get(), L"ParentName=[WindowlessXamlIslandTest]");
                LOG_OUTPUT(L"Upward tree:");
                LOG_OUTPUT(ss.str().c_str());
            };

            testElementFromPoint(122, 394, L"Node Name=[Node2] Class=[TestClass] ParentName=[Node1]");
            testElementFromPoint(227, 396, L"Node Name=[I am a xaml button.] Class=[Button] ParentName=[]");
            testElementFromPoint(309, 345, L"Node Name=[] Class=[] ParentName=[Node1]");
            testElementFromPoint(489, 358, L"Node Name=[Node3] Class=[TestClass] ParentName=[Node1]");

            // TODO: Investigate why our UIA hit test sometimes hits an InputSiteWindow
            testElementFromPoint(115, 261, L"Node Name=[Node2] Class=[TestClass] ParentName=[Node1]", true /*onlyWarn*/);
        }));

    LowBudgetWaitForIdle(uiThreadDq);

    uiThreadDq->TryEnqueue(
        Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal,
        ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([&](){
            LOG_OUTPUT(L"  Destroying appWindow...");
            windowlessXamlIslandScene.appWindow->Destroy();
        }));

    WaitForSingleObjectWithTimeout(uiThread);

}

void WindowlessXamlIslandTests::ValidateTabNavigation()
{
    VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_SINGLETHREADED));

    DispatcherQueue^ uiThreadDq;
    Event uiThreadReady;
    WindowlessXamlIslandScene windowlessXamlIslandScene;

    auto uiThread = RunOnNewThread([&]() {
        VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_SINGLETHREADED));
        auto dqc{DispatcherQueueController::CreateOnCurrentThread()};

        auto app { ref new XamlIslandTests_ApplicationWithMuxc()};

        windowlessXamlIslandScene.Create(dqc->DispatcherQueue);

        uiThreadDq = dqc->DispatcherQueue;
        uiThreadReady.Set();

        dqc->DispatcherQueue->RunEventLoop();

        windowlessXamlIslandScene.Cleanup();

        dqc->ShutdownQueue();
        app->Cleanup();

        LOG_OUTPUT(L"  Exiting UI thread...");
    });

    uiThreadReady.WaitForDefault();

    // Make sure the scene is visible.
    ::Sleep(500);


    LOG_OUTPUT(L"Send tab.");
    SendTab();

    LowBudgetWaitForIdle(uiThreadDq);

    uiThreadDq->TryEnqueue(
        Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal,
        ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([&](){
            LOG_OUTPUT(L"NavigateFocus to ChildSiteLink");
            FocusNavigationRequest^ request = FocusNavigationRequest::Create(FocusNavigationReason::First);
            windowlessXamlIslandScene.inputFocusNavigationHost->NavigateFocus(request);
        }));

    // Run with "/p:DontTest" to pause if you just want to observe the scene and not run tests.
    if (IsTestParameterSet(L"DontTest"))
    {
        WaitForSingleObjectWithTimeout(uiThread, INFINITE);
        return;
    }

    LowBudgetWaitForIdle(uiThreadDq);

    uiThreadDq->TryEnqueue(
        Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal,
        ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([&](){
            LOG_OUTPUT(L"  Destroying appWindow...");
            windowlessXamlIslandScene.appWindow->Destroy();
        }));

    WaitForSingleObjectWithTimeout(uiThread);

}

void WindowlessXamlIslandTests::ValidateDatePickerTimePickerDontCrash()
{
    VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_SINGLETHREADED));

    DispatcherQueue^ uiThreadDq;
    Event uiThreadReady;
    WindowlessXamlIslandScene windowlessXamlIslandScene;

    auto uiThread = RunOnNewThread([&]() {
        VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_SINGLETHREADED));
        auto dqc{DispatcherQueueController::CreateOnCurrentThread()};

        auto app { ref new XamlIslandTests_ApplicationWithMuxc()};

        windowlessXamlIslandScene.Create(dqc->DispatcherQueue);

        uiThreadDq = dqc->DispatcherQueue;
        uiThreadReady.Set();

        dqc->DispatcherQueue->RunEventLoop();

        windowlessXamlIslandScene.Cleanup();

        dqc->ShutdownQueue();
        app->Cleanup();

        LOG_OUTPUT(L"  Exiting UI thread...");
    });

    uiThreadReady.WaitForDefault();

    // Make sure the scene is visible.
    ::Sleep(500);

    LowBudgetWaitForIdle(uiThreadDq);

    uiThreadDq->TryEnqueue(
        Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal,
        ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([&](){
            windowlessXamlIslandScene.xamlIsland->Content = ref new DatePicker();
        }));
    LowBudgetWaitForIdle(uiThreadDq);

    uiThreadDq->TryEnqueue(
        Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal,
        ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([&](){
            windowlessXamlIslandScene.xamlIsland->Content = ref new TimePicker();
        }));
    LowBudgetWaitForIdle(uiThreadDq);

    uiThreadDq->TryEnqueue(
        Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal,
        ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([&](){
            LOG_OUTPUT(L"  Destroying appWindow...");
            windowlessXamlIslandScene.appWindow->Destroy();
        }));

    WaitForSingleObjectWithTimeout(uiThread);

}

void WindowlessXamlIslandTests::ValidateWebView2DoesntCrash()
{
    VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_SINGLETHREADED));

    DispatcherQueue^ uiThreadDq;
    Event uiThreadReady;
    WindowlessXamlIslandScene windowlessXamlIslandScene;

    auto uiThread = RunOnNewThread([&]() {
        VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_SINGLETHREADED));
        auto dqc{DispatcherQueueController::CreateOnCurrentThread()};

        auto app { ref new XamlIslandTests_ApplicationWithMuxc()};

        windowlessXamlIslandScene.Create(dqc->DispatcherQueue);

        uiThreadDq = dqc->DispatcherQueue;
        uiThreadReady.Set();

        dqc->DispatcherQueue->RunEventLoop();

        windowlessXamlIslandScene.Cleanup();

        dqc->ShutdownQueue();
        app->Cleanup();

        LOG_OUTPUT(L"  Exiting UI thread...");
    });

    uiThreadReady.WaitForDefault();

    // Make sure the scene is visible.
    ::Sleep(500);

    LowBudgetWaitForIdle(uiThreadDq);

    uiThreadDq->TryEnqueue(
        Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal,
        ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([&](){
            LOG_OUTPUT(L"  Creating WebView2");
            WebView2^ wv2 = ref new WebView2();
            windowlessXamlIslandScene.xamlIsland->Content = wv2;
            wv2->Source = ref new wf::Uri("https://bing.com");
        }));
    LowBudgetWaitForIdle(uiThreadDq);

    // Ensure WebView2 has some time to load and render
    ::Sleep(500);

    uiThreadDq->TryEnqueue(
        Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal,
        ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([&](){
            LOG_OUTPUT(L"  Destroying appWindow...");
            windowlessXamlIslandScene.appWindow->Destroy();
        }));

    WaitForSingleObjectWithTimeout(uiThread);

}


} // namespace Microsoft::UI::Xaml::Tests::Foundation::Hosting

// Return a string that describes an automation element
std::wstring GetElementString(IUIAutomationTreeWalker* walker, IUIAutomationElement* node)
{
    BSTR m_name{};
    node->get_CurrentName(&m_name);

    wrl::ComPtr<IUIAutomationElement> parent;
    walker->GetParentElement(node, &parent);

    BSTR parentName{};
    if (parent)
    {
        parent->get_CurrentName(&parentName);
    }
    else
    {
        parentName = SysAllocString(L"(none)");
    }

    BSTR className{};
    node->get_CurrentClassName(&className);

    std::wstringstream ss;
    ss  << L"Node Name=["
        << (m_name ? m_name : L"(null)")
        << L"] Class=["
        << (className ? className : L"(null)")
        << L"] ParentName=[" 
        << (parentName  ? parentName : L"(null)")
        << L"]";

    SysFreeString(m_name);
    SysFreeString(parentName);
    SysFreeString(className);

    return ss.str();
}

void WalkUiaTreeDepthFirst(std::wstringstream& ss, IUIAutomationTreeWalker* walker, IUIAutomationElement* node, int depth)
{
    std::wstring elementStr = GetElementString(walker, node);
    ss << std::wstring(depth * 2, L' ') << elementStr << "\n";

    wrl::ComPtr<IUIAutomationElement> child;

    walker->GetFirstChildElement(node, &child);
    
    while (true)
    {
        if (child == nullptr)
        {
            break;
        }

        {
            // Validate child's parent is this node
            wrl::ComPtr<IUIAutomationElement> childParent;
            walker->GetParentElement(child.Get(), &childParent);
            std::wstring childParentStr = GetElementString(walker, childParent.Get());
            if (wcscmp(childParentStr.c_str(), elementStr.c_str()) != 0)
            {
                LOG_OUTPUT(L"Parent mismatch: %s != %s", childParentStr.c_str(), elementStr.c_str());
                VERIFY_IS_TRUE(false);
            }
        }

        WalkUiaTreeDepthFirst(ss, walker, child.Get(),depth+1);

        wrl::ComPtr<IUIAutomationElement> nextChild;
        walker->GetNextSiblingElement(child.Get(), &nextChild);
        child = nextChild;
    }
}

void WalkUiaTreeUpward(std::wstringstream& ss, IUIAutomationTreeWalker* walker, IUIAutomationElement* node, const wchar_t* stopNode, int depth)
{
    auto elementStr = GetElementString(walker, node);
    ss << std::wstring(depth * 2, L' ') << elementStr << "\n";

    // Stop if elementStr contains stopNode
    if (wcsstr(elementStr.c_str(), stopNode) != nullptr)
    {
        return;
    }

    wrl::ComPtr<IUIAutomationElement> parent;
    walker->GetParentElement(node, &parent);

    if (parent != nullptr)
    {
        WalkUiaTreeUpward(ss, walker, parent.Get(), stopNode, depth + 1);
    }
}

// multiline strings OK.
bool TestStringsEqual(const wchar_t* expected, const std::wstring& observed)
{
    if (wcscmp(expected, observed.c_str()) == 0)
    {
        LOG_OUTPUT(L"String matches:");
        LOG_OUTPUT(expected);
        return true;
    }

    LOG_OUTPUT(L"Strings do not match!");
    LOG_OUTPUT(L"Expected string:");
    LOG_OUTPUT(expected);
    LOG_OUTPUT(L"Observed string:");
    LOG_OUTPUT(observed.c_str());

    int row = 0;
    int col = 0;
    for (int i=0;;i++)
    {
        if (observed[i] != expected[i])
        {
            LOG_OUTPUT(L"  (Mismatch at index %d (%d, %d): %c vs %c)", i, row, col, expected[i], observed[i]);
            break;
        }
        if (expected[i] == L'\n')
        {
            row++;
            col = 0;
        }
        else
        {
            col++;
        }
        if (expected[i] == L'\0' || observed[i] == L'\0')
        {
            break;
        }
    }
    return false;
}
