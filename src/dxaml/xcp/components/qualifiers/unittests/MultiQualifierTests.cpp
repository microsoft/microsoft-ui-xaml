// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MultiQualifierTests.h"
#include "XamlLogging.h"
#include <WexTestClass.h>
#include <functional>
#include "MultiQualifier.h"
#include "MinHeightQualifier.h"
#include <memory>
#include "QualifierContext.h"
#include "MocksAndHelpers.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Qualifiers {

        void MultiQualifierTests::IsQualified()
        {
            auto q1 = std::make_shared<MultiQualifier>();
            VERIFY_IS_FALSE(q1->IsQualified());

            auto testQualifier1 = std::make_shared<TestQualifier>();
            testQualifier1->m_qualified = true;
            testQualifier1->m_width = 100;
            testQualifier1->m_flags = QualifierFlags::Width;

            auto testQualifier2 = std::make_shared<TestQualifier>();
            testQualifier2->m_qualified = true;
            testQualifier2->m_height = 200;
            testQualifier2->m_flags = QualifierFlags::Height;

            q1->Add(testQualifier1);
            q1->Add(testQualifier2);

            QualifierContext testQC;
            testQC.OnWindowChanged(100, 100);

            q1->Evaluate(&testQC);
            VERIFY_IS_TRUE(q1->IsQualified());

            testQualifier1->m_qualified = false;
            testQualifier2->m_qualified = false;
            q1->Evaluate(&testQC);
            VERIFY_IS_FALSE(q1->IsQualified());

            testQualifier1->m_qualified = true;
            testQualifier2->m_qualified = true;
            q1->Evaluate(&testQC);
            VERIFY_IS_TRUE(q1->IsQualified());
        }

        void MultiQualifierTests::Score()
        {
            auto q1 = std::make_shared<MultiQualifier>();
            VERIFY_ARE_EQUAL(q1->Score(QualifierFlags::Width), -1);

            auto testQualifier1 = std::make_shared<TestQualifier>();
            testQualifier1->m_qualified = true;
            testQualifier1->m_width = 100;
            testQualifier1->m_flags = QualifierFlags::Width;

            auto testQualifier2 = std::make_shared<TestQualifier>();
            testQualifier2->m_qualified = true;
            testQualifier2->m_height = 200;
            testQualifier2->m_flags = QualifierFlags::Height;

            q1->Add(testQualifier1);
            q1->Add(testQualifier2);

            VERIFY_ARE_EQUAL(q1->Score(QualifierFlags::Width), 100);
        }

        void MultiQualifierTests::ScoreQualifierFlags()
        {
            auto q1 = std::make_shared<MultiQualifier>();

            VERIFY_ARE_EQUAL(q1->Score((QualifierFlags)~0), -1);

            auto testQualifier1 = std::make_shared<TestQualifier>();
            testQualifier1->m_qualified = true;
            testQualifier1->m_width = 100;
            testQualifier1->m_flags = QualifierFlags::Width;

            auto testQualifier2 = std::make_shared<TestQualifier>();
            testQualifier2->m_qualified = true;
            testQualifier2->m_height = 200;
            testQualifier2->m_flags = QualifierFlags::Height;

            q1->Add(testQualifier1);
            q1->Add(testQualifier2);

            VERIFY_ARE_EQUAL(q1->Score(QualifierFlags::Height), 200);
            VERIFY_ARE_EQUAL(q1->Score(QualifierFlags::Width), 100);
        }

        void MultiQualifierTests::Evaluate()
        {
            auto q1 = std::make_shared<MultiQualifier>();
            VERIFY_IS_FALSE(q1->IsQualified());

            auto testQualifier1 = std::make_shared<TestQualifier>();
            testQualifier1->m_qualified = true;
            testQualifier1->m_width = 100;
            testQualifier1->m_flags = QualifierFlags::Width;

            auto testQualifier2 = std::make_shared<TestQualifier>();
            testQualifier2->m_qualified = true;
            testQualifier2->m_height = 200;
            testQualifier2->m_flags = QualifierFlags::Height;

            q1->Add(testQualifier1);
            q1->Add(testQualifier2);

            QualifierContext testQC;
            testQC.OnWindowChanged(100, 100);

            q1->Evaluate(&testQC);
            VERIFY_IS_TRUE(q1->IsQualified());
        }

        void MultiQualifierTests::Flags()
        {
            auto q1 = std::make_shared<MultiQualifier>();
            VERIFY_ARE_EQUAL(q1->Flags(), QualifierFlags::None);

            auto testQualifier1 = std::make_shared<TestQualifier>();
            testQualifier1->m_qualified = true;
            testQualifier1->m_width = 100;
            testQualifier1->m_flags = QualifierFlags::Width;

            auto testQualifier2 = std::make_shared<TestQualifier>();
            testQualifier2->m_qualified = true;
            testQualifier2->m_height = 200;
            testQualifier2->m_flags = QualifierFlags::Height;

            q1->Add(testQualifier1);
            q1->Add(testQualifier2);

            VERIFY_ARE_EQUAL(q1->Flags(), QualifierFlags::Width|QualifierFlags::Height);
        }

} } } } }






