// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <StringUtilities.h>
#include <FileLoader.h>
#include <TreeHelper.h>
#include <CustomPropertySupport.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <DisableErrorReportingScopeGuard.h>
#include <collection.h>
#include <Utils.h>
#include <RefArray.h>
#include "BindingIntegrationTests.h"
#include <DataSource.h>
#include <InpcDataSource.h>
#include <Behaviors.h>
#include <CustomControl.h>
#include <DataStructureHolder.h>
#include <CustomTypeMetadataProvider.h>
#include <Converter.h>

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;

using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace ::Tests::Native::External::Framework;
using namespace WEX::Logging;

using Color = ::Windows::UI::Color;
using Colors = Microsoft::UI::Colors;

namespace Tests {
    namespace Native {
        namespace External {
            namespace Framework {
                public ref class ArbitraryObject sealed
                {
                };
            }
        }
    }
}

namespace 
{
    void VerifyBindingTraceHelper(Platform::String^ xamlString, Platform::String^ expectedMessage)
    {
        DebugSettings^ debugSettings;
        bool origIsBindingTracingEnabled = FALSE;
        auto bindingFailedRegistration = CreateSafeEventRegistration(DebugSettings, BindingFailed);
        Platform::String^ actualMessage;

        TestServices::Utilities->SetForceDebugSettingsTracingEvents(TRUE);

        TestCleanupWrapper cleanup([&]()
        {
            if (debugSettings != nullptr)
            {
                RunOnUIThread([&]
                {
                    debugSettings->IsBindingTracingEnabled = origIsBindingTracingEnabled;
                    TestServices::Utilities->SetForceDebugSettingsTracingEvents(FALSE);
                });
            }
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        RunOnUIThread([&]
        {
            debugSettings = Application::Current->DebugSettings;
            origIsBindingTracingEnabled = debugSettings->IsBindingTracingEnabled;
            debugSettings->IsBindingTracingEnabled = TRUE;

            bindingFailedRegistration.Attach(
                debugSettings,
                ref new xaml::BindingFailedEventHandler([&actualMessage](Platform::Object^ sender, xaml::BindingFailedEventArgs^ eventArgs)
                {
                    LOG_OUTPUT(L"BindingFailed Event Fired");
                    actualMessage = eventArgs->Message;
                })
            );

            auto rootStackPanel = safe_cast<StackPanel^>(XamlReader::Load(xamlString));

            TestServices::WindowHelper->WindowContent = rootStackPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_ARE_EQUAL(expectedMessage, actualMessage);
    }
}

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
namespace Framework { namespace DataBinding {

    class MultiClassRegistrator
    {
    public:
        static void RegisterDependencyProperties()
        {
            Interactivity::RegisterDependencyProperties();
            MyBehavior::RegisterDependencyProperties();
            CustomControl::RegisterDependencyProperties();
        }

        static void ClearDependencyProperties()
        {
            Interactivity::ClearDependencyProperties();
            MyBehavior::ClearDependencyProperties();
            CustomControl::ClearDependencyProperties();
        }
    };

    Platform::String^ GetFilePath()
    {
        // Get the deployment directory, and then append our test's directory to the end
        auto deploymentDir = GetTestDeploymentDir();
        return ref new Platform::String(deploymentDir + L"resources\\native\\framework\\dataBinding\\");
    }

    bool BindingIntegrationTests::ClassSetup()
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

    bool BindingIntegrationTests::TestSetup()
    {
        TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider(), ref new CustomMetadataRegistrar<MultiClassRegistrator>());
        return true;
    }

    bool BindingIntegrationTests::TestCleanup()
    {
        TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    ref class SCBDataSource sealed : public Microsoft::UI::Xaml::Tests::Common::CustomPropertyProviderBase
    {
        friend void BindingIntegrationTests::BindingWithMatchingSubstringPropertyName();

        xaml_media::Brush^ myBrush_;
        xaml_media::Brush^ myBrush2_;

    protected:
        void AddCustomProperties() override
        {
            AddCustomProperty(L"SCB", xaml_media::SolidColorBrush::typeid,
                MAKEPROPGET(SCBDataSource^, SCB),
                MAKEPROPSET(SCBDataSource^, SCB, xaml_media::SolidColorBrush^)
                );
            AddCustomProperty(L"MyBrush", xaml_media::Brush::typeid,
                MAKEPROPGET(SCBDataSource^, MyBrush),
                MAKEPROPSET(SCBDataSource^, MyBrush, xaml_media::Brush^)
                );
            AddCustomProperty(L"MyBrush2", xaml_media::Brush::typeid,
                MAKEPROPGET(SCBDataSource^, MyBrush2),
                MAKEPROPSET(SCBDataSource^, MyBrush2, xaml_media::Brush^)
                );
        }

    public:
        property xaml_media::SolidColorBrush^ SCB;
        property xaml_media::Brush^ MyBrush
        {
            xaml_media::Brush^ get()
            {
                return myBrush_;
            }

            void set(xaml_media::Brush^ value)
            {
                // Deliberately fire a changed event even if the value hasn't changed
                myBrush_ = value;
                FirePropertyChanged("MyBrush");
            }
        }
        property xaml_media::Brush^ MyBrush2
        {
            xaml_media::Brush^ get()
            {
                return myBrush2_;
            }

            void set(xaml_media::Brush^ value)
            {
                // Deliberately fire a changed event even if the value hasn't changed
                myBrush2_ = value;
                FirePropertyChanged("MyBrush2");
            }
        }
    };

    void BindingIntegrationTests::CanBindBackgroundToSolidColorBrush()
    {
        TestCleanupWrapper cleanup;
        using xaml_media::SolidColorBrush;

        RunOnUIThread([]()
        {
            auto dataSource = ref new SCBDataSource();
            dataSource->SCB = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
            auto grid = ref new Grid;
            grid->DataContext = dataSource;

            auto binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"SCB");
            grid->SetBinding(Grid::BackgroundProperty, binding);

            auto brushOnGrid = safe_cast<SolidColorBrush^>(grid->Background);
            auto brushInSource = safe_cast<SolidColorBrush^>(dataSource->SCB);
            VERIFY_IS_TRUE(
                brushOnGrid->Color.A == brushInSource->Color.A &&
                brushOnGrid->Color.R == brushInSource->Color.R &&
                brushOnGrid->Color.G == brushInSource->Color.G &&
                brushOnGrid->Color.B == brushInSource->Color.B,
                L"Grid.Background did not update");
        });
    }

    ref class PropertyOfUnknownTypeDataSource sealed : public Microsoft::UI::Xaml::Tests::Common::CustomPropertyProviderBase
    {
    protected:
        void AddCustomProperties() override
        {
            wxaml_interop::TypeName unknownTypeName = { L"UnknownNamespace.UnknownType", wxaml_interop::TypeKind::Metadata };

            AddCustomProperty(L"PropertyOfUnknownType", unknownTypeName,
                MAKEPROPGET(PropertyOfUnknownTypeDataSource^, PropertyOfUnknownType),
                MAKEPROPSET(PropertyOfUnknownTypeDataSource^, PropertyOfUnknownType, Object^)
                );
        }

    public:
        property Object^ PropertyOfUnknownType;
    };

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

    void BindingIntegrationTests::CanUpdateTwoWayBindingOnTextBox()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
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
        });
    }

    void BindingIntegrationTests::CanUpdateOneWayBindingWithValueConverter()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            auto bindingTarget = ref new TextBox;
            auto bindingSource = ref new TextBox;

            auto converter = ref new MyConverter;

            auto binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"Width");
            binding->Mode = BindingMode::OneWay;
            binding->Converter = converter;
            binding->Source = bindingSource;

            bindingTarget->SetBinding(TextBox::TextProperty, binding);

            bindingSource->Width = 4.0;
            VERIFY_IS_TRUE(L"2" == bindingTarget->Text, L"Invalid value of target property after changing source");

            // With a one-way binding, ConvertBack shouldn't be called
            bindingTarget->Text = L"8.0";
            VERIFY_ARE_EQUAL(4.0, bindingSource->Width, L"Invalid value of source property after changing target");
        });
    }

    void BindingIntegrationTests::CanBindToEnumProperty()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            auto bindingTarget = ref new TextBox;
            auto bindingSource = ref new TextBox;

            auto binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"Visibility");
            binding->Mode = BindingMode::TwoWay;
            binding->Source = bindingSource;

            bindingTarget->SetBinding(TextBox::VisibilityProperty, binding);

            bindingSource->Visibility = Visibility::Collapsed;
            VERIFY_ARE_EQUAL(Visibility::Collapsed, bindingTarget->Visibility, L"Invalid value of target property after changing source");

            bindingTarget->Visibility = Visibility::Visible;
            VERIFY_ARE_EQUAL(Visibility::Visible, bindingSource->Visibility, L"Invalid value of source property after changing target");
        });
    }

    void BindingIntegrationTests::CanBindObjectToString()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
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

            value = ref new String(L"hello2");
            bindingSource->Content = value;
            VERIFY_ARE_EQUAL(value, bindingTarget->Text, L"Invalid value of target property after changing source");
        });
    }

    void BindingIntegrationTests::CanDefineAttachedPropertyBindingInCode()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            auto b1 = ref new Button;
            auto b2 = ref new Button;
            auto canvas = ref new Canvas;

            canvas->SetLeft(b1, 42.0);
            {
                auto newBinding = ref new Binding;
                newBinding->Source = b1;
                newBinding->Mode = BindingMode::OneWay;
                newBinding->Path = ref new PropertyPath(L"(Canvas.Left)");

                b2->SetBinding(canvas->LeftProperty, newBinding);
            }

            VERIFY_ARE_EQUAL(42.0, canvas->GetLeft(b2));
        });
    }

    void BindingIntegrationTests::CanDefineAttachedPropertyBindingInXaml()
    {
        TestCleanupWrapper cleanup;
        Canvas^ rootCanvas = nullptr;

        RunOnUIThread([&]()
        {
            Platform::String^ xamlContents =
                L"<Canvas xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"    <Button x:Name = 'rootButton' ScrollViewer.IsVerticalScrollChainingEnabled = 'true' />\r\n"
                L"    <TextBlock x:Name = 'targetTextBlock' ScrollViewer.IsVerticalScrollChainingEnabled = '{Binding ElementName=rootButton, Path=(ScrollViewer.IsVerticalScrollChainingEnabled), Mode=TwoWay}' />\r\n"
                L"</Canvas>";

            rootCanvas = safe_cast<Canvas^>(XamlReader::Load(xamlContents));
            TestServices::WindowHelper->WindowContent = rootCanvas;
        });

        TestServices::WindowHelper->WaitForIdle();

        TextBlock^ targetTextBlock = nullptr;
        Button^ rootButton = nullptr;
        RunOnUIThread([&]()
        {
            rootButton = safe_cast<Button^>(rootCanvas->FindName(L"rootButton"));
            targetTextBlock = safe_cast<TextBlock^>(rootCanvas->FindName(L"targetTextBlock"));

            VERIFY_IS_TRUE(ScrollViewer::GetIsVerticalScrollChainingEnabled(targetTextBlock), L"Binding didn't pick up expected value.");

            ScrollViewer::SetIsVerticalScrollChainingEnabled(rootButton, false);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(ScrollViewer::GetIsVerticalScrollChainingEnabled(targetTextBlock), L"Updating binding source didn't update target.");

            ScrollViewer::SetIsVerticalScrollChainingEnabled(targetTextBlock, true);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(ScrollViewer::GetIsVerticalScrollChainingEnabled(rootButton), L"Updating target didn't update source on a two-way binding.");
        });
    }

    void BindingIntegrationTests::BindingWithElementName()
    {
        TestCleanupWrapper cleanup;
        StackPanel^ stackPanel = nullptr;

        RunOnUIThread([&]()
        {
            Platform::String^ stackPanelXaml =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'> "
                L"    <TextBlock x:Name = 'textBlock1' Text = 'testContent' /> "
                L"    <TextBlock x:Name = 'textBlock2' /> "
                L"</StackPanel>";

            stackPanel = safe_cast<StackPanel^>(XamlReader::Load(stackPanelXaml));
            TestServices::WindowHelper->WindowContent = stackPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        TextBlock^ textBlock1 = nullptr;
        TextBlock^ textBlock2 = nullptr;
        RunOnUIThread([&]()
        {
            textBlock1 = safe_cast<TextBlock^>(stackPanel->FindName(L"textBlock1"));
            textBlock2 = safe_cast<TextBlock^>(stackPanel->FindName(L"textBlock2"));

            VERIFY_IS_TRUE(textBlock2->Text == L"");
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            Binding^ binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"Text");
            binding->ElementName = L"textBlock1";
            textBlock2->SetBinding(TextBlock::TextProperty, binding);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(textBlock2->Text == L"testContent");
        });
    }

    void BindingIntegrationTests::CanManuallyPropagateDataContext()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            auto container = ref new StackPanel;

            auto tb = ref new TextBlock;
            tb->SetBinding(TextBlock::TextProperty, ref new Binding);
            tb->SetBinding(TextBlock::TagProperty, ref new Binding);
            container->Children->Append(tb);

            auto dataContextChangedRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::Controls::StackPanel, DataContextChanged);
            dataContextChangedRegistration.Attach(
                container,
                ref new TypedEventHandler<FrameworkElement^, DataContextChangedEventArgs^>([tb](FrameworkElement^, DataContextChangedEventArgs^ args)
            {
                tb->Text = L"Set";
                args->Handled = true;
            }));

            container->DataContext = L"Test";

            VERIFY_IS_TRUE(L"Set" == tb->Text, L"DataContextChanged handler should set the TextBlock.Text");
            VERIFY_IS_NULL(tb->Tag, L"DataContext propagation should be cancelled");

            // Clear DataContext to avoid a leak
            container->ClearValue(Panel::DataContextProperty);
        });
    }

    InpcDataSource^ CreateInpcDataSource()
    {
        auto result = ref new InpcDataSource;
        result->InpcDataSourceProperty = ref new InpcDataSource;
        result->InpcDataSourceProperty->Int32Property = 1729;
        result->InpcDataSourceProperty->StringProperty = L"Inner!!!";

        result->Int32Property = 42;
        result->StringProperty = L"<Outer>";

        return result;
    }

    void BindingIntegrationTests::CanSetDataContext()
    {
        TestCleanupWrapper cleanup;
        auto rootPanel = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetFilePath() + L"SingleNode.xaml"));
        RunOnUIThread([&]()
        {
            auto dataSource = CreateInpcDataSource();
            rootPanel->DataContext = dataSource;

            auto binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"StringProperty");
            rootPanel->SetBinding(Panel::TagProperty, binding);

            VERIFY_ARE_EQUAL(dataSource->StringProperty, safe_cast<String^>(rootPanel->Tag), L"Binding should propagate");

            // Clear DataContext to avoid a leak
            rootPanel->ClearValue(Panel::DataContextProperty);
        });
    }

    void BindingIntegrationTests::CanSetDataContextViaSetValue()
    {
        TestCleanupWrapper cleanup;
        auto rootPanel = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetFilePath() + L"SingleNode.xaml"));
        RunOnUIThread([&]()
        {
            auto dataSource = CreateInpcDataSource();
            rootPanel->SetValue(Panel::DataContextProperty, dataSource);

            auto binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"StringProperty");
            rootPanel->SetBinding(Panel::TagProperty, binding);

            VERIFY_ARE_EQUAL(dataSource->StringProperty, safe_cast<String^>(rootPanel->Tag), L"Binding should propagate");

            // Clear DataContext to avoid a leak
            rootPanel->ClearValue(Panel::DataContextProperty);
        });
    }

    void BindingIntegrationTests::CanUpdateViaInpc()
    {
        TestCleanupWrapper cleanup;
        auto rootPanel = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetFilePath() + L"SingleNode.xaml"));
        RunOnUIThread([&]()
        {
            auto dataSource = CreateInpcDataSource();
            rootPanel->DataContext = dataSource;

            auto binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"StringProperty");
            rootPanel->SetBinding(Panel::TagProperty, binding);

            dataSource->StringProperty += L":)";

            VERIFY_ARE_EQUAL(dataSource->StringProperty, safe_cast<String^>(rootPanel->Tag), L"Binding should propagate");

            // Clear DataContext to avoid a leak
            rootPanel->ClearValue(Panel::DataContextProperty);
        });
    }

    void BindingIntegrationTests::CanUpdateOnlyAfterSettingDataContext()
    {
        TestCleanupWrapper cleanup;
        auto rootPanel = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetFilePath() + L"SingleNode.xaml"));
        RunOnUIThread([&]()
        {
            auto dataSource = CreateInpcDataSource();

            auto binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"StringProperty");
            rootPanel->SetBinding(Panel::TagProperty, binding);
            VERIFY_IS_NULL(rootPanel->Tag, L"Binding without data context should not propagate");

            rootPanel->DataContext = dataSource;
            VERIFY_ARE_EQUAL(dataSource->StringProperty, safe_cast<String^>(rootPanel->Tag), L"Binding should propagate");

            // Clear DataContext to avoid a leak
            rootPanel->ClearValue(Panel::DataContextProperty);
        });
    }

    void BindingIntegrationTests::CanClearValueByClearingDataContext()
    {
        TestCleanupWrapper cleanup;
        auto rootPanel = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetFilePath() + L"SingleNode.xaml"));
        RunOnUIThread([&]()
        {
            auto dataSource = CreateInpcDataSource();
            rootPanel->DataContext = dataSource;

            auto binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"StringProperty");
            rootPanel->SetBinding(Panel::TagProperty, binding);
            VERIFY_ARE_EQUAL(dataSource->StringProperty, rootPanel->Tag->ToString(), L"Binding should propagate");

            // Clear DataContext to avoid a leak
            rootPanel->ClearValue(Panel::DataContextProperty);

            VERIFY_IS_NULL(rootPanel->Tag, L"Clearing data context should clear bound value");
        });
    }

    // Helper to streamline setting bindings
    void SetBinding(FrameworkElement^ element, DependencyProperty^ targetProperty, String^ path, BindingMode mode = BindingMode::OneWay)
    {
        auto binding = ref new Binding;
        binding->Path = ref new PropertyPath(path);
        binding->Mode = mode;
        element->SetBinding(targetProperty, binding);
    }

    void BindingIntegrationTests::CanSetDataContextAtMultipleLevels()
    {
        TestCleanupWrapper cleanup;
        auto rootPanel = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetFilePath() + L"MultipleLevels.xaml"));
        RunOnUIThread([&]()
        {
            WEX::Logging::Log::Comment(L"Find the various pieces of the tree");

            auto textBlock1 = safe_cast<TextBlock^>(rootPanel->FindName(L"textBlock1"));
            VERIFY_IS_NOT_NULL(textBlock1);
            auto panel2 = safe_cast<Panel^>(rootPanel->FindName(L"panel2"));
            VERIFY_IS_NOT_NULL(panel2);
            auto textBlock2_1 = safe_cast<TextBlock^>(rootPanel->FindName(L"textBlock2_1"));
            VERIFY_IS_NOT_NULL(textBlock2_1);
            auto canvas2_2 = safe_cast<Canvas^>(rootPanel->FindName(L"canvas2_2"));
            VERIFY_IS_NOT_NULL(canvas2_2);
            auto panel3 = safe_cast<Panel^>(rootPanel->FindName(L"panel3"));
            VERIFY_IS_NOT_NULL(panel3);
            auto panel3_1 = safe_cast<Panel^>(rootPanel->FindName(L"panel3_1"));
            VERIFY_IS_NOT_NULL(panel3_1);

            auto dataSource1 = CreateInpcDataSource();
            auto dataSource2 = CreateInpcDataSource();
            dataSource2->StringProperty = L"We are the knights...";
            dataSource2->InpcDataSourceProperty->StringProperty = L"... who say 'NI'!";

            WEX::Logging::Log::Comment(L"Set up some bindings before the data context");

            SetBinding(rootPanel, Panel::TagProperty, L"InpcDataSourceProperty.StringProperty");
            SetBinding(textBlock1, TextBlock::TextProperty, L"StringProperty");
            SetBinding(panel2, Panel::TagProperty, L"StringProperty");
            SetBinding(textBlock2_1, TextBlock::TextProperty, L"Int32Property");
            SetBinding(canvas2_2, Canvas::TagProperty, L"StringProperty");
            SetBinding(panel3, Panel::TagProperty, L"StringProperty");
            SetBinding(panel3_1, Panel::TagProperty, L"InpcDataSourceProperty.StringProperty");

            WEX::Logging::Log::Comment(L"Make sure the original default values are still set");

            VERIFY_IS_NULL(rootPanel->Tag, L"Binding without data context should not propagate");
            VERIFY_IS_TRUE(textBlock1->Text->IsEmpty(), L"Binding without data context should not propagate");
            VERIFY_IS_NULL(panel2->Tag, L"Binding without data context should not propagate");
            VERIFY_IS_TRUE(textBlock2_1->Text->IsEmpty(), L"Binding without data context should not propagate");
            VERIFY_IS_NULL(canvas2_2->Tag, L"Binding without data context should not propagate");
            VERIFY_IS_NULL(panel3->Tag, L"Binding without data context should not propagate");
            VERIFY_IS_NULL(panel3_1->Tag, L"Binding without data context should not propagate");

            WEX::Logging::Log::Comment(L"Set data context on panel2 and panel3_1. The root, textBlock1, and panel3 should be unaffected");

            panel2->DataContext = dataSource1;
            panel3_1->DataContext = dataSource2;

            VERIFY_IS_NULL(rootPanel->Tag, L"Binding without data context should not propagate");
            VERIFY_IS_TRUE(textBlock1->Text->IsEmpty(), L"Binding without data context should not propagate");
            VERIFY_ARE_EQUAL(panel2->Tag->ToString(), dataSource1->StringProperty, L"Binding should propagate");
            VERIFY_ARE_EQUAL(textBlock2_1->Text, dataSource1->Int32Property.ToString(), L"Binding should propagate via parent");
            VERIFY_ARE_EQUAL(canvas2_2->Tag->ToString(), dataSource1->StringProperty, L"Binding should propagate via parent");
            VERIFY_IS_NULL(panel3->Tag, L"Binding without data context should not propagate");
            VERIFY_ARE_EQUAL(panel3_1->Tag->ToString(), dataSource2->InpcDataSourceProperty->StringProperty, L"Binding without data context should not propagate");

            WEX::Logging::Log::Comment(L"Clear data context on panel2 and panel3_1. All values should revert to defaults");

            // Clear DataContext to avoid a leak
            panel2->ClearValue(Panel::DataContextProperty);
            panel3_1->ClearValue(Panel::DataContextProperty);
            VERIFY_IS_NULL(rootPanel->Tag, L"Binding without data context should not propagate");
            VERIFY_IS_TRUE(textBlock1->Text->IsEmpty(), L"Binding without data context should not propagate");
            VERIFY_IS_NULL(panel2->Tag, L"Binding without data context should not propagate");
            VERIFY_IS_TRUE(textBlock2_1->Text->IsEmpty(), L"Binding without data context should not propagate");
            VERIFY_IS_NULL(canvas2_2->Tag, L"Binding without data context should not propagate");
            VERIFY_IS_NULL(panel3->Tag, L"Binding without data context should not propagate");
            VERIFY_IS_NULL(panel3_1->Tag, L"Binding without data context should not propagate");
        });
    }

    void BindingIntegrationTests::CanLocalValueOverrideBinding()
    {
        TestCleanupWrapper cleanup;
        auto rootPanel = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetFilePath() + L"MultipleLevels.xaml"));
        RunOnUIThread([&]()
        {
            auto textBlock1 = safe_cast<TextBlock^>(rootPanel->FindName(L"textBlock1"));
            VERIFY_IS_NOT_NULL(textBlock1);

            auto dataSource1 = CreateInpcDataSource();
            rootPanel->DataContext = dataSource1;

            SetBinding(textBlock1, TextBlock::TextProperty, L"StringProperty");

            auto explicitValue = ref new String(L"Explicit value");
            textBlock1->Text = explicitValue;

            auto dataSource2 = CreateInpcDataSource();
            dataSource2->StringProperty = L"I am the walrus...";
            rootPanel->DataContext = dataSource2;
            VERIFY_ARE_EQUAL(explicitValue, textBlock1->Text, L"Explicit value should still be set after changing data context");

            dataSource2->StringProperty = L"...surlaw eht ma I";
            VERIFY_ARE_EQUAL(explicitValue, textBlock1->Text, L"Explicit value should still be set after changing source property");

            // Clear DataContext to avoid a leak
            rootPanel->ClearValue(Panel::DataContextProperty);
        });
    }

    void BindingIntegrationTests::CanContentPresenterUpdateContentBinding()
    {
        TestCleanupWrapper cleanup;
        Button^ templatedParent = nullptr;

        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Visibility='Collapsed'>"
                L"  <Button.Template>"
                L"      <ControlTemplate>"
                L"          <ContentPresenter Content='{Binding StringProperty}'/>"
                L"      </ControlTemplate>"
                L"  </Button.Template>"
                L"</Button>";
            templatedParent = safe_cast<Button^>(XamlReader::Load(xamlString));
            TestServices::WindowHelper->WindowContent = templatedParent;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto dataSource1 = CreateInpcDataSource();
            dataSource1->StringProperty = L"Hello World";
            templatedParent->DataContext = dataSource1;
            templatedParent->Visibility = Visibility::Visible;

            // UpdateLayout should cause the Button's ControlTemplate to expand
            // and the Binding to evaluate. The evaluation of the Binding
            // will create the default ContentTemplate for Button to display the
            // StringProperty.
            templatedParent->UpdateLayout();
            VERIFY_IS_NOT_NULL(templatedParent->ContentTemplateRoot);
        });
    }

    void BindingIntegrationTests::CanCreateRelativeSourceTemplatedParentBindingInCode()
    {
        TestCleanupWrapper cleanup;
        Button^ templatedParent = nullptr;
        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button.Template>"
                L"      <ControlTemplate>"
                L"          <TextBlock x:Name='bindingTarget'/>"
                L"      </ControlTemplate>"
                L"  </Button.Template>"
                L"</Button>";
            templatedParent = safe_cast<Button^>(XamlReader::Load(xamlString));
            TestServices::WindowHelper->WindowContent = templatedParent;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto bindingTarget = safe_cast<TextBlock^>(TreeHelper::GetVisualChildByName(templatedParent, L"bindingTarget"));
            auto binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"TabIndex");
            binding->Mode = BindingMode::TwoWay;
            auto relativeSource = ref new RelativeSource;
            relativeSource->Mode = RelativeSourceMode::TemplatedParent;
            binding->RelativeSource = relativeSource;

            bindingTarget->SetBinding(TextBlock::TextProperty, binding);

            templatedParent->TabIndex = 2;
            VERIFY_IS_TRUE(L"2" == bindingTarget->Text, L"Binding target should update after changing source");

            bindingTarget->Text = L"4";
            VERIFY_IS_TRUE(4 == templatedParent->TabIndex, L"Binding source should update after changing target");
        });
    }

    void BindingIntegrationTests::CanBindRelativeSourceTemplatedParentInDataTemplateInControlTemplate()
    {
        TestCleanupWrapper cleanup;
        Button^ rootButton = nullptr;
        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button.Template>"
                L"      <ControlTemplate>"
                L"          <ContentPresenter x:Name='templatedParent'>"
                L"              <ContentPresenter.ContentTemplate>"
                L"                  <DataTemplate>"
                L"                      <TextBlock x:Name='templateRoot' Text='{Binding Height, Mode=TwoWay, RelativeSource={RelativeSource TemplatedParent}}'/>"
                L"                  </DataTemplate>"
                L"              </ContentPresenter.ContentTemplate>"
                L"          </ContentPresenter>"
                L"      </ControlTemplate>"
                L"  </Button.Template>"
                L"</Button>";
            rootButton = safe_cast<Button^>(XamlReader::Load(xamlString));
            TestServices::WindowHelper->WindowContent = rootButton;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto templatedParent = safe_cast<ContentPresenter^>(TreeHelper::GetVisualChildByName(rootButton, L"templatedParent"));
            auto templateRoot = safe_cast<TextBlock^>(TreeHelper::GetVisualChildByName(templatedParent, L"templateRoot"));

            rootButton->Height = 4;
            templatedParent->Height = 8;

            VERIFY_IS_TRUE(L"8" == templateRoot->Text);
        });
    }

    void BindingIntegrationTests::CanBindRelativeSourceTemplatedParentInItemsPanelTemplateInControlTemplate()
    {
        TestCleanupWrapper cleanup;
        ItemsControl^ rootItemsControl = nullptr;
        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<ItemsControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <ItemsControl.Template>"
                L"      <ControlTemplate>"
                L"          <ItemsControl>"
                L"              <ItemsControl.Template>"
                L"                  <ControlTemplate>"
                L"                      <ItemsPresenter x:Name='templatedParent'/>"
                L"                  </ControlTemplate>"
                L"              </ItemsControl.Template>"
                L"              <ItemsControl.ItemsPanel>"
                L"                  <ItemsPanelTemplate>"
                L"                      <StackPanel x:Name='templateRoot' Height='{Binding Height, Mode=TwoWay, RelativeSource={RelativeSource TemplatedParent}}'/>"
                L"                  </ItemsPanelTemplate>"
                L"              </ItemsControl.ItemsPanel>"
                L"          </ItemsControl>"
                L"      </ControlTemplate>"
                L"  </ItemsControl.Template>"
                L"</ItemsControl>";
            rootItemsControl = safe_cast<ItemsControl^>(XamlReader::Load(xamlString));
            TestServices::WindowHelper->WindowContent = rootItemsControl;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto templatedParent = safe_cast<ItemsPresenter^>(TreeHelper::GetVisualChildByName(rootItemsControl, L"templatedParent"));
            auto templateRoot = safe_cast<StackPanel^>(TreeHelper::GetVisualChildByName(templatedParent, L"templateRoot"));

            rootItemsControl->Height = 4;
            templatedParent->Height = 8;

            VERIFY_ARE_EQUAL(8, templateRoot->Height);
        });
    }

    void BindingIntegrationTests::CanMoveRelativeSourceTemplatedParentBindingTarget()
    {
        TestCleanupWrapper cleanup;
        Button^ templatedParent1 = nullptr;
        Button^ templatedParent2 = nullptr;
        Canvas^ rootPanel = nullptr;
        RunOnUIThread([&]()
        {
            Platform::String^ xamlString1 =
                L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Button.Template>"
                L"        <ControlTemplate>"
                L"            <StackPanel x:Name='templateRoot'>"
                L"                <TextBlock x:Name='bindingTarget' Text='{Binding Height, Mode=OneWay, RelativeSource={RelativeSource TemplatedParent}}'/>"
                L"            </StackPanel>"
                L"        </ControlTemplate>"
                L"    </Button.Template>"
                L"</Button>";
            Platform::String^ xamlString2 =
                L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Button.Template>"
                L"        <ControlTemplate>"
                L"            <StackPanel x:Name='templateRoot'/>"
                L"        </ControlTemplate>"
                L"    </Button.Template>"
                L"</Button>";

            templatedParent1 = safe_cast<Button^>(XamlReader::Load(xamlString1));
            templatedParent2 = safe_cast<Button^>(XamlReader::Load(xamlString2));
            rootPanel = ref new Canvas;
            rootPanel->Children->Append(templatedParent1);
            rootPanel->Children->Append(templatedParent2);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto templateRoot1 = safe_cast<Panel^>(TreeHelper::GetVisualChildByName(templatedParent1, L"templateRoot"));
            auto templateRoot2 = safe_cast<Panel^>(TreeHelper::GetVisualChildByName(templatedParent2, L"templateRoot"));
            auto bindingTarget = safe_cast<TextBlock^>(TreeHelper::GetVisualChildByName(templatedParent1, L"bindingTarget"));

            templatedParent1->Height = 1;
            templatedParent2->Height = 2;

            VERIFY_IS_TRUE(L"1" == bindingTarget->Text);

            templateRoot1->Children->Clear();
            templateRoot2->Children->Append(bindingTarget);

            VERIFY_IS_TRUE(L"1" == bindingTarget->Text);
        });
    }

    void BindingIntegrationTests::CanUpdateRelativeSourceTemplatedParentBindingWithValueConverter()
    {
        TestCleanupWrapper cleanup;
        Button^ templatedParent = nullptr;
        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button.Template>"
                L"      <ControlTemplate>"
                L"          <TextBlock x:Name='bindingTarget'/>"
                L"      </ControlTemplate>"
                L"  </Button.Template>"
                L"</Button>";
            templatedParent = safe_cast<Button^>(XamlReader::Load(xamlString));
            TestServices::WindowHelper->WindowContent = templatedParent;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto bindingTarget = safe_cast<TextBlock^>(TreeHelper::GetVisualChildByName(templatedParent, L"bindingTarget"));

            auto binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"Width");
            binding->Mode = BindingMode::TwoWay;
            binding->Converter = ref new MyConverter;

            auto relativeSource = ref new RelativeSource;
            relativeSource->Mode = RelativeSourceMode::TemplatedParent;
            binding->RelativeSource = relativeSource;

            bindingTarget->SetBinding(TextBlock::TextProperty, binding);

            templatedParent->Width = 4.0;
            VERIFY_IS_TRUE(L"2" == bindingTarget->Text, L"Changing source should update target");

            bindingTarget->Text = "8.0";
            VERIFY_ARE_EQUAL(16.0, templatedParent->Width, L"Changing target should update source");
        });
    }

    ref class CustomConverter sealed : public IValueConverter
    {
    public:
        virtual Object^ Convert(Object^ value, wxaml_interop::TypeName targetType, Object^ parameter, String^ language)
        {
            return m_convertFunction(value, targetType, parameter, language);
        }
        virtual Object^ ConvertBack(Object^ value, wxaml_interop::TypeName targetType, Object^ parameter, String^ language)
        {
            return m_convertBackFunction(value, targetType, parameter, language);
        }
    internal:
        std::function<Object ^ (Object^, wxaml_interop::TypeName, Object^, String^)> m_convertFunction;
        std::function<Object ^ (Object^, wxaml_interop::TypeName, Object^, String^)> m_convertBackFunction;
    };

    void BindingIntegrationTests::ValidateValueConverterParameters()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            auto bindingTarget = ref new TextBlock;
            auto bindingSource = ref new TextBlock;

            auto converter = ref new CustomConverter;
            converter->m_convertFunction = [](Object^ value, wxaml_interop::TypeName targetType, Object^ parameter, String^ language) -> Object^
            {
                wxaml_interop::TypeName stringType = Platform::String::typeid;
                VERIFY_IS_TRUE(stringType == targetType);
                VERIFY_IS_TRUE(L"CParam" == parameter->ToString());
                VERIFY_IS_TRUE(L"en-US" == language);
                return safe_cast<double>(value) / 2.0;
            };
            converter->m_convertBackFunction = [](Object^ value, wxaml_interop::TypeName targetType, Object^ parameter, String^ language) -> Object^
            {
                wxaml_interop::TypeName doubleType = double::typeid;
                VERIFY_IS_TRUE(doubleType == targetType);
                VERIFY_IS_TRUE(L"CParam" == parameter->ToString());
                VERIFY_IS_TRUE(L"en-US" == language);
                double result = std::wcstod(safe_cast<String^>(value)->Data(), nullptr) * 2.0;
                return result.ToString();
            };

            auto binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"Width");
            binding->Mode = BindingMode::TwoWay;
            binding->Converter = converter;
            binding->ConverterParameter = ref new Platform::String(L"CParam");
            binding->ConverterLanguage = L"en-US";
            binding->Source = bindingSource;

            VERIFY_IS_TRUE(L"Width" == binding->Path->Path);
            VERIFY_ARE_EQUAL(BindingMode::TwoWay, binding->Mode);
            VERIFY_ARE_EQUAL(bindingSource, binding->Source);
            VERIFY_ARE_EQUAL(converter, binding->Converter);
            VERIFY_IS_TRUE(L"CParam" == binding->ConverterParameter->ToString());
            VERIFY_IS_TRUE(L"en-US" == binding->ConverterLanguage);

            bindingTarget->SetBinding(TextBlock::TextProperty, binding);

            bindingSource->Width = 4.0;
            VERIFY_IS_TRUE(L"2" == bindingTarget->Text);

            bindingTarget->Text = L"8.0";
            VERIFY_ARE_EQUAL(16.0, bindingSource->Width);
        });
    }

    void BindingIntegrationTests::CanValueConverterExceptionPropagateToApp()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([]()
        {
            auto bindingTarget = ref new TextBlock;
            auto bindingSource = ref new TextBlock;

            auto converter = ref new CustomConverter;
            converter->m_convertFunction = [](Object^, wxaml_interop::TypeName, Object^, String^) -> Object^
            {
                throw ref new Platform::FailureException;
            };
            converter->m_convertBackFunction = [](Object^, wxaml_interop::TypeName, Object^, String^) -> Object^
            {
                throw ref new Platform::FailureException;
            };

            auto binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"Width");
            binding->Mode = BindingMode::TwoWay;
            binding->Converter = converter;
            binding->Source = bindingSource;

            VERIFY_THROWS_WINRT(
                bindingTarget->SetBinding(TextBlock::TextProperty, binding),
                Platform::Exception^,
                L"Exception thrown by Convert method should be propagated to app");

            // Verify binding wasn't set. Otherwise, we'd see another exception here
            bindingTarget->Text = L"New text";

            // Allow Convert to not throw
            converter->m_convertFunction = [](Object^ value, wxaml_interop::TypeName, Object^, String^) -> Object^
            {
                return safe_cast<double>(value) / 2.0;
            };
            bindingTarget->SetBinding(TextBlock::TextProperty, binding);

            VERIFY_THROWS_WINRT(
                bindingTarget->Text = L"Some other text",
                Platform::Exception^,
                L"Exception thrown by ConvertBack method should be propagated to app");
        });
    }

    void BindingIntegrationTests::CanValueConverterHandleSpecialXamlTypes()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            auto binding1 = ref new Binding;
            binding1->Source = L"Not important";

            auto converter1 = ref new CustomConverter;
            binding1->Converter = converter1;

            converter1->m_convertFunction = [](Object^ value, wxaml_interop::TypeName typeName, Object^, String^) -> Object^
            {
                auto passedType = ref new Platform::Type(typeName);
                VERIFY_ARE_EQUAL(passedType, Point::typeid);
                return Point();
            };
            auto lgb = ref new LinearGradientBrush;
            BindingOperations::SetBinding(lgb, LinearGradientBrush::StartPointProperty, binding1);

            auto binding2 = ref new Binding;
            binding2->Source = L"Not important";

            auto converter2 = ref new CustomConverter;
            binding2->Converter = converter2;

            converter2->m_convertFunction = [](Object^ value, wxaml_interop::TypeName typeName, Object^, String^) -> Object^
            {
                auto passedType = ref new Platform::Type(typeName);
                VERIFY_ARE_EQUAL(passedType, Rect::typeid);
                return Rect();
            };
            auto rg = ref new RectangleGeometry;
            BindingOperations::SetBinding(rg, RectangleGeometry::RectProperty, binding2);

            auto binding3 = ref new Binding;
            binding3->Source = L"Not important";

            auto converter3 = ref new CustomConverter;
            binding3->Converter = converter3;

            converter3->m_convertFunction = [](Object^ value, wxaml_interop::TypeName typeName, Object^, String^) -> Object^
            {
                auto passedType = ref new Platform::Type(typeName);
                VERIFY_ARE_EQUAL(passedType, Size::typeid);
                return Size();
            };
            auto arcseg = ref new ArcSegment;
            BindingOperations::SetBinding(arcseg, ArcSegment::SizeProperty, binding3);
        });
    }

    void BindingIntegrationTests::CanUnresolvedPropertyPathUseFallbackValue()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            auto binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"ThisPropertyDoesNotExist");
            binding->FallbackValue = 1729;

            auto target = ref new TextBox;
            target->SetBinding(TextBox::TextProperty, binding);

            VERIFY_IS_TRUE(L"1729" == target->Text, L"FallbackValue should be used when PropertyPath can't be resolved");
        });
    }

    void BindingIntegrationTests::CanTypeConversionFailureUseFallbackValue()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([]()
        {
            auto dataSource = ref new InpcDataSource;
            dataSource->StringProperty = L"abc";

            auto binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"StringProperty");
            binding->FallbackValue = 16;
            binding->Mode = BindingMode::TwoWay;

            auto target = ref new Canvas;
            target->SetBinding(Canvas::WidthProperty, binding);
            target->DataContext = dataSource;

            VERIFY_ARE_EQUAL(16.0, target->Width, L"FallbackValue should be used when TypeConverter fails");

            target->Width = 32.0;
            VERIFY_IS_TRUE(L"32" == dataSource->StringProperty, L"Successful two-way TypeConverter should ignore FallbackValue");

            // Set the binding source to a convertible string, and verify that it gets picked up
            dataSource->StringProperty = L"64.0";
            VERIFY_ARE_EQUAL(64.0, target->Width, L"FallbackValue should not be used when the TypeConverter succeeds");
        });
    }

    void BindingIntegrationTests::CanTwoWayBindingRespectUpdateSourceProperty()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            auto testFun = [](DependencyProperty^ dp, bool useDataContext)
            {
                Platform::Object^ initialValue = ref new String(L"Initial Value");
                Platform::Object^ newValue = ref new String(L"New Value");
                auto target = ref new CustomControl;

                auto dataSource = ref new InpcDataSource;
                dataSource->ObjectProperty = initialValue;

                auto binding = ref new Binding;
                binding->Path = ref new PropertyPath(L"ObjectProperty");
                binding->Mode = BindingMode::TwoWay;
                binding->UpdateSourceTrigger = UpdateSourceTrigger::Explicit;

                if (useDataContext) {
                    target->DataContext = dataSource;
                }
                else {
                    binding->Source = dataSource;
                }

                target->SetBinding(dp, binding);
                auto bindingExpression = target->GetBindingExpression(dp);

                // Now that we are set up, let's do some actual work
                // Change the target value and verify the source isn't updated
                target->SetValue(dp, newValue);
                VERIFY_ARE_EQUAL(newValue, target->GetValue(dp), L"Target DP should always return the new value");
                VERIFY_ARE_EQUAL(initialValue, dataSource->ObjectProperty, L"Source should return the old value before UpdateSource is called");

                // Propagate update to source
                bindingExpression->UpdateSource();
                VERIFY_ARE_EQUAL(newValue, target->GetValue(dp), L"Target DP should always return the new value");
                VERIFY_ARE_EQUAL(newValue, dataSource->ObjectProperty, L"Source should return the new value after UpdateSource is called");
            };

            ref_array<DependencyProperty^, 2> propertyValues = { CustomControl::WorkingTagProperty, CustomControl::AttachedTagProperty };
            std::array<bool, 2> useDataContextValues = { false, true };

            for (auto& propertyValue : propertyValues)
            {
                for (auto& useDataContext : useDataContextValues)
                {
                    testFun(propertyValue, useDataContext);
                }
            }
        });
    }

    void BindingIntegrationTests::CanTwoWayBindingRespectUpdateSourcePropertyWithValueConverter()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            auto testFun = [](DependencyProperty^ dp, bool useDataContext)
            {
                Platform::Object^ initialValue = ref new String(L"Initial Value");
                Platform::Object^ newValue = ref new String(L"New Value");
                Platform::Object^ firstValueReturnedByConvert = ref new String(L"First convert");
                Platform::Object^ secondValueReturnedByConvert = ref new String(L"Second convert");
                Platform::Object^ firstValueReturnedByConvertBack = ref new String(L"First convert back");
                auto target = ref new CustomControl;

                auto dataSource = ref new InpcDataSource;
                dataSource->ObjectProperty = initialValue;

                auto binding = ref new Binding;
                binding->Path = ref new PropertyPath(L"ObjectProperty");
                binding->Mode = BindingMode::TwoWay;
                binding->UpdateSourceTrigger = UpdateSourceTrigger::Explicit;

                auto converter = ref new CustomConverter;
                converter->m_convertFunction = [=](Object^ value, wxaml_interop::TypeName typeName, Object^, String^)->Object^
                {
                    return (value == initialValue) ? firstValueReturnedByConvert : secondValueReturnedByConvert;
                };
                converter->m_convertBackFunction = [=](Object^ value, wxaml_interop::TypeName typeName, Object^, String^)->Object^
                {
                    return firstValueReturnedByConvertBack;
                };
                binding->Converter = converter;

                if (useDataContext) {
                    target->DataContext = dataSource;
                }
                else {
                    binding->Source = dataSource;
                }

                target->SetBinding(dp, binding);
                auto bindingExpression = target->GetBindingExpression(dp);

                // Now that we are set up, let's do some actual work
                VERIFY_ARE_EQUAL(firstValueReturnedByConvert, target->GetValue(dp), L"Invalid target value before target change");
                VERIFY_ARE_EQUAL(initialValue, dataSource->ObjectProperty, L"Invalid source value before target change");

                // Change the target value and verify the source isn't updated
                target->SetValue(dp, newValue);
                VERIFY_ARE_EQUAL(newValue, target->GetValue(dp), L"Target DP should always return the new value");
                VERIFY_ARE_EQUAL(initialValue, dataSource->ObjectProperty, L"Source should return the old value before UpdateSource is called");

                // Propagate update to source
                bindingExpression->UpdateSource();
                VERIFY_ARE_EQUAL(newValue, target->GetValue(dp), L"Target DP should always return the new value");
                VERIFY_ARE_EQUAL(firstValueReturnedByConvertBack, dataSource->ObjectProperty, L"Source should return the new value after UpdateSource is called");
            };

            ref_array<DependencyProperty^, 2> propertyValues = { CustomControl::WorkingTagProperty, CustomControl::AttachedTagProperty };
            std::array<bool, 2> useDataContextValues = { false, true };

            for (auto& propertyValue : propertyValues)
            {
                for (auto& useDataContext : useDataContextValues)
                {
                    testFun(propertyValue, useDataContext);
                }
            }
        });
    }

    void BindingIntegrationTests::CanTargetNullValueUpdateTarget()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            auto runTargetTest = [](BindingMode mode)
            {
                auto nullValue = ref new Platform::String(L"Null Value String");
                auto dataSource = ref new InpcDataSource;
                dataSource->StringProperty = nullptr;

                auto binding = ref new Binding;
                binding->Path = ref new PropertyPath(L"StringProperty");
                binding->Mode = mode;
                binding->TargetNullValue = nullValue;
                binding->Source = dataSource;

                auto target = ref new TextBox;
                target->SetBinding(TextBox::TextProperty, binding);

                VERIFY_ARE_EQUAL(nullValue, target->Text, L"TargetNullValue should have been set to target property");
            };

            const std::array<BindingMode, 3> modeArray = { BindingMode::OneTime, BindingMode::OneWay, BindingMode::TwoWay };
            for (auto& mode : modeArray)
            {
                runTargetTest(mode);
            }
        });
    }

    void BindingIntegrationTests::CanTargetNullValueUpdateSource()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            auto nullValueString = ref new Platform::String(L"Null Value String");
            auto dataSource = ref new InpcDataSource;
            dataSource->StringProperty = L"Some lame string";

            auto binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"StringProperty");
            binding->Mode = BindingMode::TwoWay;
            binding->TargetNullValue = nullValueString;
            binding->Source = dataSource;

            auto target = ref new TextBox;
            target->SetBinding(TextBox::TextProperty, binding);

            target->Text = nullValueString;

            VERIFY_IS_NULL(dataSource->StringProperty, L"Binding should set source value to null");
        });
    }

    void BindingIntegrationTests::CanBindToUserControlContentProperty()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            auto myString = ref new Platform::String(L"Microsoft Bingo");
            auto binding = ref new Binding;

            binding->Mode = BindingMode::OneTime;
            binding->Source = myString;

            auto userControl = ref new UserControl;
            userControl->SetBinding(UserControl::ContentProperty, binding);
        });
    }

    void BindingIntegrationTests::CanGetBindingExpressionGetBinding()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            auto runTest = [](BindingMode mode, DependencyProperty^ dp, bool useDataContext)
            {
                auto target = ref new CustomControl;
                VERIFY_IS_NULL(target->GetBindingExpression(dp), L"BindingExpression should be null on target without binding set");

                auto dataSource = ref new InpcDataSource;
                dataSource->ObjectProperty = ref new Platform::String(L"Initial source value");

                auto binding = ref new Binding;
                binding->Path = ref new PropertyPath(L"ObjectProperty");
                binding->Mode = mode;

                if (useDataContext)
                {
                    target->DataContext = dataSource;
                }
                else
                {
                    binding->Source = dataSource;
                }

                target->SetBinding(dp, binding);

                auto bindingExpression = target->GetBindingExpression(dp);
                VERIFY_IS_NOT_NULL(bindingExpression, L"GetBindingExpression() should not return null for data bound target");
                VERIFY_ARE_EQUAL(dataSource, safe_cast<InpcDataSource^>(bindingExpression->DataItem), L"BindingExpression.DataItem should match the data source");
                VERIFY_ARE_EQUAL(binding, bindingExpression->ParentBinding, L"BindingExpression.ParentBinding should match binding");
            };

            std::array<bool, 2> useDataContextValues = { true, false };
            ref_array<DependencyProperty^, 2> dpValues = { CustomControl::WorkingTagProperty, CustomControl::AttachedTagProperty };
            std::array<BindingMode, 3> modeValues = { BindingMode::OneTime, BindingMode::OneWay, BindingMode::TwoWay };

            for (auto& useDataContext : useDataContextValues)
            {
                for (auto& dp : dpValues)
                {
                    for (auto& mode : modeValues)
                    {
                        runTest(mode, dp, useDataContext);
                    }
                }
            }
        });
    }

    void BindingIntegrationTests::CanBindToCollectionIndexer()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            auto dataSource = ref new DataStructureHolder;
            dataSource->Collection = ref new Platform::Collections::Vector < int > ;

            for (int i = 6; i >= 0; --i)
            {
                dataSource->Collection->Append(i);
            }

            // The collection is now: {6, [5], 4, 3, 2, 1, 0}

            auto binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"Collection[1]");
            binding->Mode = BindingMode::TwoWay;

            auto tb = ref new TextBlock;
            tb->SetBinding(TextBlock::TextProperty, binding);
            tb->DataContext = dataSource;

            // 1) Check that the value is the current value
            // {6, [5], 4, 3, 2, 1, 0}
            VERIFY_IS_TRUE(tb->Text == L"5", L"The initial value should match");

            // 2) Insert a value at the 0th position, the previous value at the
            // 0th position should be come the new value
            // {25, [6], 5, 4, 3, 2, 1, 0}
            dataSource->Collection->InsertAt(0, 25);
            VERIFY_IS_TRUE(tb->Text == L"6", L"Value after insertion");

            // 3) Replace a value at the 1st position, that should become the new value
            // {25, [50], 5, 4, 3, 2, 1, 0}
            dataSource->Collection->SetAt(1, 50);
            VERIFY_IS_TRUE(tb->Text == L"50", L"Value after replacing");

            // 4) Remove a value at the 0th position, we should go back to 5
            // {50, [5], 4, 3, 2, 1, 0}
            dataSource->Collection->RemoveAt(0);
            VERIFY_IS_TRUE(tb->Text == L"5", L"Value after removing");

            // 5) Adding after the 1st index should not change the value
            // {50, [5], 4, 3, 2, 1, 0, 10}
            dataSource->Collection->Append(10);
            VERIFY_IS_TRUE(tb->Text == L"5", L"Adding at the end of the collection should not change the current value");

            // 6) Modifying something after the 1st index should not change the value
            // {50, [5], 25, 3, 2, 1, 0, 10}
            dataSource->Collection->SetAt(2, 25);
            VERIFY_IS_TRUE(tb->Text == L"5", L"Modifying the collection after the current value should not change the current value");

            // 7) Removing at the 1st position should change the value
            // {50, [25], 3, 2, 1, 0, 10}
            dataSource->Collection->RemoveAt(1);
            VERIFY_IS_TRUE(tb->Text == L"25", L"Removing the current value should have changed the current value");

            // 8) Clear the collection, that should make the value now empty
            // {}
            // Regression coverage for 577186 - ObservableCollection.Clear() can throw an exception when you are binding to that collection with an indexer
            dataSource->Collection->Clear();
            VERIFY_IS_TRUE(tb->Text == L"", L"The value after clearing the collection should be empty");
        });
    }

    void BindingIntegrationTests::DoesFloatBindingExpressionTriggerChange()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            auto dataSource = ref new InpcDataSource;
            dataSource->DoubleProperty = 0.0;

            auto binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"DoubleProperty");

            auto slider = ref new Slider;

            bool propertyChanged = false;
            slider->RegisterPropertyChangedCallback(Slider::MinimumProperty,
                ref new DependencyPropertyChangedCallback([&](DependencyObject^ sender, DependencyProperty^ prop) {
                propertyChanged = true;
            }));

            slider->SetBinding(Slider::MinimumProperty, binding);
            slider->DataContext = dataSource;

            VERIFY_IS_FALSE(propertyChanged);
        });
    }

    void BindingIntegrationTests::DoesBindingExpressionPersistAfterCoercion()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            auto dataSource = ref new InpcDataSource;
            dataSource->DoubleProperty = 0.0;

            auto binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"DoubleProperty");
            binding->Mode = BindingMode::OneWay;

            auto slider = ref new Slider;
            slider->Maximum = 100;
            double nextExpectedValue = 0;
            bool expectsNotification = true;
            slider->RegisterPropertyChangedCallback(Slider::ValueProperty,
                ref new DependencyPropertyChangedCallback([&](DependencyObject^ sender, DependencyProperty^ prop) {
                std::wstringstream s;
                s << slider->Value << L" [" << slider->Minimum << L" - " << slider->Maximum << L"]";
                Log::Comment(s.str().c_str());
                VERIFY_IS_TRUE(expectsNotification);
                if (!_isnan(nextExpectedValue))
                {
                    VERIFY_ARE_EQUAL(nextExpectedValue, slider->Value);
                }
                expectsNotification = !expectsNotification;
            }));

            slider->SetBinding(Slider::ValueProperty, binding);
            slider->DataContext = dataSource;

            // changing the minimum so that the value is coerced to be the minimum
            nextExpectedValue = 20;
            slider->Minimum = 20;
            VERIFY_IS_FALSE(expectsNotification);

            expectsNotification = true;
            // Force coercion through the bound property and verify that the Value property was coerced to Maximum (i.e. 100)
            nextExpectedValue = 100;
            dataSource->DoubleProperty = 200;
            VERIFY_IS_FALSE(expectsNotification);

            // we should not get a Value Changed notification for this:
            expectsNotification = false;
            dataSource->DoubleProperty = 400;
            // we won't have flipped this because the notification should never happen
            VERIFY_IS_FALSE(expectsNotification);

            // Verify that we haven't lost the binding when we coerced
            expectsNotification = true;
            nextExpectedValue = 50;
            dataSource->DoubleProperty = 50;
            VERIFY_IS_FALSE(expectsNotification);

            expectsNotification = true;
            nextExpectedValue = 25;
            slider->Maximum = 25;
            VERIFY_IS_FALSE(expectsNotification);
        });
    }

    void BindingIntegrationTests::CanTemplateBindCoreSourcePropertyToCustomTargetProperty()
    {
        TestCleanupWrapper cleanup;
        auto rootPanel = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetFilePath() + L"TemplateBindings.xaml"));
        CustomControl^ cc1 = nullptr;
        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto cc1 = static_cast<CustomControl^>(TreeHelper::GetVisualChildByName(rootPanel, L"cc1"));
            auto cc1target = static_cast<CustomControl^>(TreeHelper::GetVisualChildByName(cc1, L"cc1target"));

            VERIFY_ARE_EQUAL(cc1->Width, cc1target->MyWidth);

            cc1->Width = 42;
            VERIFY_ARE_EQUAL(42, cc1target->MyWidth);

            cc1->Width = 43;
            VERIFY_ARE_EQUAL(43, cc1target->MyWidth);
        });
    }

    void BindingIntegrationTests::CanBindChildInDependencyObjectCollection()
    {
        TestCleanupWrapper cleanup;

        auto rootPanel = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetFilePath() + L"Behaviors.xaml"));

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = rootPanel;

            auto behaviors = Interactivity::GetBehaviors(rootPanel);
            VERIFY_IS_NOT_NULL(behaviors);
            VERIFY_ARE_EQUAL(1u, behaviors->Size);

            auto actions = static_cast<MyBehavior^>(behaviors->GetAt(0))->Actions;
            VERIFY_IS_NOT_NULL(actions);
            VERIFY_ARE_EQUAL(1u, actions->Size);

            auto scb = static_cast<SolidColorBrush^>(actions->GetAt(0));
            VERIFY_IS_NOT_NULL(scb);

            rootPanel->DataContext = Colors::Red;
            VERIFY_ARE_EQUAL(Colors::Red, scb->Color);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void BindingIntegrationTests::CanBindToAnimation()
    {
        TestCleanupWrapper cleanup;

        DoubleAnimation^ animation = nullptr;
        auto userControl = safe_cast<UserControl^>(LoadXamlFileOnUIThread(GetFilePath() + L"Animations.xaml"));

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = userControl;

            auto vsgs = VisualStateManager::GetVisualStateGroups(static_cast<FrameworkElement^>(userControl->Content));
            auto sb = vsgs->GetAt(0)->States->GetAt(0)->Storyboard;
            animation = static_cast<DoubleAnimation^>(sb->Children->GetAt(0));

            // WPF_HOSTING_MODE_FAILURE : To->Value is 42 here.
            // AreEqual(0.0, animation->To->Value) - Values (0.000000l, 42.000000l)
            VERIFY_ARE_EQUAL(0.0, animation->To->Value);

            userControl->UpdateLayout();
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(42.0, animation->To->Value);
        });
    }

    void BindingIntegrationTests::VerifyDataTemplateExtensions()
    {
        TestCleanupWrapper cleanup;
        ListView^ rootListView = nullptr;
        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<ListView x:Name='LV' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"</ListView>";
            rootListView = safe_cast<ListView^>(XamlReader::Load(xamlString));
            TestServices::WindowHelper->WindowContent = rootListView;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto extension = ref new LocalDataTemplateExtension();
            DataTemplate::SetExtensionInstance(rootListView, extension);
            VERIFY_IS_TRUE(extension->Equals(DataTemplate::GetExtensionInstance(rootListView)));
        });

        TestServices::WindowHelper->WaitForIdle(false);
    }

    void BindingIntegrationTests::CanUseThemeResourceBindingBetweenColorPropertyAndSolidColorBrushValue()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([]()
        {
            Platform::String^ xamlString =
                L"<Border xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Border.Resources>"
                L"    <SolidColorBrush x:Key='MyBrush'>Red</SolidColorBrush>"
                L"  </Border.Resources>"
                L"  <Border>"
                L"    <Border.Background>"
                L"      <SolidColorBrush Color='{ThemeResource MyBrush}' />"
                L"    </Border.Background>"
                L"  </Border>"
                L"</Border>";
            auto rootBorder = safe_cast<Border^>(XamlReader::Load(xamlString));
            auto nestedBorder = safe_cast<Border^>(rootBorder->Child);

            // Force change of theme.
            // There should not be an exception during the change, though the conversion from
            // SolidColorBrush (MyBrush) to Color should fail silently.
            rootBorder->RequestedTheme = ElementTheme::Light;
            rootBorder->RequestedTheme = ElementTheme::Dark;

            VERIFY_ARE_NOT_EQUAL(Colors::Red, safe_cast<SolidColorBrush^>(nestedBorder->Background)->Color);

            TestServices::WindowHelper->WindowContent = rootBorder;
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void BindingIntegrationTests::DataContextChangesInParentChildTree()
    {
        TestCleanupWrapper cleanup;

        StackPanel^ parent;
        Button^ child;
        Platform::WeakReference weakRef;
        Platform::WeakReference weakRefChild;
        unsigned int changedCountParent = 0;
        unsigned int changedCountChild = 0;
        Object^ lastNewValueParent;
        Object^ lastNewValueChild;

        auto changedRegistrationParent = CreateSafeEventRegistration(Microsoft::UI::Xaml::Controls::StackPanel, DataContextChanged);
        auto changedRegistrationChild = CreateSafeEventRegistration(Microsoft::UI::Xaml::Controls::Button, DataContextChanged);

        // Setup simple parent/child tree.
        RunOnUIThread([&]()
        {
            parent = ref new StackPanel;
            child = ref new Button;

            parent->Children->Append(child);

            TestServices::WindowHelper->WindowContent = parent;
        });
        TestServices::WindowHelper->WaitForIdle(false);

        // Attach DataContextChanged listeners.
        // Do DataContext property changes and verify property values, object lifetime, events, etc.
        RunOnUIThread([&]()
        {
            changedRegistrationParent.Attach(
                parent,
                ref new TypedEventHandler<FrameworkElement^, DataContextChangedEventArgs^>(
                [&](FrameworkElement^, DataContextChangedEventArgs^ args)
                {
                    lastNewValueParent = args->NewValue;
                    changedCountParent++;
                }));
            changedRegistrationChild.Attach(
                child,
                ref new TypedEventHandler<FrameworkElement^, DataContextChangedEventArgs^>(
                [&](FrameworkElement^, DataContextChangedEventArgs^ args)
                {
                    lastNewValueChild = args->NewValue;
                    changedCountChild++;
                }));

            // Initially, set a new object on the parent's DataContext property.
            // Keep a weak ref on the object to check its lifetime later.
            auto dataContext = ref new LocalObject;
            weakRef = dataContext;

            parent->DataContext = dataContext;
            dataContext = nullptr;

            // Check the state after setting the DataContext property.
            // Both the parent's and child's DataContextChanged should have fired.
            VERIFY_IS_TRUE(changedCountParent == 1U);
            VERIFY_IS_TRUE(changedCountChild == 1U);
            VERIFY_IS_TRUE(lastNewValueParent == parent->DataContext);
            VERIFY_IS_TRUE(lastNewValueChild == child->DataContext);
            VERIFY_IS_NOT_NULL(weakRef.Resolve<LocalObject>());
            VERIFY_IS_TRUE(child->DataContext == parent->DataContext);
            VERIFY_IS_TRUE(child->GetValue(FrameworkElement::DataContextProperty) == parent->DataContext);

            // Clear DataContext on the child.
            // This should have no effect since the current value is inherited.
            child->ClearValue(FrameworkElement::DataContextProperty);

            VERIFY_IS_TRUE(changedCountParent == 1U);
            VERIFY_IS_TRUE(changedCountChild == 1U);
            VERIFY_IS_NOT_NULL(weakRef.Resolve<LocalObject>());
            VERIFY_IS_TRUE(parent->DataContext != nullptr);
            VERIFY_IS_TRUE(child->DataContext == parent->DataContext);
            VERIFY_IS_TRUE(child->GetValue(FrameworkElement::DataContextProperty) == parent->DataContext);

            // Reset the parent's DataContext to a new object.
            // Both the parent's and child's DataContextChanged should fire.
            // The original data object should be gone.
            parent->DataContext = ref new LocalObject;

            VERIFY_IS_TRUE(changedCountParent == 2U);
            VERIFY_IS_TRUE(changedCountChild == 2U);
            VERIFY_IS_TRUE(lastNewValueParent == parent->DataContext);
            VERIFY_IS_TRUE(lastNewValueChild == child->DataContext);
            VERIFY_IS_NULL(weakRef.Resolve<LocalObject>());
            VERIFY_IS_TRUE(parent->DataContext != nullptr);
            VERIFY_IS_TRUE(child->DataContext == parent->DataContext);
            VERIFY_IS_TRUE(child->GetValue(FrameworkElement::DataContextProperty) == parent->DataContext);

            weakRef = parent->DataContext;

            // Set child's DataContext to a new object.
            // Only the child's DataContextChanged should fire.
            // DataContextChangedArgs.NewValue should be the new object.
            child->DataContext = ref new LocalObject;

            VERIFY_IS_TRUE(changedCountParent == 2U);
            VERIFY_IS_TRUE(changedCountChild == 3U);
            VERIFY_IS_TRUE(lastNewValueParent == parent->DataContext);
            VERIFY_IS_TRUE(lastNewValueChild == child->DataContext);
            VERIFY_IS_NOT_NULL(weakRef.Resolve<LocalObject>());
            VERIFY_IS_TRUE(parent->DataContext != nullptr);
            VERIFY_IS_TRUE(child->DataContext != nullptr);
            VERIFY_IS_TRUE(child->DataContext != parent->DataContext);
            VERIFY_IS_TRUE(child->GetValue(FrameworkElement::DataContextProperty) != parent->DataContext);

            weakRefChild = child->DataContext;

            // Clear DataContext on the child.
            // Only the child's DataContextChanged event should fire.
            // DataContextChangedArgs.NewValue should be the inherited object.
            child->ClearValue(FrameworkElement::DataContextProperty);

            VERIFY_IS_TRUE(changedCountParent == 2U);
            VERIFY_IS_TRUE(changedCountChild == 4U);
            VERIFY_IS_TRUE(lastNewValueParent == parent->DataContext);
            VERIFY_IS_TRUE(lastNewValueChild == parent->DataContext);
            VERIFY_IS_NOT_NULL(weakRef.Resolve<LocalObject>());
            VERIFY_IS_NULL(weakRefChild.Resolve<LocalObject>());
            VERIFY_IS_TRUE(parent->DataContext != nullptr);
            VERIFY_IS_TRUE(child->DataContext != nullptr);
            VERIFY_IS_TRUE(child->DataContext == parent->DataContext);
            VERIFY_IS_TRUE(child->GetValue(FrameworkElement::DataContextProperty) == parent->DataContext);

            // Reset the parent's DataContext again to a new object.
            // Both the parent's and child's DataContextChanged events should fire.
            parent->DataContext = ref new LocalObject;

            VERIFY_IS_TRUE(changedCountParent == 3U);
            VERIFY_IS_TRUE(changedCountChild == 5U);
            VERIFY_IS_TRUE(lastNewValueParent == parent->DataContext);
            VERIFY_IS_TRUE(lastNewValueChild == parent->DataContext);
            VERIFY_IS_NULL(weakRef.Resolve<LocalObject>());
            VERIFY_IS_TRUE(parent->DataContext != nullptr);
            VERIFY_IS_TRUE(child->DataContext == parent->DataContext);
            VERIFY_IS_TRUE(child->GetValue(FrameworkElement::DataContextProperty) == parent->DataContext);

            // Finally, clear the parent's DataContext.
            // Both the parent's and child's DataContextChanged events should fire.
            // DataContextChangedArgs.NewValue should be null.
            parent->ClearValue(FrameworkElement::DataContextProperty);

            VERIFY_IS_TRUE(changedCountParent == 4U);
            VERIFY_IS_TRUE(changedCountChild == 6U);
            VERIFY_IS_NULL(lastNewValueParent);
            VERIFY_IS_NULL(lastNewValueChild);
            VERIFY_IS_NULL(parent->DataContext);
            VERIFY_IS_TRUE(child->DataContext == parent->DataContext);
            VERIFY_IS_TRUE(child->GetValue(FrameworkElement::DataContextProperty) == parent->DataContext);
        });
    }

    // Test when an InheritanceContext change occurs, to start binding
    void BindingIntegrationTests::InheritanceContextChange()
    {
        TestCleanupWrapper cleanup;

        StackPanel^ stackPanel = nullptr;
        Storyboard^ storyboard = nullptr;
        DoubleAnimationUsingKeyFrames^ animation = nullptr;
        wfc::IVector<VisualStateGroup^>^ visualStateGroups = nullptr;

        RunOnUIThread([&]()
        {
            // Create StackPanel with a VisualState without a storyboard and put it in the visual tree
            Platform::String^ stackPanelXaml =
            L"<StackPanel Width='100' Height='100' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"   <VisualStateManager.VisualStateGroups>"
            L"     <VisualStateGroup>"
            L"      <VisualState>"
            L"      </VisualState>"
            L"    </VisualStateGroup>"
            L"  </VisualStateManager.VisualStateGroups>"
            L"</StackPanel>";
            stackPanel = safe_cast<StackPanel^>(XamlReader::Load(stackPanelXaml));
            TestServices::WindowHelper->WindowContent = stackPanel;

           // Set DataContext, which will be bound later to DoubleAnimationUsingKeyFrames.AutoReverse
           stackPanel->DataContext = true;

            // Create Storyboard with a binding in an animation.
            Platform::String^ storyboardXaml =
                L"<Storyboard x:Name='storyboard1' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <DoubleAnimationUsingKeyFrames"
                L"    Storyboard.TargetName='storyboardTarget'"
                L"    Storyboard.TargetProperty='Width'"
                L"    Duration='0:0:2'"
                L"    AutoReverse='{Binding}'>"
                L"    <LinearDoubleKeyFrame Value='50' KeyTime='0:0:1'/>"
                L"    <LinearDoubleKeyFrame Value='60' KeyTime='0:0:2'/>"
                L"  </DoubleAnimationUsingKeyFrames>"
                L"</Storyboard>";
            storyboard = safe_cast<Storyboard^>(XamlReader::Load(storyboardXaml));

            animation = static_cast<DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(0));
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Verify that animation's AutoReverse property is not bound to StackPanel's DataContext
            VERIFY_ARE_EQUAL(false, animation->AutoReverse);

            // Add animation as a property value of StackPanel.
            stackPanel->Tag = animation;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Verify that animation's AutoReverse property is not bound to StackPanel's DataContext because
            // adding to Tag property will not give the animation an Inheritance Context
            VERIFY_ARE_EQUAL(false, animation->AutoReverse);

            // Add animation to StackPanel's visual state.
            visualStateGroups = VisualStateManager::GetVisualStateGroups(stackPanel);
            visualStateGroups->GetAt(0)->States->GetAt(0)->Storyboard = storyboard;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Verify that animation's AutoReverse property is  bound to StackPanel's DataContext because
            // adding it to the StackPanel visual tree gives the animation an Inheritance Context
            VERIFY_ARE_EQUAL(true, animation->AutoReverse);
       });
    }

    void BindingIntegrationTests::DataContextObjectIdentity()
    {
        TestCleanupWrapper cleanup;

        // WinRT object set in code
        RunOnUIThread([]()
        {
            auto parent = ref new StackPanel;
            auto child = ref new Button;
            parent->Children->Append(child);

            parent->DataContext = ref new LocalObject;
            VERIFY_IS_TRUE(child->DataContext == parent->DataContext);
        });

        // String set in code
        RunOnUIThread([]()
        {
            auto parent = ref new StackPanel;
            auto child = ref new Button;
            parent->Children->Append(child);

            parent->DataContext = L"StringDataContext";
            VERIFY_IS_TRUE(child->DataContext == parent->DataContext);
        });

        // WinRT object set in xaml
        RunOnUIThread([]()
        {
            Platform::String^ xamlString =
                L"<StackPanel"
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel.DataContext>"
                L"    <Grid />"
                L"  </StackPanel.DataContext>"
                L"  <Button />"
                L"</StackPanel>";

            auto parent = safe_cast<StackPanel^>(XamlReader::Load(xamlString));
            auto child = safe_cast<Button^>(parent->Children->GetAt(0));

            VERIFY_IS_TRUE(child->DataContext == parent->DataContext);
        });

        // String set in xaml
        // See Reading DataContext doesn't respect object identity (TH regression)
        /*
        RunOnUIThread([]()
        {
            Platform::String^ xamlString =
                L"<StackPanel"
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  DataContext='Hello'>"
                L"  <Button />"
                L"</StackPanel>";

            auto parent = safe_cast<StackPanel^>(XamlReader::Load(xamlString));
            auto child = safe_cast<Button^>(parent->Children->GetAt(0));

            VERIFY_IS_TRUE(child->DataContext == parent->DataContext);
        });
        */
    }

    template<typename T>
    T ConvertValue(::Platform::Object^ value)
    {
        ::Platform::Object^ convertedObjectValue = ::Microsoft::UI::Xaml::Markup::XamlBindingHelper::ConvertValue(T::typeid, value);
        return safe_cast<T>(convertedObjectValue);
    }

    template<typename T>
    T^ ConvertRefValue(::Platform::Object^ value)
    {
        ::Platform::Object^ convertedObjectValue = ::Microsoft::UI::Xaml::Markup::XamlBindingHelper::ConvertValue(T::typeid, value);
        return safe_cast<T^>(convertedObjectValue);
    }

    void BindingIntegrationTests::CanConvertFromStringToPrimitiveType()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([]()
        {
            // String -> Primitive Type
            VERIFY_ARE_EQUAL(ConvertValue<short>("27"), 27);
            VERIFY_ARE_EQUAL(ConvertValue<unsigned short>("27"), (unsigned short)27);
            VERIFY_ARE_EQUAL(ConvertValue<int>("27"), 27);
            VERIFY_ARE_EQUAL(ConvertValue<unsigned int>("27"), (unsigned int)27);
            VERIFY_ARE_EQUAL(ConvertValue<__int64>("27"), 27);
            VERIFY_ARE_EQUAL(ConvertValue<unsigned __int64>("27"), (unsigned __int64)27);
            VERIFY_ARE_EQUAL(ConvertValue<float>("27"), 27.0);
            VERIFY_ARE_EQUAL(ConvertValue<double>("27"), 27.0);
        });
    }

    void BindingIntegrationTests::CanConvertFromStringToRefType()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([]()
        {
            // String -> Solid Color Brush
            VERIFY_ARE_EQUAL(ConvertRefValue<::Microsoft::UI::Xaml::Media::SolidColorBrush>("red")->Color, ::Microsoft::UI::Colors::Red);

            // String -> Uri
            ::Platform::String^ uriStringValue = "http://static-hp-wus.s-msn.com/sc/homepage/i/65/e8a77758e8644573ba5d41ada16e8c.jpg";
            VERIFY_ARE_EQUAL(ConvertRefValue<::Windows::Foundation::Uri>(uriStringValue)->AbsoluteUri, uriStringValue);

            // String -> Bitmap Image
            VERIFY_ARE_EQUAL(ConvertRefValue<::Microsoft::UI::Xaml::Media::Imaging::BitmapImage>(uriStringValue)->UriSource->AbsoluteUri, uriStringValue);
        });
    }

    void BindingIntegrationTests::CanConvertFromStringToEnumType()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([]()
        {
            // String -> Visibility
            VERIFY_ARE_EQUAL(ConvertValue<::Microsoft::UI::Xaml::Visibility>("Collapsed"), ::Microsoft::UI::Xaml::Visibility::Collapsed);
            VERIFY_ARE_EQUAL(ConvertValue<::Microsoft::UI::Xaml::Visibility>("1"), ::Microsoft::UI::Xaml::Visibility::Collapsed);
        });
    }

    void BindingIntegrationTests::CanConvertFromTypeToType()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([]()
        {
            // Uri -> Bitmap Image
            ::Windows::Foundation::Uri^ uriValue = ref new ::Windows::Foundation::Uri("http://static-hp-wus.s-msn.com/sc/homepage/i/65/e8a77758e8644573ba5d41ada16e8c.jpg");
            VERIFY_ARE_EQUAL(ConvertRefValue<::Microsoft::UI::Xaml::Media::Imaging::BitmapImage>(uriValue)->UriSource->AbsoluteUri, uriValue->AbsoluteUri);
        });
    }

    // TextBlock started sending property change notifications even when its property values didn't change
    // This caused an app break in Blue, because the app also failed to filter its property change notifications
    // and in Blue, prop changes were async, meaning the Binding expression wasn't able to catch this ping-ponging
    // via its reentrancy check.
    void BindingIntegrationTests::CanCreateTwoWayBindingToTextBlocks()
    {
        TestCleanupWrapper cleanup;
        using xaml_media::SolidColorBrush;

        SCBDataSource^ dataSource;
        TextBlock^ tb;
        TextBlock^ tb2;

        RunOnUIThread([&]
        {
            dataSource = ref new SCBDataSource();
            dataSource->MyBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
            tb = ref new TextBlock;
            tb->DataContext = dataSource;

            auto binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"MyBrush");
            binding->Mode = BindingMode::TwoWay;
            tb->SetBinding(TextBlock::ForegroundProperty, binding);

            tb2 = ref new TextBlock;
            tb2->DataContext = dataSource;

            binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"MyBrush");
            binding->Mode = BindingMode::TwoWay;
            tb2->SetBinding(TextBlock::ForegroundProperty, binding);

            dataSource->MyBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);

            TestServices::WindowHelper->WindowContent = tb;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]
        {
            auto brushOnTb = safe_cast<SolidColorBrush^>(tb->Foreground);
            auto brushInSource = safe_cast<SolidColorBrush^>(dataSource->MyBrush);
            VERIFY_IS_TRUE(
                brushOnTb->Color.A == brushInSource->Color.A &&
                brushOnTb->Color.R == brushInSource->Color.R &&
                brushOnTb->Color.G == brushInSource->Color.G &&
                brushOnTb->Color.B == brushInSource->Color.B,
                L"TextBlock.Foreground did not update");
        });
    }

    void RunBindingUpdatesOnDOVariation(BindingMode mode)
    {
        StackPanel^ rootPanel;
        RunOnUIThread([&]
        {
            Platform::String^ xamlContent =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' x:Name='root' "
                L" xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'> "
                L"  <TextBlock Grid.Row='0' Grid.RowSpan='1' Text='Hello World!!!' x:Name='myTextBlock' Height='30' Width='55'"
                L"            FontSize='12' Foreground='Red' LineHeight='25' RenderTransformOrigin='0.5,0.5' > "
                L"   <TextBlock.Resources>"
                L"     <SolidColorBrush x:Key='scb'/>"
                L"     <Storyboard x:Name='myStoryboard'>"
                L"          <DoubleAnimation Storyboard.TargetName='MyAnimatedRectangle' Storyboard.TargetProperty='Opacity'"
                L"                           From='0.1' To='0.8' Duration='0:0:5' FillBehavior='Stop'/> "
                L"     </Storyboard>"
                L"   </TextBlock.Resources>"
                L"   <TextBlock.RenderTransform>"
                L"      <TransformGroup>"
                L"            <ScaleTransform ScaleX='1' ScaleY='1'/>"
                L"            <SkewTransform AngleX='0' AngleY='0'/>"
                L"            <RotateTransform Angle='0'/>"
                L"            <TranslateTransform X='0' Y='0'/>"
                L"        </TransformGroup>"
                L"    </TextBlock.RenderTransform>"
                L"  </TextBlock>"
                L"  <TextBox Text='Starting text' x:Name='myTextBox' Width='200' Height='45' Padding='8,3,8,5'/> "
                L"  <TextBox Text='Starting text2' x:Name='myTextBox2' Width='200' Padding='8,3,8,5'/> "
                L"  <TextBox Text='Dummy focus target' Width='200' Padding='8,3,8,5'/> "
                L"  <TextBox Text='Dummy focus target2' Width='200' Padding='8,3,8,5'/> "
                L"  <TextBlock Height='{Binding Width, Mode=TwoWay, RelativeSource={RelativeSource Self}}' /> "
                L"  <TextBlock Text='Test ElementName' x:Name='enSource' /> "
                L"  <TextBlock Text='{Binding Text, ElementName=enSource}' /> "
                L"</StackPanel>";
            rootPanel = safe_cast<StackPanel^>(XamlReader::Load(xamlContent));
            VERIFY_IS_NOT_NULL(rootPanel);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        TextBlock^ textBlock1;
        TextBox^ textBox1;
        Platform::String^ newSourceValue = L"new text";
        Platform::String^ newTargetValue = L"changed text on target";
        Platform::String^ valueTargetBefore;
        Platform::String^ valueSourceBefore;
        RunOnUIThread([&]
        {
            textBlock1 = safe_cast<TextBlock^>(TreeHelper::GetVisualChildByName(rootPanel, L"myTextBlock"));
            VERIFY_IS_NOT_NULL(textBlock1);
            textBox1 = safe_cast<TextBox^>(TreeHelper::GetVisualChildByName(rootPanel, L"myTextBox"));
            VERIFY_IS_NOT_NULL(textBox1);

            auto binding = ref new Binding;
            binding->Source = textBlock1;
            binding->Mode = mode;
            binding->Path = ref new PropertyPath(L"Text");

            Log::Comment(L"1. set a binding on a target");
            valueTargetBefore = textBox1->Text;
            valueSourceBefore = textBlock1->Text;
            textBox1->SetBinding(TextBox::TextProperty, binding);

            VERIFY_ARE_EQUAL(valueSourceBefore, textBox1->Text);
            VERIFY_ARE_EQUAL(valueSourceBefore, textBlock1->Text);

            Log::Comment(L"2. change the value on the source path");
            valueTargetBefore = textBox1->Text;
            valueSourceBefore = textBlock1->Text;
            textBlock1->Text = newSourceValue;

            if (mode == BindingMode::OneTime)
            {
                VERIFY_ARE_EQUAL(valueTargetBefore, textBox1->Text);
            }
            else
            {
                VERIFY_ARE_EQUAL(newSourceValue, textBox1->Text);
            }
            VERIFY_ARE_EQUAL(newSourceValue, textBlock1->Text);

            Log::Comment(L"3. change the value on the target path");
            valueTargetBefore = textBox1->Text;
            valueSourceBefore = textBlock1->Text;
            textBox1->Text = newTargetValue;

            Log::Comment(L"Changing the focus to the control programmatically so the source updates");
            VERIFY_IS_TRUE(textBox1->Focus(FocusState::Programmatic));
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]
        {
            VERIFY_ARE_EQUAL(newTargetValue, textBox1->Text);
            if (mode == BindingMode::TwoWay)
            {
                VERIFY_ARE_EQUAL(newTargetValue, textBlock1->Text);
            }
            else
            {
                VERIFY_ARE_EQUAL(valueSourceBefore, textBlock1->Text);
            }

            Log::Comment(L"4. change the value on the source path again");
            Platform::String^ newSourceValue2 = L"new text again";
            valueTargetBefore = textBox1->Text;
            valueSourceBefore = textBlock1->Text;
            textBlock1->Text = newSourceValue2;

            if (mode == BindingMode::TwoWay)
            {
                VERIFY_ARE_EQUAL(newSourceValue2, textBox1->Text);
            }
            else
            {
                VERIFY_ARE_EQUAL(valueTargetBefore, textBox1->Text);
            }
            VERIFY_ARE_EQUAL(newSourceValue2, textBlock1->Text);
            TestServices::WindowHelper->WindowContent = nullptr;
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void BindingIntegrationTests::TestBindingUpdatesOnDO()
    {
        TestCleanupWrapper cleanup;
        RunBindingUpdatesOnDOVariation(BindingMode::OneWay);
        RunBindingUpdatesOnDOVariation(BindingMode::OneTime);
        RunBindingUpdatesOnDOVariation(BindingMode::TwoWay);
    }

    void BindingIntegrationTests::CanCombineDataContextChangedEventWithDataContextBindingOnSameElement()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            auto container = ref new StackPanel();

            auto myBinding = ref new Binding();
            myBinding->Path = ref new PropertyPath(L"Tag");
            myBinding->Source = container;
            container->SetBinding(FrameworkElement::DataContextProperty, myBinding);

            String^ receivedValue = nullptr;

            auto dataContextChangedRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::Controls::StackPanel, DataContextChanged);
            dataContextChangedRegistration.Attach(
                container,
                ref new TypedEventHandler<FrameworkElement^, DataContextChangedEventArgs^>([&receivedValue](FrameworkElement^, DataContextChangedEventArgs^ args)
            {
                receivedValue = safe_cast<String^>(args->NewValue);
            }));

            container->Tag = "hello";

            VERIFY_ARE_STRINGS_EQUAL(L"hello", receivedValue->Data());
        });
    }


    ref class StringDataSource sealed : public Microsoft::UI::Xaml::Tests::Common::CustomPropertyProviderBase
    {
        Platform::String^ value_;
    protected:
        void AddCustomProperties() override
        {
            AddCustomProperty(L"Value", Platform::String::typeid,
                MAKEPROPGET(StringDataSource^, Value),
                MAKEPROPSET(StringDataSource^, Value, Platform::String^)
                );
        }
    public:
        property Platform::String^ Value
        {
            Platform::String^ get() { return value_; }
            void set(Platform::String^ value)
            {
                if (value != value_) {
                    value_ = value;
                    FirePropertyChanged(L"Value");
                }
            }
        }
    };

    void BindingIntegrationTests::VerifySyncUpdatesFromBinding()
    {
        TestCleanupWrapper cleanup;
        TextBlock^ tb1 = nullptr;
        TextBlock^ tb2 = nullptr;
        StringDataSource^ source = ref new StringDataSource;

        RunOnUIThread([&]
        {
            tb1 = ref new TextBlock;
            tb2 = ref new TextBlock;
            source->Value = L"original";

            // Create an asymmetric ValueConverter so we can see if the TwoWay binding mistakenly bounces back to the source.
            auto converter = ref new CustomConverter;
            converter->m_convertFunction = [](Object^ value, wxaml_interop::TypeName targetType, Object^ parameter, String^ language) -> Platform::String^
            {
                auto valueString = safe_cast<Platform::String^>(value);
                if (valueString == L"original")
                    return L"converted";
                else
                    return L"changed";

            };
            converter->m_convertBackFunction = [](Object^ value, wxaml_interop::TypeName targetType, Object^ parameter, String^ language) -> Platform::String^
            {
                return L"ConvertedBack";
            };

            auto bindingMaker = [&]
            {
                auto binding = ref new Binding;
                binding->Path = ref new PropertyPath(L"Value");
                binding->Mode = BindingMode::TwoWay;
                binding->Converter = converter;
                binding->Source = source;
                return binding;
            };

            tb1->SetBinding(TextBlock::TextProperty, bindingMaker());
            tb2->SetBinding(TextBlock::TextProperty, bindingMaker());

            auto rootPanel = ref new StackPanel;
            rootPanel->Children->Append(tb1);
            rootPanel->Children->Append(tb2);

            // These updates should propagate immediately
            VERIFY_IS_TRUE(L"original" == source->Value);
            VERIFY_IS_TRUE(L"converted" == tb1->Text);
            VERIFY_IS_TRUE(L"converted" == tb2->Text);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]
        {
            // These values shouldn't have changed due to some async update, either
            VERIFY_IS_TRUE(L"original" == source->Value);
            VERIFY_IS_TRUE(L"converted" == tb1->Text);
            VERIFY_IS_TRUE(L"converted" == tb2->Text);

            TestServices::WindowHelper->WindowContent = nullptr;
        });
    }

    void BindingIntegrationTests::BindingWithMatchingSubstringPropertyName()
    {
        TestCleanupWrapper cleanup;

        using xaml_media::SolidColorBrush;
        SCBDataSource^ dataSource;
        TextBlock^ tb;

        RunOnUIThread([&]
        {
            dataSource = ref new SCBDataSource();
            dataSource->MyBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
            tb = ref new TextBlock;
            tb->DataContext = dataSource;

            auto binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"MyBrush2");
            binding->Mode = BindingMode::OneWay;
            tb->SetBinding(TextBlock::ForegroundProperty, binding);

            // Set source MyBrush2 property.
            dataSource->MyBrush2 = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);

            TestServices::WindowHelper->WindowContent = tb;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]
        {
            // The target brush should be blue now, same as the source.
            auto targetBrush = safe_cast<SolidColorBrush^>(tb->Foreground);
            VERIFY_ARE_EQUAL(targetBrush->Color, Microsoft::UI::Colors::Blue);

            // Quietly set the source brush -- don't through the setter
            // so it won't signal the change.
            dataSource->myBrush2_ = ref new SolidColorBrush(Microsoft::UI::Colors::Red);

            // Set MyBrush, which has a name that's a substring of MyBrush2.
            // This shouldn't cause a re-read of the MyBrush2 property,
            // so the target shouldn't change.
            dataSource->MyBrush = dataSource->myBrush2_;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]
        {
            // Verify target brush hasn't changed.
            auto targetBrush = safe_cast<SolidColorBrush^>(tb->Foreground);
            VERIFY_ARE_EQUAL(targetBrush->Color, Microsoft::UI::Colors::Blue);
        });
    }


    ref class IndexedPropertyProvider sealed
        : public Microsoft::UI::Xaml::Tests::Common::CustomPropertyProviderBase
    {
        friend void BindingIntegrationTests::BindingCustomPropertyProviderOneWayIndexLookup();
        Platform::Collections::Map<int, Object^>^ entries_;

    public:
        IndexedPropertyProvider()
        {
            entries_ = ref new Platform::Collections::Map<int, Object^ >;
        }

        property ::Windows::Foundation::Collections::IMap<int, Object^>^ Entries
        {
            ::Windows::Foundation::Collections::IMap<int, Object^>^ get()
            {
                return entries_;
            }
        }

        void OnEntriesChanged(Platform::String^ name)
        {
            FirePropertyChanged(name);
        }

    protected:
        void AddCustomProperties() override
        {
            ::Windows::UI::Xaml::Interop::TypeName typeName;
            typeName.Kind = ::Windows::UI::Xaml::Interop::TypeKind::Primitive;
            typeName.Name = L"Object";

            AddCustomIndexedProperty(L"Item", typeName,
                MAKEPROPGETIDX(IndexedPropertyProvider^, entries_, int),
                MAKEPROPSETIDX(IndexedPropertyProvider^, entries_, Object^, int));
        }
    };

    void BindingIntegrationTests::BindingCustomPropertyProviderOneWayIndexLookup()
    {
        RunOnUIThread([&] {
            SCBDataSource^ brushDataSource = ref new SCBDataSource();
            brushDataSource->MyBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Red);

            auto dataSource = ref new IndexedPropertyProvider;
            dataSource->Entries->Insert(0, brushDataSource);

            auto container1 = ref new StackPanel();
            auto container2 = ref new StackPanel();

            container1->Tag = dataSource;
            auto myBinding = ref new Binding();
            myBinding->Path = ref new PropertyPath(L"Tag[0].MyBrush");
            myBinding->Source = container1;
            container2->SetBinding(FrameworkElement::TagProperty, myBinding);
            VERIFY_ARE_EQUAL(brushDataSource->MyBrush->GetHashCode(), container2->Tag->GetHashCode());
        });
    }

    void BindingIntegrationTests::BindingIVectorOneWayIndexLookup()
    {
        RunOnUIThread([&] {
            SCBDataSource^ brushDataSource = ref new SCBDataSource();
            brushDataSource->MyBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Red);

            auto dataSource = ref new Platform::Collections::Vector<SCBDataSource^>();
            dataSource->Append(brushDataSource);
            auto container1 = ref new StackPanel();
            auto container2 = ref new StackPanel();

            container1->Tag = dataSource;
            auto myBinding = ref new Binding();
            myBinding->Path = ref new PropertyPath(L"Tag[0].MyBrush");
            myBinding->Source = container1;
            container2->SetBinding(FrameworkElement::TagProperty, myBinding);
            VERIFY_ARE_EQUAL(brushDataSource->MyBrush->GetHashCode(), container2->Tag->GetHashCode());
        });
    }

    void BindingIntegrationTests::BindingIVectorIndexLookupTwoWayConverter()
    {
        RunOnUIThread([&] {

            auto aObj1 = ref new ArbitraryObject;
            auto aObj2 = ref new ArbitraryObject;

            auto dataSource = ref new Platform::Collections::Vector<ArbitraryObject^>();
            dataSource->Append(aObj1);
            auto container1 = ref new StackPanel();
            auto container2 = ref new StackPanel();

            container1->Tag = dataSource;
            auto myBinding = ref new Binding();
            myBinding->Path = ref new PropertyPath(L"Tag[0]");
            myBinding->Source = container1;
            myBinding->Mode = BindingMode::TwoWay;

            auto converter = ref new CustomConverter;
            myBinding->Converter = converter;

            converter->m_convertFunction = [&](Object^ value, wxaml_interop::TypeName targetType, Object^ parameter, String^ language) -> Object^
            {
                VERIFY_ARE_EQUAL(targetType.Kind, wxaml_interop::TypeKind::Primitive);
                VERIFY_ARE_STRINGS_EQUAL(targetType.Name->Data(), L"Object");
                VERIFY_ARE_EQUAL(value->GetHashCode(), aObj1->GetHashCode());
                return value;
            };

            converter->m_convertBackFunction = [&](Object^ value, wxaml_interop::TypeName targetType, Object^ parameter, String^ language) -> Object^
            {
                VERIFY_ARE_EQUAL(targetType.Kind, wxaml_interop::TypeKind::Primitive);
                VERIFY_ARE_STRINGS_EQUAL(targetType.Name->Data(), L"Object");
                VERIFY_ARE_EQUAL(value->GetHashCode(), aObj2->GetHashCode());
                return value;
            };

            container2->SetBinding(FrameworkElement::TagProperty, myBinding);
            VERIFY_ARE_EQUAL(aObj1->GetHashCode(), container2->Tag->GetHashCode());
            container2->Tag = aObj2;
            VERIFY_ARE_EQUAL(dataSource->GetAt(0)->GetHashCode(), aObj2->GetHashCode());
        });
    }

    void BindingIntegrationTests::BindingIMapKeyLookupTwoWayConverter()
    {
        RunOnUIThread([&] {

            auto aObj1 = ref new ArbitraryObject;
            auto aObj2 = ref new ArbitraryObject;

            auto dataSource = ref new Platform::Collections::Map<String^, Object^>();
            dataSource->Insert(L"MyObj", aObj1);
            auto container1 = ref new StackPanel();
            auto container2 = ref new StackPanel();

            container1->Tag = dataSource;
            auto myBinding = ref new Binding();
            myBinding->Path = ref new PropertyPath(L"Tag[MyObj]");
            myBinding->Source = container1;
            myBinding->Mode = BindingMode::TwoWay;

            auto converter = ref new CustomConverter;
            myBinding->Converter = converter;

            converter->m_convertFunction = [&](Object^ value, wxaml_interop::TypeName targetType, Object^ parameter, String^ language) -> Object^
            {
                VERIFY_ARE_EQUAL(targetType.Kind, wxaml_interop::TypeKind::Primitive);
                VERIFY_ARE_STRINGS_EQUAL(targetType.Name->Data(), L"Object");
                VERIFY_ARE_EQUAL(value->GetHashCode(), aObj1->GetHashCode());
                return value;
            };

            converter->m_convertBackFunction = [&](Object^ value, wxaml_interop::TypeName targetType, Object^ parameter, String^ language) -> Object^
            {
                VERIFY_ARE_EQUAL(targetType.Kind, wxaml_interop::TypeKind::Primitive);
                VERIFY_ARE_STRINGS_EQUAL(targetType.Name->Data(), L"Object");
                VERIFY_ARE_EQUAL(value->GetHashCode(), aObj2->GetHashCode());
                return value;
            };

            container2->SetBinding(FrameworkElement::TagProperty, myBinding);
            VERIFY_ARE_EQUAL(aObj1->GetHashCode(), container2->Tag->GetHashCode());
            container2->Tag = aObj2;
            VERIFY_ARE_EQUAL(aObj2->GetHashCode(), dataSource->Lookup(L"MyObj")->GetHashCode());
        });
    }

    void BindingIntegrationTests::BindingCustomPropertyProviderIntLookupTwoWayConverter()
    {
        RunOnUIThread([&] {

            auto aObj1 = ref new ArbitraryObject;
            auto aObj2 = ref new ArbitraryObject;

            auto dataSource = ref new IndexedPropertyProvider;
            dataSource->Entries->Insert(0, aObj1);
            auto container1 = ref new StackPanel();
            auto container2 = ref new StackPanel();

            container1->Tag = dataSource;
            auto myBinding = ref new Binding();
            myBinding->Path = ref new PropertyPath(L"Tag[0]");
            myBinding->Source = container1;
            myBinding->Mode = BindingMode::TwoWay;

            auto converter = ref new CustomConverter;
            myBinding->Converter = converter;

            converter->m_convertFunction = [&](Object^ value, wxaml_interop::TypeName targetType, Object^ parameter, String^ language) -> Object^
            {
                VERIFY_ARE_EQUAL(targetType.Kind, wxaml_interop::TypeKind::Primitive);
                VERIFY_ARE_STRINGS_EQUAL(targetType.Name->Data(), L"Object");
                VERIFY_ARE_EQUAL(value->GetHashCode(), aObj1->GetHashCode());
                return value;
            };

            converter->m_convertBackFunction = [&](Object^ value, wxaml_interop::TypeName targetType, Object^ parameter, String^ language) -> Object^
            {
                VERIFY_ARE_EQUAL(targetType.Kind, wxaml_interop::TypeKind::Primitive);
                VERIFY_ARE_STRINGS_EQUAL(targetType.Name->Data(), L"Object");
                VERIFY_ARE_EQUAL(value->GetHashCode(), aObj2->GetHashCode());
                return value;
            };

            container2->SetBinding(FrameworkElement::TagProperty, myBinding);
            VERIFY_ARE_EQUAL(aObj1->GetHashCode(), container2->Tag->GetHashCode());
            container2->Tag = aObj2;
            VERIFY_ARE_EQUAL(dataSource->Entries->Lookup(0)->GetHashCode(), aObj2->GetHashCode());
        });
    }

    ref class GuidDataSource sealed : public Microsoft::UI::Xaml::Tests::Common::CustomPropertyProviderBase
    {
        Platform::Guid value_;
    protected:
        void AddCustomProperties() override
        {
            AddCustomProperty(L"Value", Platform::Guid::typeid,
                MAKEPROPGET(GuidDataSource^, Value),
                MAKEPROPSET(GuidDataSource^, Value, Platform::Guid)
                );
        }
    public:
        property Platform::Guid Value
        {
            Platform::Guid get() { return value_; }
            void set(Platform::Guid value)
            {
                if (value != value_)
                {
                    value_ = value;
                    FirePropertyChanged(L"Value");
                }
            }
        }
    };

    ref class UriDataSource sealed : public Microsoft::UI::Xaml::Tests::Common::CustomPropertyProviderBase
    {
        wf::Uri^ value_;
    protected:
        void AddCustomProperties() override
        {
            AddCustomProperty(L"Value", wf::Uri::typeid,
                MAKEPROPGET(UriDataSource^, Value),
                MAKEPROPSET(UriDataSource^, Value, wf::Uri^)
                );
        }
    public:
        property wf::Uri^ Value
        {
            wf::Uri^ get() { return value_; }
            void set(wf::Uri^ value)
            {
                if (value != value_)
                {
                    value_ = value;
                    FirePropertyChanged(L"Value");
                }
            }
        }
    };

    void BindingIntegrationTests::DataSourceConversions()
    {
        TestCleanupWrapper cleanup;
        TextBlock^ textBlock = nullptr;
        CustomControl^ customControl = nullptr;

        GuidDataSource^ guidDataSource = ref new GuidDataSource;
        UriDataSource^ uriDataSource = ref new UriDataSource;
        StringDataSource^ stringDataSource = ref new StringDataSource;

        // CFA2C203-A31E-4465-8CFC-B80C5AC13938
        Platform::Guid guid1(0xCFA2C203, 0xA31E, 0x4465, 0x8C, 0xFC, 0xB8, 0x0C, 0x5A, 0xC1, 0x39, 0x38);
        // 60C8E61C-110D-445C-BF56-A54520F9B82A
        Platform::Guid guid2(0x60C8E61C, 0x110D, 0x445C, 0xBF, 0x56, 0xA5, 0x45, 0x20, 0xF9, 0xB8, 0x2A);

        wf::Uri^ uri1 = ref new Uri(L"http://foo1/");

        Binding^ guidBinding = nullptr;
        Binding^ uriBinding = nullptr;

        RunOnUIThread([&]
        {
            textBlock = ref new TextBlock;
            customControl = ref new CustomControl;
            StackPanel^ stackPanel = ref new StackPanel;
            stackPanel->Children->Append(textBlock);
            stackPanel->Children->Append(customControl);
            TestServices::WindowHelper->WindowContent = stackPanel;

            LOG_OUTPUT(L"GUID->String: Convert Source to Target");
            guidDataSource->Value = guid1;
            guidBinding = ref new Binding;
            guidBinding->Path = ref new PropertyPath(L"Value");
            guidBinding->Mode = BindingMode::TwoWay;
            guidBinding->Source = guidDataSource;
            textBlock->SetBinding(TextBlock::TextProperty, guidBinding);

            VERIFY_ARE_EQUAL(guidDataSource->Value, guid1);
            VERIFY_IS_TRUE(L"{CFA2C203-A31E-4465-8CFC-B80C5AC13938}" == textBlock->Text);

            LOG_OUTPUT(L"GUID->String: ConvertBack from Target to Source");
            textBlock->Text = L"{60C8E61C-110D-445C-BF56-A54520F9B82A}";
            VERIFY_ARE_EQUAL(guidDataSource->Value, guid2);

            LOG_OUTPUT(L"String->GUID: Convert Source to Target");
            stringDataSource->Value = L"{CFA2C203-A31E-4465-8CFC-B80C5AC13938}";
            guidBinding = ref new Binding;
            guidBinding->Path = ref new PropertyPath(L"Value");
            guidBinding->Mode = BindingMode::TwoWay;
            guidBinding->Source = stringDataSource;
            customControl->SetBinding(CustomControl::MyGuidProperty, guidBinding);

            VERIFY_IS_TRUE(stringDataSource->Value == L"{CFA2C203-A31E-4465-8CFC-B80C5AC13938}");
            VERIFY_ARE_EQUAL(customControl->MyGuid, guid1);

            LOG_OUTPUT(L"String->GUID: ConvertBack from Target to Source");
            customControl->MyGuid = guid2;
            VERIFY_IS_TRUE(stringDataSource->Value == L"{60C8E61C-110D-445C-BF56-A54520F9B82A}");

            LOG_OUTPUT(L"Uri->String: Convert Source to Target");
            uriDataSource->Value = uri1;
            uriBinding = ref new Binding;
            uriBinding->Path = ref new PropertyPath(L"Value");
            uriBinding->Mode = BindingMode::TwoWay;
            uriBinding->Source = uriDataSource;
            textBlock->SetBinding(TextBlock::TextProperty, uriBinding);

            VERIFY_ARE_EQUAL(uriDataSource->Value, uri1);
            VERIFY_IS_TRUE(L"http://foo1/" == textBlock->Text);

            // Uri->String ConvertBack from Target to Source is not supported (Note that
            // UriToStringValueConverter::ConvertBack returns E_NOTIMPL
            // and MetadataAPI::GetPrimitiveClassInfo doesn't support Windows.Foundation.Uri.)

            LOG_OUTPUT(L"String->Uri: Convert from Source to Target");
            stringDataSource->Value = L"http://foo1/";
            uriBinding = ref new Binding;
            uriBinding->Path = ref new PropertyPath(L"Value");
            uriBinding->Mode = BindingMode::TwoWay;
            uriBinding->Source = stringDataSource;
            customControl->SetBinding(CustomControl::MyUriProperty, uriBinding);

            VERIFY_IS_TRUE(stringDataSource->Value == L"http://foo1/");
            VERIFY_IS_TRUE(customControl->MyUri->ToString() == L"http://foo1/");

            // String->Uri ConvertBack from Target to Source is not supported (Note that
            // StringToUriValueConverter::ConvertBack returns E_NOTIMPL)
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void BindingIntegrationTests::DataContextChangedPreservesObjectIdentity()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            Object^ result = nullptr;
            auto obj = ref new StackPanel;

            auto dataContextChangedRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::Controls::StackPanel, DataContextChanged);
            dataContextChangedRegistration.Attach(
                obj,
                ref new TypedEventHandler<FrameworkElement^, DataContextChangedEventArgs^>([&result](FrameworkElement^, DataContextChangedEventArgs^ args)
            {
                result = args->NewValue;
            }));

            Object^ newDataContext = L"Test";
            obj->DataContext = newDataContext;

            VERIFY_ARE_EQUAL(newDataContext, result);
        });
   }

    void BindingIntegrationTests::CanBindToComposedDOWithoutTypeInfo()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            auto source = ref new CanvasWithoutTypeInfo();
            auto target = ref new StackPanel();

            auto binding = ref new Binding();
            binding->Path = ref new PropertyPath(L"Visibility");
            binding->Source = source;
            target->SetBinding(FrameworkElement::VisibilityProperty, binding);

            VERIFY_ARE_EQUAL(Visibility::Visible, target->Visibility);

            source->Visibility = Visibility::Collapsed;
            VERIFY_ARE_EQUAL(Visibility::Collapsed, target->Visibility);
        });
    }

    void BindingIntegrationTests::CanBindIncompatibleObjectToUnknownTypeProperty()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            auto target = ref new CustomControl();
            auto binding = ref new Binding();
            binding->Source = ref new CustomControl();
            target->SetBinding(CustomControl::VectorProperty, binding);
            VERIFY_ARE_EQUAL(binding->Source, target->GetValue(CustomControl::VectorProperty));
        });
    }

    void BindingIntegrationTests::VerifyUpdateSourceTrigger_LostFocus()
    {
        TestCleanupWrapper cleanup;

        StackPanel^ stackPanel;
        ComboBox^ comboBox;
        TextBlock^ textBlock;
        Button^ button;

        RunOnUIThread([&stackPanel, &comboBox, &textBlock, &button]()
        {
            stackPanel = safe_cast<StackPanel^>(XamlReader::Load(
                L"<StackPanel"
                L"  xmlns='http://schemas.microsoft.com/client/2007' "
                L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <TextBlock x:Name='m_textBlock' />"
                L"  <ComboBox x:Name='m_comboBox' SelectedItem='{Binding ElementName=m_textBlock,Path=Text,Mode=TwoWay,UpdateSourceTrigger=LostFocus}' />"
                L"  <Button x:Name='m_button' Content='Focus dummy' />"
                L"</StackPanel>"
                ));
            comboBox = safe_cast<ComboBox^>(stackPanel->FindName(L"m_comboBox"));
            textBlock = safe_cast<TextBlock^>(stackPanel->FindName(L"m_textBlock"));
            button = safe_cast<Button^>(stackPanel->FindName(L"m_button"));

            TestServices::WindowHelper->WindowContent = stackPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&comboBox]()
        {
            // Populate the ComboBox's items
            comboBox->Items->Append(ref new Platform::String(L"So then it's up with the Blue and Gold"));
            comboBox->Items->Append(ref new Platform::String(L"Down with the Red; red, red, red"));
            comboBox->Items->Append(ref new Platform::String(L"California's out for a victory"));
            comboBox->Items->Append(ref new Platform::String(L"We'll drop our battle axe on Stanford's head; chop"));
            comboBox->Items->Append(ref new Platform::String(L"When we meet her, our team will surely beat her"));
            comboBox->Items->Append(ref new Platform::String(L"Down on the Stanford farm, there'll be no sound"));
            comboBox->Items->Append(ref new Platform::String(L"When our Oski rips through the air"));
            comboBox->Items->Append(ref new Platform::String(L"Like our friend Mister Jonah, Stanford's team will be found"));
            comboBox->Items->Append(ref new Platform::String(L"In the tummy of the Golden Bear"));

            // Give focus to the ComboBox
            VERIFY_IS_TRUE(comboBox->Focus(FocusState::Programmatic));
        });
        TestServices::WindowHelper->WaitForIdle();

        // Set up a PropertyChangedCallback on TextBlock.Text so we can keep track of how many times the two-way binding updates
        int propertyChangedCount = 0;
        long long propertyChangedCallbackToken = 0;
        RunOnUIThread([&textBlock, &propertyChangedCount, &propertyChangedCallbackToken]()
        {
            propertyChangedCallbackToken = textBlock->RegisterPropertyChangedCallback(
                TextBlock::TextProperty,
                ref new DependencyPropertyChangedCallback([&propertyChangedCount](DependencyObject^ sender, DependencyProperty^ prop)
                {
                    propertyChangedCount++;
                }));
        });
        auto scopeExit = wil::scope_exit([&propertyChangedCallbackToken, &textBlock]()
        {
            if (propertyChangedCallbackToken != 0)
            {
                textBlock->UnregisterPropertyChangedCallback(TextBlock::TextProperty, propertyChangedCallbackToken);
            }
        });

        // Have the ComboBox lose focus twice just to make sure the Binding updating the first time isn't a fluke
        for (int iteration = 1; iteration < 3; iteration++)
        {
            RunOnUIThread([&comboBox, &button, &propertyChangedCount]()
            {
                // Set the ComboBox's SelectedItem to each of the Items in turn
                // Each 'iteration' will result in a different stopping point so that way we actually end up
                // with different values on the Binding's Source property
                for (unsigned int i = 0; i < comboBox->Items->Size - propertyChangedCount; i++)
                {
                    comboBox->SelectedIndex = i;
                }

                // Give focus to the Button; the Binding should now fire an update to its Source (the TextBlock's Text)
                VERIFY_IS_TRUE(button->Focus(FocusState::Programmatic));
            });
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_ARE_EQUAL(propertyChangedCount, iteration, L"Binding should have updated only once per loss of focus.");

            RunOnUIThread([&comboBox]()
            {
                // Give focus to the ComboBox
                VERIFY_IS_TRUE(comboBox->Focus(FocusState::Programmatic));
            });
            TestServices::WindowHelper->WaitForIdle();
        }
    }

    void BindingIntegrationTests::VerifyUpdateSourceTrigger_LostFocus_RequiresUIElement()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([]()
        {
            VERIFY_THROWS_WINRT(
                XamlReader::Load(
                    L"<StackPanel"
                    L"  xmlns='http://schemas.microsoft.com/client/2007' "
                    L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Border>"
                    L"    <Border.BorderBrush>"
                    L"      <SolidColorBrush x:Name='brush0' Color='Blue' />"
                    L"    </Border.BorderBrush>"
                    L"  </Border>"
                    L"  <Border>"
                    L"    <Border.BorderBrush>"
                    L"      <SolidColorBrush x:Name='brush1' Color='{Binding ElementName=brush0,Path=Color,Mode=TwoWay,UpdateSourceTrigger=LostFocus}' />"
                    L"    </Border.BorderBrush>"
                    L"  </Border>"
                    L"</StackPanel>"
                ),
                Platform::Exception^,
                L"Exception thrown by XamlReader.Load method should be propagated to app");
        });
    }

    ref class IndexedPropertyProviderDataSource sealed
        : public Microsoft::UI::Xaml::Tests::Common::CustomPropertyProviderBase
    {
        IndexedPropertyProvider^ _value;

    protected:
        void AddCustomProperties() override
        {
            AddCustomProperty(L"Value", IndexedPropertyProvider::typeid,
                MAKEPROPGET(IndexedPropertyProviderDataSource^, Value),
                MAKEPROPSET(IndexedPropertyProviderDataSource^, Value, IndexedPropertyProvider^)
                );
        }

    public:
        property IndexedPropertyProvider^ Value
        {
            IndexedPropertyProvider^ get() { return _value; }
            void set(IndexedPropertyProvider^ value) { _value = value;}
        }
    };

    ref class StringIndexedPropertyProvider sealed
        : public Microsoft::UI::Xaml::Tests::Common::CustomPropertyProviderBase
    {
        friend void BindingIntegrationTests::BindingCustomPropertyProviderOneWayIndexLookup();
        Platform::Collections::Map<String^, Object^>^ _entries;

    public:
        StringIndexedPropertyProvider()
        {
            _entries = ref new Platform::Collections::Map<String^, Object^ >;
        }

        property ::Windows::Foundation::Collections::IMap<String^, Object^>^ Entries
        {
            ::Windows::Foundation::Collections::IMap<String^, Object^>^ get()
            {
                return _entries;
            }
        }

        void OnEntriesChanged(Platform::String^ name)
        {
            FirePropertyChanged(name);
        }

    protected:
        void AddCustomProperties() override
        {
            ::Windows::UI::Xaml::Interop::TypeName typeName;
            typeName.Kind = ::Windows::UI::Xaml::Interop::TypeKind::Primitive;
            typeName.Name = L"Object";

            AddCustomIndexedProperty(L"Item", typeName,
                MAKEPROPGETIDX(StringIndexedPropertyProvider^, _entries, String^),
                MAKEPROPSETIDX(StringIndexedPropertyProvider^, _entries, Object^, String^));
        }
    };

    ref class StringIndexedPropertyProviderDataSource sealed
        : public Microsoft::UI::Xaml::Tests::Common::CustomPropertyProviderBase
    {
        StringIndexedPropertyProvider^ _value;

    protected:
        void AddCustomProperties() override
        {
            AddCustomProperty(L"Value", StringIndexedPropertyProvider::typeid,
                MAKEPROPGET(StringIndexedPropertyProviderDataSource^, Value),
                MAKEPROPSET(StringIndexedPropertyProviderDataSource^, Value, StringIndexedPropertyProvider^)
                );
        }

    public:
        property StringIndexedPropertyProvider^ Value
        {
            StringIndexedPropertyProvider^ get() { return _value; }
            void set(StringIndexedPropertyProvider^ value) { _value = value;}
        }
    };

    void BindingIntegrationTests::PropertyPathSteps()
    {
        RunOnUIThread([&]
        {
            String^ s= "s";
            String^ t = "t";
            String^ s2 = "s2";

            // IndexerPropertyAccess property path step tests. IndexerPropertyAccess supports a custom indexed property provider
            LOG_OUTPUT(L"Bind to Custom Indexed Property Provider with integer index");

            IndexedPropertyProvider^ indexedDataSource = ref new IndexedPropertyProvider;
            indexedDataSource->Entries->Insert(0, s);
            IndexedPropertyProviderDataSource^ indexedPropertyProviderDataSource = ref new IndexedPropertyProviderDataSource;
            indexedPropertyProviderDataSource->Value = indexedDataSource;

            IndexedPropertyProvider^ indexedDataSource2 = ref new IndexedPropertyProvider;
            indexedDataSource2->Entries->Insert(0, s2);
            IndexedPropertyProviderDataSource^ indexedPropertyProviderDataSource2 = ref new IndexedPropertyProviderDataSource;
            indexedPropertyProviderDataSource2->Value = indexedDataSource2;

            TextBlock^ textBlock = ref new TextBlock();
            textBlock->DataContext = indexedPropertyProviderDataSource;

            // Bind to indexedPropertyProviderDataSource
            Binding^ binding = ref new Binding();
            binding->Path = ref new PropertyPath(L"Value[0]");
            textBlock->SetBinding(TextBlock::TextProperty, binding);
            VERIFY_ARE_EQUAL(textBlock->Text, s);

            // Change item at 0-index
            indexedDataSource->Entries->Insert(0, t);
            // Custom Indexed Property Providers with an integer index notify changes using "Item[n]"
            indexedDataSource->OnEntriesChanged(L"Item[0]");
            VERIFY_ARE_EQUAL(textBlock->Text, t);

            // Change data-context
            textBlock->DataContext = indexedPropertyProviderDataSource2;
            VERIFY_ARE_EQUAL(textBlock->Text, s2);

            textBlock->ClearValue(TextBlock::DataContextProperty);

            // IntIndexerPathStep property path step tests. IntIndexerPathStep supports an int-indexed data source
            LOG_OUTPUT(L"Bind to integer indexed data source");

            Platform::Collections::Vector<String^>^ stringVector = ref new Platform::Collections::Vector<String^>;
            stringVector->InsertAt(0, s);
            textBlock->Tag = stringVector;
            binding = ref new Binding();
            binding->Path = ref new PropertyPath(L"Tag[0]");
            RelativeSource^ relativeSource = ref new RelativeSource;
            relativeSource->Mode = RelativeSourceMode::Self;
            binding->RelativeSource = relativeSource;
            textBlock->SetBinding(TextBlock::TextProperty, binding);
            VERIFY_ARE_EQUAL(textBlock->Text, s);

            // Change item at 0-index
            stringVector->SetAt(0, s2);
            VERIFY_ARE_EQUAL(textBlock->Text, s2);

            // Bind to CollectionViewSource
            stringVector = ref new Platform::Collections::Vector<String^>;
            stringVector->InsertAt(0, s);
            CollectionViewSource^ collectionViewSource = ref new CollectionViewSource;
            collectionViewSource->Source = stringVector;
            textBlock->Tag = collectionViewSource->View;
            binding = ref new Binding();
            binding->Path = ref new PropertyPath(L"Tag[0]");
            relativeSource = ref new RelativeSource;
            relativeSource->Mode = RelativeSourceMode::Self;
            binding->RelativeSource = relativeSource;
            textBlock->SetBinding(TextBlock::TextProperty, binding);
            VERIFY_ARE_EQUAL(textBlock->Text, s);

            // Change item at 0-index
            stringVector->SetAt(0, s2);
            VERIFY_ARE_EQUAL(textBlock->Text, s2);

            // StringIndexerPathStep property path step tests. StringIndexerPathStep supports a string-indexed data source
            LOG_OUTPUT(L"Bind to string indexed data source");

            Platform::Collections::Map<String^, Object^>^ stringMap = ref new Platform::Collections::Map<String^, Object^>;
            stringMap->Insert(L"first", s);
            textBlock->Tag = stringMap;
            binding = ref new Binding();
            binding->Path = ref new PropertyPath(L"Tag[first]");
            relativeSource = ref new RelativeSource;
            relativeSource->Mode = RelativeSourceMode::Self;
            binding->RelativeSource = relativeSource;
            textBlock->SetBinding(TextBlock::TextProperty, binding);
            VERIFY_ARE_EQUAL(textBlock->Text, s);

            // Change item at 0-index
            stringMap->Insert(L"first", s2);
            VERIFY_ARE_EQUAL(textBlock->Text, s2);

            // Bind to Custom Indexed Property Provider with string index
            LOG_OUTPUT(L"Bind to Custom Indexed Property Provider with string index");

            StringIndexedPropertyProvider^ stringIndexedDataSource = ref new StringIndexedPropertyProvider;
            stringIndexedDataSource->Entries->Insert(L"first", s);
            StringIndexedPropertyProviderDataSource^ stringIndexedPropertyProviderDataSource = ref new StringIndexedPropertyProviderDataSource;
            stringIndexedPropertyProviderDataSource->Value = stringIndexedDataSource;

            textBlock->DataContext = stringIndexedPropertyProviderDataSource;
            binding = ref new Binding();
            binding->Path = ref new PropertyPath(L"Value[first]");
            textBlock->SetBinding(TextBlock::TextProperty, binding);
            VERIFY_ARE_EQUAL(textBlock->Text, s);

            // Change item at 'first' index
            stringIndexedDataSource->Entries->Insert(L"first", t);
            stringIndexedDataSource->OnEntriesChanged(L"Item[first]");
            VERIFY_ARE_EQUAL(textBlock->Text, t);

            textBlock->ClearValue(TextBlock::DataContextProperty);
        });
    }

    ref class CustomCollectionView sealed :
        public xaml::DependencyObject,
        public xaml_data::ICollectionView
    {
    public:
        CustomCollectionView(wxaml_interop::IBindableVector^ source):
            m_currentPosition(-1)
        {
            m_source = source;
        }

        // ICollectionView
        virtual wf::IAsyncOperation<xaml_data::LoadMoreItemsResult>^ LoadMoreItemsAsync(unsigned int)
        {
            throw ref new Platform::NotImplementedException();
        }
        virtual bool MoveCurrentToFirst()
        {
            throw ref new Platform::NotImplementedException();
        }
        virtual bool MoveCurrentToLast()
        {
            throw ref new Platform::NotImplementedException();
        }
        virtual bool MoveCurrentToNext()
        {
            throw ref new Platform::NotImplementedException();
        }
        virtual bool MoveCurrentToPrevious()
        {
            throw ref new Platform::NotImplementedException();
        }
        property wfc::IObservableVector<Platform::Object^>^ CollectionGroups
        {
            virtual wfc::IObservableVector<Platform::Object^>^ get()
            {
                throw ref new Platform::NotImplementedException();
            }
        }
        property bool HasMoreItems
        {
            virtual bool get()
            {
                return false;
            }
        }
        virtual bool MoveCurrentTo(Platform::Object^ item) = xaml_data::ICollectionView::MoveCurrentTo
        {
            throw ref new Platform::NotImplementedException();
        }

        virtual bool MoveCurrentToPosition(int index) = xaml_data::ICollectionView::MoveCurrentToPosition
        {
            if (index < 0 || index >= static_cast<int>(m_source->Size))
            {
                return false;
            }

            m_currentPosition = index;
            m_currentChanged(this, nullptr);
            return true;
        }

        property Platform::Object^ CurrentItem
        {
           virtual Platform::Object^ get() = xaml_data::ICollectionView::CurrentItem::get
           {
               if (m_currentPosition >= 0 && m_currentPosition < static_cast<int>(m_source->Size))
               {
                   return m_source->GetAt(m_currentPosition);
               }
               return nullptr;
           }
        }

        property int CurrentPosition
        {
           virtual int get() = xaml_data::ICollectionView::CurrentPosition::get
           {
               return m_currentPosition;
           }
        }

        property bool IsCurrentAfterLast
        {
           virtual bool get() = xaml_data::ICollectionView::IsCurrentAfterLast::get
           {
               throw ref new Platform::NotImplementedException();
           }
        }

        property bool IsCurrentBeforeFirst
        {
           virtual bool get() = xaml_data::ICollectionView::IsCurrentBeforeFirst::get
           {
               throw ref new Platform::NotImplementedException();
           }
        }

        event wf::EventHandler<Platform::Object^>^ CurrentChanged
        {
           virtual wf::EventRegistrationToken add(wf::EventHandler<Platform::Object^>^ handler)
           {
               return m_currentChanged += handler;
           }
           virtual void remove(wf::EventRegistrationToken token)
           {
               m_currentChanged -= token;
           }
        }

        event xaml_data::CurrentChangingEventHandler^ CurrentChanging
        {
           virtual wf::EventRegistrationToken add(xaml_data::CurrentChangingEventHandler^ handler)
           {
               throw ref new Platform::NotImplementedException();
           }
           virtual void remove(wf::EventRegistrationToken /* token */)
           {
               throw ref new Platform::NotImplementedException();
           }
        }

        // IVector<Object^>
        virtual void Append(Platform::Object^ /*item*/)
        {
            throw ref new Platform::NotImplementedException();
        }
        virtual void Clear()
        {
            throw ref new Platform::NotImplementedException();
        }
        virtual unsigned int GetMany(unsigned int /*startIndex*/, Platform::WriteOnlyArray<Platform::Object^>^ /*items*/)
        {
            throw ref new Platform::NotImplementedException();
        }
        virtual wfc::IVectorView<Platform::Object^>^ GetView()
        {
            throw ref new Platform::NotImplementedException();
        }
        virtual void InsertAt(unsigned int /*index*/, Platform::Object^ /*item*/)
        {
            throw ref new Platform::NotImplementedException();
        }
        virtual void RemoveAt(unsigned int /*index*/)
        {
            throw ref new Platform::NotImplementedException();
        }
        virtual void RemoveAtEnd()
        {
            throw ref new Platform::NotImplementedException();
        }
        virtual void ReplaceAll(const Platform::Array<Platform::Object^>^ /*items*/)
        {
            throw ref new Platform::NotImplementedException();
        }
        virtual void SetAt(unsigned int /*index*/, Platform::Object^ /*item*/)
        {
            throw ref new Platform::NotImplementedException();
        }
        virtual Platform::Object^ GetAt(unsigned int index)
        {
            return m_source->GetAt(index);
        }

        virtual bool IndexOf(Platform::Object^ item, unsigned int* index)
        {
            return m_source->IndexOf(item, index);
        }

        property unsigned int Size
        {
            virtual unsigned int get()
            {
                return m_source->Size;
            }
        }

        // IObservableVector<Object^>
        event wfc::VectorChangedEventHandler<Platform::Object^>^ VectorChanged
        {
            virtual wf::EventRegistrationToken add(wfc::VectorChangedEventHandler<Platform::Object^>^ handler)
            {
                return m_vectorChanged += handler;
            }
            virtual void remove(wf::EventRegistrationToken token)
            {
                m_vectorChanged -= token;
            }
        }

        // IIterable<Object^>
        virtual wfc::IIterator<Platform::Object^>^ First() = wfc::IIterable<Platform::Object^>::First
        {
            throw ref new Platform::NotImplementedException();
        }

    private:
        wxaml_interop::IBindableVector^ m_source;
        int m_currentPosition;
        event wf::EventHandler<Platform::Object^>^ m_currentChanged;
        event wfc::VectorChangedEventHandler<Platform::Object^>^ m_vectorChanged;
    };

    void BindingIntegrationTests::CollectionViewBinding()
    {
        RunOnUIThread([&]
        {
            CustomConverter^ converter = ref new CustomConverter;
            converter->m_convertFunction = [](Object^ value, wxaml_interop::TypeName targetType, Object^ parameter, String^ language)
            {
                String^ trueString = "true";
                String^ falseString = "false";
                wxaml_interop::TypeName stringType = Platform::String::typeid;
                VERIFY_IS_TRUE(targetType == stringType);
                return (safe_cast<Platform::Boolean>(value) == true) ? trueString : falseString;
            };

            Platform::Collections::Vector<String^>^ stringVector = ref new Platform::Collections::Vector<String^>;
            stringVector->InsertAt(0, L"s");
            stringVector->InsertAt(1, L"s2");
            stringVector->InsertAt(2, L"s3");

            CollectionViewSource^ collectionViewSource = ref new CollectionViewSource;
            collectionViewSource->Source = stringVector;

            StackPanel ^stackPanel = ref new StackPanel();
            TextBlock^ textBlock = ref new TextBlock();

            stackPanel->Children->Append(textBlock);
            stackPanel->DataContext = collectionViewSource;

            Binding ^ binding;
            binding = ref new Binding();
            binding->Path = ref new PropertyPath(L"CurrentItem");
            textBlock->SetBinding(TextBlock::TextProperty, binding);
            VERIFY_IS_TRUE(textBlock->Text == L"s");

            binding = ref new Binding();
            binding->Path = ref new PropertyPath(L"CurrentPosition");
            textBlock->SetBinding(TextBlock::TextProperty, binding);
            VERIFY_IS_TRUE(textBlock->Text == L"0");

            binding = ref new Binding();
            binding->Path = ref new PropertyPath(L"IsCurrentBeforeFirst");
            binding->Converter = converter;
            textBlock->SetBinding(TextBlock::TextProperty, binding);
            VERIFY_IS_TRUE(textBlock->Text == L"false");

            collectionViewSource->View->MoveCurrentToPrevious();
            VERIFY_IS_TRUE(textBlock->Text == L"true");

            binding = ref new Binding();
            binding->Path = ref new PropertyPath(L"IsCurrentAfterLast");
            binding->Converter = converter;
            textBlock->SetBinding(TextBlock::TextProperty, binding);
            VERIFY_IS_TRUE(textBlock->Text == L"false");

            collectionViewSource->View->MoveCurrentToLast();
            VERIFY_IS_TRUE(textBlock->Text == L"false");
            collectionViewSource->View->MoveCurrentToNext();
            VERIFY_IS_TRUE(textBlock->Text == L"true");
            textBlock->ClearValue(TextBlock::TextProperty);

            String^ s= "s";
            String^ t = "t";
            String^ s2 = "s2";

            // Bind to custom CollectionView. The CollectionView's data is a vector of string-indexed custom property providers.
            LOG_OUTPUT(L"Bind to custom CollectionView with string-indexed custom property providers");

            StringIndexedPropertyProvider^ stringIndexedDataSource = ref new StringIndexedPropertyProvider;
            stringIndexedDataSource->Entries->Insert(L"first", s);
            StringIndexedPropertyProvider^ stringIndexedDataSource2 = ref new StringIndexedPropertyProvider;
            stringIndexedDataSource2->Entries->Insert(L"first", s2);
            Platform::Collections::Vector<Object^>^ objectVector = ref new Platform::Collections::Vector<Object^>;
            objectVector->InsertAt(0, stringIndexedDataSource);
            objectVector->InsertAt(1, stringIndexedDataSource2);

            CustomCollectionView^ customCollectionView = ref new CustomCollectionView(objectVector);
            customCollectionView->MoveCurrentToPosition(0);

            textBlock->Tag = customCollectionView;
            binding = ref new Binding();
            binding->Path = ref new PropertyPath(L"Tag[first]");
            RelativeSource^ relativeSource = ref new RelativeSource;
            relativeSource->Mode = RelativeSourceMode::Self;
            binding->RelativeSource = relativeSource;
            textBlock->SetBinding(TextBlock::TextProperty, binding);
            VERIFY_ARE_EQUAL(textBlock->Text, s);

            customCollectionView->MoveCurrentToPosition(1);
            VERIFY_ARE_EQUAL(textBlock->Text, s2);
            textBlock->ClearValue(TextBlock::TextProperty);

            // Bind to custom CollectionView. The CollectionView's data is a vector of custom property providers.
            LOG_OUTPUT(L"Bind to custom CollectionView with custom property providers");

            StringDataSource^ stringDataSource = ref new StringDataSource;
            stringDataSource->Value = s;
            StringDataSource^ stringDataSource2 = ref new StringDataSource;
            stringDataSource2->Value = s2;
            objectVector = ref new Platform::Collections::Vector<Object^>;
            objectVector->InsertAt(0, stringDataSource);
            objectVector->InsertAt(1, stringDataSource2);
            customCollectionView = ref new CustomCollectionView(objectVector);
            customCollectionView->MoveCurrentToPosition(0);

            binding = ref new Binding();
            binding->Path = ref new PropertyPath(L"Value");
            binding->Source = customCollectionView;
            textBlock->SetBinding(TextBlock::TextProperty, binding);
            VERIFY_ARE_EQUAL(textBlock->Text, s);

            customCollectionView->MoveCurrentToPosition(1);
            VERIFY_ARE_EQUAL(textBlock->Text, s2);
        });
    }

    void BindingIntegrationTests::VerifyBoolToVisibilityConverter()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            auto bindingTarget = ref new TextBox;
            auto bindingSource = ref new ToggleSwitch;

            auto binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"IsOn");
            binding->Mode = BindingMode::TwoWay;
            binding->Source = bindingSource;

            bindingTarget->SetBinding(TextBox::VisibilityProperty, binding);

            bindingSource->IsOn = true;
            VERIFY_ARE_EQUAL(Visibility::Visible, bindingTarget->Visibility, L"Invalid value of target property after changing source");

            bindingSource->IsOn = false;
            VERIFY_ARE_EQUAL(Visibility::Collapsed, bindingTarget->Visibility, L"Invalid value of target property after changing source");

            bindingTarget->Visibility = Visibility::Visible;
            VERIFY_ARE_EQUAL(true, bindingSource->IsOn, L"Invalid value of source property after changing target");

            bindingTarget->Visibility = Visibility::Collapsed;
            VERIFY_ARE_EQUAL(false, bindingSource->IsOn, L"Invalid value of source property after changing target");
        });
    }

    void BindingIntegrationTests::VerifyBoolToVisibilityConverterWithINPCDataSource()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([]()
        {
            auto dataSource = ref new InpcDataSource;
            dataSource->BooleanProperty = true;

            auto bindingTarget = ref new TextBox;

            bindingTarget->DataContext = dataSource;

            auto binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"BooleanProperty");
            binding->Mode = BindingMode::TwoWay;

            bindingTarget->SetBinding(TextBox::VisibilityProperty, binding);

            dataSource->BooleanProperty = true;
            VERIFY_ARE_EQUAL(Visibility::Visible, bindingTarget->Visibility, L"Invalid value of target property after changing source");

            dataSource->BooleanProperty = false;
            VERIFY_ARE_EQUAL(Visibility::Collapsed, bindingTarget->Visibility, L"Invalid value of target property after changing source");

            bindingTarget->Visibility = Visibility::Visible;
            VERIFY_ARE_EQUAL(true, dataSource->BooleanProperty, L"Invalid value of source property after changing target");

            bindingTarget->Visibility = Visibility::Collapsed;
            VERIFY_ARE_EQUAL(false, dataSource->BooleanProperty, L"Invalid value of source property after changing target");
        });
    }

    void BindingIntegrationTests::VerifyCanUseBoolToVisibilityConverterWithMarkup()
    {
        TestCleanupWrapper cleanup;

        StackPanel^ rootStackPanel;
        xaml_controls::ToggleSwitch^ bindingSource;
        xaml_shapes::Rectangle^ bindingTarget;

        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <ToggleSwitch x:Name='m_switch' />"
                L"  <Rectangle Height='50' Width='50' Fill='Red' Visibility='{Binding IsOn, ElementName=m_switch, Mode=TwoWay}' />"
                L"</StackPanel>";
            rootStackPanel = safe_cast<StackPanel^>(XamlReader::Load(xamlString));
            bindingSource = safe_cast<xaml_controls::ToggleSwitch^>(rootStackPanel->Children->GetAt(0));
            bindingTarget = safe_cast<xaml_shapes::Rectangle^>(rootStackPanel->Children->GetAt(1));

            TestServices::WindowHelper->WindowContent = rootStackPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            bindingSource->IsOn = true;
            VERIFY_ARE_EQUAL(Visibility::Visible, bindingTarget->Visibility, L"Invalid value of target property after changing source");

            bindingSource->IsOn = false;
            VERIFY_ARE_EQUAL(Visibility::Collapsed, bindingTarget->Visibility, L"Invalid value of target property after changing source");

            bindingTarget->Visibility = Visibility::Visible;
            VERIFY_ARE_EQUAL(true, bindingSource->IsOn, L"Invalid value of source property after changing target");

            bindingTarget->Visibility = Visibility::Collapsed;
            VERIFY_ARE_EQUAL(false, bindingSource->IsOn, L"Invalid value of source property after changing target");
        });
    }

    template <typename TClassUnderTest>
    static void BindingWorksAfterValueCoercionHelper()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([]()
        {
            TClassUnderTest^ rangebase = ref new TClassUnderTest;

            auto dataSource = ref new InpcDataSource;
            dataSource->DoubleProperty = 0.0;

            rangebase->DataContext = dataSource;

            auto binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"DoubleProperty");
            binding->Mode = BindingMode::OneWay;

            rangebase->Minimum = 0.0;
            rangebase->SetBinding(RangeBase::ValueProperty, binding);
            rangebase->Maximum = 10.0;

            // trigger value coercion
            rangebase->Minimum = 5.0;
            VERIFY_ARE_EQUAL(5.0, rangebase->Value);

            // binding expression still works
            dataSource->DoubleProperty = 7.0;
            VERIFY_ARE_EQUAL(7.0, rangebase->Value);
        });
    }

    void BindingIntegrationTests::BindingWorksAfterValueCoercion()
    {
        BindingWorksAfterValueCoercionHelper<xaml_controls::ProgressBar>();
        BindingWorksAfterValueCoercionHelper<xaml_controls::Slider>();
    }

    template <typename TClassUnderTest>
    static void BindingWorksAfterMaximumCoercionHelper()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([]()
        {
            TClassUnderTest^ rangebase = ref new TClassUnderTest;

            auto dataSource = ref new InpcDataSource;
            dataSource->DoubleProperty = 10.0;

            rangebase->DataContext = dataSource;

            auto binding = ref new Binding;
            binding->Path = ref new PropertyPath(L"DoubleProperty");
            binding->Mode = BindingMode::OneWay;

            rangebase->Minimum = 0.0;
            rangebase->Value = 10.0;
            rangebase->SetBinding(RangeBase::MaximumProperty, binding);

            // trigger value and maximum coercion.  also, maximum is updated
            rangebase->Minimum = 11.0;
            VERIFY_ARE_EQUAL(11.0, rangebase->Value);
            VERIFY_ARE_EQUAL(11.0, rangebase->Maximum);

            rangebase->Minimum = 0.0;

            // binding on max still works
            dataSource->DoubleProperty = 13.0;
            VERIFY_ARE_EQUAL(13.0, rangebase->Maximum);
        });
    }

    void BindingIntegrationTests::BindingWorksAfterMaximumCoercion()
    {
        BindingWorksAfterMaximumCoercionHelper<xaml_controls::ProgressBar>();
        BindingWorksAfterMaximumCoercionHelper<xaml_controls::Slider>();
    }

    void BindingIntegrationTests::VerifyFallbackValueCanUseThemeResource()
    {
        TestCleanupWrapper cleanup;

        StackPanel^ rootStackPanel;
        xaml_controls::ToggleSwitch^ bindingSource;
        xaml_shapes::Rectangle^ bindingTarget;

        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel.Resources>"
                L"    <ResourceDictionary>"
                L"      <ResourceDictionary.ThemeDictionaries>"
                L"        <ResourceDictionary x:Key='Default'>"
                L"          <SolidColorBrush x:Key='DefaultFillBrush' Color='Gold' />"
                L"        </ResourceDictionary>"
                L"        <ResourceDictionary x:Key='Dark'>"
                L"          <SolidColorBrush x:Key='DefaultFillBrush' Color='Blue' />"
                L"        </ResourceDictionary>"
                L"      </ResourceDictionary.ThemeDictionaries>"
                L"    </ResourceDictionary>"
                L"  </StackPanel.Resources>"
                L"  <ToggleSwitch x:Name='m_switch' />"
                L"  <Rectangle Height='50' Width='50' Fill='{Binding FooBarProperty,ElementName=m_switch,FallbackValue={ThemeResource DefaultFillBrush},Mode=OneWay}' />"
                L"</StackPanel>";
            rootStackPanel = safe_cast<StackPanel^>(XamlReader::Load(xamlString));
            bindingSource = safe_cast<xaml_controls::ToggleSwitch^>(rootStackPanel->Children->GetAt(0));
            bindingTarget = safe_cast<xaml_shapes::Rectangle^>(rootStackPanel->Children->GetAt(1));
            rootStackPanel->RequestedTheme = xaml::ElementTheme::Light;

            TestServices::WindowHelper->WindowContent = rootStackPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Gold, safe_cast<xaml_media::SolidColorBrush^>(bindingTarget->Fill)->Color);
        });

        LOG_OUTPUT(L"Changing theme to dark");
        RunOnUIThread([&]()
        {
            rootStackPanel->RequestedTheme = xaml::ElementTheme::Dark;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, safe_cast<xaml_media::SolidColorBrush^>(bindingTarget->Fill)->Color);
        });
    }

    void BindingIntegrationTests::VerifyTargetNullValueCanUseThemeResource()
    {
        TestCleanupWrapper cleanup;

        StackPanel^ rootStackPanel;
        xaml_controls::ToggleSwitch^ bindingSource;
        xaml_shapes::Rectangle^ bindingTarget;

        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel.Resources>"
                L"    <ResourceDictionary>"
                L"      <ResourceDictionary.ThemeDictionaries>"
                L"        <ResourceDictionary x:Key='Default'>"
                L"          <SolidColorBrush x:Key='DefaultFillBrush' Color='Gold' />"
                L"        </ResourceDictionary>"
                L"        <ResourceDictionary x:Key='Dark'>"
                L"          <SolidColorBrush x:Key='DefaultFillBrush' Color='Blue' />"
                L"        </ResourceDictionary>"
                L"      </ResourceDictionary.ThemeDictionaries>"
                L"    </ResourceDictionary>"
                L"  </StackPanel.Resources>"
                L"  <ToggleSwitch x:Name='m_switch' />"
                L"  <Rectangle Height='50' Width='50' Fill='{Binding Tag,ElementName=m_switch,TargetNullValue={ThemeResource DefaultFillBrush},Mode=OneWay}' />"
                L"</StackPanel>";
            rootStackPanel = safe_cast<StackPanel^>(XamlReader::Load(xamlString));
            bindingSource = safe_cast<xaml_controls::ToggleSwitch^>(rootStackPanel->Children->GetAt(0));
            bindingTarget = safe_cast<xaml_shapes::Rectangle^>(rootStackPanel->Children->GetAt(1));
            rootStackPanel->RequestedTheme = xaml::ElementTheme::Light;

            TestServices::WindowHelper->WindowContent = rootStackPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Gold, safe_cast<xaml_media::SolidColorBrush^>(bindingTarget->Fill)->Color);
        });

        LOG_OUTPUT(L"Changing theme to dark");
        RunOnUIThread([&]()
        {
            rootStackPanel->RequestedTheme = xaml::ElementTheme::Dark;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, safe_cast<xaml_media::SolidColorBrush^>(bindingTarget->Fill)->Color);
        });
    }

    void BindingIntegrationTests::VerifyBindingRefreshCountOnThemeChange()
    {
        TestCleanupWrapper cleanup;

        StackPanel^ rootStackPanel;
        xaml_controls::ToggleSwitch^ bindingSource;
        xaml_shapes::Rectangle^ bindingTarget;

        // Map of Rectangle's Name to tuple of DP change count, callback token, and the original Rectangle
        std::map<const wchar_t*,std::tuple<int, long long, xaml_shapes::Rectangle^>> propertyChangedCallbackResults;

        // Callbacks are hooked up after tree is made live, so this is the "initial" state before we start flipping the Theme
        // Remarks:
        // 1. m_oneTimeRect seems to pick up the Dark theme initially, and since it's a OneTime binding it will never update
        // Element x:Name, Fill color, DP change count
        std::vector<std::tuple<Platform::String^, ::Windows::UI::Color, int>> expectedValuesPre;
        expectedValuesPre.emplace_back(Platform::StringReference(L"m_targetNullValueRect"), Microsoft::UI::Colors::Gold, 1);
        expectedValuesPre.emplace_back(Platform::StringReference(L"m_fallbackValueRect"), Microsoft::UI::Colors::Gold, 0);
        expectedValuesPre.emplace_back(Platform::StringReference(L"m_oneTimeRect"), Microsoft::UI::Colors::Blue, 0);
        expectedValuesPre.emplace_back(Platform::StringReference(L"m_regularBindingRect"), Microsoft::UI::Colors::Red, 1);
        expectedValuesPre.emplace_back(Platform::StringReference(L"m_noThemeResourceBindingRect"), Microsoft::UI::Colors::Red, 1);

        // State after Theme has changed from Light to Dark
        // Element x:Name, Fill color, DP change count
        std::vector<std::tuple<Platform::String^, ::Windows::UI::Color, int>> expectedValuesPost;
        expectedValuesPost.emplace_back(Platform::StringReference(L"m_targetNullValueRect"), Microsoft::UI::Colors::Blue, 2);
        expectedValuesPost.emplace_back(Platform::StringReference(L"m_fallbackValueRect"), Microsoft::UI::Colors::Blue, 1);
        expectedValuesPost.emplace_back(Platform::StringReference(L"m_oneTimeRect"), Microsoft::UI::Colors::Blue, 0);
        expectedValuesPost.emplace_back(Platform::StringReference(L"m_regularBindingRect"), Microsoft::UI::Colors::Red, 1);
        expectedValuesPost.emplace_back(Platform::StringReference(L"m_noThemeResourceBindingRect"), Microsoft::UI::Colors::Red, 1);

        // State after Theme has changed from Dark to Light
        // Element x:Name, Fill color, DP change count
        std::vector<std::tuple<Platform::String^, ::Windows::UI::Color, int>> expectedValuesPostPost;
        expectedValuesPostPost.emplace_back(Platform::StringReference(L"m_targetNullValueRect"), Microsoft::UI::Colors::Gold, 3);
        expectedValuesPostPost.emplace_back(Platform::StringReference(L"m_fallbackValueRect"), Microsoft::UI::Colors::Gold, 2);
        expectedValuesPostPost.emplace_back(Platform::StringReference(L"m_oneTimeRect"), Microsoft::UI::Colors::Blue, 0);
        expectedValuesPostPost.emplace_back(Platform::StringReference(L"m_regularBindingRect"), Microsoft::UI::Colors::Red, 1);
        expectedValuesPostPost.emplace_back(Platform::StringReference(L"m_noThemeResourceBindingRect"), Microsoft::UI::Colors::Red, 1);

        auto verificationHelper = [&](std::vector<std::tuple<Platform::String^, ::Windows::UI::Color, int>> expectedValues)
        {
            RunOnUIThread([&]()
            {
                for (const auto& expectedValue : expectedValues)
                {
                    auto rectName = std::get<0>(expectedValue);
                    LOG_OUTPUT(L"Checking rectangle \'%s\'", rectName->Data());

                    auto rectangle = safe_cast<xaml_shapes::Rectangle^>(rootStackPanel->FindName(rectName));
                    auto callbackResult = propertyChangedCallbackResults.find(rectangle->Name->Data());
                    VERIFY_IS_TRUE(callbackResult != propertyChangedCallbackResults.end());

                    VERIFY_ARE_EQUAL(std::get<1>(expectedValue), safe_cast<xaml_media::SolidColorBrush^>(rectangle->Fill)->Color);
                    VERIFY_ARE_EQUAL(std::get<2>(expectedValue), std::get<0>(callbackResult->second));
                }
            });
        };

        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"   xmlns:local='using:Tests.Native.External.Framework'>"
                L"  <StackPanel.Resources>"
                L"    <ResourceDictionary>"
                L"      <ResourceDictionary.ThemeDictionaries>"
                L"        <ResourceDictionary x:Key='Default'>"
                L"          <SolidColorBrush x:Key='DefaultFillBrush' Color='Gold' />"
                L"        </ResourceDictionary>"
                L"        <ResourceDictionary x:Key='Dark'>"
                L"          <SolidColorBrush x:Key='DefaultFillBrush' Color='Blue' />"
                L"        </ResourceDictionary>"
                L"      </ResourceDictionary.ThemeDictionaries>"
                L"      <local:IdentityConverter x:Name='IdentityConverter' />"
                L"    </ResourceDictionary>"
                L"  </StackPanel.Resources>"
                L"  <ToggleSwitch x:Name='m_emptyTag' />"
                L"  <ToggleSwitch x:Name='m_tag' Tag='Red'/>"
                L"  <Rectangle x:Name='m_targetNullValueRect' Height='50' Width='50' Fill='{Binding Tag,ElementName=m_emptyTag,TargetNullValue={ThemeResource DefaultFillBrush},Mode=OneWay,Converter={StaticResource IdentityConverter}}' />"
                L"  <Rectangle x:Name='m_fallbackValueRect' Height='50' Width='50' Fill='{Binding FooBarProperty,ElementName=m_emptyTag,FallbackValue={ThemeResource DefaultFillBrush},Mode=OneWay,Converter={StaticResource IdentityConverter}}' />"
                L"  <Rectangle x:Name='m_oneTimeRect' Height='50' Width='50' Fill='{Binding FooBarProperty,ElementName=m_emptyTag,FallbackValue={ThemeResource DefaultFillBrush},Mode=OneTime,Converter={StaticResource IdentityConverter}}' />"
                L"  <Rectangle x:Name='m_regularBindingRect' Height='50' Width='50' Fill='{Binding Tag,ElementName=m_tag,FallbackValue={ThemeResource DefaultFillBrush},Mode=OneWay,Converter={StaticResource IdentityConverter}}' />"
                L"  <Rectangle x:Name='m_noThemeResourceBindingRect' Height='50' Width='50' Fill='{Binding Tag,ElementName=m_tag,Mode=OneWay,Converter={StaticResource IdentityConverter}}' />"
                L"</StackPanel>";
            rootStackPanel = safe_cast<StackPanel^>(XamlReader::Load(xamlString));
            rootStackPanel->RequestedTheme = xaml::ElementTheme::Light;

            TestServices::WindowHelper->WindowContent = rootStackPanel;

            // Set up DP change notification handlers on each Rectangle
            for (auto& expectedValue : expectedValuesPre)
            {
                auto rectName = std::get<0>(expectedValue);
                LOG_OUTPUT(L"Registering DP callback for Fill property on rectangle \'%s\'", rectName->Data());

                auto rectangle = safe_cast<xaml_shapes::Rectangle^>(rootStackPanel->FindName(rectName));
                auto token = rectangle->RegisterPropertyChangedCallback(xaml_shapes::Shape::FillProperty,
                    ref new DependencyPropertyChangedCallback([&](DependencyObject^ sender, DependencyProperty^ prop)
                    {
                        auto result = propertyChangedCallbackResults.find(safe_cast<xaml_shapes::Rectangle^>(sender)->Name->Data());
                        VERIFY_IS_TRUE(result != propertyChangedCallbackResults.end());

                        std::get<0>(result->second) += 1;
                    }));

                propertyChangedCallbackResults.emplace(rectangle->Name->Data(), std::make_tuple(0, token, rectangle));
            }

        });
        TestServices::WindowHelper->WaitForIdle();

        verificationHelper(expectedValuesPre);

        LOG_OUTPUT(L"Changing theme to dark");
        RunOnUIThread([&]()
        {
            rootStackPanel->RequestedTheme = xaml::ElementTheme::Dark;
        });
        TestServices::WindowHelper->WaitForIdle();

        verificationHelper(expectedValuesPost);

        LOG_OUTPUT(L"Changing theme back to light");
        RunOnUIThread([&]()
        {
            rootStackPanel->RequestedTheme = xaml::ElementTheme::Light;
        });
        TestServices::WindowHelper->WaitForIdle();

        verificationHelper(expectedValuesPostPost);

        RunOnUIThread([&]()
        {
            for (auto& callbackResult : propertyChangedCallbackResults)
            {
                auto token = std::get<1>(callbackResult.second);
                std::get<2>(callbackResult.second)->UnregisterPropertyChangedCallback(xaml_shapes::Shape::FillProperty, token);
            }
        });
    }

    void BindingIntegrationTests::VerifyBindingSwallowsErrorsOnThemeChange()
    {
        TestCleanupWrapper cleanup;

        StackPanel^ rootStackPanel;
        xaml_controls::ToggleSwitch^ bindingSource;
        xaml_shapes::Rectangle^ bindingTarget;

        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"   xmlns:local='using:Tests.Native.External.Framework'>"
                L"  <StackPanel.Resources>"
                L"    <ResourceDictionary>"
                L"      <ResourceDictionary.ThemeDictionaries>"
                L"        <ResourceDictionary x:Key='Default'>"
                L"          <SolidColorBrush x:Key='DefaultFillBrush' Color='Gold' />"
                L"        </ResourceDictionary>"
                L"        <ResourceDictionary x:Key='Dark'>"
                L"          <SolidColorBrush x:Key='DefaultFillBrush' Color='Blue' />"
                L"        </ResourceDictionary>"
                L"      </ResourceDictionary.ThemeDictionaries>"
                L"      <local:ThrowingConverter x:Key='ThrowingConverter' />"
                L"    </ResourceDictionary>"
                L"  </StackPanel.Resources>"
                L"  <ToggleSwitch x:Name='m_switch' />"
                L"  <Rectangle Height='50' Width='50' Fill='{Binding Tag,TargetNullValue={ThemeResource DefaultFillBrush},Mode=OneWay,Converter={StaticResource ThrowingConverter}}' />"
                L"</StackPanel>";
            rootStackPanel = safe_cast<StackPanel^>(XamlReader::Load(xamlString));
            bindingSource = safe_cast<xaml_controls::ToggleSwitch^>(rootStackPanel->Children->GetAt(0));
            bindingTarget = safe_cast<xaml_shapes::Rectangle^>(rootStackPanel->Children->GetAt(1));
            rootStackPanel->RequestedTheme = xaml::ElementTheme::Light;

            TestServices::WindowHelper->WindowContent = rootStackPanel;

            bindingTarget->DataContext = bindingSource;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_NULL(bindingTarget->Fill);
        });

        LOG_OUTPUT(L"Changing theme to dark");
        RunOnUIThread([&]()
        {
            rootStackPanel->RequestedTheme = xaml::ElementTheme::Dark;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_NULL(bindingTarget->Fill);
        });
    }

    void BindingIntegrationTests::VerifyTargetNullValueDoesNotCauseStackOverflow()
    {
        TestCleanupWrapper cleanup;

        StackPanel^ rootStackPanel;
        xaml_controls::ToggleSwitch^ bindingSource;
        xaml_shapes::Rectangle^ bindingTarget;

        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"   xmlns:local='using:Tests.Native.External.Framework'>"
                L"  <StackPanel.Resources>"
                L"    <ResourceDictionary>"
                L"      <local:NullConverter x:Key='NullConverter' />"
                L"      <SolidColorBrush x:Key='DefaultFillBrush' Color='Gold' />"
                L"    </ResourceDictionary>"
                L"  </StackPanel.Resources>"
                L"  <ToggleSwitch x:Name='m_switch' />"
                L"  <Rectangle Height='50' Width='50' Fill='{Binding Tag,ElementName=m_switch,TargetNullValue={StaticResource DefaultFillBrush},Mode=OneWay,Converter={StaticResource NullConverter}}' />"
                L"</StackPanel>";
            rootStackPanel = safe_cast<StackPanel^>(XamlReader::Load(xamlString));
            bindingSource = safe_cast<xaml_controls::ToggleSwitch^>(rootStackPanel->Children->GetAt(0));
            bindingTarget = safe_cast<xaml_shapes::Rectangle^>(rootStackPanel->Children->GetAt(1));
            rootStackPanel->RequestedTheme = xaml::ElementTheme::Light;

            TestServices::WindowHelper->WindowContent = rootStackPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_NULL(bindingTarget->Fill);
        });
    } 

    void BindingIntegrationTests::VerifyBindingTraceConvertFailed()
    {
        Platform::String^ xamlString =
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\n"
            L"   xmlns:local='using:Tests.Native.External.Framework'>\n"
            L"  <StackPanel.Resources>\n"
            L"    <ResourceDictionary>\n"
            L"      <local:IdentityConverter x:Key='IdentityConverter' />\n"
            L"      <SolidColorBrush x:Key='DefaultFillBrush' Color='Gold' />\n"
            L"    </ResourceDictionary>\n"
            L"  </StackPanel.Resources>\n"
            L"  <ToggleSwitch x:Name='m_switch' Width='100' />\n"
            L"  <Rectangle Height='50' Width='50' Fill='{Binding Width,ElementName=m_switch,Mode=OneWay,Converter={StaticResource IdentityConverter}}' />\n"
            L"</StackPanel>";

        Platform::String^ expectedMessage = 
            L"Error: Converter failed to convert value of type 'Windows.Foundation.IReference`1<Double>' "
            L"to type 'Brush'; BindingExpression: Path='Width' DataItem='Microsoft.UI.Xaml.Controls.ToggleSwitch'; "
            L"target element is 'Microsoft.UI.Xaml.Shapes.Rectangle' (Name='null'); target property is 'Fill' (type 'Brush'). ";

        VerifyBindingTraceHelper(xamlString, expectedMessage);
    }

    void BindingIntegrationTests::VerifyBindingTraceIntIndexerConnectionFailed()
    {
        Platform::String^ xamlString =
            L"<StackPanel  x:Name='m_stackPanel' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\n"
            L"   xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\n"
            L"   xmlns:local='using:Tests.Native.External.Framework'>\n"
            L"  <StackPanel.Resources>\n"
            L"    <ResourceDictionary>\n"
            L"      <local:IdentityConverter x:Key='IdentityConverter' />\n"
            L"      <SolidColorBrush x:Key='DefaultFillBrush' Color='Gold' />\n"
            L"    </ResourceDictionary>\n"
            L"  </StackPanel.Resources>\n"
            L"  <ToggleSwitch x:Name='m_switch' Width='100' />\n"
            L"  <Rectangle Height='50' Width='50' Fill='{Binding Children[5],ElementName=m_stackPanel,Mode=OneWay}' />\n"
            L"</StackPanel>";

        Platform::String^ expectedMessage = 
            L"Error: Failed to connect to index '5' in object 'Microsoft.UI.Xaml.Controls.UIElementCollection'. "
            L"BindingExpression: Path='Children[5]' DataItem='Microsoft.UI.Xaml.Controls.StackPanel'; target element "
            L"is 'Microsoft.UI.Xaml.Shapes.Rectangle' (Name='null'); target property is 'Fill' (type 'Brush')";

        VerifyBindingTraceHelper(xamlString, expectedMessage);
    }

    void BindingIntegrationTests::VerifyBindingTracePropertyConnectionFailed()
    {
        Platform::String^ xamlString =
            L"<StackPanel  x:Name='m_stackPanel' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\n"
            L"   xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\n"
            L"   xmlns:local='using:Tests.Native.External.Framework'>\n"
            L"  <StackPanel.Resources>\n"
            L"    <ResourceDictionary>\n"
            L"      <local:IdentityConverter x:Key='IdentityConverter' />\n"
            L"      <SolidColorBrush x:Key='DefaultFillBrush' Color='Gold' />\n"
            L"    </ResourceDictionary>\n"
            L"  </StackPanel.Resources>\n"
            L"  <ToggleSwitch x:Name='m_switch' Width='100' />\n"
            L"  <Rectangle Height='50' Width='50' Fill='{Binding Foobar,ElementName=m_stackPanel,Mode=OneWay}' />\n"
            L"</StackPanel>";

        Platform::String^ expectedMessage = 
            L"Error: BindingExpression path error: 'Foobar' property not found on 'Microsoft.UI.Xaml.Controls.StackPanel'. "
            L"BindingExpression: Path='Foobar' DataItem='Microsoft.UI.Xaml.Controls.StackPanel'; target element is "
            L"'Microsoft.UI.Xaml.Shapes.Rectangle' (Name='null'); target property is 'Fill' (type 'Brush')";

        VerifyBindingTraceHelper(xamlString, expectedMessage);
    }

    void BindingIntegrationTests::VerifyBindingTraceStringIndexerConnectionFailed()
    {
        Platform::String^ xamlString =
            L"<StackPanel  x:Name='m_stackPanel' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\n"
            L"   xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'\n"
            L"   xmlns:local='using:Tests.Native.External.Framework'>\n"
            L"  <StackPanel.Resources>\n"
            L"    <ResourceDictionary>\n"
            L"      <local:IdentityConverter x:Key='IdentityConverter' />\n"
            L"      <SolidColorBrush x:Key='DefaultFillBrush' Color='Gold' />\n"
            L"    </ResourceDictionary>\n"
            L"  </StackPanel.Resources>\n"
            L"  <ToggleSwitch x:Name='m_switch' Width='100' />\n"
            L"  <Rectangle Height='50' Width='50' Fill='{Binding Resources[foobar],ElementName=m_stackPanel,Mode=OneWay}' />\n"
            L"</StackPanel>";

        Platform::String^ expectedMessage = 
            L"Error: Failed to connect to index 'foobar' in object 'Microsoft.UI.Xaml.ResourceDictionary'. "
            L"BindingExpression: Path='Resources[foobar]' DataItem='Microsoft.UI.Xaml.Controls.StackPanel'; "
            L"target element is 'Microsoft.UI.Xaml.Shapes.Rectangle' (Name='null'); target property is 'Fill' "
            L"(type 'Brush')";

        VerifyBindingTraceHelper(xamlString, expectedMessage);
    }

} } } } } }
