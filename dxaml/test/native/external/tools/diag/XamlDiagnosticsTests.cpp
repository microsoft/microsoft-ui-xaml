// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "XamlDiagnosticsTests.h"
#include "XamlDiagnosticsTestHelpers.h"
#include <CustomUserControl.h>
#include "CustomTypes.XamlTypeInfo.g.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include "FileLoader.h"
#include <TestEvent.h>
#include "SafeEventRegistration.h"
#include "MainPage.xaml.h"
#include <CustomMetadataRegistrar.h>
#include <windows.foundation.numerics.h>
#include "PopupPage.xaml.h"
#include "RuleTesterHelper.h"
#include <ThemeHelper.h>

#include <experimental/resumable>
#include <pplawait.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Data;
using namespace ::Tests::Tools::XamlDiagnostics;
using namespace test_infra;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;
using namespace ::Windows::UI::WindowManagement;

namespace shared_types = ::Tests::Tools::Shared;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools {
    namespace XamlDiagnostics {

        #pragma region Test Methods

        bool XamlDiagnosticsTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool XamlDiagnosticsTests::ClassCleanup()
        {
            return true;
        }

        bool XamlDiagnosticsTests::TestSetup()
        {
            TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider(), ref new CustomMetadataRegistrar<shared_types::CustomUserControl>());
            return EnsureTapLoaded();
        }

        bool XamlDiagnosticsTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            test_infra::TestServices::WindowHelper->VerifyTestCleanup();

            return true;
        }

        void XamlDiagnosticsTests::TestGetEnums()
        {
            auto cleanup = XamlDiagnosticsTestHelpers::SetupGridAndWait();

            unsigned int enumCount = 0;
            CoTaskMemPtr<EnumType> spEnums;

            VERIFY_SUCCEEDED(m_tap->GetEnums(&enumCount, &spEnums));
            VERIFY_IS_GREATER_THAN(enumCount, 0u);

            for (size_t k = 0; k < enumCount; k++)
            {
                if (wcscmp(spEnums[k].Name, L"Windows.System.VirtualKey") == 0)
                {
                    VERIFY_IS_GREATER_THAN(static_cast<long>(spEnums[k].ValueInts->rgsabound[0].cElements), 0);
                    break;
                }
            }

            for (size_t k = 0; k < enumCount; k++)
            {
                SafeArrayDestroy(spEnums[k].ValueInts);
                SafeArrayDestroy(spEnums[k].ValueStrings);
            }
        }

        void XamlDiagnosticsTests::TestCreateInstance()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            LogThrow_IfFailed(wrl::MakeAndInitialize<VisualTreeServiceCallback>(&callback));
            auto cleanup = XamlDiagnosticsTestHelpers::SetupGridAndWait([&] {
                test_infra::TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
                LogThrow_IfFailed(m_tap->UnadviseVisualTreeChange(callback.Get()));
            });
            VERIFY_SUCCEEDED(m_tap->AdviseVisualTreeChange(callback.Get()));

            auto created = CreateTemporaryInstance(L"Microsoft.UI.Xaml.Controls.Button");
            VERIFY_ARE_NOT_EQUAL(created.Handle, 0);

            wf::IPropertyValue^ propValue;

            created = CreateTemporaryInstance(L"Microsoft.UI.Xaml.HorizontalAlignment", L"Left");
            VERIFY_ARE_NOT_EQUAL(created.Handle, 0);
            propValue = ih_cast<wf::IPropertyValue>(created.Handle);
            VERIFY_NO_THROW(propValue->GetUInt32());
            VERIFY_NO_THROW(propValue->GetInt32());

            // Verify we can create satellite enum types as well
            created = CreateTemporaryInstance(L"Microsoft.UI.Xaml.Controls.ColorSpectrumShape", L"Ring");
            VERIFY_ARE_NOT_EQUAL(created.Handle, 0);
            propValue = ih_cast<wf::IPropertyValue>(created.Handle);
            VERIFY_NO_THROW(propValue->GetUInt32());
            VERIFY_NO_THROW(propValue->GetInt32());


            created = CreateTemporaryInstance(L"Microsoft.UI.Xaml.Controls.PivotHeaderFocusVisualPlacement", L"ItemHeaders");
            VERIFY_ARE_NOT_EQUAL(created.Handle, 0);
            propValue = ih_cast<wf::IPropertyValue>(created.Handle);
            VERIFY_NO_THROW(propValue->GetUInt32());
            VERIFY_NO_THROW(propValue->GetInt32());

            // Note: We can only create this type because it's referenced in MainPage.xaml and so the xaml compiler
            // code gens what we need. If it's not referenced in markup we don't get any of that and can't create enums.
            // This is a problem we have for anything, not just enums.
            created = CreateTemporaryInstance(L"Tests.Tools.Shared.ThoughtProcess", L"MaybeSo");
            VERIFY_ARE_NOT_EQUAL(created.Handle, 0);
            propValue = ih_cast<wf::IPropertyValue>(created.Handle);

            // Platform::Box doesn't implement IPropertyValue.GetInt32 or IPropertyValue.GetUInt32.
            VERIFY_THROWS_WINRT(propValue->GetUInt32(), Platform::NotImplementedException^);
            VERIFY_THROWS_WINRT(propValue->GetInt32(), Platform::NotImplementedException^);

            LOG_OUTPUT(L"Create invalid enum from satellite control and don't crash");
            VERIFY_THROWS(CreateTemporaryInstance(L"Microsoft.UI.Xaml.Controls.PivotHeaderFocusVisualPlacement", L"zzzz"), WEX::Common::Exception);
        }

        void XamlDiagnosticsTests::TestGetSetClearProperty()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::gridString, callback);

            auto yellowRectangle = callback->GetElementByName(L"Verified");

            wil::unique_propertychainvalue width = GetPropertyChainValue(yellowRectangle.Handle, L"ActualWidth", BaseValueSourceDefault);
            double actualWidth = std::stod(width.Value);
            LOG_OUTPUT(L"%s", width.Value);
            VERIFY_IS_TRUE(actualWidth > 29 && actualWidth < 31);

            wil::unique_propertychainvalue fill = GetPropertyChainValue(yellowRectangle.Handle, L"Fill", BaseValueSourceLocal);
            VERIFY_ARE_NOT_EQUAL(std::stoll(fill.Value), 0);

            wil::unique_propertychainsource style = m_tap->GetPropertyChainSource(yellowRectangle.Handle, BaseValueSourceStyle, 3u);
            VERIFY_ARE_NOT_EQUAL(style.Handle, 0);

            InstanceHandle newBrushBlue = CreateInstance(fill.Type, L"Blue");
            InstanceHandle newBrushPurple = CreateInstance(fill.Type, L"Purple");
            VERIFY_ARE_NOT_EQUAL(newBrushBlue, 0);
            VERIFY_ARE_NOT_EQUAL(newBrushPurple, 0);

            VERIFY_SUCCEEDED(TrySetPropertyByIndex(yellowRectangle.Handle, newBrushBlue, fill.Index));
            VERIFY_SUCCEEDED(TrySetPropertyByIndex(style.Handle, newBrushPurple, fill.Index));

            VERIFY_SUCCEEDED(TryClearPropertyByIndex(yellowRectangle.Handle, fill.Index));
        }

        void XamlDiagnosticsTests::TestSetPropertyBinding()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto content = ref new Platform::String(
            L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
            L"    <StackPanel x:Name='stack'>"
            L"        <VisualStateManager.VisualStateGroups>"
            L"            <VisualStateGroup x:Name='Group1'>"
            L"                <VisualState x:Name='State1'>"
            L"                    <VisualState.Setters>"
            L"                        <Setter Target='Button1.Width' Value='10' />"
            L"                    </VisualState.Setters>"
            L"                </VisualState>"
            L"            </VisualStateGroup>"
            L"        </VisualStateManager.VisualStateGroups>"
            L"        <Button x:Name='WidthControl' Width='100' />"
            L"        <Button x:Name='Button1' Width='50' />"
            L"        <Rectangle x:Name='Verified' Fill='Yellow' />"
            L"        <Rectangle x:Name='redRect' Fill='Red' />"
            L"    </StackPanel>"
            L"</UserControl>");

            auto cleanup = m_connectionHelper->Advise(content, callback);

            auto redRectangle = callback->GetElementByName(L"redRect");

            //Create the new Binding.
            auto newBindingHandle = CreateBinding(L"Verified", L"Fill");

            SetProperty(redRectangle.Handle, newBindingHandle, L"Microsoft.UI.Xaml.Shapes.Shape.Fill");
            //Verify the new Binding in Fill has the correct metadata.
            wil::unique_propertychainvalue fill = GetPropertyChainValue(redRectangle.Handle, L"Fill", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(fill.MetadataBits & MetadataBit::IsValueBindingExpression,  MetadataBit::IsValueBindingExpression);

            auto evaluatedValue = GetPropertyChainValue(std::stoll(fill.Value), L"EvaluatedValue", BaseValueSourceLocal);

            wil::unique_propertychainvalue color = GetPropertyChainValue(std::stoll(evaluatedValue.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(L"Yellow", color.Value) == 0);

            // Now update the VSM setter to use a binding and go to that state
            auto stack = callback->GetElementByName(L"stack");
            auto widthBinding = CreateBinding(L"WidthControl", L"Width");
            auto group = GetNamedVisualStateGroupForElement(stack.Handle, L"Group1");
            auto state = GetNamedVisualStateInGroup(group, L"State1");

            auto setters = GetCollectionProperty(state, L"Microsoft.UI.Xaml.VisualState.Setters");
            auto setter = ih_cast(setters.Elements[0].Value);

            SetProperty(setter, widthBinding, L"Microsoft.UI.Xaml.Setter.Value");

            auto root = callback->GetElementByName(L"root");
            auto rootControl = ih_cast<xaml_controls::UserControl>(root.Handle);
            RunOnUIThread([&]() {
                VisualStateManager::GoToState(rootControl, Platform::StringReference(L"State1"), false);
            });

            TestServices::WindowHelper->WaitForIdle();

            auto button1 = callback->GetElementByName(L"Button1");
            VERIFY_ARE_EQUAL(100, (int)ih_cast<Platform::IBox<double>>(GetProperty(button1.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Width"))->Value);
        }

        void XamlDiagnosticsTests::TestCanSetAutoOnWidthProperty()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::gridString, callback);

            auto rect = callback->GetElementByName(L"Verified");

            wil::unique_propertychainvalue width = GetPropertyChainValue(rect.Handle, L"Width", BaseValueSourceLocal);
            VERIFY_IS_NOT_NULL(width.Value);

            InstanceHandle newDouble = CreateInstance(width.Type, L"Auto");
            VERIFY_ARE_NOT_EQUAL(newDouble, 0);
            VERIFY_SUCCEEDED(TrySetPropertyByIndex(rect.Handle, newDouble, width.Index));
        }

        void XamlDiagnosticsTests::TestGetComponents()
        {
            auto cleanup = XamlDiagnosticsTestHelpers::SetupGridAndWait();

            wrl::ComPtr<IInspectable> spDispatcher;
            wrl::ComPtr<IInspectable> spUiLayer;
            wrl::ComPtr<IInspectable> spApplication;

            VERIFY_SUCCEEDED(m_tap->GetDispatcher(&spDispatcher));
            VERIFY_ARE_NOT_EQUAL(spDispatcher.Get(), nullptr);

            RunOnUIThread([&] {
                VERIFY_SUCCEEDED(m_tap->GetUiLayer(&spUiLayer));
            });

            VERIFY_ARE_NOT_EQUAL(spUiLayer.Get(), nullptr);

            VERIFY_SUCCEEDED(m_tap->GetApplication(&spApplication));
            VERIFY_ARE_NOT_EQUAL(spApplication.Get(), nullptr);
        }

        void XamlDiagnosticsTests::TestGetHandlesAndIInspectables()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::gridString, callback);

            auto yellowRectangle = callback->GetElementByName(L"Verified");
            wrl::ComPtr<IInspectable> spYellowRectangle;
            InstanceHandle verifiedYellowRectangle = 0;

            VERIFY_SUCCEEDED(m_tap->GetIInspectableFromHandle(yellowRectangle.Handle, &spYellowRectangle));
            VERIFY_ARE_NOT_EQUAL(spYellowRectangle.Get(), nullptr);

            VERIFY_SUCCEEDED(m_tap->GetHandleFromIInspectable(spYellowRectangle.Get(), &verifiedYellowRectangle));
            VERIFY_ARE_EQUAL(yellowRectangle.Handle, verifiedYellowRectangle);
        }

        void XamlDiagnosticsTests::TestHitTest()
        {
            // We need to reset the window content here so this test is reliable. The tree is in a weird state where there
            // is an extra rectangle in the tree if this test is the first test to run. If run after a test, the previous
            // test would've cleared the window content before this one started, so we are making sure this always happens.
            test_infra::TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();

            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::gridString, callback);

            RECT rect = { 0 };

            VERIFY_IS_TRUE(!!GetWindowRect(GetForegroundWindow(), &rect));

            // TODO: this should hit test everything. just get size of cache?
            auto handles = DoHitTest(rect);
            VERIFY_ARE_EQUAL(handles.size(), 13u);

            for (auto handle : handles)
            {
                VisualElement element = callback->GetElementByHandle(handle);
                LOG_OUTPUT(L"   *HIT TESTED: %s:%s", element.Type, element.Name);
            }
        }

        void XamlDiagnosticsTests::TestHitTestReturnsInvisibleElements()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::gridWithInvisibleElementsString, callback);

            RECT rect = { 0 };

            VERIFY_IS_TRUE(!!GetWindowRect(GetForegroundWindow(), &rect));

            // TODO: this should hit test everything. just get size of cache?
            auto handles = DoHitTest(rect);
            VERIFY_ARE_EQUAL(handles.size(), 3u);

            // Let's make sure we actually found the elements we were looking for
            auto hitTestInvisible = callback->GetElementByName(L"hitTestInvisible");
            auto zeroOpacity = callback->GetElementByName(L"zeroOpacity");
            bool foundHitTestInvisible = false;
            bool foundZeroOpacity = false;

            for (auto handle : handles)
            {
                foundHitTestInvisible = foundHitTestInvisible || handle == hitTestInvisible.Handle;
                foundZeroOpacity = foundZeroOpacity || handle == zeroOpacity.Handle;
            }

            VERIFY_IS_TRUE(foundHitTestInvisible);
            VERIFY_IS_TRUE(foundZeroOpacity);
        }

        void XamlDiagnosticsTests::TestHitTestReturnsDisabledElements()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::gridWithDisabledElementsString, callback);

            RECT rect = { 0 };

            VERIFY_IS_TRUE(!!GetWindowRect(GetForegroundWindow(), &rect));

            auto handles = DoHitTest(rect);
            VERIFY_ARE_EQUAL(handles.size(), 4u);

            // Let's make sure we actually found the elements we were looking for
            auto disabled = callback->GetElementByName(L"disabled");
            bool foundDisabled = false;

            for (auto handle : handles)
            {
                foundDisabled = foundDisabled || handle == disabled.Handle;
            }

            VERIFY_IS_TRUE(foundDisabled);
        }

        void XamlDiagnosticsTests::TestHitTestDoesntReturnCollapsedElements()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::relativePanelWithCollapsedElementsString, callback);

            RECT rect = { 0 };

            VERIFY_IS_TRUE(!!GetWindowRect(GetForegroundWindow(), &rect));

            auto collapsed = callback->GetElementByName(L"collapsed");
            auto visible1 = callback->GetElementByName(L"visible1");
            auto visible2 = callback->GetElementByName(L"visible2");
            auto visible3 = callback->GetElementByName(L"visible3");

            auto handles = DoHitTest(rect);
            VERIFY_ARE_EQUAL(handles.size(), 4u);

            // Make sure we don't find the collapsed element in the returned values
            // and that we do find the visible one.
            bool foundVisible1 = false;
            bool foundVisible2 = false;
            bool foundVisible3 = false;
            for (auto handle : handles)
            {
                VERIFY_ARE_NOT_EQUAL(handle, collapsed.Handle);
                foundVisible1 = foundVisible1 || handle == visible1.Handle;
                foundVisible2 = foundVisible2 || handle == visible2.Handle;
                foundVisible3 = foundVisible3 || handle == visible3.Handle;
            }

            VERIFY_IS_TRUE(foundVisible1);
            VERIFY_IS_TRUE(foundVisible2);
            VERIFY_IS_TRUE(foundVisible3);
        }

        void XamlDiagnosticsTests::TestHitTestAfterChangingVisiblity()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::relativePanelWithCollapsedElementsString, callback);

            RECT rect = { 0 };

            VERIFY_IS_TRUE(!!GetWindowRect(GetForegroundWindow(), &rect));

            auto handles = DoHitTest(rect);
            VERIFY_ARE_EQUAL(handles.size(), 4u);

            // Let's verify that the collapsed element doesn't effect the layout of the app even though it is
            // between visible1 and visible2 in the markup.
            auto collapsed = callback->GetElementByName(L"collapsed");
            auto collapsedRenderSize = GetPropertyChainValue(collapsed.Handle, L"RenderSize");
            VERIFY_IS_TRUE(wcscmp(collapsedRenderSize.Value, L"0x0") == 0);

            // Now let's change the visibility
            auto collapsedVisibility = GetPropertyChainValue(collapsed.Handle, L"Visibility", BaseValueSourceLocal);
            InstanceHandle visibility = CreateInstance(collapsedVisibility.Type, L"Visible");

            VERIFY_SUCCEEDED(TrySetPropertyByIndex(collapsed.Handle, visibility, collapsedVisibility.Index));
            TestServices::WindowHelper->WaitForIdle();
            collapsedRenderSize = GetPropertyChainValue(collapsed.Handle, L"RenderSize");
            VERIFY_IS_TRUE(wcscmp(collapsedRenderSize.Value, L"30x30") == 0);

            // Hit test again and make sure it was returned
            handles = DoHitTest(rect);
            VERIFY_ARE_EQUAL(handles.size(), 5u);

            bool foundCollapsed = false;
            for (auto handle : handles)
            {
                foundCollapsed = foundCollapsed || handle == collapsed.Handle;
            }
            VERIFY_IS_TRUE(foundCollapsed);
        }

        void XamlDiagnosticsTests::TestHitTestDoesntReturnElementsNotInTree()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = LoadXamlFromFunction(XamlDiagnosticsTestHelpers::SetupGroupedListView, callback);

            RECT rect = { 0 };

            VERIFY_IS_TRUE(!!GetWindowRect(GetForegroundWindow(), &rect));

            auto handles = DoHitTest(rect);

            // Every element that is returned should be in the live visual tree which means
            // that XamlDiagnostics has a reference to the object in its handle map. Let's
            // get that that reference by calling m_tap->GetIInspectableFromHandle. This will fail
            // if the handle returned isn't in the live tree.
            for (auto handle : handles)
            {
                wrl::ComPtr<IInspectable> spInspectable;
                VERIFY_SUCCEEDED(m_tap->GetIInspectableFromHandle(handle, &spInspectable));
            }
        }

        void XamlDiagnosticsTests::TestRegisterInstance()
        {
            auto cleanup = XamlDiagnosticsTestHelpers::SetupGridAndWait();

            InstanceHandle handle = 0;
            xaml_shapes::Rectangle^ rect = nullptr;

            auto extraCleanup = wil::scope_exit([&]{
                if (handle > 0u)
                {
                    m_tap->UnregisterInstance(handle);
                }
            });
            RunOnUIThread([&]()
            {
                rect = ref new xaml_shapes::Rectangle();
            });
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_SUCCEEDED(m_tap->RegisterInstance(reinterpret_cast<IInspectable*>(rect), &handle));

            VERIFY_ARE_NOT_EQUAL(handle, 0);
        }

        // In order to test that unadvise actually works, we need to first get the notifications to the tree updates and then remove the cache of elements our callback has. We should
        // then unadvise and change the tree. We should verify that the cache is still empty, aka we didn't get any callback.
        void XamlDiagnosticsTests::TestUnadviseVisualTreeChange()
        {
            auto cleanup = XamlDiagnosticsTestHelpers::SetupGridAndWait();
            wrl::ComPtr<VisualTreeServiceCallback> callback = m_connectionHelper->Advise();

            // Verify the cache isn't empty
            VERIFY_IS_TRUE(!callback->IsCacheEmpty());

            // Clear the cache and unadvise to any further changes
            callback->ClearCache();
            VERIFY_SUCCEEDED(m_tap->UnadviseVisualTreeChange(callback.Get()));

            auto control = XamlDiagnosticsTestHelpers::SetupGrid();
            Microsoft::UI::Xaml::Tests::Common::RunOnUIThread([&]()
            {
                test_infra::TestServices::WindowHelper->WindowContent = control;
            });

            test_infra::TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_TRUE(callback->IsCacheEmpty());
        }

        void XamlDiagnosticsTests::TestUiLayer()
        {
            auto cleanup = XamlDiagnosticsTestHelpers::SetupGridAndWait();

            RunOnUIThread([&]()
            {
                wrl::ComPtr<IInspectable> spDOAsInsp;
                VERIFY_SUCCEEDED(m_tap->GetUiLayer(spDOAsInsp.GetAddressOf()));
                Grid^ spDO = reinterpret_cast<Grid^>(spDOAsInsp.Get());
                VERIFY_SUCCEEDED(DrawUI(spDO));
            });
        }

        void XamlDiagnosticsTests::TestGetThemeResourceProperties()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;

            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::userControlWithCustomButtonStyleString, callback);
            auto theButton = callback->GetElementByName(L"theButton");

            wil::unique_propertychainvalue borderThicknessProperty = GetPropertyChainValue(theButton.Handle, L"BorderThickness", BaseValueSourceStyle);
            VERIFY_IS_NOT_NULL(borderThicknessProperty.Value);
            double borderThickness = std::stod(borderThicknessProperty.Value);
            LOG_OUTPUT(L"%s", borderThicknessProperty.Value);
            VERIFY_IS_TRUE(borderThickness > 11 && borderThickness < 13);
        }

        void XamlDiagnosticsTests::TestCanSetValueOnCustomProperty()
        {
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/PageWithCustomUserControl.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            auto customUserControl = callback->GetElementByName(L"userControl1");

            wil::unique_propertychainvalue intProperty = GetPropertyChainValue(customUserControl.Handle, L"CustomInt");

            InstanceHandle newInt = CreateInstance(intProperty.Type, L"5");
            VERIFY_ARE_NOT_EQUAL(newInt, 0);
            VERIFY_SUCCEEDED(TrySetPropertyByIndex(customUserControl.Handle, newInt, intProperty.Index));

             //let's get the property value chain again
            wil::unique_propertychainvalue newIntProperty = GetPropertyChainValue(customUserControl.Handle, L"CustomInt", BaseValueSourceLocal);
            int actualInt = std::stoi(newIntProperty.Value);
            VERIFY_ARE_EQUAL(actualInt, 5);
        }

        void XamlDiagnosticsTests::TestProvideSourceForURI()
        {
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/external/foundation/graphics/image/SimpleImageWithUri.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            auto imageElement = callback->GetElementByName(L"imageElement");

            wil::unique_propertychainvalue sourceBitmapImage = GetPropertyChainValue(imageElement.Handle, L"Source", BaseValueSourceLocal);

            // Since the source of a an Image element is a BitmapImage, we need to then get the properties for this guy.
            wil::unique_propertychainvalue uriSourceProperty = GetPropertyChainValue(std::stoll(sourceBitmapImage.Value), L"UriSource");

            // Since we resolve the URI to the string representation rather than keeping a handle to the wf::IUriRuntimeClass object, let's make sure our metadata supports that.
            VERIFY_IS_TRUE((uriSourceProperty.MetadataBits & MetadataBit::IsValueHandle) != MetadataBit::IsValueHandle);
            VERIFY_IS_TRUE(wcscmp(L"ms-appx:///resources/native/external/foundation/graphics/image/Rainier_444_2048x1536.jpg",uriSourceProperty.Value) == 0);
        }

        void XamlDiagnosticsTests::TestCanSetURISource()
        {
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/external/foundation/graphics/image/SimpleImageWithUri.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            auto imageElement = callback->GetElementByName(L"imageElement");

            wil::unique_propertychainvalue sourceBitmapImage = GetPropertyChainValue(imageElement.Handle, L"Source", BaseValueSourceLocal);

            // Since the source of a an Image element is a BitmapImage, we need to then get the properties for this guy.
            wil::unique_propertychainvalue uriSourceProperty = GetPropertyChainValue(std::stoll(sourceBitmapImage.Value), L"UriSource");

            // Since we resolve the URI to the string representation rather than keeping a handle to the wf::IUriRuntimeClass object, let's make sure our metadata supports that.
            VERIFY_IS_TRUE((uriSourceProperty.MetadataBits & MetadataBit::IsValueHandle) != MetadataBit::IsValueHandle);
            VERIFY_IS_TRUE(wcscmp(L"ms-appx:///resources/native/external/foundation/graphics/image/Rainier_444_2048x1536.jpg", uriSourceProperty.Value) == 0);

            // Now let's create an instance of a URI and set it's source.
            InstanceHandle newUri = CreateInstance(uriSourceProperty.Type, L"ms-appx:///resources/native/external/foundation/graphics/image/barcelona.jpg");

            VERIFY_ARE_NOT_EQUAL(newUri, 0);

            VERIFY_SUCCEEDED(TrySetPropertyByIndex(std::stoll(sourceBitmapImage.Value), newUri, uriSourceProperty.Index));

            wil::unique_propertychainvalue newUriSourceProperty = GetPropertyChainValue(std::stoll(sourceBitmapImage.Value), L"UriSource", BaseValueSourceLocal);
            VERIFY_IS_NOT_NULL(newUriSourceProperty.Value);
            // Since we resolve the URI to the string representation rather than keeping a handle to the wf::IUriRuntimeClass object, let's make sure our metadata supports that.
            VERIFY_IS_TRUE((newUriSourceProperty.MetadataBits & MetadataBit::IsValueHandle) != MetadataBit::IsValueHandle);
            VERIFY_IS_TRUE(wcscmp(L"ms-appx:///resources/native/external/foundation/graphics/image/barcelona.jpg", newUriSourceProperty.Value) == 0);
        }

        void XamlDiagnosticsTests::TestProvideSourceForFontFamily()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::gridString, callback);

            auto textBlock = callback->GetElementByName(L"texty");

            wil::unique_propertychainvalue fontFamilyProperty = GetPropertyChainValue(textBlock.Handle, L"FontFamily", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(L"Segoe UI", fontFamilyProperty.Value) == 0);
        }

        void XamlDiagnosticsTests::TestCanSetSourceForFontFamily()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::gridString, callback);

            auto textBlock = callback->GetElementByName(L"texty");

            wil::unique_propertychainvalue fontFamilyProperty = GetPropertyChainValue(textBlock.Handle, L"FontFamily", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(L"Segoe UI", fontFamilyProperty.Value) == 0);

            // Now let's create an instance of FontFamily and set it's source.
            InstanceHandle fontFamily = CreateInstance(fontFamilyProperty.Type, L"Arial");

            VERIFY_ARE_NOT_EQUAL(fontFamily, 0);

            VERIFY_SUCCEEDED(TrySetPropertyByIndex(textBlock.Handle, fontFamily, fontFamilyProperty.Index));

            wil::unique_propertychainvalue newFontFamilyProperty = GetPropertyChainValue(textBlock.Handle, L"FontFamily", BaseValueSourceLocal);

            // Since we resolve the FontFamily to the string representation rather than keeping a handle to the FontFamily object, let's make sure our metadata supports that.
            VERIFY_IS_TRUE((newFontFamilyProperty.MetadataBits & MetadataBit::IsValueHandle) != MetadataBit::IsValueHandle);
            VERIFY_IS_TRUE(wcscmp(L"Arial", newFontFamilyProperty.Value) == 0);
        }

        void XamlDiagnosticsTests::TestBackgroundOnGrid()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::gridString, callback);

            auto grid = callback->GetElementByName(L"root");

            wil::unique_propertychainvalue background = GetPropertyChainValue(grid.Handle, L"Background", BaseValueSourceLocal);

            // Verify correct metadata
            VERIFY_IS_TRUE((background.MetadataBits & MetadataBit::IsValueHandle) == MetadataBit::IsValueHandle);

            wil::unique_propertychainvalue color = GetPropertyChainValue(std::stoll(background.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(L"Red", color.Value) == 0);
        }

        void XamlDiagnosticsTests::TestDoubleAndWidthHaveDifferentValues()
        {
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/PageWithCustomUserControl.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            auto customUserControl = callback->GetElementByName(L"userControl1");

            wil::unique_propertychainvalue doubleProperty = GetPropertyChainValue(customUserControl.Handle, L"CustomDouble", BaseValueSourceLocal);
            wil::unique_propertychainvalue widthProperty = GetPropertyChainValue(customUserControl.Handle, L"Width", BaseValueSourceLocal);

            VERIFY_IS_TRUE(wcscmp(doubleProperty.Value, L"-1.#IND") == 0);
            VERIFY_IS_TRUE(wcscmp(widthProperty.Value, L"Auto") == 0);
        }

        void XamlDiagnosticsTests::TestCorrectBaseValueSourceOnRowAndColumnDefinitions()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::gridString, callback);

            auto rootGrid = callback->GetElementByName(L"root");

            wil::unique_propertychainsource rowDefSource = m_tap->GetSourceForProperty(rootGrid.Handle, L"RowDefinitions", BaseValueSourceLocal);
            wil::unique_propertychainsource colDefSource = m_tap->GetSourceForProperty(rootGrid.Handle, L"ColumnDefinitions", BaseValueSourceLocal);

            VERIFY_ARE_NOT_EQUAL(colDefSource.Handle, 0);
            VERIFY_ARE_NOT_EQUAL(rowDefSource.Handle, 0);
        }

        void XamlDiagnosticsTests::TestAutoOnSetters()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::userControlWithCustomButtonStyleString, callback);

            // We need to go to the correct visual state in order for the value to take effect.
            RunOnUIThread([&]()
            {
                auto root = safe_cast<UserControl^>(TestServices::WindowHelper->WindowContent);
                VERIFY_IS_TRUE(VisualStateManager::GoToState(root, "VisualState", false));
            });

            TestServices::WindowHelper->WaitForIdle();

            auto button = callback->GetElementByName(L"theButton");
            wil::unique_propertychainvalue buttonWidth = GetPropertyChainValue(button.Handle, L"Width", BaseValueSourceStyle);
            wil::unique_propertychainvalue buttonHeight = GetPropertyChainValue(button.Handle, L"Height", BaseValueSource::Animation);

            VERIFY_IS_TRUE(wcscmp(buttonWidth.Value, L"Auto") == 0);
            VERIFY_IS_TRUE(wcscmp(buttonHeight.Value, L"Auto") == 0);
        }

        void XamlDiagnosticsTests::TestSetEnum()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;

            auto text = ref new Platform::String(L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Background='Red' Projection='{x:Null}' Margin='1,1,2,3' Height='45' Width='45'>"
            L"  <Grid.ColumnDefinitions>"
            L"      <ColumnDefinition Width='*'/>"
            L"      <ColumnDefinition Width='Auto'/>"
            L"  </Grid.ColumnDefinitions>"
            L"  <Grid.RowDefinitions>"
            L"      <RowDefinition Height='*'/>"
            L"      <RowDefinition Height='Auto'/>"
            L"  </Grid.RowDefinitions>"
            L"  <Grid.Resources>"
            L"      <Style x:Key='rectangleStyle' TargetType='Rectangle'>"
            L"          <Setter Property='Fill' Value='Green' />"
            L"      </Style>"
            L"  </Grid.Resources>"
            L"  <Rectangle x:Name='Verified' Grid.Row='1' Grid.Column='1' Style='{StaticResource rectangleStyle}' Fill='Yellow' Height='30' Width='30' />"
            L"</Grid>");

            auto cleanup = m_connectionHelper->Advise(text, callback);

            auto rect = callback->GetElementByName(L"Verified");

            auto newHorizAlign = CreateInstance(L"Microsoft.UI.Xaml.HorizontalAlignment", L"Right");
            VERIFY_ARE_NOT_EQUAL(newHorizAlign, 0);

            SetProperty(rect.Handle, newHorizAlign, L"Microsoft.UI.Xaml.FrameworkElement.HorizontalAlignment");

            wil::unique_propertychainvalue setHorizontalAlignment = GetPropertyChainValue(rect.Handle, L"HorizontalAlignment", BaseValueSourceLocal);
            auto convertedEnum = m_tap->ConvertEnumValue(setHorizontalAlignment.Type, std::stoi(setHorizontalAlignment.Value));
            VERIFY_IS_TRUE(wcscmp(convertedEnum.c_str(), L"Right") == 0);
        }

        void XamlDiagnosticsTests::TestCreateColorAndSetToBrushProperty()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::gridString, callback);

            auto rect = callback->GetElementByName(L"Verified");
            wil::unique_propertychainvalue fillProperty = GetPropertyChainValue(rect.Handle, L"Fill", BaseValueSourceLocal);

            // The color is set locally to Yellow, let's make sure that is true.
            wil::unique_propertychainvalue colorProperty = GetPropertyChainValue(std::stoll(fillProperty.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"Yellow") == 0);

            InstanceHandle newColor = CreateInstance(colorProperty.Type, L"Blue");
            VERIFY_ARE_NOT_EQUAL(newColor, 0);

            VERIFY_SUCCEEDED(TrySetPropertyByIndex(std::stoll(fillProperty.Value), newColor, colorProperty.Index));

            wil::unique_propertychainvalue setColor = GetPropertyChainValue(std::stoll(fillProperty.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(setColor.Value, L"Blue") == 0);
        }

        void XamlDiagnosticsTests::TestReturnCorrectRootsInUAP()
        {
            TestReturnCorrectRootsHelper(3 /*numberOfRoots*/);
        }

        void XamlDiagnosticsTests::TestReturnCorrectRootsInWPF()
        {
            TestReturnCorrectRootsHelper(2 /*numberOfRoots*/);
        }

        void XamlDiagnosticsTests::TestReturnCorrectRootsHelper(unsigned numberOfRoots)
        {
            // Since we always create the callback before the roots are in the tree.
            TestCleanupWrapper cleanup;

            xaml_controls::Grid^ root;
            RunOnUIThread([&]()
            {
                root = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                    L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"</Grid>"));
            });

            auto callback = m_connectionHelper->Advise();
            auto scopeGuard = wil::scope_exit([&]
            {
                VERIFY_SUCCEEDED(m_tap->UnadviseVisualTreeChange(callback.Get()));
            });

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = root;
            });

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Verifying correct number of roots returned");

            // When we run in a mode where multiple windows are supported, we give the window as the top level root.
            auto roots = callback->GetRoots();
            VERIFY_ARE_EQUAL(roots.size(), 1u);

            // We'll verify three roots are the child of the window since we aren't running in a background task and there is no RenderTargetBitmapRoot:
            //  1. RootScrollViewer
            //  2. PopupRoot
            //  3. FullWindowMediaRoot --(not under DesktopWindowXAMLSource)

            auto children = callback->GetChildren(roots.at(0));
            VERIFY_ARE_EQUAL(children.size(), numberOfRoots);

            LOG_OUTPUT(L"Verifying VisualDiagnosticsRoot isn't in cache");
            wrl::ComPtr<IInspectable> spVisualDiagRoot;

            InstanceHandle diagRootHandle = 0;
            RunOnUIThread([&]()
            {
                VERIFY_SUCCEEDED(m_tap->GetUiLayer(spVisualDiagRoot.GetAddressOf()));
                VERIFY_SUCCEEDED(m_tap->GetHandleFromIInspectable(spVisualDiagRoot.Get(), &diagRootHandle));
            });

            auto unregister = wil::scope_exit([&] {
                if (diagRootHandle > 0u)
                {
                    m_tap->UnregisterInstance(diagRootHandle);
                }
            });
            auto iter = std::find(children.begin(), children.end(), diagRootHandle);

            VERIFY_IS_TRUE(iter == children.end());
        }

        void XamlDiagnosticsTests::TestMultipleCallbacks()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback1 = m_connectionHelper->Advise();
            wrl::ComPtr<VisualTreeServiceCallback> callback2;
            LogThrow_IfFailed(wrl::MakeAndInitialize<VisualTreeServiceCallback>(&callback2));

            // We don't support multiple callbacks, make sure this doesn't work
            VERIFY_FAILED(m_tap->AdviseVisualTreeChange(callback2.Get()));

            // Make sure trying to advise didn't throw us in some weird state
            auto cleanup = XamlDiagnosticsTestHelpers::SetupGridAndWait();

            // tear down the tree before we unadvise.
            test_infra::TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();

            auto cleanupResults = callback1->VerifyTestCleanup();
            WEX::Common::Throw::IfFalse(cleanupResults.second, E_UNEXPECTED, L"The cache should be empty after tearing down the tree");

            VERIFY_SUCCEEDED(m_tap->UnadviseVisualTreeChange(callback1.Get()));
        }

        void XamlDiagnosticsTests::TestDataBoundProperties()
        {
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/PageWithCustomUserControl.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            // We bound the text on the textblock to the CustomDouble property on the customUserControl. Let's make sure that is
            // reflected.
            auto customUserControl = callback->GetElementByName(L"userControl1");
            auto textBlock = callback->GetElementByName(L"dataBound");

            wil::unique_propertychainvalue customInt = GetPropertyChainValue(customUserControl.Handle, L"CustomInt", BaseValueSourceLocal);
            wil::unique_propertychainvalue text = GetPropertyChainValue(textBlock.Handle, L"Text", BaseValueSourceLocal);

            // Since the text is data bound and the value represents a handle, we should verify the metadata supports that.
            VERIFY_IS_TRUE((text.MetadataBits & MetadataBit::IsValueBindingExpression) == MetadataBit::IsValueBindingExpression);

            auto evaluatedValue = GetPropertyChainValue(std::stoll(text.Value), L"EvaluatedValue", BaseValueSourceLocal);
            VERIFY_IS_TRUE(evaluatedValue.MetadataBits == MetadataBit::None);
            VERIFY_IS_TRUE(wcscmp(customInt.Value, evaluatedValue.Value) == 0);

            auto isValid = GetPropertyChainValue(std::stoll(text.Value), L"IsBindingValid", BaseValueSourceLocal);
            VERIFY_IS_TRUE(isValid.MetadataBits == MetadataBit::None);
            VERIFY_IS_TRUE(wcscmp(isValid.Value, L"True") == 0);
        }

        void XamlDiagnosticsTests::TestGetHandleToOwnedObject()
        {
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/PageWithNonDOBinding.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            Model^ dataSource = nullptr;
            RunOnUIThread([&]()
            {
                auto page= safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                dataSource = ref new Model();
                dataSource->ReadWrite = "WriteThis";  // change this property to verify that it really is the same data context
                page->DataContext = dataSource;
            });

            TestServices::WindowHelper->WaitForIdle();

            auto page = callback->GetElementByName(L"rootPage");

            wil::unique_propertychainvalue dataContextProperty = GetPropertyChainValue(page.Handle, L"DataContext", BaseValueSourceLocal);
            wrl::ComPtr<IInspectable> modelAsInsp;
            VERIFY_SUCCEEDED(m_tap->GetIInspectableFromHandle(std::stoll(dataContextProperty.Value), &modelAsInsp));

            RunOnUIThread([&]()
            {
                auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                Model^ inspectableAsModel = reinterpret_cast<Model^>(modelAsInsp.Get());

                // Verify that the changed ReadWrite properties match
                VERIFY_ARE_EQUAL(inspectableAsModel->ReadWrite, dataSource->ReadWrite);

                // Clear DataContext so we don't leak (not XamlDiagnostics related).
                // See tests like BindingIntegrationTests::CanSetDataContext, they do the same thing.
                page->ClearValue(Page::DataContextProperty);
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void XamlDiagnosticsTests::TestGetCalendarDatePickerDefault()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::gridString, callback);

            // Create a CalendarDatePicker and place in root grid
            InstanceHandle cdpHandle = CreateInstance(L"Microsoft.UI.Xaml.Controls.CalendarDatePicker");

            // Get the grid and do some sanity checking
            VisualElement rootGrid = callback->GetElementByName(L"root");
            wil::unique_propertychainvalue children = GetPropertyChainValue(rootGrid.Handle, L"Children");

            VERIFY_ARE_EQUAL(children.MetadataBits & MetadataBit::IsValueCollection, MetadataBit::IsValueCollection);
            VERIFY_ARE_EQUAL(children.MetadataBits & MetadataBit::IsValueHandle, MetadataBit::IsValueHandle);

            // Add date picker to the grid and wait until it has been put in the tree.
            VERIFY_SUCCEEDED(m_tap->AddChild(std::stoll(children.Value), cdpHandle, 0));

            TestServices::WindowHelper->WaitForIdle();

            // Get default date for the date picker.
            wil::unique_propertychainvalue date = GetPropertyChainValue(cdpHandle, L"Date");
            VERIFY_IS_TRUE(wcscmp(date.Value, L"0") == 0);

            // Create a DateTime instance using CX since this isn't supported in the framework yet

            ::Windows::Globalization::Calendar^ calendar = ref new ::Windows::Globalization::Calendar();
            calendar->Year = 1991;
            calendar->Month = 04;
            calendar->Day = 17;

            wrl::ComPtr<IInspectable> spCalendarDatePicker;
            VERIFY_SUCCEEDED(m_tap->GetIInspectableFromHandle(cdpHandle, &spCalendarDatePicker));

            RunOnUIThread([&]()
            {
                reinterpret_cast<xaml_controls::CalendarDatePicker^>(spCalendarDatePicker.Get())->Date = calendar->GetDateTime();
            });
            TestServices::WindowHelper->WaitForIdle();

            wil::unique_propertychainvalue defaultDate = GetPropertyChainValue(cdpHandle, L"Date", BaseValueSourceDefault);
            VERIFY_IS_TRUE(wcscmp(defaultDate.Value, L"0") == 0);

            // also make sure the value was set correctly
            wil::unique_propertychainvalue localDate = GetPropertyChainValue(cdpHandle, L"Date", BaseValueSourceLocal);
            VERIFY_ARE_NOT_EQUAL(localDate.MetadataBits & MetadataBit::IsValueHandle, MetadataBit::IsValueHandle);

            VERIFY_ARE_EQUAL(calendar->GetDateTime().UniversalTime, std::stoll(localDate.Value));
        }

        void XamlDiagnosticsTests::TestGetUnnamedProperties()
        {
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/PageWithCustomUserControl.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            auto userControl = callback->GetElementByName(L"userControl1");

            wil::unique_propertychainvalue unknownProperty = GetPropertyChainValue(userControl.Handle, L"(unnamed)", BaseValueSourceDefault);

            VERIFY_ARE_EQUAL(std::wstring(unknownProperty.ValueType), std::wstring(L"Windows.Foundation.Object"));
            VERIFY_ARE_EQUAL(std::wstring(unknownProperty.DeclaringType), std::wstring(L"Windows.Foundation.Object"));
            VERIFY_ARE_EQUAL(std::wstring(unknownProperty.Type),  std::wstring(L"Windows.Foundation.Object"));

            // When compiling to xbf, the value is stored as a string that is null, rather than just being null. This isn't necessarily
            // something we care much about, but it's a subtle difference
            unknownProperty = GetPropertyChainValue(userControl.Handle, L"(unnamed)", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(std::wstring(unknownProperty.ValueType), std::wstring(L"Windows.Foundation.String"));
            VERIFY_ARE_EQUAL(std::wstring(unknownProperty.DeclaringType), std::wstring(L"Windows.Foundation.Object"));
            VERIFY_ARE_EQUAL(std::wstring(unknownProperty.Type),  std::wstring(L"Windows.Foundation.Object"));
        }

        void XamlDiagnosticsTests::TestGetCalendarViewProperties()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::gridString, callback);

            // Create a CalendarDatePicker and place in root grid
            InstanceHandle cvHandle = CreateInstance(L"Microsoft.UI.Xaml.Controls.CalendarView");

            // Get the grid and do some sanity checking
            VisualElement rootGrid = callback->GetElementByName(L"root");
            wil::unique_propertychainvalue children = GetPropertyChainValue(rootGrid.Handle, L"Children");

            VERIFY_ARE_EQUAL(children.MetadataBits & MetadataBit::IsValueCollection, MetadataBit::IsValueCollection);
            VERIFY_ARE_EQUAL(children.MetadataBits & MetadataBit::IsValueHandle, MetadataBit::IsValueHandle);

            // Add date picker to the grid and wait until it has been put in the tree.
            VERIFY_SUCCEEDED(m_tap->AddChild(std::stoll(children.Value), cvHandle, 0));

            TestServices::WindowHelper->WaitForIdle();

            // Get default date for the date picker.
            wil::unique_propertychainvalue selectedDates = GetPropertyChainValue(cvHandle, L"SelectedDates");

            // The property is normally a collection, but this time it's null.  When a collection is null, we shouldn't set the collection bit.
            // Verify the metadata is correct.
            VERIFY_ARE_NOT_EQUAL(selectedDates.MetadataBits & MetadataBit::IsValueCollection, MetadataBit::IsValueCollection);
            VERIFY_ARE_EQUAL(selectedDates.MetadataBits & MetadataBit::IsValueNull, MetadataBit::IsValueNull);

            // Verify that the type is empty. This is the main purpose of this test as these sort of properties were failing.
            // If the metadata of this property changes in the future, the test will need to be refactored to use a property
            // that has no type name.
            VERIFY_IS_TRUE(wcscmp(selectedDates.Type, L"") == 0);
        }

        void XamlDiagnosticsTests::TestCreateInstanceOfCustomType()
        {
            TestCleanupWrapper cleanup;

            // Create a CustomUserControl
            InstanceHandle customHandle = 0;
            {
                auto tempInstance = CreateTemporaryInstance(L"Tests.Tools.Shared.CustomUserControl");
                VERIFY_ARE_NOT_EQUAL(tempInstance.Handle, 0u);
                customHandle = tempInstance.Handle;
            }
            wrl::ComPtr<IInspectable> customUserControl;
            VERIFY_ARE_EQUAL(E_NOTFOUND, m_tap->GetIInspectableFromHandle(customHandle, &customUserControl));
        }

        void XamlDiagnosticsTests::TestGetPropertyValueAndIndex()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::gridString, callback);

            // Get the rectangle Fill property, ensure the property index from its wil::unique_propertychainvalue
            // and GetPropertyIndex match.
            VisualElement rect = callback->GetElementByName(L"childVerified");
            wil::unique_propertychainvalue rectFill = GetPropertyChainValue(rect.Handle, L"Fill");
            unsigned int indexPropertyChain = rectFill.Index;
            unsigned int indexGetPropertyIndex = 0;
            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(rect.Handle, L"Microsoft.UI.Xaml.Shapes.Shape.Fill", &indexGetPropertyIndex));
            VERIFY_ARE_NOT_EQUAL(indexGetPropertyIndex,0u);
            VERIFY_ARE_EQUAL(indexPropertyChain, indexGetPropertyIndex);

            // Fetch the property value of Fill with GetLocalProperty, and ensure the property is correct
            // by checking the returned value is a SolidColorBrush.
            wrl::ComPtr<IInspectable> rectFillVal;
            InstanceHandle rectFillValHandle;
            VERIFY_SUCCEEDED(m_tap->GetProperty(rect.Handle, indexPropertyChain, &rectFillValHandle));
            VERIFY_SUCCEEDED(m_tap->GetIInspectableFromHandle(rectFillValHandle, &rectFillVal));

            auto test = safe_cast<SolidColorBrush^>(reinterpret_cast<Platform::Object^>(rectFillVal.Get()));
            VERIFY_IS_NOT_NULL(test);
        }

        void XamlDiagnosticsTests::TestGetPropertyIndexAttached()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::gridString, callback);

            VisualElement rect = callback->GetElementByName(L"childVerified");
            wil::unique_propertychainvalue gridRow = GetPropertyChainValue(rect.Handle, L"Grid.Row", BaseValueSourceLocal);
            unsigned int indexPropertyChain = gridRow.Index;
            unsigned int indexGetPropertyIndex = 0;

            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(rect.Handle, L"Microsoft.UI.Xaml.Controls.Grid.Row", &indexGetPropertyIndex));
            VERIFY_ARE_NOT_EQUAL(indexGetPropertyIndex,0u);
            VERIFY_ARE_EQUAL(indexPropertyChain, indexGetPropertyIndex);

            //Get the value of the Grid.Row attached property, which should be 1.
            wrl::ComPtr<IInspectable> gridRowVal;
            InstanceHandle gridRowValHandle;
            VERIFY_SUCCEEDED(m_tap->GetProperty(rect.Handle, indexPropertyChain, &gridRowValHandle));
            VERIFY_SUCCEEDED(m_tap->GetIInspectableFromHandle(gridRowValHandle, &gridRowVal));

            auto test = safe_cast<IPropertyValue^>(reinterpret_cast<Platform::Object^>(gridRowVal.Get()));
            VERIFY_IS_NOT_NULL(test);
            VERIFY_ARE_EQUAL(test->GetInt32(), std::stoi(gridRow.Value));
        }

        void XamlDiagnosticsTests::TestGetPropertyIndexAndValueCustom()
        {
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/PageWithCustomUserControl.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            auto customUserControl = callback->GetElementByName(L"userControl1");

            wil::unique_propertychainvalue intProperty = GetPropertyChainValue(customUserControl.Handle, L"CustomInt", BaseValueSourceLocal);
            unsigned int indexPropertyChain = intProperty.Index;
            unsigned int indexGetPropertyIndex = 0;

            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(customUserControl.Handle, L"Tests.Tools.Shared.CustomUserControl.CustomInt", &indexGetPropertyIndex));
            VERIFY_ARE_NOT_EQUAL(indexGetPropertyIndex, 0u);
            VERIFY_ARE_EQUAL(indexPropertyChain, indexGetPropertyIndex);


            wrl::ComPtr<IInspectable> customIntVal;
            InstanceHandle customIntValHandle;
            VERIFY_SUCCEEDED(m_tap->GetProperty(customUserControl.Handle, indexPropertyChain, &customIntValHandle));
            VERIFY_SUCCEEDED(m_tap->GetIInspectableFromHandle(customIntValHandle, &customIntVal));

            auto test = safe_cast<IPropertyValue^>(reinterpret_cast<Platform::Object^>(customIntVal.Get()));
            VERIFY_IS_NOT_NULL(test);
            VERIFY_ARE_EQUAL(test->GetInt32(), std::stoi(intProperty.Value));

        }

        void XamlDiagnosticsTests::TestGetPropertyIndexAndValueCollection()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::gridString, callback);

            auto rootGrid = callback->GetElementByName(L"root");
            unsigned int indexGetPropertyIndex = 0;

            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(rootGrid.Handle, L"Microsoft.UI.Xaml.Controls.Panel.Children", &indexGetPropertyIndex));
            InstanceHandle childrenValHandle;
            VERIFY_SUCCEEDED(m_tap->GetProperty(rootGrid.Handle, indexGetPropertyIndex, &childrenValHandle));
            unsigned int count = 0;
            VERIFY_SUCCEEDED(m_tap->GetCollectionCount(childrenValHandle, &count));
            VERIFY_ARE_EQUAL(count, 4u);
        }

        void XamlDiagnosticsTests::TestAppAnalysisIntegrationWithLVT()
        {
            RuleTesterHelper helper(L"AA0009", L"IntegrationWithLVT");
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/PageWithDevirtualizedListView_Panel.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            helper.VerifyRuleTriggered(1u);
            VisualElement devirtualized = callback->GetElementByName(L"devirtualized");

            helper.VerifyCanLinkToLVT(0, devirtualized.Handle);
        }

        void XamlDiagnosticsTests::TestPropertyChainBinding()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = LoadXamlFromFunction(XamlDiagnosticsTestHelpers::SetupGroupedListView, callback);

            VisualElement listView = callback->GetElementByName(L"listView");
            wil::unique_propertychainvalue sourceProperty = GetPropertyChainValue(listView.Handle, L"ItemsSource", BaseValueSourceLocal);

            VERIFY_IS_TRUE(wcscmp(sourceProperty.ValueType, L"Microsoft.UI.Xaml.Data.Binding") == 0);
        }

        void XamlDiagnosticsTests::TestMetadataNullValueBits()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::gridString, callback);

            VisualElement rootGrid = callback->GetElementByName(L"root");

            wil::unique_propertychainvalue projection = GetPropertyChainValue(rootGrid.Handle, L"Projection");
            VERIFY_ARE_EQUAL(projection.MetadataBits & MetadataBit::IsValueNull, MetadataBit::IsValueNull);
            VERIFY_ARE_NOT_EQUAL(projection.MetadataBits & MetadataBit::IsValueHandle, MetadataBit::IsValueHandle);

            wil::unique_propertychainvalue margin = GetPropertyChainValue(rootGrid.Handle, L"Margin");
            VERIFY_ARE_NOT_EQUAL(margin.MetadataBits & MetadataBit::IsValueNull, MetadataBit::IsValueNull);
        }

        void XamlDiagnosticsTests::TestCreateInstanceDependencyProperty()
        {
            auto cleanup = XamlDiagnosticsTestHelpers::SetupGridAndWait();

            //Basic validation checks that creating a DependencyProperty works

            //Invalid property names should fail
            VERIFY_THROWS(CreateInstance(L"Microsoft.UI.Xaml.DependencyProperty", L"Some.Invalid.String"), WEX::Common::Exception);

            //A valid property name should work
            auto dp = CreateTemporaryInstance(L"Microsoft.UI.Xaml.DependencyProperty", L"Microsoft.UI.Xaml.Controls.TextBlock.Text");
            VERIFY_IS_GREATER_THAN(dp.Handle, 0u);
        }

        void XamlDiagnosticsTests::TestObjectIdentity()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::gridString, callback);

            VisualElement rect = callback->GetElementByName(L"Verified");

            Platform::Object^ verified;

            RunOnUIThread([&]()
            {
                auto grid = safe_cast<xaml_controls::Grid^>(test_infra::TestServices::WindowHelper->WindowContent);
                verified = safe_cast<Platform::Object^>(grid->FindName(L"Verified"));
            });

            // make sure these handles match! Before this would've been the IDependencyObject and this would fail
            VERIFY_ARE_EQUAL(rect.Handle, reinterpret_cast<InstanceHandle>(reinterpret_cast<IInspectable*>(verified)));
        }

        void XamlDiagnosticsTests::TestResourceDataTemplate()
        {
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/PageWithCustomUserControlUsingStaticResourceDataTemplate.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);
        }

        void XamlDiagnosticsTests::VerifyDontCleanupPropertiesForLiveElements()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::stackPanelWithButtonString, callback);

            VisualElement stackPanel = callback->GetElementByName(L"MyPanel");
            wil::unique_propertychainvalue children = GetPropertyChainValue(stackPanel.Handle, L"Children");

            // The repro is that GetPropertiesValueChain has to be called on the button. This adds the StackPanel to the
            // property map as the FrameworkElement.Parent property.
            VisualElement button = callback->GetElementByName(L"MyButton");
            wil::unique_propertychainvalue background = GetPropertyChainValue(button.Handle, L"Background");
            InstanceHandle childrenHandle = std::stoll(children.Value);

            // remove the 0th element from children (which is the button). This would previously cause all of the StackPanel's
            // properties to be released.
            VERIFY_SUCCEEDED(m_tap->RemoveChild(childrenHandle, 0));

            // create and add the border
            InstanceHandle border = CreateInstance(L"Microsoft.UI.Xaml.Controls.Border");
            VERIFY_SUCCEEDED(m_tap->AddChild(childrenHandle, border, 0));
        }

        void XamlDiagnosticsTests::TestThemeResourceInStyle()
        {
            //Modify a Style's property when another property in the Style uses a ThemeResource,
            //and verify it worked.
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/PageWithThemeResourceStyle.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            VisualElement button = callback->GetElementByName(L"button");

            wil::unique_propertychainsource style = m_tap->GetPropertyChainSource(button.Handle, BaseValueSourceStyle);
            VERIFY_ARE_NOT_EQUAL(style.Handle, 0);

            wil::unique_propertychainvalue background = GetPropertyChainValue(button.Handle, L"Background", BaseValueSourceDefault);
            InstanceHandle newBrushBlue = CreateInstance(background.Type, L"Blue");

            VERIFY_SUCCEEDED(TrySetPropertyByIndex(style.Handle, newBrushBlue, background.Index));

            background = GetPropertyChainValue(button.Handle, L"Background", BaseValueSourceStyle);
            VERIFY_ARE_NOT_EQUAL(std::stoll(background.Value), 0);
            wil::unique_propertychainvalue colorProperty = GetPropertyChainValue(std::stoll(background.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"Blue") == 0);
        }

        void XamlDiagnosticsTests::TestAddNewSetterInStyle()
        {
            //Verifies that calling SetProperty on a Style with a property that doesn't exist as
            //a setter correctly adds the setter to the Style.
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/PageWithThemeResourceStyle.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            VisualElement button = callback->GetElementByName(L"button");

            wil::unique_propertychainsource style = m_tap->GetPropertyChainSource(button.Handle, BaseValueSourceStyle);
            VERIFY_ARE_NOT_EQUAL(style.Handle, 0);

            wil::unique_propertychainvalue foreground = GetPropertyChainValue(button.Handle, L"Foreground", BaseValueSourceDefault);
            InstanceHandle newBrushYellow = CreateInstance(foreground.Type, L"Yellow");

            VERIFY_SUCCEEDED(TrySetPropertyByIndex(style.Handle, newBrushYellow, foreground.Index));

            foreground = GetPropertyChainValue(button.Handle, L"Foreground", BaseValueSourceStyle);
            VERIFY_ARE_NOT_EQUAL(std::stoll(foreground.Value), 0);
            wil::unique_propertychainvalue colorProperty = GetPropertyChainValue(std::stoll(foreground.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"Yellow") == 0);

            //Verify the Button's effective value is also now Yellow
            InstanceHandle foregroundHandle;
            VERIFY_SUCCEEDED(m_tap->GetProperty(button.Handle, foreground.Index, &foregroundHandle));
            colorProperty = GetPropertyChainValue(foregroundHandle, L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"Yellow") == 0);    // WPF_HOSTING_MODE_FAILURE - fails here.
        }

        void XamlDiagnosticsTests::TestChangeSetterProperty()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;

            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::buttonWithCustomStyleDuplicatedSettersString, callback);
            auto theButton = callback->GetElementByName(L"theButton");
            auto theButtonAsDO = ih_cast<xaml_controls::Button>(theButton.Handle);

            wil::unique_propertychainsource style = m_tap->GetPropertyChainSource(theButton.Handle, BaseValueSourceStyle);
            VERIFY_ARE_NOT_EQUAL(style.Handle, 0);

            auto setters = GetCollectionProperty(style.Handle, L"Microsoft.UI.Xaml.Style.Setters");
            VERIFY_ARE_EQUAL(setters.Count, 2u);
            InstanceHandle secondSetterHandle = ih_cast(setters.Elements[1].Value);

            // Verify the Button's Background brush
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Gold, safe_cast<SolidColorBrush^>(theButtonAsDO->Background)->Color);
            });

            // Change the second Setter's Property to Foreground
            auto propertyIndex = GetPropertyIndex(secondSetterHandle, L"Microsoft.UI.Xaml.Setter.Property");
            auto propertyHandle = CreateInstance(L"Microsoft.UI.Xaml.DependencyProperty", L"Microsoft.UI.Xaml.Controls.Control.Foreground");
            VERIFY_SUCCEEDED(TrySetPropertyByIndex(secondSetterHandle, propertyHandle, propertyIndex));

            // Verify the Button's Background and Foreground brushes
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, safe_cast<SolidColorBrush^>(theButtonAsDO->Background)->Color);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Gold, safe_cast<SolidColorBrush^>(theButtonAsDO->Foreground)->Color);
            });
        }

        void XamlDiagnosticsTests::TestClearRenderProperty()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::stackPanelWithButtonString, callback);

            VisualElement button = callback->GetElementByName(L"MyButton");
            wil::unique_propertychainvalue transform = GetPropertyChainValue(button.Handle, L"RenderTransform", BaseValueSourceLocal);
            wil::unique_propertychainvalue scalex = GetPropertyChainValue(std::stoll(transform.Value), L"ScaleX", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(scalex.Value, L"3") == 0);
            VERIFY_SUCCEEDED(m_tap->ClearProperty(std::stoll(transform.Value), scalex.Index));

            //Try getting the ScaleX property again after it's been cleared - GetPropertyChainValue should throw
            //an exception since there shouldn't be a locally set value anymore.
            VERIFY_THROWS(scalex = GetPropertyChainValue(std::stoll(transform.Value), L"RenderTransform", BaseValueSourceLocal);, WEX::Common::Exception);
        }

        void XamlDiagnosticsTests::TestGetPropertyNull()
        {
                wrl::ComPtr<VisualTreeServiceCallback> callback;
                auto cleanup = XamlDiagnosticsTests::SetupGridWithCallback(callback);

                VisualElement root = callback->GetElementByName(L"root");
                unsigned int indexGetPropertyIndex = 0;
                VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(root.Handle, L"Microsoft.UI.Xaml.UIElement.Projection", &indexGetPropertyIndex));
                VERIFY_ARE_NOT_EQUAL(indexGetPropertyIndex, 0u);

                //Fetch the Projection property which should be null.  Real goal of this test is to ensure GetProperty on a null property doesn't cause a crash,
                //but also verify the handle is null.
                InstanceHandle projHandle;
                VERIFY_SUCCEEDED(m_tap->GetProperty(root.Handle, indexGetPropertyIndex, &projHandle));
                VERIFY_ARE_EQUAL(projHandle, 0u);
        }

        void XamlDiagnosticsTests::TestEvaluatedValue()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = SetupGridWithCallback(callback);

            VisualElement textBlock = callback->GetElementByName(L"texty");

            auto horizAlignValue = GetPropertyChainValue(textBlock.Handle, L"HorizontalAlignment", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(horizAlignValue.MetadataBits & MetadataBit::IsValueBindingExpression, MetadataBit::IsValueBindingExpression);

            // query properties on this guy
            auto evaluatedValue = GetPropertyChainValue(std::stoll(horizAlignValue.Value), L"EvaluatedValue", BaseValueSourceLocal);
            auto convertedEnum = m_tap->ConvertEnumValue(evaluatedValue.Type, std::stoi(evaluatedValue.Value));
            VERIFY_IS_TRUE(evaluatedValue.MetadataBits == MetadataBit::None);
            VERIFY_IS_TRUE(convertedEnum.compare(L"Right") == 0);

            auto isValid = GetPropertyChainValue(std::stoll(horizAlignValue.Value), L"IsBindingValid", BaseValueSourceLocal);
            VERIFY_IS_TRUE(isValid.MetadataBits == MetadataBit::None);
            VERIFY_IS_TRUE(wcscmp(isValid.Value, L"False") == 0);

            // Get the height property, which is also binding
            auto heightValue = GetPropertyChainValue(textBlock.Handle, L"Height", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(heightValue.MetadataBits & MetadataBit::IsValueBindingExpression, MetadataBit::IsValueBindingExpression);

            // query properties on the height guy
            auto evaluatedHeightValue = GetPropertyChainValue(std::stoll(heightValue.Value), L"EvaluatedValue", BaseValueSourceLocal);
            VERIFY_IS_TRUE(evaluatedHeightValue.MetadataBits == MetadataBit::None);
            VERIFY_IS_TRUE(wcscmp(evaluatedHeightValue.Value, L"30") == 0);

            auto isHeightValid = GetPropertyChainValue(std::stoll(heightValue.Value), L"IsBindingValid", BaseValueSourceLocal);
            VERIFY_IS_TRUE(isHeightValid.MetadataBits == MetadataBit::None);
            VERIFY_IS_TRUE(wcscmp(isHeightValid.Value, L"True") == 0);

            // update the height property on the on childVerified element, this should update the binding
            RunOnUIThread([&] {
                auto grid = safe_cast<Grid^>(TestServices::WindowHelper->WindowContent);
                auto rectangle = safe_cast<xaml_shapes::Rectangle^>(grid->FindName(L"childVerified"));
                rectangle->Height = 100;
            });

            TestServices::WindowHelper->WaitForIdle();

            // Get the evaluated value for the height
            heightValue = GetPropertyChainValue(textBlock.Handle, L"Height", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(heightValue.MetadataBits & MetadataBit::IsValueBindingExpression, MetadataBit::IsValueBindingExpression);

            // query properties on this guy again
            evaluatedHeightValue = GetPropertyChainValue(std::stoll(heightValue.Value), L"EvaluatedValue", BaseValueSourceLocal);
            VERIFY_IS_TRUE(evaluatedHeightValue.MetadataBits == MetadataBit::None);
            VERIFY_IS_TRUE(wcscmp(evaluatedHeightValue.Value, L"100") == 0);

            // Get an element that has a binding to a DO, and verify we can get the EvaluatedValue for that
            VisualElement yellowRect = callback->GetElementByName(L"childVerified");

            auto fillValue = GetPropertyChainValue(yellowRect.Handle, L"Fill", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(fillValue.MetadataBits & MetadataBit::IsValueBindingExpression, MetadataBit::IsValueBindingExpression);

            // query properties on this guy
            auto evaluatedFillValue = GetPropertyChainValue(std::stoll(fillValue.Value), L"EvaluatedValue", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(evaluatedFillValue.MetadataBits & MetadataBit::IsValueHandle, MetadataBit::IsValueHandle);

            auto isFillValid = GetPropertyChainValue(std::stoll(fillValue.Value), L"IsBindingValid", BaseValueSourceLocal);
            VERIFY_IS_TRUE(isFillValid.MetadataBits == MetadataBit::None);
            VERIFY_IS_TRUE(wcscmp(isFillValid.Value, L"True") == 0);

            // query properties on the brush
            auto boundBrushValue = GetPropertyChainValue(std::stoll(evaluatedFillValue.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(boundBrushValue.Value, L"Yellow") == 0);
        }

        void XamlDiagnosticsTests::TestBasedOnSetterChange()
        {
            //Verify that a change to an existing Setter's Value property in Style A, which is used as the
            //BasedOn Style by Style B,
            //affects an object that uses Style B.
            //Actually two tests:
            //1. If the changed Setter is overwritten in Style B, nothing changes in the object,
            //2. If the changed Setter isn't overwritten, the change does appear in the object
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::stackPanelWithNestedStylesString, callback);

            //Get the dictionary containing the various Styles.
            VisualElement button = callback->GetElementByName(L"button");
            VisualElement panel = callback->GetElementByName(L"panel");

            unsigned int indexGetPropertyIndex = 0;
            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(panel.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources", &indexGetPropertyIndex));
            InstanceHandle parentDictionaryHandle;
            VERIFY_SUCCEEDED(m_tap->GetProperty(panel.Handle, indexGetPropertyIndex, &parentDictionaryHandle));

            //Get the grandparent Style and the overwritten Setter to change.
            ResourceDictionary^ parentDictionary;
            Style^ style1;
            Platform::String^ style1Key = L"style1";
            RunOnUIThread([&]()
            {
                parentDictionary = safe_cast<ResourceDictionary^>(reinterpret_cast<Platform::Object^>(reinterpret_cast<IInspectable*>(parentDictionaryHandle)));
                style1 = safe_cast<Style^>(parentDictionary->Lookup(style1Key));
            });

            IInspectable* pStyle1Inspectable = reinterpret_cast<IInspectable*>(safe_cast<Platform::Object^>(style1));
            InstanceHandle style1Handle;
            VERIFY_SUCCEEDED(m_tap->GetHandleFromIInspectable(pStyle1Inspectable, &style1Handle));

            //Get the Button's background - it should be the overwriting value from style2, Red
            wil::unique_propertychainvalue background = GetPropertyChainValue(button.Handle, L"Background", BaseValueSourceStyle);
            wil::unique_propertychainvalue colorProperty = GetPropertyChainValue(std::stoll(background.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"Red") == 0);

            //Modify style1's overwritten Background Setter through SetProperty.
            InstanceHandle newBrushGreen = CreateInstance(background.Type, L"Green");

            VERIFY_SUCCEEDED(TrySetPropertyByIndex(style1Handle, newBrushGreen, background.Index));

            //Verify the Setter in style3 hasn't changed [note: GetPropertyChainValue ultimately uses
            //uses the Style's Setters, so we should still verify the effective value on the Button hasn't changed]
            background = GetPropertyChainValue(button.Handle, L"Background", BaseValueSourceStyle);
            colorProperty = GetPropertyChainValue(std::stoll(background.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"Red") == 0);

            //Verify the effective value is still Red
            InstanceHandle backgroundHandle;
            VERIFY_SUCCEEDED(m_tap->GetProperty(button.Handle, background.Index, &backgroundHandle));
            colorProperty = GetPropertyChainValue(backgroundHandle, L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"Red") == 0);

            //Now do the same thing for the Foreground setter, which hasn't been overwritten and
            //should show up as a change for the Button
            wil::unique_propertychainvalue foreground = GetPropertyChainValue(button.Handle, L"Foreground", BaseValueSourceStyle);
            colorProperty = GetPropertyChainValue(std::stoll(foreground.Value), L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"White") == 0);

            //Modify style1's non-overwritten Foreground Setter through SetProperty.
            InstanceHandle newBrushPurple = CreateInstance(foreground.Type, L"Purple");
            VERIFY_SUCCEEDED(TrySetPropertyByIndex(style1Handle, newBrushPurple, foreground.Index));

            //Verify the effective value is now Purple
            InstanceHandle foregroundHandle;
            VERIFY_SUCCEEDED(m_tap->GetProperty(button.Handle, foreground.Index, &foregroundHandle));
            colorProperty = GetPropertyChainValue(foregroundHandle, L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"Purple") == 0);
        }

        void XamlDiagnosticsTests::TestChangedBasedOnProperty()
        {
            //Try changing the BasedOn property of a Style and ensure objects are updated properly
            //The new Style contains new values for the Background and Foreground - Background should remain
            //overwritten, but the new Foreground value should appear on the object
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::stackPanelWithNestedStylesString, callback);

            //Get the dictionary containing the various Styles.
            VisualElement button = callback->GetElementByName(L"button");
            VisualElement panel = callback->GetElementByName(L"panel");

            //Get the parent Style

            auto style2Handle = GetItemFromElementResources(panel, L"style2");
            auto newStyleHandle = GetItemFromElementResources(panel, L"unusedStyle");

            //Change the BasedOn property of style2
            unsigned int basedOnIndex = 0;
            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(style2Handle, L"Microsoft.UI.Xaml.Style.BasedOn", &basedOnIndex));
            VERIFY_SUCCEEDED(TrySetPropertyByIndex(style2Handle, newStyleHandle, basedOnIndex));

            //Verify the effective value of Background is still Red
            unsigned int backgroundIndex = 0;
            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(button.Handle, L"Microsoft.UI.Xaml.Controls.Control.Background", &backgroundIndex));
            InstanceHandle backgroundHandle;
            VERIFY_SUCCEEDED(m_tap->GetProperty(button.Handle, backgroundIndex, &backgroundHandle));
            wil::unique_propertychainvalue colorProperty = GetPropertyChainValue(backgroundHandle, L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"Red") == 0);

            //Verify the effective value of Foreground is now White
            unsigned int foregroundIndex = 0;
            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(button.Handle, L"Microsoft.UI.Xaml.Controls.Control.Foreground", &foregroundIndex));
            InstanceHandle foregroundHandle;
            VERIFY_SUCCEEDED(m_tap->GetProperty(button.Handle, foregroundIndex, &foregroundHandle));
            colorProperty = GetPropertyChainValue(foregroundHandle, L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"Purple") == 0);
        }

        void XamlDiagnosticsTests::TestClearPropertyBinding()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::resourcesStaticResourceInBindingString, callback);

            //Verify the binding initially exists
            VisualElement child = callback->GetElementByName(L"childVerified");
            wil::unique_propertychainvalue sourceProperty = GetPropertyChainValue(child.Handle, L"Width", BaseValueSourceLocal);

            VERIFY_IS_TRUE(wcscmp(sourceProperty.ValueType, L"Microsoft.UI.Xaml.Data.Binding") == 0);

            //Clear the binding
            VERIFY_SUCCEEDED(m_tap->ClearProperty(child.Handle, sourceProperty.Index));

            //Verify the binding is cleared - gettting the local property again should throw an exception since it shouldn't exist any more
            VERIFY_THROWS(sourceProperty = GetPropertyChainValue(child.Handle, L"Width", BaseValueSourceLocal), WEX::Common::Exception);
        }

        void XamlDiagnosticsTests::GetPropertyIndexFailsForInvalidProperty()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::gridString, callback);

            auto rootGrid = callback->GetElementByName(L"root");

            unsigned int index = 0;
            LOG_OUTPUT(L"Verify getting Control.Background fails for a Grid");
            VERIFY_FAILED(m_tap->GetPropertyIndex(rootGrid.Handle, L"Microsoft.UI.Xaml.Controls.Control.Background", &index));

            LOG_OUTPUT(L"Verify getting Panel.Background succeeds for a Grid");
            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(rootGrid.Handle, L"Microsoft.UI.Xaml.Controls.Panel.Background", &index));
        }

        void XamlDiagnosticsTests::GetAttachedPropertyWithUnknownOwnerType()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto attachedString = ref new Platform::String(L"<Grid x:Name='root'"
                        L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                        L"  xmlns:local='using:Tests.Tools.Shared'"
                        L"  local:CustomUserControl.CustomAttached='3' />");
            auto cleanup = m_connectionHelper->Advise(attachedString, callback);

            auto root = callback->GetElementByName(L"root");

            // Get properties on element with the attached property. Since the ownerType is unknown, it should be empty
            auto attachedProp = GetPropertyChainValue(root.Handle, L".CustomAttached", BaseValueSourceLocal);

            // We haven't crashed, that's better than before. Make sure the value is correct while we're at it.
            VERIFY_ARE_EQUAL(3, std::stoi(attachedProp.Value));
        }

        void XamlDiagnosticsTests::VerifyGetBuiltinStyleProperties()
        {
            RunOnUIThread([&]()
            {
                // The MUXC theme resources clash with these verifications, so we'll clear them out first.
                Application::Current->Resources->MergedDictionaries->Clear();
            });
                
            // When a property is set by a built-in style, the framework only knows that it's set by some style,
            // not necessarily which one it is, so we have to fix the BaseValueSource.
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto attachedString = ref new Platform::String(L"<Button x:Name='root'"
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' />");
            auto cleanup = m_connectionHelper->Advise(attachedString, callback);

            // Just get a few properties that should show up. We won't validate what they evaluate to, since someone
            // could change that and it's not the point of the test. We'll get properties that are most likely always
            // going to be in the default style
            auto button = callback->GetElementByName(L"root");

            VERIFY_NO_THROW(GetPropertyChainValue(button.Handle, L"Background", BaseValueSourceBuiltInStyle));
            VERIFY_NO_THROW(GetPropertyChainValue(button.Handle, L"Foreground", BaseValueSourceBuiltInStyle));
            VERIFY_NO_THROW(GetPropertyChainValue(button.Handle, L"FontSize", BaseValueSourceBuiltInStyle));
            VERIFY_NO_THROW(GetPropertyChainValue(button.Handle, L"FontWeight", BaseValueSourceBuiltInStyle));

            // Make sure there's no funny business going on
            VERIFY_THROWS(GetPropertyChainValue(button.Handle, L"Background", BaseValueSourceStyle), WEX::Common::Exception);
            VERIFY_THROWS(GetPropertyChainValue(button.Handle, L"Foreground", BaseValueSourceStyle), WEX::Common::Exception);
            VERIFY_THROWS(GetPropertyChainValue(button.Handle, L"FontSize", BaseValueSourceStyle), WEX::Common::Exception);
            VERIFY_THROWS(GetPropertyChainValue(button.Handle, L"FontWeight", BaseValueSourceStyle), WEX::Common::Exception);
        }

        void XamlDiagnosticsTests::VerifySourceChainReflectsPrecedence()
        {
            // Set window size to something that will trigger the trigger
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto sourceChainString = ref new Platform::String(
                L"<Page x:Name='root'"
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Grid x:Name='root'>"
                L"    <VisualStateManager.VisualStateGroups>"
                L"      <VisualStateGroup>"
                L"        <VisualState>"
                L"          <VisualState.StateTriggers>"
                L"            <AdaptiveTrigger MinWindowWidth='10' />"
                L"          </VisualState.StateTriggers>"
                L"          <VisualState.Setters>"
                L"            <Setter Target='MyButton.Background' Value='Blue' />"
                L"          </VisualState.Setters>"
                L"        </VisualState>"
                L"      </VisualStateGroup>"
                L"    </VisualStateManager.VisualStateGroups>"
                L"    <Grid.Resources>"
                L"      <Style TargetType='Button' x:Key='MyButtonStyle'>"
                L"        <Setter Property='Background' Value='Red' />"
                L"      </Style>"
                L"    </Grid.Resources>"
                L"    <Button x:Name='MyButton' Style='{StaticResource MyButtonStyle}' Background='Yellow' />"
                L"  </Grid>"
                L"</Page>");

            auto cleanup = m_connectionHelper->Advise(sourceChainString, callback);
            auto button = callback->GetElementByName(L"MyButton");
            auto animatedValue = GetPropertyChainValue(button.Handle, L"Background", BaseValueSource::Animation);
            auto localValue = GetPropertyChainValue(button.Handle, L"Background", BaseValueSourceLocal);
            auto styleValue = GetPropertyChainValue(button.Handle, L"Background", BaseValueSourceStyle);
            auto builtInStyleValue = GetPropertyChainValue(button.Handle, L"Background", BaseValueSourceBuiltInStyle);
            auto defaultValue = GetPropertyChainValue(button.Handle, L"Background", BaseValueSourceDefault);

            // Make sure overridden is set correctly
            VERIFY_IS_FALSE(animatedValue.Overridden);
            VERIFY_IS_TRUE(localValue.Overridden);
            VERIFY_IS_TRUE(styleValue.Overridden);
            VERIFY_IS_TRUE(builtInStyleValue.Overridden);
            VERIFY_IS_TRUE(defaultValue.Overridden);

            // Make sure the PropertyChainIndex is in the correct order.
            VERIFY_IS_LESS_THAN(animatedValue.PropertyChainIndex, localValue.PropertyChainIndex);
            VERIFY_IS_LESS_THAN(localValue.PropertyChainIndex, styleValue.PropertyChainIndex);
            VERIFY_IS_LESS_THAN(styleValue.PropertyChainIndex, builtInStyleValue.PropertyChainIndex);
            VERIFY_IS_LESS_THAN(builtInStyleValue.PropertyChainIndex, defaultValue.PropertyChainIndex);
        }

        void XamlDiagnosticsTests::VerifyOnDemandTransitionCollections()
        {
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto sourceChainString = ref new Platform::String(
                L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <ItemsPresenter x:Name='MyItemsPresenter'/>"
                L"</Page>");
            auto cleanup = m_connectionHelper->Advise(sourceChainString, callback);
            auto pivotItemPresenter = callback->GetElementByName(L"MyItemsPresenter");
            auto defaultValue = GetPropertyChainValue(pivotItemPresenter.Handle, L"HeaderTransitions", BaseValueSourceDefault);

            VERIFY_IS_NOT_NULL(defaultValue.Value);
            VERIFY_IS_FALSE(defaultValue.Overridden);
        }

        void XamlDiagnosticsTests::GetPropertyChainReturnsDesiredSize()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::gridString, callback);

            auto textBox = callback->GetElementByName(L"texty");

            auto texty = ih_cast<xaml_controls::TextBox>(textBox.Handle);

            std::wstring actualDesiredSize;
            RunOnUIThread([&]
            {
                WCHAR value[20];
                value[0] = L'\0';
                swprintf_s(value, ARRAYSIZE(value), L"%gx%g", texty->DesiredSize.Width, texty->DesiredSize.Height);
                actualDesiredSize = value;
            });

            auto reportedDesiredSize = GetPropertyChainValue(textBox.Handle, L"DesiredSize");
            VERIFY_ARE_EQUAL(actualDesiredSize, std::wstring(reportedDesiredSize.Value));

            VERIFY_ARE_EQUAL(std::wstring(L"Microsoft.UI.Xaml.UIElement"), std::wstring(reportedDesiredSize.DeclaringType));
            VERIFY_ARE_EQUAL(std::wstring(L"Windows.Foundation.Size"), std::wstring(reportedDesiredSize.Type));
            VERIFY_ARE_EQUAL(std::wstring(L"Windows.Foundation.Size"), std::wstring(reportedDesiredSize.ValueType));
            VERIFY_ARE_EQUAL(MetadataBit::IsPropertyReadOnly, reportedDesiredSize.MetadataBits);
        }

        void XamlDiagnosticsTests::CorrectlyValidateFakeProperties()
        {
            auto xamlText = ref new Platform::String(
            L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Background='Red' Height='45' Width='45'>"
            L"  <Grid x:Name='child' Background='Red'>"
            L"    <Rectangle x:Name='childVerified' Height='30' Width='30'  />"
            L"  </Grid>"
            L"  <TextBox x:Name='texty' Height='{Binding ElementName=childVerified, Path=Height}'/>"
            L"  <RatingControl x:Name='rater' />"
            L"</Grid>");

             wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(xamlText, callback);

            auto textBox = callback->GetElementByName(L"texty");

            auto validateProperty = [&](InstanceHandle objectHandle, const std::wstring& propertyName, BaseValueSource valueSource, TemporaryInstance&& newValue){
                TemporaryInstance localNewValue(std::move(newValue));
                auto chainValue = GetPropertyChainValue(objectHandle, propertyName, valueSource);
                InstanceHandle wouldBeValue = 0;
                HRESULT hr = TryGetPropertyByIndex(objectHandle, chainValue.Index, &wouldBeValue);
                VERIFY_ARE_EQUAL(hr, E_FAIL);
                hr = TrySetPropertyByIndex(objectHandle, localNewValue.Handle, chainValue.Index);
                VERIFY_ARE_EQUAL(hr, E_FAIL);
                hr = TryClearPropertyByIndex(objectHandle, chainValue.Index);
                VERIFY_ARE_EQUAL(hr, E_FAIL);
            };

            LOG_OUTPUT(L"Verifying DesiredSize");
            validateProperty(textBox.Handle, L"DesiredSize", BaseValueSourceDefault, CreateTemporaryInstance(L"Windows.Foundation.Size", L"20,20"));

            auto height = GetPropertyChainValue(textBox.Handle, L"Height", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(height.MetadataBits & MetadataBit::IsValueBindingExpression, MetadataBit::IsValueBindingExpression);
            auto bindingHandle = ih_cast(height.Value);

            LOG_OUTPUT(L"Verifying IsBindingValid");
            validateProperty(bindingHandle, L"IsBindingValid", BaseValueSourceLocal, CreateTemporaryInstance(L"Windows.Foundation.Boolean", L"True"));

            LOG_OUTPUT(L"Verifying EvaluatedValue");
            validateProperty(bindingHandle, L"EvaluatedValue", BaseValueSourceLocal,  CreateTemporaryInstance(L"Windows.Foundation.Double", L"50"));

            // WinUI 3: Handout visual for RatingControl not found in XamlDiagnosticsTests::CorrectlyValidateFakeProperties
            /*auto rater = callback->GetElementByName(L"rater");

            RunOnUIThread([&] {
                auto handout = xaml_hosting::ElementCompositionPreview::GetElementVisual(ih_cast<UIElement>(rater.Handle));
                auto handinVisual = CompositionTarget::GetCompositorForCurrentThread()->CreateSpriteVisual();

                xaml_hosting::ElementCompositionPreview::SetElementChildVisual(ih_cast<UIElement>(rater.Handle), handinVisual);
            });

            LOG_OUTPUT(L"Verifying Handout Visual");
            validateProperty(rater.Handle, L"Handout Visual", BaseValueSourceLocal,  CreateTemporaryInstance(L"Windows.Foundation.Double", L"50"));*/
        }

        void XamlDiagnosticsTests::ClearTypesOnNonPlatformTypes()
        {
            auto xamlText = ref new Platform::String(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Background='Red' Height='45' Width='45'>"
                L"  <PersonPicture x:Name='personPicture'  ProfilePicture='ms-appx:///resources/native/external/foundation/graphics/image/Image0.jpg' />"
                L"</Grid>");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(xamlText, callback);

            auto person = callback->GetElementByName(L"personPicture");

            ClearProperty(person.Handle, L"Microsoft.UI.Xaml.Controls.PersonPicture.ProfilePicture");

            VERIFY_THROWS(GetPropertyChainValue(person.Handle, L"ProfilePicture", BaseValueSourceLocal), WEX::Common::Exception);
        }

        void XamlDiagnosticsTests::ValidateCorrectNamescopes()
        {
            auto xamlText = ref new Platform::String(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Background='Red' Height='45' Width='45'>"
                L"  <ScrollViewer x:Name='scrolly'/>"
                L"</Grid>");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(xamlText, callback);
            auto stackPanel = CreateInstance(L"Microsoft.UI.Xaml.Controls.StackPanel");
            auto scrollViewer = callback->GetElementByName(L"scrolly");
            SetProperty(scrollViewer.Handle,stackPanel, L"Microsoft.UI.Xaml.Controls.ContentControl.Content");

            auto innerTextBox = CreateInstance(L"Microsoft.UI.Xaml.Controls.TextBox");
            AppendToCollection(stackPanel, L"Microsoft.UI.Xaml.Controls.Panel.Children", innerTextBox);

            // Tick the UI thread a couple times, the crash happens during the enter walk of the TextBox
            test_infra::TestServices::WindowHelper->SynchronouslyTickUIThread(2);
        }

        void XamlDiagnosticsTests::TestCanSetNull()
        {
            auto xamlText = ref new Platform::String(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Background='Red' Height='45' Width='45'>"
                L"</Grid>");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(xamlText, callback);
            auto root = callback->GetElementByName(L"root");

            // Verify the value of Background is Red
            unsigned int backgroundIndex = 0;
            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(root.Handle, L"Microsoft.UI.Xaml.Controls.Panel.Background", &backgroundIndex));
            InstanceHandle backgroundHandle;
            VERIFY_SUCCEEDED(m_tap->GetProperty(root.Handle, backgroundIndex, &backgroundHandle));
            wil::unique_propertychainvalue colorProperty = GetPropertyChainValue(backgroundHandle, L"Color", BaseValueSourceLocal);
            VERIFY_IS_TRUE(wcscmp(colorProperty.Value, L"Red") == 0);

            // Set the background to null
            VERIFY_SUCCEEDED(TrySetPropertyByIndex(root.Handle, 0, backgroundIndex));

            // Verify background is now null
            InstanceHandle nullBackgroundHandle = 1; // Set to non-null to verify it's correctly set to null after the GetProperty call
            VERIFY_SUCCEEDED(m_tap->GetProperty(root.Handle, backgroundIndex, &nullBackgroundHandle));
            VERIFY_ARE_EQUAL(nullBackgroundHandle, 0);
        }

        void XamlDiagnosticsTests::TestCanGetSetSimpleProperty()
        {
            auto xamlText = ref new Platform::String(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Background='Red' Height='45' Width='45'>"
                L"</Grid>");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(xamlText, callback);
            auto root = callback->GetElementByName(L"root");

            // Test Microsoft.UI.Xaml.UIElement.Rotation
            auto rotationHandle = GetProperty(root.Handle, L"Microsoft.UI.Xaml.UIElement.Rotation");
            VERIFY_ARE_EQUAL(0, (int)ih_cast<Platform::IBox<float>>(rotationHandle)->Value);

            auto newValue = CreateInstance(L"Windows.Foundation.Single", L"50");

            SetProperty(root.Handle, newValue, L"Microsoft.UI.Xaml.UIElement.Rotation");
            auto retrievedNewValue = GetProperty(root.Handle, L"Microsoft.UI.Xaml.UIElement.Rotation");
            VERIFY_ARE_EQUAL(50, (int)ih_cast<Platform::IBox<float>>(retrievedNewValue)->Value);

            // Test Microsoft.UI.Xaml.UIElement.Scale
            newValue = CreateInstance(L"Windows.Foundation.Numerics.Vector3", L"1,4,7");
            SetProperty(root.Handle, newValue, L"Microsoft.UI.Xaml.UIElement.Scale");
            retrievedNewValue = GetProperty(root.Handle, L"Microsoft.UI.Xaml.UIElement.Scale");

            // Unfortunately the Cpp version of Windows.Foundation.Numerics.Vector3, Windows.Foundation.Numerics.float3, is treated
            // as a distinct type, meaning we can't get cast to the right box type to inspect the struct's contents.  Instead we'll
            // go through the normal property getter on the actual root grid object.
            ::Windows::Foundation::Numerics::float3 retVector3;
            RunOnUIThread([&] {
                auto grid = safe_cast<Grid^>(TestServices::WindowHelper->WindowContent);
                retVector3 = grid->Scale;
            });
            VERIFY_ARE_EQUAL(1, (int)retVector3.x);
            VERIFY_ARE_EQUAL(4, (int)retVector3.y);
            VERIFY_ARE_EQUAL(7, (int)retVector3.z);

            // Make sure creating an invalid vector 3 fails
            VERIFY_THROWS(CreateInstance(L"Windows.Foundation.Numerics.Vector3", L""),  WEX::Common::Exception);

            // make sure we can create the quaternion type
            CreateTemporaryInstance(L"Windows.Foundation.Numerics.Quaternion", L"1,2,3,4");

            auto scalePropertyChain = GetPropertyChainValue(root.Handle, L"Scale", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(std::wstring(scalePropertyChain.Value), std::wstring(L"1.000000,4.000000,7.000000"));
            VERIFY_ARE_EQUAL(std::wstring(scalePropertyChain.ValueType), std::wstring(L"Windows.Foundation.Numerics.Vector3"));
            VERIFY_ARE_EQUAL(std::wstring(scalePropertyChain.DeclaringType), std::wstring(L"Microsoft.UI.Xaml.UIElement"));
            VERIFY_ARE_EQUAL(std::wstring(scalePropertyChain.Type), std::wstring(L"Windows.Foundation.Numerics.Vector3"));

            ClearProperty(root.Handle, L"Microsoft.UI.Xaml.UIElement.Scale");

            RunOnUIThread([&] {
                auto grid = safe_cast<Grid^>(TestServices::WindowHelper->WindowContent);
                retVector3 = grid->Scale;
            });
            VERIFY_ARE_EQUAL(1, (int)retVector3.x);
            VERIFY_ARE_EQUAL(1, (int)retVector3.y);
            VERIFY_ARE_EQUAL(1, (int)retVector3.z);

            // Try setting a simple property on Xaml object which is internally a CDependencyObject, but externally not an IDependencyObject

            auto tempBrushTransition = CreateTemporaryInstance(L"Microsoft.UI.Xaml.BrushTransition");
            auto brushTransition = tempBrushTransition.Handle;
            auto duration = CreateInstance(L"Windows.Foundation.TimeSpan", L"00:00:02");

            SetProperty(brushTransition, duration, L"Microsoft.UI.Xaml.BrushTransition.Duration");

            VERIFY_ARE_EQUAL(GetProperty<Platform::IBox<::Windows::Foundation::TimeSpan>>(brushTransition, L"Microsoft.UI.Xaml.BrushTransition.Duration")->Value.Duration, 20000000);

            // Verify another type which used to explicitly block DependencyObject queries,
            // Vector3Transition

            auto tempVector3Transition = CreateTemporaryInstance(L"Microsoft.UI.Xaml.Vector3Transition");
            auto vector3Transition = tempVector3Transition.Handle;
            auto durationVec3 = CreateInstance(L"Windows.Foundation.TimeSpan", L"00:00:06");

            SetProperty(vector3Transition, durationVec3, L"Microsoft.UI.Xaml.Vector3Transition.Duration");

            VERIFY_ARE_EQUAL(GetProperty<Platform::IBox<::Windows::Foundation::TimeSpan>>(vector3Transition, L"Microsoft.UI.Xaml.Vector3Transition.Duration")->Value.Duration, 60000000);
        }

        void XamlDiagnosticsTests::TestCanGetSetNonDP()
        {
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/PageWithCustomUserControl.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            auto customUserControl = callback->GetElementByName(L"userControl1");

            // Verify we can get the non-DP brush and its initial color is correct
            // given the markup (Orange)
            auto brush = GetProperty<SolidColorBrush>(customUserControl.Handle, L"Tests.Tools.Shared.CustomUserControl.CustomBrushNonDP");
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Orange, brush->Color);
            });

            // Create a new Green brush and set it on the non-DP
            auto newBrushGreen = CreateInstance(L"Microsoft.UI.Xaml.Media.SolidColorBrush", L"Green");

            SetProperty(customUserControl.Handle, newBrushGreen, L"Tests.Tools.Shared.CustomUserControl.CustomBrushNonDP");

            // Get the new brush from the non-DP property, verify the handle is equal to the one
            // we just created, and that its color is appropriately Green
            brush = GetProperty<SolidColorBrush>(customUserControl.Handle, L"Tests.Tools.Shared.CustomUserControl.CustomBrushNonDP");

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Green, brush->Color);
            });

            auto customDouble = GetProperty<wf::IPropertyValue>(customUserControl.Handle, L"Tests.Tools.Shared.CustomUserControl.CustomDoubleNonDP");

            VERIFY_ARE_EQUAL(2, (int)customDouble->GetDouble());

            ClearProperty(customUserControl.Handle, L"Tests.Tools.Shared.CustomUserControl.CustomDoubleNonDP");

            customDouble = GetProperty<wf::IPropertyValue>(customUserControl.Handle, L"Tests.Tools.Shared.CustomUserControl.CustomDoubleNonDP");
            VERIFY_ARE_EQUAL(0, (int)customDouble->GetDouble());
        }

        void XamlDiagnosticsTests::TestCanUseIPropertyValueTypes()
        {
            auto xamlText = ref new Platform::String(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <TimePicker x:Name='timePicker' />"
                L"    <CalendarDatePicker x:Name='calendarDatePicker' />"
                L"    <ToolTip x:Name='toolTip' />"
                L"</Grid>");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(xamlText, callback);

            // Test TimeSpans
            {
                auto timePicker = callback->GetElementByName(L"timePicker");

                // Test Microsoft.UI.Xaml.Controls.TimePicker.Time
                auto newValue = CreateInstance(L"Windows.Foundation.TimeSpan", L"6:0:0");

                SetProperty(timePicker.Handle, newValue, L"Microsoft.UI.Xaml.Controls.TimePicker.Time");

                // The time here 216000000000 is weird, but corresponds to 6 hours in the 100-nanosecond units TimeSpan.Duration represents
                // and that we set above (6:00 AM = 6 hours)
                VERIFY_ARE_EQUAL(GetProperty<Platform::IBox<::Windows::Foundation::TimeSpan>>(timePicker.Handle, L"Microsoft.UI.Xaml.Controls.TimePicker.Time")->Value.Duration, 216000000000);
            }

            /*
            // Note: the framework doesn't support creating DateTimes from strings.  If it ever does, we can enable this test
            // Test DateTime
            {
                auto calendarDatePicker = callback->GetElementByName(L"calendarDatePicker");
                auto newValue = CreateInstance(L"Windows.Foundation.DateTime", L"3/25/19");

                SetProperty(calendarDatePicker.Handle, newValue, L"Microsoft.UI.Xaml.Controls.CalendarDatePicker.Date");
            }
            */

            // Test Rect
            {
                auto toolTip = callback->GetElementByName(L"toolTip");
                auto newValue = CreateInstance(L"Windows.Foundation.Rect", L"0,1,2,3");

                SetProperty(toolTip.Handle, newValue, L"Microsoft.UI.Xaml.Controls.ToolTip.PlacementRect");
                VERIFY_ARE_EQUAL(GetProperty<Platform::IBox<::Windows::Foundation::Rect>>(toolTip.Handle, L"Microsoft.UI.Xaml.Controls.ToolTip.PlacementRect")->Value.Width, 2.0f);
            }

            // Test Point
            {
                // Any UIElement will do, so just reuse the TimePicker used for the TimeSpan test
                auto timePicker = callback->GetElementByName(L"timePicker");
                auto newValue = CreateInstance(L"Windows.Foundation.Point", L"4,5");

                SetProperty(timePicker.Handle, newValue, L"Microsoft.UI.Xaml.UIElement.RenderTransformOrigin");
                VERIFY_ARE_EQUAL(GetProperty<Platform::IBox<::Windows::Foundation::Point>>(timePicker.Handle, L"Microsoft.UI.Xaml.UIElement.RenderTransformOrigin")->Value.X, 4.0f);
            }

            // Test Double
            {
                // Any UIElement will do, so just reuse the TimePicker used for the TimeSpan test
                auto timePicker = callback->GetElementByName(L"timePicker");
                auto newValue = CreateInstance(L"Windows.Foundation.Double", L"456.0");

                SetProperty(timePicker.Handle, newValue, L"Microsoft.UI.Xaml.FrameworkElement.Height");
                VERIFY_ARE_EQUAL(GetProperty<Platform::IBox<double>>(timePicker.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Height")->Value, 456.0);
            }
        }

        void XamlDiagnosticsTests::VerifyMutationEvents()
        {
            LOG_OUTPUT(L"Scenario 1: Basic adding/remove from Grid");
            {
                auto xamlText = ref new Platform::String(
                    L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Background='Red' Height='45' Width='45'>"
                    L"</Grid>");
                wrl::ComPtr<VisualTreeServiceCallback> callback;
                auto cleanup = m_connectionHelper->Advise(xamlText, callback);
                auto root = callback->GetElementByName(L"root");
                auto rootObject = ih_cast<xaml_controls::Grid>(root.Handle);

                InstanceHandle addedButtonHandle = 0, addedEllipseHandle = 0;
                RunOnUIThread([&]() {
                    auto button = ref new xaml_controls::Button();
                    auto ellipse = ref new xaml_shapes::Ellipse();

                    addedButtonHandle = ih_cast(button);
                    addedEllipseHandle = ih_cast(ellipse);
                    rootObject->Children->Append(button);
                    rootObject->Children->Append(ellipse);
                });

                TestServices::WindowHelper->WaitForIdle();

                auto children = callback->GetChildren(root.Handle);
                VERIFY_ARE_EQUAL(2u, children.size());
                VERIFY_ARE_EQUAL(addedButtonHandle, children[0]);
                VERIFY_ARE_EQUAL(addedEllipseHandle, children[1]);
            }

            LOG_OUTPUT(L"Scenario 2: Don't call extra leave in Popup");
            {
                xaml_controls::Page^ page = nullptr;
                auto callback = m_connectionHelper->Advise();
                TestCleanupWrapper wrapper([&]
                {
                    TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
                    VERIFY_SUCCEEDED(m_tap->UnadviseVisualTreeChange(callback.Get()));
                });

                RunOnUIThread([&]()
                {
                    page = TestServices::WindowHelper->SetupSimulatedAppPage();
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    page->Frame->Navigate(shared_types::PopupPage::typeid); // WPF_HOSTING_MODE_FAILURE - has Xaml with Popup IsOpen="true"
                });
                TestServices::WindowHelper->WaitForIdle();

                // The popup calls leave on it's child, but passes in false for the "live" parameter. We should no call SignalMutation
                // when this is false.
                auto buttonInPopup = callback->GetElementByName(L"buttonInPopup");
            }
        }

        void XamlDiagnosticsTests::VerifyGetCallbackWhenNoDispatcherQueues()
        {
            // It's impossible to attach before the DXamlCore has been initialized, which is the normal scenario where we would end up
            // not having any cached dispatcher queues. What we can do is call ShutdownXaml, which will remove the queue, before we advise.
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            wrl::ComPtr<VisualTreeServiceCallback> callback = m_connectionHelper->Advise();
            TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider(), ref new CustomMetadataRegistrar<shared_types::CustomUserControl>());
            auto cleanup = XamlDiagnosticsTestHelpers::SetupGridAndWait([&] {
                test_infra::TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
                m_connectionHelper->OnTestComplete(callback);
            });
            VERIFY_IS_GREATER_THAN(callback->GetCacheSize(), 0u); // make sure we got callbacks

            CreateTemporaryInstance(L"Windows.Foundation.Numerics.Vector3", L"1,4,7");
        }

        void XamlDiagnosticsTests::ModifyMediaPlayerElementSource()
        {
            auto xamlText = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <MediaPlayerElement x:Name='theMedia' Source='ms-appx:///resources/native/foundation/graphics/Media/CastingVideo.mp4' />"
                L"</Grid>");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(xamlText, callback);

            auto mediaPlayer = callback->GetElementByName(L"theMedia");
            auto mediaPlayerElement = ih_cast<xaml_controls::MediaPlayerElement>(mediaPlayer.Handle);
            ::Windows::Media::Playback::IMediaPlaybackSource^ previousSource;
            RunOnUIThread([&]()
            {
                previousSource = mediaPlayerElement->Source;
            });
            ClearProperty(mediaPlayer.Handle, L"Microsoft.UI.Xaml.Controls.MediaPlayerElement.Source");

            RunOnUIThread([&]()
            {
                //Using DependencyObect.ClearValue to clear MediaPlayerElement.Source doesn't clear the value
                //VERIFY_IS_NULL(mediaPlayerElement->Source);
            });

            auto source = CreateInstance(L"Windows.Media.Playback.IMediaPlaybackSource", L"ms-appx:///resources/native/foundation/graphics/Media/CastingVideo.mp4");
            SetProperty(mediaPlayer.Handle, source, L"Microsoft.UI.Xaml.Controls.MediaPlayerElement.Source");
            RunOnUIThread([&]()
            {
                VERIFY_IS_NOT_NULL(mediaPlayerElement->Source);
                VERIFY_ARE_NOT_EQUAL(previousSource, mediaPlayerElement->Source);
            });
        }

        // Private Test Helpers
        HRESULT XamlDiagnosticsTests::DrawUI(Grid^ pRootGrid)
        {
            const int cColumns = 10;
            const int cRows = 10;

            Grid^ grid = ref new Grid();
            grid->Name = L"MyGrid";

            for (int i = 0; i < cColumns; ++i)
            {
                grid->ColumnDefinitions->Append(ref new ColumnDefinition());
            }

            for (int i = 0; i < cRows; ++i)
            {
                grid->RowDefinitions->Append(ref new RowDefinition());
            }

            pRootGrid->Children->Append(grid);

            // Draw lines
            const double lineThickness = 10;
            const ::Windows::UI::Color lineColor = Microsoft::UI::Colors::Red;

            for (int i = 0; i <= cColumns; ++i)
            {
                Line^ line = ref new Line();
                line->Stroke = ref new SolidColorBrush(lineColor);
                line->Y2 = 1500;

                Grid::SetRowSpan(line, cRows);

                if (i < cColumns)
                {
                    Grid::SetColumn(line, i);
                    line->StrokeThickness = lineThickness;
                }
                else
                {
                    // last line
                    Grid::SetColumn(line, cColumns - 1);
                    line->HorizontalAlignment = Microsoft::UI::Xaml::HorizontalAlignment::Right;
                    line->StrokeThickness = lineThickness / 2;
                }

                grid->Children->Append(line);
            }

            for (int i = 0; i <= cRows; ++i)
            {
                Line^ line = ref new Line();
                line->Stroke = ref new SolidColorBrush(lineColor);
                line->X2 = 1500;

                Grid::SetColumnSpan(line, cColumns);

                if (i < cRows)
                {
                    Grid::SetRow(line, i);
                    line->StrokeThickness = lineThickness;
                }
                else
                {
                    // last line
                    Grid::SetRow(line, cRows - 1);
                    line->VerticalAlignment = Microsoft::UI::Xaml::VerticalAlignment::Bottom;
                    line->StrokeThickness = lineThickness / 2;
                }

                grid->Children->Append(line);
            }

            return S_OK;
        }

        Microsoft::UI::Xaml::Tests::Common::TestCleanupWrapper XamlDiagnosticsTests::SetupGridWithCallback(wrl::ComPtr<VisualTreeServiceCallback>& callback)
        {
            callback = m_connectionHelper->Advise();

            return XamlDiagnosticsTestHelpers::SetupGridAndWait([&]{
                test_infra::TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
                m_connectionHelper->OnTestComplete(callback);
            });
        }

        Microsoft::UI::Xaml::Tests::Common::TestCleanupWrapper XamlDiagnosticsTests::LoadXamlFromFunction(const std::function<UIElement^()> func, wrl::ComPtr<VisualTreeServiceCallback>& callback)
        {
            callback = m_connectionHelper->Advise();

            Microsoft::UI::Xaml::Tests::Common::TestCleanupWrapper cleanup([&] {
                m_connectionHelper->OnTestComplete(callback);
            });

            UIElement^ uielement = func();
            Microsoft::UI::Xaml::Tests::Common::RunOnUIThread([&]()
            {
                test_infra::TestServices::WindowHelper->WindowContent = uielement;
            });

            test_infra::TestServices::WindowHelper->WaitForIdle();
            return cleanup;
        }

        std::vector<InstanceHandle> XamlDiagnosticsTests::DoHitTest(const RECT& rect)
        {
            unsigned int hitTestCount = 0;
            CoTaskMemPtr<InstanceHandle> spHitTestHandles;

            // TODO: add compatibility so that we don't run this on ui thread and that everything still
            // works
            RunOnUIThread([&](){
                VERIFY_SUCCEEDED(m_tap->HitTest(rect, &hitTestCount, &spHitTestHandles));
            });

            VERIFY_ARE_NOT_EQUAL(spHitTestHandles[0], 0);

            std::vector<InstanceHandle> handlesToReturn;
            for (unsigned int i = 0; i < hitTestCount; i++)
            {
                handlesToReturn.push_back(spHitTestHandles[i]);
            }

            return handlesToReturn;
        }
        #pragma endregion

    }
} } } } }
