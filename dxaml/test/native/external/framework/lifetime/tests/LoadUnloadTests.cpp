// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "LoadUnloadTests.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <wil/result_macros.h>
#include <SafeEventRegistration.h>
#include <TestEvent.h>

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace WEX::Logging;
using namespace WEX::Common;
using namespace Microsoft;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {
        namespace Lifetime {

            bool LoadUnloadTests::ClassSetup()
            {
                CommonTestSetupHelper::CommonTestClassSetup();
                return true;
            }

            bool LoadUnloadTests::TestSetup()
            {
                TestServices::WindowHelper->InitializeXaml();
                return true;
            }
            bool LoadUnloadTests::TestCleanup()
            {
                TestServices::WindowHelper->ShutdownXaml();
                TestServices::WindowHelper->VerifyTestCleanup();
                return true;
            }

            void LoadUnloadTests::VerifyEventOrdering()
            {
                xaml_controls::StackPanel^ rootPanel = nullptr;
                xaml_controls::Grid^ grid = nullptr;
                xaml_controls::Button^ button = nullptr;
                auto buttonLoadingEvent = std::make_shared<Event>();
                auto buttonLoadedEvent = std::make_shared<Event>();
                auto buttonUnloadedEvent = std::make_shared<Event>();

                auto gridLoadedEvent = std::make_shared<Event>();
                auto gridUnloadedEvent = std::make_shared<Event>();

                auto buttonLoadingRegistration = CreateSafeEventRegistration(xaml_controls::Button, Loading);
                auto buttonLoadedRegistration = CreateSafeEventRegistration(xaml_controls::Button, Loaded);
                auto buttonUnloadedRegistration = CreateSafeEventRegistration(xaml_controls::Button, Unloaded);
                auto gridLoadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);
                auto gridUnloadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Unloaded);

                RunOnUIThread([&]()
                {
                    rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                        L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                        L"  <Grid x:Name='grid'>"
                        L"    <Button x:Name='btn' Content='1234' />"
                        L"  </Grid>"
                        L"</StackPanel>"));

                    grid = safe_cast<xaml_controls::Grid^>(rootPanel->FindName(L"grid"));
                    VERIFY_IS_NOT_NULL(grid);

                    button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"btn"));
                    VERIFY_IS_NOT_NULL(button);
                    TestServices::WindowHelper->WindowContent = rootPanel;

                    buttonLoadingRegistration.Attach(button, [&]() {
                        LOG_OUTPUT(L"    Button loading...");
                        auto parent = xaml_media::VisualTreeHelper::GetParent(button);
                        VERIFY_IS_NOT_NULL(parent);
                        buttonLoadingEvent->Set();
                    });
                    buttonLoadedRegistration.Attach(button, [&]() {
                        LOG_OUTPUT(L"    Button loaded...");
                        VERIFY_IS_FALSE(buttonUnloadedEvent->HasFired());
                        auto parent = xaml_media::VisualTreeHelper::GetParent(button);
                        VERIFY_IS_NOT_NULL(parent);
                        buttonLoadedEvent->Set();
                    });
                    buttonUnloadedRegistration.Attach(button, [&]() {
                        LOG_OUTPUT(L"    Button unloaded...");
                        auto parent = xaml_media::VisualTreeHelper::GetParent(button);
                        VERIFY_IS_NULL(parent);
                        buttonUnloadedEvent->Set();
                    });
                    gridLoadedRegistration.Attach(grid, [&]() {
                        LOG_OUTPUT(L"    Grid loaded...");
                        VERIFY_IS_TRUE(buttonLoadedEvent->HasFired()); //Child should fire Loaded event before parent
                        VERIFY_IS_FALSE(gridUnloadedEvent->HasFired());
                        auto parent = xaml_media::VisualTreeHelper::GetParent(grid);
                        VERIFY_IS_NOT_NULL(parent);
                        gridLoadedEvent->Set();
                    });
                    gridUnloadedRegistration.Attach(grid, [&]() {
                        LOG_OUTPUT(L"    Grid unloaded...");
                        VERIFY_IS_FALSE(buttonUnloadedEvent->HasFired());//Child should fire Unloaded event after parent
                        VERIFY_IS_TRUE(gridLoadedEvent->HasFired());
                        auto parent = xaml_media::VisualTreeHelper::GetParent(grid);
                        VERIFY_IS_NULL(parent);
                        gridUnloadedEvent->Set();
                    });
                });

                LOG_OUTPUT(L"1. Verify loading and loaded event order.");
                buttonLoadingEvent->WaitForDefault();
                VERIFY_IS_FALSE(buttonLoadedEvent->HasFired());
                buttonLoadedEvent->WaitForDefault();
                RunOnUIThread([&]()
                {
                    VERIFY_IS_TRUE(button->IsLoaded);
                });
                TestServices::WindowHelper->WaitForIdle();

                LOG_OUTPUT(L"2. Remove button from tree and verify unloaded event.");
                buttonUnloadedEvent->Reset();

                RunOnUIThread([&]()
                {
                    grid->Children->RemoveAt(0);
                });
                buttonUnloadedEvent->WaitForDefault();
                RunOnUIThread([&]()
                {
                    VERIFY_IS_FALSE(button->IsLoaded);
                });
                TestServices::WindowHelper->WaitForIdle();

                LOG_OUTPUT(L"3. Add and remove button from tree, verify loaded and unloaded event order..");
                buttonLoadedEvent->Reset();
                buttonUnloadedEvent->Reset();

                RunOnUIThread([&]()
                {
                    grid->Children->Append(button);
                    grid->Children->RemoveAt(0);
                });

                // Current behavior: Orphaned Unloaded event firing when element is added and removed before Tick
                buttonUnloadedEvent->WaitForDefault();
                buttonLoadedEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
                VERIFY_IS_FALSE(buttonLoadedEvent->HasFired());

                RunOnUIThread([&]()
                {
                    VERIFY_IS_FALSE(button->IsLoaded);
                });

                TestServices::WindowHelper->WaitForIdle();

                buttonLoadedEvent->Reset();
                buttonUnloadedEvent->Reset();

                RunOnUIThread([&]()
                {
                    grid->Children->Append(button);
                    VERIFY_IS_FALSE(button->IsLoaded);
                });
                buttonLoadedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                LOG_OUTPUT(L"4. Remove parent Grid from tree and verify unloaded events.");
                gridUnloadedEvent->Reset();
                buttonLoadedEvent->Reset();
                buttonUnloadedEvent->Reset();
                RunOnUIThread([&]()
                {
                    rootPanel->Children->RemoveAt(0);
                });

                gridUnloadedEvent->WaitForDefault();
                buttonUnloadedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                LOG_OUTPUT(L"5. Add back parent Grid and verify events ordering.");
                gridLoadedEvent->Reset();
                buttonLoadedEvent->Reset();
                buttonUnloadedEvent->Reset();
                gridUnloadedEvent->Reset();
                RunOnUIThread([&]()
                {
                    rootPanel->Children->Append(grid);
                });

                buttonLoadedEvent->WaitForDefault();
                gridLoadedEvent->WaitForDefault();
        }

        void LoadUnloadTests::VerifyEventOrderingForPopup()
        {
            xaml_controls::StackPanel^ rootPanel = nullptr;
            xaml_primitives::Popup^ popup = nullptr;
            xaml_controls::Grid^ grid = nullptr;

            auto popupLoadedEvent = std::make_shared<Event>();
            auto popupUnloadedEvent = std::make_shared<Event>();

            auto popupLoadedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Loaded);
            auto popupUnloadedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Unloaded);

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;

                popup = ref new xaml_primitives::Popup();
                grid = ref new xaml_controls::Grid();
                grid->Width = 200;
                grid->Height = 200;
                popup->Child = grid;

                rootPanel->Children->Append(popup);

                popupLoadedRegistration.Attach(popup, [&]() {
                    LOG_OUTPUT(L"    Popup loaded...");
                    VERIFY_IS_FALSE(popupUnloadedEvent->HasFired());
                    popupLoadedEvent->Set();
                });
                popupUnloadedRegistration.Attach(popup, [&]() {
                    LOG_OUTPUT(L"    Popup unloaded...");
                    popupUnloadedEvent->Set();
                });
            });
            TestServices::WindowHelper->WaitForIdle();
            //Popup Loaded event will fire on Tick even if it not in live tree, as soon as at the tick after the event listener is registered
            popupLoadedEvent->WaitForDefault();

            popupLoadedEvent->Reset();
            LOG_OUTPUT(L"1. Verify loaded event does not fire again.");
            RunOnUIThread([&]()
            {
                popup->IsOpen = true;
            });

            popupLoadedEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
            VERIFY_IS_FALSE(popupLoadedEvent->HasFired());

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"2. Remove popup and verify unloaded event.");
            popupUnloadedEvent->Reset();

            RunOnUIThread([&]()
            {
                rootPanel->Children->RemoveAt(0);
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void LoadUnloadTests::VerifyEventOrderingForPopupAddAndRemove()
        {
            xaml_controls::StackPanel^ rootPanel = nullptr;
            xaml_primitives::Popup^ popup = nullptr;
            xaml_controls::Grid^ grid = nullptr;

            auto popupLoadedEvent = std::make_shared<Event>();
            auto popupUnloadedEvent = std::make_shared<Event>();

            auto popupLoadedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Loaded);
            auto popupUnloadedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Unloaded);

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;

                popup = ref new xaml_primitives::Popup();
                grid = ref new xaml_controls::Grid();
                grid->Width = 200;
                grid->Height = 200;
                popup->Child = grid;
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Add and remove popup from tree, verify loaded and unloaded event order..");

            RunOnUIThread([&]()
            {
                popupLoadedRegistration.Attach(popup, [&]() {
                    LOG_OUTPUT(L"    Popup loaded...");
                    VERIFY_IS_TRUE(popupUnloadedEvent->HasFired());
                    popupLoadedEvent->Set();
                });
                popupUnloadedRegistration.Attach(popup, [&]() {
                    LOG_OUTPUT(L"    Popup unloaded...");
                    popupUnloadedEvent->Set();
                });
                rootPanel->Children->Append(popup);
                rootPanel->Children->RemoveAt(0);
            });

            // Current behavior:Unloaded event firing first before Loaded event is fired on Popup
            popupUnloadedEvent->WaitForDefault();
            popupLoadedEvent->WaitForDefault();
        }

        ref class MyPropValue sealed
        {
        public:
            MyPropValue(DependencyObject^ obj)
            {
                m_weakRef = obj;
            }

            virtual ~MyPropValue()
            {
                LOG_OUTPUT(L"~MyPropValue()");
                DependencyObject^ resolved = m_weakRef.Resolve<DependencyObject>();
                LOG_OUTPUT(L"Weak ref resolved to: %p", reinterpret_cast<IUnknown*>(resolved));
                LOG_OUTPUT(L"~MyPropValue() completed.");
            }

        private:
            Platform::WeakReference m_weakRef;
        };

        void LoadUnloadTests::ReproWeakRefCrash()
        {
            {
                xaml_controls::StackPanel^ rootPanel;
                xaml_controls::ItemContainer^ itemContainer;

                DependencyProperty^ myCustomAttachedProperty;

                RunOnUIThread([&]()
                {
                    myCustomAttachedProperty =
                        DependencyProperty::RegisterAttached(
                        L"MyCustomAttachedProperty",
                        Object::typeid,
                        Object::typeid,
                        nullptr);

                    itemContainer = ref new xaml_controls::ItemContainer();

                    itemContainer->SetValue(myCustomAttachedProperty, ref new MyPropValue(itemContainer));

                    rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                        L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                        L"</StackPanel>"));

                    rootPanel->Children->Append(itemContainer);

                    TestServices::WindowHelper->WindowContent = rootPanel;


                });
                TestServices::WindowHelper->WaitForIdle();

                LOG_OUTPUT(L"Test body complete.  The validation is that we don't crash during shutdown.");
            }

            // We do ShutdownXaml directly here so that if we crash during shutdown the test will be counted as a fail.
            LOG_OUTPUT(L"Shutting down xaml...");
            TestServices::WindowHelper->ShutdownXaml();
            LOG_OUTPUT(L"Shutdown complete.");
        }

} } } } } }
