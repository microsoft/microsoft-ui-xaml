// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CDependencyObject;
struct XamlQualifiedObject;

namespace Jupiter {
    namespace NameScoping {
        enum class NameScopeType;
    }
}

// Defines a XAML namescope used to managed the associations between names
// and the elements they represent.
class INameScope : public IObject
{
public:
    // Register an association between a name and an object in this namescope.
    virtual _Check_return_ HRESULT RegisterName(
        _In_ const xstring_ptr& strName,
        const std::shared_ptr<XamlQualifiedObject>& qoScopedObject) = 0;

    // Receive a namescope owner, if one doesn't already exist.
    virtual void EnsureNamescopeOwner(
        const std::shared_ptr<XamlQualifiedObject>& qoOwner) = 0;

    virtual CDependencyObject* GetNamescopeOwner() const = 0;
    virtual Jupiter::NameScoping::NameScopeType GetNameScopeType() const = 0;
};



