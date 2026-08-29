// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "BlockNodeBreak.h"

namespace RichTextServices
{
    class TextLineBreak;
    class TextRunCache;
}

//---------------------------------------------------------------------------
//
//  ParagraphNodeBreak
//
//  Contains state at the point where a ParagraphNode is broken.
//
//---------------------------------------------------------------------------
class ParagraphNodeBreak : public BlockNodeBreak
{
public:
    ParagraphNodeBreak(
        _In_ XUINT32 breakIndex,
        _In_opt_ RichTextServices::TextLineBreak *pLineBreak,
        _In_ RichTextServices::TextRunCache *pRunCache
        );

    // Gets the line break where the paragraph was broken.
    RichTextServices::TextLineBreak* GetLineBreak() const;

    // Gets the run cache for the paragraph.
    RichTextServices::TextRunCache* GetRunCache() const;

    // Equality operator - BlockNodeBreak override.
    bool Equals(_In_opt_ BlockNodeBreak *pBreak);

private:
    // Line breaking information for the last formatted line if it exists.
    RichTextServices::TextLineBreak *m_pLineBreak;

    // Run cache.
    RichTextServices::TextRunCache *m_pRunCache;

    virtual
    ~ParagraphNodeBreak();
};

//---------------------------------------------------------------------------
//
//  Member:
//      ParagraphNodeBreak ::GetLineBreak
//
//  Returns:
//      Line break associated with this paragraph break.
//
//---------------------------------------------------------------------------
inline RichTextServices::TextLineBreak* ParagraphNodeBreak::GetLineBreak() const
{
    return m_pLineBreak;
}

//---------------------------------------------------------------------------
//
//  Member:
//      ParagraphNodeBreak::GetRunCache
//
//  Returns:
//      Run cache for the paragraph.
//
//---------------------------------------------------------------------------
inline RichTextServices::TextRunCache* ParagraphNodeBreak::GetRunCache() const
{
    return m_pRunCache;
}

