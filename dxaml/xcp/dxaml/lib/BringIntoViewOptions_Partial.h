// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "BringIntoViewOptions.g.h"
#include <DoubleUtil.h>

namespace DirectUI
{
    PARTIAL_CLASS(BringIntoViewOptions)
    {
        private:
            DOUBLE m_horizontalAlignmentRatio{ DoubleUtil::NaN };
            DOUBLE m_verticalAlignmentRatio{ DoubleUtil::NaN };
            DOUBLE m_horizontalOffset{ 0.0 };
            DOUBLE m_verticalOffset{ 0.0 };

        protected:
            BringIntoViewOptions() {};

            void SetClampedAlignmentRatio(bool isForHorizontalAlignmentRatio, DOUBLE alignmentRatio);

        public:
            _Check_return_ HRESULT get_HorizontalAlignmentRatioImpl(_Out_ DOUBLE* value);
            _Check_return_ HRESULT put_HorizontalAlignmentRatioImpl(DOUBLE value);
            _Check_return_ HRESULT get_VerticalAlignmentRatioImpl(_Out_ DOUBLE* value);
            _Check_return_ HRESULT put_VerticalAlignmentRatioImpl(DOUBLE value);
            _Check_return_ HRESULT get_HorizontalOffsetImpl(_Out_ DOUBLE* value);
            _Check_return_ HRESULT put_HorizontalOffsetImpl(DOUBLE value);
            _Check_return_ HRESULT get_VerticalOffsetImpl(_Out_ DOUBLE* value);
            _Check_return_ HRESULT put_VerticalOffsetImpl(DOUBLE value);
    };
}

