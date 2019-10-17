// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ViewChangeBase.h"

enum class ScrollerViewKind
{
    Absolute,
    RelativeToCurrentView,
#ifdef ScrollerViewKind_RelativeToEndOfInertiaView
    RelativeToEndOfInertiaView,
#endif
};

class ViewChange : public ViewChangeBase
{
public:
    ViewChange(
        ScrollerViewKind viewKind,
        winrt::IInspectable const& options);
    ~ViewChange();

    ScrollerViewKind ViewKind() const
    {
        return m_viewKind;
    }

    winrt::IInspectable Options() const
    {
        return m_options;
    }

private:
    ScrollerViewKind m_viewKind{ ScrollerViewKind::Absolute };
    // ScrollOptions or ZoomOptions instance associated with this view change.
    winrt::IInspectable m_options{ nullptr };
};

