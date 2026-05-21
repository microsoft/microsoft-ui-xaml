// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "QualifierFactoryTests.h"
#include "QualifierFactory.h"
#include "XamlLogging.h"
#include <WexTestClass.h>
#include "MinWidthQualifier.h"
#include "MinHeightQualifier.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Qualifiers {

        void QualifierFactoryTests::MinWidth()
        {
            std::shared_ptr<IQualifier> pQualifier = QualifierFactory::Create(QualifierFlags::Width, 100);
            VERIFY_ARE_EQUAL(pQualifier->Score(QualifierFlags::Width), 100);
            VERIFY_ARE_EQUAL(pQualifier->Score(static_cast<QualifierFlags>(~QualifierFlags::Width)), -1);
            VERIFY_ARE_EQUAL(pQualifier->Flags(), QualifierFlags::Width);
        }

        void QualifierFactoryTests::MinHeight()
        {
            std::shared_ptr<IQualifier> pQualifier = QualifierFactory::Create(QualifierFlags::Height, 100);
            VERIFY_ARE_EQUAL(pQualifier->Score(QualifierFlags::Height), 100);
            VERIFY_ARE_EQUAL(pQualifier->Score(static_cast<QualifierFlags>(~QualifierFlags::Height)), -1);
            VERIFY_ARE_EQUAL(pQualifier->Flags(), QualifierFlags::Height);
        }

        void QualifierFactoryTests::Multi()
        {
            LOG_OUTPUT(L"Create MultiQualifiers with a single child qualifier");
            std::shared_ptr<IQualifier> pQualifier = QualifierFactory::Create(100, -1);
            VERIFY_ARE_EQUAL(pQualifier->Score(QualifierFlags::Width), 100);
            VERIFY_ARE_EQUAL(pQualifier->Score(static_cast<QualifierFlags>(~QualifierFlags::Width)), -1);

            pQualifier = QualifierFactory::Create(-1, 100);
            VERIFY_ARE_EQUAL(pQualifier->Score(QualifierFlags::Height), 100);
            VERIFY_ARE_EQUAL(pQualifier->Score(static_cast<QualifierFlags>(~QualifierFlags::Height)), -1);

            LOG_OUTPUT(L"Create MultiQualifiers with multiple child qualifiers");
            pQualifier = QualifierFactory::Create(200, 100);
            VERIFY_ARE_EQUAL(pQualifier->Score(QualifierFlags::Width), 200);
            VERIFY_ARE_EQUAL(pQualifier->Score(QualifierFlags::Height), 100);
            VERIFY_ARE_EQUAL(pQualifier->Score(static_cast<QualifierFlags>(~(QualifierFlags::Height|QualifierFlags::Width))), -1);

            pQualifier = QualifierFactory::Create(-1, 100);
            VERIFY_ARE_EQUAL(pQualifier->Score(QualifierFlags::Height), 100);
            VERIFY_ARE_EQUAL(pQualifier->Score(static_cast<QualifierFlags>(~(QualifierFlags::Height))), -1);

            pQualifier = QualifierFactory::Create(200, -1);
            VERIFY_ARE_EQUAL(pQualifier->Score(QualifierFlags::Width), 200);
            VERIFY_ARE_EQUAL(pQualifier->Score(static_cast<QualifierFlags>(~(QualifierFlags::Width))), -1);
        }

} } } } }




