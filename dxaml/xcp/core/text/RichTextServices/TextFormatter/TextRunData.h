// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      TextRunData provides storage for TextRun and related data used during 
//      text analysis.

#pragma once

class FontFace;

namespace RichTextServices
{
    class TextRun;

    namespace Internal
    {
        //---------------------------------------------------------------------------
        //
        //  TextRunData
        //
        //  Provides storage for TextRun and related data used during text analysis.
        //
        //---------------------------------------------------------------------------
        class TextRunData
        {
        public:
            // Initializes a new instance of the TextRunData class.
            TextRunData(
                _In_ TextRun *pTextRun,
                _In_opt_ TextRunData *pPreviousRunData,
                _In_ XUINT8 bidiLevel
                );

            // Release resources associated with the TextRunData.
            ~TextRunData();

            // Gets the previous TextRunData in the collection.
            TextRunData * GetPrevious() const;

            // Gets the next TextRunData in the collection.
            TextRunData * GetNext() const;

            // Gets the TextRun associated with this instance.
            TextRun * GetTextRun() const;

            // Gets number of characters in the run.
            XUINT32 GetLength() const;

            // Gets the bidirectional nesting level for this run.
            XUINT8 GetBidiLevel() const;

            // Gets the Font face associated with this run.
            IFssFontFace* GetFontFace() const;

            // Gets the Font scale associated with this run
            XFLOAT GetFontScale() const;

            // Detaches externally managed objects from the instance.
            void Detach();

            // Splits the run into two adjacent runs.
            Result::Enum Split(
                _In_ XUINT32 offset, 
                _Outptr_ TextRunData **ppRunData
                );

            // Sets text analysis properties.
            void SetAnalysisProperties(
                _In_ const FssScriptAnalysis &scriptAnalysis,
                _In_ XUINT8 bidiLevel,
                _In_opt_ IDWriteNumberSubstitution* pNumberSubstitution
                );

            HRESULT SetLineBreakpoints(
                _In_reads_(breakpointsCount) PALText::LineBreakpoint const* lineBreakpoints,
                XUINT32 breakpointsCount
                );

            // Sets the font face.
            void SetFontFace(
                _In_ IFssFontFace *pFontFace
                );

            // Sets the font scale
            void SetFontScale(
                _In_ XFLOAT fontScale
                );

            // Sets whether this run requires shaping or not.
            void SetGlyphBased(
                _In_ bool glyphBased
                );

            // Space to display before the run.
            XINT32 GetInitialSpacing() const;
            void   SetInitialSpacing(XINT32 initialSpacing);

            // Whether to apply CharacterSpacing property to last character.
            bool GetSpaceLastCharacter() const;
            void  SetSpaceLastCharacter(bool spaceLastCharacter);


        private:
            // Points to the next TextRunData in the collection.
            TextRunData *m_pNext;

            // Points to the previous TextRunData in the collection.
            TextRunData *m_pPrevious;

            // Contains character and formatting data for one run of text.
            TextRun *m_pTextRun;

            // Font face associated with this run.
            IFssFontFace *m_pFontFace;

            // Font scale associated with this run
            XFLOAT m_fontScale;

            // Script related data for this run.
            FssScriptAnalysis m_scriptAnalysis;

            // Bidirectional nesting level for this run.
            XUINT8 m_bidiLevel;

            // Determines whether this is this glyph based run.
            bool m_glyphBased;

            // Space to display after every character and object.
            bool  m_spaceLastCharacter;

            // Line breakpoint information for this run.            
            PALText::LineBreakpoint *m_lineBreakpoints;

            // Space to display before the run.
            XINT32 m_initialSpacing;

            IDWriteNumberSubstitution* m_pNumberSubstitution;

            // Initializes a new instance of the TextRunData class.
            TextRunData(
                _In_opt_ TextRunData *pPreviousRunData,
                _In_ TextRun *pTextRun,
                _In_ IFssFontFace *pFontFace,
                _In_ XFLOAT fontScale,
                _In_ const FssScriptAnalysis &scriptAnalysis,
                _In_ XUINT8 bidiLevel,
                _In_ bool glyphBased
                );

            friend class LsRun;
            friend class TextSegment;
        };

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      TextRunData::GetPrevious
        //
        //  Returns:
        //      Gets the previous TextRunData in the collection.
        //
        //---------------------------------------------------------------------------
        inline TextRunData * TextRunData::GetPrevious() const
        {
            return m_pPrevious;
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      TextRunData::GetNext
        //
        //  Returns:
        //      Gets the next TextRunData in the collection.
        //
        //---------------------------------------------------------------------------
        inline TextRunData * TextRunData::GetNext() const
        {
            return m_pNext;
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      TextRunData::GetTextRun
        //
        //  Returns:
        //      Gets the TextRun associated with this instance.
        //
        //---------------------------------------------------------------------------
        inline TextRun * TextRunData::GetTextRun() const
        {
            return m_pTextRun;
        }

        //-------------------------------------------------------------------
        //
        //  Member:
        //      TextRunData::GetLength
        //
        //  Synopsis:
        //      Gets number of characters in the run.
        //
        //-------------------------------------------------------------------
        inline XUINT32 TextRunData::GetLength() const
        {
            return m_pTextRun->GetLength();
        }

        //-------------------------------------------------------------------
        //
        //  Member:
        //      TextRunData::GetBidiLevel
        //
        //  Synopsis:
        //      Gets the bidirectional nesting level for this run.
        //
        //-------------------------------------------------------------------
        inline XUINT8 TextRunData::GetBidiLevel() const
        {
            return m_bidiLevel;
        }

        //-------------------------------------------------------------------
        //
        //  Member:
        //      TextRunData::GetFontFace
        //
        //  Synopsis:
        //      Gets the Font Face associated with this run.
        //
        //-------------------------------------------------------------------
        inline IFssFontFace* TextRunData::GetFontFace() const
        {
            return m_pFontFace;
        }

        //-------------------------------------------------------------------
        //
        //  Member:
        //      TextRunData::GetFontScale
        //
        //  Synopsis:
        //      Gets the Font scale associated with this run.
        //
        //-------------------------------------------------------------------
        inline XFLOAT TextRunData::GetFontScale() const
        {
            return m_fontScale;
        }    

        //-------------------------------------------------------------------
        //
        //  Member:
        //      TextRunData::GetInitialSpacing
        // 
        //  Synopsis:
        //      Returns space to be displayed before the run.
        //
        //-------------------------------------------------------------------
        inline XINT32 TextRunData::GetInitialSpacing() const
        {
            return m_initialSpacing;
        }

        //-------------------------------------------------------------------
        //
        //  Member:
        //      TextRunData::SetInitialSpacing
        //
        //  Synopsis:
        //      Sets space to be displayed before the run.
        //
        //-------------------------------------------------------------------
        inline void TextRunData::SetInitialSpacing(XINT32 initialSpacing)
        {
            m_initialSpacing = initialSpacing;
            if (initialSpacing != 0)
            {
                m_glyphBased = TRUE; // Only glyph based runs can display initial spacing
            }
        }

        //-------------------------------------------------------------------
        //
        //  Member:
        //      TextRunData::GetSpaceLastCharacter
        //
        //  Synopsis:
        //      Returns space to be displayed after every character and object.
        //
        //-------------------------------------------------------------------
        inline bool TextRunData::GetSpaceLastCharacter() const
        {
            return m_spaceLastCharacter;
        }

        //-------------------------------------------------------------------
        //
        //  Member:
        //      TextRunData::SetSpaceLastCharacter
        //
        //  Synopsis:
        //      Sets space to be displayed after every character and object.
        //
        //-------------------------------------------------------------------
        inline void TextRunData::SetSpaceLastCharacter(bool spaceLastCharacter)
        {
            m_spaceLastCharacter = spaceLastCharacter;
        }
    }
}
