// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      TextRunProperties contains formatting properties for one run of text.

#pragma once

#include "TextObject.h"
#include <weakref_ptr.h>
class InheritedProperties;

namespace RichTextServices
{
    // CharacterSpacing values are in 1000ths of the em size.
    static const XFLOAT CharacterSpacingScale = 1000.0f;


    //---------------------------------------------------------------------------
    //
    //  TextRunProperties
    //
    //  TextRunProperties contains formatting properties for one run of text.
    //
    //---------------------------------------------------------------------------
    class TextRunProperties final : public TextObject
    {
    public:

        // Initializes a new instance of the TextRunProperties class.
        TextRunProperties(
            _In_ FontTypeface *pFontTypeface,
            _In_ XFLOAT fontSize,
            _In_ bool hasUnderline,
            _In_ bool hasStrikethrough,
            _In_ XINT32 characterSpacing,
            _In_ xref::weakref_ptr<CDependencyObject> pForegroundBrushSource,
            _In_ CultureInfo strCultureInfo,
            _In_ CultureInfo strCultureListInfo,

            _In_opt_ const InheritedProperties *pInheritedProperties
        );

        // Gets font typeface.
        FontTypeface *GetFontTypeface() const;

        // Gets font rendering size.
        XFLOAT GetFontSize() const;

        // Gets the source of foreground brush.
        const xref::weakref_ptr<CDependencyObject>& GetForegroundBrushSource() const;

        bool HasUnderline() const;
        // Gets value indicating whether strikethrough is present.
        bool HasStrikethrough() const;


        // Extra space displayed after each character and object.
        XINT32 GetCharacterSpacing() const;

        const InheritedProperties *GetInheritedProperties() const;

        // Gets culture info for this run.
        CultureInfo GetCultureInfo() const;

        // Gets culture list info for this run.
        CultureInfo GetCultureListInfo() const;

        // Compares with another TextRunProperties object and checks for equality for shaping purposes
        bool EqualsForShaping(_In_ const TextRunProperties *pProperties) const;

    protected:

        // Prevent deletion through destructor.
        ~TextRunProperties() override;

    private:

        // Font typeface representing font family, weight, style, stretch and language.
        FontTypeface *m_pFontTypeface;

        // Font rendering em size for run's character data.
        XFLOAT m_fontSize;

        // The source of foreground brush.
        xref::weakref_ptr<CDependencyObject> m_pForegroundBrushSource;

        bool m_hasUnderline;
        // Value indicating whether strikethrough is present.
        bool m_hasStrikethrough;


        // Extra space to display after each character or object.
        XINT32 m_characterSpacing;

        // Silverlight inherited properties including TextOptions and Typography.
        const InheritedProperties *m_pInheritedProperties;

        // Culture info for this run.
        CultureInfo m_strCultureInfo;

        // CultureList info for this run.
        CultureInfo m_strCultureListInfo;

        // Prevent instantiation through default constructor.
        TextRunProperties();
    };

    //---------------------------------------------------------------------------
    //
    //  Member:
    //      TextRunProperties::GetFontTypeface
    //
    //  Returns:
    //      Font typeface representing font family, weight, style, stretch and language.
    //
    //---------------------------------------------------------------------------
    inline FontTypeface* TextRunProperties::GetFontTypeface() const
    {
        return m_pFontTypeface;
    }

    //---------------------------------------------------------------------------
    //
    //  Member:
    //      TextRunProperties::GetFontSize
    //
    //  Returns:
    //      Font rendering em size for run's character data.
    //
    //---------------------------------------------------------------------------
    inline XFLOAT TextRunProperties::GetFontSize() const
    {
        return m_fontSize;
    }

    //---------------------------------------------------------------------------
    //
    //  Member:
    //      TextRunProperties::GetPropertiesSource
    //
    //  Returns:
    //      The source of properties.
    //
    //---------------------------------------------------------------------------
    inline const xref::weakref_ptr<CDependencyObject>& TextRunProperties::GetForegroundBrushSource() const
    {
        return m_pForegroundBrushSource;
    }

    //---------------------------------------------------------------------------
    //
    //  Member:
    //      TextRunProperties::HasUnderline
    //
    //  Returns:
    //      Value indicating whether underline is present.
    //
    inline bool TextRunProperties::HasUnderline() const
    {
        return m_hasUnderline;
    }
    inline bool TextRunProperties::HasStrikethrough() const
    {
        return m_hasStrikethrough;
    }


    //---------------------------------------------------------------------------
    //
    //  Member:
    //      TextRunProperties::GetCharacterSpacing
    //
    //  Returns:
    //      Extra space to display after each character or object.
    //
    //---------------------------------------------------------------------------
    inline XINT32 TextRunProperties::GetCharacterSpacing() const
    {
        return m_characterSpacing;
    }

    //---------------------------------------------------------------------------
    //
    //  Member:
    //      TextRunProperties::GetInheritedProperties
    //
    //  Returns:
    //      Silverlight inherited properties includuding TextOptions and Typography.
    //
    //---------------------------------------------------------------------------
    inline const InheritedProperties *TextRunProperties::GetInheritedProperties() const
    {
        return m_pInheritedProperties;
    }

    //---------------------------------------------------------------------------
    //
    //  Member:
    //      TextRunProperties::GetCultureInfo
    //
    //  Returns:
    //      Culture info for this run.
    //
    //---------------------------------------------------------------------------
    inline CultureInfo TextRunProperties::GetCultureInfo() const
    {
        return m_strCultureInfo;
    }

    //---------------------------------------------------------------------------
    //
    //  Member:
    //      TextRunProperties::GetCultureListInfo
    //
    //  Returns:
    //      Culture list info for this run.
    //
    //---------------------------------------------------------------------------
    inline CultureInfo TextRunProperties::GetCultureListInfo() const
    {
        return m_strCultureListInfo;
    }
}

