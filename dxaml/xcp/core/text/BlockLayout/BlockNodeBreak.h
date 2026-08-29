// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TextBreak.h"

//---------------------------------------------------------------------------
//
//  BlockNodeBreak
//
//  Contains state at the point where a block is broken.
//
//---------------------------------------------------------------------------
class BlockNodeBreak : public RichTextServices::TextBreak
{
public:
    XUINT32 GetBreakIndex() const;

    // Equality operator - used in layout bypass.
    virtual bool Equals(_In_opt_ BlockNodeBreak *pBreak);

    static bool Equals(
        _In_opt_ BlockNodeBreak *pFirst,
        _In_opt_ BlockNodeBreak *pSecond
        );

protected:
    BlockNodeBreak(
        _In_ XUINT32 breakIndex
        );
    
    ~BlockNodeBreak() override;

private:
    // Index of break relative to block start.
    XUINT32 m_breakIndex;
};

//---------------------------------------------------------------------------
//
//  Member:
//      BlockNodeBreak::GetBreakIndex
//
//  Returns:
//      Index of break relative to block start.
//
//---------------------------------------------------------------------------
inline XUINT32 BlockNodeBreak::GetBreakIndex() const
{
    return m_breakIndex;
}

