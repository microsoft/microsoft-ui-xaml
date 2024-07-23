// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <D2DTextDrawingContext.h>
#include <DWriteTextRenderer.h>
#include <DWriteFontFace.h>
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <numeric>

//------------------------------------------------------------------------
//
//      Implements a custom DWrite render callback object that draws DWrite glyphs.
//
//------------------------------------------------------------------------
using namespace RuntimeFeatureBehavior;

_Check_return_ HRESULT DWriteTextRenderer::DrawGlyphRun(
    _In_ void* clientDrawingContext,
    _In_ float baselineOriginX,
    _In_ float baselineOriginY,
    DWRITE_MEASURING_MODE measuringMode,
    _In_ DWRITE_GLYPH_RUN const* glyphRun,
    _In_ DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
    _In_ IUnknown* clientDrawingEffects
    ) noexcept
{

    xref_ptr<DWriteFontFace> pFontFace;
    std::unique_ptr<PALText::GlyphRun> pPALGlyphRun;
    std::unique_ptr<XUINT16[]> pGlyphIndices;
    std::unique_ptr<XFLOAT[]> pGlyphAdvances;
    std::unique_ptr<PALText::GlyphOffset[]> pGlyphOffsets;
    bool isClipped = false;
    XPOINTF baselineOrigin = {baselineOriginX, baselineOriginY};

    if (!isClipped)
    {
        //
        // Create a copy of the glyph run data, since this data is stored for later use.
        //
        pGlyphIndices  = std::unique_ptr<XUINT16[]>(new XUINT16[glyphRun->glyphCount]);
        pGlyphAdvances = std::unique_ptr<XFLOAT[]>(new XFLOAT [glyphRun->glyphCount]);
        pGlyphOffsets  = std::unique_ptr<PALText::GlyphOffset[]>(new PALText::GlyphOffset[glyphRun->glyphCount]);
        memcpy(pGlyphIndices.get(),  glyphRun->glyphIndices,  glyphRun->glyphCount * sizeof(XUINT16));
        memcpy(pGlyphAdvances.get(), glyphRun->glyphAdvances, glyphRun->glyphCount * sizeof(XFLOAT));


        if (glyphRun->glyphOffsets)
        {
            memcpy(pGlyphOffsets.get(),  glyphRun->glyphOffsets,  glyphRun->glyphCount * sizeof(PALText::GlyphOffset));
        }
        else
        {
            memset(pGlyphOffsets.get(), 0, glyphRun->glyphCount * sizeof(PALText::GlyphOffset));
        }
        pFontFace = new DWriteFontFace(glyphRun->fontFace, nullptr);
        pPALGlyphRun = std::unique_ptr<PALText::GlyphRun>(new PALText::GlyphRun());
        pPALGlyphRun->FontFace      = pFontFace;
        pPALGlyphRun->FontEmSize    = glyphRun->fontEmSize;
        pPALGlyphRun->GlyphCount    = glyphRun->glyphCount;
        pPALGlyphRun->GlyphIndices  = pGlyphIndices.release();
        pPALGlyphRun->GlyphAdvances = pGlyphAdvances.release();
        pPALGlyphRun->GlyphOffsets  = pGlyphOffsets.release();
        pPALGlyphRun->IsSideways    = !!glyphRun->isSideways;
        pPALGlyphRun->BidiLevel     = glyphRun->bidiLevel;

        static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
        //
        // Draw the glyph run using TextDrawingContext.
        //
        const bool drawTextInGreen = runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeEnabledFeature::DrawDWriteTextLayoutInGreen);

        if (drawTextInGreen)
        {
            IFC_RETURN(m_pTextDrawingContext->DrawGlyphRun(
                       pPALGlyphRun.release(),
                       &baselineOrigin,
                       0xFF00FF00));
        }
        else
        {
            CTextBlock* pTextBlock = m_pForegroundBrushSource.lock();

            // If selection and highlighting are not enabled. Use the simple drawing path.
            // TODO: It may be simpler to unify this logic later in CTextBlock to indicate that
            //                 highlight glyphs must be redrawn.  It will require some refactoring.
            if (!pTextBlock->m_isTextSelectionEnabled &&
                (pTextBlock->m_textHighlighters == nullptr))
            {
                IFC_RETURN(m_pTextDrawingContext->DrawGlyphRun(
                            pPALGlyphRun.release(),
                            &baselineOrigin,
                            m_pForegroundBrushSource));
            }
            // If selection or highlighting is enabled, use a more complex render code path to handle high
            // contrast selection foreground text color.
            else
            {
                Microsoft::WRL::ComPtr<IDWriteTextLayout> dwriteTextLayout;
                IFC_RETURN(pTextBlock->GetDWriteTextLayout(&dwriteTextLayout));

                DWRITE_TEXT_METRICS textMetrics = {};
                IFC_RETURN(dwriteTextLayout->GetMetrics(&textMetrics));

                float pointX;
                float pointY;
                DWRITE_HIT_TEST_METRICS hitTestMetrics = {};
                IFC_RETURN(dwriteTextLayout->HitTestTextPosition(glyphRunDescription->textPosition, false, &pointX, &pointY, &hitTestMetrics)); 

                m_pTextDrawingContext->SetLineInfo(textMetrics.layoutWidth, FALSE, hitTestMetrics.top, hitTestMetrics.height);

                float glyphLength = std::accumulate(pPALGlyphRun->GlyphAdvances, pPALGlyphRun->GlyphAdvances + glyphRun->glyphCount, 0.0f);

                IFC_RETURN(m_pTextDrawingContext->DrawGlyphRun(
                            baselineOrigin,
                            glyphLength,
                            pPALGlyphRun.release(),
                            m_pForegroundBrushSource,
                            nullptr));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT DWriteTextRenderer::DrawUnderline(
    _In_ void* clientDrawingContext,
    _In_ float baselineOriginX,
    _In_ float baselineOriginY,
    _In_ DWRITE_UNDERLINE const* underline,
    _In_ IUnknown* clientDrawingEffects
    ) noexcept
{
    XPOINTF baselineOrigin = {baselineOriginX, baselineOriginY + underline->offset};
    
    IFC_RETURN(RichTextServicesHelper::MapTxErr(m_pTextDrawingContext->DrawLine(
        baselineOrigin, // The start position of the line.
        underline->width, // The width of the line.
        underline->thickness, // Thickness of the line.
        0, // The bidirectional nesting level, this is not necessary for DWTL path.
        m_pForegroundBrushSource, // The source of foreground brush.
        nullptr // Clipping rectangle.
        )));
    return S_OK;
}

_Check_return_ HRESULT DWriteTextRenderer::DrawStrikethrough(
    _In_ void* clientDrawingContext,
    _In_ float baselineOriginX,
    _In_ float baselineOriginY,
    _In_ DWRITE_STRIKETHROUGH const* strikethrough,
    _In_ IUnknown* clientDrawingEffects
    ) noexcept
{
    XPOINTF baselineOrigin = {baselineOriginX, baselineOriginY + strikethrough->offset};
    
    IFC_RETURN(RichTextServicesHelper::MapTxErr(m_pTextDrawingContext->DrawLine(
        baselineOrigin, // The start position of the line.
        strikethrough->width, // The width of the line.
        strikethrough->thickness, // Thickness of the line.
        0, // The bidirectional nesting level, this is not necessary for DWTL path.
        m_pForegroundBrushSource, // The source of foreground brush.
        nullptr // Clipping rectangle.
        )));
    return S_OK;
}

_Check_return_ HRESULT DWriteTextRenderer::DrawInlineObject(
    _In_ void* clientDrawingContext,
    _In_ float originX,
    _In_ float originY,
    _In_ IDWriteInlineObject* inlineObject,
    _In_ BOOL isSideways,
    _In_ BOOL isRightToLeft,
    _In_ IUnknown* clientDrawingEffects
    ) noexcept
{
    RRETURN(inlineObject->Draw(clientDrawingContext,
        this,
        originX,
        originY,
        isSideways,
        isRightToLeft,
        clientDrawingEffects));
}

_Check_return_ HRESULT DWriteTextRenderer::IsPixelSnappingDisabled(
    _In_opt_ void* clientDrawingContext,
    _Out_ BOOL* isDisabled
    ) noexcept
{
    *isDisabled = true; //Xaml handles pixel snapping in D2DTextDrawingContext.
    return S_OK;
}

_Check_return_ HRESULT DWriteTextRenderer::GetCurrentTransform(
    _In_opt_ void* clientDrawingContext,
    _Out_ DWRITE_MATRIX* transform
    ) noexcept
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

_Check_return_ HRESULT DWriteTextRenderer::GetPixelsPerDip(
    _In_opt_ void* clientDrawingContext,
    _Out_ float* pixelsPerDip
    ) noexcept
{
    *pixelsPerDip = 1.0;
    return S_OK;
}


