// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Transform.h"

class WinRTExpressionConversionContext;

class CTranslateTransform final : public CTransform
{
private:
    CTranslateTransform(_In_ CCoreServices *pCore)
        : CTransform(pCore)
        , m_isXAnimationDirty(false)
        , m_isYAnimationDirty(false)
    {}

protected:
    CTranslateTransform(_In_ const CTranslateTransform& original, _Out_ HRESULT& hr)
        : CTransform(original, hr)
        , m_isXAnimationDirty(false)
        , m_isYAnimationDirty(false)
    {
        m_eX = original.m_eX;
        m_eY = original.m_eY;
    }

public:
#if defined(__XAML_UNITTESTS__)
    CTranslateTransform()  // !!! FOR UNIT TESTING ONLY !!!
        : CTranslateTransform(nullptr)
    {}
#endif

    DECLARE_CREATE(CTranslateTransform);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTranslateTransform>::Index;
    }

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    DECLARE_SHAREABLEDEPENDENCYOBJECT_CLONE(CTranslateTransform);

    void GetTransform(_Out_ CMILMatrix *pMatrix) override;

    void ReleaseDCompResources() final;

    void MakeWinRTExpression(
        _Inout_ WinRTExpressionConversionContext* pWinRTContext,
        _Inout_opt_ WUComp::ICompositionPropertySet* pTransformGroupPS = nullptr
        ) override;

    void ClearWUCExpression() override;

    XFLOAT m_eX = 0.0f;
    XFLOAT m_eY = 0.0f;

    bool m_isXAnimationDirty : 1;
    bool m_isYAnimationDirty : 1;
};
