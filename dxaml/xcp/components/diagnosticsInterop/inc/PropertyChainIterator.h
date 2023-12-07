// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <iterator>
#include "style.h"
#include "XamlOM.WinUI.h"
#include "MetadataIterator.h"
#include <set>
#include "weakref_ptr.h"

namespace DirectUI {
    class DependencyObject;
}

namespace Diagnostics {

    struct PropertyChainData
    {
        explicit PropertyChainData(
            _In_ const xref_ptr<CStyle>& style,
            _In_ KnownPropertyIndex propIndex,
            _In_ BaseValueSource src)
            : Style(style)
            , Index(propIndex)
            , Source(src)
        {
        }

        PropertyChainData()
            : Index(KnownPropertyIndex::UnknownType_UnknownProperty)
            , Source(BaseValueSourceUnknown)
        {
        }

        bool operator<(const PropertyChainData& rhs) const
        {
            if (Index >= rhs.Index)
            {
                if (Source >= rhs.Source)
                {
                    return Source < rhs.Source;
                }
                return true;
            }
            return true;
        }

        bool operator>(const PropertyChainData& rhs) const
        {
            return !operator<(rhs);
        }

        bool operator==(const PropertyChainData& rhs) const
        {
            return Index == rhs.Index && Source == rhs.Source && Style == rhs.Style;
        }

        bool operator!=(const PropertyChainData& rhs) const
        {
            return !operator==(rhs);
        }

        xref_ptr<CStyle> Style;
        KnownPropertyIndex Index;
        BaseValueSource Source;
    };

    // PropertyChainIterator has an exception based implementations. Usage of this types should be
    // wrapped around a try-catch handler. It is meant to be used at the API boundary for XamlDiagnostics
    // for the GetPropertyValuesChain call
    class PropertyChainIterator final : public std::iterator<std::input_iterator_tag, PropertyChainData>
    {
    public:
        explicit PropertyChainIterator(
            _In_ const xref_ptr<CDependencyObject>& dependencyObject,
            _In_ KnownPropertyIndex index = EnumIterator<KnownPropertyIndex>::Begin());

        PropertyChainIterator();

        // Throws if cannot get the next property to evaluate or if try to iterate past the end
        PropertyChainIterator& operator++();
        PropertyChainIterator operator++(int);

        bool operator==(const PropertyChainIterator& rhs) const;
        bool operator!=(const PropertyChainIterator& rhs) const;
        PropertyChainData& operator*();

        // Returns whether or not the property is one that we want to hand back to
        // VisualStudio.  If allowCustomProperties is false, we also consider
        // custom user properties to be invalid.
        static bool IsInvalidProperty(_In_ const CPropertyBase* depProp,
            bool allowCustomProperties = false);

        // Checks whether the property is one that we want to hand back to VisualStudio
        // and that it can be assigned to this object.
        static bool IsValidPropertyForObject(
            _In_ const CPropertyBase* const prop,
            _In_ CDependencyObject* obj);

        // Gets the effective base value source for the given object and property index,
        // not including animated values.
        static BaseValueSource GetEffectiveBaseValueSource(
            _In_ const xref_ptr<CDependencyObject>& depObj,
            _In_ KnownPropertyIndex propertyIndex);

    private:
        void GoToNextProperty();
        void GoToNextSource();

        // Get's the next style and BaseValueSource to be used. There are cases where even
        // though the BaseValueSource is reported as being a style, we are unable to find an
        // appropriate style. When that occurs, this will return nullptr for the style and 
        // BaseValueSourceUnknown.
        static PropertyChainData GetNextSourceData(
            _In_ const xref_ptr<CDependencyObject>& depObj,
            _In_ const xref_ptr<CStyle>& currentStyle,
            _In_ BaseValueSource currentSource,
            _In_ KnownPropertyIndex index);

        // Get's the next BaseValueSource and property to be used. Moves the iterator forward
        // until it finds a property to use for the object. If no property was found, propertyIter
        // will be equal to endIndex and BaseValueSourceUnknown will be returned.
        static PropertyChainData GetNextPropertyData(
            _In_ const xref_ptr<CDependencyObject>& depObj,
            _In_ KnownPropertyIndex currentIndex,
            _In_ KnownPropertyIndex endIndex);

        // Get's the correct style to be used for the current evaluation based on
        // the BaseValueSource and KnownPropertyIndex
        static xref_ptr<CStyle> GetStyleForEvaluation(
            _In_ const xref_ptr<CDependencyObject>& depObj,
            _In_ BaseValueSource currentSource,
            _In_ KnownPropertyIndex propertyIndex);

        static xref_ptr<CStyle> GetNextStyleForEvaluation(
            _In_ const xref_ptr<CStyle>& currentStyle,
            _In_ KnownPropertyIndex propertyIndex);

        // Returns whether or not the property is actually from a based on setter
        static bool StyleHasProperty(
            _In_ const xref_ptr<CStyle>& currentStyle,
            _In_ KnownPropertyIndex index);

    private:
        xref_ptr<CDependencyObject>         m_do;
        PropertyChainData                   m_data;
        KnownPropertyIndex                  m_lastKnownIndex;
    };
}

