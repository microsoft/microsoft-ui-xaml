// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "xcptypes.h"
#include "DumpAdapterMetadataProvider.h"
#include "DumpHelper.h"
#include <ParserAPI.h>
#include <XamlBinaryFileAccessFactories.h>
#include <XamlBinaryFormatReader2.h>
#include <XamlBinaryFormatSubReader2.h>
#include <ObjectWriterNodeList.h>
#include <ObjectWriter.h>
#include <wil\result.h>

std::wstring GetStringForCurrentNode(std::wstring& strIndent, ObjectWriterNode& node)
{
    std::wstring currentNodeAsString;

    // adjust indent for ending scopes
    if (node.RequiresScopeToEnd())
    {
        if (strIndent.size() >= 2)
        {
            strIndent = strIndent.substr(0, strIndent.size() - 2);
        }
    }

    // append the information about the node
    xstring_ptr nodeDescription;
    xstring_ptr lineInfoString;

    node.GetLineInfoAsString(lineInfoString);
    node.ToString(nodeDescription);
    currentNodeAsString.append(L"[");
    currentNodeAsString.append(lineInfoString.GetBuffer());
    currentNodeAsString.append(L"] ");
    for (int i = lineInfoString.GetCount() + 2; i < 30; i++)
    {
        currentNodeAsString.append(L" ");
    }
    currentNodeAsString.append(strIndent);
    currentNodeAsString.append(L" ");
    currentNodeAsString.append(nodeDescription.GetBuffer());

    // if the node opens a new scope, the following nodes should be indented
    if (node.RequiresNewScope())
    {
        strIndent.append(L"  ");
    }

    return currentNodeAsString;
}

HRESULT DumpHelper::DumpXbfStream(
    _In_  IStream *pXbfStream,
    _In_  IStream *pOutStream,
    _Out_ UINT32  *puiErrorCode)
{
    std::shared_ptr<XamlBinaryFormatReader2> spVersion2Reader;
    IFC_RETURN(CreateXbf2Reader(pXbfStream, spVersion2Reader));

    if (!spVersion2Reader)
    {
        *puiErrorCode = 0x1;
        return S_OK;
    }
    else
    {
        std::vector< std::shared_ptr<XamlBinaryFormatSubReader2> > streamsToRead;

        // get the primary stream (Stream 0)
        std::shared_ptr<XamlBinaryFormatSubReader2> initialXbfReader;
        IFC_RETURN(spVersion2Reader->GetSubReader(0, initialXbfReader));
        streamsToRead.push_back(initialXbfReader);

        std::wstring allStreamsDump;
        int currentStreamIndex = 0;
        int streamIndexCount = 0;

        // iterate through all the referenced streams
        while (!streamsToRead.empty())
        {            
            auto spXbfReader = streamsToRead.front();
            std::wstring currentStreamDump;

            std::wstring strIndent;
            ObjectWriterNode node;
            bool done = false;
            while (!done)
            {
                bool endOfStream = false;
                IFC_RETURN(spXbfReader->TryReadHRESULT(node, &endOfStream));
                if (endOfStream) break;

                currentStreamDump.append(GetStringForCurrentNode(strIndent, node));

                // if node is a custom binary data node, we have additional information to process
                if (node.ProvidesCustomBinaryData())
                {
                    auto nodeCustomStream = node.GetCustomWriterNodeStream();
                    std::shared_ptr<XamlBinaryFormatSubReader2> spNewXbfReader;
                    if (nodeCustomStream)
                    {
                        spNewXbfReader = nodeCustomStream->GetSubReader();
                    }
                    else
                    {
                        spNewXbfReader = node.GetSubReader();
                    }

                    // queue the stream
                    streamsToRead.push_back(spNewXbfReader);
                    streamIndexCount++;

                    currentStreamDump.append(L" (Stream:");
                    currentStreamDump.append(std::to_wstring(streamIndexCount));
                    currentStreamDump.append(L")");
                }
                currentStreamDump.append(L"\r\n");
            }
            
            allStreamsDump.append(L"[Stream:");
            allStreamsDump.append(std::to_wstring(currentStreamIndex));
            allStreamsDump.append(L"]\r\n");
            allStreamsDump.append(currentStreamDump);
            allStreamsDump.append(L"-------\r\n");
            currentStreamIndex++;

            streamsToRead.erase(streamsToRead.begin());
        }

        // Dump the Xbf stream
        UINT32 xbfDumpBufferSize = 0;

        const byte* pXbfDumpBuffer = reinterpret_cast<const byte*>(allStreamsDump.c_str());
        xbfDumpBufferSize = (UINT32)(allStreamsDump.size() * sizeof(allStreamsDump.front()));

        ULONGLONG bytesWritten = 0;
        IFC_RETURN(pOutStream->Write(pXbfDumpBuffer, xbfDumpBufferSize, reinterpret_cast<ULONG*>(&bytesWritten)));
        IFCEXPECT_RETURN(xbfDumpBufferSize == bytesWritten);
    }

    return S_OK;
}

HRESULT DumpHelper::CreateXbf2Reader(
    _In_ IStream *pXbfStream,
    _Out_ std::shared_ptr<XamlBinaryFormatReader2>& spVersion2Reader)
{
    xstring_ptr ssSourceAssemblyName;
    std::shared_ptr<XamlSchemaContext> spContext;
    std::shared_ptr<XbfParserErrorService> spErrorService;
    std::shared_ptr<XamlReader> spReader;
    spContext = g_pCore->GetSchemaContext();
    IFC_RETURN(spContext->PushSourceAssembly(ssSourceAssemblyName));

    spErrorService = std::make_shared<XbfParserErrorService>();
    spErrorService->Initialize(g_pCore);
    IFC_RETURN(spContext->SetErrorService(spErrorService));

    ctl::ComPtr<DumpAdapterMetadataProvider> spDumpMetadataProvider;
    IFC_RETURN(ctl::make(&spDumpMetadataProvider));
    IFC_RETURN(DirectUI::MetadataAPI::SetMetadataProvider(spDumpMetadataProvider.Get()));

    IFCPTR_RETURN(pXbfStream);

    // Seek to end of stream to get size
    ULARGE_INTEGER xamlBufferSize = { 0, 0 };
    LARGE_INTEGER offset = { 0, 0 };
    IFC_RETURN(pXbfStream->Seek(offset, STREAM_SEEK_END, &xamlBufferSize));

    byte *pXbfBuffer = new byte[xamlBufferSize.LowPart];

    // Reset the seek to the beginning of the buffer
    ULARGE_INTEGER bufferPosition = { 0, 0 };
    IFC_RETURN(pXbfStream->Seek(offset, STREAM_SEEK_SET, &bufferPosition));

    ULONGLONG bytesRead = 0;
    IFC_RETURN(pXbfStream->Read(pXbfBuffer, xamlBufferSize.LowPart, reinterpret_cast<ULONG*>(&bytesRead)));
    IFCEXPECT_RETURN(xamlBufferSize.LowPart == bytesRead);

    xref_ptr<IPALMemory> spXbfBufferMemory;
    IFC_RETURN(GetPALMemoryServices()->CreatePALMemoryFromBuffer(xamlBufferSize.LowPart, reinterpret_cast<XUINT8*>(pXbfBuffer), true /*ownsBuffer*/, spXbfBufferMemory.ReleaseAndGetAddressOf()));

    IFC_RETURN(Parser::CreateXamlBinaryFileReader(
        spContext,
        spXbfBufferMemory.get(),
        Parser::XamlBufferType::Binary,
        spReader,
        spVersion2Reader));

    return S_OK;
}

