// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XbfVersioning.h"

class ObjectWriterNodeList;

class XbfWriter
{        
public:
    XbfWriter(_In_ CCoreServices *pCore, _In_ const TargetOSVersion& targetOS)
        : m_pCore(pCore)
        , m_version(targetOS)
    {
    }

    _Check_return_
    HRESULT GetOptimizedBinaryEncodingFromReader(
        _In_ const std::shared_ptr<XamlReader>& spReader,
        _In_ const std::shared_ptr<ObjectWriterNodeList>& spObjectNodeList,
        _In_ const std::array<byte, Parser::c_xbfHashSize>& byteHashForBinaryXaml,
        _In_ bool fGenerateLineInfo,
        _Out_ XUINT32 *puiBinaryXaml,
        _Outptr_result_bytebuffer_(*puiBinaryXaml) XBYTE **ppBinaryXaml
        );
    
    _Check_return_ 
    HRESULT ProcessXamlTextBuffer(
        _In_reads_bytes_(cXamlTextBufferSize)            const XBYTE *pXamlTextBuffer,
        _In_                                        XUINT32      cXamlTextBufferSize,
        _Outptr_result_bytebuffer_(*pcXamlBinaryBufferSize) XBYTE      **ppXamlBinaryBuffer,
        _Out_                                       XUINT32     *pcXamlBinaryBufferSize
        );


private:     
    CCoreServices *m_pCore;
    const TargetOSVersion m_version;
};

