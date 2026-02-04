// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlLogging.h"
#include "KeySplineUnitTests.h"
#include "KeySpline.h"
#include "DCompAnimationUnitTestHelper.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Animation {

void KeySplineUnitTests::ValidateGetSplineProgress()
{
    {
        LOG_OUTPUT(L"Blank spline - linear");
        CKeySpline spline;

        // Due to interpolation, these don't lie exactly on the line y=x.
        VERIFY_ARE_EQUAL(0, spline.GetSplineProgress(0));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.2f, 0.01f, spline.GetSplineProgress(0.2f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.4f, 0.01f, spline.GetSplineProgress(0.4f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.6f, 0.01f, spline.GetSplineProgress(0.6f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.8f, 0.01f, spline.GetSplineProgress(0.8f));
        VERIFY_ARE_EQUAL(1, spline.GetSplineProgress(1));
    }

    {
        LOG_OUTPUT(L"(0,1) (1,0) - sharp rise in the beginning, hold steady, sharp rise at the end");
        CKeySpline spline;
        spline.m_ControlPoint1.x = 0;
        spline.m_ControlPoint1.y = 1;
        spline.m_ControlPoint2.x = 1;
        spline.m_ControlPoint2.y = 0;

        VERIFY_ARE_EQUAL(0, spline.GetSplineProgress(0));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.3063f, 0.0001f, spline.GetSplineProgress(0.05f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.3876f, 0.0001f, spline.GetSplineProgress(0.1f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.4615f, 0.0001f, spline.GetSplineProgress(0.2f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.4897f, 0.0001f, spline.GetSplineProgress(0.3f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.4987f, 0.0001f, spline.GetSplineProgress(0.4f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.5f,    0.0001f, spline.GetSplineProgress(0.5f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.5011f, 0.0001f, spline.GetSplineProgress(0.6f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.5102f, 0.0001f, spline.GetSplineProgress(0.7f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.5385f, 0.0001f, spline.GetSplineProgress(0.8f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.6124f, 0.0001f, spline.GetSplineProgress(0.9f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.6936f, 0.0001f, spline.GetSplineProgress(0.95f));
        VERIFY_ARE_EQUAL(1, spline.GetSplineProgress(1));
    }

    {
        LOG_OUTPUT(L"(1,0) (0,1) - hold steady, sharp rise in the middle, hold steady");
        CKeySpline spline;
        spline.m_ControlPoint1.x = 1;
        spline.m_ControlPoint1.y = 0;
        spline.m_ControlPoint2.x = 0;
        spline.m_ControlPoint2.y = 1;

        VERIFY_ARE_EQUAL(0, spline.GetSplineProgress(0));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.0037f, 0.0001f, spline.GetSplineProgress(0.1f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.0174f, 0.0001f, spline.GetSplineProgress(0.2f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.0473f, 0.0001f, spline.GetSplineProgress(0.3f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.1107f, 0.0001f, spline.GetSplineProgress(0.4f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.1762f, 0.0001f, spline.GetSplineProgress(0.45f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.4982f, 0.0001f, spline.GetSplineProgress(0.5f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.8231f, 0.0001f, spline.GetSplineProgress(0.55f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.8895f, 0.0001f, spline.GetSplineProgress(0.6f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.9526f, 0.0001f, spline.GetSplineProgress(0.7f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.9826f, 0.0001f, spline.GetSplineProgress(0.8f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.9962f, 0.0001f, spline.GetSplineProgress(0.9f));
        VERIFY_ARE_EQUAL(1, spline.GetSplineProgress(1));
    }

    {
        LOG_OUTPUT(L"(0,1) (0,1) - sharp rise in the beginning, hold steady");
        CKeySpline spline;
        spline.m_ControlPoint1.x = 0;
        spline.m_ControlPoint1.y = 1;
        spline.m_ControlPoint2.x = 0;
        spline.m_ControlPoint2.y = 1;

        VERIFY_ARE_EQUAL(0, spline.GetSplineProgress(0));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.5174f, 0.0001f, spline.GetSplineProgress(0.01f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.7480f, 0.0001f, spline.GetSplineProgress(0.05f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.8461f, 0.0001f, spline.GetSplineProgress(0.1f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.9912f, 0.0001f, spline.GetSplineProgress(0.5f));
        VERIFY_ARE_EQUAL(1, spline.GetSplineProgress(1));
    }

    {
        LOG_OUTPUT(L"(1,0) (1,0) - hold steady, sharp rise at the end");
        CKeySpline spline;
        spline.m_ControlPoint1.x = 1;
        spline.m_ControlPoint1.y = 0;
        spline.m_ControlPoint2.x = 1;
        spline.m_ControlPoint2.y = 0;

        VERIFY_ARE_EQUAL(0, spline.GetSplineProgress(0));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.0087f, 0.0001f, spline.GetSplineProgress(0.5f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.1538f, 0.0001f, spline.GetSplineProgress(0.9f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.2509f, 0.0001f, spline.GetSplineProgress(0.95f));
        DCompAnimationUnitTestHelper::VerifyWithTolerance(0.4820f, 0.0001f, spline.GetSplineProgress(0.99f));
        VERIFY_ARE_EQUAL(1, spline.GetSplineProgress(1));
    }
}

} } } } } }
