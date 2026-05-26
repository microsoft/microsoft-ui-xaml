// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Result.h"

namespace RichTextServices
{
    struct ITextElementRecord;

    //---------------------------------------------------------------------------
    //
    //  ITextElement
    //
    //  Implemented by the host, represents an element in the backing store.
    //
    //---------------------------------------------------------------------------
    struct ITextElement
    {
        //---------------------------------------------------------------------------
        //
        //  Member:
        //      ITextElement::AddRef
        //
        //  Synopsis:
        //      Increments the count of references to this object.
        //
        //---------------------------------------------------------------------------
        virtual XUINT32 AddRef() = 0;

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      ITextElement::Release
        //
        //  Synopsis:
        //      Decrements the count of references to this object.
        //
        //---------------------------------------------------------------------------
        virtual XUINT32 Release() = 0;

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      ITextElement::GetElementRecord
        //
        //  Synopsis:
        //      Called when an element is removed, to serialize it for storage
        //      within an UndoUnit.
        //
        //  Returns:
        //      An error code indicating success or failure.
        //
        //  Notes:
        //      This method is only called if the host enables undo from a call to
        //      the IBackingStoreHost::OnBeforeChanges method.
        //
        //---------------------------------------------------------------------------
        #pragma warning (push)
        // Description: Disable prefast warning 26020 : Error in annotation at d:\dev11\sl2\xcp\core\inc\richtextserviceshelper.h(46) on 'Enum' :no parameter named RichTextServices.
        // Reason     : Per discussion with Prefast team, this annotation usage is correct but cannot be processed by OACR's parser. Upgrade to new version of OACR should fix this.
        //              Tracked in bug 92086.
        #pragma warning (disable : 26020)
        virtual RichTextServices::Result::Enum GetElementRecord(
            _Outptr_ ITextElementRecord **ppRecord
                // Receives the serialized element.
            ) = 0;
        #pragma warning (pop)
    };
}
