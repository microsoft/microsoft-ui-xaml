// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines LsParagraphSpan for master span objects with paragraph
//      scope.

#pragma once

#include "LsSpan.h"
#include "FlowDirection.h"

namespace RichTextServices
{
    namespace Internal
    {
        //-------------------------------------------------------------------
        //
        //  LsParagraphSpan
        //
        //  LsSpan for master (paragraph scope) span types.
        //
        //-------------------------------------------------------------------
        class LsParagraphSpan : public LsSpan
        {
        public:

            // Constructor
            LsParagraphSpan(
                _In_ XUINT32 characterIndex,
                _In_ FlowDirection::Enum flowDirection
                );

            // Returns the bidi level of the span.
            XUINT8 GetBidiLevel() const;

        private:

            FlowDirection::Enum m_flowDirection;
        };
    }
}

