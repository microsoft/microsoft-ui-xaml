// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ViewChangeBase.h"

enum class ScrollingPresenterViewKind
{
    Absolute,
    RelativeToCurrentView,
#ifdef ScrollingPresenterViewKind_RelativeToEndOfInertiaView
    RelativeToEndOfInertiaView,
#endif
};

class ViewChange : public ViewChangeBase
{
public:
    ViewChange(
        ScrollingPresenterViewKind viewKind,
        winrt::IInspectable const& options);
    ~ViewChange();

    ScrollingPresenterViewKind ViewKind() const
    {
        return m_viewKind;
    }

    winrt::IInspectable Options() const
    {
        return m_options;
    }

private:
    ScrollingPresenterViewKind m_viewKind{ ScrollingPresenterViewKind::Absolute };
    // ScrollOptions or ZoomOptions instance associated with this view change.
    winrt::IInspectable m_options{ nullptr };
};

