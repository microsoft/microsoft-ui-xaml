// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <minxcptypes.h>

#ifndef LAYOUT_STORAGE_H
#define LAYOUT_STORAGE_H

class CRectangleGeometry;

class CLayoutStorage
{
    // WARNING: If any methods are virtual, this will mess up copying this to the managed side
public:
    CLayoutStorage()
    {
        m_pLayoutClipGeometry = NULL; // Need to do this here because it is tested in ResetLayoutInformation.
        ResetLayoutInformation();
    }

    ~CLayoutStorage();

public:
    // This is dynamic information that is used by the layout system, and does not need to be saved
    // if the element enters or leaves the tree. It also does not need to be validated.
    // note: keep ordering as is. See methodex FN_GET_LAYOUT_INFORMATION
    XSIZEF  m_previousAvailableSize;
    XSIZEF  m_desiredSize;
    XRECTF  m_finalRect;
    XPOINTF m_offset;
    XSIZEF  m_unclippedDesiredSize;
    XSIZEF  m_size;

    // Stores the layout clip, which is always an axis-aligned rect.
    // The coordinate space of this clip varies, depending on whether or not we're applying the layout
    // clip as a self clip, or as an ancestor clip.
    // When acting as a self clip, the rect is in the local coordinate space of the element.
    // When acting as an ancestor clip, the rect is in a special coordinate space, defined as follows:
    // [Parent local coordinate space] + [Child Offset]
    // What this does is puts the layout clip above the Child's render transforms, but below its offset.
    // Note that this definition requires that the layout clip not be applied when animating the Child Offset,
    // since this is only possible with children of a Canvas, which does not apply any layout clip, this works out fine.
    CRectangleGeometry* m_pLayoutClipGeometry;

    // This should be called when the element enters the tree, if layout storage has already been allocated.
    void ResetLayoutInformation();

};
//      WPF Property            Storage accessor    WPF field exposed by property
#define PreviousConstraint      GetLayoutStorage()->m_previousAvailableSize
#define DesiredSize             GetLayoutStorage()->m_desiredSize
#define RenderSize              GetLayoutStorage()->m_size
#define VisualOffset            GetLayoutStorage()->m_offset
#define UnclippedDesiredSize    GetLayoutStorage()->m_unclippedDesiredSize
#define FinalRect               GetLayoutStorage()->m_finalRect
#define LayoutClipGeometry      GetLayoutStorage()->m_pLayoutClipGeometry

#endif // LAYOUT_STORAGE_H