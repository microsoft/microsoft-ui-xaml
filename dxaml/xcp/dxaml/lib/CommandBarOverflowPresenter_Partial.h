// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CommandBarOverflowPresenter.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(CommandBarOverflowPresenter)
    {
    public:
        _Check_return_ HRESULT SetDisplayModeState(bool isFullWidth, bool isOpenUp);

    protected:
        CommandBarOverflowPresenter();

        _Check_return_ HRESULT ChangeVisualState(bool useTransitions) override;

        IFACEMETHOD(OnApplyTemplate)() override;
    private:
        bool m_useFullWidth;
        bool m_shouldOpenUp;
    };
}
