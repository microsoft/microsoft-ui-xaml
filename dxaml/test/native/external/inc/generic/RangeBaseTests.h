// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Generic {

    template<typename TClassUnderTest>
    class RangeBaseTests
    {
    public:
        static void DoesFireRangeValueChangedEvent()
        {
            TestCleanupWrapper cleanup;

            TClassUnderTest^ rangebase = nullptr;
            auto valueChangedEvent = std::make_shared<Event>();
            auto valueChangedRegistation = CreateSafeEventRegistration(TClassUnderTest, ValueChanged);

            RunOnUIThread([&]()
            {
                rangebase = ref new TClassUnderTest();
                rangebase->Value = 0;

                valueChangedRegistation.Attach(rangebase, ref new xaml_primitives::RangeBaseValueChangedEventHandler([valueChangedEvent](Platform::Object^, xaml_primitives::RangeBaseValueChangedEventArgs^) {
                    valueChangedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = rangebase;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // change the value to see if the event launches
                rangebase->Value = 1;
            });
            valueChangedEvent->WaitForDefault();
        }

        static void IsRangeValueKeptBetweenMaxAndMin()
        {
            TestCleanupWrapper cleanup;

            TClassUnderTest^ rangebase = nullptr;
            auto valueChangedEvent = std::make_shared<Event>();
            auto valueChangedRegistation = CreateSafeEventRegistration(TClassUnderTest, ValueChanged);

            RunOnUIThread([&]()
            {
                rangebase = ref new TClassUnderTest();
                rangebase->Value = 0;
                rangebase->Maximum = 100;
                rangebase->Minimum = 0;

                // each time the value is changed, we'll check if the value is between max and min.
                valueChangedRegistation.Attach(rangebase, ref new xaml_primitives::RangeBaseValueChangedEventHandler([rangebase, valueChangedEvent](Platform::Object^, xaml_primitives::RangeBaseValueChangedEventArgs^) {
                    VERIFY_IS_TRUE(rangebase->Minimum <= rangebase->Maximum
                        && rangebase->Value >= rangebase->Minimum
                        && rangebase->Value <= rangebase->Maximum);
                    valueChangedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = rangebase;
            });
            TestServices::WindowHelper->WaitForIdle();

            // Testing for edge conditions to see if value is kept between Maximum and Minimum
            // test case 1: Regular case.
            RunOnUIThread([&]()
            {
                rangebase->Minimum = 0;
                rangebase->Maximum = 100;
                rangebase->Value = 50;
            });
            valueChangedEvent->WaitForDefault();

            // test case 2: Value smaller than minimum
            RunOnUIThread([&]()
            {
                rangebase->Minimum = 0;
                rangebase->Maximum = 100;
                rangebase->Value = -50;
            });
            valueChangedEvent->WaitForDefault();

            // test case 3: Value larger than maximum
            RunOnUIThread([&]()
            {
                rangebase->Minimum = 100;
                rangebase->Maximum = 0;
                rangebase->Value = 150;
            });
            valueChangedEvent->WaitForDefault();

            // test case 4: Maximum smaller than minimum
            RunOnUIThread([&]()
            {
                rangebase->Maximum = 0;
                rangebase->Minimum = 50;
                rangebase->Value = -50;
            });
            valueChangedEvent->WaitForDefault();

            // test case 5: Negative minimum, positive maximum
            RunOnUIThread([&]()
            {
                rangebase->Minimum = -100;
                rangebase->Maximum = 100;
                rangebase->Value = 50;
            });
            valueChangedEvent->WaitForDefault();

            // test case 6: Negative minimum, negative maximum, value equals minimum.
            RunOnUIThread([&]()
            {
                rangebase->Minimum = -100;
                rangebase->Maximum = -50;
                rangebase->Value = -100;
            });
            valueChangedEvent->WaitForDefault();

            // test case 7: Negative minimum, positive maximum, value equals maximum.
            RunOnUIThread([&]()
            {
                rangebase->Minimum = -0.5;
                rangebase->Maximum = 0.5;
                rangebase->Value = 0.5;
            });
            valueChangedEvent->WaitForDefault();
        }

        static void MinMaxValueSetThroughMarkupWork(const wchar_t* typeName)
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]()
            {
                TClassUnderTest^ rangebase = safe_cast<TClassUnderTest^>(Markup::XamlReader::Load(Platform::StringReference(
                    WEX::Common::String().Format(
                        L"<%s xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Minimum='10' Value='15' Maximum='20'/>",
                        typeName))));

                VERIFY_ARE_EQUAL(10, rangebase->Minimum);
                VERIFY_ARE_EQUAL(15, rangebase->Value);
                VERIFY_ARE_EQUAL(20, rangebase->Maximum);
            });

            RunOnUIThread([&]()
            {
                TClassUnderTest^ rangebase = safe_cast<TClassUnderTest^>(Markup::XamlReader::Load(Platform::StringReference(
                    WEX::Common::String().Format(
                        L"<%s xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Minimum='10' Maximum='20' Value='15'/>",
                        typeName))));

                VERIFY_ARE_EQUAL(10, rangebase->Minimum);
                VERIFY_ARE_EQUAL(15, rangebase->Value);
                VERIFY_ARE_EQUAL(20, rangebase->Maximum);
            });

            RunOnUIThread([&]()
            {
                TClassUnderTest^ rangebase = safe_cast<TClassUnderTest^>(Markup::XamlReader::Load(Platform::StringReference(
                    WEX::Common::String().Format(
                        L"<%s xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Value='15' Minimum='10' Maximum='20'/>",
                        typeName))));

                VERIFY_ARE_EQUAL(10, rangebase->Minimum);
                VERIFY_ARE_EQUAL(15, rangebase->Value);
                VERIFY_ARE_EQUAL(20, rangebase->Maximum);
            });

            RunOnUIThread([&]()
            {
                TClassUnderTest^ rangebase = safe_cast<TClassUnderTest^>(Markup::XamlReader::Load(Platform::StringReference(
                    WEX::Common::String().Format(
                        L"<%s xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Value='15' Maximum='20' Minimum='10'/>",
                        typeName))));

                VERIFY_ARE_EQUAL(10, rangebase->Minimum);
                VERIFY_ARE_EQUAL(15, rangebase->Value);
                VERIFY_ARE_EQUAL(20, rangebase->Maximum);
            });

            RunOnUIThread([&]()
            {
                TClassUnderTest^ rangebase = safe_cast<TClassUnderTest^>(Markup::XamlReader::Load(Platform::StringReference(
                    WEX::Common::String().Format(
                        L"<%s xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Maximum='20' Value='15' Minimum='10'/>",
                        typeName))));

                VERIFY_ARE_EQUAL(10, rangebase->Minimum);
                VERIFY_ARE_EQUAL(15, rangebase->Value);
                VERIFY_ARE_EQUAL(20, rangebase->Maximum);
            });

            RunOnUIThread([&]()
            {
                TClassUnderTest^ rangebase = safe_cast<TClassUnderTest^>(Markup::XamlReader::Load(Platform::StringReference(
                    WEX::Common::String().Format(
                        L"<%s xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Maximum='20' Minimum='10' Value='15'/>",
                        typeName))));

                VERIFY_ARE_EQUAL(10, rangebase->Minimum);
                VERIFY_ARE_EQUAL(15, rangebase->Value);
                VERIFY_ARE_EQUAL(20, rangebase->Maximum);
            });
        }
    }; // class rangebaseTests

} } } } } // namespace Microsoft::UI::Xaml::Tests::Generic
