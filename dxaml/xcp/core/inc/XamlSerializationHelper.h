// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlBinaryMetadata.h"

#ifndef EXP_CLANG
    // MSVC happily accepts static template specializations.
    #define STATIC_SPEC static
#else
    // Clang does not and emits a warning.
    #define STATIC_SPEC
#endif

namespace XamlBinaryFormatSerializationHelper
{
    struct VectorData
    {
        VectorData()
            : uiCount(0)
        {
        }
        
        UINT32 uiCount;
    };

    // SerializeItemToStream() are called by XamlBinaryFormatWriter to serialize the items into stream
    // which then saved into BinaryXaml 

    _Check_return_ static HRESULT SerializeItemToMetadataStream(_In_ const Parser::XamlBinaryFileVersion& source, _In_ IPALStream* pTargetStream)
    {
        UINT32 cbWritten = 0;
        IFC_RETURN(pTargetStream->Write(&source, sizeof(Parser::XamlBinaryFileVersion), 0, &cbWritten));
        IFCCHECK_RETURN(cbWritten == sizeof(Parser::XamlBinaryFileVersion));
        return S_OK;
    }

    template<typename T>
    _Check_return_ static HRESULT SerializeItemToNodeStream(_In_ const T& itemSource, _In_ const Parser::XamlBinaryFileVersion&, _In_ IPALStream* pTargetStream)
    {
        UINT32 cbWritten = 0;
        IFC_RETURN(pTargetStream->Write(&itemSource, sizeof(T), 0, &cbWritten));
        IFCCHECK_RETURN(cbWritten == sizeof(T));
        return S_OK;
    }
    template<typename T>
    _Check_return_ static HRESULT SerializeItemToMetadataStream(_In_ const T& itemSource, _In_ const Parser::XamlBinaryFileVersion& version, _In_ IPALStream* pTargetStream)
    {
        return SerializeItemToNodeStream(itemSource, version, pTargetStream);
    }

    // specialization for xstring_ptr
    template<>
    _Check_return_ STATIC_SPEC HRESULT SerializeItemToNodeStream(_In_ const xstring_ptr& strItemSource, _In_ const Parser::XamlBinaryFileVersion& version, _In_ IPALStream* pTargetStream)
    {
        UINT32 cch = strItemSource.GetCount();
        IFC_RETURN(SerializeItemToNodeStream(cch, version, pTargetStream));

        const WCHAR* pch = strItemSource.GetBuffer();
        UINT32 cbWritten = 0;
        IFC_RETURN(pTargetStream->Write(pch, sizeof(WCHAR) * cch, 0, &cbWritten));
        IFCCHECK_RETURN(cbWritten == sizeof(WCHAR)* cch);

        return S_OK;
    }
    template<>
    _Check_return_ STATIC_SPEC HRESULT SerializeItemToMetadataStream(_In_ const xstring_ptr& strItemSource, _In_ const Parser::XamlBinaryFileVersion& version, _In_ IPALStream* pTargetStream)
    {
        IFC_RETURN(SerializeItemToNodeStream(strItemSource, version, pTargetStream));
        // For XBFv2_1, we append a null terminator
        if (version.ShouldNullTerminateStrings())
        {
            WCHAR nullTerminator = 0;
            IFC_RETURN(SerializeItemToMetadataStream(nullTerminator, version, pTargetStream));
        }
        return S_OK;
    }
    // DeserializeItemFromStream() are called by XamlBinaryFormatReader to deserialize the items into stream
    // which were earlier saved into BinaryXaml 

    _Check_return_ static HRESULT DeserializeItemFromMetadataStream(_Out_ Parser::XamlBinaryFileVersion* pTarget, _In_ IPALStream* pSourceStream)
    {
        UINT32 cbRead = 0;
        IFC_RETURN(pSourceStream->Read(pTarget, sizeof(Parser::XamlBinaryFileVersion), &cbRead));
        IFCCHECK_RETURN(cbRead == sizeof(Parser::XamlBinaryFileVersion));
        return S_OK;
    }

    template<typename T>
    _Check_return_ static HRESULT DeserializeItemFromNodeStream(_Out_ T* pItemTarget, _In_ const Parser::XamlBinaryFileVersion&, _In_ IPALStream* pSourceStream)
    {
        UINT32 cbRead = 0;
        IFC_RETURN(pSourceStream->Read(pItemTarget, sizeof(T), &cbRead));
        IFCCHECK_RETURN(cbRead == sizeof(T));
        return S_OK;
    }
    template<typename T>
    _Check_return_ static HRESULT DeserializeItemFromMetadataStream(_Out_ T* pItemTarget, _In_ const Parser::XamlBinaryFileVersion& version, _In_ IPALStream* pSourceStream)
    {
        return DeserializeItemFromNodeStream(pItemTarget, version, pSourceStream);
    }
    // specialization for xstring_ptr
    template<>
    _Check_return_ STATIC_SPEC HRESULT DeserializeItemFromNodeStream(_Out_ xstring_ptr* pstrItemTarget, _In_ const Parser::XamlBinaryFileVersion& version, _In_ IPALStream* pSourceStream)
    {
        UINT32 cch = 0;
        IFC_RETURN(DeserializeItemFromNodeStream(&cch, version, pSourceStream));
        IFCEXPECT_RETURN(cch < (XUINT32_MAX / sizeof(WCHAR)));

        WCHAR* pch = nullptr;
        XStringBuilder bufferBuilder;
        IFC_RETURN(bufferBuilder.InitializeAndGetFixedBuffer(cch, &pch));

        UINT32 cbRead = 0;
        IFC_RETURN(pSourceStream->Read(pch, sizeof(WCHAR) * cch, &cbRead));
        IFCCHECK_RETURN(cbRead == sizeof(WCHAR) * cch);

        IFC_RETURN(bufferBuilder.DetachString(pstrItemTarget));

        return S_OK;
    }
    template<>
    _Check_return_ STATIC_SPEC HRESULT DeserializeItemFromMetadataStream(_Out_ xstring_ptr* pstrItemTarget, _In_ const Parser::XamlBinaryFileVersion& version, _In_ IPALStream* pSourceStream)
    {
        IFC_RETURN(DeserializeItemFromNodeStream(pstrItemTarget, version, pSourceStream));
        // For XBFv2_1, we append a null terminator
        if (version.ShouldNullTerminateStrings())
        {
            WCHAR nullTerminator;
            IFC_RETURN(DeserializeItemFromMetadataStream(&nullTerminator, version, pSourceStream));
        }
        return S_OK;
    }
    
    template<typename Vec>
    _Check_return_ static HRESULT SerializeVectorToNodeStream(_In_ const Vec& vecSource, _In_ const Parser::XamlBinaryFileVersion& version, _In_ IPALStream* pTargetStream)
    {
        VectorData vectorData;
        vectorData.uiCount = vecSource.size();
        IFC_RETURN(SerializeItemToNodeStream(vectorData, version, pTargetStream));

        for (const auto& elem : vecSource)
        {
            IFC_RETURN(SerializeItemToNodeStream(elem, version, pTargetStream));
        }
        return S_OK;
    }
    template<typename Vec>
    _Check_return_ static HRESULT SerializeVectorToMetadataStream(_In_ const Vec& vecSource, _In_ const Parser::XamlBinaryFileVersion& version, _In_ IPALStream* pTargetStream)
    {
        VectorData vectorData;
        vectorData.uiCount = vecSource.size();
        IFC_RETURN(SerializeItemToMetadataStream(vectorData, version, pTargetStream));

        for (const auto& elem : vecSource)
        {
            IFC_RETURN(SerializeItemToMetadataStream(elem, version, pTargetStream));
        }
        return S_OK;
    }

    template<typename Vec>
    _Check_return_ static HRESULT DeserializeVectorFromNodeStream(_Out_ Vec& vecTarget, _In_ const Parser::XamlBinaryFileVersion& version, _In_ IPALStream* pSourceStream)
    {
        IFCEXPECT_RETURN(vecTarget.size() == 0);

        VectorData vectorData;
        IFC_RETURN(DeserializeItemFromNodeStream(&vectorData, version, pSourceStream));
        
        vecTarget.reserve(vectorData.uiCount);
        
        for (XUINT32 uiCount = vectorData.uiCount; uiCount > 0; uiCount--)
        {
            typename Vec::value_type data;
            IFC_RETURN(DeserializeItemFromNodeStream(&data, version, pSourceStream));
            vecTarget.push_back(std::move(data));
        }
        return S_OK;
    }
    template<typename Vec>
    _Check_return_ static HRESULT DeserializeVectorFromMetadataStream(_Out_ Vec& vecTarget, _In_ const Parser::XamlBinaryFileVersion& version, _In_ IPALStream* pSourceStream)
    {
        IFCEXPECT_RETURN(vecTarget.size() == 0);

        VectorData vectorData;
        IFC_RETURN(DeserializeItemFromMetadataStream(&vectorData, version, pSourceStream));

        vecTarget.reserve(vectorData.uiCount);

        for (XUINT32 uiCount = vectorData.uiCount; uiCount > 0; uiCount--)
        {
            typename Vec::value_type data;
            IFC_RETURN(DeserializeItemFromMetadataStream(&data, version, pSourceStream));
            vecTarget.push_back(std::move(data));
        }
        return S_OK;
    }
};
