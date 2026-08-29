// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlBinaryFormatSubReader2.h"
#include "LineInfo.h"

unsigned int XamlLineInfo::LineNumber() const
{
    if (m_pSubReader)
    {
        return m_pSubReader->ResolveLineInfo(m_uiStreamOffset).LineNumber();
    }

    return m_uiLineNumber; 
}

unsigned int XamlLineInfo::LinePosition() const
{
   if (m_pSubReader)
   {
        return m_pSubReader->ResolveLineInfo(m_uiStreamOffset).LinePosition();
   }
   
   return m_uiLinePosition;
}