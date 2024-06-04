// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "DeferredElementCustomWriter.h"
#include "DeferredElementCustomRuntimeData.h"
#include "MetadataAPI.h"
#include <ICustomWriterCallbacks.h>
#include <XamlProperty.h>
#include <XamlSchemaContext.h>
#include <XamlTextSyntax.h>
#include <XamlType.h>
#include <XamlQualifiedObject.h>
#include <ObjectWriter.h>
#include <ObjectWriterContext.h>
#include <SavedContext.h>
#include <ObjectWriterErrorService.h>
#include <TypeTableStructs.h>
#include <XamlBinaryFormatStringConverter.h>
#include <xcperrorresource.h>
#include <XamlPredicateHelpers.h>

//#define DELOG(...) LOG(__VA_ARGS__)
#define DELOG(...)

DeferredElementCustomWriter::DeferredElementCustomWriter(
    _In_ ICustomWriterCallbacks* callbacks,
    _In_ std::shared_ptr<ObjectWriterContext> spContext)
    : m_customWriterCallbacks(callbacks)
    , m_spContext(spContext)
    , m_runtimeData(new DeferredElementCustomRuntimeData())
    , m_stackDepth(0)
    , m_allowNewCustomWriter(false)
{
    m_customWriterCallbacks->SetAllowProcessStartObjectByCustomWriter(true);
}

_Check_return_ HRESULT DeferredElementCustomWriter::WriteObject(
    _In_ const std::shared_ptr<XamlType>& spXamlType,
    _In_ bool bFromMember,
    _Out_ bool* pResult)
{
    if (m_stackDepth == 0)
    {
        DELOG(L"[DECW]: Beginning capture of deferred element.");
        m_pendingOperations.push_back(std::make_pair(m_stackDepth, Operation::WritingDeferredElement));
    }
    else
    {
        auto currentOperation = m_pendingOperations.back().second;
        ASSERT(currentOperation != Operation::None);

        switch (currentOperation)
        {
            case Operation::WritingNonDeferredProperty:
            {
                // Error: Currently, deferred elements only support properties being 
                // set using attribute syntax and data types. Other possible options 
                // such as markup extensions are not supported and will fail through here.
                std::unique_ptr<ObjectWriterErrorService> errorService(new ObjectWriterErrorService(m_spContext, true));
                IFC_RETURN(errorService->ReportError(AG_E_PARSER2_INVALID_PROPERTY_ON_DEFERRED_ELEMENT, GetLineInfo()));
                IFC_RETURN(E_FAIL);
            }
        }
    }

    ++m_stackDepth;
    *pResult = true;
    return S_OK;
}

_Check_return_ HRESULT DeferredElementCustomWriter::WriteEndObject(_Out_ bool* pResult)
{
    ASSERT(m_stackDepth > 0);
    --m_stackDepth;

    auto currentOperationDepth = m_pendingOperations.back().first;
    auto currentOperation = m_pendingOperations.back().second;
    ASSERT(currentOperation != Operation::None);

    switch (currentOperation)
    {
        case Operation::WritingDeferredElement:
        {
            if (currentOperationDepth == m_stackDepth)
            {
                m_pendingOperations.pop_back();
                DELOG(L"[DECW]: Finishing capture of deferred element.");
            }
            break;
        }
    }

    if (m_stackDepth == 0)
    {
        if (m_runtimeData->m_strName.IsNullOrEmpty())
        {
            // Error: Name needs to be set on deferred element.
            std::unique_ptr<ObjectWriterErrorService> errorService(new ObjectWriterErrorService(m_spContext, true));
            IFC_RETURN(errorService->ReportDeferredElementMustHaveName(GetLineInfo()));
        }

        IFC_RETURN(m_customWriterCallbacks->SetCustomWriterRuntimeData(
            std::unique_ptr<CustomWriterRuntimeData>(m_runtimeData.release())));

        m_customWriterCallbacks->SetAllowProcessStartObjectByCustomWriter(true);
    }

    *pResult = true;
    return S_OK;
}

_Check_return_ HRESULT DeferredElementCustomWriter::WriteMember(
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _Out_ bool* pResult)
{
    ASSERT(m_stackDepth > 0);

    Operation beginOperation = Operation::None;

    // Only collect attributes on root element.

    if (m_stackDepth == 1)
    {
        if (spProperty->IsDirective())
        {
            switch (std::static_pointer_cast<DirectiveProperty>(spProperty)->get_DirectiveKind())
            {
                case xdDeferLoadStrategy:
                    beginOperation = Operation::WritingDeferLoadStrategyDirective;
                    break;

                case xdLoad:
                    beginOperation = Operation::WritingRealizeDirective;
                    break;

                case xdName:
                    // x:Name directive
                    beginOperation = Operation::WritingDeferredElementXName;
                    break;
            }
        }
        else if (spProperty->get_PropertyToken().Equals(
                    XamlPropertyToken::FromProperty(
                        DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::DependencyObject_Name))))
        {
            // Name property
            beginOperation = Operation::WritingDeferredElementXName;
        }

        if (beginOperation != Operation::None)
        {
            m_pendingOperations.push_back(std::make_pair(m_stackDepth, beginOperation));
        }
    }

    if (IsNonDeferredProperty(spProperty))
    {
        DELOG(L"[DECW]: Beginning member write of non-deferred property.");
        ASSERT(beginOperation == Operation::None);
        m_currentDeferredElementXamlProperty = spProperty;
        m_pendingOperations.push_back(std::make_pair(m_stackDepth, Operation::WritingNonDeferredProperty));
    }

    ++m_stackDepth;

    *pResult = true;
    return S_OK;
}

_Check_return_ HRESULT DeferredElementCustomWriter::WriteEndMember(_Out_ bool* pResult)
{
    ASSERT(m_stackDepth > 0);
    --m_stackDepth;

    auto currentOperationDepth = m_pendingOperations.back().first;
    auto currentOperation = m_pendingOperations.back().second;
    ASSERT(currentOperation != Operation::None);

    switch (currentOperation)
    {
        case Operation::WritingDeferredElementXName:
        case Operation::WritingNonDeferredProperty:
        case Operation::WritingDeferLoadStrategyDirective:
        case Operation::WritingRealizeDirective:
            if (currentOperationDepth == m_stackDepth)
            {
                DELOG(L"[DECW]: Completed write member operation.");
                m_currentDeferredElementXamlProperty = nullptr;
                m_pendingOperations.pop_back();
            }
            break;
    }

    *pResult = true;
    return S_OK;
}

_Check_return_ HRESULT DeferredElementCustomWriter::WriteValue(
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
    _Out_ bool* pResult)
{
    ASSERT(m_stackDepth > 0);

    auto currentOperation = m_pendingOperations.back().second;
    ASSERT(currentOperation != Operation::None);

    switch (currentOperation)
    {
        case Operation::WritingDeferredElementXName:
            IFC_RETURN(spValue->GetCopyAsString(&m_runtimeData->m_strName));
            break;

        case Operation::WritingNonDeferredProperty:
            {
                ASSERT(m_currentDeferredElementXamlProperty != nullptr);

                std::shared_ptr<XamlType> type;
                std::shared_ptr<XamlTextSyntax> textSyntax;
                std::shared_ptr<XamlSchemaContext> schemaContext;

                IFC_RETURN(m_currentDeferredElementXamlProperty->get_Type(type));
                IFC_RETURN(type->get_TextSyntax(textSyntax));

                IFC_RETURN(m_spContext->get_SchemaContext(schemaContext));
                CCoreServices *core = schemaContext->GetCore();

                // At this point, the valueBools are actually valueStrings with the
                // strings "True" or "False". Thus, we will convert them into valueBools accordingly.
                XamlBinaryFormatStringConverter converter(core);
                auto& value = spValue->GetValue();
                converter.TryConvertValue(textSyntax->get_TextSyntaxToken(), spValue->GetValue());

                m_runtimeData->m_nonDeferredProperties.push_back(std::make_pair(m_currentDeferredElementXamlProperty, value));

                break;
            }

        case Operation::WritingDeferLoadStrategyDirective:
            m_runtimeData->m_realize = false;
            break;

        case Operation::WritingRealizeDirective:
            {
                std::shared_ptr<XamlSchemaContext> schemaContext;
                IFC_RETURN(m_spContext->get_SchemaContext(schemaContext));

                CValue value = spValue->GetValue();

                bool convertResult = XamlBinaryFormatStringConverter(schemaContext->GetCore()).TryConvertValue(
                    XamlTypeToken::FromType(DirectUI::MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Boolean)),
                    value);
                IFC_RETURN(convertResult ? S_OK : E_INVALIDARG);

                ASSERT(value.GetType() == valueBool);

                m_runtimeData->m_realize = value.As<valueBool>();
            }
            break;
    }

    *pResult = true;
    return S_OK;
}

_Check_return_ HRESULT DeferredElementCustomWriter::WriteConditionalScope(
    _In_ const std::shared_ptr<Parser::XamlPredicateAndArgs>& /* unused */,
    _Out_ bool* pResult)
{
    *pResult = true;
    return S_OK;
}

_Check_return_ HRESULT DeferredElementCustomWriter::WriteEndConditionalScope(_Out_ bool* pResult)
{
    *pResult = true;
    return S_OK;
}

_Check_return_ HRESULT DeferredElementCustomWriter::WriteNamespace(
    _In_ const xstring_ptr& spPrefix,
    _In_ const std::shared_ptr<XamlNamespace>& spXamlNamespace,
    _Out_ bool* pResult)
{
    *pResult = true;
    return S_OK;
}

// ICustomWriter
bool DeferredElementCustomWriter::IsCustomWriterFinished() const
{
    return m_stackDepth == 0;
}

bool DeferredElementCustomWriter::IsNonDeferredProperty(
    _In_ const std::shared_ptr<XamlProperty>& spProperty)
{
    const XamlPropertyToken token = spProperty->get_PropertyToken();

    return (token.Equals(XamlPropertyToken::FromProperty(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::RelativePanel_LeftOf)))
        || token.Equals(XamlPropertyToken::FromProperty(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::RelativePanel_Above)))
        || token.Equals(XamlPropertyToken::FromProperty(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::RelativePanel_RightOf)))
        || token.Equals(XamlPropertyToken::FromProperty(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::RelativePanel_Below)))
        || token.Equals(XamlPropertyToken::FromProperty(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::RelativePanel_AlignHorizontalCenterWith)))
        || token.Equals(XamlPropertyToken::FromProperty(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::RelativePanel_AlignVerticalCenterWith)))
        || token.Equals(XamlPropertyToken::FromProperty(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::RelativePanel_AlignLeftWith)))
        || token.Equals(XamlPropertyToken::FromProperty(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::RelativePanel_AlignTopWith)))
        || token.Equals(XamlPropertyToken::FromProperty(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::RelativePanel_AlignRightWith)))
        || token.Equals(XamlPropertyToken::FromProperty(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::RelativePanel_AlignBottomWith)))
        || token.Equals(XamlPropertyToken::FromProperty(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::RelativePanel_AlignLeftWithPanel)))
        || token.Equals(XamlPropertyToken::FromProperty(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::RelativePanel_AlignTopWithPanel)))
        || token.Equals(XamlPropertyToken::FromProperty(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::RelativePanel_AlignRightWithPanel)))
        || token.Equals(XamlPropertyToken::FromProperty(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::RelativePanel_AlignBottomWithPanel)))
        || token.Equals(XamlPropertyToken::FromProperty(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::RelativePanel_AlignHorizontalCenterWithPanel)))
        || token.Equals(XamlPropertyToken::FromProperty(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::RelativePanel_AlignVerticalCenterWithPanel))));
}

// DeferredElementCustomWriterActivator

DeferredElementCustomWriterActivator::DeferredElementCustomWriterActivator(
    _In_ const std::shared_ptr<ObjectWriterContext>& spContext)
    : m_spContext(spContext)
{}

_Check_return_ HRESULT DeferredElementCustomWriterActivator::ValidateAndGetDeferLoadStrategyValue(
    _In_ const xstring_ptr& strValue,
    _Out_ bool* pResult)
{
    if (strValue.Compare(XSTRING_PTR_EPHEMERAL(L"Lazy")) == 0)
    {
        *pResult = true;
        return S_OK;
    }
    else
    {
        // error -- x:DeferLoadStrategy value is invalid

        std::unique_ptr<ObjectWriterErrorService> spErrorService(
            new ObjectWriterErrorService(m_spContext, true));

        IFC_RETURN(spErrorService->ReportInvalidDeferLoadStrategyValue(
            XamlLineInfo()));
    }

    *pResult = false;
    return S_OK;
}

_Check_return_ HRESULT DeferredElementCustomWriterActivator::ValidateAndGetRealizeValue(
    _In_ const xstring_ptr& strValue,
    _Out_ bool* pResult)
{
    if (strValue.Compare(XSTRING_PTR_EPHEMERAL(L"True"), xstrCompareBehavior::xstrCompareCaseInsensitive) == 0 ||
        strValue.Compare(XSTRING_PTR_EPHEMERAL(L"False"), xstrCompareBehavior::xstrCompareCaseInsensitive) == 0)
    {
        *pResult = true;
        return S_OK;
    }
    else
    {
        // error -- x:Load value is invalid

        std::unique_ptr<ObjectWriterErrorService> spErrorService(
            new ObjectWriterErrorService(m_spContext, true));

        IFC_RETURN(spErrorService->ReportInvalidRealizeValue(
            XamlLineInfo()));
    }

    *pResult = false;
    return S_OK;
}

_Check_return_ HRESULT DeferredElementCustomWriterActivator::ValidateNotRootElement()
{
    if (m_spContext->get_LiveDepth() <= 2)
    {
        // error -- deferred element cannot be root

        std::unique_ptr<ObjectWriterErrorService> spErrorService(
            new ObjectWriterErrorService(m_spContext, true));

        IFC_RETURN(spErrorService->ReportDeferredElementCannotBeRoot(
            XamlLineInfo()));
    }

    return S_OK;
}

_Check_return_ HRESULT DeferredElementCustomWriterActivator::ShouldActivateCustomWriter(
    _In_ const CustomWriterActivatorState& activatorState,
    _Out_ bool* pResult)
{
    if (activatorState.GetTrigger() == ActivatorTrigger::MemberEnd &&
        activatorState.GetProperty() &&
        activatorState.GetValue())
    {
        std::shared_ptr<XamlProperty> spProperty = activatorState.GetProperty();

        if (spProperty->IsDirective())
        {
            switch (std::static_pointer_cast<DirectiveProperty>(spProperty)->get_DirectiveKind())
            {
                case xdDeferLoadStrategy:
                    {
                        xstring_ptr ssValue;
                        IFC_RETURN(activatorState.GetValue()->GetCopyAsString(&ssValue));
                        IFC_RETURN(ValidateAndGetDeferLoadStrategyValue(ssValue, pResult));
                        return S_OK;
                    }

                case xdLoad:
                    {
                        xstring_ptr ssValue;
                        IFC_RETURN(activatorState.GetValue()->GetCopyAsString(&ssValue));
                        IFC_RETURN(ValidateAndGetRealizeValue(ssValue, pResult));
                        return S_OK;
                    }
            }
        }
    }

    *pResult = false;
    return S_OK;
}

_Check_return_ HRESULT DeferredElementCustomWriterActivator::CreateNodeStreamCollectingWriter(
    _In_ const CustomWriterActivatorState& activatorState,
    _Out_ std::shared_ptr<ObjectWriter>* pWriter)
{
    // Remove stuff that was added to stack before directive was encountered

    std::shared_ptr<XamlSavedContext> spSavedContext;
    std::shared_ptr<ObjectWriterStack> spSavedContextStack;

    IFC_RETURN(m_spContext->GetSavedContext(spSavedContext));
    IFC_RETURN(spSavedContext->get_Stack(spSavedContextStack));
    spSavedContextStack->PopScope();  // DeferLoadStrategy/Realize value
    spSavedContextStack->PopScope();  // Namespaces, object type

    IFC_RETURN(ValidateNotRootElement());

    // Create node-stream collecting writer

    ObjectWriterSettings settings;
    settings.set_EnableEncoding(true);
    settings.set_CheckDuplicateProperty(true);
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

    // Add type of object being deferred

    bool fromMember = false;
    std::shared_ptr<XamlType> spObjectType = activatorState.GetObject(&fromMember);

    IFC_RETURN(spNodeStreamCollectingWriter->WriteObject(
        spObjectType,
        fromMember));

    // replace the type of object with proxy on outer writer's context

    std::shared_ptr<XamlSchemaContext> spSchemaContext;
    std::shared_ptr<XamlType> spDeferredElementType;

    IFC_RETURN(m_spContext->get_SchemaContext(spSchemaContext));

    IFC_RETURN(spSchemaContext->GetXamlTypeFromStableXbfIndex(
        Parser::StableXbfTypeIndex::DeferredElement,
        spDeferredElementType));

    // Yes, this needs to overwrite the original type with proxy type.
    // The original is written to node stream collecting writer above.

    ASSERT(m_spContext->Parent().exists_Type());
    m_spContext->Parent().set_Type(spDeferredElementType);

    *pWriter = std::move(spNodeStreamCollectingWriter);
    return S_OK;
}

_Check_return_ HRESULT DeferredElementCustomWriterActivator::CreateCustomWriter(
    _In_ ICustomWriterCallbacks* pCallbacks,
    _In_ const CustomWriterActivatorState& activatorState,
    _Out_ std::unique_ptr<ICustomWriter>* pWriter)
{
    bool fromMember = false;
    std::shared_ptr<XamlType> spObjectType = activatorState.GetObject(&fromMember);

    std::unique_ptr<ICustomWriter> spCustomWriter(
        new DeferredElementCustomWriter(pCallbacks, m_spContext));

    for (const auto& namespacePair : activatorState.GetNamespaces())
    {
        bool handled = false;
        IFC_RETURN(spCustomWriter->WriteNamespace(
            namespacePair.first,
            namespacePair.second,
            &handled));
        ASSERT(handled);
    }

    bool handled = false;
    IFC_RETURN(spCustomWriter->WriteObject(spObjectType, fromMember, &handled));
    ASSERT(handled);

    // Pass directive which triggered activation, so in case of x:Load we can capture the value.

    handled = false;
    IFC_RETURN(spCustomWriter->WriteMember(activatorState.GetProperty(), &handled));
    ASSERT(handled);

    handled = false;
    IFC_RETURN(spCustomWriter->WriteValue(activatorState.GetValue(), &handled));
    ASSERT(handled);

    handled = false;
    IFC_RETURN(spCustomWriter->WriteEndMember(&handled));
    ASSERT(handled);

    *pWriter = std::move(spCustomWriter);
    return S_OK;
}
