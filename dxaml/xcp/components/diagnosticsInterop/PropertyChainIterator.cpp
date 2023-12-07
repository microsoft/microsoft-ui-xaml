// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "inc\PropertyChainIterator.h"
#include "collection\inc\definitioncollection.h"

#include "MetadataAPI.h"
#include "DOPointerCast.h"
#include "TypeTableStructs.h"
#include "Grid.h"
#include "ccontrol.h"

#include "EffectiveValueEntry.h"
#include "DependencyObject.h"
#include "DXamlServices.h"

using namespace DirectUI;

namespace Diagnostics {

    PropertyChainIterator::PropertyChainIterator(
        _In_ const xref_ptr<CDependencyObject>& dependencyObject,
        _In_ KnownPropertyIndex index)
        : m_data(GetNextPropertyData(m_do, index, EnumIterator<KnownPropertyIndex>::End()))
        , m_do(dependencyObject)
        , m_lastKnownIndex(EnumIterator<KnownPropertyIndex>::End())
    {
    }

    PropertyChainIterator::PropertyChainIterator()
        : m_data(PropertyChainData(nullptr, EnumIterator<KnownPropertyIndex>::End(), BaseValueSourceUnknown))
        , m_lastKnownIndex(EnumIterator<KnownPropertyIndex>::End())
    {
    }

    //
    // Evaluate property chain using following precedence
    // 1. LocalValue/Animated (depending on whether there was an animated value and it wasn't overwritten by the local value)
    // 2. Animated/LocalValue
    // 3. Style
    // 4. Built-in style
    // 5. Default value
    //
    // Once we evaluate the default value, we'll move to the next property
    PropertyChainIterator& PropertyChainIterator::operator++()
    {
        IFCFAILFAST(m_do ? S_OK : E_POINTER); // Trying to iterate past end of property chain.  By definition in the std libraries this is undefined behavior.
        auto previousSource = m_data.Source;
        auto property = MetadataAPI::GetPropertyBaseByIndex(m_data.Index);
        bool animatedPropertyOverwritten = false;
        if (auto depProp = property->AsOrNull<CDependencyProperty>())
        {
            animatedPropertyOverwritten = m_do->IsAnimatedPropertyOverwritten(depProp);
        }

        // Normally we go animated -> local -> style -> built-in style -> default
        // If there's an animated value and it was overwritten, we instead start at the local value then go to the animated value
        if (animatedPropertyOverwritten && previousSource == BaseValueSourceLocal)
        {
            m_data.Source = BaseValueSource::Animation;
        }
        else if (property->Is<CSimpleProperty>())
        {
            // Simple properties are only in the local value source
            ASSERT(previousSource == BaseValueSourceLocal);
            GoToNextProperty();
        }
        else
        {
            switch (previousSource)
            {
            case BaseValueSource::Animation:
                //If we just evaluated the animated value, the base value source for the property is the thing to get next.
                //For non-local values (styles or default/unknown values), we'll fall through to their evaluation to fill in the
                //style information or move to the next index.  But for local values we shouldn't take any other action.
                //We need to explicitly check the base value source is local, since it's possible to have an animated value but not also a local one.
                if (!animatedPropertyOverwritten && GetEffectiveBaseValueSource(m_do, m_data.Index) == BaseValueSourceLocal)
                {
                    m_data.Source = BaseValueSourceLocal;
                    break;
                }
            case BaseValueSourceLocal:
            case BaseValueSourceStyle:
            case BaseValueSourceBuiltInStyle:
            {
                GoToNextSource();
                break;
            }
            case BaseValueSourceUnknown:
            case BaseValueSourceDefault:
                GoToNextProperty();
                break;
            }
        }

        return *this;
    }

    PropertyChainIterator PropertyChainIterator::operator++(int)
    {
        PropertyChainIterator temp(*this);
        operator++();
        return temp;
    }

    bool PropertyChainIterator::operator!=(const PropertyChainIterator& rhs) const
    {   
        return !operator==(rhs);
    }

    bool PropertyChainIterator::operator==(const PropertyChainIterator& rhs) const
    {
        return  m_data.Source == rhs.m_data.Source &&
                m_do == rhs.m_do &&
                m_data.Style == rhs.m_data.Style &&
                m_data.Index == rhs.m_data.Index;
    }

    PropertyChainData& PropertyChainIterator::operator*()
    {
        return m_data;
    }

    void PropertyChainIterator::GoToNextProperty()
    {
        m_data = GetNextPropertyData(m_do, m_data.Index, m_lastKnownIndex);
        if (m_data.Index == m_lastKnownIndex)
        {
            m_do.reset();
        }
    }

    void PropertyChainIterator::GoToNextSource()
    {
        m_data = GetNextSourceData(m_do, m_data.Style, m_data.Source, m_data.Index);
    }

    PropertyChainData PropertyChainIterator::GetNextSourceData(
        _In_ const xref_ptr<CDependencyObject>& depObj,
        _In_ const xref_ptr<CStyle>& currentStyle,
        _In_ BaseValueSource currentSource,
        _In_ KnownPropertyIndex index)
    {
        auto nextSource = currentSource;

        // If we have a current style, then we'll try to continue evaluating the based on change
        xref_ptr<CStyle> nextStyle = currentStyle ? GetNextStyleForEvaluation(currentStyle, index) : nullptr;
  
        switch (currentSource)
        {
            // In the case of BaseValueSourceLocal or Animation, we'll want to get the ActiveStyle again to re-start the
            // chain
        case BaseValueSource::Animation:
        case BaseValueSourceLocal:
        {
            nextStyle = GetStyleForEvaluation(depObj, BaseValueSourceStyle, index);
            nextSource = BaseValueSourceStyle;
        }
        // Purposefully fallthrough to BaseValueSourceStyle. At this point we have 1 of 2 possibilities:
        //  1. currentSource == BaseValueSourceStyle. Which means nextStyle is either the next based on style,
        //     or it's null.
        //  2. currentSource == BaseValueSourceLocal or BaseValueSource::Animation. Which means nextStyle is either the active style, or null
        //     if no active style.
        // In both cases, if nextStyle is null, we want to move to the built-in style
        case BaseValueSourceStyle:
            if (!nextStyle)
            {
                nextStyle = GetStyleForEvaluation(depObj, BaseValueSourceBuiltInStyle, index);
                nextSource = BaseValueSourceBuiltInStyle;
            }
            // Like above, we purposefully fall through. At this point we have 1 of 2 possibilities:
            //  1. currentSource == BaseValueSourceBuiltInStyle. Which means nextStyle is either the next based on style,
            //     or it's null.
            //  2. currentSource == BaseValueSourceLocal or BaseValueSource::Animation. Which means we've fallen through here and nextStyle being null
            //     means there is no active or built-in style.
            // In both cases, if nextStyle is null, we want to move to BaseValueSourceUnknown
        case BaseValueSourceBuiltInStyle:
            // If no style is found, then if we have a current one, it means we reached the end of the property chain
            // for this style. Otherwise, we got a BaseValueSourceStyle without actually having a style, so we'll report
            // BaseValueSourceUnknown
            if (!nextStyle)
            {
                // If we have a current style, then the chain is over and we restart at BaseValueSourceDefault
                nextSource = BaseValueSourceDefault;
            }

            break;
        default:
            IFCFAILFAST(E_UNEXPECTED);
        }

        return PropertyChainData(nextStyle, index, nextSource);
    }

    bool PropertyChainIterator::StyleHasProperty(
        _In_ const xref_ptr<CStyle>& currentStyle,
        _In_ KnownPropertyIndex index)
    {
        const bool getPropertyFromBasedOn = false;
        bool gotValue = false;

        CValue value;
        if (FAILED(currentStyle->GetPropertyValue(index, getPropertyFromBasedOn, &value, &gotValue))) return false;
        return gotValue;
    }

    // Get's the style to be used for this property
    xref_ptr<CStyle> PropertyChainIterator::GetStyleForEvaluation(
        _In_ const xref_ptr<CDependencyObject>& depObj,
        _In_ BaseValueSource currentSource,
        _In_ KnownPropertyIndex index)
    {
        xref_ptr<CStyle> style;
        if (currentSource == BaseValueSourceStyle)
        {
            auto framework = do_pointer_cast<CFrameworkElement>(depObj);
            if (framework)
            {
                style = framework->GetActiveStyle();
            }
        }
        else if (currentSource == BaseValueSourceBuiltInStyle)
        {
            auto control = do_pointer_cast<CControl>(depObj);
            if (control)
            {
                IFCFAILFAST(do_pointer_cast<CControl>(depObj)->GetBuiltInStyle(style.ReleaseAndGetAddressOf()));
            }
        }

        while (style && !StyleHasProperty(style, index))
        {
            style = style->m_pBasedOn;
        }

        return style;
    }

    xref_ptr<CStyle> PropertyChainIterator::GetNextStyleForEvaluation(
        _In_ const xref_ptr<CStyle>& currentStyle,
        _In_ KnownPropertyIndex propertyIndex)
    {
        xref_ptr<CStyle> style = currentStyle;
        do
        {
            style = style->m_pBasedOn;
        } while (style && !StyleHasProperty(style, propertyIndex));

        return style;
    }

    PropertyChainData PropertyChainIterator::GetNextPropertyData(
        _In_ const xref_ptr<CDependencyObject>& depObj,
        _In_ KnownPropertyIndex currentIndex,
        _In_ KnownPropertyIndex endIndex)
    {
        EnumIterator<KnownPropertyIndex> propertyIter(currentIndex);
        ++propertyIter;
        const CPropertyBase* prop = nullptr;
        while (*propertyIter != endIndex) 
        {
            prop = MetadataAPI::GetPropertyBaseByIndex(*propertyIter);

            if (prop && !IsInvalidProperty(prop) && IsValidPropertyForObject(prop, depObj.get()))
            {
                // Found a property we want, we're good to go.
                break;
            }
            ++propertyIter;
        }

        // If we haven't moved past the end, then get the next BaseValueSource to evaluate
        if (*propertyIter == endIndex)
        {
            return PropertyChainData(nullptr, endIndex, BaseValueSourceUnknown);
        }

        if (auto depProp = prop->AsOrNull<CDependencyProperty>())
        {
            // We always want to return the current active value's BaseValueSource first.  If a property has an animated value,
            // it should take priority, but the animated value can be overwritten by a local value.  We need
            // to check if the animated value was overwritten and return BaseValueSourceLocal as the first value
            // if it was. 
            if (depObj->IsAnimatedProperty(depProp))
            {
                //We should evaluate the local value first if it exists and overwrote the animated value
                if (depObj->IsAnimatedPropertyOverwritten(depProp))
                {
                    return PropertyChainData(nullptr, *propertyIter, BaseValueSourceLocal);
                }
                else
                {
                    return PropertyChainData(nullptr, *propertyIter, BaseValueSource::Animation);
                }
            }

            // If there aren't any animated values in play, get the effective base value source normally
            auto baseValueSource = GetEffectiveBaseValueSource(depObj, *propertyIter);

            if (baseValueSource == BaseValueSourceStyle || baseValueSource == BaseValueSourceBuiltInStyle)
            {
                // If the next source is a style then get the style object. We'll use GetNextSourceData and
                // pass in BaseValueSourceLocal because CDependencyObject::GetBaseValueSource currently returns
                // BaseValueSourceStyle for properties set by built-in styles (see comment in CDependencyObject::InvalidateProperty as to why)
                // It's ok to pass in BaseValueSourceLocal, because we'll just fall through to the BaseValueSourceBuiltInStyle if
                // this is actually set by the built-in style.
                auto data = GetNextSourceData(depObj, nullptr, BaseValueSourceLocal, *propertyIter);

                // There are cases where we CDependencyObject::GetBaseValueSource returns BaseValueSourceStyle but there is no style object.
                // For those we'll report BaseValueSourceUnknown
                if (!data.Style)
                {
                    data.Source = BaseValueSourceUnknown;
                }

                return data;
            }

            return PropertyChainData(nullptr, *propertyIter, baseValueSource);
        }
        else
        {
            MICROSOFT_TELEMETRY_ASSERT_DISABLED(prop->Is<CSimpleProperty>());
            // Simple properties can only be set locally
            return PropertyChainData(nullptr, *propertyIter, BaseValueSourceLocal);
        }
    }

    // Get the base value source, ignoring any animated values even if they're the current active value
    BaseValueSource PropertyChainIterator::GetEffectiveBaseValueSource(
        _In_ const xref_ptr<CDependencyObject>& depObj,
        _In_ KnownPropertyIndex propertyIndex)
    {
        auto depProp = MetadataAPI::GetDependencyPropertyByIndex(propertyIndex);

        auto peer = depObj->GetDXamlPeer();
        auto pValueEntry = peer ? peer->TryGetEffectiveValueEntry(propertyIndex) : nullptr;

        auto baseValueSource = BaseValueSource::BaseValueSourceUnknown;
        if (pValueEntry)
        {
            baseValueSource = pValueEntry->GetBaseValueSource();
        }
        else
        {
            const CDependencyProperty* pUnderlyingDP = nullptr;
            if (SUCCEEDED(MetadataAPI::GetUnderlyingDependencyProperty(depProp, &pUnderlyingDP)))
            {
                // If the underlying property is row or column definitions we need to special case and check the size of 
                // the collection to verify the correct size. If the size is greater than 0, than it is set locally
                if (pUnderlyingDP->GetIndex() == KnownPropertyIndex::Grid_RowDefinitions)
                {
                    auto grid = do_pointer_cast<CGrid>(depObj);
                    baseValueSource = (grid->m_pRowDefinitions && grid->m_pRowDefinitions->GetCount() > 0) ? ::BaseValueSourceLocal : ::BaseValueSourceDefault;
                }
                else if (pUnderlyingDP->GetIndex() == KnownPropertyIndex::Grid_ColumnDefinitions)
                {
                    auto grid = do_pointer_cast<CGrid>(depObj);
                    baseValueSource = (grid->m_pColumnDefinitions && grid->m_pColumnDefinitions->GetCount() > 0) ? ::BaseValueSourceLocal : ::BaseValueSourceDefault;
                }
                else
                {
                    baseValueSource = depObj->GetBaseValueSource(pUnderlyingDP);

                    // The framework will return BaseValueSourceLocal if there's an animated value and no style value, even if there isn't actually a local value
                    // Double check there is actually a local value if we get it back, otherwise there wasn't a style value so correct the source to default
                    if (baseValueSource == BaseValueSourceLocal)
                    {
                        CValue localVal;
                        bool hasLocalValue;
                        bool isTemplateBound;
                        if (SUCCEEDED(depObj->ReadLocalValue(pUnderlyingDP, &localVal, &hasLocalValue, &isTemplateBound)) && !hasLocalValue)
                        {
                            baseValueSource = BaseValueSourceDefault;
                        }
                    }
                }
            }
        }

        return baseValueSource;
    }

    bool PropertyChainIterator::IsValidPropertyForObject(
        _In_ const CPropertyBase* const prop,
        _In_ CDependencyObject* obj)
    {
        const CClassInfo* objType = nullptr;
        // CCustomProperties don't have a TargetType which is used by this method,
        // so it should never be invoked with one
        ASSERT(!prop->Is<CCustomProperty>());
        bool isValid = false;
        ctl::ComPtr<DependencyObject> peer;
        if (SUCCEEDED(DXamlServices::GetPeer(obj, &peer)) &&
            SUCCEEDED(MetadataAPI::GetClassInfoFromObject_SkipWinRTPropertyOtherType(peer.Cast<xaml::IDependencyObject>(), &objType)))
        {
            isValid = MetadataAPI::IsAssignableFrom(prop->GetTargetType()->GetIndex(), objType->GetIndex());
        }

        return isValid;
    }

    bool PropertyChainIterator::IsInvalidProperty(_In_ const CPropertyBase* const prop, bool allowCustomProperties)
    {
        // Helpers for determing invalid property conditions
        const bool isCustomProperty = prop->Is<CCustomProperty>();
        auto propIndex = prop->GetIndex();

        auto depProp = prop->AsOrNull<CDependencyProperty>();
        // Conditions for being an invalid property - if any of these are true it's invalid
        const bool isCollectionAndContentProperty = prop->GetDeclaringType()->IsCollection() && depProp && depProp->IsContentProperty();

        const bool isInvalidCustomProperty = isCustomProperty && !allowCustomProperties;

        const bool isPrivateNonCustomDP = !prop->IsPublic() && !prop->Is<CCustomDependencyProperty>();

        const bool isInvalidPropIndex = propIndex == KnownPropertyIndex::UIElement_ChildrenInternal ||
            propIndex == KnownPropertyIndex::FrameworkElement_Resources ||
            propIndex == KnownPropertyIndex::UnknownType_UnknownProperty;

        // Only dependency properties should have a valid target type.  Custom non-dependency properties
        // have no target type, so it's OK for their target types to be invalid.
        const bool isTargetTypeInvalid = !isCustomProperty && prop->GetTargetType()->GetIndex() == KnownTypeIndex::UnknownType;

        const bool isPropertyTypeInvalid = prop->GetPropertyType()->GetIndex() == KnownTypeIndex::TransitionTarget;

        return isCollectionAndContentProperty ||
            // When an app creates a custom DP, we keep two types of properties in our 
            // custom metadata cache, one of them is the custom DP and the other is 
            // just a regular property. The regular property is not exposed and should
            // never go back to the app. Our metadata seems a little backwards because
            // CustomDependencyProperties don't have the "public" metadata bit set,
            // but otherwise we only want to hand public properties back.
            isInvalidCustomProperty ||
            isPrivateNonCustomDP ||
            isInvalidPropIndex || 
            isTargetTypeInvalid ||
            isPropertyTypeInvalid;
    }
}

