// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "uielement.h"
#include <CxxMock.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { 
    namespace Xaml { namespace Focus { namespace XYFocus {
        class XYFocusCUIElement : public CUIElement
        {
        public:
            XYFocusCUIElement(_In_ KnownTypeIndex index = KnownTypeIndex::UIElement);
            bool IsFocusable() const;
            bool IsOccluded() const;
            KnownTypeIndex GetTypeIndex() const;
        private:
            KnownTypeIndex m_index;
        };

        MOCK_CLASS(FocusableXYFocusCUIElement, XYFocusCUIElement)
            FocusableXYFocusCUIElement()
            {
                this->SetSkipFocusSubtree(false);
            }

            FocusableXYFocusCUIElement(_In_ KnownTypeIndex index) : XYFocusCUIElement(index)
            {
                this->SetSkipFocusSubtree(false);
            }

            bool IsFocusable() const { return true; }

            STUB_METHOD(bool, IsOccluded, 0, const);
        END_MOCK
    }}}
}}}}