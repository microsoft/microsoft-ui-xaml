// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "VisualContentRenderer.h"
#include <LinearGradientBrush.h>
#include <WUCBrushManager.h>
#include "XamlCompositionBrush.h"
#include <XamlLight.h>
#include <microsoft.ui.composition.private.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

VisualContentRenderer::VisualContentRenderer(uint32_t maxTextureSize)
    : BaseContentRenderer(maxTextureSize)
{
}

void
VisualContentRenderer::PushRenderDataList(
    _In_opt_ PCRenderDataList* pRenderDataList,
    _In_ bool fAppendOnly
)
{
    // Incremental update of HWComp primitives is accomplished by reusing the primitives
    // on a UIElement's PCRenderDataLists. Each UIElement has a list of its pre-children
    // primitives and a list of its post-children primitives.  All primitives on a list
    // have the same parent.  During the renderwalk, the pre and post-children PCRenderDataList
    // is set via this function prior to rendering the corresponding set of primitives.
    // The list maybe NULL if no caching is desired (bitmap cache, for example), in which
    // case we can neither reuse primitives nor add newly created ones to be cached.  The list
    // maybe something other than the currently rendering UIElement's cache list (redirected
    // rendering for LTEs).  This is indicated by fAppendOnly being set, which tells us that
    // we cannot treat the existing primitives on the list as reusable, but we should
    // append newly created primitives to it.
    ASSERT(m_RecycledRenderData.empty());
    ASSERT(m_spCurrentParentCollection == nullptr);

    if (!fAppendOnly && (pRenderDataList != nullptr))
    {
        // pRenderDataList is a ref-counted list of SpriteVisuals. Swap them into m_RecycledRenderData,
        // which will now hold the references. They'll be consumed (and released) in LinkVisual.
        pRenderDataList->swap(m_RecycledRenderData);
    }

    m_idxNextReusable = 0;
    m_pRenderDataList = pRenderDataList;
}

void VisualContentRenderer::PopRenderDataList()
{
    // Toss any remaining cached primitives we have not consumed.  Consider moving these
    // to generic global cache instead where another element's render could reuse them.
    // Any visuals that were reused were released from the m_RecycledRenderData list in LinkVisuals.
    for (XUINT32 i = m_idxNextReusable; i < m_RecycledRenderData.size(); ++i)
    {
        WUComp::IVisual* cached = m_RecycledRenderData[i];
        UnlinkVisual(cached);
    }

    m_RecycledRenderData.clear();
    m_spCurrentParentCollection.Reset();
}

_Check_return_ HRESULT
VisualContentRenderer::RenderSolidColorRectangle(
    _In_ const XRECTF& rect,
    _In_ CSolidColorBrush* pBrush
)
{
    ComPtr<WUComp::IVisual> spVisual;
    ComPtr<WUComp::ISpriteVisual> spContentVisual;
    WUComp::ICompositor* pCompositor = m_dcompTreeHostNoRef->GetCompositor();
    ComPtr<WUComp::ICompositionBrush> wucBrush = pBrush->GetWUCBrush(pCompositor);

    IFC_RETURN(EnsureSpriteVisual(pCompositor, rect, AAEdge_All, &spContentVisual, &spVisual));
    IFCFAILFAST(spContentVisual->put_Brush(wucBrush.Get()));
    IFC_RETURN(LinkVisual(pBrush, nullptr, spVisual.Get()));

    return S_OK;
}

_Check_return_ HRESULT
VisualContentRenderer::RenderTextRealization(
    _In_ const XRECTF& brushBounds,
    _In_ HWTextRealization *pHwTextRealization,
    bool allowAnimatedColor)
{
    XRECTF textBounds = { 0 };
    XFLOAT brushOpacity = 1.0f;
    XUINT32 brushMask = 0xffffffff;
    bool hasBrush;
    CMILMatrix brushTextureTransform(TRUE);

    const HWRenderParams& rp = *m_pRenderParams;
    HWRenderParams localRP = rp;
    HWRenderParamsOverride hwrpOverride(this, &localRP);

    HWTexture * pTextHwTextureNoRef = pHwTextRealization->GetTextHwTexture();
    ASSERT(pTextHwTextureNoRef != NULL);

    xref_ptr<HWTexture> brushHwTexture;

    //
    // Rasterized content is produced at the scale of the world transform plus sub-pixel offset.
    // Calculate adjusted realization transform by extracting the remainder of the transform.
    //
    CMILMatrix realizationAdjustedTransform(TRUE);
    XFLOAT realizationScaleX, realizationScaleY;

    pHwTextRealization->GetRealizationScale(&realizationScaleX, &realizationScaleY);
    realizationAdjustedTransform.SetDx(static_cast<XFLOAT>(pHwTextRealization->GetSurfaceOffsetX()));
    realizationAdjustedTransform.SetDy(static_cast<XFLOAT>(pHwTextRealization->GetSurfaceOffsetY()));
    realizationAdjustedTransform.Scale(1.0f / realizationScaleX, 1.0f / realizationScaleY);

    TransformAndClipStack transformsAndClips;
    IFC_RETURN(transformsAndClips.Set(m_pRenderParams->pTransformsAndClipsToCompNode));
    transformsAndClips.PrependTransform(realizationAdjustedTransform);
    localRP.pTransformsAndClipsToCompNode = &transformsAndClips;

    textBounds.Width = static_cast<XFLOAT>(pTextHwTextureNoRef->GetWidth());
    textBounds.Height = static_cast<XFLOAT>(pTextHwTextureNoRef->GetHeight());

    //
    // Get the parameters required for the proper brush handling.
    //
    IFC_RETURN(GetBrushParameters(
        m_pRenderParams->pHWWalk->GetTextureManager(),
        m_pRenderParams->pHWWalk->GetSurfaceCache(),
        pHwTextRealization->GetForegroundBrush(),
        false /* setColorOnPrimitive - we'll use a WUC color brush instead */,
        m_pUIElementNoRef,
        nullptr, /* pNinegrid */
        &textBounds,
        &brushBounds,
        &brushOpacity,
        &brushMask,
        &hasBrush,
        brushHwTexture.ReleaseAndGetAddressOf(),
        &brushTextureTransform));

    //
    // NOTE: hasBrush is ignored.  We always create the appropriate WinRT objects, even
    // if they currently aren't able to render anything.  This is likely to be a transient
    // condition, and DWM will appropriately skip rendering and hittesting fully transparent
    // and surface-less brushes.
    //

    localRP.opacityToCompNode *= brushOpacity;
    if (OpacityToBlendInt(localRP.opacityToCompNode) > 0)
    {
        // Text is drawing aliased, because the gutter edge of the text surface is left transparent.
        // There's no need to anti-alias the geometry for primarily-transparent edges.
        // The only time anything is drawn into the gutter is when a text bitmap is large and is split
        // across adjacent surfaces - in that case, the intent is to draw the surfaces adjacent to each other
        // without seams, so the edge pixel is duplicated into each surface's gutter region when creating the
        // bitmaps, which should allow for seamless sampling across the gap with aliased rendering.
        // Aliased rendering is also cheaper - it allows for less geometry tesselation for the DWM.

        if (pHwTextRealization->IsColorBitmap())
        {
            // Dummy ImageBrush to cause WUCBrushManager to produce a WUC CompositionSurfaceBrush
            CREATEPARAMETERS cp(m_pUIElementNoRef->GetContext());
            xref_ptr<CImageBrush> dummyBrush;
            IFC_RETURN(CImageBrush::Create(reinterpret_cast<CDependencyObject**>(dummyBrush.ReleaseAndGetAddressOf()), &cp));

            // Scale the color bitmap to fill the rect
            CMILMatrix brushTransform(TRUE);
            brushTransform.SetM11(textBounds.Width);
            brushTransform.SetM22(textBounds.Height);

            BrushParams emptyBrushParams;
            IFC_RETURN(RenderPrimitive(
                nullptr,                    // element
                textBounds,                 // rect
                brushBounds,
                AAEdge_None,                // antialias
                dummyBrush,                 // brush
                emptyBrushParams,
                nullptr,                    // maskTexture
                pTextHwTextureNoRef,        // brushTexture
                &brushTransform,            // brushTransform
                nullptr,                    // nineGrid
                false                       // isCenterHollow
            ));
        }
        else
        {
            BrushParams emptyBrushParams;
            IFC_RETURN(RenderPrimitive(
                nullptr,                                    // element
                textBounds,                                 // rect
                brushBounds,
                AAEdge_None,                                // antialias
                pHwTextRealization->GetForegroundBrush(),   // brush
                emptyBrushParams,
                pTextHwTextureNoRef,                        // maskTexture
                brushHwTexture.get(),
                &brushTextureTransform,                     // brushTransform
                nullptr,                                    // nineGrid
                false                                       // isCenterHollow
            ));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT VisualContentRenderer::GeneralImageRenderContent(
    _In_ const XRECTF& rect,
    _In_ const XRECTF& brushBounds,
    _In_ CBrush *pBrush,
    const BrushParams& brushParams,
    _In_opt_ CUIElement *pUIElement,
    _In_opt_ const XTHICKNESS *pNinegrid,
    _In_opt_ HWTexture *pShapeHwTexture,
    _In_ bool fIsHollow)
{
    XFLOAT brushOpacity = 1.0f;
    XUINT32 brushMask = 0xffffffff;
    bool hasBrush;
    CMILMatrix brushTextureTransform(TRUE);

    const HWRenderParams& myRP = *m_pRenderParams;
    HWRenderParams localRP = myRP;
    HWRenderParamsOverride hwrpOverride(this, &localRP);

    xref_ptr<HWTexture> brushHwTexture;

    //
    // Brush bounds are used to compute the brush transform.
    // The brush bounds should be relative to the upper-left corner of the rect to be rendered
    // So if the rect has an offset, the brush bounds must be moved to the space of the rect.
    //
    XRECTF brushBoundsWithoutOffset = brushBounds;
    brushBoundsWithoutOffset.X -= rect.X;
    brushBoundsWithoutOffset.Y -= rect.Y;

    IFC_RETURN(GetBrushParameters(
        myRP.pHWWalk->GetTextureManager(),
        myRP.pHWWalk->GetSurfaceCache(),
        pBrush,
        false /* setColorOnPrimitive - we'll use a WUC color brush instead */,
        pUIElement,
        // Xaml has no scenarios that nine grid stretch both an alpha mask and a brush texture. The only scenario that
        // stretches the brush texture is the Image.NineGrid property, and image doesn't have a shape mask. So if there's
        // a shape mask passed in, the nine grid is meant for the mask and not the brush. Pass nullptr for the brush's
        // nine grid in that case.
        pShapeHwTexture ? nullptr : pNinegrid,
        &rect,
        &brushBoundsWithoutOffset,
        &brushOpacity,
        &brushMask,
        &hasBrush,
        brushHwTexture.ReleaseAndGetAddressOf(),
        &brushTextureTransform));

    //
    // NOTE: hasBrush is ignored.  We always create the appropriate WinRT objects, even
    // if they currently aren't able to render anything.  This is likely to be a transient
    // condition, and DWM will appropriately skip rendering and hittesting fully transparent
    // and surface-less brushes.
    //

    ASSERT(localRP.opacityToCompNode >= 0 && localRP.opacityToCompNode <= 1);
    ASSERT(brushOpacity >= 0 && brushOpacity <= 1);
    localRP.opacityToCompNode *= brushOpacity;

    if (OpacityToBlendInt(localRP.opacityToCompNode) > 0)
    {
        // Ninegrids require that the origin of the rect be (0,0)
        ASSERT((rect.X == 0.0f && rect.Y == 0.0f) || !pNinegrid);

        IFC_RETURN(RenderPrimitive(
            pUIElement,
            rect,
            rect, // brushBounds
            AAEdge_All,
            pBrush,
            brushParams,
            pShapeHwTexture,
            brushHwTexture.get(),
            &brushTextureTransform,
            pNinegrid,
            fIsHollow));
    }

    return S_OK;
}

_Check_return_ HRESULT
VisualContentRenderer::CalculateTileBrushTransform(
    _In_ CTileBrush *pTileBrush,
    _In_ const XRECTF *pBounds,
    _Out_ CMILMatrix *pSourceTextureToElement,
    _Out_ CMILMatrix *pBrushTextureToElement
) const
{
    CMILMatrix identity(TRUE);
    XRECTF imageSize;
    pTileBrush->GetNaturalBounds(&imageSize);

    //
    // Get the brush to element transform in local space pixel coordinates (use identity for the world
    // transform).
    //
    IFC_RETURN(pTileBrush->ComputeDeviceToSource(
        &identity,
        pBounds,
        pBrushTextureToElement
    ));

    pSourceTextureToElement->SetToIdentity();
    pSourceTextureToElement->Scale(imageSize.Width, imageSize.Height);
    pSourceTextureToElement->Append(*pBrushTextureToElement);

    return S_OK;
}

bool VisualContentRenderer::DoesBrushTextureFillRect(
    _In_ const XRECTF& rect,
    _In_ CBrush* brush,
    _In_opt_ HWTexture* maskTexture,
    _In_opt_ HWTexture* brushTexture,
    _In_ const CMILMatrix* brushTransform,
    _In_opt_ const XTHICKNESS* nineGrid)
{
    // Xaml <Image/> elements always generate primitives that are filled exactly by the brush texture. Specifically:
    //    - A 100x100 <Image/> element with a 50x50 image source (after stretch has been applied) will generate a 50x50 primitive.
    //    - A 100x100 <Image/> element with a 200x100 image source (after stretch has been applied) will generate a 200x200 primitive,
    //      clipped down to 100x100.
    //
    // See CMediaBase::MeasureArrangeHelper->CMediaBase::ComputeScaleFactor, which accounts for Uniform/UniformToFill and returns
    // the stretched size to be used as the ActualWidth/ActualHeight of the Image element. That size is then used as the bounds of
    // the primitive.
    //
    // In these cases we'll use WUC's CompositionStretch property to handle the actual filling. This is needed to cover the case of
    // Image.NineGrid, where we put a CompositionSurfaceBrush inside a CompositionNineGridBrush. The surface brush must not have a
    // transform set on it, otherwise it renders nothing.
    //
    // ImageBrush also has a Stretch property, and can generate primitives that aren't entirely filled by the brush texture when used
    // as the fill of a Rectangle.
    bool brushTextureFillsRect = false;

    if (brushTexture != nullptr)
    {
        if (maskTexture == nullptr && nineGrid != nullptr)
        {
            // With this combination we will generate a NineGridBrush with a source SurfaceBrush.  This SurfaceBrush must have fill mode
            // of Stretch otherwise the SpriteVisual will render nothing.  We can't rely on the floating point comparisons done in the else
            // block to guarantee this, particularly when running under high DPI, as the comparisons are not numerically stable.
            brushTextureFillsRect = true;
        }
        else
        {
            brushTextureFillsRect = brushTransform->IsScaleOrTranslationOnly()
                && brushTransform->GetM11() == rect.Width
                && brushTransform->GetM22() == rect.Height
                && std::abs(brushTransform->GetDx()) < 0.001    // stretch calculations can put this at a small decimal
                && std::abs(brushTransform->GetDy()) < 0.001;
        }
    }
    else if (KnownTypeIndex::LinearGradientBrush == brush->GetTypeIndex())
    {
        brushTextureFillsRect = brushTransform->IsIdentity();
    }

    return brushTextureFillsRect;
}

_Check_return_ HRESULT VisualContentRenderer::RenderPrimitive(
    const CUIElement* element,
    _In_ const XRECTF& rect,
    _In_ const XRECTF& brushBounds,
    _In_ AAEdge antialias,
    _In_ CBrush* brush,
    const BrushParams& brushParams,
    _In_opt_ HWTexture* maskTexture,
    _In_opt_ HWTexture* brushTexture,
    _In_ const CMILMatrix* brushTransform,
    _In_opt_ const XTHICKNESS* nineGrid,
    _In_ bool isCenterHollow)
{
    WUComp::ICompositor* pCompositor = m_dcompTreeHostNoRef->GetCompositor();
    ComPtr<WUComp::IVisual> spVisual;
    ComPtr<WUComp::ISpriteVisual> spContentVisual;
    ComPtr<WUComp::ICompositionBrush> existingBrush;
    ComPtr<WUComp::ICompositionBrush> newBrush;

    IFC_RETURN(EnsureSpriteVisual(pCompositor, rect, antialias, &spContentVisual, &spVisual));

    IFC_RETURN(spContentVisual->get_Brush(existingBrush.ReleaseAndGetAddressOf()));

    newBrush = GetCompositionBrush(
        rect,
        brushBounds,
        antialias,
        brush,
        brushParams,
        maskTexture,
        brushTexture,
        brushTransform,
        nineGrid,
        isCenterHollow,
        existingBrush.Get(),
        element);

    if (newBrush != existingBrush)
    {
        IFC_RETURN(spContentVisual->put_Brush(newBrush.Get()));
    }

    IFC_RETURN(LinkVisual(brush, nullptr, spVisual.Get()));

    return S_OK;
}

ComPtr<WUComp::ICompositionBrush> VisualContentRenderer::GetCompositionBrush(
    _In_ const XRECTF& rect,
    _In_ const XRECTF& brushBounds,
    _In_ AAEdge antialias,
    _In_ CBrush* brush,
    const BrushParams& brushParams,
    _In_opt_ HWTexture* maskTexture,
    _In_opt_ HWTexture* brushTexture,
    _In_ const CMILMatrix* brushTransform,
    _In_opt_ const XTHICKNESS* nineGrid,
    _In_ bool isCenterHollow,
    _In_opt_ WUComp::ICompositionBrush* existingBrush,
    _In_ const CUIElement* element)
{
    const auto& wucBrushManager = m_dcompTreeHostNoRef->GetWUCBrushManager();
    ComPtr<WUComp::ICompositionBrush> newBrush;

    bool brushTextureFillsRect = DoesBrushTextureFillRect(rect, brush, maskTexture, brushTexture, brushTransform, nineGrid);

    if (maskTexture != nullptr)
    {
            WUComp::ICompositor* compositor = m_dcompTreeHostNoRef->GetCompositor();
            // Alpha mask code path - WUC mask brush with either:
            // - a nine grid brush as the Mask and a surface/color brush as the Source
            // - a surface brush as the Mask and a nine grid brush as the Source
            // - a surface brush as the Mask and a surface/color brush as the Source
            newBrush = wucBrushManager->GetMaskBrush(
                existingBrush,
                maskTexture,
                brush,
                brushParams,
                brushTexture,
                brushTextureFillsRect ? nullptr : brushTransform,
                nineGrid,
                m_pElementRenderParams->m_realizationScaleX,
                m_pElementRenderParams->m_realizationScaleY,
                isCenterHollow,
                brushBounds,
                element,
                compositor);
    }
    else if (nineGrid != nullptr)
    {
        WUComp::ICompositor* compositor = m_dcompTreeHostNoRef->GetCompositor();
        // Nine grid code path with no mask - the nine grid is meant for a solid color, linear gradient, effect brush, or a brush texture - WUC nine grid brush
        newBrush = wucBrushManager->GetNineGridBrush(
            existingBrush,
            nineGrid,
            m_pElementRenderParams->m_realizationScaleX,
            m_pElementRenderParams->m_realizationScaleY,
            isCenterHollow,
            brush,
            brushTexture,
            brushTextureFillsRect ? nullptr : brushTransform,
            &brushBounds,
            element,
            compositor);
    }
    else
    {
        // No alpha mask and no nine grid - either a WUC surface brush or a WUC color brush or a WUC linear gradient brush or a WUC effect brush.

        // Note: Transparent color brushes with non-0 opacity are supposed to be hittestable without paying the cost of rendering.
        // MSFT: 8230370 <SpriteVisuals Stage 1: Hit-test primitive> tracks this.

        if (KnownTypeIndex::SolidColorBrush == brush->GetTypeIndex())
        {
            newBrush = wucBrushManager->GetColorBrush(static_cast<CSolidColorBrush*>(brush), brushParams);
        }
        else if (KnownTypeIndex::LinearGradientBrush == brush->GetTypeIndex())
        {
            newBrush = wucBrushManager->GetLinearGradientBrush(static_cast<CLinearGradientBrush*>(brush), brushBounds, brushTextureFillsRect ? nullptr : brushTransform);
        }
        else if (KnownTypeIndex::XamlCompositionBrushBase == brush->GetTypeIndex())
        {
            // WUCBrushManager not needed, since CXamlCompositionBrush effectively holds the CompositionBrush.

            wrl::ComPtr<WUComp::ICompositionBrush> compositionBrush;
            xref_ptr<CXamlCompositionBrush> xamlCompositionBrush(static_cast<CXamlCompositionBrush *>(brush));

            IFCFAILFAST(xamlCompositionBrush->GetCompositionBrush(element, m_dcompTreeHostNoRef->GetCompositor(), &compositionBrush));

            if (compositionBrush)
            {
                IFCFAILFAST(compositionBrush.As(&newBrush));
            }
        }
        else
        {
            newBrush = wucBrushManager->GetSurfaceBrush(existingBrush, brushTexture, brushTextureFillsRect ? nullptr : brushTransform);
        }
    }

    return newBrush;
}

ComPtr<WUComp::ICompositionBrush> VisualContentRenderer::GetCompositionBrush(
    _In_ CBrush* brush,
    _In_ CShape* pShapeElement)
{
    XRECTF shapeRect = { 0, 0, pShapeElement->GetActualWidth(), pShapeElement->GetActualHeight() };
    BrushParams brushParams;
    CMILMatrix brushTextureTransform(TRUE);

    return GetCompositionBrush(
            shapeRect,
            shapeRect,
            AAEdge_All,
            brush,
            brushParams,
            nullptr,
            nullptr,
            &brushTextureTransform,
            nullptr,
            false,
            nullptr,
            pShapeElement);
}

bool VisualContentRenderer::ShouldSpriteVisualBeTransparentForInput() const
{
    return !m_pRenderParams->m_isHitTestVisibleSubtree;
}

_Check_return_ HRESULT
VisualContentRenderer::EnsureSpriteVisual(
    _In_ WUComp::ICompositor * pCompositor,
    _In_ const XRECTF& rect,
    _In_ AAEdge antialias,
    _Outptr_ WUComp::ISpriteVisual ** ppContentVisual,
    _Outptr_ WUComp::IVisual ** ppVisual
) const
{
    ComPtr<WUComp::IVisual> spVisual;
    ComPtr<WUComp::ISpriteVisual> spContentVisual;

    if (IsRecycledVisual())
    {
        spVisual = m_RecycledRenderData[m_idxNextReusable];
        IFC_RETURN(spVisual.As(&spContentVisual));
    }
    else
    {
        IFC_RETURN(pCompositor->CreateSpriteVisual(&spContentVisual));
        VERIFYHR(spContentVisual.As(&spVisual));
    }

    IFC_RETURN(PopulateVisual(rect, spVisual.Get()));

    // Antialiasing
    WUComp::CompositionBorderMode borderMode;
    if (antialias == AAEdge_All)
    {
        borderMode = WUComp::CompositionBorderMode::CompositionBorderMode_Soft;
    }
    else
    {
        ASSERT(antialias == AAEdge_None);

        borderMode = WUComp::CompositionBorderMode::CompositionBorderMode_Hard;
    }

    IFC_RETURN(spVisual->put_BorderMode(borderMode));

    // Hit-test visibility
    ComPtr<WUComp::IVisual3> visual3;
    VERIFYHR(spVisual.As(&visual3));
    IFC_RETURN(visual3->put_IsHitTestVisible(!ShouldSpriteVisualBeTransparentForInput()));

    *ppContentVisual = spContentVisual.Detach();
    *ppVisual = spVisual.Detach();

    return S_OK;
}

void VisualContentRenderer::EnsureShapeVisual(
    _In_ CShape * pShapeElement,
    _Outptr_ WUComp::IShapeVisual ** ppContentVisual,
    _Outptr_ WUComp::IVisual ** ppVisual) const
{
    ComPtr<WUComp::IVisual> visual;
    ComPtr<WUComp::IShapeVisual> shapeVisual;

    if (IsRecycledVisual())
    {
        visual = m_RecycledRenderData[m_idxNextReusable];
        IFCFAILFAST(visual.As(&shapeVisual));
    }
    else
    {
        // Create a ShapeVisual
        IFCFAILFAST(GetCompositor5()->CreateShapeVisual(&shapeVisual));
        shapeVisual.As(&visual);
    }

    // Give the ShapeVisual a size and offset
    XRECTF elementBounds = { 0, 0, pShapeElement->GetActualWidth(), pShapeElement->GetActualHeight() };
    IFCFAILFAST(PopulateVisual(elementBounds, visual.Get()));

    // Hit-test visibility
    ComPtr<WUComp::IVisual3> visual3;
    VERIFYHR(visual.As(&visual3));
    IFCFAILFAST(visual3->put_IsHitTestVisible(!ShouldSpriteVisualBeTransparentForInput()));

    *ppContentVisual = shapeVisual.Detach();
    *ppVisual = visual.Detach();
}

_Check_return_ HRESULT VisualContentRenderer::PopulateVisual(
    _In_ const XRECTF& rect,
    _Inout_ WUComp::IVisual* pVisual) const
{
    // Size:
    // The bounds of the rect is the size we're after.
    // TODO_WinRT:  legacy DComp applies localClip to these bounds first, and avoids producing
    // a primitive in the case that the resulting rect is empty.  With visuals we can apply a clip property instead,
    // but we may still want to compute the clipped bounds and produce no visual if the clipped bounds is empty.
    wfn::Vector2 size;
    size.X = rect.Width;
    size.Y = rect.Height;
    IFC_RETURN(pVisual->put_Size(size));

    // Opacity:
    IFC_RETURN(pVisual->put_Opacity(m_pRenderParams->opacityToCompNode));

    // Clip:
    // NOTE: Composition nodes are pushed for all elements with projections, transforms, or clip transforms that
    //           do not preserve axis-alignment. This guarantees that all content on the UI thread is rectangular,
    //           relative to its nearest comp node.
    HWClip localClip; // Infinite by default
    ASSERT(!m_pRenderParams->pTransformsAndClipsToCompNode->HasProjection());
    IFC_RETURN(m_pRenderParams->pTransformsAndClipsToCompNode->TransformToLocalSpace(&localClip));
    IFC_RETURN(EnsureClip(m_dcompTreeHostNoRef->GetCompositor(), pVisual, rect, localClip));

    // Transform:
    // The transform is expected to be translate+scale only, other components would have ended up on a
    // parent visual.
    CMILMatrix transformToCompNode;
    m_pRenderParams->pTransformsAndClipsToCompNode->Get2DTransformInLeafmostProjection(&transformToCompNode);
    ASSERT(transformToCompNode.IsScaleOrTranslationOnly());

    // Scale:
    wfn::Vector3 scale;
    scale.X = transformToCompNode._11;
    scale.Y = transformToCompNode._22;
    scale.Z = 1.0f;
    // Avoid calling put_Scale if not needed. It will create an unnecessary component transform otherwise.
    // We recycle visuals, so we don't know for sure that the scale on the visual is identity. Visual::GetScale will just
    // return the default scale if there is no component transform, so there's no risk in calling it.
    wfn::Vector3 existingScale;
    IFCFAILFAST(pVisual->get_Scale(&existingScale));
    if (scale.X != existingScale.X || scale.Y != existingScale.Y || scale.Z != existingScale.Z)
    {
        IFC_RETURN(pVisual->put_Scale(scale));
    }

    // Offset:
    // First apply the rect offset, if one is provided. Typically, this will be 0, but it can be non-zero
    // if we're building the SpriteVisual from something like a SW-rasterizer-based scenario. Note that 
    // transformToCompNode scale has not yet been applied to this offset, so we need to apply it here.
    wfn::Vector3 offset;
    offset.X = rect.X * scale.X;
    offset.Y = rect.Y * scale.Y;
    offset.Z = 0.0f;    // Would this Z component ever be useful to us?

                        // Then add any transformToCompNode translation component
    offset.X += transformToCompNode._31;
    offset.Y += transformToCompNode._32;

    // Now that we have the combined offset, actually apply it to the visual.
    IFC_RETURN(pVisual->put_Offset(offset));

    return S_OK;
}

_Check_return_ HRESULT
VisualContentRenderer::EnsureClip(
    _In_ WUComp::ICompositor * pCompositor,
    _In_ WUComp::IVisual * pVisual,
    _In_ const XRECTF & rect,
    _In_ const HWClip & localClip
) const
{
    ComPtr<WUComp::ICompositionClip> spCompositionClip;

    if (!localClip.IsInfinite())
    {
        ASSERT(localClip.IsRectangular());
        XRECTF localClipRect;
        localClip.GetRectangularClip(&localClipRect);

        const float leftInset = localClipRect.X - rect.X;
        const float topInset = localClipRect.Y - rect.Y;
        const float rightInset = (rect.X + rect.Width) - (localClipRect.X + localClipRect.Width);
        const float bottomInset = (rect.Y + rect.Height) - (localClipRect.Y + localClipRect.Height);

        // Only set the clip if it actually clips off the rect (i.e. there is a positive inset somewhere).
        if (leftInset > 0 || topInset > 0 || rightInset > 0 || bottomInset > 0)
        {
            ComPtr<WUComp::IInsetClip> spInsetClip;

            if (IsRecycledVisual())
            {
                IFC_RETURN(pVisual->get_Clip(&spCompositionClip));
                if (spCompositionClip != nullptr)
                {
                    IGNOREHR(spCompositionClip.As(&spInsetClip));
                }
            }

            if (spInsetClip == nullptr)
            {
                IFC_RETURN(pCompositor->CreateInsetClip(&spInsetClip));
                VERIFYHR(spInsetClip.As(&spCompositionClip));
            }

            IFC_RETURN(spInsetClip->put_LeftInset(leftInset));
            IFC_RETURN(spInsetClip->put_TopInset(topInset));
            IFC_RETURN(spInsetClip->put_RightInset(rightInset));
            IFC_RETURN(spInsetClip->put_BottomInset(bottomInset));
        }
    }

    IFC_RETURN(pVisual->put_Clip(spCompositionClip.Get()));
    return S_OK;
}

_Check_return_ HRESULT VisualContentRenderer::LinkVisual(
    _In_opt_ CBrush* brush1,
    _In_opt_ CBrush* brush2,
    _In_ WUComp::IVisual* visual)
{
    if (IsRecycledVisual())
    {
        // We don't know where in whose collection this visual is.  Unlink from existing collection.
        // TODO: We could in many cases determine whether the visual already has the right parent.
        // But, that's pointless unless we can reorder an existing element in the collection without removing it first.
        UnparentVisual(visual);

        // We've consumed this cached primitive
        ReleaseInterface(m_RecycledRenderData[m_idxNextReusable]);
        ++m_idxNextReusable;
        ASSERT(m_idxNextReusable <= m_RecycledRenderData.size());
    }

    if (m_spCurrentParentCollection == nullptr)
    {
        // Insert this visual into the right place in the current container.
        IFC_RETURN(m_pRenderParams->pHWWalk->InsertVisualIntoCurrentContainer(visual, m_spCurrentParentCollection.ReleaseAndGetAddressOf()));
    }
    else
    {
        // Insert cheaply if we already know what collection it needs to go into.
        IFC_RETURN(m_pRenderParams->pHWWalk->InsertVisualIntoCollection(visual, m_spCurrentParentCollection.Get()));
    }

    // Store the visual in the optional render data cache
    if (m_pRenderDataList)
    {
#if DBG
        CheckVisualTypeConsistency(visual);
#endif
        IFC_RETURN(m_pRenderDataList->push_back(visual));
        visual->AddRef();
    }

    TargetByLights(brush1, brush2, visual);

    return S_OK;
}

void VisualContentRenderer::TargetByLights(
    _In_opt_ CBrush* brush1,
    _In_opt_ CBrush* brush2,
    _In_ WUComp::IVisual* visual)
{
    // Check if this visual should be targeted by any lights. We need a XamlLight somewhere in the ancestor chain of the element
    // that's creating this visual, and we need the brush used in this visual to request an ID that matches the ID of some light
    // in the ancestor chain.
    // Note: The CUIElement is responsible for removing this visual from the target list when it releases the visual, either from
    // leaving the rendered scene or from releasing the visual from content being dirty.

    bool brush1IsTarget = false;
    bool brush2IsTarget = false;
    if (brush1 != nullptr && brush1->IsLightTarget())
    {
        brush1IsTarget = true;
    }
    if (brush2 != nullptr && brush2->IsLightTarget())
    {
        brush2IsTarget = true;
    }

    if (m_pRenderParams->m_xamlLights != nullptr && (brush1IsTarget || brush2IsTarget))
    {
        auto& targetMap = m_dcompTreeHostNoRef->GetXamlLightTargetMap();

        for (const auto& light : *(m_pRenderParams->m_xamlLights))
        {
            if (light->HasWUCLight() &&
                ((brush1IsTarget && brush1->IsTargetedByLight(light)) ||
                 (brush2IsTarget && brush2->IsTargetedByLight(light))) )
            {
                targetMap.AddTargetVisual(m_pUIElementNoRef, visual, light);
            }
        }
    }
}

void VisualContentRenderer::CheckVisualTypeConsistency(_In_ WUComp::IVisual* newVisual) const
{
    if (m_pRenderDataList->size() == 0)
    {
        return;
    }

    ComPtr<WUComp::IVisual> visual = m_pRenderDataList->unsafe_get_item(0);
    ComPtr<WUComp::ISpriteVisual> spriteVisual;
    ComPtr<WUComp::IShapeVisual> shapeVisual;

    if (SUCCEEDED(visual.As(&spriteVisual)))
    {
        ASSERTSUCCEEDED(newVisual->QueryInterface(IID_PPV_ARGS(spriteVisual.ReleaseAndGetAddressOf())));
    }
    else if (SUCCEEDED(visual.As(&shapeVisual)))
    {
        ASSERTSUCCEEDED(newVisual->QueryInterface(IID_PPV_ARGS(shapeVisual.ReleaseAndGetAddressOf())));
    }
    else
    {
        ASSERT(false);
    }
}

void VisualContentRenderer::UnparentVisual(_In_ WUComp::IVisual* visual)
{
    // Also untarget any lights that are targeting this visual
    auto& targetMap = m_dcompTreeHostNoRef->GetXamlLightTargetMap();
    targetMap.RemoveTargetVisual(m_pUIElementNoRef, visual);

    ComPtr<WUComp::IContainerVisual> spParent;
    IFCFAILFAST(visual->get_Parent(&spParent));
    if (spParent != nullptr)
    {
        ComPtr<WUComp::IVisualCollection> spParentCollection;
        IFCFAILFAST(spParent->get_Children(&spParentCollection));
        IFCFAILFAST(spParentCollection->Remove(visual));
    }
}

void VisualContentRenderer::UnlinkVisual(_In_ WUComp::IVisual * visual)
{
    UnparentVisual(visual);
    ReleaseInterface(visual);
}

bool VisualContentRenderer::IsRecycledVisual() const
{
    // Returns true if the current primitive we are working on comes from the
    // recycle list.  If so, we need to pay attention to its properties not
    // being the default.  It makes sense to call this method only inside
    // a Push/Pop render data list bracket.
    return m_idxNextReusable < m_RecycledRenderData.size();
}

_Check_return_ HRESULT VisualContentRenderer::RenderShape(_In_ CShape *pShapeElement)
{
    ComPtr<WUComp::IVisual> visual;
    ComPtr<WUComp::IShapeVisual> shapeVisual;
    ComPtr<WUComp::ICompositionSpriteShape> sprite;

    EnsureShapeVisual(pShapeElement, &shapeVisual, &visual);

    const KnownTypeIndex typeIndex = pShapeElement->GetTypeIndex();
    switch (typeIndex)
    {
    case KnownTypeIndex::Rectangle:
    case KnownTypeIndex::Ellipse:
    case KnownTypeIndex::Line:
    case KnownTypeIndex::Path:
    case KnownTypeIndex::Polygon:
    case KnownTypeIndex::Polyline:
        IFC_RETURN(pShapeElement->Render(this, &sprite));
        break;
    default:
        // These are the only shapes
        IFCFAILFAST(E_NOTIMPL);
    }

    // Add the new shape to the ShapeVisual's collection of shapes
    wrl::ComPtr<wfc::IVector<WUComp::CompositionShape*>> shapeCollection;
    IFCFAILFAST(shapeVisual->get_Shapes(&shapeCollection));
    // WUCShapes_TODO: It would be better to reuse the sprite if available, but
    // we can only do that once we're saving the MockWUCSpriteCompositionShapes.
    shapeCollection->Clear();

    ComPtr<WUComp::ICompositionShape> compShape;
    sprite.As(&compShape);
    shapeCollection->Append(compShape.Get());

    // Add ShapeVisual into the Visual tree
    IFC_RETURN(LinkVisual(pShapeElement->GetStroke(), pShapeElement->m_pFill, visual.Get()));

    return S_OK;
}

wrl::ComPtr<WUComp::ICompositor5> VisualContentRenderer::GetCompositor5() const
{
    return m_dcompTreeHostNoRef->GetCompositor5();
}

CD2DFactory* VisualContentRenderer::GetSharedD2DFactoryNoRef() const
{
    CD3D11Device *pDeviceNoRef = GetRenderParams()->pRenderTarget->GetGraphicsDeviceManager()->GetGraphicsDevice();

    CD2DFactory *pSharedD2DFactoryNoRef = pDeviceNoRef->GetD2DFactory();
    FAIL_FAST_ASSERT(pSharedD2DFactoryNoRef != nullptr);

    return pSharedD2DFactoryNoRef;
}
