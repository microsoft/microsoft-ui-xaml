// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Abstract base class representing one run of content. TextRun may be
//      of different types as detailed by TextRunType enum.

#pragma once

#include "TextRunType.h"
#include "TextObject.h"

namespace RichTextServices
{
    class  TextRunProperties;

    //---------------------------------------------------------------------------
    //
    //  TextRun
    //
    //  TextRun represents one run of content.
    //
    //---------------------------------------------------------------------------
    class TextRun : public TextObject
    {
    public:

        // Gets the length of the run in backing store characters.
        XUINT32 GetLength() const;

        // Gets type of data stored in this run.
        TextRunType::Enum GetType() const;

        // Gets index of the first character of the run.
        XUINT32 GetCharacterIndex() const;

    protected:

        // Number of characters in the run.
        XUINT32 m_length;

        // Initializes a new instance of the TextRun class.
        TextRun(
            _In_ XUINT32 length,
                // Number of characters in the run.
            _In_ XUINT32 characterIndex,
                // Index of the first character of the run.
            _In_ TextRunType::Enum type
                // Type of data stored in this run - hidden characters, plain text, etc.
        );

        // Release resources associated with the TextRun.
        ~TextRun() override;

    private:

        // Type of data stored in this run - hidden characters, plain text, etc.
        TextRunType::Enum m_type;

        // Index of the first character of the run (relative to the current context.
        XUINT32 m_characterIndex;
    };

    //---------------------------------------------------------------------------
    //
    //  Member:
    //      TextRun::GetLength
    //
    //  Synopsis:
    //      Gets number of characters in the run.
    //
    //---------------------------------------------------------------------------
    inline XUINT32 TextRun::GetLength() const
    {
        return m_length;
    }

    //---------------------------------------------------------------------------
    //
    //  Member:
    //      TextRun::GetCharacterIndex
    //
    //  Synopsis:
    //      Gets index of the first character of the run.
    //
    //---------------------------------------------------------------------------
    inline XUINT32 TextRun::GetCharacterIndex() const
    {
        return m_characterIndex;
    }

    //---------------------------------------------------------------------------
    //
    //  Member:
    //      TextRun::GetType
    //
    //  Synopsis:
    //      Gets type of data stored in this run.
    //
    //---------------------------------------------------------------------------
    inline TextRunType::Enum TextRun::GetType() const
    {
        return m_type;
    }
}
