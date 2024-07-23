// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"






//------------------------------------------------------------------------
//
//  Private constructor
//
//------------------------------------------------------------------------

InheritedProperties::InheritedProperties()
{
    m_pCoreInheritedPropGenerationCounter = NULL;
    m_pWriter = NULL;

    // Property flag bitsets
    m_fIsSetLocally    = 0;
    m_fIsSetByStyle    = 0;
}



//------------------------------------------------------------------------
//
//  Detach from a client dependency object.
//
//  If the client was the owning writer of this InheritedProperties, we clear
//  pWriter, so no future object allocated at the same address could
//  ever erroneously be considered a valid writer.
//
//------------------------------------------------------------------------

void InheritedProperties::DisconnectFrom(_In_ CDependencyObject *pFrom)
{
    if (pFrom == m_pWriter)
    {
        m_pWriter = NULL;
    }
    CReferenceCount::Release();
}

//------------------------------------------------------------------------
//
//  Release from other than a DependencyObject
//
//  Performs the same as ReleaseInterface, named differently to highlight
//  that this must not be called when releasing from a DO.
//
//------------------------------------------------------------------------

void InheritedProperties::ReleaseFromNonDependencyObject(
    _Inout_ const InheritedProperties **ppInheritedProperties
)
{
    if (*ppInheritedProperties != NULL)
    {
        (*ppInheritedProperties)->CReferenceCount::Release();
        *ppInheritedProperties= NULL;
    }
}

void InheritedProperties::ReleaseFromNonDependencyObject(
    _Inout_ InheritedProperties **ppInheritedProperties
)
{
    if (*ppInheritedProperties != NULL)
    {
        (*ppInheritedProperties)->CReferenceCount::Release();
        *ppInheritedProperties= NULL;
    }
}

//------------------------------------------------------------------------
//
//  Create a new InheritedProperties object
//
//  Allocates a new object initialised to default values by copying from
//  TextCore's GetDefaultInheritedProperties.
//
//------------------------------------------------------------------------

// static
_Check_return_ HRESULT InheritedProperties::CreateCopy(
    _In_          CDependencyObject    *pWriter,
    _In_          InheritedProperties  *pTemplate,
    _Inout_ InheritedProperties **ppInheritedProperties
)
{
    HRESULT              hr                   = S_OK;
    InheritedProperties *pInheritedProperties = NULL;

    pInheritedProperties = new InheritedProperties();
   *pInheritedProperties = *pTemplate;
    pInheritedProperties->m_cReferences = 1;
    pInheritedProperties->m_pWriter = pWriter;

    ReleaseFromNonDependencyObject(ppInheritedProperties);
   *ppInheritedProperties = pInheritedProperties;
    RRETURN(hr);//RRETURN_REMOVAL
}



//------------------------------------------------------------------------
//
//  Create a new InheritedProperties object
//
//  Allocates a new object initialised to default values. This method is
//  used just once to create the default values used by Create.
//
//------------------------------------------------------------------------

//static
_Check_return_ HRESULT InheritedProperties::CreateDefault(
    _In_        CCoreServices        *pCore,
    _Outptr_ InheritedProperties **ppDefaultInheritedProperties
)
{
    HRESULT              hr       = S_OK;
    InheritedProperties *pDefault = NULL;

    // Create the InheritedProperties properties object and fill in the default properties
    pDefault = new InheritedProperties();
    pDefault->m_pCoreInheritedPropGenerationCounter = &pCore->m_cInheritedPropGenerationCounter;
    pDefault->m_cGenerationCounter = 0; // Only the default InheritedProperties has generation 0

    *ppDefaultInheritedProperties = pDefault;

    // Guarantee that only the default InheritedProperties object has a zero generation counter.
    pCore->m_cInheritedPropGenerationCounter++;

    RRETURN(hr);//RRETURN_REMOVAL
}



//------------------------------------------------------------------------
//
//  Return the default value for a property
//
//------------------------------------------------------------------------

//static
_Check_return_ HRESULT InheritedProperties::GetDefaultValue(_In_ KnownPropertyIndex userIndex, _Inout_ CValue* pDefaultValue)
{
    switch (userIndex)
    {

    // Typography properties
    case KnownPropertyIndex::Typography_AnnotationAlternates:    pDefaultValue->SetSigned(0);                      break;
    case KnownPropertyIndex::Typography_EastAsianExpertForms:    pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_EastAsianLanguage:       pDefaultValue->SetEnum(static_cast<XUINT32>(DirectUI::FontEastAsianLanguage::Normal));  break;
    case KnownPropertyIndex::Typography_EastAsianWidths:         pDefaultValue->SetEnum(static_cast<XUINT32>(DirectUI::FontEastAsianWidths::Normal));    break;
    case KnownPropertyIndex::Typography_StandardLigatures:       pDefaultValue->SetBool(TRUE);                     break;
    case KnownPropertyIndex::Typography_ContextualLigatures:     pDefaultValue->SetBool(TRUE);                     break;
    case KnownPropertyIndex::Typography_DiscretionaryLigatures:  pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_HistoricalLigatures:     pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_StandardSwashes:         pDefaultValue->SetSigned(0);                      break;
    case KnownPropertyIndex::Typography_ContextualSwashes:       pDefaultValue->SetSigned(0);                      break;
    case KnownPropertyIndex::Typography_ContextualAlternates:    pDefaultValue->SetBool(TRUE);                     break;
    case KnownPropertyIndex::Typography_StylisticAlternates:     pDefaultValue->SetSigned(0);                      break;
    case KnownPropertyIndex::Typography_StylisticSet1:           pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_StylisticSet2:           pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_StylisticSet3:           pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_StylisticSet4:           pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_StylisticSet5:           pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_StylisticSet6:           pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_StylisticSet7:           pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_StylisticSet8:           pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_StylisticSet9:           pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_StylisticSet10:          pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_StylisticSet11:          pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_StylisticSet12:          pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_StylisticSet13:          pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_StylisticSet14:          pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_StylisticSet15:          pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_StylisticSet16:          pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_StylisticSet17:          pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_StylisticSet18:          pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_StylisticSet19:          pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_StylisticSet20:          pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_Capitals:                pDefaultValue->SetEnum(static_cast<XUINT32>(DirectUI::FontCapitals::Normal));           break;
    case KnownPropertyIndex::Typography_CapitalSpacing:          pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_Kerning:                 pDefaultValue->SetBool(TRUE);                     break;
    case KnownPropertyIndex::Typography_CaseSensitiveForms:      pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_HistoricalForms:         pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_Fraction:                pDefaultValue->SetEnum(static_cast<XUINT32>(DirectUI::FontFraction::Normal));           break;
    case KnownPropertyIndex::Typography_NumeralStyle:            pDefaultValue->SetEnum(static_cast<XUINT32>(DirectUI::FontNumeralStyle::Normal));       break;
    case KnownPropertyIndex::Typography_NumeralAlignment:        pDefaultValue->SetEnum(static_cast<XUINT32>(DirectUI::FontNumeralAlignment::Normal));   break;
    case KnownPropertyIndex::Typography_SlashedZero:             pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_MathematicalGreek:       pDefaultValue->SetBool(FALSE);                    break;
    case KnownPropertyIndex::Typography_Variants:                pDefaultValue->SetEnum(static_cast<XUINT32>(DirectUI::FontVariants::Normal));           break;

    // TextOptions properties
    case KnownPropertyIndex::TextOptions_TextHintingMode:          pDefaultValue->SetEnum(static_cast<XUINT32>(DirectUI::TextHintingMode::Fixed));    break;
    case KnownPropertyIndex::TextOptions_TextRenderingMode:        pDefaultValue->SetEnum(static_cast<XUINT32>(DirectUI::TextRenderingMode::Auto));   break;
    case KnownPropertyIndex::TextOptions_TextFormattingMode:       pDefaultValue->SetEnum(static_cast<XUINT32>(DirectUI::TextFormattingMode::Ideal)); break;

    default:
        IFC_RETURN(E_INVALIDARG);
        break;
    }

    return S_OK;
}



//------------------------------------------------------------------------
//
//  Destructor
//
//  Called only by Release, not public.
//
//------------------------------------------------------------------------

InheritedProperties::~InheritedProperties()
{
}


//------------------------------------------------------------------------
//
//  Private function to map a property index to an internal property enum
//
//------------------------------------------------------------------------


enum class InheritedPropertiesProperty
{
    AnnotationAlternates,
    EastAsianExpertForms,
    EastAsianLanguage,
    EastAsianWidths,
    StandardLigatures,
    ContextualLigatures,
    DiscretionaryLigatures,
    HistoricalLigatures,
    StandardSwashes,
    ContextualSwashes,
    ContextualAlternates,
    StylisticAlternates,
    StylisticSet1,
    StylisticSet2,
    StylisticSet3,
    StylisticSet4,
    StylisticSet5,
    StylisticSet6,
    StylisticSet7,
    StylisticSet8,
    StylisticSet9,
    StylisticSet10,
    StylisticSet11,
    StylisticSet12,
    StylisticSet13,
    StylisticSet14,
    StylisticSet15,
    StylisticSet16,
    StylisticSet17,
    StylisticSet18,
    StylisticSet19,
    StylisticSet20,
    Capitals,
    CapitalSpacing,
    Kerning,
    CaseSensitiveForms,
    HistoricalForms,
    Fraction,
    NumeralStyle,
    NumeralAlignment,
    SlashedZero,
    MathematicalGreek,
    Variants,
    InheritedPropertiesPropertyLimit,
    TextHintingMode,
    TextRenderingMode,
    TextFormattingMode,
    MaxPropertyNumber
};
DEFINE_ENUM_FLAG_OPERATORS(InheritedPropertiesProperty);

_Check_return_ InheritedPropertiesProperty ToPropertyNumber(KnownPropertyIndex propertyIndex)
{
    switch (propertyIndex)
    {
    // Typography properties
    case KnownPropertyIndex::Typography_AnnotationAlternates:    return InheritedPropertiesProperty::AnnotationAlternates;    break;
    case KnownPropertyIndex::Typography_EastAsianExpertForms:    return InheritedPropertiesProperty::EastAsianExpertForms;    break;
    case KnownPropertyIndex::Typography_EastAsianLanguage:       return InheritedPropertiesProperty::EastAsianLanguage;       break;
    case KnownPropertyIndex::Typography_EastAsianWidths:         return InheritedPropertiesProperty::EastAsianWidths;         break;
    case KnownPropertyIndex::Typography_StandardLigatures:       return InheritedPropertiesProperty::StandardLigatures;       break;
    case KnownPropertyIndex::Typography_ContextualLigatures:     return InheritedPropertiesProperty::ContextualLigatures;     break;
    case KnownPropertyIndex::Typography_DiscretionaryLigatures:  return InheritedPropertiesProperty::DiscretionaryLigatures;  break;
    case KnownPropertyIndex::Typography_HistoricalLigatures:     return InheritedPropertiesProperty::HistoricalLigatures;     break;
    case KnownPropertyIndex::Typography_StandardSwashes:         return InheritedPropertiesProperty::StandardSwashes;         break;
    case KnownPropertyIndex::Typography_ContextualSwashes:       return InheritedPropertiesProperty::ContextualSwashes;       break;
    case KnownPropertyIndex::Typography_ContextualAlternates:    return InheritedPropertiesProperty::ContextualAlternates;    break;
    case KnownPropertyIndex::Typography_StylisticAlternates:     return InheritedPropertiesProperty::StylisticAlternates;     break;
    case KnownPropertyIndex::Typography_StylisticSet1:           return InheritedPropertiesProperty::StylisticSet1;           break;
    case KnownPropertyIndex::Typography_StylisticSet2:           return InheritedPropertiesProperty::StylisticSet2;           break;
    case KnownPropertyIndex::Typography_StylisticSet3:           return InheritedPropertiesProperty::StylisticSet3;           break;
    case KnownPropertyIndex::Typography_StylisticSet4:           return InheritedPropertiesProperty::StylisticSet4;           break;
    case KnownPropertyIndex::Typography_StylisticSet5:           return InheritedPropertiesProperty::StylisticSet5;           break;
    case KnownPropertyIndex::Typography_StylisticSet6:           return InheritedPropertiesProperty::StylisticSet6;           break;
    case KnownPropertyIndex::Typography_StylisticSet7:           return InheritedPropertiesProperty::StylisticSet7;           break;
    case KnownPropertyIndex::Typography_StylisticSet8:           return InheritedPropertiesProperty::StylisticSet8;           break;
    case KnownPropertyIndex::Typography_StylisticSet9:           return InheritedPropertiesProperty::StylisticSet9;           break;
    case KnownPropertyIndex::Typography_StylisticSet10:          return InheritedPropertiesProperty::StylisticSet10;          break;
    case KnownPropertyIndex::Typography_StylisticSet11:          return InheritedPropertiesProperty::StylisticSet11;          break;
    case KnownPropertyIndex::Typography_StylisticSet12:          return InheritedPropertiesProperty::StylisticSet12;          break;
    case KnownPropertyIndex::Typography_StylisticSet13:          return InheritedPropertiesProperty::StylisticSet13;          break;
    case KnownPropertyIndex::Typography_StylisticSet14:          return InheritedPropertiesProperty::StylisticSet14;          break;
    case KnownPropertyIndex::Typography_StylisticSet15:          return InheritedPropertiesProperty::StylisticSet15;          break;
    case KnownPropertyIndex::Typography_StylisticSet16:          return InheritedPropertiesProperty::StylisticSet16;          break;
    case KnownPropertyIndex::Typography_StylisticSet17:          return InheritedPropertiesProperty::StylisticSet17;          break;
    case KnownPropertyIndex::Typography_StylisticSet18:          return InheritedPropertiesProperty::StylisticSet18;          break;
    case KnownPropertyIndex::Typography_StylisticSet19:          return InheritedPropertiesProperty::StylisticSet19;          break;
    case KnownPropertyIndex::Typography_StylisticSet20:          return InheritedPropertiesProperty::StylisticSet20;          break;
    case KnownPropertyIndex::Typography_Capitals:                return InheritedPropertiesProperty::Capitals;                break;
    case KnownPropertyIndex::Typography_CapitalSpacing:          return InheritedPropertiesProperty::CapitalSpacing;          break;
    case KnownPropertyIndex::Typography_Kerning:                 return InheritedPropertiesProperty::Kerning;                 break;
    case KnownPropertyIndex::Typography_CaseSensitiveForms:      return InheritedPropertiesProperty::CaseSensitiveForms;      break;
    case KnownPropertyIndex::Typography_HistoricalForms:         return InheritedPropertiesProperty::HistoricalForms;         break;
    case KnownPropertyIndex::Typography_Fraction:                return InheritedPropertiesProperty::Fraction;                break;
    case KnownPropertyIndex::Typography_NumeralStyle:            return InheritedPropertiesProperty::NumeralStyle;            break;
    case KnownPropertyIndex::Typography_NumeralAlignment:        return InheritedPropertiesProperty::NumeralAlignment;        break;
    case KnownPropertyIndex::Typography_SlashedZero:             return InheritedPropertiesProperty::SlashedZero;             break;
    case KnownPropertyIndex::Typography_MathematicalGreek:       return InheritedPropertiesProperty::MathematicalGreek;       break;
    case KnownPropertyIndex::Typography_Variants:                return InheritedPropertiesProperty::Variants;                break;
    // TextOptions properties
    case KnownPropertyIndex::TextOptions_TextHintingMode:        return InheritedPropertiesProperty::TextHintingMode;         break;
    case KnownPropertyIndex::TextOptions_TextRenderingMode:      return InheritedPropertiesProperty::TextRenderingMode;       break;
    case KnownPropertyIndex::TextOptions_TextFormattingMode:     return InheritedPropertiesProperty::TextFormattingMode;      break;
    default: ASSERT(FALSE);                                                                 break;
    }

    return InheritedPropertiesProperty::InheritedPropertiesPropertyLimit;
}



//------------------------------------------------------------------------
//
//  Set a property flag
//
//------------------------------------------------------------------------

void InheritedProperties::SetPropertyFlag(
    _In_ const CDependencyProperty *pDp,
    InheritedPropertyFlag flag,
    bool fState
)
{
    // InheritedProperty flags are stored in the bitsets m_fIsSetLocally,
    // m_fIsSetByStyle and m_fHasStyleBinding. These are currently allocated as
    // XUINT64. If the following COMPILE_ASSERT breaks it wwill be necessary to
    // make them larger.
    COMPILE_ASSERT(static_cast<XUINT32>(InheritedPropertiesProperty::MaxPropertyNumber) <= 64);

    ASSERT(pDp != NULL);
    ASSERT((pDp->IsInherited())  &&  (pDp->IsStorageGroup()));

    XUINT32 propertyNumber = static_cast<XUINT32>(ToPropertyNumber(pDp->GetIndex()));

    XUINT64 bitmask = (XUINT64)1 << propertyNumber;

    if (fState)
    {
        // Set the flag
        switch (flag)
        {
        case InheritedPropertyFlag::IsSetLocally:    m_fIsSetLocally |= bitmask; break;
        case InheritedPropertyFlag::IsSetByStyle:    m_fIsSetByStyle    |= bitmask; break;
        default: ASSERT(FALSE);
        }
    }
    else
    {
        // Clear the flag
        switch (flag)
        {
        case InheritedPropertyFlag::IsSetLocally:    m_fIsSetLocally &= ~bitmask; break;
        case InheritedPropertyFlag::IsSetByStyle:    m_fIsSetByStyle    &= ~bitmask; break;
        default: ASSERT(FALSE);                               break;
        }
    }
}

//------------------------------------------------------------------------
//
//  Test a property flag
//
//------------------------------------------------------------------------

_Check_return_ bool InheritedProperties::IsPropertyFlagSet(
    _In_ const CDependencyProperty *pDp,
    InheritedPropertyFlag flag
)
{
    ASSERT(pDp != NULL);
    ASSERT((pDp->IsInherited())  &&  (pDp->IsStorageGroup()));

    XUINT32 propertyNumber = static_cast<XUINT32>(ToPropertyNumber(pDp->GetIndex()));

    XUINT64 bitmask = (XUINT64)1 << propertyNumber;

    switch (flag)
    {
    case InheritedPropertyFlag::IsSetLocally:      return !!(m_fIsSetLocally    & bitmask);
    case InheritedPropertyFlag::IsSetByStyle:      return !!(m_fIsSetByStyle    & bitmask);
    default: ASSERT(FALSE); return false;
    }
}


//------------------------------------------------------------------------
//
//  Copy property values from another InheritedProperties object, except properties
//  that are set locally.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT InheritedProperties::CopyNonLocallySetProperties(
    _In_ const InheritedProperties *pOther
)
{
    HRESULT hr = S_OK;

    ASSERT(m_pWriter != NULL);
    ASSERT(m_pWriter->m_pInheritedProperties == this);

    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::AnnotationAlternates)))) { m_typography.m_nAnnotationAlternates = pOther->m_typography.m_nAnnotationAlternates; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::EastAsianExpertForms)))) { m_typography.m_fEastAsianExpertForms = pOther->m_typography.m_fEastAsianExpertForms; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::EastAsianLanguage)))) { m_typography.m_EastAsianLanguage = pOther->m_typography.m_EastAsianLanguage; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::EastAsianWidths)))) { m_typography.m_EastAsianWidths = pOther->m_typography.m_EastAsianWidths; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::StandardLigatures)))) { m_typography.m_fStandardLigatures = pOther->m_typography.m_fStandardLigatures; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::ContextualLigatures)))) { m_typography.m_fContextualLigatures = pOther->m_typography.m_fContextualLigatures; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::DiscretionaryLigatures)))) { m_typography.m_fDiscretionaryLigatures = pOther->m_typography.m_fDiscretionaryLigatures; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::HistoricalLigatures)))) { m_typography.m_fHistoricalLigatures = pOther->m_typography.m_fHistoricalLigatures; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::StandardSwashes)))) { m_typography.m_nStandardSwashes = pOther->m_typography.m_nStandardSwashes; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::ContextualSwashes)))) { m_typography.m_nContextualSwashes = pOther->m_typography.m_nContextualSwashes; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::ContextualAlternates)))) { m_typography.m_fContextualAlternates = pOther->m_typography.m_fContextualAlternates; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::StylisticAlternates)))) { m_typography.m_nStylisticAlternates = pOther->m_typography.m_nStylisticAlternates; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::StylisticSet1)))) { m_typography.m_fStylisticSet1 = pOther->m_typography.m_fStylisticSet1; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::StylisticSet2)))) { m_typography.m_fStylisticSet2 = pOther->m_typography.m_fStylisticSet2; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::StylisticSet3)))) { m_typography.m_fStylisticSet3 = pOther->m_typography.m_fStylisticSet3; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::StylisticSet4)))) { m_typography.m_fStylisticSet4 = pOther->m_typography.m_fStylisticSet4; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::StylisticSet5)))) { m_typography.m_fStylisticSet5 = pOther->m_typography.m_fStylisticSet5; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::StylisticSet6)))) { m_typography.m_fStylisticSet6 = pOther->m_typography.m_fStylisticSet6; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::StylisticSet7)))) { m_typography.m_fStylisticSet7 = pOther->m_typography.m_fStylisticSet7; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::StylisticSet8)))) { m_typography.m_fStylisticSet8 = pOther->m_typography.m_fStylisticSet8; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::StylisticSet9)))) { m_typography.m_fStylisticSet9 = pOther->m_typography.m_fStylisticSet9; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::StylisticSet10)))) { m_typography.m_fStylisticSet10 = pOther->m_typography.m_fStylisticSet10; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::StylisticSet11)))) { m_typography.m_fStylisticSet11 = pOther->m_typography.m_fStylisticSet11; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::StylisticSet12)))) { m_typography.m_fStylisticSet12 = pOther->m_typography.m_fStylisticSet12; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::StylisticSet13)))) { m_typography.m_fStylisticSet13 = pOther->m_typography.m_fStylisticSet13; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::StylisticSet14)))) { m_typography.m_fStylisticSet14 = pOther->m_typography.m_fStylisticSet14; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::StylisticSet15)))) { m_typography.m_fStylisticSet15 = pOther->m_typography.m_fStylisticSet15; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::StylisticSet16)))) { m_typography.m_fStylisticSet16 = pOther->m_typography.m_fStylisticSet16; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::StylisticSet17)))) { m_typography.m_fStylisticSet17 = pOther->m_typography.m_fStylisticSet17; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::StylisticSet18)))) { m_typography.m_fStylisticSet18 = pOther->m_typography.m_fStylisticSet18; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::StylisticSet19)))) { m_typography.m_fStylisticSet19 = pOther->m_typography.m_fStylisticSet19; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::StylisticSet20)))) { m_typography.m_fStylisticSet20 = pOther->m_typography.m_fStylisticSet20; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::Capitals)))) { m_typography.m_Capitals = pOther->m_typography.m_Capitals; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::CapitalSpacing)))) { m_typography.m_fCapitalSpacing = pOther->m_typography.m_fCapitalSpacing; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::Kerning)))) { m_typography.m_fKerning = pOther->m_typography.m_fKerning; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::CaseSensitiveForms)))) { m_typography.m_fCaseSensitiveForms = pOther->m_typography.m_fCaseSensitiveForms; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::HistoricalForms)))) { m_typography.m_fHistoricalForms = pOther->m_typography.m_fHistoricalForms; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::Fraction)))) { m_typography.m_Fraction = pOther->m_typography.m_Fraction; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::NumeralStyle)))) { m_typography.m_NumeralStyle = pOther->m_typography.m_NumeralStyle; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::NumeralAlignment)))) { m_typography.m_NumeralAlignment = pOther->m_typography.m_NumeralAlignment; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::SlashedZero)))) { m_typography.m_fSlashedZero = pOther->m_typography.m_fSlashedZero; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::MathematicalGreek)))) { m_typography.m_fMathematicalGreek = pOther->m_typography.m_fMathematicalGreek; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::Variants)))) { m_typography.m_Variants = pOther->m_typography.m_Variants; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::TextHintingMode)))) { m_textOptions.m_textHintingMode = pOther->m_textOptions.m_textHintingMode; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::TextRenderingMode)))) { m_textOptions.m_textRenderingMode = pOther->m_textOptions.m_textRenderingMode; }
    if (!(m_fIsSetLocally & ((XUINT64)1 << static_cast<XUINT32>(InheritedPropertiesProperty::TextFormattingMode)))) { m_textOptions.m_textFormattingMode = pOther->m_textOptions.m_textFormattingMode; }

//Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Map an inherited DP to a DO.
//
//  For example, maps DP=KnownPropertyIndex::Control_FontSize, DO=RichTextBlock
//  to DP=KnownPropertyIndex::RichTextBlock_FontSize.
//
//  When tracing a property down an inheritance tree the initial property
//  descriptor may not match the element under inspection. This method
//  identifies the correct property decriptor to use for any DO.
//
//------------------------------------------------------------------------

/*static*/ const CDependencyProperty *InheritedProperties::GetCorrespondingInheritedProperty(
    _In_ CDependencyObject   *pdo,
    _In_ const CDependencyProperty *pdp
)
{
    KnownPropertyIndex propertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty;

    if (pdp->IsInheritedAttachedPropertyInStorageGroup())
    {
        // Inherited attached properties share the same property index across
        // all elements they may attach to.
        return pdp;
    }


    switch (pdp->GetIndex())
    {
    case KnownPropertyIndex::RichTextBlock_CharacterSpacing:
    case KnownPropertyIndex::TextBlock_CharacterSpacing:
    case KnownPropertyIndex::Control_CharacterSpacing:
        case KnownPropertyIndex::TextElement_CharacterSpacing:
            if (pdo->GetTypeIndex() == KnownTypeIndex::RichTextBlock) { propertyIndex = KnownPropertyIndex::RichTextBlock_CharacterSpacing; }
            else if (pdo->GetTypeIndex() == KnownTypeIndex::TextBlock)      { propertyIndex = KnownPropertyIndex::TextBlock_CharacterSpacing; }
            else if (pdo->OfTypeByIndex<KnownTypeIndex::Control>())        { propertyIndex = KnownPropertyIndex::Control_CharacterSpacing; }
            else if (pdo->OfTypeByIndex<KnownTypeIndex::TextElement>())    { propertyIndex = KnownPropertyIndex::TextElement_CharacterSpacing; }
            break;

        case KnownPropertyIndex::RichTextBlock_FontFamily:
        case KnownPropertyIndex::TextBlock_FontFamily:
        case KnownPropertyIndex::Control_FontFamily:
        case KnownPropertyIndex::TextElement_FontFamily:
            if (pdo->GetTypeIndex() == KnownTypeIndex::RichTextBlock) { propertyIndex = KnownPropertyIndex::RichTextBlock_FontFamily; }
            else if (pdo->GetTypeIndex() == KnownTypeIndex::TextBlock)      { propertyIndex = KnownPropertyIndex::TextBlock_FontFamily; }
            else if (pdo->OfTypeByIndex<KnownTypeIndex::Control>())        { propertyIndex = KnownPropertyIndex::Control_FontFamily; }
            else if (pdo->OfTypeByIndex<KnownTypeIndex::TextElement>())    { propertyIndex = KnownPropertyIndex::TextElement_FontFamily; }
            break;

        case KnownPropertyIndex::RichTextBlock_FontSize:
        case KnownPropertyIndex::TextBlock_FontSize:
        case KnownPropertyIndex::Control_FontSize:
        case KnownPropertyIndex::TextElement_FontSize:
            if (pdo->GetTypeIndex() == KnownTypeIndex::RichTextBlock) { propertyIndex = KnownPropertyIndex::RichTextBlock_FontSize; }
            else if (pdo->GetTypeIndex() == KnownTypeIndex::TextBlock)      { propertyIndex = KnownPropertyIndex::TextBlock_FontSize; }
            else if (pdo->OfTypeByIndex<KnownTypeIndex::Control>())        { propertyIndex = KnownPropertyIndex::Control_FontSize; }
            else if (pdo->OfTypeByIndex<KnownTypeIndex::TextElement>())    { propertyIndex = KnownPropertyIndex::TextElement_FontSize; }
            break;

        case KnownPropertyIndex::RichTextBlock_FontStretch:
        case KnownPropertyIndex::TextBlock_FontStretch:
        case KnownPropertyIndex::Control_FontStretch:
        case KnownPropertyIndex::TextElement_FontStretch:
            if (pdo->GetTypeIndex() == KnownTypeIndex::RichTextBlock) { propertyIndex = KnownPropertyIndex::RichTextBlock_FontStretch; }
            else if (pdo->GetTypeIndex() == KnownTypeIndex::TextBlock)      { propertyIndex = KnownPropertyIndex::TextBlock_FontStretch; }
            else if (pdo->OfTypeByIndex<KnownTypeIndex::Control>())        { propertyIndex = KnownPropertyIndex::Control_FontStretch; }
            else if (pdo->OfTypeByIndex<KnownTypeIndex::TextElement>())    { propertyIndex = KnownPropertyIndex::TextElement_FontStretch; }
            break;

        case KnownPropertyIndex::RichTextBlock_FontStyle:
        case KnownPropertyIndex::TextBlock_FontStyle:
        case KnownPropertyIndex::Control_FontStyle:
        case KnownPropertyIndex::TextElement_FontStyle:
            if (pdo->GetTypeIndex() == KnownTypeIndex::RichTextBlock) { propertyIndex = KnownPropertyIndex::RichTextBlock_FontStyle; }
            else if (pdo->GetTypeIndex() == KnownTypeIndex::TextBlock)      { propertyIndex = KnownPropertyIndex::TextBlock_FontStyle; }
            else if (pdo->OfTypeByIndex<KnownTypeIndex::Control>())        { propertyIndex = KnownPropertyIndex::Control_FontStyle; }
            else if (pdo->OfTypeByIndex<KnownTypeIndex::TextElement>())    { propertyIndex = KnownPropertyIndex::TextElement_FontStyle; }
            break;

        case KnownPropertyIndex::RichTextBlock_FontWeight:
        case KnownPropertyIndex::TextBlock_FontWeight:
        case KnownPropertyIndex::Control_FontWeight:
        case KnownPropertyIndex::TextElement_FontWeight:
            if (pdo->GetTypeIndex() == KnownTypeIndex::RichTextBlock) { propertyIndex = KnownPropertyIndex::RichTextBlock_FontWeight; }
            else if (pdo->GetTypeIndex() == KnownTypeIndex::TextBlock)      { propertyIndex = KnownPropertyIndex::TextBlock_FontWeight; }
            else if (pdo->OfTypeByIndex<KnownTypeIndex::Control>())        { propertyIndex = KnownPropertyIndex::Control_FontWeight; }
            else if (pdo->OfTypeByIndex<KnownTypeIndex::TextElement>())    { propertyIndex = KnownPropertyIndex::TextElement_FontWeight; }
            break;

        case KnownPropertyIndex::RichTextBlock_Foreground:
        case KnownPropertyIndex::TextBlock_Foreground:
        case KnownPropertyIndex::Control_Foreground:
        case KnownPropertyIndex::TextElement_Foreground:
        case KnownPropertyIndex::IconElement_Foreground:
            if (pdo->GetTypeIndex() == KnownTypeIndex::RichTextBlock) { propertyIndex = KnownPropertyIndex::RichTextBlock_Foreground; }
            else if (pdo->GetTypeIndex() == KnownTypeIndex::TextBlock)      { propertyIndex = KnownPropertyIndex::TextBlock_Foreground; }
            else if (pdo->OfTypeByIndex<KnownTypeIndex::Control>())        { propertyIndex = KnownPropertyIndex::Control_Foreground; }
            else if (pdo->OfTypeByIndex<KnownTypeIndex::TextElement>())    { propertyIndex = KnownPropertyIndex::TextElement_Foreground; }
            else if (pdo->OfTypeByIndex<KnownTypeIndex::IconElement>())    { propertyIndex = KnownPropertyIndex::IconElement_Foreground; }
            break;

        // To make AppBarButtons work correctly in Windows Blue, IconElement.Foreground must be able to
        // inherit directly from ContentPresenter.Foreground. This is captured separately from the other
        // Foreground-property cases above, in order to avoid introducing regressions into Win8 apps:
        // Since there was no general linkage between ContentPresenter.Foreground and other controls'
        // Foreground properties previously, we don't want to introduce one now.
        case KnownPropertyIndex::ContentPresenter_Foreground:
            if (pdo->OfTypeByIndex<KnownTypeIndex::IconElement>())    { propertyIndex = KnownPropertyIndex::IconElement_Foreground; }
            break;

        case KnownPropertyIndex::RichTextBlock_TextAlignment:
        case KnownPropertyIndex::TextBlock_TextAlignment:
        case KnownPropertyIndex::RichEditBox_TextAlignment:
        case KnownPropertyIndex::TextBox_TextAlignment:
            if (pdo->GetTypeIndex() == KnownTypeIndex::RichTextBlock) { propertyIndex = KnownPropertyIndex::RichTextBlock_TextAlignment; }
            else if (pdo->GetTypeIndex() == KnownTypeIndex::TextBlock)      { propertyIndex = KnownPropertyIndex::TextBlock_TextAlignment; }
            else if (pdo->GetTypeIndex() == KnownTypeIndex::RichEditBox)   { propertyIndex = KnownPropertyIndex::RichEditBox_TextAlignment; }
            else if (pdo->GetTypeIndex() == KnownTypeIndex::TextBox)        { propertyIndex = KnownPropertyIndex::TextBox_TextAlignment; }
            break;

        case KnownPropertyIndex::RichTextBlock_LineHeight:
        case KnownPropertyIndex::TextBlock_LineHeight:
            if (pdo->GetTypeIndex() == KnownTypeIndex::RichTextBlock) { propertyIndex = KnownPropertyIndex::RichTextBlock_LineHeight; }
            else if (pdo->GetTypeIndex() == KnownTypeIndex::TextBlock)      { propertyIndex = KnownPropertyIndex::TextBlock_LineHeight; }
            break;

        case KnownPropertyIndex::RichTextBlock_LineStackingStrategy:
        case KnownPropertyIndex::TextBlock_LineStackingStrategy:
            if (pdo->GetTypeIndex() == KnownTypeIndex::RichTextBlock) { propertyIndex = KnownPropertyIndex::RichTextBlock_LineStackingStrategy; }
            else if (pdo->GetTypeIndex() == KnownTypeIndex::TextBlock)      { propertyIndex = KnownPropertyIndex::TextBlock_LineStackingStrategy; }
            break;

        case KnownPropertyIndex::FrameworkElement_Language:
        case KnownPropertyIndex::TextElement_Language:
            if (pdo->OfTypeByIndex<KnownTypeIndex::FrameworkElement>()) { propertyIndex = KnownPropertyIndex::FrameworkElement_Language; }
            else if (pdo->OfTypeByIndex<KnownTypeIndex::TextElement>())       { propertyIndex = KnownPropertyIndex::TextElement_Language; }
            break;

        case KnownPropertyIndex::TextElement_TextDecorations:
        case KnownPropertyIndex::TextBlock_TextDecorations:
        case KnownPropertyIndex::RichTextBlock_TextDecorations:
            if (pdo->GetTypeIndex() == KnownTypeIndex::TextBlock) { propertyIndex = KnownPropertyIndex::TextBlock_TextDecorations; }
            else if (pdo->GetTypeIndex() == KnownTypeIndex::RichTextBlock) { propertyIndex = KnownPropertyIndex::RichTextBlock_TextDecorations; }
            else if (pdo->OfTypeByIndex<KnownTypeIndex::TextElement>())    { propertyIndex = KnownPropertyIndex::TextElement_TextDecorations; }
            break;

        case KnownPropertyIndex::FrameworkElement_FlowDirection:
        case KnownPropertyIndex::Run_FlowDirection:
            if (pdo->GetTypeIndex() == KnownTypeIndex::Run)                { propertyIndex = KnownPropertyIndex::Run_FlowDirection; }
            else if (pdo->OfTypeByIndex<KnownTypeIndex::FrameworkElement>()) { propertyIndex = KnownPropertyIndex::FrameworkElement_FlowDirection; }
            break;

        case KnownPropertyIndex::FrameworkElement_DataContext:
            if (pdo->OfTypeByIndex<KnownTypeIndex::FrameworkElement>())   { propertyIndex = KnownPropertyIndex::FrameworkElement_DataContext; }
            break;

        case KnownPropertyIndex::UIElement_AllowDrop:
            if (pdo->OfTypeByIndex<KnownTypeIndex::UIElement>())           { propertyIndex = KnownPropertyIndex::UIElement_AllowDrop; }
            break;

        case KnownPropertyIndex::FrameworkElement_AllowFocusOnInteraction:
        case KnownPropertyIndex::TextElement_AllowFocusOnInteraction:
        case KnownPropertyIndex::FlyoutBase_AllowFocusOnInteraction:
            if (pdo->OfTypeByIndex<KnownTypeIndex::FrameworkElement>()) { propertyIndex = KnownPropertyIndex::FrameworkElement_AllowFocusOnInteraction; }
            else if (pdo->OfTypeByIndex<KnownTypeIndex::TextElement>()) { propertyIndex = KnownPropertyIndex::TextElement_AllowFocusOnInteraction; }
            else if (pdo->OfTypeByIndex<KnownTypeIndex::FlyoutBase>()) { propertyIndex = KnownPropertyIndex::FlyoutBase_AllowFocusOnInteraction; }
            break;

        case KnownPropertyIndex::FrameworkElement_AllowFocusWhenDisabled:
        case KnownPropertyIndex::FlyoutBase_AllowFocusWhenDisabled:
            if (pdo->OfTypeByIndex<KnownTypeIndex::FrameworkElement>()) { propertyIndex = KnownPropertyIndex::FrameworkElement_AllowFocusWhenDisabled; }
            else if (pdo->OfTypeByIndex<KnownTypeIndex::FlyoutBase>()) { propertyIndex = KnownPropertyIndex::FlyoutBase_AllowFocusWhenDisabled; }
            break;

        case KnownPropertyIndex::UIElement_HighContrastAdjustment:
            if (pdo->OfTypeByIndex<KnownTypeIndex::UIElement>()) { propertyIndex = KnownPropertyIndex::UIElement_HighContrastAdjustment; }
            break;
    }


    if (propertyIndex == KnownPropertyIndex::UnknownType_UnknownProperty)
    {
        // This element does not support this property
        return NULL;
    }
    else if (propertyIndex == pdp->GetIndex())
    {
        // The dp passed is the correct one for this element
        return pdp;
    }
    else
    {
        return pdo->GetPropertyByIndexInline(propertyIndex);
    }
}


#if DBG

// Perf collection
XUINT32 g_FEsWithNoTextFormatting               = 0;
XUINT32 g_FEsWithTextFormatting                 = 0;
XUINT32 g_TEsWithNoTextFormatting               = 0;
XUINT32 g_TEsWithTextFormatting                 = 0;
XUINT32 g_FEsWithNoInheritedProperties          = 0;
XUINT32 g_FEsWithLocalInheritedProperties       = 0;
XUINT32 g_FEsWithParentInheritedProperties      = 0;
XUINT32 g_TEsWithNoInheritedProperties          = 0;
XUINT32 g_TEsWithLocalInheritedProperties       = 0;
XUINT32 g_TEsWithParentInheritedProperties      = 0;
XUINT32 g_OtherDOsWithNoInheritedProperties     = 0;
XUINT32 g_OtherDOsWithLocalInheritedProperties  = 0;
XUINT32 g_OtherDOsWithParentInheritedProperties = 0;


void InheritedProperties::RecordTextPropertyUsage(_In_ CDependencyObject *pDo)
{
    TextFormatting **ppTextFormatting = pDo->GetTextFormattingMember();

    if (ppTextFormatting == NULL)
    {
        // This is neither a TE nor an FE
        if (pDo->m_pInheritedProperties == NULL)
        {
            g_OtherDOsWithNoInheritedProperties++;
        }
        else if (pDo->m_pInheritedProperties->m_pWriter == pDo)
        {
            g_OtherDOsWithLocalInheritedProperties++;
        }
        else
        {
            g_OtherDOsWithParentInheritedProperties++;
        }
    }
    else
    {
        if (pDo->OfTypeByIndex<KnownTypeIndex::FrameworkElement>())
        {
            if (*ppTextFormatting == NULL)
            {
                g_FEsWithNoTextFormatting++;
            }
            else
            {
                g_FEsWithTextFormatting++;
            }

            if (pDo->m_pInheritedProperties == NULL)
            {
                g_FEsWithNoInheritedProperties++;
                g_OtherDOsWithNoInheritedProperties--;
            }
            else if (pDo->m_pInheritedProperties->m_pWriter == pDo)
            {
                g_FEsWithLocalInheritedProperties++;
                g_OtherDOsWithLocalInheritedProperties--;
            }
            else
            {
                g_FEsWithParentInheritedProperties++;
                g_OtherDOsWithParentInheritedProperties--;
            }
        }
        else
        {
            ASSERT(pDo->OfTypeByIndex<KnownTypeIndex::TextElement>());
            if (*ppTextFormatting == NULL)
            {
                g_TEsWithNoTextFormatting++;
            }
            else
            {
                g_TEsWithTextFormatting++;
            }

            if (pDo->m_pInheritedProperties == NULL)
            {
                g_TEsWithNoInheritedProperties++;
                g_OtherDOsWithNoInheritedProperties--;
            }
            else if (pDo->m_pInheritedProperties->m_pWriter == pDo)
            {
                g_TEsWithLocalInheritedProperties++;
                g_OtherDOsWithLocalInheritedProperties--;
            }
            else
            {
                g_TEsWithParentInheritedProperties++;
                g_OtherDOsWithParentInheritedProperties--;
            }
        }
    }
}


XUINT32 g_NoopEnsureTextFormattings     = 0;
XUINT32 g_UpdateEnsureTextFormattings   = 0;
XUINT32 g_CreationEnsureTextFormattings = 0;
XUINT32 g_PulledEnsureTextFormattings   = 0;

void InheritedProperties::RecordNoopEnsureTextFormatting()     {g_NoopEnsureTextFormattings++;}
void InheritedProperties::RecordUpdateEnsureTextFormatting()   {g_UpdateEnsureTextFormattings++;}
void InheritedProperties::RecordCreationEnsureTextFormatting() {g_CreationEnsureTextFormattings++;}
void InheritedProperties::RecordPulledEnsureTextFormatting()   {g_PulledEnsureTextFormattings++;}

void InheritedProperties::TraceDependencyObjectTextPropertyUsage()
{
    TRACE(TraceAlways, L"");
    TRACE(TraceAlways, L"Inherited property storage group statistics:");
    TRACE(TraceAlways, L"                                    Text elements  Framework elements  Other dependency objects");
    TRACE(TraceAlways, L"                                    -------------  ------------------  ------------------------");
    TRACE(TraceAlways, L"   No      TextFormatting                   %5d               %5d",                         g_TEsWithNoTextFormatting,  g_FEsWithNoTextFormatting);
    TRACE(TraceAlways, L"   Has     TextFormatting                   %5d               %5d",                         g_TEsWithTextFormatting,    g_FEsWithTextFormatting);
    TRACE(TraceAlways, L"   No      InheritedProperties              %5d               %5d                     %5d", g_TEsWithNoInheritedProperties,      g_FEsWithNoInheritedProperties,      g_OtherDOsWithNoInheritedProperties);
    TRACE(TraceAlways, L"   Parent  InheritedProperties              %5d               %5d                     %5d", g_TEsWithParentInheritedProperties,  g_FEsWithParentInheritedProperties,  g_OtherDOsWithParentInheritedProperties);
    TRACE(TraceAlways, L"   Local   InheritedProperties              %5d               %5d                     %5d", g_TEsWithLocalInheritedProperties,   g_FEsWithLocalInheritedProperties,   g_OtherDOsWithLocalInheritedProperties);
    TRACE(TraceAlways, L"");
    TRACE(TraceAlways, L"EnsureTextFormatting storage group update statistics:");
    TRACE(TraceAlways, L"   Noop     %5d,  Update %5d", g_NoopEnsureTextFormattings,     g_UpdateEnsureTextFormattings);
    TRACE(TraceAlways, L"   Creation %5d,  Pulled %5d", g_CreationEnsureTextFormattings, g_PulledEnsureTextFormattings);
    TRACE(TraceAlways, L"");
}

#endif

