// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

// Evaluates a property path against a row item. A one-time {Binding} on a throwaway
// ContentControl is used rather than reflection because it is the same evaluator the cells use,
// so a path that displays also sorts - including indexers and dotted paths.
//
// TableView reads ItemKeyPath and ParentKeyPath through it, so a tree's keys resolve exactly like
// the cells' bindings.
//
// Not thread-safe and UI-thread affine (it drives the binding engine). Reuse one instance per
// path - EnsureBinding rebinds only when the path changes, so repeated Resolve calls on the same
// path cost a DataContext write and a property read.
struct SortMemberPathResolver
{
    explicit SortMemberPathResolver(const winrt::hstring& sortMemberPath) :
        SortMemberPath(sortMemberPath)
    {
    }

    winrt::IInspectable Resolve(const winrt::IInspectable& item)
    {
        if (!item || SortMemberPath.empty())
        {
            return nullptr;
        }

        try
        {
            EnsureBinding();
            Probe.DataContext(item);
            auto clearDataContext = wil::scope_exit([this]() noexcept
            {
                ClearDataContext();
            });

            return Probe.Content();
        }
        catch (...)
        {
            ClearDataContext();
            return nullptr;
        }
    }

private:
    void EnsureBinding()
    {
        if (!Probe)
        {
            Probe = winrt::ContentControl{};
        }

        if (BoundPath == SortMemberPath)
        {
            return;
        }

        ClearDataContext();
        Probe.ClearValue(winrt::ContentControl::ContentProperty());

        winrt::Binding binding;
        binding.Path(winrt::PropertyPath{ SortMemberPath });
        binding.Mode(winrt::BindingMode::OneTime);
        winrt::BindingOperations::SetBinding(
            Probe,
            winrt::ContentControl::ContentProperty(),
            binding);
        BoundPath = SortMemberPath;
    }

    // Never hold the last row item alive through the probe once the key has been read.
    void ClearDataContext() noexcept
    {
        try
        {
            if (Probe)
            {
                Probe.DataContext(nullptr);
            }
        }
        catch (...)
        {
        }
    }

    winrt::hstring SortMemberPath;
    winrt::hstring BoundPath;
    winrt::ContentControl Probe{ nullptr };
};
