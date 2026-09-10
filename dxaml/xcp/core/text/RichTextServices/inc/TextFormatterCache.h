// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      TextFormatterCache provides TextFormatter caching services in order 
//      to improve performance.

#pragma once

namespace RichTextServices
{
    class TextFormatter;

    //---------------------------------------------------------------------------
    //
    //  TextFormatterCache
    //
    //  Provides TextFormatter caching services in order to improve performance.
    //
    //---------------------------------------------------------------------------
    class TextFormatterCache
    {
    public:
        // Initializes a new instance of the TextFormatterCache class.
        TextFormatterCache(
            _In_ IFontAndScriptServices *pFontAndScriptServices
                // Provides an interface to access font and script specific data.
            );

        // Release resources associated with the TextFormatterCache.
        ~TextFormatterCache();

        // Acquires TextFormatter for exclusive use. When pPreferredTextFormatter is supplied it is
        // checked out in preference to any other formatter so continuation formatting runs on the
        // exact formatter (LineServices context) that produced a previous line break, to which the
        // break record is bound. The preferred formatter is taken from the free list if present, or
        // re-registered if it was evicted by ReleaseUnusedTextFormatters but is still alive (kept by
        // a cached break record's reference). Only when no preferred formatter is supplied is an
        // arbitrary cached or newly created formatter returned.
        Result::Enum AcquireTextFormatter(
            _Outptr_ TextFormatter **ppTextFormatter,
            _In_opt_ TextFormatter *pPreferredTextFormatter = nullptr
            );

        // Releases TextFormatter and makes it available for reuse.
        void ReleaseTextFormatter(
            _In_opt_ TextFormatter *pTextFormatter
            );

        //Releases unused TextFormatters to reduce memory usage
        void ReleaseUnusedTextFormatters();

    private:

        // Stores data associated with cached TextFormatters
        struct TextFormatterData
        {
            TextFormatter *pTextFormatter;
            TextFormatterData *pNext;
        };

        // The head of collection of free TextFormatters
        TextFormatterData *m_pFreeTextFormatters;

        // The head of collection of TextFormatters currently in use
        TextFormatterData *m_pUsedTextFormatters;

        // Provides an interface to access font and script specific data.
        IFontAndScriptServices *m_pFontAndScriptServices;
    };
}
