// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlBinaryMetadata.h"
#include "XamlBinaryMetadataReader2.h"
#include "ObjectWriterNode.h"

#include <vector_map.h>

class XamlBinaryMetadataReader2;
class XamlBinaryFormatSubReader2;

class XamlBinaryFormatReader2 : 
    public std::enable_shared_from_this<XamlBinaryFormatReader2>
{
public:
    XamlBinaryFormatReader2(
        _In_ const std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext,
        _In_ const std::shared_ptr<XamlBinaryMetadataReader2>& spXamlBinaryMetadataReader,
        _In_ const xref_ptr<IPALMemory>& spXbfMemory,
        _In_ const XUINT32 nodeStreamStartOffset
        )
        : m_spXamlSchemaContext(spXamlSchemaContext)
        , m_spXamlBinaryMetadataReader(spXamlBinaryMetadataReader)        
        , m_spXbfMemory(spXbfMemory)
        , m_cbNodeStreamStartOffset(nodeStreamStartOffset)
    {
    }

    _Check_return_ HRESULT Initialize(_In_ const xref_ptr<IPALStream>& spNodeStreamStream);

    _Check_return_ static HRESULT Create(
        _In_ const std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext,
        _In_ const std::shared_ptr<XamlBinaryMetadataReader2>& spXamlBinaryMetadataReader,
        _In_ const xref_ptr<IPALStream>& spNodeStreamStream,
        _In_ const xref_ptr<IPALMemory>& spXbfMemory,
        _In_ const XUINT32 nodeStreamStartOffset,
        _Out_ std::shared_ptr<XamlBinaryFormatReader2>& spXamlBinaryFormatReader
        );

    _Check_return_ HRESULT GetSchemaContext(std::shared_ptr<XamlSchemaContext>& outSchemaContext)
    {
        outSchemaContext = m_spXamlSchemaContext.lock();
        if (!outSchemaContext)
        {
            IFC_RETURN(E_FAIL);
        }
        return S_OK;
    }

    const Parser::XamlBinaryFileVersion& GetVersion() const
    {
        return m_spXamlBinaryMetadataReader->GetVersion();
    }

    _Check_return_ HRESULT GetSubReader(
        _In_ const unsigned int streamIndex,
        _Out_ std::shared_ptr<XamlBinaryFormatSubReader2>& spBinaryReader);

    typedef std::pair<unsigned int, std::shared_ptr<CustomWriterRuntimeData>> StreamLengthRuntimeDataPair;

    const StreamLengthRuntimeDataPair* TryGetRuntimeData(unsigned int masterStreamOffset) const;
    void SetRuntimeData(unsigned int masterStreamOffset, StreamLengthRuntimeDataPair data);

    XamlBinaryMetadataReader2& GetMetadataReader() const;

    xstring_ptr GetXbfHash() const;

    const std::shared_ptr<std::vector<xstring_ptr_storage>>& GetStringStorage() const
    {
        return m_spXamlBinaryMetadataReader->GetStringStorage();
    }

private:
    std::weak_ptr<XamlSchemaContext> m_spXamlSchemaContext;
    std::shared_ptr<XamlBinaryMetadataReader2> m_spXamlBinaryMetadataReader;  
    xvector< PersistedXamlSubStream > m_vecMasterStreamIndex;
    xref_ptr<IPALMemory> m_spXbfMemory;
    XUINT32 m_cbNodeStreamStartOffset{};
    XUINT32 m_cbNodeStreamSize{};
    XUINT32 m_cbStreamTableSize{};

    // A vector_map of stream offset positions mapped to a tuple containing
    // the stream length of the CustomWriterRuntimeData, stored from the first time
    // we parsed it, and the instance itself. This cache allows us to skip reparsing
    // the data and allows us to share it between all instances of objects created
    // from this reader.
    std::unique_ptr<containers::vector_map<unsigned int,
        StreamLengthRuntimeDataPair >> m_cachedRuntimeData;
};

