// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Represents the transform from some point in the tree up to the root.
// Used to scale and provide subpixel positioning for realizations.
class CTransformToRoot
{
    //
    // Operates in one of two modes:
    //
    // 3D mode - There is a projection between the current element and the root of the tree.
    //    - pTransformToRoot3D is the cumulative transform to the root, and includes both 2D and
    //      3D transforms. When we create a realization, we'll pass the element size through
    //      this transform and get the 2D scales that will be applied to the element. That scale
    //      is then used to create the realization.
    //    - pTransformToRoot2D contains only the 2D transforms to the root. pTransformToRoot3D
    //      could give a near-zero scale if there's a projection that rotates its content to be
    //      perpendicular to the screen. We won't regenerate realizations if the projection starts
    //      animating, which will leave blurry content throughout the animation. The workaround is
    //      to clamp the 2D scales to a minimum value, assuming that the projections aren't there.
    //    - Text realizations care about subpixel positioning for text clarity, but subpixel
    //      positioning no longer applies when the text has been projected, so the offsets are set
    //      to 0.
    //
    // 2D mode - There is no projection between the current element and the root of the tree.
    //    - This is a performance optimization. 3D mode will still cover this case, but prepending
    //      transforms to a 4x4 matrix is more math than prepending to a 3x2.
    //    - pTransformToRoot3D is NULL.
    //    - pTransformToRoot2D is the cumulative transform to the root. It is also used for subpixel
    //      positioning of text.
    //

public:
    CTransformToRoot();
    ~CTransformToRoot();

    void SetTo2DScale(XFLOAT scaleXY);

    void Prepend(_In_ const CMILMatrix &transform2D);

    void Prepend(_In_ const CMILMatrix4x4 &transform3D);

    void Append(_In_ const CMILMatrix &transform2D);

    void Append(_In_ const CMILMatrix4x4 &transform3D);

    void MultiplyRasterizationScale(double scale);

    _Check_return_ HRESULT GetScaleDimensions(
        _In_ CUIElement *pElement,
        _Out_ XFLOAT *pScaleX,
        _Out_ XFLOAT *pScaleY
        ) const;

    void GetScaleDimensions(
        _In_ XSIZEF *pElementSize,
        _Out_ XFLOAT *pScaleX,
        _Out_ XFLOAT *pScaleY
        ) const;

    void GetMinimumScaleDimensions(
        _Out_ XFLOAT *pScaleX,
        _Out_ XFLOAT *pScaleY
        ) const;

    CMILMatrix GetRasterizationMatrix(_In_ CUIElement* element) const;

    CMILMatrix Get2DTransformToRoot(_In_ CUIElement *pElement) const;

    bool IsSameAs(
        _In_ const CTransformToRoot *pOther
        ) const;

    bool IsSameScaleAndSubPixelOffsetAs(
        _In_ const CTransformToRoot *other
        ) const;

    void Set(
        _In_ const CTransformToRoot *pOther
        );

    bool Is3DMode() const;

    void PixelSnap();

private:

    //
    // The rasterization scale comes from both transforms in the tree and an additional, explicit RasterizationScale
    // specified on the elements themselves. The transforms from the tree must be multiplied together, but explicit
    // RasterizationScales can't because they don't scale translations below them. We're keeping the explicit
    // RasterizationScales as a separate float to be multiplied in when retrieving the rasterization transform.
    //
    // Note that this restricts the explicit RasterizationScale to be a uniform scale. If it was separated into X and
    // Y components, then it would have to be multiplied in with transforms in the tree, which can contain rotations.
    // In that case we'll need a separate matrix that contains only the scales and rotations multiplied together, while
    // ignoring the translations.
    //

    CMILMatrix4x4 m_transformToRoot3D;
    CMILMatrix m_transformToRoot2D;
    bool m_is3DMode;

    // Collects the world transform, including the UIElement.RasterizationScale properties.
    //
    // UIE.RasterizationScale is used by the app to specify an additional scale to apply for rasterization and alpha
    // mask generation. We do not just keep a scalar to multiply into the scale at the end, because it can also affect
    // positioning. While it does not affect where Xaml places an element, it is expected to be applied by components
    // outside Xaml, such as the ICoreWindowSiteBridge, so it does affect the final position of elements on
    // screen. Therefore, we must take it into consideration when calculating things like subpixel positioning of
    // text, otherwise our text will be blurry on screen.
    CMILMatrix m_transformToRoot2DWithRasterizationScale;

    // We still need to keep this for 3D cases. With 3D we don't worry about subpixel positioning, so we can keep a
    // scalar and multiply it in rather than keep another 4x4 matrix.
    float m_additionalRasterizationScale { 1.0f };
};

