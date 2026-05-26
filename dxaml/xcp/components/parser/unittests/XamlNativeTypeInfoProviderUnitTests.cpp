// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XamlNativeTypeInfoProviderUnitTests.h"
#include "ParserUtilities.h"
#include "ParserUnitTestIncludes.h"
#include "MockParserCoreServices.h"
#include <XamlLogging.h>
#include <ThreadLocalStorage.h>
#include <MockDynamicMetadataStorage.h>

using namespace DirectUI;
using namespace Microsoft::UI::Xaml::Tests::Metadata;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    bool XamlNativeTypeInfoProviderUnitTests::ClassSetup()
    {
        THROW_IF_FAILED(StaticLockGlobalInit());
        return true;
    }

    bool XamlNativeTypeInfoProviderUnitTests::ClassCleanup()
    {
        StaticLockGlobalDeinit();
        return true;
    }

    void XamlNativeTypeInfoProviderUnitTests::Create()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);
        auto spNativeProvider = GetNativeTypeInfoProvider(spSchemaContext);

        VERIFY_IS_NOT_NULL(spNativeProvider);
    }

    void XamlNativeTypeInfoProviderUnitTests::ResolveAssembly()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);
        auto spNativeProvider = GetNativeTypeInfoProvider(spSchemaContext);
        XamlAssemblyToken assemblyToken;
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strMyAssemblyName, L"System.Windows");

        // TODO: Remove assembly concept from parser because it is redundant. Currently
        // the managed provider has no resolution capability, so this is expected to fail.
        VERIFY_SUCCEEDED(spNativeProvider->ResolveAssembly(c_strMyAssemblyName, assemblyToken));
        VERIFY_IS_TRUE(assemblyToken == XamlAssemblyToken(tpkNative, 1));
    }

    void XamlNativeTypeInfoProviderUnitTests::GetTypeNamespace()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);
        auto spNativeProvider = GetNativeTypeInfoProvider(spSchemaContext);

        // verify we can resolve known type namespaces
        {
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strKnownTypenamespace, L"Microsoft.UI.Xaml.Controls");
            XamlTypeNamespaceToken typeNamespaceToken;
            XamlTypeNamespaceToken controlsNamespaceToken(tpkNative, KnownNamespaceIndex::Microsoft_UI_Xaml_Controls);

            VERIFY_SUCCEEDED(spNativeProvider->GetTypeNamespace(c_strKnownTypenamespace, typeNamespaceToken));
            VERIFY_IS_TRUE(typeNamespaceToken.GetHandle() == controlsNamespaceToken.GetHandle());
        }
    }

    void XamlNativeTypeInfoProviderUnitTests::GetTypeNamespaceForType()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);
        auto spNativeProvider = GetNativeTypeInfoProvider(spSchemaContext);

        // verify we can resolve known types
        {
            xstring_ptr typeNamespaceName;
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strControlsNamespace, L"Microsoft.UI.Xaml.Controls");
            XamlTypeNamespaceToken typeNamespaceToken;
            XamlTypeNamespaceToken controlsTypeNamespaceToken(tpkNative, KnownNamespaceIndex::Microsoft_UI_Xaml_Controls);
            XamlTypeToken gridTypeToken(tpkNative, KnownTypeIndex::Grid);

            VERIFY_SUCCEEDED(spNativeProvider->GetTypeNamespaceForType(gridTypeToken, typeNamespaceToken, &typeNamespaceName));
            VERIFY_IS_TRUE(typeNamespaceToken.GetHandle() == controlsTypeNamespaceToken.GetHandle());
            VERIFY_ARE_EQUAL(true, !!typeNamespaceName.Equals(c_strControlsNamespace, xstrCompareCaseSensitive));
        }
    }

    void XamlNativeTypeInfoProviderUnitTests::LookupTypeFlags()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);
        auto spNativeProvider = GetNativeTypeInfoProvider(spSchemaContext);

        // verify we can retrieve type flags
        {
            XamlBitSet<BoolTypeBits> bitsChecked;
            XamlBitSet<BoolTypeBits> bitsReturned;
            XamlBitSet<BoolTypeBits> bitsToRequest;
            bitsToRequest.SetAllBits();

            XamlTypeToken gridTypeToken(tpkNative, KnownTypeIndex::Grid);

            VERIFY_SUCCEEDED(spNativeProvider->LookupTypeFlags(gridTypeToken, bitsToRequest, bitsChecked, bitsReturned));
            VERIFY_IS_TRUE(bitsChecked.IsBitSet(btbConstructible) && bitsReturned.IsBitSet(btbConstructible));
            VERIFY_IS_TRUE(bitsChecked.IsBitSet(btbIsPublic) && bitsReturned.IsBitSet(btbIsPublic));
        }
    }


    void XamlNativeTypeInfoProviderUnitTests::LookupPropertyFlags()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);
        auto spNativeProvider = GetNativeTypeInfoProvider(spSchemaContext);

        // verify we can retrieve type flags
        {
            XamlBitSet<BoolPropertyBits> bitsChecked;
            XamlBitSet<BoolPropertyBits> bitsReturned;
            XamlBitSet<BoolPropertyBits> bitsToRequest;
            bitsToRequest.SetAllBits();

            XamlPropertyToken rectangleRadiusXProperty(tpkNative, KnownPropertyIndex::Rectangle_RadiusX);

            VERIFY_SUCCEEDED(spNativeProvider->LookupPropertyFlags(rectangleRadiusXProperty, bitsToRequest, bitsChecked, bitsReturned));
            VERIFY_IS_TRUE(bitsChecked.IsBitSet(bpbIsPublic) && bitsReturned.IsBitSet(bpbIsPublic));
        }
    }

    void XamlNativeTypeInfoProviderUnitTests::ResolveTypeName()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);
        auto spNativeProvider = GetNativeTypeInfoProvider(spSchemaContext);

        // verify we can resolve type name against default namespaces.
        {
            XamlTypeNamespaceToken defaultNamespaceToken(tpkUnknown, KnownNamespaceIndex::UnknownNamespace);
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strGridType, L"Grid");
            XamlTypeToken typeToken;
            XamlTypeToken gridType(tpkNative, KnownTypeIndex::Grid);

            VERIFY_SUCCEEDED(spNativeProvider->ResolveTypeName(defaultNamespaceToken, c_strGridType, typeToken));
            VERIFY_IS_TRUE(typeToken == gridType);
        }

        // verify we can resolve type name against specified namespaces.
        {
            XamlTypeNamespaceToken controlsNamespaceToken(tpkNative, KnownNamespaceIndex::Microsoft_UI_Xaml_Controls);
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strGridType, L"Grid");
            XamlTypeToken typeToken;
            XamlTypeToken gridType(tpkNative, KnownTypeIndex::Grid);

            VERIFY_SUCCEEDED(spNativeProvider->ResolveTypeName(controlsNamespaceToken, c_strGridType, typeToken));
            VERIFY_IS_TRUE(typeToken == gridType);
        }
    }

    void XamlNativeTypeInfoProviderUnitTests::ResolvePropertyName()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);
        auto spNativeProvider = GetNativeTypeInfoProvider(spSchemaContext);

        {
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strRadiusXProperty, L"RadiusX");
            XamlTypeToken rectangleType(tpkNative, KnownTypeIndex::Rectangle);
            XamlPropertyToken propertyToken;
            XamlPropertyToken rectangleRadiusXProperty(tpkNative, KnownPropertyIndex::Rectangle_RadiusX);
            XamlTypeToken propertyTypeToken;
            XamlTypeToken rectangleRadiusXPropertyType(tpkNative, KnownTypeIndex::Double);

            VERIFY_SUCCEEDED(spNativeProvider->ResolvePropertyName(rectangleType, c_strRadiusXProperty, propertyToken, propertyTypeToken));
            VERIFY_IS_TRUE(propertyToken == rectangleRadiusXProperty);
            VERIFY_IS_TRUE(propertyTypeToken == rectangleRadiusXPropertyType);
        }
    }

    void XamlNativeTypeInfoProviderUnitTests::ResolveDependencyPropertyName()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);
        auto spNativeProvider = GetNativeTypeInfoProvider(spSchemaContext);
        auto mock = TlsProvider<DynamicMetadataStorageMock>::CreateWrappedObject();

        {
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strGridRowProperty, L"Grid.Row");
            XamlTypeToken buttonTypeToken(tpkManaged, KnownTypeIndex::Button);
            XamlPropertyToken propertyToken;
            XamlPropertyToken gridRowPropertyToken(tpkNative, KnownPropertyIndex::Grid_Row);
            XamlTypeToken propertyTypeToken;
            XamlTypeToken gridRowPropertyTypeToken(tpkNative, KnownTypeIndex::Int32);

            VERIFY_SUCCEEDED(spNativeProvider->ResolveDependencyPropertyName(buttonTypeToken, c_strGridRowProperty, propertyToken, propertyTypeToken));
            VERIFY_IS_TRUE(propertyToken == gridRowPropertyToken);
            VERIFY_IS_TRUE(propertyTypeToken == gridRowPropertyTypeToken);
        }
    }

    void XamlNativeTypeInfoProviderUnitTests::ProviderMetadataQueryTests()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);
        auto spNativeProvider = GetNativeTypeInfoProvider(spSchemaContext);

        // verify type name can be retrieved given a type token
        {
            xstring_ptr strTypeName;
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strGridTypeName, L"Grid");
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strGridTypeFullName, L"Microsoft.UI.Xaml.Controls.Grid");
            XamlTypeToken gridType(tpkNative, KnownTypeIndex::Grid);

            VERIFY_SUCCEEDED(spNativeProvider->GetTypeName(gridType, &strTypeName));
            VERIFY_ARE_EQUAL(true, !!strTypeName.Equals(c_strGridTypeName, xstrCompareCaseSensitive));

            VERIFY_SUCCEEDED(spNativeProvider->GetTypeFullName(gridType, &strTypeName));
            VERIFY_ARE_EQUAL(true, !!strTypeName.Equals(c_strGridTypeFullName, xstrCompareCaseSensitive));
        }

        // verify property name can be retrieved given a property token
        {
            xstring_ptr strPropertyName;
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strRectangleRadiusXPropertyName, L"RadiusX");
            XamlPropertyToken rectangleRadiusXPropertyToken(tpkNative, KnownPropertyIndex::Rectangle_RadiusX);

            VERIFY_SUCCEEDED(spNativeProvider->GetPropertyName(rectangleRadiusXPropertyToken, &strPropertyName));
            VERIFY_ARE_EQUAL(true, !!strPropertyName.Equals(c_strRectangleRadiusXPropertyName, xstrCompareCaseSensitive));
        }

        // verify base type can be retrieved given a type token
        {
            XamlTypeToken rectangleTypeToken(tpkNative, KnownTypeIndex::Rectangle);
            XamlTypeToken baseTypeToken;

            // TODO: Misleading method
            VERIFY_SUCCEEDED(spNativeProvider->GetBaseType(rectangleTypeToken, baseTypeToken));
            VERIFY_IS_TRUE(baseTypeToken == rectangleTypeToken);
        }

        // verify declaring type can be retrieved given a property token
        {
            XamlPropertyToken rectangleRadiusXPropertyToken(tpkNative, KnownPropertyIndex::Rectangle_RadiusX);
            XamlTypeToken rectangleTypeToken(tpkNative, KnownTypeIndex::Rectangle);
            XamlTypeToken declaringPropertyTypeToken;

            VERIFY_SUCCEEDED(spNativeProvider->GetDeclaringType(rectangleRadiusXPropertyToken, declaringPropertyTypeToken));
            VERIFY_IS_TRUE(declaringPropertyTypeToken == rectangleTypeToken);
        }

        // verify assignable from
        {
            XamlTypeToken solidColorBrushType(tpkNative, KnownTypeIndex::SolidColorBrush);
            XamlTypeToken brushType(tpkNative, KnownTypeIndex::Brush);
            bool isAssignable;

            VERIFY_SUCCEEDED(spNativeProvider->IsAssignableFrom(solidColorBrushType, brushType, isAssignable));
            VERIFY_IS_TRUE(isAssignable == TRUE);
        }

        // verify content property
        {
            XamlTypeToken gridType(tpkNative, KnownTypeIndex::Grid);
            XamlPropertyToken gridContentProperty(tpkNative, KnownPropertyIndex::Panel_Children);
            XamlTypeToken gridContentPropertyType(tpkNative, KnownTypeIndex::UIElementCollection);
            XamlPropertyToken contentPropertyToken;
            XamlTypeToken contentPropertyTypeToken;

            VERIFY_SUCCEEDED(spNativeProvider->GetContentProperty(gridType, contentPropertyToken, contentPropertyTypeToken));
            VERIFY_IS_TRUE(contentPropertyToken == gridContentProperty);
            VERIFY_IS_TRUE(contentPropertyTypeToken == gridContentPropertyType);
        }

        // verify runtime name property
        {
            XamlTypeToken gridType(tpkNative, KnownTypeIndex::Grid);
            XamlTypeToken namePropertyType;
            XamlPropertyToken nameProperty;
            XamlPropertyToken dependencyObjectNameProperty(tpkNative, KnownPropertyIndex::DependencyObject_Name);
            XamlTypeToken stringType(tpkNative, KnownTypeIndex::String);

            VERIFY_SUCCEEDED(spNativeProvider->GetRuntimeNameProperty(gridType, nameProperty, namePropertyType));
            VERIFY_IS_TRUE(nameProperty == dependencyObjectNameProperty);
            VERIFY_IS_TRUE(namePropertyType == stringType);
        }

        // verify text syntax type converter
        {
            XamlTypeToken objectType(tpkNative, KnownTypeIndex::Object);
            XamlTypeToken stringType(tpkNative, KnownTypeIndex::String);
            XamlTypeToken textSyntaxConverterType;

            VERIFY_SUCCEEDED(spNativeProvider->GetTextSyntaxForType(objectType, textSyntaxConverterType));
            VERIFY_IS_TRUE(textSyntaxConverterType == stringType);
        }

        // verify content wrapper
        {
            XamlTypeToken inlineCollectionType(tpkNative, KnownTypeIndex::InlineCollection);
            XamlTypeToken contentWrapperType;
            XamlTypeToken runType(tpkNative, KnownTypeIndex::Run);

            VERIFY_SUCCEEDED(spNativeProvider->GetContentWrapper(inlineCollectionType, contentWrapperType));
            VERIFY_IS_TRUE(contentWrapperType == runType);
        }

        // verify dictionary key type
        {
            XamlTypeToken xamlPropertyType;
            XamlPropertyToken xamlProperty;

            XamlTypeToken gridType(tpkNative, KnownTypeIndex::Grid);
            XamlTypeToken styleType(tpkNative, KnownTypeIndex::Style);

            VERIFY_SUCCEEDED(spNativeProvider->GetDictionaryKeyProperty(styleType, xamlProperty, xamlPropertyType));
            VERIFY_IS_TRUE(xamlProperty == XamlPropertyToken(tpkNative, KnownPropertyIndex::Style_TargetType));
            VERIFY_IS_TRUE(xamlPropertyType == XamlTypeToken(tpkNative, KnownTypeIndex::TypeName));
        }

        // verify xml lang property type
        {
            XamlTypeToken xamlPropertyType;
            XamlPropertyToken xamlProperty;

            XamlTypeToken gridType(tpkNative, KnownTypeIndex::Grid);
            XamlTypeToken textElementType(tpkNative, KnownTypeIndex::TextElement);

            VERIFY_SUCCEEDED(spNativeProvider->GetXmlLangProperty(gridType, xamlProperty, xamlPropertyType));
            VERIFY_IS_TRUE(xamlProperty == XamlPropertyToken(tpkNative, KnownPropertyIndex::FrameworkElement_Language));
            VERIFY_IS_TRUE(xamlPropertyType == XamlTypeToken(tpkNative, KnownTypeIndex::String));

            VERIFY_SUCCEEDED(spNativeProvider->GetXmlLangProperty(textElementType, xamlProperty, xamlPropertyType));
            VERIFY_IS_TRUE(xamlProperty == XamlPropertyToken(tpkNative, KnownPropertyIndex::TextElement_Language));
            VERIFY_IS_TRUE(xamlPropertyType == XamlTypeToken(tpkNative, KnownTypeIndex::String));
        }

        // verify collection item type property
        {
            XamlTypeToken uiElementCollectionType(tpkNative, KnownTypeIndex::UIElementCollection);
            XamlTypeToken contentPropertyTypeToken;

            VERIFY_SUCCEEDED(spNativeProvider->GetCollectionItemType(uiElementCollectionType, contentPropertyTypeToken));
            VERIFY_IS_TRUE(contentPropertyTypeToken == XamlTypeToken(tpkNative, KnownTypeIndex::UIElement));
        }

        // verify text syntax converter for property
        {
            XamlPropertyToken widthProperty(tpkNative, KnownPropertyIndex::FrameworkElement_Width);
            XamlPropertyToken columnDefinitionWidthProperty(tpkNative, KnownPropertyIndex::ColumnDefinition_Width);
            XamlTypeToken typeConverter;

            VERIFY_SUCCEEDED(spNativeProvider->GetTextSyntaxForProperty(widthProperty, typeConverter));
            VERIFY_IS_TRUE(typeConverter == XamlTypeToken(tpkNative, KnownTypeIndex::LengthConverter));

            VERIFY_SUCCEEDED(spNativeProvider->GetTextSyntaxForProperty(columnDefinitionWidthProperty, typeConverter));
            VERIFY_IS_TRUE(typeConverter == XamlTypeToken(tpkNative, KnownTypeIndex::GridLength));
        }
    }

    std::shared_ptr<XamlSchemaContext> XamlNativeTypeInfoProviderUnitTests::GetSchemaContext(const std::shared_ptr<ParserErrorReporter>& customErrorReporter)
    {
        ParserUtilities parserUtils;
        return parserUtils.GetSchemaContext(customErrorReporter);
    }

    std::shared_ptr<XamlTypeInfoProvider> XamlNativeTypeInfoProviderUnitTests::GetNativeTypeInfoProvider(const std::shared_ptr<XamlSchemaContext>& schemaContext)
    {
        std::shared_ptr<XamlTypeInfoProvider> spNativeProvider;
        VERIFY_SUCCEEDED(schemaContext->GetTypeInfoProvider(tpkNative, spNativeProvider));
        return spNativeProvider;
    }

} } } } }
