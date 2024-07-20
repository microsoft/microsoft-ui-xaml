// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ObjectWriterFrame.h>

ObjectWriterFrame::ObjectWriterFrame()
{}

void ObjectWriterFrame::EnsureDirectives()
{
    EnsureFrameLegacyStorage();
    if (!m_spFrameLegacyStorage->m_spDirectiveValuesMap)
    {
        m_spFrameLegacyStorage->m_spDirectiveValuesMap = std::make_shared<MapDirectiveToQO>();
    }
}

_Check_return_ HRESULT ObjectWriterFrame::AddNamespacePrefix(
    _In_ const xstring_ptr& inPrefix, 
    _In_ const std::shared_ptr<XamlNamespace>& spXamlNamespace)
{
    EnsureOverflowStorage();
    if (!m_spFrameOverflowStorage->m_Namespaces)
    {
        m_spFrameOverflowStorage->m_Namespaces = std::make_shared<tNamespaceMap::element_type>();
    }

    (*m_spFrameOverflowStorage->m_Namespaces)[inPrefix] = spXamlNamespace;

    return S_OK;
}

std::shared_ptr<XamlNamespace> ObjectWriterFrame::FindNamespaceByPrefix(
    _In_ const xstring_ptr& inPrefix) const
{
    if (!m_spFrameOverflowStorage || !m_spFrameOverflowStorage->m_Namespaces)
    {
        return std::shared_ptr<XamlNamespace>();
    }

    auto itXamlNamespace = m_spFrameOverflowStorage->m_Namespaces->find(inPrefix);
    if (itXamlNamespace == m_spFrameOverflowStorage->m_Namespaces->end())
    {
        return std::shared_ptr<XamlNamespace>();
    }
    else
    {
        return itXamlNamespace->second;
    }
}


std::shared_ptr<ObjectWriterFrame::MapDirectiveToQO> ObjectWriterFrame::get_Directives()
{
    EnsureDirectives();
    return m_spFrameLegacyStorage->m_spDirectiveValuesMap;
}

std::shared_ptr<XamlQualifiedObject> ObjectWriterFrame::get_Collection() const
{
    if (m_spFrameLegacyStorage)
    {
        return m_spFrameLegacyStorage->m_qoCollection;
    }

    return nullptr;
}

std::shared_ptr<XamlQualifiedObject> ObjectWriterFrame::get_Value() const
{
    if (m_spFrameLegacyStorage)
    {
        return m_spFrameLegacyStorage->m_qoValue;
    }

    return nullptr;
}

bool ObjectWriterFrame::get_IsPropertyAssigned(
    _In_ const std::shared_ptr<XamlProperty>& inProperty) const
{
    if (m_spFrameLegacyStorage && m_spFrameLegacyStorage->m_spAssignedProperties) {
        auto it = std::find_if(m_spFrameLegacyStorage->m_spAssignedProperties->begin(), m_spFrameLegacyStorage->m_spAssignedProperties->end(),
            [&inProperty](const std::shared_ptr<XamlProperty>& value) -> bool
        {
            return !!XamlProperty::AreEqual(inProperty, value);
        });
        return it != m_spFrameLegacyStorage->m_spAssignedProperties->end();
    }

    return false;
}

void ObjectWriterFrame::NotifyPropertyAssigned(
    _In_ const std::shared_ptr<XamlProperty>& inProperty)
{
    EnsureFrameLegacyStorage();
    // This will have already been done by the caller, and the
    // result of not doing it is harmless but inefficient.
    ASSERT(!get_IsPropertyAssigned(inProperty));

    if (!m_spFrameLegacyStorage->m_spAssignedProperties)
    {
        m_spFrameLegacyStorage->m_spAssignedProperties = std::make_shared<std::vector<std::shared_ptr<XamlProperty>>>();
    }
    m_spFrameLegacyStorage->m_spAssignedProperties->push_back(inProperty);
}

// Get the replacement Property Value from the set of replacement property
// values for this frame, and remove the key-value pair.
//
//  Notes:
//      The reason for the removal is that at some later time we'll want to
//      apply the remaining properties the properties that haven't been applied 
//      in the course of replacing properties that are seen in the original XAML.
_Check_return_ HRESULT ObjectWriterFrame::GetAndRemoveReplacementPropertyValue(
    _In_ const std::shared_ptr<XamlProperty>& spProperty, 
    _Out_ std::shared_ptr<XamlQualifiedObject>& qoValue)
{
    if (m_spFrameOverflowStorage && m_spFrameOverflowStorage->m_spReplacementPropertyValues)
    {
        IFC_RETURN(m_spFrameOverflowStorage->m_spReplacementPropertyValues->RemoveByKey(spProperty, qoValue));

        if (m_spFrameOverflowStorage->m_spReplacementPropertyValues->empty())
        {
            // When the last one is removed we clear out the member. 
            // This allows ObjectWriterContext & ObjectWriter to use the faster
            // exists_ReplacementPropertyValues() to see if there are no properties 
            // to replace rather than having to get and then check the size.
            m_spFrameOverflowStorage->m_spReplacementPropertyValues.reset();
        }
    }
    return S_OK;
}
