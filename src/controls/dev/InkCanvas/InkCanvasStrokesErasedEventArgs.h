// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "InkCanvasStrokesErasedEventArgs.g.h"

namespace winrt::implementation
{
    struct InkCanvasStrokesErasedEventArgs : InkCanvasStrokesErasedEventArgsT<InkCanvasStrokesErasedEventArgs>
    {
        InkCanvasStrokesErasedEventArgs() = default;
        InkCanvasStrokesErasedEventArgs(winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::UI::Input::Inking::InkStroke> const& strokes)
            : m_strokes(strokes) {}

        winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::UI::Input::Inking::InkStroke> Strokes() { return m_strokes; }

    private:
        winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::UI::Input::Inking::InkStroke> m_strokes{ nullptr };
    };
}
