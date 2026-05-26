// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "IObjectWriterCallbacks.h"
#include "CDependencyObject.h"

namespace Jupiter {
    namespace NameScoping {
        enum class NameScopeType;
    }
}

struct XamlQualifiedObject;

// IObjectWriterCallbacks is a terrible interface that only ever sets the 
// templated parent of an object in OnObjectCreated and you shouldn't use it
// for anything ever.
class ObjectWriterCallbacksTemplateParentSetter
    : public ObjectWriterCallbacksDelegate
{
public:
    ObjectWriterCallbacksTemplateParentSetter(xref_ptr<CDependencyObject> templatedParent, 
        Jupiter::NameScoping::NameScopeType nameScopeType)
        : m_templatedParent(xref::get_weakref(std::move(templatedParent)))
        , m_nameScopeType(nameScopeType)
    {}

    HRESULT OnObjectCreated(_In_ const std::shared_ptr<XamlQualifiedObject>& qoRoot, _In_ const std::shared_ptr<XamlQualifiedObject>& qoNewInstance) override;

private:
    xref::weakref_ptr<CDependencyObject> m_templatedParent;
    Jupiter::NameScoping::NameScopeType m_nameScopeType;
};

