// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      HiddenRun represents hidden content that is not visible and does not
//      affect line layout.

#pragma once

#include "TextRun.h"

namespace RichTextServices
{
    //---------------------------------------------------------------------------
    //
    //  HiddenRun
    //
    //  HiddenRun represents hidden content that is not visible and
    //  does not affect line layout.
    //
    //---------------------------------------------------------------------------
    class HiddenRun : public TextRun
    {
    public:

        // Initializes a new instance of the HiddenRun class.
        HiddenRun(_In_ XUINT32 characterIndex);

    protected:
        // Release resources associated with the HiddenRun.
        ~HiddenRun() override;
    };
}
