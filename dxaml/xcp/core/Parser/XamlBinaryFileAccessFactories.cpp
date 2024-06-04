// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlBinaryFileAccessFactories.h"
#include "XamlBinaryMetadata.h"
#include "XamlSerializationHelper.h"
#include "XamlBinaryMetadataReader.h"
#include "XamlBinaryMetadataReader2.h"
#include "XamlBinaryFormatReader.h"
#include "XamlBinaryFormatReader2.h"
#include <ParserAPI.h>

namespace Parser
{
    _Check_return_ HRESULT
        CreateXamlBinaryFileReader(
        _In_ const std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext,
        _In_ IPALMemory* pFileStream,
        _In_ XamlBufferType bufferType,
        _Out_ std::shared_ptr<XamlReader>& spReader,
        _Out_ std::shared_ptr<XamlBinaryFormatReader2>& spVersion2Reader)
    {
        XUINT32 uiOffset = 0;
        const auto uiBinaryXaml = pFileStream->GetSize();
        auto pBinaryXaml = static_cast<XBYTE*>(pFileStream->GetAddress());

        IFCEXPECT_RETURN(pBinaryXaml);
        IFCEXPECT_RETURN(uiBinaryXaml > 2 * sizeof(XUINT32));

        auto puintBinaryXaml = static_cast<XUINT32*>(pFileStream->GetAddress());

        // Read the size of our buffers
        XUINT32 uiMetadataSize = puintBinaryXaml[1];
        XUINT32 uiNodeSize = puintBinaryXaml[2];

        uiOffset += 3 * sizeof(XUINT32);

        IFCEXPECT_RETURN(uiNodeSize < uiBinaryXaml && uiMetadataSize < uiBinaryXaml);
        IFCEXPECT_RETURN(uiNodeSize + uiMetadataSize + 3 * sizeof(XUINT32) <= uiBinaryXaml);

        // verify magic number in header
        IFCEXPECT_RETURN(pBinaryXaml[0] == 0x58 && pBinaryXaml[1] == 0x42 && pBinaryXaml[2] == 0x046 && pBinaryXaml[3] == 0x00);

        xref_ptr<IPALStream> spMetadataFile;
        xref_ptr<IPALMemory> spMetadataFileMemory;
        IFC_RETURN(GetPALMemoryServices()->CreatePALMemoryFromBuffer(uiMetadataSize, pBinaryXaml + uiOffset, false /* OwnsBuffer */, spMetadataFileMemory.ReleaseAndGetAddressOf()));
        IFC_RETURN(GetPALMemoryServices()->CreateIPALStreamFromIPALMemory(spMetadataFileMemory.get(), spMetadataFile.ReleaseAndGetAddressOf()));

        uiOffset += uiMetadataSize;

        xref_ptr<IPALStream> spNodeFile;
        xref_ptr<IPALMemory> spNodeFileMemory;
        IFC_RETURN(GetPALMemoryServices()->CreatePALMemoryFromBuffer(uiNodeSize, pBinaryXaml + uiOffset, false /* OwnsBuffer */, spNodeFileMemory.ReleaseAndGetAddressOf()));
        IFC_RETURN(GetPALMemoryServices()->CreateIPALStreamFromIPALMemory(spNodeFileMemory.get(), spNodeFile.ReleaseAndGetAddressOf()));

        XamlBinaryFileVersion fileVersion;
        IFC_RETURN(XamlBinaryFormatSerializationHelper::DeserializeItemFromMetadataStream(&fileVersion, spMetadataFile));

        if (fileVersion == XbfV1)
        {
            // Create a version 1 XBF reader
            auto metadataReader = std::make_shared<XamlBinaryMetadataReader>(spXamlSchemaContext, spMetadataFile, fileVersion);
            std::shared_ptr<XamlBinaryFormatReader> tempReader;
            IFC_RETURN(metadataReader->LoadMetadata());
            IFC_RETURN(XamlBinaryFormatReader::Create(spXamlSchemaContext, metadataReader, spNodeFile, tempReader));
            spReader = std::move(tempReader);
        }
        else if (fileVersion.m_uMajorBinaryFileVersion == 2)
        {
            auto metadataReader = std::make_shared<XamlBinaryMetadataReader2>(spXamlSchemaContext, spMetadataFile, spMetadataFileMemory, bufferType, fileVersion);
            IFC_RETURN(metadataReader->LoadMetadata());
            IFC_RETURN(XamlBinaryFormatReader2::Create(spXamlSchemaContext, metadataReader, spNodeFile, xref_ptr<IPALMemory>(pFileStream), uiOffset, spVersion2Reader));
        }
        else
        {
            std::shared_ptr<ParserErrorReporter> errorReporter;
            IFC_RETURN(spXamlSchemaContext->GetErrorService(errorReporter));

            // The error service can't handle NULL parameters
            DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strNullLiteral, L"null");

            IFC_RETURN(errorReporter->SetError(
                AG_E_PARSER2_XBF_METADATA_VERSION,
                0,
                0,
                c_strNullLiteral,
                c_strNullLiteral));

            IFC_RETURN(E_FAIL);
        }

        return S_OK;
    }
}

