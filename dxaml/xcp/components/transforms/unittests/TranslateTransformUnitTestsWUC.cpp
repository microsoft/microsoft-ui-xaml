// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlLogging.h"
#include "TranslateTransformUnitTests.h"
#include "TranslateTransform.h"
#include "ExpressionHelper.h"
#include "WinRTExpressionUnitTestHelper.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Transforms {

void TranslateTransformUnitTestsWUC::ValidateWinRTExpression()
{
    {
        LOG_OUTPUT(L"Static X, static Y");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        CTranslateTransform transform;
        transform.m_eX = -100.98f;
        transform.m_eY = -5.0f;

        transform.MakeWinRTExpression(&context);

        transform.m_isWinRTExpressionDirty = true;
        transform.m_eY = 234.56f;
        transform.MakeWinRTExpression(&context);

        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();
        WinRTExpressionUnitTestHelper::VerifyTranslateTransform(
            pWinRTExpressionNoRef,
            ExpressionHelper::sc_Expression_Translate,
            -100.98f,
            234.56f,
            nullptr,
            nullptr
            );
    }
    {
        LOG_OUTPUT(L"Animated X, static Y");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        ScalarKFAHelper scalarKFA1(context);
        WUComp::ICompositionAnimation* pAnim1NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA1);

        CTranslateTransform transform;
        transform.SetDCompAnimation(pAnim1NoRef, KnownPropertyIndex::TranslateTransform_XAnimation);
        transform.m_isXAnimationDirty = true;
        transform.m_eY = 234.56f;

        transform.MakeWinRTExpression(&context);

        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();
        WinRTExpressionUnitTestHelper::VerifyTranslateTransform(
            pWinRTExpressionNoRef,
            ExpressionHelper::sc_Expression_Translate,
            0,
            234.56f,
            pAnim1NoRef,
            nullptr
            );
    }

    {
        LOG_OUTPUT(L"Static X, animated Y");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        ScalarKFAHelper scalarKFA2(context);
        WUComp::ICompositionAnimation* pAnim2NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA2);

        CTranslateTransform transform;
        transform.m_eX = -100.98f;
        transform.SetDCompAnimation(pAnim2NoRef, KnownPropertyIndex::TranslateTransform_YAnimation);
        transform.m_isYAnimationDirty = true;

        transform.MakeWinRTExpression(&context);

        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();
        WinRTExpressionUnitTestHelper::VerifyTranslateTransform(
            pWinRTExpressionNoRef,
            ExpressionHelper::sc_Expression_Translate,
            -100.98f,
            0,
            nullptr,
            pAnim2NoRef
            );
    }

    {
        LOG_OUTPUT(L"Animated X, animated Y");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        ScalarKFAHelper scalarKFA1(context);
        WUComp::ICompositionAnimation* pAnim1NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA1);

        ScalarKFAHelper scalarKFA2(context);
        WUComp::ICompositionAnimation* pAnim2NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA2);

        CTranslateTransform transform;
        transform.SetDCompAnimation(pAnim1NoRef, KnownPropertyIndex::TranslateTransform_XAnimation);
        transform.m_isXAnimationDirty = true;
        transform.SetDCompAnimation(pAnim2NoRef, KnownPropertyIndex::TranslateTransform_YAnimation);
        transform.m_isYAnimationDirty = true;

        transform.MakeWinRTExpression(&context);

        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();
        WinRTExpressionUnitTestHelper::VerifyTranslateTransform(
            pWinRTExpressionNoRef,
            ExpressionHelper::sc_Expression_Translate,
            0,
            0,
            pAnim1NoRef,
            pAnim2NoRef
            );
    }
}

void TranslateTransformUnitTestsWUC::ValidateDeviceLostCleanupWUC()
{
    CTranslateTransform transform;

    {
        // Create the DComp context in a block so it is cleaned up / lost at the end of this block
        LOG_OUTPUT(L"Create DComp resource");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        // Create and set the animations
        ScalarKFAHelper scalarKFA1(context);
        WUComp::ICompositionAnimation* pAnim1NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA1);

        ScalarKFAHelper scalarKFA2(context);
        WUComp::ICompositionAnimation* pAnim2NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA2);

        transform.SetDCompAnimation(pAnim1NoRef, KnownPropertyIndex::TranslateTransform_XAnimation);
        transform.m_isXAnimationDirty = true;
        transform.SetDCompAnimation(pAnim2NoRef, KnownPropertyIndex::TranslateTransform_YAnimation);
        transform.m_isYAnimationDirty = true;

        // Create and verify the expression
        transform.MakeWinRTExpression(&context);
        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();
        WinRTExpressionUnitTestHelper::VerifyTranslateTransform(
            pWinRTExpressionNoRef,
            ExpressionHelper::sc_Expression_Translate,
            0, 0, pAnim1NoRef, pAnim2NoRef);
    }

    {
        LOG_OUTPUT(L"Clean up after device lost");
        transform.ReleaseDCompResources();
        VERIFY_IS_NULL(transform.GetWinRTExpression());
        VERIFY_IS_NULL(transform.GetDCompAnimation(KnownPropertyIndex::TranslateTransform_XAnimation));
        VERIFY_IS_NULL(transform.GetDCompAnimation(KnownPropertyIndex::TranslateTransform_YAnimation));
    }
}

} } } } } }
