// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "CustomWriterRuntimeData.h"
#include "MemoryStreamBuffer.h"
#include "XamlBinaryFormatWriter2.h"
#include "XamlBinaryFormatSubWriter2.h"
#include "XamlBinaryFormatCommon.h"
#include "XamlSerializationHelper.h"
#include "ObjectWriterNodeList.h"
#include "ObjectWriterNode.h"
#include "ObjectWriterNodeType.h"

using namespace DirectUI;

_Check_return_ HRESULT XamlBinaryFormatWriter2::WriteAllNodes(
    _In_ const std::shared_ptr<ObjectWriterNodeList>& spObjectNodeList
    )
{
    std::vector< unsigned int > offsetList;
    xref_ptr<IPALStream> spNodeStreamStream;
    xref_ptr<IPALStream> spNodeStreamTableStream;
    xref_ptr<CMemoryStreamBuffer> spNodeStreamBuffer;
    xref_ptr<CMemoryStreamBuffer> spNodeStreamTableBuffer;

    IFC_RETURN(spNodeStreamBuffer.init(new CMemoryStreamBuffer((XUINT32)-1)));
    IFC_RETURN(spNodeStreamBuffer->CreateStream(spNodeStreamStream.ReleaseAndGetAddressOf()));

    IFC_RETURN(spNodeStreamTableBuffer.init(new CMemoryStreamBuffer((XUINT32)-1)));
    IFC_RETURN(spNodeStreamTableBuffer->CreateStream(spNodeStreamTableStream.ReleaseAndGetAddressOf()));

    IFC_RETURN(ProcessSubNodes(spObjectNodeList, offsetList));

    // it is unexpected to find an offset entry in the first substream
    ASSERT(offsetList.empty());

    ASSERT(m_spSubNodeStreamList.size() == m_spSubLineStreamList.size());
    for (XUINT32 i = 0; i < m_spSubNodeStreamList.size(); i++)
    {
        XUINT32 subNodeStreamOffset = 0;
        XUINT32 subLineStreamOffset = 0;
        
        // write the sub node stream out into the master stream
        IFC_RETURN(GetMasterStreamOffset(spNodeStreamStream, &subNodeStreamOffset));
        IFC_RETURN(WriteToMasterStream(spNodeStreamStream, m_spSubNodeStreamList[i]));

        // write the sub line stream out into the master stream
        IFC_RETURN(GetMasterStreamOffset(spNodeStreamStream, &subLineStreamOffset));
        IFC_RETURN(WriteToMasterStream(spNodeStreamStream, m_spSubLineStreamList[i]));

        PersistedXamlSubStream persistedSubStream;
        persistedSubStream.m_nodeStreamOffset = subNodeStreamOffset;
        persistedSubStream.m_lineStreamOffset = subLineStreamOffset;
        m_vecMasterSubStreamList.push_back(persistedSubStream);
    }

    // serialize the sub stream table 
    IFC_RETURN(XamlBinaryFormatSerializationHelper::SerializeVectorToNodeStream(m_vecMasterSubStreamList, GetVersion(), spNodeStreamTableStream));

    // pack it up into the master stream
    IFC_RETURN(WriteToMasterStream(m_spMasterNodeStream, spNodeStreamTableStream));
    IFC_RETURN(WriteToMasterStream(m_spMasterNodeStream, spNodeStreamStream));
    
    return S_OK;
}

_Check_return_ HRESULT XamlBinaryFormatWriter2::WriteToMasterStream(
    _In_ xref_ptr<IPALStream> spMasterStream,
    _In_ xref_ptr<IPALStream> spStream)
{
    XUINT64 uiSize = 0;
    XUINT32 subStreamSize = 0;

    IFC_RETURN(spStream->GetSize(&uiSize));
    if (uiSize == (XUINT32)-1)
    {
        uiSize = 0;
    }
    subStreamSize = static_cast<XUINT32>(uiSize);

    {
        XUINT32 cbRead = 0;
        std::unique_ptr<XBYTE[]> spBinaryBuffer(new XBYTE[subStreamSize]);

        IFC_RETURN(spStream->Read(spBinaryBuffer.get(), subStreamSize, nullptr));
        IFC_RETURN(spMasterStream->Write(spBinaryBuffer.get(), subStreamSize, 0, &cbRead));
        if (cbRead != subStreamSize)
        {
            IFC_RETURN(E_FAIL);
        }
    }

    return S_OK;
}

_Check_return_ HRESULT XamlBinaryFormatWriter2::GetMasterStreamOffset(
    _In_ xref_ptr<IPALStream> spMasterStream,
    _Out_ XUINT32 *pOffset)
{
    XUINT64 offset = 0;

    IFC_RETURN(spMasterStream->GetSize(&offset));

    // This is a bit weird how it doesn't return 0 before
    // the first entry is written.
    if (offset == (XUINT32)-1)
    {
        offset = 0;
    }
    *pOffset = static_cast<XUINT32>(offset);

    return S_OK;
}

_Check_return_ HRESULT XamlBinaryFormatWriter2::ProcessSubNodes(
    _In_ const std::shared_ptr<ObjectWriterNodeList>& spObjectNodeList,
    _Out_ std::vector<unsigned int>& streamOffsetList)
{
    // remember this subsegment by adding to the sub lists
    std::shared_ptr<XamlBinaryFormatSubWriter2> spSubWriter = 
        std::make_shared<XamlBinaryFormatSubWriter2>(m_pCore, m_spMetadataStore, m_fGenerateLineInfo);
    IFC_RETURN(spSubWriter->Initialize());
    IFC_RETURN(m_spSubNodeStreamList.push_back(spSubWriter->GetNodeStream()));
    IFC_RETURN(m_spSubLineStreamList.push_back(spSubWriter->GetLineStream()));

    // now process all the nodes in the substream
    for (auto& node : spObjectNodeList->GetNodeList())
    {
        // send to the subwriter
        IFC_RETURN(spSubWriter->PersistNode(node));

        // handle offset marker tokens
        if (node.GetNodeType() == ObjectWriterNodeType::StreamOffsetMarker)
        {
            XUINT64 streamOffset = 0;
            XUINT32 uiOffset = 0;
            IFC_RETURN(spSubWriter->GetNodeStreamOffset(&streamOffset));
            uiOffset = static_cast<XUINT32>(streamOffset);
            streamOffsetList.push_back(uiOffset);
        }
        // special case handling here for segmented nodes
        else if (node.ProvidesCustomBinaryData())
        {
            std::vector<unsigned int> streamOffsetTable;
            // TODO: maybe safer to have ProcessSubNodes return this value?
            XUINT32 targetStreamIndex = m_spSubNodeStreamList.size(); 

            // process the target substream
            IFC_RETURN(ProcessSubNodes(node.GetNodeList(), streamOffsetTable));
            // Standard Blob Header Signature
            // [TargetSubStreamIndex],[ReferencesCount],[Array of StringId Reference Keys]
            IFC_RETURN(spSubWriter->PersistConstant(targetStreamIndex));

            // persist the reference resources
            IFC_RETURN(PersistReferencedResourceList(node.GetListOfReferences(), spSubWriter));         

            if (node.GetNodeType() == ObjectWriterNodeType::SetCustomRuntimeData)
            {
                IFC_RETURN(node.GetCustomWriterData()->Serialize(spSubWriter.get(), streamOffsetTable));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT XamlBinaryFormatWriter2::PersistReferencedResourceList(
    _In_ const std::vector<std::pair<bool, xstring_ptr>>& listOfResources,
    _In_ const std::shared_ptr<XamlBinaryFormatSubWriter2>& spSubWriter)
{
    XUINT32 staticResourceReferencesCount = 0;
    XUINT32 themeResourceReferencesCount = 0;

    // count number of static and theme resources
    for (auto& resourceReference : listOfResources)
    {
        auto isStaticResource = resourceReference.first;
        if (isStaticResource)
        {
            staticResourceReferencesCount++;
        }
        else
        {
            themeResourceReferencesCount++;
        }
    }

    // [StaticResourceCount,ThemeResourceCount]..[Array]
    IFC_RETURN(spSubWriter->PersistConstant(staticResourceReferencesCount));
    IFC_RETURN(spSubWriter->PersistConstant(themeResourceReferencesCount));
    for (auto& resourceReference : listOfResources)
    {
        auto isStaticResource = resourceReference.first;
        if (isStaticResource)
        {
            IFC_RETURN(spSubWriter->PersistSharedString(resourceReference.second));
        }
    }

    for (auto& resourceReference : listOfResources)
    {
        auto isThemeResource = !resourceReference.first;
        if (isThemeResource)
        {
            IFC_RETURN(spSubWriter->PersistSharedString(resourceReference.second));
        }
    }

    return S_OK;
}

