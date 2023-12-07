// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

enum ErrorType
{
    NoError              = 0,
    UnknownError         = 1,     // Unknown error.
    InitializeError      = 2,
    ParserError          = 3,     // Error occurs during parsing of xaml content.
    ObjectModelError     = 4,     // Error occurs after UIElement is added into the tree.
    RuntimeError         = 5,     // Error occurs in a synchronize call inside JavaScript function.
    DownloadError        = 6,     // Error occurs during the package downloading time.
    MediaError           = 7,     // Error occurs in a MediaElement. // TODO: eval
    ImageError           = 8,     // Error occurs in an Image element or ImageBrush handling.
    ManagedError         = 9      // Error occurs in managed code.
};
