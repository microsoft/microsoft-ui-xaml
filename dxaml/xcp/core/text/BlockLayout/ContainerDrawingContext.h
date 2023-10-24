// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Generates rendering instructions for ContainerNode.

#pragma once

#include "DrawingContext.h"

class ContainerNode;

//---------------------------------------------------------------------------
//
//  ContainerDrawingContext
//
//  Generates rendering instructions for ContainerNode.
//
//---------------------------------------------------------------------------
class ContainerDrawingContext : public DrawingContext
{
public:
    // Initializes a new instance of the ContainerDrawingContext class.
    ContainerDrawingContext(_In_ ContainerNode *pNode);

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

    void InvalidateRenderCache() override;

    void ClearForegroundHighlightInfo() override;

    void AppendForegroundHighlightInfo(
        uint32_t count,
        _In_reads_(count) const XRECTF* pRectangles,
        _In_ CSolidColorBrush* foregroundBrush,
        uint32_t startIndex, // Index of first rect within the array that corresponds to this node.
        _In_ const XPOINTF& nodeOffset, // Offset of the node in page space, used to compute relative offsets within a node.
        _Out_ uint32_t* pAdvanceCount // Number of rectangles contained in node's render space.
        ) override;

    void CleanupRealizations() override
    {
    }

    void SetBackPlateConfiguration(bool backPlateActive, bool useHyperlinkForeground) override;

    void SetControlEnabled(bool enabled) override;

private:
    // Container node associated with the drawing context.
    ContainerNode *m_pNode;
};
