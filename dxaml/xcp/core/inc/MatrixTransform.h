// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Transform.h"
#include "Matrix.h"

class WinRTExpressionConversionContext;
class CMatrix;

class CMatrixTransform final : public CTransform
{
private:
    CMatrixTransform(_In_ CCoreServices *pCore)
        : CTransform(pCore)
    {}

    _Check_return_ HRESULT FromString(
         _In_ CREATEPARAMETERS *pCreate);

protected:
    CMatrixTransform(_In_ const CMatrixTransform& original, _Out_ HRESULT& hr);

public:
#if defined(__XAML_UNITTESTS__)
    CMatrixTransform()  // !!! FOR UNIT TESTING ONLY !!!
        : CMatrixTransform(nullptr)
    {}
#endif

    ~CMatrixTransform() override;

    DECLARE_CREATE_WITH_TYPECONVERTER(CMatrixTransform);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CMatrixTransform>::Index;
    }

    DECLARE_SHAREABLEDEPENDENCYOBJECT_CLONE(CMatrixTransform);

    void GetTransform(_Out_ CMILMatrix *pMatrix) override;

    void MakeWinRTExpression(
        _Inout_ WinRTExpressionConversionContext* pWinRTContext,
        _Inout_opt_ WUComp::ICompositionPropertySet* pTransformGroupPS = nullptr
        ) override;

    void ReleaseDCompResources() final;

    void SetMatrix(_In_ const CMILMatrix& matrix);

public:
    // TODO: MERGE: Why is CMatrix a DO?  Seems unnecessarily heavy-weight.
    CMatrix *m_pMatrix = nullptr;
};
