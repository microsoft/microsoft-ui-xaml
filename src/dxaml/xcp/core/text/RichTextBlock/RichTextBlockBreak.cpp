// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "BlockNodeBreak.h"

//---------------------------------------------------------------------------
//
//  Member:
//      RichTextBlockBreak::RichTextBlockBreak
//
//---------------------------------------------------------------------------
RichTextBlockBreak::RichTextBlockBreak(
    _In_opt_ BlockNodeBreak *pBlockBreak
        // Break record for last block.
    ) :
    m_pBlockBreak(pBlockBreak)
{
    AddRefInterface(m_pBlockBreak);
}

//---------------------------------------------------------------------------
//
//  Member:
//      RichTextBlockBreak::~RichTextBlockBreak
//
//---------------------------------------------------------------------------
RichTextBlockBreak::~RichTextBlockBreak()
{
    ReleaseInterface(m_pBlockBreak);
}
