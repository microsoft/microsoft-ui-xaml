// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlBinaryFormatValidator.h"

XamlBinaryFormatValidator::~XamlBinaryFormatValidator()
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a new instance of a XamlBinaryFormatValidator, and returns
//      an std::shared_ptr to that new instance.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
XamlBinaryFormatValidator::Create(
        _In_ const std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext,
        _In_ const std::shared_ptr<XamlReader>& spXamlReader,
        _Out_ std::shared_ptr<XamlBinaryFormatValidator>& spXamlBinaryFormatValidator
    )
{
    spXamlBinaryFormatValidator = std::make_shared<XamlBinaryFormatValidator>(spXamlSchemaContext, spXamlReader);
    
    // initialize the parser stack
    IFC_RETURN(spXamlBinaryFormatValidator->m_stack.push(T_EOS));
    IFC_RETURN(spXamlBinaryFormatValidator->m_stack.push(NT_DOCUMENT));

    return S_OK;
}

_Check_return_ HRESULT XamlBinaryFormatValidator::Read()
{
    HRESULT hr = S_OK;
    HRESULT xr = S_OK;
    XamlNodeStreamSymbol symbol;
    XamlNodeStreamSymbol top;
    bool fRepeat = true;

    if (S_OK != (xr = m_spXamlReader->Read()))
    {
        // XamlReader::Read() can return something other than S_OK for a number of reasons, 
        // including running out of nodes or encountering an error. Regardless, stack should be empty.
        if (!m_stack.empty())
        {
            // XBFReader::Read() returns S_FALSE when the end of the stream is reached, so
            // we want to convert it back to E_FAIL if the stack isn't empty. However, a quirk of XBFReader
            // is that it doesn't return an EOS node, so if T_EOS is the last symbol on the stack, we keep S_FALSE
            IFC(m_stack.top(top));
            hr = (xr == S_FALSE && top != T_EOS) ? E_FAIL : xr;
        }
        goto Cleanup;
    }
    else if (m_stack.empty())
    {
        // Stack shouldn't be empty if we were able to successfully read, since T_EOS is always last thing on stack
        IFC(E_FAIL);
    }

    IFC(ConvertNodeToSymbol(m_spXamlReader->CurrentNode(), &symbol));
   
    // Validation is done using a LL(1) parser to build the input node stream using the rules specified in the XAML grammar
    // The parser consists of the input node stream, an internal stack to store unprocessed terminal and non-terminal symbols,
    // and a parsing table (implemented as nested switch statements) that tell it which grammar rule to apply given the current
    // symbol in the node stream and the topmost symbol on the stack.
    // The general algorithm is as follows:
    // 1. Look at the current symbol in the nodestream and the topmost symbol on the stack
    // 2. If the two symbols match, pop the topmost symbol off the stack and move to the next symbol in the stream.
    //    Else, determine which rule to apply in order to rewrite the topmost symbol of the stack to try to match what is seen in the
    //    input node stream. If no matching rule can be found, then the input node stream violates the grammar, resulting in an error.
    // 3. Repeat until the end of the node stream is reached
    //
    // The algorithm has been modified to allow it to process one node from the stream at a time. In addition, the implementation of the
    // rules includes some refactoring to avoid excessive rewriting of the stack.

    // XAML grammar using BNF notation
    // NT_DOCUMENT ::= NT_ELEMENT
    // NT_ELEMENT ::= T_PREFIX_DEFINITION_NODE* T_START_ELEMENT_NODE NT_DIRECTIVE_PROPERTIES NT_PROPERTY* T_END_ELEMENT_NODE
    // NT_DIRECTIVE_PROPERTIES ::= (NT_UID_DIRECTIVE_PROPERTY | NT_NAME_DIRECTIVE_PROPERTY | NT_DIRECTIVE_PROPERTY)[0, 1] NT_DIRECTIVE_PROPERTY*
    // NT_UID_DIRECTIVE_PROPERTY ::= T_START_XUID_DIRECTIVE_PROPERTY_NODE NT_DIRECTIVE_PROPERTY_CONTENT T_END_PROPERTY_NODE
    // NT_NAME_DIRECTIVE_PROPERTY ::= T_START_XNAME_DIRECTIVE_PROPERTY_NODE NT_DIRECTIVE_PROPERTY_CONTENT T_END_PROPERTY_NODE
    // NT_DIRECTIVE_PROPERTY ::= T_START_X_DIRECTIVE_PROPERTY_NODE NT_DIRECTIVE_PROPERTY_CONTENT T_END_PROPERTY_NODE
    // NT_DIRECTIVE_PROPERTY_CONTENT ::= T_TEXTVALUE_NODE | T_VALUE_NODE 
    // NT_PROPERTY ::= (T_START_MULTI_ITEM_PROPERTY_NODE NT_PROPERTY_CONTENT* T_END_PROPERTY_NODE) | (T_START_SINGLE_ITEM_PROPERTY_NODE NT_PROPERTY_CONTENT T_END_PROPERTY_NODE)
    // NT_PROPERTY_CONTENT ::= NT_ELEMENT | T_TEXTVALUE_NODE | T_VALUE_NODE | <empty>

    while (fRepeat)
    {
        IFC(m_stack.top(top));
        if (symbol == top)
        {
            // Current symbol in nodestream matches top of stack
            IFC(m_stack.pop());
            fRepeat = FALSE;
        }
        else if (symbol == T_END_OF_ATTRIBUTES_NODE)
        {
            // The ObjectWriter doesn't care about EOA nodes, and the XamlPullParser spits them out in the wrong places anyway, so just ignore them
            // if they show up in the node stream
            fRepeat = FALSE;
        }
        else
        {
            // Use top of stack and current symbol to determine which rule to apply
            switch (top)
            {
                case NT_DOCUMENT:
                    // NT_DOCUMENT ::= NT_ELEMENT
                    IFC(m_stack.pop());
                    IFC(m_stack.push(NT_ELEMENT));
                    break;
                case NT_ELEMENT:
                    // NT_ELEMENT ::= T_PREFIX_DEFINITION_NODE* T_START_ELEMENT_NODE NT_DIRECTIVE_PROPERTIES NT_PROPERTY* T_END_OF_ATTRIBUTES_NODE NT_PROPERTY* T_END_ELEMENT_NODE
                    switch (symbol)
                    {
                        case T_PREFIX_DEFINITION_NODE:
                        case T_START_ELEMENT_NODE:
                            IFC(m_stack.pop());
                            IFC(m_stack.push(T_END_ELEMENT_NODE));
                            IFC(m_stack.push(S_PROPERTY_ZERO_OR_MORE));
                            IFC(m_stack.push(NT_DIRECTIVE_PROPERTIES));
                            IFC(m_stack.push(T_START_ELEMENT_NODE));
                            IFC(m_stack.push(S_PREFIX_DEFINITION_NODE_ZERO_OR_MORE));
                            break;
                        default:
                            DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strErrorMessage, L"NT_ELEMENT ::= T_PREFIX_DEFINITION_NODE* T_START_ELEMENT_NODE NT_DIRECTIVE_PROPERTIES NT_PROPERTY* T_END_OF_ATTRIBUTES_NODE NT_PROPERTY* T_END_ELEMENT_NODE");
                            IFC(SetErrorInfo(m_spXamlReader->CurrentNode(), c_strErrorMessage));
                            IFC(E_FAIL);
                    }
                    break;
                case NT_DIRECTIVE_PROPERTIES:
                    // NT_DIRECTIVE_PROPERTIES ::= (NT_UID_DIRECTIVE_PROPERTY | NT_NAME_DIRECTIVE_PROPERTY | NT_DIRECTIVE_PROPERTY)[0, 1] NT_DIRECTIVE_PROPERTY*
                    switch (symbol)
                    {
                        case T_START_XUID_DIRECTIVE_PROPERTY_NODE:
                            IFC(m_stack.pop());
                            IFC(m_stack.push(S_DIRECTIVE_PROPERTY_ZERO_OR_MORE));
                            IFC(m_stack.push(NT_UID_DIRECTIVE_PROPERTY));
                            break;
                        case T_START_XNAME_DIRECTIVE_PROPERTY_NODE:
                            IFC(m_stack.pop());
                            IFC(m_stack.push(S_DIRECTIVE_PROPERTY_ZERO_OR_MORE));
                            IFC(m_stack.push(NT_NAME_DIRECTIVE_PROPERTY));
                            break;
                        case T_START_X_DIRECTIVE_PROPERTY_NODE:
                            IFC(m_stack.pop());
                            IFC(m_stack.push(S_DIRECTIVE_PROPERTY_ZERO_OR_MORE));
                            IFC(m_stack.push(NT_DIRECTIVE_PROPERTY));
                            break;
                        default:
                            IFC(m_stack.pop());
                    }
                    break;
                case NT_UID_DIRECTIVE_PROPERTY:
                    // NT_UID_DIRECTIVE_PROPERTY ::= T_START_XUID_DIRECTIVE_PROPERTY_NODE NT_DIRECTIVE_PROPERTY_CONTENT T_END_PROPERTY_NODE
                    switch (symbol)
                    {
                        case T_START_XUID_DIRECTIVE_PROPERTY_NODE:
                            IFC(m_stack.pop());
                            IFC(m_stack.push(T_END_PROPERTY_NODE));
                            IFC(m_stack.push(NT_DIRECTIVE_PROPERTY_CONTENT));
                            IFC(m_stack.push(T_START_XUID_DIRECTIVE_PROPERTY_NODE));                           
                            break;
                        default:
                            DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strErrorMessage, L"NT_UID_DIRECTIVE_PROPERTY ::= T_START_XUID_DIRECTIVE_PROPERTY_NODE NT_DIRECTIVE_PROPERTY_CONTENT T_END_PROPERTY_NODE");
                            IFC(SetErrorInfo(m_spXamlReader->CurrentNode(), c_strErrorMessage));
                            IFC(E_FAIL);
                    }
                    break;
                case NT_NAME_DIRECTIVE_PROPERTY:
                    // NT_NAME_DIRECTIVE_PROPERTY ::= T_START_XNAME_DIRECTIVE_PROPERTY_NODE NT_DIRECTIVE_PROPERTY_CONTENT T_END_PROPERTY_NODE
                    switch (symbol)
                    {
                        case T_START_XNAME_DIRECTIVE_PROPERTY_NODE:
                            IFC(m_stack.pop());
                            IFC(m_stack.push(T_END_PROPERTY_NODE));
                            IFC(m_stack.push(NT_DIRECTIVE_PROPERTY_CONTENT));
                            IFC(m_stack.push(T_START_XNAME_DIRECTIVE_PROPERTY_NODE));
                            break;
                        default:
                            DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strErrorMessage, L"NT_NAME_DIRECTIVE_PROPERTY ::= T_START_XNAME_DIRECTIVE_PROPERTY_NODE NT_DIRECTIVE_PROPERTY_CONTENT T_END_PROPERTY_NODE");
                            IFC(SetErrorInfo(m_spXamlReader->CurrentNode(), c_strErrorMessage));
                            IFC(E_FAIL);
                    }
                    break;
                case NT_DIRECTIVE_PROPERTY:
                    // NT_DIRECTIVE_PROPERTY ::= T_START_X_DIRECTIVE_PROPERTY_NODE NT_DIRECTIVE_PROPERTY_CONTENT T_END_PROPERTY_NODE
                    switch (symbol)
                    {
                        case T_START_X_DIRECTIVE_PROPERTY_NODE:
                            IFC(m_stack.pop());
                            IFC(m_stack.push(T_END_PROPERTY_NODE));
                            IFC(m_stack.push(NT_DIRECTIVE_PROPERTY_CONTENT));
                            IFC(m_stack.push(T_START_X_DIRECTIVE_PROPERTY_NODE));                           
                            break;
                        default:
                            DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strErrorMessage, L"NT_DIRECTIVE_PROPERTY ::= T_START_X_DIRECTIVE_PROPERTY_NODE NT_DIRECTIVE_PROPERTY_CONTENT T_END_PROPERTY_NODE");
                            IFC(SetErrorInfo(m_spXamlReader->CurrentNode(), c_strErrorMessage));
                            IFC(E_FAIL);
                    }
                    break;
                case NT_DIRECTIVE_PROPERTY_CONTENT:
                    // NT_DIRECTIVE_PROPERTY_CONTENT ::= T_TEXTVALUE_NODE | T_VALUE_NODE
                    switch (symbol)
                    {
                        case T_TEXTVALUE_NODE:
                            IFC(m_stack.pop());
                            IFC(m_stack.push(T_TEXTVALUE_NODE));
                            break;
                        case T_VALUE_NODE:
                            IFC(m_stack.pop());
                            IFC(m_stack.push(T_VALUE_NODE));
                            break;
                        default:
                            DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strErrorMessage, L"NT_DIRECTIVE_PROPERTY_CONTENT ::= T_TEXTVALUE_NODE | T_VALUE_NODE");
                            IFC(SetErrorInfo(m_spXamlReader->CurrentNode(), c_strErrorMessage));
                            IFC(E_FAIL);
                    }
                    break;       
                case NT_PROPERTY:
                    // NT_PROPERTY ::= (T_START_MULTI_ITEM_PROPERTY_NODE NT_PROPERTY_CONTENT* T_END_PROPERTY_NODE) | (T_START_SINGLE_ITEM_PROPERTY_NODE NT_PROPERTY_CONTENT[0,1] T_END_PROPERTY_NODE)
                    switch (symbol)
                    {
                        case T_START_MULTI_ITEM_PROPERTY_NODE:
                            IFC(m_stack.pop());
                            IFC(m_stack.push(T_END_PROPERTY_NODE));
                            IFC(m_stack.push(S_PROPERTY_CONTENT_ZERO_OR_MORE));
                            IFC(m_stack.push(T_START_MULTI_ITEM_PROPERTY_NODE));                           
                            break;
                        case T_START_SINGLE_ITEM_PROPERTY_NODE:
                            IFC(m_stack.pop());
                            IFC(m_stack.push(T_END_PROPERTY_NODE));
                            IFC(m_stack.push(NT_PROPERTY_CONTENT));
                            IFC(m_stack.push(T_START_SINGLE_ITEM_PROPERTY_NODE));
                            break;
                        default:
                            DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strErrorMessage, L"NT_PROPERTY ::= (T_START_MULTI_ITEM_PROPERTY_NODE NT_PROPERTY_CONTENT* T_END_PROPERTY_NODE) | (T_START_SINGLE_ITEM_PROPERTY_NODE NT_PROPERTY_CONTENT[0,1] T_END_PROPERTY_NODE)");
                            IFC(SetErrorInfo(m_spXamlReader->CurrentNode(), c_strErrorMessage));
                            IFC(E_FAIL);
                    }
                    break;
                case NT_PROPERTY_CONTENT:
                    // NT_PROPERTY_CONTENT ::= NT_ELEMENT | T_TEXTVALUE_NODE | T_VALUE_NODE | <empty>
                    switch (symbol)
                    {
                        case T_PREFIX_DEFINITION_NODE:
                        case T_START_ELEMENT_NODE:
                            IFC(m_stack.pop());
                            IFC(m_stack.push(NT_ELEMENT));
                            break;
                        case T_TEXTVALUE_NODE:
                            IFC(m_stack.pop());
                            IFC(m_stack.push(T_TEXTVALUE_NODE));
                            break;
                        case T_VALUE_NODE:
                            IFC(m_stack.pop());
                            IFC(m_stack.push(T_VALUE_NODE));
                            break;
                        case T_END_PROPERTY_NODE:
                            IFC(m_stack.pop());
                            break;
                        default:
                            DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strErrorMessage, L"NT_PROPERTY_CONTENT ::= NT_ELEMENT | T_TEXTVALUE_NODE | T_VALUE_NODE | <empty>");
                            IFC(SetErrorInfo(m_spXamlReader->CurrentNode(), c_strErrorMessage));
                            IFC(E_FAIL);
                    }
                    break;
                case S_PREFIX_DEFINITION_NODE_ZERO_OR_MORE:
                    // T_PREFIX_DEFINITION_NODE* ::= T_PREFIX_DEFINITION_NODE T_PREFIX_DEFINITION_NODE* | <empty>
                    switch (symbol)
                    {
                        case T_PREFIX_DEFINITION_NODE:
                            IFC(m_stack.push(T_PREFIX_DEFINITION_NODE));
                            break;
                        default:
                            IFC(m_stack.pop());
                    }
                    break;
                case S_PROPERTY_CONTENT_ZERO_OR_MORE:
                    // NT_PROPERTY_CONTENT* ::= NT_PROPERTY_CONTENT NT_PROPERTY_CONTENT* | <empty>
                    switch (symbol)
                    {
                        case T_PREFIX_DEFINITION_NODE:
                        case T_START_ELEMENT_NODE:
                        case T_TEXTVALUE_NODE:
                        case T_VALUE_NODE:
                            IFC(m_stack.push(NT_PROPERTY_CONTENT));
                            break;
                        default:
                            IFC(m_stack.pop());
                    }
                    break;
                case S_PROPERTY_ZERO_OR_MORE:
                    // NT_PROPERTY* ::= NT_PROPERTY NT_PROPERTY* | <empty>
                    switch (symbol)
                    {
                        case T_START_MULTI_ITEM_PROPERTY_NODE:
                        case T_START_SINGLE_ITEM_PROPERTY_NODE:
                            IFC(m_stack.push(NT_PROPERTY));
                            break;
                        default:
                            IFC(m_stack.pop());
                    }
                    break;
                case S_DIRECTIVE_PROPERTY_ZERO_OR_MORE:
                    // NT_DIRECTIVE_PROPERTY* ::= NT_DIRECTIVE_PROPERTY NT_DIRECTIVE_PROPERTY* | <empty>
                    switch (symbol)
                    {
                        case T_START_X_DIRECTIVE_PROPERTY_NODE:
                            IFC(m_stack.push(NT_DIRECTIVE_PROPERTY));
                            break;
                        default:
                            IFC(m_stack.pop());
                    }
                    break;
                default:
                    // Unexpected symbol on top of stack. This usually means that
                    // a terminal symbol that was pushed onto the stack doesn't have
                    // a matching terminal symbol in the nodestream (i.e. error in the nodestream)
                    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strErrorMessage, L"Unexpected symbol encountered in node stream");
                    IFC(SetErrorInfo(m_spXamlReader->CurrentNode(), c_strErrorMessage));
                    IFC(E_FAIL);
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   ConvertNodeToSymbol
//
//  Synopsis:
//         Converts a XamlNode to the corresponding XamlNodeStreamSymbol.
//         Returns T_UNKNOWN if unsuccessful.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT XamlBinaryFormatValidator::ConvertNodeToSymbol(
    _In_ const XamlNode& node, 
    _Out_ XamlNodeStreamSymbol* symbol
    )
{
    XamlNodeType nodeType;
    std::shared_ptr<XamlQualifiedObject> value;
    std::shared_ptr<XamlProperty> prop;

    *symbol = T_UNKNOWN;
    nodeType = node.get_NodeType();

    // Process the node based on type
    switch(nodeType)
    {
        case XamlNodeType::xntStartObject:
            *symbol = T_START_ELEMENT_NODE;
            break;
        case XamlNodeType::xntEndObject:
            *symbol = T_END_ELEMENT_NODE;
            break;
        case XamlNodeType::xntStartProperty:
            prop = node.get_Property();

            if (prop->IsDirective())
            {
                *symbol = T_START_X_DIRECTIVE_PROPERTY_NODE;
            }
            else
            {
                std::shared_ptr<XamlType> propType;
                bool bIsCollection = false;

                if (prop->IsImplicit())
                {
                    if (std::static_pointer_cast<ImplicitProperty>(prop)->get_ImplicitPropertyType() == iptItems)
                    {
                        bIsCollection = TRUE;
                    }
                }
                else
                {
                    IFC_RETURN(prop->get_Type(propType));
                    IFC_RETURN(propType->IsCollection(bIsCollection));
                }

                if (bIsCollection)
                {
                    *symbol = T_START_MULTI_ITEM_PROPERTY_NODE;
                }
                else
                {
                    *symbol = T_START_SINGLE_ITEM_PROPERTY_NODE;
                }
            }
            break;
        case XamlNodeType::xntEndProperty:
            *symbol = T_END_PROPERTY_NODE;
            break;
        case XamlNodeType::xntText:
            *symbol = T_TEXTVALUE_NODE;
            break;
        case XamlNodeType::xntValue:
            value = node.get_Value();

            if (value->GetValue().GetType() == valueString)
            {
                // Value nodes with string types are equivalent to textvalue nodes
                *symbol = T_TEXTVALUE_NODE;
            }
            else
            {
                *symbol = T_VALUE_NODE;
            }
            break;
        case XamlNodeType::xntNamespace:
            *symbol = T_PREFIX_DEFINITION_NODE;
            break;
        case XamlNodeType::xntEndOfStream:
            *symbol = T_EOS;
            break;
        case XamlNodeType::xntEndOfAttributes:
            *symbol = T_END_OF_ATTRIBUTES_NODE;
            break;
        case XamlNodeType::xntLineInfo:
        default:
            IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

const XamlNode& XamlBinaryFormatValidator::CurrentNode()
{
    return m_spXamlReader->CurrentNode();
}

