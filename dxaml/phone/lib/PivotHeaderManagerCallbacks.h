// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <PivotCommon.h>

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{

struct IPivotHeaderManagerCallbacks
{
    virtual bool IsHeaderItemsCarouselEnabled() const = 0;
    virtual unsigned GetPivotPanelMultiplier() const = 0;
    _Check_return_ virtual HRESULT OnHeaderItemTapped(_In_ INT32 idx, _In_ bool shouldPlaySound) = 0;
    _Check_return_ virtual HRESULT GetHeaderTemplate(_Outptr_result_maybenull_ xaml::IDataTemplate** ppTemplate) = 0;
    _Check_return_ virtual HRESULT OnHeaderPanelMeasure(float viewportSize) = 0;
};

struct IPivotHeaderManagerPanelEvents
{
    _Check_return_ virtual HRESULT HeaderPanelMeasureEvent(float viewportSize) = 0;
    _Check_return_ virtual HRESULT HeaderPanelSetLteOffsetEvent(_In_ DOUBLE primaryHorizontalOffset, _In_ DOUBLE ghostHorizontalOffset, _In_ DOUBLE verticalOffset) = 0;
};

struct IPivotHeaderManagerItemEvents
{
    virtual bool GetIsLocked() const= 0;
    _Check_return_ virtual HRESULT PivotHeaderItemTapped(_In_ xaml_primitives::IPivotHeaderItem* pItem) = 0;
    _Check_return_ virtual HRESULT VsmUnlockedStateChangeCompleteEvent() = 0;
};

} } } } XAML_ABI_NAMESPACE_END
