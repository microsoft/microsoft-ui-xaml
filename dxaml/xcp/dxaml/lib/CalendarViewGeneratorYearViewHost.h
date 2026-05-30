// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CalendarViewGeneratorHost.h"

namespace DirectUI
{

    class CalendarViewBaseItem;

    class CalendarViewGeneratorYearViewHost
        : public CalendarViewGeneratorHost
    {
    protected:
        _Check_return_ HRESULT GetContainer(
            _In_ IInspectable* pItem,
            _In_opt_ xaml::IDependencyObject* pRecycledContainer,
            _Outptr_ CalendarViewBaseItem** ppContainer) override;


    public:
        _Check_return_ IFACEMETHOD(PrepareItemContainer)(
            _In_ xaml::IDependencyObject* pContainer,
            _In_ IInspectable* pItem) override;


        _Check_return_ HRESULT GetPossibleItemStrings(_Outptr_ const std::vector<wrl_wrappers::HString>** ppStrings) override;

        _Check_return_ HRESULT GetIsFirstItemInScope(_In_ int index, _Out_ bool* pIsFirstItemInScope) override;

        int GetMaximumScopeSize() override
        {
            return 13; // a year has 13 months in maximum.
        }

        INT64 GetAverageTicksPerUnit() override;

        _Check_return_ HRESULT GetUnit(_Out_ int* pValue) override;
        _Check_return_ HRESULT SetUnit(_In_ int value) override;
        _Check_return_ HRESULT AddUnits(_In_ int value) override;
        _Check_return_ HRESULT AddScopes(_In_ int value) override;
        _Check_return_ HRESULT GetFirstUnitInThisScope(_Out_ int* pValue) override;
        _Check_return_ HRESULT GetLastUnitInThisScope(_Out_ int* pValue) override;
        _Check_return_ HRESULT OnScopeChanged() override;

        _Check_return_ HRESULT UpdateLabel(_In_ CalendarViewBaseItem* pItem, _In_ bool isLabelVisible) override;

        _Check_return_ HRESULT CompareDate(_In_ wf::DateTime lhs, _In_ wf::DateTime rhs, _Out_ int* pResult) override;
    };


}
