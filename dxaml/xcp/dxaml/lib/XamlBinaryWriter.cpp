// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "xamlbinarywriter.h"
#include "XBFXamlTypeWrapper.h"

using namespace DirectUI;

HRESULT XamlBinaryWriterFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(xaml_markup::IXamlBinaryWriterStatics)))
    {
        *ppObject = static_cast<xaml_markup::IXamlBinaryWriterStatics*>(this);
    }
    else
    {
        RRETURN(ctl::AbstractActivationFactory::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


IFACEMETHODIMP XamlBinaryWriterFactory::Write(_In_ wfc::IVector<wsts::IRandomAccessStream*>* inputStreams, _In_ wfc::IVector<wsts::IRandomAccessStream*>* outputStreams, _In_ xaml_markup::IXamlMetadataProvider* xamlMetadataProvider, _Out_ xaml_markup::XamlBinaryWriterErrorInformation* returnValue)
{
    HRESULT hr = S_OK;

    ARG_VALIDRETURNPOINTER(returnValue);
    ARG_NOTNULL(inputStreams, "inputStreams");
    ARG_NOTNULL(outputStreams, "outputStreams");
    ARG_NOTNULL(xamlMetadataProvider, "xamlMetadataProvider");
    IFC(WriteImpl(inputStreams, outputStreams, xamlMetadataProvider, returnValue));
Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT XamlBinaryWriterFactory::WriteImpl(
    _In_ wfc::IVector<wsts::IRandomAccessStream*>* pInputStreams,
    _In_ wfc::IVector<wsts::IRandomAccessStream*>* pOutputStreams,
    _In_ xaml_markup::IXamlMetadataProvider* pProvider,
    _Out_ xaml_markup::XamlBinaryWriterErrorInformation* returnValue)
{
    HRESULT  hr                = S_OK;
    XBYTE   *pXamlTextBuffer   = NULL;
    XBYTE   *pXamlBinaryBuffer = NULL;
    XUINT32 cInputStreams      = 0;
    XUINT32 cOutputStreams     = 0;
    wsts::IRandomAccessStream* pInputRandomAccessStream = NULL;
    wsts::IRandomAccessStream* pOutputRandomAccessStream = NULL;
    wsts::IInputStream  *pInputStream = NULL;
    wsts::IOutputStream *pOutputStream = NULL;
    xaml_markup::IXamlMetadataProvider *pWrapperMetadataProvider = NULL;
    bool fNewCoreInitialized = false;

    // Initialize and override the Xaml Metadata Provider
    if (DXamlCore::GetCurrent() == nullptr)
    {
        IFC(DXamlCore::Initialize(InitializationType::Xbf));
        fNewCoreInitialized = TRUE;
    }

    IFC(XBFXamlMetadataProviderWrapper::CreateXBFXamlMetadataProviderWrapper(pProvider, &pWrapperMetadataProvider));
    MetadataAPI::OverrideMetadataProvider(pWrapperMetadataProvider);

    IFC(pInputStreams->get_Size(&cInputStreams));
    IFC(pOutputStreams->get_Size(&cOutputStreams));
    IFCEXPECT(cInputStreams == cOutputStreams);

    if (returnValue)
    {
        returnValue->InputStreamIndex = 0;
        returnValue->LineNumber = 0;
        returnValue->LinePosition = 0;
    }

    // Loop through the input streams, read the buffer, process the buffer into binary and write it out.
    for (XUINT32 i = 0; i < cInputStreams ; i++)
    {
        ULONGLONG  cXamlTextBufferSize      = 0;
        XUINT32    cXamlTextBufferBytesRead = 0;
        XUINT32    cXamlBinaryBufferSize    = 0;

        // Get the Xaml Text Buffer from the Input Stream
        IFC(pInputStreams->GetAt(i, &pInputRandomAccessStream));
        IFCPTR(pInputRandomAccessStream);
        IFC(pInputRandomAccessStream->get_Size(&cXamlTextBufferSize));
        IFC(pInputRandomAccessStream->GetInputStreamAt(0, &pInputStream));
        if (pInputStream)
        {
            ASSERT(cXamlTextBufferSize < XUINT32_MAX);

            // Read the Input Stream into a Byte Buffer
            IFC(ReadBytesFromInputStream(
                pInputStream,
                static_cast<XUINT32>(cXamlTextBufferSize),
                &cXamlTextBufferBytesRead,
                &pXamlTextBuffer));
            ASSERT( cXamlTextBufferBytesRead == static_cast<XUINT32>(cXamlTextBufferSize) );

            // Call the parser to generate the Binary Xaml
            if (SUCCEEDED(CoreImports::Parser_GenerateBinaryXaml(
                    DXamlCore::GetCurrent()->GetHandle(),
                    pXamlTextBuffer,
                    cXamlTextBufferBytesRead,
                    &pXamlBinaryBuffer,
                    &cXamlBinaryBufferSize)))
            {
                // Write the generated Binary Buffer into the Output Stream
                IFC(pOutputStreams->GetAt(i, &pOutputRandomAccessStream));
                IFCPTR(pOutputRandomAccessStream);
                IFC(pOutputRandomAccessStream->GetOutputStreamAt(0, &pOutputStream));
                if (pOutputStream)
                {
                    IFC(pOutputRandomAccessStream->put_Size(cXamlBinaryBufferSize));
                    IFC(WriteBytesToOutputStream(pOutputStream, cXamlBinaryBufferSize, pXamlBinaryBuffer));
                }
            }
            else
            {
                if (returnValue)
                {
                    IErrorService *pErrorService = NULL;
                    IError *pError = NULL;

                    IFC(DXamlCore::GetCurrent()->GetHandle()->getErrorService(&pErrorService));

                    if (pErrorService)
                    {
                        IFC(pErrorService->GetLastReportedError(&pError));

                        returnValue->InputStreamIndex = i;

                        if (pError)
                        {
                            returnValue->LineNumber = pError->GetLineNumber();
                            returnValue->LinePosition = pError->GetCharPosition();
                        }
                        else
                        {
                            // we don't have sufficient line information, so just report beginning of file.
                            returnValue->LineNumber = 1;
                            returnValue->LinePosition = 1;
                        }
                    }
                }
                TRACE(TraceAlways, L"XBFPARSER: Failed to convert to binary format for xaml file #(%d).", i);
                goto Cleanup;
            }
        }

        if (pXamlTextBuffer)
        {
            delete []pXamlTextBuffer;
            pXamlTextBuffer = NULL;
        }
        ReleaseInterface(pInputStream);
        ReleaseInterface(pInputRandomAccessStream);

        if (pXamlBinaryBuffer)
        {
            delete []pXamlBinaryBuffer;
            pXamlBinaryBuffer = NULL;
        }
        ReleaseInterface(pOutputStream);
        ReleaseInterface(pOutputRandomAccessStream);
    }

Cleanup:
    if (pXamlTextBuffer)
    {
        delete []pXamlTextBuffer;
        pXamlTextBuffer = NULL;
    }
    ReleaseInterface(pInputStream);
    ReleaseInterface(pInputRandomAccessStream);

    if (pXamlBinaryBuffer)
    {
        delete []pXamlBinaryBuffer;
        pXamlBinaryBuffer = NULL;
    }
    ReleaseInterface(pOutputStream);
    ReleaseInterface(pOutputRandomAccessStream);
    ReleaseInterface(pWrapperMetadataProvider);

    MetadataAPI::OverrideMetadataProvider(nullptr);
    if (fNewCoreInitialized)
    {
        IGNOREHR(DXamlCore::Deinitialize());
        DirectUI::MetadataAPI::Reset();
    }

    RRETURN(hr);
}

_Check_return_
HRESULT XamlBinaryWriterFactory::WriteBytesToOutputStream(
    _In_                     wsts::IOutputStream *pOutputStream,
    _In_                     XUINT32                                   cBufferSize,
    _In_reads_bytes_(cBufferSize) XBYTE                                    *pBuffer)
{
    HRESULT hr            = S_OK;
    XUINT32 cBytesWritten = 0;
    Microsoft::WRL::ComPtr<wsts::IDataWriter> spDataWriter;
    Microsoft::WRL::ComPtr<wsts::IDataWriterFactory> spDataWriterFactory;
    Microsoft::WRL::ComPtr<wf::IAsyncOperation<UINT32>> spStoreOperation;
    Microsoft::WRL::ComPtr<wsts::IOutputStream> spTempStream;

    IFCPTR(pOutputStream);
    IFCPTR(pBuffer);

    // Get DataWriter instance
    IFC(wf::GetActivationFactory(wrl_wrappers::HStringReference(
            RuntimeClass_Windows_Storage_Streams_DataWriter).Get(),
            &spDataWriterFactory));

    // Use DataWriter to write the buffer to the Output Stream
    IFC(spDataWriterFactory->CreateDataWriter(pOutputStream, &spDataWriter));
    IFC(spDataWriter->WriteBytes(cBufferSize, pBuffer));
    IFC(spDataWriter->StoreAsync(&spStoreOperation));

    // Wait for completion
    IFC(wil::wait_for_completion_nothrow(spStoreOperation.Get(), &cBytesWritten));
    
    if (cBufferSize != cBytesWritten)
    {
        IFC(E_UNEXPECTED);
    }

    // detach the stream from the data writer so that it does not close it automatically
    spDataWriter->DetachStream(&spTempStream);

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT XamlBinaryWriterFactory::ReadBytesFromInputStream(
    _In_                                 wsts::IInputStream  *pInputStream,
    _In_                                 XUINT32                                   cBufferSize,
    _Out_                                XUINT32                                  *pcBytesRead,
    _Outptr_result_bytebuffer_((*pcBytesRead))   XBYTE                                   **ppBuffer)
{
    HRESULT  hr         = S_OK;
    XUINT32  cBytesRead = 0;
    XBYTE   *pBuffer    = NULL;
    Microsoft::WRL::ComPtr<wsts::IDataReaderFactory> spDataReaderFactory;
    Microsoft::WRL::ComPtr<wsts::IDataReader> spDataReader;
    Microsoft::WRL::ComPtr<wf::IAsyncOperation<UINT32>> spLoadOperation;
    Microsoft::WRL::ComPtr<wsts::IInputStream> spTempStream;

    IFCPTR(pInputStream);
    IFCPTR(pcBytesRead);
    IFCPTR(ppBuffer);

    *pcBytesRead = 0;
    *ppBuffer    = NULL;

    // Get Data Reader instance
    IFC(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Windows_Storage_Streams_DataReader).Get(),
            &spDataReaderFactory));

    // Use Data Reader to read the buffer from the Input Stream
    IFC(spDataReaderFactory->CreateDataReader(pInputStream, &spDataReader));
    IFC(spDataReader->LoadAsync(cBufferSize, &spLoadOperation));
    IFC(wil::wait_for_completion_nothrow(spLoadOperation.Get(), &cBytesRead));

    if (cBufferSize >= cBytesRead)
    {
        pBuffer = new XBYTE[cBytesRead];
        IFC(spDataReader->ReadBytes(cBytesRead, pBuffer));
    }
    else
    {
        IFC(E_UNEXPECTED);
    }

    // detach the stream from the data reader so that it does not close it automatically
    spDataReader->DetachStream(&spTempStream);

    // transfer the results
    *ppBuffer = pBuffer;
    pBuffer = NULL;
    *pcBytesRead = cBytesRead;

Cleanup:
    if (pBuffer)
    {
        delete []pBuffer;
        pBuffer = NULL;
    }

    RRETURN(hr);
}

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_XamlBinaryWriter()
    {
        RRETURN(ctl::ActivationFactoryCreator<DirectUI::XamlBinaryWriterFactory>::CreateActivationFactory());
    }
}
