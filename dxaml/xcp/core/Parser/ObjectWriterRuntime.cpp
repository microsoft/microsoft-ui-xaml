// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ObjectWriterRuntime.h"
#include "ObjectWriterErrorService.h"

#include <ObjectWriterContext.h>
#include <CustomWriterRuntimeContext.h>
#include <CustomWriterRuntimeData.h>
#include <SubObjectWriterResult.h>

#include <MetadataAPI.h>

#include <DXamlServices.h>
#include <dependencyLocator\inc\DependencyLocator.h>
#include <diagnosticsInterop\inc\ResourceGraph.h>

using namespace DirectUI;

ObjectWriterRuntime::ObjectWriterRuntime(
    _In_ const std::shared_ptr<ObjectWriterContext>& spContext,
    _In_ const std::shared_ptr<ObjectWriterErrorService>& spErrorService,
    _In_ const bool isValidatorEnabled)
    : ObjectWriterCommonRuntime(spContext, spErrorService, isValidatorEnabled, false /* isEncoding */),
    m_shouldStoreSourceInformation(DXamlServices::ShouldStoreSourceInformation())
{
}

HRESULT ObjectWriterRuntime::AddNamespacePrefixImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const xstring_ptr& strNamespacePrefix,
    _In_ const std::shared_ptr<XamlNamespace>& spNamespace)
{
    return S_OK;
}

HRESULT ObjectWriterRuntime::PushConstantImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue)
{
    return S_OK;
}

HRESULT ObjectWriterRuntime::SetDirectivePropertyImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<DirectiveProperty>& spProperty,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance)
{
    return S_OK;
}

HRESULT ObjectWriterRuntime::PushScopeImpl(
    _In_ const XamlLineInfo& lineInfo)
{
    return S_OK;
}

HRESULT ObjectWriterRuntime::PopScopeImpl(
    _In_ const XamlLineInfo& lineInfo)
{
    return S_OK;
}

HRESULT ObjectWriterRuntime::CreateTypeImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<ObjectWriterCallbacksDelegate>& spCallback,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spRootObjectInstance,
    _Out_ std::shared_ptr<XamlQualifiedObject>& spInstance)
{
    HRESULT xr = S_OK;

    xr = spType->CreateInstance(spInstance);
    IFC_RETURN(m_spErrorService->WrapErrorWithParserErrorAndRethrow(xr, lineInfo));
    if (FAILED(xr))
    {
        // Raise an error like "Cannot create instance of type '%0'"
        xstring_ptr spTypeName;
        IFC_RETURN(spType->get_FullName(&spTypeName));
        IFC_RETURN(m_spErrorService->ReportError(AG_E_PARSER2_TYPE_UNCREATEABLE, lineInfo, spTypeName));
        IFC_RETURN(E_FAIL);
    }

    TraceElementSourceInfo(lineInfo, spType, spInstance);

    if (spCallback)
    {
        IFC_RETURN(spCallback->OnObjectCreated(spRootObjectInstance, spInstance));
    }

    return S_OK;
}

HRESULT ObjectWriterRuntime::GetValueImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _Out_ std::shared_ptr<XamlQualifiedObject>& spValue)
{
    XamlQualifiedObject value;
    IFC_RETURN(spProperty->GetValue(*spInstance, value));
    spValue = std::make_shared<XamlQualifiedObject>(std::move(value));

    // In the case of the following xaml, we go through a PushScopeGetValue node in order to retrieve
    // the ResourceDictionary rather than a PushScopeCreateTypeBeginInit if the Resource Dictionary is
    // explicitly defined (i.e.: <Application.Resources><ResourceDictionary>....)
    //  <Application.Resources>
    //      <SolidColorBrush x:Key="myBrush">Blue</SolidColorBrush>
    //  </Application.Resources>
    if (m_shouldStoreSourceInformation)
    {
        std::shared_ptr<XamlType> xamlType;
        IFC_RETURN(spProperty->get_Type(xamlType));

        IFC_RETURN(StoreSourceInformationHelper(
            lineInfo,
            xamlType,
            spValue));

        TraceElementSourceInfo(lineInfo, xamlType, spValue);
    }

    return S_OK;
}

HRESULT ObjectWriterRuntime::CreateTypeWithInitialValueImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<ObjectWriterCallbacksDelegate>& spCallback,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spRootObjectInstance,
    _Inout_ std::shared_ptr<XamlQualifiedObject>& spInstance)
{
    //
    // The instance was created during type conversion.
    //

    if (spCallback)
    {
        IFC_RETURN(spCallback->OnObjectCreated(spRootObjectInstance, spInstance));
    }

    return S_OK;
}

HRESULT ObjectWriterRuntime::TypeConvertValueImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ std::shared_ptr<XamlServiceProviderContext> spContext,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<XamlTextSyntax>& spTextSyntaxConverter,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
    _In_ bool fIsPropertyAssignment,
    _Out_ std::shared_ptr<XamlQualifiedObject>& spInstance)
{
    HRESULT xr = S_OK;

    xr = spType->CreateFromValue(
        std::static_pointer_cast<XamlServiceProviderContext>(spContext),
        spTextSyntaxConverter,
        spValue,
        fIsPropertyAssignment,
        spInstance);

    IFC_RETURN(m_spErrorService->WrapErrorWithParserErrorAndRethrow(xr, lineInfo));
    if (FAILED(xr))
    {
        xstring_ptr ssName;
        xstring_ptr ssValue;
        IFC_RETURN(spType->get_FullName(&ssName));
        IFC_RETURN(spValue->GetCopyAsString(&ssValue));

        // TODO: Error code should do this automatically.
        if (ssValue.IsNull())
        {
            ssValue = xstring_ptr::EmptyString();
        }

        // Raise an error like "Failed to create a '%1' from the text '%0'."
        IFC_RETURN(m_spErrorService->ReportError(AG_E_PARSER2_OW_TYPE_CONVERSION_FAILED, lineInfo, ssValue, ssName));
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

HRESULT ObjectWriterRuntime::SetValueImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue)
{
    HRESULT xr = S_OK;
    bool bBindTemplates = !m_spContext->get_ExpandTemplates();

    if (spValue->GetTypeToken().GetHandle() != KnownTypeIndex::DeferredElement)
    {
        xr = spProperty->SetValue(
            spInstance,
            spValue,
            bBindTemplates);
    }
    else
    {
        xr = AddDeferredProxy(
            spInstance,
            std::shared_ptr<XamlQualifiedObject>(),
            spProperty,
            spValue);
    }

    IFC_RETURN(m_spErrorService->WrapErrorWithParserErrorAndRethrow(xr, lineInfo));
    if (FAILED(xr))
    {
        xstring_ptr ssPropertyName;
        ASSERT(!!spProperty);
        if (spProperty)
        {
            IFC_RETURN(spProperty->get_FullName(&ssPropertyName));
        }

        // Report a failed attempt to set a value
        IFC_RETURN(m_spErrorService->ReportSetValueError(xr, lineInfo, ssPropertyName, xstring_ptr::EmptyString()));
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

HRESULT ObjectWriterRuntime::CheckPeerTypeImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue)
{
    std::shared_ptr<XamlSchemaContext> spSchemaContext;
    xstring_ptr strClassName;

    IFC_RETURN(spValue->GetCopyAsString(&strClassName));
    IFC_RETURN(m_spContext->get_SchemaContext(spSchemaContext));
    if (FAILED(spSchemaContext->CheckPeerType(
        spInstance,
        strClassName,
        FALSE)))
    {
        // Raise a generic error like "Parser
        // internal error: Object writer '%0'."
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strErrorType, L"xClassNotDerivedFromElement");
        IFC_RETURN(m_spErrorService->ReportError(AG_E_PARSER2_INTERNAL_OW_GENERIC, lineInfo, c_strErrorType));
        IFC_RETURN(E_UNEXPECTED);
    }

    return S_OK;
}

HRESULT ObjectWriterRuntime::AddToCollectionImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spParentPropertyType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spParentCollection,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
    _In_ const std::shared_ptr<XamlType>& spValueType)
{
    HRESULT xr = S_OK;

    xr = AddToCollectionImplHelper(
        spParentCollection,
        spValue);

    IFC_RETURN(m_spErrorService->WrapErrorWithParserErrorAndRethrow(xr, lineInfo));
    if (FAILED(xr))
    {
        xstring_ptr ssParentPropertyTypeName;
        xstring_ptr ssCurrentTypeName;
        IFC_RETURN(spParentPropertyType->get_FullName(&ssParentPropertyTypeName));

        if (spValueType->IsUnknown())
        {
            // To get here we tried to do something like the below and we won't have a current type.
            ////<FooCollection>
            ////    <FooItem />
            ////    Random text
            ////</FooCollection>
            xstring_ptr ssValue;
            IFC_RETURN(spValue->GetCopyAsString(&ssValue));

            // Raise an error like "Cannot add text '%0' to collection of
            // type '%1'."
            IFC_RETURN(m_spErrorService->ReportError(AG_E_PARSER2_OW_CANT_ADD_TEXT_TO_COLLECTION, lineInfo, ssValue, ssParentPropertyTypeName));
            IFC_RETURN(E_FAIL);
        }
        else
        {
            IFC_RETURN(spValueType->get_FullName(&ssCurrentTypeName));

            // ZERRTODO: Update the error message to read like "Cannot add
            // instance of type 'SolidColorBrush' to a collection of type
            // 'UIElementCollection'. Only items of type 'UIElement' are
            // allowed."

            // Raise en error like "Cannot add instance of type '%0' to a
            // collection of type '%1'."
            IFC_RETURN(m_spErrorService->ReportError(AG_E_PARSER2_OW_WRONG_ITEM_TYPE_FOR_COLLECTION, lineInfo, ssCurrentTypeName, ssParentPropertyTypeName));
            IFC_RETURN(E_FAIL);
        }
    }

    return S_OK;
}

HRESULT ObjectWriterRuntime::AddToCollectionImplHelper(
    _In_ const std::shared_ptr<XamlQualifiedObject>& spParentCollection,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue)
{
    std::shared_ptr<XamlSchemaContext> spXamlSchemaContext;
    std::shared_ptr<XamlRuntime> spXamlRuntime;

    ASSERT(!!spParentCollection);
    IFC_RETURN(m_spContext->get_SchemaContext(spXamlSchemaContext));
    IFC_RETURN(spXamlSchemaContext->GetRuntime(spParentCollection->GetTypeToken().GetRuntimeKind(), spXamlRuntime));

    if (spValue->GetTypeToken().GetHandle() != KnownTypeIndex::DeferredElement)
    {
        IFC_RETURN(spXamlRuntime->Add(spParentCollection, spValue));
    }
    else
    {
        IFC_RETURN(AddDeferredProxy(
            std::shared_ptr<XamlQualifiedObject>(),
            spParentCollection,
            m_spContext->Current().get_Member(),
            spValue));
    }

    return S_OK;
}

HRESULT ObjectWriterRuntime::AddToDictionaryImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spParentCollection,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
    _In_ const std::shared_ptr<XamlType>& spValueType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spKey)
{
    HRESULT xr = S_OK;

    xr = AddToDictionaryImplHelper(
        spParentCollection,
        spValue,
        spValueType,
        spKey);

    IFC_RETURN(m_spErrorService->WrapErrorWithParserErrorAndRethrow(xr, lineInfo));
    if (FAILED(xr))
    {
        if (xr == E_DO_RESOURCE_KEYCONFLICT)
        {
            // Raise an error like "The dictionary key '%0' is already used.
            // Key attributes are used as keys when inserting objects into a
            // dictionary and must be unique."
            xstring_ptr ssKey;
            IFC_RETURN(spKey->GetCopyAsString(&ssKey));
            IFC_RETURN(m_spErrorService->ReportError(AG_E_PARSER2_OW_DUPLICATE_KEY, lineInfo, ssKey));
            IFC_RETURN(E_FAIL);
        }
        else
        {
            IFC_RETURN(xr);
        }
    }

    return S_OK;
}

HRESULT ObjectWriterRuntime::AddToDictionaryImplHelper(
    _In_ const std::shared_ptr<XamlQualifiedObject>& spParentCollection,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
    _In_ const std::shared_ptr<XamlType>& spValueType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spKey)
{
    std::shared_ptr<XamlSchemaContext> spXamlSchemaContext;
    std::shared_ptr<XamlRuntime> spXamlRuntime;

    ASSERT(!!spParentCollection);
    IFC_RETURN(m_spContext->get_SchemaContext(spXamlSchemaContext));
    IFC_RETURN(spXamlSchemaContext->GetRuntime(spParentCollection->GetTypeToken().GetRuntimeKind(), spXamlRuntime));
    IFC_RETURN(spXamlRuntime->AddToDictionary(spParentCollection, spValue, spKey));

    return S_OK;
}

HRESULT ObjectWriterRuntime::SetConnectionIdImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spComponentConnector,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spConnectionId,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spTarget)
{
    std::shared_ptr<XamlSchemaContext> spXamlSchemaContext;
    std::shared_ptr<XamlRuntime> spXamlRuntime;

    ASSERT(!!spComponentConnector);
    IFC_RETURN(m_spContext->get_SchemaContext(spXamlSchemaContext));
    IFC_RETURN(spXamlSchemaContext->GetRuntime(tfkManaged, spXamlRuntime));
    IFC_RETURN(spXamlRuntime->SetConnectionId(spComponentConnector, spConnectionId, spTarget));

    return S_OK;
}

HRESULT ObjectWriterRuntime::GetXBindConnectorImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spComponentConnector,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spConnectionId,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spTarget,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spReturnConnector)
{
    std::shared_ptr<XamlSchemaContext> spXamlSchemaContext;
    std::shared_ptr<XamlRuntime> spXamlRuntime;

    ASSERT(!!spComponentConnector);
    IFC_RETURN(m_spContext->get_SchemaContext(spXamlSchemaContext));
    IFC_RETURN(spXamlSchemaContext->GetRuntime(tfkManaged, spXamlRuntime));
    IFC_RETURN(spXamlRuntime->GetXBindConnector(spComponentConnector, spConnectionId, spTarget, spReturnConnector));

    return S_OK;
}

// Create an association between a name and an instance in a given namescope.
HRESULT ObjectWriterRuntime::SetNameImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spName,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
    _In_ const xref_ptr<INameScope>& spNamescope)
{
    if (spNamescope)
    {
        ASSERT(!!spName);
        CValue* pName = spName->GetCValuePtr();
        IFCPTR_RETURN(pName);

        // Create the association between the name and the instance
        switch (pName->GetType())
        {
        case valueString:
            TraceSetNameOnCurrentInstanceBegin(pName->AsString().GetBuffer());
            IFC_RETURN(spNamescope->RegisterName(pName->AsString(), spInstance));
            break;
        case valueObject:
        {
            CString* pString = do_pointer_cast<CString>(pName->AsObject());
            if (pString != nullptr)
            {
                TraceSetNameOnCurrentInstanceBegin(pString->m_strString.GetBuffer());
                IFC_RETURN(spNamescope->RegisterName(pString->m_strString, spInstance));
            }
            else
            {
                // No-op when an expression is assigned to the x:Name property.
                TraceSetNameOnCurrentInstanceBegin(L"MarkupExtension");
                IFCEXPECT_RETURN(pName->AsObject()->OfTypeByIndex<KnownTypeIndex::MarkupExtensionBase>());
            }
        }
            break;
        default:
            IFC_RETURN(E_FAIL);
            break;
        }
    }

    return S_OK;
}

HRESULT ObjectWriterRuntime::GetResourcePropertyBagImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spKey,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spObject,
    _Out_ SP_MapPropertyToQO& spPropertyBag)
{
    xref_ptr<IPALUri> spUidBaseUri;

    spUidBaseUri = m_spContext->get_BaseUri();
    if (!spUidBaseUri)
    {
        // In order to correctly resolve x:Uids inside templates, we need to
        // propagate the base URI from the initial parse, which is saved on
        // the ObjectWriterContext as the XamlResourceUri. If we didn't do this,
        // the x:Uid resolution would fall back to a hardcoded default base URI,
        // which is wrong in many scenarios.
        spUidBaseUri = m_spContext->get_XamlResourceUri();
    }

    IFC_RETURN(GetResourcePropertyBagImplHelper(
        lineInfo,
        spKey,
        spUidBaseUri,
        spObject,
        spType,
        spServiceProviderContext,
        spPropertyBag));

    return S_OK;
}

//
//  Get list of properties and un-converted string values associated
//  with this resource-id (spKey) given the URI context spBaseUri.
//
HRESULT ObjectWriterRuntime::GetResourcePropertyBagImplHelper(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spKey,
    _In_ const xref_ptr<IPALUri> spBaseUri,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spObject,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
    _Out_ SP_MapPropertyToQO& spPropertyBag)
{
    xstring_ptr spKeyString;
    std::shared_ptr<XamlSchemaContext> spSchemaContext;
    std::vector<std::pair<xstring_ptr, xstring_ptr>> rawPropertyBag;

    IFC_RETURN(spKey->GetCopyAsString(&spKeyString));
    IFCEXPECT_ASSERT_RETURN(!spKeyString.IsNull());
    IFC_RETURN(m_spContext->get_SchemaContext(spSchemaContext));
    IFC_RETURN(spServiceProviderContext->GetSchemaContext(spSchemaContext));
    CCoreServices* pCore = spSchemaContext->GetCore();

    IFC_RETURN(pCore->ShimGetResourcePropertyBagRaw(spKeyString, spBaseUri, rawPropertyBag));

    if (!rawPropertyBag.empty())
    {
        std::shared_ptr<XamlType> spXamlStringType;
        IFC_RETURN(spSchemaContext->get_StringXamlType(spXamlStringType));
        spPropertyBag = std::make_shared<MapPropertyToQO>();

        for (const auto& propertyValuePair : rawPropertyBag)
        {
            CValue vTemp;
            std::shared_ptr<XamlProperty> spProperty;
            std::shared_ptr<XamlQualifiedObject> spValue;

            // MakePri produces a nonconformant MRT resource name (e.g.
            // `[using:Microsoft.UI.Xaml.Automation]AutomationProperties/Name` rather than the expected
            // `[using:Microsoft.UI.Xaml.Automation]AutomationProperties.Name`) when parsing a .resw that
            // specifies an attached property. We need to replace the / with a . or else we'll fail to
            // resolve the specified attached property.
            xstring_ptr adjustedPropertyName;
            auto slashPos = propertyValuePair.first.FindChar(L'/');

            if (slashPos != xstring_ptr_view::npos)
            {
                auto propertyNameLength = propertyValuePair.first.GetCount();

                xstring_ptr firstHalf;
                xstring_ptr secondHalf;
                IFC_RETURN(propertyValuePair.first.SubString(0, slashPos, &firstHalf));
                IFC_RETURN(propertyValuePair.first.SubString(slashPos + 1, propertyNameLength, &secondHalf));

                XStringBuilder adjustedPropertyNameBuilder;
                IFC_RETURN(adjustedPropertyNameBuilder.Initialize(propertyNameLength));
                IFC_RETURN(adjustedPropertyNameBuilder.Append(firstHalf));
                IFC_RETURN(adjustedPropertyNameBuilder.AppendChar(L'.'));
                IFC_RETURN(adjustedPropertyNameBuilder.Append(secondHalf));
                IFC_RETURN(adjustedPropertyNameBuilder.DetachString(&adjustedPropertyName));
            }
            else
            {
                adjustedPropertyName = propertyValuePair.first;
            }

            if (adjustedPropertyName.FindChar(L'[') != xstring_ptr_view::npos)
            {
                IFC_RETURN(spSchemaContext->GetXamlProperty(adjustedPropertyName, spProperty));
            }
            else
            {
                IFC_RETURN(spType->GetProperty(adjustedPropertyName, spProperty));
            }

            if (spProperty)
            {
                vTemp.SetString(propertyValuePair.second);

                spValue = std::make_shared<XamlQualifiedObject>();
                IFC_RETURN(spValue->SetValue(vTemp));
                spValue->SetTypeToken(spXamlStringType->get_TypeToken());
                IFC_RETURN(spPropertyBag->push_back(MapPropertyToQO::TPair(spProperty, spValue)));
            }
            else
            {
                std::shared_ptr<ParserErrorReporter> errorService;

                IFC_RETURN(spSchemaContext->GetErrorService(errorService));
                IFC_RETURN(errorService->SetError(AG_E_PARSER2_CANNOT_RESOLVE_PROP_FROM_UID,
                    lineInfo.LineNumber(),
                    lineInfo.LinePosition(),
                    spKeyString,
                    adjustedPropertyName));
                IFC_RETURN(E_FAIL);
            }

        }
    }

    return S_OK;
}

HRESULT ObjectWriterRuntime::ProvideValueImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spMarkupExtension,
    _Out_ std::shared_ptr<XamlQualifiedObject>& spValue)
{
    HRESULT xr = S_OK;

    xr = ProvideValueImplHelper(
        spServiceProviderContext,
        spMarkupExtension,
        spValue);
    IFC_RETURN(m_spErrorService->WrapErrorWithParserErrorAndRethrow(xr, lineInfo));
    if (FAILED(xr))
    {
        // Raise an error like "Markup extension could not provide value."
        IFC_RETURN(m_spErrorService->ReportError(AG_E_PARSER2_OW_MARKUP_EXTENSION_COULD_NOT_PROVIDE_VALUE, lineInfo));
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

HRESULT ObjectWriterRuntime::ProvideValueImplHelper(
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spMarkupExtension,
    _Out_ std::shared_ptr<XamlQualifiedObject>& spValue)
{
    std::shared_ptr<XamlSchemaContext> spXamlSchemaContext;
    std::shared_ptr<XamlRuntime> spXamlRuntime;

    ASSERT(!!spMarkupExtension);
    IFC_RETURN(m_spContext->get_SchemaContext(spXamlSchemaContext));
    IFC_RETURN(spXamlSchemaContext->GetRuntime(spMarkupExtension->GetTypeToken().GetRuntimeKind(), spXamlRuntime));
    IFC_RETURN(spXamlRuntime->CallProvideValue(spMarkupExtension, spServiceProviderContext, spValue));
    if (m_shouldStoreSourceInformation)
    {
        auto markupExtension = checked_cast<CMarkupExtensionBase>(spMarkupExtension->GetDependencyObject());
        std::shared_ptr<XamlQualifiedObject> targetObject;
        spServiceProviderContext->GetMarkupExtensionTargetObject(targetObject);

        std::shared_ptr<XamlProperty> targetProp;
        spServiceProviderContext->GetMarkupExtensionTargetProperty(targetProp);
        if (targetProp)
        {
            // TargetProp can be null for things like <NullExtension/> or <StaticResource/>
            const auto resourceGraph = Diagnostics::GetResourceGraph();
            resourceGraph->CacheMarkupExtensionToTarget(markupExtension, targetProp->get_PropertyToken().GetHandle(), targetObject->GetDependencyObject());
        }
    }

    return S_OK;
}

HRESULT ObjectWriterRuntime::BeginInitImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spXamlType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance)
{
    IFC_RETURN(SetGuardImplHelper(
        spXamlType,
        spInstance,
        true /* StartGuard */));

    IFC_RETURN(SetUriBaseImplHelper(
        spXamlType,
        spInstance,
        m_spContext->get_BaseUri()));

    IFC_RETURN(StoreSourceInformationHelper(
        lineInfo,
        spXamlType,
        spInstance));

    return S_OK;
}

HRESULT ObjectWriterRuntime::EndInitImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spXamlType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance)
{
    HRESULT xr = S_OK;

    xr = SetGuardImplHelper(
        spXamlType,
        spInstance,
        false /* EndGuard */);

    // Try to rethrow any errors first, but otherwise fail normally.
    IFC_RETURN(m_spErrorService->WrapErrorWithParserErrorAndRethrow(xr, lineInfo));
    IFC_RETURN(xr);

    return S_OK;
}

HRESULT ObjectWriterRuntime::SetCustomRuntimeDataImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
    _In_ std::shared_ptr<CustomWriterRuntimeData> customRuntimeData,
    _In_ std::shared_ptr<SubObjectWriterResult> subObjectWriterResult,
    _In_ xref_ptr<INameScope> nameScope,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spXBindConnector,
    const std::shared_ptr<ObjectWriterCallbacksDelegate>& callback)
{
    CVisualStateGroupCollection *pVisualStateGroupCollection = nullptr;
    CResourceDictionary *pResourceDictionary = nullptr;
    CStyle *pStyle = nullptr;
    CDeferredElement *pDeferredElement = nullptr;

    try
    {
        std::shared_ptr<XamlQualifiedObject> targetType;
        IFC_RETURN(m_spContext->ServiceProvider_GetAmbientTargetType(targetType));
        std::shared_ptr<XamlSchemaContext> schemaContext;
        IFC_RETURN(m_spContext->get_SchemaContext(schemaContext));

        xref::weakref_ptr<CDependencyObject> weakRefToRootInstance(m_spContext->get_RootInstance()->GetOwnedDependencyObject().get());

        // Get saved stack for the writer to use when looking up static resources later.
        std::shared_ptr<ObjectWriterStack> spSavedStack;
        std::shared_ptr<XamlSavedContext> spSavedContext;
        IFC_RETURN(m_spContext->GetSavedContext(spSavedContext));
        IFC_RETURN(spSavedContext->get_Stack(spSavedStack));

        // Clean up the instances on the saved stack in order to avoid circular references
        RemoveObjectReferencesFromStack(spSavedStack);

        auto customWriterRuntimeContext = std::unique_ptr<CustomWriterRuntimeContext>(
            new CustomWriterRuntimeContext(
                schemaContext,
                targetType,
                m_spContext->get_BaseUri() ? m_spContext->get_BaseUri() : m_spContext->get_XamlResourceUri(),
                weakRefToRootInstance,
                subObjectWriterResult->GetSubReader(),
                spSavedStack,
                nameScope,
                m_spContext->get_XbfHash(),
                spXBindConnector,
                callback));

        // Ideally we'd have a way to query for support of the ICustomWriterRuntimeDataReceiver
        // interface. Because we don't, and because we can't cast a CDependencyObject directly
        // to this type we have to cast to the instance type first. Boo.

        // This is NOT a place to put type-specific logic. All type-specific operations should happen inside the
        // SetCustomWriterRuntimeData implementation on the type itself.
        if (SUCCEEDED(DoPointerCast(pVisualStateGroupCollection, spInstance->GetValue())))
        {
            TraceSetCustomRuntimeDataForVSMBegin();
            IFC_RETURN(pVisualStateGroupCollection->SetCustomWriterRuntimeData(std::move(customRuntimeData), std::move(customWriterRuntimeContext)));
            TraceSetCustomRuntimeDataForVSMEnd();
        }
        else if (SUCCEEDED(DoPointerCast(pResourceDictionary, spInstance->GetValue())))
        {
            TraceSetCustomRuntimeDataForResourceDictionaryBegin();
            IFC_RETURN(pResourceDictionary->SetCustomWriterRuntimeData(std::move(customRuntimeData), std::move(customWriterRuntimeContext)));
            TraceSetCustomRuntimeDataForResourceDictionaryEnd();
        }
        else if (SUCCEEDED(DoPointerCast(pStyle, spInstance->GetValue())))
        {
            TraceSetCustomRuntimeDataForStyleBegin();
            IFC_RETURN(pStyle->SetCustomWriterRuntimeData(std::move(customRuntimeData), std::move(customWriterRuntimeContext)));
            TraceSetCustomRuntimeDataForStyleEnd();
        }
        else if (SUCCEEDED(DoPointerCast(pDeferredElement, spInstance->GetValue())))
        {
            TraceSetCustomRuntimeDataForDeferredElementBegin();
            IFC_RETURN(pDeferredElement->SetCustomWriterRuntimeData(std::move(customRuntimeData), std::move(customWriterRuntimeContext)));
            TraceSetCustomRuntimeDataForDeferredElementEnd();
        }

#if DBG
        // Make sure that the data for the style was cached. Styles currently don't store their runtime context. Checks like this should
        // be added for other types if we require their CustomWriterRuntimeContext for Edit & Continue.
        if (pStyle && m_shouldStoreSourceInformation)
        {
            ASSERT(Diagnostics::GetResourceGraph()->GetCachedRuntimeContext(pStyle));
        }
#endif

    }
    CATCH_RETURN();
    return S_OK;
}

void ObjectWriterRuntime::RemoveObjectReferencesFromStack(
    _In_ const std::shared_ptr<ObjectWriterStack>& spObjectWriterStack)
{
    for (auto it = spObjectWriterStack->begin();
        it != spObjectWriterStack->end();
        ++it)
    {
        (*it).clear_Collection();

        // Replace the instances with weakrefs
        if ((*it).exists_Instance())
        {
            auto weakRefToInstance = xref::get_weakref(it->get_Instance()->GetOwnedDependencyObject().get());
            if (weakRefToInstance)
            {
                it->set_WeakRefInstance(std::move(weakRefToInstance));
            }
        }
    }
}

HRESULT ObjectWriterRuntime::DeferResourceDictionaryItemsImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ ObjectWriter* pObjectWriter,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spCollection,
    _In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList,
    _In_ const bool fIsDictionaryWithKeyProperty,
    _Out_ bool& hasDeferred)
{
    bool fShouldDefer = false;
    bool fHasNonDeferrableContent = false;

    // Should we defer?
    IFC_RETURN(CResourceDictionary::ShouldDeferNodeStream(spNodeList, &fShouldDefer, &fHasNonDeferrableContent));

    if (fShouldDefer)
    {
        std::shared_ptr<XamlSavedContext>     spSavedContext;
        std::shared_ptr<XamlQualifiedObject>  collection = spCollection;
        CResourceDictionary              *pDictionary = nullptr;

        // Make sure this is a ResourceDictionary
        IFC_RETURN(DoPointerCast(pDictionary, collection->GetValue()));

        // Save the ObjectWriterContext for deferred playback.
        IFC_RETURN(m_spContext->GetSavedContext(spSavedContext));

        // Start deferral
        IFC_RETURN(PushScope(lineInfo));
        IFC_RETURN(pDictionary->DeferKeysAsXaml((fIsDictionaryWithKeyProperty && !fHasNonDeferrableContent), m_spContext->get_MarkupExtensionContext(), spNodeList, spSavedContext));
        IFC_RETURN(PopScope(lineInfo));

        // TODO: Revisit the fact that this needs the ObjectWriter
        // Playback any non deferrable content
        IFC_RETURN(pDictionary->Ensure(pObjectWriter));
    }

    hasDeferred = (fShouldDefer == TRUE);

    return S_OK;
}

HRESULT ObjectWriterRuntime::DeferTemplateContentImpl(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
    _In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList,
    _Out_ std::shared_ptr<XamlQualifiedObject>& spTemplateContent)
{
    HRESULT hr = S_OK;
    CTemplateContent *pTemplateContent = nullptr;
    std::shared_ptr<XamlSchemaContext> spSchemaContext;

    IFC(m_spContext->get_SchemaContext(spSchemaContext));
    CCoreServices* pCore = spSchemaContext->GetCore();

    // TODO:
    // HACK.  There is a mechanism for defining what the
    // TemplateContent's DeferriedContentLoader is, and it revolves
    // around an alternate use of the TextSyntax, but this will have to
    // do until we can evaluate that method.
    {
        std::shared_ptr<XamlSavedContext> spSavedContext;
        XamlTypeToken sTemplateContentTypeToken(tpkNative, KnownTypeIndex::TemplateContent);
        CREATEPARAMETERS cp(pCore);

        IFC(CTemplateContent::Create((CDependencyObject**)&pTemplateContent, &cp));
        pTemplateContent->SetIsParsing(TRUE);

        IFC(m_spContext->GetSavedContext(spSavedContext));

        // push an objectwriterframe so that the stack is in the correct state for looking up ambient properties.
        // The reason for this is that AmbientProperty lookup will always skip the current stack level.  In this
        // case the current stack level is the Template.  As the template could actually have one of the ambient
        // property values that the TemplateContent is interested in (TargetTypeProperty), so skipping the
        // Template would pose a problem.
        IFC(PushScope(lineInfo));
        IFC(pTemplateContent->RecordXaml(m_spContext->get_MarkupExtensionContext(), spNodeList, spSavedContext, std::shared_ptr<XamlBinaryFormatSubReader2>(), std::vector<std::pair<bool, xstring_ptr>>()));
        IFC(PopScope(lineInfo));

        IFC(XamlQualifiedObject::CreateNoAddRef(pCore, sTemplateContentTypeToken, pTemplateContent, spTemplateContent));
        pTemplateContent = nullptr;
    }

    // Store the event root in the template because it will be needed to expand
    if (spInstance)
    {
        // set the deferred template on the template property
        IFC(SetValue(
            lineInfo,
            spType,
            spInstance,
            spProperty,  // Template property
            spTemplateContent));

        IFC(RaiseTemplateOwnerEventImplHelper(spInstance));
    }

Cleanup:
    if (pTemplateContent)
    {
        // if pTemplateContent is not null here, then something has gone wrong.
        // unpeg the pTemplateContent so that it will not cause a leak.
        pTemplateContent->UnpegManagedPeerNoRef();
        ReleaseInterface(pTemplateContent);
    }
    RRETURN(hr);
}

HRESULT ObjectWriterRuntime::GetStreamOffsetTokenImpl(
    _Out_ UINT32 *pTokenIndex)
{
    *pTokenIndex = 0;

    // should it fail or should we no op?
    RRETURN(E_FAIL);
}

_Check_return_ HRESULT
ObjectWriterRuntime::BeginConditionalScopeImpl(
    _In_ const XamlLineInfo&,
    _In_ const std::shared_ptr<Parser::XamlPredicateAndArgs>&)
{
    return S_OK;
}

_Check_return_ HRESULT
ObjectWriterRuntime::EndConditionalScopeImpl(
    _In_ const XamlLineInfo&)
{
    return S_OK;
}

HRESULT ObjectWriterRuntime::SetGuardImplHelper(
    _In_ const std::shared_ptr<XamlType>& spXamlType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
    _In_ bool fStartGuard)
{
    std::shared_ptr<XamlSchemaContext> spXamlSchemaContext;
    std::shared_ptr<XamlRuntime> spXamlRuntime;

    IFC_RETURN(m_spContext->get_SchemaContext(spXamlSchemaContext));
    IFC_RETURN(spXamlSchemaContext->GetRuntime(spXamlType->get_TypeToken().GetRuntimeKind(), spXamlRuntime));
    IFC_RETURN(spXamlRuntime->InitializationGuard(spXamlType, spInstance, fStartGuard, m_spContext->get_MarkupExtensionContext()));

    return S_OK;
}

HRESULT ObjectWriterRuntime::SetUriBaseImplHelper(
    _In_ const std::shared_ptr<XamlType>& spXamlType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
    _In_ xref_ptr<IPALUri> spBaseUri)
{
    std::shared_ptr<XamlSchemaContext> spXamlSchemaContext;
    std::shared_ptr<XamlRuntime> spXamlRuntime;

    ASSERT(!!spInstance);
    IFC_RETURN(m_spContext->get_SchemaContext(spXamlSchemaContext));
    // TODO: remove when we can pass the type token back and forth through LoadComponent.
    if (!(spInstance->GetTypeToken().IsEmpty()))
    {
        IFC_RETURN(spXamlSchemaContext->GetRuntime(spInstance->GetTypeToken().GetRuntimeKind(), spXamlRuntime));
        IFC_RETURN(spXamlRuntime->SetUriBase(spXamlType, spInstance, spBaseUri));
    }

    return S_OK;
}

HRESULT ObjectWriterRuntime::RaiseTemplateOwnerEventImplHelper(_In_ const std::shared_ptr<XamlQualifiedObject>& qoInstance)
{
    if (EventEnabledSetTemplateOwnerInfo() && !!qoInstance)
    {
        XUINT8 frameLevel = 0;

        std::shared_ptr<XamlType> spFrameworkTemplateXamlType;
        std::shared_ptr<XamlType> spUIElementXamlType;
        std::shared_ptr<XamlType> spStyleXamlType;
        std::shared_ptr<XamlSchemaContext> spSchemaContext;

        IFC_RETURN(m_spContext->get_SchemaContext(spSchemaContext));
        IFC_RETURN(spSchemaContext->GetXamlType(
            XamlTypeToken(tpkNative, KnownTypeIndex::FrameworkTemplate),
            spFrameworkTemplateXamlType));
        IFC_RETURN(spSchemaContext->GetXamlType(
            XamlTypeToken(tpkNative, KnownTypeIndex::Style),
            spStyleXamlType));
        IFC_RETURN(spSchemaContext->GetXamlType(
            XamlTypeToken(tpkNative, KnownTypeIndex::UIElement),
            spUIElementXamlType));

        ASSERT(!!spFrameworkTemplateXamlType);
        ASSERT(!!spStyleXamlType);
        ASSERT(!!spUIElementXamlType);

        for (auto it = m_spContext->get_StackBegin();
            it != m_spContext->get_StackEnd();
            ++it, ++frameLevel)
        {
            bool bCanAssignTo = false;
            std::shared_ptr<XamlType> spFrameType = (*it).get_Type();
            std::shared_ptr<XamlQualifiedObject> qoFrameInstance = (*it).get_Instance();

            if (!!spFrameType || !!qoFrameInstance)
            {
                break;
            }

            if (frameLevel == 0)
            {
                // We only want to raise the SetTemplateOwner event for FrameworkTemplates
                IFC_RETURN(spFrameworkTemplateXamlType->IsAssignableFrom(spFrameType, bCanAssignTo));
                if (!bCanAssignTo)
                {
                    break;
                }
            }
            else if (frameLevel == 1)
            {
                // If the FrameworkTemplate's parent is a UIElement, use it as the OwnerId.
                IFC_RETURN(spUIElementXamlType->IsAssignableFrom(spFrameType, bCanAssignTo));
                if (bCanAssignTo)
                {
                    TraceSetTemplateOwnerInfo(
                        (XUINT64)qoInstance->GetDependencyObject(),
                        (XUINT64)qoFrameInstance->GetDependencyObject(),
                        FALSE);
                    break;
                }
            }
            else if (frameLevel == 3)
            {
                // If the FrameworkTemplate's great grandparent is a Style, use it as the OwnerId.
                // Styles are always three levels up (parent = Setter, grandparent = SetterBaseCollection).
                IFC_RETURN(spStyleXamlType->IsAssignableFrom(spFrameType, bCanAssignTo));
                if (bCanAssignTo)
                {
                    TraceSetTemplateOwnerInfo(
                        (XUINT64)qoInstance->GetDependencyObject(),
                        (XUINT64)qoFrameInstance->GetDependencyObject(),
                        TRUE);
                }
                break;
            }
        }
    }

    return S_OK;
}

HRESULT ObjectWriterRuntime::StoreSourceInformationHelper(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spXamlType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance)
{
    if (m_shouldStoreSourceInformation && spInstance->GetDependencyObject())
    {
        XamlTypeToken tType = spXamlType->get_TypeToken();
        XamlTypeInfoProviderKind providerKind = tType.GetProviderKind();

        std::shared_ptr<XamlSchemaContext> spSchemaContext;
        std::shared_ptr<XamlTypeInfoProvider> spProvider;
        XamlTypeToken tBaseToken;

        unsigned int line = 0;
        unsigned int column = 0;

        bool bIsDO = false;

        tBaseToken = XamlTypeToken::FromType(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::DependencyObject));
        IFC_RETURN(m_spContext->get_SchemaContext(spSchemaContext));
        IFC_RETURN(spSchemaContext->GetTypeInfoProvider(providerKind, spProvider));

        IFC_RETURN(spProvider->IsAssignableFrom(tType, tBaseToken, bIsDO));

        // If this isn't a DO, then we don't need to bother. The source info properties are do properties and
        // Xaml Diagnostics queries an interface implemented by DependencyObject in order to get the source info.
        if (!bIsDO)
        {
            return S_OK;
        }

        line = lineInfo.LineNumber();
        column = lineInfo.LinePosition();

        // Set the line value if we have one.
        if (line != 0)
        {
            std::shared_ptr<XamlProperty> spLineProp;
            std::shared_ptr<XamlQualifiedObject> spLineValue;

            IFC_RETURN(spSchemaContext->GetXamlProperty(
                XamlPropertyToken::FromProperty(DirectUI::MetadataAPI::GetPropertyByIndex(KnownPropertyIndex::DependencyObject_Line)),
                XamlTypeToken::FromType(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::DependencyObject)),
                spLineProp));

            CValue lineVal;
            lineVal.SetSigned(line);
            IFC_RETURN(XamlQualifiedObject::Create(spSchemaContext->GetCore(), XamlTypeToken(), spLineValue));
            IFC_RETURN(spLineValue->SetValue(lineVal));

            IFC_RETURN(SetValue(lineInfo, spXamlType, spInstance, spLineProp, spLineValue));
        }


        // Set the column value if we have one.
        if (column != 0)
        {
            std::shared_ptr<XamlProperty> spColumnProp;
            std::shared_ptr<XamlQualifiedObject> spColumnValue;

            IFC_RETURN(spSchemaContext->GetXamlProperty(
                XamlPropertyToken::FromProperty(DirectUI::MetadataAPI::GetPropertyByIndex(KnownPropertyIndex::DependencyObject_Column)),
                XamlTypeToken::FromType(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::DependencyObject)),
                spColumnProp));

            CValue columnVal;
            columnVal.SetSigned(column);
            IFC_RETURN(XamlQualifiedObject::Create(spSchemaContext->GetCore(), XamlTypeToken(), spColumnValue));
            IFC_RETURN(spColumnValue->SetValue(columnVal));

            IFC_RETURN(SetValue(lineInfo, spXamlType, spInstance, spColumnProp, spColumnValue));

        }

        // Set the parse uri value.
        if (m_resourceUri.IsNull())
        {
            auto resourceUri = m_spContext->get_BaseUri() ? m_spContext->get_BaseUri() : m_spContext->get_XamlResourceUri();

            if (resourceUri)
            {
                IFC_RETURN(resourceUri->GetCanonical(&m_resourceUri));
            }
            else
            {
                m_resourceUri = xstring_ptr::EmptyString();
            }
        }

        std::shared_ptr<XamlProperty> spParseUriProp;
        std::shared_ptr<XamlQualifiedObject> spParseUriValue;

        IFC_RETURN(spSchemaContext->GetXamlProperty(
            XamlPropertyToken::FromProperty(DirectUI::MetadataAPI::GetPropertyByIndex(KnownPropertyIndex::DependencyObject_ParseUri)),
            XamlTypeToken::FromType(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::DependencyObject)),
            spParseUriProp));

        CValue parseUriVal;
        parseUriVal.SetString(m_resourceUri);
        IFC_RETURN(XamlQualifiedObject::Create(spSchemaContext->GetCore(), XamlTypeToken(), spParseUriValue));
        IFC_RETURN(spParseUriValue->SetValue(parseUriVal));

        IFC_RETURN(SetValue(lineInfo, spXamlType, spInstance, spParseUriProp, spParseUriValue));

        // Set the xbf hash value.
        std::shared_ptr<XamlProperty> spHashProp;
        std::shared_ptr<XamlQualifiedObject> spHashValue;

        IFC_RETURN(spSchemaContext->GetXamlProperty(
            XamlPropertyToken::FromProperty(DirectUI::MetadataAPI::GetPropertyByIndex(KnownPropertyIndex::DependencyObject_XbfHash)),
            XamlTypeToken::FromType(MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::DependencyObject)),
            spHashProp));

        CValue hashVal;
        hashVal.SetString(m_spContext->get_XbfHash());
        IFC_RETURN(XamlQualifiedObject::Create(spSchemaContext->GetCore(), XamlTypeToken(), spHashValue));
        IFC_RETURN(spHashValue->SetValue(hashVal));

        IFC_RETURN(SetValue(lineInfo, spXamlType, spInstance, spHashProp, spHashValue));
    }

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterRuntime::AddDeferredProxy(
    _In_ const std::shared_ptr<XamlQualifiedObject>& spParent,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spParentCollection,
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spProxy)
{
    std::shared_ptr<XamlQualifiedObject> spInstance;
    std::shared_ptr<XamlSchemaContext> spXamlSchemaContext;
    std::shared_ptr<XamlRuntime> spXamlRuntime;

    IFC_RETURN(m_spContext->get_SchemaContext(spXamlSchemaContext));

    if (spParent == nullptr)
    {
        // Case for collections
        IFC_RETURN(spXamlSchemaContext->GetRuntime(spParentCollection->GetTypeToken().GetRuntimeKind(), spXamlRuntime));

        switch (m_spContext->get_LiveDepth())
        {
            case 0:
                IFC_RETURN(E_FAIL);
                break;

            case 1:
                // One element, parent will be null - use root
                spInstance = m_spContext->get_RootInstance();
                break;

            default:
                // Many elements, use parent
                spInstance = m_spContext->Parent().get_Instance();
                break;
        }
    }
    else
    {
        // Case for properties
        IFC_RETURN(spXamlSchemaContext->GetRuntime(spProperty->get_PropertyToken().GetRuntimeKind(), spXamlRuntime));
        spInstance = spParent;
    }

    IFC_RETURN(spXamlRuntime->AddDeferredProxy(
        spInstance,
        spParentCollection,
        spProperty->get_PropertyToken(),
        spProxy));

    return S_OK;
}

void ObjectWriterRuntime::TraceElementSourceInfo(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spXamlType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance
)
{
    if (!EventEnabledElementCreatedWithSourceInfo1())
    {
        return;
    }

    auto pCDO = spInstance->GetDependencyObject();
    auto pUIElement = do_pointer_cast<CUIElement>(pCDO);
    if (pUIElement)
    {
        xstring_ptr xamlTypeName;
        IGNOREHR(spXamlType->get_FullName(&xamlTypeName));

        auto resourceUri = m_spContext->get_BaseUri() ? m_spContext->get_BaseUri() : m_spContext->get_XamlResourceUri();
        auto line = lineInfo.LineNumber();
        auto column = lineInfo.LinePosition();

        xstring_ptr canonicalUri;
        if (resourceUri)
        {
            IGNOREHR(resourceUri->GetCanonical(&canonicalUri));
        }

        // We'll fire this event regardless if there is source info or not. At a minimum, we'll generally get the filename which is important.
        // It is possible to not even have a file name if the XamlReader::Load API is used to load the xaml string. We should still fire this
        // event even in this condition so we can get the type name.
        TraceElementCreatedWithSourceInfo1(
            reinterpret_cast<UINT64>(pCDO), xamlTypeName.GetBuffer(), canonicalUri.GetBuffer(),
            line, column, m_spContext->get_XbfHash().GetBuffer());
    }
}
