// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CDependencyObject;

// This namespace contains TEMPORARY methods that allow for the nice island of
// sane header files to perform actions that would require including the world.
// They MUST be removed as we refactor and cleanup the header file story to be
// more sane.
namespace HeaderDependencyBridges 
{
    // TEMPORARY: Casts the given DO to CString and returns the given
    // xstring_ptr associated with it.
    xstring_ptr StringFromDependencyObject(_In_ CDependencyObject* pDO);
}
