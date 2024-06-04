// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RecyclePool.g.h"

// For MuxFinal builds we don't include RecyclePool in the WinMD
// and we use custom definition of the dependency properties
#ifdef MUX_PRERELEASE
#include "RecyclePool.properties.h"
#endif

class RecyclePool :
    public ReferenceTracker<RecyclePool, winrt::implementation::RecyclePoolT, winrt::composing>
#ifdef MUX_PRERELEASE
    , public RecyclePoolProperties
#endif
{
public:
    RecyclePool();

#ifndef MUX_PRERELEASE
    // We're using this runtime instance in a place that it might leak out and .NET Core gets upset when
    // it sees types not in the public surface area. Return object since no one needs to know the real type.
    hstring GetRuntimeClassName() const
    {
        return winrt::hstring_name_of<winrt::IInspectable>();
    }
#endif

#pragma region IRecyclePool
    void PutElement(
        winrt::UIElement const& element,
        winrt::hstring const& key);
    void PutElement(
        winrt::UIElement const& element,
        winrt::hstring const& key,
        winrt::UIElement const& owner);
    winrt::UIElement TryGetElement(
        winrt::hstring const& key);
    winrt::UIElement TryGetElement(
        winrt::hstring const& key,
        winrt::UIElement const& owner);
#pragma endregion

#pragma region IRecyclePoolOverrides
    void PutElementCore(
        winrt::UIElement const& element,
        winrt::hstring const& key,
        winrt::UIElement const& owner);
    winrt::UIElement TryGetElementCore(
        winrt::hstring const& key,
        winrt::UIElement const& owner);
#pragma endregion

#pragma region IRecyclePoolStatics 
    static winrt::DependencyProperty ReuseKeyProperty() { return s_reuseKeyProperty; }
    static winrt::hstring GetReuseKey(winrt::UIElement const& element);
    static void SetReuseKey(winrt::UIElement const& element, winrt::hstring const& value);

#ifndef MUX_PRERELEASE
    static winrt::DependencyProperty PoolInstanceProperty() { return s_PoolInstanceProperty; }
#endif
    static winrt::RecyclePool GetPoolInstance(winrt::DataTemplate const& dataTemplate);
    static void SetPoolInstance(winrt::DataTemplate const& dataTemplate, winrt::RecyclePool const& recyclePool);
#pragma endregion

    static void EnsureProperties();
    static void ClearProperties();

    /* internal */
    static winrt::DataTemplate GetOriginTemplate(winrt::UIElement const& element);
    static void SetOriginTemplate(winrt::UIElement const& element, winrt::DataTemplate const& value);

private:
#ifndef MUX_PRERELEASE
    static GlobalDependencyProperty s_PoolInstanceProperty;
#endif
    static GlobalDependencyProperty s_reuseKeyProperty;
    static GlobalDependencyProperty s_originTemplateProperty;

    winrt::Panel EnsureOwnerIsPanelOrNull(const winrt::UIElement& owner);

    struct ElementInfo
    {
        ElementInfo(const ITrackerHandleManager* refManager, const winrt::UIElement& element, const winrt::Panel& owner)
            :m_element(refManager, element), m_owner(refManager, owner) {}

        winrt::UIElement Element() const { return m_element.get(); };
        winrt::Panel Owner() const { return m_owner.get(); };

    private:
        tracker_ref<winrt::UIElement> m_element;
        tracker_ref<winrt::Panel> m_owner;
    };

    std::map<winrt::hstring /*key*/, std::vector<ElementInfo>> m_elements;
};
