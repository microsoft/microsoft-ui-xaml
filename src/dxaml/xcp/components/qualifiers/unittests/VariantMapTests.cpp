// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "VariantMapTests.h"
#include "XamlLogging.h"
#include <WexTestClass.h>
#include <functional>
#include "VariantMap.h"
#include "MocksAndHelpers.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Qualifiers {

        void VariantMapUnitTests::AddQualifiers()
        {
            VariantMap<xstring_ptr> vm;

            LOG_OUTPUT(L"Create VariantMap. Add qualifiers.");
            VariantMap<int*> variantMap;

            // Qualified and has width value set
            auto q1 = std::make_shared<TestQualifier>();
            q1->m_qualified = true;
            q1->m_width = 100;
            int v1 = 1;

            // Higher precedence, but doesn't meet qualifying conditions
            auto q2 = std::make_shared<TestQualifier>();
            q2->m_width = 200;
            int v2 = 2;

            // Qualified and has platform set
            // This is the highest precedence item that is qualified
            auto q3 = std::make_shared<TestQualifier>();
            q3->m_width = 50;
            q3->m_qualified = true;
            int v3 = 3;

            // Does not meet qualifying conditions
            auto q4 = std::make_shared<TestQualifier>();
            int v4 = 4;

            VERIFY_SUCCEEDED(variantMap.Add(&v1, q1));
            VERIFY_SUCCEEDED(variantMap.Add(&v2, q2));
            VERIFY_SUCCEEDED(variantMap.Add(&v3, q3));
            VERIFY_SUCCEEDED(variantMap.Add(&v4, q4));
            int* selection = *variantMap.SelectedItem();
            VERIFY_IS_TRUE(q1->m_evaluateCalled);
            VERIFY_IS_TRUE(q2->m_evaluateCalled);
            VERIFY_IS_TRUE(q3->m_evaluateCalled);
            VERIFY_IS_TRUE(q4->m_evaluateCalled);

            LOG_OUTPUT(L"Verify item with greatest width set.");
            VERIFY_IS_NOT_NULL(selection);
            VERIFY_ARE_EQUAL(*selection, 1);
        };

        void VariantMapUnitTests::Evaluation()
        {
            LOG_OUTPUT(L"Create VariantMap. Add qualifiers.");

            class TestClass
            {
                public:
                    TestClass(int value) : m_value(value) {}
                    int m_value;
            };

            VariantMap<TestClass*> variantMap;

            auto q1 = std::make_shared<TestQualifier>();
            q1->m_qualified = true;
            q1->m_width = 10;
            q1->m_height = 10;
            TestClass v1(0);

            auto q2 = std::make_shared<TestQualifier>();
            q2->m_qualified = true;
            q2->m_width = 10;
            q2->m_height = 20;
            TestClass v2(1);

            VERIFY_SUCCEEDED(variantMap.Add(&v1, q1));
            TestClass* selection = *variantMap.SelectedItem();
            VERIFY_IS_NOT_NULL(selection);
            VERIFY_ARE_EQUAL(selection->m_value, 0);
            VERIFY_IS_TRUE(q1->m_evaluateCalled);
            q1->m_evaluateCalled = false;
            q2->m_evaluateCalled = false;

            LOG_OUTPUT(L"Verify Add triggers evaluation.");
            VERIFY_SUCCEEDED(variantMap.Add(&v2, q2));
            selection = *variantMap.SelectedItem();
            VERIFY_IS_NOT_NULL(selection);
            VERIFY_ARE_EQUAL(selection->m_value, 1);
            VERIFY_IS_TRUE(q1->m_evaluateCalled);
            VERIFY_IS_TRUE(q2->m_evaluateCalled);
            q1->m_evaluateCalled = false;
            q2->m_evaluateCalled = false;

            LOG_OUTPUT(L"Verify context change triggers evaluation.");
            q2->m_width = 5;

            VERIFY_SUCCEEDED(variantMap.OnQualifierContextChanged());
            selection = *variantMap.SelectedItem();
            VERIFY_IS_NOT_NULL(selection);
            VERIFY_ARE_EQUAL(selection->m_value, 0);
            VERIFY_IS_TRUE(q1->m_evaluateCalled);
            VERIFY_IS_TRUE(q2->m_evaluateCalled);
        };

        void VariantMapUnitTests::Selection()
        {
            LOG_OUTPUT(L"Create VariantMap. Add qualifiers.");
            VariantMap<int*> variantMap;

            auto q1 = std::make_shared<TestQualifier>();
            q1->m_qualified = true;
            q1->m_width = 10;
            int v1 = 0;

            auto q2 = std::make_shared<TestQualifier>();
            q2->m_qualified = true;
            q2->m_width = 20;
            q2->m_height = 2;
            int v2 = 2;

            auto q3 = std::make_shared<TestQualifier>();
            q3->m_qualified = false;
            q3->m_width = 30;
            int v3 = 4;

            auto q4 = std::make_shared<TestQualifier>();
            q4->m_qualified = true;
            q4->m_width = 20;
            q4->m_height = 1;
            int v4 = 6;

            VERIFY_SUCCEEDED(variantMap.Add(&v1, q1));
            VERIFY_SUCCEEDED(variantMap.Add(&v2, q2));
            VERIFY_SUCCEEDED(variantMap.Add(&v3, q3));
            VERIFY_SUCCEEDED(variantMap.Add(&v4, q4));
            int* selection = *variantMap.SelectedItem();

            LOG_OUTPUT(L"Verify item w/ greatest width is selected when no items have platform set.");
            VERIFY_IS_NOT_NULL(selection);
            VERIFY_ARE_EQUAL(*selection, 2);
            VERIFY_IS_TRUE(q1->m_evaluateCalled);
            VERIFY_IS_TRUE(q2->m_evaluateCalled);
            VERIFY_IS_TRUE(q3->m_evaluateCalled);
            VERIFY_IS_TRUE(q4->m_evaluateCalled);
            q1->m_evaluateCalled = false;
            q2->m_evaluateCalled = false;
            q3->m_evaluateCalled = false;
            q4->m_evaluateCalled = false;

            // Tied in width, set q4 to have greater height
            q4->m_height = 3;

            LOG_OUTPUT(L"Verify item w/ greatest height is selected when width it tied.");
            VERIFY_SUCCEEDED(variantMap.Evaluate());
            selection = *variantMap.SelectedItem();
            VERIFY_IS_NOT_NULL(selection);
            VERIFY_ARE_EQUAL(*selection, 6);
            VERIFY_IS_TRUE(q1->m_evaluateCalled);
            VERIFY_IS_TRUE(q2->m_evaluateCalled);
            VERIFY_IS_TRUE(q3->m_evaluateCalled);
            VERIFY_IS_TRUE(q4->m_evaluateCalled);
        };

        void VariantMapUnitTests::VariantMapCallback()
        {
            class Listener : public IVariantMapChangedCallback
            {
                public:
                    Listener() : m_callbackCalled(false) { };
                    virtual HRESULT OnVariantMapChanged() { m_callbackCalled = true; return S_OK; };

                    bool m_callbackCalled;
            };

            LOG_OUTPUT(L"Create VariantMap. Add qualifiers.");
            VariantMap<int*> variantMap;

            auto q1 = std::make_shared<TestQualifier>();
            q1->m_qualified = true;
            q1->m_width = 10;
            int v1 = 0;

            auto q2 = std::make_shared<TestQualifier>();
            q2->m_qualified = true;
            q2->m_width = 20;
            q2->m_height = 2;
            int v2 = 2;

            auto q3 = std::make_shared<TestQualifier>();
            q3->m_qualified = false;
            q3->m_width = 30;
            int v3 = 4;

            auto q4 = std::make_shared<TestQualifier>();
            q4->m_qualified = true;
            q4->m_width = 20;
            q4->m_height = 1;
            int v4 = 6;

            VERIFY_SUCCEEDED(variantMap.Add(&v1, q1));
            VERIFY_SUCCEEDED(variantMap.Add(&v2, q2));
            VERIFY_SUCCEEDED(variantMap.Add(&v3, q3));
            VERIFY_SUCCEEDED(variantMap.Add(&v4, q4));

            LOG_OUTPUT(L"Create VariantMap listeners.");
            Listener listener1;
            Listener listener2;
            Listener listener3;
            VERIFY_SUCCEEDED(variantMap.RegisterForChangeCallback(&listener1));
            VERIFY_SUCCEEDED(variantMap.RegisterForChangeCallback(&listener2));
            VERIFY_SUCCEEDED(variantMap.RegisterForChangeCallback(&listener3));

            LOG_OUTPUT(L"Get SelectedItem. Verify callbacks were called.");
            int* selection = *variantMap.SelectedItem();
            VERIFY_IS_TRUE(listener1.m_callbackCalled);
            VERIFY_IS_TRUE(listener2.m_callbackCalled);
            VERIFY_IS_TRUE(listener3.m_callbackCalled);
            listener1.m_callbackCalled = false;
            listener2.m_callbackCalled = false;
            listener3.m_callbackCalled = false;

            LOG_OUTPUT(L"Verify item w/ greatest width is selected when no items have platform set.");
            VERIFY_IS_NOT_NULL(selection);
            VERIFY_ARE_EQUAL(*selection, 2);
            VERIFY_IS_TRUE(q1->m_evaluateCalled);
            VERIFY_IS_TRUE(q2->m_evaluateCalled);
            VERIFY_IS_TRUE(q3->m_evaluateCalled);
            VERIFY_IS_TRUE(q4->m_evaluateCalled);
            q1->m_evaluateCalled = false;
            q2->m_evaluateCalled = false;
            q3->m_evaluateCalled = false;
            q4->m_evaluateCalled = false;

            // Tied in width, set q4 to have greater height
            q4->m_height = 3;

            LOG_OUTPUT(L"Verify item w/ greatest height is selected when width it tied.");
            VERIFY_SUCCEEDED(variantMap.Evaluate());
            selection = *variantMap.SelectedItem();
            VERIFY_IS_NOT_NULL(selection);
            VERIFY_ARE_EQUAL(*selection, 6);
            VERIFY_IS_TRUE(q1->m_evaluateCalled);
            VERIFY_IS_TRUE(q2->m_evaluateCalled);
            VERIFY_IS_TRUE(q3->m_evaluateCalled);
            VERIFY_IS_TRUE(q4->m_evaluateCalled);
            q1->m_evaluateCalled = false;
            q2->m_evaluateCalled = false;
            q3->m_evaluateCalled = false;
            q4->m_evaluateCalled = false;
            VERIFY_IS_TRUE(listener1.m_callbackCalled);
            VERIFY_IS_TRUE(listener2.m_callbackCalled);
            VERIFY_IS_TRUE(listener3.m_callbackCalled);
            listener1.m_callbackCalled = false;
            listener2.m_callbackCalled = false;
            listener3.m_callbackCalled = false;

            // Unregister listener1 and listener3
            VERIFY_SUCCEEDED(variantMap.UnRegisterForChangeCallback(&listener1));
            VERIFY_SUCCEEDED(variantMap.UnRegisterForChangeCallback(&listener3));
        };

        void VariantMapUnitTests::RemoveItem()
        {
            LOG_OUTPUT(L"Create VariantMap. Add qualifiers.");
            VariantMap<int*> variantMap;

            // Qualified and has width value set
            // Width is lower precedence than platform
            auto q1 = std::make_shared<TestQualifier>();
            q1->m_qualified = true;
            q1->m_width = 100;
            int v1 = 1;

            // Higher precedence, but doesn't meet qualifying conditions
            auto q2 = std::make_shared<TestQualifier>();
            q2->m_width = 200;
            int v2 = 2;

            // Qualified and has platform set
            // This is the highest precedence item that is qualified
            auto q3 = std::make_shared<TestQualifier>();
            q3->m_width = 50;
            q3->m_qualified = true;
            int v3 = 3;

            // Does not meet qualifying conditions
            auto q4 = std::make_shared<TestQualifier>();
            q3->m_width = 500;
            q4->m_qualified = true;
            int v4 = 4;

            VERIFY_SUCCEEDED(variantMap.Add(&v1, q1));
            VERIFY_SUCCEEDED(variantMap.Add(&v2, q2));
            VERIFY_SUCCEEDED(variantMap.Add(&v3, q3));
            VERIFY_SUCCEEDED(variantMap.Add(&v4, q4));
            VERIFY_SUCCEEDED(variantMap.Remove(&v1, q1));
            VERIFY_SUCCEEDED(variantMap.Remove(&v2, q2));
            VERIFY_SUCCEEDED(variantMap.Remove(&v3, q3));
            VERIFY_SUCCEEDED(variantMap.Add(&v4, q4));
            int* selection = *variantMap.SelectedItem();
            VERIFY_IS_FALSE(q1->m_evaluateCalled);
            VERIFY_IS_FALSE(q2->m_evaluateCalled);
            VERIFY_IS_FALSE(q3->m_evaluateCalled);
            VERIFY_IS_TRUE(q4->m_evaluateCalled);

            LOG_OUTPUT(L"Verify item with greatest precedence.");
            VERIFY_IS_NOT_NULL(selection);
            VERIFY_ARE_EQUAL(*selection, 4);

        };

        void VariantMapUnitTests::UpdateItem()
        {
            LOG_OUTPUT(L"Create VariantMap. Add qualifiers.");
            VariantMap<int*> variantMap;

            // Qualified and has width value set
            // Width is lower precedence than platform
            auto q1 = std::make_shared<TestQualifier>();
            q1->m_qualified = true;
            q1->m_width = 100;
            int v1 = 1;

            // Higher precedence, but doesn't meet qualifying conditions
            auto q2 = std::make_shared<TestQualifier>();
            q2->m_width = 200;
            int v2 = 2;

            // Qualified and has platform set
            // This is the highest precedence item that is qualified
            auto q3 = std::make_shared<TestQualifier>();
            q3->m_width = 50;
            q3->m_qualified = true;
            int v3 = 3;

            // Does not meet qualifying conditions
            auto q4 = std::make_shared<TestQualifier>();
            int v4 = 4;

            VERIFY_SUCCEEDED(variantMap.Add(&v1, q1));
            VERIFY_SUCCEEDED(variantMap.Add(&v2, q2));
            VERIFY_SUCCEEDED(variantMap.Add(&v3, q3));
            VERIFY_SUCCEEDED(variantMap.Add(&v4, q4));
            int* selection = *variantMap.SelectedItem();
            VERIFY_IS_TRUE(q1->m_evaluateCalled);
            VERIFY_IS_TRUE(q2->m_evaluateCalled);
            VERIFY_IS_TRUE(q3->m_evaluateCalled);
            VERIFY_IS_TRUE(q4->m_evaluateCalled);
            q1->m_evaluateCalled = false;
            q2->m_evaluateCalled = false;
            q3->m_evaluateCalled = false;
            q4->m_evaluateCalled = false;

            LOG_OUTPUT(L"Verify item with greatest precedence.");
            VERIFY_IS_NOT_NULL(selection);
            VERIFY_ARE_EQUAL(*selection, 1);

            LOG_OUTPUT(L"Update qualifiers2 and qualifiers3, then re-evaluate");
            auto q2_2 = std::make_shared<TestQualifier>();
            q2_2->m_width = 50000;
            q2_2->m_qualified = true;

            auto q3_2 = std::make_shared<TestQualifier>();
            q3_2->m_width = 1;
            q3_2->m_qualified = true;

            VERIFY_SUCCEEDED(variantMap.Replace(&v2, q2, q2_2));
            VERIFY_SUCCEEDED(variantMap.Replace(&v3, q3, q3_2));

            VERIFY_SUCCEEDED(variantMap.Evaluate());
            selection = *variantMap.SelectedItem();
            VERIFY_IS_TRUE(q1->m_evaluateCalled);
            VERIFY_IS_FALSE(q2->m_evaluateCalled);
            VERIFY_IS_FALSE(q3->m_evaluateCalled);
            VERIFY_IS_TRUE(q2_2->m_evaluateCalled);
            VERIFY_IS_TRUE(q3_2->m_evaluateCalled);
            VERIFY_IS_TRUE(q4->m_evaluateCalled);

            LOG_OUTPUT(L"Verify item with platform set.");
            VERIFY_IS_NOT_NULL(selection);
            VERIFY_ARE_EQUAL(*selection, 2);
        };

} } } } }

