// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ViewChangeBase.h"

enum class ScrollPresenterViewKind
{
    Absolute,
    RelativeToCurrentView,
#ifdef ScrollPresenterViewKind_RelativeToEndOfInertiaView
    RelativeToEndOfInertiaView,
#endif
};

class ViewChange : public ViewChangeBase
{
public:
    ViewChange(
        ScrollPresenterViewKind viewKind,
        winrt::IInspectable const& options);
    ~ViewChange();

    ScrollPresenterViewKind ViewKind() const
    {
        return m_viewKind;
    }

    winrt::IInspectable Options() const
    {
        return m_options;
    }

private:
    ScrollPresenterViewKind m_viewKind{ ScrollPresenterViewKind::Absolute };
    // ScrollingScrollOptions or ScrollingZoomOptions instance associated with this view change.
    winrt::IInspectable m_options{ nullptr };
};

