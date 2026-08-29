// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines LsRun represented by a plsrun value dispatched to LS during FetchRun.

#pragma once

namespace RichTextServices
{
    class TextRun;

    namespace Internal
    {
        class TextRunData;
        class LsSpan;

        //-------------------------------------------------------------------
        //
        //  LsRun
        //
        //  Run represented by a plsrun value dispatched to LS during FetchRun.
        //
        //-------------------------------------------------------------------
        class LsRun
        {
        public:

            // Creates run from a TextRun
            static Result::Enum CreateFromTextRun(
                _In_ XUINT32 characterIndex, 
                _In_ const TextRunData *pRunData, 
                _In_ LsSpan *pMasterLsSpan,
                _Outptr_ LsRun **ppLsRun
                );

            // Creates run indicating start of Reverse Object
            static Result::Enum CreateReverseObjectOpen(
                _In_ XUINT32 characterIndex, 
                _In_ XUINT8 bidiLevel, 
                _Inout_ LsSpan **ppMasterLsSpan,
                _Outptr_ LsRun **ppLsRun
                );

            // Creates run indicating end of Reverse Object
            static Result::Enum CreateReverseObjectClose(
                _In_ XUINT32 characterIndex, 
                _In_ XUINT8 bidiLevel, 
                _Inout_ LsSpan **ppMasterLsSpan,
                _Outptr_ LsRun **ppLsRun
                );

            // Destructor
            ~LsRun();

            LsRun *GetNext() const;

            // Returns original run provided by TextSource.
            const TextRun *GetTextRun() const;

            // Returns glyph typeface associated with this run.
            IFssFontFace *GetFontFace() const;

            // Returns font scale associated with this run.
            XFLOAT GetFontScale() const;

            // Gets index of the first character of the run.
            XUINT32 GetCharacterIndex() const;

            // Gets number of characters in the run.
            XUINT32 GetLength() const;

            // Gets the script which determines the shaping engine to be used for this run.
            const FssScriptAnalysis & GetScriptAnalysis() const;

            IDWriteNumberSubstitution* GetNumberSubstitution() const;

            // Gets the bidirectional nesting level for this run.
            XUINT8 GetBidiLevel() const;
            
            // Gets the line break condition for the character at the specified position relative to the beginning of this run.
            PALText::LineBreakpoint const& GetLineBreakpoint(XUINT32 cp) const;

            // Gets algorithmic style simulations to be applied to the font face.
            FssFontSimulations::Enum GetFontSimulations() const;

            // Determines whether this is this glyph based run.
            bool IsGlyphBased() const;

            // Space to be displayed before this run.
            XINT32 GetInitialSpacing() const;

            // Whether to apply CharacterSpacing after this pLsRun
            bool GetSpaceLastCharacter() const;

            // Whether to apply CharacterSpacing after pLastCharacter.
            bool GetSpaceLastCharacter(const WCHAR *pLastCharacter) const;

            // Initialize data representing the run.
            void InitializeRunData(
                _In_ Ptls6::LSCP cpCurrent,
                _In_ Ptls6::LSSPAN lsSpanMaster,    
                _Out_ Ptls6::LSFETCHRESULT* pfetchres
                );

            bool HasEqualProperties(_In_ const LsRun *pRun) const;

        private:
            // Initializes a new instance of the LsRun class
            LsRun(
                _In_ XUINT32 characterIndex, 
                _In_ XUINT32 length,
                _In_ XUINT8 bidiLevel
                );

            // Initializes a new instance of the LsRun class
            LsRun(
                _In_ XUINT32 characterIndex, 
                _In_ XUINT32 length,
                _In_opt_ TextRun *pTextRun,
                _In_opt_ IFssFontFace *pFontFace,
                _In_ XFLOAT scale,
                _In_ const FssScriptAnalysis &scriptAnalysis,
                _In_ XUINT8 bidiLevel,
                _In_ bool glyphBased,
                _In_ XINT32 initialSpacing,
                _In_ bool spaceLastCharacter,
                _In_opt_ IDWriteNumberSubstitution* pNumberSubstitution
                );

            // Data representing the run.
            Ptls6::lsfetchresult m_lsrun;

            // Original run provided by TextSource.
            TextRun *m_pTextRun;

            // Font face associated with this run.
            IFssFontFace *m_pFontFace;

            // Index of the first character of the run (relative to the current context).
            XUINT32 m_characterIndex;

            // Number of characters in the run.
            XUINT32 m_length;
            
            // Scale of the font face
            XFLOAT m_fontScale;

            // Script which determines the shaping engine to be used for this run.
            FssScriptAnalysis m_scriptAnalysis;

            // Bidirectional nesting level for this run.
            XUINT8 m_bidiLevel;

            // Determines whether this is this glyph based run.
            bool m_glyphBased;

            // Whether to apply CharacterSpacing to the last character in a run
            bool m_spaceLastCharacter;

            // Line breakpoint information for this run.            
            PALText::LineBreakpoint *m_lineBreakpoints;            

            // Pointer to the previous LsRun in the run cache.
            LsRun *m_pPrevious;

            // Pointer to the next LsRun in the run cache.
            LsRun *m_pNext;

            // Space to display before the run in 1000ths of the font size 
            XINT32 m_initialSpacing;

            IDWriteNumberSubstitution *m_pNumberSubstitution;
           
            friend class LsRunCache;
        };

        //-------------------------------------------------------------------
        //
        //  Member:
        //      LsRun::GetNext
        //
        //  Synopsis:
        //      Gets the following run.
        //
        //-------------------------------------------------------------------
        inline LsRun *LsRun::GetNext() const
        {
            return m_pNext;
        }

        //-------------------------------------------------------------------
        //
        //  Member:
        //      LsRun::GetTextRun
        //
        //  Synopsis:
        //      Returns original run provided by TextSource.
        //
        //-------------------------------------------------------------------
        inline const TextRun * LsRun::GetTextRun() const
        {
            return m_pTextRun;
        }

        //-------------------------------------------------------------------
        //
        //  Member:
        //      LsRun::GetFontFace
        //
        //  Synopsis:
        //      Returns font face associated with this run.
        //
        //-------------------------------------------------------------------
        inline IFssFontFace * LsRun::GetFontFace() const
        {
            return m_pFontFace;
        }

        //-------------------------------------------------------------------
        //
        //  Member:
        //      LsRun::GetFontScale
        //
        //  Synopsis:
        //      Returns font scale associated with this run.
        //
        //-------------------------------------------------------------------
        inline XFLOAT LsRun::GetFontScale() const
        {
            return m_fontScale;
        }

        //-------------------------------------------------------------------
        //
        //  Member:
        //      LsRun::GetCharacterIndex
        //
        //  Synopsis:
        //      Gets index of the first character of the run.
        //
        //-------------------------------------------------------------------
        inline XUINT32 LsRun::GetCharacterIndex() const
        {
            return m_characterIndex;
        }

        //-------------------------------------------------------------------
        //
        //  Member:
        //      LsRun::GetLength
        //
        //  Synopsis:
        //      Gets number of characters in the run.
        //
        //-------------------------------------------------------------------
        inline XUINT32 LsRun::GetLength() const
        {
            return m_length;
        }

        //-------------------------------------------------------------------
        //
        //  Member:
        //      LsRun::GetScript
        //
        //  Synopsis:
        //      Gets the script which determines the shaping engine to be used
        //      for this run.
        //
        //-------------------------------------------------------------------
        inline const FssScriptAnalysis & LsRun::GetScriptAnalysis() const
        {
            return m_scriptAnalysis;
        }

        //-------------------------------------------------------------------
        //
        //  Member:
        //      LsRun::GetNumberSubstitution
        //
        //  Synopsis:
        //      Gets the number substitution property for this run.
        //
        //-------------------------------------------------------------------
        inline IDWriteNumberSubstitution* LsRun::GetNumberSubstitution() const
        {
            return m_pNumberSubstitution;
        }


        //-------------------------------------------------------------------
        //
        //  Member:
        //      LsRun::GetBidiLevel
        //
        //  Synopsis:
        //      Gets the bidirectional nesting level for this run.
        //
        //-------------------------------------------------------------------
        inline XUINT8 LsRun::GetBidiLevel() const
        {
            return m_bidiLevel;
        }


        //-------------------------------------------------------------------
        //
        //  Member:
        //      TextRunData::GetLineBreakpoint
        //
        //  Synopsis:
        //      Gets the line break condition for the character at the specified
        //      position relative to the beginning of this run.
        //
        //-------------------------------------------------------------------
        inline PALText::LineBreakpoint const& LsRun::GetLineBreakpoint(XUINT32 cp) const
        {
            ASSERT(m_lineBreakpoints != NULL);
            ASSERT(cp < m_length);
            return m_lineBreakpoints[cp];
        }


        //-------------------------------------------------------------------
        //
        //  Member:
        //      LsRun::GetFontSimulations
        //
        //  Synopsis:
        //      Gets algorithmic style simulations to be applied to the font face.
        //
        //-------------------------------------------------------------------
        inline FssFontSimulations::Enum LsRun::GetFontSimulations() const
        {
            return m_pFontFace->GetSimulations();
        }

        //-------------------------------------------------------------------
        //
        //  Member:
        //      LsRun::IsGlyphBased
        //
        //  Synopsis:
        //      Determines whether this is this glyph based run.
        //
        //-------------------------------------------------------------------
        inline bool LsRun::IsGlyphBased() const
        {
            return m_glyphBased;
        }

        //-------------------------------------------------------------------
        //
        //  Member:
        //      LsRun::GetInitialSpacing
        //
        //  Synopsis:
        //      Returns the space to be displayed before this run.
        //
        //-------------------------------------------------------------------
        inline XINT32 LsRun::GetInitialSpacing() const
        {
            return m_initialSpacing;
        }

        //-------------------------------------------------------------------
        //
        //  Member:
        //      LsRun::GetSpaceLastCharacter
        //
        //  Synopsis:
        //      Returns the space to be displayed after every character or 
        //      object.
        //
        //-------------------------------------------------------------------
        inline bool LsRun::GetSpaceLastCharacter() const
        {
            return m_spaceLastCharacter;
        }
    }
}
