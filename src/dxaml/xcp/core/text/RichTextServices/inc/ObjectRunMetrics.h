// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      ObjectRunMetrics provides information about the 
//      size and properties of an embedded inline object.

#pragma once

namespace RichTextServices
{
    //---------------------------------------------------------------------------
    //
    //  ObjectRunMetrics
    //
    //  Provides information about size and properties of an embedded inline
    //  object.
    //
    //---------------------------------------------------------------------------
    struct ObjectRunMetrics
    {
        XFLOAT width;
        XFLOAT height;
        XFLOAT baseline;
    };
}
