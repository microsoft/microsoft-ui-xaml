// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "VirtualizingLayoutContext.h"

class RepeaterLayoutContext :
    public winrt::implements<RepeaterLayoutContext, VirtualizingLayoutContext>
{
public:
    explicit RepeaterLayoutContext(const winrt::ItemsRepeater& owner);

    // Explicitly implement GetRuntimeClassName because winrt::implements chooses the first interface
    // as our name and we want the concrete VirtualizingLayoutContext as our name.
    [[nodiscard]] hstring GetRuntimeClassName() const
    {
        return VirtualizingLayoutContext::GetRuntimeClassName();
    }

#pragma region ILayoutContextOverrides

    winrt::IInspectable LayoutStateCore();
    void LayoutStateCore(winrt::IInspectable const& value);

#pragma endregion

#pragma region IVirtualizingLayoutContextOverrides

    int32_t ItemCountCore();
    winrt::UIElement GetOrCreateElementAtCore(int index, winrt::ElementRealizationOptions const& options);

    winrt::IInspectable GetItemAtCore(int index);
    void RecycleElementCore(winrt::UIElement const& element);

    winrt::Rect RealizationRectCore();

    int32_t RecommendedAnchorIndexCore();

    winrt::Point LayoutOriginCore();
    void LayoutOriginCore(winrt::Point const& value);

#pragma endregion

private:
    winrt::ItemsRepeater GetOwner();

    // We hold a weak reference to prevent a leaking reference
    // cycle between the ItemsRepeater and its layout.
    winrt::weak_ref<winrt::ItemsRepeater> m_owner{};
};
