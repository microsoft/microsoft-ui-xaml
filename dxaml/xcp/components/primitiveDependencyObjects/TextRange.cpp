// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <depends.h>
#include <CreateParameters.h>
#include "TextRange.h"

_Check_return_ HRESULT
CTextRange::Create(
    _Outptr_ CDependencyObject** objectOut,
    _In_ CREATEPARAMETERS* create
    )
{
    xref_ptr<CTextRange> textRange(new CTextRange(create->m_pCore));

    switch (create->m_value.GetType())
    {
    case valueObject:
        {
            auto origTextRange = checked_cast<CTextRange>(create->m_value);

            if (origTextRange)
            {
                textRange->m_range = origTextRange->m_range;
            }
        }
        break;

    case valueTextRange:
        textRange->m_range = create->m_value.As<valueTextRange>();
        break;

    default:
        // Create will work, but is not written with expectation of other valueTypes
        ASSERT(create->m_value.GetType() == valueAny);
        break;
    }

    *objectOut = textRange.detach();

    return S_OK;
}