// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "BitmapSource_Partial.h"
#include "SvgImageSource_Partial.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// TODO: There is some commonality and differences with OnStreamReadCompleted and SetSourceShared between SVGImageSource_Partial.cpp and BitmapSource_Partial.cpp
//                 It is tricky to reconcile these differences at the moment but would be nice to reconcile them and create a common function in the future.

ULONG SvgImageSource::z_ulUniqueAsyncActionId = 1;

SvgImageSource::~SvgImageSource()
{
    // If we are destroyed and there is an outstanding operation, cancel it
    if (m_spStreamReadOperationAsyncInfo != nullptr)
    {
        m_spStreamReadOperationAsyncInfo->Cancel();
    }
}

// Handles the completed event of an asynchronous stream read.
_Check_return_ HRESULT
SvgImageSource::OnStreamReadCompleted(_In_ wsts::IStreamReadOperation *pOperation, _In_ wf::AsyncStatus /*status*/)
{
    HRESULT hr = S_OK;
    bool continueOperation = true;
    IFCPTR(pOperation);
    IFC(CheckThread());

    // We only care about finishing the SetSource operation for the operation
    // used by the most recent stream read (i.e. the one referenced by m_spStreamReadOperation)
    // If these pointers aren't equal then that means someone has called SetSource
    // again before the first operation has completed.
    if (pOperation == m_spStreamReadOperation.Get())
    {
        // The operation completed, safe to deref the stream
        m_spStream = nullptr;
        if (m_spSetSourceAsyncOperation != nullptr)
        {
            // Check if the action has already been cancelled
            continueOperation = m_spSetSourceAsyncOperation->CoreContinueAsyncAction();
        }
        if (continueOperation)
        {
            ctl::ComPtr<wsts::IBuffer> buffer;
            ctl::ComPtr<::Windows::Storage::Streams::IBufferByteAccess> bufferInternal;
            {
                // http://osgvsowi/6854651 it is possible to have invalid stream or stream closed.
                SuspendFailFastOnStowedException suspender;

                IFC(pOperation->GetResults(&buffer));
            }

            if (buffer != nullptr)
            {
                // Get the byte pointer and size from the buffer.
                uint32_t count = 0;
                IFC(buffer->get_Length(&count));
                IFC(buffer.As(&bufferInternal));

                uint8_t* pixels = nullptr;
                IFC(bufferInternal->Buffer(&pixels));

                // Call into core to set the source to the new image.
                // CopyTo adds a ref to the action, which will be kept in the Core
                wrl::ComPtr<SvgImageSourceSetSourceAsyncOperation> setSourceAsyncOperation = m_spSetSourceAsyncOperation;

                auto imageSource = static_cast<CImageSource*>(GetHandle());
                IFC(imageSource->SetSource(count, pixels, m_spSetSourceAsyncOperation.Get()));
                setSourceAsyncOperation.Detach();  // If the call succeeded we handle off this reference to the Core
            }
            else
            {
                Trace(L"SetSource trying to read nullptr buffer");
                IFCCATASTROPHIC(FALSE);
            }
        }
    }

Cleanup:
    // If we have an Async action and there was an error *or* the action
    // was cancelled, we need to FireCompletion now
    if ((m_spSetSourceAsyncOperation != nullptr) &&
        (FAILED(hr) || !continueOperation))
    {
        m_spSetSourceAsyncOperation->CoreSetError(hr);  //CoreSetError does nothing if hr==S_OK
        m_spSetSourceAsyncOperation->CoreFireCompletion();
        m_spSetSourceAsyncOperation = nullptr;
    }
    if (pOperation == m_spStreamReadOperation.Get())
    {
        // If the operation we're holding completed, clear the pointers
        m_spStreamReadOperation = nullptr;
        m_spStreamReadOperationAsyncInfo = nullptr;
    }
    return hr;
}

_Check_return_ HRESULT
SvgImageSource::SetSourceShared(_In_ wsts::IRandomAccessStream* pStreamSource)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wsts::IInputStream> inputStream;
    ctl::ComPtr<wsts::IBuffer> buffer;
    ctl::ComPtr<wsts::IBufferFactory> bufferFactory;
    ctl::ComPtr<wsts::IStreamReadCompletedEventHandler> spCompletedHandler;
    ctl::ComPtr<IUnknown> spOperationReference;
    XUINT64 size = 0;
    UINT32 count = 0;

    IFCPTR(pStreamSource);
    IFC(CheckThread());

    // Return an invalid argument error if someone gives us a stream we can't read.
    hr = do_query_interface(inputStream, pStreamSource);
    IFC((hr == E_NOTIMPL || inputStream == nullptr) ? E_INVALIDARG : hr);

    IFC(pStreamSource->get_Size(&size));
    if (size > UINT32_MAX)
    {
        // IRandomAccessStream's Size property returns a 64-bit value, but its
        // raw stream data is exposed as an IBuffer with a 32-bit length...
        // This shouldn't ever happen, but return an error if the stream is too big
        // for its own buffer. A 32-bit length is big enough for a 4gb image.
        IFC(E_INVALIDARG);
    }
    count = static_cast<UINT32>(size);

    // CBuffer is the standard WinRT implementation of IBuffer and IBufferByteAccess.
    IFC(GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Storage_Streams_Buffer).Get(),
        &bufferFactory));
    IFC(bufferFactory->Create(count, &buffer));

    // ref the stream to make sure it doesn't go away during the operation
    m_spStream = pStreamSource;

    IFC(inputStream->ReadAsync(
        buffer.Get(),
        count,
        wsts::InputStreamOptions_None,
        &m_spStreamReadOperation));
    // Reading...
    // (note: must load m_spStreamReadOperationAsyncInfo before calling put_Completed)
    IFC(m_spStreamReadOperation.As(&m_spStreamReadOperationAsyncInfo));
    // NOTE:  The async operation may complete during put_Completed and our completion handler (OnSetSourceCompleted above)
    // clears out m_spStreamReadOperation.  This additional reference makes sure the operation isn't deleted until
    // after put_Completed has returned
    IFC(m_spStreamReadOperation.As(&spOperationReference));

    // Hook up to the stream read operation's Completed event.
    spCompletedHandler.Attach(new ClassMemberCallback2<
            SvgImageSource,
            ISvgImageSource,
            wsts::IStreamReadCompletedEventHandler,
            wsts::IStreamReadOperation,
            wf::AsyncStatus>(this, &SvgImageSource::OnStreamReadCompleted));

    IFC(m_spStreamReadOperation->put_Completed(spCompletedHandler.Get()));

Cleanup:
    if (FAILED(hr))
    {
        m_spStream = nullptr;
    }
    RRETURN(hr);
}

// Sets the source of this SvgImageSource from a stream.  Supports  the WinRT async pattern
_Check_return_ HRESULT
SvgImageSource::SetSourceAsyncImpl(_In_ wsts::IRandomAccessStream* pStreamSource,
        _Outptr_ wf::IAsyncOperation<xaml_imaging::SvgImageSourceLoadStatus>** returnValue)
{
    IFC_RETURN(CheckThread());

    CancelSetSourceAsyncActions();

    ctl::ComPtr<DirectUI::IDispatcher> spDispatcher;
    IFC_RETURN(GetXamlDispatcher(&spDispatcher));

    auto actionId = ::InterlockedIncrement(&SvgImageSource::z_ulUniqueAsyncActionId);
    IFC_RETURN(Microsoft::WRL::MakeAndInitialize<SvgImageSourceSetSourceAsyncOperation>(&m_spSetSourceAsyncOperation, actionId, spDispatcher.Get()));

    IFC_RETURN(m_spSetSourceAsyncOperation->StartOperation());
    IFC_RETURN(SetSourceShared(pStreamSource));

    IFC_RETURN(m_spSetSourceAsyncOperation.CopyTo(returnValue));

    return S_OK;

}

void
SvgImageSource::CancelSetSourceAsyncActions()
{
    // If we are already holding an operation, clear it
    if (m_spStreamReadOperationAsyncInfo != nullptr)
    {
        m_spStreamReadOperationAsyncInfo->Cancel();
        m_spStreamReadOperation = nullptr;
        m_spStreamReadOperationAsyncInfo = nullptr;
    }
    if (m_spSetSourceAsyncOperation != nullptr)
    {
        m_spSetSourceAsyncOperation->Cancel();
        m_spSetSourceAsyncOperation->CoreFireCompletion();
        m_spSetSourceAsyncOperation = nullptr;
    }
    m_spStream = nullptr;
}

_Check_return_ HRESULT SvgImageSourceFactory::CreateInstanceWithUriSourceImpl(
    _In_ wf::IUriRuntimeClass* pUri,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ ISvgImageSource** ppInstance)
{
    ctl::ComPtr<SvgImageSource> spInstance;
    ctl::ComPtr<IInspectable> spInner;

    IFCEXPECT_RETURN(pOuter == nullptr || ppInner != nullptr);
    IFCPTR_RETURN(pUri);

    IFC_RETURN(CheckActivationAllowed());

    IFC_RETURN(ctl::BetterAggregableCoreObjectActivationFactory::ActivateInstance(pOuter, &spInner));
    IFC_RETURN(spInner.As(&spInstance));
    IFC_RETURN(spInstance->put_UriSource(pUri));

    if (ppInner)
    {
        *ppInner = spInner.Detach();
    }

    *ppInstance = spInstance.Detach();

    return S_OK;
}

void SvgImageSourceSetSourceAsyncOperation::CoreSetError(HRESULT hr)
{
    CoreSetErrorImpl(hr);
    switch(hr)
    {
        case AG_E_NETWORK_ERROR:
        {
            m_status = xaml_imaging::SvgImageSourceLoadStatus_NetworkError;
            break;
        }
        case E_INVALIDARG:
        {
            m_status = xaml_imaging::SvgImageSourceLoadStatus_InvalidFormat;
            break;
        }
        default:
        {
            m_status = xaml_imaging::SvgImageSourceLoadStatus_Other;
            break;
        }
    }
}

SvgImageSourceSetSourceAsyncOperation::SvgImageSourceSetSourceAsyncOperation()
{
    m_status = xaml_imaging::SvgImageSourceLoadStatus_Success;
}

