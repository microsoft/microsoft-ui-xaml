// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Custom types and static utilities for LS client.

#pragma once

namespace RichTextServices
{
    namespace Internal
    {
        //---------------------------------------------------------------------------
        //
        //  LsSpanType
        //
        //  Defines possible types of spans submitted to LS.
        //
        //---------------------------------------------------------------------------
        struct LsSpanType
        {
            enum Enum
            {
                // Undefined span.
                Undefined           = 0,

                // Master span for the paragraph.
                Paragraph           = 1,

                // TextEmbedded object span type.
                TextEmbeddedObject  = 2,

                // First from 61 possible bidi span levels - level 0.
                BidiBase            = 10,
            };
        };
        
        //---------------------------------------------------------------------------
        //
        //  LsObjectId
        //
        //  Defines possible types of objects for which custom / installed
        //  LS handlers are used.
        //
        //---------------------------------------------------------------------------
        struct LsObjectId
        {
            enum Enum
            {
                // Reverse object.
                ReverseObject       = 0,

                // Text embedded object.
                TextEmbeddedObject  = 1,

                // Max Object ID
                ObjectIdMax         = 2,
            };
        };

        // Helper for LineServicesGetGlyphs, returns TRUE iff a char->glyph map
        // contains entries where more than a single code point maps to any
        // number of glyphs.
        bool HasMultiCharacterClusters(
            _In_ XUINT32 cwch,
                // Number of characters to be shaped
            _In_ XUINT32 cGlyphs,
                // Number of glyphs representing the characters
            _In_reads_(cwch) Ptls6::PGMAP rggmap
                // Parallel to the char codes mapping characters->glyph info
            );

        // Helper to determine whether an LS text cell requires caret stops at each code point.
        Result::Enum StopAtEachCharacterInCell(
            _In_ Ptls6::PLSQSUBINFO pSublineInfo,
            _In_ LONG sublineCount,
            _In_ Ptls6::LSTEXTCELL textCell,
            _In_ IFontAndScriptServices *pFontAndScriptServices,
            _Out_ bool *pStopAtEveryCharacter
        );

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      RtlFromLSTFLOW
        //
        //  Synopsis:
        //      Helper to find bidi level given LSTFLOW.
        //
        //---------------------------------------------------------------------------
        inline bool RtlFromLSTFLOW(_In_ Ptls6::LSTFLOW lstflow)
        {
            return (lstflow & fUDirection) ? true : false;
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      ResultFromLSErr
        //
        //  Synopsis:
        //      Gets Result::Enum from LS error code.
        //
        //---------------------------------------------------------------------------
        inline static Result::Enum ResultFromLSErr(
            _In_ Ptls6::LSERR error
            )
        {
            if (error == lserrTooLongGlyphContext)
            {
                return Result::InternalError;
            }
            if (error != lserrNone)
            {
                return Result::FormattingError;
            }
            return Result::Success;
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      LSErrFromResult
        //
        //  Synopsis:
        //      Gets LS error code from Result::Enum.
        //
        //---------------------------------------------------------------------------
        inline static Ptls6::LSERR LSErrFromResult(
            _In_ Result::Enum txerr
            )
        {
            if (txerr != Result::Success)
            {
                return lserrClientAbort;
            }
            return lserrNone;
        }


        //---------------------------------------------------------------------------
        //
        //  Member:
        //      IsLSSpaceCharacter
        //
        //  Synopsis:
        //      Is this a character that LS will treat as a "space" during line breaking?
        //      This matches the set of characters we give to LS in LsTextFormatter::InitTextConfiguration.
        //
        //---------------------------------------------------------------------------

        static bool IsLSSpaceCharacter(WCHAR ch)
        {
            return (ch == UNICODE_SPACE)
                || (ch == UNICODE_EM_SPACE)
                || (ch == UNICODE_EN_SPACE)
                || (ch == UNICODE_THIN_SPACE)
                || (ch == UNICODE_IDEOGRAPHIC_SPACE);
        }        
    }
}
