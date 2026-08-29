// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlLogging.h"
#include "SkewTransformUnitTests.h"
#include "SkewTransform.h"
#include "WinRTExpressionUnitTestHelper.h"
#include "ExpressionHelper.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Transforms {

void SkewTransformUnitTestsWUC::ValidateWinRTExpression()
{
    {
        LOG_OUTPUT(L"Static angle X, static angle Y, static center");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        CSkewTransform transform;
        transform.m_ptCenter.x = 456.789f;
        transform.m_ptCenter.y = 357.9f;
        transform.m_eAngleX = -97.53f;
        transform.m_eAngleY = 1.357f;

        transform.MakeWinRTExpression(&context);

        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();
        WinRTExpressionUnitTestHelper::VerifySkewTransform(
            pWinRTExpressionNoRef,
            ExpressionHelper::sc_Expression_Skew_CenterPoint,
            true, /* hasCenterPoint */
            456.789f,
            357.9f,
            -97.53f,
            1.357f,
            nullptr,
            nullptr,
            nullptr,
            nullptr
            );
    }

    {
        LOG_OUTPUT(L"Animated angle X, static angle Y, static center");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        ScalarKFAHelper scalarKFA1(context);
        WUComp::ICompositionAnimation* pAnim1NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA1);

        CSkewTransform transform;
        transform.m_ptCenter.x = 456.789f;
        transform.m_ptCenter.y = 357.9f;
        transform.SetDCompAnimation(pAnim1NoRef, KnownPropertyIndex::SkewTransform_AngleXAnimation);
        transform.m_isAngleXAnimationDirty = true;
        transform.m_eAngleY = 1.357f;

        transform.MakeWinRTExpression(&context);

        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();
        WinRTExpressionUnitTestHelper::VerifySkewTransform(
            pWinRTExpressionNoRef,
            ExpressionHelper::sc_Expression_Skew_CenterPoint,
            true, /* hasCenterPoint */
            456.789f,
            357.9f,
            0,
            1.357f,
            nullptr,
            nullptr,
            pAnim1NoRef,
            nullptr
            );
    }

    {
        LOG_OUTPUT(L"Static angle X, animated angle Y, static center");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        ScalarKFAHelper scalarKFA2(context);
        WUComp::ICompositionAnimation* pAnim2NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA2);

        CSkewTransform transform;
        transform.m_ptCenter.x = 456.789f;
        transform.m_ptCenter.y = 357.9f;
        transform.m_eAngleX = -97.53f;
        transform.SetDCompAnimation(pAnim2NoRef, KnownPropertyIndex::SkewTransform_AngleYAnimation);
        transform.m_isAngleYAnimationDirty = true;

        transform.MakeWinRTExpression(&context);

        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();
        WinRTExpressionUnitTestHelper::VerifySkewTransform(
            pWinRTExpressionNoRef,
            ExpressionHelper::sc_Expression_Skew_CenterPoint,
            true, /* hasCenterPoint */
            456.789f,
            357.9f,
            -97.53f,
            0,
            nullptr,
            nullptr,
            nullptr,
            pAnim2NoRef
            );
    }

    {
        LOG_OUTPUT(L"Static angle X, static angle Y, animated center");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        ScalarKFAHelper scalarKFA3(context);
        WUComp::ICompositionAnimation* pAnim3NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA3);

        CSkewTransform transform;
        transform.m_ptCenter.x = 456.789f;
        transform.SetDCompAnimation(pAnim3NoRef, KnownPropertyIndex::SkewTransform_CenterYAnimation);
        transform.m_isCenterYAnimationDirty = true;
        transform.m_eAngleX = -97.53f;
        transform.m_eAngleY = 1.357f;

        transform.MakeWinRTExpression(&context);

        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();
        WinRTExpressionUnitTestHelper::VerifySkewTransform(
            pWinRTExpressionNoRef,
            ExpressionHelper::sc_Expression_Skew_CenterPoint,
            true, /* hasCenterPoint */
            456.789f,
            0,
            -97.53f,
            1.357f,
            nullptr,
            pAnim3NoRef,
            nullptr,
            nullptr
            );
    }

    {
        LOG_OUTPUT(L"Animated angle X, animated angle Y, animated center");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        ScalarKFAHelper scalarKFA1(context);
        WUComp::ICompositionAnimation* pAnim1NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA1);

        ScalarKFAHelper scalarKFA2(context);
        WUComp::ICompositionAnimation* pAnim2NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA2);

        ScalarKFAHelper scalarKFA3(context);
        WUComp::ICompositionAnimation* pAnim3NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA3);

        ScalarKFAHelper scalarKFA4(context);
        WUComp::ICompositionAnimation* pAnim4NoRef = static_cast<WUComp::ICompositionAnimation*>(scalarKFA4);

        CSkewTransform transform;
        transform.SetDCompAnimation(pAnim3NoRef, KnownPropertyIndex::SkewTransform_CenterXAnimation);
        transform.m_isCenterXAnimationDirty = true;
        transform.SetDCompAnimation(pAnim4NoRef, KnownPropertyIndex::SkewTransform_CenterYAnimation);
        transform.m_isCenterYAnimationDirty = true;
        transform.SetDCompAnimation(pAnim1NoRef, KnownPropertyIndex::SkewTransform_AngleXAnimation);
        transform.m_isAngleXAnimationDirty = true;
        transform.SetDCompAnimation(pAnim2NoRef, KnownPropertyIndex::SkewTransform_AngleYAnimation);
        transform.m_isAngleYAnimationDirty = true;

        transform.MakeWinRTExpression(&context);

        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();
        WinRTExpressionUnitTestHelper::VerifySkewTransform(
            pWinRTExpressionNoRef,
            ExpressionHelper::sc_Expression_Skew_CenterPoint,
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

void SkewTransformUnitTestsWUC::ValidateDeviceLostCleanupWUC()
{
    CSkewTransform transform;

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

        transform.SetDCompAnimation(pAnim3NoRef, KnownPropertyIndex::SkewTransform_CenterXAnimation);
        transform.m_isCenterXAnimationDirty = true;
        transform.SetDCompAnimation(pAnim4NoRef, KnownPropertyIndex::SkewTransform_CenterYAnimation);
        transform.m_isCenterYAnimationDirty = true;
        transform.SetDCompAnimation(pAnim1NoRef, KnownPropertyIndex::SkewTransform_AngleXAnimation);
        transform.m_isAngleXAnimationDirty = true;
        transform.SetDCompAnimation(pAnim2NoRef, KnownPropertyIndex::SkewTransform_AngleYAnimation);
        transform.m_isAngleYAnimationDirty = true;

        // Create and verify the expression
        transform.MakeWinRTExpression(&context);
        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();
        WinRTExpressionUnitTestHelper::VerifySkewTransform(
            pWinRTExpressionNoRef,
            ExpressionHelper::sc_Expression_Skew_CenterPoint,
            true, /* hasCenterPoint */
            0, 0, 0, 0,
            pAnim3NoRef, pAnim4NoRef, pAnim1NoRef, pAnim2NoRef);
    }

    {
        LOG_OUTPUT(L"Clean up after device lost");
        transform.ReleaseDCompResources();
        VERIFY_IS_NULL(transform.GetWinRTExpression());
        VERIFY_IS_NULL(transform.GetDCompAnimation(KnownPropertyIndex::SkewTransform_CenterXAnimation));
        VERIFY_IS_NULL(transform.GetDCompAnimation(KnownPropertyIndex::SkewTransform_CenterYAnimation));
        VERIFY_IS_NULL(transform.GetDCompAnimation(KnownPropertyIndex::SkewTransform_AngleXAnimation));
        VERIFY_IS_NULL(transform.GetDCompAnimation(KnownPropertyIndex::SkewTransform_AngleYAnimation));
    }
}

} } } } } }
