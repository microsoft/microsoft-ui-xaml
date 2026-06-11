// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "IQualifierContextCallback.h"
#include "QualifierFlags.h"
#include "IQualifier.h"
#include "QualifierContext.h"
#include "IVariantMapChangedCallback.h"
#include <algorithm>

template<class TTarget, class TData = void*>
struct VariantMapItem
{
    VariantMapItem(TTarget& t, std::shared_ptr<IQualifier> pQualifier) :
        m_target(t), m_pQualifier(pQualifier)  {};

    VariantMapItem(TTarget& t, std::shared_ptr<IQualifier> pQualifier, TData& pData) :
        m_target(t), m_pQualifier(pQualifier), m_pData(pData) {};

    TTarget m_target;
    std::shared_ptr<IQualifier> m_pQualifier;
    TData m_pData;
};

// Selects the best candidate based on a global context
// including window dimensions, platform, etc.
template<class TTarget, class TData = void*>
class VariantMap : public IQualifierContextCallback
{
    public:
       VariantMap();
        ~VariantMap();

        _Check_return_ HRESULT Add(_In_ TTarget target, _In_ std::shared_ptr<IQualifier>);
        _Check_return_ HRESULT Add(_In_ TTarget target, _In_ std::shared_ptr<IQualifier>, TData pData);

        // Add comparison to Qualifiers
        // Check the value of all dimensions
        _Check_return_ HRESULT Replace(_In_ TTarget target, _In_ std::shared_ptr<IQualifier>, _In_ std::shared_ptr<IQualifier>);
        _Check_return_ HRESULT Replace(_In_ TData data, _In_ std::shared_ptr<IQualifier>, _In_ std::shared_ptr<IQualifier>);

        // Remove qualifier from map
        // Evaluate
        _Check_return_ HRESULT Remove(_In_ TTarget target, _In_ std::shared_ptr<IQualifier>);
        TTarget* SelectedItem();

        // Clears all VariantMapItems for a specific target
        _Check_return_ HRESULT Clear(_In_ TTarget target);

        // If m_isSortedByPrecedence then sort
        // If !isDirty no-op
        // else call evaluate on each qualifier, take the m_target qualified item
        _Check_return_ HRESULT Evaluate();
        _Check_return_ HRESULT OnQualifierContextChanged() override;
        QualifierFlags m_qualifierFlags;

        // Callbacks
        _Check_return_ HRESULT RegisterForChangeCallback(_In_ IVariantMapChangedCallback* listener);
        _Check_return_ HRESULT UnRegisterForChangeCallback(_In_ IVariantMapChangedCallback* listener);

        // Set OnSelectionChanged method evaluated when 'best-fit' selection changes
        void SetOnSelectionChanged(_In_ std::function<HRESULT(TTarget, TTarget, bool)> func);

        static bool CompareQualifiers(_In_ std::shared_ptr<IQualifier> q1, _In_ std::shared_ptr<IQualifier> q2);

        std::vector<VariantMapItem<TTarget, TData> >& GetVariantMapItems();

        void EnableEvaluations() { m_isEvaluationEnabled = true; };
        void DisableEvaluations() { m_isEvaluationEnabled = false; };

        void SetQualifierContext(std::shared_ptr<QualifierContext> const& qualifierContext);

    private:
        std::vector<VariantMapItem<TTarget, TData> > m_qualifiedObjects;
        TTarget m_selectedItem{};
        std::shared_ptr<QualifierContext> m_pQualifierContext;
        bool m_dirty;
        bool m_isSelectionValid;
        bool m_isInitialEvaluation;
        bool m_isEvaluationEnabled;
        std::vector<IVariantMapChangedCallback*> m_listeners;

        std::function<HRESULT(TTarget&, TTarget&, bool&)> m_fOnSelectionChanged;
};

// StateTriggerVariantMap version of VariantMap used by the trigger engine
class VisualStateToken;
using StateTriggerVariantMap = VariantMap<VisualStateToken, xref_ptr<CDependencyObject>>;

template <class TTarget, class TData>
VariantMap<TTarget, TData>::VariantMap() :
    m_pQualifierContext(nullptr),
    m_dirty(false),
    m_isSelectionValid(false),
    m_isInitialEvaluation(false),
    m_qualifierFlags(QualifierFlags::None),
    m_isEvaluationEnabled(true)
{
}

template <class TTarget, class TData>
VariantMap<TTarget, TData>::~VariantMap()
{
    // Unregister from QualifierContext
    if (m_pQualifierContext)
    {
        m_pQualifierContext->RegisterChangedCallback(this, QualifierFlags::None);
    }
}

template <class TTarget, class TData>
_Check_return_ HRESULT VariantMap<TTarget, TData>::Add(_In_ TTarget target, _In_ std::shared_ptr<IQualifier> qualifier)
{
    return Add(target, qualifier, TData());
};

template <class TTarget, class TData>
_Check_return_ HRESULT VariantMap<TTarget, TData>::Add(_In_ TTarget target, _In_ std::shared_ptr<IQualifier> qualifier, TData pData)
{
    // Adds an item to the VariantMap and sets dirty flag
    // If flag is dirty when 'SelectedItem' is requested, VariantMap will re-evaluate.
    m_qualifiedObjects.push_back(VariantMapItem<TTarget, TData>(target, qualifier, pData));
    m_dirty = true;

    // If this VariantItem requires notification for context changes
    // that this VariantMap has not registered for, re-register
    // for new context changes.
    if(m_pQualifierContext && ((m_qualifierFlags & qualifier->Flags()) != qualifier->Flags()))
    {
        m_qualifierFlags = static_cast<QualifierFlags>(m_qualifierFlags | qualifier->Flags());
        m_pQualifierContext->RegisterChangedCallback(this, m_qualifierFlags);
    }

    return S_OK; // RRETURN_REMOVAL
};

template <class TTarget, class TData>
_Check_return_ HRESULT VariantMap<TTarget, TData>::Remove(_In_ TTarget target, _In_ std::shared_ptr<IQualifier> qualifier)
{
    m_dirty = true;

    // Comparator for qualifier
    auto comparator = [&](VariantMapItem<TTarget, TData>& i)
    {
        return ((i.m_target == target) &&
                VariantMap<TTarget, TData>::CompareQualifiers(i.m_pQualifier, qualifier));
    };

    auto iterVariantMapItem = std::find_if(begin(m_qualifiedObjects), end(m_qualifiedObjects), comparator);
    ASSERT(iterVariantMapItem != end(m_qualifiedObjects));

    // Remove existing item
    m_qualifiedObjects.erase(iterVariantMapItem);

    return S_OK; // RRETURN_REMOVAL
};

template <class TTarget, class TData>
_Check_return_ HRESULT VariantMap<TTarget, TData>::Replace(_In_ TTarget target, _In_ std::shared_ptr<IQualifier> existingQualifier, _In_ std::shared_ptr<IQualifier> newQualifier)
{
    m_dirty = true;

    // Remove existing item
    IFC_RETURN(Remove(target, existingQualifier));

    // Add new item
    IFC_RETURN(Add(target, newQualifier));

    return S_OK;
};

// Replace a qualifier identified by a unique TData instance
template <class TTarget, class TData>
_Check_return_ HRESULT VariantMap<TTarget, TData>::Replace(_In_ TData data, _In_ std::shared_ptr<IQualifier> existingQualifier, _In_ std::shared_ptr<IQualifier> newQualifier)
{
    m_dirty = true;

    // Comparator for qualifier
    auto comparator = [&](VariantMapItem<TTarget, TData>& i)
    {
        return ((i.m_pData == data) &&
                VariantMap<TTarget, TData>::CompareQualifiers(i.m_pQualifier, existingQualifier));
    };

    auto iterVariantMapItem = std::find_if(begin(m_qualifiedObjects), end(m_qualifiedObjects), comparator);
    ASSERT(iterVariantMapItem != end(m_qualifiedObjects));

    // Remove existing item
    auto target = iterVariantMapItem->m_target;
    m_qualifiedObjects.erase(iterVariantMapItem);

    // Add new item
    return Add(target, newQualifier, data);
};

// Clears all VariantMapItems for a specific target
template <class TTarget, class TData>
_Check_return_ HRESULT VariantMap<TTarget, TData>::Clear(_In_ TTarget target)
{
    auto isTarget = [&](VariantMapItem<TTarget, TData>& a) { return a.m_target == target; };
    auto it = /*[nodiscard] forcing to assign return value*/
        remove_if(begin(m_qualifiedObjects), end(m_qualifiedObjects), isTarget); 
    return S_OK; // RRETURN_REMOVAL
};

template <class TTarget, class TData>
bool VariantMap<TTarget, TData>::CompareQualifiers(_In_ std::shared_ptr<IQualifier> q1, _In_ std::shared_ptr<IQualifier> q2)
{
    // For a qualifier to be equal to another qualifier, all context dimensions must match
    return ((q1->Score(QualifierFlags::Identifier)  == q2->Score(QualifierFlags::Identifier)) &&
            (q1->Score(QualifierFlags::Height)      == q2->Score(QualifierFlags::Height)) &&
            (q1->Score(QualifierFlags::Width)       == q2->Score(QualifierFlags::Width)));
};

template <class TTarget, class TData>
TTarget* VariantMap<TTarget, TData>::SelectedItem()
{
    // If VariantMap has changed, reevaluate to ensure SelectedItem is up to date
    if(m_dirty)
    {
        IFCFAILFAST(Evaluate());
        m_dirty = false;
    }

    if(m_isSelectionValid)
    {
        return &m_selectedItem;
    }
    else
    {
        return nullptr;
    }
};

// Chooses the best item that meets qualifying all its qualifying conditions
// based on the current context (window width/height, platform, etc.)
template <class TTarget, class TData>
_Check_return_ HRESULT VariantMap<TTarget, TData>::Evaluate()
{
    if(!m_isEvaluationEnabled) return S_OK;

    // Determine which items meet their qualifying conditions
    std::for_each(begin(m_qualifiedObjects), end(m_qualifiedObjects), [&](VariantMapItem<TTarget, TData>& i) { i.m_pQualifier->Evaluate(m_pQualifierContext.get()); });

    // ----------------------------------------------------------
    //  Precedence order for qualifiers:
    //
    //      Extensible
    //      WindowWidth
    //      WindowHeight
    //
    // ----------------------------------------------------------

    // Sort items by precedence
    auto comparator = [&](VariantMapItem<TTarget, TData> a, VariantMapItem<TTarget, TData> b)
        {
            if(a.m_pQualifier->Score(QualifierFlags::Extensible) != b.m_pQualifier->Score(QualifierFlags::Extensible))
            {
                return a.m_pQualifier->Score(QualifierFlags::Extensible) > b.m_pQualifier->Score(QualifierFlags::Extensible);
            }
            else if(a.m_pQualifier->Score(QualifierFlags::Width) != b.m_pQualifier->Score(QualifierFlags::Width))
            {
                return a.m_pQualifier->Score(QualifierFlags::Width) > b.m_pQualifier->Score(QualifierFlags::Width);
            }
            else if(a.m_pQualifier->Score(QualifierFlags::Height) != b.m_pQualifier->Score(QualifierFlags::Height))
            {
                return a.m_pQualifier->Score(QualifierFlags::Height) > b.m_pQualifier->Score(QualifierFlags::Height);
            }

            return false;
        };

    std::sort(begin(m_qualifiedObjects), end(m_qualifiedObjects), comparator);

    // Evaluate all items
    // Choose the highest ranked item that meets all its qualifying conditions
    auto isQualified = [](VariantMapItem<TTarget, TData>& a) { return a.m_pQualifier->IsQualified(); };
    auto candidate = find_if(begin(m_qualifiedObjects), end(m_qualifiedObjects), isQualified);

    if(candidate == end(m_qualifiedObjects))
    {
        TTarget defaultValue = TTarget();
        m_isSelectionValid = false;

        if(!(m_selectedItem == defaultValue))
        {
            auto previousSelectedItem = m_selectedItem;
            m_selectedItem = defaultValue;

            for(auto& l : m_listeners)
            {
                IFC_RETURN(l->OnVariantMapChanged());
            }

            // Call OnSelectionChanged
            if(m_fOnSelectionChanged)
            {
                m_fOnSelectionChanged(m_selectedItem, previousSelectedItem, m_isInitialEvaluation);
            }
        }
    }
    else
    {
        m_isSelectionValid = true;

        // If selected item has changed, save it and notify listeners
        if(!(m_selectedItem == candidate->m_target))
        {
            auto previousSelectedItem = m_selectedItem;
            m_selectedItem = candidate->m_target;

            for(auto& l : m_listeners)
            {
                IFC_RETURN(l->OnVariantMapChanged());
            }

            // Call OnSelectionChanged
            if(m_fOnSelectionChanged)
            {
                m_fOnSelectionChanged(candidate->m_target, previousSelectedItem, m_isInitialEvaluation);
            }
        }
    }

    // Update m_selectedItem and dirty flag
    m_dirty = false;

    // Initial Evaluation
    m_isInitialEvaluation = false;

    return S_OK;
}

template <class TTarget, class TData>
_Check_return_ HRESULT VariantMap<TTarget, TData>::OnQualifierContextChanged()
{
    return Evaluate();
};

template <class TTarget, class TData>
std::vector<VariantMapItem<TTarget, TData> >& VariantMap<TTarget, TData>::GetVariantMapItems()
{
    return m_qualifiedObjects;
};

// Callbacks

template <class TTarget, class TData>
_Check_return_ HRESULT VariantMap<TTarget, TData>::RegisterForChangeCallback(_In_ IVariantMapChangedCallback* listener)
{
    m_listeners.push_back(listener);

    return S_OK; // RRETURN_REMOVAL
};

template <class TTarget, class TData>
_Check_return_ HRESULT VariantMap<TTarget, TData>::UnRegisterForChangeCallback(_In_ IVariantMapChangedCallback* listener)
{
    auto iterListener = find(begin(m_listeners), end(m_listeners), listener);
    ASSERT(iterListener != end(m_listeners));

    m_listeners.erase(iterListener);

    return S_OK; // RRETURN_REMOVAL
};

template <class TTarget, class TData>
void VariantMap<TTarget, TData>::SetOnSelectionChanged(std::function<HRESULT(TTarget, TTarget, bool)> func)
{
    m_fOnSelectionChanged = func;
};

template <class TTarget, class TData>
void VariantMap<TTarget, TData>::SetQualifierContext(std::shared_ptr<QualifierContext> const& qualifierContext)
{
    if (m_pQualifierContext.get() == qualifierContext.get())
    {
        return;
    }

    // We'll unregister from the old context and register with the new context
    // if we're swapping in a new context.  Note that RegisterChangedCallback
    // has logic that unregisters if we pass in QualifierFlags::None.
    if (m_pQualifierContext)
    {
        m_pQualifierContext->RegisterChangedCallback(this, QualifierFlags::None);
    }

    m_pQualifierContext = qualifierContext;

    if (m_pQualifierContext)
    {
        // If we don't have an old context, then we'll register a callback with the existing flags.
        m_pQualifierContext->RegisterChangedCallback(this, m_qualifierFlags);
    }
};