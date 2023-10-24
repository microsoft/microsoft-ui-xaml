// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Base interface for drawing contexts supporting PC, D2D and SW rendering.

#pragma once

//---------------------------------------------------------------------------
//
//  DrawingContext
//
//  Base interface for drawing contexts supporting PC, D2D and SW rendering.
//
//---------------------------------------------------------------------------
class DrawingContext
{
public:
    // Release resources associated with the DrawingContext.
    virtual ~DrawingContext();

    // Renders content in PC rendering walk.
    virtual _Check_return_ HRESULT HWRenderContent(
        _In_ IContentRenderer* pContentRenderer
        ) = 0;

    // Renders content using D2D.
    virtual _Check_return_ HRESULT D2DEnsureResources(
        _In_ const D2DPrecomputeParams &cp,
        _In_ const CMILMatrix *pRenderTransform
        ) = 0;

    virtual _Check_return_ HRESULT D2DRenderContent(
        _In_ const SharedRenderParams &sharedRP,
        _In_ const D2DRenderParams &d2dRP,
        _In_ XFLOAT opacity
        ) = 0;

    virtual void InvalidateRenderCache() = 0;

    // Sets/Gets transform applied to the content of the drawing context.
    const CMILMatrix & GetTransform() const;
    void SetTransform(_In_ const CMILMatrix &transform);

    virtual void ClearForegroundHighlightInfo() = 0;

    // Sets information about highlight rects so that ParagraphDrawingContext
    // can use this to render glyphs with appropriate foreground in high contrast themes.
    // This includes text selection
    virtual void AppendForegroundHighlightInfo(
        uint32_t count, // Total count of highlight rects.
        _In_reads_(count) const XRECTF* pRectangles, // Array of highlight rects.
        _In_ CSolidColorBrush* foregroundBrush, // Color to use for text rendering of the intersecting text
        uint32_t startIndex, // Index of first rect within the array that corresponds to this node.
        _In_ const XPOINTF& nodeOffset, // Offset of the node in page space, used to compute relative offsets within a node.
        _Out_ uint32_t* pAdvanceCount // Number of rectangles processed by the node - whether contained or before the node's space.
        ) = 0;

    virtual void CleanupRealizations() = 0;

    virtual void SetBackPlateConfiguration(bool backPlateActive, bool useHyperlinkForeground) = 0;

    virtual void SetControlEnabled(bool enabled) = 0;

protected:
    // Transform applied to the content of the drawing context.
    CMILMatrix m_transform;

    // Initializes a new instance of the DrawingContext class.
    DrawingContext();
};

// Gets transform applied to the content of the drawing context.
inline const CMILMatrix & DrawingContext::GetTransform() const
{
    return m_transform;
}

// Sets transform applied to the content of the drawing context.
inline void DrawingContext::SetTransform(_In_ const CMILMatrix &transform)
{
    m_transform = transform;
}
