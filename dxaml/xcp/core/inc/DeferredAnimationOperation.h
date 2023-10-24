// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// This is a wrapper object used to hold animation operations
// that we don't want to perform immediately. Used by Animations
// and VSM Setters for modifying the animated values of custom
// properties (since this can result in a call out to app code
// [note: it's not clear that this is actually a meaningful
// distinction anyway, because the call out to app code is going to
// happen sooner or later, but legacy...]). VSM Setters also use this
// mechanism to asynchronously modify a property's animated value,
// since it will be deferred to the next tick.

class CDeferredAnimationOperation final
{
public:
    enum class DeferredOperation
    {
        Set,
        Clear
    };

public:
    CDeferredAnimationOperation(
        _In_ std::pair<xref_ptr<CDependencyObject>, const KnownPropertyIndex> target,
        _In_ CValue& value,
        _In_ bool targetHasPeggedPeer,
        _In_ DeferredOperation operation,
        _In_ xref_ptr<CDependencyObject> sourceSetter);
    CDeferredAnimationOperation(CDeferredAnimationOperation&& other) = default;
    ~CDeferredAnimationOperation();

    _Check_return_ HRESULT Execute();
    const auto& Target() const { return m_target; }

private:
    _Check_return_ HRESULT SetAnimatedValue();
    _Check_return_ HRESULT ClearAnimatedValue();

    std::pair<xref_ptr<CDependencyObject>, const KnownPropertyIndex> m_target;
    xref_ptr<CDependencyObject> m_sourceSetter;
    CValue m_vValue;
    DeferredOperation m_operation;
    bool m_targetHasPeggedPeer;
};