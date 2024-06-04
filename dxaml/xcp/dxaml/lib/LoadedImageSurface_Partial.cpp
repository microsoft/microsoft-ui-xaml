// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "LoadedImageSurface_Partial.h"
#include "LoadedImageSurface.g.h"
#include "LoadedImageSurface.h"
#include "DCompSurface.h"
#include "BufferMemoryProxy.h"

#include <microsoft.ui.composition.private.h>

using namespace DirectUI;

_Check_return_ HRESULT LoadedImageSurface::Close()
{
    IFC_RETURN(CheckThread());

    if (m_streamReadOperation != nullptr)
    {
        Microsoft::WRL::ComPtr<IAsyncInfo> asyncInfo;
        IFC_RETURN(m_streamReadOperation.As(&asyncInfo));
        IFC_RETURN(asyncInfo->Cancel());
    }

    IFC_RETURN(static_cast<CLoadedImageSurface*>(GetHandle())->Close());

    return S_OK;
}

_Check_return_ HRESULT LoadedImageSurface::GetRealSurface(_Outptr_ WUComp::ICompositionSurface** value)
{
    *value = nullptr;

    xref_ptr<DCompSurface> dcompSurface;
    IFC_RETURN(static_cast<CLoadedImageSurface*>(GetHandle())->GetCompositionSurface(dcompSurface.ReleaseAndGetAddressOf()));

    SetInterface(*value, dcompSurface->GetWinRTSurface());

    return S_OK;
}

static _Check_return_ HRESULT GetCoreUri(_In_ wf::IUriRuntimeClass* uri, _Out_ xstring_ptr *strUri)
{
    wrl_wrappers::HString tempAbsoluteUri;
    IFC_RETURN(uri->get_AbsoluteUri(tempAbsoluteUri.GetAddressOf()));
    IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(tempAbsoluteUri, strUri));
    return S_OK;
}

_Check_return_ HRESULT LoadedImageSurface::InitFromUri(_In_ wf::IUriRuntimeClass* uri, _In_ wf::Size size)
{
    xstring_ptr strUri;
    IFC_RETURN(GetCoreUri(uri, &strUri));

    auto coreLoadedImageSurface = static_cast<CLoadedImageSurface*>(GetHandle());
    coreLoadedImageSurface->SetDesiredSize(size.Width, size.Height);
    IFC_RETURN(coreLoadedImageSurface->InitFromUri(std::move(strUri)));

    return S_OK;
}

_Check_return_ HRESULT LoadedImageSurface::InitFromStream(_In_ wsts::IRandomAccessStream* stream, _In_ wf::Size size)
{
    auto coreLoadedImageSurface = static_cast<CLoadedImageSurface*>(GetHandle());
    coreLoadedImageSurface->SetDesiredSize(size.Width, size.Height);

    Microsoft::WRL::ComPtr<wsts::IInputStream> inputStream;

    // Return an invalid argument error if someone gives us a stream we can't read.
    HRESULT hr = stream->QueryInterface(IID_PPV_ARGS(&inputStream));
    IFC_RETURN(hr == E_NOTIMPL ? E_INVALIDARG : hr);

    uint64_t streamSize = 0;
    IFC_RETURN(stream->get_Size(&streamSize));
    if (streamSize > UINT32_MAX)
    {
        // IRandomAccessStream's Size property returns a 64-bit value, but its
        // raw stream data is exposed as an IBuffer with a 32-bit length.
        IFC_RETURN(E_INVALIDARG);
    }

    Microsoft::WRL::ComPtr<wsts::IBuffer> buffer;
    ctl::ComPtr<wsts::IBufferFactory> bufferFactory;
    
    IFC_RETURN(GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Storage_Streams_Buffer).Get(),
        &bufferFactory));
    IFC_RETURN(bufferFactory->Create(static_cast<UINT>(streamSize), &buffer));

    IFC_RETURN(inputStream->ReadAsync(
        buffer.Get(),
        static_cast<UINT>(streamSize),
        wsts::InputStreamOptions_None,
        &m_streamReadOperation));

    ctl::ComPtr<wsts::IStreamReadCompletedEventHandler> callback;
    callback.Attach(new ClassMemberCallback2<LoadedImageSurface,
                                             ILoadedImageSurface,
                                             wsts::IStreamReadCompletedEventHandler,
                                             wsts::IStreamReadOperation,
                                             wf::AsyncStatus>(this, &LoadedImageSurface::OnStreamReadCompleted));
    IFC_RETURN(m_streamReadOperation->put_Completed(callback.Get()));

    return S_OK;
}

_Check_return_ HRESULT LoadedImageSurface::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(WUComp::ICompositionSurfaceFacade)))
    {
        *ppObject = static_cast<WUComp::ICompositionSurfaceFacade*>(this);
    }
    else
    {
        return LoadedImageSurfaceGenerated::QueryInterfaceImpl(iid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

_Check_return_ HRESULT LoadedImageSurface::OnStreamReadCompleted(_In_ wsts::IStreamReadOperation *sender, _In_ wf::AsyncStatus /*status*/)
{
    IFCFAILFAST(CheckThread());

    Microsoft::WRL::ComPtr<wsts::IBuffer> buffer;
    {
        // http://osgvsowi/6854651 it is possible to have invalid stream or stream closed.
        SuspendFailFastOnStowedException suspender;
        IFC_RETURN(sender->GetResults(&buffer));
    }

    if (buffer != nullptr)
    {
        auto bufferMemoryProxy = wil::make_unique_failfast<BufferMemoryProxy>();
        IFC_RETURN(bufferMemoryProxy->SetBuffer(buffer));

        auto coreLoadedImageSurface = static_cast<CLoadedImageSurface*>(GetHandle());
        IFC_RETURN(coreLoadedImageSurface->InitFromMemory(std::move(bufferMemoryProxy)));
    }

    return S_OK;
}


// xaml_media::ILoadedImageSurfaceStatics

_Check_return_ HRESULT LoadedImageSurfaceFactory::StartLoadFromUriWithSizeImpl(
    _In_ wf::IUriRuntimeClass* pUri,
    _In_ wf::Size desiredMaximumSize,
    _Outptr_ xaml_media::ILoadedImageSurface** ppReturnValue)
{
    ctl::ComPtr<LoadedImageSurface> loadedImageSurface;
    ctl::make<LoadedImageSurface>(&loadedImageSurface);

    IFC_RETURN(loadedImageSurface->InitFromUri(pUri, desiredMaximumSize));

    *ppReturnValue = loadedImageSurface.Detach();
    return S_OK;
}

_Check_return_ HRESULT LoadedImageSurfaceFactory::StartLoadFromUriImpl(
    _In_ wf::IUriRuntimeClass* pUri,
    _Outptr_ xaml_media::ILoadedImageSurface** ppReturnValue)
{
    return StartLoadFromUriWithSizeImpl(pUri, {0, 0}, ppReturnValue);
}

_Check_return_ HRESULT LoadedImageSurfaceFactory::StartLoadFromStreamWithSizeImpl(
    _In_ wsts::IRandomAccessStream* pStream,
    _In_ wf::Size desiredMaximumSize,
    _Outptr_ xaml_media::ILoadedImageSurface** ppReturnValue)
{
    ctl::ComPtr<LoadedImageSurface> loadedImageSurface;
    ctl::make<LoadedImageSurface>(&loadedImageSurface);

    IFC_RETURN(loadedImageSurface->InitFromStream(pStream, desiredMaximumSize));

    *ppReturnValue = loadedImageSurface.Detach();
    return S_OK;
}

_Check_return_ HRESULT LoadedImageSurfaceFactory::StartLoadFromStreamImpl(
    _In_ wsts::IRandomAccessStream* pStream,
    _Outptr_ xaml_media::ILoadedImageSurface** ppReturnValue)
{
    return StartLoadFromStreamWithSizeImpl(pStream, {0, 0}, ppReturnValue);
}
