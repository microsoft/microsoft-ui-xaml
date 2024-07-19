// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ImageMetadata.h"
#include <Clock.h>
#include <NamespaceAliases.h>
#include <xref_ptr.h>
#include <weakref_ptr.h>
#include <windows.system.threading.h>
#include <memory>
#include <mutex>

class ImageDecodeParams;
class EncodedImageData;
struct IImageDecodeCallback;
struct IImageDecoder;

class AsyncImageDecoder
{
public:
    // Callback will be called from background thread.
    // Callback is released together with AsyncImageDecoder instance.
    AsyncImageDecoder(
        std::unique_ptr<IImageDecoder> spImageDecodingContext,
        _In_ std::shared_ptr<EncodedImageData> spEncodedImageData,
        bool isAutoPlay,
        _In_ xref::weakref_ptr<IImageDecodeCallback> imageDecodeCallback
        );
    ~AsyncImageDecoder();

    HRESULT SetDecodeParams(
        _In_ xref_ptr<ImageDecodeParams> decodeParams,
        _In_ uint64_t requestId);

    HRESULT PlayAnimation();
    HRESULT StopAnimation();

    void SuspendAnimation();
    HRESULT ResumeAnimation();

    void CleanupDeviceRelatedResources();

    bool IsDecodeInProgress();

    static void SetSuspendOffThreadDecoding(bool isOffThreadDecodingSuspended);
    static bool IsOffThreadDecodingSuspended();

private:
    using Clock = Jupiter::HighResolutionClock;

    struct SharedState
    {
        std::mutex m_mutex;

        bool m_suspended = false;
        bool m_stopped = false;
        bool m_hasDecodingResult = false;
        bool m_needsPresentAfterDecode = true;

        // TODO: Currently this can't be checked without taking the m_mutex lock, which blocks the UI thread. Consider
        // changing this to std::atomic so the UI thread can read it without taking the lock.
        bool m_decodeInProgress = false;

        bool NeedMoreFrames() const;
        void CancelPresent();

        int m_currentFrame = 0; // Currently decoding(ed) but not yet presented
        uint32_t m_currentLoop = 0;
        HRESULT m_nextDecodingResult = S_OK;
        Clock::duration m_nextFrameDelay = Clock::duration::zero();
        Clock::time_point m_nextDesiredPresentTime;

        std::shared_ptr<EncodedImageData> m_spEncodedImageData;
        xref_ptr<ImageDecodeParams> m_spDecodeParams;
        xref::weakref_ptr<IImageDecodeCallback> m_imageDecodeCallback;
        uint64_t m_requestId;

        std::unique_ptr<IImageDecoder> m_spImageDecoder;
        wrl::ComPtr<wsyt::IThreadPoolTimer> m_spPresentTimer;
        wrl::ComPtr<IWICBitmapSource> m_spNextBitmapSource;
    };

    std::shared_ptr<SharedState> m_spSharedState;

    static _Check_return_ HRESULT SetupDecodeCurrentFrame(std::shared_ptr<SharedState> sharedState);
    static _Check_return_ HRESULT OnDecodeCurrentFrame(std::shared_ptr<SharedState> sharedState);
    static _Check_return_ HRESULT ScheduleNextPresent(std::shared_ptr<SharedState> sharedState);
    static _Check_return_ HRESULT PresentAndProceedToNextFrame(std::shared_ptr<SharedState> sharedState);
    static _Check_return_ HRESULT RestartPlayback(std::shared_ptr<SharedState> sharedState);

    // Test-only flag that pauses off-thread image decoding.
    // Note: Use with caution. Off-thread image decoding takes a lock on m_spSharedState.m_mutex, which the UI thread also
    // tries to take in some circumstances. The test will deadlock if the decoding thread is holding the mutex and waiting
    // on this flag, while the UI thread is waiting for the mutex to set this flag.
    static std::atomic<bool> s_suspendOffThreadDecoding;
};
