// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Transform.h"

class WinRTExpressionConversionContext;

class CRotateTransform final : public CTransform
{
private:
    CRotateTransform(_In_ CCoreServices *pCore)
        : CTransform(pCore)
        , m_isCenterXAnimationDirty(false)
        , m_isCenterYAnimationDirty(false)
        , m_isAngleAnimationDirty(false)
    {}

protected:
    // CNoParentShareableDependencyObject overrides
    CRotateTransform(_In_ const CRotateTransform& original, _Out_ HRESULT& hr)
        : CTransform(original, hr)
        , m_isCenterXAnimationDirty(false)
        , m_isCenterYAnimationDirty(false)
        , m_isAngleAnimationDirty(false)
    {
        m_ptCenter = original.m_ptCenter;
        m_eAngle   = original.m_eAngle;
    }

public:
#if defined(__XAML_UNITTESTS__)
    CRotateTransform()  // !!! FOR UNIT TESTING ONLY !!!
        : CRotateTransform(nullptr)
    {}
#endif

    DECLARE_CREATE(CRotateTransform);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CRotateTransform>::Index;
    }

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    DECLARE_SHAREABLEDEPENDENCYOBJECT_CLONE(CRotateTransform);

    void GetTransform(_Out_ CMILMatrix *pMatrix) override;

    void ReleaseDCompResources() final;

    void MakeWinRTExpression(
        _Inout_ WinRTExpressionConversionContext* pWinRTContext,
        _Inout_opt_ WUComp::ICompositionPropertySet* pTransformGroupPS = nullptr
        ) override;

    void ClearWUCExpression() override;

    XPOINTF m_ptCenter  = {};
    XFLOAT m_eAngle     = 0.0f;

    bool m_isCenterXAnimationDirty : 1;
    bool m_isCenterYAnimationDirty : 1;
    bool m_isAngleAnimationDirty : 1;
};
