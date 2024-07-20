// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

HRESULT XamlWriter::WriteNode(_In_ const XamlNode& xamlNode)
{
    HRESULT hr = S_OK;

    SetLineInfo(xamlNode.get_LineInfo());

    switch (xamlNode.get_NodeType())
    {
        case XamlNodeType::xntNamespace:
        {
            IFC(WriteNamespace(xamlNode.get_Prefix(), xamlNode.get_Namespace()));
        }
        break;

        case XamlNodeType::xntStartObject:
        {
            IFC(WriteObject(xamlNode.get_XamlType(), xamlNode.get_IsRetrievedObject()));
        }
        break;

        case XamlNodeType::xntEndObject:
        {
            IFC(WriteEndObject());
        }
        break;

        case XamlNodeType::xntStartProperty:
        {
            ASSERT(!!xamlNode.get_Property());
            IFC(WriteMember(xamlNode.get_Property()));
        }
        break;

        case XamlNodeType::xntEndProperty:
        {
            IFC_RETURN(WriteEndMember());
        }
        break;

        case XamlNodeType::xntStartConditionalScope:
        {
            IFC(WriteConditionalScope(xamlNode.get_XamlPredicateAndArgs()));
        }
        break;

        case XamlNodeType::xntEndConditionalScope:
        {
            IFC(WriteEndConditionalScope());
        }
        break;

        case XamlNodeType::xntText:
        {
            // TODO: This mean effectively that XamlWriter will always have
            // a deep dependency on CCoreSerivces. Why is that okay?
            CValue vTemp;
            xstring_ptr spTextString;
            std::shared_ptr<XamlType> spXamlStringType;
            std::shared_ptr<XamlSchemaContext> spSchemaContext;
            std::shared_ptr<XamlQualifiedObject> qo;

            IFCEXPECT(xamlNode.get_Text());
            IFC(xamlNode.get_Text()->get_Text(&spTextString));

            vTemp.SetString(spTextString);

            IFC(GetSchemaContext(spSchemaContext));
            IFC(spSchemaContext->get_StringXamlType(spXamlStringType));
            qo = std::make_shared<XamlQualifiedObject>();
            IFC(qo->SetValue(vTemp));
            qo->SetTypeToken(spXamlStringType->get_TypeToken());
            IFC(WriteValue(qo));
        }
        break;

        case XamlNodeType::xntValue:
        {
            IFC(WriteValue(xamlNode.get_Value()));
        }
        break;

        case XamlNodeType::xntEndOfStream:
        case XamlNodeType::xntEndOfAttributes:
            break;
        case XamlNodeType::xntNone:
            ASSERT(FALSE);
            break;

        default:
            IFC(E_FAIL);
    }

Cleanup:
    if (FAILED(hr))
    {
        HRESULT xr = ProcessError();
        if (FAILED(xr))
        {
            hr = xr;
        }
    }

    return hr;
}