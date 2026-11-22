// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <Utils.h>
#include <collection.h>
#include <XamlTailored.h>
#include <StringUtilities.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <DisableErrorReportingScopeGuard.h>
#include <ThreadingAssertOverride.h>
#include <ppltasks.h>
#include <XamlMetadataProviderOverrider.h>
#include <TreeHelper.h>

#include "PropertySystemIntegrationTests.h"
#include "MyButton.h"
#include "ObjectOwnedDPs.h"
#include "NullablePropertiesButton.h"
#include "CustomInterfaceObject.h"
#include "CustomTypes.XamlTypeInfo.g.h"

#undef GetCurrentTime
#include <Microsoft.UI.Xaml.h>
#include <microsoft.ui.xaml.private.h>

#undef max
#define VERIFY_THROWS_WRONG_THREAD(Expression) VERIFY_THROWS_SPECIFIC_WINRT(Expression, Platform::Exception^, [&](Platform::Exception^ e) { return e->HResult == RPC_E_WRONG_THREAD; });

using namespace Platform;
using namespace Platform::Collections;
using namespace Concurrency;
using namespace Microsoft::WRL;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Media::Media3D;

using namespace test_infra;
using namespace std;
using namespace ::Tests::Native::External::Framework;
using namespace Microsoft::UI::Xaml::Tests::Common;

using Colors = Microsoft::UI::Colors;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace PropertySystem {

        IVector<VisualStateGroup^>^ GetVisualStateGroups(Control^ control);
        VisualStateGroup^ GetVSGByName(Control^ control, Platform::String^ vsgName);

        bool PropertySystemIntegrationTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool PropertySystemIntegrationTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool PropertySystemIntegrationTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void PropertySystemIntegrationTests::DefaultValues()
        {
            RunOnUIThread([&]()
            {
                // RepeatBehavior
                auto value = Timeline::RepeatBehaviorProperty->GetMetadata(Timeline::typeid)->DefaultValue;
                VERIFY_IS_NOT_NULL(value);

                RepeatBehavior repeatBehavior = (RepeatBehavior)value;
                VERIFY_ARE_EQUAL(RepeatBehaviorType::Count, repeatBehavior.Type);

                // CornerRadius
                value = Border::CornerRadiusProperty->GetMetadata(Border::typeid)->DefaultValue;
                VERIFY_IS_NOT_NULL(value);

                // Thickness
                value = Border::BorderThicknessProperty->GetMetadata(Border::typeid)->DefaultValue;
                VERIFY_IS_NOT_NULL(value);

                // Color
                value = SolidColorBrush::ColorProperty->GetMetadata(SolidColorBrush::typeid)->DefaultValue;
                VERIFY_IS_NOT_NULL(value);

                // Some properties' metadata were regressed at one point, either due to CValue ownership, or reliance on a peer
                // Verify we can retrieve these without an exception
                LinearGradientBrush::EndPointProperty->GetMetadata(LinearGradientBrush::typeid);
                DatePicker::MinYearProperty->GetMetadata(DatePicker::typeid);
                DatePicker::MaxYearProperty->GetMetadata(DatePicker::typeid);
                DatePicker::DateProperty->GetMetadata(DatePicker::typeid);
                TimePicker::ClockIdentifierProperty->GetMetadata(TimePicker::typeid);
                TimePicker::TimeProperty->GetMetadata(TimePicker::typeid);
                RangeBase::MaximumProperty->GetMetadata(RangeBase::typeid);
                RangeBase::LargeChangeProperty->GetMetadata(RangeBase::typeid);
                RangeBase::SmallChangeProperty->GetMetadata(RangeBase::typeid);
            });
        }

        void PropertySystemIntegrationTests::SetAndGetValueDoesntLeak()
        {
            RunOnUIThread([&]()
            {
                auto b = ref new Border();
                b->CornerRadius = { 2, 2, 2, 2 };
                b->GetValue(Border::CornerRadiusProperty);
            });
        }

        void PropertySystemIntegrationTests::UnboxMatrix3D()
        {
            RunOnUIThread([&]()
            {
                auto pp = ref new PlaneProjection();
                auto matrix = pp->ProjectionMatrix;
                VERIFY_ARE_EQUAL(Matrix3DHelper::Identity, matrix);
            });
        }

        void PropertySystemIntegrationTests::SetAndGetPropertyPath()
        {
            RunOnUIThread([&]()
            {
                auto dp = DependencyProperty::RegisterAttached(L"SetAndGetPropertyPath_DP", PropertyPath::typeid, Border::typeid, nullptr);

                auto path = ref new PropertyPath(L"Test");

                auto b = ref new Border();
                b->SetValue(dp, path);
                auto actualPath = static_cast<PropertyPath^>(b->GetValue(dp));

                VERIFY_ARE_STRINGS_EQUAL(L"Test", actualPath->Path->Data());
            });
        }

        void PropertySystemIntegrationTests::SetterTypeMismatchDoesNotUpdateProperty()
        {
            DisableErrorReportingScopeGuard disableErrors;

            RunOnUIThread([&]()
            {
                auto dp = DependencyProperty::RegisterAttached(L"SetterTypeMismatchDoesNotUpdateProperty_DP", double::typeid, Border::typeid, nullptr);

                xaml::Thickness invalidValue = { 42, 42, 42, 42 };
                auto setter = ref new Setter();
                setter->Property = dp;
                setter->Value = invalidValue;

                auto style = ref new Style(Border::typeid);
                style->Setters->Append(setter);

                auto b = ref new Border();
                b->Style = style;

                auto value = safe_cast<double>(b->GetValue(dp));
                VERIFY_ARE_EQUAL((double)0.0, value);
            });
        }

        void PropertySystemIntegrationTests::ObjectIdentity()
        {
            RunOnUIThread([&]()
            {
                auto b = ref new CustomControls::MyButton();

                // Set to a DO.
                auto sp = ref new StackPanel();
                b->Tag = sp;
                b->DataContext = sp;
                b->Content = sp;
                VERIFY_ARE_EQUAL(sp, b->Tag);
                VERIFY_ARE_EQUAL(sp, b->DataContext);
                VERIFY_ARE_EQUAL(sp, b->Content);
                VERIFY_ARE_EQUAL(sp, b->GetLastNewContent());
                VERIFY_IS_NULL(b->GetLastOldContent());

                b->Content = nullptr;
                VERIFY_ARE_EQUAL(sp, b->GetLastOldContent());
                VERIFY_IS_NULL(b->GetLastNewContent());

                // Set to a primitive.
                auto primitive = safe_cast<Object^>(42);
                b->Tag = primitive;
                b->DataContext = primitive;
                b->Content = primitive;
                VERIFY_ARE_EQUAL(primitive, b->Tag);
                VERIFY_ARE_EQUAL(primitive, b->DataContext);
                VERIFY_ARE_EQUAL(primitive, b->Content);
                VERIFY_ARE_EQUAL(primitive, b->GetLastNewContent());
                VERIFY_IS_NULL(b->GetLastOldContent());

                b->Content = nullptr;
                VERIFY_ARE_EQUAL(primitive, b->GetLastOldContent());
                VERIFY_IS_NULL(b->GetLastNewContent());

                // Set to a complex object (non-DO).
                auto complex = ref new PropertyMetadata(nullptr);
                b->Tag = complex;
                b->DataContext = complex;
                b->Content = complex;
                VERIFY_ARE_EQUAL(complex, b->Tag);
                VERIFY_ARE_EQUAL(complex, b->DataContext);
                VERIFY_ARE_EQUAL(complex, b->Content);
                VERIFY_ARE_EQUAL(complex, b->GetLastNewContent());
                VERIFY_IS_NULL(b->GetLastOldContent());

                b->Content = nullptr;
                VERIFY_ARE_EQUAL(complex, b->GetLastOldContent());
                VERIFY_IS_NULL(b->GetLastNewContent());
            });
        }

        void PropertySystemIntegrationTests::SetAndGetWithDifferentTypes()
        {
            RunOnUIThread([&]()
            {
                auto b = ref new Button();
                auto f = ref new Frame();
                auto cb = ref new CheckBox();
                auto tt = ref new ToolTip();

                // Int
                b->SetValue(Control::TabIndexProperty, 42);
                VERIFY_ARE_EQUAL(42, safe_cast<int>(b->GetValue(Control::TabIndexProperty)));

                // Bool
                b->SetValue(Control::IsEnabledProperty, false);
                VERIFY_ARE_EQUAL(false, safe_cast<bool>(b->GetValue(Control::IsEnabledProperty)));

                // Double
                b->SetValue(Control::WidthProperty, 42.5);
                VERIFY_ARE_EQUAL(static_cast<double>(42.5), safe_cast<double>(b->GetValue(Control::WidthProperty)));

                // String
                b->SetValue(FrameworkElement::LanguageProperty, L"hello");
                VERIFY_ARE_STRINGS_EQUAL(L"hello", safe_cast<String^>(b->GetValue(Control::LanguageProperty))->Data());

                // Object
                auto sp = ref new StackPanel();
                b->SetValue(FrameworkElement::TagProperty, sp);
                VERIFY_ARE_EQUAL(sp, b->GetValue(Control::TagProperty));

                // TypeName
                wxaml_interop::TypeName tn = Page::typeid;
                f->SetValue(Frame::SourcePageTypeProperty, tn);
                VERIFY_ARE_EQUAL(tn.Name, safe_cast<wxaml_interop::TypeName>(f->GetValue(Frame::SourcePageTypeProperty)).Name);

                // FontWeight
                wut::FontWeight weight = { 100 };
                b->SetValue(Control::FontWeightProperty, weight);
                VERIFY_ARE_EQUAL(weight.Weight, safe_cast<wut::FontWeight>(b->GetValue(Control::FontWeightProperty)).Weight);

                // Nullable<bool>
                cb->SetValue(CheckBox::IsCheckedProperty, true);
                VERIFY_IS_TRUE(safe_cast<bool>(cb->GetValue(CheckBox::IsCheckedProperty)));
                cb->SetValue(CheckBox::IsCheckedProperty, false);
                VERIFY_IS_FALSE(safe_cast<bool>(cb->GetValue(CheckBox::IsCheckedProperty)));
                cb->SetValue(CheckBox::IsCheckedProperty, nullptr);
                VERIFY_IS_NULL(cb->GetValue(CheckBox::IsCheckedProperty));

                // Nullable<Rect>
                VERIFY_IS_NULL(tt->GetValue(ToolTip::PlacementRectProperty));
                tt->SetValue(ToolTip::PlacementRectProperty, nullptr);
                VERIFY_IS_NULL(tt->GetValue(ToolTip::PlacementRectProperty));
            });
        }

        void PropertySystemIntegrationTests::ClearValue()
        {
            RunOnUIThread([&]()
            {
                auto b = ref new Button();

                // Int
                b->SetValue(Control::TabIndexProperty, 42);
                VERIFY_ARE_EQUAL(42, safe_cast<int>(b->GetValue(Control::TabIndexProperty)));

                b->ClearValue(Control::TabIndexProperty);
                VERIFY_ARE_EQUAL(std::numeric_limits<int>::max(), safe_cast<int>(b->GetValue(Control::TabIndexProperty)));

                // Object
                auto sp = ref new StackPanel();
                b->SetValue(FrameworkElement::TagProperty, sp);
                VERIFY_ARE_EQUAL(sp, b->GetValue(Control::TagProperty));

                b->ClearValue(FrameworkElement::TagProperty);
                VERIFY_IS_NULL(b->GetValue(Control::TagProperty));
            });
        }

        void PropertySystemIntegrationTests::CustomDPsGetNotifiedForChangesOnly()
        {
            RunOnUIThread([&]()
            {
                int numberOfChanges = 0;

                auto dp = DependencyProperty::RegisterAttached(
                    L"CustomDPsGetNotifiedForChangesOnly_DP",
                    double::typeid,
                    Border::typeid,
                    PropertyMetadata::Create(
                        (Object^)nullptr,
                        ref new PropertyChangedCallback([&numberOfChanges](DependencyObject^ d, DependencyPropertyChangedEventArgs^ e)
                        {
                            numberOfChanges++;
                        })));

                auto b = ref new Border();

                b->SetValue(dp, safe_cast<double>(42.5));
                VERIFY_ARE_EQUAL(1, numberOfChanges);

                b->SetValue(dp, safe_cast<double>(42.5));
                VERIFY_ARE_EQUAL(1, numberOfChanges);

                b->SetValue(dp, safe_cast<double>(42.6));
                VERIFY_ARE_EQUAL(2, numberOfChanges);
            });
        }

        void PropertySystemIntegrationTests::TransformsCoerceNaNValues()
        {
            RunOnUIThread([&]()
            {
                auto skewTransform = ref new SkewTransform();
                skewTransform->SetValue(SkewTransform::CenterXProperty, std::numeric_limits<double>::quiet_NaN());
                VERIFY_ARE_EQUAL(0, skewTransform->CenterX);
            });
        }

        void PropertySystemIntegrationTests::ItemsSourceReentrancy()
        {
            RunOnUIThread([&]()
            {
                auto list = ref new Vector<Object^>();

                int invocationCount = 0;
                auto c = ref new ItemsControl();
                c->Items->VectorChanged += ref new VectorChangedEventHandler<Object^>(
                    [&invocationCount](wfc::IObservableVector<Platform::Object ^>^ sender, wfc::IVectorChangedEventArgs^ ev)
                {
                    invocationCount++;
                });
                c->ItemsSource = list;
                c->ItemsSource = list;
                VERIFY_ARE_EQUAL(1, invocationCount);
            });
        }

        void PropertySystemIntegrationTests::EmptyStringOnTextBlockCreatesOneRun()
        {
            RunOnUIThread([&]()
            {
                auto tb = ref new TextBlock();
                tb->Text = L"";
                VERIFY_ARE_EQUAL(1U, tb->Inlines->Size);
            });
        }

        void PropertySystemIntegrationTests::FrameworkPeerReferenceFromStyleGetsProtectionFromTargetObject()
        {
            RunOnUIThread([&]()
            {
                auto dp = DependencyProperty::RegisterAttached(L"FrameworkPeerReferenceFromStyleGetsProtectionFromTargetObject_DP", xaml_imaging::BitmapImage::typeid, Button::typeid, nullptr);

                auto button = ref new Button();

                auto style = ref new Style(Button::typeid);

                auto setter = ref new Setter();
                setter->Property = dp;
                setter->Value = ref new xaml_imaging::BitmapImage();
                style->Setters->Append(setter);

                WeakReference weakRef(setter->Value);

                button->Style = style;

                // Remove the Setter reference to BitmapImage. At this point the only reference keeping alive
                // the BitmapImage should be coming from the Button.
                style->Setters->RemoveAt(0);
                setter = nullptr;

                // Verify that the BitmapImage is still alive.
                VERIFY_IS_NOT_NULL(weakRef.Resolve<xaml_imaging::BitmapImage>());
            });
        }

        void PropertySystemIntegrationTests::DependencyPropertyChangedFiresBeforeSelectionChangedEvent()
        {
            auto selectionChangedRegistration = CreateSafeEventRegistration(Selector, SelectionChanged);

            RunOnUIThread([&]()
            {
                auto list = ref new Vector<Object^>();
                list->Append(21);
                list->Append(42);

                Object^ actualTag = nullptr;
                auto selector = ref new ListBox();
                selector->ItemsSource = list;

                auto binding = ref new Binding();
                binding->Path = ref new PropertyPath(L"SelectedValue");
                binding->Source = selector;

                selector->SetBinding(FrameworkElement::TagProperty, binding);

                selectionChangedRegistration.Attach(selector,
                    ref new xaml_controls::SelectionChangedEventHandler(
                        [&actualTag]
                        (Platform::Object^ sender, xaml_controls::SelectionChangedEventArgs^ e)
                    {
                        actualTag = static_cast<Selector^>(sender)->Tag;
                    }));

                selector->SelectedValue = 42;

                VERIFY_IS_NOT_NULL(actualTag);
                VERIFY_ARE_EQUAL(42, safe_cast<int>(actualTag));
            });
        }

        void PropertySystemIntegrationTests::DependencyPropertyChangedFiresForCustomDPs()
        {
            RunOnUIThread([&]()
            {
                auto dp = DependencyProperty::RegisterAttached(L"DependencyPropertyChangedFiresForCustomDPs_DP", int::typeid, Border::typeid, nullptr);

                auto border = ref new Border();

                auto binding = ref new Binding();
                binding->Path = ref new PropertyPath(L"Width");
                binding->Source = border;

                border->SetBinding(dp, binding);

                border->Width = 42;

                auto result = border->GetValue(dp);
                VERIFY_IS_NOT_NULL(result);
                VERIFY_ARE_EQUAL(42, safe_cast<int>(result));
            });
        }

        void PropertySystemIntegrationTests::DependencyPropertyChangedFiresForCustomDPsWhenValueChanged()
        {
            RunOnUIThread([&]()
            {
                int numberOfChanges_object = 0;
                auto dpOfTypeObject = DependencyProperty::RegisterAttached(L"DependencyPropertyChangedFiresForCustomDPsWhenValueChanged_object_DP",
                    Object::typeid,
                    Border::typeid,
                    PropertyMetadata::Create(
                        (Object^)nullptr,
                        ref new PropertyChangedCallback([&numberOfChanges_object](DependencyObject^ d, DependencyPropertyChangedEventArgs^ e)
                        {
                            numberOfChanges_object++;
                        })));

                auto border = ref new Border();

                VERIFY_ARE_EQUAL(0, numberOfChanges_object);

                // We expect the default value to be nullptr.
                border->SetValue(dpOfTypeObject, Visibility::Collapsed);
                VERIFY_ARE_EQUAL(1, numberOfChanges_object);

                // We're not changing the value here, so make sure we didn't invoke the change handler.
                border->SetValue(dpOfTypeObject, Visibility::Collapsed);
                VERIFY_ARE_EQUAL(1, numberOfChanges_object);

                border->SetValue(dpOfTypeObject, Visibility::Visible);
                VERIFY_ARE_EQUAL(2, numberOfChanges_object);

                // Visibility.Visible = 0 and VerticalAlignment.Top = 0, but we still want to
                // consider this a change because the types differ.
                border->SetValue(dpOfTypeObject, VerticalAlignment::Top);
                VERIFY_ARE_EQUAL(3, numberOfChanges_object);

                int numberOfChanges_double = 0;

                auto dpOfTypeDouble = DependencyProperty::RegisterAttached(L"DependencyPropertyChangedFiresForCustomDPsWhenValueChanged_double_DP",
                    double::typeid,
                    Border::typeid,
                    PropertyMetadata::Create(
                        (Object^)nullptr,
                        ref new PropertyChangedCallback([&numberOfChanges_double](DependencyObject^ d, DependencyPropertyChangedEventArgs^ e)
                        {
                            numberOfChanges_double++;
                        })));

                VERIFY_ARE_EQUAL(0, numberOfChanges_double);

                // We expect the default value to be 0 (double), so setting it to 0 (double) should not raise a change notification.
                border->SetValue(dpOfTypeDouble, static_cast<double>(0.0f));
                VERIFY_ARE_EQUAL(0, numberOfChanges_double);

                // Setting it to a 0 (float) should not raise a change notification, because our equality check can compare between
                // floats and doubles.
                border->SetValue(dpOfTypeDouble, static_cast<float>(0.0f));
                VERIFY_ARE_EQUAL(0, numberOfChanges_double);

                // Setting it to a 0 (int) is currently expected to raise a change notification, because our equality check doesn't consider
                // 0 (int) to be equal to 0 (double).
                border->SetValue(dpOfTypeDouble, static_cast<Object^>(0));
                VERIFY_ARE_EQUAL(1, numberOfChanges_double);

                // Setting it to a 0 (double) now is expected to raise a change notification (for Windows 8.1 compatibility reasons).
                border->SetValue(dpOfTypeDouble, static_cast<double>(0.0f));
                VERIFY_ARE_EQUAL(2, numberOfChanges_double);

                // Changing the value should result in a change notification.
                border->SetValue(dpOfTypeDouble, static_cast<double>(1.0f));
                VERIFY_ARE_EQUAL(3, numberOfChanges_double);
            });
        }

        void PropertySystemIntegrationTests::DependencyPropertyChangedFiresSynchronously()
        {
            // (RC#76981) Test Failure: Framework::PropertySystem::PropertySystemIntegrationTests::DependencyPropertyChangedFiresSynchronously
            TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

            RunOnUIThread([&]()
            {
                auto dp = DependencyProperty::RegisterAttached(L"DependencyPropertyChangedFiresSynchronously_DP", int::typeid, Border::typeid, nullptr);

                auto border = ref new Border();

                auto binding = ref new Binding();
                binding->Path = ref new PropertyPath(L"Width");
                binding->Source = border;

                border->SetBinding(dp, binding);

                // Set the source property via a style.
                auto setter = ref new Setter();
                setter->Property = FrameworkElement::WidthProperty;
                setter->Value = 40;

                auto style = ref new Style(Border::typeid);
                style->Setters->Append(setter);

                border->Style = style;

                auto result = border->GetValue(dp);
                VERIFY_IS_NOT_NULL(result);
                VERIFY_ARE_EQUAL(40, safe_cast<int>(result));

                // Set the source property from app code.
                border->Width = 42;

                result = border->GetValue(dp);
                VERIFY_IS_NOT_NULL(result);
                VERIFY_ARE_EQUAL(42, safe_cast<int>(result));
            });
        }

        void PropertySystemIntegrationTests::ItemPropertiesReentrancy()
        {
            RunOnUIThread([&]()
            {
                auto list = ref new Vector<Object^>();

                int invocationCount = 0;
                auto c = ref new ItemsControl();
                auto d = ref new MyItemTemplateSelector();
                auto e = ref new MyItemContainerStyleSelector();
                auto selector = c->ItemTemplateSelector;
                c->Items->VectorChanged += ref new VectorChangedEventHandler<Object^>(
                    [&invocationCount](wfc::IObservableVector<Platform::Object ^>^ sender, wfc::IVectorChangedEventArgs^ ev)
                {
                    invocationCount++;
                });
                c->ItemsSource = list;

                c->ItemTemplateSelector = d;
                c->ItemTemplateSelector = d;

                c->ItemContainerStyle = c->ItemContainerStyle;

                c->ItemContainerStyleSelector = e;
                c->ItemContainerStyleSelector = e;

                VERIFY_ARE_EQUAL(1, invocationCount);
            });
        }

        void PropertySystemIntegrationTests::SparseBoolPropertyConvertsStringsToBooleans()
        {
            TestCleanupWrapper cleanup;
            StackPanel^ container = nullptr;
            ToggleButton^ tb = nullptr;
            Storyboard^ storyboard = nullptr;
            auto storyboardCompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed);
            auto storyboardCompletedEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                container = ref new StackPanel();
                tb = ref new ToggleButton();
                storyboard = ref new Storyboard();
                auto animation = ref new ObjectAnimationUsingKeyFrames();
                auto keyFrame = ref new DiscreteObjectKeyFrame();

                TimeSpan ts0 = { 0 };

                KeyTime keyTime;
                keyTime.TimeSpan = ts0;
                keyFrame->KeyTime = keyTime;
                keyFrame->Value = L"True";
                animation->KeyFrames->Append(keyFrame);

                TimeSpan ts1 = { 1 };
                Duration duration = DurationHelper::FromTimeSpan(ts1);
                animation->Duration = duration;

                Storyboard::SetTarget(animation, tb);
                Storyboard::SetTargetProperty(animation, L"IsChecked");

                storyboard->Children->Append(animation);
                storyboard->Duration = duration;

                container->Tag = storyboard;

                VERIFY_IS_FALSE(tb->IsChecked->Value);

                storyboardCompletedRegistration.Attach(storyboard,
                    ref new wf::EventHandler<Object^>(
                    [storyboardCompletedEvent]
                (Object^ sender, Object^ e)
                {
                    storyboardCompletedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = container;
                container->UpdateLayout();
            });

            RunOnUIThread([&]()
            {
                storyboard->Begin();
            });

            storyboardCompletedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(tb->IsChecked->Value);
            });
        }

        void PropertySystemIntegrationTests::SparseIntPropertyConvertsStringsToInts()
        {
            TestCleanupWrapper cleanup;
            StackPanel^ container = nullptr;
            ListBox^ lb = nullptr;
            Storyboard^ storyboard = nullptr;
            auto storyboardCompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed);
            auto storyboardCompletedEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                container = ref new StackPanel();
                lb = ref new ListBox();
                lb->Items->Append(L"Item 1");
                lb->Items->Append(L"Item 2");
                storyboard = ref new Storyboard();
                auto animation = ref new ObjectAnimationUsingKeyFrames();
                auto keyFrame = ref new DiscreteObjectKeyFrame();

                TimeSpan ts0 = { 0 };

                KeyTime keyTime;
                keyTime.TimeSpan = ts0;
                keyFrame->KeyTime = keyTime;
                keyFrame->Value = L"1";
                animation->KeyFrames->Append(keyFrame);

                TimeSpan ts1 = { 1 };
                Duration duration = DurationHelper::FromTimeSpan(ts1);
                animation->Duration = duration;

                Storyboard::SetTarget(animation, lb);
                Storyboard::SetTargetProperty(animation, L"SelectedIndex");

                storyboard->Children->Append(animation);
                storyboard->Duration = duration;

                container->Tag = storyboard;

                VERIFY_ARE_EQUAL(-1, lb->SelectedIndex);

                storyboardCompletedRegistration.Attach(storyboard,
                    ref new wf::EventHandler<Object^>(
                    [storyboardCompletedEvent]
                (Object^ sender, Object^ e)
                {
                    storyboardCompletedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = container;
                container->UpdateLayout();
            });

            RunOnUIThread([&]()
            {
                storyboard->Begin();
            });

            storyboardCompletedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(1, lb->SelectedIndex);
            });
        }

        void PropertySystemIntegrationTests::CanAssignIntTypesToInt32()
        {
            RunOnUIThread([&]()
            {
                auto dp = xaml_docs::TextElement::CharacterSpacingProperty;

                auto block = ref new xaml_docs::Span();

                block->SetValue(dp, static_cast<UINT8>(1));
                VERIFY_ARE_EQUAL(1, safe_cast<int>(block->GetValue(dp)));

                block->SetValue(dp, static_cast<INT16>(2));
                VERIFY_ARE_EQUAL(2, safe_cast<int>(block->GetValue(dp)));

                block->SetValue(dp, static_cast<INT16>(-2));
                VERIFY_ARE_EQUAL(-2, safe_cast<int>(block->GetValue(dp)));

                block->SetValue(dp, static_cast<UINT16>(3));
                VERIFY_ARE_EQUAL(3, safe_cast<int>(block->GetValue(dp)));

                block->SetValue(dp, static_cast<INT32>(4));
                VERIFY_ARE_EQUAL(4, safe_cast<int>(block->GetValue(dp)));

                block->SetValue(dp, static_cast<INT32>(-4));
                VERIFY_ARE_EQUAL(-4, safe_cast<int>(block->GetValue(dp)));

                block->SetValue(dp, static_cast<UINT32>(5));
                VERIFY_ARE_EQUAL(5, safe_cast<int>(block->GetValue(dp)));

                block->SetValue(dp, static_cast<UINT32>(-1));
                VERIFY_ARE_EQUAL(-1, safe_cast<int>(block->GetValue(dp)));
            });
        }

        void PropertySystemIntegrationTests::CanSupportInt64CustomDependencyProperty()
        {
            RunOnUIThread([&]()
            {
                auto dp = DependencyProperty::RegisterAttached(L"Int64Property", int64_t::typeid, Border::typeid, nullptr);

                auto border = ref new Border;

                auto value = safe_cast<int64_t>(border->GetValue(dp));
                ++value;
                border->SetValue(dp, value);
            });
        }

        void PropertySystemIntegrationTests::CanUseCustomDependencyPropertyWithBadDefaultValue()
        {
            RunOnUIThread([&]()
            {
                auto brush = ref new SolidColorBrush(Colors::Red);

                auto dp = DependencyProperty::RegisterAttached(
                                L"DoubleProperty",
                                double::typeid,
                                Control::typeid,
                                ref new PropertyMetadata(brush));

                auto control = ref new Button;

                // Set binding that won't resolve successfully, so the property
                // system will use the custom DP's default value.
                auto border = ref new Border();
                auto binding = ref new Binding();
                binding->Path = ref new PropertyPath(L"Unknown");
                binding->Source = border;
                control->SetBinding(dp, binding);

                // Check that the current value is the default.
                auto value = static_cast<SolidColorBrush^>(control->GetValue(dp));

                VERIFY_ARE_EQUAL(Colors::Red, value->Color);
            });
        }

        void PropertySystemIntegrationTests::CanUseCustomInterfaceDependencyProperty()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]()
            {
                auto provider =
                    std::unique_ptr<XamlMetadataProviderOverrider>(new XamlMetadataProviderOverrider(ref new MetadataProvider));

                auto interfaceProp =
                    Microsoft::UI::Xaml::DependencyProperty::RegisterAttached(
                    L"CustomObject",
                    ICustomInterface::typeid,
                    Platform::Object::typeid,
                    nullptr);

                auto textProp =
                    Microsoft::UI::Xaml::DependencyProperty::RegisterAttached(
                    L"Text",
                    Platform::String::typeid,
                    Platform::Object::typeid,
                    nullptr);

                // Create ICustomInterface object, bind the attached text property to the Tag property.
                auto customObject = ref new CustomInterfaceObject();
                auto binding = ref new Binding();
                binding->Path = ref new PropertyPath(L"Tag");
                BindingOperations::SetBinding(customObject, textProp, binding);

                // Create button, set DataContext to source data.
                auto button = ref new Button;
                auto data = ref new Border;
                data->Tag = L"Bar";
                button->DataContext = data;

                // Set the attached ICustomInterface property on the button to the custom object.
                // The custom object should be boxed internally as a DO instead of an IInspectable,
                // and its inheritance context should be set.
                button->SetValue(interfaceProp, customObject);

                // Check that the custom object's Tag property has been bound and set to the data's Tag value.
                auto text = static_cast<Platform::String^>(customObject->GetValue(textProp));
                VERIFY_IS_TRUE(text == L"Bar");
            });
        }

        void PropertySystemIntegrationTests::CanBoxAndUnBoxDateTime()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([]()
            {

                ::Windows::Foundation::DateTime birthDay;
                birthDay.UniversalTime = 130582620000000000l;  // 10/20/2014, the value doesn't matter.

                auto dp = ref new xaml_controls::DatePicker();
                dp->SetValue(xaml_controls::DatePicker::DateProperty, birthDay);

                auto value = safe_cast<::Windows::Foundation::DateTime>(dp->GetValue(xaml_controls::DatePicker::DateProperty));

                VERIFY_ARE_EQUAL(value.UniversalTime, birthDay.UniversalTime);
            });
        }

        void PropertySystemIntegrationTests::CanRegisterDependencyPropertiesWithObjectOwner()
        {
            TestCleanupWrapper cleanup;
            RunOnUIThread([]()
            {
                auto provider =
                    std::unique_ptr<XamlMetadataProviderOverrider>(new XamlMetadataProviderOverrider(ref new MetadataProvider));

                SomeTypeWithDPsOwnedByObject::InitDPs();
                DOWithDPsOwnedByObject::InitDPs();

                {
                    HRESULT hr = S_OK;
                    DisableErrorReportingScopeGuard guard;

                    try
                    {
                        auto doInstance = safe_cast<DOWithDPsOwnedByObject^>(
                            XamlReader::Load(
                                L"<local:DOWithDPsOwnedByObject"
                                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                                L"  xmlns:local='using:Tests.Native.External.Framework'"
                                L"  Text='{Binding}' />"));
                    }
                    catch (Platform::Exception^ e)
                    {
                        // We get an exception because DOWithDPsOwnedByObject is a DO and we don't
                        // check object owned DPs for dependency objects to stay compatible with windows 8.x
                        // behavior.
                        // Since we don't find the Text dependency property, the framework does a regular property
                        // assignment which fails since it tries to assign a Binding instance to a string property.
                        hr = e->HResult;
                    }

                    VERIFY_FAILED(hr);
                }

                // Below, Text is assigned as a regular property.
                // And Binding gets assigned the binding itself rather than the data bound object.
                // See the explanation above on the exception to understand why.
                auto doInstance = safe_cast<DOWithDPsOwnedByObject^>(
                    XamlReader::Load(
                        L"<local:DOWithDPsOwnedByObject"
                        L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"  xmlns:local='using:Tests.Native.External.Framework'"
                        L"  Text='Hello'"
                        L"  Binding='{Binding SomePath}' />"));

                VERIFY_ARE_EQUAL(ref new Platform::String(L"SomePath"), doInstance->Binding->Path->Path);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"Hello"), doInstance->Text);

                // SomeTypeWithDPsOwnedByObject.Text is registered on a non-DO type with an Object owner.
                // It can be data bound with no problem.
                doInstance = safe_cast<DOWithDPsOwnedByObject^>(
                    XamlReader::Load(
                        L"<local:DOWithDPsOwnedByObject"
                        L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"  xmlns:local='using:Tests.Native.External.Framework'"
                        L"  local:SomeTypeWithDPsOwnedByObject.Text='{Binding}' />"));

                auto expectedText = ref new Platform::String(L"World");
                doInstance->DataContext = expectedText;
                VERIFY_ARE_EQUAL(expectedText, SomeTypeWithDPsOwnedByObject::GetText(doInstance));

                SomeTypeWithDPsOwnedByObject::ResetDPs();
                DOWithDPsOwnedByObject::ResetDPs();
            });
        }

        void PropertySystemIntegrationTests::SupportsWeakRefDependencyProperties()
        {
            TestCleanupWrapper cleanup;

            // Page::Frame
            RunOnUIThread([]()
            {
                wxaml_interop::TypeName pageType = { L"Microsoft.UI.Xaml.Controls.Page", wxaml_interop::TypeKind::Primitive };

                auto frame = ref new Frame();
                WeakReference weakRef(frame);

                // Implicitly sets page's Frame property
                frame->Navigate(pageType);

                auto page = safe_cast<Page^>(frame->Content);

                VERIFY_IS_TRUE(page != nullptr, L"Frame's Content value is a page");
                VERIFY_IS_TRUE(page->Frame == frame, L"Page's Frame value is the frame");
                VERIFY_IS_TRUE(static_cast<Frame^>(page->GetValue(Page::FrameProperty)) == frame, L"Page's Frame value is the frame");

                // Release our frame reference, but keep our reference on the page.
                frame = nullptr;

                VERIFY_IS_TRUE(page->Frame == nullptr, L"Page's Frame value is null after releasing local frame reference");
                VERIFY_IS_TRUE(weakRef.Resolve<Frame>() == nullptr, L"Frame has been destroyed");
            });

            // Hub::SemanticZoomOwner
            RunOnUIThread([&]()
            {
                auto sezo = ref new SemanticZoom();
                WeakReference weakRef(sezo);

                auto hub = ref new Hub();

                // Implicitly sets hub's SemanticZoomOwner property
                sezo->ZoomedInView = hub;

                VERIFY_IS_TRUE(hub->SemanticZoomOwner == sezo, L"Hub's SemanticZoomOwner value is the SemanticZoom");
                VERIFY_IS_TRUE(static_cast<SemanticZoom^>(hub->GetValue(Hub::SemanticZoomOwnerProperty)) == sezo, L"Hub's SemanticZoomOwner value is the SemanticZoom");

                sezo = nullptr;

                VERIFY_IS_TRUE(hub->SemanticZoomOwner == nullptr, L"Hub's SemanticZoomOwner value is null after releasing local SemanticZoom reference");
                VERIFY_IS_TRUE(weakRef.Resolve<SemanticZoom>() == nullptr, L"SemanticZoom has been destroyed");
            });

            // ListView::SemanticZoomOwner
            RunOnUIThread([&]()
            {
                auto sezo = ref new SemanticZoom();
                WeakReference weakRef(sezo);

                auto listView = ref new ListView();

                // Implicitly sets listView's SemanticZoomOwner property
                sezo->ZoomedInView = listView;

                VERIFY_IS_TRUE(listView->SemanticZoomOwner == sezo, L"ListView's SemanticZoomOwner value is the SemanticZoom");
                VERIFY_IS_TRUE(static_cast<SemanticZoom^>(listView->GetValue(ListViewBase::SemanticZoomOwnerProperty)) == sezo, L"ListView's SemanticZoomOwner value is the SemanticZoom");

                sezo = nullptr;

                VERIFY_IS_TRUE(listView->SemanticZoomOwner == nullptr, L"ListView's SemanticZoomOwner value is null after releasing local SemanticZoom reference");
                VERIFY_IS_TRUE(weakRef.Resolve<SemanticZoom>() == nullptr, L"SemanticZoom has been destroyed");
            });
        }

        void PropertySystemIntegrationTests::CanSetFloatPropertyToThicknessThemeResource()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Grid.Resources>"
                    L"        <ResourceDictionary>"
                    L"        <ResourceDictionary.ThemeDictionaries>"
                    L"            <ResourceDictionary x:Key='Default'>"
                    L"                <Thickness x:Key='Thickness1'>4,3,2,1</Thickness>"
                    L"            </ResourceDictionary>"
                    L"        </ResourceDictionary.ThemeDictionaries>"
                    L"        </ResourceDictionary>"
                    L"    </Grid.Resources>"
                    L"    <Button Width='{ThemeResource Thickness1}' Content='Hello' />"
                    L"</Grid>";

                auto panel = safe_cast<Panel^>(XamlReader::Load(xamlString));
                auto button = safe_cast<Button^>(panel->Children->GetAt(0));
                VERIFY_ARE_EQUAL(4, button->Width, L"Button.Width property should be set to the Thickness.Left value");
            });
        }

        void PropertySystemIntegrationTests::CanOverwriteValueOfResourceDictionaryWithExplicitKey()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([]()
            {
                Platform::String^ xamlString =
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Grid.Resources>"
                    L"        <ResourceDictionary x:Key='Default'>"
                    L"            <Thickness x:Key='Thickness1'>4,3,2,1</Thickness>"
                    L"        </ResourceDictionary>"
                    L"    </Grid.Resources>"
                    L"</Grid>";

                auto panel = safe_cast<Panel^>(XamlReader::Load(xamlString));

                VERIFY_ARE_EQUAL(panel->Resources->Size, 1U);

                auto rd = ref new ResourceDictionary();
                panel->Resources = rd;

                VERIFY_ARE_EQUAL(panel->Resources->Size, 0U);
            });
        }

        void PropertySystemIntegrationTests::ImageSourceDoesNotReturnStaleValue()
        {
            RunOnUIThread([&]()
            {
                auto image = ref new Image();
                auto source = ref new Microsoft::UI::Xaml::Media::Imaging::BitmapImage(ref new Uri("ms-appx:///notrelevant.png"));
                image->Source = source;

                auto root = ref new Canvas();
                TestServices::WindowHelper->WindowContent = root;

                // Have the image temporarily enter the tree to trigger an image load.
                root->Children->Append(image);
                root->Children->Clear();

                // Now the image is no longer in the live tree, clear its source property.
                image->ClearValue(Image::SourceProperty);

                // Verify that going through GetValue() does not return a stale value.
                VERIFY_IS_NULL(image->GetValue(Image::SourceProperty));
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void PropertySystemIntegrationTests::ThemeExpressionEvaluationDoesNotOverwriteBaseValueSource()
        {
            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' RequestedTheme='Dark'>"
                    L"    <Grid.Resources>"
                    L"        <SolidColorBrush x:Key='redBrush' Color='Red' />"
                    L"        <SolidColorBrush x:Key='blueBrush' Color='Blue' />"
                    L"        <Style TargetType='Rectangle' x:Key='redStyle'>"
                    L"            <Setter Property='Fill' Value='{ThemeResource redBrush}' />"
                    L"        </Style>"
                    L"        <Style TargetType='Rectangle' x:Key='blueStyle'>"
                    L"            <Setter Property='Fill' Value='{ThemeResource blueBrush}' />"
                    L"        </Style>"
                    L"    </Grid.Resources>"
                    L"    <Rectangle x:Name='myRectangle' Style='{StaticResource redStyle}' />"
                    L"</Grid>";

                auto panel = safe_cast<Panel^>(XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = panel; // Set the content to make sure the style is applied

                auto myRectangle = safe_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(panel->FindName(L"myRectangle"));
                VERIFY_ARE_EQUAL(Colors::Red, static_cast<SolidColorBrush^>(myRectangle->Fill)->Color);

                // Force re-evaluation of the theme expression.
                panel->RequestedTheme = ElementTheme::Light;

                // Make sure the rectangle is still red.
                VERIFY_ARE_EQUAL(Colors::Red, static_cast<SolidColorBrush^>(myRectangle->Fill)->Color);

                myRectangle->Style = safe_cast<Style^>(panel->Resources->Lookup("blueStyle"));

                // Make sure the rectangle changed to blue.
                VERIFY_ARE_EQUAL(Colors::Blue, static_cast<SolidColorBrush^>(myRectangle->Fill)->Color);
            });
        }

        void PropertySystemIntegrationTests::NewStyleClearsThemeResourceExpression()
        {
            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' RequestedTheme='Dark'>"
                    L"    <Grid.Resources>"
                    L"        <SolidColorBrush x:Key='redBrush' Color='Red' />"
                    L"        <SolidColorBrush x:Key='blueBrush' Color='Blue' />"
                    L"        <Style TargetType='Rectangle' x:Key='redStyle'>"
                    L"            <Setter Property='Fill' Value='{ThemeResource redBrush}' />"
                    L"        </Style>"
                    L"        <Style TargetType='Rectangle' x:Key='blueStyle'>"
                    L"            <Setter Property='Fill' Value='Blue' />"
                    L"        </Style>"
                    L"    </Grid.Resources>"
                    L"    <Rectangle x:Name='myRectangle' Style='{StaticResource redStyle}' />"
                    L"</Grid>";

                auto panel = safe_cast<Panel^>(XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = panel; // Set the content to make sure the style is applied

                auto myRectangle = safe_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(panel->FindName(L"myRectangle"));
                VERIFY_ARE_EQUAL(Colors::Red, static_cast<SolidColorBrush^>(myRectangle->Fill)->Color);

                // Force re-evaluation of the theme expression.
                panel->RequestedTheme = ElementTheme::Light;

                // Make sure the rectangle is still red.
                VERIFY_ARE_EQUAL(Colors::Red, static_cast<SolidColorBrush^>(myRectangle->Fill)->Color);

                myRectangle->Style = safe_cast<Style^>(panel->Resources->Lookup("blueStyle"));

                // Make sure the rectangle changed to blue.
                VERIFY_ARE_EQUAL(Colors::Blue, static_cast<SolidColorBrush^>(myRectangle->Fill)->Color);

                // Switch theme again.
                panel->RequestedTheme = ElementTheme::Dark;

                // Make sure the rectangle is still blue.
                VERIFY_ARE_EQUAL(Colors::Blue, static_cast<SolidColorBrush^>(myRectangle->Fill)->Color);
            });
        }

        void PropertySystemIntegrationTests::GradientColorPropertyConvertsStringsToColor()
        {
            TestCleanupWrapper cleanup;
            xaml_shapes::Rectangle^ r = nullptr;
            LinearGradientBrush^ brush = nullptr;
            GradientStop^ gs0 = nullptr;
            GradientStop^ gs1 = nullptr;
            Storyboard^ storyboard = nullptr;
            auto storyboardCompletedRegistration = CreateSafeEventRegistration(Storyboard, Completed);
            auto storyboardCompletedEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                r = ref new xaml_shapes::Rectangle();

                brush = ref new LinearGradientBrush();
                brush->StartPoint = Point(0, 0);
                brush->EndPoint = Point(1, 1);

                gs0 = ref new GradientStop();
                gs0->Offset = 0.0;
                brush->GradientStops->Append(gs0);

                gs1 = ref new GradientStop();
                gs1->Offset = 1.0;
                brush->GradientStops->Append(gs1);

                r->Fill = brush;

                storyboard = ref new Storyboard();
                auto animation = ref new ObjectAnimationUsingKeyFrames();
                auto keyFrame = ref new DiscreteObjectKeyFrame();

                KeyTime keyTime;
                keyTime.TimeSpan = TimeSpan();

                keyFrame->KeyTime = keyTime;
                keyFrame->Value = L"#FF123456";

                animation->KeyFrames->Append(keyFrame);

                Storyboard::SetTarget(animation, gs1);
                Storyboard::SetTargetProperty(animation, L"Color");

                storyboard->Children->Append(animation);

                storyboardCompletedRegistration.Attach(storyboard,
                    ref new wf::EventHandler<Object^>(
                        [storyboardCompletedEvent]
                (Object^ sender, Object^ e)
                {
                    storyboardCompletedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = r;
                r->UpdateLayout();
            });

            RunOnUIThread([&]()
            {
                auto c = gs1->Color;

                VERIFY_ARE_EQUAL(c.A, 0x00);
                VERIFY_ARE_EQUAL(c.R, 0x00);
                VERIFY_ARE_EQUAL(c.G, 0x00);
                VERIFY_ARE_EQUAL(c.B, 0x00);

                storyboard->Begin();
            });

            storyboardCompletedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                auto c = gs1->Color;

                VERIFY_ARE_EQUAL(c.A, 0xFF);
                VERIFY_ARE_EQUAL(c.R, 0x12);
                VERIFY_ARE_EQUAL(c.G, 0x34);
                VERIFY_ARE_EQUAL(c.B, 0x56);
            });
        }

        void PropertySystemIntegrationTests::VerifyThreadException_DependencyObject()
        {
            TestCleanupWrapper cleanup;
            DisableErrorReportingScopeGuard disableErrors;
            ThreadingAssertOverride disableThreadingAssert;

            Shapes::Rectangle^ mainViewRectangle;

            Platform::String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Button x:Name='myButton' Background='Orange'>Button</Button>"
                L"</StackPanel>";

            RunOnUIThread([&]()
            {
                auto stackPanel = dynamic_cast<StackPanel^>(XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = stackPanel;

                WEX::Logging::Log::Comment(L"Create rectangle in main view");
                mainViewRectangle = ref new Shapes::Rectangle();
            });

            TestServices::WindowHelper->WaitForIdle();

            WEX::Logging::Log::Comment(L"Create secondary view");
            auto secondaryView = TestServices::WindowHelper->CreateNewView(ref new ViewCreatedCallback([&] {
                // RunOnUI thread is not needed, since we are already on the correct thread
                auto stackPanel = dynamic_cast<StackPanel^>(XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = stackPanel;
            }));

            WEX::Logging::Log::Comment(L"Bring secondary view to the front");
            TestServices::WindowHelper->BringSecondaryViewToFront(secondaryView);

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto stackPanel = dynamic_cast<StackPanel^>(TestServices::WindowHelper->WindowContent);
                WEX::Logging::Log::Comment(L"Assign main view rectangle to StackPanel in secondary view");
                VERIFY_THROWS_WRONG_THREAD(stackPanel->Children->Append(mainViewRectangle));
            });

            TestServices::WindowHelper->WaitForIdle();

            WEX::Logging::Log::Comment(L"Bring main view to the front");
            TestServices::WindowHelper->BringMainViewToFront();

            if (secondaryView) { delete secondaryView; }
        }

        void PropertySystemIntegrationTests::VerifyThreadException_MultiParentShareableDO()
        {
            TestCleanupWrapper cleanup;
            DisableErrorReportingScopeGuard disableErrors;
            ThreadingAssertOverride disableThreadingAssert;

            ListView^ mainViewListView;

            Platform::String^ xamlString =
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      x:Name='root' Background='SlateBlue' Width='400' Height='200' VerticalAlignment='Top' HorizontalAlignment='Left'> "
                L"  <Button x:Name='myButton' Content='button.flyout' HorizontalAlignment='Left' FontSize='25' > "
                L"    <Button.Flyout> "
                L"      <Flyout Placement='Top'> "
                L"        <AutoSuggestBox x:Name='ASB'/> "
                L"      </Flyout> "
                L"    </Button.Flyout> "
                L"  </Button> "
                L"</Grid>";

            RunOnUIThread([&]()
            {
                auto grid = dynamic_cast<Grid^>(XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = grid;

                WEX::Logging::Log::Comment(L"Create ListView in main view");
                mainViewListView = ref new ListView();
                TestServices::WindowHelper->WindowContent = grid;
            });

            TestServices::WindowHelper->WaitForIdle();

            WEX::Logging::Log::Comment(L"Create secondary view");
            auto secondaryView = TestServices::WindowHelper->CreateNewView(ref new ViewCreatedCallback([&] {
                // RunOnUI thread is not needed, since we are already on the correct thread
                auto grid = dynamic_cast<Grid^>(XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = grid;
            }));

            WEX::Logging::Log::Comment(L"Bring secondary view to the front");
            TestServices::WindowHelper->BringSecondaryViewToFront(secondaryView);

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto grid = dynamic_cast<Grid^>(TestServices::WindowHelper->WindowContent);
                auto secondaryViewButton = safe_cast<xaml_controls::Button^>(grid->FindName(L"myButton"));
                auto secondaryViewFlyout = safe_cast<xaml_controls::Flyout^>(secondaryViewButton->Flyout);

                WEX::Logging::Log::Comment(L"Assign main view ListView to Flyout in secondary view");
                VERIFY_THROWS_WRONG_THREAD(secondaryViewFlyout->Content = mainViewListView);
            });

            TestServices::WindowHelper->WaitForIdle();

            WEX::Logging::Log::Comment(L"Bring main view to the front");
            TestServices::WindowHelper->BringMainViewToFront();

            if (secondaryView) { delete secondaryView; }
        }

        void PropertySystemIntegrationTests::VerifyThreadException_ShareableDO()
        {
            TestCleanupWrapper cleanup;
            DisableErrorReportingScopeGuard disableErrors;
            ThreadingAssertOverride disableThreadingAssert;

            VisualState^ mainViewVisualState;

            Platform::String^ xamlString =
                L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button.Template>"
                L"    <ControlTemplate TargetType='ButtonBase'>"
                L"      <Grid Background='Transparent' x:Name='LayoutRoot'>"
                L"        <VisualStateManager.VisualStateGroups>"
                L"          <VisualStateGroup x:Name='VSG1'>"
                L"            <VisualState x:Name='Focused' />"
                L"          </VisualStateGroup>"
                L"        </VisualStateManager.VisualStateGroups>"
                L"      </Grid>"
                L"    </ControlTemplate>"
                L"  </Button.Template>"
                L"</Button>";

            RunOnUIThread([&]()
            {
                auto button = dynamic_cast<Button^>(XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = button;

                WEX::Logging::Log::Comment(L"Create visual state in main view");
                mainViewVisualState = ref new VisualState();
                mainViewVisualState->SetValue(FrameworkElement::NameProperty, "Unfocused");
            });

            TestServices::WindowHelper->WaitForIdle();

            WEX::Logging::Log::Comment(L"Create secondary view");
            auto secondaryView = TestServices::WindowHelper->CreateNewView(ref new ViewCreatedCallback([&] {
                // RunOnUI thread is not needed, since we are already on the correct thread
                auto button = dynamic_cast<Button^>(XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = button;
            }));

            WEX::Logging::Log::Comment(L"Bring secondary view to the front");
            TestServices::WindowHelper->BringSecondaryViewToFront(secondaryView);

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto button = dynamic_cast<Button^>(TestServices::WindowHelper->WindowContent);
                auto secondaryViewVSG = GetVSGByName(button, L"VSG1");

                VERIFY_IS_NOT_NULL(secondaryViewVSG, L"VSG not found.");

                WEX::Logging::Log::Comment(L"Assign main view VisualState to VisualStateGroup in secondary view");
                VERIFY_THROWS_WRONG_THREAD(secondaryViewVSG->States->Append(mainViewVisualState));
            });

            TestServices::WindowHelper->WaitForIdle();

            WEX::Logging::Log::Comment(L"Bring main view to the front");
            TestServices::WindowHelper->BringMainViewToFront();

            if (secondaryView) { delete secondaryView; }
        }

        void PropertySystemIntegrationTests::VerifyThreadException_NoParentShareableDO()
        {
            TestCleanupWrapper cleanup;
            DisableErrorReportingScopeGuard disableErrors;
            ThreadingAssertOverride disableThreadingAssert;

            SolidColorBrush^ mainViewBrush;

            Platform::String^ xamlString =
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Button x:Name='myButton' Background='Orange'>Button</Button>"
                L"</Grid>";

            RunOnUIThread([&]()
            {
                auto grid = dynamic_cast<Grid^>(XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = grid;

                WEX::Logging::Log::Comment(L"Create brush in main view");
                mainViewBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Teal);
            });

            TestServices::WindowHelper->WaitForIdle();

            WEX::Logging::Log::Comment(L"Create secondary view");
            auto secondaryView = TestServices::WindowHelper->CreateNewView(ref new ViewCreatedCallback([&] {
                // RunOnUI thread is not needed, since we are already on the correct thread
                auto grid = dynamic_cast<Grid^>(XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = grid;
            }));

            WEX::Logging::Log::Comment(L"Bring secondary view to the front");
            TestServices::WindowHelper->BringSecondaryViewToFront(secondaryView);

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto grid = dynamic_cast<Grid^>(TestServices::WindowHelper->WindowContent);
                auto secondaryViewButton = safe_cast<xaml_controls::Button^>(grid->FindName(L"myButton"));

                WEX::Logging::Log::Comment(L"Assign main view brush to button in secondary view");
                VERIFY_THROWS_WRONG_THREAD(secondaryViewButton->Background = mainViewBrush);
            });

            TestServices::WindowHelper->WaitForIdle();

            WEX::Logging::Log::Comment(L"Bring main view to the front");
            TestServices::WindowHelper->BringMainViewToFront();

            if (secondaryView) { delete secondaryView; }
        }

        void PropertySystemIntegrationTests::VerifyThreadException_DataBinding()
        {
            TestCleanupWrapper cleanup;
            DisableErrorReportingScopeGuard disableErrors;
            ThreadingAssertOverride disableThreadingAssert;

            Button^ mainViewButton = nullptr;
            Button^ secondaryViewButton = nullptr;
            Data::Binding^ binding = nullptr;

            Platform::String^ xamlString =
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Button x:Name='myButton' Background='Orange'>Button</Button>"
                L"</Grid>";

            RunOnUIThread([&]()
            {
                auto grid = dynamic_cast<Grid^>(XamlReader::Load(xamlString));
                WEX::Logging::Log::Comment(L"Create brush, button1, and binding in main view");
                mainViewButton = safe_cast<xaml_controls::Button^>(grid->FindName(L"myButton"));

                binding = ref new Microsoft::UI::Xaml::Data::Binding();
                binding->Mode = Microsoft::UI::Xaml::Data::BindingMode::TwoWay;
                binding->Source = mainViewButton;
                binding->Path = ref new PropertyPath(L"Background");

                TestServices::WindowHelper->WindowContent = grid;
            });

            TestServices::WindowHelper->WaitForIdle();

            WEX::Logging::Log::Comment(L"Create secondary view");
            auto secondaryView = TestServices::WindowHelper->CreateNewView(ref new ViewCreatedCallback([&] {
                // RunOnUI thread is not needed, since we are already on the correct thread
                auto grid = dynamic_cast<Grid^>(XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = grid;
            }));

            TestServices::WindowHelper->WaitForIdle();

            WEX::Logging::Log::Comment(L"Bring secondary view to the front");
            TestServices::WindowHelper->BringSecondaryViewToFront(secondaryView);

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                WEX::Logging::Log::Comment(L"Create button2 in secondary view");
                secondaryViewButton = ref new Button();

                WEX::Logging::Log::Comment(L"Create two way binding of BackgroundProperty between mainViewButton and secondaryViewButton");
                VERIFY_THROWS_WRONG_THREAD(secondaryViewButton->SetBinding(Button::BackgroundProperty, binding));
            });

            TestServices::WindowHelper->WaitForIdle();

            WEX::Logging::Log::Comment(L"Bring main view to the front");
            TestServices::WindowHelper->BringMainViewToFront();

            TestServices::WindowHelper->WaitForIdle();

            if (secondaryView) { delete secondaryView; }
        }

        void PropertySystemIntegrationTests::VerifyThreadException_ContentControlContent()
        {
            TestCleanupWrapper cleanup;
            DisableErrorReportingScopeGuard disableErrors;
            ThreadingAssertOverride disableThreadingAssert;

            Shapes::Rectangle^ mainViewRectangle;

            Platform::String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Button x:Name='myButton' Background='Orange'>Button</Button>"
                L"</StackPanel>";

            RunOnUIThread([&]()
            {
                auto stackPanel = dynamic_cast<StackPanel^>(XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = stackPanel;

                WEX::Logging::Log::Comment(L"Create rectangle in main view");
                mainViewRectangle = ref new Shapes::Rectangle();
            });

            TestServices::WindowHelper->WaitForIdle();

            WEX::Logging::Log::Comment(L"Create secondary view");
            auto secondaryView = TestServices::WindowHelper->CreateNewView(ref new ViewCreatedCallback([&] {
                // RunOnUI thread is not needed, since we are already on the correct thread
                auto stackPanel = dynamic_cast<StackPanel^>(XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = stackPanel;
            }));

            WEX::Logging::Log::Comment(L"Bring secondary view to the front");
            TestServices::WindowHelper->BringSecondaryViewToFront(secondaryView);

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto secondaryViewContentControl = ref new xaml_controls::ContentControl();

                WEX::Logging::Log::Comment(L"Assign main view rectangle to ContentControl in secondary view");
                VERIFY_THROWS_WRONG_THREAD(secondaryViewContentControl->Content = mainViewRectangle);
            });

            TestServices::WindowHelper->WaitForIdle();

            WEX::Logging::Log::Comment(L"Bring main view to the front");
            TestServices::WindowHelper->BringMainViewToFront();

            if (secondaryView) { delete secondaryView; }
        }

        void PropertySystemIntegrationTests::VerifyThreadException_CustomDependencyProperty()
        {
            TestCleanupWrapper cleanup;
            DisableErrorReportingScopeGuard disableErrors;
            ThreadingAssertOverride disableThreadingAssert;

            DependencyObject^ mainViewDO;

            Platform::String^ xamlString =
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Button x:Name='myButton' Background='Orange'>Button</Button>"
                L"</StackPanel>";

            RunOnUIThread([&]()
            {
                auto stackPanel = dynamic_cast<StackPanel^>(XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = stackPanel;

                WEX::Logging::Log::Comment(L"Create DependencyObject in main view");
                mainViewDO = ref new TextBlock();
            });

            TestServices::WindowHelper->WaitForIdle();

            WEX::Logging::Log::Comment(L"Create secondary view");
            auto secondaryView = TestServices::WindowHelper->CreateNewView(ref new ViewCreatedCallback([&] {
                // RunOnUI thread is not needed, since we are already on the correct thread
                auto stackPanel = dynamic_cast<StackPanel^>(XamlReader::Load(xamlString));
                TestServices::WindowHelper->WindowContent = stackPanel;
            }));

            WEX::Logging::Log::Comment(L"Bring secondary view to the front");
            TestServices::WindowHelper->BringSecondaryViewToFront(secondaryView);

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto secondaryViewBorder = ref new Border();
                auto customDP = DependencyProperty::RegisterAttached(
                    L"CustomDP",
                    Platform::Object::typeid,
                    Border::typeid,
                    nullptr);
                WEX::Logging::Log::Comment(L"Assign main view DO value to a border's custom DP in secondary view");
                VERIFY_THROWS_WRONG_THREAD(secondaryViewBorder->SetValue(customDP, mainViewDO));
            });

            TestServices::WindowHelper->WaitForIdle();

            WEX::Logging::Log::Comment(L"Bring main view to the front");
            TestServices::WindowHelper->BringMainViewToFront();

            if (secondaryView) { delete secondaryView; }
        }

        void PropertySystemIntegrationTests::CanSetNullableProperty()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]()
            {
                auto provider =
                    std::unique_ptr<XamlMetadataProviderOverrider>(new XamlMetadataProviderOverrider(ref new MetadataProvider));

                NullablePropertiesButton::InitDPs();

                Platform::String^ xamlString =
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' xmlns:local='using:Tests.Native.External.Framework' RequestedTheme='Dark'>"
                    L"    <Grid.Resources>"
                    L"        <x:Int32 x:Key='intResource'>9</x:Int32>"
                    L"    </Grid.Resources>"
                    L"    <local:NullablePropertiesButton x:Name='buttonVal9' NullableDouble='{StaticResource intResource}' />"
                    L"    <local:NullablePropertiesButton x:Name='buttonVal5' NullableDouble='5' />"
                    L"    <local:NullablePropertiesButton x:Name='buttonValNull' NullableDouble='{x:Null}' />"
                    L"</Grid>";

                auto grid = safe_cast<Grid^>(XamlReader::Load(xamlString));
                auto buttonVal9 = safe_cast<NullablePropertiesButton^>(grid->FindName(L"buttonVal9"));
                VERIFY_IS_NOT_NULL(buttonVal9, L"Couldn't find buttonVal9");
                VERIFY_IS_NOT_NULL(buttonVal9->NullableDouble, L"buttonVal9's NullableDouble unexpectedly null");
                VERIFY_ARE_EQUAL(buttonVal9->NullableDouble->Value, 9.0);

                auto buttonVal5 = safe_cast<NullablePropertiesButton^>(grid->FindName(L"buttonVal5"));
                VERIFY_IS_NOT_NULL(buttonVal5, L"Couldn't find buttonVal5");
                VERIFY_IS_NOT_NULL(buttonVal5->NullableDouble, L"buttonVal5's NullableDouble unexpectedly null");
                VERIFY_ARE_EQUAL(buttonVal5->NullableDouble->Value, 5.0);

                auto buttonValNull = safe_cast<NullablePropertiesButton^>(grid->FindName(L"buttonValNull"));
                VERIFY_IS_NOT_NULL(buttonValNull, L"Couldn't find buttonValNull");
                VERIFY_IS_NULL(buttonValNull->NullableDouble, L"buttonValNull's NullableDouble unexpectedly not null");

                NullablePropertiesButton::ResetDPs();
            });
        }

        void PropertySystemIntegrationTests::TestStyleSelector()
        {
            TestCleanupWrapper cleanup;

            Panel^ panel;

            auto items = ref new Vector<CustomDataObject^>();
            items->Append(ref new CustomDataObject(true));
            items->Append(ref new CustomDataObject(false));

            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<Panel"
                    L"      xmlns='http://schemas.microsoft.com/client/2007'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"</Panel>";

                panel = safe_cast<Panel^>(XamlReader::Load(xamlString));

                TestServices::WindowHelper->WindowContent = panel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                Platform::String^ dataTemplateString =
                    L"<DataTemplate x:Key='trueTemplate'"
                    L"      xmlns='http://schemas.microsoft.com/client/2007'"
                    L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Border Background='Transparent'/>"
                    L"</DataTemplate>";

                DataTemplate^ dataTemplate = safe_cast<DataTemplate^>(XamlReader::Load(dataTemplateString));

                CustomStyleSelector^ styleSelector = ref new CustomStyleSelector();
                auto listView = ref new ListView();

                listView->ItemsSource = items,
                listView->ItemContainerStyleSelector = styleSelector,
                listView->ItemContainerTransitions = nullptr,
                listView->ItemTemplate = dataTemplate;

                panel->Children->Append(listView);

                items->GetAt(0)->Flag = true;
                items->GetAt(1)->Flag = true;
                items->GetAt(0)->Flag = false;
                items->GetAt(1)->Flag = false;
                items->GetAt(0)->Flag = true;
                items->GetAt(1)->Flag = true;
                items->GetAt(0)->Flag = false;
                items->GetAt(1)->Flag = false;

                panel->Children->RemoveAt(panel->Children->Size - 1);
            });
        };

        IVector<VisualStateGroup^>^ GetVisualStateGroups(Control^ control)
        {
            auto layoutRoot = safe_cast<FrameworkElement^>(TreeHelper::GetVisualChildByName(control, L"LayoutRoot"));
            VERIFY_IS_NOT_NULL(layoutRoot, WEX::Common::String().Format(L"Finding FE named 'LayoutRoot' in %s.", control->Name->Data()));
            return VisualStateManager::GetVisualStateGroups(layoutRoot);
        }

        VisualStateGroup^ GetVSGByName(Control^ control, Platform::String^ vsgName)
        {
            VisualStateGroup^ result = nullptr;
            auto visualStateGroups = GetVisualStateGroups(control);
            auto iter = std::find_if(
                begin(visualStateGroups),
                end(visualStateGroups),
                [vsgName](VisualStateGroup^ currentGroup)
            {
                return currentGroup->Name == vsgName;
            });

            if (iter != end(visualStateGroups))
            {
                result = *iter;
            }

            return result;
        }

        void PropertySystemIntegrationTests::CompactEnumConversion()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]()
            {
                auto setter = ref new Setter(FrameworkElement::HorizontalAlignmentProperty, HorizontalAlignment::Right);

                VERIFY_NO_THROW(safe_cast<HorizontalAlignment>(setter->Value));
                VERIFY_ARE_EQUAL(HorizontalAlignment::Right, safe_cast<HorizontalAlignment>(setter->Value));
            });
        }

        namespace
        {
            using AbiRect = ABI::Windows::Foundation::Rect;
            using AbiPoint = ABI::Windows::Foundation::Point;
            using AbiSize = ABI::Windows::Foundation::Size;

            const float c_positiveInfinity = std::numeric_limits<float>::infinity();
            const float c_negativeInfinity = -std::numeric_limits<float>::infinity();

            Microsoft::WRL::ComPtr<ABI::Microsoft::UI::Xaml::IRectHelperStatics> GetRectHelperStatics()
            {
                Microsoft::WRL::ComPtr<ABI::Microsoft::UI::Xaml::IRectHelperStatics> statics;
                VERIFY_SUCCEEDED(RoGetActivationFactory(
                    Microsoft::WRL::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_RectHelper).Get(),
                    IID_PPV_ARGS(&statics)));
                VERIFY_IS_NOT_NULL(statics.Get());
                return statics;
            }

            AbiRect MakeAbiRect(float x, float y, float width, float height)
            {
                AbiRect rect = {};
                rect.X = x;
                rect.Y = y;
                rect.Width = width;
                rect.Height = height;
                return rect;
            }

            AbiPoint MakeAbiPoint(float x, float y)
            {
                AbiPoint point = {};
                point.X = x;
                point.Y = y;
                return point;
            }

            AbiSize MakeAbiSize(float width, float height)
            {
                AbiSize size = {};
                size.Width = width;
                size.Height = height;
                return size;
            }

            void VerifyAbiRect(const AbiRect& actual, float x, float y, float width, float height)
            {
                VERIFY_ARE_EQUAL(x, actual.X);
                VERIFY_ARE_EQUAL(y, actual.Y);
                VERIFY_ARE_EQUAL(width, actual.Width);
                VERIFY_ARE_EQUAL(height, actual.Height);
            }

            void VerifyIsEmptyRect(const AbiRect& actual)
            {
                VerifyAbiRect(actual, c_positiveInfinity, c_positiveInfinity, c_negativeInfinity, c_negativeInfinity);
            }

            Microsoft::WRL::ComPtr<ABI::Windows::Foundation::IPropertyValue> AsAbiPropertyValue(Platform::Object^ value)
            {
                VERIFY_IS_NOT_NULL(value);
                Microsoft::WRL::ComPtr<ABI::Windows::Foundation::IPropertyValue> propertyValue;
                VERIFY_SUCCEEDED(reinterpret_cast<IInspectable*>(value)->QueryInterface(IID_PPV_ARGS(&propertyValue)));
                return propertyValue;
            }

            // Calls every scalar and array getter of IPropertyValue and verifies that each one is rejected with E_NOTIMPL.
            void VerifyAllPropertyValueGettersAreNotImplemented(ABI::Windows::Foundation::IPropertyValue* propertyValue)
            {
                BYTE uint8Value = 0;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetUInt8(&uint8Value));
                INT16 int16Value = 0;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetInt16(&int16Value));
                UINT16 uint16Value = 0;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetUInt16(&uint16Value));
                INT32 int32Value = 0;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetInt32(&int32Value));
                UINT32 uint32Value = 0;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetUInt32(&uint32Value));
                INT64 int64Value = 0;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetInt64(&int64Value));
                UINT64 uint64Value = 0;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetUInt64(&uint64Value));
                FLOAT singleValue = 0;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetSingle(&singleValue));
                DOUBLE doubleValue = 0;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetDouble(&doubleValue));
                WCHAR char16Value = 0;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetChar16(&char16Value));
                BOOLEAN booleanValue = FALSE;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetBoolean(&booleanValue));
                HSTRING stringValue = nullptr;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetString(&stringValue));
                VERIFY_IS_NULL(stringValue);
                GUID guidValue = {};
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetGuid(&guidValue));
                ABI::Windows::Foundation::DateTime dateTimeValue = {};
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetDateTime(&dateTimeValue));
                ABI::Windows::Foundation::TimeSpan timeSpanValue = {};
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetTimeSpan(&timeSpanValue));
                AbiPoint pointValue = {};
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetPoint(&pointValue));
                AbiSize sizeValue = {};
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetSize(&sizeValue));
                AbiRect rectValue = {};
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetRect(&rectValue));

                UINT32 length = 0;
                BYTE* uint8Array = nullptr;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetUInt8Array(&length, &uint8Array));
                INT16* int16Array = nullptr;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetInt16Array(&length, &int16Array));
                UINT16* uint16Array = nullptr;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetUInt16Array(&length, &uint16Array));
                INT32* int32Array = nullptr;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetInt32Array(&length, &int32Array));
                UINT32* uint32Array = nullptr;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetUInt32Array(&length, &uint32Array));
                INT64* int64Array = nullptr;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetInt64Array(&length, &int64Array));
                UINT64* uint64Array = nullptr;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetUInt64Array(&length, &uint64Array));
                FLOAT* singleArray = nullptr;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetSingleArray(&length, &singleArray));
                DOUBLE* doubleArray = nullptr;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetDoubleArray(&length, &doubleArray));
                WCHAR* char16Array = nullptr;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetChar16Array(&length, &char16Array));
                BOOLEAN* booleanArray = nullptr;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetBooleanArray(&length, &booleanArray));
                HSTRING* stringArray = nullptr;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetStringArray(&length, &stringArray));
                IInspectable** inspectableArray = nullptr;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetInspectableArray(&length, &inspectableArray));
                GUID* guidArray = nullptr;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetGuidArray(&length, &guidArray));
                ABI::Windows::Foundation::DateTime* dateTimeArray = nullptr;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetDateTimeArray(&length, &dateTimeArray));
                ABI::Windows::Foundation::TimeSpan* timeSpanArray = nullptr;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetTimeSpanArray(&length, &timeSpanArray));
                AbiPoint* pointArray = nullptr;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetPointArray(&length, &pointArray));
                AbiSize* sizeArray = nullptr;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetSizeArray(&length, &sizeArray));
                AbiRect* rectArray = nullptr;
                VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetRectArray(&length, &rectArray));
            }
        }

        void PropertySystemIntegrationTests::RectHelperFactoryMethodsValidateAndNormalize()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([]()
            {
                auto statics = GetRectHelperStatics();
                AbiRect result = {};

                LOG_OUTPUT(L"FromCoordinatesAndDimensions copies every field.");
                VERIFY_SUCCEEDED(statics->FromCoordinatesAndDimensions(1.5f, -2.25f, 3.0f, 4.5f, &result));
                VerifyAbiRect(result, 1.5f, -2.25f, 3.0f, 4.5f);

                LOG_OUTPUT(L"FromCoordinatesAndDimensions accepts a zero width and height.");
                VERIFY_SUCCEEDED(statics->FromCoordinatesAndDimensions(5.0f, 6.0f, 0.0f, 0.0f, &result));
                VerifyAbiRect(result, 5.0f, 6.0f, 0.0f, 0.0f);

                LOG_OUTPUT(L"FromCoordinatesAndDimensions rejects a negative width or a negative height and resets the result.");
                result = MakeAbiRect(9.0f, 9.0f, 9.0f, 9.0f);
                VERIFY_ARE_EQUAL(E_INVALIDARG, statics->FromCoordinatesAndDimensions(1.0f, 2.0f, -0.5f, 3.0f, &result));
                VerifyAbiRect(result, 0.0f, 0.0f, 0.0f, 0.0f);
                result = MakeAbiRect(9.0f, 9.0f, 9.0f, 9.0f);
                VERIFY_ARE_EQUAL(E_INVALIDARG, statics->FromCoordinatesAndDimensions(1.0f, 2.0f, 3.0f, -0.5f, &result));
                VerifyAbiRect(result, 0.0f, 0.0f, 0.0f, 0.0f);
                // A null result pointer is rejected as an invalid argument.
                VERIFY_ARE_EQUAL(E_INVALIDARG, statics->FromCoordinatesAndDimensions(1.0f, 2.0f, 3.0f, 4.0f, nullptr));

                LOG_OUTPUT(L"FromPoints normalizes the points independently on each axis.");
                VERIFY_SUCCEEDED(statics->FromPoints(MakeAbiPoint(10.0f, 2.0f), MakeAbiPoint(4.0f, 9.0f), &result));
                VerifyAbiRect(result, 4.0f, 2.0f, 6.0f, 7.0f);
                VERIFY_SUCCEEDED(statics->FromPoints(MakeAbiPoint(1.0f, 9.0f), MakeAbiPoint(5.0f, 3.0f), &result));
                VerifyAbiRect(result, 1.0f, 3.0f, 4.0f, 6.0f);
                VERIFY_SUCCEEDED(statics->FromPoints(MakeAbiPoint(8.0f, 7.0f), MakeAbiPoint(-2.0f, -3.0f), &result));
                VerifyAbiRect(result, -2.0f, -3.0f, 10.0f, 10.0f);
                VERIFY_SUCCEEDED(statics->FromPoints(MakeAbiPoint(3.0f, 3.0f), MakeAbiPoint(3.0f, 3.0f), &result));
                VerifyAbiRect(result, 3.0f, 3.0f, 0.0f, 0.0f);
                VERIFY_ARE_EQUAL(E_INVALIDARG, statics->FromPoints(MakeAbiPoint(0.0f, 0.0f), MakeAbiPoint(1.0f, 1.0f), nullptr));

                LOG_OUTPUT(L"FromLocationAndSize copies location and size, including a zero size.");
                VERIFY_SUCCEEDED(statics->FromLocationAndSize(MakeAbiPoint(7.0f, -8.0f), MakeAbiSize(2.0f, 3.0f), &result));
                VerifyAbiRect(result, 7.0f, -8.0f, 2.0f, 3.0f);
                VERIFY_SUCCEEDED(statics->FromLocationAndSize(MakeAbiPoint(7.0f, 8.0f), MakeAbiSize(0.0f, 0.0f), &result));
                VerifyAbiRect(result, 7.0f, 8.0f, 0.0f, 0.0f);

                LOG_OUTPUT(L"FromLocationAndSize returns the Empty rect when either dimension of the size is negative.");
                VERIFY_SUCCEEDED(statics->FromLocationAndSize(MakeAbiPoint(7.0f, 8.0f), MakeAbiSize(-1.0f, 3.0f), &result));
                VerifyIsEmptyRect(result);
                VERIFY_SUCCEEDED(statics->FromLocationAndSize(MakeAbiPoint(7.0f, 8.0f), MakeAbiSize(2.0f, -1.0f), &result));
                VerifyIsEmptyRect(result);
                VERIFY_ARE_EQUAL(E_INVALIDARG, statics->FromLocationAndSize(MakeAbiPoint(0.0f, 0.0f), MakeAbiSize(1.0f, 1.0f), nullptr));
            });
        }

        void PropertySystemIntegrationTests::RectHelperEdgeAccessorsHandleEmptyAndZeroSizedRects()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([]()
            {
                auto statics = GetRectHelperStatics();
                float left = 0.0f;
                float top = 0.0f;
                float right = 0.0f;
                float bottom = 0.0f;

                auto verifyEdges = [&statics, &left, &top, &right, &bottom](const AbiRect& rect, float expectedLeft, float expectedTop, float expectedRight, float expectedBottom)
                {
                    VERIFY_SUCCEEDED(statics->GetLeft(rect, &left));
                    VERIFY_SUCCEEDED(statics->GetTop(rect, &top));
                    VERIFY_SUCCEEDED(statics->GetRight(rect, &right));
                    VERIFY_SUCCEEDED(statics->GetBottom(rect, &bottom));
                    VERIFY_ARE_EQUAL(expectedLeft, left);
                    VERIFY_ARE_EQUAL(expectedTop, top);
                    VERIFY_ARE_EQUAL(expectedRight, right);
                    VERIFY_ARE_EQUAL(expectedBottom, bottom);
                };

                LOG_OUTPUT(L"Regular rect with a negative origin.");
                verifyEdges(MakeAbiRect(-3.0f, 5.0f, 7.0f, 11.0f), -3.0f, 5.0f, 4.0f, 16.0f);

                LOG_OUTPUT(L"Zero-sized rect: right/bottom equal left/top.");
                verifyEdges(MakeAbiRect(2.5f, -1.5f, 0.0f, 0.0f), 2.5f, -1.5f, 2.5f, -1.5f);

                LOG_OUTPUT(L"A negative height alone does not make the rect empty.");
                verifyEdges(MakeAbiRect(1.0f, 2.0f, 3.0f, -4.0f), 1.0f, 2.0f, 4.0f, -2.0f);

                LOG_OUTPUT(L"A negative width makes the rect empty: right and bottom are negative infinity.");
                verifyEdges(MakeAbiRect(1.0f, 2.0f, -1.0f, 5.0f), 1.0f, 2.0f, c_negativeInfinity, c_negativeInfinity);

                LOG_OUTPUT(L"The Empty rect: left/top are positive infinity, right/bottom are negative infinity.");
                AbiRect empty = {};
                VERIFY_SUCCEEDED(statics->get_Empty(&empty));
                verifyEdges(empty, c_positiveInfinity, c_positiveInfinity, c_negativeInfinity, c_negativeInfinity);

                VERIFY_ARE_EQUAL(E_INVALIDARG, statics->GetLeft(empty, nullptr));
                VERIFY_ARE_EQUAL(E_INVALIDARG, statics->GetTop(empty, nullptr));
                VERIFY_ARE_EQUAL(E_INVALIDARG, statics->GetRight(empty, nullptr));
                VERIFY_ARE_EQUAL(E_INVALIDARG, statics->GetBottom(empty, nullptr));
            });
        }

        void PropertySystemIntegrationTests::RectHelperEqualsAndUnionWithPoint()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([]()
            {
                auto statics = GetRectHelperStatics();
                AbiRect empty = {};
                VERIFY_SUCCEEDED(statics->get_Empty(&empty));

                auto verifyEquals = [&statics](const AbiRect& target, const AbiRect& value, bool expected)
                {
                    BOOLEAN areEqual = !expected;
                    VERIFY_SUCCEEDED(statics->Equals(target, value, &areEqual));
                    VERIFY_ARE_EQUAL(expected, !!areEqual);
                };

                const AbiRect baseRect = MakeAbiRect(1.0f, 2.0f, 3.0f, 4.0f);

                LOG_OUTPUT(L"Equals compares every field.");
                verifyEquals(baseRect, MakeAbiRect(1.0f, 2.0f, 3.0f, 4.0f), true);
                verifyEquals(baseRect, MakeAbiRect(9.0f, 2.0f, 3.0f, 4.0f), false);
                verifyEquals(baseRect, MakeAbiRect(1.0f, 9.0f, 3.0f, 4.0f), false);
                verifyEquals(baseRect, MakeAbiRect(1.0f, 2.0f, 9.0f, 4.0f), false);
                verifyEquals(baseRect, MakeAbiRect(1.0f, 2.0f, 3.0f, 9.0f), false);

                LOG_OUTPUT(L"A field that is smaller than the target's field is also a difference.");
                verifyEquals(baseRect, MakeAbiRect(0.5f, 2.0f, 3.0f, 4.0f), false);
                verifyEquals(baseRect, MakeAbiRect(1.0f, 1.5f, 3.0f, 4.0f), false);
                verifyEquals(baseRect, MakeAbiRect(1.0f, 2.0f, 2.5f, 4.0f), false);
                verifyEquals(baseRect, MakeAbiRect(1.0f, 2.0f, 3.0f, 3.5f), false);

                LOG_OUTPUT(L"Equals treats any rect with a negative width as the Empty rect.");
                verifyEquals(empty, empty, true);
                verifyEquals(MakeAbiRect(1.0f, 2.0f, -1.0f, 4.0f), empty, true);
                verifyEquals(empty, MakeAbiRect(1.0f, 2.0f, -1.0f, 4.0f), true);
                verifyEquals(empty, baseRect, false);
                verifyEquals(baseRect, empty, false);
                verifyEquals(MakeAbiRect(0.0f, 0.0f, 0.0f, 0.0f), empty, false);
                VERIFY_ARE_EQUAL(E_INVALIDARG, statics->Equals(baseRect, baseRect, nullptr));

                AbiRect result = {};
                const AbiRect target = MakeAbiRect(10.0f, 20.0f, 30.0f, 40.0f);

                LOG_OUTPUT(L"UnionWithPoint leaves the rect unchanged for points inside it or on its edge.");
                VERIFY_SUCCEEDED(statics->UnionWithPoint(target, MakeAbiPoint(15.0f, 25.0f), &result));
                VerifyAbiRect(result, 10.0f, 20.0f, 30.0f, 40.0f);
                VERIFY_SUCCEEDED(statics->UnionWithPoint(target, MakeAbiPoint(40.0f, 60.0f), &result));
                VerifyAbiRect(result, 10.0f, 20.0f, 30.0f, 40.0f);
                VERIFY_SUCCEEDED(statics->UnionWithPoint(target, MakeAbiPoint(10.0f, 20.0f), &result));
                VerifyAbiRect(result, 10.0f, 20.0f, 30.0f, 40.0f);

                LOG_OUTPUT(L"UnionWithPoint grows the rect toward a point on each side.");
                VERIFY_SUCCEEDED(statics->UnionWithPoint(target, MakeAbiPoint(4.0f, 25.0f), &result));
                VerifyAbiRect(result, 4.0f, 20.0f, 36.0f, 40.0f);
                VERIFY_SUCCEEDED(statics->UnionWithPoint(target, MakeAbiPoint(50.0f, 25.0f), &result));
                VerifyAbiRect(result, 10.0f, 20.0f, 40.0f, 40.0f);
                VERIFY_SUCCEEDED(statics->UnionWithPoint(target, MakeAbiPoint(15.0f, 8.0f), &result));
                VerifyAbiRect(result, 10.0f, 8.0f, 30.0f, 52.0f);
                VERIFY_SUCCEEDED(statics->UnionWithPoint(target, MakeAbiPoint(15.0f, 70.0f), &result));
                VerifyAbiRect(result, 10.0f, 20.0f, 30.0f, 50.0f);
                VERIFY_SUCCEEDED(statics->UnionWithPoint(target, MakeAbiPoint(0.0f, 0.0f), &result));
                VerifyAbiRect(result, 0.0f, 0.0f, 40.0f, 60.0f);

                LOG_OUTPUT(L"UnionWithPoint on the Empty rect produces a zero-sized rect at the point.");
                VERIFY_SUCCEEDED(statics->UnionWithPoint(empty, MakeAbiPoint(3.0f, 4.0f), &result));
                VerifyAbiRect(result, 3.0f, 4.0f, 0.0f, 0.0f);
                VERIFY_ARE_EQUAL(E_INVALIDARG, statics->UnionWithPoint(target, MakeAbiPoint(0.0f, 0.0f), nullptr));
            });
        }

        void PropertySystemIntegrationTests::RectHelperDipsRectToPhysicalRectScalesEveryField()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([]()
            {
                auto statics = GetRectHelperStatics();
                Microsoft::WRL::ComPtr<ABI::Microsoft::UI::Xaml::IRectHelperStaticsPrivate> privateStatics;
                VERIFY_SUCCEEDED(statics.As(&privateStatics));

                // A unit width makes the resulting width equal to the scale that is applied to every field.
                const AbiRect dips = MakeAbiRect(3.0f, -5.0f, 1.0f, 7.0f);
                AbiRect physical = MakeAbiRect(0.0f, 0.0f, 0.0f, 0.0f);
                VERIFY_SUCCEEDED(privateStatics->DipsRectToPhysicalRect(dips, &physical));

                const float scale = physical.Width;
                LOG_OUTPUT(L"DipsRectToPhysicalRect scale = %f", scale);
                VERIFY_IS_GREATER_THAN(scale, 0.0f);
                VerifyAbiRect(physical, dips.X * scale, dips.Y * scale, dips.Width * scale, dips.Height * scale);

                LOG_OUTPUT(L"A zero rect stays a zero rect.");
                VERIFY_SUCCEEDED(privateStatics->DipsRectToPhysicalRect(MakeAbiRect(0.0f, 0.0f, 0.0f, 0.0f), &physical));
                VerifyAbiRect(physical, 0.0f, 0.0f, 0.0f, 0.0f);
                VERIFY_ARE_EQUAL(E_INVALIDARG, privateStatics->DipsRectToPhysicalRect(dips, nullptr));

                TestServices::WindowHelper->WindowContent = ref new Border();
            });
            TestServices::WindowHelper->WaitForIdle();

            // The test scale override is applied to every content root and is removed again by the cleanup wrapper.
            LOG_OUTPUT(L"With a known rasterization scale of 1.5 every field is multiplied by 1.5.");
            TestServices::WindowHelper->SetWindowSizeOverrideWithScale(::Windows::Foundation::Size(400, 300), 1.5f);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([]()
            {
                auto statics = GetRectHelperStatics();
                Microsoft::WRL::ComPtr<ABI::Microsoft::UI::Xaml::IRectHelperStaticsPrivate> privateStatics;
                VERIFY_SUCCEEDED(statics.As(&privateStatics));

                AbiRect physical = MakeAbiRect(0.0f, 0.0f, 0.0f, 0.0f);
                VERIFY_SUCCEEDED(privateStatics->DipsRectToPhysicalRect(MakeAbiRect(3.0f, -5.0f, 1.0f, 7.0f), &physical));
                VerifyAbiRect(physical, 4.5f, -7.5f, 1.5f, 10.5f);
            });
        }

        void PropertySystemIntegrationTests::XamlBoxedStructRejectsScalarAndArrayPropertyValueGetters()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([]()
            {
                auto border = ref new Border();
                Thickness margin = { 1, 2, 3, 4 };
                border->Margin = margin;
                CornerRadius cornerRadius = { 5, 6, 7, 8 };
                border->CornerRadius = cornerRadius;

                Platform::Object^ boxedMargin = border->GetValue(FrameworkElement::MarginProperty);
                Platform::Object^ boxedCornerRadius = border->GetValue(Border::CornerRadiusProperty);

                std::vector<Platform::Object^> boxedValues = { boxedMargin, boxedCornerRadius };
                for (Platform::Object^ boxed : boxedValues)
                {
                    auto propertyValue = AsAbiPropertyValue(boxed);

                    ABI::Windows::Foundation::PropertyType type = ABI::Windows::Foundation::PropertyType_Empty;
                    VERIFY_SUCCEEDED(propertyValue->get_Type(&type));
                    VERIFY_ARE_EQUAL(ABI::Windows::Foundation::PropertyType_OtherType, type);

                    BOOLEAN isNumericScalar = TRUE;
                    VERIFY_SUCCEEDED(propertyValue->get_IsNumericScalar(&isNumericScalar));
                    VERIFY_IS_FALSE(!!isNumericScalar);

                    VerifyAllPropertyValueGettersAreNotImplemented(propertyValue.Get());
                }

                LOG_OUTPUT(L"The failed getters leave the boxed values intact.");
                VERIFY_ARE_EQUAL(margin, safe_cast<Thickness>(boxedMargin));
                auto unboxedCornerRadius = safe_cast<CornerRadius>(boxedCornerRadius);
                VERIFY_ARE_EQUAL(cornerRadius.TopLeft, unboxedCornerRadius.TopLeft);
                VERIFY_ARE_EQUAL(cornerRadius.TopRight, unboxedCornerRadius.TopRight);
                VERIFY_ARE_EQUAL(cornerRadius.BottomRight, unboxedCornerRadius.BottomRight);
                VERIFY_ARE_EQUAL(cornerRadius.BottomLeft, unboxedCornerRadius.BottomLeft);
            });
        }

        void PropertySystemIntegrationTests::XamlBoxedEnumReportsNumericScalarAndUInt32Value()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([]()
            {
                auto border = ref new Border();

                auto verifyEnumBox = [](Platform::Object^ boxed, UINT32 expectedValue)
                {
                    auto propertyValue = AsAbiPropertyValue(boxed);

                    ABI::Windows::Foundation::PropertyType type = ABI::Windows::Foundation::PropertyType_Empty;
                    VERIFY_SUCCEEDED(propertyValue->get_Type(&type));
                    VERIFY_ARE_EQUAL(ABI::Windows::Foundation::PropertyType_OtherType, type);

                    BOOLEAN isNumericScalar = FALSE;
                    VERIFY_SUCCEEDED(propertyValue->get_IsNumericScalar(&isNumericScalar));
                    VERIFY_IS_TRUE(!!isNumericScalar);

                    UINT32 uint32Value = 0xFFFFFFFF;
                    VERIFY_SUCCEEDED(propertyValue->GetUInt32(&uint32Value));
                    VERIFY_ARE_EQUAL(expectedValue, uint32Value);

                    INT32 int32Value = -1;
                    VERIFY_SUCCEEDED(propertyValue->GetInt32(&int32Value));
                    VERIFY_ARE_EQUAL(static_cast<INT32>(expectedValue), int32Value);

                    BYTE uint8Value = 0;
                    VERIFY_ARE_EQUAL(E_NOTIMPL, propertyValue->GetUInt8(&uint8Value));
                };

                LOG_OUTPUT(L"Visibility.Visible (0)");
                verifyEnumBox(border->GetValue(UIElement::VisibilityProperty), 0u);

                LOG_OUTPUT(L"Visibility.Collapsed (1)");
                border->Visibility = Visibility::Collapsed;
                verifyEnumBox(border->GetValue(UIElement::VisibilityProperty), 1u);

                LOG_OUTPUT(L"HorizontalAlignment.Right (2) and HorizontalAlignment.Stretch (3)");
                border->HorizontalAlignment = HorizontalAlignment::Right;
                verifyEnumBox(border->GetValue(FrameworkElement::HorizontalAlignmentProperty), 2u);
                border->HorizontalAlignment = HorizontalAlignment::Stretch;
                verifyEnumBox(border->GetValue(FrameworkElement::HorizontalAlignmentProperty), 3u);
            });
        }

        void PropertySystemIntegrationTests::EnumPropertyAcceptsInt32AndUInt32Boxes()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([]()
            {
                auto border = ref new Border();
                VERIFY_ARE_EQUAL(Visibility::Visible, border->Visibility);

                LOG_OUTPUT(L"A boxed Int32 sets the enum value.");
                border->SetValue(UIElement::VisibilityProperty, PropertyValue::CreateInt32(1));
                VERIFY_ARE_EQUAL(Visibility::Collapsed, border->Visibility);
                border->SetValue(UIElement::VisibilityProperty, PropertyValue::CreateInt32(0));
                VERIFY_ARE_EQUAL(Visibility::Visible, border->Visibility);

                LOG_OUTPUT(L"A boxed UInt32 sets the enum value.");
                border->SetValue(UIElement::VisibilityProperty, PropertyValue::CreateUInt32(1u));
                VERIFY_ARE_EQUAL(Visibility::Collapsed, border->Visibility);
                border->SetValue(UIElement::VisibilityProperty, PropertyValue::CreateUInt32(0u));
                VERIFY_ARE_EQUAL(Visibility::Visible, border->Visibility);

                LOG_OUTPUT(L"The boxed integer is mapped to the matching member of a multi-valued enum.");
                border->SetValue(FrameworkElement::HorizontalAlignmentProperty, PropertyValue::CreateInt32(2));
                VERIFY_ARE_EQUAL(HorizontalAlignment::Right, border->HorizontalAlignment);
                border->SetValue(FrameworkElement::HorizontalAlignmentProperty, PropertyValue::CreateUInt32(1u));
                VERIFY_ARE_EQUAL(HorizontalAlignment::Center, border->HorizontalAlignment);

                LOG_OUTPUT(L"Every member of the enum is reachable from both a boxed UInt32 and a boxed Int32.");
                border->SetValue(FrameworkElement::HorizontalAlignmentProperty, PropertyValue::CreateUInt32(2u));
                VERIFY_ARE_EQUAL(HorizontalAlignment::Right, border->HorizontalAlignment);
                border->SetValue(FrameworkElement::HorizontalAlignmentProperty, PropertyValue::CreateUInt32(3u));
                VERIFY_ARE_EQUAL(HorizontalAlignment::Stretch, border->HorizontalAlignment);
                border->SetValue(FrameworkElement::HorizontalAlignmentProperty, PropertyValue::CreateUInt32(0u));
                VERIFY_ARE_EQUAL(HorizontalAlignment::Left, border->HorizontalAlignment);
                border->SetValue(FrameworkElement::HorizontalAlignmentProperty, PropertyValue::CreateInt32(3));
                VERIFY_ARE_EQUAL(HorizontalAlignment::Stretch, border->HorizontalAlignment);
                border->SetValue(FrameworkElement::HorizontalAlignmentProperty, PropertyValue::CreateInt32(1));
                VERIFY_ARE_EQUAL(HorizontalAlignment::Center, border->HorizontalAlignment);
                border->SetValue(FrameworkElement::HorizontalAlignmentProperty, PropertyValue::CreateInt32(0));
                VERIFY_ARE_EQUAL(HorizontalAlignment::Left, border->HorizontalAlignment);
            });
        }

} } } } } }
