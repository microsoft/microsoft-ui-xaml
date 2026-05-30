// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ReplayPointerUpdateTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <FileLoader.h>
#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <WUCRenderingScopeGuard.h>
#include <windowsnumerics.h>


using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Composition;
using namespace ::Windows::Foundation::Numerics;

using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Input { namespace Pointer {

Platform::String^ ReplayPointerUpdateTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\foundation\\input\\pointer\\";
}

bool ReplayPointerUpdateTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool ReplayPointerUpdateTests::TestSetup()
{
    TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool ReplayPointerUpdateTests::TestCleanup()
{
    TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void ReplayPointerUpdateTests::ReplayPointerUpdate()
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    std::shared_ptr<Event> btn2EnteredEvent = std::make_shared<Event>();
    std::shared_ptr<Event> btn3EnteredEvent = std::make_shared<Event>();

    StackPanel^ rootSP = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ReplayPointerUpdate.xaml"));
    Button^ btn1;
    Button^ btn2;
    Button^ btn3;
    wf::EventRegistrationToken btn2EnteredToken;
    wf::EventRegistrationToken btn3EnteredToken;
    RunOnUIThread([&]()
    {
        wh->WindowContent = rootSP;
    });
    wh->SynchronouslyTickUIThread(2);

    RunOnUIThread([&]()
    {
        btn1 = safe_cast<Button^>(rootSP->FindName(L"btn1"));
        btn2 = safe_cast<Button^>(rootSP->FindName(L"btn2"));
        btn3 = safe_cast<Button^>(rootSP->FindName(L"btn3"));
    });

    TestServices::InputHelper->MoveMouse(btn1);
    wh->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"hover1").GetString());

    RunOnUIThread([&]()
    {
        btn2EnteredToken = btn2->PointerEntered += ref new PointerEventHandler([&](Platform::Object^ sender, PointerRoutedEventArgs^ args)
        {
            btn2EnteredEvent->Set();
        });
        rootSP->Children->RemoveAt(0);
    });
    wh->RequestReplayPreviousPointerUpdate_TempTestHook();
    btn2EnteredEvent->WaitForDefault();
    wh->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"hover2").GetString());

    RunOnUIThread([&]()
    {
        btn3EnteredToken = btn3->PointerEntered += ref new PointerEventHandler([&](Platform::Object^ sender, PointerRoutedEventArgs^ args)
        {
            btn3EnteredEvent->Set();
        });
        rootSP->Children->RemoveAt(0);
    });
    wh->RequestReplayPreviousPointerUpdate_TempTestHook();
    btn3EnteredEvent->WaitForDefault();
    wh->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, Platform::StringReference(L"hover3").GetString());

    RunOnUIThread([&]()
    {
        btn2->PointerEntered -= btn2EnteredToken;
        btn3->PointerEntered -= btn3EnteredToken;
    });
}

void ReplayPointerUpdateTests::AutoPointerUpdateReplay()
{
    const auto& wh = TestServices::WindowHelper;

    wh->SetWindowSizeOverride(wf::Size(400, 600));
    TestCleanupWrapper cleanup;

    const int buttonCount = 15;
    const float buttonHeight = 75.0;
    const float buttonWidth = 150.0;
    struct {
        Button^ button;
        wf::EventRegistrationToken enterToken = {};
        wf::EventRegistrationToken exitToken = {};
        bool isEntered = false;
    } buttonInfo[buttonCount];
    ScrollViewer^ scrollViewer;
    float3 button3InitialOffset;
    Visual^ button3Visual;

    std::shared_ptr<Event> btnEnteredEvent = std::make_shared<Event>();

    LOG_OUTPUT(L"Constructing Xaml Scene");
    RunOnUIThread([&]()
    {
        Grid^ root = ref new Grid();

        scrollViewer = ref new ScrollViewer();
        scrollViewer->HorizontalAlignment = HorizontalAlignment::Left;
        scrollViewer->VerticalAlignment = VerticalAlignment::Top;
        scrollViewer->VerticalScrollMode = ScrollMode::Auto;
        root->Children->Append(scrollViewer);

        StackPanel^ buttonPanel = ref new StackPanel();
        scrollViewer->Content = buttonPanel;

        for (int i = 0; i < buttonCount; i++)
        {
            Button^ button = ref new Button();
            button->Content = ref new Platform::String(WEX::Common::String().Format(L"Button %d", i));
            button->Height = buttonHeight;
            button->Width = buttonWidth;
            buttonPanel->Children->Append(button);

            buttonInfo[i].button = button;
            buttonInfo[i].enterToken = button->PointerEntered += ref new PointerEventHandler([=,&buttonInfo](Platform::Object^ sender, PointerRoutedEventArgs^ args)
            {
                LOG_OUTPUT(WEX::Common::String().Format(L"Button %d Entered", i));
                buttonInfo[i].isEntered = true;
                btnEnteredEvent->Set();
            });
            buttonInfo[i].exitToken = button->PointerExited += ref new PointerEventHandler([=, &buttonInfo](Platform::Object^ sender, PointerRoutedEventArgs^ args)
            {
                LOG_OUTPUT(WEX::Common::String().Format(L"Button %d Exited", i));
                buttonInfo[i].isEntered = false;
            });
         }
        wh->WindowContent = root;

        // We need to "prefetch" the handoff visual since if we wait till we need it, the creation of the compnode
        // on the next tick will mess with our values.
        button3Visual = Microsoft::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(buttonInfo[3].button);
    });
    wh->WaitForIdle();

    TestServices::InputHelper->MoveMouse(buttonInfo[1].button);
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Verifying that layout modifications will replay pointer");

    LOG_OUTPUT(L"  > Move mouse over button 2");
    TestServices::InputHelper->MoveMouse(buttonInfo[2].button);
    wh->WaitForIdle();

    btnEnteredEvent->Reset();
    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(buttonInfo[2].isEntered);
        VERIFY_IS_TRUE(buttonInfo[2].button->IsPointerOver);

        LOG_OUTPUT(L"  > Collapsing the first button");
        buttonInfo[0].button->Visibility = Visibility::Collapsed;
    });
    btnEnteredEvent->WaitForDefault();
    wh->WaitForIdle();

    btnEnteredEvent->Reset();
    RunOnUIThread([&]()
    {
        VERIFY_IS_FALSE(buttonInfo[2].isEntered);
        VERIFY_IS_FALSE(buttonInfo[2].button->IsPointerOver);
        VERIFY_IS_TRUE(buttonInfo[3].isEntered);
        VERIFY_IS_TRUE(buttonInfo[3].button->IsPointerOver);

        // Reset back to original state
        LOG_OUTPUT(L"  > Restoring the first button");
        buttonInfo[0].button->Visibility = Visibility::Visible;
    });
    btnEnteredEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(buttonInfo[2].isEntered);
        VERIFY_IS_TRUE(buttonInfo[2].button->IsPointerOver);
        VERIFY_IS_FALSE(buttonInfo[3].isEntered);
        VERIFY_IS_FALSE(buttonInfo[3].button->IsPointerOver);
    });

    LOG_OUTPUT(L"> Verifying that handoff visual transform changes will trigger Replay");

    btnEnteredEvent->Reset();
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Getting Handoff Visual for button 3");
        button3InitialOffset = button3Visual->Offset;
        LOG_OUTPUT(L"  > Setting Offset");
        button3Visual->Offset = float3(button3InitialOffset.x,  button3InitialOffset.y - buttonHeight, button3InitialOffset.z);
    });
    btnEnteredEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        VERIFY_IS_FALSE(buttonInfo[2].isEntered);
        VERIFY_IS_FALSE(buttonInfo[2].button->IsPointerOver);
        VERIFY_IS_TRUE(buttonInfo[3].isEntered);
        VERIFY_IS_TRUE(buttonInfo[3].button->IsPointerOver);
    });

    LOG_OUTPUT(L"> Verifying that animations will trigger Replay");

    btnEnteredEvent->Reset();
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Creating animation");
        Vector3KeyFrameAnimation ^ animation = button3Visual->Compositor->CreateVector3KeyFrameAnimation();
        animation->InsertKeyFrame(0.0f, float3(button3InitialOffset.x, button3InitialOffset.y - buttonHeight, button3InitialOffset.z));
        animation->InsertKeyFrame(1.0f, float3(button3InitialOffset.x, button3InitialOffset.y, button3InitialOffset.z));

        ::Windows::Foundation::TimeSpan timeSpan;
        timeSpan.Duration = 3000000L;
        animation->Duration = timeSpan;
        LOG_OUTPUT(L"  > Starting animation on button3");
        button3Visual->StartAnimation(L"Offset", animation);
    });

    btnEnteredEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(buttonInfo[2].isEntered);
        VERIFY_IS_TRUE(buttonInfo[2].button->IsPointerOver);
        VERIFY_IS_FALSE(buttonInfo[3].isEntered);
        VERIFY_IS_FALSE(buttonInfo[3].button->IsPointerOver);

        // Reset back to original state
        LOG_OUTPUT(L"  > Resetting button 3");
        button3Visual->StopAnimation(L"Offset");
        button3Visual->Offset = float3(button3InitialOffset.x, button3InitialOffset.y, button3InitialOffset.z);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Verify scrolling will trigger replay");

    LOG_OUTPUT(L"  > Move mouse over first button");
    btnEnteredEvent->Reset();
    TestServices::InputHelper->MoveMouse(buttonInfo[0].button);
    btnEnteredEvent->WaitForDefault();

    btnEnteredEvent->Reset();
    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(buttonInfo[0].isEntered);
        VERIFY_IS_TRUE(buttonInfo[0].button->IsPointerOver);
        LOG_OUTPUT(L"  > Scrolling down 5 buttons via ChangeView");
        scrollViewer->ChangeView(nullptr /*horizontalOffset*/, static_cast<double>(buttonHeight * 5.0f), nullptr /*zoomFactor*/, false /*disableAnimation*/);
    });
    btnEnteredEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(buttonInfo[5].isEntered);
        VERIFY_IS_TRUE(buttonInfo[5].button->IsPointerOver);
        VERIFY_IS_FALSE(buttonInfo[0].isEntered);
        VERIFY_IS_FALSE(buttonInfo[0].button->IsPointerOver);
    });

    LOG_OUTPUT(L"  > Use Mousewheel to return to top of button list");
    btnEnteredEvent->Reset();
    TestServices::InputHelper->ScrollMouseWheel(buttonInfo[5].button, 200);
    btnEnteredEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(buttonInfo[0].isEntered);
        VERIFY_IS_TRUE(buttonInfo[0].button->IsPointerOver);
        VERIFY_IS_FALSE(buttonInfo[5].isEntered);
        VERIFY_IS_FALSE(buttonInfo[5].button->IsPointerOver);
    });

    RunOnUIThread([&]()
    {
        for (int i = 0; i < buttonCount; i++)
        {
            if (buttonInfo[i].button == nullptr) continue;
            buttonInfo[i].button->PointerEntered -= buttonInfo[i].enterToken;
            buttonInfo[i].button->PointerExited -= buttonInfo[i].exitToken;
            buttonInfo[i].button = nullptr;
        }
    });
    wh->WaitForIdle();
}

void ReplayPointerUpdateTests::AfterKeyboardInput()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;

    StackPanel^ rootSP = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ReplayPointerUpdate.xaml"));
    Button^ btn1;
    Button^ btn2;
    TextBox^ textbox;
     XamlRoot^ xamlRoot = nullptr;

    RunOnUIThread([&]()
    {
        wh->WindowContent = rootSP;
    });
    wh->SynchronouslyTickUIThread(2);

    auto clickEvent = std::make_shared<Event>();
    auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
    auto btnEnteredEvent = std::make_shared<Event>();
    auto btnEnteredRegistration = CreateSafeEventRegistration(xaml_controls::Button, PointerEntered);
    auto btnMovedEvent = std::make_shared<Event>();
    auto btnMovedRegistration = CreateSafeEventRegistration(xaml_controls::Button, PointerMoved);

    RunOnUIThread([&]()
    {
        btn1 = safe_cast<Button^>(rootSP->FindName(L"btn1"));
        btn2 = safe_cast<Button^>(rootSP->FindName(L"btn2"));
        clickRegistration.Attach(btn1, [clickEvent]() {clickEvent->Set(); });
        btnEnteredRegistration.Attach(btn2, [&]() {btnEnteredEvent->Set(); });
        btnMovedRegistration.Attach(btn1, [&]() {btnMovedEvent->Set(); });
        xamlRoot = btn1->XamlRoot;
    });
    wh->WaitForIdle();

    TestServices::InputHelper->MoveMouse(btn1);
    wh->WaitForIdle();

    // Approximately one second after a pointer input, the system will generate another dummy pointer input at the same location to
    // facilitate hover.  We need to make sure that we wait for that mouse input to pass before we start checking last input 
    // devices because if it comes during our test sequence it will change the last input device and we won't get a valid test.
    btnMovedEvent->Reset();
    if (btnMovedEvent->WaitForNoThrow())
    {
        LOG_OUTPUT(L"Ignored generated pointer event");
    }
        
    RunOnUIThread([&]()
    {
        btn1->Focus(FocusState::Programmatic);
    });

    VERIFY_ARE_EQUAL(wh->GetLastInputMethod(xamlRoot), test_infra::LastInputDeviceType::Mouse);

    TestServices::KeyboardHelper->Enter();
    clickEvent->WaitForDefault();

    VERIFY_ARE_EQUAL(wh->GetLastInputMethod(xamlRoot), test_infra::LastInputDeviceType::Keyboard);

    LOG_OUTPUT(L"Hide first button to generate pointer replay");
    RunOnUIThread([&]()
    {
        btn1->Visibility = Visibility::Collapsed;
    });

    LOG_OUTPUT(L"Wait for the second button to get a pointer over indicating the event was replayed");
    btnEnteredEvent->WaitForDefault();

    VERIFY_ARE_EQUAL(wh->GetLastInputMethod(xamlRoot), test_infra::LastInputDeviceType::Keyboard);

}
} } } } } } }
