// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PivotHeaderManager.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls { namespace Primitives
{

class PivotPanel :
    public PivotPanelGenerated
{

public:
    PivotPanel();

    _Check_return_ HRESULT SetParentPivot(_In_opt_ xaml_controls::IPivot* pivot);

    _Check_return_ HRESULT SetSnapPointOffsetInPixels(_In_ FLOAT offset);
    _Check_return_ HRESULT SetSnapPointIntervalInPixels(_In_ FLOAT size);
    _Check_return_ HRESULT SetSectionWidth(_In_ FLOAT availableWidth);
    _Check_return_ HRESULT InvalidateMeasure();

private:
    ~PivotPanel();

    _Check_return_ HRESULT InitializeImpl() override;

public:
    // IFrameworkElementOverrides methods
    _Check_return_ HRESULT ArrangeOverrideImpl(
        _In_ wf::Size finalSize,
        _Out_ wf::Size* returnValue) override;
    _Check_return_ HRESULT MeasureOverrideImpl(
        _In_ wf::Size finalSize,
        _Out_ wf::Size* returnValue) override;

     // Implementation of IScrollSnapPointsInfo
    _Check_return_ HRESULT get_AreHorizontalSnapPointsRegularImpl(_Out_ boolean* pValue) override;
    _Check_return_ HRESULT get_AreVerticalSnapPointsRegularImpl(_Out_ boolean* pValue) override;

    _Check_return_ HRESULT GetIrregularSnapPointsImpl(
        _In_ xaml_controls::Orientation orientation,
        _In_ xaml_primitives::SnapPointsAlignment alignment,
        _Outptr_ wfc::IVectorView<FLOAT>** returnValue);

    _Check_return_ HRESULT GetRegularSnapPointsImpl(
        _In_ xaml_controls::Orientation orientation,
        _In_ xaml_primitives::SnapPointsAlignment alignment,
        _Out_ FLOAT* offset,
        _Out_ FLOAT* returnValue);

    _Check_return_ HRESULT RaiseSnapPointsChangedEvents();

private:
    FLOAT m_availableWidth;
    FLOAT m_headerHeight;

    wrl::WeakRef m_parentPivotWeakRef;
};

ActivatableClass(PivotPanel);

} } } } } XAML_ABI_NAMESPACE_END
