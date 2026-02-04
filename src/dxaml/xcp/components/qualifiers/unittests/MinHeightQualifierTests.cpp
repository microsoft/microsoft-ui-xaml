// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MinHeightQualifierTests.h"
#include "XamlLogging.h"
#include <WexTestClass.h>
#include <functional>
#include "MinHeightQualifier.h"
#include "MinHeightQualifier.h"
#include <memory>
#include "QualifierContext.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Qualifiers {

        void MinHeightQualifierTests::IsQualified()
        {
            auto q1 = std::make_shared<MinHeightQualifier>(100);
            VERIFY_IS_FALSE(q1->IsQualified());

            QualifierContext testQC;
            testQC.OnWindowChanged(0, 300);

            q1->Evaluate(&testQC);
            VERIFY_IS_TRUE(q1->IsQualified());

            testQC.OnWindowChanged(0, 99);
            q1->Evaluate(&testQC);
            VERIFY_IS_FALSE(q1->IsQualified());

            testQC.OnWindowChanged(0, 100);
            q1->Evaluate(&testQC);
            VERIFY_IS_TRUE(q1->IsQualified());
        }

        void MinHeightQualifierTests::ScoreQualifierFlags()
        {
            auto q1 = std::make_shared<MinHeightQualifier>(100);

            VERIFY_ARE_EQUAL(q1->Score((QualifierFlags)~QualifierFlags::Height), -1);
            VERIFY_ARE_EQUAL(q1->Score(QualifierFlags::Height), 100);
        }

        void MinHeightQualifierTests::Evaluate()
        {
            auto q1 = std::make_shared<MinHeightQualifier>(100);
            VERIFY_IS_FALSE(q1->IsQualified());

            QualifierContext testQC;
            testQC.OnWindowChanged(0, 1000);

            q1->Evaluate(&testQC);
            VERIFY_IS_TRUE(q1->IsQualified());
        }

        void MinHeightQualifierTests::Flags()
        {
            auto q1 = std::make_shared<MinHeightQualifier>(1);
            VERIFY_ARE_EQUAL(q1->Flags(), QualifierFlags::Height);
        }

} } } } }




