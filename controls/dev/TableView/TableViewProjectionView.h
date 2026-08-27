// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>

#include "..\Collections\VectorChangedEventArgs.h"
#include "..\TabularShaping\TabularRevoke.h"

class TableViewProjectionView :
    public winrt::implements<
        TableViewProjectionView,
        winrt::IObservableVector<winrt::IInspectable>,
        winrt::Windows::Foundation::Collections::IVector<winrt::IInspectable>,
        winrt::Windows::Foundation::Collections::IIterable<winrt::IInspectable>>
{
public:
    ~TableViewProjectionView()
    {
        // Never let a revoke escape a destructor: this view is released with its owning
        // TableViewSource, which the CLR can drop on the finalizer thread, and an escaping
        // exception there terminates the process.
        TabularShapingHelpers::SafeRevoke(m_innerVectorChangedRevoker);
    }

    void SetInner(winrt::IObservableVector<winrt::IInspectable> const& inner)
    {
        if (SameObject(m_inner, inner))
        {
            return;
        }

        m_innerVectorChangedRevoker.revoke();
        m_inner = inner;

        if (m_inner)
        {
            auto weakThis = get_weak();
            m_innerVectorChangedRevoker = m_inner.VectorChanged(winrt::auto_revoke,
                [weakThis](
                    winrt::IObservableVector<winrt::IInspectable> const&,
                    winrt::IVectorChangedEventArgs const& args)
                {
                    if (auto strongThis = weakThis.get())
                    {
                        strongThis->m_vectorChanged(*strongThis, args);
                    }
                });
        }

        m_vectorChanged(*this, winrt::make<VectorChangedEventArgs>(winrt::CollectionChange::Reset, 0));
    }

    winrt::event_token VectorChanged(winrt::VectorChangedEventHandler<winrt::IInspectable> const& handler)
    {
        return m_vectorChanged.add(handler);
    }

    void VectorChanged(winrt::event_token const& token)
    {
        m_vectorChanged.remove(token);
    }

    winrt::IInspectable GetAt(uint32_t const index)
    {
        if (m_inner)
        {
            return m_inner.GetAt(index);
        }

        winrt::throw_hresult(E_BOUNDS);
    }

    uint32_t Size()
    {
        return m_inner ? m_inner.Size() : 0;
    }

    winrt::IVectorView<winrt::IInspectable> GetView()
    {
        return m_inner ? m_inner.GetView() : EmptyVector().GetView();
    }

    bool IndexOf(winrt::IInspectable const& value, uint32_t& index)
    {
        if (m_inner)
        {
            return m_inner.IndexOf(value, index);
        }

        index = 0;
        return false;
    }

    uint32_t GetMany(uint32_t const startIndex, winrt::array_view<winrt::IInspectable> values)
    {
        return m_inner ? m_inner.GetMany(startIndex, values) : 0;
    }

    void SetAt(uint32_t const, winrt::IInspectable const&)
    {
        ThrowReadOnly();
    }

    void InsertAt(uint32_t const, winrt::IInspectable const&)
    {
        ThrowReadOnly();
    }

    void RemoveAt(uint32_t const)
    {
        ThrowReadOnly();
    }

    void Append(winrt::IInspectable const&)
    {
        ThrowReadOnly();
    }

    void RemoveAtEnd()
    {
        ThrowReadOnly();
    }

    void Clear()
    {
        ThrowReadOnly();
    }

    void ReplaceAll(winrt::array_view<winrt::IInspectable const>)
    {
        ThrowReadOnly();
    }

    winrt::IIterator<winrt::IInspectable> First()
    {
        return m_inner ? m_inner.First() : EmptyVector().First();
    }

private:
    static winrt::IObservableVector<winrt::IInspectable> EmptyVector()
    {
        return winrt::single_threaded_observable_vector<winrt::IInspectable>();
    }

    static bool SameObject(
        winrt::IObservableVector<winrt::IInspectable> const& left,
        winrt::IObservableVector<winrt::IInspectable> const& right)
    {
        if (!left || !right)
        {
            return !left && !right;
        }

        return left.as<winrt::IUnknown>() == right.as<winrt::IUnknown>();
    }

    [[noreturn]] static void ThrowReadOnly()
    {
        throw winrt::hresult_illegal_method_call(
            L"TableViewSource.View is a read-only observable projection. "
            L"Use TableViewSource.From/Filter/GroupBy/Sort/Clear* to change the projection.");
    }

    winrt::IObservableVector<winrt::IInspectable> m_inner{ nullptr };
    winrt::IObservableVector<winrt::IInspectable>::VectorChanged_revoker m_innerVectorChangedRevoker{};
    event<winrt::VectorChangedEventHandler<winrt::IInspectable>> m_vectorChanged;
};
