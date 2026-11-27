// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ElementSoundPlayerTests.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace ElementSound {

namespace
{
    const xaml::ElementSoundKind c_allSoundKinds[] =
    {
        xaml::ElementSoundKind::Focus,
        xaml::ElementSoundKind::Invoke,
        xaml::ElementSoundKind::Show,
        xaml::ElementSoundKind::Hide,
        xaml::ElementSoundKind::MovePrevious,
        xaml::ElementSoundKind::MoveNext,
        xaml::ElementSoundKind::GoBack,
    };

    template <typename TAction>
    void VerifyThrowsInvalidArg(TAction action)
    {
        bool threw = false;
        try
        {
            action();
        }
        catch (Platform::Exception^ e)
        {
            threw = true;
            VERIFY_ARE_EQUAL(E_INVALIDARG, e->HResult);
        }
        VERIFY_IS_TRUE(threw);
    }

    void VerifyPlayValidatesSoundKind()
    {
        for (const auto soundKind : c_allSoundKinds)
        {
            LOG_OUTPUT(L"Play(%d)", static_cast<int>(soundKind));
            xaml::ElementSoundPlayer::Play(soundKind);
        }

        // The first value after GoBack, and a negative value, are not valid sounds.
        VerifyThrowsInvalidArg([]() { xaml::ElementSoundPlayer::Play(static_cast<xaml::ElementSoundKind>(static_cast<int>(xaml::ElementSoundKind::GoBack) + 1)); });
        VerifyThrowsInvalidArg([]() { xaml::ElementSoundPlayer::Play(static_cast<xaml::ElementSoundKind>(-1)); });
    }

    // Restores the process-wide ElementSoundPlayer settings to their defaults, which also stops the sound worker.
    void RestoreElementSoundPlayerDefaults()
    {
        xaml::ElementSoundPlayer::State = xaml::ElementSoundPlayerState::Off;
        xaml::ElementSoundPlayer::SpatialAudioMode = xaml::ElementSpatialAudioMode::Auto;
        xaml::ElementSoundPlayer::Volume = 1.0;
        xaml::ElementSoundPlayer::State = xaml::ElementSoundPlayerState::Auto;
    }
}

bool ElementSoundPlayerTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool ElementSoundPlayerTests::TestSetup()
{
    TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool ElementSoundPlayerTests::TestCleanup()
{
    TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void ElementSoundPlayerTests::StateVolumeAndSpatialAudioModeRoundTrip()
{
    TestCleanupWrapper cleanup;
    auto restoreDefaults = wil::scope_exit([]()
    {
        RunOnUIThread([]() { RestoreElementSoundPlayerDefaults(); });
    });

    RunOnUIThread([]()
    {
        // Defaults.
        VERIFY_ARE_EQUAL(xaml::ElementSoundPlayerState::Auto, xaml::ElementSoundPlayer::State);
        VERIFY_ARE_EQUAL(1.0, xaml::ElementSoundPlayer::Volume);
        VERIFY_ARE_EQUAL(xaml::ElementSpatialAudioMode::Auto, xaml::ElementSoundPlayer::SpatialAudioMode);

        // State round-trips, and turning sounds on and off again doesn't change the volume.
        for (const auto state : { xaml::ElementSoundPlayerState::On, xaml::ElementSoundPlayerState::Off, xaml::ElementSoundPlayerState::Auto, xaml::ElementSoundPlayerState::On })
        {
            xaml::ElementSoundPlayer::State = state;
            VERIFY_ARE_EQUAL(state, xaml::ElementSoundPlayer::State);
            VERIFY_ARE_EQUAL(1.0, xaml::ElementSoundPlayer::Volume);
        }

        // Volume round-trips while sounds are on, including both ends of the valid range.
        for (const double volume : { 0.0, 0.37, 1.0, 0.0, 0.5 })
        {
            xaml::ElementSoundPlayer::Volume = volume;
            VERIFY_ARE_EQUAL(volume, xaml::ElementSoundPlayer::Volume);
            VERIFY_ARE_EQUAL(xaml::ElementSoundPlayerState::On, xaml::ElementSoundPlayer::State);
        }

        // SpatialAudioMode round-trips while sounds are on, and doesn't change the state or the volume.
        for (const auto mode : { xaml::ElementSpatialAudioMode::On, xaml::ElementSpatialAudioMode::Off, xaml::ElementSpatialAudioMode::Auto, xaml::ElementSpatialAudioMode::On })
        {
            xaml::ElementSoundPlayer::SpatialAudioMode = mode;
            VERIFY_ARE_EQUAL(mode, xaml::ElementSoundPlayer::SpatialAudioMode);
            VERIFY_ARE_EQUAL(xaml::ElementSoundPlayerState::On, xaml::ElementSoundPlayer::State);
            VERIFY_ARE_EQUAL(0.5, xaml::ElementSoundPlayer::Volume);
        }

        // Setting the same values again is a no-op.
        xaml::ElementSoundPlayer::State = xaml::ElementSoundPlayerState::On;
        xaml::ElementSoundPlayer::Volume = 0.5;
        xaml::ElementSoundPlayer::SpatialAudioMode = xaml::ElementSpatialAudioMode::On;
        VERIFY_ARE_EQUAL(xaml::ElementSoundPlayerState::On, xaml::ElementSoundPlayer::State);
        VERIFY_ARE_EQUAL(0.5, xaml::ElementSoundPlayer::Volume);
        VERIFY_ARE_EQUAL(xaml::ElementSpatialAudioMode::On, xaml::ElementSoundPlayer::SpatialAudioMode);

        // Turning sounds off keeps the volume and spatial audio mode.
        xaml::ElementSoundPlayer::State = xaml::ElementSoundPlayerState::Off;
        VERIFY_ARE_EQUAL(xaml::ElementSoundPlayerState::Off, xaml::ElementSoundPlayer::State);
        VERIFY_ARE_EQUAL(0.5, xaml::ElementSoundPlayer::Volume);
        VERIFY_ARE_EQUAL(xaml::ElementSpatialAudioMode::On, xaml::ElementSoundPlayer::SpatialAudioMode);
    });
}

void ElementSoundPlayerTests::VolumeOutOfRangeIsRejected()
{
    TestCleanupWrapper cleanup;
    auto restoreDefaults = wil::scope_exit([]()
    {
        RunOnUIThread([]() { RestoreElementSoundPlayerDefaults(); });
    });

    RunOnUIThread([]()
    {
        for (const auto state : { xaml::ElementSoundPlayerState::Off, xaml::ElementSoundPlayerState::On })
        {
            xaml::ElementSoundPlayer::State = state;
            xaml::ElementSoundPlayer::Volume = 0.4;

            for (const double invalidVolume : { -0.001, 1.001, -1.0, 2.0 })
            {
                LOG_OUTPUT(L"State = %d, setting Volume = %f", static_cast<int>(state), invalidVolume);
                VerifyThrowsInvalidArg([invalidVolume]() { xaml::ElementSoundPlayer::Volume = invalidVolume; });
                VERIFY_ARE_EQUAL(0.4, xaml::ElementSoundPlayer::Volume);
                VERIFY_ARE_EQUAL(state, xaml::ElementSoundPlayer::State);
            }
        }
    });
}

void ElementSoundPlayerTests::PlayValidatesSoundKind()
{
    TestCleanupWrapper cleanup;
    auto restoreDefaults = wil::scope_exit([]()
    {
        RunOnUIThread([]() { RestoreElementSoundPlayerDefaults(); });
    });

    RunOnUIThread([]()
    {
        // With sounds off, the sound kind is still validated.
        xaml::ElementSoundPlayer::State = xaml::ElementSoundPlayerState::Off;
        VerifyPlayValidatesSoundKind();

        // With sounds on.
        xaml::ElementSoundPlayer::State = xaml::ElementSoundPlayerState::On;
        xaml::ElementSoundPlayer::Volume = 0.5;
        VerifyPlayValidatesSoundKind();

        // With sounds on and spatial audio on.
        xaml::ElementSoundPlayer::SpatialAudioMode = xaml::ElementSpatialAudioMode::On;
        VerifyPlayValidatesSoundKind();

        // With sounds on but a volume of 0, nothing is played but sounds are still validated.
        xaml::ElementSoundPlayer::Volume = 0.0;
        VerifyPlayValidatesSoundKind();

        VERIFY_ARE_EQUAL(xaml::ElementSoundPlayerState::On, xaml::ElementSoundPlayer::State);
        VERIFY_ARE_EQUAL(0.0, xaml::ElementSoundPlayer::Volume);
        VERIFY_ARE_EQUAL(xaml::ElementSpatialAudioMode::On, xaml::ElementSoundPlayer::SpatialAudioMode);
    });

    TestServices::WindowHelper->WaitForIdle();
}

void ElementSoundPlayerTests::ButtonInvokeWithSpatialAudio()
{
    TestCleanupWrapper cleanup;
    auto restoreDefaults = wil::scope_exit([]()
    {
        RunOnUIThread([]() { RestoreElementSoundPlayerDefaults(); });
    });

    auto defaultModeClicks = std::make_shared<int>(0);
    auto silentClicks = std::make_shared<int>(0);
    xaml_controls::Button^ defaultModeButton = nullptr;
    xaml_controls::Button^ silentButton = nullptr;

    RunOnUIThread([&]()
    {
        xaml::ElementSoundPlayer::State = xaml::ElementSoundPlayerState::On;
        xaml::ElementSoundPlayer::Volume = 0.75;
        xaml::ElementSoundPlayer::SpatialAudioMode = xaml::ElementSpatialAudioMode::On;

        defaultModeButton = ref new xaml_controls::Button();
        defaultModeButton->Content = L"Default";
        defaultModeButton->Width = 120;
        defaultModeButton->Height = 40;
        defaultModeButton->Click += ref new xaml::RoutedEventHandler([defaultModeClicks](Platform::Object^, xaml::RoutedEventArgs^)
        {
            ++(*defaultModeClicks);
        });

        silentButton = ref new xaml_controls::Button();
        silentButton->Content = L"Silent";
        silentButton->Width = 80;
        silentButton->Height = 40;
        silentButton->ElementSoundMode = xaml::ElementSoundMode::Off;
        silentButton->Click += ref new xaml::RoutedEventHandler([silentClicks](Platform::Object^, xaml::RoutedEventArgs^)
        {
            ++(*silentClicks);
        });

        auto root = ref new xaml_controls::StackPanel();
        root->Margin = xaml::ThicknessHelper::FromLengths(30, 50, 0, 0);
        root->Children->Append(defaultModeButton);
        root->Children->Append(silentButton);
        TestServices::WindowHelper->WindowContent = root;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(xaml::ElementSoundMode::Default, defaultModeButton->ElementSoundMode);
        VERIFY_ARE_EQUAL(xaml::ElementSoundMode::Off, silentButton->ElementSoundMode);

        auto defaultModePeer = safe_cast<xaml_automation_peers::ButtonAutomationPeer^>(xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(defaultModeButton));
        auto silentPeer = safe_cast<xaml_automation_peers::ButtonAutomationPeer^>(xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(silentButton));

        // Invoking requests the Invoke interaction sound for the button (positioned, since spatial audio is on).
        defaultModePeer->Invoke();
        silentPeer->Invoke();
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // A second interaction after the first interaction sound was played.
        auto defaultModePeer = safe_cast<xaml_automation_peers::ButtonAutomationPeer^>(xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(defaultModeButton));
        defaultModePeer->Invoke();
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // The sound mode doesn't change the click behavior, and the sound settings are unchanged by the interactions.
        VERIFY_ARE_EQUAL(2, *defaultModeClicks);
        VERIFY_ARE_EQUAL(1, *silentClicks);
        VERIFY_ARE_EQUAL(xaml::ElementSoundPlayerState::On, xaml::ElementSoundPlayer::State);
        VERIFY_ARE_EQUAL(0.75, xaml::ElementSoundPlayer::Volume);
        VERIFY_ARE_EQUAL(xaml::ElementSpatialAudioMode::On, xaml::ElementSoundPlayer::SpatialAudioMode);

        defaultModeButton = nullptr;
        silentButton = nullptr;
    });
}

void ElementSoundPlayerTests::InteractionSoundsForTextElementsAndFlyouts()
{
    TestCleanupWrapper cleanup;
    auto restoreDefaults = wil::scope_exit([]()
    {
        RunOnUIThread([]() { RestoreElementSoundPlayerDefaults(); });
    });

    xaml::IElementSoundPlayerStaticsPrivate^ soundPlayerPrivate = nullptr;
    xaml_controls::StackPanel^ root = nullptr;
    xaml_controls::Button^ owner = nullptr;
    xaml_controls::Flyout^ flyout = nullptr;
    xaml_controls::Flyout^ emptyFlyout = nullptr;
    auto openedEvent = std::make_shared<Event>();
    auto closedEvent = std::make_shared<Event>();
    wf::EventRegistrationToken openedToken = {};
    wf::EventRegistrationToken closedToken = {};

    RunOnUIThread([&]()
    {
        Platform::Object^ factory = nullptr;
        VERIFY_SUCCEEDED(::Windows::Foundation::GetActivationFactory(
            Microsoft::WRL::Wrappers::HStringReference(L"Microsoft.UI.Xaml.ElementSoundPlayer").Get(),
            reinterpret_cast<IInspectable**>(&factory)));
        soundPlayerPrivate = dynamic_cast<xaml::IElementSoundPlayerStaticsPrivate^>(factory);
        VERIFY_IS_NOT_NULL(soundPlayerPrivate);

        xaml::ElementSoundPlayer::State = xaml::ElementSoundPlayerState::On;
        xaml::ElementSoundPlayer::Volume = 0.6;
        xaml::ElementSoundPlayer::SpatialAudioMode = xaml::ElementSpatialAudioMode::On;

        root = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Margin='20,30,0,0'>"
            L"  <TextBlock x:Name='text'>"
            L"    <Hyperlink x:Name='link'>Link</Hyperlink>"
            L"    <Run Text=' '/>"
            L"    <Hyperlink x:Name='silentLink' ElementSoundMode='Off'>Silent link</Hyperlink>"
            L"  </TextBlock>"
            L"  <ContentControl x:Name='silentParent' ElementSoundMode='Off'>"
            L"    <Button x:Name='innerButton' Content='Inner'/>"
            L"  </ContentControl>"
            L"  <Button x:Name='focusOnlyButton' Content='Focus only' ElementSoundMode='FocusOnly'/>"
            L"  <Button x:Name='owner' Content='Owner'/>"
            L"</StackPanel>"));
        owner = safe_cast<xaml_controls::Button^>(root->FindName(L"owner"));

        flyout = ref new xaml_controls::Flyout();
        auto flyoutContent = ref new xaml_controls::Border();
        flyoutContent->Width = 80;
        flyoutContent->Height = 60;
        flyout->Content = flyoutContent;
        openedToken = flyout->Opened += ref new wf::EventHandler<Platform::Object^>([openedEvent](Platform::Object^, Platform::Object^) { openedEvent->Set(); });
        closedToken = flyout->Closed += ref new wf::EventHandler<Platform::Object^>([closedEvent](Platform::Object^, Platform::Object^) { closedEvent->Set(); });

        emptyFlyout = ref new xaml_controls::Flyout();

        TestServices::WindowHelper->WindowContent = root;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        auto link = safe_cast<xaml::DependencyObject^>(root->FindName(L"link"));
        auto silentLink = safe_cast<xaml::DependencyObject^>(root->FindName(L"silentLink"));
        auto text = safe_cast<xaml::DependencyObject^>(root->FindName(L"text"));
        auto innerButton = safe_cast<xaml::DependencyObject^>(root->FindName(L"innerButton"));
        auto focusOnlyButton = safe_cast<xaml::DependencyObject^>(root->FindName(L"focusOnlyButton"));

        // The effective sound mode is the most restrictive mode of the element and its ancestors.
        VERIFY_ARE_EQUAL(xaml::ElementSoundMode::Default, soundPlayerPrivate->GetEffectiveSoundMode(link));
        VERIFY_ARE_EQUAL(xaml::ElementSoundMode::Off, soundPlayerPrivate->GetEffectiveSoundMode(silentLink));
        VERIFY_ARE_EQUAL(xaml::ElementSoundMode::Off, soundPlayerPrivate->GetEffectiveSoundMode(innerButton));
        VERIFY_ARE_EQUAL(xaml::ElementSoundMode::FocusOnly, soundPlayerPrivate->GetEffectiveSoundMode(focusOnlyButton));
        VERIFY_ARE_EQUAL(xaml::ElementSoundMode::Default, soundPlayerPrivate->GetEffectiveSoundMode(owner));
        VERIFY_ARE_EQUAL(xaml::ElementSoundMode::Default, soundPlayerPrivate->GetEffectiveSoundMode(emptyFlyout));

        // Interaction sounds for a text element (positioned at its containing TextBlock), for silent elements and for
        // elements whose sound mode doesn't allow the sound. Pending requests are prioritized (Focus is the lowest), and
        // all requests below run in one UI callback before the pending sound is played and its priority reset. So the
        // Focus request for the FocusOnly button (which its sound mode allows) comes first; after the Invoke request for
        // `link`, a Focus request would be skipped before its sound mode is checked.
        soundPlayerPrivate->RequestInteractionSoundForElement(xaml::ElementSoundKind::Focus, focusOnlyButton);
        soundPlayerPrivate->RequestInteractionSoundForElement(xaml::ElementSoundKind::Invoke, link);
        soundPlayerPrivate->RequestInteractionSoundForElement(xaml::ElementSoundKind::Invoke, silentLink);
        soundPlayerPrivate->RequestInteractionSoundForElement(xaml::ElementSoundKind::Invoke, text);
        soundPlayerPrivate->RequestInteractionSoundForElement(xaml::ElementSoundKind::Invoke, focusOnlyButton);

        // A flyout without content is positioned at the center of the window.
        soundPlayerPrivate->RequestInteractionSoundForElement(xaml::ElementSoundKind::Show, emptyFlyout);

        // Invalid sound kinds are rejected.
        VerifyThrowsInvalidArg([soundPlayerPrivate, link]()
        {
            soundPlayerPrivate->RequestInteractionSoundForElement(static_cast<xaml::ElementSoundKind>(static_cast<int>(xaml::ElementSoundKind::GoBack) + 1), link);
        });
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        flyout->ShowAt(owner);
    });
    auto hideFlyout = wil::scope_exit([&]()
    {
        // Don't leave the flyout open if a verification fails while it is shown.
        RunOnUIThread([&]()
        {
            if (flyout && flyout->IsOpen)
            {
                flyout->Hide();
            }
        });
    });
    openedEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(xaml::ElementSoundMode::Default, soundPlayerPrivate->GetEffectiveSoundMode(flyout));

        // A flyout with content is positioned at its content.
        soundPlayerPrivate->RequestInteractionSoundForElement(xaml::ElementSoundKind::Hide, flyout);
        flyout->Hide();
    });
    closedEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // The requests don't change the player settings.
        VERIFY_ARE_EQUAL(xaml::ElementSoundPlayerState::On, xaml::ElementSoundPlayer::State);
        VERIFY_ARE_EQUAL(0.6, xaml::ElementSoundPlayer::Volume);
        VERIFY_ARE_EQUAL(xaml::ElementSpatialAudioMode::On, xaml::ElementSoundPlayer::SpatialAudioMode);

        flyout->Opened -= openedToken;
        flyout->Closed -= closedToken;
        flyout = nullptr;
        emptyFlyout = nullptr;
        owner = nullptr;
        root = nullptr;
        soundPlayerPrivate = nullptr;
    });
}

} } } } } }
