// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "GeneralTransform.h"
#include "DirtyFlags.h"
#include <fwd/Windows.UI.Composition.h>

#include <microsoft.ui.composition.h>

class WinRTExpressionConversionContext;
class CMILMatrix;
class CTimeManager;

class CTransform : public CGeneralTransform
{
protected:
    CTransform(_In_ CCoreServices *pCore)
        : CGeneralTransform(pCore)
    {}

    CTransform(_In_ const CTransform& original, _Out_ HRESULT& hr)
        : CGeneralTransform(original, hr)
    {}

public:
#if defined(__XAML_UNITTESTS__)
    CTransform() // !!! FOR UNIT TESTING ONLY !!!
        : CTransform(nullptr)
    {}
#endif

    ~CTransform() override;

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTransform>::Index;
    }

    _Check_return_ HRESULT TransformPoints(
        _In_reads_(cPoints) XPOINTF *pptOriginal,
        _Inout_updates_(cPoints) XPOINTF *pptTransformed,
        XUINT32 cPoints = 1
        ) final;

    virtual void GetTransform(_Out_ CMILMatrix *pMatrix) = 0;

    void EnsureWUCAnimationStarted(_Inout_ WinRTExpressionConversionContext* context) override;

    virtual void MakeWinRTExpression(
        _Inout_ WinRTExpressionConversionContext* pWinRTContext,
        _Inout_opt_ WUComp::ICompositionPropertySet* pTransformGroupPS = nullptr
        ) = 0;

    void MakeWinRTExpressionWithOrigin(
        XFLOAT originX,
        XFLOAT originY,
        _Inout_ WinRTExpressionConversionContext* pWinRTContext,
        _Outptr_result_maybenull_ WUComp::IExpressionAnimation** ppExpression
        );

    virtual void ClearWUCExpression();

    _Check_return_ HRESULT Inverse(_Outptr_ CGeneralTransform **ppInverse);

    static _Check_return_ HRESULT TryTransform(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult
        );

    static float ClampAngle360(float originalAngle);

    static void BuildRotateMatrix(
        _Out_ CMILMatrix *pMatrix,
        float eAngle,
        XPOINTF ptCenter);

    static void BuildSkewMatrix(
        _Out_ CMILMatrix *pMatrix,
        float eAngleX,
        float eAngleY,
        XPOINTF ptCenter);

    WUComp::IExpressionAnimation* GetWinRTExpression()
    {
        return m_spWinRTExpression.Get();
    }

    void ReleaseDCompResources() override;
    void SetDCompResourceDirty() override;

    static void NWSetRenderDirty(
        _In_ CDependencyObject* pTarget,
        DirtyFlags flags
        );

public:
    // TODO: WinComp: We should store the transform a single (IUnknown) pointer
    //                since only or the other is ever used with a given transform.
    bool m_isWinRTExpressionDirty = true;
    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> m_spWinRTExpression;
};
