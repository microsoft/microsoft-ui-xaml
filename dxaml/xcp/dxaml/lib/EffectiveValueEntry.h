// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlOM.WinUI.h"
#include "ModifiedValue.h"
#include "ReferenceTrackerInterfaces.h"
#include "TrackerPtr.h"

namespace Theming {
    enum class Theme : uint8_t;
}

namespace DirectUI
{
    // Class only used for expressions.

    class EffectiveValueEntry final : public PReferenceTrackerInternal
    {
    public:
        ~EffectiveValueEntry();

        ctl::ComPtr<IInspectable> GetBaseValue() const;

        _Check_return_ HRESULT SetBaseValue(
            _In_ IInspectable* pValue,
            _In_ BaseValueSource baseValueSource);

        void ClearBaseValue();

        ctl::ComPtr<IInspectable> GetEffectiveValue() const;

        _Check_return_ HRESULT GetLocalValue(
            _Out_ IInspectable** ppValue);

        _Check_return_ HRESULT SetExpressionValue(
            _In_ IInspectable* pValue);

        void ClearExpressionValue();

        BaseValueSource GetBaseValueSource() const
        {
            return static_cast<BaseValueSource>(m_fullValueSource & fvsBaseValueSourceMask);
        }

        bool IsPropertyLocal() const
        {
            return (GetBaseValueSource() == BaseValueSourceLocal);
        }

        bool HasBaseValue() const
        {
            return (m_fullValueSource & fvsBaseValueSourceMask) != 0;
        }

        bool HasModifiers() const
        {
            return (m_fullValueSource & fvsModifiersMask) != 0;
        }

        bool IsExpression() const
        {
            return (m_fullValueSource & fvsIsExpression) != 0;
        }

        // PReferenceTrackerInternal
        bool ReferenceTrackerWalk(
            _In_ EReferenceTrackerWalkType walkType,
            _In_ bool fIsRoot = false) override;

        _Check_return_ HRESULT NotifyThemeChanged(
            _In_ Theming::Theme theme,
            _In_ bool forceRefresh,
            _Out_ bool& valueChanged);

    private:
        // Base Value.
        TrackerPtr<IInspectable> m_BaseValue;

        // Expression Value
        TrackerPtr<IInspectable> m_ExpressionValue;

        // Value Source
        XUINT32 m_fullValueSource = 0;
    };
}