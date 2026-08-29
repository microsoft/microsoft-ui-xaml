// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ExtensibleQualifier.h"

ExtensibleQualifier::ExtensibleQualifier(_In_ const bool* pTriggered) :
            m_pTriggered(pTriggered)
{ };

bool ExtensibleQualifier::IsQualified()
{
    return m_pTriggered && *m_pTriggered;
};

XINT32 ExtensibleQualifier::Score(_In_ QualifierFlags flags)
{
    if(flags == QualifierFlags::Extensible) return 1;
    else if(flags == QualifierFlags::Identifier) return static_cast<XINT32>(reinterpret_cast<uintptr_t>(m_pTriggered));

    return -1;
};

void ExtensibleQualifier::Evaluate(_In_ QualifierContext* qualifierContext)
{
    (qualifierContext);
};

QualifierFlags ExtensibleQualifier::Flags() { return QualifierFlags::Extensible; };

