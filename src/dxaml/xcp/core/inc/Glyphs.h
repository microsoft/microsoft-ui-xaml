// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CString;
class FontFace;
class D2DTextDrawingContext;
struct HWTextRenderParams;
class IContentRenderer;

//------------------------------------------------------------------------
//
//  Class:  CGlyphPathBuilder
//
//  Synopsis:
//      Object holding parsed result of Glyphs.Indices property.
//
//------------------------------------------------------------------------

class CGlyphPathBuilder
{
public:
    _Field_size_(m_GlyphCapacity)     XUINT16 *m_pIndices;
    _Field_size_(m_GlyphCapacity)     XFLOAT  *m_pUOffsets;
    _Field_size_(m_GlyphCapacity)     XFLOAT  *m_pVOffsets;
    _Field_size_(m_GlyphCapacity)     XFLOAT  *m_pAdvances;
    _Field_size_(m_CodepointCapacity) XUINT16 *m_pClusterMap;
    XUINT32  m_GlyphCapacity;
    XUINT32  m_CodepointCapacity;
    __range( <=, m_GlyphCapacity)  XUINT32  m_GlyphCount;
    __range( <=, m_CodepointCapacity) XUINT32  m_CodepointCount;

    CGlyphPathBuilder()
    {
        m_pIndices          = NULL;
        m_pUOffsets         = NULL;
        m_pVOffsets         = NULL;
        m_pAdvances         = NULL;
        m_pClusterMap       = NULL;
        m_GlyphCapacity     = 0;
        m_CodepointCapacity = 0;
        m_GlyphCount        = 0;
        m_CodepointCount    = 0;
    }

    ~CGlyphPathBuilder()
    {
        delete [] m_pIndices;
        delete [] m_pUOffsets;
        delete [] m_pVOffsets;
        delete [] m_pAdvances;
        delete [] m_pClusterMap;
    }

    _Check_return_ HRESULT AllocateGlyphCapacity(_In_ XUINT32 requiredCapacity)
    {
        XUINT16 *pNewIndices  = NULL;
        XFLOAT  *pNewUOffsets = NULL;
        XFLOAT  *pNewVOffsets = NULL;
        XFLOAT  *pNewAdvances = NULL;
        XUINT32 newCapacity = requiredCapacity + requiredCapacity/2 + 20;

        if (newCapacity < requiredCapacity) // i.e. in case of integer overflow
        {
            return E_OUTOFMEMORY;
        }

        if (newCapacity < m_GlyphCapacity) // ignore shrinking
        {
            return S_OK;
        }

        pNewIndices  = new XUINT16[newCapacity];
        pNewUOffsets = new XFLOAT[newCapacity];
        pNewVOffsets = new XFLOAT[newCapacity];
        pNewAdvances = new XFLOAT[newCapacity];

        if (m_GlyphCapacity > 0)
        {
            memcpy(pNewIndices,  m_pIndices,  m_GlyphCapacity * sizeof(XUINT16));
            memcpy(pNewUOffsets, m_pUOffsets, m_GlyphCapacity * sizeof(XFLOAT));
            memcpy(pNewVOffsets, m_pVOffsets, m_GlyphCapacity * sizeof(XFLOAT));
            memcpy(pNewAdvances, m_pAdvances, m_GlyphCapacity * sizeof(XFLOAT));
        }

        delete [] m_pIndices;  m_pIndices  = pNewIndices;
        delete [] m_pAdvances; m_pAdvances = pNewAdvances;
        delete [] m_pUOffsets; m_pUOffsets = pNewUOffsets;
        delete [] m_pVOffsets; m_pVOffsets = pNewVOffsets;

        m_GlyphCapacity = newCapacity;

        return S_OK;
    }

    _Check_return_ HRESULT EnsureGlyphCapacity(XUINT32 requiredCapacity)
    {
        if (m_GlyphCapacity <= requiredCapacity)
        {
            return AllocateGlyphCapacity(requiredCapacity);
        }
        else
        {
            return S_OK;
        }
    }

    _Check_return_ HRESULT AllocateCodepointCapacity(XUINT32 requiredCapacity)
    {
        XUINT16 *pNewClusterMap = NULL;
        XUINT32 newCapacity = requiredCapacity + requiredCapacity/2 + 20;

        if (newCapacity < requiredCapacity) // i.e. in case of integer overflow
        {
            return E_OUTOFMEMORY;
        }

        if (newCapacity < m_CodepointCapacity)  // ignore shrinking
        {
            return S_OK;
        }

        pNewClusterMap = new XUINT16[newCapacity];

        if (m_CodepointCapacity > 0)
        {
            memcpy(pNewClusterMap,  m_pClusterMap,  m_CodepointCapacity * sizeof(XUINT16));
        }

        delete [] m_pClusterMap;

        m_pClusterMap       = pNewClusterMap;
        m_CodepointCapacity = newCapacity;

        return S_OK;
    }

    _Check_return_ HRESULT EnsureCodepointCapacity(XUINT32 requiredCapacity)
    {
        if (m_CodepointCapacity <= requiredCapacity)
        {
            return AllocateCodepointCapacity(requiredCapacity);
        }
        else
        {
            return S_OK;
        }
    }
};

//------------------------------------------------------------------------
//
//  Class:  CGlyphs
//
//  Synopsis:
//
//  Object created for <Glyphs> tag.
//
//  The CGlyphs object encapulates all that is required to display a run of
//  characters whcih are all form the same physical font, with the same brush.
//
//------------------------------------------------------------------------

class CGlyphs final : public CFrameworkElement
{
private:
    CGlyphs(_In_ CCoreServices *pCore)
        : CFrameworkElement(pCore)
        , m_bIsSideways(FALSE)
        , m_fGlyphsDirty(FALSE)
        , m_bIsOriginYSet(FALSE)
    {}

    ~CGlyphs() override;

   _Check_return_ HRESULT DetermineGlyphsAndPositions();

   _Check_return_ HRESULT GetOrigin(_Out_ XFLOAT* peOriginX, _Out_ XFLOAT* peOriginY);

protected:
    _Check_return_ HRESULT MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize) override;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) override;

public:

   // Constructor

    DECLARE_CREATE(CGlyphs);

    // CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CGlyphs>::Index;
    }

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    bool IsRightToLeft() final
    {
        // Glyphs are never mirrored. E.g. the horizontal stokes of an 'E' are
        // always to the right of the vertical stroke.
        return false;
    }

public:
    _Check_return_ HRESULT EnterImpl(
        _In_ CDependencyObject *pNamescopeOwner,
        EnterParams params
    ) override;

    void CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp) final;

    // Do not use the FrameworkElement method for storing the actual width/height, which in most cases
    // would be calculated by the layout system. In the case of Glyphs, it refers to the natural size of the text.
    _Check_return_ HRESULT GetActualWidth(_Out_ XFLOAT* peWidth) override;

    _Check_return_ HRESULT GetActualHeight(_Out_ XFLOAT* peHeight) override;

    virtual _Check_return_ HRESULT CalculateBounds();

   _Check_return_ HRESULT EnsureFontFace(); // Sets m_pFontFace according to m_pFontUri

   IPALUri *GetDefaultBaseUri();

protected:
    // Since the Glyphs element is a leaf and draws only a single glyph run, opacity can be applied directly to its edges.
    _Check_return_ bool NWCanApplyOpacityWithoutLayer() override { return true; }

private:
    _Check_return_ HRESULT
    PreRender(
        _Out_ bool *pfNeedRendering
        );

    // Draws GlyphRun into the TextDrawingContext.
    _Check_return_ HRESULT DrawGlyphRun();

    // Creates TextDrawingContext if necessary.
    _Check_return_ HRESULT EnsureTextDrawingContext();

//-----------------------------------------------------------------------------
// PC Methods/Fields
//-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT HWRender(
        _In_ IContentRenderer* pContentRenderer
        );

    void GetIndependentlyAnimatedBrushes(
        _Outptr_ CSolidColorBrush **ppFillBrush,
        _Outptr_ CSolidColorBrush **ppStrokeBrush
        ) override;

//-----------------------------------------------------------------------------
//
//  Bounds and Hit Testing
//
//-----------------------------------------------------------------------------
protected:
    _Check_return_ HRESULT GenerateContentBounds(
        _Out_ XRECTF_RB* pBounds
        ) override;

    _Check_return_ HRESULT HitTestLocalInternal(
        _In_ const XPOINTF& target,
        _Out_ bool* pHit
        ) override;

    _Check_return_ HRESULT HitTestLocalInternal(
        _In_ const HitTestPolygon& target,
        _Out_ bool* pHit
        ) override;

public:
    // CGlyphs specific properties

    CBrush*                         m_pFill                 = nullptr;
    xstring_ptr                     m_strUnicodeString;
    xstring_ptr                     m_strIndices;
    xstring_ptr                     m_strFontUri;
    DirectUI::StyleSimulations      m_nStyleSimulations     = DirectUI::StyleSimulations::None;
    XFLOAT                          m_eFontRenderingEmSize  = 0.0f;
    XFLOAT                          m_eOriginX              = 0.0f;
    XFLOAT                          m_eOriginY              = 0.0f;
    XINT32                          m_colorPaletteIndex     = 0;
    bool                            m_isColorFontEnabled    = true;

private:
    bool m_bIsSideways     : 1;
    bool m_bIsOriginYSet   : 1;
    bool m_fGlyphsDirty    : 1;    // Implies glyphs and positions need recalculaiton

    // Typeface information

    IFssFontFace*                   m_pFontFace             = nullptr;

    // Description of glyphs and their positions
    XUINT32                         m_cGlyphs               = 0;
    XUINT16*                        m_pGlyphIndices         = nullptr;
    XFLOAT*                         m_pUOffsets             = nullptr;  // Individual glyph horizontal offsets
    XFLOAT*                         m_pVOffsets             = nullptr;  // Glyph vertical offsets. NULL if all vertical offsets are 0.0f.

    // Cached glyphrun bounds, in element local space
    XRECTF_WH*                      m_pBounds               = nullptr;  // Includes OriginX and OriginY
    XFLOAT                          m_eAccumulatedAdvance   = 0.0f;     // use for WPF compatible AlignmentBox width

    // Lazily created default base uri
    IPALUri*                        m_pDefaultBaseUri       = nullptr;

    // Created during prerender, used and released during render
    D2DTextDrawingContext*          m_pTextDrawingContext   = nullptr;
};
