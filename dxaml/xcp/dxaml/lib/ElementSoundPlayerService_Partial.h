// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "ElementSoundPlayerService.g.h"
#include "Control.g.h"
#include "XboxUtility.h"
#include <fwd/windows.media.h>

namespace DirectUI
{
    class ElementSoundPlayerServiceWorkerData;

    PARTIAL_CLASS(ElementSoundPlayerService)
    {
    public:
        // UI-thread-only public methods
        ElementSoundPlayerService();
        ~ElementSoundPlayerService() override;

        static xaml::ElementSoundMode GetEffectiveSoundMode(_In_ DependencyObject* element);
        static _Check_return_ HRESULT RequestInteractionSoundForElementStatic(xaml::ElementSoundKind sound, _In_opt_ DependencyObject* element);
        static _Check_return_ HRESULT RequestInteractionSoundForElementStatic(DirectUI::ElementSoundKind sound, _In_ CDependencyObject* element);
        static _Check_return_ HRESULT PlayInteractionSoundStatic();

        double GetVolume() const
        {
            ASSERT(S_OK == CheckThread());

            return m_volume;
        }

        xaml::ElementSoundPlayerState GetPlayerState() const
        {
            ASSERT(S_OK == CheckThread());

            return m_playerState;
        }

        _Check_return_ HRESULT Initialize() override;
        _Check_return_ HRESULT SetPlayerState(xaml::ElementSoundPlayerState state);
        _Check_return_ HRESULT SetVolume(double volume);
        _Check_return_ HRESULT Play(xaml::ElementSoundKind sound);
        // RequestInteractionSoundForElement is an internal API that checks the ElementSoundState of your control for you
        // if the ElementSoundState allows this sound to be played on behalf of the specified DO, it process the request.
        // It does not immediately play the sound you're requesting, and might not play it at all:
        //    We only want to play one sound per user interaction.
        //    Say your app has a button launches a flyout, when you click that button you'll hear an Invoke sound (because the button was clicked)
        //    and then a Show sound (because the flyout was shown) in quick succession like a sound effect shotgun.
        //    That's a yucky experience, to avoid it we've added this concept of "requesting" a sound on an interaction.
        //    If the sound you're requesting is the highest priority sound requested so far for the current interaction
        //    Then the interaction m_interactionSound is updated.
        _Check_return_ HRESULT RequestInteractionSoundForElement(xaml::ElementSoundKind sound, _In_ DirectUI::DependencyObject* element);
        _Check_return_ HRESULT TearDownAudioGraph();

        // For testing purposes only. Invoked by IXamlTestHooks::SetPlayingSoundNodeCallback implementation.
        void SetPlayingSoundNodeCallback(_In_opt_ std::function<void(xaml::ElementSoundKind, BOOLEAN isSpatialAudio, float x, float y, float z, double volume)> callback);

        // For testing purposes only.  Invoked by test cleanup to ensure that the teardown thread is completed
        // prior to returning from the teardown request.
        void WaitForThreadOnTearDown();

        _Check_return_ HRESULT SetSpatialAudioMode(_In_ xaml::ElementSpatialAudioMode isSpatialAudioEnabled);
        _Check_return_ xaml::ElementSpatialAudioMode GetSpatialAudioMode() { return m_spatialAudioMode; }

        bool CalculateSpatialAudioSetting();

        // Contains parameters unique to each sound playback instance
        struct ElementSoundData
        {
            // Members used by spatial and non-spatial sounds
            xaml::ElementSoundKind elementSoundKind = xaml::ElementSoundKind_Focus;
            double volume = 0;

            // Members used by spatial sounds only
            DependencyObject* targetElement = nullptr;

            // All Xaml sounds currently play in omni and have no direction or rotation.
            wfn::Vector3 position = {};
        };

    private:
        // UI-thread-only private methods
        static bool IsSilentElement(_In_ xaml::IDependencyObject* element);
        static bool ShouldElementPlaySound(xaml::ElementSoundKind sound, _In_ DependencyObject* element);
        static ElementSoundPlayerService* GetCurrent();

        bool ShouldPlaySound() const
        {
            ASSERT(S_OK == CheckThread());

            return m_volume > 0 && (m_playerState == xaml::ElementSoundPlayerState_On || (m_playerState == xaml::ElementSoundPlayerState_Auto && XboxUtility::IsOnXbox()));
        }

        _Check_return_ HRESULT EnsureWorkerThread();

        // Calling PlayInteractionSound will play whatever is currently in m_interactionSound
        _Check_return_ HRESULT PlayInteractionSound();

        // Worker-thread-only private methods
        static unsigned long WINAPI WorkerJobStatic(_In_ void* workerDataWeakReference);

        // Thread-safe private methods
        static int GetSoundPriority(xaml::ElementSoundKind sound);

        static _Check_return_ HRESULT EnsureValidSound(xaml::ElementSoundKind sound);

        // Methods for calculating sound position for spatial audio
        static void CalculateElementCenterPoint(_In_ DependencyObject* element, _Out_ float& x, _Out_ float& y);

    private:
        // UI-thread-only fields
        bool m_hasInteractionSound{ false };
        double m_volume{ 1.0 }; // Volume is 1 by default, this means that the sound resource is being played at 100% of the app's allocated volume
        xaml::ElementSoundKind m_interactionSound{ xaml::ElementSoundKind_Focus };
        xaml::ElementSoundPlayerState m_playerState{ xaml::ElementSoundPlayerState_Auto };
        TrackerPtr<ElementSoundPlayerServiceWorkerData> m_tpWorkerData;
        std::function<void(xaml::ElementSoundKind, BOOLEAN isSpatialAudio, float x, float y, float z, double volume)> m_playingSoundNodeCallback{ nullptr };

        // Global settings for spatial audio : Default value set in ElementSoundPlayerService::Initialize
        xaml::ElementSpatialAudioMode m_spatialAudioMode = xaml::ElementSpatialAudioMode_Auto;
        bool m_isSpatialAudioEnabled = false;

        // ElementSoundPlayerService sound data for the queued sound
        ElementSoundData m_pendingSoundData;
    };

    class __declspec(uuid("da6fc7cc-27b0-4367-9df0-538dfca8c72e")) ElementSoundPlayerServiceWorkerData : public ctl::WeakReferenceSource
    {
    private:
        // Thread-safe constants
        static const UINT32 c_NumberOfElementSounds = 7;
        static const UINT32 c_NumberOfFocusSounds = 5;
        static const UINT32 c_MaxNumberOfInputNodes = c_NumberOfElementSounds + c_NumberOfFocusSounds + 1;

        // Status shared among the XAML UI thread and XAML worker thread
        enum WorkerDataUIStatus // Enum used as a bit flag
        {
            UINone = 0x00,
            HasPendingSound = 0x01,
            HasPendingTeardown = 0x02
        };

        // Status shared among the XAML worker thread and Audio threads
        enum WorkerDataAudioStatus // Enum used as a bit flag
        {
            AudioNone = 0x00,
            HasAudioGraphOn = 0x01,
            HasPendingAudioGraph = 0x02 // Used while the AudioGraph is being set up, so we don't make multiple AudioGraphs.
        };

        struct AudioFrameInputNodeState
        {
            ctl::ComPtr<wm::Audio::IAudioFrameInputNode> inputNode;
            EventRegistrationToken audioFrameCompletedEvent{};
            bool isPlaying = false;
        };

    public:
        // UI-thread-only public methods
        ElementSoundPlayerServiceWorkerData();
        ~ElementSoundPlayerServiceWorkerData() override;

        _Check_return_ HRESULT Initialize() override;
        _Check_return_ HRESULT TearDownAudioGraph();

        // Worker-thread-only public methods

        // For testing purposes only. Invoked by IXamlTestHooks::SetPlayingSoundNodeCallback implementation.
        void SetPlayingSoundNodeCallback(_In_opt_ std::function<void(xaml::ElementSoundKind, BOOLEAN isSpatialAudio, float x, float y, float z, double volume)> callback)
        {
            m_playingSoundNodeCallback = std::move(callback);
        }

        // For testing purposes only.
        void EnsureWaitForThreadOnTearDown();

        _Check_return_ HRESULT WorkerJob();
        _Check_return_ HRESULT SetPendingSound(const ElementSoundPlayerService::ElementSoundData& pendingSoundData);

        bool m_isSpatialAudioEnabled = false;

    protected:
        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** object) override;

    private:
        // Worker-thread-only private methods
        std::wstring GetPath(xaml::ElementSoundKind sound) const;
        ctl::ComPtr<wm::Audio::IAudioFrameInputNode> GetAudioNodeForSound(xaml::ElementSoundKind sound);
        void CycleFocusIndex();
        void TearDownAudioGraphPrivate();

        _Check_return_ HRESULT SetUpAudioGraph();
        _Check_return_ HRESULT PlayPendingSound();
        _Check_return_ HRESULT StopAudioGraph();
        _Check_return_ HRESULT LoadRawSound(xaml::ElementSoundKind sound);
        _Check_return_ HRESULT CreateAudioNodeForSound(xaml::ElementSoundKind sound, _COM_Outptr_ wm::Audio::IAudioFrameInputNode** audioFrameInputNodeResult);
        _Check_return_ HRESULT PlaySoundNode(_In_ wm::Audio::IAudioFrameInputNode* node);
        _Check_return_ HRESULT OnCreateAudioGraphCompleted(_In_ wf::IAsyncOperation<wm::Audio::CreateAudioGraphResult*>* asyncOp, wf::AsyncStatus status);
        _Check_return_ HRESULT OnCreateDeviceOutputNodeCompleted(_In_ wf::IAsyncOperation<wm::Audio::CreateAudioDeviceOutputNodeResult*>* asyncOp, wf::AsyncStatus status);
        _Check_return_ HRESULT OnGraphSetupCompleted();
        _Check_return_ HRESULT OnAudioFrameCompleted(_In_ wm::Audio::IAudioFrameInputNode* sender);

        // Thread-safe constants
        static const DWORD c_stopGraphDelayInMs = 2000;
        static const UINT32 c_sampleRateInHz = 48000;
        static const UINT32 c_channelCount = 1;
        static const UINT32 c_bitsPerSample = 16;

        static constexpr const wchar_t* s_SoundFileNames[c_NumberOfElementSounds] = {
            L"Focus0_48000Hz.raw",
            L"Invoke_48000Hz.raw",
            L"Show_48000Hz.raw",
            L"Hide_48000Hz.raw",
            L"MovePrevious_48000Hz.raw",
            L"MoveNext_48000Hz.raw",
            L"GoBack_48000Hz.raw" };

        static constexpr const wchar_t* s_FocusSoundFileNames[c_NumberOfFocusSounds] = {
            L"Focus0_48000Hz.raw",
            L"Focus1_48000Hz.raw",
            L"Focus2_48000Hz.raw",
            L"Focus3_48000Hz.raw",
            L"Focus4_48000Hz.raw" };

        // Thread-safe private methods
        static void EnsureSoundIndexIsInRange(xaml::ElementSoundKind sound)
        {
            ASSERT(sound >= 0);
            ASSERT(sound < static_cast<int>(c_NumberOfElementSounds));
        }

        static void EnsureFocusIndexIsInRange(UINT32 focusIndex)
        {
            ASSERT(focusIndex < c_NumberOfFocusSounds);
        }

        // Worker-thread-only fields
        UINT32 m_focusIndex{ 0 };  // The index of the next focus sound we'll play

        // Worker thread sound data : exclusive to the worker thread - does not require a lock
        ElementSoundPlayerService::ElementSoundData m_currentSoundData;

        // Arrays to store AudioFrameInputNodes, and AudioFrames for each of the sounds in the ElementSound enum
        std::map<DWORD_PTR, std::unique_ptr<AudioFrameInputNodeState>> m_inputNodes;
        std::array<ctl::ComPtr<wm::IAudioFrame>, c_NumberOfElementSounds> m_inputFrames;
        std::array<ctl::ComPtr<wm::IAudioFrame>, c_NumberOfFocusSounds> m_focusInputFrames;

        // Shared fields between XAML UI and worker threads
        WorkerDataUIStatus m_uiStatus{ WorkerDataUIStatus::UINone };
        ElementSoundPlayerService::ElementSoundData m_pendingSoundData;
                                                                               // the last requested sound so that once creation is finished, we can play it.
        HANDLE m_event{ nullptr };
        HANDLE m_teardownEvent{ nullptr };
        std::function<void(xaml::ElementSoundKind, BOOLEAN isSpatialAudio, float x, float y, float z, double volume)> m_playingSoundNodeCallback{ nullptr };
        SRWLOCK m_uiLock;

        // Shared fields between XAML worker thread and Audio threads
        UINT32 m_voiceCount{ 0 };
        WorkerDataAudioStatus m_audioStatus{ WorkerDataAudioStatus::AudioNone };
        ctl::ComPtr<wm::Audio::IAudioNode> m_outputNode;
        ctl::ComPtr<wm::Audio::IAudioGraph> m_graph;
        EventRegistrationToken m_audioGraphUnrecoverableErrorOccurredEventToken{};
        SRWLOCK m_audioLock;
    };
}
