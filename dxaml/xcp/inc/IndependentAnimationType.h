// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

enum class IndependentAnimationType
{
    None,
    Transform,         // Animation of element's 2D transform or offset, or TransitionTarget transform
    ElementProjection, // Animation of element's perspective transform
    ElementTransform3D,// Animation of element's Transform3D
    ElementOpacity,    // Animation of element's opacity
    ElementClip,       // Animation of element's rectangular clip
    TransitionOpacity, // Animation of TransitionTarget opacity
    TransitionClip,    // Animation of TransitionTarget clip
    BrushColor,        // Animation of a brush's color
    Offset             // Animation of element's Canvas.Left/Top
};