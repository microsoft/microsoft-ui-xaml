// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlLogging.h"
#include "RotateTransformUnitTests.h"
#include "RotateTransform.h"
#include "WinRTExpressionUnitTestHelper.h"
#include "ExpressionHelper.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Transforms {

void RotateTransformUnitTestsWUC::ValidateWinRTExpression()
{
    {
        LOG_OUTPUT(L"Static angle, static center");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        CRotateTransform transform;
        transform.m_ptCenter.x = 456.789f;
        transform.m_ptCenter.y = 357.9f;
        transform.m_eAngle = 246.80f;

        transform.MakeWinRTExpression(&context);

        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();
        WinRTExpressionUnitTestHelper::VerifyRotateTransform(
            pWinRTExpressionNoRef,
            ExpressionHelper::sc_Expression_Rotate_CenterPoint,
            true, /* hasCenterPoint */
            456.789f,
            357.9f,
            246.80f,
            nullptr,
            nullptr,
            nullptr
            );
    }

    {
        LOG_OUTPUT(L"Animated angle, static center");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        ScalarKFAHelper scalarKFA1(context);
        WUComp::ICompositionAnimation* pAnim1NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA1);

        CRotateTransform transform;
        transform.m_ptCenter.x = 456.789f;
        transform.m_ptCenter.y = 357.9f;
        transform.SetDCompAnimation(pAnim1NoRef, KnownPropertyIndex::RotateTransform_AngleAnimation);
        transform.m_isAngleAnimationDirty = true;

        transform.MakeWinRTExpression(&context);

        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();
        WinRTExpressionUnitTestHelper::VerifyRotateTransform(
            pWinRTExpressionNoRef,
            ExpressionHelper::sc_Expression_Rotate_CenterPoint,
            true, /* hasCenterPoint */
            456.789f,
            357.9f,
            0,
            nullptr,
            nullptr,
            pAnim1NoRef
            );
    }

    {
        LOG_OUTPUT(L"Static angle, animated center");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        ScalarKFAHelper scalarKFA2(context);
        WUComp::ICompositionAnimation* pAnim2NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA2);

        CRotateTransform transform;
        transform.SetDCompAnimation(pAnim2NoRef, KnownPropertyIndex::RotateTransform_CenterXAnimation);
        transform.m_isCenterXAnimationDirty = true;
        transform.m_ptCenter.y = 357.9f;
        transform.m_eAngle = 246.80f;

        transform.MakeWinRTExpression(&context);

        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();
        WinRTExpressionUnitTestHelper::VerifyRotateTransform(
            pWinRTExpressionNoRef,
            ExpressionHelper::sc_Expression_Rotate_CenterPoint,
            true, /* hasCenterPoint */
            0,
            357.9f,
            246.80f,
            pAnim2NoRef,
            nullptr,
            nullptr
            );
    }

    {
        LOG_OUTPUT(L"Animated angle, animated center");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        ScalarKFAHelper scalarKFA1(context);
        WUComp::ICompositionAnimation* pAnim1NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA1);

        ScalarKFAHelper scalarKFA2(context);
        WUComp::ICompositionAnimation* pAnim2NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA2);

        ScalarKFAHelper scalarKFA3(context);
        WUComp::ICompositionAnimation* pAnim3NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA3);

        CRotateTransform transform;
        transform.SetDCompAnimation(pAnim2NoRef, KnownPropertyIndex::RotateTransform_CenterXAnimation);
        transform.m_isCenterXAnimationDirty = true;
        transform.SetDCompAnimation(pAnim3NoRef, KnownPropertyIndex::RotateTransform_CenterYAnimation);
        transform.m_isCenterYAnimationDirty = true;
        transform.SetDCompAnimation(pAnim1NoRef, KnownPropertyIndex::RotateTransform_AngleAnimation);
        transform.m_isAngleAnimationDirty = true;

        transform.MakeWinRTExpression(&context);

        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();
        WinRTExpressionUnitTestHelper::VerifyRotateTransform(
            pWinRTExpressionNoRef,
            ExpressionHelper::sc_Expression_Rotate_CenterPoint,
            true, /* hasCenterPoint */
            0,
            0,
            0,
            pAnim2NoRef,
            pAnim3NoRef,
            pAnim1NoRef
            );
    }
}

void RotateTransformUnitTestsWUC::ValidateDeviceLostCleanupWUC()
{
    CRotateTransform transform;

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
        WUComp::ICompositionAnimation* pAnim3NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA2);

        transform.SetDCompAnimation(pAnim2NoRef, KnownPropertyIndex::RotateTransform_CenterXAnimation);
        transform.m_isCenterXAnimationDirty = true;
        transform.SetDCompAnimation(pAnim3NoRef, KnownPropertyIndex::RotateTransform_CenterYAnimation);
        transform.m_isCenterYAnimationDirty = true;
        transform.SetDCompAnimation(pAnim1NoRef, KnownPropertyIndex::RotateTransform_AngleAnimation);
        transform.m_isAngleAnimationDirty = true;

        // Create and verify the expression
        transform.MakeWinRTExpression(&context);
        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();
        WinRTExpressionUnitTestHelper::VerifyRotateTransform(
            pWinRTExpressionNoRef,
            ExpressionHelper::sc_Expression_Rotate_CenterPoint,
            true, /* hasCenterPoint */
            0, 0, 0,
            pAnim2NoRef, pAnim3NoRef, pAnim1NoRef);
    }

    {
        LOG_OUTPUT(L"Clean up after device lost");
        transform.ReleaseDCompResources();
        VERIFY_IS_NULL(transform.GetWinRTExpression());
        VERIFY_IS_NULL(transform.GetDCompAnimation(KnownPropertyIndex::RotateTransform_CenterXAnimation));
        VERIFY_IS_NULL(transform.GetDCompAnimation(KnownPropertyIndex::RotateTransform_CenterYAnimation));
        VERIFY_IS_NULL(transform.GetDCompAnimation(KnownPropertyIndex::RotateTransform_AngleAnimation));
    }
}

} } } } } }
