// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MinHeightQualifier.h" 

bool MinHeightQualifier::IsQualified() 
{ 
    return m_qualified;
};

XINT32 MinHeightQualifier::Score(_In_ QualifierFlags flags) 
{ 
    if((flags & QualifierFlags::Height) != QualifierFlags::None)
    {
        return m_minHeight;
    }

    return -1;
}; 

void MinHeightQualifier::Evaluate(_In_ QualifierContext* qualifierContext) 
{
    m_qualified = (m_minHeight <= qualifierContext->m_windowHeight);
};

QualifierFlags MinHeightQualifier::Flags() { return QualifierFlags::Height; }; 

