// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Function:   purecall
//
//  Synopsis:
//      Provides a stub when a pure virtual function is called. 
// 
//  Implementation notes:
//      This function should probably call the debugger. 
//
//------------------------------------------------------------------------

int
__cdecl
_purecall()
{
    return 0;
}
