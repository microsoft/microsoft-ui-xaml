// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "CDependencyObject.h"

#pragma once

class CVisualStateGroup;
class CVisualState;
class CVisualTransition;
class CControl;
class CDependencyObject;
class CFrameworkElement;
class CStoryboard;

class CVisualStateManager final
    : public CDependencyObject
{
public:
    static _Check_return_ HRESULT GoToState(
            _In_ CDependencyObject *pObject,
            _In_z_ const WCHAR *pStateName,
            _In_ CVisualState* pVisualState,
            _In_ CVisualStateGroup* pVisualStateGroup,
            _In_ bool useTransitions,
            _Out_opt_ bool* pSuccess = nullptr);

    static _Check_return_ HRESULT GoToState(
            _In_ CDependencyObject *pObject,
            _In_z_ const WCHAR *pStateName,
            _In_ bool useTransitions,
            _Out_opt_ bool* pSuccess = nullptr);

    DECLARE_CREATE(CVisualStateManager);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CVisualStateManager>::Index;
    }

    static HRESULT _Check_return_ FindVisualState(
        _In_ CControl* pControl,
        _In_opt_ CFrameworkElement* pImplControl,
        _In_z_ const WCHAR* pStateName,
        _Outptr_opt_result_maybenull_ CVisualStateGroup** ppStateGroup,
        _Outptr_opt_result_maybenull_ CVisualState** ppState,
        _Out_ bool *pFound);

private:
    CVisualStateManager(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

public:
    static _Check_return_ HRESULT GetTransition(
        _In_ CVisualStateGroup *pVisualStateGroup,
        _In_opt_ CVisualState *pFrom, _In_ CVisualState *pTo,
        _Outptr_result_maybenull_ CVisualTransition **ppVisualTransition);
};
