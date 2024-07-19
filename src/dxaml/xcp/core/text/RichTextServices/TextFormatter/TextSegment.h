// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      TextSegment provides storage for segment of text content currently
//      being analyzed.

#pragma once

#include <TextAnalysis.h>

struct IDWriteFactory2;

namespace RichTextServices
{
    class TextSource;

    namespace Internal
    {
        class TextRunData;
        class TextAnalysisSink;
        class TextItemizer;

        class StackTextAnalysisSource final : public ABI::Microsoft::Internal::FrameworkUdk::ITextFormatCallbacks
        {
        public:
            StackTextAnalysisSource(
                _In_reads_(textLength) wchar_t const* textString,
                uint32_t textLength,
                _In_z_ wchar_t const* localeName,
                DWRITE_READING_DIRECTION readingDirection,
                IDWriteNumberSubstitution* numberSubstitution
                )
                :   textLength_(textLength),
                    textString_(textString),
                    localeName_(localeName),
                    readingDirection_(readingDirection),
                    numberSubstitution_(numberSubstitution)
            {
            }

            HRESULT STDMETHODCALLTYPE GetTextAtPosition(
                uint32_t textPosition,
                _Out_ wchar_t const** textString,
                _Out_ uint32_t* textLength
                ) throw() override
            {
                *textString = &textString_[textPosition];
                *textLength = textLength_ - textPosition;
                return S_OK;
            }


            HRESULT STDMETHODCALLTYPE GetTextBeforePosition(
                uint32_t textPosition,
                _Out_ wchar_t const** textString,
                _Out_ uint32_t* textLength
                ) throw() override
            {
                *textString = &textString_[0];
                *textLength = textPosition - 0; // text length is valid from current position backward
                return S_OK;
            }


            DWRITE_READING_DIRECTION STDMETHODCALLTYPE GetParagraphReadingDirection() throw() override
            {
                return readingDirection_;
            }


            HRESULT STDMETHODCALLTYPE GetLocaleName(
                uint32_t textPosition,
                _Out_ uint32_t* textLength,
                _Out_ wchar_t const** localeName
                ) throw() override
            {
                // The pointer returned should remain valid until the next call,
                // or until analysis ends. Since only one locale name is supported,
                // the text length is valid from the current position forward to
                // the end of the string.

                *localeName = localeName_;
                *textLength = textLength_ - textPosition;

                return S_OK;
            }


            HRESULT STDMETHODCALLTYPE GetNumberSubstitution(
                uint32_t textPosition,
                _Out_ uint32_t* textLength,
                _COM_Outptr_ IDWriteNumberSubstitution** numberSubstitution
                ) throw() override
            {
                if (numberSubstitution_ != nullptr)
                    numberSubstitution_->AddRef();

                *numberSubstitution = numberSubstitution_;
                *textLength = textLength_ - textPosition;

                return S_OK;
            }

            BOOL STDMETHODCALLTYPE SupportsGetLocaleNameList() override
            {
                return FALSE;
            }

            HRESULT STDMETHODCALLTYPE GetLocaleNameList(
                uint32_t textPosition,
                _Out_ uint32_t* textLength,
                _Out_ wchar_t const** localeName
                ) throw() override
            {
                return E_NOTIMPL;
            }

            // This class is a stack local variable, so the count can only be one.
            unsigned long STDMETHODCALLTYPE AddRef() throw() override
            {
                return 1;
            }

            unsigned long STDMETHODCALLTYPE Release() throw() override
            {
                return 1;
            }

            HRESULT STDMETHODCALLTYPE QueryInterface(IID const& iid, _Out_ void** object) throw() override
            {
                if (iid == __uuidof(IUnknown)
                ||  iid == __uuidof(IDWriteTextAnalysisSource)
                    )
                {
                    *object = static_cast<IDWriteTextAnalysisSource*>(this);
                }
                else
                {
                    *object = nullptr;
                    return E_NOINTERFACE;
                }

                AddRef();
                return S_OK;
            }

            uint32_t GetTextLength() const
            {
                return textLength_;
            }

        private:
            // All weak references.
            uint32_t textLength_;
            wchar_t const* textString_;
            wchar_t const* localeName_;
            DWRITE_READING_DIRECTION readingDirection_;
            IDWriteNumberSubstitution* numberSubstitution_;
        };

        //---------------------------------------------------------------------------
        //
        //  TextSegment
        //
        //  Provides storage for segment of text content currently being analyzed.
        //
        //---------------------------------------------------------------------------
        class TextSegment final : public PALText::ITextAnalysisSource
        {
        public:
            // Initializes a new instance of the TextSegment class.
            TextSegment();

            // Release resources associated with the TextSegment.
            ~TextSegment();

            // Determines whether segment is empty.
            bool IsEmpty() const;

            // Clears the content of the segment.
            void Clear();

            // Populates empty text segment with content provided by given TextSource.
            Result::Enum Populate(
                _In_ XUINT32 characterIndex,
                _In_ TextSource *pTextSource,
                _In_ FlowDirection::Enum paragraphFlowDirection
                );

            // Performs analysis on pre-populated content and creates collection of runs.
            Result::Enum Analyze(_In_ IFontAndScriptServices *pFontAndScriptServices);

            XUINT32 GetTotalLength() const;

            // Fetches the next run from the collection on analyzed text content.
            TextRunData * FetchNextRun();


            // Get a block of text starting at the specified text position.
            HRESULT GetTextAtPosition(
                _In_ XUINT32 textPosition,
                _Outptr_result_buffer_(*pTextLength) WCHAR const **ppTextString,
                _Out_ XUINT32 *pTextLength
                ) override;

            // Get a block of text immediately preceding the specified position.
            HRESULT GetTextBeforePosition(
                _In_ XUINT32 textPosition,
                _Outptr_result_buffer_(*pTextLength) WCHAR const** ppTextString,
                _Out_ XUINT32* pTextLength
                ) override;

            // Get paragraph reading direction.
            FssReadingDirection::Enum GetParagraphReadingDirection() override;

            // Get locale name on the range affected by it.
            HRESULT GetLocaleName(
                _In_ XUINT32 textPosition,
                _Out_ XUINT32* pTextLength,
                _Outptr_result_z_ WCHAR const** ppLocaleName
                ) override;

            // Get number substitution on the range affected by it.
            HRESULT GetNumberSubstitution(
                _In_ XUINT32 textPosition,
                _Out_ XUINT32* pTextLength,
                _Outptr_ IDWriteNumberSubstitution** ppNumberSubstitution
                ) override;

            HRESULT GetLocaleNameList(
                _In_ UINT32 textPosition,
                _Out_ UINT32* pTextLength,
                _Outptr_result_z_ WCHAR const** pplocaleNameList
                ) final;

            // Referenced counted object interface.
            XUINT32 AddRef() override;
            XUINT32 Release() override;

        private:

            // Points to the first RunData in the collection representing the text content of this segment.
            TextRunData *m_pFirstRunData;

            IFontAndScriptServices *m_pFontAndScriptServices;

            // Recently accessed TextRunData. Cached to optimize sequential access.
            mutable TextRunData *m_pRecentRunData;

            // The index of the first character of the run represented by m_pRecentRunData.
            mutable XUINT32 m_characterIndexOfRecentRunData;

            // Sum of the lengths of all the TextRuns in this segment.
            XUINT32 m_totalLength;

            XUINT32 m_totalLengthForAnalyzingLineBreakpoints;
            bool m_analyzingLineBreakpoints;

            // The paragraph flow direction.
            FlowDirection::Enum m_paragraphFlowDirection;

            // Buffers to hold the explicit directional embedding and object replacement characters to be passed to DWrite.
            static const WCHAR m_LRM;
            static const WCHAR m_RLM;
            static const WCHAR m_LRE;
            static const WCHAR m_RLE;
            static const WCHAR m_PDF;
            static const WCHAR m_hidden;

            // Analyzes the text to determine which fonts to use for the different text ranges.
            Result::Enum AnalyzeFonts();

            // Analyzes the text to determine the text ranges that require shaping and those that do not.
            Result::Enum AnalyzeComplexity(_In_ PALText::ITextAnalyzer *pTextAnalyzer);

            HRESULT NeedNumberSubstitution(_Out_ bool* needNumberSubstitution);

            // Returns a pointer to the run at a given text position.
            // This methods treats non-textual runs as if they were not there.
            TextRun * GetTextRunAtPosition(_In_ XUINT32 textPosition) const;

            //  Gets text for analysis at the specified position..
            Result::Enum GetAnalysisTextAtPosition(
                _In_ XUINT32 textPosition,
                _In_ bool getPreviousText,
                _Outptr_result_buffer_(*pTextLength) WCHAR const **ppTextString,
                _Out_ XUINT32 *pTextLength
                );

            // Set TextRun m_initialSpacing and m_spaceLastCharacter to achieve
            // balanced character spacing at direction boundaries in bidirectional
            // text.
            void BalanceBidiCharacterSpacing();

            // Return true if this run should be hidden from DWrite during line break analysis.
            static bool SkipForLineBreakAnalysis(_In_ TextRun* pTextRun);


            struct LineBreakpointAnalysisHelper : public PALText::ITextAnalysisSink
            {
                LineBreakpointAnalysisHelper(_In_ TextSegment* pOwner);
                virtual ~LineBreakpointAnalysisHelper();


                // Report script analysis for the text range.
                HRESULT SetScriptAnalysis(
                    _In_ XUINT32 textPosition,
                    _In_ XUINT32 textLength,
                    _In_ FssScriptAnalysis const* pScriptAnalysis
                    ) override
                {
                    ASSERT(FALSE);
                    return E_NOTIMPL;
                }

                // Set bidirectional level on the range, called once per each
                // level run change (either explicit or resolved implicit).
                HRESULT SetBidiLevel(
                    _In_ XUINT32 textPosition,
                    _In_ XUINT32 textLength,
                    _In_ XUINT8 explicitLevel,
                    _In_ XUINT8 resolvedLevel
                    ) override
                {
                    ASSERT(FALSE);
                    return E_NOTIMPL;
                }

                // Set number substitution on the range, called once per each
                // level run change (either explicit or resolved implicit).
                HRESULT SetNumberSubstitution(
                    _In_ XUINT32 textPosition,
                    _In_ XUINT32 textLength,
                    _In_ IDWriteNumberSubstitution* pNumberSubstitution
                    ) override
                {
                    ASSERT(FALSE);
                    return E_NOTIMPL;
                }

                // Set line breakpoints for the given range.
                HRESULT SetLineBreakpoints(
                    _In_ XUINT32 textPosition,
                    _In_ XUINT32 textLength,
                    _In_reads_(textLength) PALText::LineBreakpoint const* lineBreakpoints
                    ) override;


                // Referenced counted object interface.
                XUINT32 AddRef() override;
                XUINT32 Release() override;

            private:
                TextSegment *m_pOwner;

                // When LineBreakpointAnalysisHelper is in use it temporarily modifies the m_totalLength
                // field of the TextSegment to be the m_totalLengthForAnalyzingLineBreakpoints value
                // and this is just stashing the old original m_totalLength so we can restore it in
                // our destructor.
                XUINT32 m_oldTotalLength;
            };

        };

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      TextSegment::IsEmpty
        //
        //  Returns:
        //      Determines whether segment is empty.
        //
        //---------------------------------------------------------------------------
        inline bool TextSegment::IsEmpty() const
        {
            return (m_pFirstRunData == NULL);
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      TextSegment::GetTotalLength
        //
        //  Returns:
        //      Returns the length of this text segment.
        //
        //---------------------------------------------------------------------------
        inline XUINT32 TextSegment::GetTotalLength() const
        {
            return m_totalLength;
        }
    }
}
