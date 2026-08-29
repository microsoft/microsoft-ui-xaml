// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Generates rendering instructions for ParagraphNode.

#pragma once

#include "DrawingContext.h"
#include "ParagraphNode.h"

class D2DTextDrawingContext;

//---------------------------------------------------------------------------
//
//  ParagraphDrawingContext
//
//  Generates rendering instructions for ParagraphNode.
//
//---------------------------------------------------------------------------
class ParagraphDrawingContext : public DrawingContext
{
public:
    // Initializes a new instance of the ParagraphDrawingContext class.
    ParagraphDrawingContext(
        _In_ CCoreServices *pCore,
        _In_ ParagraphNode *pNode
        );

    // Release resources associated with the ParagraphDrawingContext.
    virtual ~ParagraphDrawingContext();

    // Renders content in PC rendering walk.
    _Check_return_ HRESULT HWRenderContent(
        _In_ IContentRenderer* pContentRenderer
        ) override;

    // Renders content using D2D.
    _Check_return_ HRESULT D2DEnsureResources(
        _In_ const D2DPrecomputeParams &cp,
        _In_ const CMILMatrix *pRenderTransform
        ) override;

    _Check_return_ HRESULT D2DRenderContent(
        _In_ const SharedRenderParams &sharedRP,
        _In_ const D2DRenderParams &d2dRP,
        _In_ XFLOAT opacity
        ) override;

    // Gets the TextDrawingContext which is capable of rendering text lines.
    _Check_return_ HRESULT GetTextDrawingContext(
        _Outptr_ RichTextServices::TextDrawingContext **ppTextDrawingContext
        );

    void InvalidateRenderCache() override;

    void ClearForegroundHighlightInfo() override;

    void AppendForegroundHighlightInfo(
        uint32_t count,
        _In_reads_(count) const XRECTF *pRectangles,
        _In_ CSolidColorBrush* selectionForegroundBrush,
        uint32_t startIndex, // Index of first rect within the array that corresponds to this node.
        _In_ const XPOINTF& nodeOffset, // Offset of the node in page space, used to compute relative offsets within a node.
        _Out_ uint32_t* pAdvanceCount // Number of rectangles contained in node's render space.
        ) override;

    void CleanupRealizations() override;

    void SetBackPlateConfiguration(bool backPlateActive, bool useHyperlinkForeground) override;

    void SetControlEnabled(bool enabled) override;

private:

    // Pointer to core services.
    CCoreServices *m_pCore;

    // TextDrawingContext which is capable of rendering text lines.
    D2DTextDrawingContext *m_pTextDrawingContext;

    // Paragraph node associated with the drawing context.
    ParagraphNode *m_pNode;
};

