// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class XamlBinaryFormatSubReader2;

// XamlLineInfo can be used in one of two ways, and one of two ways only:
// 1. It is initialzed with a line and column number in which case the m_pSubReader will always be nullptr and stream offset is 0.
// 2. It is iniliazed with a stream offset and a weak reference to a m_pSubReader object and the line and column will always be 0.

// Scenario 1 is used when parsing Xaml text and storing the line info in the XBF stream
// Scenario 2 is used when parsing an XBF stream. The stream offset value is the offset in the line stream where 
// the line info for the current node is stored. We hold a weak reference to the m_pSubReader because the m_pSubReader
// has a strong reference to the XamlLineInfo. When the m_pSubReader get's destroyed it will clean up the XamlLineInfo.

class XamlLineInfo
{
private:
    unsigned int m_uiLineNumber;
    unsigned int m_uiLinePosition;
    unsigned int m_uiStreamOffset;
    XamlBinaryFormatSubReader2* m_pSubReader;

public:
    XamlLineInfo()
        : m_uiLineNumber(0)
        , m_uiLinePosition(0)
        , m_uiStreamOffset(0)
        , m_pSubReader(nullptr)
    {
    }

    XamlLineInfo(XamlBinaryFormatSubReader2 *pSubReader, unsigned int uiStreamOffset)
        : m_uiLineNumber(0)
        , m_uiLinePosition(0)
        , m_uiStreamOffset(uiStreamOffset)
        , m_pSubReader(pSubReader)
    {
    }

    XamlLineInfo(unsigned int uiLineNumber, unsigned int uiLinePosition)
        : m_uiLineNumber(uiLineNumber)
        , m_uiLinePosition(uiLinePosition)
        , m_uiStreamOffset(0)
        , m_pSubReader(nullptr)
    {
    }

    unsigned int LineNumber() const;
    unsigned int LinePosition() const;
};

