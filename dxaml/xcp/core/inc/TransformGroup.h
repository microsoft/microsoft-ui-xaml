// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Transform.h"
#include "Matrix.h"

class WinRTExpressionConversionContext;

// A group of 2D transforms compounded
class CTransformGroup final : public CTransform
{
private:
    CTransformGroup(_In_ CCoreServices *pCore)
        : CTransform(pCore)
    {
        m_groupMatrix.SetToIdentity();
    }

    _Check_return_ HRESULT FromString(
        _In_ CREATEPARAMETERS *pCreate
        );

    void UpdateCachedTransform();

protected:
    CTransformGroup(_In_ const CTransformGroup& original, _Out_ HRESULT& hr);

public:
#if defined(__XAML_UNITTESTS__)
    CTransformGroup()  // !!! FOR UNIT TESTING ONLY !!!
        : CTransformGroup(nullptr)
    {}
#endif

    ~CTransformGroup() override;

    DECLARE_CREATE_WITH_TYPECONVERTER(CTransformGroup);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTransformGroup>::Index;
    }

    DECLARE_SHAREABLEDEPENDENCYOBJECT_CLONE(CTransformGroup);

    void GetTransform(_Out_ CMILMatrix *pMatrix) override
    {
        if (m_fNWTransformsDirty)
        {
            UpdateCachedTransform();
        }
        memcpy(pMatrix, &m_groupMatrix, sizeof(CMILMatrix));
    }

    _Check_return_ HRESULT static GetTransformValue(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _In_reads_(cArgs) CValue *pArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult
        );

    static void NWSetTransformsDirty(
        _In_ CDependencyObject *pTarget,
        DirtyFlags flags
        );

    void MakeWinRTExpression(
        _Inout_ WinRTExpressionConversionContext* pWinRTContext,
        _Inout_opt_ WUComp::ICompositionPropertySet* pTransformGroupPS = nullptr
        ) override;

    void ClearWUCExpression() override;

    void NWPropagateDirtyFlag(DirtyFlags flags) override;

public:
    CTransformCollection *m_pChild  = nullptr;

private:
    // transform starts as identity so it's not dirty
    CMILMatrix m_groupMatrix;
    XUINT32 m_uiCacheUpdatedFrame   = 0;
    bool m_fNWTransformsDirty = false;
};
