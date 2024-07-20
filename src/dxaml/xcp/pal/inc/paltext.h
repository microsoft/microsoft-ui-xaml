// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Matrix.h"

struct IPALResource;
interface IDWriteNumberSubstitution;
interface IDWriteFactory;
interface IDWriteFactory1;
interface IDWriteFactory2;
interface IDWriteFactory3;
interface IDWriteFactory4;
interface IDWriteFontDownloadListener;
interface IDWriteFontDownloadQueue;
interface IDWriteFontCollection1;

#ifndef DWRITE_E_REMOTEFONT
#define DWRITE_E_REMOTEFONT _HRESULT_TYPEDEF_(0x8898500DL)
#endif

namespace PALText
{
    struct IFontAndScriptServicesFactory;
}

typedef WCHAR MappedDigitString[4];
// Mapping from ASCII numbers to translated native digits unicodes.
struct NumberSubstitutionData
{
    MappedDigitString numerals[10];
    MappedDigitString percentSymbol;
    MappedDigitString decimalSeparator;
    MappedDigitString groupSeparator;
};


struct TextOptions;
struct InputMessage;
enum XEDITKEY;

//--------------------------------------------------------
//
// PAL Text Data Interfaces
//
//--------------------------------------------------------

struct IPALTextServices
{

// TextBox and selection services

    // Provides platform's Font and Script Services representing layer for platform dependent
    // functionality related to fonts handling and Unicode processing.
    virtual _Check_return_ HRESULT CreateFontAndScriptServices(_Outptr_ PALText::IFontAndScriptServicesFactory** ppFontAndScriptServices) = 0;
};

namespace PALText
{
    const XFLOAT DefaultGamma               = 1.8f;
    const XFLOAT DefaultEnhancedContrast    = 0.5f;
    const XFLOAT DefaultClearTypeLevel      = 0.5f;
    const XFLOAT GrayscaleClearTypeLevel    = 0.0f;

    struct IFontFace;

    // Represents a method of rendering glyphs.
    struct RenderingMode
    {
        enum Enum
        {
            // Specifies that the rendering mode is determined automatically based on the font and size.
            Default,

            // Specifies that no anti-aliasing is performed. Each pixel is either set to the foreground
            // color of the text or retains the color of the background.
            Aliased,

            // Specifies ClearType rendering with the same metrics as aliased text. Glyphs can only
            // be positioned on whole-pixel boundaries.
            ClearTypeGdiClassic,

            // Specifies ClearType rendering with the same metrics as text rendering using GDI using a font
            // created with CLEARTYPE_NATURAL_QUALITY. Glyph metrics are closer to their ideal values than
            // with aliased text, but glyphs are still positioned on whole-pixel boundaries.
            ClearTypeGdiNatural,

            // Specifies ClearType rendering with anti-aliasing in the horizontal dimension only. This is
            // typically used with small to medium font sizes (up to 16 ppem).
            ClearTypeNatural,

            // Specifies ClearType rendering with anti-aliasing in both horizontal and vertical dimensions.
            // This is typically used at larger sizes to makes curves and diagonal lines look smoother, at
            // the expense of some softness.
            ClearTypeNaturalSymmetric,

            // Specifies that rendering should bypass the rasterizer and use the outlines directly. This is
            // typically used at very large sizes.
            Outline,
        };
    };

    // Represents the internal structure of a device pixel (i.e., the physical arrangement of red,
    // green, and blue color components) that is assumed for purposes of rendering text.
    struct PixelGeometry
    {
        enum Enum
        {
            // The red, green, and blue color components of each pixel are assumed to occupy the same point.
            Flat,

            // Each pixel comprises three vertical stripes, with red on the left, green in the center, and
            // blue on the right. This is the most common pixel geometry for LCD monitors.
            RGB,

            // Each pixel comprises three vertical stripes, with blue on the left, green in the center, and
            // red on the right.
            BGR,
        };
    };

    // Identifies a type of alpha texture. An alpha texture is a bitmap of alpha values, each
    // representing the darkness (i.e., opacity) of a pixel or subpixel.
    struct TextureType
    {
        enum Enum
        {
            // Specifies an alpha texture for aliased text rendering (i.e., bi-level, where each pixel is either fully opaque or fully transparent),
            // with one byte per pixel.
            Aliased1x1,

            // Specifies an alpha texture for ClearType text rendering, with three bytes per pixel in the horizontal dimension and
            // one byte per pixel in the vertical dimension.
            ClearType3x1,
        };
    };

    // Specifies algorithmic style simulations to be applied to the font face.
    // Bold and oblique simulations can be combined via bitwise OR operation.
    struct FontSimulations
    {
        enum Enum
        {
            // No simulations are performed.
            None    = 0x0000,

            // Algorithmic emboldening is performed.
            Bold    = 0x0001,

            // Algorithmic italicization is performed.
            Oblique = 0x0002,

            // Algorithmic emboldening and italicization is performed.
            BoldOblique = 0x0003,
        };
    };

    // Direction for how reading progresses.
    struct ReadingDirection
    {
        enum Enum
        {
            // Reading progresses from left to right.
            LeftToRight,

            // Reading progresses from right to left.
            RightToLeft,
        };
    };

    // Additional shaping requirements.
    struct ScriptShapes
    {
        enum Enum
        {
            // No additional shaping requirement. Text is shaped with the writing system default behavior.
            Default = 0,

            // Text should leave no visual on display i.e. control or format control characters.
            NoVisual = 1,
        };
    };

    // Line breaking conditions
    struct LineBreakingCondition
    {
        enum Enum
        {
            Neutral     = 0,  // = DWRITE_BREAK_CONDITION_NEUTRAL       = LS brkcondCan    (1)
            CanBreak    = 1,  // = DWRITE_BREAK_CONDITION_CAN_BREAK     = LS brkcondPlease (2)
            MayNotBreak = 2,  // = DWRITE_BREAK_CONDITION_MAY_NOT_BREAK = LS brkcondNever  (0)
            MustBreak   = 3   // = DWRITE_BREAK_CONDITION_MUST_BREAK    = LS brkcondMust   (3)
        };
    };

    // Specifies the metrics of a font face that are applicable to all glyphs within the font face.
    struct FontMetrics
    {
        XUINT16 DesignUnitsPerEm;
        XUINT16 Ascent;
        XUINT16 Descent;
        XINT16  LineGap;
        XUINT16 CapHeight;
        XUINT16 XHeight;
        XINT16  UnderlinePosition;
        XUINT16 UnderlineThickness;
        XINT16  StrikethroughPosition;
        XUINT16 StrikethroughThickness;
        XINT16  GlyphBoxLeft;
        XINT16  GlyphBoxTop;
        XINT16  GlyphBoxRight;
        XINT16  GlyphBoxBottom;
    };

    // Specifies the metrics of an individual glyph. The units depend on how the metrics are obtained.
    struct GlyphMetrics
    {
        XINT32  LeftSideBearing;
        XUINT32 AdvanceWidth;
        XINT32  RightSideBearing;
        XINT32  TopSideBearing;
        XUINT32 AdvanceHeight;
        XINT32  BottomSideBearing;
        XINT32  VerticalOriginY;
    };

    // Association of text and its writing system script as well as some display attributes.
    struct ScriptAnalysis
    {
        XUINT16 script;
        ScriptShapes::Enum shapes;
    };

    // Line break opportunity flags for a single character position
    // This struct must match the DWRITE_LINE_BREAKPOINT struct.
    struct LineBreakpoint
    {
        XUINT8 breakConditionBefore  :2;
        XUINT8 breakConditionAfter   :2;
        XUINT8 isWhitespace          :1;
        XUINT8 isSoftHyphen          :1;
        XUINT8 padding               :2;
    };

    // Optional adjustment to a glyph's position. An glyph offset changes the position of a glyph without affecting
    // the pen position. Offsets are in logical, pre-transform units.
    struct GlyphOffset
    {
        XFLOAT AdvanceOffset;
        XFLOAT AscenderOffset;
    };

    // Contains the information needed by renderers to draw glyph runs.
    // All coordinates are in device independent pixels (DIPs).
    struct GlyphRun
    {
        _Notnull_ IFontFace* FontFace;
        XFLOAT FontEmSize;
        XUINT32 GlyphCount;
        _Field_size_(GlyphCount) XUINT16 const * GlyphIndices;
        _Field_size_opt_(GlyphCount) XFLOAT const * GlyphAdvances;
        _Field_size_opt_(GlyphCount) GlyphOffset const * GlyphOffsets;
        bool IsSideways;
        XUINT32 BidiLevel;
    };

    // Specifies properties used to identify and execute typographic feature in the font.
    struct FontFeature
    {
        XUINT32 NameTag;
        XUINT32 Parameter;
    };

    // Defines a set of typographic features to be applied during shaping.
    // Notice the character range which this feature list spans is specified
    // as a separate parameter to GetGlyphs.
    struct TypographicFeatures
    {
        _Field_size_(FeatureCount) FontFeature* Features;
        XUINT32 FeatureCount;
    };

    // Shaping output properties per input character.
    struct ShapingTextProperties
    {
        XUINT16 IsShapedAlone       : 1;
        XUINT16 Reserved            : 15;
    };

    /// Shaping output properties per output glyph.
    struct ShapingGlyphProperties
    {
        XUINT16 Justification       : 4;
        XUINT16 IsClusterStart      : 1;
        XUINT16 IsDiacritic         : 1;
        XUINT16 IsZeroWidthSpace    : 1;
        XUINT16 Reserved            : 9;
    };

    // The interface that represents an absolute reference to a font face.
    struct IFontFace : public IObject
    {
        // Returns true if the font face is either TrueType or TrueTypeCollection
        virtual bool HasTrueTypeOutlines() = 0;

         // Gets the OpenType Offset for the font
        virtual HRESULT TryGetFontOffset(
            _Outptr_result_bytebuffer_(*pcOffset) const void **ppOffset,
            _Out_ XUINT32 *pcOffset,
            _Out_ bool *pExists
            ) = 0;

        // Determines whether two FontFaces are equal.
        virtual bool Equals(
            _In_ IFontFace *pFontFace
            ) = 0;

        // Obtains the algorithmic style simulation flags of a font face.
        virtual FontSimulations::Enum GetSimulations() = 0;

        // Determines whether the font is a symbol font.
        virtual bool IsSymbolFont() = 0;

        // Obtains design units and common metrics for the font face.
        virtual void GetMetrics(
            _Out_ FontMetrics* pFontFaceMetrics
            ) = 0;

        // Obtains the number of glyphs in the font face.
        virtual XUINT16 GetGlyphCount() = 0;

        // Obtains ideal glyph metrics in font design units.
        virtual HRESULT GetDesignGlyphMetrics(
            _In_reads_(glyphCount) XUINT16 const *pGlyphIndices,
            _In_ XUINT32 glyphCount,
            _Out_writes_(glyphCount) GlyphMetrics *pGlyphMetrics,
            _In_ bool isSideways
            ) = 0;

        // Returns the nominal mapping of UTF-32 Unicode code points to glyph indices
        // as defined by the font 'cmap' table.
        virtual HRESULT GetGlyphIndices(
            _In_reads_(codePointCount) XUINT32 const *pCodePoints,
            _In_ XUINT32 codePointCount,
            _Out_writes_(codePointCount) XUINT16 *pGlyphIndices
            ) = 0;

        // Finds the specified OpenType font table if it exists and returns a pointer to it.
        virtual HRESULT TryGetFontTable(
            _In_ XUINT32 openTypeTableTag,
            _Outptr_result_bytebuffer_(*pTableSize) const void **ppTableData,
            _Out_ XUINT32 *pTableSize,
            _Outptr_ void **ppTableContext,
            _Out_ bool *pExists
            ) = 0;

        // Returns true if the font is monospaced
        virtual bool IsMonospacedFont() = 0;

        // Returns the advances in design units for a sequences of glyphs.
        virtual HRESULT GetDesignGlyphAdvances(
            _In_ XUINT32 glyphCount,
            _In_reads_(glyphCount) XUINT16 const *pGlyphIndices,
            _Out_writes_(glyphCount) XINT32 *pGlyphAdvances,
            _In_ bool isSideways
        ) = 0;

        // Returns the pixel-aligned advances for a sequences of glyphs.
        virtual HRESULT GetGdiCompatibleGlyphAdvances(
            _In_ XFLOAT emSize,
            _In_ XFLOAT pixelsPerDip,
            _In_opt_ CMILMatrix const *pTransform,
            _In_ TextOptions const *pTextOptions,
            _In_ bool isSideways,
            _In_ XUINT32 glyphCount,
            _In_reads_(glyphCount) XUINT16 const *pGlyphIndices,
            _Out_writes_(glyphCount) XINT32 *pGlyphAdvances
        ) = 0;

        // Retrieves the kerning pair adjustments from the font's kern table.
        virtual HRESULT GetKerningPairAdjustments(
            _In_ XUINT32 glyphCount,
            _In_reads_(glyphCount) XUINT16 const* pGlyphIndices,
            _Out_writes_(glyphCount) XINT32* pGlyphAdvanceAdjustments
            ) = 0;

        // Returns whether or not the font supports pair-kerning.
        virtual bool HasKerningPairs() = 0;

        // Returns whether we can bypass full shaping.
        virtual bool CanOptimizeShaping() = 0;

        // Returns true if the font has a COLR table
        virtual bool IsColorFont() = 0;

        virtual BOOL IsCharacterLocal(UINT32 ch) = 0;
    };

    // Represents a set of fonts that share the same design but are differentiated
    // by weight, stretch, and style.
    struct IFontFamily : public IObject
    {
        //  For a given physical font typeface determines which glyph typeface to
        //  use for rendering the missing glyph.
        virtual HRESULT LookupNominalFontFace(
            _In_ XUINT32 weight,
            _In_ XUINT32 style,
            _In_ XUINT32 stretch,
            _Outptr_ IFontFace **ppFontFace
            ) = 0;
    };

    // Encapsulates a collection of fonts.
    struct IFontCollection : public IObject
    {
        // Looks for a physical font in the font collection.
        virtual HRESULT LookupPhysicalFontFamily(
            _In_z_ WCHAR const *pFamilyName,
            _Outptr_result_maybenull_ IFontFamily **ppPhysicalFontFamily
            ) = 0;
    };

    // The interface implemented by the text analyzer's client to provide text to
    // the analyzer.
    struct ITextAnalysisSource : public IObject
    {
        // Get a block of text starting at the specified text position.
        virtual HRESULT GetTextAtPosition(
            _In_ XUINT32 textPosition,
            _Outptr_result_buffer_(*pTextLength) WCHAR const **ppTextString,
            _Out_ XUINT32 *pTextLength
            ) = 0;

        // Get a block of text immediately preceding the specified position.
        virtual HRESULT GetTextBeforePosition(
            _In_ XUINT32 textPosition,
            _Outptr_result_buffer_(*pTextLength) WCHAR const **ppTextString,
            _Out_ XUINT32 *pTextLength
            ) = 0;

        // Get paragraph reading direction.
        virtual ReadingDirection::Enum GetParagraphReadingDirection() = 0;

        // Get locale name on the range affected by it.
        virtual HRESULT GetLocaleName(
            _In_ XUINT32 textPosition,
            _Out_ XUINT32 *pTextLength,
            _Outptr_result_z_ WCHAR const **ppLocaleName
            ) = 0;
        // Get number substitution on the range affected by it.
        virtual HRESULT GetNumberSubstitution(
        _In_ XUINT32 textPosition,
        _Out_ XUINT32* pTextLength,
        _Outptr_ IDWriteNumberSubstitution** ppNumberSubstitution
        ) = 0;

        // Get language fallback list
        virtual HRESULT GetLocaleNameList(
            _In_ UINT32 textPosition,
            _Out_ UINT32* pTextLength,
            _Outptr_result_z_ WCHAR const** pplocaleNameList
        ) = 0;

    };

    // The interface implemented by the text analyzer's client to receive the
    // output of a given text analysis.
    struct ITextAnalysisSink : public IObject
    {
        // Report script analysis for the text range.
        virtual HRESULT SetScriptAnalysis(
            _In_ XUINT32 textPosition,
            _In_ XUINT32 textLength,
            _In_ ScriptAnalysis const *pScriptAnalysis
            ) = 0;

        // Set bidirectional level on the range, called once per each
        virtual HRESULT SetBidiLevel(
            _In_ XUINT32 textPosition,
            _In_ XUINT32 textLength,
            _In_ XUINT8 explicitLevel,
            _In_ XUINT8 resolvedLevel
            ) = 0;
        // Set number substituion on the range.
        virtual HRESULT SetNumberSubstitution(
            _In_ XUINT32 textPosition,
            _In_ XUINT32 textLength,
            _In_ IDWriteNumberSubstitution* pNumberSubstitution) =0;

        // Set line breakpoints for the given range.
        virtual HRESULT SetLineBreakpoints(
            _In_ XUINT32 textPosition,
            _In_ XUINT32 textLength,
            _In_reads_(textLength) PALText::LineBreakpoint const* lineBreakpoints
            ) = 0;
    };

    // Analyzes various text properties for complex script processing.
    struct ITextAnalyzer : public IObject
    {
        // Analyzes a text range for script boundaries.
        virtual HRESULT AnalyzeScript(
            _In_ ITextAnalysisSource *pAnalysisSource,
            _In_ XUINT32 textPosition,
            _In_ XUINT32 textLength,
            _In_ ITextAnalysisSink *pAnalysisSink
            ) = 0;

        // Analyzes a text range for script directionality.
        virtual HRESULT AnalyzeBidi(
            _In_ ITextAnalysisSource *pAnalysisSource,
            _In_ XUINT32 textPosition,
            _In_ XUINT32 textLength,
            _In_ ITextAnalysisSink *pAnalysisSink
            ) = 0;

        // Analyzes a text range for number substitution.
        virtual HRESULT AnalyzeNumberSubstitution(
            _In_ ITextAnalysisSource *pAnalysisSource,
            _In_ XUINT32 textPosition,
            _In_ XUINT32 textLength,
            _In_ ITextAnalysisSink *pAnalysisSink
            ) = 0;

        // Analyzes a text range for line-break opportunities.
        virtual HRESULT AnalyzeLineBreakpoints(
            _In_ ITextAnalysisSource *pAnalysisSource,
            _In_ XUINT32 textPosition,
            _In_ XUINT32 textLength,
            _In_ ITextAnalysisSink *pAnalysisSink
            ) = 0;

        // Determines the complexity of text, and whether or not full script
        // shaping needs to be called (GetGlyphs).
        virtual HRESULT GetTextComplexity(
            _In_reads_(textLength) WCHAR const *pTextString,
            _In_ XUINT32 textLength,
            _In_ IFontFace *pFontFace,
            _Out_ bool* pIsTextSimple,
            _Out_ XUINT32* pTextLengthRead
            ) = 0;

        // Determines the most appropriate reading order based on the content
        virtual HRESULT GetContentReadingDirection(
            _In_ ITextAnalysisSource *pAnalysisSource,
            _In_ XUINT32 textPosition,
            _In_ XUINT32 textLength,
            _Out_ PALText::ReadingDirection::Enum *readingDirection,
            _Out_ bool *isAmbiguousReadingDirection
            ) = 0;

    };

    // Analyzes various glyph properties for complex script processing.
    struct IGlyphAnalyzer : public IObject
    {
        // Parses the input text string and maps it to the set of glyphs and associated
        // glyph data according to the font and the writing system's rendering rules.
        virtual HRESULT GetGlyphs(
            _In_reads_(textLength) WCHAR const *pTextString,
            _In_ XUINT32 textLength,
            _In_ IFontFace *pFontFace,
            _In_ bool isSideways,
            _In_ bool isRightToLeft,
            _In_ ScriptAnalysis const *pScriptAnalysis,
            _In_opt_z_ WCHAR const *pLocaleName,
            _In_opt_ IDWriteNumberSubstitution* numberSubstitution,
            _In_reads_opt_(featureRanges) TypographicFeatures const **ppFeatures,
            _In_reads_opt_(featureRanges) XUINT32 const *pFeatureRangeLengths,
            _In_ XUINT32 featureRanges,
            _Out_writes_(textLength) XUINT16 *pClusterMap,
            _Out_writes_(textLength) ShapingTextProperties *pTextProps,
            _Outptr_result_buffer_(*pGlyphCount) XUINT16 **ppGlyphIndices,
            _Outptr_result_buffer_(*pGlyphCount) ShapingGlyphProperties **ppGlyphProps,
            _Out_ XUINT32 *pGlyphCount
            ) = 0;

        // Place glyphs output from the GetGlyphs method according to the font
        // and the writing system's rendering rules.
        virtual HRESULT GetGlyphPlacements(
            _In_reads_(textLength) WCHAR const *pTextString,
            _In_reads_(textLength) XUINT16 const *pClusterMap,
            _In_reads_(textLength) ShapingTextProperties *pTextProps,
            _In_ XUINT32 textLength,
            _In_reads_(glyphCount) XUINT16 const *pGlyphIndices,
            _In_reads_(glyphCount) ShapingGlyphProperties const *pGlyphProps,
            _In_ XUINT32 glyphCount,
            _In_ IFontFace *pFontFace,
            _In_ XFLOAT fontEmSize,
            _In_ bool isSideways,
            _In_ bool isRightToLeft,
            _In_ const TextOptions *pTextOptions,
            _In_ ScriptAnalysis const *pScriptAnalysis,
            _In_opt_z_ WCHAR const *pLocaleName,
            _In_reads_opt_(featureRanges) TypographicFeatures const **ppFeatures,
            _In_reads_opt_(featureRanges) XUINT32 const *pFeatureRangeLengths,
            _In_ XUINT32 featureRanges,
            _Out_writes_(glyphCount) XFLOAT *pGlyphAdvances,
            _Out_writes_(glyphCount) GlyphOffset *pGlyphOffsets
            ) = 0;

        // Applies spacing between characters, properly adjusting glyph clusters
        // and diacritics.
        virtual HRESULT ApplyCharacterSpacing(
            _In_ XFLOAT characterSpacing,
            _In_ bool spaceLastCharacter,
            _In_ ScriptAnalysis const *pScriptAnalysis,
            _In_ XUINT32 textLength,
            _In_reads_(textLength) WCHAR const *pTextString,
            _In_reads_(textLength) XUINT16 const *pClusterMap,
            _In_ XUINT32 glyphCount,
            _In_reads_(glyphCount) ShapingGlyphProperties const *pGlyphProps,
            _Inout_updates_(glyphCount) XFLOAT *pGlyphAdvances,
            _Inout_updates_(glyphCount) GlyphOffset *pGlyphOffsets
            ) = 0;

        // Checks whether character spacing can be applied to a given string.
        virtual HRESULT GetCharacterSpaceability(
            _In_ ScriptAnalysis const *pScriptAnalysis,
            _In_ XUINT32 textLength,
            _In_reads_(textLength) WCHAR const *pTextString,
            _In_reads_(textLength) XUINT16 const *pClusterMap,
            _In_ XUINT32 glyphCount,
            _In_reads_(glyphCount) ShapingGlyphProperties const *pGlyphProps,
            _Out_writes_(glyphCount) bool *pSpaceabilityMap
            ) = 0;
    };

    // Analyzes various script properties.
    struct IScriptAnalyzer : public IObject
    {
        // Checks whether we should restrict the caret to whole clusters,
        // like Thai and Devanagari. Scripts such as Arabic by default allow
        // navigation between clusters. Others like Thai always navigate
        // across whole clusters.
        virtual HRESULT IsRestrictCaretToClusters(
            _In_ ScriptAnalysis const *pScriptAnalysis,
            _Out_ bool *pIsRestrictCaretToClusters
            ) = 0;
    };

    struct IFontFallback : public IObject
    {
        virtual HRESULT MapCharacters(
            ITextAnalysisSource* pAnalysisSource,
            XUINT32 textPosition,
            XUINT32 textLength,
            _In_opt_ IFontCollection* pBaseFontCollection,
            _In_opt_z_ WCHAR const* pBaseFamilyName,
            XUINT32 baseWeight,
            XUINT32 baseStyle,
            XUINT32 baseStretch,
            _Deref_out_range_(0, textLength) XUINT32* pMappedLength,
            _COM_Outptr_ IFontFace** ppMappedFont,
            _Out_ XFLOAT* pScale
            ) = 0;
    };

    /// <summary>
    /// Range of Unicode codepoints.
    /// </summary>
    struct UNICODE_RANGE
    {
        /// <summary>
        /// The first codepoint in the Unicode range.
        /// </summary>
        XUINT32 first;

        /// <summary>
        /// The last codepoint in the Unicode range.
        /// </summary>
        XUINT32 last;
    };

    struct IFontFallbackBuilder : public IObject
    {
        virtual HRESULT AddMapping(
            _In_reads_(rangesCount) UNICODE_RANGE const* ranges,
            XUINT32 rangesCount,
            _In_reads_(targetFamilyNamesCount) WCHAR const** targetFamilyNames,
            XUINT32 targetFamilyNamesCount,
            _In_opt_ IFontCollection* fontCollection = NULL,
            _In_opt_z_ WCHAR const* localeName = NULL,
            _In_opt_z_ WCHAR const* baseFamilyName = NULL,
            XFLOAT scale = 1.0f
            ) = 0;

        virtual HRESULT AddMappings(
            IFontFallback* fontFallback
            ) = 0;

       virtual HRESULT CreateFontFallback(
            _COM_Outptr_ IFontFallback** fontFallback
            ) = 0;

    };

    // The root factory interface for all Font and Script Services objects.
    struct IFontAndScriptServicesFactory : public IObject
    {
        // Gets a font collection representing the set of installed fonts.
        virtual HRESULT GetSystemFontCollection(
            _Outptr_ PALText::IFontCollection **ppFontCollection
            ) = 0;

        // Creates a font collection from a given uri.
        virtual HRESULT CreateCustomFontCollection(
            _In_ IPALResource* pResource,
            _Outptr_ PALText::IFontCollection **ppFontCollection
            ) = 0;

        // Creates a custom font face.
        virtual HRESULT CreateCustomFontFace(
            _In_ IPALResource* pResource,
            _In_ XUINT32 faceIndex,
            _In_ PALText::FontSimulations::Enum fontSimulations,
            _Outptr_ PALText::IFontFace **ppFontFace
            ) = 0;

        // Return an interface to perform text analysis with.
        virtual HRESULT CreateTextAnalyzer(
            _Outptr_ PALText::ITextAnalyzer **ppTextAnalyzer
            ) = 0;

        // Return an interface to perform glyph analysis with.
        virtual HRESULT CreateGlyphAnalyzer(
            _Outptr_ PALText::IGlyphAnalyzer **ppGlyphAnalyzer
            ) = 0;

        // Return an interface to perform script analysis with.
        virtual HRESULT CreateScriptAnalyzer(
            _Outptr_ PALText::IScriptAnalyzer **ppScriptAnalyzer
            ) = 0;

        virtual HRESULT GetSystemFontFallback(
            _Outptr_ PALText::IFontFallback **ppFontFallback
            ) = 0;

        virtual HRESULT CreateFontFallbackBuilder(
            _Outptr_ PALText::IFontFallbackBuilder **ppFontFallbackBuilder
            ) = 0;
    };
}

typedef PALText::FontSimulations FssFontSimulations;
typedef PALText::ReadingDirection FssReadingDirection;
typedef PALText::ScriptShapes FssScriptShapes;
typedef PALText::FontMetrics FssFontMetrics;
typedef PALText::GlyphMetrics FssGlyphMetrics;
typedef PALText::ScriptAnalysis FssScriptAnalysis;
typedef PALText::GlyphOffset FssGlyphOffset;
typedef PALText::GlyphRun FssGlyphRun;
typedef PALText::TypographicFeatures FssTypographicFeatures;
typedef PALText::ShapingTextProperties FssShapingTextProperties;
typedef PALText::ShapingGlyphProperties FssShapingGlyphProperties;

typedef PALText::IFontFace IFssFontFace;
typedef PALText::IFontFamily IFssFontFamily;
typedef PALText::IFontCollection IFssFontCollection;
typedef PALText::IFontFallback IFssFontFallback;
typedef PALText::IFontFallbackBuilder IFssFontFallbackBuilder;
typedef PALText::IFontAndScriptServicesFactory IPALFontAndScriptServices;
