// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TxUtil.h"

namespace RichTextServices
{
    //---------------------------------------------------------------------------
    //
    //  Member:
    //      TxerrFromXResultWorker
    //
    //  Synopsis:
    //      Maps HRESULT codes to Result::Enum. Helper for TxerrFromXResult, handles
    //      non-inlined cases.
    //
    //---------------------------------------------------------------------------
    Result::Enum TxerrFromXResultWorker(
        _In_ HRESULT xResult
            // HRESULT value for the conversion to Result::Enum
        )
    {
        Result::Enum txerr = Result::Success;

        if (!SUCCEEDED(xResult))
        {
            switch (xResult)
            {
                case E_OUTOFMEMORY:
                    txerr = Result::OutOfMemory;
                    break;
                    
                case E_NOTIMPL:
                    txerr = Result::NotImplemented;
                    break;
                    
                case E_INVALIDARG:
                    txerr = Result::InvalidParameter;
                    break;
                    
                case E_UNEXPECTED:
                    txerr = Result::Unexpected;
                    break;
                    
                default:
                    txerr = Result::FormattingError;
                    break;
            }
        }

        return txerr;
    }
}
