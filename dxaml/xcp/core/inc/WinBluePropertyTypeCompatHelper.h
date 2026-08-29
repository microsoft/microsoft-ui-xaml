// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class WinBluePropertyTypeCompatHelper final
{

public:
    void IncreaseScopeDepth(_In_ const std::shared_ptr<XamlType>& currentXamlPropertyType);
    void DecreaseScopeDepth();
    const std::shared_ptr<XamlType>& GetWinBlueDeclaringTypeForProperty(_In_ const std::shared_ptr<XamlProperty>& inXamlProperty, _In_ const std::shared_ptr<XamlType>& inXamlType) const;

private:
    bool ShouldUseBlueMappingForObjectType(_In_ const std::shared_ptr<XamlProperty>& inXamlProperty) const;
    int m_scopeDepth = 0;
    std::shared_ptr<XamlType> m_spCurrentXamlObjectType;
};
