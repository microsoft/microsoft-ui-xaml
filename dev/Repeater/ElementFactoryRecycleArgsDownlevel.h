// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// We do some fakery here to make this work downlevel.
// Windows::UI::Xaml:ElementFactoryRecycleArgs type has been added to WUX in RS5. However, we want Repeater to use it downlevel. 
// The way we achieve this is by implementing the Window::UI::Xaml::IElementFactoryRecycleArgs
// interface here on our own type and 'lie' that our runtime class name is Windows.UI.Xaml.ElementFactoryRecycleArgs !

class ElementFactoryRecycleArgsDownlevel
    : public ReferenceTracker<
        ElementFactoryRecycleArgsDownlevel,
        reference_tracker_implements_t<winrt::Windows::UI::Xaml::IElementFactoryRecycleArgs>::type>
{
public:
    using class_type = winrt::ElementFactoryRecycleArgs;

    operator class_type() const noexcept
    {
        return static_cast<winrt::IInspectable>(*this).as<class_type>();
    }

    hstring GetRuntimeClassName() const
    {
        // The type name being in windows namespace is intentional.
        return L"Windows.UI.Xaml.ElementFactoryRecycleArgs";
    }

#pragma region IElementFactoryRecycleArgs
    winrt::UIElement Element();
    void Element(winrt::UIElement const& value);

    winrt::UIElement Parent();
    void Parent(winrt::UIElement const& value);
#pragma endregion

private:
    tracker_ref<winrt::UIElement> m_element{ this };
    tracker_ref<winrt::UIElement> m_parent{ this };
};