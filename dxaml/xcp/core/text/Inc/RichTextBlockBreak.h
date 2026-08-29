// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef NO_HEADER_INCLUDES
#include "TextBreak.h"
#endif

class BlockNodeBreak;

//---------------------------------------------------------------------------
//
//  RichTextBlockBreak
//
//  Contains state at the point where a CRichTextBlock or CRichTextBlockOverflow
//  has stopped formatting due to breaking process.
//
//---------------------------------------------------------------------------
class RichTextBlockBreak : public RichTextServices::TextBreak
{
public:
    // Initializes instance of RichTextBlockBreak.
    RichTextBlockBreak(
        _In_opt_ BlockNodeBreak *pBlockBreak
        );

    BlockNodeBreak *GetBlockBreak() const;

private:

    // Information about where the last block was broken if it was partially formatted.
    BlockNodeBreak *m_pBlockBreak;

    ~RichTextBlockBreak() override;
};

//---------------------------------------------------------------------------
//
//  Member:
//      RichTextBlockBreak::GetBlockBreak
//
//---------------------------------------------------------------------------
inline BlockNodeBreak *RichTextBlockBreak::GetBlockBreak() const
{
    return m_pBlockBreak;
}
