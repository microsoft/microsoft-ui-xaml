// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <microsoft.ui.composition.h>

class CTimeManager;

enum class KnownPropertyIndex : UINT16;

class WinRTExpressionConversionContext
{
public:
    WinRTExpressionConversionContext() = default;
    explicit WinRTExpressionConversionContext(_In_ WUComp::ICompositor *pCompositor);
    explicit WinRTExpressionConversionContext(_In_ WinRTExpressionConversionContext *pOtherContext);

    void CreateExpression(
        _In_opt_ const wchar_t* expressionString,
        _Outptr_ WUComp::IExpressionAnimation** expression,
        _Outptr_ WUComp::ICompositionPropertySet** propertySet
        );

    // Initialize the specified property in a property set with a static value
    // Used when creating a new expression. There might be expired WUC animations that we don't want to attach again, in which
    // case the static value set here will be referenced by the expression string.
    static void InitializeExpression(
        _In_ WUComp::ICompositionPropertySet* propertySet,
        _In_ LPCWSTR propertyName,
        float staticValue);

    // Update specified property in expression with static or animated scalar value
    void UpdateExpression(
        _In_ WUComp::IExpressionAnimation* expression,
        _In_ LPCWSTR propertyName,
        float staticValue,
        bool isAnimationDirty,
        _In_opt_ WUComp::ICompositionAnimation* animation,
        _In_ CDependencyObject* targetObject,
        _In_ KnownPropertyIndex targetPropertyIndex,
        _In_opt_ CTimeManager* timeManager
        );

    void ConnectSubExpression(
        bool useMatrix4x4,
        _In_ WUComp::IExpressionAnimation* overallExpression,
        _In_opt_ WUComp::IExpressionAnimation* subExpression,
        _In_ const wchar_t* propertyName
        );

    void UpdateCompositeTransformComponent(
        bool useMatrix4x4,
        _In_ WUComp::IExpressionAnimation* overallExpression,
        _In_opt_ WUComp::IExpressionAnimation* componentExpression,
        _In_ const wchar_t* propertyName
        );

    void CreateMatrixTransform(
        bool useMatrix4x4,
        _In_opt_ const CMILMatrix *pMatrix,
        _Outptr_ WUComp::IExpressionAnimation** ppExpression
        );

    void CreateMatrixTransform3D(
        _In_opt_ const CMILMatrix4x4 *pMatrix,
        _Outptr_ WUComp::IExpressionAnimation** ppExpression
        );

    void UpdateTransformGroupExpression(
        int expressionCount,
        _Inout_ WUComp::IExpressionAnimation* pExpression
        );

    // Helper to build up TransformGroup expressions
    void AddExpressionToTransformGroupPropertySet(
        _In_ WUComp::IExpressionAnimation *pExpression,
        _In_ WUComp::ICompositionPropertySet *pTransformGroupPropertySet
        );

    // TODO: WinComp: Actions requiring compositor should be handled by WinRTConversionContext itself - it should not be handed out.
    WUComp::ICompositor* GetCompositorNoRef()
    {
        return m_spCompositor.Get();
    }

    unsigned int GetExpressionCount() const { return m_expressionCount; }

    static constexpr wfn::Matrix3x2 c_identityMatrix3x2 = { 1, 0, 0, 1, 0, 0 };
    static constexpr wfn::Matrix4x4 c_identityMatrix4x4 = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

private:
    void InitializeCompositeTransformPS();

    Microsoft::WRL::ComPtr<WUComp::ICompositor> m_spCompositor;

    // Index of last transform in TransformGroup added to the PS
    int m_expressionCount;
};
