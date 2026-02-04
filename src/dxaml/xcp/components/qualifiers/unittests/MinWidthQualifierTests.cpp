// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MinWidthQualifierTests.h"
#include "XamlLogging.h"
#include <WexTestClass.h>
#include <functional>
#include "MinWidthQualifier.h"
#include "MinHeightQualifier.h"
#include <memory>
#include "QualifierContext.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Qualifiers {

        void MinWidthQualifierTests::IsQualified()
        {
            auto q1 = std::make_shared<MinWidthQualifier>(100);
            VERIFY_IS_FALSE(q1->IsQualified());

            QualifierContext testQC;
            testQC.OnWindowChanged(300, 0);

            q1->Evaluate(&testQC);
            VERIFY_IS_TRUE(q1->IsQualified());

            testQC.OnWindowChanged(99, 0);
            q1->Evaluate(&testQC);
            VERIFY_IS_FALSE(q1->IsQualified());

            testQC.OnWindowChanged(100, 0);
            q1->Evaluate(&testQC);
            VERIFY_IS_TRUE(q1->IsQualified());
        }

        void MinWidthQualifierTests::ScoreQualifierFlags()
        {
            auto q1 = std::make_shared<MinWidthQualifier>(100);

            VERIFY_ARE_EQUAL(q1->Score((QualifierFlags)~QualifierFlags::Width), -1);
            VERIFY_ARE_EQUAL(q1->Score(QualifierFlags::Width), 100);
        }

        void MinWidthQualifierTests::Evaluate()
        {
            auto q1 = std::make_shared<MinWidthQualifier>(100);
            VERIFY_IS_FALSE(q1->IsQualified());

            QualifierContext testQC;
            testQC.OnWindowChanged(300, 0);

            q1->Evaluate(&testQC);
            VERIFY_IS_TRUE(q1->IsQualified());
        }

        void MinWidthQualifierTests::Flags()
        {
            auto q1 = std::make_shared<MinWidthQualifier>(1);
            VERIFY_ARE_EQUAL(q1->Flags(), QualifierFlags::Width);
        }

} } } } }



