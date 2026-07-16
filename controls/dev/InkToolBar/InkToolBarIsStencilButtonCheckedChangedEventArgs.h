// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "InkToolBarIsStencilButtonCheckedChangedEventArgs.g.h"

namespace winrt::implementation
{
    struct InkToolBarIsStencilButtonCheckedChangedEventArgs :
        InkToolBarIsStencilButtonCheckedChangedEventArgsT<InkToolBarIsStencilButtonCheckedChangedEventArgs>
    {
        InkToolBarIsStencilButtonCheckedChangedEventArgs() = default;
        InkToolBarIsStencilButtonCheckedChangedEventArgs(
            winrt::InkToolBarStencilButton const& button,
            winrt::InkToolBarStencilKind kind)
            : m_stencilButton(button), m_stencilKind(kind) {}

        winrt::InkToolBarStencilButton StencilButton() { return m_stencilButton; }
        winrt::InkToolBarStencilKind StencilKind() { return m_stencilKind; }

    private:
        winrt::InkToolBarStencilButton m_stencilButton{ nullptr };
        winrt::InkToolBarStencilKind m_stencilKind{ winrt::InkToolBarStencilKind::Ruler };
    };
}

