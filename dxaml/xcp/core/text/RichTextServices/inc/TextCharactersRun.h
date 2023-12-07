// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      TextCharactersRun class represents Unicode text content with associated
//      formatting properties.

#pragma once

#include "TextRun.h"

namespace RichTextServices
{
    class  TextRunProperties;

    //---------------------------------------------------------------------------
    //
    //  TextCharactersRun
    //
    //  TextCharactersRun class represents Unicode text content with associated
    //  formatting properties.
    //
    //---------------------------------------------------------------------------
    class TextCharactersRun final : public TextRun
    {
    public:

        // Initializes a new instance of the TextRun class.
        TextCharactersRun(
            _In_reads_opt_(length) const WCHAR *pCharacters,
                // Character buffer containing the run's character data.
            _In_ XUINT32 length,
                // Number of characters in the run.
            _In_ XUINT32 characterIndex,
                // Index of the first character of the run.
            _In_ TextRunProperties *pProperties
                // Formatting properties shared by all characters in the run
        );

        // Gets character buffer containing the run's character data.
        const WCHAR * GetCharacters() const;

        // Gets formatting properties shared by all characters in the run.
        TextRunProperties *GetProperties() const;

        bool IsTab() const;

        // Splits the run into two adjacent runs.
        virtual Result::Enum Split(
            _In_ XUINT32 offset,
            _Outptr_ TextCharactersRun **ppTextCharactersRun
            );

    private:

        // Character buffer containing the run's character data.
        const WCHAR *m_pCharacters;

        // Resolved formatting properties shared by all characters in the run.
        TextRunProperties* m_pProperties;

        // Release resources associated with the TextCharactersRun.
        ~TextCharactersRun() override;
    };

    //---------------------------------------------------------------------------
    //
    //  Member:
    //      TextCharactersRun::GetCharacters
    //
    //  Synopsis:
    //      Gets character buffer containing the run's character data.
    //
    //---------------------------------------------------------------------------
    inline const WCHAR *TextCharactersRun::GetCharacters() const
    {
        return m_pCharacters;
    }

    //---------------------------------------------------------------------------
    //
    //  Member:
    //      TextCharactersRun::GetProperties
    //
    //  Synopsis:
    //      Gets formatting properties shared by all characters in the run.
    //
    //---------------------------------------------------------------------------
    inline TextRunProperties * TextCharactersRun::GetProperties() const
    {
        return m_pProperties;
    }

   //---------------------------------------------------------------------------
    //
    //  Member:
    //      TextCharactersRun::GetProperties
    //
    //  Synopsis:
    //      Gets formatting properties shared by all characters in the run.
    //
    //---------------------------------------------------------------------------
    inline bool TextCharactersRun::IsTab() const
    {
        return (m_length == 1 &&
                m_pCharacters[0] == UNICODE_CHARACTER_TABULATION);
    }

}
