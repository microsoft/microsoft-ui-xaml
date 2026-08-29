// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      TextRunCache provides run caching services in order to improve performance.

#pragma once

#include "TextObject.h"

namespace RichTextServices
{
    class TextRun;

    //---------------------------------------------------------------------------
    //
    //  TextRunCache
    //
    //  Provides run caching services in order to improve performance.
    //
    //---------------------------------------------------------------------------
    class TextRunCache : public TextObject
    {
    public:

        // Creates a new instance of the TextRunCache class.
        static Result::Enum Create(_Outptr_ TextRunCache **ppTextRunCache);

        // Clears all runs in the cache.
        // Should be called when content is invalidated.
        virtual void Clear() = 0;
    };
}

