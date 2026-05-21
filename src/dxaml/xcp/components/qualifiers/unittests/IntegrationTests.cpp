// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "IntegrationTests.h"
#include "XamlLogging.h"
#include <WexTestClass.h>
#include <functional>
#include "VariantMap.h"
#include "MocksAndHelpers.h"
#include "MinWidthQualifier.h"
#include "MinHeightQualifier.h"
#include "MultiQualifier.h"
#include <memory>

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Qualifiers {

        void IntegrationTests::QualifierContextWithVariantMaps()
        {
            VariantMap<xstring_ptr> vm;

            LOG_OUTPUT(L"Create QualifierContext and VariantMaps. Add VariantMaps.");
            auto qualifierContext = std::make_shared<QualifierContext>();
            VariantMap<int*> variantMap1;
            VariantMap<int*> variantMap2;
            VariantMap<int*> variantMap3;

            variantMap1.SetQualifierContext(qualifierContext);
            variantMap2.SetQualifierContext(qualifierContext);
            variantMap3.SetQualifierContext(qualifierContext);

            // Create two qualifiers to add to the VariantMaps
            LOG_OUTPUT(L"Create test qualifiers and add to VariantMaps.");
            auto q1 = std::make_shared<TestQualifier>();
            q1->m_qualified = true;
            q1->m_width = 100;
            q1->m_flags = QualifierFlags::Width;
            int v1 = 1;

            LOG_OUTPUT(L"Create test qualifiers and add to 2nd VariantMap.");
            auto q2 = std::make_shared<TestQualifier>();
            q2->m_qualified = true;
            q2->m_width = 200;
            q2->m_flags = QualifierFlags::Width;
            int v2 = 2;

            LOG_OUTPUT(L"Add qualifiers to variant map");
            // VariantMaps need to register with the QualifierContext for any
            // context changes it needs to respond to.  This should occur
            // on VariantMap<>::Add().
            VERIFY_SUCCEEDED(variantMap1.Add(&v1, q1));
            VERIFY_SUCCEEDED(variantMap1.Add(&v2, q2));
            VERIFY_SUCCEEDED(variantMap2.Add(&v1, q1));
            VERIFY_SUCCEEDED(variantMap2.Add(&v2, q2));

            LOG_OUTPUT(L"Simulate window changed");
            // VariantMaps should be called back and evaluate when width changes
            qualifierContext->OnWindowChanged(200, 0);

            // Verify variantMap1 selection
            int* selection = *variantMap1.SelectedItem();
            VERIFY_IS_NOT_NULL(selection);
            VERIFY_ARE_EQUAL(*selection, 2);

            // Verify variantMap2 selection
            selection = *variantMap2.SelectedItem();
            VERIFY_IS_NOT_NULL(selection);
            VERIFY_ARE_EQUAL(*selection, 2);

            // Change the context to disqualify item v2
            // TestQualifier.m_qulaified must be set manually
            ((TestQualifier*)q2.get())->m_qualified = false;
            qualifierContext->OnWindowChanged(150, 0);

            // Verify variantMap1 selection
            selection = *variantMap1.SelectedItem();
            VERIFY_IS_NOT_NULL(selection);
            VERIFY_ARE_EQUAL(*selection, 1);

            // Verify variantMap2 selection
            selection = *variantMap2.SelectedItem();
            VERIFY_IS_NOT_NULL(selection);
            VERIFY_ARE_EQUAL(*selection, 1);
        };

        void IntegrationTests::MinWidth()
        {
            LOG_OUTPUT(L"Create VariantMap. Add qualifiers.");
            auto qualifierContext = std::make_shared<QualifierContext>();
            VariantMap<int*> variantMap1;
            VariantMap<int*> variantMap2;
            VariantMap<int*> variantMap3;

            variantMap1.SetQualifierContext(qualifierContext);
            variantMap2.SetQualifierContext(qualifierContext);
            variantMap3.SetQualifierContext(qualifierContext);

            // Create two qualifiers to add to the VariantMaps
            auto q1 = std::make_shared<MinWidthQualifier>(100);
            int v1 = 1;
            auto q2 = std::make_shared<MinWidthQualifier>(200);
            int v2 = 2;
            auto q3 = std::make_shared<MinWidthQualifier>(300);
            int v3 = 3;

            VERIFY_SUCCEEDED(variantMap1.Add(&v1, q1));
            VERIFY_SUCCEEDED(variantMap1.Add(&v2, q2));
            VERIFY_SUCCEEDED(variantMap1.Add(&v3, q3));

            int* selection = *variantMap1.SelectedItem();
            VERIFY_IS_NOT_NULL(selection);
            VERIFY_ARE_EQUAL(*selection, 3);

            qualifierContext->OnWindowChanged(200, 0);
            selection = *variantMap1.SelectedItem();
            VERIFY_IS_NOT_NULL(selection);
            VERIFY_ARE_EQUAL(*selection, 2);
        }

        void IntegrationTests::MinHeight()
        {
            LOG_OUTPUT(L"Create VariantMap. Add qualifiers.");
            auto qualifierContext = std::make_shared<QualifierContext>();
            VariantMap<int*> variantMap1;
            VariantMap<int*> variantMap2;
            VariantMap<int*> variantMap3;

            variantMap1.SetQualifierContext(qualifierContext);
            variantMap2.SetQualifierContext(qualifierContext);
            variantMap3.SetQualifierContext(qualifierContext);

            // Create two qualifiers to add to the VariantMaps
            auto q1 = std::make_shared<MinHeightQualifier>(100);
            int v1 = 1;
            auto q2 = std::make_shared<MinHeightQualifier>(200);
            int v2 = 2;
            auto q3 = std::make_shared<MinHeightQualifier>(300);
            int v3 = 3;

            VERIFY_SUCCEEDED(variantMap1.Add(&v1, q1));
            VERIFY_SUCCEEDED(variantMap1.Add(&v2, q2));
            VERIFY_SUCCEEDED(variantMap1.Add(&v3, q3));

            int* selection = *variantMap1.SelectedItem();
            VERIFY_IS_NOT_NULL(selection);
            VERIFY_ARE_EQUAL(*selection, 3);

            qualifierContext->OnWindowChanged(0, 200);
            selection = *variantMap1.SelectedItem();
            VERIFY_IS_NOT_NULL(selection);
            VERIFY_ARE_EQUAL(*selection, 2);
        }

        void IntegrationTests::MultiQualifierWidthHeight()
        {
            LOG_OUTPUT(L"Create VariantMap. Add qualifiers.");
            auto qualifierContext = std::make_shared<QualifierContext>();
            VariantMap<int*> variantMap1;
            VariantMap<int*> variantMap2;
            VariantMap<int*> variantMap3;

            variantMap1.SetQualifierContext(qualifierContext);
            variantMap2.SetQualifierContext(qualifierContext);
            variantMap3.SetQualifierContext(qualifierContext);

            // Create two multi qualifiers to add to the VariantMaps
            auto q1 = std::make_shared<MinWidthQualifier>(100);
            auto q2 = std::make_shared<MinHeightQualifier>(200);
            int v1 = 1;

            auto q3 = std::make_shared<MinWidthQualifier>(100);
            auto q4 = std::make_shared<MinHeightQualifier>(300);
            int v2 = 2;

            // Create the multi qualifiers
            auto multiQualifier1 = std::make_shared<MultiQualifier>();
            multiQualifier1->Add(q1);
            multiQualifier1->Add(q2);

            auto multiQualifier2 = std::make_shared<MultiQualifier>();
            multiQualifier2->Add(q3);
            multiQualifier2->Add(q4);

            VERIFY_SUCCEEDED(variantMap1.Add(&v1, multiQualifier1));
            VERIFY_SUCCEEDED(variantMap1.Add(&v2, multiQualifier2));

            LOG_OUTPUT(L"Simulate window change");
            qualifierContext->OnWindowChanged(100, 500);

            int* selection = *variantMap1.SelectedItem();
            VERIFY_IS_NOT_NULL(selection);
            VERIFY_ARE_EQUAL(*selection, 2);
        }

} } } } }


