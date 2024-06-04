// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//      Event args for errors occurred during the synchronized method calls
//      in JavaScript event handling function.

class CRuntimeErrorEventArgs final : public CErrorEventArgs
{

public:

    xstring_ptr m_strMethodName;    // Method name which is called by Script function.

    XUINT32     m_uLineNumber;            // Line number in script file where runtime error happens.
    XUINT32     m_uCharPosition;          // Charactor position in the error line where the run time error occurs.

    // Destructor
    ~CRuntimeErrorEventArgs() override
    {
    }

    CRuntimeErrorEventArgs(_In_ CCoreServices* pCore) : CErrorEventArgs(pCore)
    {
        m_eType = RuntimeError;

        m_uLineNumber = 0;
        m_uCharPosition = 0;
    }
};
