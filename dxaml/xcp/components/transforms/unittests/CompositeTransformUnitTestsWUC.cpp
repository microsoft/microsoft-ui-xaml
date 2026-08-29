// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlLogging.h"
#include "CompositeTransformUnitTests.h"
#include "CompositeTransform.h"
#include "WinRTExpressionUnitTestHelper.h"
#include "ExpressionHelper.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Transforms {

void CompositeTransformUnitTestsWUC::ValidateWinRTExpression_Empty()
{
    LOG_OUTPUT(L"Default CompositeTransform (no-op)");
    WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();
    CCompositeTransform transform;

    transform.MakeWinRTExpression(&context);
    WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();

    WinRTExpressionUnitTestHelper::VerifyExpressionString(pWinRTExpressionNoRef, ExpressionHelper::sc_Expression_CompositeTransform_ScaleTranslate);

    std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap = WinRTExpressionUnitTestHelper::GetPropertyInfoMap(pWinRTExpressionNoRef);
    WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap, L"X", 0.0f, nullptr);
    WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap, L"Y", 0.0f, nullptr);
    WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap, L"ScaleX", 1.0f, nullptr);
    WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap, L"ScaleY", 1.0f, nullptr);
}

void CompositeTransformUnitTestsWUC::ValidateWinRTExpression_OneProperty()
{
    {
        LOG_OUTPUT(L"Static translate");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        CCompositeTransform transform;
        transform.m_eTranslateX = 43.21f;
        transform.m_eTranslateY = 12.34f;

        transform.MakeWinRTExpression(&context);
        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();

        WinRTExpressionUnitTestHelper::VerifyExpressionString(pWinRTExpressionNoRef, ExpressionHelper::sc_Expression_CompositeTransform_ScaleTranslate);

        std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap = WinRTExpressionUnitTestHelper::GetPropertyInfoMap(pWinRTExpressionNoRef);
        WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap, L"X", 43.21f, nullptr);
        WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap, L"Y", 12.34f, nullptr);
        WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap, L"ScaleX", 1.0f, nullptr);
        WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap, L"ScaleY", 1.0f, nullptr);
    }

    {
        LOG_OUTPUT(L"Animated translate");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        ScalarKFAHelper scalarKFA1(context);
        WUComp::ICompositionAnimation* pAnim1NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA1);

        ScalarKFAHelper scalarKFA2(context);
        WUComp::ICompositionAnimation* pAnim2NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA2);

        CCompositeTransform transform;
        transform.SetDCompAnimation(pAnim1NoRef, KnownPropertyIndex::CompositeTransform_TranslateXAnimation);
        transform.m_isTranslateXAnimationDirty = true;
        transform.SetDCompAnimation(pAnim2NoRef, KnownPropertyIndex::CompositeTransform_TranslateYAnimation);
        transform.m_isTranslateYAnimationDirty = true;

        transform.MakeWinRTExpression(&context);
        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();

        WinRTExpressionUnitTestHelper::VerifyExpressionString(pWinRTExpressionNoRef, ExpressionHelper::sc_Expression_CompositeTransform_ScaleTranslate);

        std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap = WinRTExpressionUnitTestHelper::GetPropertyInfoMap(pWinRTExpressionNoRef);
        WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap, L"X", 0.0f, pAnim1NoRef);
        WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap, L"Y", 0.0f, pAnim2NoRef);
        WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap, L"ScaleX", 1.0f, nullptr);
        WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap, L"ScaleY", 1.0f, nullptr);
    }

    {
        LOG_OUTPUT(L"Static scale");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        CCompositeTransform transform;
        transform.m_eScaleX = 43.21f;
        transform.m_eScaleY = 12.34f;

        transform.MakeWinRTExpression(&context);
        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();

        WinRTExpressionUnitTestHelper::VerifyExpressionString(pWinRTExpressionNoRef, ExpressionHelper::sc_Expression_CompositeTransform_ScaleTranslate);

        std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap = WinRTExpressionUnitTestHelper::GetPropertyInfoMap(pWinRTExpressionNoRef);
        WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap, L"X", 0.0f, nullptr);
        WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap, L"Y", 0.0f, nullptr);
        WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap, L"ScaleX", 43.21f, nullptr);
        WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap, L"ScaleY", 12.34f, nullptr);
    }

    {
        LOG_OUTPUT(L"Animated scale");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        ScalarKFAHelper scalarKFA1(context);
        WUComp::ICompositionAnimation* pAnim1NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA1);

        ScalarKFAHelper scalarKFA2(context);
        WUComp::ICompositionAnimation* pAnim2NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA2);

        CCompositeTransform transform;
        transform.SetDCompAnimation(pAnim1NoRef, KnownPropertyIndex::CompositeTransform_ScaleXAnimation);
        transform.m_isScaleXAnimationDirty = true;
        transform.SetDCompAnimation(pAnim2NoRef, KnownPropertyIndex::CompositeTransform_ScaleYAnimation);
        transform.m_isScaleYAnimationDirty = true;

        transform.MakeWinRTExpression(&context);
        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();

        WinRTExpressionUnitTestHelper::VerifyExpressionString(pWinRTExpressionNoRef, ExpressionHelper::sc_Expression_CompositeTransform_ScaleTranslate);

        std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap = WinRTExpressionUnitTestHelper::GetPropertyInfoMap(pWinRTExpressionNoRef);
        WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap, L"X", 0.0f, nullptr);
        WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap, L"Y", 0.0f, nullptr);
        WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap, L"ScaleX", 0.0f, pAnim1NoRef);
        WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap, L"ScaleY", 0.0f, pAnim2NoRef);
    }

    {
        LOG_OUTPUT(L"Static rotate");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        CCompositeTransform transform;
        transform.m_eRotation = 43.21f;

        transform.MakeWinRTExpression(&context);
        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();

        WinRTExpressionUnitTestHelper::VerifyExpressionString(pWinRTExpressionNoRef, ExpressionHelper::sc_Expression_CompositeTransform_CenterPoint);

        std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap = WinRTExpressionUnitTestHelper::GetPropertyInfoMap(pWinRTExpressionNoRef);
        {
            auto it = propertyInfoMap.find(std::wstring(L"Rotate"));
            ASSERT(it != propertyInfoMap.end());
            const MockDComp::PropertyInfo& propertyInfo = it->second;

            VERIFY_IS_TRUE(propertyInfo.propertyValueType == MockDComp::CompositionValueType::Matrix3x2);
            VERIFY_IS_TRUE(propertyInfo.m_animation != nullptr);

            Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> spExpression;
            propertyInfo.m_animation.As(&spExpression);

            WinRTExpressionUnitTestHelper::VerifyRotateTransform(
                spExpression.Get(),
                ExpressionHelper::sc_Expression_Rotate,
                false, /* hasCenterPoint */
                0, 0, 43.21f,
                nullptr, nullptr, nullptr
                );
        }
    }

    {
        LOG_OUTPUT(L"Animated rotate");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        ScalarKFAHelper scalarKFA1(context);
        WUComp::ICompositionAnimation* pAnim1NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA1);

        CCompositeTransform transform;
        transform.SetDCompAnimation(pAnim1NoRef, KnownPropertyIndex::CompositeTransform_RotateAnimation);
        transform.m_isRotateAnimationDirty = true;

        transform.MakeWinRTExpression(&context);
        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();

        WinRTExpressionUnitTestHelper::VerifyExpressionString(pWinRTExpressionNoRef, ExpressionHelper::sc_Expression_CompositeTransform_CenterPoint);

        std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap = WinRTExpressionUnitTestHelper::GetPropertyInfoMap(pWinRTExpressionNoRef);
        {
            auto it = propertyInfoMap.find(std::wstring(L"Rotate"));
            ASSERT(it != propertyInfoMap.end());
            const MockDComp::PropertyInfo& propertyInfo = it->second;

            VERIFY_IS_TRUE(propertyInfo.propertyValueType == MockDComp::CompositionValueType::Matrix3x2);
            VERIFY_IS_TRUE(propertyInfo.m_animation != nullptr);

            Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> spExpression;
            propertyInfo.m_animation.As(&spExpression);

            WinRTExpressionUnitTestHelper::VerifyRotateTransform(
                spExpression.Get(),
                ExpressionHelper::sc_Expression_Rotate,
                false, /* hasCenterPoint */
                0, 0, 0,
                nullptr, nullptr, pAnim1NoRef
                );
        }
    }

    {
        LOG_OUTPUT(L"Static skew");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        CCompositeTransform transform;
        transform.m_eSkewX = 43.21f;
        transform.m_eSkewY = 12.34f;

        transform.MakeWinRTExpression(&context);
        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();

        WinRTExpressionUnitTestHelper::VerifyExpressionString(pWinRTExpressionNoRef, ExpressionHelper::sc_Expression_CompositeTransform_CenterPoint);

        std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap = WinRTExpressionUnitTestHelper::GetPropertyInfoMap(pWinRTExpressionNoRef);
        {
            auto it = propertyInfoMap.find(std::wstring(L"Skew"));
            ASSERT(it != propertyInfoMap.end());
            const MockDComp::PropertyInfo& propertyInfo = it->second;

            VERIFY_IS_TRUE(propertyInfo.propertyValueType == MockDComp::CompositionValueType::Matrix3x2);
            VERIFY_IS_TRUE(propertyInfo.m_animation != nullptr);

            Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> spExpression;
            propertyInfo.m_animation.As(&spExpression);

            WinRTExpressionUnitTestHelper::VerifySkewTransform(
                spExpression.Get(),
                ExpressionHelper::sc_Expression_Skew,
                false, /* hasCenterPoint */
                0, 0, 43.21f, 12.34f,
                nullptr, nullptr, nullptr, nullptr
                );
        }
    }

    {
        LOG_OUTPUT(L"Animated skew");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        ScalarKFAHelper scalarKFA1(context);
        WUComp::ICompositionAnimation* pAnim1NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA1);

        ScalarKFAHelper scalarKFA2(context);
        WUComp::ICompositionAnimation* pAnim2NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA2);

        CCompositeTransform transform;
        transform.SetDCompAnimation(pAnim1NoRef, KnownPropertyIndex::CompositeTransform_SkewXAnimation);
        transform.m_isSkewXAnimationDirty = true;
        transform.SetDCompAnimation(pAnim2NoRef, KnownPropertyIndex::CompositeTransform_SkewYAnimation);
        transform.m_isSkewYAnimationDirty = true;

        transform.MakeWinRTExpression(&context);
        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();

        WinRTExpressionUnitTestHelper::VerifyExpressionString(pWinRTExpressionNoRef, ExpressionHelper::sc_Expression_CompositeTransform_CenterPoint);

        std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap = WinRTExpressionUnitTestHelper::GetPropertyInfoMap(pWinRTExpressionNoRef);
        {
            auto it = propertyInfoMap.find(std::wstring(L"Skew"));
            ASSERT(it != propertyInfoMap.end());
            const MockDComp::PropertyInfo& propertyInfo = it->second;

            VERIFY_IS_TRUE(propertyInfo.propertyValueType == MockDComp::CompositionValueType::Matrix3x2);
            VERIFY_IS_TRUE(propertyInfo.m_animation != nullptr);

            Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> spExpression;
            propertyInfo.m_animation.As(&spExpression);

            WinRTExpressionUnitTestHelper::VerifySkewTransform(
                spExpression.Get(),
                ExpressionHelper::sc_Expression_Skew,
                false, /* hasCenterPoint */
                0, 0, 0, 0,
                nullptr, nullptr, pAnim1NoRef, pAnim2NoRef
                );
        }
    }
}

void CompositeTransformUnitTestsWUC::ValidateWinRTExpression_PropertyCombinations()
{
    {
        LOG_OUTPUT(L"Animated translate & static scale");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        ScalarKFAHelper scalarKFA1(context);
        WUComp::ICompositionAnimation* pAnim1NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA1);

        ScalarKFAHelper scalarKFA2(context);
        WUComp::ICompositionAnimation* pAnim2NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA2);

        CCompositeTransform transform;
        transform.SetDCompAnimation(pAnim1NoRef, KnownPropertyIndex::CompositeTransform_TranslateXAnimation);
        transform.m_isTranslateXAnimationDirty = true;
        transform.SetDCompAnimation(pAnim2NoRef, KnownPropertyIndex::CompositeTransform_TranslateYAnimation);
        transform.m_isTranslateYAnimationDirty = true;
        transform.m_eScaleX = 43.21f;
        transform.m_eScaleY = 12.34f;

        transform.MakeWinRTExpression(&context);
        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();

        WinRTExpressionUnitTestHelper::VerifyExpressionString(pWinRTExpressionNoRef, ExpressionHelper::sc_Expression_CompositeTransform_ScaleTranslate);

        std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap = WinRTExpressionUnitTestHelper::GetPropertyInfoMap(pWinRTExpressionNoRef);
        WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap, L"X", 0.0f, pAnim1NoRef);
        WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap, L"Y", 0.0f, pAnim2NoRef);
        WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap, L"ScaleX", 43.21f, nullptr);
        WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap, L"ScaleY", 12.34f, nullptr);
    }

    // TODO_WinRT: Add this case since it covers center point animation
    {
        LOG_OUTPUT(L"Animated & static center & static scale");
    }

    {
        LOG_OUTPUT(L"Static everything");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        CCompositeTransform transform;
        transform.m_eScaleX = 1.2f;
        transform.m_eScaleY = 2.3f;
        transform.m_eRotation = 3.4f;
        transform.m_eSkewX = 4.5f;
        transform.m_eSkewY = 5.6f;
        transform.m_ptCenter.x = 6.7f;
        transform.m_ptCenter.y = 7.8f;
        transform.m_eTranslateX = 8.9f;
        transform.m_eTranslateY = 9.1f;

        transform.MakeWinRTExpression(&context);
        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();

        WinRTExpressionUnitTestHelper::VerifyExpressionString(pWinRTExpressionNoRef, ExpressionHelper::sc_Expression_CompositeTransform_CenterPoint);

        std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap = WinRTExpressionUnitTestHelper::GetPropertyInfoMap(pWinRTExpressionNoRef);
        {
            auto it = propertyInfoMap.find(std::wstring(L"Translate"));
            ASSERT(it != propertyInfoMap.end());
            const MockDComp::PropertyInfo& propertyInfo = it->second;

            VERIFY_IS_TRUE(propertyInfo.propertyValueType == MockDComp::CompositionValueType::Matrix3x2);
            VERIFY_IS_TRUE(propertyInfo.m_animation != nullptr);

            Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> spExpression;
            propertyInfo.m_animation.As(&spExpression);

            WinRTExpressionUnitTestHelper::VerifyTranslateTransform(
                spExpression.Get(),
                ExpressionHelper::sc_Expression_Translate,
                8.9f, 9.1f,
                nullptr, nullptr
                );
        }

        {
            auto it = propertyInfoMap.find(std::wstring(L"Scale"));
            ASSERT(it != propertyInfoMap.end());
            const MockDComp::PropertyInfo& propertyInfo = it->second;

            VERIFY_IS_TRUE(propertyInfo.propertyValueType == MockDComp::CompositionValueType::Matrix3x2);
            VERIFY_IS_TRUE(propertyInfo.m_animation != nullptr);

            Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> spExpression;
            propertyInfo.m_animation.As(&spExpression);

            WinRTExpressionUnitTestHelper::VerifyScaleTransform(
                spExpression.Get(),
                ExpressionHelper::sc_Expression_Scale,
                false, /* hasCenterPoint */
                0, 0, 1.2f, 2.3f,
                nullptr, nullptr, nullptr, nullptr
                );
        }

        {
            auto it = propertyInfoMap.find(std::wstring(L"Skew"));
            ASSERT(it != propertyInfoMap.end());
            const MockDComp::PropertyInfo& propertyInfo = it->second;

            VERIFY_IS_TRUE(propertyInfo.propertyValueType == MockDComp::CompositionValueType::Matrix3x2);
            VERIFY_IS_TRUE(propertyInfo.m_animation != nullptr);

            Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> spExpression;
            propertyInfo.m_animation.As(&spExpression);

            WinRTExpressionUnitTestHelper::VerifySkewTransform(
                spExpression.Get(),
                ExpressionHelper::sc_Expression_Skew,
                false, /* hasCenterPoint */
                0, 0, 4.5f, 5.6f,
                nullptr, nullptr, nullptr, nullptr
                );
        }

        {
            auto it = propertyInfoMap.find(std::wstring(L"Rotate"));
            ASSERT(it != propertyInfoMap.end());
            const MockDComp::PropertyInfo& propertyInfo = it->second;

            VERIFY_IS_TRUE(propertyInfo.propertyValueType == MockDComp::CompositionValueType::Matrix3x2);
            VERIFY_IS_TRUE(propertyInfo.m_animation != nullptr);

            Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> spExpression;
            propertyInfo.m_animation.As(&spExpression);

            WinRTExpressionUnitTestHelper::VerifyRotateTransform(
                spExpression.Get(),
                ExpressionHelper::sc_Expression_Rotate,
                false, /* hasCenterPoint */
                0, 0, 3.4f,
                nullptr, nullptr, nullptr
                );
        }
    }

    // TODO_WinRT: Add validation for center point animation
    {
        LOG_OUTPUT(L"Animated everything");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        ScalarKFAHelper scalarKFA1(context);
        WUComp::ICompositionAnimation* pAnim1NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA1);

        ScalarKFAHelper scalarKFA2(context);
        WUComp::ICompositionAnimation* pAnim2NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA2);

        ScalarKFAHelper scalarKFA3(context);
        WUComp::ICompositionAnimation* pAnim3NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA3);

        ScalarKFAHelper scalarKFA4(context);
        WUComp::ICompositionAnimation* pAnim4NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA4);

        ScalarKFAHelper scalarKFA5(context);
        WUComp::ICompositionAnimation* pAnim5NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA5);

        ScalarKFAHelper scalarKFA6(context);
        WUComp::ICompositionAnimation* pAnim6NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA6);

        ScalarKFAHelper scalarKFA7(context);
        WUComp::ICompositionAnimation* pAnim7NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA7);

        ScalarKFAHelper scalarKFA8(context);
        WUComp::ICompositionAnimation* pAnim8NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA8);

        ScalarKFAHelper scalarKFA9(context);
        WUComp::ICompositionAnimation* pAnim9NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA9);


        CCompositeTransform transform;
        transform.SetDCompAnimation(pAnim1NoRef, KnownPropertyIndex::CompositeTransform_ScaleXAnimation);
        transform.m_isScaleXAnimationDirty = true;
        transform.SetDCompAnimation(pAnim2NoRef, KnownPropertyIndex::CompositeTransform_ScaleYAnimation);
        transform.m_isScaleYAnimationDirty = true;
        transform.SetDCompAnimation(pAnim3NoRef, KnownPropertyIndex::CompositeTransform_RotateAnimation);
        transform.m_isRotateAnimationDirty = true;
        transform.SetDCompAnimation(pAnim4NoRef, KnownPropertyIndex::CompositeTransform_SkewXAnimation);
        transform.m_isSkewXAnimationDirty = true;
        transform.SetDCompAnimation(pAnim5NoRef, KnownPropertyIndex::CompositeTransform_SkewYAnimation);
        transform.m_isSkewYAnimationDirty = true;
        transform.SetDCompAnimation(pAnim6NoRef, KnownPropertyIndex::CompositeTransform_CenterXAnimation);
        transform.m_isCenterXAnimationDirty = true;
        transform.SetDCompAnimation(pAnim7NoRef, KnownPropertyIndex::CompositeTransform_CenterYAnimation);
        transform.m_isCenterYAnimationDirty = true;
        transform.SetDCompAnimation(pAnim8NoRef, KnownPropertyIndex::CompositeTransform_TranslateXAnimation);
        transform.m_isTranslateXAnimationDirty = true;
        transform.SetDCompAnimation(pAnim9NoRef, KnownPropertyIndex::CompositeTransform_TranslateYAnimation);
        transform.m_isTranslateYAnimationDirty = true;

        transform.MakeWinRTExpression(&context);
        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();

        WinRTExpressionUnitTestHelper::VerifyExpressionString(pWinRTExpressionNoRef, ExpressionHelper::sc_Expression_CompositeTransform_CenterPoint);

        std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap = WinRTExpressionUnitTestHelper::GetPropertyInfoMap(pWinRTExpressionNoRef);
        {
            auto it = propertyInfoMap.find(std::wstring(L"Translate"));
            ASSERT(it != propertyInfoMap.end());
            const MockDComp::PropertyInfo& propertyInfo = it->second;

            VERIFY_IS_TRUE(propertyInfo.propertyValueType == MockDComp::CompositionValueType::Matrix3x2);
            VERIFY_IS_TRUE(propertyInfo.m_animation != nullptr);

            Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> spExpression;
            propertyInfo.m_animation.As(&spExpression);

            WinRTExpressionUnitTestHelper::VerifyTranslateTransform(
                spExpression.Get(),
                ExpressionHelper::sc_Expression_Translate,
                0, 0,
                pAnim8NoRef, pAnim9NoRef
                );
        }

        {
            auto it = propertyInfoMap.find(std::wstring(L"Scale"));
            ASSERT(it != propertyInfoMap.end());
            const MockDComp::PropertyInfo& propertyInfo = it->second;

            VERIFY_IS_TRUE(propertyInfo.propertyValueType == MockDComp::CompositionValueType::Matrix3x2);
            VERIFY_IS_TRUE(propertyInfo.m_animation != nullptr);

            Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> spExpression;
            propertyInfo.m_animation.As(&spExpression);

            WinRTExpressionUnitTestHelper::VerifyScaleTransform(
                spExpression.Get(),
                ExpressionHelper::sc_Expression_Scale,
                false, /* hasCenterPoint */
                0, 0, 0, 0,
                nullptr, nullptr, pAnim1NoRef, pAnim2NoRef
                );
        }

        {
            auto it = propertyInfoMap.find(std::wstring(L"Skew"));
            ASSERT(it != propertyInfoMap.end());
            const MockDComp::PropertyInfo& propertyInfo = it->second;

            VERIFY_IS_TRUE(propertyInfo.propertyValueType == MockDComp::CompositionValueType::Matrix3x2);
            VERIFY_IS_TRUE(propertyInfo.m_animation != nullptr);

            Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> spExpression;
            propertyInfo.m_animation.As(&spExpression);

            WinRTExpressionUnitTestHelper::VerifySkewTransform(
                spExpression.Get(),
                ExpressionHelper::sc_Expression_Skew,
                false, /* hasCenterPoint */
                0, 0, 0, 0,
                nullptr, nullptr, pAnim4NoRef, pAnim5NoRef
                );
        }

        {
            auto it = propertyInfoMap.find(std::wstring(L"Rotate"));
            ASSERT(it != propertyInfoMap.end());
            const MockDComp::PropertyInfo& propertyInfo = it->second;

            VERIFY_IS_TRUE(propertyInfo.propertyValueType == MockDComp::CompositionValueType::Matrix3x2);
            VERIFY_IS_TRUE(propertyInfo.m_animation != nullptr);

            Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> spExpression;
            propertyInfo.m_animation.As(&spExpression);

            WinRTExpressionUnitTestHelper::VerifyRotateTransform(
                spExpression.Get(),
                ExpressionHelper::sc_Expression_Rotate,
                false, /* hasCenterPoint */
                0, 0, 0,
                nullptr, nullptr, pAnim3NoRef
                );
        }
    }
}

void CompositeTransformUnitTestsWUC::ValidateDeviceLostCleanupWUC()
{
    CCompositeTransform transform;

    {
        // Create the DComp context in a block so it is cleaned up / lost at the end of this block
        LOG_OUTPUT(L"Create DComp resource");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        // Create and set the animations
        ScalarKFAHelper scalarKFA1(context);
        WUComp::ICompositionAnimation* pAnim1NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA1);

        ScalarKFAHelper scalarKFA2(context);
        WUComp::ICompositionAnimation* pAnim2NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA2);

        ScalarKFAHelper scalarKFA3(context);
        WUComp::ICompositionAnimation* pAnim3NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA3);

        ScalarKFAHelper scalarKFA4(context);
        WUComp::ICompositionAnimation* pAnim4NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA4);

        ScalarKFAHelper scalarKFA5(context);
        WUComp::ICompositionAnimation* pAnim5NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA5);

        ScalarKFAHelper scalarKFA6(context);
        WUComp::ICompositionAnimation* pAnim6NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA6);

        ScalarKFAHelper scalarKFA7(context);
        WUComp::ICompositionAnimation* pAnim7NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA7);

        ScalarKFAHelper scalarKFA8(context);
        WUComp::ICompositionAnimation* pAnim8NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA8);

        ScalarKFAHelper scalarKFA9(context);
        WUComp::ICompositionAnimation* pAnim9NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA9);

        transform.SetDCompAnimation(pAnim1NoRef, KnownPropertyIndex::CompositeTransform_ScaleXAnimation);
        transform.m_isScaleXAnimationDirty = true;
        transform.SetDCompAnimation(pAnim2NoRef, KnownPropertyIndex::CompositeTransform_ScaleYAnimation);
        transform.m_isScaleYAnimationDirty = true;
        transform.SetDCompAnimation(pAnim3NoRef, KnownPropertyIndex::CompositeTransform_RotateAnimation);
        transform.m_isRotateAnimationDirty = true;
        transform.SetDCompAnimation(pAnim4NoRef, KnownPropertyIndex::CompositeTransform_SkewXAnimation);
        transform.m_isSkewXAnimationDirty = true;
        transform.SetDCompAnimation(pAnim5NoRef, KnownPropertyIndex::CompositeTransform_SkewYAnimation);
        transform.m_isSkewYAnimationDirty = true;
        transform.SetDCompAnimation(pAnim6NoRef, KnownPropertyIndex::CompositeTransform_CenterXAnimation);
        transform.m_isCenterXAnimationDirty = true;
        transform.SetDCompAnimation(pAnim7NoRef, KnownPropertyIndex::CompositeTransform_CenterYAnimation);
        transform.m_isCenterYAnimationDirty = true;
        transform.SetDCompAnimation(pAnim8NoRef, KnownPropertyIndex::CompositeTransform_TranslateXAnimation);
        transform.m_isTranslateXAnimationDirty = true;
        transform.SetDCompAnimation(pAnim9NoRef, KnownPropertyIndex::CompositeTransform_TranslateYAnimation);
        transform.m_isTranslateYAnimationDirty = true;

        // Create and verify the expression
        transform.MakeWinRTExpression(&context);
        VERIFY_IS_NOT_NULL(transform.GetWinRTExpression());
        // No need to validate the pieces - already tested by ValidateWinRTExpression_PropertyCombinations
    }

    {
        LOG_OUTPUT(L"Clean up after device lost");
        transform.ReleaseDCompResources();
        VERIFY_IS_NULL(transform.GetWinRTExpression());
        VERIFY_IS_NULL(transform.m_spScaleExpression);
        VERIFY_IS_NULL(transform.m_spRotateExpression);
        VERIFY_IS_NULL(transform.m_spSkewExpression);
        VERIFY_IS_NULL(transform.m_spTranslateExpression);

        VERIFY_IS_NULL(transform.GetDCompAnimation(KnownPropertyIndex::CompositeTransform_ScaleXAnimation));
        VERIFY_IS_NULL(transform.GetDCompAnimation(KnownPropertyIndex::CompositeTransform_ScaleYAnimation));
        VERIFY_IS_NULL(transform.GetDCompAnimation(KnownPropertyIndex::CompositeTransform_RotateAnimation));
        VERIFY_IS_NULL(transform.GetDCompAnimation(KnownPropertyIndex::CompositeTransform_SkewXAnimation));
        VERIFY_IS_NULL(transform.GetDCompAnimation(KnownPropertyIndex::CompositeTransform_SkewYAnimation));
        VERIFY_IS_NULL(transform.GetDCompAnimation(KnownPropertyIndex::CompositeTransform_CenterXAnimation));
        VERIFY_IS_NULL(transform.GetDCompAnimation(KnownPropertyIndex::CompositeTransform_CenterYAnimation));
        VERIFY_IS_NULL(transform.GetDCompAnimation(KnownPropertyIndex::CompositeTransform_TranslateXAnimation));
        VERIFY_IS_NULL(transform.GetDCompAnimation(KnownPropertyIndex::CompositeTransform_TranslateYAnimation));
    }
}

} } } } } }
