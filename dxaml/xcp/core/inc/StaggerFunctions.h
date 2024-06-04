// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CStaggerFunctionBase : public CNoParentShareableDependencyObject
{
public:
    CStaggerFunctionBase(_In_ CCoreServices *pCore)
        : CNoParentShareableDependencyObject(pCore)
    {}

    // Creation function
    DECLARE_CREATE(CStaggerFunctionBase);

    KnownTypeIndex GetTypeIndex() const override
    { 
        return DependencyObjectTraits<CStaggerFunctionBase>::Index; 
    }

    // Should the managed peer tree be updated during SetParent?
    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }

    virtual _Check_return_ HRESULT GetTransitionDelays(
        _In_ XUINT32 cElements,
        _In_reads_(cElements) CUIElement** ppElements,
        _In_reads_(cElements) XRECTF *pBounds, 
        _Out_writes_(cElements) XFLOAT* pDelays)
    {
        return E_NOTIMPL;
    }

    static _Check_return_ HRESULT GetTransitionDelayValues(
        _In_ CDependencyObject* pStaggerFunction,
        _In_ XUINT32 cElements,
        _In_reads_(cElements) CUIElement** ppElements,
        _In_reads_(cElements) XRECTF *pBounds, 
        _Out_writes_(cElements) XFLOAT* pDelays);
};
