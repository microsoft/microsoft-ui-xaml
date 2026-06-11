// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "BitmapSource_Partial.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

ULONG BitmapSource::z_ulUniqueAsyncActionId = 1;

//------------------------------------------------------------------------
//
//  Synopsis:
//      Constructs a BitmapSource.
//
//------------------------------------------------------------------------

BitmapSource::BitmapSource()
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Destructs the BitmapSource and releases its buffer.
//
//------------------------------------------------------------------------

BitmapSource::~BitmapSource()
{
    // If we are destroyed and there is an outstanding operation, cancel it
    if (m_spStreamReadOperationAsyncInfo != NULL)
        m_spStreamReadOperationAsyncInfo->Cancel();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handles the completed event of an asynchronous stream read.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
BitmapSource::OnStreamReadCompleted(_In_ wsts::IStreamReadOperation *pOperation, _In_ wf::AsyncStatus /*status*/)
{
    HRESULT hr = S_OK;
    wsts::IBuffer *pBuffer = NULL;
    ::Windows::Storage::Streams::IBufferByteAccess *pBufferInternal = NULL;
    Microsoft::WRL::ComPtr<BitmapSourceSetSourceAsyncAction> spSetSourceAsyncAction;
    bool bContinue = true;
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
        if (m_spSetSourceAsyncAction != nullptr)
        {
            // Check if the action has already been cancelled
            bContinue = m_spSetSourceAsyncAction->CoreContinueAsyncAction();
        }
        if (bContinue)
        {
            {
                // http://osgvsowi/6854651 it is possible to have invalid stream or stream closed.
                SuspendFailFastOnStowedException suspender;

                IFC(pOperation->GetResults(&pBuffer));
            }

            if (pBuffer != NULL)
            {
                XBYTE* pPixels = NULL;
                XUINT32 count = 0;

                // Get the byte pointer and size from the buffer.
                IFC(pBuffer->get_Length(&count));
                IFC(pBuffer->QueryInterface(__uuidof(::Windows::Storage::Streams::IBufferByteAccess), reinterpret_cast<void**>(&pBufferInternal)));
                IFC(pBufferInternal->Buffer(&pPixels));

                // Call into core to set the source to the new image.
                // CopyTo adds a ref to the action, which will be kept in the Core
                spSetSourceAsyncAction = m_spSetSourceAsyncAction;
                IFC(static_cast<CImageSource*>(GetHandle())->SetSource(count, pPixels, m_spSetSourceAsyncAction.Get()));
                spSetSourceAsyncAction.Detach();  // If the call succeeded we handle off this reference to the Core
            }
            else
            {
                Trace(L"SetSource trying to read NULL buffer");
                IFCCATASTROPHIC(FALSE);
            }
        }
    }

Cleanup:
    // If we have an Async action and there was an error *or* the action
    // was cancelled, we need to FireCompletion now
    if ((m_spSetSourceAsyncAction != nullptr) &&
        (FAILED(hr) || !bContinue))
    {
        m_spSetSourceAsyncAction->CoreSetError(hr);  //CoreSetError does nothing if hr==S_OK
        m_spSetSourceAsyncAction->CoreFireCompletion();
        m_spSetSourceAsyncAction = nullptr;
    }
    if (pOperation == m_spStreamReadOperation.Get())
    {
        // If the operation we're holding completed, clear the pointers
        m_spStreamReadOperation = nullptr;
        m_spStreamReadOperationAsyncInfo = nullptr;

        // Signal completion
        IFC(m_Event.Set());
    }
    ReleaseInterface(pBuffer);
    ReleaseInterface(pBufferInternal);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
// Factored to make adding a SetSourceAsync method easy
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
BitmapSource::SetSourceShared(_In_ wsts::IRandomAccessStream* pStreamSource)
{
    HRESULT hr = S_OK;
    wsts::IInputStream *pInputStream = NULL;
    ctl::ComPtr<wsts::IBuffer> buffer;
    ctl::ComPtr<wsts::IBufferFactory> bufferFactory;
    ctl::ComPtr<wsts::IStreamReadCompletedEventHandler> spCompletedHandler;
    ctl::ComPtr<IUnknown> spOperationReference;
    XUINT64 size = 0;
    UINT32 count = 0;

    IFCPTR(pStreamSource);
    IFC(CheckThread());

    // Return an invalid argument error if someone gives us a stream we can't read.
    hr = pStreamSource->QueryInterface(__uuidof(wsts::IInputStream), reinterpret_cast<void**>(&pInputStream));
    IFC((hr == E_NOTIMPL || pInputStream == NULL) ? E_INVALIDARG : hr);

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

    IFC(pInputStream->ReadAsync(
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
            BitmapSource,
            IBitmapSource,
            wsts::IStreamReadCompletedEventHandler,
            wsts::IStreamReadOperation,
            wf::AsyncStatus>(this, &BitmapSource::OnStreamReadCompleted));

    IFC(m_spStreamReadOperation->put_Completed(spCompletedHandler.Get()));

Cleanup:
    if (FAILED(hr))
    {
        m_spStream = nullptr;
    }
    ReleaseInterface(pInputStream);
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the source of this BitmapSource from a stream.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT BitmapSource::SetSourceImpl(_In_ wsts::IRandomAccessStream* pStreamSource)
{
    auto guard = wil::scope_exit([this]()
    {
        m_Event.Close();
    });

    IFC_RETURN(CheckThread());
    CancelSetSourceAsyncActions();

    m_Event.Close();

    IFC_RETURN(m_Event.Start());
    IFC_RETURN(SetSourceShared(pStreamSource));
    IFC_RETURN(m_Event.Wait());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the source of this BitmapSource from a stream.  Supports  the WinRT async pattern
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
BitmapSource::SetSourceAsyncImpl(_In_ wsts::IRandomAccessStream* pStreamSource,
        _Outptr_ wf::IAsyncAction** returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DirectUI::IDispatcher> spDispatcher;
    ULONG actionId = 0;
    IFC(CheckThread());
    CancelSetSourceAsyncActions();
    IFC(GetXamlDispatcher(&spDispatcher));
    actionId = ::InterlockedIncrement(&BitmapSource::z_ulUniqueAsyncActionId);
    IFC(Microsoft::WRL::MakeAndInitialize<BitmapSourceSetSourceAsyncAction>(&m_spSetSourceAsyncAction, actionId, spDispatcher.Get()));

    IFC(m_spSetSourceAsyncAction->StartOperation());

    // 19H1 Bug #18511523:  SetSourceShared may silently fail due to app code swallowing errors.  The flow looks like this:
    // -SetSourceShared calls ReadAsync on the underlying stream, which can go into "foreign" code
    // -Foreign code runs and finishes reading the stream
    // -OnStreamReadCompleted is called.  This may fail for various reasons, causing us to cleanup and release m_spSetSourceAsyncAction.
    // The problem with this flow is that the error flows back through foreign code which may swallow the error and cause
    // this cdoe to run without seeing any error, and subsequently crash accessing a null m_spSetSourceAsyncAction.
    // The fix is to copy the pointer to the out parameter before calling SetSourceShared().  If an error occurs in OnStreamReadCompleted(),
    // we'll mark the action as failed, which the app can then examine to handle the error.
    IFC(m_spSetSourceAsyncAction.CopyTo(returnValue));
    IFC(SetSourceShared(pStreamSource));

Cleanup:
    RRETURN(hr);

}

//------------------------------------------------------------------------
//
//  Synopsis:
//      If the app calls SetSource/SetSourceAsync again we should immediately cancel any outstanding
// actions.
//
//------------------------------------------------------------------------

void
BitmapSource::CancelSetSourceAsyncActions()
{
    // If we are already holding an operation, clear it
    if (m_spStreamReadOperationAsyncInfo != nullptr)
    {
        m_spStreamReadOperationAsyncInfo->Cancel();
        m_spStreamReadOperation = nullptr;
        m_spStreamReadOperationAsyncInfo = nullptr;
    }
    if (m_spSetSourceAsyncAction != nullptr)
    {
        m_spSetSourceAsyncAction->Cancel();
        m_spSetSourceAsyncAction->CoreFireCompletion();
        m_spSetSourceAsyncAction = nullptr;
    }
    m_spStream = nullptr;
}

_Check_return_ HRESULT BitmapSource::SetSourceAsync(_In_ CBitmapSource* pNative, _In_ wsts::IRandomAccessStream* pStreamSource,
                                                     _Outptr_ wf::IAsyncAction** ppReturnValue)
{
    ctl::ComPtr<DependencyObject> target;
    IFCFAILFAST(DXamlCore::GetCurrent()->GetPeer(pNative, &target));
    return target.Cast<BitmapSource>()->SetSourceAsyncImpl(pStreamSource, ppReturnValue);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Cleans up the event
//
//------------------------------------------------------------------------
EventWrapper::~EventWrapper()
{
    Close();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Call this before starting the async operation
//
//------------------------------------------------------------------------
HRESULT
EventWrapper::Start()
{
    HRESULT hr = S_OK;
    m_hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_hEvent == NULL)
    {
        IFC(HRESULT_FROM_WIN32(GetLastError()));
        IFCCATASTROPHIC(FALSE); // If CreateEvent failed, we should have gotten an HRESULT from GetLastError
    }
Cleanup:

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Call this when the operation completes
//
//------------------------------------------------------------------------
HRESULT
EventWrapper::Set()
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES

    // m_hEvent may be null if nobody has called Start and therefore
    // nobody is waiting.  In this case we do nothing.  This makes it easier
    // to share the code between the sync case where we use the event and
    // the async case where we don't
    if (m_hEvent != NULL)
    {
        IFCCATASTROPHIC(::SetEvent(m_hEvent));
    }
Cleanup:

    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     Wait
//
//------------------------------------------------------------------------
HRESULT
EventWrapper::Wait()
{
    IFCCATASTROPHIC_RETURN(m_hEvent != NULL);
    DWORD dwIndex = 0;
    IFC_RETURN(::CoWaitForMultipleHandles(COWAIT_DISPATCH_CALLS,
                        INFINITE,
                        1,  // cHandles
                        &m_hEvent,// handle array of size 1
                        &dwIndex));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      All done, clean up resources
//
//------------------------------------------------------------------------
void
EventWrapper::Close()
{
    if (m_hEvent != NULL)
    {
        ::CloseHandle(m_hEvent);
    }
    m_hEvent = NULL;
}

