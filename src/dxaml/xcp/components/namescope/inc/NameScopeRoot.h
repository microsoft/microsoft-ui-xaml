// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "XcpAllocation.h"

class CDependencyObject;
class CTemplateContent;

namespace Jupiter {
    namespace NameScoping {

        class INameScopeDeferredElementCreator;
        class NameScopeTable;

        // NameScopes - what are they?
        //
        // Today in the platform we have two types of NameScopes:
        // - ParseTime/Template NameScopes
        // - RunTime/Standard NameScopes
        //
        // Previously the platfrom supported a more sophisticated set of features here, but we've
        // simplified and streamlined these concepts over time.
        //
        // TemplateNameScopes are NameScopes created when a template is expanded and the parser encounters x:Name
        // directives. Their primary use-case is to power the machinery behind GetTemplateChild in control templates.
        // They are create-one and set-in-stone, as it's generally assumed that the children of an expanded control
        // template are ALWAYS going to remain members of that control template, and even if they are removed they
        // maintain special knowledge of their template parent via a weakref.
        //
        // When a template is expanded all the members have the IsTemplateNameScopeMember flag set on them, which
        // tells them to ensure they only register in a TemplateNameScope table. This has performance benefits:
        // - The NameScope owner can be found from a Template child by simply retrieving the TemplateParent.
        //   StandardNameScopes find their owners through a tree walk towards root.
        // - Enter/Leave is irrelevant now. The items are registered once in the table and even if they leave
        //   the runtime tree they are considered members of that NameScope.
        // - The strings can be shared between instances of a given Template.
        //
        // StandardNameScopes are a little different. They are present everywhere ControlTemplates aren't and
        // create the idea of a NameScope determined by a runtime VisualTree. When elements enter the tree they
        // register themselves in the StandardNameScope table for the given XAML fragment they're a part of.

        enum class NameScopeType
        {
            TemplateNameScope,
            StandardNameScope
        };

        struct NameScopeHash
        {
            std::size_t operator()(const std::pair<const CDependencyObject*, NameScopeType> &x) const
            {
                return std::hash<const CDependencyObject*>()(x.first) ^ std::hash<size_t>()(static_cast<size_t>(x.second));
            }
        };

        class NameScopeRoot
        {
        public:

            // For forward declaration of unique_ptr types...
            ~NameScopeRoot();
            NameScopeRoot();

            // This flag should only be used as an optimization in cases that warrant it for most
            // consumers as when an owner acquires a NameScope table is pretty hard to pin down.
            bool HasStandardNameScopeTable(_In_ const CDependencyObject* namescopeOwner) const;

            // When TemplateContent is non-null this method will ensure a TemplateNameScope on the
            // nameScopeOwner exists and matches the CTemplateContent instance. Otherwise it will create
            // a StandardNameScopeTable.
            void EnsureNameScope(_In_ const CDependencyObject* nameScopeOwner, _In_opt_ CTemplateContent* templateContent);

            // All the following methods end in 'IfExists'. The policy with NameScopes has generally become pretty
            // laissez-faire so none of these methods throw errors if the entry isn't present.

            // CDO should call this ONCE and ONLY ONCE in its dtor to force clear the namescope table
            // after we're sure it's never going to be used again. Do not try to do something cute with this
            // method.
            void RemoveNameScopeIfExists(_In_ const CDependencyObject* nameScopeOwner, NameScopeType nameScopeType);

            // ClearNamedObject can only operator on StandardNameScopes.
            void ClearNamedObjectIfExists(const xstring_ptr_view& name, _In_ const CDependencyObject* nameScopeOwner);

            xref_ptr<CDependencyObject> GetNamedObjectIfExists(const xstring_ptr_view& name, const CDependencyObject* nameScopeOwner, NameScopeType nameScopeType);

            // These methods will store the named object in the NameScope table for the given owner. They ASSERT that
            // the table already exists, and will overwrite any existing entry silently.
            void SetNamedObject(const xstring_ptr_view& name, const CDependencyObject* nameScopeOwner, NameScopeType nameScopeType, xref_ptr<CDependencyObject> element);
            void SetNamedObject(const xstring_ptr_view& name, const CDependencyObject* nameScopeOwner, NameScopeType nameScopeType, xref::weakref_ptr<CDependencyObject> weakElement);
            void SetNamedObject(const xstring_ptr_view& name, const CDependencyObject* nameScopeOwner, NameScopeType nameScopeType, std::weak_ptr<INameScopeDeferredElementCreator> deferredCreator);

            // Clear all the namescope tables, along with any strong references they held to
            // elements.
            void ReleaseAllTables(bool flagFinalShutdown);

            // Useful to ASSERT proper shutdown of CCoreServices. If DOs call in to unregister themselves
            // after the tables have tore down this is breaking one of our shutdown invariants and should
            // be ASSERTed.
            bool HasBeenReleased() const { return m_finalShutdown; }

            // This will move entries from one StandardnameScope to another StandardNameScope.
            // ASSERTs that both tables already exist.
            // This should be used sparingly as it is rare to need to wholesale move one table's
            // entries into another table. An example of such a scenario is Window in Desktop apps; its Content
            // is parented into the visual tree by a CWindowChrome and thus belongs to the CWindowChrome's namescope,
            // not the Window's. Although named objects in the Content that live on Entered dependency properties
            // will be registered into the new namescope, those that don't (e.g. object with x:Name assigned to a custom
            // attached property) will be left orphaned in the old namescope.
            void MoveNameScopeTableEntries(
               const CDependencyObject* sourceNameScopeOwner,
               const CDependencyObject* destinationNameScopeOwner,
               const NameScopeType nameScopeType);

        private:
            // While these data structures should generally be more efficient than the previous ones,
            // there's a ton of optimizations we can do here as well. If measurement-guided investigations
            // show these structures have a significant impact in any key scenarios we can revisit and
            // build more compact ones.
#if XCP_MONITOR
            using NamescopeKey = std::pair<const CDependencyObject*, NameScopeType>;
            using NamescopeAllocator = XcpAllocation::LeakIgnoringAllocator<std::pair<const NamescopeKey, std::unique_ptr<NameScopeTable>>>;
            typedef std::unordered_map<std::pair<const CDependencyObject*, NameScopeType>, std::unique_ptr<NameScopeTable>, NameScopeHash, std::equal_to<NamescopeKey>, NamescopeAllocator> NameScopeTableMap;
#else
            typedef std::unordered_map<std::pair<const CDependencyObject*, NameScopeType>, std::unique_ptr<NameScopeTable>, NameScopeHash> NameScopeTableMap;
#endif
            NameScopeTableMap m_namescopeTables;

            bool m_finalShutdown = false;
        };
    }
}
