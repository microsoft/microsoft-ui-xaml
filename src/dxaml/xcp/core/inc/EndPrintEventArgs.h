// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Synopsis:
//      EventArgs used for reproting errors while printing.
//      Used by the EndPrint event to give app authors the errors - if any
//      that happened during printing.

class CEndPrintEventArgs final : public CEventArgs
{
public:
    CEndPrintEventArgs()
    {
        m_iErrorCode = 0;
    }

    XINT32 m_iErrorCode;
};
