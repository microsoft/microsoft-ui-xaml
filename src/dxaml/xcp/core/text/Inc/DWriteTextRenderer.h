// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class DWriteTextRenderer final : public IDWriteTextRenderer
{
public:

    DWriteTextRenderer(_In_ D2DTextDrawingContext* pTextDrawingContext, _In_ const xref::weakref_ptr<CTextBlock>& pBrushSource)
    {
        m_pTextDrawingContext = pTextDrawingContext;
        m_pForegroundBrushSource = pBrushSource;
    }

    ~DWriteTextRenderer()
    {
    }

    HRESULT STDMETHODCALLTYPE DrawGlyphRun(
        _In_ void* clientDrawingContext,
        _In_ float baselineOriginX,
        _In_ float baselineOriginY,
        DWRITE_MEASURING_MODE measuringMode,
        _In_ DWRITE_GLYPH_RUN const* glyphRun,
        _In_ DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
        _In_ IUnknown* clientDrawingEffects
        ) throw() override;

    HRESULT STDMETHODCALLTYPE DrawUnderline(
        _In_ void* clientDrawingContext,
        _In_ float baselineOriginX,
        _In_ float baselineOriginY,
        _In_ DWRITE_UNDERLINE const* underline,
        _In_ IUnknown* clientDrawingEffects
        ) throw() override;

    HRESULT STDMETHODCALLTYPE DrawStrikethrough(
        _In_ void* clientDrawingContext,
        _In_ float baselineOriginX,
        _In_ float baselineOriginY,
        _In_ DWRITE_STRIKETHROUGH const* strikethrough,
        _In_ IUnknown* clientDrawingEffects
        ) throw() override;

    HRESULT STDMETHODCALLTYPE DrawInlineObject(
        _In_ void* clientDrawingContext,
        _In_ float originX,
        _In_ float originY,
        _In_ IDWriteInlineObject* inlineObject,
        _In_ BOOL isSideways,
        _In_ BOOL isRightToLeft,
        _In_ IUnknown* clientDrawingEffects
        ) throw() override;

    HRESULT STDMETHODCALLTYPE IsPixelSnappingDisabled(
        _In_opt_ void* clientDrawingContext,
        _Out_ BOOL* isDisabled
        ) throw() override;

    HRESULT STDMETHODCALLTYPE GetCurrentTransform(
        _In_opt_ void* clientDrawingContext,
        _Out_ DWRITE_MATRIX* transform
        ) throw() override;

    HRESULT STDMETHODCALLTYPE GetPixelsPerDip(
        _In_opt_ void* clientDrawingContext,
        _Out_ float* pixelsPerDip
        ) throw() override;

    HRESULT STDMETHODCALLTYPE QueryInterface(IID const& iid, _Out_ void** object) throw() override
    {
        if (iid == __uuidof(IUnknown)
        ||  iid == __uuidof(IDWriteTextRenderer)
            )
        {
            *object = static_cast<IDWriteTextRenderer*>(this);
        }
        else
        {
            *object = nullptr;
            return E_NOINTERFACE;
        }

        AddRef();
        return S_OK;
    }

    unsigned long STDMETHODCALLTYPE AddRef() throw() override
    {
        return 1; // Static stack class
    }

    unsigned long STDMETHODCALLTYPE Release() throw() override
    {
        return 1; // Static stack class
    }
    xref::weakref_ptr<CTextBlock> m_pForegroundBrushSource;
    xref_ptr<D2DTextDrawingContext> m_pTextDrawingContext;
};

