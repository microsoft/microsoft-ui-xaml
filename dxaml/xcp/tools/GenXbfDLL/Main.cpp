// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "xcptypes.h"
#include "DumpHelper.h"
#include <ParserAPI.h>
#include <XamlBinaryFileAccessFactories.h>
#include <XamlBinaryFormatReader2.h>
#include <XamlBinaryFormatSubReader2.h>
#include <XbfVersioning.h>
#include <ObjectWriterNodeList.h>
#include <ObjectWriter.h>
#include <XbfWriter.h>
#include <wil\result.h>

//#define DEBUG_GENXBF

//
// XBF Generator Design
//
// The XBF Generator relies on the following components:
//
// 1) Xaml Parser
// 2) XamlSchemaContext
// 3) XamlParserCallbacks
// 4) DXaml TypeInfo
//
// The Xaml Parser uses a subset of CCoreServices which are used to retrieve information about the Core Types
// and the XamlParserCallbacks which are used to retrieve information about the DXaml/IXamlMetadataProvider Types.
//
// Processing Steps:
//   1) Initialize XbfCoreServices to initialize the known core types table.
//   2) Initialize the DXamlCore MetadataStore to initialize the known dxaml types table.
//   3) Initialize DXaml XmlNs definitions.
//   3) Read Xaml File Buffer
//   4) Process Xaml Text Buffer using XamlTextReader and convert to XamlNode stream.
//   5) Process XamlNode stream and convert to XamlBinaryFormat.
//   6) Write Xaml Binary Buffer to file.
//

XbfCoreServices *g_pCore = NULL;
enum  XbfGenerationFlags
{
    Default = 0x0,
    DisableLineInfo = 0x1
};

//
// ProcessXamlText
//
//   Input:  Xaml Text Byte Stream
//   Output: Xaml Binary Byte Stream
//
//   Converts the Xaml Text Byte stream into Xaml Binary Byte stream.
//
_Check_return_ HRESULT
ProcessXamlText(
    _In_reads_bytes_(cbResourceBuffer) UINT8* pbResourceBuffer,
    _In_ UINT32 cbResourceBuffer,
    _In_ const TargetOSVersion& targetOS,
    _In_ bool fGenerateLineInfo,
    _In_ const std::array<byte, Parser::c_xbfHashSize>& byteXbfHash,
    _Outptr_result_buffer_(*puiBinaryXamlCount) byte **ppBinaryXaml,
    _Out_ UINT32 *puiBinaryXamlCount,
    _Out_ UINT32 *puiErrorCode,
    _Out_ UINT32 *puiLine,
    _Out_ UINT32 *puiColumn
    )
{
    HRESULT hr = S_OK;
    std::shared_ptr<XamlTextReader> spXamlTextReader;
    std::shared_ptr<XbfWriter> spXbfWriter;
    xstring_ptr ssSourceAssemblyName;
    std::shared_ptr<XbfParserErrorService> spErrorService;
    bool bRequireDefaultNamespace = false;
    bool shouldProcessUid = true;
    bool bPushedSourceAssembly = false;
    std::shared_ptr<XamlSchemaContext> spContext;
    std::shared_ptr<ObjectWriter> spObjectWriter;
    std::shared_ptr<ObjectWriterNodeList> spObjectNodeList;
    spContext = g_pCore->GetSchemaContext();
    IFC(spContext->PushSourceAssembly(ssSourceAssemblyName));
    bPushedSourceAssembly = true;

    spErrorService = std::make_shared<XbfParserErrorService>();
    spErrorService->Initialize(g_pCore);
    IFC(spContext->SetErrorService(spErrorService));

    IFC(XamlTextReader::Create(spContext,
                               XamlTextReaderSettings(bRequireDefaultNamespace, shouldProcessUid, false /*bForceUtf16*/),
                               cbResourceBuffer,
                               pbResourceBuffer,
                               spXamlTextReader));
    {
        ObjectWriterSettings owSettings;
        owSettings.set_EnableEncoding(true);
        IFC(ObjectWriter::Create(spContext, owSettings, spObjectWriter));
    }

    while ((hr = spXamlTextReader->Read()) == S_OK)
    {
        IFC(spObjectWriter->WriteNode(spXamlTextReader->CurrentNode()));
    }
    IFC(hr);

    spObjectNodeList = spObjectWriter->GetNodeList();
    if (spObjectNodeList)
    {
        IFC(spObjectNodeList->Optimize());
    }

    spXbfWriter = std::make_shared<XbfWriter>(g_pCore, targetOS);

    IFC(spXbfWriter->GetOptimizedBinaryEncodingFromReader(spXamlTextReader, spObjectNodeList, byteXbfHash, fGenerateLineInfo, puiBinaryXamlCount, ppBinaryXaml));

Cleanup:
    if (bPushedSourceAssembly)
    {
        spContext->PopSourceAssembly();
    }

    if (FAILED(hr))
    {
        XamlNode node = spXamlTextReader->CurrentNode();
        XamlLineInfo lineInfo = node.get_LineInfo();

        *puiLine = lineInfo.LineNumber();
        *puiColumn = lineInfo.LinePosition();
        *puiErrorCode = hr;
    }

    return(SUCCEEDED(hr) ? S_OK : hr);
}

//
// Initialize
//
//   Initialize the Known Type System for both Core and DXaml
//   and create the XbfCoreServices.
//
_Check_return_ HRESULT Initialize()
{
#ifdef DEBUG_GENXBF
    while (!::IsDebuggerPresent()) {
        ::Sleep(1000);
    }
    ::DebugBreak();
#endif
    HRESULT hr = S_OK;

    ASSERT(g_pCore == NULL);
    IFC(DirectUI::StaticLockGlobalInit());
    IFC(XbfCoreServices::Create(&g_pCore));
    g_pCore->SetIsGeneratingBinaryXaml(TRUE);


Cleanup:
    return hr;
}

//
// Deinitialize
//
//   Destory the core and shutdown.
//
_Check_return_ HRESULT Deinitialize()
{
    if (g_pCore)
    {
        delete g_pCore;
        g_pCore = NULL;
    }

    // msbuild.exe can reuse the same instance of loaded GenXbf.dll
    // which in should not reuse the metadata cache from a previous build
    // so we destroy the Metadata cache as it is a global static.
    DirectUI::MetadataAPI::Destroy();

    DirectUI::StaticLockGlobalDeinit();

    return S_OK;
}

#pragma warning (disable: 26014) // disabling for TVS bug:7238687, ppXamlStreams/ppXbfStreams are properly validated through numStreams and fileIndex

// WriteImpl implements the writing to the streams that Write, WriteToStreams, and Write all use.
_Check_return_ HRESULT WriteImpl(
    _In_reads_(numStreams) IStream **ppXamlStreams,
    _In_ UINT32 numStreams,
    _In_ IXbfMetadataProvider *pMetadataProvider,
    _In_ const std::vector<std::array<byte,Parser::c_xbfHashSize>>& byteXbfHash,
    _In_ const TargetOSVersion& targetOS,
    _In_ XbfGenerationFlags generatorFlags,
    _In_reads_(numStreams) IStream **ppXbfStreams,
    _Out_  UINT32 *puiErrorCode,
    _Out_  UINT32 *puiErrorFileIndex,
    _Out_  UINT32 *puiErrorLine,
    _Out_  UINT32 *puiErrorColumn)
{
    // We really don't expect to be invoked for pre-Windows 10 apps
    if (targetOS < Parser::Versioning::OSVersions::WIN10_TH1)
    {
        IFC_RETURN(E_UNEXPECTED);
    }

    UINT32 fileIndex = 0;
    auto errorFileIndexGuard = wil::scope_exit([&]
    {
        if (fileIndex < numStreams)
        {
            *puiErrorFileIndex = fileIndex;
        }
    });

    ctl::ComPtr<XamlMetadataProviderAdapter> spXamlMetadataProviderAdapter;
    IFC_RETURN(ctl::make(&spXamlMetadataProviderAdapter));
    spXamlMetadataProviderAdapter->SetXamlMetadataProvider(pMetadataProvider);
    IFC_RETURN(DirectUI::MetadataAPI::SetMetadataProvider(spXamlMetadataProviderAdapter.Get()));

    IFCPTR_RETURN(ppXamlStreams);

    // Loop through input streams, read the buffer, process the buffer into binary and write it out
    for (fileIndex = 0; fileIndex < numStreams; fileIndex++)
    {
        // Seek to end of stream to get size
        ULARGE_INTEGER xamlBufferSize = { 0, 0 };
        LARGE_INTEGER offset = { 0, 0 };
        IFC_RETURN(ppXamlStreams[fileIndex]->Seek(offset, STREAM_SEEK_END, &xamlBufferSize));

        std::unique_ptr<byte[]> spXamlText(new byte[xamlBufferSize.LowPart]);
        // Reset the seek to the beginning of the buffer
        ULARGE_INTEGER bufferPosition = { 0, 0 };
        IFC_RETURN(ppXamlStreams[fileIndex]->Seek(offset, STREAM_SEEK_SET, &bufferPosition));

        // Get the xaml text buffer from the input stream
        ULONG bytesRead = 0;
        IFC_RETURN(ppXamlStreams[fileIndex]->Read(spXamlText.get(), xamlBufferSize.LowPart, &bytesRead));
        IFCEXPECT_RETURN(xamlBufferSize.LowPart == bytesRead);

        // Generate the Binary
        UINT32 xbfBufferSize = 0;
        byte* pXbfBinary = nullptr;
        IFC_RETURN(ProcessXamlText(spXamlText.get(),
            static_cast<UINT32>(xamlBufferSize.LowPart),
            targetOS,
            generatorFlags != XbfGenerationFlags::DisableLineInfo,
            byteXbfHash[fileIndex],
            &pXbfBinary,
            &xbfBufferSize,
            puiErrorCode,
            puiErrorLine,
            puiErrorColumn));
        std::unique_ptr<byte[]> spXbfBuffer(pXbfBinary);

        // Write binary to the output stream
        ULONG bytesWritten = 0;
        IFC_RETURN(ppXbfStreams[fileIndex]->Write(spXbfBuffer.get(), xbfBufferSize, &bytesWritten));
        IFCEXPECT_RETURN(xbfBufferSize == bytesWritten);
    }

    errorFileIndexGuard.release();
    return S_OK;
}

#pragma warning (default: 26014)

//
// PUBLIC EXPORT:
//   WriteToStreams(IStream[] inputStreams,   unsigned int numberOfStreams, IXbfMetadataProvider provider, TargetOSVersion targetOS, IStream[] outputStreams,);
//
//   API called to write to XBF streams. This API is similar to Write, but takes the TargetOSVersion info struct as an input parameter that the XBF Generator
//   will use for versioning the XBF files starting with the Threshold release. This API is used by VS in the XAML Compiler, it takes an input stream of XAML text
//   and writes to streams that contain the XBF binary.
//
extern "C" HRESULT WINAPI
WriteToStreams(
_In_reads_(numFiles) IStream **ppXamlStreams,
_In_   UINT32 numFiles,
_In_   IXbfMetadataProvider *pMetadataProvider,
_In_   const TargetOSVersion& targetOS,
_In_reads_(numFiles)  IStream **ppXbfStreams,
_Out_  UINT32 *puiErrorCode,
_Out_  UINT32 *puiErrorFileIndex,
_Out_  UINT32 *puiErrorLine,
_Out_  UINT32 *puiErrorColumn)
{
    HRESULT hr = S_OK;
    std::vector<std::array<byte, Parser::c_xbfHashSize>> xbfEmptyHash(numFiles);    // initializes array to {0}

    IFC(Initialize());

    // We pass null checksum here because this API doesn't support checksums
    hr = WriteImpl(
        ppXamlStreams,
        numFiles,
        pMetadataProvider,
        xbfEmptyHash,
        targetOS,
        XbfGenerationFlags::Default,
        ppXbfStreams,
        puiErrorCode,
        puiErrorFileIndex,
        puiErrorLine,
        puiErrorColumn
        );

    if (FAILED(hr))
    {
        // prefer the ParserErrorService error information
        std::shared_ptr<XamlSchemaContext> spContext;
        std::shared_ptr<ParserErrorReporter> spErrorService;
        std::shared_ptr<XbfParserErrorService> spParserErrorService;

        spContext = std::static_pointer_cast<XamlSchemaContext>(g_pCore->GetSchemaContext());
        IFC(spContext->GetErrorService(spErrorService));

        spParserErrorService = std::static_pointer_cast<XbfParserErrorService>(spErrorService);
        if (spParserErrorService && spParserErrorService->IsErrorRecorded())
        {
            *puiErrorCode = spParserErrorService->GetErrorCode();
            *puiErrorLine = spParserErrorService->GetErrorLine();
            *puiErrorColumn = spParserErrorService->GetErrorColumn();
        }
    }

    IFC(Deinitialize());
Cleanup:
    return hr;
}

//
// PUBLIC EXPORT:
//   Write(IStream[] inputStreams, unsigned int numberOfStreams, char[] checksum, unsigned int checksumSize, IXbfMetadataProvider provider, TargetOSVersion targetOS, IStream[] outputStreams);
//
//   API called to write to XBF streams. Similar to WriteWithStreams but also contains the checksum of the XAML file calculated by VS. The pass it to us and
//   we add it to the header of the XBF. VS needs a checksum for debugging, they can verify when under the debugger if the XAML file has changed at all, so that
//   our new source information doesn't point developers to bogus locations when they've changed a file. VS calculates it on their end, and reads it from the XamlDiagnostics
//   tap to ensure they have matiching versions.
//
extern "C" HRESULT WINAPI
Write(
_In_reads_(numFiles) IStream **ppXamlStreams,
_In_   UINT32 numFiles,
_In_reads_bytes_(cbChecksum) byte** pbChecksum,
_In_   UINT32 cbChecksum,
_In_   IXbfMetadataProvider *pMetadataProvider,
_In_   const TargetOSVersion& targetOS,
_In_   XbfGenerationFlags generatorFlags,
_In_reads_(numFiles)  IStream **ppXbfStreams,
_Out_  UINT32 *puiErrorCode,
_Out_  UINT32 *puiErrorFileIndex,
_Out_  UINT32 *puiErrorLine,
_Out_  UINT32 *puiErrorColumn)
{
    HRESULT hr = S_OK;

    std::vector<std::array<byte, Parser::c_xbfHashSize>> xbfHash(numFiles);    // initializes array to {0}
    for (UINT32 i = 0; i < numFiles; ++i)
    {
        std::copy_n(pbChecksum[i], cbChecksum, xbfHash[i].begin());
    }


    IFC(Initialize());
    hr = WriteImpl(
        ppXamlStreams,
        numFiles,
        pMetadataProvider,
        xbfHash,
        targetOS,
        generatorFlags,
        ppXbfStreams,
        puiErrorCode,
        puiErrorFileIndex,
        puiErrorLine,
        puiErrorColumn
        );

    if (FAILED(hr))
    {
        // prefer the ParserErrorService error information
        std::shared_ptr<XamlSchemaContext> spContext;
        std::shared_ptr<ParserErrorReporter> spErrorService;
        std::shared_ptr<XbfParserErrorService> spParserErrorService;

        spContext = std::static_pointer_cast<XamlSchemaContext>(g_pCore->GetSchemaContext());
        IFC(spContext->GetErrorService(spErrorService));

        spParserErrorService = std::static_pointer_cast<XbfParserErrorService>(spErrorService);
        if (spParserErrorService && spParserErrorService->IsErrorRecorded())
        {
            *puiErrorCode = spParserErrorService->GetErrorCode();
            *puiErrorLine = spParserErrorService->GetErrorLine();
            *puiErrorColumn = spParserErrorService->GetErrorColumn();
        }
    }

    IFC(Deinitialize());
Cleanup:
    return hr;
}

extern "C" HRESULT WINAPI
Dump(
_In_ IStream *pXbfStream,
_In_ IStream *pOutStream,
_In_ UINT32 mode,
_Out_  UINT32 *puiErrorCode)
{
    IFC_RETURN(Initialize());

    *puiErrorCode = 0;

    IFC_RETURN(DumpHelper::DumpXbfStream(
        pXbfStream,
        pOutStream,
        puiErrorCode
        ));

    IFC_RETURN(Deinitialize());

    return S_OK;
}
