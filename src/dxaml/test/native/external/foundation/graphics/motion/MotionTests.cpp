// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "MotionTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <FileLoader.h>
#include <TestCleanupWrapper.h>
#include <WUCRenderingScopeGuard.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <DisableRenderingScopeGuard.h>

#include "FeatureFlags.h"

using namespace Platform;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

bool MotionTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();

    return true;
}

bool MotionTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool MotionTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

#if WI_IS_FEATURE_PRESENT(Feature_XamlMotionSystemHoldbacks)

// TODO: Dynamically enable velocity instead of skipping test.
#define VELOCITY_TESTGUARD_MOTIONS_HOLDBACKS if (!Feature_XamlMotionSystemHoldbacks::IsEnabled()) { LOG_OUTPUT(L"Motions holdbacks velocity feature disabled, skipping test"); return; }

void MotionTests::EventHandlerHelper(Platform::Object^ sender, Platform::Object^ args, UIElement^ expectedSender, std::shared_ptr<Event> event)
{
    UIElement^ senderUIE = safe_cast<UIElement^>(sender);
    VERIFY_ARE_EQUAL(expectedSender, senderUIE);
    VERIFY_IS_NOT_NULL(args);
    event->Set();
}

void MotionTests::BasicShownHiddenEvent()
{
    VELOCITY_TESTGUARD_MOTIONS_HOLDBACKS

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;

    auto shownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto hiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);

    std::shared_ptr<Event> shownEvent = std::make_shared<Event>();
    std::shared_ptr<Event> hiddenEvent = std::make_shared<Event>();

    wh->SetWindowSizeOverride(wf::Size(400, 400));

    xaml_shapes::Rectangle^ content;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        content = ref new xaml_shapes::Rectangle();
        content->Width = 50;
        content->Height = 50;
        content->Fill = ref new SolidColorBrush(mu::Colors::Purple);

        LOG_OUTPUT(L"> Attaching handler to UIElement.Shown event.");
        shownRegistration.Attach(content,
            ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^, Platform::Object^)
        {
            LOG_OUTPUT(L"  > UIElement.Shown event fired.");
            shownEvent->Set();
        }));

        LOG_OUTPUT(L"> Attaching handler to UIElement.Hidden event.");
        hiddenRegistration.Attach(content,
            ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^, Platform::Object^)
        {
            LOG_OUTPUT(L"  > UIElement.Hidden event fired.");
            hiddenEvent->Set();
        }));

        Canvas^ root = ref new Canvas();
        root->Children->Append(content);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for UIElement.Shown event.");
    shownEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Hiding content element.");
        content->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for UIElement.Hide event.");
    hiddenEvent->WaitForDefault();
}

void MotionTests::ShownEvent_AddToTree()
{
    VELOCITY_TESTGUARD_MOTIONS_HOLDBACKS

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;

    auto grandparentShownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto parentShownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto selfShownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto contentShownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto popupShownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto popupContentShownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto parentlessPopupShownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto parentlessPopupContentShownRegistration = CreateSafeEventRegistration(UIElement, Shown);

    std::shared_ptr<Event> grandparentShownEvent = std::make_shared<Event>();
    std::shared_ptr<Event> parentShownEvent = std::make_shared<Event>();
    std::shared_ptr<Event> selfShownEvent = std::make_shared<Event>();
    std::shared_ptr<Event> contentShownEvent = std::make_shared<Event>();
    std::shared_ptr<Event> popupShownEvent = std::make_shared<Event>();
    std::shared_ptr<Event> popupContentShownEvent = std::make_shared<Event>();
    std::shared_ptr<Event> parentlessPopupShownEvent = std::make_shared<Event>();
    std::shared_ptr<Event> parentlessPopupContentShownEvent = std::make_shared<Event>();

    xaml_shapes::Rectangle^ content;
    Grid^ grandparent;
    Grid^ parent;
    Grid^ self;
    xaml_primitives::Popup^ popup;
    Grid^ popupContent;
    xaml_primitives::Popup^ parentlessPopup;
    Grid^ parentlessPopupContent;

    wh->SetWindowSizeOverride(wf::Size(400, 400));
    Canvas^ root;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        grandparent = ref new Grid();
        parent = ref new Grid();
        self = ref new Grid();
        content = ref new xaml_shapes::Rectangle();
        popup = ref new xaml_primitives::Popup();
        popupContent = ref new Grid();
        parentlessPopup = ref new xaml_primitives::Popup();
        parentlessPopupContent = ref new Grid();

        LOG_OUTPUT(L"> Attaching handler to UIElement.Shown events.");
        grandparentShownRegistration.Attach(grandparent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Grandparent UIElement.Shown event fired."); EventHandlerHelper(sender, args, grandparent, grandparentShownEvent); }));
        parentShownRegistration.Attach(parent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Parent UIElement.Shown event fired."); EventHandlerHelper(sender, args, parent, parentShownEvent); }));
        selfShownRegistration.Attach(self, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Self UIElement.Shown event fired."); EventHandlerHelper(sender, args, self, selfShownEvent); }));
        contentShownRegistration.Attach(content, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Content UIElement.Shown event fired."); EventHandlerHelper(sender, args, content, contentShownEvent); }));
        popupShownRegistration.Attach(popup, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Popup UIElement.Shown event fired."); EventHandlerHelper(sender, args, popup, popupShownEvent); }));
        popupContentShownRegistration.Attach(popupContent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > PopupContent UIElement.Shown event fired."); EventHandlerHelper(sender, args, popupContent, popupContentShownEvent); }));
        parentlessPopupShownRegistration.Attach(parentlessPopup, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > ParentlessPopup UIElement.Shown event fired."); EventHandlerHelper(sender, args, parentlessPopup, parentlessPopupShownEvent); }));
        parentlessPopupContentShownRegistration.Attach(parentlessPopupContent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > ParentlessPopupContent UIElement.Shown event fired."); EventHandlerHelper(sender, args, parentlessPopupContent, parentlessPopupContentShownEvent); }));

        root = ref new Canvas();
        wh->WindowContent = root;

        {
            auto xamlRoot = root->XamlRoot;
            if (xamlRoot)
            {
                // UAP will return a null content root and does not need this to be set
                parentlessPopup->XamlRoot = xamlRoot;
            }
        }
    });
    wh->WaitForIdle();

    // ^ boilerplate

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Adding grandparent and parent to tree.");
        root->Children->Append(grandparent);
        grandparent->Children->Append(parent);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for grandparent and parent UIElement.Shown events.");
    grandparentShownEvent->WaitForDefault();
    parentShownEvent->WaitForDefault();
    VERIFY_IS_FALSE(selfShownEvent->HasFired());
    VERIFY_IS_FALSE(contentShownEvent->HasFired());
    VERIFY_IS_FALSE(popupShownEvent->HasFired());
    VERIFY_IS_FALSE(popupContentShownEvent->HasFired());
    VERIFY_IS_FALSE(parentlessPopupShownEvent->HasFired());
    VERIFY_IS_FALSE(parentlessPopupContentShownEvent->HasFired());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Adding content to self.");
        self->Children->Append(content);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> No UIElement.Shown event, because self isn't in the tree yet.");
    selfShownEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
    VERIFY_IS_FALSE(selfShownEvent->HasFired());
    VERIFY_IS_FALSE(contentShownEvent->HasFired());
    VERIFY_IS_FALSE(popupShownEvent->HasFired());
    VERIFY_IS_FALSE(popupContentShownEvent->HasFired());
    VERIFY_IS_FALSE(parentlessPopupShownEvent->HasFired());
    VERIFY_IS_FALSE(parentlessPopupContentShownEvent->HasFired());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Adding self to tree.");
        parent->Children->Append(self);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for self and content UIElement.Shown events.");
    selfShownEvent->WaitForDefault();
    contentShownEvent->WaitForDefault();
    VERIFY_IS_FALSE(popupShownEvent->HasFired());
    VERIFY_IS_FALSE(popupContentShownEvent->HasFired());
    VERIFY_IS_FALSE(parentlessPopupShownEvent->HasFired());
    VERIFY_IS_FALSE(parentlessPopupContentShownEvent->HasFired());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Adding popup to tree and opening it. Opening parentless popup.");
        self->Children->Append(popup);
        popup->IsOpen = true;
        parentlessPopup->IsOpen = true;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for popup and parentlessPopup UIElement.Shown event.");
    popupShownEvent->WaitForDefault();
    parentlessPopupShownEvent->WaitForDefault();
    VERIFY_IS_FALSE(popupContentShownEvent->HasFired());
    VERIFY_IS_FALSE(parentlessPopupContentShownEvent->HasFired());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Adding popupContent to popup. Adding parentlessPopupContent to parentlessPopup.");
        popup->Child = popupContent;
        parentlessPopup->Child = parentlessPopupContent;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for popupContent and parentlessPopupContent UIElement.Shown event.");
    popupContentShownEvent->WaitForDefault();
    parentlessPopupContentShownEvent->WaitForDefault();
}

void MotionTests::ShownEvent_VisibilityVisible()
{
    VELOCITY_TESTGUARD_MOTIONS_HOLDBACKS

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;

    auto grandparentShownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto parentShownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto selfShownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto contentShownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto popupShownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto popupContentShownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto parentlessPopupShownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto parentlessPopupContentShownRegistration = CreateSafeEventRegistration(UIElement, Shown);

    std::shared_ptr<Event> grandparentShownEvent = std::make_shared<Event>();
    std::shared_ptr<Event> parentShownEvent = std::make_shared<Event>();
    std::shared_ptr<Event> selfShownEvent = std::make_shared<Event>();
    std::shared_ptr<Event> contentShownEvent = std::make_shared<Event>();
    std::shared_ptr<Event> popupShownEvent = std::make_shared<Event>();
    std::shared_ptr<Event> popupContentShownEvent = std::make_shared<Event>();
    std::shared_ptr<Event> parentlessPopupShownEvent = std::make_shared<Event>();
    std::shared_ptr<Event> parentlessPopupContentShownEvent = std::make_shared<Event>();

    xaml_shapes::Rectangle^ content;
    Grid^ grandparent;
    Grid^ parent;
    Grid^ self;
    xaml_primitives::Popup^ popup;
    Grid^ popupContent;
    xaml_primitives::Popup^ parentlessPopup;
    Grid^ parentlessPopupContent;

    wh->SetWindowSizeOverride(wf::Size(400, 400));
    Canvas^ root;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        grandparent = ref new Grid();
        parent = ref new Grid();
        self = ref new Grid();
        content = ref new xaml_shapes::Rectangle();
        popup = ref new xaml_primitives::Popup();
        popupContent = ref new Grid();
        parentlessPopup = ref new xaml_primitives::Popup();
        parentlessPopupContent = ref new Grid();

        LOG_OUTPUT(L"> Attaching handler to UIElement.Shown events.");
        grandparentShownRegistration.Attach(grandparent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Grandparent UIElement.Shown event fired."); EventHandlerHelper(sender, args, grandparent, grandparentShownEvent); }));
        parentShownRegistration.Attach(parent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Parent UIElement.Shown event fired."); EventHandlerHelper(sender, args, parent, parentShownEvent); }));
        selfShownRegistration.Attach(self, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Self UIElement.Shown event fired."); EventHandlerHelper(sender, args, self, selfShownEvent); }));
        contentShownRegistration.Attach(content, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Content UIElement.Shown event fired."); EventHandlerHelper(sender, args, content, contentShownEvent); }));
        popupShownRegistration.Attach(popup, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Popup UIElement.Shown event fired."); EventHandlerHelper(sender, args, popup, popupShownEvent); }));
        popupContentShownRegistration.Attach(popupContent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > PopupContent UIElement.Shown event fired."); EventHandlerHelper(sender, args, popupContent, popupContentShownEvent); }));
        parentlessPopupShownRegistration.Attach(parentlessPopup, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > ParentlessPopup UIElement.Shown event fired."); EventHandlerHelper(sender, args, parentlessPopup, parentlessPopupShownEvent); }));
        parentlessPopupContentShownRegistration.Attach(parentlessPopupContent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > ParentlessPopupContent UIElement.Shown event fired."); EventHandlerHelper(sender, args, parentlessPopupContent, parentlessPopupContentShownEvent); }));

        root = ref new Canvas();
        wh->WindowContent = root;

        {
            auto xamlRoot = root->XamlRoot;
            if (xamlRoot)
            {
                // UAP will return a null content root and does not need this to be set
                parentlessPopup->XamlRoot = xamlRoot;
            }
        }
    });
    wh->WaitForIdle();

    // ^ boilerplate

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Attaching tree, collapsing everything.");
        grandparent->Visibility = Visibility::Collapsed; root->Children->Append(grandparent);
        parent->Visibility = Visibility::Collapsed; grandparent->Children->Append(parent);
        self->Visibility = Visibility::Collapsed; parent->Children->Append(self);
        content->Visibility = Visibility::Collapsed; self->Children->Append(content);
        popup->Visibility = Visibility::Collapsed; self->Children->Append(popup); popup->IsOpen = true;
        popupContent->Visibility = Visibility::Collapsed; popup->Child = popupContent;
        parentlessPopup->Visibility = Visibility::Collapsed;                      parentlessPopup->IsOpen = true;
        parentlessPopupContent->Visibility = Visibility::Collapsed; parentlessPopup->Child = parentlessPopupContent;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Uncollapsing grandparent and parent.");
        grandparent->Visibility = Visibility::Visible;
        parent->Visibility = Visibility::Visible;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for grandparent and parent UIElement.Shown events.");
    grandparentShownEvent->WaitForDefault();
    parentShownEvent->WaitForDefault();
    VERIFY_IS_FALSE(selfShownEvent->HasFired());
    VERIFY_IS_FALSE(contentShownEvent->HasFired());
    VERIFY_IS_FALSE(popupShownEvent->HasFired());
    VERIFY_IS_FALSE(popupContentShownEvent->HasFired());
    VERIFY_IS_FALSE(parentlessPopupShownEvent->HasFired());
    VERIFY_IS_FALSE(parentlessPopupContentShownEvent->HasFired());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Uncollapsing content.");
        content->Visibility = Visibility::Visible;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> No UIElement.Shown event, because self is still collapsed.");
    selfShownEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
    VERIFY_IS_FALSE(selfShownEvent->HasFired());
    VERIFY_IS_FALSE(contentShownEvent->HasFired());
    VERIFY_IS_FALSE(popupShownEvent->HasFired());
    VERIFY_IS_FALSE(popupContentShownEvent->HasFired());
    VERIFY_IS_FALSE(parentlessPopupShownEvent->HasFired());
    VERIFY_IS_FALSE(parentlessPopupContentShownEvent->HasFired());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Uncollapsing self.");
        self->Visibility = Visibility::Visible;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for self and content UIElement.Shown events.");
    selfShownEvent->WaitForDefault();
    contentShownEvent->WaitForDefault();
    VERIFY_IS_FALSE(popupShownEvent->HasFired());
    VERIFY_IS_FALSE(popupContentShownEvent->HasFired());
    VERIFY_IS_FALSE(parentlessPopupShownEvent->HasFired());
    VERIFY_IS_FALSE(parentlessPopupContentShownEvent->HasFired());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Uncollapsing popup and parentlessPopup.");
        popup->Visibility = Visibility::Visible;
        parentlessPopup->Visibility = Visibility::Visible;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for popup UIElement.Shown event.");
    popupShownEvent->WaitForDefault();
    parentlessPopupShownEvent->WaitForDefault();
    VERIFY_IS_FALSE(popupContentShownEvent->HasFired());
    VERIFY_IS_FALSE(parentlessPopupContentShownEvent->HasFired());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Uncollapsing popupContent and parentlessPopupContent.");
        popupContent->Visibility = Visibility::Visible;
        parentlessPopupContent->Visibility = Visibility::Visible;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for popupContent and parentlessPopupContent UIElement.Shown event.");
    popupContentShownEvent->WaitForDefault();
    parentlessPopupContentShownEvent->WaitForDefault();
}

void MotionTests::ShownEvent_PopupOpen()
{
    VELOCITY_TESTGUARD_MOTIONS_HOLDBACKS

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;

    auto grandparentShownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto parentShownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto selfShownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto contentShownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto popupShownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto popupContentShownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto parentlessPopupShownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto parentlessPopupContentShownRegistration = CreateSafeEventRegistration(UIElement, Shown);

    std::shared_ptr<Event> grandparentShownEvent = std::make_shared<Event>();
    std::shared_ptr<Event> parentShownEvent = std::make_shared<Event>();
    std::shared_ptr<Event> selfShownEvent = std::make_shared<Event>();
    std::shared_ptr<Event> contentShownEvent = std::make_shared<Event>();
    std::shared_ptr<Event> popupShownEvent = std::make_shared<Event>();
    std::shared_ptr<Event> popupContentShownEvent = std::make_shared<Event>();
    std::shared_ptr<Event> parentlessPopupShownEvent = std::make_shared<Event>();
    std::shared_ptr<Event> parentlessPopupContentShownEvent = std::make_shared<Event>();

    xaml_shapes::Rectangle^ content;
    Grid^ grandparent;
    Grid^ parent;
    Grid^ self;
    xaml_primitives::Popup^ popup;
    Grid^ popupContent;
    xaml_primitives::Popup^ parentlessPopup;
    Grid^ parentlessPopupContent;

    wh->SetWindowSizeOverride(wf::Size(400, 400));
    Canvas^ root;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        grandparent = ref new Grid();
        parent = ref new Grid();
        self = ref new Grid();
        content = ref new xaml_shapes::Rectangle();
        popup = ref new xaml_primitives::Popup();
        popupContent = ref new Grid();
        parentlessPopup = ref new xaml_primitives::Popup();
        parentlessPopupContent = ref new Grid();

        LOG_OUTPUT(L"> Attaching handler to UIElement.Shown events.");
        grandparentShownRegistration.Attach(grandparent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Grandparent UIElement.Shown event fired."); EventHandlerHelper(sender, args, grandparent, grandparentShownEvent); }));
        parentShownRegistration.Attach(parent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Parent UIElement.Shown event fired."); EventHandlerHelper(sender, args, parent, parentShownEvent); }));
        selfShownRegistration.Attach(self, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Self UIElement.Shown event fired."); EventHandlerHelper(sender, args, self, selfShownEvent); }));
        contentShownRegistration.Attach(content, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Content UIElement.Shown event fired."); EventHandlerHelper(sender, args, content, contentShownEvent); }));
        popupShownRegistration.Attach(popup, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Popup UIElement.Shown event fired."); EventHandlerHelper(sender, args, popup, popupShownEvent); }));
        popupContentShownRegistration.Attach(popupContent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > PopupContent UIElement.Shown event fired."); EventHandlerHelper(sender, args, popupContent, popupContentShownEvent); }));
        parentlessPopupShownRegistration.Attach(parentlessPopup, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > ParentlessPopup UIElement.Shown event fired."); EventHandlerHelper(sender, args, parentlessPopup, parentlessPopupShownEvent); }));
        parentlessPopupContentShownRegistration.Attach(parentlessPopupContent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > ParentlessPopupContent UIElement.Shown event fired."); EventHandlerHelper(sender, args, parentlessPopupContent, parentlessPopupContentShownEvent); }));

        root = ref new Canvas();
        wh->WindowContent = root;

        {
            auto xamlRoot = root->XamlRoot;
            if (xamlRoot)
            {
                // UAP will return a null content root and does not need this to be set
                parentlessPopup->XamlRoot = xamlRoot;
            }
        }
    });
    wh->WaitForIdle();

    // ^ boilerplate

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Attaching tree.");
        parentlessPopup->Child = parentlessPopupContent;
        popup->Child = popupContent;
        self->Children->Append(popup);
        self->Children->Append(content);
        parent->Children->Append(self);
        grandparent->Children->Append(parent);
        root->Children->Append(grandparent);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for grandparent, parent, self, and content UIElement.Shown events.");
    grandparentShownEvent->WaitForDefault();
    parentShownEvent->WaitForDefault();
    selfShownEvent->WaitForDefault();
    contentShownEvent->WaitForDefault();
    VERIFY_IS_FALSE(popupShownEvent->HasFired());
    VERIFY_IS_FALSE(popupContentShownEvent->HasFired());
    VERIFY_IS_FALSE(parentlessPopupShownEvent->HasFired());
    VERIFY_IS_FALSE(parentlessPopupContentShownEvent->HasFired());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Opening popup and parentlessPopup.");
        popup->IsOpen = true;
        parentlessPopup->IsOpen = true;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for popup, popupContent, parentlessPopup, and parentlessPopupContent UIElement.Shown events.");
    popupShownEvent->WaitForDefault();
    popupContentShownEvent->WaitForDefault();
    parentlessPopupShownEvent->WaitForDefault();
    parentlessPopupContentShownEvent->WaitForDefault();
}

void MotionTests::HiddenEvent_RemoveFromTree()
{
    VELOCITY_TESTGUARD_MOTIONS_HOLDBACKS

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;

    auto grandparentHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);
    auto parentHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);
    auto selfHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);
    auto contentHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);
    auto popupHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);
    auto popupContentHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);
    auto parentlessPopupHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);
    auto parentlessPopupContentHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);

    std::shared_ptr<Event> grandparentHiddenEvent = std::make_shared<Event>();
    std::shared_ptr<Event> parentHiddenEvent = std::make_shared<Event>();
    std::shared_ptr<Event> selfHiddenEvent = std::make_shared<Event>();
    std::shared_ptr<Event> contentHiddenEvent = std::make_shared<Event>();
    std::shared_ptr<Event> popupHiddenEvent = std::make_shared<Event>();
    std::shared_ptr<Event> popupContentHiddenEvent = std::make_shared<Event>();
    std::shared_ptr<Event> parentlessPopupHiddenEvent = std::make_shared<Event>();
    std::shared_ptr<Event> parentlessPopupContentHiddenEvent = std::make_shared<Event>();

    xaml_shapes::Rectangle^ content;
    Grid^ grandparent;
    Grid^ parent;
    Grid^ self;
    xaml_primitives::Popup^ popup;
    Grid^ popupContent;
    xaml_primitives::Popup^ parentlessPopup;
    Grid^ parentlessPopupContent;

    wh->SetWindowSizeOverride(wf::Size(400, 400));
    Canvas^ root;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        grandparent = ref new Grid();
        parent = ref new Grid();
        self = ref new Grid();
        content = ref new xaml_shapes::Rectangle();
        popup = ref new xaml_primitives::Popup();
        popupContent = ref new Grid();
        parentlessPopup = ref new xaml_primitives::Popup();
        parentlessPopupContent = ref new Grid();

        LOG_OUTPUT(L"> Attaching handler to UIElement.Hidden events.");
        grandparentHiddenRegistration.Attach(grandparent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Grandparent UIElement.Hidden event fired."); EventHandlerHelper(sender, args, grandparent, grandparentHiddenEvent); }));
        parentHiddenRegistration.Attach(parent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Parent UIElement.Hidden event fired."); EventHandlerHelper(sender, args, parent, parentHiddenEvent); }));
        selfHiddenRegistration.Attach(self, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Self UIElement.Hidden event fired."); EventHandlerHelper(sender, args, self, selfHiddenEvent); }));
        contentHiddenRegistration.Attach(content, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Content UIElement.Hidden event fired."); EventHandlerHelper(sender, args, content, contentHiddenEvent); }));
        popupHiddenRegistration.Attach(popup, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Popup UIElement.Hidden event fired."); EventHandlerHelper(sender, args, popup, popupHiddenEvent); }));
        popupContentHiddenRegistration.Attach(popupContent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > PopupContent UIElement.Hidden event fired."); EventHandlerHelper(sender, args, popupContent, popupContentHiddenEvent); }));
        parentlessPopupHiddenRegistration.Attach(parentlessPopup, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > ParentlessPopup UIElement.Hidden event fired."); EventHandlerHelper(sender, args, parentlessPopup, parentlessPopupHiddenEvent); }));
        parentlessPopupContentHiddenRegistration.Attach(parentlessPopupContent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > ParentlessPopupContent UIElement.Hidden event fired."); EventHandlerHelper(sender, args, parentlessPopupContent, parentlessPopupContentHiddenEvent); }));

        root = ref new Canvas();
        wh->WindowContent = root;

        {
            auto xamlRoot = root->XamlRoot;
            if (xamlRoot)
            {
                // UAP will return a null content root and does not need this to be set
                parentlessPopup->XamlRoot = xamlRoot;
            }
        }

        LOG_OUTPUT(L"> Attaching tree.");
        root->Children->Append(grandparent);
        grandparent->Children->Append(parent);
        parent->Children->Append(self);
        self->Children->Append(content);
        self->Children->Append(popup); popup->IsOpen = true;
        popup->Child = popupContent;
        parentlessPopup->IsOpen = true;
        parentlessPopup->Child = parentlessPopupContent;
    });
    wh->WaitForIdle();

    // ^ boilerplate

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Removing popupContent and parentlessPopupContent.");
        popup->Child = nullptr;
        parentlessPopup->Child = nullptr;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for popupContent and parentlessPopupContent UIElement.Hidden event.");
    popupContentHiddenEvent->WaitForDefault();
    parentlessPopupContentHiddenEvent->WaitForDefault();
    VERIFY_IS_FALSE(popupHiddenEvent->HasFired());
    VERIFY_IS_FALSE(parentlessPopupHiddenEvent->HasFired());
    VERIFY_IS_FALSE(contentHiddenEvent->HasFired());
    VERIFY_IS_FALSE(selfHiddenEvent->HasFired());
    VERIFY_IS_FALSE(parentHiddenEvent->HasFired());
    VERIFY_IS_FALSE(grandparentHiddenEvent->HasFired());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Removing popup.");
        self->Children->RemoveAt(1);    // 0 is content
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for popup UIElement.Hidden event.");
    popupHiddenEvent->WaitForDefault();
    VERIFY_IS_FALSE(parentlessPopupHiddenEvent->HasFired());
    VERIFY_IS_FALSE(contentHiddenEvent->HasFired());
    VERIFY_IS_FALSE(selfHiddenEvent->HasFired());
    VERIFY_IS_FALSE(parentHiddenEvent->HasFired());
    VERIFY_IS_FALSE(grandparentHiddenEvent->HasFired());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Removing self.");
        parent->Children->Clear();
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for self and content UIElement.Hidden events.");
    VERIFY_IS_FALSE(parentlessPopupHiddenEvent->HasFired());
    selfHiddenEvent->WaitForDefault();
    contentHiddenEvent->WaitForDefault();
    VERIFY_IS_FALSE(parentHiddenEvent->HasFired());
    VERIFY_IS_FALSE(grandparentHiddenEvent->HasFired());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Removing parent and grandparent.");
        grandparent->Children->Clear();
        root->Children->Clear();
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for grandparent and parent UIElement.Hidden events.");
    VERIFY_IS_FALSE(parentlessPopupHiddenEvent->HasFired());
    parentHiddenEvent->WaitForDefault();
    grandparentHiddenEvent->WaitForDefault();
}

void MotionTests::HiddenEvent_VisibilityCollapsed()
{
    VELOCITY_TESTGUARD_MOTIONS_HOLDBACKS

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;

    auto grandparentHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);
    auto parentHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);
    auto selfHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);
    auto contentHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);
    auto popupHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);
    auto popupContentHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);
    auto parentlessPopupHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);
    auto parentlessPopupContentHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);

    std::shared_ptr<Event> grandparentHiddenEvent = std::make_shared<Event>();
    std::shared_ptr<Event> parentHiddenEvent = std::make_shared<Event>();
    std::shared_ptr<Event> selfHiddenEvent = std::make_shared<Event>();
    std::shared_ptr<Event> contentHiddenEvent = std::make_shared<Event>();
    std::shared_ptr<Event> popupHiddenEvent = std::make_shared<Event>();
    std::shared_ptr<Event> popupContentHiddenEvent = std::make_shared<Event>();
    std::shared_ptr<Event> parentlessPopupHiddenEvent = std::make_shared<Event>();
    std::shared_ptr<Event> parentlessPopupContentHiddenEvent = std::make_shared<Event>();

    xaml_shapes::Rectangle^ content;
    Grid^ grandparent;
    Grid^ parent;
    Grid^ self;
    xaml_primitives::Popup^ popup;
    Grid^ popupContent;
    xaml_primitives::Popup^ parentlessPopup;
    Grid^ parentlessPopupContent;

    wh->SetWindowSizeOverride(wf::Size(400, 400));
    Canvas^ root;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        grandparent = ref new Grid();
        parent = ref new Grid();
        self = ref new Grid();
        content = ref new xaml_shapes::Rectangle();
        popup = ref new xaml_primitives::Popup();
        popupContent = ref new Grid();
        parentlessPopup = ref new xaml_primitives::Popup();
        parentlessPopupContent = ref new Grid();

        LOG_OUTPUT(L"> Attaching handler to UIElement.Hidden events.");
        grandparentHiddenRegistration.Attach(grandparent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Grandparent UIElement.Hidden event fired."); EventHandlerHelper(sender, args, grandparent, grandparentHiddenEvent); }));
        parentHiddenRegistration.Attach(parent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Parent UIElement.Hidden event fired."); EventHandlerHelper(sender, args, parent, parentHiddenEvent); }));
        selfHiddenRegistration.Attach(self, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Self UIElement.Hidden event fired."); EventHandlerHelper(sender, args, self, selfHiddenEvent); }));
        contentHiddenRegistration.Attach(content, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Content UIElement.Hidden event fired."); EventHandlerHelper(sender, args, content, contentHiddenEvent); }));
        popupHiddenRegistration.Attach(popup, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Popup UIElement.Hidden event fired."); EventHandlerHelper(sender, args, popup, popupHiddenEvent); }));
        popupContentHiddenRegistration.Attach(popupContent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > PopupContent UIElement.Hidden event fired."); EventHandlerHelper(sender, args, popupContent, popupContentHiddenEvent); }));
        parentlessPopupHiddenRegistration.Attach(parentlessPopup, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > ParentlessPopup UIElement.Hidden event fired."); EventHandlerHelper(sender, args, parentlessPopup, parentlessPopupHiddenEvent); }));
        parentlessPopupContentHiddenRegistration.Attach(parentlessPopupContent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > ParentlessPopupContent UIElement.Hidden event fired."); EventHandlerHelper(sender, args, parentlessPopupContent, parentlessPopupContentHiddenEvent); }));

        root = ref new Canvas();
        wh->WindowContent = root;

        {
            auto xamlRoot = root->XamlRoot;
            if (xamlRoot)
            {
                // UAP will return a null content root and does not need this to be set
                parentlessPopup->XamlRoot = xamlRoot;
            }
        }

        LOG_OUTPUT(L"> Attaching tree.");
        root->Children->Append(grandparent);
        grandparent->Children->Append(parent);
        parent->Children->Append(self);
        self->Children->Append(content);
        self->Children->Append(popup); popup->IsOpen = true;
        popup->Child = popupContent;
        parentlessPopup->IsOpen = true;
        parentlessPopup->Child = parentlessPopupContent;
    });
    wh->WaitForIdle();

    // ^ boilerplate

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Collapsing popupContent and parentlessPopupContent.");
        popupContent->Visibility = Visibility::Collapsed;
        parentlessPopupContent->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for popupContent and parentlessPopupContent UIElement.Hidden event.");
    popupContentHiddenEvent->WaitForDefault();
    parentlessPopupContentHiddenEvent->WaitForDefault();
    VERIFY_IS_FALSE(popupHiddenEvent->HasFired());
    VERIFY_IS_FALSE(parentlessPopupHiddenEvent->HasFired());
    VERIFY_IS_FALSE(contentHiddenEvent->HasFired());
    VERIFY_IS_FALSE(selfHiddenEvent->HasFired());
    VERIFY_IS_FALSE(parentHiddenEvent->HasFired());
    VERIFY_IS_FALSE(grandparentHiddenEvent->HasFired());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Collapsing popup and parentlessPopup.");
        popup->Visibility = Visibility::Collapsed;
        parentlessPopup->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for popup UIElement.Hidden event.");
    popupHiddenEvent->WaitForDefault();
    parentlessPopupHiddenEvent->WaitForDefault();
    VERIFY_IS_FALSE(contentHiddenEvent->HasFired());
    VERIFY_IS_FALSE(selfHiddenEvent->HasFired());
    VERIFY_IS_FALSE(parentHiddenEvent->HasFired());
    VERIFY_IS_FALSE(grandparentHiddenEvent->HasFired());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Collapsing self.");
        self->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for self and content UIElement.Hidden events.");
    selfHiddenEvent->WaitForDefault();
    contentHiddenEvent->WaitForDefault();
    VERIFY_IS_FALSE(parentHiddenEvent->HasFired());
    VERIFY_IS_FALSE(grandparentHiddenEvent->HasFired());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Collapsing grandparent and parent.");
        grandparent->Visibility = Visibility::Collapsed;
        parent->Visibility = Visibility::Collapsed;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for grandparent and parent UIElement.Hidden events.");
    parentHiddenEvent->WaitForDefault();
    grandparentHiddenEvent->WaitForDefault();
}

void MotionTests::HiddenEvent_PopupClose()
{
    VELOCITY_TESTGUARD_MOTIONS_HOLDBACKS

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;

    auto grandparentHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);
    auto parentHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);
    auto selfHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);
    auto contentHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);
    auto popupHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);
    auto popupContentHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);
    auto parentlessPopupHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);
    auto parentlessPopupContentHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);

    std::shared_ptr<Event> grandparentHiddenEvent = std::make_shared<Event>();
    std::shared_ptr<Event> parentHiddenEvent = std::make_shared<Event>();
    std::shared_ptr<Event> selfHiddenEvent = std::make_shared<Event>();
    std::shared_ptr<Event> contentHiddenEvent = std::make_shared<Event>();
    std::shared_ptr<Event> popupHiddenEvent = std::make_shared<Event>();
    std::shared_ptr<Event> popupContentHiddenEvent = std::make_shared<Event>();
    std::shared_ptr<Event> parentlessPopupHiddenEvent = std::make_shared<Event>();
    std::shared_ptr<Event> parentlessPopupContentHiddenEvent = std::make_shared<Event>();

    xaml_shapes::Rectangle^ content;
    Grid^ grandparent;
    Grid^ parent;
    Grid^ self;
    xaml_primitives::Popup^ popup;
    Grid^ popupContent;
    xaml_primitives::Popup^ parentlessPopup;
    Grid^ parentlessPopupContent;

    wh->SetWindowSizeOverride(wf::Size(400, 400));
    Canvas^ root;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        grandparent = ref new Grid();
        parent = ref new Grid();
        self = ref new Grid();
        content = ref new xaml_shapes::Rectangle();
        popup = ref new xaml_primitives::Popup();
        popupContent = ref new Grid();
        parentlessPopup = ref new xaml_primitives::Popup();
        parentlessPopupContent = ref new Grid();

        LOG_OUTPUT(L"> Attaching handler to UIElement.Hidden events.");
        grandparentHiddenRegistration.Attach(grandparent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Grandparent UIElement.Hidden event fired."); EventHandlerHelper(sender, args, grandparent, grandparentHiddenEvent); }));
        parentHiddenRegistration.Attach(parent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Parent UIElement.Hidden event fired."); EventHandlerHelper(sender, args, parent, parentHiddenEvent); }));
        selfHiddenRegistration.Attach(self, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Self UIElement.Hidden event fired."); EventHandlerHelper(sender, args, self, selfHiddenEvent); }));
        contentHiddenRegistration.Attach(content, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Content UIElement.Hidden event fired."); EventHandlerHelper(sender, args, content, contentHiddenEvent); }));
        popupHiddenRegistration.Attach(popup, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Popup UIElement.Hidden event fired."); EventHandlerHelper(sender, args, popup, popupHiddenEvent); }));
        popupContentHiddenRegistration.Attach(popupContent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > PopupContent UIElement.Hidden event fired."); EventHandlerHelper(sender, args, popupContent, popupContentHiddenEvent); }));
        parentlessPopupHiddenRegistration.Attach(parentlessPopup, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > ParentlessPopup UIElement.Hidden event fired."); EventHandlerHelper(sender, args, parentlessPopup, parentlessPopupHiddenEvent); }));
        parentlessPopupContentHiddenRegistration.Attach(parentlessPopupContent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > ParentlessPopupContent UIElement.Hidden event fired."); EventHandlerHelper(sender, args, parentlessPopupContent, parentlessPopupContentHiddenEvent); }));

        root = ref new Canvas();
        wh->WindowContent = root;

        {
            auto xamlRoot = root->XamlRoot;
            if (xamlRoot)
            {
                // UAP will return a null content root and does not need this to be set
                parentlessPopup->XamlRoot = xamlRoot;
            }
        }

        LOG_OUTPUT(L"> Attaching tree.");
        root->Children->Append(grandparent);
        grandparent->Children->Append(parent);
        parent->Children->Append(self);
        self->Children->Append(content);
        self->Children->Append(popup); popup->IsOpen = true;
        popup->Child = popupContent;
        parentlessPopup->IsOpen = true;
        parentlessPopup->Child = parentlessPopupContent;
    });
    wh->WaitForIdle();

    // ^ boilerplate

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Closing popup and parentlessPopup.");
        popup->IsOpen = false;
        parentlessPopup->IsOpen = false;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for popup, popupContent, parentlessPopup, and parentlessPopupContent UIElement.Hidden events.");
    popupHiddenEvent->WaitForDefault();
    popupContentHiddenEvent->WaitForDefault();
    parentlessPopupHiddenEvent->WaitForDefault();
    parentlessPopupContentHiddenEvent->WaitForDefault();
}

void MotionTests::ShownHiddenEvent_RepeatedlyAddToTree()
{
    VELOCITY_TESTGUARD_MOTIONS_HOLDBACKS

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;

    auto selfShownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto selfHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);

    std::shared_ptr<Event> selfShownEvent = std::make_shared<Event>();
    std::shared_ptr<Event> selfHiddenEvent = std::make_shared<Event>();

    Grid^ parent;
    Grid^ self;

    wh->SetWindowSizeOverride(wf::Size(400, 400));
    Canvas^ root;

    LOG_OUTPUT(L"> Repeatedly add an element to the tree and remove it. Make sure that Shown/Hidden events work correctly on elements that aren't in the tree.");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        self = ref new Grid();
        parent = ref new Grid();

        LOG_OUTPUT(L"> Attaching handler.");
        selfShownRegistration.Attach(self, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^)
        {
            LOG_OUTPUT(L"  > UIElement.Shown event fired.");
            Grid^ senderGrid = safe_cast<Grid^>(sender);
            VERIFY_ARE_EQUAL(self, senderGrid);
            VERIFY_IS_FALSE(selfShownEvent->HasFired());
            selfShownEvent->Set();
        }));
        selfHiddenRegistration.Attach(self, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^)
        {
            LOG_OUTPUT(L"  > UIElement.Hidden event fired.");
            Grid^ senderGrid = safe_cast<Grid^>(sender);
            VERIFY_ARE_EQUAL(self, senderGrid);
            VERIFY_IS_FALSE(selfHiddenEvent->HasFired());
            selfHiddenEvent->Set();
        }));

        root = ref new Canvas();
        wh->WindowContent = root;

        LOG_OUTPUT(L"> Attaching tree.");
        root->Children->Append(self);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for UIElement.Shown event.");
    selfShownEvent->WaitForDefault();
    VERIFY_IS_FALSE(selfHiddenEvent->HasFired());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Removing element.");
        root->Children->Clear();
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"> Waiting for UIElement.Hidden event.");
    VERIFY_IS_TRUE(selfShownEvent->HasFired());
    selfHiddenEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Adding element back.");
        selfHiddenEvent->Reset();
        selfShownEvent->Reset();
        root->Children->Append(self);
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"> Waiting for UIElement.Shown event.");
    selfShownEvent->WaitForDefault();
    VERIFY_IS_FALSE(selfHiddenEvent->HasFired());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Removing element.");
        root->Children->Clear();
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"> Waiting for UIElement.Hidden event.");
    VERIFY_IS_TRUE(selfShownEvent->HasFired());
    selfHiddenEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Adding element back via a parent.");
        selfHiddenEvent->Reset();
        selfShownEvent->Reset();
        root->Children->Append(parent);
        parent->Children->Append(self);
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"> Waiting for UIElement.Shown event.");
    selfShownEvent->WaitForDefault();
    VERIFY_IS_FALSE(selfHiddenEvent->HasFired());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Removing element.");
        parent->Children->Clear();
        root->Children->Clear();
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"> Waiting for UIElement.Hidden event.");
    VERIFY_IS_TRUE(selfShownEvent->HasFired());
    selfHiddenEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Adding element back via a parent.");
        selfHiddenEvent->Reset();
        selfShownEvent->Reset();
        parent->Children->Append(self);
        root->Children->Append(parent);
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"> Waiting for UIElement.Shown event.");
    selfShownEvent->WaitForDefault();
    VERIFY_IS_FALSE(selfHiddenEvent->HasFired());

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Removing element.");
        root->Children->Clear();
        parent->Children->Clear();
    });
    wh->WaitForIdle();
    LOG_OUTPUT(L"> Waiting for UIElement.Hidden event.");
    VERIFY_IS_TRUE(selfShownEvent->HasFired());
    selfHiddenEvent->WaitForDefault();
}

void MotionTests::ShownHiddenEvent_CannotMixWithECP()
{
    VELOCITY_TESTGUARD_MOTIONS_HOLDBACKS

    using namespace ::Windows::UI::Composition;
    using namespace Microsoft::UI::Xaml::Hosting;

    auto wh = TestServices::WindowHelper;

    auto selfShownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto selfHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);

    wh->SetWindowSizeOverride(wf::Size(400, 400));
    Canvas^ root;
    Grid^ self;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        self = ref new Grid();

        root = ref new Canvas();
        root->Children->Append(self);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Calling ElementCompositionPreview.SetImplicitHideAnimation.");

        Compositor^ compositor = Microsoft::UI::Xaml::Media::CompositionTarget::GetCompositorForCurrentThread();
        ScalarKeyFrameAnimation^ kfa = compositor->CreateScalarKeyFrameAnimation();
        ElementCompositionPreview::SetImplicitShowAnimation(self, kfa);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        HRESULT hr;

        hr = S_OK;
        try
        {
            LOG_OUTPUT(L"> Attaching Shown handler. Expecting failure.");
            selfShownRegistration.Attach(self, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^) { }));
        }
        catch (Platform::Exception^ e)
        {
            hr = e->HResult;
        }
        VERIFY_ARE_EQUAL(HRESULT_FROM_WIN32(ERROR_INVALID_OPERATION), hr);

        hr = S_OK;
        try
        {
            LOG_OUTPUT(L"> Attaching Hidden handler. Expecting failure.");
            selfHiddenRegistration.Attach(self, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^) { }));
        }
        catch (Platform::Exception^ e)
        {
            hr = e->HResult;
        }
        VERIFY_ARE_EQUAL(HRESULT_FROM_WIN32(ERROR_INVALID_OPERATION), hr);

    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Clearing ElementCompositionPreview.SetImplicitHideAnimation.");
        ElementCompositionPreview::SetImplicitShowAnimation(self, nullptr);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Attaching Shown handler. Expecting success.");
        selfShownRegistration.Attach(self, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^) { }));

        LOG_OUTPUT(L"> Attaching Hidden handler. Expecting success.");
        selfHiddenRegistration.Attach(self, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^) { }));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        HRESULT hr;
        Compositor^ compositor = Microsoft::UI::Xaml::Media::CompositionTarget::GetCompositorForCurrentThread();
        ScalarKeyFrameAnimation^ kfa = compositor->CreateScalarKeyFrameAnimation();

        hr = S_OK;
        try
        {
            LOG_OUTPUT(L"> Calling ElementCompositionPreview.SetImplicitShowAnimation. Expecting failure.");
            ElementCompositionPreview::SetImplicitShowAnimation(self, kfa);
        }
        catch (Platform::Exception^ e)
        {
            hr = e->HResult;
        }
        VERIFY_ARE_EQUAL(HRESULT_FROM_WIN32(ERROR_INVALID_OPERATION), hr);

        hr = S_OK;
        try
        {
            LOG_OUTPUT(L"> Calling ElementCompositionPreview.SetImplicitHideAnimation with null. Expecting failure.");
            ElementCompositionPreview::SetImplicitHideAnimation(self, nullptr);
        }
        catch (Platform::Exception^ e)
        {
            hr = e->HResult;
        }
        VERIFY_ARE_EQUAL(HRESULT_FROM_WIN32(ERROR_INVALID_OPERATION), hr);
    });
    wh->WaitForIdle();
}

void MotionTests::ShownHiddenEvent_TrackEffectiveVisibility()
{
    VELOCITY_TESTGUARD_MOTIONS_HOLDBACKS

    auto wh = TestServices::WindowHelper;

    auto selfShownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto selfHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);

    auto selfShownRegistration2 = CreateSafeEventRegistration(UIElement, Shown);
    auto selfHiddenRegistration2 = CreateSafeEventRegistration(UIElement, Hidden);
    auto selfShownRegistration3 = CreateSafeEventRegistration(UIElement, Shown);
    auto selfHiddenRegistration3 = CreateSafeEventRegistration(UIElement, Hidden);

    wh->SetWindowSizeOverride(wf::Size(400, 400));
    Canvas^ root;
    Grid^ self;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating elements.");

        self = ref new Grid();

        root = ref new Canvas();
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Should not be tracking effective visibility by default.");
        VERIFY_IS_FALSE(wh->IsTrackingEffectiveVisibility(self));
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Attaching to UIElement.Hidden.");
        selfHiddenRegistration.Attach(self, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^) { }));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Should now be tracking effective visibility.");
        VERIFY_IS_TRUE(wh->IsTrackingEffectiveVisibility(self));
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Detaching from UIElement.Hidden.");
        selfHiddenRegistration.Detach();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Should no longer be tracking effective visibility by default.");
        VERIFY_IS_FALSE(wh->IsTrackingEffectiveVisibility(self));
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Attaching to UIElement.Shown.");
        selfShownRegistration.Attach(self, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^)
        {
            LOG_OUTPUT(L"  > UIElement.Shown");
            LOG_OUTPUT(L"  > Unregister by returning RPC_E_DISCONNECTED.");
            throw ref new Platform::COMException(RPC_E_DISCONNECTED);
        }));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Should now be tracking effective visibility.");
        VERIFY_IS_TRUE(wh->IsTrackingEffectiveVisibility(self));
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Adding to tree, which fires UIElement.Shown.");
        root->Children->Append(self);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Should no longer be tracking effective visibility by default.");
        VERIFY_IS_FALSE(wh->IsTrackingEffectiveVisibility(self));
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Attaching to UIElement.Shown twice.");
        selfShownRegistration2.Attach(self, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^) { }));
        selfShownRegistration3.Attach(self, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^) { }));

        LOG_OUTPUT(L"> Attaching to UIElement.Hidden twice.");
        selfHiddenRegistration2.Attach(self, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^) { }));
        selfHiddenRegistration3.Attach(self, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^) { }));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Should now be tracking effective visibility.");
        VERIFY_IS_TRUE(wh->IsTrackingEffectiveVisibility(self));
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Detaching from UIElement.Hidden.");
        selfHiddenRegistration2.Detach();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Should still be tracking effective visibility.");
        VERIFY_IS_TRUE(wh->IsTrackingEffectiveVisibility(self));
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Detaching from UIElement.Shown and Hidden.");
        selfShownRegistration3.Detach();
        selfHiddenRegistration3.Detach();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Should still be tracking effective visibility.");
        VERIFY_IS_TRUE(wh->IsTrackingEffectiveVisibility(self));
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Detaching from UIElement.Shown.");
        selfShownRegistration2.Detach();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Should no longer be tracking effective visibility by default.");
        VERIFY_IS_FALSE(wh->IsTrackingEffectiveVisibility(self));
    });
}

void MotionTests::ShownHiddenEvent_ChangeLayoutDuringShown()
{
    VELOCITY_TESTGUARD_MOTIONS_HOLDBACKS

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    auto selfShownRegistration = CreateSafeEventRegistration(UIElement, Shown);

    wh->SetWindowSizeOverride(wf::Size(400, 400));
    Canvas^ root;
    Grid^ self;
    Grid^ layout;
    Grid^ render;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating elements.");

        self = ref new Grid();

        render = ref new Grid();
        render->Background = ref new SolidColorBrush(mu::Colors::Red);

        layout = ref new Grid();
        layout->Width = 100;
        layout->Height = 100;
        layout->Children->Append(render);

        root = ref new Canvas();
        root->Children->Append(layout);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Attaching to UIElement.Shown.");
        selfShownRegistration.Attach(self, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^)
        {
            LOG_OUTPUT(L"  > UIElement.Shown raised. Changing element width.");
            layout->Width = 50;
        }));

        root->Children->Append(self);
    });
    wh->WaitForIdle();

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void MotionTests::ShownEvent_RemoveDuringShown()
{
    VELOCITY_TESTGUARD_MOTIONS_HOLDBACKS

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    ::Windows::Foundation::EventRegistrationToken renderingEventToken = {};

    TestCleanupWrapper cleanup([&]()
    {
        if (renderingEventToken.Value != 0)
        {
            RunOnUIThread([&]()
            {
                CompositionTarget::Rendering::remove(renderingEventToken);
            });
        }

        wh->ResetWindowContentAndWaitForIdle();
    });

    auto selfShownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto selfHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);

    wh->SetWindowSizeOverride(wf::Size(400, 400));
    Canvas^ root;
    Grid^ self;

    int frameCount = 0;
    int frameCountOnShown = -1;
    int frameCountOnHidden = -1;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating elements.");

        self = ref new Grid();
        self->Width = 100;
        self->Height = 100;
        self->Background = ref new SolidColorBrush(mu::Colors::Red);

        root = ref new Canvas();
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        auto onRendering = ref new ::Windows::Foundation::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ o)
        {
            LOG_OUTPUT(L"> CompositionTarget.Rendering");
            frameCount++;
        });
        LOG_OUTPUT(L"> Registering for CompositionTarget.Rendering to count frames");
        renderingEventToken = CompositionTarget::Rendering::add(onRendering);
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Attaching to Shown/Hidden.");
        selfShownRegistration.Attach(self, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^)
        {
            LOG_OUTPUT(L"  > UIElement.Shown raised. Removing element.");
            root->Children->Clear();
            frameCountOnShown = frameCount;
        }));
        selfHiddenRegistration.Attach(self, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^)
        {
            LOG_OUTPUT(L"  > UIElement.Hidden raised.");
            frameCountOnHidden = frameCount;
            CompositionTarget::Rendering::remove(renderingEventToken);
        }));

        root->Children->Append(self);
    });
    wh->SynchronouslyTickUIThread(3);

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    LOG_OUTPUT(L"> UIElement.Hidden should have been raised on a frame after UIElement.Shown.");
    VERIFY_IS_GREATER_THAN(frameCountOnHidden, frameCountOnShown);
}

void MotionTests::HiddenEvent_AddDuringHidden()
{
    VELOCITY_TESTGUARD_MOTIONS_HOLDBACKS

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    ::Windows::Foundation::EventRegistrationToken renderingEventToken = {};

    TestCleanupWrapper cleanup([&]()
    {
        if (renderingEventToken.Value != 0)
        {
            RunOnUIThread([&]()
            {
                CompositionTarget::Rendering::remove(renderingEventToken);
            });
        }

        wh->ResetWindowContentAndWaitForIdle();
    });

    auto selfShownRegistration = CreateSafeEventRegistration(UIElement, Shown);
    auto selfHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);

    wh->SetWindowSizeOverride(wf::Size(400, 400));
    Canvas^ root;
    Grid^ self;

    int frameCount = 0;
    int frameCountOnShown = -1;
    int frameCountOnHidden = -1;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating elements.");

        self = ref new Grid();
        self->Width = 100;
        self->Height = 100;
        self->Background = ref new SolidColorBrush(mu::Colors::Red);

        root = ref new Canvas();
        root->Children->Append(self);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        auto onRendering = ref new ::Windows::Foundation::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ o)
        {
            LOG_OUTPUT(L"> CompositionTarget.Rendering");
            frameCount++;
        });
        LOG_OUTPUT(L"> Registering for CompositionTarget.Rendering to count frames");
        renderingEventToken = CompositionTarget::Rendering::add(onRendering);
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Attaching to Shown/Hidden.");
        selfHiddenRegistration.Attach(self, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^)
        {
            LOG_OUTPUT(L"  > UIElement.Hidden raised. Adding element back.");
            root->Children->Append(self);
            frameCountOnHidden = frameCount;
        }));
        selfShownRegistration.Attach(self, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^)
        {
            LOG_OUTPUT(L"  > UIElement.Shown raised.");
            frameCountOnShown = frameCount;
            CompositionTarget::Rendering::remove(renderingEventToken);
        }));

        root->Children->Clear();
    });
    wh->SynchronouslyTickUIThread(3);

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    LOG_OUTPUT(L"> UIElement.Shown should have been raised on a frame after UIElement.Hidden.");
    VERIFY_IS_GREATER_THAN(frameCountOnShown, frameCountOnHidden);
}

void MotionTests::KeepAlive_RequestReleaseAPI()
{
    VELOCITY_TESTGUARD_MOTIONS_HOLDBACKS

    const auto& wh = TestServices::WindowHelper;

    xaml_shapes::Rectangle^ content;
    Grid^ grandparent;
    Grid^ parent;
    Grid^ self;
    xaml_primitives::Popup^ popup;
    Grid^ popupContent;
    xaml_primitives::Popup^ parentlessPopup;
    Grid^ parentlessPopupContent;

    wh->SetWindowSizeOverride(wf::Size(400, 400));
    Canvas^ root;

    TestCleanupWrapper cleanup;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        grandparent = ref new Grid();
        parent = ref new Grid();
        self = ref new Grid();
        content = ref new xaml_shapes::Rectangle();
        popup = ref new xaml_primitives::Popup();
        popupContent = ref new Grid();
        parentlessPopup = ref new xaml_primitives::Popup();
        parentlessPopupContent = ref new Grid();

        root = ref new Canvas();
        wh->WindowContent = root;

        {
            auto xamlRoot = root->XamlRoot;
            if (xamlRoot)
            {
                // UAP will return a null content root and does not need this to be set
                parentlessPopup->XamlRoot = xamlRoot;
            }
        }

        LOG_OUTPUT(L"> Attaching tree.");
        root->Children->Append(grandparent);
        grandparent->Children->Append(parent);
        parent->Children->Append(self);
        self->Children->Append(content);
        self->Children->Append(popup); popup->IsOpen = true;
        popup->Child = popupContent;
        parentlessPopup->IsOpen = true;
        parentlessPopup->Child = parentlessPopupContent;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L">> KeepAlive on content with removing content from the tree:");
    LOG_OUTPUT(L"   > Calling RequestKeepAlive on content.");
    RunOnUIThread([&]() { wh->RequestKeepAlive(content); }); wh->WaitForIdle();
    LOG_OUTPUT(L"   > Removing content from tree.");
    RunOnUIThread([&]() { self->Children->RemoveAt(0); }); wh->WaitForIdle();
    LOG_OUTPUT(L"     > content should be kept visible.");
    VERIFY_IS_TRUE(wh->IsKeepingVisible(content));
    LOG_OUTPUT(L"   > Calling ReleaseKeepAlive on content.");
    RunOnUIThread([&]() { wh->ReleaseKeepAlive(content); }); wh->WaitForIdle();
    LOG_OUTPUT(L"     > content should not be kept visible anymore.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(content));
    LOG_OUTPUT(L"   > Restoring tree.");
    RunOnUIThread([&]() { self->Children->Append(content); }); wh->WaitForIdle();

    LOG_OUTPUT(L">> KeepAlive on popup with closing popup:");
    LOG_OUTPUT(L"   > Calling RequestKeepAlive on popup.");
    RunOnUIThread([&]() { wh->RequestKeepAlive(popup); }); wh->WaitForIdle();
    LOG_OUTPUT(L"   > Closing popup.");
    RunOnUIThread([&]() { popup->IsOpen = false; }); wh->WaitForIdle();
    LOG_OUTPUT(L"     > popup should be kept visible.");
    VERIFY_IS_TRUE(wh->IsKeepingVisible(popup));
    LOG_OUTPUT(L"   > Calling ReleaseKeepAlive on popup.");
    RunOnUIThread([&]() { wh->ReleaseKeepAlive(popup); }); wh->WaitForIdle();
    LOG_OUTPUT(L"     > popup should not be kept visible anymore.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(popup));
    LOG_OUTPUT(L"   > Restoring tree.");
    RunOnUIThread([&]() { popup->IsOpen = true; }); wh->WaitForIdle();

    LOG_OUTPUT(L">> KeepAlive on popup's content with collapsing popup:");
    LOG_OUTPUT(L"   > Calling RequestKeepAlive on popupContent.");
    RunOnUIThread([&]() { wh->RequestKeepAlive(popupContent); }); wh->WaitForIdle();
    LOG_OUTPUT(L"   > Collapsing popup.");
    RunOnUIThread([&]() { popup->Visibility = Visibility::Collapsed; }); wh->WaitForIdle();
    LOG_OUTPUT(L"     > popup should be kept visible.");
    VERIFY_IS_TRUE(wh->IsKeepingVisible(popup));
    LOG_OUTPUT(L"     > popupContent should not be kept visible. It doesn't need to be.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(popupContent));
    LOG_OUTPUT(L"   > Calling ReleaseKeepAlive on popupContent.");
    RunOnUIThread([&]() { wh->ReleaseKeepAlive(popupContent); }); wh->WaitForIdle();
    LOG_OUTPUT(L"     > popup should not be kept visible anymore.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(popup));
    LOG_OUTPUT(L"   > Restoring tree.");
    RunOnUIThread([&]() { popup->Visibility = Visibility::Visible; }); wh->WaitForIdle();

    LOG_OUTPUT(L">> KeepAlive on self with collapsing an ancestor:");
    LOG_OUTPUT(L"   > Calling RequestKeepAlive on self.");
    RunOnUIThread([&]() { wh->RequestKeepAlive(self); }); wh->WaitForIdle();
    LOG_OUTPUT(L"   > Collapsing parent.");
    RunOnUIThread([&]() { parent->Visibility = Visibility::Collapsed; }); wh->WaitForIdle();
    LOG_OUTPUT(L"     > parent should be kept visible.");
    VERIFY_IS_TRUE(wh->IsKeepingVisible(parent));
    LOG_OUTPUT(L"     > self should not be kept visible. It doesn't need to be.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(self));
    LOG_OUTPUT(L"   > Calling ReleaseKeepAlive on self.");
    RunOnUIThread([&]() { wh->ReleaseKeepAlive(self); }); wh->WaitForIdle();
    LOG_OUTPUT(L"     > parent should not be kept visible anymore.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(parent));
    LOG_OUTPUT(L"   > Restoring tree.");
    RunOnUIThread([&]() { parent->Visibility = Visibility::Visible; }); wh->WaitForIdle();

    LOG_OUTPUT(L">> KeepAlive on parent and self:");
    LOG_OUTPUT(L"   > Calling RequestKeepAlive on parent.");
    RunOnUIThread([&]() { wh->RequestKeepAlive(parent); }); wh->WaitForIdle();
    LOG_OUTPUT(L"   > Calling RequestKeepAlive on self.");
    RunOnUIThread([&]() { wh->RequestKeepAlive(self); }); wh->WaitForIdle();
    LOG_OUTPUT(L"   > Collapsing parent.");
    RunOnUIThread([&]() { parent->Visibility = Visibility::Collapsed; }); wh->WaitForIdle();
    LOG_OUTPUT(L"     > parent should be kept visible.");
    VERIFY_IS_TRUE(wh->IsKeepingVisible(parent));
    LOG_OUTPUT(L"     > self should not be kept visible - it wasn't collapsed.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(self));
    LOG_OUTPUT(L"   > Calling ReleaseKeepAlive on parent.");
    RunOnUIThread([&]() { wh->ReleaseKeepAlive(parent); }); wh->WaitForIdle();
    LOG_OUTPUT(L"     > self should still not be kept visible.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(self));
    LOG_OUTPUT(L"     > parent should still kept visible via self.");
    VERIFY_IS_TRUE(wh->IsKeepingVisible(parent));
    LOG_OUTPUT(L"   > Calling ReleaseKeepAlive on self.");
    RunOnUIThread([&]() { wh->ReleaseKeepAlive(self); }); wh->WaitForIdle();
    LOG_OUTPUT(L"     > self should still not be kept visible.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(self));
    LOG_OUTPUT(L"     > parent should no longer be kept visible either.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(parent));
    LOG_OUTPUT(L"   > Restoring tree.");
    RunOnUIThread([&]() { parent->Visibility = Visibility::Visible; }); wh->WaitForIdle();
}

void MotionTests::KeepAlive_HiddenEventHandler()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;

    auto selfHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);

    std::shared_ptr<Event> selfHiddenEvent = std::make_shared<Event>();

    xaml_shapes::Rectangle^ content;
    Grid^ grandparent;
    Grid^ parent;
    Grid^ self;
    xaml_primitives::Popup^ popup;
    Grid^ popupContent;
    xaml_primitives::Popup^ parentlessPopup;
    Grid^ parentlessPopupContent;

    wh->SetWindowSizeOverride(wf::Size(400, 400));
    Canvas^ root;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        grandparent = ref new Grid();
        parent = ref new Grid();
        self = ref new Grid();
        content = ref new xaml_shapes::Rectangle();
        popup = ref new xaml_primitives::Popup();
        popupContent = ref new Grid();
        parentlessPopup = ref new xaml_primitives::Popup();
        parentlessPopupContent = ref new Grid();

        LOG_OUTPUT(L"> Attaching handler to UIElement.Hidden event.");
        selfHiddenRegistration.Attach(self, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Self UIElement.Hidden event fired."); EventHandlerHelper(sender, args, self, selfHiddenEvent); }));

        root = ref new Canvas();
        wh->WindowContent = root;

        {
            auto xamlRoot = root->XamlRoot;
            if (xamlRoot)
            {
                // UAP will return a null content root and does not need this to be set
                parentlessPopup->XamlRoot = xamlRoot;
            }
        }

        LOG_OUTPUT(L"> Attaching tree.");
        root->Children->Append(grandparent);
        grandparent->Children->Append(parent);
        parent->Children->Append(self);
        self->Children->Append(content);
        self->Children->Append(popup); popup->IsOpen = true;
        popup->Child = popupContent;
        parentlessPopup->IsOpen = true;
        parentlessPopup->Child = parentlessPopupContent;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Removing self from the tree.");
        parent->Children->RemoveAt(0);

        LOG_OUTPUT(L"> self should be immediately marked as KeepAlive.");
        VERIFY_IS_TRUE(wh->IsKeepingVisible(self));
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for self UIElement.Hidden event.");
    selfHiddenEvent->WaitForDefault();
    selfHiddenEvent->Reset();

    LOG_OUTPUT(L"> self should no longer be marked as KeepAlive.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(self));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Restoring tree.");
        parent->Children->Append(self);
    });
    wh->WaitForIdle();

    ///

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Removing content from the tree.");
        self->Children->RemoveAt(0);

        LOG_OUTPUT(L"> content should not be marked as KeepAlive. It has no Hidden event handler.");
        VERIFY_IS_FALSE(wh->IsKeepingVisible(content));
    });
    wh->WaitForIdle();

    ///

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Removing grandparent from the tree.");
        root->Children->RemoveAt(0);

        LOG_OUTPUT(L"> grandparent should be immediately marked as KeepAlive.");
        VERIFY_IS_TRUE(wh->IsKeepingVisible(grandparent));
        LOG_OUTPUT(L"> self should not be marked as KeepAlive. It doesn't need to be.");
        VERIFY_IS_FALSE(wh->IsKeepingVisible(self));
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for self UIElement.Hidden event.");
    selfHiddenEvent->WaitForDefault();
    selfHiddenEvent->Reset();

    LOG_OUTPUT(L"> grandparent should no longer be marked as KeepAlive.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(self));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Restoring tree.");
        root->Children->Append(grandparent);
    });
    wh->WaitForIdle();

    ///

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Calling RequestKeepAlive on self.");
        wh->RequestKeepAlive(self);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> self should not be marked as KeepVisible yet. It's still in the tree.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(self));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Removing self from the tree.");
        parent->Children->RemoveAt(0);

        LOG_OUTPUT(L"> self should be immediately marked as KeepAlive.");
        VERIFY_IS_TRUE(wh->IsKeepingVisible(self));
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for self UIElement.Hidden event.");
    selfHiddenEvent->WaitForDefault();
    selfHiddenEvent->Reset();

    LOG_OUTPUT(L"> self should still be marked as KeepAlive because we called RequestKeepAlive before UIE.Hidden.");
    VERIFY_IS_TRUE(wh->IsKeepingVisible(self));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Calling ReleaseKeepAlive on self.");
        wh->ReleaseKeepAlive(self);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> self should no longer be marked as KeepAlive.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(self));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Restoring tree.");
        parent->Children->Append(self);
    });
    wh->WaitForIdle();

    ///

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Attaching new handler to self's UIElement::Hidden event.");
        selfHiddenRegistration.Detach();
        selfHiddenRegistration.Attach(self, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            {
                LOG_OUTPUT(L"  > Self UIElement.Hidden event fired. Calling RequestKeepAlive on self.");
                EventHandlerHelper(sender, args, self, selfHiddenEvent);
                wh->RequestKeepAlive(self);
            }));
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Removing self from the tree.");
        parent->Children->RemoveAt(0);

        LOG_OUTPUT(L"> self should be immediately marked as KeepAlive.");
        VERIFY_IS_TRUE(wh->IsKeepingVisible(self));
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for self UIElement.Hidden event.");
    selfHiddenEvent->WaitForDefault();
    selfHiddenEvent->Reset();

    LOG_OUTPUT(L"> self should still be marked as KeepAlive because the Hidden handler called it.");
    VERIFY_IS_TRUE(wh->IsKeepingVisible(self));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Calling ReleaseKeepAlive on self.");
        wh->ReleaseKeepAlive(self);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> self should no longer be marked as KeepAlive.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(self));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Restoring tree.");
        selfHiddenRegistration.Detach();
        parent->Children->Append(self);
    });
    wh->WaitForIdle();
}

void MotionTests::KeepAlive_HiddenEventHandlerPopup()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;

    auto popupHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);
    auto parentlessPopupHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);

    std::shared_ptr<Event> popupHiddenEvent = std::make_shared<Event>();
    std::shared_ptr<Event> parentlessPopupHiddenEvent = std::make_shared<Event>();

    xaml_shapes::Rectangle^ content;
    Grid^ grandparent;
    Grid^ parent;
    Grid^ self;
    xaml_primitives::Popup^ popup;
    Grid^ popupContent;
    xaml_primitives::Popup^ parentlessPopup;
    Grid^ parentlessPopupContent;

    wh->SetWindowSizeOverride(wf::Size(400, 400));
    Canvas^ root;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        grandparent = ref new Grid();
        parent = ref new Grid();
        self = ref new Grid();
        content = ref new xaml_shapes::Rectangle();
        popup = ref new xaml_primitives::Popup();
        popupContent = ref new Grid();
        parentlessPopup = ref new xaml_primitives::Popup();
        parentlessPopupContent = ref new Grid();

        LOG_OUTPUT(L"> Attaching handler to UIElement.Hidden events.");
        popupHiddenRegistration.Attach(popup, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > Popup UIElement.Hidden event fired."); EventHandlerHelper(sender, args, popup, popupHiddenEvent); }));
        parentlessPopupHiddenRegistration.Attach(parentlessPopup, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > ParentlessPopup UIElement.Hidden event fired."); EventHandlerHelper(sender, args, parentlessPopup, parentlessPopupHiddenEvent); }));

        root = ref new Canvas();
        wh->WindowContent = root;

        {
            auto xamlRoot = root->XamlRoot;
            if (xamlRoot)
            {
                // UAP will return a null content root and does not need this to be set
                parentlessPopup->XamlRoot = xamlRoot;
            }
        }

        LOG_OUTPUT(L"> Attaching tree.");
        root->Children->Append(grandparent);
        grandparent->Children->Append(parent);
        parent->Children->Append(self);
        self->Children->Append(content);
        self->Children->Append(popup); popup->IsOpen = true;
        popup->Child = popupContent;
        parentlessPopup->IsOpen = true;
        parentlessPopup->Child = parentlessPopupContent;
    });
    wh->WaitForIdle();

    ///

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Closing popup.");
        popup->IsOpen = false;

        LOG_OUTPUT(L"> popup should be immediately marked as KeepAlive.");
        VERIFY_IS_TRUE(wh->IsKeepingVisible(popup));
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for popup UIElement.Hidden event.");
    popupHiddenEvent->WaitForDefault();
    popupHiddenEvent->Reset();

    LOG_OUTPUT(L"> popup should no longer be marked as KeepAlive.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(popup));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Restoring tree.");
        popup->IsOpen = true;
    });
    wh->WaitForIdle();

    ///

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Removing popup from the tree.");
        self->Children->RemoveAt(1);

        LOG_OUTPUT(L"> popup should be immediately marked as KeepAlive. It's implicitly closed when it's removed from the tree.");
        VERIFY_IS_TRUE(wh->IsKeepingVisible(popup));
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for popup UIElement.Hidden event.");
    popupHiddenEvent->WaitForDefault();
    popupHiddenEvent->Reset();

    LOG_OUTPUT(L"> popup should not be marked as KeepAlive now.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(popup));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Restoring tree.");
        VERIFY_IS_FALSE(popup->IsOpen);
        self->Children->Append(popup);
        popup->IsOpen = true;
    });
    wh->WaitForIdle();

    ///

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Removing parent from the tree.");
        grandparent->Children->RemoveAt(0);

        LOG_OUTPUT(L"> parent should be immediately marked as KeepAlive.");
        VERIFY_IS_TRUE(wh->IsKeepingVisible(parent));
        LOG_OUTPUT(L"> popup should not be marked as KeepAlive. It doesn't need to be.");
        VERIFY_IS_FALSE(wh->IsKeepingVisible(popup));
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for popup UIElement.Hidden event.");
    popupHiddenEvent->WaitForDefault();
    popupHiddenEvent->Reset();

    LOG_OUTPUT(L"> grandparent should no longer be marked as KeepAlive.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(parent));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Restoring tree.");
        VERIFY_IS_FALSE(popup->IsOpen);
        grandparent->Children->Append(parent);
        popup->IsOpen = true;
    });
    wh->WaitForIdle();

    ///

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Calling RequestKeepAlive on popup.");
        wh->RequestKeepAlive(popup);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> popup should not be marked as KeepVisible yet. It's still open.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(popup));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Closing popup.");
        popup->IsOpen = false;

        LOG_OUTPUT(L"> popup should be immediately marked as KeepAlive.");
        VERIFY_IS_TRUE(wh->IsKeepingVisible(popup));
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for popup UIElement.Hidden event.");
    popupHiddenEvent->WaitForDefault();
    popupHiddenEvent->Reset();

    LOG_OUTPUT(L"> popup should still be marked as KeepAlive because we called RequestKeepAlive before UIE.Hidden.");
    VERIFY_IS_TRUE(wh->IsKeepingVisible(popup));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Calling ReleaseKeepAlive on popup.");
        wh->ReleaseKeepAlive(popup);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> popup should no longer be marked as KeepAlive.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(popup));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Restoring tree.");
        popup->IsOpen = true;
    });
    wh->WaitForIdle();

    ///

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Attaching new handler to popup's UIElement::Hidden event.");
        popupHiddenRegistration.Detach();
        popupHiddenRegistration.Attach(popup, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            {
                LOG_OUTPUT(L"  > Popup UIElement.Hidden event fired. Calling RequestKeepAlive on popup.");
                EventHandlerHelper(sender, args, popup, popupHiddenEvent);
                wh->RequestKeepAlive(popup);
            }));
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Closing popup.");
        popup->IsOpen = false;

        LOG_OUTPUT(L"> popup should be immediately marked as KeepAlive.");
        VERIFY_IS_TRUE(wh->IsKeepingVisible(popup));
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for popup UIElement.Hidden event.");
    popupHiddenEvent->WaitForDefault();
    popupHiddenEvent->Reset();

    LOG_OUTPUT(L"> popup should still be marked as KeepAlive because the Hidden handler called it.");
    VERIFY_IS_TRUE(wh->IsKeepingVisible(popup));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Calling ReleaseKeepAlive on popup.");
        wh->ReleaseKeepAlive(popup);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> popup should no longer be marked as KeepAlive.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(popup));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Restoring tree.");
        popupHiddenRegistration.Detach();
        popup->IsOpen = true;
    });
    wh->WaitForIdle();

    ///

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Removing popupContent from the tree.");
        popup->Child = nullptr;

        LOG_OUTPUT(L"> popup should not be marked as KeepAlive. It wasn't closed.");
        VERIFY_IS_FALSE(wh->IsKeepingVisible(popup));
        LOG_OUTPUT(L"> popupContent should not be marked as KeepAlive. It has no Hidden event handler.");
        VERIFY_IS_FALSE(wh->IsKeepingVisible(popupContent));
    });
    wh->WaitForIdle();

    ///

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Closing parentlessPopup.");
        parentlessPopup->IsOpen = false;

        LOG_OUTPUT(L"> parentlessPopup should be immediately marked as KeepAlive.");
        VERIFY_IS_TRUE(wh->IsKeepingVisible(parentlessPopup));
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for parentlessPopup UIElement.Hidden event.");
    parentlessPopupHiddenEvent->WaitForDefault();
    parentlessPopupHiddenEvent->Reset();

    LOG_OUTPUT(L"> parentlessPopup should no longer be marked as KeepAlive.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(parentlessPopup));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Restoring tree.");
        parentlessPopup->IsOpen = true;
    });
    wh->WaitForIdle();

    ///

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Calling RequestKeepAlive on parentlessPopup.");
        wh->RequestKeepAlive(parentlessPopup);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> parentlessPopup should not be marked as KeepVisible yet. It's still open.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(parentlessPopup));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Closing parentlessPopup.");
        parentlessPopup->IsOpen = false;

        LOG_OUTPUT(L"> parentlessPopup should be immediately marked as KeepAlive.");
        VERIFY_IS_TRUE(wh->IsKeepingVisible(parentlessPopup));
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for parentlessPopup UIElement.Hidden event.");
    parentlessPopupHiddenEvent->WaitForDefault();
    parentlessPopupHiddenEvent->Reset();

    LOG_OUTPUT(L"> parentlessPopup should still be marked as KeepAlive because we called RequestKeepAlive before UIE.Hidden.");
    VERIFY_IS_TRUE(wh->IsKeepingVisible(parentlessPopup));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Calling ReleaseKeepAlive on parentlessPopup.");
        wh->ReleaseKeepAlive(parentlessPopup);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> parentlessPopup should no longer be marked as KeepAlive.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(parentlessPopup));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Restoring tree.");
        parentlessPopup->IsOpen = true;
    });
    wh->WaitForIdle();

    ///

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Attaching new handler to parentlessPopup's UIElement::Hidden event.");

        parentlessPopupHiddenRegistration.Detach();
        parentlessPopupHiddenRegistration.Attach(parentlessPopup, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            {
                LOG_OUTPUT(L"  > ParentlessPopup UIElement.Hidden event fired. Calling RequestKeepAlive on self.");
                EventHandlerHelper(sender, args, parentlessPopup, parentlessPopupHiddenEvent);
                wh->RequestKeepAlive(parentlessPopup);
            }));
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Closing parentlessPopup.");
        parentlessPopup->IsOpen = false;

        LOG_OUTPUT(L"> parentlessPopup should be immediately marked as KeepAlive.");
        VERIFY_IS_TRUE(wh->IsKeepingVisible(parentlessPopup));
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for parentlessPopup UIElement.Hidden event.");
    parentlessPopupHiddenEvent->WaitForDefault();
    parentlessPopupHiddenEvent->Reset();

    LOG_OUTPUT(L"> parentlessPopup should still be marked as KeepAlive because the Hidden handler called it.");
    VERIFY_IS_TRUE(wh->IsKeepingVisible(parentlessPopup));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Calling ReleaseKeepAlive on parentlessPopup.");
        wh->ReleaseKeepAlive(parentlessPopup);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> parentlessPopup should no longer be marked as KeepAlive.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(parentlessPopup));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Restoring tree.");
        parentlessPopupHiddenRegistration.Detach();
        parentlessPopup->IsOpen = true;
    });
    wh->WaitForIdle();
}

void MotionTests::KeepAlive_HiddenEventHandlerPopupContent()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto wh = TestServices::WindowHelper;

    auto popupContentHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);
    auto parentlessPopupContentHiddenRegistration = CreateSafeEventRegistration(UIElement, Hidden);

    std::shared_ptr<Event> popupContentHiddenEvent = std::make_shared<Event>();
    std::shared_ptr<Event> parentlessPopupContentHiddenEvent = std::make_shared<Event>();

    xaml_shapes::Rectangle^ content;
    Grid^ grandparent;
    Grid^ parent;
    Grid^ self;
    xaml_primitives::Popup^ popup;
    Grid^ popupContent;
    xaml_primitives::Popup^ parentlessPopup;
    Grid^ parentlessPopupContent;

    wh->SetWindowSizeOverride(wf::Size(400, 400));
    Canvas^ root;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        grandparent = ref new Grid();
        parent = ref new Grid();
        self = ref new Grid();
        content = ref new xaml_shapes::Rectangle();
        popup = ref new xaml_primitives::Popup();
        popupContent = ref new Grid();
        parentlessPopup = ref new xaml_primitives::Popup();
        parentlessPopupContent = ref new Grid();

        LOG_OUTPUT(L"> Attaching handler to UIElement.Hidden events.");
        popupContentHiddenRegistration.Attach(popupContent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > PopupContent UIElement.Hidden event fired."); EventHandlerHelper(sender, args, popupContent, popupContentHiddenEvent); }));
        parentlessPopupContentHiddenRegistration.Attach(parentlessPopupContent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            { LOG_OUTPUT(L"  > ParentlessPopupContent UIElement.Hidden event fired."); EventHandlerHelper(sender, args, parentlessPopupContent, parentlessPopupContentHiddenEvent); }));

        root = ref new Canvas();
        wh->WindowContent = root;

        {
            auto xamlRoot = root->XamlRoot;
            if (xamlRoot)
            {
                // UAP will return a null content root and does not need this to be set
                parentlessPopup->XamlRoot = xamlRoot;
            }
        }

        LOG_OUTPUT(L"> Attaching tree.");
        root->Children->Append(grandparent);
        grandparent->Children->Append(parent);
        parent->Children->Append(self);
        self->Children->Append(content);
        self->Children->Append(popup); popup->IsOpen = true;
        popup->Child = popupContent;
        parentlessPopup->IsOpen = true;
        parentlessPopup->Child = parentlessPopupContent;
    });
    wh->WaitForIdle();

    ///

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Closing popup.");
        popup->IsOpen = false;

        LOG_OUTPUT(L"> popup should be immediately marked as KeepAlive.");
        VERIFY_IS_TRUE(wh->IsKeepingVisible(popup));
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for popupContent UIElement.Hidden event.");
    popupContentHiddenEvent->WaitForDefault();
    popupContentHiddenEvent->Reset();

    LOG_OUTPUT(L"> popup should no longer be marked as KeepAlive.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(popup));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Restoring tree.");
        popup->IsOpen = true;
    });
    wh->WaitForIdle();

    ///

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Removing popup from the tree.");
        self->Children->RemoveAt(1);

        LOG_OUTPUT(L"> popup should be immediately marked as KeepAlive. It's implicitly closed when it's removed from the tree.");
        VERIFY_IS_TRUE(wh->IsKeepingVisible(popup));
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for popupContent UIElement.Hidden event.");
    popupContentHiddenEvent->WaitForDefault();
    popupContentHiddenEvent->Reset();

    LOG_OUTPUT(L"> popup should not be marked as KeepAlive now.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(popup));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Restoring tree.");
        VERIFY_IS_FALSE(popup->IsOpen);
        self->Children->Append(popup);
        popup->IsOpen = true;
    });
    wh->WaitForIdle();

    ///

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Removing parent from the tree.");
        grandparent->Children->RemoveAt(0);

        LOG_OUTPUT(L"> parent should be immediately marked as KeepAlive.");
        VERIFY_IS_TRUE(wh->IsKeepingVisible(parent));
        LOG_OUTPUT(L"> popupContent should not be marked as KeepAlive. It doesn't need to be.");
        VERIFY_IS_FALSE(wh->IsKeepingVisible(popupContent));
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for popupContent UIElement.Hidden event.");
    popupContentHiddenEvent->WaitForDefault();
    popupContentHiddenEvent->Reset();

    LOG_OUTPUT(L"> grandparent should no longer be marked as KeepAlive.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(parent));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Restoring tree.");
        VERIFY_IS_FALSE(popup->IsOpen);
        grandparent->Children->Append(parent);
        popup->IsOpen = true;
    });
    wh->WaitForIdle();

    ///

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Removing popupContent from the tree.");
        popup->Child = nullptr;

        LOG_OUTPUT(L"> popupContent should be immediately marked as KeepAlive.");
        VERIFY_IS_TRUE(wh->IsKeepingVisible(popupContent));
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for popupContent UIElement.Hidden event.");
    popupContentHiddenEvent->WaitForDefault();
    popupContentHiddenEvent->Reset();

    LOG_OUTPUT(L"> popupContent should no longer be marked as KeepAlive.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(popupContent));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Restoring tree.");
        popup->Child = popupContent;
    });
    wh->WaitForIdle();

    ///

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Calling RequestKeepAlive on popupContent.");
        wh->RequestKeepAlive(popupContent);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> popupContent should not be marked as KeepVisible yet. It's still open.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(popupContent));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Closing popup.");
        popup->IsOpen = false;

        LOG_OUTPUT(L"> popupContent should be immediately marked as KeepAlive.");
        VERIFY_IS_TRUE(wh->IsKeepingVisible(popupContent));
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for popupContent UIElement.Hidden event.");
    popupContentHiddenEvent->WaitForDefault();
    popupContentHiddenEvent->Reset();

    LOG_OUTPUT(L"> popupContent should still be marked as KeepAlive because we called RequestKeepAlive before UIE.Hidden.");
    VERIFY_IS_TRUE(wh->IsKeepingVisible(popupContent));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Calling ReleaseKeepAlive on popupContent.");
        wh->ReleaseKeepAlive(popupContent);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> popupContent should no longer be marked as KeepAlive.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(popupContent));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Restoring tree.");
        popup->IsOpen = true;
    });
    wh->WaitForIdle();

    ///

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Attaching new handler to popupContent's UIElement::Hidden event.");

        popupContentHiddenRegistration.Detach();
        popupContentHiddenRegistration.Attach(popupContent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            {
                LOG_OUTPUT(L"  > popupContent UIElement.Hidden event fired. Calling RequestKeepAlive on self.");
                EventHandlerHelper(sender, args, popupContent, popupContentHiddenEvent);
                wh->RequestKeepAlive(popupContent);
            }));
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Closing popup.");
        popup->IsOpen = false;

        LOG_OUTPUT(L"> popupContent should be immediately marked as KeepAlive.");
        VERIFY_IS_TRUE(wh->IsKeepingVisible(popupContent));
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for popupContent UIElement.Hidden event.");
    popupContentHiddenEvent->WaitForDefault();
    popupContentHiddenEvent->Reset();

    LOG_OUTPUT(L"> popupContent should still be marked as KeepAlive because the Hidden handler called it.");
    VERIFY_IS_TRUE(wh->IsKeepingVisible(popupContent));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Calling ReleaseKeepAlive on popupContent.");
        wh->ReleaseKeepAlive(popupContent);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> popupContent should no longer be marked as KeepAlive.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(popupContent));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Restoring tree.");
        popupContentHiddenRegistration.Detach();
        popup->IsOpen = true;
    });
    wh->WaitForIdle();

    ///

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Closing parentlessPopup.");
        parentlessPopup->IsOpen = false;

        LOG_OUTPUT(L"> parentlessPopup should be immediately marked as KeepAlive.");
        VERIFY_IS_TRUE(wh->IsKeepingVisible(parentlessPopup));
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for parentlessPopupContent UIElement.Hidden event.");
    parentlessPopupContentHiddenEvent->WaitForDefault();
    parentlessPopupContentHiddenEvent->Reset();

    LOG_OUTPUT(L"> parentlessPopup should no longer be marked as KeepAlive.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(parentlessPopup));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Restoring tree.");
        parentlessPopup->IsOpen = true;
    });
    wh->WaitForIdle();

    ///

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Calling RequestKeepAlive on parentlessPopupContent.");
        wh->RequestKeepAlive(parentlessPopupContent);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> parentlessPopupContent should not be marked as KeepVisible yet. It's still open.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(parentlessPopupContent));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Closing parentlessPopup.");
        parentlessPopup->IsOpen = false;

        LOG_OUTPUT(L"> parentlessPopupContent should be immediately marked as KeepAlive.");
        VERIFY_IS_TRUE(wh->IsKeepingVisible(parentlessPopupContent));
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for parentlessPopupContent UIElement.Hidden event.");
    parentlessPopupContentHiddenEvent->WaitForDefault();
    parentlessPopupContentHiddenEvent->Reset();

    LOG_OUTPUT(L"> parentlessPopupContent should still be marked as KeepAlive because we called RequestKeepAlive before UIE.Hidden.");
    VERIFY_IS_TRUE(wh->IsKeepingVisible(parentlessPopupContent));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Calling ReleaseKeepAlive on parentlessPopupContent.");
        wh->ReleaseKeepAlive(parentlessPopupContent);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> parentlessPopupContent should no longer be marked as KeepAlive.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(parentlessPopupContent));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Restoring tree.");
        parentlessPopup->IsOpen = true;
    });
    wh->WaitForIdle();

    ///

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L">> Attaching new handler to parentlessPopupContent's UIElement::Hidden event.");

        parentlessPopupContentHiddenRegistration.Detach();
        parentlessPopupContentHiddenRegistration.Attach(parentlessPopupContent, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^ sender, Platform::Object^ args)
            {
                LOG_OUTPUT(L"  > parentlessPopupContent UIElement.Hidden event fired. Calling RequestKeepAlive on self.");
                EventHandlerHelper(sender, args, parentlessPopupContent, parentlessPopupContentHiddenEvent);
                wh->RequestKeepAlive(parentlessPopupContent);
            }));
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Closing parentlessPopup.");
        parentlessPopup->IsOpen = false;

        LOG_OUTPUT(L"> parentlessPopupContent should be immediately marked as KeepAlive.");
        VERIFY_IS_TRUE(wh->IsKeepingVisible(parentlessPopupContent));
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> Waiting for parentlessPopupContent UIElement.Hidden event.");
    parentlessPopupContentHiddenEvent->WaitForDefault();
    parentlessPopupContentHiddenEvent->Reset();

    LOG_OUTPUT(L"> parentlessPopupContent should still be marked as KeepAlive because the Hidden handler called it.");
    VERIFY_IS_TRUE(wh->IsKeepingVisible(parentlessPopupContent));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Calling ReleaseKeepAlive on parentlessPopupContent.");
        wh->ReleaseKeepAlive(parentlessPopupContent);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"> parentlessPopupContent should no longer be marked as KeepAlive.");
    VERIFY_IS_FALSE(wh->IsKeepingVisible(parentlessPopupContent));

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Restoring tree.");
        parentlessPopupContentHiddenRegistration.Detach();
        parentlessPopup->IsOpen = true;
    });
    wh->WaitForIdle();
}

#endif

} } } } } }
