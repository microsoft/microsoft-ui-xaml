// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DWrite.h"

//------------------------------------------------------------------------
//
//  Private constructor
//
//------------------------------------------------------------------------

Typography::Typography()
{
    m_nAnnotationAlternates   = 0;
    m_fEastAsianExpertForms   = false;
    m_EastAsianLanguage       = DirectUI::FontEastAsianLanguage::Normal;
    m_EastAsianWidths         = DirectUI::FontEastAsianWidths::Normal;
    m_fStandardLigatures      = true;
    m_fContextualLigatures    = true;
    m_fDiscretionaryLigatures = false;
    m_fHistoricalLigatures    = false;
    m_nStandardSwashes        = 0;
    m_nContextualSwashes      = 0;
    m_fContextualAlternates   = true;
    m_nStylisticAlternates    = 0;
    m_fStylisticSet1          = false;
    m_fStylisticSet2          = false;
    m_fStylisticSet3          = false;
    m_fStylisticSet4          = false;
    m_fStylisticSet5          = false;
    m_fStylisticSet6          = false;
    m_fStylisticSet7          = false;
    m_fStylisticSet8          = false;
    m_fStylisticSet9          = false;
    m_fStylisticSet10         = false;
    m_fStylisticSet11         = false;
    m_fStylisticSet12         = false;
    m_fStylisticSet13         = false;
    m_fStylisticSet14         = false;
    m_fStylisticSet15         = false;
    m_fStylisticSet16         = false;
    m_fStylisticSet17         = false;
    m_fStylisticSet18         = false;
    m_fStylisticSet19         = false;
    m_fStylisticSet20         = false;
    m_Capitals                = DirectUI::FontCapitals::Normal;
    m_fCapitalSpacing         = false;
    m_fKerning                = true;
    m_fCaseSensitiveForms     = false;
    m_fHistoricalForms        = false;
    m_Fraction                = DirectUI::FontFraction::Normal;
    m_NumeralStyle            = DirectUI::FontNumeralStyle::Normal;
    m_NumeralAlignment        = DirectUI::FontNumeralAlignment::Normal;
    m_fSlashedZero            = false;
    m_fMathematicalGreek      = false;
    m_Variants                = DirectUI::FontVariants::Normal;
}


//------------------------------------------------------------------------
//
//  Whether all the typography property values are default
//
//------------------------------------------------------------------------

_Check_return_ bool Typography::IsTypographyDefault() const
{
    return  m_nAnnotationAlternates   == 0
        &&  m_fEastAsianExpertForms   == false
        &&  m_EastAsianLanguage       == DirectUI::FontEastAsianLanguage::Normal
        &&  m_EastAsianWidths         == DirectUI::FontEastAsianWidths::Normal
        &&  m_fStandardLigatures      == true
        &&  m_fContextualLigatures    == true
        &&  m_fDiscretionaryLigatures == false
        &&  m_fHistoricalLigatures    == false
        &&  m_nStandardSwashes        == 0
        &&  m_nContextualSwashes      == 0
        &&  m_fContextualAlternates   == true
        &&  m_nStylisticAlternates    == 0
        &&  m_fStylisticSet1          == false
        &&  m_fStylisticSet2          == false
        &&  m_fStylisticSet3          == false
        &&  m_fStylisticSet4          == false
        &&  m_fStylisticSet5          == false
        &&  m_fStylisticSet6          == false
        &&  m_fStylisticSet7          == false
        &&  m_fStylisticSet8          == false
        &&  m_fStylisticSet9          == false
        &&  m_fStylisticSet10         == false
        &&  m_fStylisticSet11         == false
        &&  m_fStylisticSet12         == false
        &&  m_fStylisticSet13         == false
        &&  m_fStylisticSet14         == false
        &&  m_fStylisticSet15         == false
        &&  m_fStylisticSet16         == false
        &&  m_fStylisticSet17         == false
        &&  m_fStylisticSet18         == false
        &&  m_fStylisticSet19         == false
        &&  m_fStylisticSet20         == false
        &&  m_Capitals                == DirectUI::FontCapitals::Normal
        &&  m_fCapitalSpacing         == false
        &&  m_fKerning                == true
        &&  m_fCaseSensitiveForms     == false
        &&  m_fHistoricalForms        == false
        &&  m_Fraction                == DirectUI::FontFraction::Normal
        &&  m_NumeralStyle            == DirectUI::FontNumeralStyle::Normal
        &&  m_NumeralAlignment        == DirectUI::FontNumeralAlignment::Normal
        &&  m_fSlashedZero            == false
        &&  m_fMathematicalGreek      == false
        &&  m_Variants                == DirectUI::FontVariants::Normal;
}


//------------------------------------------------------------------------
//
//  Whether two Typography's have all the same typography properties
//
//------------------------------------------------------------------------

_Check_return_ bool Typography::IsTypographySame(_In_ const Typography *pOther) const
{
    return  m_nAnnotationAlternates   == pOther->m_nAnnotationAlternates
        &&  m_fEastAsianExpertForms   == pOther->m_fEastAsianExpertForms
        &&  m_EastAsianLanguage       == pOther->m_EastAsianLanguage
        &&  m_EastAsianWidths         == pOther->m_EastAsianWidths
        &&  m_fStandardLigatures      == pOther->m_fStandardLigatures
        &&  m_fContextualLigatures    == pOther->m_fContextualLigatures
        &&  m_fDiscretionaryLigatures == pOther->m_fDiscretionaryLigatures
        &&  m_fHistoricalLigatures    == pOther->m_fHistoricalLigatures
        &&  m_nStandardSwashes        == pOther->m_nStandardSwashes
        &&  m_nContextualSwashes      == pOther->m_nContextualSwashes
        &&  m_fContextualAlternates   == pOther->m_fContextualAlternates
        &&  m_nStylisticAlternates    == pOther->m_nStylisticAlternates
        &&  m_fStylisticSet1          == pOther->m_fStylisticSet1
        &&  m_fStylisticSet2          == pOther->m_fStylisticSet2
        &&  m_fStylisticSet3          == pOther->m_fStylisticSet3
        &&  m_fStylisticSet4          == pOther->m_fStylisticSet4
        &&  m_fStylisticSet5          == pOther->m_fStylisticSet5
        &&  m_fStylisticSet6          == pOther->m_fStylisticSet6
        &&  m_fStylisticSet7          == pOther->m_fStylisticSet7
        &&  m_fStylisticSet8          == pOther->m_fStylisticSet8
        &&  m_fStylisticSet9          == pOther->m_fStylisticSet9
        &&  m_fStylisticSet10         == pOther->m_fStylisticSet10
        &&  m_fStylisticSet11         == pOther->m_fStylisticSet11
        &&  m_fStylisticSet12         == pOther->m_fStylisticSet12
        &&  m_fStylisticSet13         == pOther->m_fStylisticSet13
        &&  m_fStylisticSet14         == pOther->m_fStylisticSet14
        &&  m_fStylisticSet15         == pOther->m_fStylisticSet15
        &&  m_fStylisticSet16         == pOther->m_fStylisticSet16
        &&  m_fStylisticSet17         == pOther->m_fStylisticSet17
        &&  m_fStylisticSet18         == pOther->m_fStylisticSet18
        &&  m_fStylisticSet19         == pOther->m_fStylisticSet19
        &&  m_fStylisticSet20         == pOther->m_fStylisticSet20
        &&  m_Capitals                == pOther->m_Capitals
        &&  m_fCapitalSpacing         == pOther->m_fCapitalSpacing
        &&  m_fKerning                == pOther->m_fKerning
        &&  m_fCaseSensitiveForms     == pOther->m_fCaseSensitiveForms
        &&  m_fHistoricalForms        == pOther->m_fHistoricalForms
        &&  m_Fraction                == pOther->m_Fraction
        &&  m_NumeralStyle            == pOther->m_NumeralStyle
        &&  m_NumeralAlignment        == pOther->m_NumeralAlignment
        &&  m_fSlashedZero            == pOther->m_fSlashedZero
        &&  m_fMathematicalGreek      == pOther->m_fMathematicalGreek
        &&  m_Variants                == pOther->m_Variants;
}



//---------------------------------------------------------------------------
//
//  Build a flat array of opentype feature name + parameter in OTLS format
//  for passing to OTLS GetGlyphs/GetGlyphPositions.
//
//---------------------------------------------------------------------------

_Check_return_ HRESULT TypographyAddFeature(
    _Inout_                             XUINT32                  *pFeatureCount,
    _In_                                XUINT32                   featuresAllocated,
    _In_                                OPENTYPE_TAG              tag,
    _In_                                XLONG                     parameter,
    _Out_writes_opt_(featuresAllocated) TEXTRANGE_FEATURE_RECORD *pFeatures
)
{
    if (pFeatures != NULL)
    {
        IFCEXPECT_ASSERT_RETURN(*pFeatureCount < featuresAllocated);
        pFeatures[*pFeatureCount].tagFeature = tag;
        pFeatures[*pFeatureCount].lParameter = parameter;
    }
    (*pFeatureCount)++;

    return S_OK;
}


_Check_return_ HRESULT TypographyGenerateFeatureList(
    _In_                             const Typography         *pProperties,
    _In_                             XUINT32                  *pFeatureCount,
    _Out_writes_opt_(*pFeatureCount) TEXTRANGE_FEATURE_RECORD *pFeatures
)
{
    XUINT32 featureCount = 0;

    if (pProperties->m_nAnnotationAlternates != 0)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('n','a','l','t'), pProperties->m_nAnnotationAlternates, pFeatures));
    }

    if (pProperties->m_fEastAsianExpertForms)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('e','x','p','t'), 1, pFeatures));
    }

    if (pProperties->m_EastAsianLanguage != DirectUI::FontEastAsianLanguage::Normal)
    {
        switch (pProperties->m_EastAsianLanguage)
        {
        case DirectUI::FontEastAsianLanguage::HojoKanji:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('h','o','j','o'), 1, pFeatures));
            break;

        case DirectUI::FontEastAsianLanguage::Jis04:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('j','p','0','4'), 1, pFeatures));
            break;

        case DirectUI::FontEastAsianLanguage::Jis78:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('j','p','7','8'), 1, pFeatures));
            break;

        case DirectUI::FontEastAsianLanguage::Jis83:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('j','p','8','3'), 1, pFeatures));
            break;

        case DirectUI::FontEastAsianLanguage::Jis90:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('j','p','9','0'), 1, pFeatures));
            break;

        case DirectUI::FontEastAsianLanguage::NlcKanji:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('n','l','c','k'), 1, pFeatures));
            break;

        case DirectUI::FontEastAsianLanguage::Simplified:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','m','p','l'), 1, pFeatures));
            break;

        case DirectUI::FontEastAsianLanguage::Traditional:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('t','r','a','d'), 1, pFeatures));
            break;

        case DirectUI::FontEastAsianLanguage::TraditionalNames:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('t','n','a','m'), 1, pFeatures));
            break;

        default:
            break;
        }
    }

    if (pProperties->m_EastAsianWidths != DirectUI::FontEastAsianWidths::Normal)
    {
        switch (pProperties->m_EastAsianWidths)
        {
        case DirectUI::FontEastAsianWidths::Full:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('f','w','i','d'), 1, pFeatures));
            break;

        case DirectUI::FontEastAsianWidths::Half:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('h','a','l','f'), 1, pFeatures));
            break;

        case DirectUI::FontEastAsianWidths::Proportional:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('p','w','i','d'), 1, pFeatures));
            break;

        case DirectUI::FontEastAsianWidths::Quarter:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('q','w','i','d'), 1, pFeatures));
            break;

        case DirectUI::FontEastAsianWidths::Third:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('t','w','i','d'), 1, pFeatures));
            break;

        default:
            break;
        }
    }

    if (pProperties->m_fStandardLigatures)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('l','i','g','a'), 1, pFeatures));
    }
    else
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('l','i','g','a'), 0, pFeatures));
    }

    if (pProperties->m_fContextualLigatures)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('c','l','i','g'), 1, pFeatures));
    }
    else
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('c','l','i','g'), 0, pFeatures));
    }

    if (pProperties->m_fDiscretionaryLigatures)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('d','l','i','g'), 1, pFeatures));
    }

    if (pProperties->m_fHistoricalLigatures)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('h','l','i','g'), 1, pFeatures));
    }

    if (pProperties->m_nStandardSwashes != 0)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','w','s','h'), pProperties->m_nStandardSwashes, pFeatures));
    }

    if (pProperties->m_nContextualSwashes != 0)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('c','s','w','h'), pProperties->m_nContextualSwashes, pFeatures));
    }

    if (pProperties->m_fContextualAlternates)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('c','a','l','t'), 1, pFeatures));
    }
    else
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('c','a','l','t'), 0, pFeatures));
    }

    if (pProperties->m_nStylisticAlternates != 0)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','a','l','t'), pProperties->m_nStylisticAlternates, pFeatures));
    }

    if (pProperties->m_fStylisticSet1)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','s','0','1'), 1, pFeatures));
    }

    if (pProperties->m_fStylisticSet2)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','s','0','2'), 1, pFeatures));
    }

    if (pProperties->m_fStylisticSet3)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','s','0','3'), 1, pFeatures));
    }

    if (pProperties->m_fStylisticSet4)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','s','0','4'), 1, pFeatures));
    }

    if (pProperties->m_fStylisticSet5)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','s','0','5'), 1, pFeatures));
    }

    if (pProperties->m_fStylisticSet6)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','s','0','6'), 1, pFeatures));
    }

    if (pProperties->m_fStylisticSet7)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','s','0','7'), 1, pFeatures));
    }

    if (pProperties->m_fStylisticSet8)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','s','0','8'), 1, pFeatures));
    }

    if (pProperties->m_fStylisticSet9)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','s','0','9'), 1, pFeatures));
    }

    if (pProperties->m_fStylisticSet10)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','s','1','0'), 1, pFeatures));
    }

    if (pProperties->m_fStylisticSet11)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','s','1','1'), 1, pFeatures));
    }

    if (pProperties->m_fStylisticSet12)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','s','1','2'), 1, pFeatures));
    }

    if (pProperties->m_fStylisticSet13)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','s','1','3'), 1, pFeatures));
    }

    if (pProperties->m_fStylisticSet14)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','s','1','4'), 1, pFeatures));
    }

    if (pProperties->m_fStylisticSet15)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','s','1','5'), 1, pFeatures));
    }

    if (pProperties->m_fStylisticSet16)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','s','1','6'), 1, pFeatures));
    }

    if (pProperties->m_fStylisticSet17)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','s','1','7'), 1, pFeatures));
    }

    if (pProperties->m_fStylisticSet18)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','s','1','8'), 1, pFeatures));
    }

    if (pProperties->m_fStylisticSet19)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','s','1','9'), 1, pFeatures));
    }

    if (pProperties->m_fStylisticSet20)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','s','2','0'), 1, pFeatures));
    }

    if (pProperties->m_Capitals != DirectUI::FontCapitals::Normal)
    {
        switch (pProperties->m_Capitals)
        {
        case DirectUI::FontCapitals::AllSmallCaps:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('c','2','s','c'), 1, pFeatures));
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','m','c','p'), 1, pFeatures));
            break;

        case DirectUI::FontCapitals::SmallCaps:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','m','c','p'), 1, pFeatures));
            break;

        case DirectUI::FontCapitals::AllPetiteCaps:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('c','2','p','c'), 1, pFeatures));
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('p','c','a','p'), 1, pFeatures));
            break;

        case DirectUI::FontCapitals::PetiteCaps:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('p','c','a','p'), 1, pFeatures));
            break;

        case DirectUI::FontCapitals::Unicase:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('u','n','i','c'), 1, pFeatures));
            break;

        case DirectUI::FontCapitals::Titling:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('t','i','t','l'), 1, pFeatures));
            break;

        default:
            break;
        }
    }

    if (pProperties->m_fCapitalSpacing)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('c','p','s','p'), 1, pFeatures));
    }

    if (pProperties->m_fKerning)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('k','e','r','n'), 1, pFeatures));
    }
    else
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('k','e','r','n'), 0, pFeatures));
    }

    if (pProperties->m_fCaseSensitiveForms)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('c','a','s','e'), 1, pFeatures));
    }

    if (pProperties->m_fHistoricalForms)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('h','i','s','t'), 1, pFeatures));
    }

    if (pProperties->m_Fraction != DirectUI::FontFraction::Normal)
    {
        switch (pProperties->m_Fraction)
        {
        case DirectUI::FontFraction::Stacked:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('a','f','r','c'), 1, pFeatures));
            break;

        case DirectUI::FontFraction::Slashed:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('f','r','a','c'), 1, pFeatures));
            break;

        default:
            break;
        }
    }

    if (pProperties->m_NumeralStyle != DirectUI::FontNumeralStyle::Normal)
    {
        switch (pProperties->m_NumeralStyle)
        {
        case DirectUI::FontNumeralStyle::Lining:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('l','n','u','m'), 1, pFeatures));
            break;

        case DirectUI::FontNumeralStyle::OldStyle:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('o','n','u','m'), 1, pFeatures));
            break;

        default:
            break;
        }
    }

    if (pProperties->m_NumeralAlignment != DirectUI::FontNumeralAlignment::Normal)
    {
        switch (pProperties->m_NumeralAlignment)
        {
        case DirectUI::FontNumeralAlignment::Proportional:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('p','n','u','m'), 1, pFeatures));
            break;

        case DirectUI::FontNumeralAlignment::Tabular:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('t','n','u','m'), 1, pFeatures));
            break;

        default:
            break;
        }
    }

    if (pProperties->m_fSlashedZero)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('z','e','r','o'), 1, pFeatures));
    }

    if (pProperties->m_fMathematicalGreek)
    {
        IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('m','g','r','k'), 1, pFeatures));
    }

    if (pProperties->m_Variants != DirectUI::FontVariants::Normal)
    {
        switch (pProperties->m_Variants)
        {
        case DirectUI::FontVariants::Superscript:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','u','p','s'), 1, pFeatures));
            break;

        case DirectUI::FontVariants::Subscript:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','u','b','s'), 1, pFeatures));
            break;

        case DirectUI::FontVariants::Ordinal:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('o','r','d','n'), 1, pFeatures));
            break;

        case DirectUI::FontVariants::Inferior:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('s','i','n','f'), 1, pFeatures));
            break;

        case DirectUI::FontVariants::Ruby:
            IFC_RETURN(TypographyAddFeature(&featureCount, *pFeatureCount, DWRITE_MAKE_OPENTYPE_TAG('r','u','b','y'), 1, pFeatures));
            break;

        default:
            break;
        }
    }

    *pFeatureCount = featureCount;

    return S_OK;
}


_Check_return_ HRESULT Typography::GetOpenTypeFeatureSelections(
    _Out_       XLONG                     *pFeatureCount,
    _Outptr_ OpenTypeFeatureSelection **ppFeatureSelections
) const
{
    HRESULT                   hr           = S_OK;
    XUINT32                   featureCount = 0;
    TEXTRANGE_FEATURE_RECORD *pFeatures    = NULL;

    // Determine how many feature records will be required.

    IFC(TypographyGenerateFeatureList(this, &featureCount, NULL));

    if (featureCount == 0)
    {
        // No features required.

        *pFeatureCount = 0;
        *ppFeatureSelections = NULL;
    }
    else
    {
        // Allocate and populate the feature array.

        pFeatures = new TEXTRANGE_FEATURE_RECORD[featureCount];

        IFC(TypographyGenerateFeatureList(this, &featureCount, pFeatures));

        *pFeatureCount = featureCount;
        *ppFeatureSelections = (OpenTypeFeatureSelection*) pFeatures;
        pFeatures = NULL;
    }


Cleanup:
    delete [] pFeatures;
    RRETURN(hr);
}

template <typename T>
class FieldOffset
{

private:
    size_t offset_;
    
public: 
    FieldOffset(size_t offset)
    {
        offset_ = offset;
    }

    template <typename O>
    T const& ReadFrom(O const* o) const
    {
        auto* p = reinterpret_cast<uint8_t const*>(o) + offset_;
        return *reinterpret_cast<T const*>(p);
    }
};

struct XamlBoolToDWriteFeatureMapping
{
    FieldOffset<bool> field;
    DWRITE_FONT_FEATURE feature;
};

static const XamlBoolToDWriteFeatureMapping mappings[] = {
{offsetof(Typography, m_fEastAsianExpertForms), {DWRITE_FONT_FEATURE_TAG_EXPERT_FORMS, 1}},
{offsetof(Typography, m_fStandardLigatures), {DWRITE_FONT_FEATURE_TAG_STANDARD_LIGATURES, 1}},
{offsetof(Typography, m_fContextualLigatures), {DWRITE_FONT_FEATURE_TAG_CONTEXTUAL_LIGATURES, 1}},
{offsetof(Typography, m_fDiscretionaryLigatures), {DWRITE_FONT_FEATURE_TAG_DISCRETIONARY_LIGATURES, 1}},
{offsetof(Typography, m_fHistoricalLigatures), {DWRITE_FONT_FEATURE_TAG_HISTORICAL_LIGATURES, 1}},
{offsetof(Typography, m_fCapitalSpacing), {DWRITE_FONT_FEATURE_TAG_CAPITAL_SPACING, 1}},
{offsetof(Typography, m_fCaseSensitiveForms), {DWRITE_FONT_FEATURE_TAG_CASE_SENSITIVE_FORMS, 1}},
{offsetof(Typography, m_fHistoricalForms), {DWRITE_FONT_FEATURE_TAG_HISTORICAL_FORMS, 1}},
{offsetof(Typography, m_fSlashedZero), {DWRITE_FONT_FEATURE_TAG_SLASHED_ZERO, 1}},
{offsetof(Typography, m_fMathematicalGreek), {DWRITE_FONT_FEATURE_TAG_MATHEMATICAL_GREEK, 1}},
{offsetof(Typography, m_fStylisticSet1), {DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_1, 1}},
{offsetof(Typography, m_fStylisticSet2), {DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_2, 1}},
{offsetof(Typography, m_fStylisticSet3), {DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_3, 1}},
{offsetof(Typography, m_fStylisticSet4), {DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_4, 1}},
{offsetof(Typography, m_fStylisticSet5), {DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_5, 1}},
{offsetof(Typography, m_fStylisticSet6), {DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_6, 1}},
{offsetof(Typography, m_fStylisticSet7), {DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_7, 1}},
{offsetof(Typography, m_fStylisticSet8), {DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_8, 1}},
{offsetof(Typography, m_fStylisticSet9), {DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_9, 1}},
{offsetof(Typography, m_fStylisticSet10), {DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_10, 1}},
{offsetof(Typography, m_fStylisticSet11), {DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_11, 1}},
{offsetof(Typography, m_fStylisticSet12), {DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_12, 1}},
{offsetof(Typography, m_fStylisticSet13), {DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_13, 1}},
{offsetof(Typography, m_fStylisticSet14), {DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_14, 1}},
{offsetof(Typography, m_fStylisticSet15), {DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_15, 1}},
{offsetof(Typography, m_fStylisticSet16), {DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_16, 1}},
{offsetof(Typography, m_fStylisticSet17), {DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_17, 1}},
{offsetof(Typography, m_fStylisticSet18), {DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_18, 1}},
{offsetof(Typography, m_fStylisticSet19), {DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_19, 1}},
{offsetof(Typography, m_fStylisticSet20), {DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_20, 1}}
};

static const DWRITE_FONT_FEATURE FontEastAsianLanguageFeatures[] = {
    {DWRITE_FONT_FEATURE_TAG_DEFAULT, 0}, // Normal (just placeholder for indexing)
    {DWRITE_FONT_FEATURE_TAG_HOJO_KANJI_FORMS, 1}, 
    {DWRITE_FONT_FEATURE_TAG_JIS04_FORMS, 1}, 
    {DWRITE_FONT_FEATURE_TAG_JIS78_FORMS, 1}, 
    {DWRITE_FONT_FEATURE_TAG_JIS83_FORMS, 1}, 
    {DWRITE_FONT_FEATURE_TAG_JIS90_FORMS, 1}, 
    {DWRITE_FONT_FEATURE_TAG_NLC_KANJI_FORMS, 1}, 
    {DWRITE_FONT_FEATURE_TAG_SIMPLIFIED_FORMS, 1}, 
    {DWRITE_FONT_FEATURE_TAG_TRADITIONAL_FORMS, 1}, 
    {DWRITE_FONT_FEATURE_TAG_TRADITIONAL_NAME_FORMS, 1}
};

static const DWRITE_FONT_FEATURE FontEastAsianWidthsFeatures[] = {
    {DWRITE_FONT_FEATURE_TAG_DEFAULT, 0}, // Normal (just placeholder for indexing)
    {DWRITE_FONT_FEATURE_TAG_FULL_WIDTH, 1}, 
    {DWRITE_FONT_FEATURE_TAG_HALF_FORMS, 1}, 
    {DWRITE_FONT_FEATURE_TAG_PROPORTIONAL_WIDTHS, 1}, 
    {DWRITE_FONT_FEATURE_TAG_QUARTER_WIDTHS, 1}, 
    {DWRITE_FONT_FEATURE_TAG_THIRD_WIDTHS, 1}
};

static const DWRITE_FONT_FEATURE FontVariantsFeatures[] = {
    {DWRITE_FONT_FEATURE_TAG_DEFAULT, 0}, // Normal (just placeholder for indexing)
    {DWRITE_FONT_FEATURE_TAG_SUPERSCRIPT, 1}, 
    {DWRITE_FONT_FEATURE_TAG_SUBSCRIPT, 1}, 
    {DWRITE_FONT_FEATURE_TAG_ORDINALS, 1}, 
    {DWRITE_FONT_FEATURE_TAG_SCIENTIFIC_INFERIORS, 1}, 
    {DWRITE_FONT_FEATURE_TAG_RUBY_NOTATION_FORMS, 1}
};

_Check_return_ HRESULT AddFeatureToIDWriteTypography(_In_ IDWriteTypography* typography, _In_ const DWRITE_FONT_FEATURE  fontFeature)
{
    IFC_RETURN(typography->AddFontFeature(fontFeature));
    return S_OK;
}

_Check_return_ HRESULT Typography::UpdateDWriteTypographyFeatures(_Inout_ IDWriteTypography* typography) const
{
    for (auto const& mapping : mappings)
    {
        if (mapping.field.ReadFrom(this))
        {
            IFC_RETURN(AddFeatureToIDWriteTypography(typography, mapping.feature));
        }
    }

    if (m_nAnnotationAlternates != 0)
    {
        IFC_RETURN(AddFeatureToIDWriteTypography(typography, {DWRITE_FONT_FEATURE_TAG_ALTERNATE_ANNOTATION_FORMS, static_cast<UINT32>(m_nAnnotationAlternates)}));
    }

    if (m_EastAsianLanguage != DirectUI::FontEastAsianLanguage::Normal)
    {
        ASSERT(static_cast<int>(m_EastAsianLanguage) < ARRAYSIZE(FontEastAsianLanguageFeatures));
        IFC_RETURN(AddFeatureToIDWriteTypography(typography, FontEastAsianLanguageFeatures[static_cast<int>(m_EastAsianLanguage)]));
    }
    
    if (m_EastAsianWidths != DirectUI::FontEastAsianWidths::Normal)
    {
        ASSERT(static_cast<int>(m_EastAsianWidths) < ARRAYSIZE(FontEastAsianWidthsFeatures));
        IFC_RETURN(AddFeatureToIDWriteTypography(typography, FontEastAsianWidthsFeatures[static_cast<int>(m_EastAsianWidths)]));

    }

    if (m_nStandardSwashes != 0)
    {
        IFC_RETURN(AddFeatureToIDWriteTypography(typography, {DWRITE_FONT_FEATURE_TAG_SWASH, static_cast<UINT32>(m_nStandardSwashes)}));
    }

    if (m_nContextualSwashes != 0)
    {
        IFC_RETURN(AddFeatureToIDWriteTypography(typography, {DWRITE_FONT_FEATURE_TAG_CONTEXTUAL_SWASH, static_cast<UINT32>(m_nContextualSwashes)}));
    }

    if (m_fContextualAlternates)
    {
        IFC_RETURN(AddFeatureToIDWriteTypography(typography, {DWRITE_FONT_FEATURE_TAG_CONTEXTUAL_ALTERNATES, 1}));
    }
    else
    {
        IFC_RETURN(AddFeatureToIDWriteTypography(typography, {DWRITE_FONT_FEATURE_TAG_CONTEXTUAL_ALTERNATES, 0}));
    }

    if (m_nStylisticAlternates != 0)
    {
        IFC_RETURN(AddFeatureToIDWriteTypography(typography, {DWRITE_FONT_FEATURE_TAG_STYLISTIC_ALTERNATES, static_cast<UINT32>(m_nStylisticAlternates)}));
    }

    if (m_Capitals != DirectUI::FontCapitals::Normal)
    {
        switch (m_Capitals)
        {
        case DirectUI::FontCapitals::AllSmallCaps:
            IFC_RETURN(AddFeatureToIDWriteTypography(typography, {DWRITE_FONT_FEATURE_TAG_SMALL_CAPITALS_FROM_CAPITALS, 1}));
            IFC_RETURN(AddFeatureToIDWriteTypography(typography, {DWRITE_FONT_FEATURE_TAG_SMALL_CAPITALS, 1}));
            break;

        case DirectUI::FontCapitals::SmallCaps:
            IFC_RETURN(AddFeatureToIDWriteTypography(typography, {DWRITE_FONT_FEATURE_TAG_SMALL_CAPITALS, 1}));
            break;

        case DirectUI::FontCapitals::AllPetiteCaps:
            IFC_RETURN(AddFeatureToIDWriteTypography(typography, {DWRITE_FONT_FEATURE_TAG_PETITE_CAPITALS_FROM_CAPITALS, 1}));
            IFC_RETURN(AddFeatureToIDWriteTypography(typography, {DWRITE_FONT_FEATURE_TAG_PETITE_CAPITALS, 1}));
            break;

        case DirectUI::FontCapitals::PetiteCaps:
            IFC_RETURN(AddFeatureToIDWriteTypography(typography, {DWRITE_FONT_FEATURE_TAG_PETITE_CAPITALS, 1}));
            break;

        case DirectUI::FontCapitals::Unicase:
            IFC_RETURN(AddFeatureToIDWriteTypography(typography, {DWRITE_FONT_FEATURE_TAG_UNICASE, 1}));
            break;

        case DirectUI::FontCapitals::Titling:
            IFC_RETURN(AddFeatureToIDWriteTypography(typography, {DWRITE_FONT_FEATURE_TAG_TITLING, 1}));
            break;

        default:
            ASSERT(false);
            break;
        }
    }
 
    if (m_fKerning)
    {
        IFC_RETURN(AddFeatureToIDWriteTypography(typography, {DWRITE_FONT_FEATURE_TAG_KERNING, 1}));
    }
    else
    {
        IFC_RETURN(AddFeatureToIDWriteTypography(typography, {DWRITE_FONT_FEATURE_TAG_KERNING, 0}));
    }

    if (m_Fraction != DirectUI::FontFraction::Normal)
    {
        switch (m_Fraction)
        {
        case DirectUI::FontFraction::Stacked:
            IFC_RETURN(AddFeatureToIDWriteTypography(typography, {DWRITE_FONT_FEATURE_TAG_ALTERNATIVE_FRACTIONS, 1}));
            break;

        case DirectUI::FontFraction::Slashed:
            IFC_RETURN(AddFeatureToIDWriteTypography(typography, {DWRITE_FONT_FEATURE_TAG_FRACTIONS, 1}));
            break;

        default:
            ASSERT(false);
            break;
        }
    }

    if (m_NumeralStyle != DirectUI::FontNumeralStyle::Normal)
    {
        switch (m_NumeralStyle)
        {
        case DirectUI::FontNumeralStyle::Lining:
            IFC_RETURN(AddFeatureToIDWriteTypography(typography, {DWRITE_FONT_FEATURE_TAG_LINING_FIGURES, 1}));
            break;

        case DirectUI::FontNumeralStyle::OldStyle:
            IFC_RETURN(AddFeatureToIDWriteTypography(typography, {DWRITE_FONT_FEATURE_TAG_OLD_STYLE_FIGURES, 1}));
            break;

        default:
            ASSERT(false);
            break;
        }
    }

    if (m_NumeralAlignment != DirectUI::FontNumeralAlignment::Normal)
    {
        switch (m_NumeralAlignment)
        {
        case DirectUI::FontNumeralAlignment::Proportional:
            IFC_RETURN(AddFeatureToIDWriteTypography(typography, {DWRITE_FONT_FEATURE_TAG_PROPORTIONAL_FIGURES, 1}));
            break;

        case DirectUI::FontNumeralAlignment::Tabular:
            IFC_RETURN(AddFeatureToIDWriteTypography(typography, {DWRITE_FONT_FEATURE_TAG_TABULAR_FIGURES, 1}));
            break;

        default:
            ASSERT(false);
            break;
        }
    }

    if (m_Variants != DirectUI::FontVariants::Normal)
    {
        ASSERT(static_cast<int>(m_Variants) < ARRAYSIZE(FontVariantsFeatures));
        IFC_RETURN(AddFeatureToIDWriteTypography(typography, FontVariantsFeatures[static_cast<int>(m_Variants)]));
    }

    return S_OK;
}


