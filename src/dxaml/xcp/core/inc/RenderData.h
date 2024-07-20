// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class HWRealization;
class HWCompRenderDataNode;

struct PrimitiveCompositionPropertyData
{
    void Init(RenderWalkType type)
    {
        pHWRealizationCache = NULL;
        pPreChildrenRenderData = NULL;
        pPostChildrenRenderData = NULL;
        isInScene = FALSE;
        wasRedirectedTransformAnimating = FALSE;
        pLastCompNodeInSubgraphNoRef = NULL;
        lastSpriteVisualInSubgraphNoRef = nullptr;

        ASSERT(type == RWT_WinRTComposition);
    }

    void Clear();

    void ClearInSceneData();

    void ClearRenderData(_In_ CCoreServices* coreServices);

    static void ClearRenderDataList(_In_ PCRenderDataList* pRenderData, _In_ CCoreServices* coreServices);

    // Realization of contents.  It's included as PC property data because elements
    // with realizations render their content with D2D or SW.
    __maybenull HWRealization *pHWRealizationCache;

    // TODO: INCWALK: FixedSizeAllocator? Inline?
    // All the PC render data for this element.
    // Note: This list takes a ref in both sprite visuals mode and legacy DComp primitives mode.
    PCRenderDataList* pPreChildrenRenderData;
    PCRenderDataList* pPostChildrenRenderData;

    // True if render data is in the persistent PC scene.
    bool isInScene                       : 1;

    //
    // The following fields are only valid if isInScene is TRUE.
    //

    // Used only for redirected elements. This stores the target's 'isTrasnformAnimating' flag
    // so it can be compared frame-over-frame.
    bool wasRedirectedTransformAnimating : 1;

    // Stores the last (top-most) comp node pushed in the element's subgraph.
    // With asynchronous comp tree updates, this will always be a post-subgraph node either for this element's composition
    // peer, or for the composition peer of a descendent element. Since it's always a post-subgraph node, it will always be
    // a render-data node.
    // With synchronous comp tree updates, this will be a comp tree node.
    HWCompNode *pLastCompNodeInSubgraphNoRef;

    // Stores the last (top-most) SpriteVisual in the element's subgraph.
    WUComp::IVisual* lastSpriteVisualInSubgraphNoRef;
};

struct RenderData
{
    RenderData()
        : type(RWT_None)
    {}

    RenderWalkType type;
};

struct PropertyRenderData : RenderData
{
    PrimitiveCompositionPropertyData pc;

    void Reset(RenderWalkType newType)
    {
        switch (type)
        {
            case RWT_WinRTComposition:
                pc.Clear();
                break;
            case RWT_None:
            case RWT_NonePreserveDComp:
                break;
            default:
                ASSERT(FALSE);
        }

        switch (newType)
        {
            case RWT_WinRTComposition:
                pc.Init(newType);
                break;
            case RWT_None:
            case RWT_NonePreserveDComp:
                break;
            default:
                ASSERT(FALSE);
        }

        type = newType;
    }

    bool IsRenderWalkTypeForComposition() const
    {
        return type == RWT_WinRTComposition;
    }
};

// TODO: HWPC: Software edge store vector could move here, not worthwhile unless there's some D2D or PC
// TODO: HWPC: content render data stored on UIElement as well. Maybe D2D bounds?
struct ContentRenderData : RenderData
{
    void Reset(RenderWalkType newType)
    {
        type = newType;
    }

};
