// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// These are serialized in XBFv1, so new values should be added at the end
enum XamlNodeType
{
    xntNone = 0,       // need something to return when un-inited. (and past Eof)
    xntStartObject = 1,
    xntEndObject = 2,
    xntStartProperty = 3,
    xntEndProperty = 4,
    xntText = 5,
    xntValue = 6,
    xntNamespace = 7,

    // These are 'internal' types
    xntEndOfAttributes = 8,
    xntEndOfStream = 9,
    xntLineInfo = 10,
    xntLineInfoAbsolute = 11,

    xntStartConditionalScope = 12,
    xntEndConditionalScope = 13,
};
