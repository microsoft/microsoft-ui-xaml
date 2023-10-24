// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <TraceLoggingInterop.h>

// GUID for "Microsoft-Windows-Xaml": {531a35ab-63ce-4bcf-aa98-f88c7a89e455}
DECLARE_TRACELOGGING_CLASS(ImagingTelemetryLogging, "Microsoft-Windows-XAML", (0x531a35ab, 0x63ce, 0x4bcf, 0xaa, 0x98, 0xf8, 0x8c, 0x7a, 0x89, 0xe4, 0x55));

class ImagingTelemetry final : public TelemetryBase
{
    IMPLEMENT_TELEMETRY_CLASS(ImagingTelemetry, ImagingTelemetryLogging);

public:

    // Start of Uri decoding - called when the source Uri is set
    DEFINE_TRACELOGGING_EVENT_PARAM2(SetUriSource_noactivity,
        uint64_t, Id,
        PCWSTR, Uri,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));


    BEGIN_COMPLIANT_TRACELOGGING_ACTIVITY_CLASS_WITH_LEVEL(ImageDecodeActivity, PDT_ProductAndServicePerformance, WINEVENT_LEVEL_VERBOSE)

        // Note: events are identified by "Id", which is the pointer to the object. This can be either an ImageSource or a LoadedImageSurface.
        DEFINE_ACTIVITY_START(XUINT64 Id)
        {
            TraceLoggingClassWriteStart(ImageDecodeActivity, TraceLoggingValue(Id));
        }

        // Note: The casing of the params matters. WPA has a bug where the Trace Logging table sorts its columns in
        // alphabetical order, with upper case before lower case. We want "Id" to be the first field for all these
        // events so it can be used to group all these events, so name it starting with an upper case "Id". Uri should
        // come next, so it's named "Uri". All other fields start with a lower case letter.
        // See https://perftoolkit.visualstudio.com/DefaultCollection/WPA/_workitems/edit/2438

        // Start of Uri decoding - called when the source Uri is set
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM2(SetUriSource,
            uint64_t, Id,
            PCWSTR, Uri,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        // Start of stream decoding - called when the source stream is set (SetSource/SetSourceAsync)
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM1(SetStreamSource,
            uint64_t, Id,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        // Start of stream decoding - called when the SoftwareBitmap is set on a SoftwareBitmapSource
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM1(SetSoftwareBitmap,
            uint64_t, Id,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        // Start of LoadedImageSurface decoding - called when a LoadedImageSurface loads a Uri
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM2(SetLoadedImageSurfaceUri,
            uint64_t, Id,
            PCWSTR, Uri,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        // Start of LoadedImageSurface decoding - called when a LoadedImageSurface initializes from memory
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM1(SetLoadedImageSurfaceMemory,
            uint64_t, Id,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        // When ImageCache queues a UI thread callback to kick off the off-thread download.
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM2(QueueProcessDownload,
            uint64_t, Id,
            PCWSTR, Uri,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        // When an ImageSource finds an ImageCache that has a download already in progress, it attaches itself and waits
        // for the existing download to complete.
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM1(WaitForDownloadInProgress,
            uint64_t, Id,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        // When an ImageSource finds an ImageCache that has already completed the download, the ImageSource can read the
        // metadata immediately and proceed with triggering a decode.
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM1(FoundCompletedDownload,
            uint64_t, Id,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        // When we start parsing an encoded image's metadata
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM1(ParseImageMetadataStart,
            uint64_t, Id,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        // When we stop parsing an encoded image's metadata
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM2(ParseImageMetadataStop,
            uint64_t, Id,
            HRESULT, parseHR,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        // When an image download has completed, or we found previously cached bits so we don't need to download
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM3(ImageDownloadCompleteNotification,
            uint64_t, Id,
            PCWSTR, Uri,
            bool, decodeToRenderSize,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        // When we create a new ImageCache with encoded bits
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM2(CreateImageCache,
            uint64_t, Id,
            PCWSTR, Uri,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        // When we find an existing ImageCache with encoded bits
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM2(FoundImageCache,
            uint64_t, Id,
            PCWSTR, Uri,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        // When we found cached encoded bits and created a new ImageCache from them
        // This use case is a bit unintuitive. ImageSource will create an ImageCache to start downloading, then when the
        // download completes it creates a _second_ ImageCache to kick off the decode. The second ImageCache picks up
        // the existing cached encoded bits. Why not just use the ImageCache we already have?
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM2(CreateImageCacheFromExistingEncodedData,
            uint64_t, Id,
            PCWSTR, Uri,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        // A request for a decode to render size has come in.
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM5(RequestDecodeToRenderSize,
            uint64_t, Id,
            PCSTR, imageState,
            uint32_t, requestedWidth,
            uint32_t, requestedHeight,
            bool, decodeToRenderSize,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        // The ImageSource has been disqualified for decoding to render size.
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM2(DecodeToRenderSizeDisqualified,
            uint64_t, Id,
            PCSTR, reason,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        // When a decode to render size starts.
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM7(DecodeToRenderSizeStart,
            uint64_t, Id,
            uint32_t, imageSourceWidth,
            uint32_t, imageSourceHeight,
            uint32_t, newWidth,
            uint32_t, newHeight,
            uint32_t, metadataWidth,
            uint32_t, metadataHeight,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        // When a decode to dender size stops.
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM1(DecodeToRenderSizeStop,
            uint64_t, Id,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        // The ImageCache posts a work item back to the UI thread to queue decode requests.
        // Note that there are two levels of indirection here. This queues the ProcessDecodeRequests, and once that
        // is processed it may queue the actual decode work off-thread.
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM2(QueueProcessDecodeRequests,
            uint64_t, Id,
            PCWSTR, Uri,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        // The ImageCache running the ProcessDecodeRequests that was queued by QueueProcessDecodeRequests.
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM3(ProcessDecodeRequests,
            uint64_t, Id,
            PCWSTR, Uri,
            uint32_t, State,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        // The ImageCache kicks off a decode once it's finished downloading
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM2(QueueDecodeFromImageCache,
            uint64_t, Id,
            PCWSTR, Uri,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        // The point where Xaml queues off-thread work that does the decode. This should follow immediately from QueueDecodeFromImageCache.
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM1(QueueOffThreadDecode,
            uint64_t, Id,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        // The work running off-thread to decode the image. This work was queued by QueueOffThreadDecode.
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM3(OffThreadDecodeStart,
            uint64_t, Id,
            PCWSTR, Uri,
            bool, isHardwareDecode,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        // The work running off-thread to decode the image. This work was queued by QueueOffThreadDecode.
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM1(OffThreadDecodeStop,
            uint64_t, Id,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        // When a direct SoftwareBitmap upload to hardware failed, so we fall back to uploading it to a software surface
        // instead. We'll try to upload the software surface to hardware later, with automatic retries later if that
        // encounters another failure.
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM2(SoftwareBitmapFallbackAfterUploadError,
            uint64_t, Id,
            uint32_t, uploadResult,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        // TODO: Events for decoding on the UI thread

        // Decode completed notification received on UI thread
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM2(DecodeResultAvailable,
            uint64_t, Id,
            uint32_t, decodeResult,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    END_ACTIVITY_CLASS();
};
