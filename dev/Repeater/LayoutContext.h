// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "LayoutContext.g.h"

class LayoutContext :
    public winrt::implementation::LayoutContextT<LayoutContext, winrt::composable>
{
public:
#pragma region ILayoutContext
    winrt::IInspectable LayoutState();
    void LayoutState(winrt::IInspectable const& value);
#pragma endregion

#pragma region ILayoutContextOverrides
    virtual winrt::IInspectable LayoutStateCore();
    virtual void LayoutStateCore(winrt::IInspectable const& state);
#pragma endregion

    int Indent()
    {
        return m_indent;
    }

    void Indent(int value)
    {
        m_indent = value;
    }

private:
    int m_indent{ 0 };

};
