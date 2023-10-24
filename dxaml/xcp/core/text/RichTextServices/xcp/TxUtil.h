// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace RichTextServices
{
    //---------------------------------------------------------------------------
    //
    //  Function:
    //      RichTextServices::TxerrFromXResult
    //
    //  Synopsis:
    //      Maps HRESULT codes to Result::Enum.
    //
    //---------------------------------------------------------------------------
    inline Result::Enum TxerrFromXResult(
        _In_ HRESULT xResult
            // HRESULT value for the conversion to Result::Enum
        )
    {
        extern Result::Enum TxerrFromXResultWorker(_In_ HRESULT xResult);
        return (S_OK == xResult) ? Result::Success : TxerrFromXResultWorker(xResult);
    }

    //---------------------------------------------------------------------------
    //
    //  Macro:
    //      IFC_FROM_HRESULT_RTS(hr)
    //
    //  Synopsis:
    //      Converts an HRESULT into the closest matching Result::Enum and exits
    //      the current method if the Result is a failure code.
    //
    //  Notes:
    //
    //      Always use IFC_FROM_HRESULT_RTS to validate a method that returns an HRESULT.
    //      IFC(TxerrFromXResult(Foo()) is troublesome because PREFast will not understand that we
    //      always jump to Cleanup when Foo fails.
    //
    //      extern HRESULT XResultAPI();
    //
    //      Result::Enum Sample()
    //      {
    //          Result::Enum txhr = Result::Success;
    //          IFC_FROM_HRESULT_RTS(XResultAPI());
    //      Cleanup:
    //          return hr;
    //      }
    //
    //---------------------------------------------------------------------------
    #define IFC_FROM_HRESULT_RTS(xresult)                                                           \
    {                                                                                               \
        HRESULT xhr = xresult;                                                                      \
        txhr = TxerrFromXResult(xhr);                                                               \
        if (FAILED(xhr) || txhr != RichTextServices::Result::Success)                               \
            goto Cleanup;                                                                           \
    }
}
