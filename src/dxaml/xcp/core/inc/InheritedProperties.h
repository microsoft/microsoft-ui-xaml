// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ReferenceCount.h>
#include <CDependencyObject.h>
#include <EnumDefs.g.h>

class OpenTypeFeatureSelection;
struct IDWriteTypography;

//------------------------------------------------------------------------
//
//  Types of InheritedProperties property flag.
//
//------------------------------------------------------------------------

enum class InheritedPropertyFlag {
    IsSetLocally,
    IsSetByStyle
};

//------------------------------------------------------------------------
//
//  Typography attached properties
//
//------------------------------------------------------------------------

struct Typography
{
    XINT32                          m_nAnnotationAlternates;
    DirectUI::FontEastAsianLanguage m_EastAsianLanguage;
    DirectUI::FontEastAsianWidths   m_EastAsianWidths;
    XINT32                          m_nStandardSwashes;
    XINT32                          m_nContextualSwashes;
    XINT32                          m_nStylisticAlternates;
    DirectUI::FontCapitals          m_Capitals;
    DirectUI::FontFraction          m_Fraction;
    DirectUI::FontNumeralStyle      m_NumeralStyle;
    DirectUI::FontNumeralAlignment  m_NumeralAlignment;
    DirectUI::FontVariants          m_Variants;
    bool                            m_fEastAsianExpertForms;
    bool                            m_fStandardLigatures;
    bool                            m_fContextualLigatures;
    bool                            m_fDiscretionaryLigatures;
    bool                            m_fHistoricalLigatures;
    bool                            m_fContextualAlternates;
    bool                            m_fStylisticSet1;
    bool                            m_fStylisticSet2;
    bool                            m_fStylisticSet3;
    bool                            m_fStylisticSet4;
    bool                            m_fStylisticSet5;
    bool                            m_fStylisticSet6;
    bool                            m_fStylisticSet7;
    bool                            m_fStylisticSet8;
    bool                            m_fStylisticSet9;
    bool                            m_fStylisticSet10;
    bool                            m_fStylisticSet11;
    bool                            m_fStylisticSet12;
    bool                            m_fStylisticSet13;
    bool                            m_fStylisticSet14;
    bool                            m_fStylisticSet15;
    bool                            m_fStylisticSet16;
    bool                            m_fStylisticSet17;
    bool                            m_fStylisticSet18;
    bool                            m_fStylisticSet19;
    bool                            m_fStylisticSet20;
    bool                            m_fCapitalSpacing;
    bool                            m_fKerning;
    bool                            m_fCaseSensitiveForms;
    bool                            m_fHistoricalForms;
    bool                            m_fSlashedZero;
    bool                            m_fMathematicalGreek;

    Typography();

    _Check_return_ bool IsTypographyDefault() const;
    _Check_return_ bool IsTypographySame(_In_ const Typography *pOther) const;
    _Check_return_ HRESULT UpdateDWriteTypographyFeatures(_Inout_ IDWriteTypography* typography) const;
    _Check_return_ HRESULT GetOpenTypeFeatureSelections(
        _Out_       XLONG                     *pFeatureCount,
        _Outptr_ OpenTypeFeatureSelection **ppFeatureSelections
        ) const;

};

// The measuring method used for text layout.
struct MeasuringMode
{
    enum Enum
    {
        // Text is measured using glyph ideal metrics whose values are independent to the current display resolution.
        Natural,

        // Text is measured using glyph display compatible metrics whose values tuned for the current display resolution.
        GdiClassic,

        // Text is measured using the same glyph display metrics as text measured by GDI using a font
        // created with CLEARTYPE_NATURAL_QUALITY.
        GdiNatural,
    };
};

// Distinct kinds of cache entry for a glyph
struct GlyphCacheKind
{
    enum Enum
    {
        Aliased = 0,  // Single bit per pixel, whole pixel glyph widths
        Subpixel = 1,  // Overscaled glyphs, subpixel glyph widths
        Compatible = 2   // Overscaled glyphs, whole pixel glyph widths using GDI compatible metrics
    };
};

//------------------------------------------------------------------------
//
//  TextOptions attached properties
//
//------------------------------------------------------------------------

struct TextOptions
{
    DirectUI::TextHintingMode    m_textHintingMode;
    DirectUI::TextRenderingMode  m_textRenderingMode;
    DirectUI::TextFormattingMode m_textFormattingMode;

    TextOptions();

    // Returns glyph sizing method to be used for layout measurement
    MeasuringMode::Enum GetMeasuringMode() const;

    // Returns what kind of cache entry to use for these options.
    GlyphCacheKind::Enum GetCacheKind() const;

    static const TextOptions Default;

private:
    TextOptions(
        DirectUI::TextHintingMode    textHintingMode,
        DirectUI::TextRenderingMode  textRenderingMode,
        DirectUI::TextFormattingMode textFormattingMode
        );
};

//------------------------------------------------------------------------
//
//  InheritedProperties
//
//  A storage group which collects inherited properties such as the OpenType
//  typographic feature settings, and the text rendering options.
//
//  The generation counter on this object is compared to the global
//  counter on core, to decide whether non-locally set properties are up to
//  date.
//
//------------------------------------------------------------------------

class InheritedProperties : public CReferenceCount
{
private:
    XUINT32 Release() const;   // Clients must call Disconnect.

public:

    // Disconnect:
    // When releasing an InheritedProperties from a DO it's vital to call
    // DisconnectFrom, and not just call Release.
    // For this reason InheritedProperties does not expose Release,
    // but instead exposes DisconnectFrom(DO) for use in DO code, and
    // ReleaseFromNonDependencyObject for other cases.

    void DisconnectFrom(_In_ CDependencyObject *pFrom);

    static void ReleaseFromNonDependencyObject(
        _Inout_ const InheritedProperties **ppInheritedProperties
        );

    static void ReleaseFromNonDependencyObject(
        _Inout_ InheritedProperties **ppInheritedProperties
        );

    static _Check_return_ HRESULT CreateCopy(
        _In_          CDependencyObject    *pWriter,
        _In_          InheritedProperties  *pTemplate,
        _Inout_ InheritedProperties **ppInheritedProperties
        );

    static _Check_return_ HRESULT CreateDefault(
        _In_        CCoreServices        *pCoreServices,
        _Outptr_ InheritedProperties **ppInheritedProperties
        );

    static _Check_return_ HRESULT GetDefaultValue(_In_ KnownPropertyIndex userIndex, _Out_ CValue* pDefaultValue);

    // Property flags for inherited properties are maintained in the
    // inherited property storage group.
    // Since the InheritedProperties storage group is attached to DpendencyObject, this
    // avoids the cost of over 40 property slots on DO, when the vast majority
    // of DOs have no inherited properties.

    void SetPropertyFlag(
        _In_ const CDependencyProperty *pDp,
        InheritedPropertyFlag     flag,
        bool                      fState
        );

    _Check_return_ bool IsPropertyFlagSet(
        _In_ const CDependencyProperty *pDp,
        InheritedPropertyFlag     flag
        );

    _Check_return_ HRESULT CopyNonLocallySetProperties(_In_ const InheritedProperties *pOther);


    // Map an inherited DP to a DO.
    // For example, maps DP=KnownPropertyIndex::Control_FontSize, DO=RichTextBlock
    // to DP=KnownPropertyIndex::RichTextBlock_FontSize.
    static const CDependencyProperty *GetCorrespondingInheritedProperty(
        _In_ CDependencyObject   *pdo,
        _In_ const CDependencyProperty *pdp
        );

    _Check_return_ bool IsOld()
    {
        return m_cGenerationCounter != *m_pCoreInheritedPropGenerationCounter;
    }

    void SetIsUpToDate()
    {
        m_cGenerationCounter = *m_pCoreInheritedPropGenerationCounter;
    }

#if DBG
    static void RecordTextPropertyUsage(_In_ CDependencyObject *pDo);
    static void RecordNoopEnsureTextFormatting();
    static void RecordUpdateEnsureTextFormatting();
    static void RecordCreationEnsureTextFormatting();
    static void RecordPulledEnsureTextFormatting();
    static void TraceDependencyObjectTextPropertyUsage();
#endif

    XUINT32 *m_pCoreInheritedPropGenerationCounter;
    XUINT32  m_cGenerationCounter;

    // An InheritedProperties object may be referenced by a number of dependency
    // objects, but only one may write to it.
    CDependencyObject *m_pWriter;

    // The constituent properties are public so that the property system can
    // set them from within SetValue.
    Typography  m_typography;  // OpenType features
    TextOptions m_textOptions; // Text rendering control

private:
    // Property flags are publically accessible through the SetPropertyFlag and
    // IsPropertyFlagSet methods.
    XUINT64 m_fIsSetLocally;
    XUINT64 m_fIsSetByStyle;

    // Clients must use Create and ReleaseInterface.
    InheritedProperties();
    ~InheritedProperties() override;
    InheritedProperties(_In_ CCoreServices *pCore);
};
