// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Transform.h"

class WinRTExpressionConversionContext;

class CScaleTransform final : public CTransform
{
private:
    CScaleTransform(_In_ CCoreServices *pCore)
        : CTransform(pCore)
        , m_isCenterXAnimationDirty(false)
        , m_isCenterYAnimationDirty(false)
        , m_isScaleXAnimationDirty(false)
        , m_isScaleYAnimationDirty(false)
    {}

protected:
    CScaleTransform(_In_ const CScaleTransform& original, _Out_ HRESULT& hr)
        : CTransform(original,hr)
        , m_isCenterXAnimationDirty(false)
        , m_isCenterYAnimationDirty(false)
        , m_isScaleXAnimationDirty(false)
        , m_isScaleYAnimationDirty(false)
    {
        m_ptCenter = original.m_ptCenter;
        m_eScaleX  = original.m_eScaleX;
        m_eScaleY  = original.m_eScaleY;
    }

public:
#if defined(__XAML_UNITTESTS__)
    CScaleTransform()  // !!! FOR UNIT TESTING ONLY !!!
        : CScaleTransform(nullptr)
    {}
#endif

    DECLARE_CREATE(CScaleTransform);

    DECLARE_SHAREABLEDEPENDENCYOBJECT_CLONE(CScaleTransform);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CScaleTransform>::Index;
    }

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    void GetTransform(_Out_ CMILMatrix *pMatrix) override;

    void ReleaseDCompResources() final;

    void MakeWinRTExpression(
        _Inout_ WinRTExpressionConversionContext* pWinRTContext,
        _Inout_opt_ WUComp::ICompositionPropertySet* pTransformGroupPS = nullptr
        ) override;

    void ClearWUCExpression() override;

    XPOINTF m_ptCenter  = {};
    XFLOAT m_eScaleX    = 1.0f;
    XFLOAT m_eScaleY    = 1.0f;

    bool m_isCenterXAnimationDirty : 1;
    bool m_isCenterYAnimationDirty : 1;
    bool m_isScaleXAnimationDirty : 1;
    bool m_isScaleYAnimationDirty : 1;
};
