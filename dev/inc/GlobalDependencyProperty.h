// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Helper to use instead of winrt::DependencyProperty in a static global. Avoids getting a dynamic initializer and
// destructor, so it saves on dll size as well as avoids destructing a WinRT object at dll unload.
struct GlobalDependencyProperty
{
    GlobalDependencyProperty() = default;

    constexpr GlobalDependencyProperty(nullptr_t)
    {
    }

    // Can assign to nullptr which will free the DP.
    GlobalDependencyProperty& operator=(nullptr_t)
    {
        Property() = nullptr;
        return *this;
    }

    // Can also copy from winrt::DependencyProperty to transfer ownership for initialization.
    GlobalDependencyProperty& operator=(winrt::DependencyProperty const& other)
    {
        Property() = other;
        return *this;
    }

    GlobalDependencyProperty(winrt::DependencyProperty const& other)
    {
        Property() = other;
    }

    // Cannot copy or assign this helper because it is only for global static usage.
    GlobalDependencyProperty(GlobalDependencyProperty const&) = delete;
    GlobalDependencyProperty& operator=(GlobalDependencyProperty const& other) = delete;

    operator winrt::DependencyProperty() const
    {
        return Property();
    }

    operator bool() const
    {
        return static_cast<bool>(m_dependencyProperty);
    }

    bool operator==(winrt::DependencyProperty const& other) const
    {
        return Property() == other;
    }

    bool operator==(nullptr_t) const
    {
        return Property() == nullptr;
    }

private:
    winrt::DependencyProperty& Property()
    {
        return reinterpret_cast<winrt::DependencyProperty&>(m_dependencyProperty);
    }

    [[nodiscard]] winrt::DependencyProperty const& Property() const
    {
        return reinterpret_cast<winrt::DependencyProperty const&>(m_dependencyProperty);
    }

    IUnknown* m_dependencyProperty{};
};
