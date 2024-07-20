// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CompositorTreeHost : public CXcpObjectBase<IObject, CXcpObjectAddRefPolicy>
{
private:
    CompositorTreeHost();
    ~CompositorTreeHost() override;

public:
    static _Check_return_ HRESULT Create(_Outptr_ CompositorTreeHost **ppCompTreeHost);

    void Cleanup();

    void TrackTemporaryNode(HWCompNode& temporaryNode);

    _Check_return_ HRESULT RemoveTemporaryNodes();

private:
    // A list of temporary compositor nodes associated with a UI thread frame. These node will be removed
    // on the next UI thread frame.
    //
    // Normally, an element in an unrendered subtree (e.g. clipped out, 0 opacity) will not have a comp node,
    // even when an animation targets it. However, if there's an LTE targeting a descendant of the element, then
    // the element would need a comp node to collect transforms. In that case we create a temporary composition
    // peer.
    //
    // The comp node is temporary for two reasons:
    //   1. If its subtree starts rendering, we don't want to stop the render walk at its comp node boundary.
    //      None of its children will have rendered anything.
    //   2. The comp node is created when walking another part of the tree, so we don't have context for where
    //      to add its comp node in its parent's child collection. Since it is used purely for collecting the
    //      transform and does not have any associated render data, we can just add it at the end. Once it
    //      starts rendering, the comp node will need to be repositioned in the correct spot.
    //
    xvector<HWCompNode *> m_temporaryNodes;
};
