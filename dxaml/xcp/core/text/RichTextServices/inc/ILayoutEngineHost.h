// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ElementType.h"
#include "TextAlignment.h"

namespace RichTextServices
{
    class BackingStore;
    class TextPosition;
    class ElementReference;
    class TextFormatter;
    class TextSource;
    class TextRunCache;
    class TextParagraphProperties;

    //---------------------------------------------------------------------------
    //
    //  ILayoutEngineHost provides the layout engine with access to the backing
    //  store and element types and properties.
    //
    //  The owner of a LayoutEngine must implement this interface,
    //  which is passed to LayoutEngine methods.
    //
    //---------------------------------------------------------------------------
    struct ILayoutEngineHost
    {
        //---------------------------------------------------------------------------
        //
        //  Synopsis:
        //      Gets the BackingStore containing the content.
        //
        //  Returns:
        //      An error code indicating success or failure.
        //
        //---------------------------------------------------------------------------
        #pragma warning (push)
        // Description: Disable prefast warning 26020 : Error in annotation on 'Enum' :no parameter named RichTextServices.
        // Reason     : Per discussion with Prefast team, this annotation usage is correct but cannot be processed by OACR's parser. Upgrade to new version of OACR should fix this.
        //              Tracked in bug 92086.
        #pragma warning (disable : 26020)
        virtual Result::Enum GetBackingStore(_Outptr_ BackingStore **ppBackingStore) = 0;
        #pragma warning (pop)

        //---------------------------------------------------------------------------
        //
        //  Synopsis:
        //      Asks the host if the measure in progress should end prematurely, before
        //      the entire page is measured.
        //
        //  Returns:
        //      TRUE to stop measuring immediately.
        //
        //  Notes:
        //      Hosts typically use this method to break the otherwise unbounded full
        //      measure into manageable chunks, freeing up the calling thread.
        //
        //---------------------------------------------------------------------------
        virtual bool InterruptMeasure() = 0;

        //---------------------------------------------------------------------------
        //
        //  Synopsis:
        //      Gets the ElementType associated with a backing store element.
        //
        //  Returns:
        //      An error code indicating success or failure.
        //
        //---------------------------------------------------------------------------
        virtual Result::Enum GetElementType(
            _In_ const ElementReference &reference, // TODO: should take an IElement?
            _Out_ ElementType::Enum *pElementType
            ) = 0;

        //---------------------------------------------------------------------------
        //
        //  Synopsis:
        //      Gets a TextFormatter used to measure lines.
        //
        //  Returns:
        //      An error code indicating success or failure.
        //
        //  Notes:
        //      The LayoutEngine will call ReleaseTextFormatter after it is
        //      finished using the TextFormatter.
        //
        //---------------------------------------------------------------------------
        virtual Result::Enum GetTextFormatter(_Outptr_ TextFormatter **ppFormatter) = 0;

        //---------------------------------------------------------------------------
        //
        //  Synopsis:
        //      Releases a TextFormatter previously allocated with a call to GetTextFormatter.
        //
        //---------------------------------------------------------------------------
        virtual void ReleaseTextFormatter(_In_ TextFormatter *pFormatter) = 0;

        //---------------------------------------------------------------------------
        //
        //  Synopsis:
        //      Gets the TextSource containing the content.
        //
        //  Notes:
        //      The returned TextSource references the Paragraph with the content
        //      start index provided.  It is NOT a TextSource for the entire document.
        //
        //  Returns:
        //      An error code indicating success or failure.
        //
        //---------------------------------------------------------------------------
        virtual Result::Enum GetParagraphTextSource(
            _In_ XUINT32 paragraphContentStartIndex,
                // Index of the first character of the first line, i.e. the index immediately
                // following the Paragraph start edge.
            _Outptr_ TextSource **ppSource
            ) = 0;

        //---------------------------------------------------------------------------
        //
        //  Synopsis:
        //      Gets the TextAlignment and TextParagraphProperties matching a
        //      backing store element.
        //
        //  Returns:
        //      An error code indicating success or failure.
        //
        //---------------------------------------------------------------------------
        virtual Result::Enum GetTextParagraphProperties(
            _In_ const ElementReference &reference,
            _Out_opt_ TextAlignment::Enum *pAlignment,
            _Outptr_opt_ TextParagraphProperties **ppParagraphProperties
            ) = 0;
    };
}
