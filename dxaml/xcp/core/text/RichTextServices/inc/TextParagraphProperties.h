// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "FlowDirection.h"
#include "TextObject.h"

namespace RichTextServices
{
    class TextRunProperties;

    //---------------------------------------------------------------------------
    //
    //  TextParagraphProperties
    //
    //  Provides a set of properties, such as alignment, or indentation,
    //  that can be applied to a paragraph.
    //
    //---------------------------------------------------------------------------
    class TextParagraphProperties : public TextObject
    {
    public:

        // Constructor.
        TextParagraphProperties(
            _In_ FlowDirection::Enum   flowDirection,
            _In_ TextRunProperties    *pDefaultTextRunProperties,
            _In_ XFLOAT                fistLineIndent,
            _In_ DirectUI::TextWrapping          textWrapping,
            _In_ DirectUI::TextLineBounds        textLineBounds,
            DirectUI::TextAlignment     textAlignment
            );

        // Gets flow direction.
        FlowDirection::Enum GetFlowDirection() const;

        // Gets default incremental tab value.
        XFLOAT GetDefaultIncrementalTab() const;

        // Gets paragraph's default run properties.
        const TextRunProperties *GetDefaultTextRunProperties() const;

        // Properties controlling line indent.
        XFLOAT GetFirstLineIndent() const;

        // Gets paragraph's TextWrapping setting
        DirectUI::TextWrapping GetTextWrapping() const;

        // Gets value for specified flags.
        bool GetFlags(_In_ XUINT32 flags) const;

        // Gets TextLineBoundsd property value
        DirectUI::TextLineBounds GetTextLineBounds() const;

        DirectUI::TextAlignment GetTextAlignment() const { return m_textAlignment; }

        // Sets flags to specified value.
        void SetFlags(
            _In_ XUINT32 flags,
            _In_ bool value
            );

        struct Flags
        {
            enum Enum
            {
                Justify                              = 0x00000001,
                TrimSideBearings                     = 0x00000002, // Flag for enabling OpticalMarginAlignment
                DetermineTextReadingOrderFromContent = 0x00000004,
                DetermineAlignmentFromContent        = 0x00000008,
            };
        };

    protected:

        // Destructor.
        ~TextParagraphProperties() override;

    private:
        TextRunProperties *m_pDefaultTextRunProperties;
            // Paragraph's default run properties.

        FlowDirection::Enum m_flowDirection;
            // Paragraph's flow direction.

        XFLOAT m_firstLineIndent;
            // Properties controlling line indent.

        DirectUI::TextWrapping m_textWrapping;
            // Paragraph's textwrapping setting

        XUINT32 m_flags;
            // Storage for flags.

        DirectUI::TextLineBounds m_textLineBounds;
            // Paragraph's TextLineBounds setting

        DirectUI::TextAlignment m_textAlignment;
    };
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextParagraphProperties::GetFlowDirection()
//
//  Returns:
//      Paragraph's flow direction .
//
//---------------------------------------------------------------------------
inline RichTextServices::FlowDirection::Enum RichTextServices::TextParagraphProperties::GetFlowDirection() const
{
    return m_flowDirection;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextParagraphProperties::GetDefaultIncrementalTab()
//
//  Returns:
//      Default incremental tab advance.
//
//  Notes:
//      Default incremental tab is set to the font size for default text run
//      properties.
//
//---------------------------------------------------------------------------
inline XFLOAT RichTextServices::TextParagraphProperties::GetDefaultIncrementalTab() const
{
    return 4 * m_pDefaultTextRunProperties->GetFontSize();
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextParagraphProperties::GetDefaultTextRunProperties
//
//  Returns:
//      Gets paragraph's default run properties.
//
//---------------------------------------------------------------------------
inline const RichTextServices::TextRunProperties * RichTextServices::TextParagraphProperties::GetDefaultTextRunProperties() const
{
    return m_pDefaultTextRunProperties;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextParagraphProperties::GetFirstLineIndent
//
//  Returns:
//      Gets first line indent.
//
//---------------------------------------------------------------------------
inline XFLOAT RichTextServices::TextParagraphProperties::GetFirstLineIndent() const
{
    return m_firstLineIndent;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextParagraphProperties::GetFlags()
//
//  Returns:
//      Gets value for specified flags.
//
//---------------------------------------------------------------------------
inline bool RichTextServices::TextParagraphProperties::GetFlags(_In_ XUINT32 flags) const
{
    return ((m_flags & flags) == flags) ? TRUE : FALSE;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextParagraphProperties::SetFlags()
//
//  Returns:
//      Sets flags to specified value.
//
//---------------------------------------------------------------------------
inline void RichTextServices::TextParagraphProperties::SetFlags(
    _In_ XUINT32 flags,
    _In_ bool value
    )
{
    if (value) { m_flags |= flags; } else { m_flags &= ~flags; }
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextParagraphProperties::GetTextWrapping()
//
//  Returns:
//      returns TextWrapping setting
//
//---------------------------------------------------------------------------
inline DirectUI::TextWrapping RichTextServices::TextParagraphProperties::GetTextWrapping() const
{
    return m_textWrapping;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextParagraphProperties::GetTextLineBounds()
//
//  Returns:
//      returns TextLineBounds setting
//
//---------------------------------------------------------------------------
inline DirectUI::TextLineBounds RichTextServices::TextParagraphProperties::GetTextLineBounds() const
{
    return m_textLineBounds;
}
