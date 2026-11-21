// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <functional>

// Escape cancels a focusless pointer drag; handledEventsToo so a host handling Escape cannot strand it.
class DragEscapeHook
{
public:
    DragEscapeHook() = default;
    DragEscapeHook(const DragEscapeHook&) = delete;
    DragEscapeHook& operator=(const DragEscapeHook&) = delete;
    ~DragEscapeHook()
    {
        try { Detach(); } catch (...) {}
    }

    // onEscape returns whether it consumed the key; an Escape with no drag in flight belongs to the app.
    void Attach(const winrt::XamlRoot& xamlRoot, std::function<bool()> onEscape)
    {
        auto const root = !m_handler && xamlRoot ? xamlRoot.Content().try_as<winrt::UIElement>() : nullptr;
        if (!root)
        {
            return;
        }

        // RemoveHandler needs the same boxed instance.
        m_handler = winrt::box_value(winrt::KeyEventHandler(
            [onEscape = std::move(onEscape)](winrt::IInspectable const&, winrt::KeyRoutedEventArgs const& args)
            {
                if (args.Key() == winrt::VirtualKey::Escape && onEscape())
                {
                    args.Handled(true);
                }
            }));
        root.AddHandler(winrt::UIElement::KeyDownEvent(), m_handler, true /* handledEventsToo */);
        m_root = winrt::make_weak(root);
    }

    void Detach()
    {
        auto const handler = std::exchange(m_handler, nullptr);
        auto const root = std::exchange(m_root, nullptr).get();
        if (handler && root)
        {
            root.RemoveHandler(winrt::UIElement::KeyDownEvent(), handler);
        }
    }

private:
    winrt::IInspectable m_handler{ nullptr };
    winrt::weak_ref<winrt::UIElement> m_root{ nullptr };
};
