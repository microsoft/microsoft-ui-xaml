// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlLogging.h"
#include "ScaleTransformUnitTests.h"
#include "ScaleTransform.h"
#include "WinRTExpressionUnitTestHelper.h"
#include "ExpressionHelper.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Transforms {

void ScaleTransformUnitTestsWUC::ValidateWinRTExpression()
{
    {
        LOG_OUTPUT(L"Static X, static Y, static center");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        CScaleTransform transform;
        transform.m_ptCenter.x = 456.789f;
        transform.m_ptCenter.y = 357.9f;
        transform.m_eScaleX = -100.98f;
        transform.m_eScaleY = 234.56f;

        transform.MakeWinRTExpression(&context);

        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();
        WinRTExpressionUnitTestHelper::VerifyScaleTransform(
            pWinRTExpressionNoRef,
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
        LOG_OUTPUT(L"Animated X, static Y, static center");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        ScalarKFAHelper scalarKFA1(context);
        WUComp::ICompositionAnimation* pAnim1NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA1);

        CScaleTransform transform;
        transform.m_ptCenter.x = 456.789f;
        transform.m_ptCenter.y = 357.9f;
        transform.SetDCompAnimation(pAnim1NoRef, KnownPropertyIndex::ScaleTransform_ScaleXAnimation);
        transform.m_isScaleXAnimationDirty = true;
        transform.m_eScaleY = 234.56f;

        transform.MakeWinRTExpression(&context);

        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();
        WinRTExpressionUnitTestHelper::VerifyScaleTransform(
            pWinRTExpressionNoRef,
            ExpressionHelper::sc_Expression_Scale_CenterPoint,
            true, /* hasCenterPoint */
            456.789f,
            357.9f,
            0,
            234.56f,
            nullptr,
            nullptr,
            pAnim1NoRef,
            nullptr
            );
    }

    {
        LOG_OUTPUT(L"Static X, animated Y, static center");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        ScalarKFAHelper scalarKFA2(context);
        WUComp::ICompositionAnimation* pAnim2NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA2);

        CScaleTransform transform;
        transform.m_ptCenter.x = 456.789f;
        transform.m_ptCenter.y = 357.9f;
        transform.m_eScaleX = -100.98f;
        transform.SetDCompAnimation(pAnim2NoRef, KnownPropertyIndex::ScaleTransform_ScaleYAnimation);
        transform.m_isScaleYAnimationDirty = true;

        transform.MakeWinRTExpression(&context);

        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();
        WinRTExpressionUnitTestHelper::VerifyScaleTransform(
            pWinRTExpressionNoRef,
            ExpressionHelper::sc_Expression_Scale_CenterPoint,
            true, /* hasCenterPoint */
            456.789f,
            357.9f,
            -100.98f,
            0,
            nullptr,
            nullptr,
            nullptr,
            pAnim2NoRef
            );
    }

    {
        LOG_OUTPUT(L"Static X, static Y, animated center");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        ScalarKFAHelper scalarKFA3(context);
        WUComp::ICompositionAnimation* pAnim3NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA3);

        CScaleTransform transform;
        transform.SetDCompAnimation(pAnim3NoRef, KnownPropertyIndex::ScaleTransform_CenterXAnimation);
        transform.m_isCenterXAnimationDirty = true;
        transform.m_ptCenter.y = 357.9f;
        transform.m_eScaleX = -100.98f;
        transform.m_eScaleY = 234.56f;

        transform.MakeWinRTExpression(&context);

        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();
        WinRTExpressionUnitTestHelper::VerifyScaleTransform(
            pWinRTExpressionNoRef,
            ExpressionHelper::sc_Expression_Scale_CenterPoint,
            true, /* hasCenterPoint */
            0,
            357.9f,
            -100.98f,
            234.56f,
            pAnim3NoRef,
            nullptr,
            nullptr,
            nullptr
            );
    }

    {
        LOG_OUTPUT(L"Animated X, animated Y, animated center");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        ScalarKFAHelper scalarKFA1(context);
        WUComp::ICompositionAnimation* pAnim1NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA1);

        ScalarKFAHelper scalarKFA2(context);
        WUComp::ICompositionAnimation* pAnim2NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA2);

        ScalarKFAHelper scalarKFA3(context);
        WUComp::ICompositionAnimation* pAnim3NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA3);

        ScalarKFAHelper scalarKFA4(context);
        WUComp::ICompositionAnimation* pAnim4NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA4);

        CScaleTransform transform;
        transform.SetDCompAnimation(pAnim3NoRef, KnownPropertyIndex::ScaleTransform_CenterXAnimation);
        transform.m_isCenterXAnimationDirty = true;
        transform.SetDCompAnimation(pAnim4NoRef, KnownPropertyIndex::ScaleTransform_CenterYAnimation);
        transform.m_isCenterYAnimationDirty = true;
        transform.SetDCompAnimation(pAnim1NoRef, KnownPropertyIndex::ScaleTransform_ScaleXAnimation);
        transform.m_isScaleXAnimationDirty = true;
        transform.SetDCompAnimation(pAnim2NoRef, KnownPropertyIndex::ScaleTransform_ScaleYAnimation);
        transform.m_isScaleYAnimationDirty = true;

        transform.MakeWinRTExpression(&context);

        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();
        WinRTExpressionUnitTestHelper::VerifyScaleTransform(
            pWinRTExpressionNoRef,
            ExpressionHelper::sc_Expression_Scale_CenterPoint,
            true, /* hasCenterPoint */
            0,
            0,
            0,
            0,
            pAnim3NoRef,
            pAnim4NoRef,
            pAnim1NoRef,
            pAnim2NoRef
            );
    }
}

void ScaleTransformUnitTestsWUC::ValidateDeviceLostCleanupWUC()
{
    CScaleTransform transform;

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

        transform.SetDCompAnimation(pAnim3NoRef, KnownPropertyIndex::ScaleTransform_CenterXAnimation);
        transform.m_isCenterXAnimationDirty = true;
        transform.SetDCompAnimation(pAnim4NoRef, KnownPropertyIndex::ScaleTransform_CenterYAnimation);
        transform.m_isCenterYAnimationDirty = true;
        transform.SetDCompAnimation(pAnim1NoRef, KnownPropertyIndex::ScaleTransform_ScaleXAnimation);
        transform.m_isScaleXAnimationDirty = true;
        transform.SetDCompAnimation(pAnim2NoRef, KnownPropertyIndex::ScaleTransform_ScaleYAnimation);
        transform.m_isScaleYAnimationDirty = true;

        // Create and verify the expression
        transform.MakeWinRTExpression(&context);
        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();
        WinRTExpressionUnitTestHelper::VerifyScaleTransform(
            pWinRTExpressionNoRef,
            ExpressionHelper::sc_Expression_Scale_CenterPoint,
            true, /* hasCenterPoint */
            0, 0, 0, 0,
            pAnim3NoRef, pAnim4NoRef, pAnim1NoRef, pAnim2NoRef);
    }

    {
        LOG_OUTPUT(L"Clean up after device lost");
        transform.ReleaseDCompResources();
        VERIFY_IS_NULL(transform.GetWinRTExpression());
        VERIFY_IS_NULL(transform.GetDCompAnimation(KnownPropertyIndex::ScaleTransform_CenterXAnimation));
        VERIFY_IS_NULL(transform.GetDCompAnimation(KnownPropertyIndex::ScaleTransform_CenterYAnimation));
        VERIFY_IS_NULL(transform.GetDCompAnimation(KnownPropertyIndex::ScaleTransform_ScaleXAnimation));
        VERIFY_IS_NULL(transform.GetDCompAnimation(KnownPropertyIndex::ScaleTransform_ScaleYAnimation));
    }
}

} } } } } }
