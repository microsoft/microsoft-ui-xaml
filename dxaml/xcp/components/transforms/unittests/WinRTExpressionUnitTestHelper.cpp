// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "WinRTExpressionUnitTestHelper.h"
#include "WinRTLocalExpressionBuilder.h"
#include "Transform.h"
#include "StringUtilities.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Transforms {

// TODO_WinRT: Implement this when adding builder coverage (to UIElementUnitTests.cpp)
//WinRTLocalExpressionBuilder WinRTExpressionUnitTestHelper::MakeWinRTLocalExpressionBuilder()
//{
//}

WinRTExpressionConversionContext WinRTExpressionUnitTestHelper::MakeContext()
{
    wrl::ComPtr<WUComp::ICompositor> spMockCompositor;
    VERIFY_SUCCEEDED(MockDComp::CreateMockCompositor(&spMockCompositor));
    return WinRTExpressionConversionContext(spMockCompositor.Get());
}

void WinRTExpressionUnitTestHelper::VerifyTranslateTransform(
    _In_ WUComp::IExpressionAnimation* pExpression,
    _In_ const wchar_t* pszExpectedExpressionString,
    float x,
    float y,
    _In_opt_ WUComp::ICompositionAnimation* pXAnimation,
    _In_opt_ WUComp::ICompositionAnimation* pYAnimation
    )
{
    WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

    // Verify overall string
    VerifyExpressionString(pExpression, pszExpectedExpressionString);

    // Verify properties
    std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap = GetPropertyInfoMap(pExpression);
    VerifyScalarProperty(propertyInfoMap, L"X", x, pXAnimation);
    VerifyScalarProperty(propertyInfoMap, L"Y", y, pYAnimation);
}

void WinRTExpressionUnitTestHelper::VerifyScaleTransform(
    _In_ WUComp::IExpressionAnimation* pExpression,
    _In_ const wchar_t* pszExpectedExpressionString,
    _In_ bool hasCenterPoint,
    float centerX,
    float centerY,
    float scaleX,
    float scaleY,
    _In_opt_ WUComp::ICompositionAnimation* pCenterXAnimation,
    _In_opt_ WUComp::ICompositionAnimation* pCenterYAnimation,
    _In_opt_ WUComp::ICompositionAnimation* pScaleXAnimation,
    _In_opt_ WUComp::ICompositionAnimation* pScaleYAnimation
    )
{
    WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

    // Verify overall string
    VerifyExpressionString(pExpression, pszExpectedExpressionString);

    // Verify properties
    std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap = GetPropertyInfoMap(pExpression);

    if (hasCenterPoint)
    {
        VerifyScalarProperty(propertyInfoMap, L"CenterX", centerX, pCenterXAnimation);
        VerifyScalarProperty(propertyInfoMap, L"CenterY", centerY, pCenterYAnimation);
    }

    VerifyScalarProperty(propertyInfoMap, L"ScaleX", scaleX, pScaleXAnimation);
    VerifyScalarProperty(propertyInfoMap, L"ScaleY", scaleY, pScaleYAnimation);
}

void WinRTExpressionUnitTestHelper::VerifyRotateTransform(
    _In_ WUComp::IExpressionAnimation* pExpression,
    _In_ const wchar_t* pszExpectedExpressionString,
    _In_ bool hasCenterPoint,
    float centerX,
    float centerY,
    float angle,
    _In_opt_ WUComp::ICompositionAnimation* pCenterXAnimation,
    _In_opt_ WUComp::ICompositionAnimation* pCenterYAnimation,
    _In_opt_ WUComp::ICompositionAnimation* pAngleAnimation
    )
{
    WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

    // Verify overall string
    VerifyExpressionString(pExpression, pszExpectedExpressionString);

    // Verify properties
    std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap = GetPropertyInfoMap(pExpression);

    if (hasCenterPoint)
    {
        VerifyScalarProperty(propertyInfoMap, L"CenterX", centerX, pCenterXAnimation);
        VerifyScalarProperty(propertyInfoMap, L"CenterY", centerY, pCenterYAnimation);
    }

    VerifyScalarProperty(propertyInfoMap, L"RotateAngle", angle, pAngleAnimation);
}

void WinRTExpressionUnitTestHelper::VerifySkewTransform(
    _In_ WUComp::IExpressionAnimation* pExpression,
    _In_ const wchar_t* pszExpectedExpressionString,
    _In_ bool hasCenterPoint,
    float centerX,
    float centerY,
    float angleX,
    float angleY,
    _In_opt_ WUComp::ICompositionAnimation* pCenterXAnimation,
    _In_opt_ WUComp::ICompositionAnimation* pCenterYAnimation,
    _In_opt_ WUComp::ICompositionAnimation* pAngleXAnimation,
    _In_opt_ WUComp::ICompositionAnimation* pAngleYAnimation
    )
{
    WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

    // Verify overall string
    VerifyExpressionString(pExpression, pszExpectedExpressionString);

    // Verify properties
    std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap = GetPropertyInfoMap(pExpression);

    if (hasCenterPoint)
    {
        VerifyScalarProperty(propertyInfoMap, L"CenterX", centerX, pCenterXAnimation);
        VerifyScalarProperty(propertyInfoMap, L"CenterY", centerY, pCenterYAnimation);
    }

    VerifyScalarProperty(propertyInfoMap, L"SkewAngleX", angleX, pAngleXAnimation);
    VerifyScalarProperty(propertyInfoMap, L"SkewAngleY", angleY, pAngleYAnimation);
}

void WinRTExpressionUnitTestHelper::VerifyMatrixTransform(
    _In_ WUComp::IExpressionAnimation* pExpression,
    _In_ const wchar_t* pszExpectedExpressionString,
    float m11,
    float m12,
    float m21,
    float m22,
    float m31,
    float m32
    )
{
    WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

    // Verify overall string
    VerifyExpressionString(pExpression, pszExpectedExpressionString);

    // Verify properties
    std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap = GetPropertyInfoMap(pExpression);

    {
        auto it = propertyInfoMap.find(std::wstring(L"M11"));
        ASSERT(it != propertyInfoMap.end());
        const MockDComp::PropertyInfo& propertyInfo = it->second;

        VERIFY_IS_TRUE(propertyInfo.propertyValueType == MockDComp::CompositionValueType::Scalar);
        VERIFY_IS_TRUE(propertyInfo.m_animation == nullptr);            // MatrixTransform animation not supported
        VERIFY_ARE_EQUAL(m11, propertyInfo.value.scalar);
    }

    {
        auto it = propertyInfoMap.find(std::wstring(L"M12"));
        ASSERT(it != propertyInfoMap.end());
        const MockDComp::PropertyInfo& propertyInfo = it->second;

        VERIFY_IS_TRUE(propertyInfo.propertyValueType == MockDComp::CompositionValueType::Scalar);
        VERIFY_IS_TRUE(propertyInfo.m_animation == nullptr);            // MatrixTransform animation not supported
        VERIFY_ARE_EQUAL(m12, propertyInfo.value.scalar);
    }

    {
        auto it = propertyInfoMap.find(std::wstring(L"M21"));
        ASSERT(it != propertyInfoMap.end());
        const MockDComp::PropertyInfo& propertyInfo = it->second;

        VERIFY_IS_TRUE(propertyInfo.propertyValueType == MockDComp::CompositionValueType::Scalar);
        VERIFY_IS_TRUE(propertyInfo.m_animation == nullptr);            // MatrixTransform animation not supported
        VERIFY_ARE_EQUAL(m21, propertyInfo.value.scalar);
    }

    {
        auto it = propertyInfoMap.find(std::wstring(L"M22"));
        ASSERT(it != propertyInfoMap.end());
        const MockDComp::PropertyInfo& propertyInfo = it->second;

        VERIFY_IS_TRUE(propertyInfo.propertyValueType == MockDComp::CompositionValueType::Scalar);
        VERIFY_IS_TRUE(propertyInfo.m_animation == nullptr);            // MatrixTransform animation not supported
        VERIFY_ARE_EQUAL(m22, propertyInfo.value.scalar);
    }

    {
        auto it = propertyInfoMap.find(std::wstring(L"Tx"));
        ASSERT(it != propertyInfoMap.end());
        const MockDComp::PropertyInfo& propertyInfo = it->second;

        VERIFY_IS_TRUE(propertyInfo.propertyValueType == MockDComp::CompositionValueType::Scalar);
        VERIFY_IS_TRUE(propertyInfo.m_animation == nullptr);            // MatrixTransform animation not supported
        VERIFY_ARE_EQUAL(m31, propertyInfo.value.scalar);
    }

    {
        auto it = propertyInfoMap.find(std::wstring(L"Ty"));
        ASSERT(it != propertyInfoMap.end());
        const MockDComp::PropertyInfo& propertyInfo = it->second;

        VERIFY_IS_TRUE(propertyInfo.propertyValueType == MockDComp::CompositionValueType::Scalar);
        VERIFY_IS_TRUE(propertyInfo.m_animation == nullptr);            // MatrixTransform animation not supported
        VERIFY_ARE_EQUAL(m32, propertyInfo.value.scalar);
    }
}

void WinRTExpressionUnitTestHelper::VerifyRenderTransformOrigin(
    _In_ WUComp::IExpressionAnimation* pExpression,
    float adjustX,
    float adjustY)
{
    WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

    // Verify properties
    std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap = GetPropertyInfoMap(pExpression);
    VerifyScalarProperty(propertyInfoMap, L"X", adjustX, nullptr);
    VerifyScalarProperty(propertyInfoMap, L"Y", adjustY, nullptr);
}

void WinRTExpressionUnitTestHelper::VerifyExpressionString(_In_ WUComp::IExpressionAnimation* expr, _In_ const wchar_t* pszExpected)
{
    HSTRING expression;
    if (expr == nullptr)
    {
        VERIFY_IS_TRUE(pszExpected == nullptr);
    }
    else
    {
        VERIFY_SUCCEEDED(expr->get_Expression(&expression));
        ASSERT(WindowsGetStringLen(expression) > 0);
        VERIFY_ARE_STRINGS_EQUAL(pszExpected, expression);
    }

    // Note: WUC EA::get_Expression API has unconventional behavior in that it does not AddRef
    //       the returned hstring. Hence don't use wrl_wrappers::HString wrapper here,
    //       as its dtor would cause an unmatched Release (or rather WindowsDeleteString()).
}

void WinRTExpressionUnitTestHelper::VerifyScalarProperty(
    _In_ std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap,
    _In_ const wchar_t* pszPropertyName,
    _In_ float expectedValue,
    _In_opt_ WUComp::ICompositionAnimation* animation
    )
{
    auto it = propertyInfoMap.find(std::wstring(pszPropertyName));
    ASSERT(it != propertyInfoMap.end());
    const MockDComp::PropertyInfo& propertyInfo = it->second;

    VERIFY_IS_TRUE(propertyInfo.propertyValueType == MockDComp::CompositionValueType::Scalar);

    if (animation)
    {
        VERIFY_IS_NOT_NULL(propertyInfo.m_animation);
        VERIFY_IS_TRUE(animation == propertyInfo.GetAnimationTemplateNoRef());
    }
    else
    {
        VERIFY_ARE_EQUAL(expectedValue, propertyInfo.value.scalar);
    }
}

std::map<std::wstring, MockDComp::PropertyInfo>& WinRTExpressionUnitTestHelper::GetPropertyInfoMap(_In_ WUComp::IExpressionAnimation* expr)
{
    Microsoft::WRL::ComPtr<WUComp::ICompositionObject> spCO;
    Microsoft::WRL::ComPtr<WUComp::ICompositionPropertySet> spPS;

    VERIFY_SUCCEEDED(expr->QueryInterface(IID_PPV_ARGS(&spCO)));
    VERIFY_SUCCEEDED(spCO->get_Properties(&spPS));

    return MockDComp::GetPropertyInfoMap(spPS.Get());
}

} } } } } }
