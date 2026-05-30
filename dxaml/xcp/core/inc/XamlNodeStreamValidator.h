// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class XamlParserContext;
class XamlNode;

class XamlNodeStreamValidator
{
public:
    explicit XamlNodeStreamValidator(const std::shared_ptr<XamlParserContext>& inContext);
    ~XamlNodeStreamValidator() = default;

    _Check_return_ HRESULT ShowNode(const XamlNode& inNode);

private:
    _Check_return_ HRESULT ValidateStartObject(const XamlNode& inNode);
    _Check_return_ HRESULT ValidateStartProperty(const XamlNode& inNode);
    _Check_return_ HRESULT ReportUnknownType(const XamlNode& inNode);
    _Check_return_ HRESULT ReportUnknownProperty(const XamlNode& inNode);

private:
    std::shared_ptr<XamlParserContext> m_ParserContext;
    std::size_t m_conditionalScopeDepth = 0;
};

