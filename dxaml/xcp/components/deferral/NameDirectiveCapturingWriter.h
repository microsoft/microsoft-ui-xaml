// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class XamlProperty;
struct XamlQualifiedObject;
class XamlSchemaContext;

namespace Jupiter {
    namespace Deferral {
        class NameDirectiveCapturingWriter
        {
        public:
            NameDirectiveCapturingWriter(_In_ XamlSchemaContext* context)
                : m_context(context)
            {}

            _Check_return_ HRESULT WriteMember(_In_ const XamlProperty* xamlProperty);
            _Check_return_ HRESULT WriteEndMember();
            _Check_return_ HRESULT WriteValue(_In_ const XamlQualifiedObject* value);

            std::vector<xstring_ptr>& GetCapturedNameList();

        private:
            bool m_withinXNameMember;
            std::vector<xstring_ptr> m_nameList;
            XamlSchemaContext* m_context;
        };
    }
}
