// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <NameScopeRoot.h>

#include <NameScopeTable.h>
#include <StandardNameScopeTable.h>
#include <TemplateNameScopeTable.h>

#include <CDependencyObject.h>
#include <TemplateContent.h>
#include <CControl.h>
#include <ContentPresenter.h>
#include <ItemsPresenter.h>
#include "XamlTelemetry.h"

#include <FrameworkUdk/Containment.h>

// Bug 54433864: [Coca-Cola] Using WinUI ListView and/or ItemsRepeater causes a substantial increase in unmanaged memory usage
// Bug 55948921: [WASDK 1.6] Multiple std::unordered_sets, unordered_maps, vectors grow unbounded
#define WINAPPSDK_CHANGEID_55948921 55948921

namespace Jupiter {
    namespace NameScoping {
        bool NameScopeRoot::HasStandardNameScopeTable(_In_ const CDependencyObject* namescopeOwner) const
        {
            return m_namescopeTables.count(std::make_pair(namescopeOwner, NameScopeType::StandardNameScope)) > 0;
        }

        void NameScopeRoot::EnsureNameScope(_In_ const CDependencyObject* nameScopeOwner, _In_opt_ CTemplateContent* templateContent)
        {
            auto& entry = m_namescopeTables[
                std::make_pair(nameScopeOwner, templateContent != nullptr ? NameScopeType::TemplateNameScope : NameScopeType::StandardNameScope)];

            if (!entry && !templateContent)
            {
                entry.reset(new StandardNameScopeTable());
            }
            else if (templateContent)
            {
                entry.reset(new TemplateNameScopeTable(xref_ptr<CTemplateContent>(templateContent)));
            }
        }

        void NameScopeRoot::RemoveNameScopeIfExists(_In_ const CDependencyObject* nameScopeOwner, NameScopeType nameScopeType)
        {
            m_namescopeTables.erase(std::make_pair(nameScopeOwner, nameScopeType));

            if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_55948921>())
            {
                // Shrink down the backing vector if there are too many empty buckets. We have scenarios where lots of tabs
                // are opened and closed, and after garbage collection this map has over 32k buckets yet only ~30 items,
                // which unnecessarily takes up memory. As a heuristic, shrink when there are 100x more buckets than items.
                // Also only shrink if there are more than 500 buckets so we don't thrash when there are only a few
                // elements.
                if (m_namescopeTables.bucket_count() > 500 && m_namescopeTables.load_factor() < 0.01)
                {
                    // Note: rehash in xhash only increases the number of buckets. We have to build a new unordered_map.
                    NameScopeTableMap shrunk(std::make_move_iterator(m_namescopeTables.begin()), std::make_move_iterator(m_namescopeTables.end()));

                    TraceLoggingProviderWrite(
                        XamlTelemetry, "Memory_ResizeNameScopeRootUnorderedMap",
                        TraceLoggingUInt64(m_namescopeTables.bucket_count(), "OriginalBucketCount"),
                        TraceLoggingUInt64(shrunk.bucket_count(), "NewBucketCount"),
                        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

                    m_namescopeTables = std::move(shrunk);
                }
            }
        }

        void NameScopeRoot::ClearNamedObjectIfExists(const xstring_ptr_view& name, _In_ const CDependencyObject* nameScopeOwner)
        {
            auto entry = m_namescopeTables.find(std::make_pair(nameScopeOwner, NameScopeType::StandardNameScope));
            if (entry != m_namescopeTables.end())
            {
                entry->second->UnregisterName(name);
            }
        }

        xref_ptr<CDependencyObject> NameScopeRoot::GetNamedObjectIfExists(const xstring_ptr_view& name, const CDependencyObject* nameScopeOwner, NameScopeType nameScopeType)
        {
            auto entry = m_namescopeTables.find(std::make_pair(nameScopeOwner, nameScopeType));
            if (entry == m_namescopeTables.end()) return nullptr;
            return entry->second->TryGetElement(name);
        }

        void NameScopeRoot::ReleaseAllTables(bool flagFinalShutdown)
        {
            ASSERT(!m_finalShutdown);
            m_namescopeTables.clear();
            m_finalShutdown = flagFinalShutdown;
        }

        void NameScopeRoot::SetNamedObject(const xstring_ptr_view& name, const CDependencyObject* nameScopeOwner, NameScopeType nameScopeType, xref_ptr<CDependencyObject> element)
        {
            // If you hit an access violation here because 'nameScopeTable' is nullptr and 'nameScopeType' is NameScopeType::TemplateNamescope,
            // then you have very likely run into a situation similar to MSFT:11717061, particularly if the attempted name registration is
            // occurring during realization of an XBFv2-deferred object.
            //
            // If that is the case, then the AV is occurring because the Control owning the desired template namescope discarded its expanded template (possibly because it
            // left the live tree and the Template property was being set by its implicit style), but some piece of code is triggering XBFv2 realization via a cached template
            // part, which brings us here. Because std::unordered_map::operator[] will default construct a value if the key does not exist, 'nameScopeTable' ends up being an
            // empty std::unique_ptr that is then dereferenced.
            //
            // Since the expanded template has been discarded, all cached template parts are no longer valid. Audit your code for uses of invalid cached template parts
            // and, if necessary, add code to either clean up the cached references or verify that they are still valid before using them.
            // See PivotHeaderItem for an example of such.
            auto& nameScopeTable = m_namescopeTables[std::make_pair(nameScopeOwner, nameScopeType)];
            ASSERT(nameScopeTable);
            nameScopeTable->RegisterName(name, std::move(element));
        }

        void NameScopeRoot::SetNamedObject(const xstring_ptr_view& name, const CDependencyObject* nameScopeOwner, NameScopeType nameScopeType, xref::weakref_ptr<CDependencyObject> weakElement)
        {
            // If you hit an access violation here because 'nameScopeTable' is nullptr and 'nameScopeType' is NameScopeType::TemplateNamescope,
            // then you have very likely run into a situation similar to MSFT:11717061, particularly if the attempted name registration is
            // occurring during realization of an XBFv2-deferred object.
            //
            // If that is the case, then the AV is occurring because the Control owning the desired template namescope discarded its expanded template (possibly because it
            // left the live tree and the Template property was being set by its implicit style), but some piece of code is triggering XBFv2 realization via a cached template
            // part, which brings us here. Because std::unordered_map::operator[] will default construct a value if the key does not exist, 'nameScopeTable' ends up being an
            // empty std::unique_ptr that is then dereferenced.
            //
            // Since the expanded template has been discarded, all cached template parts are no longer valid. Audit your code for uses of invalid cached template parts
            // and, if necessary, add code to either clean up the cached references or verify that they are still valid before using them.
            // See PivotHeaderItem for an example of such.
            auto& nameScopeTable = m_namescopeTables[std::make_pair(nameScopeOwner, nameScopeType)];
            ASSERT(nameScopeTable);
            nameScopeTable->RegisterName(name, std::move(weakElement));
        }

        void NameScopeRoot::SetNamedObject(const xstring_ptr_view& name, const CDependencyObject* nameScopeOwner, NameScopeType nameScopeType, std::weak_ptr<INameScopeDeferredElementCreator> deferredCreator)
        {
            // If you hit an access violation here because 'nameScopeTable' is nullptr and 'nameScopeType' is NameScopeType::TemplateNamescope,
            // then you have very likely run into a situation similar to MSFT:11717061, particularly if the attempted name registration is
            // occurring during realization of an XBFv2-deferred object.
            //
            // If that is the case, then the AV is occurring because the Control owning the desired template namescope discarded its expanded template (possibly because it
            // left the live tree and the Template property was being set by its implicit style), but some piece of code is triggering XBFv2 realization via a cached template
            // part, which brings us here. Because std::unordered_map::operator[] will default construct a value if the key does not exist, 'nameScopeTable' ends up being an
            // empty std::unique_ptr that is then dereferenced.
            //
            // Since the expanded template has been discarded, all cached template parts are no longer valid. Audit your code for uses of invalid cached template parts
            // and, if necessary, add code to either clean up the cached references or verify that they are still valid before using them.
            // See PivotHeaderItem for an example of such.
            auto& nameScopeTable = m_namescopeTables[std::make_pair(nameScopeOwner, nameScopeType)];
            ASSERT(nameScopeTable);
            nameScopeTable->RegisterName(name, std::move(deferredCreator));
        }

        void NameScopeRoot::MoveNameScopeTableEntries(
            const CDependencyObject* sourceNameScopeOwner,
            const CDependencyObject* destinationNameScopeOwner,
            const NameScopeType nameScopeType)
        {
            if (nameScopeType == NameScopeType::StandardNameScope)
            {
                auto pSourceTable = static_cast<StandardNameScopeTable*>(m_namescopeTables[std::make_pair(sourceNameScopeOwner, nameScopeType)].get());
                auto pDestinationTable = static_cast<StandardNameScopeTable*>(m_namescopeTables[std::make_pair(destinationNameScopeOwner, nameScopeType)].get());
                ASSERT(pSourceTable);
                ASSERT(pDestinationTable);

                StandardNameScopeTable::MoveEntries(pSourceTable, pDestinationTable);
                RemoveNameScopeIfExists(sourceNameScopeOwner, NameScopeType::StandardNameScope);
            }
            else
            {
                // We only support moving StandardNameScopeTable entries
                ASSERT(false);
            }
        }

        NameScopeRoot::NameScopeRoot() = default;
        NameScopeRoot::~NameScopeRoot() = default;
    }
}