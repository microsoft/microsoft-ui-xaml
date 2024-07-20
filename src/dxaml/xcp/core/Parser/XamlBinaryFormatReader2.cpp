// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlBinaryFormatReader2.h"
#include "XamlBinaryFormatSubReader2.h"
#include "XamlSerializationHelper.h"
#include "XamlBinaryFormatCommon.h"
#include "ObjectWriterNode.h"

_Check_return_ HRESULT
XamlBinaryFormatReader2::Create(
    _In_ const std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext,
    _In_ const std::shared_ptr<XamlBinaryMetadataReader2>& spXamlBinaryMetadataReader,
    _In_ const xref_ptr<IPALStream>& spNodeStreamStream,
    _In_ const xref_ptr<IPALMemory>& spXbfMemory,
    _In_ const XUINT32 nodeStreamStartOffset,
    _Out_ std::shared_ptr<XamlBinaryFormatReader2>& spXamlBinaryFormatReader
    )
{
    spXamlBinaryFormatReader = std::make_shared<XamlBinaryFormatReader2>(spXamlSchemaContext, spXamlBinaryMetadataReader, spXbfMemory, nodeStreamStartOffset);

    IFC_RETURN(spXamlBinaryFormatReader->Initialize(spNodeStreamStream));

    return S_OK;
}

_Check_return_ HRESULT 
XamlBinaryFormatReader2::Initialize(_In_ const xref_ptr<IPALStream>& spNodeStreamStream)
{
    XUINT64 totalFileSize = 0;
    XUINT64 streamTableSize = 0;

    IFC_RETURN(spNodeStreamStream->GetSize(&totalFileSize));
    m_cbNodeStreamSize = static_cast<XUINT32>(totalFileSize);

    IFC_RETURN(XamlBinaryFormatSerializationHelper::DeserializeVectorFromNodeStream(m_vecMasterStreamIndex, GetVersion(), spNodeStreamStream));
    IFC_RETURN(spNodeStreamStream->GetPosition(&streamTableSize));
    m_cbStreamTableSize = static_cast<XUINT32>(streamTableSize);

    return S_OK;
}

_Check_return_ HRESULT
XamlBinaryFormatReader2::GetSubReader(
    _In_ const unsigned int streamIndex,
    _Out_ std::shared_ptr<XamlBinaryFormatSubReader2>& spBinaryReader)
{

    unsigned int sizeOfNodeStream = 0;
    unsigned int sizeOfLineStream = 0;

    // While ideally we'd be dealing with size_t everywhere, most of the XBF reader code
    // has an assumption baked in that an XBF file will never exceed 4GB by using 32-bit
    // pointers for offset calculations. We're using 64-bit values here simply to make some
    // of the implicit casts below work.
    uint64_t requestedOffsetIntoStream = 0;
    uint64_t requestedOffsetIntoLineInformation = 0;
    
    sizeOfNodeStream = m_vecMasterStreamIndex[streamIndex].m_lineStreamOffset - m_vecMasterStreamIndex[streamIndex].m_nodeStreamOffset;
    requestedOffsetIntoStream = m_vecMasterStreamIndex[streamIndex].m_nodeStreamOffset + m_cbStreamTableSize + m_cbNodeStreamStartOffset;
    requestedOffsetIntoLineInformation = m_vecMasterStreamIndex[streamIndex].m_lineStreamOffset + m_cbStreamTableSize + m_cbNodeStreamStartOffset;

    if ((streamIndex + 1) < m_vecMasterStreamIndex.size())
    {
        sizeOfLineStream = m_vecMasterStreamIndex[streamIndex + 1].m_nodeStreamOffset - m_vecMasterStreamIndex[streamIndex].m_lineStreamOffset;
    }
    else
    {
        sizeOfLineStream = m_cbNodeStreamSize - m_vecMasterStreamIndex[streamIndex].m_lineStreamOffset - m_cbStreamTableSize;
    }
    
    spBinaryReader = std::shared_ptr<XamlBinaryFormatSubReader2>(
        new XamlBinaryFormatSubReader2(
            shared_from_this(),
            // There's a loss of percision here, but this value will never fall outside the value of a unsigned
            // 32-bit integer.
            static_cast<unsigned int>(requestedOffsetIntoStream), 
            sizeOfNodeStream, sizeOfLineStream,
            static_cast<uint8_t*>(m_spXbfMemory->GetAddress()) + requestedOffsetIntoStream,
            static_cast<uint8_t*>(m_spXbfMemory->GetAddress()) + requestedOffsetIntoLineInformation));

    return S_OK;
}

xstring_ptr 
XamlBinaryFormatReader2::GetXbfHash() const
{
    return m_spXamlBinaryMetadataReader->GetXbfHash();
}

const XamlBinaryFormatReader2::StreamLengthRuntimeDataPair* 
    XamlBinaryFormatReader2::TryGetRuntimeData(unsigned int masterStreamOffset) const
{
    ASSERT(masterStreamOffset < m_cbNodeStreamSize + m_cbNodeStreamStartOffset);
    
    // Early return when we haven't even allocated the cached runtime data map.
    if (!m_cachedRuntimeData) return nullptr;

    const auto& result = m_cachedRuntimeData->find(masterStreamOffset);
    if (result != m_cachedRuntimeData->end())
    {
        return &((*result).second);
    }
    else
    {
        return nullptr;
    }
}

void XamlBinaryFormatReader2::SetRuntimeData(unsigned int masterStreamOffset, StreamLengthRuntimeDataPair data)
{
    ASSERT(masterStreamOffset < m_cbNodeStreamSize + m_cbNodeStreamStartOffset);
    
    if (!m_cachedRuntimeData)
    {
        m_cachedRuntimeData.reset(new containers::vector_map<unsigned int, StreamLengthRuntimeDataPair>());
    }
    (*m_cachedRuntimeData)[masterStreamOffset] = std::move(data);
}

XamlBinaryMetadataReader2& XamlBinaryFormatReader2::GetMetadataReader() const
{ 
    return *m_spXamlBinaryMetadataReader.get(); 
}

