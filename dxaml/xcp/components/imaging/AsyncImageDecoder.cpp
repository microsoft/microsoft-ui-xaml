// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AsyncImageDecoder.h"
#include "ThreadPoolService.h"
#include "EncodedImageData.h"
#include "ImageDecodeParams.h"
#include "AsyncDecodeResponse.h"
#include "OfferableSoftwareBitmap.h"
#include "imaginginterfaces.h"
#include "ImagingUtility.h"
#include <MUX-ETWEvents.h>
#include <Clock.h>

/*
    AsyncImageDecoder creates background thread to make calls to IImageDecoder.
    IImageDecodeCallback must be released in UI thread so background threads
    cannot hold a strong reference to it. IImageDecodeCallback is owned by
    SharedState and explicitly released in AsyncImageDecoder destructor.
    Background threads are allowed to run past AsyncImageDecoder destruction
    but they will early terminate when checking IImageDecodeCallback for nullptr.

    AsyncImageDecoder immediately starts decoding of the first frame upon
    construction. The first frame is presented immediately. After that when in
    Play mode every next frame present is sheduled according to the current frame
    delay. Present and decode are called from different threads. If on present
    decoding has not finished yet then m_missedPresent flag is set to indicate
    that when decoding is done the frame should be presented immediately.

    On Stop the last decoded frame is discarded; decoding of the frame #0
    starts which is then presented instantly.

    Background decoding thread and timer thread capture the shared state mutex.
    All AsyncImageDecoder public functions are called from the UI thread so
    they capture the shared state mutex too.
*/

std::atomic<bool> AsyncImageDecoder::s_suspendOffThreadDecoding = false;

AsyncImageDecoder::AsyncImageDecoder(
    std::unique_ptr<IImageDecoder> spImageDecodingContext,
    _In_ std::shared_ptr<EncodedImageData> spEncodedImageData,
    bool isAutoPlay,
    _In_ xref::weakref_ptr<IImageDecodeCallback> imageDecodeCallback
    )
    : m_spSharedState(std::make_shared<SharedState>())
{
    // Store the input parameters since they will be used to decode future frames
    m_spSharedState->m_spImageDecoder = std::move(spImageDecodingContext);
    m_spSharedState->m_spEncodedImageData = std::move(spEncodedImageData);
    m_spSharedState->m_imageDecodeCallback = std::move(imageDecodeCallback);

    m_spSharedState->m_stopped = !isAutoPlay;
    m_spSharedState->m_nextDesiredPresentTime = Clock::now();
}

AsyncImageDecoder::~AsyncImageDecoder()
{
    // Called from UI thread so need to lock shared state first
    std::lock_guard<std::mutex> lock(m_spSharedState->m_mutex);

    // Background threads may hold SharedState for some time.
    // If an asynchronous call is scheduled on IImageDecodeCallback it may
    // be completed after destruction of the AsyncImageDecoder

    m_spSharedState->CancelPresent();
}

bool AsyncImageDecoder::SharedState::NeedMoreFrames() const
{
    auto nextFrame = m_currentFrame + 1;
    auto nextLoop = (nextFrame == m_spEncodedImageData->GetMetadata().frameCount) ? m_currentLoop + 1 : m_currentLoop;

    // We are playing when:
    //  - not explicitly stopped or suspended
    //  - the image is animated
    //  - have more loops to play
    bool atLastLoop = nextLoop == m_spEncodedImageData->GetMetadata().loopCount &&
                      m_spEncodedImageData->GetMetadata().loopCount != 0; // loop infinitely when 0
    bool hasManyFrames = m_spEncodedImageData->IsAnimatedImage();
    return !m_suspended && !m_stopped && hasManyFrames && !atLastLoop && m_spDecodeParams != nullptr;
}

void AsyncImageDecoder::SharedState::CancelPresent()
{
    m_needsPresentAfterDecode = false;

    if (m_spPresentTimer != nullptr)
    {
        // Even though we Cancel the handler may have already started executing
        IFCFAILFAST(m_spPresentTimer->Cancel());
        m_spPresentTimer.Reset();
    }
}

// static
HRESULT AsyncImageDecoder::SetupDecodeCurrentFrame(std::shared_ptr<SharedState> sharedState)
{
    uint64_t imageSourceId = sharedState->m_spDecodeParams->GetImageId();
    ImagingTelemetry::QueueOffThreadDecode(imageSourceId);

    ASSERT(!sharedState->m_decodeInProgress);
    sharedState->m_decodeInProgress = true;

    // Capture strong reference to shared state so it is not destroyed during callback run
    auto asyncJob = wrl::Callback<FreeThreaded<wsyt::IWorkItemHandler>>(
        [sharedState = std::move(sharedState)](_In_opt_ wf::IAsyncAction*)
    {
        std::lock_guard<std::mutex> lock(sharedState->m_mutex);
        return OnDecodeCurrentFrame(sharedState);
    });

    // TODO: Potentially use the spAsyncAction for cancellation
    wrl::ComPtr<wf::IAsyncAction> spAsyncAction;
    IFC_RETURN(ThreadPoolService::GetInstance().GetThreadPoolFactory()->RunAsync(asyncJob.Get(), &spAsyncAction));

    return S_OK;
}

// static
HRESULT AsyncImageDecoder::OnDecodeCurrentFrame(std::shared_ptr<SharedState> sharedState)
{
    using namespace std::chrono;

    milliseconds nextFrameDelayMS;
    // Note: Despite the "DecodeFrame" name, this is a call that creates the tree of WIC objects that can decode a
    // frame. The actual WIC objects decode lazily, and no decoding actually happens until we call CopyPixels to read
    // the pixels out. That happens during presenting.
    sharedState->m_nextDecodingResult = sharedState->m_spImageDecoder->DecodeFrame(
        *sharedState->m_spEncodedImageData,
        *sharedState->m_spDecodeParams,
        sharedState->m_currentFrame,
        sharedState->m_spNextBitmapSource,
        nextFrameDelayMS);

    // Insert an artificial delay to ensure rendering no more than ~60fps for gif with very small or 0 delay
    auto one60th = duration_cast<Clock::duration>(duration<Clock::rep, std::ratio<1, 60>>(1));
    sharedState->m_nextFrameDelay = std::max(duration_cast<Clock::duration>(nextFrameDelayMS), one60th);

    sharedState->m_decodeInProgress = false;

    if (sharedState->m_needsPresentAfterDecode)
    {
        sharedState->m_needsPresentAfterDecode = false;
        sharedState->m_nextDesiredPresentTime = Clock::now();
        IFC_RETURN(PresentAndProceedToNextFrame(std::move(sharedState)));
    }
    else
    {
        sharedState->m_hasDecodingResult = true;
    }

    return S_OK;
}

// static
HRESULT AsyncImageDecoder::ScheduleNextPresent(std::shared_ptr<SharedState> sharedState)
{
    ASSERT(sharedState->m_spPresentTimer == nullptr);
    ASSERT(!sharedState->m_needsPresentAfterDecode);

    // Capture strong reference to shared state so it is not destroyed during callback run
    auto timerCallback = wrl::Callback<FreeThreaded<wsyt::ITimerElapsedHandler>>(
        [sharedState](_In_opt_ wsyt::IThreadPoolTimer *timer)
    {
        std::lock_guard<std::mutex> lock(sharedState->m_mutex);
        // The timer may have been canceled and then possibly recreated
        // Make sure that only the recent timer instance gets executed
        if (sharedState->m_spPresentTimer.Get() == timer)
        {
            sharedState->m_spPresentTimer.Reset();

            if (sharedState->m_hasDecodingResult)
            {
                sharedState->m_hasDecodingResult = false;
                IFC_RETURN(PresentAndProceedToNextFrame(sharedState));
            }
            else
            {
                sharedState->m_needsPresentAfterDecode = true;
                TraceAsyncImageDecoderFrameNotReadyInfo();
            }
        }
        return S_OK;
    });

    // When resumed from suspended state the desired present time may appear in the past.
    // Then shift it to the present to avoid the "fast forward" animation glitch.
    auto now = Clock::now();
    sharedState->m_nextDesiredPresentTime = std::max(sharedState->m_nextDesiredPresentTime, now);
    auto adjustedDelay = sharedState->m_nextDesiredPresentTime - now;
    wf::TimeSpan timeSpan{ std::chrono::duration_cast<TimeSpanDuration>(adjustedDelay).count() };

    IFC_RETURN(ThreadPoolService::GetInstance().GetThreadPoolTimerFactory()->CreateTimer(
        timerCallback.Get(), timeSpan, &sharedState->m_spPresentTimer));

    return S_OK;
}

// static
HRESULT AsyncImageDecoder::PresentAndProceedToNextFrame(std::shared_ptr<SharedState> sharedState)
{
    if (auto imageDecodeCallback = sharedState->m_imageDecodeCallback.lock())
    {
        // Realize the bitmap source into the software or hardware surfaces.
        // Note that for static image case (not an animated GIF) the real decoding is happening here!
        // For animated GIF this may do some less expensive post processing such as scaling.
        xref_ptr<OfferableSoftwareBitmap> spSoftwareBitmap;
        if (SUCCEEDED(sharedState->m_nextDecodingResult))
        {
            sharedState->m_nextDecodingResult = ImagingUtility::RealizeBitmapSource(
                sharedState->m_spEncodedImageData->GetMetadata(),
                sharedState->m_spNextBitmapSource.Get(),
                *sharedState->m_spDecodeParams,
                spSoftwareBitmap);
            sharedState->m_spNextBitmapSource.Reset();
        }

        // TODO: separate SurfaceUpdateList from ImageDecodeParams and manage its lifetime separately.
        // Now this is necessary because some objects like ImageCache/DecodedImageCache cache the DecodeParams
        // so we release the surface update list here so that DecodeParams no longer holds onto the ref count.
        auto surfaceUpdateListToReleaseOnUIThread = sharedState->m_spEncodedImageData->IsAnimatedImage() ?
            SurfaceUpdateList() : sharedState->m_spDecodeParams->DetachSurfaceUpdateList();

        // Dispatch the UI thread notification.
        // Note that AsyncDecodeResponse must always be moved all the way up to the UI thread so that
        // it's never shared between threads.
        IFC_RETURN(imageDecodeCallback->OnDecode(
            make_xref<AsyncDecodeResponse>(
                sharedState->m_nextDecodingResult,
                std::move(spSoftwareBitmap),
                std::move(surfaceUpdateListToReleaseOnUIThread)),
            sharedState->m_requestId));

        // If error occurred during frame decode treat it as the end of stream without looping
        if (sharedState->NeedMoreFrames() && SUCCEEDED(sharedState->m_nextDecodingResult))
        {
            sharedState->m_nextDesiredPresentTime += sharedState->m_nextFrameDelay;
            sharedState->m_currentFrame++;
            if (sharedState->m_currentFrame == sharedState->m_spEncodedImageData->GetMetadata().frameCount)
            {
                sharedState->m_currentFrame = 0;
                sharedState->m_currentLoop++;
                ASSERT(sharedState->m_spEncodedImageData->GetMetadata().loopCount == 0 ||
                    sharedState->m_currentLoop < sharedState->m_spEncodedImageData->GetMetadata().loopCount);
            }

            IFC_RETURN(SetupDecodeCurrentFrame(sharedState));
            IFC_RETURN(ScheduleNextPresent(sharedState));
        }
        else if (sharedState->m_spEncodedImageData->IsAnimatedImage())
        {
            TraceImageAnimationEndInfo();
        }
    }
    return S_OK;
}

// static
HRESULT AsyncImageDecoder::RestartPlayback(std::shared_ptr<SharedState> sharedState)
{
    if (sharedState->NeedMoreFrames())
    {
        sharedState->m_nextDesiredPresentTime = Clock::now() + sharedState->m_nextFrameDelay;
        sharedState->m_needsPresentAfterDecode = false;

        if (!sharedState->m_decodeInProgress && !sharedState->m_hasDecodingResult)
        {
            IFC_RETURN(SetupDecodeCurrentFrame(sharedState));
        }
        IFC_RETURN(ScheduleNextPresent(sharedState));
    }

    return S_OK;
}

// NOTE: AutoPlay is supposed to kick in when we open a new URI/Stream but not when we resize
// or recover from device lost. SetDecodeParams is (not yet) involved in all these scenarios
// and we'll likely have to do something here when we enable them.
HRESULT AsyncImageDecoder::SetDecodeParams(
    _In_ xref_ptr<ImageDecodeParams> decodeParams,
    _In_ uint64_t requestId)
{
    // Called from UI thread so need to lock shared state first
    std::lock_guard<std::mutex> lock(m_spSharedState->m_mutex);

    ASSERT(decodeParams != nullptr);
    m_spSharedState->m_spDecodeParams = std::move(decodeParams);
    m_spSharedState->m_requestId = requestId;

    // Cancel any pending present and present immediately upon decode.
    // Discard any existing decode results including the error condition.
    m_spSharedState->CancelPresent();
    m_spSharedState->m_needsPresentAfterDecode = true;
    m_spSharedState->m_hasDecodingResult = false;

    // When stopped force re-decode the first frame.
    // Otherwise proceed with the next frame in line.
    if (m_spSharedState->m_stopped)
    {
        m_spSharedState->m_currentFrame = 0;
        m_spSharedState->m_currentLoop = 0;
    }

    if (!m_spSharedState->m_decodeInProgress)
    {
        IFC_RETURN(SetupDecodeCurrentFrame(m_spSharedState));
    }

    return S_OK;
}

HRESULT AsyncImageDecoder::PlayAnimation()
{
    // Called from UI thread so need to lock shared state first
    std::lock_guard<std::mutex> lock(m_spSharedState->m_mutex);

    if (m_spSharedState->m_stopped)
    {
        m_spSharedState->m_stopped = false;
        IFC_RETURN(RestartPlayback(m_spSharedState));
    }

    return S_OK;
}

HRESULT AsyncImageDecoder::StopAnimation()
{
    // Called from UI thread so need to lock shared state first
    std::lock_guard<std::mutex> lock(m_spSharedState->m_mutex);

    m_spSharedState->CancelPresent();

    m_spSharedState->m_stopped = true;
    m_spSharedState->m_currentLoop = 0;

    // Go back to frame #0
    if (m_spSharedState->m_currentFrame != 0)
    {
        // Discard existing frame if any and re-decode the frame 0
        m_spSharedState->m_hasDecodingResult = false;
        m_spSharedState->m_currentFrame = 0;
    }

    if (m_spSharedState->m_decodeInProgress)
    {
        ASSERT(!m_spSharedState->m_hasDecodingResult);
        m_spSharedState->m_needsPresentAfterDecode = true;
    }
    else if (m_spSharedState->m_hasDecodingResult)
    {
        ASSERT(!m_spSharedState->m_decodeInProgress);
        m_spSharedState->m_hasDecodingResult = false;
        IFC_RETURN(PresentAndProceedToNextFrame(m_spSharedState));
    }
    else
    {
        m_spSharedState->m_needsPresentAfterDecode = true;
        IFC_RETURN(SetupDecodeCurrentFrame(m_spSharedState));
    }

    return S_OK;
}

void AsyncImageDecoder::SuspendAnimation()
{
    // Called from UI thread so need to lock shared state first
    std::lock_guard<std::mutex> lock(m_spSharedState->m_mutex);

    m_spSharedState->CancelPresent();
    m_spSharedState->m_suspended = true;
}

HRESULT AsyncImageDecoder::ResumeAnimation()
{
    // Called from UI thread so need to lock shared state first
    std::lock_guard<std::mutex> lock(m_spSharedState->m_mutex);

    if (m_spSharedState->m_suspended)
    {
        m_spSharedState->m_suspended = false;
        IFC_RETURN(RestartPlayback(m_spSharedState));
    }

    return S_OK;
}

void AsyncImageDecoder::CleanupDeviceRelatedResources()
{
    // Called from UI thread so need to lock shared state first
    std::lock_guard<std::mutex> lock(m_spSharedState->m_mutex);

    if (m_spSharedState->m_spDecodeParams != nullptr)
    {
        m_spSharedState->m_spDecodeParams->DetachSurfaceUpdateList();
    }
    m_spSharedState->CancelPresent();
    m_spSharedState->m_suspended = true;
}

bool AsyncImageDecoder::IsDecodeInProgress()
{
    // Called from UI thread so need to lock shared state first
    std::lock_guard<std::mutex> lock(m_spSharedState->m_mutex);

    return m_spSharedState->m_decodeInProgress;
}

/* static */ void AsyncImageDecoder::SetSuspendOffThreadDecoding(bool isOffThreadDecodingSuspended)
{
    s_suspendOffThreadDecoding = isOffThreadDecodingSuspended;
}

/* static */ bool AsyncImageDecoder::IsOffThreadDecodingSuspended()
{
    return s_suspendOffThreadDecoding;
}
