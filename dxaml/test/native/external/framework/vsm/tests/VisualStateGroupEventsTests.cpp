// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <StringUtilities.h>
#include <memory>
#include <string>
#include <vector>
#include "VisualStateGroupEventsTests.h"

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {

    namespace
    {
        // Button whose template root carries one VisualStateGroup with two empty states.
        const wchar_t c_templatedButtonMarkup[] =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='Button'>"
            L"      <Grid x:Name='TemplateRoot'>"
            L"        <VisualStateManager.VisualStateGroups>"
            L"          <VisualStateGroup x:Name='TestGroup'>"
            L"            <VisualState x:Name='StateA' />"
            L"            <VisualState x:Name='StateB' />"
            L"          </VisualStateGroup>"
            L"        </VisualStateManager.VisualStateGroups>"
            L"      </Grid>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

        enum class StateEventKind { Changing, Changed };

        struct StateEventRecord
        {
            StateEventKind kind;
            Object^ sender;
            VisualState^ oldState;
            VisualState^ newState;
            Control^ control;
        };

        typedef std::vector<StateEventRecord> StateEventLog;

        struct TemplatedButtonParts
        {
            Button^ button;
            FrameworkElement^ templateRoot;
            VisualStateGroup^ group;
            VisualState^ stateA;
            VisualState^ stateB;
        };

        TemplatedButtonParts LoadTemplatedButton()
        {
            TemplatedButtonParts parts;
            RunOnUIThread([&]()
            {
                parts.button = safe_cast<Button^>(XamlReader::Load(ref new String(c_templatedButtonMarkup)));
                TestServices::WindowHelper->WindowContent = parts.button;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                parts.templateRoot = safe_cast<FrameworkElement^>(VisualTreeHelper::GetChild(parts.button, 0));
                VERIFY_ARE_STRINGS_EQUAL(L"TemplateRoot", parts.templateRoot->Name->Data());

                auto groups = VisualStateManager::GetVisualStateGroups(parts.templateRoot);
                VERIFY_ARE_EQUAL(1u, groups->Size);
                parts.group = groups->GetAt(0);
                VERIFY_ARE_EQUAL(2u, parts.group->States->Size);
                parts.stateA = parts.group->States->GetAt(0);
                parts.stateB = parts.group->States->GetAt(1);
                VERIFY_ARE_STRINGS_EQUAL(L"StateA", parts.stateA->Name->Data());
                VERIFY_ARE_STRINGS_EQUAL(L"StateB", parts.stateB->Name->Data());
            });
            return parts;
        }

        VisualStateChangedEventHandler^ MakeRecorder(
            StateEventKind kind,
            const std::shared_ptr<StateEventLog>& log,
            const std::shared_ptr<Event>& raised)
        {
            return ref new VisualStateChangedEventHandler([kind, log, raised](Object^ sender, VisualStateChangedEventArgs^ args)
            {
                log->push_back({ kind, sender, args->OldState, args->NewState, args->Control });
                if (raised)
                {
                    raised->Set();
                }
            });
        }

        void VerifyRecord(
            const StateEventRecord& record,
            StateEventKind expectedKind,
            FrameworkElement^ expectedSender,
            VisualState^ expectedOldState,
            VisualState^ expectedNewState,
            Control^ expectedControl)
        {
            VERIFY_IS_TRUE(record.kind == expectedKind, L"Unexpected event kind or order.");
            VERIFY_IS_TRUE(dynamic_cast<FrameworkElement^>(record.sender) == expectedSender, L"Sender must be the control's template root.");
            VERIFY_IS_TRUE(record.oldState == expectedOldState, L"Unexpected OldState.");
            VERIFY_IS_TRUE(record.newState == expectedNewState, L"Unexpected NewState.");
            VERIFY_IS_TRUE(record.control == expectedControl, L"Unexpected Control.");
        }
    }

    // Exposes the protected VisualStateManager raise methods.
    ref class EventRaisingVisualStateManager sealed : public ::Microsoft::UI::Xaml::VisualStateManager
    {
    public:
        void InvokeRaiseCurrentStateChanging(VisualStateGroup^ group, VisualState^ oldState, VisualState^ newState, Control^ control)
        {
            RaiseCurrentStateChanging(group, oldState, newState, control);
        }

        void InvokeRaiseCurrentStateChanged(VisualStateGroup^ group, VisualState^ oldState, VisualState^ newState, Control^ control)
        {
            RaiseCurrentStateChanged(group, oldState, newState, control);
        }
    };

    bool VisualStateGroupEventsTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool VisualStateGroupEventsTests::TestSetup()
    {
        TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool VisualStateGroupEventsTests::TestCleanup()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void VisualStateGroupEventsTests::CurrentStateChangingRaisedBeforeChangedOnGoToState()
    {
        TestCleanupWrapper cleanup;
        auto parts = LoadTemplatedButton();
        auto log = std::make_shared<StateEventLog>();
        auto changedEvent = std::make_shared<Event>();
        ::Windows::Foundation::EventRegistrationToken changingToken = {};
        ::Windows::Foundation::EventRegistrationToken changedToken = {};

        RunOnUIThread([&]()
        {
            changingToken = parts.group->CurrentStateChanging += MakeRecorder(StateEventKind::Changing, log, nullptr);
            changedToken = parts.group->CurrentStateChanged += MakeRecorder(StateEventKind::Changed, log, changedEvent);
            VERIFY_IS_TRUE(VisualStateManager::GoToState(parts.button, L"StateA", false));
        });
        changedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(parts.group->CurrentState == parts.stateA);
            log->clear();
            changedEvent->Reset();
            VERIFY_IS_TRUE(VisualStateManager::GoToState(parts.button, L"StateB", false));
        });
        changedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(2u, static_cast<unsigned int>(log->size()));
            VerifyRecord((*log)[0], StateEventKind::Changing, parts.templateRoot, parts.stateA, parts.stateB, parts.button);
            VerifyRecord((*log)[1], StateEventKind::Changed, parts.templateRoot, parts.stateA, parts.stateB, parts.button);
            VERIFY_IS_TRUE(parts.group->CurrentState == parts.stateB);

            parts.group->CurrentStateChanging -= changingToken;
            parts.group->CurrentStateChanged -= changedToken;
        });
    }

    void VisualStateGroupEventsTests::CurrentStateChangingHandlerNotInvokedAfterRemoval()
    {
        TestCleanupWrapper cleanup;
        auto parts = LoadTemplatedButton();
        auto log = std::make_shared<StateEventLog>();
        auto changedEvent = std::make_shared<Event>();
        ::Windows::Foundation::EventRegistrationToken changingToken = {};
        ::Windows::Foundation::EventRegistrationToken changedToken = {};

        RunOnUIThread([&]()
        {
            changingToken = parts.group->CurrentStateChanging += MakeRecorder(StateEventKind::Changing, log, nullptr);
            changedToken = parts.group->CurrentStateChanged += MakeRecorder(StateEventKind::Changed, log, changedEvent);
            VERIFY_IS_TRUE(VisualStateManager::GoToState(parts.button, L"StateA", false));
        });
        changedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Precondition: the Changing handler was live before removal.
            VERIFY_ARE_EQUAL(2u, static_cast<unsigned int>(log->size()));
            VERIFY_IS_TRUE((*log)[0].kind == StateEventKind::Changing);

            parts.group->CurrentStateChanging -= changingToken;
            log->clear();
            changedEvent->Reset();
            VERIFY_IS_TRUE(VisualStateManager::GoToState(parts.button, L"StateB", false));
        });
        changedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1u, static_cast<unsigned int>(log->size()), L"Only the CurrentStateChanged handler may run.");
            VerifyRecord((*log)[0], StateEventKind::Changed, parts.templateRoot, parts.stateA, parts.stateB, parts.button);

            parts.group->CurrentStateChanged -= changedToken;
        });
    }

    void VisualStateGroupEventsTests::CustomVsmRaiseCurrentStateChangingInvokesGroupHandlers()
    {
        TestCleanupWrapper cleanup;
        auto parts = LoadTemplatedButton();
        auto log = std::make_shared<StateEventLog>();

        RunOnUIThread([&]()
        {
            auto changingToken = parts.group->CurrentStateChanging += MakeRecorder(StateEventKind::Changing, log, nullptr);
            auto changedToken = parts.group->CurrentStateChanged += MakeRecorder(StateEventKind::Changed, log, nullptr);
            auto vsm = ref new EventRaisingVisualStateManager();
            auto stateBeforeRaise = parts.group->CurrentState;

            vsm->InvokeRaiseCurrentStateChanging(parts.group, parts.stateA, parts.stateB, parts.button);

            // Raised synchronously, only the Changing event, exactly once.
            VERIFY_ARE_EQUAL(1u, static_cast<unsigned int>(log->size()));
            VerifyRecord((*log)[0], StateEventKind::Changing, parts.templateRoot, parts.stateA, parts.stateB, parts.button);

            // Raising does not itself change the group's current state.
            VERIFY_IS_TRUE(parts.group->CurrentState == stateBeforeRaise);

            parts.group->CurrentStateChanging -= changingToken;
            parts.group->CurrentStateChanged -= changedToken;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1u, static_cast<unsigned int>(log->size()), L"No additional deferred raise is expected.");
        });
    }

    void VisualStateGroupEventsTests::CustomVsmRaiseCurrentStateChangedInvokesGroupHandlers()
    {
        TestCleanupWrapper cleanup;
        auto parts = LoadTemplatedButton();
        auto log = std::make_shared<StateEventLog>();

        RunOnUIThread([&]()
        {
            auto changingToken = parts.group->CurrentStateChanging += MakeRecorder(StateEventKind::Changing, log, nullptr);
            auto changedToken = parts.group->CurrentStateChanged += MakeRecorder(StateEventKind::Changed, log, nullptr);
            auto vsm = ref new EventRaisingVisualStateManager();

            // Old/new states are passed through unchanged; use the reverse transition (B -> A) to distinguish them from the Changing test.
            vsm->InvokeRaiseCurrentStateChanged(parts.group, parts.stateB, parts.stateA, parts.button);

            VERIFY_ARE_EQUAL(1u, static_cast<unsigned int>(log->size()));
            VerifyRecord((*log)[0], StateEventKind::Changed, parts.templateRoot, parts.stateB, parts.stateA, parts.button);

            parts.group->CurrentStateChanging -= changingToken;
            parts.group->CurrentStateChanged -= changedToken;
        });
    }

    void VisualStateGroupEventsTests::CustomVsmRaiseForControlWithoutTemplateIsIgnored()
    {
        TestCleanupWrapper cleanup;
        auto parts = LoadTemplatedButton();
        auto log = std::make_shared<StateEventLog>();

        RunOnUIThread([&]()
        {
            auto changingToken = parts.group->CurrentStateChanging += MakeRecorder(StateEventKind::Changing, log, nullptr);
            auto changedToken = parts.group->CurrentStateChanged += MakeRecorder(StateEventKind::Changed, log, nullptr);
            auto vsm = ref new EventRaisingVisualStateManager();

            // Never added to the tree, so no template has been applied.
            auto untemplatedControl = ref new Button();
            VERIFY_ARE_EQUAL(0, VisualTreeHelper::GetChildrenCount(untemplatedControl));

            vsm->InvokeRaiseCurrentStateChanging(parts.group, parts.stateA, parts.stateB, untemplatedControl);
            vsm->InvokeRaiseCurrentStateChanged(parts.group, parts.stateA, parts.stateB, untemplatedControl);
            VERIFY_ARE_EQUAL(0u, static_cast<unsigned int>(log->size()));

            // The same calls for the templated control do raise, so the group handlers are live.
            vsm->InvokeRaiseCurrentStateChanging(parts.group, parts.stateA, parts.stateB, parts.button);
            VERIFY_ARE_EQUAL(1u, static_cast<unsigned int>(log->size()));

            parts.group->CurrentStateChanging -= changingToken;
            parts.group->CurrentStateChanged -= changedToken;
        });
    }

    void VisualStateGroupEventsTests::CustomVsmRaiseWithNullNewStateFails()
    {
        TestCleanupWrapper cleanup;
        auto parts = LoadTemplatedButton();
        auto log = std::make_shared<StateEventLog>();

        RunOnUIThread([&]()
        {
            auto changingToken = parts.group->CurrentStateChanging += MakeRecorder(StateEventKind::Changing, log, nullptr);
            auto changedToken = parts.group->CurrentStateChanged += MakeRecorder(StateEventKind::Changed, log, nullptr);
            auto vsm = ref new EventRaisingVisualStateManager();

            HRESULT changingHr = S_OK;
            try
            {
                vsm->InvokeRaiseCurrentStateChanging(parts.group, parts.stateA, nullptr, parts.button);
            }
            catch (Exception^ e)
            {
                changingHr = static_cast<HRESULT>(e->HResult);
            }

            HRESULT changedHr = S_OK;
            try
            {
                vsm->InvokeRaiseCurrentStateChanged(parts.group, parts.stateA, nullptr, parts.button);
            }
            catch (Exception^ e)
            {
                changedHr = static_cast<HRESULT>(e->HResult);
            }

            // The public VisualStateManager wrapper validates every argument (ARG_NOTNULL -> E_INVALIDARG).
            VERIFY_ARE_EQUAL(E_INVALIDARG, changingHr);
            VERIFY_ARE_EQUAL(E_INVALIDARG, changedHr);
            VERIFY_ARE_EQUAL(0u, static_cast<unsigned int>(log->size()));

            parts.group->CurrentStateChanging -= changingToken;
            parts.group->CurrentStateChanged -= changedToken;
        });
    }

} } } } }
