// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ElementSoundPlayerService.g.h"
#include <AudioGraphSettingsExtension.h>
#include "FlyoutBase.g.h"
#include "Hyperlink.g.h"
#include <Pathcch.h>
#include "Callback.h"
#include "Window_Partial.h"
#include "Flyout.g.h"
#include "UIElement.g.h"
#include "DesktopUtility.h"
#include "RootScale.h"
#include <MemoryBuffer.h>

using namespace DirectUI;
using namespace xaml_primitives;

// UI-thread-only public methods

ElementSoundPlayerService::ElementSoundPlayerService()
{
    ASSERT(S_OK == CheckThread());
}

ElementSoundPlayerService::~ElementSoundPlayerService()
{
    VERIFYHR(TearDownAudioGraph());
}

/* GetEffectiveSoundMode does a walk up the visual tree from the provided element to see if any of its parents apply more restrictive SoundMode rules than it does.
If a parent's sound mode is more restrictive than the element's, then the parent's rule applies as the "effective sound mode". */
/*static*/
xaml::ElementSoundMode
ElementSoundPlayerService::GetEffectiveSoundMode(_In_ DependencyObject* element)
{
    return static_cast<xaml::ElementSoundMode>(element->GetHandle()->GetEffectiveSoundMode());
}

/*static*/
_Check_return_ HRESULT
ElementSoundPlayerService::RequestInteractionSoundForElementStatic(xaml::ElementSoundKind sound, _In_opt_ DependencyObject* element)
{
    IFC_RETURN(EnsureValidSound(sound));

    if (element)
    {
        ElementSoundPlayerService* service = GetCurrent();

        if (service)
        {
            IFC_RETURN(service->RequestInteractionSoundForElement(sound, element));
        }
    }

    return S_OK;
}

/*static*/
_Check_return_ HRESULT
ElementSoundPlayerService::RequestInteractionSoundForElementStatic(DirectUI::ElementSoundKind sound, _In_ CDependencyObject* element)
{
    IFC_RETURN(EnsureValidSound(static_cast<xaml::ElementSoundKind>(sound)));

    DXamlCore* core = DXamlCore::GetCurrent();

    if (core)
    {
        ctl::ComPtr<DependencyObject> target;

        IFC_RETURN(core->GetPeer(element, &target));

        if (target)
        {
            ElementSoundPlayerService* service = core->GetElementSoundPlayerServiceNoRef();

            if (service)
            {
                IFC_RETURN(service->RequestInteractionSoundForElement(static_cast<xaml::ElementSoundKind>(sound), target.Get()));
            }
        }
    }

    return S_OK;
}

/*static*/
_Check_return_ HRESULT
ElementSoundPlayerService::PlayInteractionSoundStatic()
{
    ElementSoundPlayerService* service = GetCurrent();

    if (service)
    {
        IFC_RETURN(service->PlayInteractionSound());
    }

    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerService::Initialize()
{
    ASSERT(S_OK == CheckThread());

    IFC_RETURN(__super::Initialize());

    // Get spatial audio setting based on SpatialAudioMode and platform
    m_isSpatialAudioEnabled = CalculateSpatialAudioSetting();

    if (ShouldPlaySound())
    {
        IFC_RETURN(EnsureWorkerThread());
    }

    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerService::SetPlayerState(xaml::ElementSoundPlayerState state)
{
    ASSERT(S_OK == CheckThread());

    if (m_playerState != state)
    {
        m_playerState = state;
        if (ShouldPlaySound())
        {
            IFC_RETURN(EnsureWorkerThread());
        }
        else
        {
            IFC_RETURN(TearDownAudioGraph());
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerService::SetVolume(double volume)
{
    ASSERT(S_OK == CheckThread());
    ASSERT(volume >= 0 && volume <= 1.0);

    if (m_volume != volume)
    {
        m_volume = volume;

        if (ShouldPlaySound())
        {
            IFC_RETURN(EnsureWorkerThread());
        }
        else
        {
            IFC_RETURN(TearDownAudioGraph());
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerService::Play(xaml::ElementSoundKind sound)
{
    ASSERT(S_OK == CheckThread());

    auto cleanup = wil::scope_exit([&]()
    {
        TracePlaySoundEnd();
    });

    TracePlaySoundBegin();

    IFC_RETURN(EnsureValidSound(sound));
    if (ShouldPlaySound())
    {
        IFC_RETURN(EnsureWorkerThread());

        // Call into the worker to play the sound
        m_pendingSoundData.elementSoundKind = sound;
        m_pendingSoundData.volume = m_volume;

        IFC_RETURN(m_tpWorkerData->SetPendingSound(m_pendingSoundData));

        // Reset sound data
        m_pendingSoundData = {};
    }

    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerService::RequestInteractionSoundForElement(xaml::ElementSoundKind sound, _In_ DependencyObject* element)
{
    ASSERT(S_OK == CheckThread());

    IFC_RETURN(EnsureValidSound(sound));

    // If the sound being added is equal or higher priority than the current interaction sound
    // then update the interaction sound
    if (GetSoundPriority(sound) >= GetSoundPriority(m_interactionSound))
    {
        if (ShouldElementPlaySound(sound, element))
        {
            m_interactionSound = sound;
            m_hasInteractionSound = true;

            // There is a new pending sound of higher priority. Overwrite the sound data for the previous sound.
            m_pendingSoundData = {};

            // Store the sound
            m_pendingSoundData.elementSoundKind = sound;

            // Set the sound volume to the current ElementSoundPlayer volume setting
            m_pendingSoundData.volume = m_volume;

            DXamlCore* dxamlCore = DXamlCore::GetCurrent();
            if (dxamlCore)
            {
                // If SpatialAudio is enabled, calculate the position in listener space for the element's requested sound
                // Note that the element position is *WINDOW COORDINATES*, not *SCREEN COORDINATES*. Position assumes that
                // the window is full screen, as requested by Shell.
                if (m_isSpatialAudioEnabled)
                {
                    // Store the element
                    m_pendingSoundData.targetElement = element;

                    float elementX = 0;
                    float elementY = 0;

                    // Calculate the center point in listener coordinates for this element, using physical pixels
                    CalculateElementCenterPoint(m_pendingSoundData.targetElement, elementX, elementY);

                    wf::Rect windowBounds = {};
                    IFC_RETURN(DXamlCore::GetCurrent()->GetContentBoundsForElement(element->GetHandle(), &windowBounds));

                    //perform transformation only if the element is spatially aware
                    if (windowBounds.Width > 0 && windowBounds.Height > 0)
                    {
                        // Adjust core window logical coordinates to physical client coordinates
                        const float zoomScale = RootScale::GetRasterizationScaleForElement(element->GetHandle());
                        windowBounds.Width *= zoomScale;
                        windowBounds.Height *= zoomScale;

                        // 1) Normalize element's position from pixel count to (0.0 � 1.0) range
                        // 2) Convert 2D element position to 3D sound position using the Y dimension of element's position as
                        //    the elevation of the sound with Y equal to zero as highest point and Y max as the lowest point
                        // 3) Map the 2D normalized element position to the sound position by changing the X coordinate value
                        //    from(0.0 to 1.0) to(-0.5 to 0.5) range
                        // 4) Magnifying  the planner position via multiplying by a gain factor
                        //
                        // Note: The Y coordinate of the sound position represents elevation,
                        // while X-Z values represent the transverse / horizontal plane
                        m_pendingSoundData.position.X = ((elementX / windowBounds.Width) - 0.5f) * 4.0f;
                        m_pendingSoundData.position.Y = (1.0f - (elementY / windowBounds.Height));
                        m_pendingSoundData.position.Z = ((elementY / windowBounds.Height) - 1.0f) * 4.0f;
                    }
                }

                IFC_RETURN(dxamlCore->GetXamlDispatcherNoRef()->RunAsync(
                    MakeCallback(&ElementSoundPlayerService::PlayInteractionSoundStatic)));
            }
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerService::TearDownAudioGraph()
{
    ASSERT(S_OK == CheckThread());

    auto workerData = m_tpWorkerData.GetSafeReference();
    if (workerData)
    {
        IFC_RETURN(workerData->TearDownAudioGraph());
    }

    m_tpWorkerData.Clear();

    return S_OK;
}

// For testing purposes only. Invoked by IXamlTestHooks::SetPlayingSoundNodeCallback implementation.
void
ElementSoundPlayerService::SetPlayingSoundNodeCallback(_In_opt_ std::function<void(xaml::ElementSoundKind, BOOLEAN isSpatialAudio, float x, float y, float z, double volume)> callback)
{
    ASSERT(S_OK == CheckThread());

    if (m_tpWorkerData)
    {
        m_tpWorkerData->SetPlayingSoundNodeCallback(callback);
    }
    else
    {
        m_playingSoundNodeCallback = std::move(callback);
    }
}

// For testing purposes only.
void 
ElementSoundPlayerService::WaitForThreadOnTearDown()
{
    ASSERT(S_OK == CheckThread());

    auto workerData = m_tpWorkerData.GetSafeReference();
    if (workerData)
    {
        workerData->EnsureWaitForThreadOnTearDown();
    }
}

// UI-thread-only private methods

/* An element type is a silent if it doesn't have a settable has an ElementSoundMode property.
These types of elements should not make sounds by default because it would be impossible
for our developers to turn those sounds off. */
/*static*/
bool
ElementSoundPlayerService::IsSilentElement(_In_ xaml::IDependencyObject* element)
{
    CDependencyObject* nativeTarget = static_cast<DependencyObject*>(element)->GetHandle();

    ASSERT(nativeTarget);

    return !(nativeTarget->OfTypeByIndex<KnownTypeIndex::Control>() ||
             nativeTarget->OfTypeByIndex<KnownTypeIndex::Hyperlink>() ||
             nativeTarget->OfTypeByIndex<KnownTypeIndex::FlyoutBase>());
}

/*static*/
bool
ElementSoundPlayerService::ShouldElementPlaySound(xaml::ElementSoundKind sound, _In_ DependencyObject* element)
{
    if (!IsSilentElement(element))
    {
        switch (GetEffectiveSoundMode(element))
        {
        case xaml::ElementSoundMode_Off:
            break;
        case xaml::ElementSoundMode_FocusOnly:
            if (sound == xaml::ElementSoundKind_Focus)
            {
                return true;
            }
            break;
        case xaml::ElementSoundMode_Default:
            return true;
        }
    }
    return false;
}

/*static*/
ElementSoundPlayerService*
ElementSoundPlayerService::GetCurrent()
{
    DXamlCore* core = DXamlCore::GetCurrent();

    if (core)
    {
        return core->GetElementSoundPlayerServiceNoRef();
    }
    return nullptr;
}

_Check_return_ HRESULT
ElementSoundPlayerService::EnsureWorkerThread()
{
    ASSERT(S_OK == CheckThread());

    if (m_tpWorkerData)
    {
        return S_OK; // Worker thread is already running
    }

    ctl::ComPtr<ElementSoundPlayerServiceWorkerData> workerData;

    IFC_RETURN(ctl::make(&workerData));

    // Set test callback function on the worker thread data object
    if (m_playingSoundNodeCallback)
    {
        workerData->SetPlayingSoundNodeCallback(m_playingSoundNodeCallback);
    }

    // Set spatial audio setting on the worker thread data object.
    // (The AudioGraph needs to be recreated each time this setting changes)
    workerData->m_isSpatialAudioEnabled = m_isSpatialAudioEnabled;

    SetPtrValue(m_tpWorkerData, workerData);

    wrl::ComPtr<IWeakReference> workerDataWeakRef;

    IFC_RETURN(m_tpWorkerData->GetWeakReference(&workerDataWeakRef));

    if (nullptr == ::CreateThread(
        nullptr /*lpThreadAttributes*/,
        0 /*dwStackSize*/,
        WorkerJobStatic /*lpStartAddress*/,
        reinterpret_cast<void*>(workerDataWeakRef.Detach()) /*lpParameter*/,
        0 /*dwCreationFlags*/,
        nullptr /*lpThreadId*/))
    {
        m_tpWorkerData.Clear();
        IFC_RETURN(HRESULT_FROM_WIN32(GetLastError()));
    }

    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerService::PlayInteractionSound()
{
    ASSERT(S_OK == CheckThread());

    if (m_hasInteractionSound)
    {
        IFC_RETURN(Play(m_interactionSound));
        m_hasInteractionSound = false;
        m_interactionSound = xaml::ElementSoundKind_Focus;
    }

    return S_OK;
}

// Worker-thread-only private methods
/*static*/
unsigned long WINAPI
ElementSoundPlayerService::WorkerJobStatic(_In_ void* workerDataWeakReference)
{
    wrl::ComPtr<IWeakReference> workerDataWeakRef;
    wrl::ComPtr<ElementSoundPlayerServiceWorkerData> workerData;

    workerDataWeakRef.Attach(reinterpret_cast<IWeakReference*>(workerDataWeakReference));
    ASSERT(workerDataWeakRef);
    VERIFYHR(workerDataWeakRef->Resolve<ElementSoundPlayerServiceWorkerData>(&workerData));
    workerDataWeakRef.Reset();

    if (workerData)
    {
        VERIFYHR(workerData->WorkerJob());
    }
    return 0;
}

// Thread-safe private methods

// Sounds are considered higher priority if they convey more specific meaning
// The GetSoundPriority method is used to prioritize requested interaction sounds
/*static*/
int
ElementSoundPlayerService::GetSoundPriority(xaml::ElementSoundKind sound)
{
    int priority = -1;

    switch (sound)
    {
    case xaml::ElementSoundKind_Focus:
        priority = 0;
        break;
    case xaml::ElementSoundKind_Invoke:
        priority = 1;
        break;
    case xaml::ElementSoundKind_Show:
    case xaml::ElementSoundKind_Hide:
        priority = 2;
        break;
    case xaml::ElementSoundKind_MovePrevious:
    case xaml::ElementSoundKind_MoveNext:
        priority = 3;
        break;
    case xaml::ElementSoundKind_GoBack:
        priority = 4;
        break;
    default:
        ASSERT(false);
    }
    return priority;
}

/*static*/
_Check_return_ HRESULT
ElementSoundPlayerService::EnsureValidSound(xaml::ElementSoundKind sound)
{
    if (sound != xaml::ElementSoundKind_Focus &&
        sound != xaml::ElementSoundKind_Invoke &&
        sound != xaml::ElementSoundKind_Show &&
        sound != xaml::ElementSoundKind_Hide &&
        sound != xaml::ElementSoundKind_MovePrevious &&
        sound != xaml::ElementSoundKind_MoveNext &&
        sound != xaml::ElementSoundKind_GoBack)
    {
        IFC_RETURN(E_INVALIDARG);
    }
    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerService::SetSpatialAudioMode(_In_ xaml::ElementSpatialAudioMode spatialAudioMode)
{
    if (m_spatialAudioMode != spatialAudioMode)
    {
        // Save the current mode
        m_spatialAudioMode = spatialAudioMode;

        // Calculate spatial audio setting from the mode and platform
        m_isSpatialAudioEnabled = CalculateSpatialAudioSetting();

        // Tear down the AudioGraph and clear cached AudioInputNodes
        IFC_RETURN(TearDownAudioGraph());
    }

    return S_OK;
}

// Calculate element center point in physical pixels
/*static*/
void ElementSoundPlayerService::CalculateElementCenterPoint(_In_ DependencyObject* element, _Out_ float& x, _Out_ float& y)
{
    CDependencyObject* coreDependencyObject = element->GetHandle();

    CUIElement* coreUIElement = nullptr;

    if (coreDependencyObject->OfTypeByIndex<KnownTypeIndex::UIElement>())
    {
        coreUIElement = static_cast<CUIElement*>(coreDependencyObject);
    }
    else if (coreDependencyObject->OfTypeByIndex<KnownTypeIndex::TextElement>())
    {
        coreUIElement = static_cast<CTextElement*>(coreDependencyObject)->GetContainingFrameworkElement();
    }
    else if (coreDependencyObject->OfTypeByIndex<KnownTypeIndex::Flyout>())
    {
        DirectUI::Flyout* flyout = static_cast<Flyout*>(element);
        ctl::ComPtr<xaml::IUIElement> spContent;

        IFCFAILFAST(flyout->get_Content(&spContent));

        if (spContent)
        {
            DirectUI::UIElement* uiElement = static_cast<UIElement*>(spContent.Get());
            coreUIElement = static_cast<CUIElement*>(uiElement->GetHandle());
        }

        // Use center screen position if this flyout does not have content
    }

    if (coreUIElement)
    {
        XRECTF_RB rect = {};
        XRECTF rectBounded;
        EmptyRectF(&rectBounded);
        IFCFAILFAST(coreUIElement->GetGlobalBounds(&rect));

        rectBounded = ToXRectF(rect);

        x = rectBounded.X + (rectBounded.Width / 2);
        y = rectBounded.Y + (rectBounded.Height / 2);
    }
}

bool ElementSoundPlayerService::CalculateSpatialAudioSetting()
{
    switch (m_spatialAudioMode)
    {
        case xaml::ElementSpatialAudioMode_Off:
            return false;
            break;

        case xaml::ElementSpatialAudioMode_On:
            return true;
            break;

        case xaml::ElementSpatialAudioMode_Auto:
            return XboxUtility::IsOnXbox();
            break;
    }

    return false;
}

// ElementSoundPlayerServiceWorkerData class implementation

// UI-thread-only public methods
ElementSoundPlayerServiceWorkerData::ElementSoundPlayerServiceWorkerData()
{
}

ElementSoundPlayerServiceWorkerData::~ElementSoundPlayerServiceWorkerData()
{
    if (m_event)
    {
        CloseHandle(m_event);
    }
}

_Check_return_ HRESULT
ElementSoundPlayerServiceWorkerData::Initialize()
{
    InitializeSRWLock(&m_uiLock);
    InitializeSRWLock(&m_audioLock);

    m_event = CreateEvent(nullptr, false /*bManualReset*/, false /*bInitialState*/, nullptr);
    IFCPTR_RETURN(m_event);

    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerServiceWorkerData::TearDownAudioGraph()
{
    {
        auto uiLock = wil::AcquireSRWLockExclusive(&m_uiLock);

        m_uiStatus = static_cast<WorkerDataUIStatus>(m_uiStatus | WorkerDataUIStatus::HasPendingTeardown);
    }

    ASSERT(m_event);
    IFCW32_RETURN(SetEvent(m_event));

    // If we have been request to wait for the tear down, then wait on that event.  We don't really care
    // whether we succeed (or timeout) as this is only invoked in test environments to ensure we have completed the
    // completed the tear down before leak detection occurs.
    if (m_teardownEvent)
    {
        WaitForSingleObject(m_teardownEvent, 5000);
        CloseHandle(m_teardownEvent);
        m_teardownEvent = nullptr;
    }

    return S_OK;
}

// Worker-thread-only public methods

_Check_return_ HRESULT
ElementSoundPlayerServiceWorkerData::WorkerJob()
{
    ASSERT(m_event);

    do
    {
        IFC_RETURN(SetUpAudioGraph());

        DWORD dwMilliseconds = INFINITE;

        {
            auto uiLock = wil::AcquireSRWLockShared(&m_uiLock);
            auto audioLock = wil::AcquireSRWLockShared(&m_audioLock);

            if (m_graph &&
                m_voiceCount == 0 &&
                (m_uiStatus & WorkerDataUIStatus::HasPendingSound) == WorkerDataUIStatus::UINone &&
                (m_audioStatus & WorkerDataAudioStatus::HasAudioGraphOn) == WorkerDataAudioStatus::HasAudioGraphOn)
            {
                dwMilliseconds = c_stopGraphDelayInMs;
            }
        }

        const DWORD dwRet = WaitForSingleObject(m_event, dwMilliseconds);
        switch (dwRet)
        {
        case WAIT_OBJECT_0:
        case WAIT_TIMEOUT:
            bool hasPendingTeardown;
            bool hasPendingSound;
            bool hasPendingAudioGraph;

            {
                auto uiLock = wil::AcquireSRWLockShared(&m_uiLock);

                hasPendingTeardown = (m_uiStatus & WorkerDataUIStatus::HasPendingTeardown) == WorkerDataUIStatus::HasPendingTeardown;
                hasPendingSound = (m_uiStatus & WorkerDataUIStatus::HasPendingSound) == WorkerDataUIStatus::HasPendingSound;

                // m_currentSoundData is exclusive to the worker thread
                m_currentSoundData = m_pendingSoundData;
            }

            {
                auto audioLock = wil::AcquireSRWLockShared(&m_audioLock);

                hasPendingAudioGraph = (m_audioStatus & WorkerDataAudioStatus::HasPendingAudioGraph) == WorkerDataAudioStatus::HasPendingAudioGraph;
            }

            if (hasPendingTeardown)
            {
                TearDownAudioGraphPrivate();
                // Terminate this worker thread.

                return S_OK;
            }

            if (hasPendingSound && !hasPendingAudioGraph)
            {
                IFC_RETURN(PlayPendingSound());
            }
            else if (dwRet == WAIT_TIMEOUT && !hasPendingSound)
            {
                IFC_RETURN(StopAudioGraph());
            }
            break;
        }
    }
    while (true);

    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerServiceWorkerData::SetPendingSound(const ElementSoundPlayerService::ElementSoundData& pendingSoundData)
{
    ASSERT(m_event);

    {
        auto uiLock = wil::AcquireSRWLockExclusive(&m_uiLock);

        m_uiStatus = static_cast<WorkerDataUIStatus>(m_uiStatus | WorkerDataUIStatus::HasPendingSound);

        // This is the UI thread: save the sound data so it is available for the worker thread
        m_pendingSoundData = pendingSoundData;
    }

    IFCW32_RETURN(SetEvent(m_event));

    return S_OK;
}

// Worker-thread-only private methods

std::wstring
ElementSoundPlayerServiceWorkerData::GetPath(xaml::ElementSoundKind sound) const
{
    std::wstring windowsPath;
    auto fileName = L"";

    windowsPath.resize(MAX_PATH);
    GetWindowsDirectory(&windowsPath[0], MAX_PATH);
    PathCchAppend(&windowsPath[0], MAX_PATH, L"Media");

    if (sound == xaml::ElementSoundKind_Focus)
    {
        EnsureFocusIndexIsInRange(m_focusIndex);
        fileName = s_FocusSoundFileNames[m_focusIndex];
    }
    else
    {
        EnsureSoundIndexIsInRange(sound);
        fileName = s_SoundFileNames[sound];
    }
    PathCchAppend(&windowsPath[0], MAX_PATH, fileName);
    return windowsPath;
}

ctl::ComPtr<wm::Audio::IAudioFrameInputNode>
ElementSoundPlayerServiceWorkerData::GetAudioNodeForSound(xaml::ElementSoundKind sound)
{
    ctl::ComPtr<wm::Audio::IAudioFrameInputNode> audioInputNode;

    // Find any node that is not playing, and mark it as playing.
    for (const auto& iteratorInputNodes : m_inputNodes)
    {
        if (!iteratorInputNodes.second->isPlaying)
        {
            audioInputNode = iteratorInputNodes.second->inputNode;
            iteratorInputNodes.second->isPlaying = true;
            break;
        }
    }

    if (!audioInputNode && (UINT32) m_inputNodes.size() >= c_MaxNumberOfInputNodes)
    {
        // This should be a very rare occurrence. If we have too many nodes in the map that are
        // all still playing a sound, then reuse one of the nodes rather than creating a new one.
        // It will cause that node's sound to be truncated, but it allows us to keep memory usage under control.
        audioInputNode = m_inputNodes.begin()->second->inputNode;
    }
    return audioInputNode;
}

void
ElementSoundPlayerServiceWorkerData::CycleFocusIndex()
{
    if (m_focusIndex < c_NumberOfFocusSounds - 1)
    {
        m_focusIndex++;
    }
    else
    {
        m_focusIndex = 0;
    }
}

void
ElementSoundPlayerServiceWorkerData::TearDownAudioGraphPrivate()
{
    {
        auto uiLock = wil::AcquireSRWLockExclusive(&m_uiLock);

        m_uiStatus = WorkerDataUIStatus::UINone;
    }

    {
        auto audioLock = wil::AcquireSRWLockExclusive(&m_audioLock);

        if (m_graph)
        {
            VERIFYHR(m_graph->Stop());

            VERIFYHR(m_graph->remove_UnrecoverableErrorOccurred(m_audioGraphUnrecoverableErrorOccurredEventToken));
            m_audioGraphUnrecoverableErrorOccurredEventToken.value = 0;
            m_graph.Reset();
        }

        m_audioStatus = WorkerDataAudioStatus::AudioNone;
        m_outputNode.Reset();

        // Detach events and release input nodes.
        for (const auto& iteratorInputNodes : m_inputNodes)
        {
            VERIFYHR(iteratorInputNodes.second->inputNode->remove_AudioFrameCompleted(iteratorInputNodes.second->audioFrameCompletedEvent));
        }
        m_inputNodes.clear();
    }
      // If something is waiting for the teardown to complete, signal the event.
     if (m_teardownEvent)
     {
         SetEvent(m_teardownEvent);
     }
}

_Check_return_ HRESULT
ElementSoundPlayerServiceWorkerData::SetUpAudioGraph()
{
    auto audioLock = wil::AcquireSRWLockExclusive(&m_audioLock);

    if (!m_graph && (m_audioStatus & WorkerDataAudioStatus::HasPendingAudioGraph) == WorkerDataAudioStatus::AudioNone)
    {
        ctl::ComPtr<wm::Audio::IAudioGraphSettingsFactory> settingsFactory;

        // On NSKU all windows media DLLs are missing and this call will fail
        // When this happens we want to fail silently rather than crash
        if (SUCCEEDED(ctl::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Windows_Media_Audio_AudioGraphSettings).Get(),
            &settingsFactory)))
        {
            ctl::ComPtr<wm::Audio::IAudioGraphSettings> settings;
            ctl::ComPtr<wm::Audio::IAudioGraphSettings2> settings2;

            IFC_RETURN(settingsFactory->Create(wm::Render::AudioRenderCategory_SoundEffects, &settings));

            // MSFT: 7638787 - AudioGraph allocates huge buffers internally unless we tell it that we do not intend to change
            // playback speed, so set the MaxPlaybackSpeedFactor to 1.0 (indicating we will not change the speed). Ignore errors,
            // playback will still work if this fails, it just won't be very performant.
            if (SUCCEEDED(settings.As(&settings2)))
            {
                IFC_RETURN(settings2->put_MaxPlaybackSpeedFactor(1.0));
            }

            if (m_isSpatialAudioEnabled)
            {
                // UseSpatialAudioAPI enables ISAC instead of HRTF to render Spatial Audio
                if (XboxUtility::IsOnXbox() ||
                    DesktopUtility::IsOnDesktop())
                {
                    IFC_RETURN(AudioGraphSettingsExtension_SetUseSpatialAudioApi(settings.Get(), true));
                }
            }

            wrl::ComPtr<IWeakReference> workerDataWeakRef;

            IFC_RETURN(GetWeakReference(&workerDataWeakRef));

            auto graphCompletionCallback = wrl::Callback<wf::IAsyncOperationCompletedHandler<wm::Audio::CreateAudioGraphResult*>>
                ([workerDataWeakRef](_In_ wf::IAsyncOperation<wm::Audio::CreateAudioGraphResult*>* asyncOp, wf::AsyncStatus status)
            {
                wrl::ComPtr<ElementSoundPlayerServiceWorkerData> workerData;

                IFC_RETURN(workerDataWeakRef->Resolve<ElementSoundPlayerServiceWorkerData>(&workerData));
                if (workerData)
                {
                    IFC_RETURN(workerData->OnCreateAudioGraphCompleted(asyncOp, status));
                }
                return S_OK;
            });

            ctl::ComPtr<wm::Audio::IAudioGraphStatics> graphFactory;
            ctl::ComPtr<wf::IAsyncOperation<wm::Audio::CreateAudioGraphResult*>> graphAsyncOp;
            ctl::ComPtr<IAsyncInfo> graphAsyncInfo;

            IFC_RETURN(ctl::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Windows_Media_Audio_AudioGraph).Get(),
                &graphFactory));
            IFC_RETURN(graphFactory->CreateAsync(settings.Get(), &graphAsyncOp));
            IFC_RETURN(graphAsyncOp.As(&graphAsyncInfo));

            m_audioStatus = static_cast<WorkerDataAudioStatus>(m_audioStatus | WorkerDataAudioStatus::HasPendingAudioGraph);

            IFC_RETURN(graphAsyncOp->put_Completed(graphCompletionCallback.Get()));
        }

    }

    return S_OK;
}

void ElementSoundPlayerServiceWorkerData::EnsureWaitForThreadOnTearDown()
{
    // Create the teardown event.  This will be used in the teardown process to prevent
    // it from returning until the thread is completely shut down and all memory it is
    // using is released.
    if (!m_teardownEvent)
    {
        m_teardownEvent = CreateEvent(nullptr, false /*bManualReset*/, false /*bInitialState*/, nullptr);
    }
}

_Check_return_ HRESULT
ElementSoundPlayerServiceWorkerData::PlayPendingSound()
{
    {
        auto uiLock = wil::AcquireSRWLockExclusive(&m_uiLock);

        ASSERT((m_uiStatus & WorkerDataUIStatus::HasPendingSound) == WorkerDataUIStatus::HasPendingSound);

        m_uiStatus = static_cast<WorkerDataUIStatus>(m_uiStatus & ~WorkerDataUIStatus::HasPendingSound);
    }

    IFC_RETURN(LoadRawSound(m_currentSoundData.elementSoundKind));

    ctl::ComPtr<wm::Audio::IAudioFrameInputNode> audioInputNode = GetAudioNodeForSound(m_currentSoundData.elementSoundKind);

    if (!audioInputNode)
    {
        IFC_RETURN(CreateAudioNodeForSound(m_currentSoundData.elementSoundKind, &audioInputNode));
    }
    else if (m_isSpatialAudioEnabled)
    {
        // If this is a spatial audio sound whose AudioInputNode was cached, update
        // the AudioInputNode's Emitter position to the sound position.
        ctl::ComPtr<wm::Audio::IAudioInputNode2> audioInputNode2;
        audioInputNode.As(&audioInputNode2);

        // QI for AudioInputNode2 should never fail
        FAIL_FAST_ASSERT(audioInputNode2);

        ctl::ComPtr<wm::Audio::IAudioNodeEmitter> emitter;
        IFCFAILFAST(audioInputNode2->get_Emitter(&emitter));

        // Spatial Audio is set for this sound and emitter should always exist
        FAIL_FAST_ASSERT(emitter);

        IFC_RETURN(emitter->put_Position(m_currentSoundData.position));
    }

    // Keep this consistent with the existing implementation.  If audioInputNode is NULL
    // after LoadRawSound, return without error.
    if (audioInputNode)
    {
        IFC_RETURN(PlaySoundNode(audioInputNode.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerServiceWorkerData::StopAudioGraph()
{
    auto audioLock = wil::AcquireSRWLockExclusive(&m_audioLock);

    if (m_graph &&
        m_voiceCount == 0 &&
        (m_audioStatus & WorkerDataAudioStatus::HasAudioGraphOn) == WorkerDataAudioStatus::HasAudioGraphOn)
    {
        IFC_RETURN(m_graph->Stop());
        m_audioStatus = static_cast<WorkerDataAudioStatus>(m_audioStatus & ~WorkerDataAudioStatus::HasAudioGraphOn);
    }
    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerServiceWorkerData::LoadRawSound(xaml::ElementSoundKind sound)
{
    // Return immediately if we have already loaded the sound file
    ctl::ComPtr<wm::IAudioFrame> frame;

    if (sound == xaml::ElementSoundKind_Focus)
    {
        frame = m_focusInputFrames[m_focusIndex];
    }
    else
    {
        frame = m_inputFrames[sound];
    }

    if (frame != nullptr)
    {
        return S_OK;
    }

    wil::unique_hfile hFile(CreateFile(
        GetPath(sound).c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr));

    if (hFile.get() == INVALID_HANDLE_VALUE)
    {
        return S_OK;
    }

    DWORD fileSize = GetFileSize(hFile.get(), nullptr);

    ctl::ComPtr<wm::IAudioFrameFactory> frameFactory;

    IFC_RETURN(ctl::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Media_AudioFrame).Get(),
        &frameFactory));

    IFC_RETURN(frameFactory->Create(fileSize, &frame));

    //Get the internal buffer of the frame
    ctl::ComPtr<wm::IAudioBuffer> audioBuffer;

    IFC_RETURN(frame->LockBuffer(wm::AudioBufferAccessMode_Write, &audioBuffer));

    ctl::ComPtr<wf::IMemoryBuffer> spMemoryBuffer;
    ctl::ComPtr<wf::IMemoryBufferReference> spMemoryBufferReference;
    ctl::ComPtr<wf_::IMemoryBufferByteAccess> spMemoryBufferByteAccess;

    IFC_RETURN(audioBuffer.As(&spMemoryBuffer));
    IFC_RETURN(spMemoryBuffer->CreateReference(&spMemoryBufferReference));
    IFC_RETURN(spMemoryBufferReference.As(&spMemoryBufferByteAccess));

    BYTE* buffer = nullptr;
    UINT32 tempCap = 0;

    IFC_RETURN(spMemoryBufferByteAccess->GetBuffer(&buffer, &tempCap));
    IFCCHECK_RETURN(ReadFile(hFile.get(), buffer, fileSize, &fileSize, nullptr));
    IFC_RETURN(audioBuffer->put_Length(fileSize));

    if (sound == xaml::ElementSoundKind_Focus)
    {
        m_focusInputFrames[m_focusIndex].Attach(frame.Detach());
    }
    else
    {
        m_inputFrames[sound].Attach(frame.Detach());
    }

    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerServiceWorkerData::CreateAudioNodeForSound(xaml::ElementSoundKind sound, _COM_Outptr_ wm::Audio::IAudioFrameInputNode** audioFrameInputNodeResult)
{
    ctl::ComPtr<wm::MediaProperties::IAudioEncodingPropertiesStatics> audioEncodingPropertiesFactory;

    IFC_RETURN(ctl::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Media_MediaProperties_AudioEncodingProperties).Get(),
        &audioEncodingPropertiesFactory));

    wrl::ComPtr<wm::MediaProperties::IAudioEncodingProperties> audioEncodingProperties;

    IFC_RETURN(audioEncodingPropertiesFactory->CreatePcm(c_sampleRateInHz, c_channelCount, c_bitsPerSample, &audioEncodingProperties));

    auto audioLock = wil::AcquireSRWLockShared(&m_audioLock);

    if (!m_graph)
    {
        return S_OK;
    }

    ctl::ComPtr<wm::Audio::IAudioFrameInputNode> frameInputNode;

    // If this sound requires spatial audio, create an AudioInputNode with an Emitter
    if (m_isSpatialAudioEnabled)
    {
        ctl::ComPtr<wm::Audio::IAudioGraph2> graph2;
        IFC_RETURN(m_graph.As(&graph2));

        // Create an emitter using its default constructor : these defaults match our requirements (omni, etc.)
        ctl::ComPtr<wm::Audio::IAudioNodeEmitter> audioNodeEmitter;
        ctl::ComPtr<IInspectable> inspectable;
        IFC_RETURN(RoActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Windows_Media_Audio_AudioNodeEmitter).Get(), &inspectable));
        IFC_RETURN(inspectable.As(&audioNodeEmitter));

        // Set the emitter position to the position of the element
        IFC_RETURN(audioNodeEmitter->put_Position(m_currentSoundData.position));

        // Create the input node using the emitter : Listener is created on AudioDeviceOutputNode by default
        IFC_RETURN(graph2->CreateFrameInputNodeWithFormatAndEmitter(audioEncodingProperties.Get(), audioNodeEmitter.Get(), &frameInputNode));
    }
    else
    {
        IFC_RETURN(m_graph->CreateFrameInputNodeWithFormat(audioEncodingProperties.Get(), &frameInputNode));
    }

    ctl::ComPtr<wm::Audio::IAudioInputNode> inputNode;

    IFC_RETURN(frameInputNode.As(&inputNode));

    ASSERT(m_outputNode);

    IFC_RETURN(inputNode->AddOutgoingConnection(m_outputNode.Get()));

    wrl::ComPtr<IWeakReference> workerDataWeakRef;

    IFC_RETURN(GetWeakReference(&workerDataWeakRef));

    auto audioFrameCompletionCallback = wrl::Callback<wf::ITypedEventHandler<wm::Audio::AudioFrameInputNode*, wm::Audio::AudioFrameCompletedEventArgs*>>
        ([workerDataWeakRef](_In_ wm::Audio::IAudioFrameInputNode* sender, _In_ wm::Audio::IAudioFrameCompletedEventArgs*)
    {
        wrl::ComPtr<ElementSoundPlayerServiceWorkerData> workerData;

        IFC_RETURN(workerDataWeakRef->Resolve<ElementSoundPlayerServiceWorkerData>(&workerData));
        if (workerData)
        {
            IFC_RETURN(workerData->OnAudioFrameCompleted(sender));
        }
        return S_OK;
    });

    std::unique_ptr<AudioFrameInputNodeState> nodeState(new AudioFrameInputNodeState());

    nodeState->inputNode = frameInputNode;
    nodeState->isPlaying = true;

    IFC_RETURN(frameInputNode->add_AudioFrameCompleted(
        audioFrameCompletionCallback.Get(),
        &nodeState->audioFrameCompletedEvent));

    m_inputNodes[reinterpret_cast<DWORD_PTR>(frameInputNode.Get())] = std::move(nodeState);

    *audioFrameInputNodeResult = frameInputNode.Detach();

    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerServiceWorkerData::PlaySoundNode(_In_ wm::Audio::IAudioFrameInputNode* node)
{
    ctl::ComPtr<wm::Audio::IAudioNode> audioNode;
    ctl::ComPtr<wm::Audio::IAudioFrameInputNode> audioInputNode(node);
    ctl::ComPtr<wm::IAudioFrame> audioFrame;

    // Retrieve the AudioFrame for this sound
    if (m_currentSoundData.elementSoundKind == xaml::ElementSoundKind_Focus)
    {
        audioFrame = m_focusInputFrames[m_focusIndex];
        CycleFocusIndex();
    }
    else
    {
        audioFrame = m_inputFrames[m_currentSoundData.elementSoundKind];
    }

    {
        auto audioLock = wil::AcquireSRWLockExclusive(&m_audioLock);

        // Restart the graph if it is off
        if (m_graph && (m_audioStatus & WorkerDataAudioStatus::HasAudioGraphOn) == WorkerDataAudioStatus::AudioNone)
        {
            IFC_RETURN(m_graph->Start());
            m_audioStatus = static_cast<WorkerDataAudioStatus>(m_audioStatus | WorkerDataAudioStatus::HasAudioGraphOn);
        }

        IFC_RETURN(audioInputNode.As(&audioNode));
        IFC_RETURN(audioNode->put_OutgoingGain(m_currentSoundData.volume));
        IFC_RETURN(audioNode->Reset());
        IFC_RETURN(audioInputNode->AddFrame(audioFrame.Get()));
        IFC_RETURN(audioNode->Start());

        m_voiceCount++;
    }

    {
        auto uiLock = wil::AcquireSRWLockShared(&m_uiLock);

        // This callback is used only by test code - it is invoked by IXamlTestHooks::SetPlayingSoundNodeCallback.
        if (m_playingSoundNodeCallback)
        {
            wfn::Vector3 emitterPosition = {};
            ctl::ComPtr<wm::Audio::IAudioInputNode2> audioInputNode2;
            ctl::ComPtr<wm::Audio::IAudioNodeEmitter> emitter;
            audioInputNode.As(&audioInputNode2);
            if (audioInputNode2)
            {
                IFCFAILFAST(audioInputNode2->get_Emitter(&emitter));

                if (emitter)
                {
                    IFCFAILFAST(emitter->get_Position(&emitterPosition));
                }
            }

            m_playingSoundNodeCallback(
                    m_currentSoundData.elementSoundKind,
                    !!emitter,
                    emitterPosition.X,
                    emitterPosition.Y,
                    emitterPosition.Z,
                    m_currentSoundData.volume);
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerServiceWorkerData::OnCreateAudioGraphCompleted(_In_ wf::IAsyncOperation<wm::Audio::CreateAudioGraphResult*>* asyncOp, wf::AsyncStatus status)
{
    ctl::ComPtr<wf::IAsyncOperation<wm::Audio::CreateAudioDeviceOutputNodeResult*>> deviceOutputAsyncOp;

    {
        auto audioLock = wil::AcquireSRWLockExclusive(&m_audioLock);

        if (status == wf::AsyncStatus::Error || status == wf::AsyncStatus::Canceled)
        {
            m_audioStatus = static_cast<WorkerDataAudioStatus>(m_audioStatus & ~WorkerDataAudioStatus::HasPendingAudioGraph);
            return S_OK;
        }
        ASSERT(status == wf::AsyncStatus::Completed);

        ctl::ComPtr<wm::Audio::ICreateAudioGraphResult> createAudioGraphResult;
        IFC_RETURN(asyncOp->GetResults(&createAudioGraphResult));

        wm::Audio::AudioGraphCreationStatus audioGraphCreationStatus;
        IFC_RETURN(createAudioGraphResult->get_Status(&audioGraphCreationStatus));

        if (audioGraphCreationStatus != wm::Audio::AudioGraphCreationStatus_Success)
        {
            m_audioStatus = static_cast<WorkerDataAudioStatus>(m_audioStatus & ~WorkerDataAudioStatus::HasPendingAudioGraph);
            return S_OK;
        }

        // Get the graph that was created
        ctl::ComPtr<wm::Audio::IAudioGraph> graph;

        IFC_RETURN(createAudioGraphResult->get_Graph(&graph));

        if (!graph)
        {
            m_audioStatus = static_cast<WorkerDataAudioStatus>(m_audioStatus & ~WorkerDataAudioStatus::HasPendingAudioGraph);
            return S_OK;
        }

        m_graph = graph;

        wrl::ComPtr<IWeakReference> workerDataWeakRef;

        IFC_RETURN(GetWeakReference(&workerDataWeakRef));

        // Hook up to the UnrecoverableErrorOccurred event on the AudioGraph instance:
        auto unrecoverableErrorOccurredCallback = wrl::Callback<wf::ITypedEventHandler<wm::Audio::AudioGraph*, wm::Audio::AudioGraphUnrecoverableErrorOccurredEventArgs*>>
            ([workerDataWeakRef](_In_ wm::Audio::IAudioGraph*, _In_ wm::Audio::IAudioGraphUnrecoverableErrorOccurredEventArgs*)
        {
            wrl::ComPtr<ElementSoundPlayerServiceWorkerData> workerData;

            IFC_RETURN(workerDataWeakRef->Resolve<ElementSoundPlayerServiceWorkerData>(&workerData));
            if (workerData)
            {
                // If an unrecoverable error is raised by AudioGraph, we tear down the AudioGraph and the input/output AudioNodes
                // The next time we attempt to play a sound we will attempt to recreate them.
                workerData->TearDownAudioGraphPrivate();
            }
            return S_OK;
        });

        IFC_RETURN(graph->add_UnrecoverableErrorOccurred(
            unrecoverableErrorOccurredCallback.Get(),
            &m_audioGraphUnrecoverableErrorOccurredEventToken));

        // Create the device output node
        IFC_RETURN(graph->CreateDeviceOutputNodeAsync(&deviceOutputAsyncOp));
    }

    wrl::ComPtr<IWeakReference> workerDataWeakRef;

    IFC_RETURN(GetWeakReference(&workerDataWeakRef));

    auto deviceOutputNodeCompletionCallback = wrl::Callback<wf::IAsyncOperationCompletedHandler<wm::Audio::CreateAudioDeviceOutputNodeResult*>>
        ([workerDataWeakRef](_In_ wf::IAsyncOperation<wm::Audio::CreateAudioDeviceOutputNodeResult*>* asyncOp, wf::AsyncStatus status)
    {
        wrl::ComPtr<ElementSoundPlayerServiceWorkerData> workerData;

        IFC_RETURN(workerDataWeakRef->Resolve<ElementSoundPlayerServiceWorkerData>(&workerData));
        if (workerData)
        {
            IFC_RETURN(workerData->OnCreateDeviceOutputNodeCompleted(asyncOp, status));
        }
        return S_OK;
    });

    IFC_RETURN(deviceOutputAsyncOp->put_Completed(deviceOutputNodeCompletionCallback.Get()));

    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerServiceWorkerData::OnCreateDeviceOutputNodeCompleted(_In_ wf::IAsyncOperation<wm::Audio::CreateAudioDeviceOutputNodeResult*>* asyncOp, wf::AsyncStatus status)
{
    auto audioLock = wil::AcquireSRWLockExclusive(&m_audioLock);

    if (!m_graph)
    {
        return S_OK;
    }

    if (status == wf::AsyncStatus::Error || status == wf::AsyncStatus::Canceled)
    {
        m_audioStatus = static_cast<WorkerDataAudioStatus>(m_audioStatus & ~WorkerDataAudioStatus::HasPendingAudioGraph);
        return S_OK;
    }
    ASSERT(status == wf::AsyncStatus::Completed);

    ctl::ComPtr<wm::Audio::ICreateAudioDeviceOutputNodeResult> createDeviceOutputResult;
    IFC_RETURN(asyncOp->GetResults(&createDeviceOutputResult));

    wm::Audio::AudioDeviceNodeCreationStatus audioDeviceNodeCreationStatus;
    IFC_RETURN(createDeviceOutputResult->get_Status(&audioDeviceNodeCreationStatus));

    if (audioDeviceNodeCreationStatus != wm::Audio::AudioDeviceNodeCreationStatus_Success)
    {
        m_audioStatus = static_cast<WorkerDataAudioStatus>(m_audioStatus & ~WorkerDataAudioStatus::HasPendingAudioGraph);
        return S_OK;
    }

    ctl::ComPtr<wm::Audio::IAudioDeviceOutputNode> deviceOutputNode;
    IFC_RETURN(createDeviceOutputResult->get_DeviceOutputNode(&deviceOutputNode));
    IFC_RETURN(deviceOutputNode.As(&m_outputNode));

    IFC_RETURN(m_graph->ResetAllNodes());
    m_audioStatus = static_cast<WorkerDataAudioStatus>(m_audioStatus & ~WorkerDataAudioStatus::HasPendingAudioGraph);
    IFC_RETURN(OnGraphSetupCompleted());

    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerServiceWorkerData::OnGraphSetupCompleted()
{
    ASSERT(m_event);

    // Waking up the worker thread to play the potential pending sound.
    IFCW32_RETURN(SetEvent(m_event));

    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerServiceWorkerData::OnAudioFrameCompleted(_In_ wm::Audio::IAudioFrameInputNode* sender)
{
    bool setEvent = false;

    {
        auto audioLock = wil::AcquireSRWLockExclusive(&m_audioLock);

        // Mark this node as no longer playing
        auto pair = m_inputNodes.find(reinterpret_cast<DWORD_PTR>(sender));
        if (pair != m_inputNodes.end())
        {
            pair->second->isPlaying = false;
        }

        if (m_voiceCount > 0)
        {
            m_voiceCount--;
        }
        setEvent = m_voiceCount == 0;
    }

    if (setEvent)
    {
        ASSERT(m_event);

        // Waking up the worker thread to start the 2 seconds countdown before stopping graph.
        IFCW32_RETURN(SetEvent(m_event));
    }
    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerServiceWorkerData::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** object)
{
    if (InlineIsEqualGUID(iid, __uuidof(ElementSoundPlayerServiceWorkerData)))
    {
        *object = static_cast<ElementSoundPlayerServiceWorkerData*>(this);
    }
    else
    {
        return __super::QueryInterfaceImpl(iid, object);
    }

    AddRefOuter();
    return S_OK;
}
