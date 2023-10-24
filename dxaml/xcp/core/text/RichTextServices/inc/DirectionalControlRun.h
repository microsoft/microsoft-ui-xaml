// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      DirectionalControlRun class represents directional control data
//      to be used in bidirectional layout.

#pragma once

#include "TextRun.h"
#include "DirectionalControl.h"

namespace RichTextServices
{
    //---------------------------------------------------------------------------
    //
    //  DirectionalControlRun
    //
    //  DirectionalControlRun class represents directional control data.
    //
    //---------------------------------------------------------------------------
    class DirectionalControlRun : public TextRun
    {
    public:

        // Initializes a new instance of the TextRun class.
        DirectionalControlRun(
            _In_ XUINT32 characterIndex,
                // Index of the first character of the run.
            _In_ DirectionalControl::Enum control
                // DirectionalControl type specified by this run.
        );

        // Gets the DirectionalControl specified by this run.
        DirectionalControl::Enum GetDirectionalControl() const;

    protected:

        // Release resources associated with the DirectionalControlRun.
        ~DirectionalControlRun() override;

    private:

        // Type of control specified by this run.
        DirectionalControl::Enum m_control;
    };

    //---------------------------------------------------------------------------
    //
    //  Member:
    //      DirectionalControlRun::GetDirectionalControl
    //
    //  Synopsis:
    //      Gets the DirectionalControl specified by this run.
    //
    //---------------------------------------------------------------------------
    inline DirectionalControl::Enum DirectionalControlRun::GetDirectionalControl() const
    {
        return m_control;
    }
}
