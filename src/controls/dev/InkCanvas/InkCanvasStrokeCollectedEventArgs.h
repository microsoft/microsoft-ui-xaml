// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "InkCanvasStrokeCollectedEventArgs.g.h"

namespace winrt::implementation
{
    struct InkCanvasStrokeCollectedEventArgs : InkCanvasStrokeCollectedEventArgsT<InkCanvasStrokeCollectedEventArgs>
    {
        InkCanvasStrokeCollectedEventArgs() = default;
        InkCanvasStrokeCollectedEventArgs(winrt::Windows::UI::Input::Inking::InkStroke const& stroke)
            : m_stroke(stroke) {}

        winrt::Windows::UI::Input::Inking::InkStroke Stroke() { return m_stroke; }

    private:
        winrt::Windows::UI::Input::Inking::InkStroke m_stroke{ nullptr };
    };
}
