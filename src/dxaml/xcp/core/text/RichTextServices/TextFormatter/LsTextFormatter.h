// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      LsTextFormatter provides services for formatting text and breaking text lines using LS.

#pragma once

#include "TextFormatter.h"
#include "LsHostContext.h"

namespace RichTextServices
{
    namespace Internal
    {
        //---------------------------------------------------------------------------
        //
        //  LsTextFormatter
        //
        //  Provides services for formatting text and breaking text lines using LS.
        //
        //---------------------------------------------------------------------------
        class LsTextFormatter : public TextFormatter
        {
        public:

            // Constructor.
            LsTextFormatter(
                _In_ IFontAndScriptServices *pFontAndScriptServices
                    // Provides an interface to access font and script specific data
            );

            // Creates a TextLine that is used for formatting and displaying text content.
            Result::Enum FormatLine(
                _In_ TextSource *pTextSource,
                _In_ XUINT32 firstCharIndex,
                _In_ XFLOAT wrappingWidth,
                _In_ TextParagraphProperties *pTextParagraphProperties,
                _In_opt_ TextLineBreak *pPreviousLineBreak,
                _In_opt_ TextRunCache *pTextRunCache,
                _Outptr_ TextLine **ppTextLine
                );

        protected:

            // Destructor.
            virtual ~LsTextFormatter();

        private:

            // Initializes line breaking control for LS.
            Result::Enum InitLineBreakingInfo(Ptls6::LSBREAKINGINFO &lsBreakingInfo) const;

            // Initializes client callbacks for LS.
            void InitClientCallbacks(Ptls6::LSCBK &lsCbk) const;

            // Initializes installed object handlers for LS.
            Result::Enum InitObjectHandlers(
                _Out_ LONG* pCount,
                _Outptr_ Ptls6::PCLSIMETHODS *ppHandlers
                ) const;

            // Initializes straight-text configuration data for LS.
            void InitTextConfiguration(Ptls6::LSTXTCFG &lsTextConfig) const;

            // Creates LS context.
            Result::Enum CreateLsContext();

            // Releases LS context.
            void ReleaseLsContext();

            // Validates wrapping width by adjusting it against min/max permitted values.
            static XFLOAT ValidateWrappingWidth(
                _In_ XFLOAT wrappingWidth
                );

            // Calculates the max line width based on wrapping and other paragraph properties.
            static XFLOAT CalculateMaxLineWidth(
                _In_ XFLOAT wrappingWidth,
                _In_ TextParagraphProperties *pTextParagraphProperties
                );

        private:

            friend class LsTextLine;

            Ptls6::PLSC m_pLsContext;
                // Context created by LS. It must be provided as explicit input parameter to all Line Services APIs.

            Ptls6::LSCONTEXTINFO m_lsContextInfo{};
                // Configuration information required to create LS context.

            LsHostContext m_lsHostContext;
                // Context used by LS callbacks to access host specific information.
        };
    }
}

