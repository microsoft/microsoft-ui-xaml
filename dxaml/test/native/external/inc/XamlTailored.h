// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Adapted for the public SDK from TAEF's Tailored.h
// Note: There are two of these things. One in test\native\external\inc and one in test\infra\client\inc. And they have
// different RunOnUIThread implementations.

#pragma once

#include <ppltasks.h>
#include <TestEvent.h>
#include <chrono>
#include <future>
#include <wil/resource.h>

#include <XamlLogging.h>
#include <CommonTestSetupHelper.h>

// TAEF's VERIFY_THROWS declares the __exception as a ref (&), which fails to catch WinRT exceptions (^).
// This helper is identical to VERIFY_THROWS but leaves the __exception type upto the caller.
#define VERIFY_THROWS_WINRT(__operation, __exception, ...)                                                                                          \
{                                                                                                                                                   \
    bool __exceptionHit = false;                                                                                                                    \
    try {                                                                                                                                           \
        __operation;                                                                                                                                \
    }                                                                                                                                               \
    catch(__exception __e) {                                                                                                                        \
        WEX::TestExecution::Private::MacroVerify::ExpectedExceptionThrown(__e, L#__exception, L#__operation, __VA_ARGS__);                          \
        __exceptionHit = true;                                                                                                                      \
    }                                                                                                                                               \
                                                                                                                                                    \
    if (!__exceptionHit) {                                                                                                                          \
        WEX::TestExecution::Private::MacroVerify::ExpectedExceptionNotThrown(L#__exception, L#__operation, PRIVATE_VERIFY_ERROR_INFO, __VA_ARGS__); \
    }                                                                                                                                               \
}

// TAEF's VERIFY_THROWS_SPECIFIC declares the __exception as a ref (&), which fails to catch WinRT exceptions (^).
// This helper is identical to VERIFY_THROWS_SPECIFIC but leaves the __exception type upto the caller.
#define VERIFY_THROWS_SPECIFIC_WINRT(__operation, __exception, __func, ...)                                                                             \
{                                                                                                                                                    \
    bool __exceptionHit = false;                                                                                                                     \
    try                                                                                                                                              \
    {                                                                                                                                                \
        __operation;                                                                                                                                 \
    }                                                                                                                                                \
    catch(__exception __e)                                                                                                                          \
    {                                                                                                                                                \
        bool __cond = __func(__e);                                                                                                                   \
        if (__cond)                                                                                                                                  \
        {                                                                                                                                            \
            WEX::TestExecution::Private::MacroVerify::ExpectedExceptionThrown(__e, L#__exception, L#__operation, __VA_ARGS__);                       \
            __exceptionHit = true;                                                                                                                   \
        }                                                                                                                                            \
        else                                                                                                                                         \
        {                                                                                                                                            \
            throw;                                                                                                                                   \
        }                                                                                                                                            \
    }                                                                                                                                                \
                                                                                                                                                     \
    if (!__exceptionHit)                                                                                                                             \
    {                                                                                                                                                \
        WEX::TestExecution::Private::MacroVerify::ExpectedExceptionNotThrown(L#__exception, L#__operation, PRIVATE_VERIFY_ERROR_INFO, __VA_ARGS__);  \
    }                                                                                                                                                \
}

#define THROW_IF_NULL(x) WEX::Common::Throw::If(x == nullptr, E_POINTER, L#x L" should not be null.");
#define THROW_IF_NULL_WITH_MSG(x, msg) WEX::Common::Throw::If(x == nullptr, E_POINTER, msg);

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {

        template <typename TFunction>
        void RunOnUIThread(const TFunction& function)
        {
            RunOnDispatcherThread(
                test_infra::TestServices::WindowHelper->CurrentDispatcher,
                true,
                function);
        }

        template <typename TFunction>
        void RunOnDispatcherThread(
            Microsoft::UI::Dispatching::DispatcherQueue^ dispatcher,
            const bool waitForCompletion,
            const TFunction& function)
        {
            if (dispatcher->HasThreadAccess)
            {
                function();
            }
            else
            {
                using namespace std::chrono_literals;

                Event operationCompleted(EventOptions::CaptureAndFailFastOnTimeout, L"UI operation");

                const boolean success = dispatcher->TryEnqueue(Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, ref new Microsoft::UI::Dispatching::DispatcherQueueHandler([&, waitForCompletion]()
                {
                    // Because this this will be run on the core dispatcher if it throws we
                    // both don't want that failure to bubble up to the CoreDispatcher itself
                    // and we want to capture that failure and surface it in the test execution
                    // path. WEX::SafeInvoke will catch typical exceptions, log them, and set
                    // the test to failing.
                    auto scopeExit = wil::scope_exit([&, waitForCompletion]
                    {
                        WEX::SafeInvoke([&, waitForCompletion]() -> bool
                        {
                            if (waitForCompletion)
                            {
                                operationCompleted.Set();
                            }
                            return true;
                        });
                    });

                    WEX::SafeInvoke([&]() -> bool { function(); return true; });
                }));

                WEX::Common::Throw::IfFalse(success, E_FAIL, L"DispatcherQueue failed to queue item.");
                if (waitForCompletion)
                {
                    operationCompleted.WaitFor(5min);
                }
            }
        }

        template <typename TClass>
        static Platform::String^ GetClassName()
        {
            return GetClassName(TClass::typeid);
        }

        static Platform::String^ GetClassName(Platform::Type^ type)
        {
            auto fullName = type->FullName;
            auto posOfLastPeriod =
                std::find(
                    std::reverse_iterator<const wchar_t*>(fullName->End()),
                    std::reverse_iterator<const wchar_t*>(fullName->Begin()),
                    '.');
            return ref new Platform::String(posOfLastPeriod.base());
        }

        static bool GetTestMetadataParam(_In_z_ const wchar_t* param)
        {
            WEX::Common::String value;
            LogThrow_IfFailedWithMessage(WEX::TestExecution::TestData::TryGetValue(param, value),
                WEX::Common::String().Format(L"Failed to get parameter: %s", param));

            return value.CompareNoCase(L"True") == 0;
        }

        static WEX::Common::String GetTestDeploymentDir()
        {
            WEX::Common::String deploymentDir;
            LogThrow_IfFailed(
                WEX::TestExecution::RuntimeParameters::TryGetValue(WEX::TestExecution::RuntimeParameterConstants::c_szTestDeploymentDir, deploymentDir));
            if(deploymentDir.Right(1) != "\\")
            {
                deploymentDir.Append(L"\\");
            }
            return deploymentDir;
        }
    }
} } } }


namespace WEX {
    namespace TestExecution
    {
        // Type traits used by TAEF for logging and comparing values of various types

        template <>
        class VerifyOutputTraits<Platform::String^>
        {
        public:
            static WEX::Common::NoThrowString ToString(Platform::String^ const& platformString)
            {
                return WEX::Common::NoThrowString(platformString->Data());
            }
        };

        template <>
        class VerifyOutputTraits<Platform::StringReference>
        {
        public:
            static WEX::Common::NoThrowString ToString(const Platform::StringReference& stringRef)
            {
                return WEX::Common::NoThrowString(stringRef.Data());
            }
        };

        template <>
        class VerifyOutputTraits<::Windows::Foundation::DateTime>
        {
        public:
            static WEX::Common::NoThrowString ToString(const ::Windows::Foundation::DateTime& dateTime)
            {
                auto formatter = ref new ::Windows::Globalization::DateTimeFormatting::DateTimeFormatter(L"shortdate longtime");
                auto result = WEX::Common::NoThrowString(formatter->Format(dateTime)->Data());

                // The DateTimeFormatter outputs Left-To-Right markers, which get messed up when logged to the console.
                // We remove them from the string:
                result.Remove(L'\u200E');

                return result;
            }
        };

        template <>
        class VerifyCompareTraits<Platform::StringReference, Platform::StringReference>
        {
        public:
            static bool AreEqual(const Platform::StringReference& expected, const Platform::StringReference& actual)
            {
                return expected.GetString() == actual.GetString();
            }
        };

        template <>
        class VerifyCompareTraits<::Windows::Foundation::DateTime, ::Windows::Foundation::DateTime>
        {
        public:
            static bool AreEqual(const ::Windows::Foundation::DateTime& expected, const ::Windows::Foundation::DateTime& actual)
            {
                return expected.UniversalTime == actual.UniversalTime;
            }
        };

        template <>
        class VerifyCompareTraits<::Windows::UI::Color, ::Windows::UI::Color>
        {
        public:
            static bool AreEqual(const ::Windows::UI::Color& expected, const ::Windows::UI::Color& actual)
            {
                return (expected.A == actual.A &&
                    expected.R == actual.R &&
                    expected.G == actual.G &&
                    expected.B == actual.B);
            }
        };

    }
}

namespace XamlOneCoreTransforms {
    constexpr bool IsEnabled() { return false; }
}

