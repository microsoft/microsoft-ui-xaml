// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <XamlLogging.h>

#include <StringUtilities.h>
#include <XStringUtilities.h>

#include <TypeTableStructs.h>
#include <CustomClassInfo.h>
#include <MetadataAPI.h>
#include <MockDependencyProperty.h>
#include <MockDynamicMetadataStorage.h>
#include <MockXamlMetadataProvider.h>
#include <MockClassInfo.h>
#include <ThreadLocalStorage.h>
#include <CStaticLock.h>

#include "MetadataUnitTests.h"
#include "CustomXamlProviders.h"

using namespace DirectUI;
using namespace xaml_interop;
using namespace xaml_markup;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Metadata {

    DECLARE_CONST_STRING_IN_TEST_CODE(c_NameDependencyObject, L"DependencyObject");
    DECLARE_CONST_STRING_IN_TEST_CODE(c_NameControl, L"Control");
    DECLARE_CONST_STRING_IN_TEST_CODE(c_FullNameDependencyObject, L"Microsoft.UI.Xaml.DependencyObject");
    DECLARE_CONST_STRING_IN_TEST_CODE(c_FullNameControl, L"Microsoft.UI.Xaml.Controls.Control");

    DECLARE_CONST_STRING_IN_TEST_CODE(c_NamePoint, L"Point");
    DECLARE_CONST_STRING_IN_TEST_CODE(c_NameRect, L"Rect");
    DECLARE_CONST_STRING_IN_TEST_CODE(c_NameSize, L"Size");

    DECLARE_CONST_STRING_IN_TEST_CODE(c_Name, L"Name");
    DECLARE_CONST_STRING_IN_TEST_CODE(c_Width, L"Width");

    bool MetadataUnitTests::ClassSetup()
    {
        THROW_IF_FAILED(StaticLockGlobalInit());
        return true;
    }

    bool MetadataUnitTests::ClassCleanup()
    {
        StaticLockGlobalDeinit();
        return true;
    }

    // Verify the common storage access (DynamicMetadataStorageInstanceWithLock) locks and unlocks automatically.
    void MetadataUnitTests::StorageAccessIsThreadSafe()
    {
        auto mock = TlsProvider<DynamicMetadataStorageMock>::CreateWrappedObject();

        // Verify member access via DynamicMetadataStorageInstanceWithLock
        {
            DynamicMetadataStorageInstanceWithLock storage;
            VERIFY_IS_TRUE(CStaticLock::IsLocked());
            auto customPropertiesCache = storage->m_customPropertiesCache;
            VERIFY_IS_TRUE(mock->LastAccessWasLocked, L"DynamicMetadataStorage accessed with a lock");
        }

        // Verify state after DynamicMetadataStorageInstanceWithLock destruction
        VERIFY_IS_FALSE(CStaticLock::IsLocked());

        // Verify access on Reset
        mock->LastAccessWasLocked = false;
        DynamicMetadataStorageInstanceWithLock::Reset();
        VERIFY_IS_TRUE(mock->LastAccessWasLocked, L"DynamicMetadataStorage accessed with a lock");
    }

    void MetadataUnitTests::CanResolveCustomTypeByTypeName()
    {
        wrl_wrappers::HStringReference customTypeStringRef(L"CustomNamespace.CustomType");

        auto customType = MockXamlType::CreateMetadata(customTypeStringRef.Get())
            ->WithBaseType(KnownTypeIndex::Object);

        MockXamlMetadataProvider provider;
        provider.GetXamlTypeByFullNameCallback = [&customTypeStringRef, customType](HSTRING fullName, IXamlType** ppXamlType) -> HRESULT
        {
            if (fullName == customTypeStringRef)
            {
                return customType.CopyTo(ppXamlType);
            }

            *ppXamlType = nullptr;
            return S_OK;
        };

        wxaml_interop::TypeName customTypeName = { customTypeStringRef.Get(), wxaml_interop::TypeKind_Metadata };
        const CClassInfo* resolvedType = nullptr;
        VERIFY_SUCCEEDED(MetadataAPI::GetClassInfoByTypeName(customTypeName, &resolvedType));
        VERIFY_IS_NOT_NULL(resolvedType);
        VERIFY_ARE_EQUAL(customType.Get(), static_cast<const CCustomClassInfo*>(resolvedType)->GetXamlTypeNoRef());
    }

    void MetadataUnitTests::CanResolveDirectiveOnCustomType()
    {
        auto mock = TlsProvider<DynamicMetadataStorageMock>::CreateWrappedObject();

        CClassInfo customType;
        customType.m_nIndex = static_cast<KnownTypeIndex>(KnownTypeCount);
        customType.m_nBaseTypeIndex = KnownTypeIndex::Control;

        const CDependencyProperty* result;
        VERIFY_SUCCEEDED(MetadataAPI::TryGetDependencyPropertyByName(&customType, c_Name, &result, /* allowDirectives */ true));
        VERIFY_ARE_EQUAL(KnownPropertyIndex::DependencyObject_Name, result->GetIndex());
    }

    void MetadataUnitTests::IsAssignableFrom()
    {
        // Positive tests.
        VERIFY_IS_TRUE(!!MetadataAPI::IsAssignableFrom(KnownTypeIndex::DependencyObject, KnownTypeIndex::FrameworkElement));
        VERIFY_IS_TRUE(!!MetadataAPI::IsAssignableFrom(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::DependencyObject), MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::FrameworkElement)));

        VERIFY_IS_TRUE(!!MetadataAPI::IsAssignableFrom(KnownTypeIndex::DependencyObject, KnownTypeIndex::Control));
        VERIFY_IS_TRUE(!!MetadataAPI::IsAssignableFrom(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::DependencyObject), MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Control)));

        VERIFY_IS_TRUE(!!MetadataAPI::IsAssignableFrom(KnownTypeIndex::DependencyObject, KnownTypeIndex::String));
        VERIFY_IS_TRUE(!!MetadataAPI::IsAssignableFrom(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::DependencyObject), MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::String)));

        // Negative tests.
        VERIFY_IS_FALSE(!!MetadataAPI::IsAssignableFrom(KnownTypeIndex::Control, KnownTypeIndex::DependencyObject));
        VERIFY_IS_FALSE(!!MetadataAPI::IsAssignableFrom(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Control), MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::DependencyObject)));

        VERIFY_IS_FALSE(!!MetadataAPI::IsAssignableFrom(KnownTypeIndex::FrameworkElement, KnownTypeIndex::Application));
        VERIFY_IS_FALSE(!!MetadataAPI::IsAssignableFrom(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::FrameworkElement), MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Application)));

        // If target type index is KnownTypeIndex::Object, anything can be assigned to it.
        VERIFY_IS_TRUE(!!MetadataAPI::IsAssignableFrom(KnownTypeIndex::Object, KnownTypeIndex::ICommand));
        VERIFY_IS_TRUE(!!MetadataAPI::IsAssignableFrom(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Object), MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::ICommand)));

        // enum has no type handle
        VERIFY_IS_TRUE(!!MetadataAPI::IsAssignableFrom(KnownTypeIndex::Stretch, KnownTypeIndex::Stretch));
        VERIFY_IS_TRUE(!!MetadataAPI::IsAssignableFrom(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Stretch), MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Stretch)));

        VERIFY_IS_FALSE(!!MetadataAPI::IsAssignableFrom(KnownTypeIndex::DependencyObject, KnownTypeIndex::Stretch));
        VERIFY_IS_FALSE(!!MetadataAPI::IsAssignableFrom(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::DependencyObject), MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Stretch)));

        VERIFY_IS_FALSE(!!MetadataAPI::IsAssignableFrom(KnownTypeIndex::Stretch, KnownTypeIndex::SolidColorBrush));
        VERIFY_IS_FALSE(!!MetadataAPI::IsAssignableFrom(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Stretch), MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::SolidColorBrush)));

        VERIFY_IS_FALSE(!!MetadataAPI::IsAssignableFrom(KnownTypeIndex::SolidColorBrush, KnownTypeIndex::Stretch));
        VERIFY_IS_FALSE(!!MetadataAPI::IsAssignableFrom(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::SolidColorBrush), MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Stretch)));

        VERIFY_IS_FALSE(!!MetadataAPI::IsAssignableFrom(KnownTypeIndex::Enumerated, KnownTypeIndex::Stretch));
        VERIFY_IS_FALSE(!!MetadataAPI::IsAssignableFrom(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Enumerated), MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Stretch)));

        VERIFY_IS_FALSE(!!MetadataAPI::IsAssignableFrom(KnownTypeIndex::Stretch, KnownTypeIndex::Enumerated));
        VERIFY_IS_FALSE(!!MetadataAPI::IsAssignableFrom(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Stretch), MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Enumerated)));

        // 32-bit type handle cusp
        VERIFY_IS_TRUE(!!MetadataAPI::IsAssignableFrom(KnownTypeIndex::ListViewBase, KnownTypeIndex::ListView));
        VERIFY_IS_TRUE(!!MetadataAPI::IsAssignableFrom(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::ListViewBase), MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::ListView)));

        // Internal hierarchy leaking to the outside world.  Yes, Duration, RepeatBehavior and KeyTime can be assigned to TimeSpan...
        VERIFY_IS_TRUE(!!MetadataAPI::IsAssignableFrom(KnownTypeIndex::TimeSpan, KnownTypeIndex::Duration));
        VERIFY_IS_TRUE(!!MetadataAPI::IsAssignableFrom(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::TimeSpan), MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Duration)));

        VERIFY_IS_TRUE(!!MetadataAPI::IsAssignableFrom(KnownTypeIndex::TimeSpan, KnownTypeIndex::RepeatBehavior));
        VERIFY_IS_TRUE(!!MetadataAPI::IsAssignableFrom(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::TimeSpan), MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::RepeatBehavior)));

        VERIFY_IS_TRUE(!!MetadataAPI::IsAssignableFrom(KnownTypeIndex::Duration, KnownTypeIndex::RepeatBehavior));
        VERIFY_IS_TRUE(!!MetadataAPI::IsAssignableFrom(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Duration), MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::RepeatBehavior)));

        VERIFY_IS_TRUE(!!MetadataAPI::IsAssignableFrom(KnownTypeIndex::TimeSpan, KnownTypeIndex::KeyTime));
        VERIFY_IS_TRUE(!!MetadataAPI::IsAssignableFrom(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::TimeSpan), MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::KeyTime)));
    }

    void MetadataUnitTests::GetClassInfoByName()
    {
        const CClassInfo* pType = nullptr;
        VERIFY_SUCCEEDED(MetadataAPI::GetClassInfoByFullName(c_NameDependencyObject, &pType));
        VERIFY_ARE_EQUAL(KnownTypeIndex::DependencyObject, pType->GetIndex());

        VERIFY_SUCCEEDED(MetadataAPI::GetClassInfoByFullName(c_NameControl, &pType));
        VERIFY_ARE_EQUAL(KnownTypeIndex::Control, pType->GetIndex());
    }

    void MetadataUnitTests::GetClassInfoByFullName()
    {
        const CClassInfo* pType = nullptr;
        VERIFY_SUCCEEDED(MetadataAPI::GetClassInfoByFullName(c_FullNameDependencyObject, &pType));
        VERIFY_ARE_EQUAL(KnownTypeIndex::DependencyObject, pType->GetIndex());

        VERIFY_SUCCEEDED(MetadataAPI::GetClassInfoByFullName(c_FullNameControl, &pType));
        VERIFY_ARE_EQUAL(KnownTypeIndex::Control, pType->GetIndex());
    }

    void MetadataUnitTests::GetTypeNameByClassInfo()
    {
        wxaml_interop::TypeName typeNameDO = {};
        VERIFY_SUCCEEDED(MetadataAPI::GetTypeNameByClassInfo(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::DependencyObject), &typeNameDO));
        VERIFY_ARE_EQUAL(wxaml_interop::TypeKind::TypeKind_Metadata, typeNameDO.Kind);
        VERIFY_ARE_STRINGS_EQUAL(L"Microsoft.UI.Xaml.DependencyObject", typeNameDO.Name);

        wxaml_interop::TypeName typeNameInt32 = {};
        VERIFY_SUCCEEDED(MetadataAPI::GetTypeNameByClassInfo(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Int32), &typeNameInt32));
        VERIFY_ARE_EQUAL(wxaml_interop::TypeKind::TypeKind_Primitive, typeNameInt32.Kind);
        VERIFY_ARE_STRINGS_EQUAL(L"Int32", typeNameInt32.Name);

        WindowsDeleteString(typeNameInt32.Name);
        WindowsDeleteString(typeNameDO.Name);
    }

    void MetadataUnitTests::IsConstructible()
    {
        VERIFY_IS_TRUE(!!MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::DependencyObject)->IsConstructible());
        VERIFY_IS_FALSE(!!MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::EventHandlerStub)->IsConstructible());
    }

    void MetadataUnitTests::BaseTypes()
    {
        VERIFY_ARE_EQUAL(KnownTypeIndex::UnknownType, MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Object)->GetBaseType()->GetIndex());
        VERIFY_ARE_EQUAL(KnownTypeIndex::Object, MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::DependencyObject)->GetBaseType()->GetIndex());
        VERIFY_ARE_EQUAL(KnownTypeIndex::UIElement, MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::FrameworkElement)->GetBaseType()->GetIndex());
        VERIFY_ARE_EQUAL(KnownTypeIndex::UnknownType, MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::IVectorOfPageStackEntry)->GetBaseType()->GetIndex());
    }

    void MetadataUnitTests::ValidateISupportInitializeFlagOnTypes()
    {
        VERIFY_IS_TRUE(!!MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Selector)->IsISupportInitialize());
        VERIFY_IS_TRUE(!!MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::GridView)->IsISupportInitialize());
    }

    void MetadataUnitTests::DependencyObjectIsInGoodState()
    {
        const CClassInfo* pType = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::DependencyObject);
        VERIFY_ARE_EQUAL(KnownPropertyIndex::UnknownType_UnknownProperty, pType->GetContentProperty()->GetIndex());
        VERIFY_ARE_EQUAL(KnownNamespaceIndex::Microsoft_UI_Xaml, pType->GetNamespace()->GetIndex());
        VERIFY_ARE_STRINGS_EQUAL(L"DependencyObject", pType->GetName());
        VERIFY_ARE_STRINGS_EQUAL(L"Microsoft.UI.Xaml.DependencyObject", pType->GetFullName());
        VERIFY_IS_FALSE(!!pType->HasTypeConverter());
        VERIFY_IS_FALSE(!!pType->IsBindable());
        VERIFY_IS_TRUE(!!pType->IsBuiltinType());
        VERIFY_IS_FALSE(!!pType->IsCollection());
        VERIFY_IS_TRUE(!!pType->IsConstructible());
        VERIFY_IS_FALSE(!!pType->IsDictionary());
        VERIFY_IS_FALSE(!!pType->IsInterface());
        VERIFY_IS_FALSE(!!pType->IsEnum());
        VERIFY_IS_FALSE(!!pType->IsISupportInitialize());
        VERIFY_IS_FALSE(!!pType->IsMarkupExtension());
        VERIFY_IS_TRUE(!!pType->IsNullable());
        VERIFY_IS_FALSE(!!pType->IsNumericType());
        VERIFY_IS_FALSE(!!pType->IsPrimitive());
        VERIFY_IS_FALSE(!!pType->IsValueType());
        VERIFY_IS_FALSE(!!pType->IsWhitespaceSignificant());
        VERIFY_IS_FALSE(!!pType->TrimSurroundingWhitespace());
    }

    void MetadataUnitTests::GetPrimitiveClassInfo()
    {
        const CClassInfo* pNormalizedType = nullptr;

        // DO should normalize to Object.
        const CClassInfo* pTypeDO = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::DependencyObject);
        VERIFY_SUCCEEDED(MetadataAPI::GetPrimitiveClassInfo(pTypeDO, &pNormalizedType));
        VERIFY_ARE_EQUAL(KnownTypeIndex::Object, pNormalizedType->GetIndex());

        // Int32 should normalize to Int32.
        const CClassInfo* pTypeInt32 = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Int32);
        VERIFY_SUCCEEDED(MetadataAPI::GetPrimitiveClassInfo(pTypeInt32, &pNormalizedType));
        VERIFY_ARE_EQUAL(KnownTypeIndex::Int32, pNormalizedType->GetIndex());
    }

    void MetadataUnitTests::CanExtractNamespaceNameAndShortName()
    {
        xephemeral_string_ptr namespaceName, typeName;

        DECLARE_CONST_STRING_IN_TEST_CODE(c_fullTypeName1, L"FooNamespace.FooType");
        VERIFY_SUCCEEDED(MetadataAPI::ExtractNamespaceNameAndShortName(c_fullTypeName1, &namespaceName, &typeName));
        VERIFY_IS_TRUE(!!namespaceName.Equals(L"FooNamespace"));
        VERIFY_IS_TRUE(!!typeName.Equals(L"FooType"));

        DECLARE_CONST_STRING_IN_TEST_CODE(c_fullTypeName2, L"FooNamespace.FooSubNamespace.FooType");
        VERIFY_SUCCEEDED(MetadataAPI::ExtractNamespaceNameAndShortName(c_fullTypeName2, &namespaceName, &typeName));
        VERIFY_IS_TRUE(!!namespaceName.Equals(L"FooNamespace.FooSubNamespace"));
        VERIFY_IS_TRUE(!!typeName.Equals(L"FooType"));

        DECLARE_CONST_STRING_IN_TEST_CODE(c_fullTypeName3, L"FooNamespace.FooType<BarType>");
        VERIFY_SUCCEEDED(MetadataAPI::ExtractNamespaceNameAndShortName(c_fullTypeName3, &namespaceName, &typeName));
        VERIFY_IS_TRUE(!!namespaceName.Equals(L"FooNamespace"));
        VERIFY_IS_TRUE(!!typeName.Equals(L"FooType<BarType>"));

        DECLARE_CONST_STRING_IN_TEST_CODE(c_fullTypeName4, L"FooNamespace.FooSubNamespace.FooType<BarType>");
        VERIFY_SUCCEEDED(MetadataAPI::ExtractNamespaceNameAndShortName(c_fullTypeName4, &namespaceName, &typeName));
        VERIFY_IS_TRUE(!!namespaceName.Equals(L"FooNamespace.FooSubNamespace"));
        VERIFY_IS_TRUE(!!typeName.Equals(L"FooType<BarType>"));

        DECLARE_CONST_STRING_IN_TEST_CODE(c_fullTypeName5, L"FooNamespace.FooType<BarNamespace.BarType>");
        VERIFY_SUCCEEDED(MetadataAPI::ExtractNamespaceNameAndShortName(c_fullTypeName5, &namespaceName, &typeName));
        VERIFY_IS_TRUE(!!namespaceName.Equals(L"FooNamespace"));
        VERIFY_IS_TRUE(!!typeName.Equals(L"FooType<BarNamespace.BarType>"));

        DECLARE_CONST_STRING_IN_TEST_CODE(c_fullTypeName6, L"FooNamespace.FooSubNamespace.FooType<BarNamespace.BarType>");
        VERIFY_SUCCEEDED(MetadataAPI::ExtractNamespaceNameAndShortName(c_fullTypeName6, &namespaceName, &typeName));
        VERIFY_IS_TRUE(!!namespaceName.Equals(L"FooNamespace.FooSubNamespace"));
        VERIFY_IS_TRUE(!!typeName.Equals(L"FooType<BarNamespace.BarType>"));

        DECLARE_CONST_STRING_IN_TEST_CODE(c_fullTypeName7, L"FooNamespace.FooType<BarNamespace.BarSubNamespace.BarType>");
        VERIFY_SUCCEEDED(MetadataAPI::ExtractNamespaceNameAndShortName(c_fullTypeName7, &namespaceName, &typeName));
        VERIFY_IS_TRUE(!!namespaceName.Equals(L"FooNamespace"));
        VERIFY_IS_TRUE(!!typeName.Equals(L"FooType<BarNamespace.BarSubNamespace.BarType>"));

        DECLARE_CONST_STRING_IN_TEST_CODE(c_fullTypeName8, L"FooNamespace.FooSubNamespace.FooType<BarNamespace.BarSubNamespace.BarType>");
        VERIFY_SUCCEEDED(MetadataAPI::ExtractNamespaceNameAndShortName(c_fullTypeName8, &namespaceName, &typeName));
        VERIFY_IS_TRUE(!!namespaceName.Equals(L"FooNamespace.FooSubNamespace"));
        VERIFY_IS_TRUE(!!typeName.Equals(L"FooType<BarNamespace.BarSubNamespace.BarType>"));
    }

    void MetadataUnitTests::IXamlMemberTypeMayReturnNull()
    {
        auto mock = TlsProvider<DynamicMetadataStorageMock>::CreateWrappedObject();

        IXamlMemberTypeMayReturnNull_XamlMember member;
        const CDependencyProperty* pResult = nullptr;

        // Import a custom property.
        const CClassInfo* pType = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::DependencyObject);
        VERIFY_ARE_EQUAL(S_OK, MetadataAPI::ImportPropertyInfo(pType, &member, &pResult));
    }

    void MetadataUnitTests::FoundationTypesInCorrectNamespace()
    {
        const CClassInfo* pPointType = MetadataAPI::GetBuiltinClassInfoByName(c_NamePoint);
        VERIFY_ARE_EQUAL(KnownNamespaceIndex::Windows_Foundation, pPointType->GetNamespace()->m_nIndex);

        const CClassInfo* pRectType = MetadataAPI::GetBuiltinClassInfoByName(c_NameRect);
        VERIFY_ARE_EQUAL(KnownNamespaceIndex::Windows_Foundation, pRectType->GetNamespace()->m_nIndex);

        const CClassInfo* pSizeType = MetadataAPI::GetBuiltinClassInfoByName(c_NameSize);
        VERIFY_ARE_EQUAL(KnownNamespaceIndex::Windows_Foundation, pSizeType->GetNamespace()->m_nIndex);
    }

    void MetadataUnitTests::RequiresPeerActivation()
    {
        // Types that currently have custom logic/state in the framework peer.
        VERIFY_IS_TRUE(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::EntranceThemeTransition)->RequiresPeerActivation());
        VERIFY_IS_TRUE(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::AddDeleteThemeTransition)->RequiresPeerActivation());
        VERIFY_IS_TRUE(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Binding)->RequiresPeerActivation());
        VERIFY_IS_TRUE(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Button)->RequiresPeerActivation());
        VERIFY_IS_TRUE(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::TextBox)->RequiresPeerActivation());
        VERIFY_IS_TRUE(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::UserControl)->RequiresPeerActivation());
        VERIFY_IS_TRUE(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::ContentControl)->RequiresPeerActivation());
        VERIFY_IS_TRUE(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::ItemsControl)->RequiresPeerActivation());
        VERIFY_IS_TRUE(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::ListView)->RequiresPeerActivation());

        // Types that don't have custom logic/state in the framework peer.
        VERIFY_IS_FALSE(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::DependencyObject)->RequiresPeerActivation());
        VERIFY_IS_FALSE(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::TextBlock)->RequiresPeerActivation());
        VERIFY_IS_FALSE(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Control)->RequiresPeerActivation());
        VERIFY_IS_FALSE(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::ContentPresenter)->RequiresPeerActivation());
        VERIFY_IS_FALSE(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::StackPanel)->RequiresPeerActivation());
        VERIFY_IS_FALSE(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Grid)->RequiresPeerActivation());
        VERIFY_IS_FALSE(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Canvas)->RequiresPeerActivation());
        VERIFY_IS_FALSE(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Border)->RequiresPeerActivation());
        VERIFY_IS_FALSE(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Underline)->RequiresPeerActivation());
    }

    void MetadataUnitTests::GetStorageType()
    {
        CDependencyProperty dp;

        dp.SetPropertyTypeIndex(KnownTypeIndex::String);
        VERIFY_ARE_EQUAL(valueString, dp.GetStorageType());

        dp.SetPropertyTypeIndex(KnownTypeIndex::Uri);
        VERIFY_ARE_EQUAL(valueString, dp.GetStorageType());

        dp.SetPropertyTypeIndex(KnownTypeIndex::Int32);
        VERIFY_ARE_EQUAL(valueSigned, dp.GetStorageType());

        dp.SetPropertyTypeIndex(KnownTypeIndex::Double);
        dp.SetIndex(KnownPropertyIndex::TimeSpan_Seconds);
        VERIFY_ARE_EQUAL(valueDouble, dp.GetStorageType());
    }

    void MetadataUnitTests::GetOffset()
    {
        MockDependencyProperty dp;

        dp.offset = 42;
        VERIFY_ARE_EQUAL(42, dp.GetOffset());
    }

    void MetadataUnitTests::GetGroupOffset()
    {
        MockDependencyProperty dp;

        dp.groupOffset = 42;
        VERIFY_ARE_EQUAL(42, dp.GetGroupOffset());
    }

    void MetadataUnitTests::ReRegisteringBuiltinDPDoesNotAV()
    {
        auto mock = TlsProvider<DynamicMetadataStorageMock>::CreateWrappedObject();

        MockDependencyProperty dp;

        VERIFY_SUCCEEDED(MetadataAPI::AssociateDependencyProperty(
            MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::FrameworkElement),
            &dp));

        // Register one more time.
        VERIFY_SUCCEEDED(MetadataAPI::AssociateDependencyProperty(
            MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::FrameworkElement),
            &dp));

        VERIFY_ARE_EQUAL(1, mock->GetStorage()->m_customDPsByTypeAndNameCache->size());
    }

    void MetadataUnitTests::ReRegisteringCustomDPSetsUnderlyingDP()
    {
        DECLARE_CONST_STRING_IN_TEST_CODE(c_fooPropertyName, L"Foo");

        auto mock = TlsProvider<DynamicMetadataStorageMock>::CreateWrappedObject();

        CCustomProperty* customProp = nullptr;
        VERIFY_SUCCEEDED(CCustomProperty::Create(
            mock->GetStorage()->GetNextAvailablePropertyIndex(),
            KnownTypeIndex::DependencyObject,
            KnownTypeIndex::FrameworkElement,
            MetaDataPropertyInfoFlags::None,
            nullptr,
            c_fooPropertyName,
            &customProp));

        VERIFY_ARE_EQUAL(nullptr, mock->GetStorage()->m_customDPsByTypeAndNameCache);

        // Register "Foo"
        VERIFY_SUCCEEDED(MetadataAPI::AssociateDependencyProperty(
            MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::FrameworkElement),
            customProp));
        VERIFY_ARE_EQUAL(1, mock->GetStorage()->m_customDPsByTypeAndNameCache->size());

        // Verify stored property
        DynamicMetadataStorage::PropertiesTable* propertiesMap
            = mock->GetStorage()->m_customDPsByTypeAndNameCache->find(KnownTypeIndex::FrameworkElement)->second.get();
        const CDependencyProperty* storedProperty1 = propertiesMap->find(c_fooPropertyName)->second;
        VERIFY_ARE_EQUAL(storedProperty1, customProp);

        // Register "Foo" again with different prop
        CCustomProperty* customProp2 = nullptr;
        VERIFY_SUCCEEDED(CCustomProperty::Create(
            mock->GetStorage()->GetNextAvailablePropertyIndex(),
            KnownTypeIndex::DependencyObject,
            KnownTypeIndex::FrameworkElement,
            MetaDataPropertyInfoFlags::None,
            nullptr,
            c_fooPropertyName,
            &customProp2));

        VERIFY_SUCCEEDED(MetadataAPI::AssociateDependencyProperty(
            MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::FrameworkElement),
            customProp2));

        VERIFY_ARE_EQUAL(1, mock->GetStorage()->m_customDPsByTypeAndNameCache->size());

        // Verify stored property is still the original property
        const CDependencyProperty* storedProperty2 = propertiesMap->find(c_fooPropertyName)->second;
        VERIFY_ARE_EQUAL(storedProperty2, customProp2);

        // Verify underlying DP was set
        const CDependencyProperty* underlyingProp = nullptr;
        VERIFY_SUCCEEDED(customProp->TryGetUnderlyingDP(&underlyingProp));
        VERIFY_ARE_EQUAL(underlyingProp, customProp2);
    }

    void MetadataUnitTests::RunClassConstructorIsDelayed()
    {
        auto mock1 = TlsProvider<ClassInfoCallbacks>::CreateWrappedObject();
        bool constructorCalled = false;
        mock1->RunClassConstructorIfNecessary = [&constructorCalled]() -> HRESULT
        {
            constructorCalled = true;
            return S_OK;
        };

        IXamlMemberTypeMayReturnNull_XamlMember member;
        const CDependencyProperty* pResult = nullptr;

        // Import a custom property.
        wrl_wrappers::HStringReference customTypeStringRef(L"CustomNamespace.CustomType");

        auto customType = MockXamlType::CreateMetadata(customTypeStringRef.Get())
            ->WithBaseType(KnownTypeIndex::Object);

        MockXamlMetadataProvider provider;
        provider.GetXamlTypeByFullNameCallback = [&customTypeStringRef, customType](HSTRING fullName, IXamlType** ppXamlType) -> HRESULT
        {
            if (fullName == customTypeStringRef)
            {
                return customType.CopyTo(ppXamlType);
            }

            *ppXamlType = nullptr;
            return S_OK;
        };

        wxaml_interop::TypeName customTypeName = { customTypeStringRef.Get(), wxaml_interop::TypeKind_Metadata };
        const CClassInfo* resolvedType = nullptr;
    }

    void MetadataUnitTests::RunClassConstructorIsNotDelayed()
    {
        auto mock1 = TlsProvider<ClassInfoCallbacks>::CreateWrappedObject();
        bool constructorCalled = false;
        mock1->RunClassConstructorIfNecessary = [&constructorCalled]() -> HRESULT
        {
            constructorCalled = true;
            return S_OK;
        };

        IXamlMemberTypeMayReturnNull_XamlMember member;
        const CDependencyProperty* pResult = nullptr;

        // Import a custom property.
        wrl_wrappers::HStringReference customTypeStringRef(L"CustomNamespace.CustomType");

        auto customType = MockXamlType::CreateMetadata(customTypeStringRef.Get())
            ->WithBaseType(KnownTypeIndex::Object);

        MockXamlMetadataProvider provider;
        provider.GetXamlTypeByFullNameCallback = [&customTypeStringRef, customType](HSTRING fullName, IXamlType** ppXamlType) -> HRESULT
        {
            if (fullName == customTypeStringRef)
            {
                return customType.CopyTo(ppXamlType);
            }

            *ppXamlType = nullptr;
            return S_OK;
        };

        wxaml_interop::TypeName customTypeName = { customTypeStringRef.Get(), wxaml_interop::TypeKind_Metadata };
        const CClassInfo* resolvedType = nullptr;
    }

    void MetadataUnitTests::TestPropertyGetters()
    {
        DECLARE_CONST_STRING_IN_TEST_CODE(c_customPropertyName, L"CustomProperty");
        DECLARE_CONST_STRING_IN_TEST_CODE(c_customDependencyPropertyName, L"CustomDependencyProperty");

        auto mock = TlsProvider<DynamicMetadataStorageMock>::CreateWrappedObject();

        CCustomProperty* customProp = nullptr;

        VERIFY_SUCCEEDED(CCustomProperty::Create(
            mock->GetStorage()->GetNextAvailablePropertyIndex(),
            KnownTypeIndex::DependencyObject,
            KnownTypeIndex::FrameworkElement,
            MetaDataPropertyInfoFlags::None,
            nullptr,
            c_customPropertyName,
            &customProp));

        VERIFY_IS_TRUE(customProp->Is<CCustomProperty>());
        VERIFY_IS_TRUE(customProp->Is<CDependencyProperty>());
        VERIFY_IS_FALSE(customProp->Is<CCustomDependencyProperty>());
        VERIFY_IS_FALSE(customProp->Is<CSimpleProperty>());
        VERIFY_ARE_EQUAL(customProp, customProp->AsOrNull<CCustomProperty>());
        VERIFY_ARE_EQUAL(customProp, customProp->AsOrNull<CDependencyProperty>());
        VERIFY_IS_NULL(customProp->AsOrNull<CCustomDependencyProperty>());
        VERIFY_IS_NULL(customProp->AsOrNull<CSimpleProperty>());

        CCustomDependencyProperty* customDependencyProp = nullptr;

        VERIFY_SUCCEEDED(CCustomDependencyProperty::Create(
            mock->GetStorage()->GetNextAvailablePropertyIndex(),
            MetaDataPropertyInfoFlags::None,
            c_customDependencyPropertyName,
            &customDependencyProp));

        VERIFY_IS_FALSE(customDependencyProp->Is<CCustomProperty>());
        VERIFY_IS_TRUE(customDependencyProp->Is<CDependencyProperty>());
        VERIFY_IS_TRUE(customDependencyProp->Is<CCustomDependencyProperty>());
        VERIFY_IS_FALSE(customDependencyProp->Is<CSimpleProperty>());
        VERIFY_IS_NULL(customDependencyProp->AsOrNull<CCustomProperty>());
        VERIFY_ARE_EQUAL(customDependencyProp, customDependencyProp->AsOrNull<CDependencyProperty>());
        VERIFY_ARE_EQUAL(customDependencyProp, customDependencyProp->AsOrNull<CCustomDependencyProperty>());
        VERIFY_IS_NULL(customDependencyProp->AsOrNull<CSimpleProperty>());

        const CPropertyBase* builtInProperty = MetadataAPI::GetPropertyBaseByIndex(KnownPropertyIndex::DependencyObject_Name);

        VERIFY_IS_FALSE(builtInProperty->Is<CCustomProperty>());
        VERIFY_IS_TRUE(builtInProperty->Is<CDependencyProperty>());
        VERIFY_IS_FALSE(builtInProperty->Is<CCustomDependencyProperty>());
        VERIFY_IS_FALSE(builtInProperty->Is<CSimpleProperty>());
        VERIFY_IS_NULL(builtInProperty->AsOrNull<CCustomProperty>());
        VERIFY_ARE_EQUAL(builtInProperty, builtInProperty->AsOrNull<CDependencyProperty>());
        VERIFY_IS_NULL(builtInProperty->AsOrNull<CCustomDependencyProperty>());
        VERIFY_IS_NULL(builtInProperty->AsOrNull<CSimpleProperty>());

        {
            auto prop = MetadataAPI::GetPropertyBaseByIndex(KnownPropertyIndex::DependencyObject_Name);
            VERIFY_ARE_EQUAL(builtInProperty, prop);
        }

        {
            auto prop = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::DependencyObject_Name);
            VERIFY_ARE_EQUAL(builtInProperty, prop);
        }

        {
            const CPropertyBase* prop = MetadataAPI::TryGetBuiltInPropertyBaseByName(
                MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::DependencyObject),
                c_Name,
                true);
            VERIFY_ARE_EQUAL(builtInProperty, prop);
        }

        {
            const CPropertyBase* prop = MetadataAPI::TryGetBuiltInPropertyBaseByName(
                MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::FrameworkElement),
                c_customPropertyName);
            VERIFY_IS_NULL(prop);
        }

        {
            const CDependencyProperty* prop = nullptr;

            VERIFY_SUCCEEDED(MetadataAPI::TryGetDependencyPropertyByName(
                MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::DependencyObject),
                c_Name,
                &prop,
                true));

            VERIFY_ARE_EQUAL(builtInProperty, prop);
        }
    }
} } } } }