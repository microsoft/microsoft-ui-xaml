// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MultiParentShareableDependencyObject.h"
#include "DependencyObjectTraits.h"
#include "DependencyObjectTraits.g.h"

// Base transform class.  This class can not be directly created.
class CGeneralTransform : public CMultiParentShareableDependencyObject
{
protected:
    CGeneralTransform(_In_ CCoreServices *pCore)
        : CMultiParentShareableDependencyObject(pCore)
    {}

    CGeneralTransform(_In_ const CGeneralTransform& original, _Out_ HRESULT& hr)
        : CMultiParentShareableDependencyObject(original, hr)
    {}

public:
    DECLARE_CREATE(CGeneralTransform);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CGeneralTransform>::Index;
    }

    _Check_return_ HRESULT static TransformXY(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult);

    // Transform a rect.  Returns a bounding box if the rect is transformed
    _Check_return_ HRESULT TransformRect(_In_ const XRECTF& source, _Out_ XRECTF* pTransformedRect);

    virtual _Check_return_ HRESULT TransformPoints(
        _In_reads_(cPoints) XPOINTF *pptOriginal,
        _Inout_updates_(cPoints) XPOINTF *pptTransformed,
        XUINT32 cPoints = 1
        )
    {
        UNREFERENCED_PARAMETER(pptOriginal);
        UNREFERENCED_PARAMETER(pptTransformed);
        UNREFERENCED_PARAMETER(cPoints);
        RRETURN(E_NOTIMPL);
    }
};
