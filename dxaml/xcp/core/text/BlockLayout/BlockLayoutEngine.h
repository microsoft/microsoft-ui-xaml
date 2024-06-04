// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class BlockNode;
class BlockNodeBreak;

namespace RichTextServices
{
    struct ITextBackingStore;
    class TextFormatter;
}

//---------------------------------------------------------------------------
//
//  BlockLayoutEngine
//
//---------------------------------------------------------------------------
class BlockLayoutEngine
{
public:
    BlockLayoutEngine(
        _In_ CDependencyObject *pOwner
        );

    ~BlockLayoutEngine();

    _Check_return_ HRESULT CreatePageNode(
        _In_opt_ CBlockCollection *pBlocks,
        _In_ CFrameworkElement *pPageOwner,
        _Outptr_ BlockNode **ppPageNode
        );

    CDependencyObject * GetOwner() const;

private:
    CDependencyObject *m_pOwner;
};

inline CDependencyObject *BlockLayoutEngine::GetOwner() const
{
    return m_pOwner;
}
