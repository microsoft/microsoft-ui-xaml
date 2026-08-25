// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewRowTemplateSelector.h"
#include "TableView.h"

// Activation factory registration lives in
// controls/dev/Generated/TableViewRowTemplateSelector.properties.cpp, as for the other
// runtimeclasses here.

void TableViewRowTemplateSelector::SetOwningTableViewInternal(winrt::TableView const& owner)
{
    m_owningTableView = winrt::make_weak(owner);
}

winrt::DataTemplate TableViewRowTemplateSelector::ResolveTemplateFromMarkup(std::wstring_view markup)
{
    try
    {
        return winrt::XamlReader::Load(winrt::hstring{ markup }).try_as<winrt::DataTemplate>();
    }
    catch (...)
    {
        // Markup is constant, so failure means the Tabular metadata provider could not resolve
        // the container type. Assert rather than surface it later as an opaque
        // "Null encountered as data template" from ItemTemplateWrapper.
        MUX_ASSERT(false);
        return nullptr;
    }
}

void TableViewRowTemplateSelector::EnsureTemplates()
{
    if (m_templatesResolved)
    {
        return;
    }

    m_rowTemplate         = ResolveTemplateFromMarkup(s_rowContainerMarkup);
    m_groupHeaderTemplate = ResolveTemplateFromMarkup(s_groupHeaderContainerMarkup);

    // Latch only once both exist: latching on entry would make one failure permanent and leave
    // the table empty for the life of the control.
    m_templatesResolved = m_rowTemplate != nullptr && m_groupHeaderTemplate != nullptr;
    MUX_ASSERT(m_templatesResolved);
}

void TableViewRowTemplateSelector::Detach()
{
    // Breaks the template -> pool edge; see the header for why this cannot be a destructor.
    if (m_rowTemplate)
    {
        winrt::RecyclePool::SetPoolInstance(m_rowTemplate, nullptr);
    }

    if (m_groupHeaderTemplate)
    {
        winrt::RecyclePool::SetPoolInstance(m_groupHeaderTemplate, nullptr);
    }

    m_rowTemplate = nullptr;
    m_groupHeaderTemplate = nullptr;
    m_templatesResolved = false;
    m_owningTableView = nullptr;
}

winrt::DataTemplate TableViewRowTemplateSelector::SelectTemplateCore(winrt::IInspectable const& item)
{
    EnsureTemplates();

    auto kind = TableViewRowKind::Data;
    if (auto const owner = m_owningTableView.get())
    {
        kind = winrt::get_self<TableView>(owner)->GetRowKindForItem(item);
    }

    return kind == TableViewRowKind::GroupHeader ? m_groupHeaderTemplate : m_rowTemplate;
}

winrt::DataTemplate TableViewRowTemplateSelector::SelectTemplateCore(
    winrt::IInspectable const& item,
    winrt::DependencyObject const& /*container*/)
{
    // Container identity cannot change the answer.
    return SelectTemplateCore(item);
}
