// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "HeapMemory.h"
#include "Xbf2MetadataUnitTests.h"
#include "ParserUtilities.h"
#include "ParserUnitTestIncludes.h"
#include "MockParserCoreServices.h"
#include <XamlLogging.h>
#include <BasePALResource.h>
#include <FilePathResource.h>
#include <XamlBinaryFileAccessFactories.h>
#include <xamlbinaryformatreader2.h>
#include <XamlReader.h>
#include <XamlSerializationHelper.h>

using namespace DirectUI;
using namespace Microsoft::UI::Xaml::Tests::Metadata;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    #pragma region Test Class Initialization & Cleanup
    bool Xbf2MetadataUnitTests::ClassSetup()
    {
        //THROW_IF_FAILED(StaticLockGlobalInit());
        return true;
    }

    bool Xbf2MetadataUnitTests::ClassCleanup()
    {
        //THROW_IF_FAILED(StaticLockGlobalDeinit());
        return true;
    }
    #pragma endregion

    #pragma region Test Helper Utilities
    WEX::Common::String GetResourcePath(_In_ WEX::Common::String inXamlFile)
    {
        WEX::Common::String deploymentDirPath;
        WEX::TestExecution::RuntimeParameters::TryGetValue(WEX::TestExecution::RuntimeParameterConstants::c_szTestDeploymentDir, deploymentDirPath);

        if(deploymentDirPath.Right(1) != "\\")
        {
            deploymentDirPath.Append(L"\\");
        }

        deploymentDirPath += inXamlFile;

        return deploymentDirPath;
    }

    xref_ptr<IPALMemory> GetXbfBuffer()
    {
        xref_ptr<IPALMemory> spBuffer;
        xref_ptr<IPALUri> filePathUri;
        xref_ptr<IPALResource> filePathResource;
        WEX::Common::String xbfFile(GetResourcePath(L"resources\\native\\framework\\parser\\SampleXbf2.xbf"));
        xephemeral_string_ptr xbfFilePathStr(xbfFile.GetBuffer(), xbfFile.GetLength());
        xstring_ptr strXbfFilePath;
        VERIFY_SUCCEEDED(xbfFilePathStr.Promote(&strXbfFilePath));

        VERIFY_SUCCEEDED(CFilePathResource::Create(filePathUri.get(), strXbfFilePath, filePathResource.ReleaseAndGetAddressOf()));
        VERIFY_SUCCEEDED(filePathResource->Load(spBuffer.ReleaseAndGetAddressOf()));

        return spBuffer;
    }

    xref_ptr<IPALMemory> GetXbfBufferClone()
    {
        auto xbfBuffer = GetXbfBuffer();
        xref_ptr<IPALMemory> xbfBufferClone;
        uint8_t* pMemoryBuffer = new uint8_t[xbfBuffer->GetSize()];
        memcpy(pMemoryBuffer, xbfBuffer->GetAddress(), xbfBuffer->GetSize());
        VERIFY_SUCCEEDED(CHeapMemory::Create(xbfBuffer->GetSize(), pMemoryBuffer, xbfBufferClone.ReleaseAndGetAddressOf()));

        return xbfBufferClone;
    }

    std::shared_ptr<XamlBinaryMetadataReader2> GetMetadataReader(_In_ xref_ptr<IPALMemory> xbfBuffer)
    {
        ParserUtilities parserUtils;
        uint32_t bufferOffset = 0;
        const auto uiBinaryXaml = xbfBuffer->GetSize();
        auto pBinaryXaml = static_cast<XBYTE*>(xbfBuffer->GetAddress());

        // Read the size of buffers
        auto puintBinaryXaml = static_cast<XUINT32*>(xbfBuffer->GetAddress());
        uint32_t uiMetadataSize = puintBinaryXaml[1];
        uint32_t uiNodeSize = puintBinaryXaml[2];

        VERIFY_IS_TRUE(uiMetadataSize > 0);
        VERIFY_IS_TRUE(uiNodeSize > 0);

        // verify magic number in header
        VERIFY_IS_TRUE(pBinaryXaml[0] == 0x58 && pBinaryXaml[1] == 0x42 && pBinaryXaml[2] == 0x046 && pBinaryXaml[3] == 0x00);

        bufferOffset += 3 * sizeof(uint32_t);
        xref_ptr<IPALMemory> spMetadataFileMemory = parserUtils.GetPALMemoryFromBuffer(uiMetadataSize, pBinaryXaml + bufferOffset);
        xref_ptr<IPALStream> spMetadataStream = parserUtils.GetPALStreamFromPALMemory(spMetadataFileMemory);

        ::Parser::XamlBinaryFileVersion fileVersion;
        VERIFY_SUCCEEDED(XamlBinaryFormatSerializationHelper::DeserializeItemFromMetadataStream(&fileVersion, spMetadataStream));

        VERIFY_IS_TRUE(fileVersion.m_uMajorBinaryFileVersion == 2);

        // construct schema context with no custom error reporter
        std::shared_ptr<ParserErrorReporter> nullErrorReporter;
        auto schemaContext = parserUtils.GetSchemaContext(nullErrorReporter);
        // TODO: Figure out why mock MetadataStorage is bringing in dependencies that cannot
        // be loaded by TAEF.
        // auto mock = TlsProvider<DynamicMetadataStorageMock>::CreateWrappedObject();
        auto metadataReader = std::make_shared<XamlBinaryMetadataReader2>(
            schemaContext,
            spMetadataStream,
            spMetadataFileMemory,
            ::Parser::XamlBufferType::Binary,
            fileVersion);
        return metadataReader;
    }
    #pragma endregion

    const unsigned int c_XbfStringTableCount = 90;
    const unsigned int c_XbfAssemblyTableCount = 1;
    const unsigned int c_XbfTypeNamespaceTableCount = 2;
    const unsigned int c_XbfTypeTableCount = 2;
    const unsigned int c_XbfPropertyTableCount = 1;
    const unsigned int c_XbfXmlNamespaceTableCount = 8;

    void Xbf2MetadataUnitTests::Create()
    {
        auto xbfBuffer = GetXbfBuffer();
        auto metadataReader = GetMetadataReader(xbfBuffer);
        VERIFY_SUCCEEDED(metadataReader->LoadMetadata());
    }

    void Xbf2MetadataUnitTests::VerifyBasicMetadataLoading()
    {
        auto xbfBuffer = GetXbfBuffer();
        auto metadataReader = GetMetadataReader(xbfBuffer);
        VERIFY_SUCCEEDED(metadataReader->LoadMetadata());

        VERIFY_ARE_EQUAL(metadataReader->GetStringTableCount(), c_XbfStringTableCount);
        VERIFY_ARE_EQUAL(metadataReader->GetAssemblyTableCount(), c_XbfAssemblyTableCount);
        VERIFY_ARE_EQUAL(metadataReader->GetTypeNamespaceTableCount(), c_XbfTypeNamespaceTableCount);
        VERIFY_ARE_EQUAL(metadataReader->GetTypeTableCount(), c_XbfTypeTableCount);
        VERIFY_ARE_EQUAL(metadataReader->GetPropertyTableCount(), c_XbfPropertyTableCount);
        VERIFY_ARE_EQUAL(metadataReader->GetXmlNamespaceTableCount(), c_XbfXmlNamespaceTableCount);
    }

    void Xbf2MetadataUnitTests::VerifyStringTable()
    {
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strCollapsedString, L"Collapsed");
        auto xbfBuffer = GetXbfBuffer();
        auto metadataReader = GetMetadataReader(xbfBuffer);
        xstring_ptr stringEntry;
        VERIFY_SUCCEEDED(metadataReader->LoadMetadata());
        VERIFY_ARE_EQUAL(metadataReader->GetStringTableCount(), c_XbfStringTableCount);

        PersistedXamlNode2 entryIndex;
        entryIndex.m_uiObjectId = 10;
        VERIFY_SUCCEEDED(metadataReader->GetString(entryIndex, stringEntry));
        VERIFY_ARE_EQUAL(stringEntry.Compare(c_strCollapsedString), 0);
    }

    void Xbf2MetadataUnitTests::VerifyXmlNamespaceTable()
    {
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strXamlNamespace, L"http://schemas.microsoft.com/winfx/2006/xaml/presentation");
        auto xbfBuffer = GetXbfBuffer();
        auto metadataReader = GetMetadataReader(xbfBuffer);
        VERIFY_SUCCEEDED(metadataReader->LoadMetadata());
        VERIFY_ARE_EQUAL(metadataReader->GetXmlNamespaceTableCount(), c_XbfXmlNamespaceTableCount);

        PersistedXamlNode2 entryIndex;
        std::shared_ptr<XamlNamespace> xamlNamespace;
        entryIndex.m_uiObjectId = 0;
        VERIFY_SUCCEEDED(metadataReader->GetXmlNamespace(entryIndex, xamlNamespace));
        xstring_ptr strTargetNamespace = xamlNamespace->get_TargetNamespace();
        VERIFY_ARE_EQUAL(strTargetNamespace.Compare(c_strXamlNamespace), 0);

    }

    void VerifyTableOffsetFailures(xref_ptr<IPALMemory> xbfBuffer, const std::function<void()> &initialization, const std::function<void()> &cleanup)
    {
        // make table offset incorrect
        {
            auto metadataReader = GetMetadataReader(xbfBuffer);
            initialization();
            VERIFY_FAILED(metadataReader->LoadMetadata());
            cleanup();
        }

        // verify we are in good state again
        {
            auto metadataReader = GetMetadataReader(xbfBuffer);
            VERIFY_SUCCEEDED(metadataReader->LoadMetadata());
        }
    }

    void Xbf2MetadataUnitTests::VerifyTableLoadFailures()
    {
        auto xbfBuffer = GetXbfBufferClone();
        uint64_t headerPtrOffset = 3 * sizeof(uint32_t)+sizeof(::Parser::XamlBinaryFileVersion);
        uint8_t *headerPtr = static_cast<uint8_t*>(xbfBuffer->GetAddress()) + headerPtrOffset;
        XamlBinaryFileHeader2* metadataHeader = reinterpret_cast<XamlBinaryFileHeader2*>(headerPtr);

        // incorrect assembly list offset
        VerifyTableOffsetFailures(xbfBuffer,
            [&metadataHeader]() { metadataHeader->m_uAssemblyListOffset += 100; },
            [&metadataHeader]() { metadataHeader->m_uAssemblyListOffset -= 100; });

        // incorrect property list offset
        VerifyTableOffsetFailures(xbfBuffer,
            [&metadataHeader]() { metadataHeader->m_uPropertyListOffset += 100; },
            [&metadataHeader]() { metadataHeader->m_uPropertyListOffset -= 100; });

        // incorrect string table offset
        VerifyTableOffsetFailures(xbfBuffer,
            [&metadataHeader]() { metadataHeader->m_uStringTableOffset += 100; },
            [&metadataHeader]() { metadataHeader->m_uStringTableOffset -= 100; });

        // incorrect type list offset
        VerifyTableOffsetFailures(xbfBuffer,
            [&metadataHeader]() { metadataHeader->m_uTypeListOffset += 100; },
            [&metadataHeader]() { metadataHeader->m_uTypeListOffset -= 100; });

        // incorrect type namespace list offset
        VerifyTableOffsetFailures(xbfBuffer,
            [&metadataHeader]() { metadataHeader->m_uTypeNamespaceListOffset += 100; },
            [&metadataHeader]() { metadataHeader->m_uTypeNamespaceListOffset -= 100; });

        // incorrect type namespace list offset
        VerifyTableOffsetFailures(xbfBuffer,
            [&metadataHeader]() { metadataHeader->m_uXmlNamespaceListOffset += 100; },
            [&metadataHeader]() { metadataHeader->m_uXmlNamespaceListOffset -= 100; });
    }

} } } } }
