// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace PropertySystem {

    class PropertySystemIntegrationTests
    {
    public:
        BEGIN_TEST_CLASS(PropertySystemIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Method") 
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(DefaultValues)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that DPs return the right default value.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SetAndGetValueDoesntLeak)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that we can set and get a value without causing a leak.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SetAndGetPropertyPath)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify we can set and then get a PropertyPath object in a custom DP.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(UnboxMatrix3D)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SetterTypeMismatchDoesNotUpdateProperty)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify setting an invalid setter value does not update the property.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ObjectIdentity)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that calling SetValue() preserves object identity for properties of type Object.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SetAndGetWithDifferentTypes)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that we can call SetValue()/GetValue() for a variety of property types.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ClearValue)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that ClearValue() reverts the effective value to the default value.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CustomDPsGetNotifiedForChangesOnly)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that custom DPs only get notified about changes when the value really changed.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(TransformsCoerceNaNValues)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that transforms coerce NaN values into zero values.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ItemsSourceReentrancy)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(EmptyStringOnTextBlockCreatesOneRun)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(FrameworkPeerReferenceFromStyleGetsProtectionFromTargetObject)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DependencyPropertyChangedFiresBeforeSelectionChangedEvent)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DependencyPropertyChangedFiresForCustomDPs)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DependencyPropertyChangedFiresSynchronously)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DependencyPropertyChangedFiresForCustomDPsWhenValueChanged)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ItemPropertiesReentrancy)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SparseBoolPropertyConvertsStringsToBooleans)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SparseIntPropertyConvertsStringsToInts)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanAssignIntTypesToInt32)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSupportInt64CustomDependencyProperty)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanUseCustomDependencyPropertyWithBadDefaultValue)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanUseCustomInterfaceDependencyProperty)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBoxAndUnBoxDateTime)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanRegisterDependencyPropertiesWithObjectOwner)
            TEST_METHOD_PROPERTY(L"Description",
                L"Validates that the parser can set dependency properties registered with Object as the owner type.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SupportsWeakRefDependencyProperties)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetFloatPropertyToThicknessThemeResource)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOverwriteValueOfResourceDictionaryWithExplicitKey)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ImageSourceDoesNotReturnStaleValue)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ThemeExpressionEvaluationDoesNotOverwriteBaseValueSource)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(NewStyleClearsThemeResourceExpression)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(GradientColorPropertyConvertsStringsToColor)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyThreadException_DependencyObject)
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // Uses multiple UWP views
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyThreadException_MultiParentShareableDO)
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // Uses multiple UWP views
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyThreadException_ShareableDO)
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // Uses multiple UWP views
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyThreadException_NoParentShareableDO)
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // Uses multiple UWP views
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyThreadException_DataBinding)
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // Uses multiple UWP views
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyThreadException_ContentControlContent)
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // Uses multiple UWP views
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyThreadException_CustomDependencyProperty)
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // Uses multiple UWP views
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetNullableProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that we can set a value on a nullable property.")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Failed to create a 'Windows.Foundation.IReference`1<Double>' from the text '5'
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(TestStyleSelector)
            TEST_METHOD_PROPERTY(L"Description",
                L"Jupiter ObjectLifetimetests (Ported from Legacy:Lifetime_Managed_UnreferencedObjectTests)")
        END_TEST_METHOD()

        TEST_METHOD(CompactEnumConversion)
    };

    ref class MyItemTemplateSelector : public Microsoft::UI::Xaml::Controls::DataTemplateSelector
    {

    };

    ref class MyItemContainerStyleSelector : public Microsoft::UI::Xaml::Controls::StyleSelector
    {

    };

    ref class CustomDataObject sealed
    {
    public:

        CustomDataObject(bool flag)
        {
            Flag = flag;
        }

        property bool Flag;
    };

    ref class CustomStyleSelector : public Microsoft::UI::Xaml::Controls::StyleSelector
    {
    public:
        Microsoft::UI::Xaml::Style^ SelectStyleCore(Platform::Object^ item, DependencyObject^ container) override
        {
            Microsoft::UI::Xaml::FrameworkElement^ element = safe_cast<Microsoft::UI::Xaml::FrameworkElement^>(container);
            CustomDataObject^ dataObj = safe_cast<CustomDataObject^>(item);

            if (element != nullptr && dataObj != nullptr)
            {
                Microsoft::UI::Xaml::Media::Brush^ brush = dataObj->Flag ?
                    ref new Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::LimeGreen) :
                    ref new Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Red);

                Microsoft::UI::Xaml::Style^ style = ref new Microsoft::UI::Xaml::Style();
                style->TargetType = Microsoft::UI::Xaml::Controls::ListViewItem::typeid;

                Setter^ bgSetter = ref new Setter();
                bgSetter->Property = Microsoft::UI::Xaml::Controls::ListViewItem::BackgroundProperty;
                bgSetter->Value = brush;

                style->Setters->Append(bgSetter);

                return style;
            }

            return nullptr;
        }
    };

} } } } } }
