// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "NameDirectiveCapturingWriter.h"

#include <XamlProperty.h>
#include <XamlQualifiedObject.h>
#include <XamlSchemaContext.h>
#include <CString.h>
#include <MetadataAPI.h>
#include <TypeTableStructs.h>

namespace Jupiter {
    namespace Deferral {
        _Check_return_ HRESULT NameDirectiveCapturingWriter::WriteMember(_In_ const XamlProperty* xamlProperty)
        {
            std::shared_ptr<DirectiveProperty> nameDirective;
            IFC_RETURN(m_context->get_X_NameProperty(nameDirective));

            if (XamlProperty::AreEqual(xamlProperty, nameDirective.get()) ||
                xamlProperty->get_PropertyToken().Equals(
                    XamlPropertyToken::FromProperty(
                        DirectUI::MetadataAPI::GetPropertyByIndex(KnownPropertyIndex::DependencyObject_Name))))
            {
                m_withinXNameMember = true;
            }

            return S_OK;
        }

        _Check_return_ HRESULT NameDirectiveCapturingWriter::WriteEndMember()
        {
            m_withinXNameMember = false;
            return S_OK;
        }

        _Check_return_ HRESULT NameDirectiveCapturingWriter::WriteValue(_In_ const XamlQualifiedObject* value)
        {
            if (m_withinXNameMember)
            {
                xstring_ptr name;
                if (value->GetValue().GetType() == ValueType::valueString)
                {
                    name = value->GetValue().AsString();

                }
                else if (value->GetTypeToken().Equals(XamlTypeToken::FromType(DirectUI::MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::String))) &&
                    value->GetDependencyObject() != nullptr)
                {
                    name = static_cast<CString*>(value->GetDependencyObject())->m_strString;
                }
                m_nameList.push_back(std::move(name));
            }

            return S_OK;
        }

        std::vector<xstring_ptr>& NameDirectiveCapturingWriter::GetCapturedNameList()
        {
            return m_nameList;
        }
    }
}
