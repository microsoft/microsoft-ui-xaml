// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "WinReader.h"
#include <ReaderString.h>

namespace XmlReaderWrapper {
    std::unique_ptr<CWinReader> CreateLegacyXmlReaderWrapper()
    {
        std::unique_ptr<CWinReader> result;
        xref_ptr<IXmlReader> reader;
        if (SUCCEEDED(CreateXmlReader(__uuidof(IXmlReader), 
            reinterpret_cast<void**>(reader.ReleaseAndGetAddressOf()), nullptr)))
        {
            result.reset(new CWinReader(std::move(reader)));
        }
        return result;
    }
}

CWinReader::CWinReader(xref_ptr<IXmlReader> reader)
    : m_reader(std::move(reader))
{}

_Check_return_ HRESULT
CWinReader::Read(_Out_ XmlNodeType *pType)
{
    return m_reader->Read(pType);
}

_Check_return_ HRESULT
CWinReader::SetInput(
    _In_ unsigned int cBuffer,
    _In_reads_(cBuffer) const uint8_t *pBuffer,
    _In_ bool bForceUtf16)
{
    void *pvGlob;
    HGLOBAL hGlob;
    // Create an HGLOBAL large enough for the whole block and copy to it.
    hGlob = GlobalAlloc(GMEM_MOVEABLE, cBuffer);
    IFCOOMFAILFAST(hGlob);
    pvGlob = GlobalLock(hGlob);
    // We have to check for a NULL return from the GlobalLock because GlobalAlloc can give us
    // any old handle ptr for a GMEM_MOVEABLE request. We won't really know how valid the handle is
    // till we call lock. If the handle is garbage then we will get a NULL ptr back and can check there.
    IFCOOMFAILFAST(pvGlob);
    memcpy(pvGlob, pBuffer, cBuffer);
    GlobalUnlock(hGlob);

    // Now create a stream on it and set in the reader.
    IFC_RETURN(CreateStreamOnHGlobal(hGlob, TRUE, m_stream.ReleaseAndGetAddressOf()));
    
    // On Windows CE, the stream reader will continue reading off the end of the stream
    // unless we call IStream::SetSize() to force the logical stream size to the length
    // of the file just opened
    ULARGE_INTEGER streamSize;
    streamSize.LowPart = cBuffer;
    streamSize.HighPart = 0;
    IFC_RETURN(m_stream->SetSize(streamSize));

    xref_ptr<IXmlReaderInput> xmlReaderInput;
    if (bForceUtf16)
    {
        // utf-16 encoding is to be enforced.  This can be useful if we know the input
        // came from a unicode string, which would be in utf-16, and we don't want
        // parsing to fail if that string happens to include <?xml encoding="utf-8"?>
        IFC_RETURN(CreateXmlReaderInputWithEncodingName(
            m_stream.get(), nullptr, L"utf-16",
            FALSE, // indicates the utf-16 encoding is required
            nullptr,
            xmlReaderInput.ReleaseAndGetAddressOf()));
        IFC_RETURN(m_reader->SetInput(xmlReaderInput.get()));
    }
    else
    {
        IFC_RETURN(m_reader->SetInput(m_stream));
    }

    return S_OK;
}

// Get the namespace prefix of the current element or attribute
_Check_return_ HRESULT
CWinReader::GetPrefix(
    _Inout_ ReaderString *pReaderString)
{
    const wchar_t* pString = nullptr;
    unsigned int cString = 0;
    IFC_RETURN(m_reader->GetPrefix(&pString, &cString));
    pReaderString->SetString(cString, pString);
    return S_OK;
}

_Check_return_ HRESULT
CWinReader::GetNamespaceUri(
    _Inout_ ReaderString *pReaderString)
{
    const wchar_t* pString = nullptr;
    unsigned int cString = 0;
    IFC_RETURN(m_reader->GetNamespaceUri(&pString, &cString));
    pReaderString->SetString(cString, pString);
    return S_OK;
}

_Check_return_ HRESULT
CWinReader::GetLocalName(
    _Inout_ ReaderString *pReaderString)
{
    const wchar_t* pString = nullptr;
    unsigned int cString = 0;
    IFC_RETURN(m_reader->GetLocalName(&pString, &cString));
    pReaderString->SetString(cString, pString);
    return S_OK;
}

_Check_return_ HRESULT
CWinReader::GetValue(_Inout_ ReaderString *pReaderString)
{
    const wchar_t* pString = nullptr;
    unsigned int cString = 0;
    IFC_RETURN(m_reader->GetValue(&pString, &cString));
    pReaderString->SetString(cString, pString);
    return S_OK;
}

bool CWinReader::EmptyElement()
{
    return !!m_reader->IsEmptyElement();
}

_Check_return_ HRESULT
CWinReader::FirstAttribute()
{
    return m_reader->MoveToFirstAttribute();
}

_Check_return_ HRESULT
CWinReader::NextAttribute()
{
    return m_reader->MoveToNextAttribute();
}

_Check_return_ HRESULT
CWinReader::GetPosition(_Out_ unsigned int *pnLine, _Out_ unsigned int *pnColumn)
{
    IFC_RETURN(m_reader->GetLineNumber(pnLine));
    IFC_RETURN(m_reader->GetLinePosition(pnColumn));
    return S_OK;
}
