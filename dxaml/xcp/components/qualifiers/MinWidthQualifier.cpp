// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MinWidthQualifier.h" 

bool MinWidthQualifier::IsQualified() 
{ 
    return m_qualified;
};

XINT32 MinWidthQualifier::Score(_In_ QualifierFlags flags) 
{ 
    if((flags & QualifierFlags::Width) != QualifierFlags::None)
    {
        return m_minWidth;
    }

    return -1;
}; 

void MinWidthQualifier::Evaluate(_In_ QualifierContext* qualifierContext) 
{
    m_qualified = (m_minWidth <= qualifierContext->m_windowWidth);
};

QualifierFlags MinWidthQualifier::Flags() { return QualifierFlags::Width; }; 

