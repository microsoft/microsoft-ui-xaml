// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "FrameIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <FirstTestPage.h>
#include <SecondTestPage.h>
#include <ThirdTestPage.h>
#include "CustomTypes.XamlTypeInfo.g.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace ::Tests::Native::External::Controls::Frame;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Frame {

    bool FrameIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool FrameIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider());
        return true;
    }

    bool FrameIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void FrameIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::Frame>::CanInstantiate();
    }

    void FrameIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::Frame>::CanEnterAndLeaveLiveTree();
    }

    void FrameIntegrationTests::CanRaiseNavigationEvents()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Frame^ frame = nullptr;
        auto frameNavigatingEventRegistration = CreateSafeEventRegistration(xaml_controls::Frame, Navigating);
        auto frameNavigatedEventRegistration = CreateSafeEventRegistration(xaml_controls::Frame, Navigated);
        auto frameNavigatingEvent = std::make_shared<Event>();
        auto frameNavigatedEvent = std::make_shared<Event>();

        wxaml_interop::TypeName pageType = { L"Microsoft.UI.Xaml.Controls.Page", wxaml_interop::TypeKind::Primitive };

        RunOnUIThread([&]()
        {
            frame = ref new xaml_controls::Frame();
            frameNavigatingEventRegistration.Attach(frame, ref new xaml_navigation::NavigatingCancelEventHandler([&](Platform::Object^, xaml_navigation::NavigatingCancelEventArgs^)
            {
                frameNavigatingEvent->Set();
            }));
            frameNavigatedEventRegistration.Attach(frame, ref new xaml_navigation::NavigatedEventHandler([&](Platform::Object^, xaml_navigation::NavigationEventArgs^)
            {
                frameNavigatedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = frame;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            frame->Navigate(pageType);
        });

        frameNavigatingEvent->WaitForDefault();
        frameNavigatedEvent->WaitForDefault();
    }

    void FrameIntegrationTests::CanNavigateBetweenPages()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Frame^ frame = nullptr;
        wxaml_interop::TypeName pageType = { L"Microsoft.UI.Xaml.Controls.Page", wxaml_interop::TypeKind::Primitive };

        RunOnUIThread([&]()
        {
            frame = ref new xaml_controls::Frame();
            TestServices::WindowHelper->WindowContent = frame;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            frame->Navigate(pageType, "Page 1");
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 0, 0);
            frame->Navigate(pageType, "Page 2");
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 1, 0);
            frame->Navigate(pageType, "Page 3");
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 2, 0);
            frame->GoBack();
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 1, 1);
            frame->GoBack();
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 0, 2);
            frame->GoForward();
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 1, 1);
            frame->GoForward();
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 2, 0);
        });
    }


    void FrameIntegrationTests::InvalidNavigationDestinationHasHelpfulError()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            wxaml_interop::TypeName destinationTypes[] =
            {
                { L"Microsoft.UI.Xaml.Controls.UserControl", wxaml_interop::TypeKind::Primitive },
                { L"Microsoft.UI.Xaml.Window", wxaml_interop::TypeKind::Primitive },
                { L"Microsoft.UI.Xaml.Controls.Grid", wxaml_interop::TypeKind::Primitive }
            };

            for (auto destinationType : destinationTypes)
            {
                for (int overload = 0; overload < 6; ++overload)
                {
                    LOG_OUTPUT(L"Invalid destination: %s, overload: %d", destinationType.Name->Data(), overload);
                    auto frame = ref new xaml_controls::Frame();
                    TestServices::WindowHelper->WindowContent = frame;
                    auto transitionInfo = ref new Microsoft::UI::Xaml::Media::Animation::SuppressNavigationTransitionInfo();
                    auto options = ref new xaml_navigation::FrameNavigationOptions();
                    options->IsNavigationStackEnabled = overload != 5;
                    options->TransitionInfoOverride = transitionInfo;

                    int navigationFailedCount = 0;
                    int navigatedCount = 0;
                    auto failedRegistration = CreateSafeEventRegistration(xaml_controls::Frame, NavigationFailed);
                    auto navigatedRegistration = CreateSafeEventRegistration(xaml_controls::Frame, Navigated);
                    failedRegistration.Attach(frame, ref new xaml_navigation::NavigationFailedEventHandler(
                        [&](Platform::Object^, xaml_navigation::NavigationFailedEventArgs^ args)
                        {
                            ++navigationFailedCount;
                            args->Handled = true;
                        }));
                    navigatedRegistration.Attach(frame, ref new xaml_navigation::NavigatedEventHandler(
                        [&](Platform::Object^, xaml_navigation::NavigationEventArgs^)
                        {
                            ++navigatedCount;
                        }));

                    auto navigate = [&]()
                    {
                        switch (overload)
                        {
                        case 0:
                            frame->Navigate(destinationType);
                            break;
                        case 1:
                            frame->Navigate(destinationType, L"Parameter");
                            break;
                        case 2:
                            frame->Navigate(destinationType, L"Parameter", transitionInfo);
                            break;
                        case 3:
                            frame->NavigateToType(destinationType, L"Parameter", nullptr);
                            break;
                        default:
                            frame->NavigateToType(destinationType, L"Parameter", options);
                            break;
                        }
                    };

                    auto verifyError = [destinationType](Platform::InvalidArgumentException^ exception)
                    {
                        VERIFY_ARE_EQUAL(E_INVALIDARG, exception->HResult);
                        auto message = exception->Message;
                        LOG_OUTPUT(L"Navigation error message: \"%s\"", message ? message->Data() : L"(null)");
                        return message != nullptr &&
                            wcsstr(message->Data(), destinationType.Name->Data()) != nullptr &&
                            wcsstr(message->Data(), L"The destination type must derive from Microsoft.UI.Xaml.Controls.Page.") != nullptr &&
                            wcsstr(message->Data(), L"https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.frame.navigate") != nullptr;
                    };
                    VERIFY_THROWS_SPECIFIC_WINRT(navigate(), Platform::InvalidArgumentException^, verifyError);
                    VERIFY_IS_NULL(frame->Content);
                    ValidateFrameStack(frame, 0, 0);
                    VERIFY_IS_FALSE(frame->CanGoBack);
                    VERIFY_IS_FALSE(frame->CanGoForward);
                    VERIFY_ARE_EQUAL(0, navigationFailedCount);
                    VERIFY_ARE_EQUAL(0, navigatedCount);
                }
            }
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void FrameIntegrationTests::InvalidNavigationDestinationPreservesFrameState()
    {
        TestCleanupWrapper cleanup;
        wxaml_interop::TypeName pageType = { L"Microsoft.UI.Xaml.Controls.Page", wxaml_interop::TypeKind::Primitive };
        wxaml_interop::TypeName invalidType = { L"Microsoft.UI.Xaml.Controls.UserControl", wxaml_interop::TypeKind::Primitive };

        for (int historyMode = 0; historyMode < 3; ++historyMode)
        {
            xaml_controls::Frame^ frame = nullptr;
            RunOnUIThread([&]()
            {
                frame = ref new xaml_controls::Frame();
                TestServices::WindowHelper->WindowContent = frame;
            });
            TestServices::WindowHelper->WaitForIdle();

            auto navigateToPage = [&](Platform::String^ parameter)
            {
                RunOnUIThread([&]()
                {
                    VERIFY_IS_TRUE(frame->Navigate(pageType, parameter));
                    VERIFY_ARE_EQUAL(frame, safe_cast<xaml_controls::Page^>(frame->Content)->Frame);
                });
                TestServices::WindowHelper->WaitForIdle();
            };

            navigateToPage(L"Page 1");
            navigateToPage(L"Page 2");
            navigateToPage(L"Page 3");
            RunOnUIThread([&]()
            {
                frame->GoBack();
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                ValidateFrameStack(frame, 1, 1);
                auto content = frame->Content;
                auto sourcePageType = frame->SourcePageType;
                auto currentSourcePageType = frame->CurrentSourcePageType;
                auto backEntry = frame->BackStack->GetAt(0);
                auto forwardEntry = frame->ForwardStack->GetAt(0);
                auto canGoBack = frame->CanGoBack;
                auto canGoForward = frame->CanGoForward;

                if (historyMode == 1)
                {
                    frame->IsNavigationStackEnabled = false;
                }

                auto options = ref new xaml_navigation::FrameNavigationOptions();
                options->IsNavigationStackEnabled = false;
                auto navigate = [&]()
                {
                    if (historyMode == 2)
                    {
                        frame->NavigateToType(invalidType, nullptr, options);
                    }
                    else
                    {
                        frame->Navigate(invalidType);
                    }
                };
                VERIFY_THROWS_SPECIFIC_WINRT(navigate(), Platform::InvalidArgumentException^,
                    [](Platform::InvalidArgumentException^ exception) { return exception->HResult == E_INVALIDARG; });

                VERIFY_ARE_EQUAL(content, frame->Content);
                VERIFY_ARE_EQUAL(frame, safe_cast<xaml_controls::Page^>(content)->Frame);
                VERIFY_ARE_EQUAL(sourcePageType.Name, frame->SourcePageType.Name);
                VERIFY_ARE_EQUAL(sourcePageType.Kind, frame->SourcePageType.Kind);
                VERIFY_ARE_EQUAL(currentSourcePageType.Name, frame->CurrentSourcePageType.Name);
                VERIFY_ARE_EQUAL(currentSourcePageType.Kind, frame->CurrentSourcePageType.Kind);
                ValidateFrameStack(frame, 1, 1);
                VERIFY_ARE_EQUAL(backEntry, frame->BackStack->GetAt(0));
                VERIFY_ARE_EQUAL(forwardEntry, frame->ForwardStack->GetAt(0));
                VERIFY_ARE_EQUAL(canGoBack, frame->CanGoBack);
                VERIFY_ARE_EQUAL(canGoForward, frame->CanGoForward);
                VERIFY_ARE_EQUAL(historyMode != 1, frame->IsNavigationStackEnabled);
                frame->IsNavigationStackEnabled = true;
            });

            navigateToPage(L"Page 4");
            RunOnUIThread([&]()
            {
                ValidateFrameStack(frame, 2, 0);
                frame->GoBack();
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                ValidateFrameStack(frame, 1, 1);
                VERIFY_ARE_EQUAL(frame, safe_cast<xaml_controls::Page^>(frame->Content)->Frame);
                frame->GoForward();
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                ValidateFrameStack(frame, 2, 0);
                VERIFY_ARE_EQUAL(frame, safe_cast<xaml_controls::Page^>(frame->Content)->Frame);
            });
        }
    }

    void FrameIntegrationTests::CanceledInvalidNavigationDoesNotThrow()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Frame^ frame = nullptr;
        wxaml_interop::TypeName pageType = { L"Microsoft.UI.Xaml.Controls.Page", wxaml_interop::TypeKind::Primitive };
        wxaml_interop::TypeName invalidType = { L"Microsoft.UI.Xaml.Controls.UserControl", wxaml_interop::TypeKind::Primitive };
        int stoppedCount = 0;
        int navigationFailedCount = 0;
        int navigatedCount = 0;
        auto navigatingRegistration = CreateSafeEventRegistration(xaml_controls::Frame, Navigating);
        auto stoppedRegistration = CreateSafeEventRegistration(xaml_controls::Frame, NavigationStopped);
        auto failedRegistration = CreateSafeEventRegistration(xaml_controls::Frame, NavigationFailed);
        auto navigatedRegistration = CreateSafeEventRegistration(xaml_controls::Frame, Navigated);

        RunOnUIThread([&]()
        {
            frame = ref new xaml_controls::Frame();
            TestServices::WindowHelper->WindowContent = frame;
            VERIFY_IS_TRUE(frame->Navigate(pageType));
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto content = frame->Content;
            navigatingRegistration.Attach(frame, ref new xaml_navigation::NavigatingCancelEventHandler(
                [&](Platform::Object^, xaml_navigation::NavigatingCancelEventArgs^ args)
                {
                    args->Cancel = true;
                }));
            stoppedRegistration.Attach(frame, ref new xaml_navigation::NavigationStoppedEventHandler(
                [&](Platform::Object^, xaml_navigation::NavigationEventArgs^)
                {
                    ++stoppedCount;
                }));
            failedRegistration.Attach(frame, ref new xaml_navigation::NavigationFailedEventHandler(
                [&](Platform::Object^, xaml_navigation::NavigationFailedEventArgs^)
                {
                    ++navigationFailedCount;
                }));
            navigatedRegistration.Attach(frame, ref new xaml_navigation::NavigatedEventHandler(
                [&](Platform::Object^, xaml_navigation::NavigationEventArgs^)
                {
                    ++navigatedCount;
                }));

            frame->Navigate(invalidType);
            VERIFY_ARE_EQUAL(content, frame->Content);
            VERIFY_ARE_EQUAL(frame, safe_cast<xaml_controls::Page^>(content)->Frame);
            ValidateFrameStack(frame, 0, 0);
            VERIFY_ARE_EQUAL(1, stoppedCount);
            VERIFY_ARE_EQUAL(0, navigationFailedCount);
            VERIFY_ARE_EQUAL(0, navigatedCount);
            navigatingRegistration.Detach();
            VERIFY_IS_TRUE(frame->Navigate(pageType));
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 1, 0);
            VERIFY_ARE_EQUAL(frame, safe_cast<xaml_controls::Page^>(frame->Content)->Frame);
            VERIFY_ARE_EQUAL(1, stoppedCount);
            VERIFY_ARE_EQUAL(0, navigationFailedCount);
            VERIFY_ARE_EQUAL(1, navigatedCount);
        });
    }

    void FrameIntegrationTests::CanDisableNavigationHistoryUsingNavigationMethod()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Frame^ frame = nullptr;
        wxaml_interop::TypeName pageType = { L"Microsoft.UI.Xaml.Controls.Page", wxaml_interop::TypeKind::Primitive };
        Microsoft::UI::Xaml::Navigation::FrameNavigationOptions^ navOptions = nullptr;

        RunOnUIThread([&]()
        {
            frame = ref new xaml_controls::Frame();
            TestServices::WindowHelper->WindowContent = frame;
            navOptions = ref new Microsoft::UI::Xaml::Navigation::FrameNavigationOptions();
            navOptions->IsNavigationStackEnabled = false;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            frame->NavigateToType(pageType, "Page 1", navOptions);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 0, 0);
            frame->NavigateToType(pageType, "Page 2", navOptions);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 0, 0);
            navOptions->IsNavigationStackEnabled = true;
            frame->NavigateToType(pageType, "Page 3", navOptions); // This becomes frame 0
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 0, 0);
            frame->NavigateToType(pageType, "Page 2", navOptions); // now it has a back
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 1, 0);
            frame->NavigateToType(pageType, "Page 1", navOptions);

        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 2, 0);
            frame->GoBack();
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 1, 1);
        });
    }

    void FrameIntegrationTests::CanDisableNavigationHistoryFromFrame()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Frame^ frame = nullptr;
        wxaml_interop::TypeName pageType = { L"Microsoft.UI.Xaml.Controls.Page", wxaml_interop::TypeKind::Primitive };

        RunOnUIThread([&]()
        {
            frame = ref new xaml_controls::Frame();
            frame->IsNavigationStackEnabled = false;
            TestServices::WindowHelper->WindowContent = frame;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            frame->Navigate(pageType, "Page 1");
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 0, 0);
            frame->Navigate(pageType, "Page 2");
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 0, 0);
            frame->IsNavigationStackEnabled = true;
            frame->Navigate(pageType, "Page 3"); // This becomes frame 0
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 0, 0);
            frame->Navigate(pageType, "Page 1"); // Now it has a back
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 1, 0);
            frame->GoBack();
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 0, 1);
        });
    }
    void FrameIntegrationTests::ValidateFrameStack(xaml_controls::Frame^ frame, int expectedBackStackDepth, unsigned int expectedForwardStackDepth)
    {
        VERIFY_ARE_EQUAL(frame->BackStackDepth, expectedBackStackDepth);
        VERIFY_ARE_EQUAL(frame->ForwardStack->Size, expectedForwardStackDepth);
    }

    void FrameIntegrationTests::CanNavigateWithNavigationTransitionInfo()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Frame^ frame = nullptr;

        Microsoft::UI::Xaml::Navigation::PageStackEntry^ backStackEntry = nullptr;
        Microsoft::UI::Xaml::Navigation::PageStackEntry^ forwardStackEntry = nullptr;
        Microsoft::UI::Xaml::Media::Animation::SlideNavigationTransitionInfo^ slideNTI = nullptr;
        Microsoft::UI::Xaml::Media::Animation::CommonNavigationTransitionInfo^ commonNTI = nullptr;

        wxaml_interop::TypeName pageType = { L"Microsoft.UI.Xaml.Controls.Page", wxaml_interop::TypeKind::Primitive };

        RunOnUIThread([&]()
        {
            slideNTI = ref new Microsoft::UI::Xaml::Media::Animation::SlideNavigationTransitionInfo();
            commonNTI = ref new Microsoft::UI::Xaml::Media::Animation::CommonNavigationTransitionInfo();

            frame = ref new xaml_controls::Frame();
            TestServices::WindowHelper->WindowContent = frame;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            frame->Navigate(pageType, "Page 1", slideNTI);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 0, 0);
            frame->Navigate(pageType, "Page 2", slideNTI);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 1, 0);
            frame->Navigate(pageType, "Page 3", slideNTI);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"CanNavigateWithNavigationTransitionInfo: BackStack size=%d", frame->BackStack->Size);
            VERIFY_IS_TRUE(frame->BackStack->Size != 0);
            backStackEntry = frame->BackStack->GetAt(frame->BackStack->Size-1);
            VERIFY_IS_TRUE(backStackEntry->NavigationTransitionInfo == slideNTI);

            // Go back to the previous page with CommonNavigationTransitionInfo
            frame->GoBack(commonNTI);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"CanNavigateWithNavigationTransitionInfo: ForwardStack size=%d", frame->ForwardStack->Size);
            VERIFY_IS_TRUE(frame->ForwardStack->Size != 0);
            forwardStackEntry = frame->ForwardStack->GetAt(frame->ForwardStack->Size - 1);
            VERIFY_IS_TRUE(forwardStackEntry->NavigationTransitionInfo == commonNTI);
        });
    }

    void FrameIntegrationTests::ValidateReEntrancyPrevention()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Frame^ frame = nullptr;
        auto frameNavigatedEventRegistration = CreateSafeEventRegistration(xaml_controls::Frame, Navigated);
        auto frameNavigatedEvent = std::make_shared<Event>();

        wxaml_interop::TypeName pageType = { L"Microsoft.UI.Xaml.Controls.Page", wxaml_interop::TypeKind::Primitive };

        RunOnUIThread([&]()
        {
            frame = ref new xaml_controls::Frame();
            frameNavigatedEventRegistration.Attach(frame, ref new xaml_navigation::NavigatedEventHandler([&](Platform::Object^, xaml_navigation::NavigationEventArgs^)
            {
                frameNavigatedEvent->Set();
                // This Navigate will be silently suppressed because of the re-entrancy check.
                frame->Navigate(pageType, "Page 2");
            }));

            TestServices::WindowHelper->WindowContent = frame;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            frame->Navigate(pageType, "Page 1");
        });

        TestServices::WindowHelper->WaitForIdle();
        frameNavigatedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 0, 0);
            frameNavigatedEventRegistration.Detach();
        });

        frameNavigatedEvent->Reset();

        RunOnUIThread([&]()
        {
            frameNavigatedEventRegistration.Attach(frame, ref new xaml_navigation::NavigatedEventHandler([&](Platform::Object^, xaml_navigation::NavigationEventArgs^)
            {
                frameNavigatedEvent->Set();
                // This navigation will be silently suppressed because of the re-entrancy check.
                frame->GoBack();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            frame->Navigate(pageType, "Page 2");
        });

        TestServices::WindowHelper->WaitForIdle();
        frameNavigatedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 1, 0);
            frameNavigatedEventRegistration.Detach();
        });

        frameNavigatedEvent->Reset();

        RunOnUIThread([&]()
        {
            frameNavigatedEventRegistration.Attach(frame, ref new xaml_navigation::NavigatedEventHandler([&](Platform::Object^, xaml_navigation::NavigationEventArgs^)
            {
                frameNavigatedEvent->Set();
                // This navigation will be silently suppressed because of the re-entrancy check.
                frame->GoForward();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            frame->GoBack();
        });

        TestServices::WindowHelper->WaitForIdle();
        frameNavigatedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 0, 1);
        });
    }

    void FrameIntegrationTests::CanGetNavigationStateWithCurrentPageNull()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Frame^ frame = nullptr;

        Platform::String^ navigation = L"1,3,2,31,Microsoft.UI.Xaml.Controls.Page,12,6,Page 1,0,31,Microsoft.UI.Xaml.Controls.Page,12,6,Page 2,0,31,Microsoft.UI.Xaml.Controls.Page,12,6,Page 3,0";

        RunOnUIThread([&]()
        {
            frame = ref new xaml_controls::Frame();

            TestServices::WindowHelper->WindowContent = frame;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            frame->SetNavigationState(navigation, true);

            auto navigationHistory = frame->GetNavigationState();

            VERIFY_ARE_EQUAL(navigation, navigationHistory);
        });
    }

    void FrameIntegrationTests::CanSetNavigationStateWithoutNavigatingToCurrent()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Frame^ frame = nullptr;

        Platform::String^ navigation1 = L"1,3,2,31,Microsoft.UI.Xaml.Controls.Page,12,6,Page 1,0,31,Microsoft.UI.Xaml.Controls.Page,12,6,Page 2,0,31,Microsoft.UI.Xaml.Controls.Page,12,6,Page 3,0";
        Platform::String^ navigation2 = L"1,3,1,31,Microsoft.UI.Xaml.Controls.Page,12,6,Page 1,0,31,Microsoft.UI.Xaml.Controls.Page,12,6,Page 2,0,31,Microsoft.UI.Xaml.Controls.Page,12,6,Page 3,0";

        auto frameNavigatingEventRegistration = CreateSafeEventRegistration(xaml_controls::Frame, Navigating);
        auto frameNavigatedEventRegistration = CreateSafeEventRegistration(xaml_controls::Frame, Navigated);
        auto frameNavigatingEvent = std::make_shared<Event>();
        auto frameNavigatedEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            frame = ref new xaml_controls::Frame();

            frameNavigatingEventRegistration.Attach(frame, ref new xaml_navigation::NavigatingCancelEventHandler([&](Platform::Object^, xaml_navigation::NavigatingCancelEventArgs^)
            {
                frameNavigatingEvent->Set();
            }));
            frameNavigatedEventRegistration.Attach(frame, ref new xaml_navigation::NavigatedEventHandler([&](Platform::Object^, xaml_navigation::NavigationEventArgs^)
            {
                frameNavigatedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = frame;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            frame->SetNavigationState(navigation1, true);
        });

        // Validate SetNavigationState doesn't trigger navigation

        frameNavigatingEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
        frameNavigatedEvent->WaitForNoThrow(std::chrono::milliseconds(1000));

        ValidateGoBackBehaviorWhenCurrentIsNull(frame, navigation2);
        ValidateGoForwardBehaviorWhenCurrentIsNull(frame, navigation2);
        ValidateNavigateBehaviorWhenCurrentIsNull(frame, navigation2);
    }

    void FrameIntegrationTests::ValidateGoBackBehaviorWhenCurrentIsNull(xaml_controls::Frame^ frame, Platform::String^ navigationHistory)
    {
        // Validate GoBack doesn't add items to the forward stack when current page is NULL

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 3, 0);
            frame->GoBack();
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 2, 0);
            frame->GoBack();
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 1, 1);
            frame->SetNavigationState(navigationHistory, true);
        });

        TestServices::WindowHelper->WaitForIdle();

        // Validate GoBack doesn't add items to the forward stack when current page is NULL and forward stack is not empty

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 2, 1);
            frame->GoBack();
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 1, 1);
        });
    }

    void FrameIntegrationTests::ValidateGoForwardBehaviorWhenCurrentIsNull(xaml_controls::Frame^ frame, Platform::String^ navigationHistory)
    {
        // Validate GoForward doesn't add items to the back stack when current page is NULL

        RunOnUIThread([&]()
        {
            frame->SetNavigationState(navigationHistory, true);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 2, 1);
            frame->GoForward();
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 2, 0);
        });
    }

    void FrameIntegrationTests::ValidateNavigateBehaviorWhenCurrentIsNull(xaml_controls::Frame^ frame, Platform::String^ navigationHistory)
    {
        // Validate Navigate works when current page is NULL and that forward stack is cleared after navigation.

        wxaml_interop::TypeName pageType = { L"Microsoft.UI.Xaml.Controls.Page", wxaml_interop::TypeKind::Primitive };

        RunOnUIThread([&]()
        {
            frame->SetNavigationState(navigationHistory, true);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 2, 1);
            frame->Navigate(pageType);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateFrameStack(frame, 2, 0);
        });
    }

    void FrameIntegrationTests::CacheModeDisabled()
    {
        VerifyCachePageNavigationHelper(xaml_navigation::NavigationCacheMode::Disabled, new int[7]{ 1, 10, 2, 11, 20, 12, 3 });
    }

    void FrameIntegrationTests::CacheModeEnabled()
    {
        VerifyCachePageNavigationHelper(xaml_navigation::NavigationCacheMode::Enabled, new int[7]{ 1, 10, 1, 10, 20, 10, 3 });
    }

    void FrameIntegrationTests::CacheModeRequired()
    {
        VerifyCachePageNavigationHelper(xaml_navigation::NavigationCacheMode::Required, new int[7]{ 1, 10, 1, 10, 20, 10, 1 });
    }

    void FrameIntegrationTests::CanceledNavigation()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Frame^ frame = nullptr;
        auto frameNavigatingEventRegistration = CreateSafeEventRegistration(xaml_controls::Frame, Navigating);
        auto frameStoppedEventRegistration = CreateSafeEventRegistration(xaml_controls::Frame, NavigationStopped);
        auto frameNavigatingEvent = std::make_shared<Event>();
        auto frameStoppedEvent = std::make_shared<Event>();

        wxaml_interop::TypeName pageType = { L"Microsoft.UI.Xaml.Controls.Page", wxaml_interop::TypeKind::Primitive };

        RunOnUIThread([&]()
        {
            frame = ref new xaml_controls::Frame();
            frameNavigatingEventRegistration.Attach(frame, ref new xaml_navigation::NavigatingCancelEventHandler([&](Platform::Object^, xaml_navigation::NavigatingCancelEventArgs^ args)
            {
                args->Cancel = true;
                frameNavigatingEvent->Set();
            }));
            frameStoppedEventRegistration.Attach(frame, ref new xaml_navigation::NavigationStoppedEventHandler([&](Platform::Object^, xaml_navigation::NavigationEventArgs^)
            {
                frameStoppedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = frame;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            frame->Navigate(pageType);
        });

        frameNavigatingEvent->WaitForDefault();
        frameStoppedEvent->WaitForDefault();
    }

    void FrameIntegrationTests::VerifyCachePageNavigationHelper(xaml_navigation::NavigationCacheMode cacheMode, int expectedValues[])
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Frame^ frame = nullptr;

        RunOnUIThread([&]()
        {
            frame = ref new xaml_controls::Frame();
            frame->CacheSize = 2;
            TestServices::WindowHelper->WindowContent = frame;
        });

        TestServices::WindowHelper->WaitForIdle();

        // When NavigationCacheMode is Disabled all InstanceCounter will pick the latest Counter value
        // When NavigationCacheMode is Required all InstanceCounter will always use the initial Counter value

        RunOnUIThread([&]()
        {
            FirstTestPage::Counter = 1;
            SecondTestPage::Counter = 10;
            ThirdTestPage::Counter = 20;
            FirstTestPage::CacheMode = cacheMode;
            SecondTestPage::CacheMode = cacheMode;
            ThirdTestPage::CacheMode = cacheMode;
            frame->Navigate(FirstTestPage::typeid, "Page 1");
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto page = safe_cast<FirstTestPage^>(frame->Content);
            VERIFY_ARE_EQUAL(expectedValues[0], page->InstanceCounter);

            FirstTestPage::Counter = 2;
            frame->Navigate(SecondTestPage::typeid, "Page 2");
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto page = safe_cast<SecondTestPage^>(frame->Content);
            VERIFY_ARE_EQUAL(expectedValues[1], page->InstanceCounter);

            frame->GoBack();
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto page = safe_cast<FirstTestPage^>(frame->Content);

            // When NavigationCacheMode is Enabled InstanceCounter will still use the initial Counter of FirstTestPage value as it is within the CacheSize limit
            VERIFY_ARE_EQUAL(expectedValues[2], page->InstanceCounter);

            SecondTestPage::Counter = 11;
            frame->GoForward();
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto page = safe_cast<SecondTestPage^>(frame->Content);

            // When NavigationCacheMode is Enabled InstanceCounter will still use the initial Counter value of SecondTestPage as it is within the CacheSize limit
            VERIFY_ARE_EQUAL(expectedValues[3], page->InstanceCounter);

            frame->Navigate(ThirdTestPage::typeid, "Page 3");
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto page = safe_cast<ThirdTestPage^>(frame->Content);

            // When NavigationCacheMode is Enabled InstanceCounter will still use the initial Counter value of ThirdTestPage as it is within the CacheSize limit
            VERIFY_ARE_EQUAL(expectedValues[4], page->InstanceCounter);

            SecondTestPage::Counter = 12;
            frame->GoBack();
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto page = safe_cast<SecondTestPage^>(frame->Content);

            // When NavigationCacheMode is Enabled InstanceCounter will still use the initial Counter value of SecondTestPage as it is within the CacheSize limit
            VERIFY_ARE_EQUAL(expectedValues[5], page->InstanceCounter);

            FirstTestPage::Counter = 3;
            frame->GoBack();
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto page = safe_cast<FirstTestPage^>(frame->Content);

            // When NavigationCacheMode is Enabled this page will be regenerated due to CacheSize and InstanceCounter will use the latest Counter value of FirstTestPage
            VERIFY_ARE_EQUAL(expectedValues[6], page->InstanceCounter);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::Frame
