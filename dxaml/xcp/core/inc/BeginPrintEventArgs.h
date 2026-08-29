// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Synopsis:
//      EventArgs used for StartPrint event in Printing context.
//      Doesn't provide anything special now, but allows for future
//      additions.

class CBeginPrintEventArgs final : public CEventArgs
{
public:
    CBeginPrintEventArgs()
    {
    }
};
