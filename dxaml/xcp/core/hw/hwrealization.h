// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Classes responsible for storage of realizations for various
//      types of content that may be transform-dependant.

#pragma once

namespace HWRealizationType
{
    enum Enum
    {
        Cache = 0,
        Shape,
        Text,
        Facade
    };
}

// Base class for HW realizations.
class HWRealization : public CXcpObjectBase<IObject>
{
public:
    HWRealization(HWRealizationType::Enum realizationType);

    HWRealizationType::Enum GetType() const
    {
        return m_realizationType;
    }

    virtual bool NeedsUpdate(
        _In_ const CMILMatrix *pRealizationTransform,
        bool isTransformAnimating
        );

    void UpdateRealizationParameters(
        _In_ const CMILMatrix *pRealizationTransform,
        bool isTransformAnimating,
        XFLOAT surfaceOffsetX,
        XFLOAT surfaceOffsetY,
        const bool renderCollapsedMask);

    virtual XFLOAT GetSurfaceOffsetX() const
    {
        return m_surfaceOffsetX;
    }

    virtual XFLOAT GetSurfaceOffsetY() const
    {
        return m_surfaceOffsetY;
    }

    void GetRealizationScale(_Out_ XFLOAT *pScaleX, _Out_ XFLOAT *pScaleY);

    bool HasSubPixelOffsets() const;

    virtual bool HasLostRealizationTexture()
    {
        return false;
    }

protected:
    CMILMatrix m_realizationTransform;
    bool m_renderedForAnimation;

    // Shapes are drawn with alpha masks, and there is no guarantee that the shape generates pixels starting at (0, 0).
    // Paths, for example, are free to start drawing anywhere, and can even start at a negative offset. The alpha mask
    // will contain all pixels drawn by the shape, and we store the top-left corner here. Fill and stroke use separate
    // masks, and there is no guarantee that they have the same offset due to stroke thicknesses inflating the geometry.
    // We take the union of both sets of bounds and use that for both masks.
    XFLOAT m_surfaceOffsetX;
    XFLOAT m_surfaceOffsetY;

    bool m_renderedCollapsedMask { false };

private:
    HWRealizationType::Enum m_realizationType;
};

// Shape/Border element HW realization. When used for borders, the fill is the background and the stroke is the border.
class HWShapeRealization final : public HWRealization
{
public:
    HWShapeRealization();
    ~HWShapeRealization() override;

    void SetStrokeHwTexture(_In_opt_ HWTexture *pNewStrokeHwTexture);

    _Ret_maybenull_ HWTexture *GetStrokeHwTexture()
    {
        return m_pStrokeHwTexture;
    }

    void SetFillHwTexture(_In_opt_ HWTexture *pNewFillHwTexture);

    _Ret_maybenull_ HWTexture *GetFillHwTexture()
    {
        return m_pFillHwTexture;
    }

    bool HasLostRealizationTexture() override;

    bool MaskNeedsUpdate(
        const bool renderCollapsedMask,
        _In_opt_ CBrush *pFillBrush,
        bool isFillBrushDirty,
        bool isFillBrushAnimated,
        _Out_ bool* pIsFillForHitTestOnly,
        _In_opt_ CBrush *pStrokeBrush,
        bool isStrokeBrushDirty,
        bool isStrokeBrushAnimated,
        _Out_ bool* pIsStrokeForHitTestOnly
        );

    bool MaskPartNeedsUpdate(
        _In_opt_ CBrush *pBrush,
        bool isBrushDirty,
        bool isBrushAnimated,
        _In_opt_ HWTexture *pHwTexture,
        _Inout_ bool *pWasClipped,
        _Out_ bool *pIsForHitTestOnly
        );

private:
    _Maybenull_ HWTexture *m_pStrokeHwTexture;
    _Maybenull_ HWTexture *m_pFillHwTexture;
    bool m_fStrokeWasClipped;
    bool m_fFillWasClipped;
};

class HWTextRealization final : public HWRealization
{
private:
    ~HWTextRealization() override;

public:
    HWTextRealization();

    bool NeedsUpdate(
        _In_ const CMILMatrix *pWorldTransform,
        bool isTransformAnimating
        ) override;

    // The subpixel offset was applied when drawing into the surface - it needs to be removed when drawing the surface.
    XFLOAT GetSurfaceOffsetX() const override;
    XFLOAT GetSurfaceOffsetY() const override;

    void SetTextHwTexture(_In_ HWTexture *pNewTextHwTexture);

    _Ret_notnull_ HWTexture *GetTextHwTexture() const
    {
        return m_pTextHwTexture;
    }

    void SetForegroundBrush(_In_ CBrush *pForegroundBrush);

    _Ret_notnull_ CBrush* GetForegroundBrush() const
    {
        return m_pForegroundBrush;
    }

    bool HasLostRealizationTexture() override;

    bool IsColorBitmap()
    {
        return m_isColorBitmap;
    }

    void SetIsColorBitmap(bool isColorBitmap);

private:
    _Maybenull_ HWTexture *m_pTextHwTexture = nullptr;
    _Maybenull_ CBrush *m_pForegroundBrush = nullptr;
    bool m_isColorBitmap = false;
};
