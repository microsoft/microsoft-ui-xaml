// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlLogging.h"
#include "MatrixTransformUnitTests.h"
#include "MatrixTransform.h"
#include "WinRTExpressionUnitTestHelper.h"
#include "ExpressionHelper.h"
#include "CMatrix.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Transforms {

void MatrixTransformUnitTestsWUC::ValidateWinRTExpression()
{

    {
        LOG_OUTPUT(L"Blank Transform");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        CMatrixTransform blankTransform;
        blankTransform.MakeWinRTExpression(&context);

        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = blankTransform.GetWinRTExpression();
        WinRTExpressionUnitTestHelper::VerifyMatrixTransform(
            pWinRTExpressionNoRef,
            ExpressionHelper::sc_Expression_MatrixTransform,
            1, 0, 0, 1, 0, 0
            );
    }

    {
        LOG_OUTPUT(L"Static entries");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        CMatrix doMatrix;
        doMatrix.m_matrix._11 = 11;
        doMatrix.m_matrix._12 = 12;
        doMatrix.m_matrix._21 = 21;
        doMatrix.m_matrix._22 = 22;
        doMatrix.m_matrix._31 = 31;
        doMatrix.m_matrix._32 = 32;

        CMatrixTransform transform;
        transform.m_pMatrix = &doMatrix;
        transform.MakeWinRTExpression(&context);

        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();
        WinRTExpressionUnitTestHelper::VerifyMatrixTransform(
            pWinRTExpressionNoRef,
            ExpressionHelper::sc_Expression_MatrixTransform,
            11, 12, 21, 22, 31, 32
            );
    }
}

void MatrixTransformUnitTestsWUC::ValidateDeviceLostCleanupWUC()
{
    CMatrixTransform transform;

    {
        LOG_OUTPUT(L"Create DComp resource");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();
        transform.MakeWinRTExpression(&context);
        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();
        WinRTExpressionUnitTestHelper::VerifyMatrixTransform(
            pWinRTExpressionNoRef,
            ExpressionHelper::sc_Expression_MatrixTransform,
            1, 0, 0, 1, 0, 0);
    }

    {
        LOG_OUTPUT(L"Clean up after device lost");
        transform.ReleaseDCompResources();
        VERIFY_IS_NULL(transform.GetWinRTExpression());
    }
}

} } } } } }
