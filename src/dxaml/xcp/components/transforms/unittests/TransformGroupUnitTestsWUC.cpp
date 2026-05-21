// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlLogging.h"
#include "TransformGroupUnitTests.h"
#include "TransformGroup.h"
#include "TransformCollection.h"
#include "TranslateTransform.h"
#include "CompositeTransform.h"
#include "ScaleTransform.h"
#include "RotateTransform.h"
#include "SkewTransform.h"
#include "WinRTExpressionUnitTestHelper.h"
#include "ExpressionHelper.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Transforms {

void TransformGroupUnitTestsWUC::ValidateWinRTExpression()
{
    {
        LOG_OUTPUT(L"Empty children");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();
        CTransformGroup transform;

        transform.MakeWinRTExpression(&context);
        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();

        VERIFY_IS_NULL(pWinRTExpressionNoRef);
    }

    {
        LOG_OUTPUT(L"Null children");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();
        CTransformGroup transform;
        CTransformCollection collection;
        transform.m_pChild = &collection;

        transform.MakeWinRTExpression(&context);
        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();

        VERIFY_IS_NULL(pWinRTExpressionNoRef);
    }

    {
        LOG_OUTPUT(L"One child");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        CTranslateTransform tr0;
        tr0.m_eX = 43.21f;
        tr0.m_eY = 12.34f;

        CTransformGroup transform;
        CTransformCollection collection;
        transform.m_pChild = &collection;
        transform.m_pChild->push_back(xref_ptr<CDependencyObject>(&tr0));

        transform.MakeWinRTExpression(&context);
        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();

        WinRTExpressionUnitTestHelper::VerifyExpressionString(pWinRTExpressionNoRef, L"PS.Expr0");

        std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap = WinRTExpressionUnitTestHelper::GetPropertyInfoMap(pWinRTExpressionNoRef);
        {
            auto it = propertyInfoMap.find(std::wstring(L"Expr0"));
            ASSERT(it != propertyInfoMap.end());
            const MockDComp::PropertyInfo& propertyInfo = it->second;

            VERIFY_IS_TRUE(propertyInfo.propertyValueType == MockDComp::CompositionValueType::Matrix3x2);
            VERIFY_IS_TRUE(propertyInfo.m_animation != nullptr);

            Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> spExpression;
            propertyInfo.m_animation.As(&spExpression);

            WinRTExpressionUnitTestHelper::VerifyTranslateTransform(
                spExpression.Get(),
                ExpressionHelper::sc_Expression_Translate,
                43.21f, 12.34f,
                nullptr, nullptr
                );
        }
    }

    {
        LOG_OUTPUT(L"Many children");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        CTranslateTransform tr0;
        tr0.m_eX = 43.21f;
        tr0.m_eY = 12.34f;

        ScalarKFAHelper scalarKFA1(context);
        WUComp::ICompositionAnimation* pAnim1NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA1);

        CTranslateTransform tr1;
        tr1.SetDCompAnimation(pAnim1NoRef, KnownPropertyIndex::TranslateTransform_XAnimation);
        tr1.m_isXAnimationDirty = true;
        tr1.m_eY = 234.56f;

        CScaleTransform tr2;
        tr2.m_ptCenter.x = 456.789f;
        tr2.m_ptCenter.y = 357.9f;
        tr2.m_eScaleX = -100.98f;
        tr2.m_eScaleY = 234.56f;

        ScalarKFAHelper scalarKFA2(context);
        WUComp::ICompositionAnimation* pAnim2NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA2);

        CRotateTransform tr3;
        tr3.m_ptCenter.x = 456.789f;
        tr3.m_ptCenter.y = 357.9f;
        tr3.SetDCompAnimation(pAnim2NoRef, KnownPropertyIndex::RotateTransform_AngleAnimation);
        tr3.m_isAngleAnimationDirty = true;

        CTransformGroup transform;
        CTransformCollection collection;
        transform.m_pChild = &collection;
        transform.m_pChild->push_back(xref_ptr<CDependencyObject>(&tr0));
        transform.m_pChild->push_back(xref_ptr<CDependencyObject>(&tr1));
        transform.m_pChild->push_back(xref_ptr<CDependencyObject>(&tr2));
        transform.m_pChild->push_back(xref_ptr<CDependencyObject>(&tr3));

        transform.MakeWinRTExpression(&context);
        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();

        WinRTExpressionUnitTestHelper::VerifyExpressionString(pWinRTExpressionNoRef, L"PS.Expr0 * PS.Expr1 * PS.Expr2 * PS.Expr3");

        std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap = WinRTExpressionUnitTestHelper::GetPropertyInfoMap(pWinRTExpressionNoRef);
        {
            auto it = propertyInfoMap.find(std::wstring(L"Expr0"));
            ASSERT(it != propertyInfoMap.end());
            const MockDComp::PropertyInfo& propertyInfo = it->second;

            VERIFY_IS_TRUE(propertyInfo.propertyValueType == MockDComp::CompositionValueType::Matrix3x2);
            VERIFY_IS_TRUE(propertyInfo.m_animation != nullptr);

            Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> spExpression;
            propertyInfo.m_animation.As(&spExpression);

            WinRTExpressionUnitTestHelper::VerifyTranslateTransform(
                spExpression.Get(),
                ExpressionHelper::sc_Expression_Translate,
                43.21f, 12.34f,
                nullptr, nullptr
                );
        }

        {
            auto it = propertyInfoMap.find(std::wstring(L"Expr1"));
            ASSERT(it != propertyInfoMap.end());
            const MockDComp::PropertyInfo& propertyInfo = it->second;

            VERIFY_IS_TRUE(propertyInfo.propertyValueType == MockDComp::CompositionValueType::Matrix3x2);
            VERIFY_IS_TRUE(propertyInfo.m_animation != nullptr);

            Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> spExpression;
            propertyInfo.m_animation.As(&spExpression);

            WinRTExpressionUnitTestHelper::VerifyTranslateTransform(
                spExpression.Get(),
                ExpressionHelper::sc_Expression_Translate,
                0, 234.56f,
                pAnim1NoRef, nullptr
                );
        }

        {
            auto it = propertyInfoMap.find(std::wstring(L"Expr2"));
            ASSERT(it != propertyInfoMap.end());
            const MockDComp::PropertyInfo& propertyInfo = it->second;

            VERIFY_IS_TRUE(propertyInfo.propertyValueType == MockDComp::CompositionValueType::Matrix3x2);
            VERIFY_IS_TRUE(propertyInfo.m_animation != nullptr);

            Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> spExpression;
            propertyInfo.m_animation.As(&spExpression);

            WinRTExpressionUnitTestHelper::VerifyScaleTransform(
                spExpression.Get(),
                ExpressionHelper::sc_Expression_Scale_CenterPoint,
                true, /* hasCenterPoint */
                456.789f,
                357.9f,
                -100.98f,
                234.56f,
                nullptr,
                nullptr,
                nullptr,
                nullptr
                );
        }

        {
            auto it = propertyInfoMap.find(std::wstring(L"Expr3"));
            ASSERT(it != propertyInfoMap.end());
            const MockDComp::PropertyInfo& propertyInfo = it->second;

            VERIFY_IS_TRUE(propertyInfo.propertyValueType == MockDComp::CompositionValueType::Matrix3x2);
            VERIFY_IS_TRUE(propertyInfo.m_animation != nullptr);

            Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> spExpression;
            propertyInfo.m_animation.As(&spExpression);

            WinRTExpressionUnitTestHelper::VerifyRotateTransform(
                spExpression.Get(),
                ExpressionHelper::sc_Expression_Rotate_CenterPoint,
                true, /* hasCenterPoint */
                456.789f,
                357.9f,
                0,
                nullptr,
                nullptr,
                pAnim2NoRef
                );
        }
    }

    {
        LOG_OUTPUT(L"Transform chain");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        CTranslateTransform tr0;
        tr0.m_eX = 43.21f;
        tr0.m_eY = 12.34f;

        CTransformGroup tr1;
        CTransformCollection c1;
        tr1.m_pChild = &c1;
        tr1.m_pChild->push_back(xref_ptr<CDependencyObject>(&tr0));

        CTransformGroup tr2;
        CTransformCollection c2;
        tr2.m_pChild = &c2;
        tr2.m_pChild->push_back(xref_ptr<CDependencyObject>(&tr1));

        CTransformGroup tr3;
        CTransformCollection c3;
        tr3.m_pChild = &c3;
        tr3.m_pChild->push_back(xref_ptr<CDependencyObject>(&tr2));

        tr3.MakeWinRTExpression(&context);
        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = tr3.GetWinRTExpression();

        // tr3: "PS.Expr0", Expr0 -> tr2
        // tr2: "PS.Expr0", Expr0 -> tr1
        // tr1: "PS.Expr0", Expr0 -> tr0
        // tr0: sc_Expression_Translate
        WinRTExpressionUnitTestHelper::VerifyExpressionString(pWinRTExpressionNoRef, L"PS.Expr0");

        std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap_tr3 = WinRTExpressionUnitTestHelper::GetPropertyInfoMap(pWinRTExpressionNoRef);
        {
            auto it = propertyInfoMap_tr3.find(std::wstring(L"Expr0"));
            ASSERT(it != propertyInfoMap_tr3.end());
            const MockDComp::PropertyInfo& propertyInfo_tr2 = it->second;

            VERIFY_IS_TRUE(propertyInfo_tr2.propertyValueType == MockDComp::CompositionValueType::Matrix3x2);
            VERIFY_IS_TRUE(propertyInfo_tr2.m_animation != nullptr);

            Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> spExpression_tr2;
            propertyInfo_tr2.m_animation.As(&spExpression_tr2);

            std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap_tr2 = WinRTExpressionUnitTestHelper::GetPropertyInfoMap(spExpression_tr2.Get());
            {
                auto it2 = propertyInfoMap_tr2.find(std::wstring(L"Expr0"));
                ASSERT(it2 != propertyInfoMap_tr2.end());
                const MockDComp::PropertyInfo& propertyInfo_tr1 = it2->second;

                VERIFY_IS_TRUE(propertyInfo_tr1.propertyValueType == MockDComp::CompositionValueType::Matrix3x2);
                VERIFY_IS_TRUE(propertyInfo_tr1.m_animation != nullptr);

                Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> spExpression_tr1;
                propertyInfo_tr1.m_animation.As(&spExpression_tr1);

                std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap_tr1 = WinRTExpressionUnitTestHelper::GetPropertyInfoMap(spExpression_tr1.Get());
                {
                    auto it3 = propertyInfoMap_tr1.find(std::wstring(L"Expr0"));
                    ASSERT(it3 != propertyInfoMap_tr1.end());
                    const MockDComp::PropertyInfo& propertyInfo_tr0 = it3->second;

                    VERIFY_IS_TRUE(propertyInfo_tr0.propertyValueType == MockDComp::CompositionValueType::Matrix3x2);
                    VERIFY_IS_TRUE(propertyInfo_tr0.m_animation != nullptr);

                    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> spExpression_tr0;
                    propertyInfo_tr1.m_animation.As(&spExpression_tr0);

                    std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap_tr0 = WinRTExpressionUnitTestHelper::GetPropertyInfoMap(spExpression_tr0.Get());
                    {
                        auto it4 = propertyInfoMap_tr0.find(std::wstring(L"Expr0"));
                        ASSERT(it4 != propertyInfoMap_tr0.end());
                        const MockDComp::PropertyInfo& propertyInfo_leaf = it4->second;

                        VERIFY_IS_TRUE(propertyInfo_leaf.propertyValueType == MockDComp::CompositionValueType::Matrix3x2);
                        VERIFY_IS_TRUE(propertyInfo_leaf.m_animation != nullptr);

                        Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> spExpression_leaf;
                        propertyInfo_leaf.m_animation.As(&spExpression_leaf);

                        WinRTExpressionUnitTestHelper::VerifyTranslateTransform(
                            spExpression_leaf.Get(),
                            ExpressionHelper::sc_Expression_Translate,
                            43.21f, 12.34f,
                            nullptr, nullptr
                            );
                    }
                }
            }
        }
    }

    {
        LOG_OUTPUT(L"Transform group containing blank composite transform");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();
        CCompositeTransform tr0;

        CTransformGroup tr1;
        CTransformCollection c1;
        tr1.m_pChild = &c1;
        tr1.m_pChild->push_back(xref_ptr<CDependencyObject>(&tr0));

        tr1.MakeWinRTExpression(&context);
        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = tr1.GetWinRTExpression();

        WinRTExpressionUnitTestHelper::VerifyExpressionString(pWinRTExpressionNoRef, L"PS.Expr0");

        std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap = WinRTExpressionUnitTestHelper::GetPropertyInfoMap(pWinRTExpressionNoRef);
        {
            auto it = propertyInfoMap.find(std::wstring(L"Expr0"));
            ASSERT(it != propertyInfoMap.end());
            const MockDComp::PropertyInfo& propertyInfo = it->second;

            VERIFY_IS_TRUE(propertyInfo.propertyValueType == MockDComp::CompositionValueType::Matrix3x2);
            VERIFY_IS_TRUE(propertyInfo.m_animation != nullptr);

            Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> spExpression;
            propertyInfo.m_animation.As(&spExpression);

            std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap_compositeTransform = WinRTExpressionUnitTestHelper::GetPropertyInfoMap(spExpression.Get());
            WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap_compositeTransform, L"X", 0.0f, nullptr);
            WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap_compositeTransform, L"Y", 0.0f, nullptr);
            WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap_compositeTransform, L"ScaleX", 1.0f, nullptr);
            WinRTExpressionUnitTestHelper::VerifyScalarProperty(propertyInfoMap_compositeTransform, L"ScaleY", 1.0f, nullptr);
        }
    }
}

void TransformGroupUnitTestsWUC::ValidateDeviceLostCleanupWUC()
{
    CTranslateTransform tr0;
    CTranslateTransform tr1;
    CScaleTransform tr2;

    CTransformGroup transform;
    CTransformCollection collection;
    transform.m_pChild = &collection;
    transform.m_pChild->push_back(xref_ptr<CDependencyObject>(&tr0));
    transform.m_pChild->push_back(xref_ptr<CDependencyObject>(&tr1));
    transform.m_pChild->push_back(xref_ptr<CDependencyObject>(&tr2));

    WUComp::IExpressionAnimation* expression0NoRef;
    WUComp::IExpressionAnimation* expression1NoRef;
    WUComp::IExpressionAnimation* expression2NoRef;

    {
        LOG_OUTPUT(L"Create DComp resource");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();
        transform.MakeWinRTExpression(&context);
        VERIFY_IS_NOT_NULL(transform.GetWinRTExpression());
        // The pieces have been validated in ValidateWinRTExpression. No need to check them again.

        expression0NoRef = tr0.GetWinRTExpression();
        expression1NoRef = tr1.GetWinRTExpression();
        expression2NoRef = tr2.GetWinRTExpression();

        VERIFY_IS_NOT_NULL(expression0NoRef);
        VERIFY_IS_NOT_NULL(expression1NoRef);
        VERIFY_IS_NOT_NULL(expression2NoRef);
    }

    {
        LOG_OUTPUT(L"Clean up after device lost");
        transform.ReleaseDCompResources();
        VERIFY_IS_NULL(transform.GetWinRTExpression());

        // The components should still be the same as before
        VERIFY_ARE_EQUAL(expression0NoRef, tr0.GetWinRTExpression());
        VERIFY_ARE_EQUAL(expression1NoRef, tr1.GetWinRTExpression());
        VERIFY_ARE_EQUAL(expression2NoRef, tr2.GetWinRTExpression());
    }
}

} } } } } }
