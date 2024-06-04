// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

XamlNodeStreamValidator::XamlNodeStreamValidator(const std::shared_ptr<XamlParserContext>& inContext)
    : m_ParserContext(inContext)
{
}

_Check_return_ HRESULT XamlNodeStreamValidator::ShowNode(const XamlNode& inNode)
{
    // Does not explicitly try to verify the order of the nodes.
    // Assumes at least that the parser has done this
    // and will not try to duplicate that.

    switch (inNode.get_NodeType())
    {
        case XamlNodeType::xntStartConditionalScope:
        {
            ++m_conditionalScopeDepth;
        }
        break;
        case XamlNodeType::xntEndConditionalScope:
        {
            ASSERT(m_conditionalScopeDepth > 0);
            --m_conditionalScopeDepth;
        }
        break;
        case XamlNodeType::xntStartObject:
        {
            IFC_RETURN(ValidateStartObject(inNode));
        }
        break;
        case XamlNodeType::xntStartProperty:
        {
            IFC_RETURN(ValidateStartProperty(inNode));
        }
        break;
        default:
            break;
    }

    return S_OK;
}

_Check_return_ HRESULT XamlNodeStreamValidator::ValidateStartObject(const XamlNode& inNode)
{
    // If the type was declared within a conditional XAML section, we may not have been
    // able to immediately resolve it because it doesn't exist yet (it does, however, 
    // have to exist later when the conditional predicate is evaluated)
    if (inNode.IsUnknown() && m_conditionalScopeDepth == 0)
    {
        IFC_RETURN(ReportUnknownType(inNode));
        IFC_RETURN(E_FAIL);
    }
    
    return S_OK;
}

HRESULT XamlNodeStreamValidator::ValidateStartProperty(const XamlNode& inNode)
{
    // If the property was declared within a conditional XAML section, we may not have been
    // able to immediately resolve it because it doesn't exist yet (it does, however, 
    // have to exist later when the conditional predicate is evaluated)
    if (inNode.IsUnknown() && m_conditionalScopeDepth == 0)
    {
        // If this node represents an _UnknownContent property and has a
        // declaring type, provide a better error message like "Cannot add
        // content to an object of type '%0'."
        if (inNode.get_Property())
        {
            xstring_ptr spPropertyName;
            IFC_RETURN(inNode.get_Property()->get_Name(&spPropertyName));
            if (spPropertyName.Equals(c_strUnknownContent, xstrCompareCaseSensitive))
            {
                std::shared_ptr<XamlType> spDeclaringType;
                IFC_RETURN(inNode.get_Property()->get_DeclaringType(spDeclaringType));
                if (spDeclaringType)
                {
                    xstring_ptr spDeclaringTypeName;
                    IFC_RETURN(spDeclaringType->get_FullName(&spDeclaringTypeName));
                    if (!spDeclaringTypeName.IsNullOrEmpty())
                    {
                        // Report the error
                        std::shared_ptr<ParserErrorReporter> spErrorReporter;
                        IFC_RETURN(m_ParserContext->GetErrorService(spErrorReporter));
                        IFC_RETURN(spErrorReporter->SetError(
                            AG_E_PARSER2_CANNOT_ADD_ANY_CHILDREN,
                            inNode.get_LineInfo().LineNumber(),
                            inNode.get_LineInfo().LinePosition(),
                            spDeclaringTypeName));
                        IFC_RETURN(E_FAIL);
                    }
                }
            }
        }


        // Otherwise report a generic unknown property
        IFC_RETURN(ReportUnknownProperty(inNode));
        IFC_RETURN(E_FAIL);
    }
    
    return S_OK;
}

_Check_return_ HRESULT XamlNodeStreamValidator::ReportUnknownType(const XamlNode& inNode)
{
    std::shared_ptr<XamlNamespace> xamlNamespace;
    std::shared_ptr<ParserErrorReporter> errorReporter;
    xstring_ptr ssNamespaceUri;
    xstring_ptr ssTypeName;
  
    ASSERT(!!inNode.get_XamlType());
    ASSERT(inNode.get_XamlType()->IsUnknown());

    IFC_RETURN(inNode.get_XamlType()->GetXamlNamespace(xamlNamespace));
    IFC_RETURN(inNode.get_XamlType()->get_Name(&ssTypeName));
    bool bIsValidNamespace = xamlNamespace->get_IsResolved();
    ssNamespaceUri = xamlNamespace->get_TargetNamespace();
    IFC_RETURN(m_ParserContext->GetErrorService(errorReporter));
    
    if (bIsValidNamespace)
    {
        IFC_RETURN(errorReporter->SetError(AG_E_PARSER2_UNKNOWN_TYPE, inNode.get_LineInfo().LineNumber(), inNode.get_LineInfo().LinePosition(), ssTypeName));
    }
    else
    {
        IFC_RETURN(errorReporter->SetError(AG_E_PARSER2_UNKNOWN_NAMESPACE, inNode.get_LineInfo().LineNumber(), inNode.get_LineInfo().LinePosition(), ssTypeName, ssNamespaceUri));
    }

    return S_OK;
}

_Check_return_ HRESULT XamlNodeStreamValidator::ReportUnknownProperty(const XamlNode& inNode)
{
    // TODO: Should check namespace.
    // TODO: Can also distinguish whether type is known.
    
    std::shared_ptr<ParserErrorReporter> spErrorReporter;
    XUINT32 uiErrorCode;
    std::shared_ptr<XamlProperty> spProperty;
    xstring_ptr spPropertyName;
    std::shared_ptr<XamlType> spDeclaringType;
    xstring_ptr spDeclaringTypeName;
    XUINT32 uiLine;
    XUINT32 uiPosition;
    
    // Get the error service
    IFC_RETURN(m_ParserContext->GetErrorService(spErrorReporter));
    
    // Get the property name
    spProperty = inNode.get_Property();
    ASSERT(!!spProperty);
    ASSERT(spProperty->IsUnknown());
    IFC_RETURN(spProperty->get_Name(&spPropertyName));
    
    // Get the property's declaring type's name
    IFC_RETURN(spProperty->get_DeclaringType(spDeclaringType));
    if (spDeclaringType)
    {
        IFC_RETURN(spDeclaringType->get_Name(&spDeclaringTypeName));
    }
    else
    {
        // Default unknown declaring types to the empty string, which will be
        // turned into "unknown" later by the error reporter
        spDeclaringTypeName = xstring_ptr::EmptyString();
    }
    
    // Get the error code
    uiErrorCode = spProperty->IsAttachable() ?
        AG_E_PARSER2_UNKNOWN_ATTACHABLE_PROP_ON_TYPE :
        AG_E_PARSER2_UNKNOWN_PROP_ON_TYPE;
    
    // Get the line and column numbers
    uiLine = inNode.get_LineInfo().LineNumber();
    uiPosition = inNode.get_LineInfo().LinePosition();
    
    // Report the error
    IFC_RETURN(spErrorReporter->SetError(uiErrorCode, uiLine, uiPosition, spPropertyName, spDeclaringTypeName));
    
    return S_OK;
}