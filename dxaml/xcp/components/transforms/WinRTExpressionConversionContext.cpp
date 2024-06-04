// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ExpressionHelper.h"
#include "WinRTExpressionConversionContext.h"
#include <Windows.UI.Composition.h>
#include <string>
#include <TimeMgr.h>
#include <DCompTreeHost.h>

using namespace Microsoft::WRL::Wrappers;

WinRTExpressionConversionContext::WinRTExpressionConversionContext(_In_ WUComp::ICompositor *pCompositor)
    : m_spCompositor(pCompositor)
    , m_expressionCount(0)
{
}

WinRTExpressionConversionContext::WinRTExpressionConversionContext(_In_ WinRTExpressionConversionContext *pOtherContext)
    : m_spCompositor(pOtherContext->m_spCompositor)
    , m_expressionCount(0)
{
}

// If expressionString is provided, create an ExpressionAnimation using this string.
// Otherwise create an empty ExpressionAnimation.
void WinRTExpressionConversionContext::CreateExpression(
    _In_opt_ const wchar_t* expressionString,
    _Outptr_ WUComp::IExpressionAnimation** expression,
    _Outptr_ WUComp::ICompositionPropertySet** propertySet)
{
    wrl::ComPtr<WUComp::IExpressionAnimation> expr;
    wrl::ComPtr<WUComp::ICompositionAnimation> exprICA;
    wrl::ComPtr<WUComp::ICompositionObject> exprICO;
    wrl::ComPtr<WUComp::ICompositionPropertySet> expressionPropertySet;
    wrl::ComPtr<WUComp::ICompositionObject> psICO;

    if (expressionString != nullptr)
    {
        IFCFAILFAST(m_spCompositor->CreateExpressionAnimationWithExpression(HStringReference(expressionString).Get(), &expr));
    }
    else
    {
        IFCFAILFAST(m_spCompositor->CreateExpressionAnimation(&expr));
    }

    IFCFAILFAST(expr.As(&exprICA));
    IFCFAILFAST(expr.As(&exprICO));
    IFCFAILFAST(exprICO->get_Properties(&expressionPropertySet));
    IFCFAILFAST(expressionPropertySet.As(&psICO));
    IFCFAILFAST(exprICA->SetReferenceParameter(HStringReference(ExpressionHelper::sc_paramName_PropertySet).Get(), psICO.Get()));

    IFCFAILFAST(expr.CopyTo(expression));
    IFCFAILFAST(expressionPropertySet.CopyTo(propertySet));
}

/* static */ void WinRTExpressionConversionContext::InitializeExpression(
    _In_ WUComp::ICompositionPropertySet* propertySet,
    _In_ LPCWSTR propertyName,
    float staticValue)
{
    IFCFAILFAST(propertySet->InsertScalar(HStringReference(propertyName).Get(), staticValue));
}

void WinRTExpressionConversionContext::UpdateExpression(
    _In_ WUComp::IExpressionAnimation* expression,
    _In_ LPCWSTR propertyName,
    float staticValue,
    bool isAnimationDirty,
    _In_opt_ WUComp::ICompositionAnimation* animation,
    _In_opt_ CDependencyObject* targetObject,
    _In_ KnownPropertyIndex targetPropertyIndex,
    _In_opt_ CTimeManager* timeManager
    )
{
    wrl::ComPtr<WUComp::ICompositionObject> exprICO;
    wrl::ComPtr<WUComp::ICompositionPropertySet> propertySet;

    IFCFAILFAST(expression->QueryInterface(IID_PPV_ARGS(&exprICO)));
    IFCFAILFAST(exprICO->get_Properties(&propertySet));

    // Note: This method needs to no-op under certain conditions.
    // Suppose there is a TranslateTransform with X animated by some WUC animation and Y of 25. Then the app updates Y to be 50.
    // We end up in this method for the X property first. We're passed the WUC animation that's already set, plus a dirty flag
    // for it (it's clean). In that case we need to no-op. If we set a constant value, then we will incorrectly stomp over the
    // X animation that's already playing. If we set the animation again, under the current CTimeManager::StartWUCAnimation
    // infrastructure that would restart the animation from 0, which would also be incorrect.

    if (animation == nullptr)
    {
        IFCFAILFAST(propertySet->InsertScalar(HStringReference(propertyName).Get(), staticValue));
    }
    else if (isAnimationDirty)
    {
        wrl::ComPtr<WUComp::ICompositionObjectPartner> propertySetICOP;
        IFCFAILFAST(propertySet.As(&propertySetICOP));

        // No need to initialize the WUC property to a static value. That should have already been taken care of by
        // InitializeExpression after the expression was created.

        ASSERT(animation != nullptr);
        ASSERT(targetObject != nullptr);

        CTimeManager::StartWUCAnimation(
            m_spCompositor.Get(),
            propertySetICOP.Get(),
            propertyName,
            animation,
            targetObject,
            targetPropertyIndex,
            timeManager);
    }
}

void WinRTExpressionConversionContext::CreateMatrixTransform(
    bool useMatrix4x4,
    _In_opt_ const CMILMatrix *pMatrix,
    _Outptr_ WUComp::IExpressionAnimation** ppExpression
    )
{
    wrl::ComPtr<WUComp::IExpressionAnimation> spEA;
    wrl::ComPtr<WUComp::ICompositionPropertySet> propertySet;
    CreateExpression(useMatrix4x4 ? ExpressionHelper::sc_Expression_MatrixTransform4x4 : ExpressionHelper::sc_Expression_MatrixTransform, &spEA, &propertySet);

    // MatrixTransform is not animatable (this was also the case with legacy DComp animations).
    // TODO_WinRT: Consider generating a string with the values inlined for performance.

    // Set to pMatrix values if they are provided, otherwise set to identity
    InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_M11, pMatrix ? pMatrix->_11 : 1.0f);
    InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_M12, pMatrix ? pMatrix->_12 : 0.0f);
    InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_M21, pMatrix ? pMatrix->_21 : 0.0f);
    InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_M22, pMatrix ? pMatrix->_22 : 1.0f);
    InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_TX, pMatrix ? pMatrix->_31 : 0.0f);
    InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_TY, pMatrix ? pMatrix->_32 : 0.0f);

    IFCFAILFAST(spEA.CopyTo(ppExpression));
}

// Note this matrix is not animatable. Xaml usage of 3D Matrix Transform is limited to constant matrices so far.
void WinRTExpressionConversionContext::CreateMatrixTransform3D(
    _In_opt_ const CMILMatrix4x4 *pMatrix,
    _Outptr_ WUComp::IExpressionAnimation** ppExpression
    )
{
    wrl::ComPtr<WUComp::IExpressionAnimation> transform3D_EA;
    wrl::ComPtr<WUComp::ICompositionPropertySet> propertySet;

    CreateExpression(ExpressionHelper::sc_Expression_MatrixTransform3D, &transform3D_EA, &propertySet);

    wfn::Matrix4x4 wfnMatrix = c_identityMatrix4x4;
    if (pMatrix)
    {
        pMatrix->ToMatrix4x4(&wfnMatrix);
    }

    IFCFAILFAST(propertySet->InsertMatrix4x4(
        HStringReference(ExpressionHelper::sc_propertyName_MatrixTransform3D_Matrix).Get(),
        wfnMatrix));

    IFCFAILFAST(transform3D_EA.CopyTo(ppExpression));
}

void WinRTExpressionConversionContext::UpdateTransformGroupExpression(
    int expressionCount,
    _Inout_ WUComp::IExpressionAnimation* pExpression
    )
{
    std::wstring expression;
    wchar_t* expressionChars = nullptr;

    switch (expressionCount)
    {
        case 0:
        {
            IFCFAILFAST(E_UNEXPECTED);
            break;
        }
        case 1:
        {
            expressionChars = const_cast<wchar_t *>(ExpressionHelper::sc_Expression_TransformGroup1);
            break;
        }
        case 2:
        {
            expressionChars = const_cast<wchar_t *>(ExpressionHelper::sc_Expression_TransformGroup2);
            break;
        }
        case 3:
        {
            expressionChars = const_cast<wchar_t *>(ExpressionHelper::sc_Expression_TransformGroup3);
            break;
        }
        default:
        {
            // "PS.Expr"
            std::wstring componentName(ExpressionHelper::sc_paramName_PropertySet);
            componentName.append(L".");
            componentName.append(ExpressionHelper::sc_TransformGroupPropertyNameRoot);
            int i = 0;

            // PS.Expr0 * PS.Expr1 * PS.Expr2 * PS.Expr3 ...
            for (i = 0; i < expressionCount - 1; i++)
            {
                expression.append(componentName);
                expression.append(std::to_wstring(i));
                expression.append(L" * ");
            }

            expression.append(componentName);
            expression.append(std::to_wstring(i));
            expressionChars = const_cast<wchar_t *>(expression.c_str());
        }
    }

    IFCFAILFAST(pExpression->put_Expression(HStringReference(expressionChars).Get()));
}

void WinRTExpressionConversionContext::AddExpressionToTransformGroupPropertySet(
    _In_ WUComp::IExpressionAnimation *pExpression,
    _In_ WUComp::ICompositionPropertySet *pTransformGroupPropertySet
    )
{
    Microsoft::WRL::ComPtr<WUComp::ICompositionObject> spPropertySetAsCO;
    Microsoft::WRL::ComPtr<WUComp::ICompositionAnimation> spExpressionAsCompositionAnimation;

    // eg "Expr1"
    std::wstring targetProperty;
    targetProperty.append(ExpressionHelper::sc_TransformGroupPropertyNameRoot);
    targetProperty.append(std::to_wstring(m_expressionCount));

    // Set default to create property so it can then be animated
    IFCFAILFAST(pTransformGroupPropertySet->InsertMatrix3x2(HStringReference(targetProperty.c_str()).Get(), c_identityMatrix3x2));

    IFCFAILFAST(pTransformGroupPropertySet->QueryInterface(IID_PPV_ARGS(&spPropertySetAsCO)));
    IFCFAILFAST(pExpression->QueryInterface(IID_PPV_ARGS(&spExpressionAsCompositionAnimation)));
    IFCFAILFAST(spPropertySetAsCO->StartAnimation(HStringReference(targetProperty.c_str()).Get(), spExpressionAsCompositionAnimation.Get()));

    m_expressionCount++;
}

// Connects componentExpression to the specified property in overallExpression's "Properties" PS
// If subExpression is not provided, the property is set to Identity.
void WinRTExpressionConversionContext::ConnectSubExpression(
    bool useMatrix4x4,
    _In_ WUComp::IExpressionAnimation* overallExpression,
    _In_opt_ WUComp::IExpressionAnimation* subExpression,
    _In_ const wchar_t* propertyName
    )
{
    wrl::ComPtr<WUComp::ICompositionObject> overallCO;
    wrl::ComPtr<WUComp::ICompositionObject> propertySetCO;
    wrl::ComPtr<WUComp::ICompositionPropertySet> propertySet;

    IFCFAILFAST(overallExpression->QueryInterface(IID_PPV_ARGS(&overallCO)));
    IFCFAILFAST(overallCO->get_Properties(&propertySet));
    IFCFAILFAST(propertySet.As(&propertySetCO));

    if (useMatrix4x4)
    {
        IFCFAILFAST(propertySet->InsertMatrix4x4(HStringReference(propertyName).Get(), c_identityMatrix4x4));
    }
    else
    {
        IFCFAILFAST(propertySet->InsertMatrix3x2(HStringReference(propertyName).Get(), c_identityMatrix3x2));
    }

    if (subExpression)
    {
        wrl::ComPtr<WUComp::ICompositionAnimation> subExpressionCA;
        IFCFAILFAST(subExpression->QueryInterface(IID_PPV_ARGS(&subExpressionCA)));
        IFCFAILFAST(propertySetCO->StartAnimation(HStringReference(propertyName).Get(), subExpressionCA.Get()));
    }
}


// Set the specified CompositeTransform component (Scale/Skew/Rotate/Translate) using provided expression.
// If expression is not provided, the component is not being used - set to identity.
void WinRTExpressionConversionContext::UpdateCompositeTransformComponent(
    bool useMatrix4x4,
    _In_ WUComp::IExpressionAnimation* overallExpression,
    _In_opt_ WUComp::IExpressionAnimation* componentExpression,
    _In_ const wchar_t* propertyName
    )
{
     ASSERT(std::wstring(propertyName) == std::wstring(ExpressionHelper::sc_paramName_Scale) ||
            std::wstring(propertyName) == std::wstring(ExpressionHelper::sc_paramName_Skew) ||
            std::wstring(propertyName) == std::wstring(ExpressionHelper::sc_paramName_Rotate) ||
            std::wstring(propertyName) == std::wstring(ExpressionHelper::sc_paramName_RotateX) ||
            std::wstring(propertyName) == std::wstring(ExpressionHelper::sc_paramName_RotateY) ||
            std::wstring(propertyName) == std::wstring(ExpressionHelper::sc_paramName_RotateZ) ||
            std::wstring(propertyName) == std::wstring(ExpressionHelper::sc_paramName_Translate));

     ConnectSubExpression(useMatrix4x4, overallExpression, componentExpression, propertyName);
}
