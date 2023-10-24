// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Transform.h"

class WinRTExpressionConversionContext;

class CSkewTransform final : public CTransform
{
private:
    CSkewTransform(_In_ CCoreServices *pCore)
        : CTransform(pCore)
        , m_isCenterXAnimationDirty(false)
        , m_isCenterYAnimationDirty(false)
        , m_isAngleXAnimationDirty(false)
        , m_isAngleYAnimationDirty(false)
    {}

protected:
    CSkewTransform(_In_ const CSkewTransform& original, _Out_ HRESULT& hr)
        : CTransform(original, hr)
        , m_isCenterXAnimationDirty(false)
        , m_isCenterYAnimationDirty(false)
        , m_isAngleXAnimationDirty(false)
        , m_isAngleYAnimationDirty(false)
    {
        m_ptCenter = original.m_ptCenter;
        m_eAngleX  = original.m_eAngleX;
        m_eAngleY  = original.m_eAngleY;
    }

public:
#if defined(__XAML_UNITTESTS__)
    CSkewTransform()  // !!! FOR UNIT TESTING ONLY !!!
        : CSkewTransform(nullptr)
    {}
#endif

    DECLARE_CREATE(CSkewTransform);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CSkewTransform>::Index;
    }

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    DECLARE_SHAREABLEDEPENDENCYOBJECT_CLONE(CSkewTransform);

    void GetTransform(_Out_ CMILMatrix *pMatrix) override;

    void ReleaseDCompResources() final;

    void MakeWinRTExpression(
        _Inout_ WinRTExpressionConversionContext* pWinRTContext,
        _Inout_opt_ WUComp::ICompositionPropertySet* pTransformGroupPS = nullptr
        ) override;

    void ClearWUCExpression() override;

    XPOINTF m_ptCenter  = {};
    XFLOAT m_eAngleX    = 0.0f;
    XFLOAT m_eAngleY    = 0.0f;

    bool m_isCenterXAnimationDirty : 1;
    bool m_isCenterYAnimationDirty : 1;
    bool m_isAngleXAnimationDirty : 1;
    bool m_isAngleYAnimationDirty : 1;
};
