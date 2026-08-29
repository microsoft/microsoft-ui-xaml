// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MultiParentShareableDependencyObject.h"
#include "RenderParams.h"
#include "CMatrix.h"
#include <fwd/Windows.UI.Composition.h>

#include <microsoft.ui.composition.h>

class WinRTExpressionConversionContext;

class CTransform3D : public CMultiParentShareableDependencyObject
{
public:
#if defined(__XAML_UNITTESTS__)
    CTransform3D()  // !!! FOR UNIT TESTING ONLY !!!
        : CTransform3D(nullptr)
    {}
#endif

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTransform3D>::Index;
    }

    virtual void UpdateTransformMatrix(
        float elementWidth,
        float elementHeight
        ) = 0;

    virtual bool IsRenderedTransform2D() const = 0;

    virtual bool HasDepth() = 0;

    void SetDCompResourceDirty() override;

    static void NWSetRenderDirty(
        _In_ CDependencyObject* pTarget,
        DirtyFlags flags
        );

    void ReleaseDCompResources() override;

    const CMILMatrix4x4& GetTransformMatrix() const;

    virtual void MakeWinRTExpression(
        _Inout_ WinRTExpressionConversionContext* pWinRTContext,
        float elementWidth,
        float elementHeight
        ) = 0;

    WUComp::IExpressionAnimation* GetWinRTExpression()
    {
        return m_spWinRTExpression.Get();
    }

    virtual void ClearWUCExpression();

    void EnsureWUCAnimationStarted(_Inout_ WinRTExpressionConversionContext* context) override;

protected:
    CTransform3D(_In_ CCoreServices *pCore)
        : CMultiParentShareableDependencyObject(pCore)
    {}

    ~CTransform3D() override;

protected:
    // TODO_WinRT: We should store the transform a single (IUnknown) pointer
    //             since only one or the other is ever used with a given transform.
    bool m_isWinRTExpressionDirty = true;
    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> m_spWinRTExpression;

    CMILMatrix4x4 m_matTransform3D = CMILMatrix4x4(true);

    float m_elementWidth    = 0.0f;
    float m_elementHeight   = 0.0f;
};
