// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

enum class CompositionRequirement
{
    IndependentAnimation,
    Manipulatable,
    IndependentManipulation,
    IndependentClipManipulation,
    RootElement,
    RedirectionElement,
    Projection,
    Transform3D,
    RenderTransformAxisUnaligned,
    LocalClipAxisUnaligned,
    SwapChainContent,
    HandOffVisualNeeded,
    HandInVisualNeeded,
    UsesCompositeMode,
    IsNonHitTestableChildOfSwapChainOrMap,
    HasConnectedAnimation,
    HasImplicitShowAnimation,
    HasImplicitHideAnimation,
    XamlLight,
    RenderTargetBitmap,
    HasRoundedCorners,
    HasFacadeAnimation,
    HasTranslateZ,
    HasNonZeroRotation,
    HasScaleZ,
    HasNonIdentityTransformMatrix,
    HasNonZeroCenterPoint,
    HasNonDefaultRotationAxis,
    ShadowCaster,
    ProjectedShadowDefaultReceiver,
    ProjectedShadowCustomReceiver,
    TransitionRootWithChildren
};