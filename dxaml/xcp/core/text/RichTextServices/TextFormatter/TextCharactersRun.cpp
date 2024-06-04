// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      TextCharactersRun class implementation.

#include "precomp.h"

using namespace RichTextServices;

//---------------------------------------------------------------------------
//
//  Member:
//      TextCharactersRun::TextCharactersRun
//
//  Synopsis:
//      Initializes a new instance of the TextCharactersRun class.
//
//---------------------------------------------------------------------------
TextCharactersRun::TextCharactersRun(
    _In_reads_opt_(length) const WCHAR *pCharacters,
        // Character buffer containing the run's character data.
    _In_ XUINT32 length,
        // Number of characters in the run.
    _In_ XUINT32 characterIndex,
        // Index of the first character of the run.
    _In_ TextRunProperties *pProperties
        // Formatting properties shared by all characters in the run.
    ) :
    TextRun(length, characterIndex, TextRunType::Text),
    m_pCharacters(pCharacters),
    m_pProperties(pProperties)
{
    ASSERT(pProperties->GetInheritedProperties() != NULL);
    AddRefInterface(pProperties);
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextCharactersRun::~TextCharactersRun
//
//  Synopsis:
//      Destructor.
//
//  Notes:
//      Derefs run's text properties pointer. TextRunProperties may be shared
//      between runs.
//
//---------------------------------------------------------------------------
TextCharactersRun::~TextCharactersRun()
{
    ReleaseInterface(m_pProperties);
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextCharactersRun::Split
//
//  Synopsis:
//      Splits the run into two adjacent runs.
//
//---------------------------------------------------------------------------
Result::Enum TextCharactersRun::Split(
    _In_ XUINT32 offset,
    _Outptr_ TextCharactersRun **ppTextCharactersRun
    )
{
    Result::Enum txhr = Result::Success;
    TextCharactersRun *pTextCharactersRun = NULL;

    ASSERT_EXPECT_RTS(offset > 0 && offset < m_length);

    IFC_OOM_RTS(pTextCharactersRun = new TextCharactersRun(
        m_pCharacters + offset,
        m_length - offset,
        this->GetCharacterIndex() + offset,
        m_pProperties));

    m_length = offset;

    *ppTextCharactersRun = pTextCharactersRun;
    pTextCharactersRun = NULL;

Cleanup:
    ReleaseInterface(pTextCharactersRun);
    return txhr;
}
