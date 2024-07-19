// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "BaseContentRenderer.h"
#include <MaxTextureSizeProvider.h>
#include <FocusRectangle.h>
#include <FocusRectOptions.h>
#include <DCompTreeHost.h>
#include <LinearGradientBrush.h>
#include <XamlCompositionBrush.h>
#include <PixelFormat.h>
#include "ColorUtil.h"
#include <InputServices.h>
#include <XcpInputPaneHandler.h>
#include "HitTestParams.h"
#include "XamlIslandRoot.h"

BaseContentRenderer::BaseContentRenderer(uint32_t maxTextureSize)
 : m_pRenderDataList(nullptr)
 , m_pRenderParams(nullptr)
 , m_pElementRenderParams(nullptr)
 , m_pTransformToRoot(nullptr)
 , m_pUIElementNoRef(nullptr)
 , m_dcompTreeHostNoRef(nullptr)
 , m_maxTextureSize(maxTextureSize)
 , m_updatedShapeRealization(false)
{
}

void BaseContentRenderer::SetRenderParams(_In_ const HWRenderParams* pRenderParams)
{
    m_pRenderParams = pRenderParams;
    m_dcompTreeHostNoRef = m_pRenderParams->pRenderTarget->GetDCompTreeHost();
}

const HWRenderParams* BaseContentRenderer::GetRenderParams() const
{
    return m_pRenderParams;
}

void BaseContentRenderer::SetElementRenderParams(_In_ HWElementRenderParams* pElementRenderParams)
{
    m_pElementRenderParams = pElementRenderParams;
}

HWElementRenderParams* BaseContentRenderer::GetElementRenderParams() const
{
    return m_pElementRenderParams;
}

void BaseContentRenderer::SetUIElement(_In_ CUIElement* pUIElement)
{
    // ContentRenderer is always expected to be a short-lived stack allocation, don't bother ref-counting
    // TODO: Make an effort to get rid of this member.  It is needed only for text rendering using an image
    // brush (RequestImageDecode).  RenderTextRealization() should pass in the associated UIElement.
    m_pUIElementNoRef = pUIElement;
}

CUIElement* BaseContentRenderer::GetUIElement()
{
    return m_pUIElementNoRef;
}

void BaseContentRenderer::SetUpdatedShapeRealization(bool value)
{
    m_updatedShapeRealization = value;
}

bool BaseContentRenderer::GetUpdatedShapeRealization() const
{
    return m_updatedShapeRealization;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      A helper function to ensure a correctly sized HWTexture exists for
//      a stroke or fill mask part.
//
//------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT
BaseContentRenderer::MaskPartEnsureRealizationTexture(
    _In_ const HWRenderParamsBase &myRP,
    _In_ IMaskPartRenderParams* pParams,
    XUINT32 width,
    XUINT32 height,
    _In_ HWShapeRealization *pHwShapeRealization,
    bool hasBrushAnimation,
    _Out_ bool *pIsForHitTestOnly)
{
    HRESULT hr = S_OK;

    CBrush *pPartBrush = pParams->GetBrush();

    HWTexture *pNewTexture = nullptr;

    bool partHasContent = pPartBrush != nullptr
                              && width > 0
                              && height > 0
                              && (hasBrushAnimation || OpacityToBlendInt(pPartBrush->m_eOpacity) > 0);

    bool isForHitTestOnly = false;

    if (partHasContent)
    {
        if (pPartBrush->GetTypeIndex() == KnownTypeIndex::ImageBrush)
        {
            CImageBrush *pPartImageBrush = static_cast<CImageBrush*>(pPartBrush);

            partHasContent = (pPartImageBrush->GetSoftwareSurface() != nullptr);
            IFC(pPartImageBrush->ReloadSoftwareSurfaceIfReleased());
        }
        else if (!hasBrushAnimation && pPartBrush->GetTypeIndex() == KnownTypeIndex::SolidColorBrush)
        {
            CSolidColorBrush *pPartSolidColorBrush = static_cast<CSolidColorBrush*>(pPartBrush);

            // Non-transparent brushes with transparent colors are special-cased to hit-test but not incur rendering
            // overhead. In order to hit-test in the DWM, a primitive must still be generated, but it will be textured
            // with a special hit-test-only surface. The mask texture is optimized away in this case, since the
            // DWM does not use them for hit-testing.
            isForHitTestOnly = ColorUtils::IsTransparentColor(pPartSolidColorBrush->m_rgb);

            partHasContent = !isForHitTestOnly;
        }
        // TODO_Materials: Do we need to consider EffectBrush here?
    }

    // If there is content to draw for this part, ensure there's a correctly-sized
    // HWTexture we can use.
    if (partHasContent)
    {
        HWTexture *pHwPartTextureNoRef = pParams->GetCachedHwTexture(pHwShapeRealization);

        const bool needToRecreatePartHwTexture = pHwPartTextureNoRef == nullptr
                                               || pHwPartTextureNoRef->GetWidth() != width
                                               || pHwPartTextureNoRef->GetHeight() != height
                                               || pHwPartTextureNoRef->IsSurfaceLost();

        // If the size of the part has changed or the device was lost, re-create the HW texture...
        if (needToRecreatePartHwTexture)
        {
            // Free the existing HWTexture in the atlas before allocating a new one.
            pParams->SetCachedHwTexture(pHwShapeRealization, nullptr);

            ASSERT(pNewTexture == nullptr);
            IFC(myRP.pHWWalk->GetTextureManager()->CreateTexture(
                pixelGray8bpp,
                width,
                height,
                HWTextureFlags_IncludePadding,
                &pNewTexture
                ));

            // Store the resulting HW texture reference on the element so that we can re-use it later.
            pParams->SetCachedHwTexture(pHwShapeRealization, pNewTexture);
        }
    }
    else
    {
        // Clear the cache.
        pParams->SetCachedHwTexture(
            pHwShapeRealization,
            nullptr);
    }

    *pIsForHitTestOnly = !!isForHitTestOnly;

Cleanup:
    ReleaseInterfaceNoNULL(pNewTexture);
    return hr;
}

// Get the brush mask, transform, and texture pointer
// Note that the brush transform is returned in 2 different forms:
//
// *pBrushTextureTransform is set to the entire brush transform
//   This is used in the non-tiled case
//
// pTileIterator->m_brushToElement is set to the portion of the brush
//   transform that contains everything except for the final scale
//   which normalizes textures coordinates.  This is used in the tiled case.
_Check_return_ HRESULT BaseContentRenderer::GetBrushParameters(
    _In_ HWTextureManager *pTextureManager,
    _In_ SurfaceCache *pSurfaceCache,
    _In_ CBrush *pBrush,
    bool setColorOnPrimitive,
    _In_opt_ CUIElement *pUIElement,
    _In_opt_ const XTHICKNESS *pNinegrid,
    _In_ const XRECTF* primitiveBounds,
    _In_ const XRECTF* brushBounds,
    _Out_ XFLOAT *pLocalOpacity,
    _Out_ XUINT32 *pBrushMask,
    _Out_ bool *pHasBrush,
    _Out_ HWTexture** ppBrushTexture,
    _Out_opt_ CMILMatrix *pBrushTextureTransform)
{
    HRESULT hr = S_OK;

    XUINT32 brushMask = 0xffffffff;
    bool hasBrush = true;

    HWTexture *pBrushTexture = nullptr;
    IPALSurface *pHWSurface = nullptr;
    HWTexture *pHWTexture = nullptr;
    CMILMatrix brushToElement(TRUE);
    const HWRenderParamsBase& myRP = *m_pRenderParams;

    CTileBrush *pTileBrush = nullptr;

    //
    // Get the data from the brush
    //

    switch (pBrush->GetTypeIndex())
    {
        case KnownTypeIndex::SolidColorBrush:
        {
            if (setColorOnPrimitive)
            {
                CSolidColorBrush *pSolidColorBrush = static_cast<CSolidColorBrush*>(pBrush);
                if (OpacityToBlendInt(pBrush->m_eOpacity) <= 0)
                {
                    hasBrush = false;
                }
                else if (ColorUtils::IsTransparentColor(pSolidColorBrush->m_rgb))
                {
                    // Non-transparent brushes with transparent colors are special-cased to hit-test but not incur
                    // rendering overhead. In order to hit-test in the DWM, a primitive must still be generated, but
                    // it will be textured with a special hit-test-only surface.
                    CMILMatrix hitTestTransform(TRUE);

                    IFC(pTextureManager->GetHitTestTexture(&pBrushTexture));

                    // TODO_WinRT
                    // We need the inverse if this is the way we do hittest-only prims.  Currently ignored.
                    hitTestTransform.SetDx(-brushBounds->X);
                    hitTestTransform.SetDy(-brushBounds->Y);
                    hitTestTransform.Scale(
                        1.0f / brushBounds->Width,
                        1.0f / brushBounds->Height
                        );

                    *pBrushTextureTransform = hitTestTransform;
                }
                else
                {
                    brushMask = pSolidColorBrush->m_rgb;
                }
            }
        }
        break;

        case KnownTypeIndex::ImageBrush:
        {
            IFC(HWWalk::RequestImageDecode(
                brushBounds,
                myRP,
                static_cast<CImageBrush*>(pBrush),
                pUIElement,
                pNinegrid));

            pTileBrush = static_cast<CTileBrush*>(pBrush);
            CMILMatrix brushTransform(TRUE);
            ASSERT(brushBounds != nullptr);

            // If we only have a software surface we need to create a hardware texture or tiling to back the image
            IFC(pTileBrush->EnsureAndUpdateHardwareResources(
                pTextureManager,
                myRP.pRenderTarget,
                pSurfaceCache
                ));

            // Render nothing if the image source has 0 area.
            XRECTF naturalBounds;
            pTileBrush->GetNaturalBounds(&naturalBounds);

            if (IsEmptyRectF(naturalBounds))
            {
                hasBrush = false;
            }

            if (pTileBrush->GetHardwareSurface() != nullptr)
            {
                SetInterface(pBrushTexture, pTileBrush->GetHardwareSurface());
            }
            else
            {
                // Neither a hardware surface or tiled surface is available. This is probably because the image
                // is still being downloaded/loaded. Draw nothing until it's available.
                hasBrush = false;
            }

            if (hasBrush)
            {
                IFC(CalculateTileBrushTransform(
                    pTileBrush,
                    brushBounds,
                    &brushTransform,
                    &brushToElement));
            }

            // For PC, this is from element space to texture space.
            // For WinRT, the transform goes in the opposite direction:  texture to element.
            *pBrushTextureTransform = brushTransform;
        }
        break;

        case KnownTypeIndex::LinearGradientBrush:
        {
            // A WUC linear gradient brush will stretch to fill the visual that uses it, which is exactly what Xaml wants in
            // every case except for RichTextBlock. RichTextBlock uses brush bounds that are different from the rect of the text,
            // so we need to put an additional scale on the WUC linear gradient brush, which is calculated here. The caller will
            // ignore this transform if it comes out to be identity.
            CMILMatrix transform(true);
            if (primitiveBounds->Width > 0 && primitiveBounds->Height > 0)
            {
                transform.SetM11(brushBounds->Width / primitiveBounds->Width);
                transform.SetM22(brushBounds->Height / primitiveBounds->Height);
                transform.SetDx(brushBounds->X);
                transform.SetDy(brushBounds->Y);
            }
            *pBrushTextureTransform = transform;
        }
        break;

        case KnownTypeIndex::XamlCompositionBrushBase:
        {
            CXamlCompositionBrush *pXamlCompositionBrush= static_cast<CXamlCompositionBrush*>(pBrush);
            brushMask = pXamlCompositionBrush->m_fallbackColor;
        }
        break;
    }

    *pLocalOpacity = ClampOpacity(pBrush->m_eOpacity);
    *pBrushMask = brushMask;
    *pHasBrush = hasBrush;
    SetInterface(*ppBrushTexture, pBrushTexture);

Cleanup:
    ReleaseInterfaceNoNULL(pHWSurface);
    ReleaseInterfaceNoNULL(pHWTexture);
    ReleaseInterfaceNoNULL(pBrushTexture);

    return hr;
}

_Check_return_ HRESULT BaseContentRenderer::MaskCombinedRenderHelper(
    _In_ CUIElement *pUIElement,
    _In_opt_ IMaskPartRenderParams *pStrokeMaskPart,
    _In_opt_ IMaskPartRenderParams *pFillMaskPart,
    bool forceMaskDirty
    )
{
    HWShapeRealization *pHwShapeRealization = nullptr;
    HWRealization *pHwRealization = nullptr;

    bool needsRealizationUpdate = false;
    bool fMaskDirty = false;
    bool isFillForHitTestOnly = false;
    bool isStrokeForHitTestOnly = false;

    auto guard = wil::scope_exit([&pHwRealization, &pHwShapeRealization]()
    {
        ReleaseInterface(pHwRealization);
        ReleaseInterface(pHwShapeRealization);
    });

    CMILMatrix realizationScale = m_pRenderParams->GetRasterizationScaleTransform(pUIElement);

    //
    // Check whether the element is elegible for a nine grid. Even if it is, there can be things precluding it
    // from rendering with one, such as an unsupported brush type.
    //
    // Note that we always ask for layout rounding here regardless of the UseLayoutRounding property set on the
    // border. We want nine grid insets that turn into whole numbers once multiplied by the rasterization scale.
    // These insets determine how much of the mask is mapped into the nine grid, not how much of the mask contains
    // the border stroke. Always mapping an integer number of pixels on all four sides guarantees that we don't
    // introduce any unnecessary blurriness. The border stroke is drawn into the alpha mask later, and at that
    // time we'll layout round the border thickness depending on the UseLayoutRounding property on the border.
    //
    XTHICKNESS nineGrid;
    const bool hasNineGrid = HWWalk::GetNinegridForBorderElement(pUIElement, true /* forceIntegerInsets */, &nineGrid);

    // Determine whether we can use a collapsed nine grid (showing just the four corners) to render this element. Both
    // the background and the border must support using a collapsed nine grid for that to happen. Note that the border
    // has the option of using a 1:1 (uncollapsed) nine grid, because it still benefits from avoiding overdraw from the
    // hollow center. The background gains no benefits from a 1:1 nine grid, so it's collapsed or nothing.
    const bool renderCollapsedMask = hasNineGrid
         && (!pFillMaskPart || pFillMaskPart->ShouldUseCollapsedNineGrid()) // If a part is missing, it doesn't prevent collapsing the nine grid
         && (!pStrokeMaskPart || pStrokeMaskPart->ShouldUseCollapsedNineGrid());

    //
    // Step one: determine element's content dirtiness once: we render
    // the fill and the stroke separately and want to make sure that
    // they behave consistently when the element properties change.
    //
    SetInterface(
        pHwRealization,
        static_cast<HWRealization *>(
            pUIElement->GetHWRealizationCache()));

    //
    // Make sure that we have a realization object ready... If we had
    // a realization, we will want to replace it with a new one if its
    // type has changed (cached -> shape).
    //
    bool createdFreshRealization = false;

    if (pHwRealization != nullptr &&
        pHwRealization->GetType() == HWRealizationType::Shape)
    {
        SetInterface(
            pHwShapeRealization,
            static_cast<HWShapeRealization *>(pHwRealization));

        fMaskDirty = pUIElement->IsMaskDirty(
            pHwShapeRealization,
            renderCollapsedMask,
            m_pElementRenderParams->isFillBrushAnimating,
            m_pElementRenderParams->isStrokeBrushAnimating,
            &isFillForHitTestOnly,
            &isStrokeForHitTestOnly
            );

        fMaskDirty |= forceMaskDirty;
    }

    if (pHwShapeRealization == nullptr)
    {
        //
        // Create a new shape realization and store it on the UI element:
        //
        pHwShapeRealization = new HWShapeRealization();

        ReplaceInterface(pHwRealization, pHwShapeRealization);

        pUIElement->SetHWRealizationCache(pHwShapeRealization);

        createdFreshRealization = TRUE;
    }

    //
    // Step two: if the content was dirty, re-rasterize the shape masks
    // for the stroke and for the fill using our software rasterizer.
    //
    // Note that the transform to the root will be taken into account when
    // producing the realization, instead of the transform to the nearest comp node.
    //
    ASSERT(pHwRealization == static_cast<HWRealization *>(pHwShapeRealization));
    ASSERT(pHwRealization->GetType() == HWRealizationType::Shape);

    //
    // First try rendering the shape at the scale in the world transform...
    //
    // Rasterized content is produced at the scale of the world transform. The remainder
    // of the transform is applied when drawing the rasterized surface later on.
    //

    ASSERT(realizationScale.IsScaleOnly());
    float scaleX, scaleY;
    realizationScale.GetScaleDimensions(&scaleX, &scaleY);

    //
    // Of the elements that require an alpha mask to render, shapes are an exception. Other elements like rounded
    // borders produce content based on their layout size, but shapes are not restricted by their layout size.
    // Elements like Path contain arbitrary geometries for rendering, and are free to render into negative space.
    // So when we bound the element for the purposes of creating a mask, we need to handle shapes differently.
    //
    // e.g. For a Path that draws a rectangle from (50, 50) to (250, 150), the offset is (50, 50), the width is 200,
    // and the height is 100. For a Path that draws an ellipse from (-100, -50) to (50, 50), the offset is (-100, -50),
    // the width is 150, and the height is 100.
    //
    uint32_t alphaMaskWidth, alphaMaskHeight;
    XPOINTF alphaMaskOffset;
    if (pUIElement->OfTypeByIndex(KnownTypeIndex::Shape))
    {
        // Shapes not only need tighter bounds due to their geometries, but their fills and strokes can have
        // different bounds due to stroke thicknesses and dash arrays. Note that the stroke bounds are not
        // necessarily strictly larger than the fill bounds, because the stroke might have missing pieces due
        // to a dash array. We take a union of the two of them for a mask size that can hold both the fill
        // and the stroke.

        CShape* shape = static_cast<CShape*>(pUIElement);

        if (shape->ShouldRender(nullptr /* pHasElementBounds - don't care */, nullptr /* pElementBounds - don't care */))
        {
            XRECTF_RB fillBounds;
            IFC_RETURN(shape->GetFillBounds(&fillBounds));

            XRECTF_RB strokeBounds;
            IFC_RETURN(shape->GetStrokeBounds(&strokeBounds));

            UnionRectF(&fillBounds, &strokeBounds);

            alphaMaskOffset.x = static_cast<float>(XcpFloor(fillBounds.left * scaleX));
            alphaMaskOffset.y = static_cast<float>(XcpFloor(fillBounds.top * scaleY));
            alphaMaskWidth = XcpCeiling( (XcpCeiling(fillBounds.right) - XcpFloor(fillBounds.left)) * scaleX );
            alphaMaskHeight = XcpCeiling( (XcpCeiling(fillBounds.bottom) - XcpFloor(fillBounds.top)) * scaleY );
        }
        else
        {
            // A shape that's 0x0 doesn't render anything.
            // Note that we don't go through bounding code because GetStrokeBounds will inflate whatever geometry is
            // inside the shape, even if it's 0. But a zero-sized shape doesn't render anything at all.
            alphaMaskOffset.x = 0;
            alphaMaskOffset.y = 0;
            alphaMaskWidth = 0;
            alphaMaskHeight = 0;
        }
    }
    else
    {
        alphaMaskOffset.x = 0;
        alphaMaskOffset.y = 0;
        alphaMaskWidth = XcpCeiling( XcpCeiling(pUIElement->GetActualWidth()) * scaleX );
        alphaMaskHeight = XcpCeiling( XcpCeiling(pUIElement->GetActualHeight()) * scaleY );

        // D2D mask rendering requires us to know the size of the mask surface before rendering it. For a collapsed
        // mask, the surface size is just the nine grid insets plus a single pixel in the center.
        if (renderCollapsedMask)
        {
            //
            // Note: round the result here. Do not use ceiling.
            //
            // For 1:1 masks, the size of the mask doesn't matter much as long as it matches the size of the visual.
            // The mask can be a pixel or two larger in either dimension, as long as those extra rows and columns are
            // filled with transparent pixels. The visual will also be increased in size to match, so the filled pixels
            // on screen still look correct. There will be a slight discrepancy in hit testing but that will not be
            // noticeable.
            //
            // Here we're not using a 1:1 mask. We're using a collapsed mask, and the exact size of the mask matters.
            // Consider this edge case:
            //
            //      <Border Width="100" Height="100" CornerRadius="10,0,0,10" Background="Red" />
            //
            // Here there are rounded corners on the left but square corners on the right. The collapsed alpha mask
            // will contain 10px of rounded corners on the left, 0px of rounded corner on the right, and 1px of content
            // in the center for 11 pixels total. The matching nine grid will have an inset of 10px on the left for the
            // rounded corners and 0px on the right for the square corners. The 10px left inset matches the left 10px in
            // mask to draw the rounded corners. There is no right inset, so the remaining 1px of the mask is mapped to
            // the remaining 90px of the border.
            //
            // Consider what happens if we made the mask 12px instead of 11, with a transparent column on the right.
            // The nine grid insets don't change. The left 10px of the mask still maps to the left 10px of the border.
            // But the remaining 2px of the mask now map to the other 90px of the border. This is a problem because
            // those two pixels are an opaque pixel and a fully transparent pixel. Stretching them over the border
            // means we create a gradient from opaque to transparent over the right 90px of the border, which looks
            // very noticeably wrong.
            //
            // This error can come up at display scales larger than 1. We need to calculate the size of the mask before
            // rendering, and we need to take the display scale into account. Using ceiling can produce this extra
            // pixel when we hit floating point errors.
            //
            // The solution is to round instead. We are not running the risk of cutting off any content. The nine
            // grid itself is calculated while forcing integer insets, which means they already account for layout
            // rounding and the display scale (see HWWalk::GetNinegridForBorderElement and HWWalk::GetNinegridForBorderParameters).
            // We're also adding a layout rounded pixel to account for the center of the border (see
            // CFrameworkElement::CreateBorderGeometriesAndBrushClipsCommon). The result should be a whole number
            // after being multiplied by the display scale again, possibly with floating point error, so rounding
            // is enough.
            //
            // Also note: There's still a problem lurking here. We're multiplying by the rasterization scale, which
            // includes the root scale plus any additional transforms like UIElement.RenderTransform down to the element.
            // We've only rounded for the root scale. Multiplying this in might still end up with a decimal that's not
            // just floating point error.
            //
            // But we can live with this because render transforms are rare. This problem also already exists, because
            // we only round the collapsed nine grid insets based on the root scale and not the entire render transform
            // down to this element, so we might have blurriness or seam issues already.
            //
            // Fixing this would involve passing the entire render transform down to where we layout round the nine grid
            // insets. We don't have to fix it until someone actually hits it.
            //
            float onePixel = pUIElement->LayoutRound(1.0f);
            uint32_t collapsedAlphaMaskWidth = XcpRound( (nineGrid.left + onePixel + nineGrid.right) * scaleX);
            uint32_t collapsedAlphaMaskHeight = XcpRound( (nineGrid.top + onePixel + nineGrid.bottom) * scaleY);

            alphaMaskWidth = MIN(alphaMaskWidth, collapsedAlphaMaskWidth);
            alphaMaskHeight = MIN(alphaMaskHeight, collapsedAlphaMaskHeight);
        }
    }

    // Figure out whether both masks will fit inside max texture bounds. Scale things down if not.
    {
        const uint32_t maxTextureSize = GetMaxTextureSize();

        if (alphaMaskWidth > maxTextureSize)
        {
            const float scaleXDownToFitMaxTexture = static_cast<float>(maxTextureSize) / static_cast<float>(alphaMaskWidth);
            realizationScale.Scale(scaleXDownToFitMaxTexture, 1);
            alphaMaskWidth = MIN(static_cast<uint32_t>(alphaMaskWidth * scaleXDownToFitMaxTexture), maxTextureSize);
        }

        if (alphaMaskHeight > maxTextureSize)
        {
            const float scaleYDownToFitMaxTexture = static_cast<float>(maxTextureSize) / static_cast<float>(alphaMaskHeight);
            realizationScale.Scale(1, scaleYDownToFitMaxTexture);
            alphaMaskHeight = MIN(static_cast<uint32_t>(alphaMaskHeight * scaleYDownToFitMaxTexture), maxTextureSize);
        }
    }

    //
    // If the content was dirty, re-rasterize the shape masks for the stroke and for the fill.
    //
    if (   createdFreshRealization
        || pHwShapeRealization->NeedsUpdate(&realizationScale, m_pRenderParams->isTransformAnimating)
        || pHwShapeRealization->HasLostRealizationTexture()
        || fMaskDirty)
    {
        //
        // Remember the scale and surface offsets for later use...
        //
        pHwShapeRealization->UpdateRealizationParameters(
            &realizationScale,
            m_pRenderParams->isTransformAnimating,
            alphaMaskOffset.x,
            alphaMaskOffset.y,
            renderCollapsedMask);

        needsRealizationUpdate = TRUE;
    }

    if (needsRealizationUpdate)
    {
        // Ensure correctly-sized HWTextures are available, as needed, for the stroke and fill.
        if (pFillMaskPart != nullptr)
        {
            IFC_RETURN(MaskPartEnsureRealizationTexture(
                *m_pRenderParams,
                pFillMaskPart,
                alphaMaskWidth,
                alphaMaskHeight,
                pHwShapeRealization,
                m_pElementRenderParams->isFillBrushAnimating,
                &isFillForHitTestOnly));

            if (pHwShapeRealization->GetFillHwTexture() != nullptr)
            {
                IFC_RETURN(AlphaMask::RasterizeFill(
                    pUIElement,
                    realizationScale,
                    &alphaMaskOffset,
                    renderCollapsedMask,
                    pHwShapeRealization->GetFillHwTexture()->GetCompositionSurface()));
            }
        }

        if (pStrokeMaskPart != nullptr)
        {
            IFC_RETURN(MaskPartEnsureRealizationTexture(
                *m_pRenderParams,
                pStrokeMaskPart,
                alphaMaskWidth,
                alphaMaskHeight,
                pHwShapeRealization,
                m_pElementRenderParams->isStrokeBrushAnimating,
                &isStrokeForHitTestOnly));

            if (pHwShapeRealization->GetStrokeHwTexture() != nullptr)
            {
                IFC_RETURN(AlphaMask::RasterizeStroke(
                    pUIElement,
                    realizationScale,
                    &alphaMaskOffset,
                    renderCollapsedMask,
                    pHwShapeRealization->GetStrokeHwTexture()->GetCompositionSurface()));
            }
        }

        m_updatedShapeRealization = true;
    }

    // Render the shape primitives using the now-updated realizations.
    {
        XPOINTF shapeOffset;
        shapeOffset.x = pHwShapeRealization->GetSurfaceOffsetX();
        shapeOffset.y = pHwShapeRealization->GetSurfaceOffsetY();

        // The shape mask is at the scale of realization. This does not necessarily match the world scale
        // when drawing it, unless the realization was updated.
        XFLOAT realizationScaleX, realizationScaleY;
        pHwShapeRealization->GetRealizationScale(&realizationScaleX, &realizationScaleY);

        // Create parameters for rendering shape fill.
        if (pFillMaskPart)
        {
            BrushParams brushParams;
            brushParams.m_element = pUIElement;
            brushParams.m_brushProperty = ElementBrushProperty::Fill;
            IFC_RETURN(MaskPartRenderHelper(
                pUIElement,
                shapeOffset,
                realizationScaleX,
                realizationScaleY,
                pFillMaskPart,
                brushParams,
                pHwShapeRealization->GetFillHwTexture(),
                isFillForHitTestOnly,
                renderCollapsedMask,
                // Backgrounds only render with a nine grid if the mask is collapsed. They don't benefit from
                // a nine grid with an unstretched mask.
                (renderCollapsedMask && hasNineGrid) ? &nineGrid : nullptr));
        }

        // Create parameters for rendering shape stroke.
        if (pStrokeMaskPart)
        {
            BrushParams emptyBrushParams;
            IFC_RETURN(MaskPartRenderHelper(
                pUIElement,
                shapeOffset,
                realizationScaleX,
                realizationScaleY,
                pStrokeMaskPart,
                emptyBrushParams,
                pHwShapeRealization->GetStrokeHwTexture(),
                isStrokeForHitTestOnly,
                renderCollapsedMask,
                // Borders can render with a nine grid even if the mask isn't collapsed, because they benefit from
                // using a hollow nine grid to avoid overdraw.
                hasNineGrid ? &nineGrid : nullptr));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      A helper function to render a panel
//
//------------------------------------------------------------------------
 _Check_return_ HRESULT
BaseContentRenderer::PanelRenderContentHelper(
    _In_ CPanel *pPanel
    )
{
    return BorderRenderHelper(pPanel);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      A common helper for all border-like elements
//
//------------------------------------------------------------------------
_Check_return_ HRESULT BaseContentRenderer::BorderRenderHelper(_In_ CFrameworkElement* pFrameworkElement)
{
    if (HWWalk::IsBorderLikeElementThatRequiresSoftwareRendering(pFrameworkElement))
    {
        BorderOutlinePart borderOutlinePart(pFrameworkElement);
        BorderBackgroundPart borderBackgroundPart(pFrameworkElement);

        // If we're applying rounded corner clipping to this element's CompNode, defer drawing
        // this element's border until post-children phase, so that the border is drawn on top
        // of any child content that might encroach into the border area.
        bool shouldRenderBorderAsPostChildren = pFrameworkElement->RequiresCompNodeForRoundedCorners();

        IFC_RETURN(MaskCombinedRenderHelper(
            pFrameworkElement,
            shouldRenderBorderAsPostChildren ? nullptr : &borderOutlinePart,
            &borderBackgroundPart,
            false   // forceMaskDirty
            ));
    }
    else
    {
        IFC_RETURN(BorderHWRenderHelper(pFrameworkElement));
    }

    return S_OK;
}

// Draw post-children content for all border-like elements.
_Check_return_ HRESULT
BaseContentRenderer::BorderLikeElementPostChildrenRender(
    _In_ CFrameworkElement *element
    )
{
    // Currently the only reason to draw post-children content is when rounded corner clipping is being used.
    // This condition should already have been checked so assert it here.
    ASSERT(element->RequiresCompNodeForRoundedCorners());

    // If we're applying rounded corner clipping to this element's CompNode, draw the element's border
    // on top of any child content that might encroach into the border area.
    BorderOutlinePart borderOutlinePart(element);

    // TFS # 12557739:  If the realization was updated in the first pass of rendering this element due to a
    // realization scale change, the shape realization's scale info is already updated by this time and cannot
    // be used to detect if a realization scale change has happened for the mask.  This would cause the mask to
    // not be updated to its new size, leaving the mask at the wrong size.
    // To fix this, we detect a realization update in the first pass, store it in m_updatedShapeRealization,
    // use this to force the re-creation of the mask.
    // TODO:  Ideally we should refactor the shape realization both for performance and cleaner code by
    // updating fill and stroke realizations independently - don't share state with the overall realization.
    IFC_RETURN(MaskCombinedRenderHelper(
        element,
        &borderOutlinePart,
        nullptr,
        m_updatedShapeRealization   // Sets forceMaskDirty == true if we updated the realization on first pass
        ));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      A helper function to render non-masked border and background
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
BaseContentRenderer::BorderHWRenderHelper(
    _In_ CFrameworkElement* pFrameworkElement
    )
{
    ASSERT(pFrameworkElement->OfTypeByIndex<KnownTypeIndex::Panel>()
        || pFrameworkElement->OfTypeByIndex<KnownTypeIndex::Border>()
        || pFrameworkElement->OfTypeByIndex<KnownTypeIndex::ContentPresenter>());

    const HWRenderParams& myRP = *m_pRenderParams;
    const HWElementRenderParams& myElementRP = *m_pElementRenderParams;
    CBrush * backgroundBrush = pFrameworkElement->GetBackgroundBrush();
    CBrush * borderBrush = pFrameworkElement->GetBorderBrush();
    CSolidColorBrush *solidBackgroundBrushNoRef = do_pointer_cast<CSolidColorBrush>(backgroundBrush);
    CSolidColorBrush *solidBorderBrushNoRef = do_pointer_cast<CSolidColorBrush>(borderBrush);

    XRECTF bounds = {
        0.0f,
        0.0f,
        pFrameworkElement->GetActualWidth(),
        pFrameworkElement->GetActualHeight()
    };

    const bool extendBackgroundUnderBorder =
        (pFrameworkElement->GetBackgroundSizing() == DirectUI::BackgroundSizing::OuterBorderEdge);

    // If we have solid color brushes for both the background and border brushes,
    // if neither of those solid color brushes are being animated,
    // if those solid color brushes are the exact same color and opacity or if
    // the background extends under a transparent border,
    // and if we have no rounded corners,
    // then we can expand the background to function as the borders as well,
    // since they will look exactly the same.
    // Performing this optimization will prevent seams between the background
    // and border in this case.
    const bool canUseBackgroundAsBorder =
        solidBackgroundBrushNoRef != nullptr
        && solidBorderBrushNoRef != nullptr
        && !myElementRP.isFillBrushAnimating
        && !myElementRP.isStrokeBrushAnimating
        && (
            (
                solidBackgroundBrushNoRef->m_rgb == solidBorderBrushNoRef->m_rgb
                && solidBackgroundBrushNoRef->m_eOpacity == solidBorderBrushNoRef->m_eOpacity
            )
            ||
            (
                extendBackgroundUnderBorder
                && ColorUtils::IsTransparentColor(solidBorderBrushNoRef->m_rgb)
            )
        );

    // Draw the background rectangle.
    if (backgroundBrush != nullptr
        && (myElementRP.isFillBrushAnimating
            || OpacityToBlendInt(backgroundBrush->m_eOpacity) > 0))
    {
        XRECTF innerRect;
        XTHICKNESS thickness;

        if (pFrameworkElement->GetUseLayoutRounding())
        {
            bounds.Width  = pFrameworkElement->LayoutRound(bounds.Width);
            bounds.Height = pFrameworkElement->LayoutRound(bounds.Height);

            thickness = CBorder::GetLayoutRoundedThickness(pFrameworkElement);
        }
        else
        {
            thickness = pFrameworkElement->GetBorderThickness();
        }

        //
        // Because the background and the border are rendered as two separate primitives, there could be a small
        // seam between them from floating point error. In order to reduce seaming, we allow the background to
        // overlap the border by a small amount in certain cases.
        //
        // Normally the background is shrunk down to not overlap the border, for two reasons:
        //
        //  1. If the background is an image brush, then the bounds of that image shouldn't overlap the border area.
        //     Otherwise parts of the image would be covered up by the border area.
        //
        //  2. If the border area has transparency, then the background shouldn't show through.
        //
        // But, if everything is solid and opaque then the bounds of the background don't matter. We allow it to overlap
        // the border area to prevent seaming. In all our default control templates, the brushes are all solid colors,
        // so this addresses the most common scenario.
        //
        // This does have some negative effects on fill rate because of a bigger background primitive, but we don't
        // expect giant borders with large border thicknesses and small backgrounds. The expected case (e.g. a button)
        // has a large background and a small border thickness.
        //
        // Alternately, if we're going to be using the background as the border as well, or if the background is
        // supposed to extend under the border, then we want the background to take up the full bounds as well.
        if (extendBackgroundUnderBorder || canUseBackgroundAsBorder
            || (
                // The border brush must exist, otherwise the border area should be fully transparent and the background
                // shouldn't show through. The border brush must not have any transparency. It must be a solid color
                // brush with an opaque color and no brush opacity.
                solidBorderBrushNoRef != nullptr
                && solidBorderBrushNoRef->m_eOpacity == 1.0f
                && ColorUtils::IsOpaqueColor(solidBorderBrushNoRef->m_rgb)

                //
                // The background must be either null or a solid color. Otherwise its brush bounds affect rendering, and
                // we need the correct background rect.
                && (backgroundBrush == nullptr || solidBackgroundBrushNoRef)

                //
                // The border brush must not be animated, otherwise the animation could give it transparency.
                // The opacity of the background doesn't matter, so it's allowed to be animated.
                && !myElementRP.isStrokeBrushAnimating

                //
                // The border itself and its ancestors must never have any transparency.
                && (IsCloseReal(myRP.opacityToCompNode, 1.0f) || IsCloseReal(1.0f, myRP.opacityToCompNode))
                && (IsCloseReal(myRP.opacityFromCompNodeToRoot, 1.0f) || IsCloseReal(1.0f, myRP.opacityFromCompNodeToRoot))
                && !myRP.isOpacityAnimating
                )
            )
        {
            // Allow the border to overlap the background in order to prevent a seam between them.
            innerRect = bounds;
        }
        else
        {
            // Shrink the background down so that it doesn't overlap the border. It needs to be sized correctly
            // for the rendering to be correct.
            CBorder::HelperDeflateRect(bounds, thickness, innerRect);
        }

        BrushParams brushParams;
        brushParams.m_element = pFrameworkElement;
        brushParams.m_brushProperty = ElementBrushProperty::Fill;
        IFC_RETURN(GeneralImageRenderContent(
            innerRect,
            innerRect,
            backgroundBrush,
            brushParams,
            pFrameworkElement,
            nullptr /* pNinegrid */,
            nullptr /* pShapeHwTexture */,
            FALSE /* fIsHollow */
            ));
    }

    // Draw the border as a hollow ninegrid if we're not using the background as the border.
    if (!canUseBackgroundAsBorder && borderBrush != nullptr)
    {
        //
        // Don't ask for a layout rounded ceiling operation at the end of nine grid inset calculation. Here we're
        // not drawing with a mask. Instead we're using the nine grid insets to represent the border thicknesses
        // directly. The border thicknesses will already be layout rounded as needed if the border has specified
        // UseLayoutRounding="true". Doing a layout rounded ceiling operation on the final insets can bump them up
        // by a pixel due to floating point error, and the border we draw here will be thicker than expected.
        //
        XTHICKNESS ninegrid;
        const bool canDrawWithNinegrid = HWWalk::GetNinegridForBorderElement(pFrameworkElement, false /* forceIntegerInsets */, &ninegrid);
        ASSERT(canDrawWithNinegrid);

        if (CBorder::HasNonZeroThickness(ninegrid))
        {
            BrushParams emptyBrushParams;
            IFC_RETURN(GeneralImageRenderContent(
                bounds,
                bounds,
                borderBrush,
                emptyBrushParams,
                pFrameworkElement,
                &ninegrid /* pNinegrid */,
                nullptr /* pShapeHwTexture */,
                TRUE /* fIsHollow */
            ));
        }
    }

    // Elements that switched from needing a mask to render to not needing a mask to render must discard their mask.
    // Otherwise, they'll end up with a stale mask that they don't know is stale. They won't know the mask is stale
    // because they won't be marked dirty anymore, because they've already rendered without the mask.
    pFrameworkElement->SetHWRealizationCache(nullptr);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      A helper function to render a stroke or fill mask part.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
BaseContentRenderer::MaskPartRenderHelper(
    _In_opt_ CUIElement *pUIElement,
    _In_ const XPOINTF& shapeOffset,
    XFLOAT realizationScaleX,
    XFLOAT realizationScaleY,
    _In_ IMaskPartRenderParams* pParams,
    const BrushParams& brushParams,
    _In_opt_ HWTexture *pHwPartTexture,
    bool isForHitTestOnly,
    const bool renderCollapsedMask,
    const XTHICKNESS* nineGrid)
{
    //
    // Draw the shape part texture as a rectangular primitive.
    //
    if (pHwPartTexture != nullptr || isForHitTestOnly)
    {
        XRECTF shapeRect;
        XRECTF brushBounds;
        IFC_RETURN(pParams->GetBrushBounds(brushBounds));

        // If this primitive is generated for hit-testing only, the rasterized texture for the
        // primitive mask was optimized away to minimize perf impact. In order to bound the primitive,
        // the brush bounds are used, which are potentially looser than the rasterized bounds may have been.
        if (isForHitTestOnly)
        {
            shapeRect = brushBounds;
        }
        else
        {
            ASSERT(pHwPartTexture != nullptr);

            if (renderCollapsedMask)
            {
                // If we've collapsed the mask to save on memory usage, then we can't infer the element size from the
                // size of the mask. Use the element bounds instead. There are several assumptions that we can safely
                // make here:
                //    - This optimization is only enabled for border-like elements (Border/Grid/ContentPresenter) with
                //      rounded corners. These elements all have the top-left corner at (0, 0) in local coordinates
                //      and don't have a shape offset, which handles scenarios like shapes that can render content at
                //      arbitrary local coordinates.
                //      Note that normally having a border with a border thickness and no border brush will cause content
                //      to render at an offset. In CFrameworkElement::GenerateBorderEdgesCommon we explicitly check for
                //      this and prevent it.
                ASSERT(shapeOffset.x == 0 && shapeOffset.y == 0);
                shapeRect.X = 0;
                shapeRect.Y = 0;
                //    - This optimization is only enabled when rendering with a brush which fills the entire bounds of
                //      the element (as opposed to ImageBrush which can leave parts of the element empty depending on
                //      the stretch mode and aspect ratio of the image).
                shapeRect.Width = pUIElement->GetActualWidth();
                shapeRect.Height = pUIElement->GetActualHeight();
                if (pUIElement->GetUseLayoutRounding())
                {
                    shapeRect.Width  = pUIElement->LayoutRound(shapeRect.Width);
                    shapeRect.Height = pUIElement->LayoutRound(shapeRect.Height);
                }
            }
            else
            {
                shapeRect.X = shapeOffset.x / realizationScaleX;
                shapeRect.Y = shapeOffset.y / realizationScaleY;
                shapeRect.Width = static_cast<XFLOAT>(pHwPartTexture->GetWidth()) / realizationScaleX;
                shapeRect.Height = static_cast<XFLOAT>(pHwPartTexture->GetHeight()) / realizationScaleY;
            }
        }

        //
        // A solid nine grid can be skipped if we're only rendering a hit test visual. The visual will be transparent
        // with no backing mask and doesn't benefit from a nine grid brush. A hollow nine grid should still be applied
        // if we're rendering a hit test visual, because that transparent visual should still be hollow in the middle.
        //
        // If hollowness and hit-testing-ness don't preclude us from using a nine grid, also check the brush set on the
        // visual. An ImageBrush can also preclude a nine grid from being used.
        //
        const bool useHollowNineGrid = pParams->ShouldUseHollowNineGrid();
        const bool useNinegrid = (!useHollowNineGrid && isForHitTestOnly) ? false : pParams->ShouldUseNineGrid();

        // Note: don't read renderCollapsedMask from the IMaskPartRenderParams here. The background and border masks
        // are rendered together. Either both are collapsed (if both IMaskPartRenderParams allow the mask to be
        // collapsed) or neither are collapsed (if either IMaskPartRenderParams prohibits collapsing).

        m_pElementRenderParams->m_realizationScaleX = realizationScaleX;
        m_pElementRenderParams->m_realizationScaleY = realizationScaleY;

        IFC_RETURN(GeneralImageRenderContent(
            shapeRect,
            brushBounds,
            pParams->GetBrush(),
            brushParams,
            pUIElement,
            useNinegrid ? nineGrid : nullptr,
            pHwPartTexture,
            useHollowNineGrid));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Renders this PopupRoot's light dismiss layer.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT BaseContentRenderer::PopupRootRenderContent(
    _In_ CPopupRoot* pPopupRoot
    )
{
    bool requiresLightDismiss = false;

    if (pPopupRoot->m_pOpenPopups)
    {
        for (CXcpList<CPopup>::XCPListNode *pNode = pPopupRoot->m_pOpenPopups->GetHead();
             !requiresLightDismiss && pNode != nullptr;
             pNode = pNode->m_pNext)
        {
            if (!pNode->m_pData->IsUnloading())
            {
                requiresLightDismiss |= pNode->m_pData->m_fIsLightDismiss;
            }
        }
    }

    if (requiresLightDismiss)
    {
        xref_ptr<CBrush> transparentBrush;
        XRECTF_RB lightDismissBoundsRB;
        XRECTF lightDismissBoundsWH;

        IFC_RETURN(pPopupRoot->GetInnerBounds(&lightDismissBoundsRB));
        lightDismissBoundsWH = ToXRectF(lightDismissBoundsRB);

        IFC_RETURN(pPopupRoot->GetContext()->GetTransparentBrush(transparentBrush.ReleaseAndGetAddressOf()));
        ASSERT(transparentBrush);

        BrushParams brushParams;
        IFC_RETURN(GeneralImageRenderContent(
            lightDismissBoundsWH,
            lightDismissBoundsWH,
            transparentBrush,
            brushParams,
            nullptr /* pUIElement */,
            nullptr /* pNinegrid */,
            nullptr /* pShapeHwTexture */,
            false   /* fIsHollow */
            ));
    }

    return S_OK;
}

_Check_return_ HRESULT BaseContentRenderer::PanelRenderContent(_In_ CPanel *pPanel)
{
    if (pPanel->OfTypeByIndex<KnownTypeIndex::SwapChainPanel>())
    {
        IFC_RETURN(SwapChainPanelRenderContent(static_cast<CSwapChainPanel *>(pPanel)));
    }
    else if (!(pPanel->GetContext()->IsTransparentBackground() && pPanel->OfTypeByIndex<KnownTypeIndex::RootVisual>())
        && !(pPanel->OfTypeByIndex<KnownTypeIndex::XamlIslandRoot>() && static_cast<CXamlIslandRoot*>(pPanel)->HasTransparentBackground())
        && (pPanel->HasLayoutStorage() || (!pPanel->IsDefaultWidth() && !pPanel->IsDefaultHeight()))
        && !pPanel->OfTypeByIndex<KnownTypeIndex::SwapChainBackgroundPanel>())
    {
        // TODO: JCOMP: We could actually support the Background property on SCBP with DComp now, but it
        // TODO: JCOMP: might be a perf trap for developers due to overdraw.
        // SCBP background should never be rendered - apps should be blocked at the API layer from setting it.
        ASSERT(!pPanel->OfTypeByIndex<KnownTypeIndex::SwapChainPanel>() && !pPanel->OfTypeByIndex<KnownTypeIndex::SwapChainBackgroundPanel>());

        IFC_RETURN(PanelRenderContentHelper(pPanel));

        if (pPanel->OfTypeByIndex<KnownTypeIndex::RootVisual>() || pPanel->OfTypeByIndex<KnownTypeIndex::XamlIslandRoot>())
        {
            CCoreServices* core = pPanel->GetContext();
            CContentRoot* contentRoot = VisualTree::GetContentRootForElement(pPanel);

            if (contentRoot->GetInputManager().IsInputPaneShowing())
            {
                // When the input pane is up, the RootScrollViewer auto-resizes its height to move its content out from
                // underneath it.  However the input pane may not span all the way across the window.  If we did nothing
                // this would expose the "backplate" visual whose color isn't necessarily what we want the user to see.
                // Even worse, they could see all the way through to the desktop if the the window background was suppressed
                // via the IWindowPrivate::put_TransparentBackground API.
                // This code retrieves potentially non-empty "exposure rect" and covers it with a SpriteVisual whose color
                // matches the (design supplied) background color of the input pane.
                XRECTF exposureRect = contentRoot->GetInputManager().GetInputPaneHandler()->GetInputPaneExposureRect();
                if (!IsEmptyRectF(exposureRect))
                {
                    xref_ptr<CSolidColorBrush> brush;
                    CValue v;
                    v.SetColor(0xFF1A1A1A);
                    CREATEPARAMETERS cp(core, v);
                    IFCFAILFAST(CSolidColorBrush::Create((CDependencyObject**)brush.ReleaseAndGetAddressOf(), &cp));

                    BrushParams brushParams;
                    IFC_RETURN(GeneralImageRenderContent(
                        exposureRect,
                        exposureRect,
                        brush,
                        brushParams,
                        pPanel,
                        nullptr /* pNinegrid */,
                        nullptr /* pShapeHwTexture */,
                        false /* fIsHollow */
                        ));
                }
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      ContentPresenter "rendering".  This method doesn't do any actual
//      rendering work, see comments below.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
BaseContentRenderer::ContentPresenterRenderContent(
    _In_ CContentPresenter *pContentPresenter
    )
{
    return BorderRenderHelper(pContentPresenter);
}

//------------------------------------------------------------------------
//
//  Synopsis: UserControlRenderBackground
//      Renders a UserControl background by using a rectangle.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
BaseContentRenderer::UserControlRenderBackground(
    _In_ CUserControl *pUserControl
    )
{
    if (pUserControl && pUserControl->m_pBackground)
    {
        // Set the bounds with the actual left, top position and
        // actual width/height.
        XRECTF bounds = {
            pUserControl->GetActualOffsetX(),
            pUserControl->GetActualOffsetY(),
            pUserControl->GetActualWidth(),
            pUserControl->GetActualHeight()
        };

        BrushParams brushParams;
        IFC_RETURN(GeneralImageRenderContent(
            bounds,
            bounds,
            pUserControl->m_pBackground,
            brushParams,
            pUserControl,
            nullptr     /* pNinegrid */,
            nullptr     /* pShapeHwTexture */,
            false       /* fIsHollow */
            ));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      SwapChainPanel "rendering".  This method doesn't do any actual
//      rendering work, see comments below.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
BaseContentRenderer::SwapChainPanelRenderContent(
    _In_ CSwapChainPanel *pSwapChainPanel
    )
{
    // Let the SwapChainPanel know about the composition scale portion of the transform to root.
    // SwapChainPanel needs this so it can notify the app about composition scale via a XAML event
    // and two read-only properties, CompositionScaleX and CompositionScaleY.
    // The app can use these scale factors to re-create or resize their swapchain at a different scale,
    // and set an inverse transform on their swapchain to compensate for the transform we'll apply
    // to the DComp visuals in the SwapChainPanel hierarchy.
    CMILMatrix realizationScale = m_pRenderParams->GetRasterizationScaleTransform(pSwapChainPanel);
    IFC_RETURN(pSwapChainPanel->UpdateCompositionScale(realizationScale, m_pRenderParams->isTransformAnimating));

    return S_OK;
}

_Check_return_ HRESULT BaseContentRenderer::ListViewBaseItemChromeRenderLayer(
    _In_ CListViewBaseItemChrome *pChrome,
    _In_ ListViewBaseItemChromeLayerPosition layer
    )
{
    return pChrome->RenderLayer(this, layer);
}

_Check_return_ HRESULT BaseContentRenderer::ImageRenderContent(
    _In_ CImage *pImage
    )
{
    HWRenderVisibility visibility = HWRenderVisibility::Invisible;

    if (pImage->GetBackgroundBrush() &&
        OpacityToBlendInt(pImage->GetBackgroundBrush()->m_eOpacity))
    {
        XRECTF bounds = {
            0.0f,
            0.0f,
            pImage->GetActualWidth(),
            pImage->GetActualHeight()
        };

        // TODO: HWPC: Adjust bounds for image stretch / alignment

        XTHICKNESS ninegrid;
        XTHICKNESS *pNinegrid = nullptr;

        if (HWWalk::ShouldImageUseNineGrid(pImage))
        {
            ninegrid = pImage->GetNinegrid();
            pNinegrid = &ninegrid;
        }

        BrushParams brushParams;
        IFC_RETURN(GeneralImageRenderContent(
            bounds,
            bounds,
            pImage->GetBackgroundBrush(),
            brushParams,
            pImage,
            pNinegrid,
            nullptr /* pShapeHwTexture */,
            false /* fIsHollow */
            ));

        visibility = HWRenderVisibility::Visible;
    }

    IFC_RETURN(pImage->NotifyRenderContent(visibility));

    return S_OK;
}

_Check_return_ HRESULT BaseContentRenderer::GlyphsRenderContent(
    _In_ CGlyphs *pGlyphs
    )
{
    return pGlyphs->HWRender(this);
}

_Check_return_ HRESULT BaseContentRenderer::TextBlockRenderContent(
    _In_ CTextBlock *pTextBlock
    )
{
    IFC_RETURN(pTextBlock->HWRenderContent(this));

    IFC_RETURN(pTextBlock->NotifyRenderContent(HWRenderVisibility::Visible));

    return S_OK;
}

_Check_return_ HRESULT BaseContentRenderer::RichTextBlockRenderContent(
    _In_ CRichTextBlock *pRichTextBlock
    )
{
    return pRichTextBlock->HWRenderContent(this);
}

_Check_return_ HRESULT BaseContentRenderer::RichTextBlockOverflowRenderContent(
    _In_ CRichTextBlockOverflow *pRichTextBlockOverflow
    )
{
    return pRichTextBlockOverflow->HWRenderContent(this);
}

_Check_return_ HRESULT BaseContentRenderer::TextBoxViewRenderContent(
    _In_ CTextBoxView *pTextBoxView
    )
{
    return pTextBoxView->HwRender(this);
}

_Check_return_ HRESULT BaseContentRenderer::RectangleRenderContent(
    _In_ CRectangle *pRectangle
    )
{
    ASSERT(!HWWalk::IsShapeMaskRequiredForRectangle(pRectangle));

    HWRenderVisibility visibility = HWRenderVisibility::Invisible;

    if (pRectangle->m_pFill != nullptr
        && (m_pElementRenderParams->isFillBrushAnimating
            || OpacityToBlendInt(pRectangle->m_pFill->m_eOpacity) > 0))
    {
        XRECTF bounds = {
            0.0f,
            0.0f,
            pRectangle->GetActualWidth(),
            pRectangle->GetActualHeight()
        };

        // Account for shape stretching.
        CShape::UpdateRectangleBoundsForStretchMode(pRectangle->m_Stretch, bounds);

        BrushParams brushParams;
        IFC_RETURN(GeneralImageRenderContent(
            bounds,
            bounds,
            pRectangle->m_pFill,
            brushParams,
            pRectangle,
            nullptr /* pNinegrid */,
            nullptr /* pShapeHwTexture */,
            false /* fIsHollow */
            ));

        visibility = HWRenderVisibility::Visible;
    }

    IFC_RETURN(pRectangle->NotifyRenderContent(visibility));

    // Elements that switched from needing a mask to render to not needing a mask to render must discard their mask.
    // Otherwise, they'll end up with a stale mask that they don't know is stale. They won't know the mask is stale
    // because they won't be marked dirty anymore, because they've already rendered without the mask.
    pRectangle->SetHWRealizationCache(nullptr);

    return S_OK;
}

_Check_return_ HRESULT
BaseContentRenderer::BorderRenderContent(
    _In_ CBorder *pBorder
    )
{
    return BorderRenderHelper(pBorder);
}

_Check_return_ HRESULT
BaseContentRenderer::ContentControlItemRender(
    _In_ CContentControl *pContentControl
    )
{
    // We can directly render GridViewItems or ListViewItems if they are chromed.
    if (pContentControl->m_pListViewBaseItemChrome)
    {
#ifdef DBG
        CUIElementCollection *pCollection = nullptr;
        XUINT32 childCount = 0;

        pCollection = static_cast<CUIElementCollection*>(pContentControl->GetChildren());
        if (pCollection)
        {
            childCount = pCollection->GetCount();
        }

        ASSERT(childCount == 1);
#endif

        IFC_RETURN(pContentControl->m_pListViewBaseItemChrome->RenderLayer(
            this,
            ListViewBaseItemChromeLayerPosition_Base_Pre
            ));
    }

    return S_OK;
}

_Check_return_ HRESULT BaseContentRenderer::CalendarViewBaseItemChromeRenderLayer(
    _In_ CCalendarViewBaseItemChrome *pChrome,
    _In_ CalendarViewBaseItemChromeLayerPosition layer
    )
{
    return pChrome->RenderChrome(this, layer);
}

_Check_return_ HRESULT BaseContentRenderer::TextBlockPostChildrenRender(
    _In_ CTextBlock *pTextBlock
    )
{
    return pTextBlock->HWPostChildrenRender(this);
}

_Check_return_ HRESULT BaseContentRenderer::RichTextBlockPostChildrenRender(
    _In_ CRichTextBlock *pRichTextBlock
    )
{
    return pRichTextBlock->HWPostChildrenRender(this);
}

_Check_return_ HRESULT BaseContentRenderer::RichTextBlockOverflowPostChildrenRender(
    _In_ CRichTextBlockOverflow *pRichTextBlockOverflow
    )
{
    return pRichTextBlockOverflow->HWPostChildrenRender(this);
}

_Check_return_ HRESULT BaseContentRenderer::ContentControlPostChildrenRender(
    _In_ CContentControl *pContentControl
    )
{
    // We can directly render GridViewItems or ListViewItems if they are chromed.
    if (pContentControl->m_pListViewBaseItemChrome)
    {
        IFC_RETURN(pContentControl->m_pListViewBaseItemChrome->RenderLayer(
            this,
            ListViewBaseItemChromeLayerPosition_Base_Post
            ));
    }

    return S_OK;
}

_Check_return_ HRESULT
BaseContentRenderer::RenderFocusRectangle(
    _In_ CUIElement *pUIElement,
    _In_ FocusRectangleOptions &focusOptions
    )
{
    // If bounds haven't been set by the caller, default to just use the element bounds
    if (pUIElement && focusOptions.bounds.Width == 0.0f && focusOptions.bounds.Height == 0.0f)
    {
        focusOptions.UseElementBounds(pUIElement);
    }

    const bool areThicknessesUniform =
           (!focusOptions.drawFirst  || focusOptions.firstThickness.IsUniform())
        && (!focusOptions.drawSecond || focusOptions.secondThickness.IsUniform());

    // We don't support drawing dotted-line rects with variable thicknesses
    FAIL_FAST_ASSERT(areThicknessesUniform || focusOptions.isContinuous);

    XRECTF bounds = focusOptions.bounds;
    if (focusOptions.drawFirst)
    {
        if (focusOptions.firstThickness.IsUniform())
        {
            if (focusOptions.firstThickness.left != 0.0f) // bail for zero-width thickness
            {
                // Cheaper to call this when thickness is uniform
                IFC_RETURN(RenderFocusRectangle(
                    pUIElement->GetContext(),
                    bounds,
                    focusOptions.isContinuous,
                    0.5f,
                    focusOptions.firstThickness.left,
                    focusOptions.firstBrush.get()));
            }
        }
        else
        {
            IFC_RETURN(RenderFocusRectangleWithThickness(
                pUIElement->GetContext(),
                bounds,
                focusOptions.firstBrush,
                focusOptions.firstThickness));
        }

        if (focusOptions.isContinuous)
        {
            bounds = ShrinkRectByThickness(bounds, focusOptions.firstThickness);
        }
    }
    if (focusOptions.drawSecond)
    {
        // dotted lines must have same thickness, just re-use the first one
        const XTHICKNESS& thickness = focusOptions.isContinuous ?
            focusOptions.secondThickness :
            focusOptions.firstThickness;

        if (thickness.IsUniform())
        {
            if (thickness.left != 0.0f) // bail for zero-width thickness
            {
                // Cheaper to call this when thickness is uniform
                IFC_RETURN(RenderFocusRectangle(
                    pUIElement->GetContext(),
                    bounds,
                    focusOptions.isContinuous,
                    1.5f,
                    thickness.left,
                    focusOptions.secondBrush.get()));
            }
        }
        else
        {
            IFC_RETURN(RenderFocusRectangleWithThickness(
                pUIElement->GetContext(),
                bounds,
                focusOptions.secondBrush,
                thickness));
        }
    }
    return S_OK;
}

// Render a focus rect with optional dotted line
_Check_return_ HRESULT
BaseContentRenderer::RenderFocusRectangle(
    _In_ CCoreServices *pCore,
    _In_ XRECTF bounds,
    _In_ bool isContinuous,
    _In_ float dashOffset,
    _In_ float strokeThickness,
    _In_ CBrush* brush
)
{
    xref_ptr<CFocusRectangle> spFocusRectangle;
    IFC_RETURN(CFocusRectangle::Create(pCore, spFocusRectangle.ReleaseAndGetAddressOf()));

    IFC_RETURN(spFocusRectangle->SetStrokeThickness(strokeThickness));

    // if the rectangle is continuous (not dashed), we clear the StrokeDashArray
    // by default, the stroke dash array is {1.0, 1.0}
    if (isContinuous)
    {
        IFC_RETURN(spFocusRectangle->SetStrokeDashArray(nullptr));
    }
    else
    {
        IFC_RETURN(spFocusRectangle->SetStrokeDashOffset(dashOffset));
    }

    IFC_RETURN(spFocusRectangle->SetBounds(bounds, brush));
    ShapeStrokePart strokeMaskPart(spFocusRectangle.get());
    ShapeFillPart fillMaskPart(spFocusRectangle.get());

    // Push a translate transform to offset the rectangle and honor
    // bounds.X and bounds.Y
    CMILMatrix renderTransform(true);
    renderTransform.AppendTranslation(bounds.X, bounds.Y);

    const HWRenderParams& rp = *(GetRenderParams());
    HWRenderParams localRP = rp;
    HWRenderParamsOverride hwrpOverride(this, &localRP);

    TransformAndClipStack transformsAndClips;
    IFC_RETURN(transformsAndClips.Set(rp.pTransformsAndClipsToCompNode));
    transformsAndClips.PrependTransform(renderTransform);
    localRP.pTransformsAndClipsToCompNode = &transformsAndClips;

    IFC_RETURN(MaskCombinedRenderHelper(
        spFocusRectangle.get(),
        &strokeMaskPart,
        &fillMaskPart,
        false  // forceMaskDirty
        ));

    return S_OK;
}

// Render a focus rect with no dotted line, but supports non-uniform thickness
_Check_return_ HRESULT
BaseContentRenderer::RenderFocusRectangleWithThickness(
    _In_ CCoreServices *pCore,
    _In_ const XRECTF& bounds,
    _In_ CBrush *brush,
    _In_ const XTHICKNESS& thickness
    )
{
    xref_ptr<CBorder> spBorder;
    IFC_RETURN(CBorder::CreateForFocusRendering(pCore, spBorder.ReleaseAndGetAddressOf()));

    spBorder->m_borderThickness = thickness;
    spBorder->m_pBorderBrush = brush;
    AddRefInterface(brush);

    spBorder->m_eWidth = bounds.Width;
    spBorder->m_eHeight = bounds.Height;
    spBorder->EnterPCScene();
    spBorder->ClearPCRenderData();

    // Push a translate transform to offset the rectangle and honor
    // bounds.X and bounds.Y
    CMILMatrix renderTransform(true);
    renderTransform.AppendTranslation(bounds.X, bounds.Y);

    const HWRenderParams& rp = *(GetRenderParams());
    HWRenderParams localRP = rp;
    HWRenderParamsOverride hwrpOverride(this, &localRP);

    TransformAndClipStack transformsAndClips;
    IFC_RETURN(transformsAndClips.Set(rp.pTransformsAndClipsToCompNode));
    transformsAndClips.PrependTransform(renderTransform);
    localRP.pTransformsAndClipsToCompNode = &transformsAndClips;

    IFC_RETURN(BorderRenderContent(spBorder.get()));
    return S_OK;
}

void BaseContentRenderer::AddDirtyElementForNextFrame(_In_ CUIElement* element)
{
    m_dirtyElementsForNextFrameNoRef.push_back(element);
}

void BaseContentRenderer::DirtyElementsForNextFrame()
{
    for (const auto& element: m_dirtyElementsForNextFrameNoRef)
    {
        CUIElement::NWSetContentDirty(element, DirtyFlags::Render);
    }
    m_dirtyElementsForNextFrameNoRef.clear();
}
