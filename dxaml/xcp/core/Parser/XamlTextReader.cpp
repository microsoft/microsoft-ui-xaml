// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      TextReader allows a consumer of XamlNodes treat the XamlPullParser
//      as a XamlReader.

#include "precomp.h"
#include <WinReader.h>

XamlTextReader::~XamlTextReader()
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a new instance of a XamlTextReader, and returns an std::shared_ptr
//      to that new instance.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
XamlTextReader::Create(
    _In_ std::shared_ptr<XamlSchemaContext> spXamlSchemaContext,
    _In_ const XamlTextReaderSettings& textReaderSettings,
    _In_ XUINT32 cSource,                           
    _In_reads_bytes_(cSource) const XUINT8 *pSource,           
    _Out_ std::shared_ptr<XamlTextReader>& spXamlTextReader
    )
{
    // XMLLite doesn't properly handle leading whitespace, so we need to strip
    // it off before we begin parsing
    IFC_RETURN(StripLeadingWhitespaceForXmlLiteParser(cSource, pSource, &cSource, &pSource));

    auto reader = XmlReaderWrapper::CreateLegacyXmlReaderWrapper();
    IFC_RETURN(reader->SetInput(cSource, pSource, textReaderSettings.get_IsUtf16Encoded()));

    std::shared_ptr<XamlParserContext> spParserContext;
    IFC_RETURN(XamlParserContext::Create(spXamlSchemaContext, spParserContext));

    auto scanner = std::make_shared<XamlScanner>(spParserContext, std::move(reader), textReaderSettings);
    IFC_RETURN(scanner->Init());

    auto parser = std::make_shared<XamlPullParser>(spParserContext, scanner);
    spXamlTextReader = std::make_shared<XamlTextReader>(spXamlSchemaContext, parser);
    
    return S_OK;
}

_Check_return_ HRESULT
XamlTextReader::Read()
{
    RRETURN(m_spXamlPullParser->Parse(m_currentXamlNode));
}


const XamlNode&
XamlTextReader::CurrentNode()
{
    return m_currentXamlNode;
}

_Check_return_ HRESULT 
XamlTextReader::GetSchemaContext(std::shared_ptr<XamlSchemaContext>& outSchemaContext)
{
    outSchemaContext = m_spXamlSchemaContext.lock();
    if (!outSchemaContext)
    {
        IFC_RETURN(E_FAIL);
    }
    return S_OK;
}


_Check_return_ HRESULT 
XamlTextReader::StripLeadingWhitespaceForXmlLiteParser(
    _In_ XUINT32 cSource,
    _In_reads_(cSource) const XUINT8* pSource,
    _Out_ XUINT32* pcResult,
    _Outptr_result_buffer_(*pcResult) const XUINT8** ppResult
    )
{
    HRESULT      hr          = S_OK;
    XINT32       bUnicode    = 0;   // True for UNICODE/UTF-16 files.
    XINT32       nEndian     = 0;    // Undecided, little, or big
    const WCHAR *pTempSource = NULL;

// Our XML reader doesn't like leading whitespace, so it fails out at the beginning
// of parsing.  Worse still, we don't know if the file is UTF-8 or UTF-16 nor the
// endian nature of the file.  Time for some detective work.  First, UTF-16 files
// must have an even number of bytes.

    bUnicode = !(cSource & 1);
    nEndian = 0;

// If the file could be UNICODE look for valid BOM, or allowed white space characters
// in either byte order.

    if (bUnicode)
    {
        pTempSource = reinterpret_cast<const WCHAR*>(pSource);

        while ((nEndian <= 0) && (cSource > 2))
        {
            switch (*pTempSource)
            {
            case 0x0020:        // Could be either big or little endian but it is a space
            case 0x2000:        // Could be either big or little endian but it is a space
            case 0xfeff:        // Could be a BOM or zero width no-break space.
            case 0xfffe:        // Could be a BOM or zero width no-break space.
                nEndian = -1;   // We must be UNICODE but we don't know the endian type
                pTempSource++;  // Examine the next UTF-16 character
                cSource = cSource - 2;
                break;

            case 0x0009:
            case 0x000a:
            case 0x000d:
            case 0x00a0:
                nEndian = 1;    // We're UNICODE and little endian
                break;

            case 0x0900:        // We're UNICODE and big endian
            case 0x0a00:
            case 0x0d00:
            case 0xa000:
                nEndian = 2;
                break;

            default:            // Not a possible UTF-16 space character
                if (!nEndian)   // If we didn't have a previous ambiguous space check for ANSI
                    bUnicode = FALSE;

                nEndian = 3;    // We're UNICODE but we don't know the endian type.
                break;
            }
        }

        pSource = reinterpret_cast<const XUINT8*>(pTempSource);
    }

// At this point if we know we're UNICODE and we know if we're big or little endian
// then we can consume additional whitespace characters.  If we might be ANSI then
// we have to check those before proceeding.

    if (bUnicode)
    {
        WCHAR   swap;

        switch(nEndian)
        {
        case 1:     // Little endian
#pragma warning (push)
// Description: Disable prefast  warning 26014: Potential read overflow using expression '* pTempSource' 
//              This is likely to be due to incorrect or insufficient validation of the buffer access 
// Reason     : Ignore prefast warning about cSource being an odd number. We have
//              verified that cSource is even (bUnicode is true).
#pragma warning (disable : 26014)            
            while ((cSource > 0) && (xisspace(*pTempSource)))
            {
                pTempSource++;
                cSource = cSource - 2;
            }
#pragma warning (pop)
            pSource = reinterpret_cast<const XUINT8*>(pTempSource);
            break;

        case 2:     // Big endian
            while (cSource > 0)
            {
#pragma warning (push)
// Description: Disable prefast  warning 26014: Potential read overflow using expression '(* pSource)[1]'.
//              This is likely to be due to incorrect or insufficient validation of the buffer access 
// Reason     : bUnicode is true which means cSource is an even number. So since cSource > 0 then
//              it has to be >= 2.
#pragma warning (disable : 26014) 
                swap = pSource[1] | (pSource[0] << 8);
#pragma warning (pop)
                if (!xisspace(swap))
                    break;

                pSource = pSource + 2;
                cSource = cSource - 2;
            }
        }
    }
    else
    {
        WCHAR   chTmp;

        while (cSource > 0)
        {
            chTmp = WCHAR(pSource[0]);
            if (!xisspace(chTmp))
                break;

            pSource = pSource + 1;
            cSource = cSource - 1;
        }
    }

    *pcResult = cSource;
    *ppResult = pSource;

    return hr;
}

