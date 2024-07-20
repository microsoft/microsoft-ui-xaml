// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CParserErrorEventArgs final : public CErrorEventArgs
{

public:

    xstring_ptr  m_strXamlFileName;          // Xaml file name.
    xstring_ptr  m_strXmlElement;            // The name of the element tag in the xaml file.
    xstring_ptr  m_strXmlAttribute;          // The Attribute name in the xaml file.

    XUINT32     m_uLineNumber;            // Line number in xaml file where syntactic error happens.
    XUINT32     m_uCharPosition;          // Charactor position in the error line where the error occurs.

    CParserErrorEventArgs(_In_ CCoreServices* pCore) : CErrorEventArgs(pCore)
    {
        m_eType = ParserError;
    }

    // Destructor
    ~CParserErrorEventArgs() override
    {
    }
};
