// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <xmllite.h>

class ReaderString;
class CWinReader;

namespace XmlReaderWrapper {
    std::unique_ptr<CWinReader> CreateLegacyXmlReaderWrapper();
}

class CWinReader 
{
public:
    explicit CWinReader(xref_ptr<IXmlReader> reader);

    _Check_return_ HRESULT SetInput(_In_ unsigned int cBuffer, _In_reads_(cBuffer) const uint8_t *pBuffer, _In_ bool bForceUtf16);
    _Check_return_ HRESULT Read(_Out_ XmlNodeType *pType);
    _Check_return_ HRESULT GetPrefix(_Inout_ ReaderString *pReaderString);
    _Check_return_ HRESULT GetNamespaceUri(_Inout_ ReaderString *pReaderString);
    _Check_return_ HRESULT GetLocalName(_Inout_ ReaderString *pReaderString);
    _Check_return_ HRESULT GetValue(_Inout_ ReaderString *pReaderString);
    bool EmptyElement();
    _Check_return_ HRESULT FirstAttribute();
    _Check_return_ HRESULT NextAttribute();
    _Check_return_ HRESULT GetPosition(_Out_ unsigned int *pnLine, _Out_ unsigned int *pnColumn);

private:
    xref_ptr<IStream> m_stream;
    xref_ptr<IXmlReader> m_reader;
};
