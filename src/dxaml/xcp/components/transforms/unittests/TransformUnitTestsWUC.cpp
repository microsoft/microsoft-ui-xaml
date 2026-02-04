// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlLogging.h"
#include "TransformUnitTests.h"
#include "CompositeTransform.h"
#include "WinRTExpressionUnitTestHelper.h"
#include "ExpressionHelper.h"
#include "Matrix.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Transforms {

void TransformUnitTestsWUC::ValidateWinRTExpressionWithOrigin()
{
    {
        LOG_OUTPUT(L"No origin");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> overallExpression;

        CCompositeTransform transform;
        transform.m_eTranslateX = 10;

        transform.MakeWinRTExpressionWithOrigin(
            0,
            0,
            &context,
            overallExpression.GetAddressOf()
            );

        WUComp::IExpressionAnimation *pWinRTExpressionNoRef = transform.GetWinRTExpression();

        WinRTExpressionUnitTestHelper::VerifyExpressionString(overallExpression.Get(), ExpressionHelper::sc_Expression_CompositeTransform_ScaleTranslate);
        VERIFY_ARE_EQUAL(pWinRTExpressionNoRef, overallExpression.Get());
    }

    {
        LOG_OUTPUT(L"With origin");
        WinRTExpressionConversionContext context = WinRTExpressionUnitTestHelper::MakeContext();

        Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> overallExpression;
        CCompositeTransform transform;
        transform.m_eTranslateX = 10;

        transform.MakeWinRTExpressionWithOrigin(
            12,
            24,
            &context,
            overallExpression.GetAddressOf()
            );

        WinRTExpressionUnitTestHelper::VerifyExpressionString(overallExpression.Get(), ExpressionHelper::sc_RtoFull3x2);
        WinRTExpressionUnitTestHelper::VerifyRenderTransformOrigin(overallExpression.Get(), 12, 24);

        std::map<std::wstring, MockDComp::PropertyInfo>& propertyInfoMap = WinRTExpressionUnitTestHelper::GetPropertyInfoMap(overallExpression.Get());

        {
            auto it = propertyInfoMap.find(std::wstring(L"Expression"));
            ASSERT(it != propertyInfoMap.end());
            const MockDComp::PropertyInfo& propertyInfo = it->second;

            VERIFY_IS_TRUE(propertyInfo.propertyValueType == MockDComp::CompositionValueType::Matrix3x2);
            VERIFY_IS_TRUE(propertyInfo.m_animation != nullptr);

            Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> spExpression;
            propertyInfo.m_animation.As(&spExpression);

            WinRTExpressionUnitTestHelper::VerifyExpressionString(spExpression.Get(), ExpressionHelper::sc_Expression_CompositeTransform_ScaleTranslate);
        }
    }
}

} } } } } }
