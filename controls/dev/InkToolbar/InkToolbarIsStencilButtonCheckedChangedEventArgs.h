// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "InkToolbarIsStencilButtonCheckedChangedEventArgs.g.h"

namespace winrt::implementation
{
    struct InkToolbarIsStencilButtonCheckedChangedEventArgs :
        InkToolbarIsStencilButtonCheckedChangedEventArgsT<InkToolbarIsStencilButtonCheckedChangedEventArgs>
    {
        InkToolbarIsStencilButtonCheckedChangedEventArgs() = default;
        InkToolbarIsStencilButtonCheckedChangedEventArgs(
            winrt::InkToolbarStencilButton const& button,
            winrt::InkToolbarStencilKind kind)
            : m_stencilButton(button), m_stencilKind(kind) {}

        winrt::InkToolbarStencilButton StencilButton() { return m_stencilButton; }
        winrt::InkToolbarStencilKind StencilKind() { return m_stencilKind; }

    private:
        winrt::InkToolbarStencilButton m_stencilButton{ nullptr };
        winrt::InkToolbarStencilKind m_stencilKind{ winrt::InkToolbarStencilKind::Ruler };
    };
}

