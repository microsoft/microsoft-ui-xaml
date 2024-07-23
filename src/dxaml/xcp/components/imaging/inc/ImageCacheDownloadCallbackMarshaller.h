// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <palnetwork.h>

class ImageCache;
class ImageTaskDispatcher;
struct IPALStream;
struct IPALDownloadResponseCallback;

// TODO: Currently ImageProviderDecoderHandlerTask and ImageCacheDownloadCallbackMarshaller handle marshalling back to the UI thread
//                 These could be improved and streamlined (along with PAL stuff removed) in the future.  Ideally via std::function queueing on
//                 ImageTaskDispatcher and the data members could be captured by lambda or bind instead of declaring new functors that inherit from
//                 IImageTask with custom data.
class ImageCacheDownloadCallbackMarshaller final
    : public CXcpObjectBase< IPALDownloadResponseCallback >
{
public:
    ImageCacheDownloadCallbackMarshaller(
        _In_ ImageTaskDispatcher* dispatcher,
        _In_ ImageCache* imageCache);

    ~ImageCacheDownloadCallbackMarshaller() override;

    _Check_return_ HRESULT GotResponse(
        _In_ xref_ptr<IPALDownloadResponse> response,
        HRESULT status
        ) override;

    _Check_return_ HRESULT GotData(
        uint64_t size,
        uint64_t totalSize
        ) override;

private:

    // The ImageTaskDispatcher lives for the length of the process and ImageCache destruction will abort any downloads
    // prior to this object being destroy when XAML closes.
    ImageTaskDispatcher* m_dispatcherNoRef;

    // This does not take a reference on image cache to prevent a reference cycle.  Additionally, it
    // is guaranteed to not callback because ImageCache will call Abort on the downloader in the case of ImageCache
    // destruction which should prevent it from firing any further callbacks.  Abort is thread-safe and should
    // work nicely.  This avoids a reference cycle and supports early abort on downloads if the image is no longer
    // necessary.
    xref::weakref_ptr<ImageCache> m_imageCache;

    DWORD m_creatorThreadId = 0;
};
