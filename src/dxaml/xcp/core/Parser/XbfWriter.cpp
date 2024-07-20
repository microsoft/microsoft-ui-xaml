// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XbfWriter.h"
#include "MemoryStreamBuffer.h"
#include "ObjectWriterNodeList.h"
#include "ParserSettings.h"
#include "XamlBinaryFormatWriter.h"
#include "XamlBinaryFormatWriter2.h"
#include "XamlBinaryMetadata.h"
#include "XamlBinaryMetadataStore.h"

using namespace Parser;

//---------------------------------------------------------------------------
//
//  Synopsis:
//      
//  Notes:
//      This is "best effort" generation.  There may be some Xaml that isn't
//      correct, but may not be consumed at run-time anyway. 
//
//---------------------------------------------------------------------------
_Check_return_ 
HRESULT XbfWriter::ProcessXamlTextBuffer(
        _In_reads_bytes_(cXamlTextBufferSize)            XBYTE const *pXamlTextBuffer,
        _In_                                        XUINT32      cXamlTextBufferSize,
        _Outptr_result_bytebuffer_(*pcXamlBinaryBufferSize) XBYTE      **ppXamlBinaryBuffer,
        _Out_                                       XUINT32     *pcXamlBinaryBufferSize
    )
{
    HRESULT hr = S_OK;
    std::shared_ptr<XamlTextReader> spXamlTextReader;
    std::shared_ptr<ObjectWriterNodeList> spObjectNodeList;
    xstring_ptr ssSourceAssemblyName;
    std::shared_ptr<ParserErrorService> spErrorService;
    bool bRequireDefaultNamespace = false;
    bool bPushedSourceAssembly = false;
    std::shared_ptr<XamlSchemaContext> spContext;

    // Dummy hash: third Party XBF don't require a hash as their integrity is guaranteed by the signed package.
    std::array<byte, c_xbfHashSize> xbfEmptyHash = { 0 };

    IFCPTR(pXamlTextBuffer);
    IFCPTR(ppXamlBinaryBuffer);
    IFCPTR(pcXamlBinaryBufferSize);

    *ppXamlBinaryBuffer = nullptr;
    *pcXamlBinaryBufferSize = 0;
   
    spContext = std::static_pointer_cast<XamlSchemaContext>(m_pCore->GetSchemaContext());
    IFC(spContext->PushSourceAssembly(ssSourceAssemblyName));
    bPushedSourceAssembly = true;
    
    spErrorService = std::make_shared<ParserErrorService>();
    spErrorService->Initialize(m_pCore); 
    IFC(spContext->SetErrorService(spErrorService));

    IFC(XamlTextReader::Create(
        spContext,
        XamlTextReaderSettings(bRequireDefaultNamespace, true /* shouldProcessUid */, false /* force utf-16 */),
        cXamlTextBufferSize,
        pXamlTextBuffer,
        spXamlTextReader));

    IFC(GetOptimizedBinaryEncodingFromReader(
        std::static_pointer_cast<XamlReader>(spXamlTextReader), 
        spObjectNodeList,
        xbfEmptyHash,
        true /* generate line information */, 
        pcXamlBinaryBufferSize, 
        ppXamlBinaryBuffer));

    TRACE(TraceAlways, L"XBFPARSER: Processed file to XBF container: InSize:(%d), OutSize:(%d)", cXamlTextBufferSize, *pcXamlBinaryBufferSize);

Cleanup:
    if (bPushedSourceAssembly)
    {
        spContext->PopSourceAssembly();
    }

    return hr;
}

//
//   Input:  XamlReader
//   Output: XBF Byte Stream
//
//   Converts the XamlReader stream into byte buffer using the XamlBinaryFormatWriter.
//   
//   Byte Buffer Format:
//       [ Header          ]
//       [ Metadata Store  ]
//       [ Node Stream     ]
//
_Check_return_
HRESULT XbfWriter::GetOptimizedBinaryEncodingFromReader(
    _In_ const std::shared_ptr<XamlReader>& spReader,
    _In_ const std::shared_ptr<ObjectWriterNodeList>& spObjectNodeList,
    _In_ const std::array<byte, c_xbfHashSize>& hashForBinaryXaml,    
    _In_ bool fGenerateLineInfo,
    _Out_ XUINT32 *puiBinaryXaml,
    _Outptr_result_bytebuffer_(*puiBinaryXaml) XBYTE **ppBinaryXaml
    )
{
    xref_ptr<IPALStream> spMetadataFile;
    xref_ptr<IPALStream> spNodeFile;
    xref_ptr<CMemoryStreamBuffer> spMetadataBuffer;
    xref_ptr<CMemoryStreamBuffer> spNodeBuffer;

    IFC_RETURN(spMetadataBuffer.init(new CMemoryStreamBuffer((XUINT32) -1)));
    IFC_RETURN(spMetadataBuffer->CreateStream(spMetadataFile.ReleaseAndGetAddressOf()));

    IFC_RETURN(spNodeBuffer.init(new CMemoryStreamBuffer((XUINT32)-1)));
    IFC_RETURN(spNodeBuffer->CreateStream(spNodeFile.ReleaseAndGetAddressOf()));

    auto xbfVersion = Parser::Versioning::GetXBFVersion(m_version);
    if (xbfVersion == Parser::XbfV1)
    {
        std::shared_ptr<XamlBinaryFormatWriter> spXamlBinaryFormatWriter;
        std::shared_ptr<XamlBinaryMetadataStore> spXamlBinaryMetadataStore;
        IFC_RETURN(XamlBinaryMetadataStore::Create(
            m_version,
            spXamlBinaryMetadataStore));
        spXamlBinaryFormatWriter = std::make_shared<XamlBinaryFormatWriter>(
            m_pCore,
            spXamlBinaryMetadataStore,
            spMetadataFile,
            spNodeFile,
            fGenerateLineInfo);
        IFC_RETURN(spXamlBinaryFormatWriter->WriteAllNodes(spReader));
        IFC_RETURN(spXamlBinaryMetadataStore->WriteMetadata(spMetadataFile, hashForBinaryXaml));
    }
    else if (xbfVersion.m_uMajorBinaryFileVersion == 2)
    {
        std::shared_ptr<XamlBinaryFormatWriter2> spXamlBinaryFormatWriter;
        std::shared_ptr<XamlBinaryMetadataStore> spXamlBinaryMetadataStore;
        IFC_RETURN(XamlBinaryMetadataStore::Create(
            m_version,
            spXamlBinaryMetadataStore));
        spXamlBinaryFormatWriter = std::make_shared<XamlBinaryFormatWriter2>(
            m_pCore,
            spXamlBinaryMetadataStore,
            spNodeFile,
            fGenerateLineInfo);
        IFC_RETURN(spXamlBinaryFormatWriter->WriteAllNodes(spObjectNodeList));
        IFC_RETURN(spXamlBinaryMetadataStore->WriteMetadata(spMetadataFile, hashForBinaryXaml));    
    }
    else
    {
        IFC_RETURN(E_FAIL);
    }

    // We now have the node stream saved in our buffers. Copy them to an output buffer
    // Add 2 XUINT32 for the size of each stream.

    XUINT64 uiSize = 0;
    IFC_RETURN(spMetadataFile->GetSize(&uiSize));
    XUINT32 uiMetadataSize = static_cast<XUINT32>(uiSize);
    IFCEXPECT_RETURN(uiMetadataSize == uiSize);

    IFC_RETURN(spNodeFile->GetSize(&uiSize));
    XUINT32 uiNodeSize = static_cast<XUINT32>(uiSize);
    IFCEXPECT_RETURN(uiNodeSize == uiSize);

    XUINT32 uiTotalStreamSize = uiNodeSize + uiMetadataSize + 3 * sizeof(XUINT32);
    IFCEXPECT_RETURN(uiTotalStreamSize > uiNodeSize && uiTotalStreamSize > uiMetadataSize);

    std::unique_ptr<XBYTE[]> spBinaryBuffer(new XBYTE[uiTotalStreamSize]);

    XUINT32 *puintBinaryBuffer = reinterpret_cast<XUINT32*>(spBinaryBuffer.get());

    // Add Magic Header (0x58 0x42 0x46 0x00) ASCII: 'XBF'
    spBinaryBuffer[0] = 0x58;
    spBinaryBuffer[1] = 0x42;
    spBinaryBuffer[2] = 0x46;
    spBinaryBuffer[3] = 0x00;
    puintBinaryBuffer[1] = uiMetadataSize;
    puintBinaryBuffer[2] = uiNodeSize;

    XUINT32 uiCurrentOffset = sizeof(XUINT32) * 3;

    IFC_RETURN(spMetadataFile->Read(spBinaryBuffer.get() + uiCurrentOffset, uiMetadataSize, nullptr));
    uiCurrentOffset += uiMetadataSize;
    IFC_RETURN(spNodeFile->Read(spBinaryBuffer.get() + uiCurrentOffset, uiNodeSize, nullptr));

    *puiBinaryXaml = uiTotalStreamSize;
    *ppBinaryXaml = spBinaryBuffer.release();
        
    return S_OK;
}


