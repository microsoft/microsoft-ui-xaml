// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <FileLoader.h>
#include <TreeHelper.h>
#include <CustomPropertySupport.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <DisableErrorReportingScopeGuard.h>
#include "BindingEvents.h"
#include "TraceConsumerSession.h"
#include "MUX-ETWEvents.h"
#include <TestEvent.h>

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Media;

using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
namespace Tools { namespace ETW { namespace LayoutCausality {


    bool BindingEventTests::ClassSetup()
    {
        // It's very important to call EnsureInitialized on TestServices
        // from ClassSetup. This method will wait for the window to be
        // activated on launch, which avoids a race condition that will block
        // input from being routed to the app. It will also wait for the
        // debugger to attach when the waitForDebugger runtime parameter is
        // specified.
        CommonTestSetupHelper::CommonTestClassSetup();

        return true;
    }

    bool BindingEventTests::TestCleanup()
    {
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    ref class MyConverter sealed : public IValueConverter
    {
    public:
        virtual Object^ Convert(Object^ value, wxaml_interop::TypeName, Object^, String^)
        {
            return safe_cast<double>(value) / 2.0;
        }

        virtual Object^ ConvertBack(Object^ value, wxaml_interop::TypeName, Object^, String^)
        {
            return std::wcstod(safe_cast<String^>(value)->Data(), nullptr) * 2.0;
        }
    };

    void BindingEventTests::CanTraceTwoWayBinding()
    {
        TestCleanupWrapper cleanup;

        TraceConsumerSession session(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER);
        TraceConsumer::EnableTracingByEventId(UpdateTargetBindingBegin_value);
        TraceConsumer::EnableTracingByEventId(UpdateTargetBindingEnd_value);
        TraceConsumer::EnableTracingByEventId(UpdateSourceBindingBegin_value);
        TraceConsumer::EnableTracingByEventId(UpdateSourceBindingEnd_value);
        std::shared_ptr<Event> bindingUpdateEvent = std::make_shared<Event>();

        RunOnUIThread([&bindingUpdateEvent]()
        {
            auto bindingTarget = ref new TextBox;
            auto bindingSource = ref new TextBox;

            auto converter = ref new MyConverter;

            auto binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"Width");
            binding->Mode = BindingMode::TwoWay;
            binding->Converter = converter;
            binding->Source = bindingSource;

            bindingTarget->SetBinding(TextBox::TextProperty, binding);

            bindingSource->Width = 4.0;
            VERIFY_IS_TRUE(L"2" == bindingTarget->Text, L"Invalid value of target property after changing source");

            bindingTarget->Text = L"8.0";
            VERIFY_ARE_EQUAL(16.0, bindingSource->Width, L"Invalid value of source property after changing target");
            bindingUpdateEvent->Set();
        });

        bindingUpdateEvent->WaitForDefault();

        session.Stop();

        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(UpdateTargetBindingBegin_value, 1));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(UpdateTargetBindingEnd_value, 1));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(UpdateSourceBindingBegin_value, 1));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(UpdateSourceBindingEnd_value, 1));
    }

    void BindingEventTests::CanTraceOneWayBinding()
    {
        TestCleanupWrapper cleanup;

        TraceConsumerSession session(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER);
        TraceConsumer::EnableTracingByEventId(UpdateTargetBindingBegin_value);
        TraceConsumer::EnableTracingByEventId(UpdateTargetBindingEnd_value);
        TraceConsumer::EnableTracingByEventId(UpdateSourceBindingBegin_value);
        TraceConsumer::EnableTracingByEventId(UpdateSourceBindingEnd_value);
        std::shared_ptr<Event> bindingUpdateEvent = std::make_shared<Event>();

        RunOnUIThread([&bindingUpdateEvent]()
        {
            auto bindingTarget = ref new TextBox;
            auto bindingSource = ref new Button;

            auto binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"Content");
            binding->Mode = BindingMode::OneWay;
            binding->Source = bindingSource;

            bindingTarget->SetBinding(TextBox::TextProperty, binding);

            auto value = ref new String(L"hello");
            bindingSource->Content = value;
            VERIFY_ARE_EQUAL(value, bindingTarget->Text, L"Invalid value of target property after changing source");

            value = L"hello2";
            bindingSource->Content = value;
            VERIFY_ARE_EQUAL(value, bindingTarget->Text, L"Invalid value of target property after changing source");
            bindingUpdateEvent->Set();
        });

        bindingUpdateEvent->WaitForDefault();

        session.Stop();

        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(UpdateTargetBindingBegin_value, 2));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(UpdateTargetBindingEnd_value, 2));
        // One way binding should not update source
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(UpdateSourceBindingBegin_value, 0));
        VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(UpdateSourceBindingEnd_value, 0));

    }

} } } } } } }
