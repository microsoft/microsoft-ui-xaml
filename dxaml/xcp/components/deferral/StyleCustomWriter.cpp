// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "StyleCustomWriter.h"
#include "StyleCustomRuntimeData.h"
#include "ICustomWriterCallbacks.h"
#include <StreamOffsetToken.h>
#include <XamlProperty.h>
#include <XamlQualifiedObject.h>
#include <XamlSchemaContext.h>
#include <XamlTextSyntax.h>
#include <CString.h>
#include <ObjectWriter.h>
#include <ObjectWriterContext.h>
#include <MetadataAPI.h>
#include <TypeTableStructs.h>
#include <XamlBinaryMetadataStore.h>
#include <XamlBinaryFormatStringConverter.h>
#include <XamlPredicateHelpers.h>

//#define STYLELOG(...) STYLELOG(__VA_ARGS__)
#define STYLELOG(...)

StyleCustomWriter::StyleCustomWriter(
    _In_ ICustomWriterCallbacks* callbacks,
    _In_ std::shared_ptr<XamlSchemaContext> context,
    _In_ std::shared_ptr<ObjectWriterContext> objectWriterContext)
    : m_customWriterCallbacks(callbacks)
    , m_stackDepth(0)
    , m_context(context)
    , m_objectWriterContext(objectWriterContext)
    , m_allowNewCustomWriter(true)
{
}

_Check_return_ HRESULT StyleCustomWriter::Initialize()
{
    m_runtimeData = std::unique_ptr<StyleCustomRuntimeData>(new StyleCustomRuntimeData());

    IFC_RETURN(m_context->get_X_KeyProperty(m_spXKeyProperty));
    IFC_RETURN(m_context->get_X_NameProperty(m_spXNameProperty));
    m_core = m_context->GetCore();

    STYLELOG(L"[STYLECW]: Created. Beginning custom writer actions for Style.");
    return S_OK;
}

_Check_return_ HRESULT StyleCustomWriter::Create(
    _In_ ICustomWriterCallbacks* callbacks,
    _In_ std::shared_ptr<XamlSchemaContext> context,
    _In_ std::shared_ptr<ObjectWriterContext> objectWriterContext,
    _Out_ std::unique_ptr<ICustomWriter>* ppValue)
{
    auto pWriter = std::unique_ptr<StyleCustomWriter>(new StyleCustomWriter(callbacks, context, objectWriterContext));
    IFC_RETURN(pWriter->Initialize());
    *ppValue = std::move(pWriter);
    return S_OK;
}

_Check_return_ HRESULT StyleCustomWriter::WriteObject(
    _In_ const std::shared_ptr<XamlType>& spXamlType,
    _In_ bool bFromMember,
    _Out_ bool* pResult)
{
    if (m_stackDepth == 0)
    {
        STYLELOG(L"[STYLECW]: Beginning capture of Style.");
        m_pendingOperations.emplace_back(m_stackDepth, Operation::WritingStyle);
    }
    else
    {
        auto currentOperation = m_pendingOperations.back().second;

        switch (currentOperation)
        {
            case Operation::WritingStyleBasedOn:
            {
                // If we enter here, the Style BasedOn property is some kind of object (e.g. StaticResource).
                // Ignore any nested writes for it.

                STYLELOG(L"[STYLECW]: Beginning capture of object in Style BasedOn.");
                m_pendingOperations.emplace_back(m_stackDepth, Operation::WritingStyleBasedOnObject);
                break;
            }
            case Operation::WritingStyleSetters:
            {
                // Starting the setters collection.

                if (IsType(spXamlType, KnownTypeIndex::SetterBaseCollection))
                {
                    STYLELOG(L"[STYLECW]: Beginning capture of optimized Style.");
                    m_pendingOperations.emplace_back(m_stackDepth, Operation::WritingSetterBaseCollection);
                }
                else
                {
                    ASSERT(FALSE);
                }
                break;
            }
            case Operation::WritingSetterBaseCollection:
            {
                // Starting a new setter.

                if (IsType(spXamlType, KnownTypeIndex::Setter))
                {
                    STYLELOG(L"[STYLECW]: Beginning capture of Setter.");
                    m_runtimeData->m_setters.push_back(StyleSetterEssence());
                    // Setters marked as conditional should not be optimized, nor should they be
                    // optimized if Property/Value is marked as conditional.
                    // TODO: Add correct check to disable optimization if conditional.
                    // OptimizedStyle will instantiate the full Setter object at runtime.
                    m_currentSetterInfo.shouldOptimize = true;
                    m_runtimeData->m_offsetTokenStates.emplace_back(true, true);
                    IFC_RETURN(m_customWriterCallbacks->CreateStreamOffsetToken(&m_currentSetterInfo.offsetToken));
                    m_pendingOperations.emplace_back(m_stackDepth, Operation::WritingSetter);
                }
                else
                {
                    ASSERT(FALSE);
                }
                break;
            }
            case Operation::WritingSetterValue:
            {
                // If we enter here, the Setter Value property is some kind of object.

                STYLELOG(L"[STYLECW]: Beginning capture of object in Setter Value.");

                if (IsType(spXamlType, KnownTypeIndex::StaticResource))
                {
                    m_runtimeData->m_offsetTokenStates.emplace_back(false, true);
                    StreamOffsetToken token;
                    IFC_RETURN(m_customWriterCallbacks->CreateStreamOffsetToken(&token));
                    m_runtimeData->m_setters.back().SetStaticResourceValue(token);
                    m_pendingOperations.emplace_back(m_stackDepth, Operation::WritingSetterValueObject);
                }
                else if (IsType(spXamlType, KnownTypeIndex::ThemeResource))
                {
                    m_runtimeData->m_offsetTokenStates.emplace_back(false, true);
                    StreamOffsetToken token;
                    IFC_RETURN(m_customWriterCallbacks->CreateStreamOffsetToken(&token));
                    m_runtimeData->m_setters.back().SetThemeResourceValue(token);
                    m_pendingOperations.emplace_back(m_stackDepth, Operation::WritingSetterValueObject);
                }
                else if (IsType(spXamlType, KnownTypeIndex::CustomResource))
                {
                    m_runtimeData->m_offsetTokenStates.emplace_back(false, true);
                    StreamOffsetToken token;
                    IFC_RETURN(m_customWriterCallbacks->CreateStreamOffsetToken(&token));
                    m_runtimeData->m_setters.back().SetObjectValue(token);
                    m_pendingOperations.emplace_back(m_stackDepth, Operation::WritingSetterValueObject);
                }
                else
                {
                    bool isMarkupExtension = false;
                    IFC_RETURN(spXamlType->IsMarkupExtension(isMarkupExtension));

                    // Setters with a custom markup extension providing the value for Setter.Value
                    // can't be optimized, otherwise ProvideValue() doesn't get called
                    if (isMarkupExtension && !DirectUI::MetadataAPI::IsKnownIndex(spXamlType->get_TypeToken().GetHandle()))
                    {
                        m_currentSetterInfo.shouldOptimize = false;
                    }
                    else
                    {
                        m_runtimeData->m_offsetTokenStates.emplace_back(false, true);
                        StreamOffsetToken token;
                        IFC_RETURN(m_customWriterCallbacks->CreateStreamOffsetToken(&token));
                        m_runtimeData->m_setters.back().SetObjectValue(token);
                    }
                    m_pendingOperations.emplace_back(m_stackDepth, Operation::WritingSetterValueObject);
                }

                break;
            }
            case Operation::WritingStyle:
            case Operation::WritingStyleBasedOnObject:
            case Operation::WritingSetterValueObject:
            {
                break;
            }
            default:
            {
                ASSERT(FALSE);
            }
        }
    }

    m_stackDepth++;

    *pResult = ShouldHandleOperation(m_pendingOperations.back().second);
    return S_OK;
}

_Check_return_ HRESULT StyleCustomWriter::WriteEndObject(_Out_ bool* pResult)
{
    ASSERT(m_stackDepth > 0);
    --m_stackDepth;

    auto currentOperationDepth = m_pendingOperations.back().first;
    auto currentOperation = m_pendingOperations.back().second;

    switch (currentOperation)
    {
        case Operation::WritingStyle:
        case Operation::WritingStyleBasedOnObject:
        case Operation::WritingSetterBaseCollection:
        case Operation::WritingSetterValueObject:
        {
            break;
        }
        case Operation::WritingSetter:
        {
            if (m_currentSetterInfo.shouldOptimize)
            {
                // We're optimizing the setter nodes. Convert the setter's value
                // from a string to an actual value if possible.
                IFC_RETURN(ResolveSetterValue(m_runtimeData->m_setters.back()));
            }
            else
            {
                // We're skipping optimization of the setter nodes. Set the setter's offset
                // token and the offset token state's 'optimized' flag so the setter's nodes
                // will be preserved in the stream (see StyleCustomRuntimeData::PrepareStream).
                m_runtimeData->m_setters.back().SetSetterObject(m_currentSetterInfo.offsetToken, m_currentSetterInfo.isMutable);
                m_runtimeData->m_offsetTokenStates[m_currentSetterInfo.offsetToken.GetIndex()].second = false;
            }
            m_currentSetterInfo.reset();
            break;
        }
        default:
        {
            ASSERT(FALSE);
        }
    }

    if (currentOperationDepth == m_stackDepth)
    {
        m_pendingOperations.pop_back();
        STYLELOG(L"[STYLECW]: Finishing capture of object.");
    }

    // If this is the last pending operation after this call returns,
    // the CustomWriter will be destroyed.
    if (m_pendingOperations.empty())
    {
        IFC_RETURN(m_customWriterCallbacks->SetCustomWriterRuntimeData(std::move(m_runtimeData)));
    }

    *pResult = ShouldHandleOperation(currentOperation);
    return S_OK;
}

_Check_return_ HRESULT StyleCustomWriter::WriteMember(
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _Out_ bool* pResult)
{
    auto currentOperation = m_pendingOperations.back().second;

    switch (currentOperation)
    {
        case Operation::WritingStyle:
        {
            // Expect Style Setters, TargetType, BasedOn, x:Key, and x:Name members.
            // Allow additional members as "unknown" and ignore any nested writes for them.
            // Also temporarily store the XamlProperty so we can use it when
            // we resolve the style's TargetType.

            ASSERT(m_currentStyleXamlProperty == nullptr);

            m_currentStyleXamlProperty = spProperty;

            if (IsProperty(spProperty, KnownPropertyIndex::Style_Setters))
            {
                STYLELOG(L"[STYLECW]: Beginning member write of Style_Setters property.");
                m_pendingOperations.emplace_back(m_stackDepth, Operation::WritingStyleSetters);
            }
            else if (IsProperty(spProperty, KnownPropertyIndex::Style_TargetType))
            {
                STYLELOG(L"[STYLECW]: Beginning member write of Style_TargetType property.");
                m_pendingOperations.emplace_back(m_stackDepth, Operation::WritingStyleTargetType);
            }
            else if (IsProperty(spProperty, KnownPropertyIndex::Style_BasedOn))
            {
                STYLELOG(L"[STYLECW]: Beginning member write of Style_BasedOn property.");
                m_pendingOperations.emplace_back(m_stackDepth, Operation::WritingStyleBasedOn);
            }
            else if (XamlProperty::AreEqual(spProperty, m_spXKeyProperty))
            {
                STYLELOG(L"[STYLECW]: Beginning member write of Style x:Key property.");
                m_pendingOperations.emplace_back(m_stackDepth, Operation::WritingStyleXKey);
            }
            else if (XamlProperty::AreEqual(spProperty, m_spXNameProperty))
            {
                STYLELOG(L"[STYLECW]: Beginning member write of Style x:Name property.");
                m_pendingOperations.emplace_back(m_stackDepth, Operation::WritingStyleXName);
            }
            else
            {
                STYLELOG(L"[STYLECW]: Beginning member write of unknown Style property.");
                m_pendingOperations.emplace_back(m_stackDepth, Operation::WritingStyleUnknownProperty);
            }

            break;
        }
        case Operation::WritingSetter:
        {
            // Expect only Setter Property and Value members.

            if (spProperty->IsDirective())
            {
                STYLELOG(L"[STYLECW]: Beginning member write of Setter Directive.");
                m_pendingOperations.emplace_back(m_stackDepth, Operation::WritingSetterDirective);

                if (   std::static_pointer_cast<DirectiveProperty>(spProperty)->get_DirectiveKind() == xdName
                    || std::static_pointer_cast<DirectiveProperty>(spProperty)->get_DirectiveKind() == xdConnectionId)
                {
                    m_currentSetterInfo.isMutable = true;
                }

                // Skip optimizing the setter nodes if the setter has x:Uid, or is mutable
                m_currentSetterInfo.shouldOptimize = m_currentSetterInfo.shouldOptimize
                    && std::static_pointer_cast<DirectiveProperty>(spProperty)->get_DirectiveKind() != xdUid
                    && !m_currentSetterInfo.isMutable;
            }
            else if (IsProperty(spProperty, KnownPropertyIndex::Setter_Property))
            {
                STYLELOG(L"[STYLECW]: Beginning member write of Setter Property.");
                m_pendingOperations.emplace_back(m_stackDepth, Operation::WritingSetterProperty);
            }
            else if (IsProperty(spProperty, KnownPropertyIndex::Setter_Target))
            {
                STYLELOG(L"[STYLECW]: Beginning member write of Setter Target.");
                m_pendingOperations.emplace_back(m_stackDepth, Operation::WritingSetterTarget);
            }
            else if (IsProperty(spProperty, KnownPropertyIndex::Setter_Value))
            {
                STYLELOG(L"[STYLECW]: Beginning member write of Setter Value.");
                m_pendingOperations.emplace_back(m_stackDepth, Operation::WritingSetterValue);
                m_allowNewCustomWriter = false;
            }
            else
            {
                ASSERT(FALSE);
            }

            break;
        }
        case Operation::WritingStyleBasedOnObject:
        case Operation::WritingSetterBaseCollection:
        case Operation::WritingSetterValueObject:
        {
            break;
        }
        case Operation::WritingStyleSetters:
        {
            break;
        }
        default:
        {
            ASSERT(FALSE);
        }
    }

    m_stackDepth++;

    *pResult = ShouldHandleOperation(m_pendingOperations.back().second);
    return S_OK;
}

_Check_return_ HRESULT StyleCustomWriter::WriteEndMember(_Out_ bool* pResult)
{
    ASSERT(m_stackDepth > 0);
    --m_stackDepth;

    auto currentOperationDepth = m_pendingOperations.back().first;
    auto currentOperation = m_pendingOperations.back().second;

    m_currentStyleXamlProperty = nullptr;

    switch (currentOperation)
    {
        case Operation::WritingStyleXKey:
        case Operation::WritingStyleXName:
        case Operation::WritingStyleTargetType:
        case Operation::WritingStyleBasedOn:
        case Operation::WritingStyleUnknownProperty:
        {
            if (currentOperationDepth == m_stackDepth)
            {
                STYLELOG(L"[STYLECW]: Completed write member operation.");
                m_pendingOperations.pop_back();
            }
            break;
        }
        case Operation::WritingStyleSetters:
        case Operation::WritingSetterDirective:
        case Operation::WritingSetterProperty:
        case Operation::WritingSetterTarget:
        {
            if (currentOperationDepth == m_stackDepth)
            {
                STYLELOG(L"[STYLECW]: Completed write member operation.");
                m_pendingOperations.pop_back();
            }
            break;
        }
        case Operation::WritingSetterValue:
        {
            if (currentOperationDepth == m_stackDepth)
            {
                STYLELOG(L"[STYLECW]: Completed write member operation.");
                m_pendingOperations.pop_back();
                m_allowNewCustomWriter = true;
            }
            break;
        }
        case Operation::WritingSetterBaseCollection:
        case Operation::WritingStyleBasedOnObject:
        case Operation::WritingSetterValueObject:
        {
            break;
        }
        default:
        {
            ASSERT(FALSE);
        }
    }

    *pResult = ShouldHandleOperation(currentOperation);
    return S_OK;
}

_Check_return_ HRESULT StyleCustomWriter::WriteConditionalScope(
    _In_ const std::shared_ptr<Parser::XamlPredicateAndArgs>& /* unused */,
    _Out_ bool* pResult)
{
    // TODO: Complete properly (should push current depth as well as the XamlPredicateAndArgs onto a stack
    // which is then consulted when we encounter Style.Setters [should store predicate in overall CWRD to control
    // whether or not a zero Setter count needs to be reported at runtime], or when we're about to create a
    // StyleSetterEssence [store predicate in the StyleSetterEssence and evaluate at runtime to determine if
    // the OptimizedStyle should skip over that particular StyleSetterEssence])
    // For now, only allow conditional XAML within the Setter.Value's value; all other instances (including if the
    // Setter.Value object itself is declared conditionally, although this would be pretty dumb to do regardless
    // since it can potentially result in Setter.Value having no value) will result in an exception.
    if (m_pendingOperations.back().second != Operation::WritingSetterValueObject)
    {
        IFC_RETURN(E_FAIL);
    }

    *pResult = true;
    return S_OK;
}

_Check_return_ HRESULT StyleCustomWriter::WriteEndConditionalScope(_Out_ bool* pResult)
{
    // TODO: Complete properly (see comment on WriteConditionalScope()).
    // For now, only allow conditional XAML within the Setter.Value's value; all other instances will result in an
    // exception.
    if (m_pendingOperations.back().second != Operation::WritingSetterValueObject)
    {
        IFC_RETURN(E_FAIL);
    }

    *pResult = true;
    return S_OK;
}

_Check_return_ HRESULT StyleCustomWriter::WriteValue(
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
    _Out_ bool* pResult)
{
    auto currentOperation = m_pendingOperations.back().second;

    switch (currentOperation)
    {
        case Operation::WritingStyleTargetType:
        {
            // Resolve the Style TargetType from a string value to KnownTypeIndex.
            // Also temporarily store the corresponding XamlType so we can use it when
            // we resolve the style's setters.

            std::shared_ptr<XamlTextSyntax> spTextSyntax;
            std::shared_ptr<XamlType> spType;
            std::shared_ptr<XamlType> spTargetXamlType;

            ASSERT(m_targetXamlType == nullptr);
            ASSERT(m_currentStyleXamlProperty != nullptr);

            IFC_RETURN(m_currentStyleXamlProperty->get_Type(spType));
            IFC_RETURN(spType->get_TextSyntax(spTextSyntax));

            ASSERT(IsType(spTextSyntax->get_TextSyntaxToken(), KnownTypeIndex::TypeName));
            ASSERT(spValue->GetValue().GetType() == valueString);

            IFC_RETURN(m_objectWriterContext->get_MarkupExtensionContext()->ResolveXamlType(spValue->GetValue().AsString(), spTargetXamlType));

            m_targetXamlType = spTargetXamlType;

            break;
        }
        case Operation::WritingSetterProperty:
        case Operation::WritingSetterTarget:
        {
            // Resolve the Setter Target from a string value to XamlProperty.

            std::shared_ptr<XamlProperty> xamlProperty;
            std::shared_ptr<XamlType> xamlType;
            xstring_ptr propertyName;
            xstring_ptr stringValue;

            GetStringValue(spValue, stringValue);

            ASSERT(!stringValue.IsNullOrEmpty());

            // Get the property name and optionally resolve the type if the value was of
            // the form "type.property".
            auto ichDot = stringValue.FindChar(L'.');
            if (ichDot != xstring_ptr_view::npos)
            {
                xstring_ptr typeName;
                unsigned int length = stringValue.GetCount();

                if (currentOperation == Operation::WritingSetterTarget)
                {
                    // Setter.Target uses property-path syntax, which requires parentheses
                    // around fully qualified property names (e.g. "(Canvas.Left)" vs "Canvas.Left")
                    // so those need to be removed first
                    bool hasParentheses = stringValue.GetChar(0) == L'(' && stringValue.GetChar(length - 1) == L')';
                    IFC_RETURN((hasParentheses) ? S_OK : E_FAIL);

                    IFC_RETURN(stringValue.SubString(1, ichDot, &typeName));
                    IFC_RETURN(stringValue.SubString(ichDot + 1, length - 1, &propertyName));
                }
                else
                {
                    IFC_RETURN(stringValue.SubString(0, ichDot, &typeName));
                    IFC_RETURN(stringValue.SubString(ichDot + 1, length, &propertyName));
                }

                IFC_RETURN(m_objectWriterContext->get_MarkupExtensionContext()->ResolveXamlType(typeName, xamlType));
            }
            else
            {
                ASSERT(m_targetXamlType != nullptr);
                xamlType = m_targetXamlType;
                propertyName = stringValue;
            }

            if (xamlType)
            {
                IFC_RETURN(xamlType->GetDependencyProperty(propertyName, xamlProperty));
            }
            else
            {
                IFC_RETURN(E_FAIL);
            }

            if (xamlProperty == nullptr)
            {
                m_runtimeData->m_setters.back().SetUnresolvedPropertyData(xamlType, propertyName);
            }
            else
            {
                m_runtimeData->m_setters.back().SetResolvedPropertyData(xamlProperty);
            }

            break;
        }
        case Operation::WritingSetterValue:
        {
            // If we enter here, the Setter Value property is just a string value.
            // We'll try to resolve it to an actual value when the setter object ends.

            xstring_ptr stringValue;
            GetStringValue(spValue, stringValue);
            m_runtimeData->m_setters.back().SetContainerValue(stringValue);

            break;
        }
        case Operation::WritingStyleSetters:
        case Operation::WritingStyleXKey:
        case Operation::WritingStyleXName:
        case Operation::WritingStyleBasedOn:
        case Operation::WritingStyleUnknownProperty:
        case Operation::WritingSetterDirective:
        case Operation::WritingSetterValueObject:
        case Operation::WritingSetterBaseCollection:
        case Operation::WritingStyleBasedOnObject:
        {
            break;
        }
        default:
        {
            ASSERT(FALSE);
        }
    }

    *pResult = ShouldHandleOperation(currentOperation);
    return S_OK;
}

bool StyleCustomWriter::ShouldHandleOperation(_In_ Operation operation)
{
    bool shouldHandle = true;

    switch(operation)
    {
        case Operation::WritingStyle:
        case Operation::WritingStyleSetters:
        case Operation::WritingSetterBaseCollection:
        case Operation::WritingSetter:
        case Operation::WritingSetterDirective:
        case Operation::WritingSetterProperty:
        case Operation::WritingSetterTarget:
        case Operation::WritingSetterValue:
        case Operation::WritingSetterValueObject:
            shouldHandle = true;
            break;
        case Operation::WritingStyleXKey:
        case Operation::WritingStyleXName:
        case Operation::WritingStyleTargetType:
        case Operation::WritingStyleBasedOn:
        case Operation::WritingStyleBasedOnObject:
        case Operation::WritingStyleUnknownProperty:
            shouldHandle = false;
            break;
    }

    return shouldHandle;
}

_Check_return_ HRESULT StyleCustomWriter::WriteNamespace(
    _In_ const xstring_ptr& spPrefix,
    _In_ const std::shared_ptr<XamlNamespace>& spXamlNamespace,
    _Out_ bool* pResult)
{
    STYLELOG(L"[STYLECW]: WriteNamespace passed in.");
    *pResult = true;
    return S_OK;
}

#pragma endregion

bool StyleCustomWriter::IsProperty(_In_ _Const_ const std::shared_ptr<XamlProperty>& spProperty, KnownPropertyIndex propIndex)
{
    const CDependencyProperty* prop = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(propIndex);
    return spProperty->get_PropertyToken().Equals(XamlPropertyToken::FromProperty(prop));
}

bool StyleCustomWriter::IsType(_In_ _Const_ const std::shared_ptr<XamlType>& spType, KnownTypeIndex typeIndex)
{
    return IsType(spType->get_TypeToken(), typeIndex);
}

bool StyleCustomWriter::IsType(_In_ _Const_ const XamlTypeToken& typeToken, KnownTypeIndex typeIndex)
{
    const CClassInfo* type = DirectUI::MetadataAPI::GetClassInfoByIndex(typeIndex);
    return typeToken.Equals(XamlTypeToken::FromType(type));
}

void StyleCustomWriter::GetStringValue(_In_ _Const_ const std::shared_ptr<XamlQualifiedObject>& spValue, _In_ xstring_ptr& stringValue)
{
    if (spValue->GetValue().GetType() == ValueType::valueString)
    {
        stringValue = spValue->GetValue().AsString();

    }
    else if (IsType(spValue->GetTypeToken(), KnownTypeIndex::String) && spValue->GetDependencyObject() != nullptr)
    {
        stringValue = static_cast<CString*>(spValue->GetDependencyObject())->m_strString;
    }
}

// Convert setter value from string to actual value if possible.
_Check_return_ HRESULT StyleCustomWriter::ResolveSetterValue(StyleSetterEssence& setter)
{
    if (setter.HasContainerValue() && setter.IsPropertyResolved())
    {
        std::shared_ptr<XamlTextSyntax> spTextSyntax;

        IFC_RETURN(setter.GetXamlProperty()->get_TextSyntax(spTextSyntax));

        XamlBinaryFormatStringConverter converter(m_core);
        auto& valueContainer = setter.GetValueContainer();

        converter.TryConvertValue(spTextSyntax->get_TextSyntaxToken(), valueContainer);
    }

    return S_OK;
}

// ICustomWriter
bool StyleCustomWriter::IsCustomWriterFinished() const
{
    return m_pendingOperations.empty();
}

StyleCustomWriterActivator::StyleCustomWriterActivator(
    _In_ const std::shared_ptr<ObjectWriterContext>& spContext)
    : m_spContext(spContext)
{}

_Check_return_ HRESULT StyleCustomWriterActivator::ShouldActivateCustomWriter(
    _In_ const CustomWriterActivatorState& activatorState,
    _Out_ bool* pResult)
{
#ifdef DBG
    if (activatorState.GetTrigger() == ActivatorTrigger::ObjectStart)
    {
        ASSERT(activatorState.GetObject(nullptr));
    }
#endif

    *pResult = activatorState.GetTrigger() == ActivatorTrigger::ObjectStart &&
           activatorState.GetObject(nullptr)->get_TypeToken().GetHandle() == KnownTypeIndex::Style;
    return S_OK;
}

_Check_return_ HRESULT StyleCustomWriterActivator::CreateNodeStreamCollectingWriter(
    _In_ const CustomWriterActivatorState& activatorState,
    _Out_ std::shared_ptr<ObjectWriter>* pWriter)
{
    std::shared_ptr<XamlSavedContext> spSavedContext;
    IFC_RETURN(m_spContext->GetSavedContext(spSavedContext));

    ObjectWriterSettings settings;
    settings.set_EnableEncoding(true);
    settings.set_ExpandTemplates(m_spContext->get_ExpandTemplates());

    std::shared_ptr<ObjectWriter> spNodeStreamCollectingWriter;

    IFC_RETURN(ObjectWriter::Create(
        spSavedContext,
        settings,
        spNodeStreamCollectingWriter));

    // Add namespaces
    for (const auto& namespacePair : activatorState.GetNamespaces())
    {
        IFC_RETURN(spNodeStreamCollectingWriter->WriteNamespace(
            namespacePair.first,
            namespacePair.second));
    }

    *pWriter = spNodeStreamCollectingWriter;
    return S_OK;
}

_Check_return_ HRESULT StyleCustomWriterActivator::CreateCustomWriter(
    _In_ ICustomWriterCallbacks* pCallbacks,
    _In_ const CustomWriterActivatorState& activatorState,
    _Out_ std::unique_ptr<ICustomWriter>* pWriter)
{
    std::shared_ptr<XamlSchemaContext> spSchemaContext;
    IFC_RETURN(m_spContext->get_SchemaContext(spSchemaContext));
    IFC_RETURN(StyleCustomWriter::Create(pCallbacks, spSchemaContext, m_spContext, pWriter));
    return S_OK;
}
