// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <DOCollection.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>

class CUIElementWeakCollection final : public CCollection
{
private:
    explicit CUIElementWeakCollection(_In_ CCoreServices* core) : CCollection(core) { }

public:
    ~CUIElementWeakCollection() override = default;

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject** object,
        _In_ CREATEPARAMETERS* create);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CUIElementWeakCollection>::Index;
    }

    bool ContainsNoRefItems() const override { return true; }

    XUINT32 GetCount() const override;
    _Check_return_ HRESULT Append(_In_ CValue& value, XUINT32* index) override;
    _Check_return_ HRESULT Insert(XUINT32 index, _In_ CValue& value) override;
    void* RemoveAt(XUINT32 index) override;
    void* GetItemWithAddRef(XUINT32 index) override;
    _Check_return_ HRESULT Neat(bool processLeave) override;
    _Check_return_ HRESULT MoveInternal(XINT32 index, XINT32 position) override;

    const xref::weakref_ptr<CUIElement>& GetElementAt(XUINT32 index) const
    {
        return m_items[index];
    }

    // Note: Collection has a virtual SetChangeCallback, but that uses std::weak_ptr, which requires std::shared_ptr, which
    // does its own reference counting outside of DOs that have their own AddRef/Release. It's hard to get a std::weak_ptr
    // to a DO, so we use a no ref pointer instead. The DO is responsible for calling back here with nullptr in its dtor.
    void SetCollectionChangeCallback(ICollectionChangeCallback* callback)
    {
        m_changeCallbackNoRef = callback;
    }

    void SetChangeCallback(const std::weak_ptr<ICollectionChangeCallback>& callback) override
    {
        // Call SetCollectionChangeCallback instead.
        IFCFAILFAST(E_NOTIMPL);
    }

    void DisallowAncestorsInCollection()
    {
        // This must be set before any elements are added to the collection.
        FAIL_FAST_ASSERT(m_items.size() == 0);
        m_disallowAncestorsInCollection = true;
    }

    // Make sure there are no ancestors of the owner in this collection. Returns E_INVALIDARG if one is found.
    _Check_return_ HRESULT CheckForAncestorElements(_In_opt_ CUIElement* newParent) const;

    std::vector<xref::weakref_ptr<CUIElement>>::const_iterator begin() const
    {
        return m_items.begin();
    }

    std::vector<xref::weakref_ptr<CUIElement>>::const_iterator end() const
    {
        return m_items.end();
    }

private:
    _Check_return_ HRESULT ValidateItem(const CValue& newItem);
    _Check_return_ HRESULT CheckForAncestor(const CUIElement* element, _In_opt_ CUIElement* newParent) const;

    ICollectionChangeCallback* m_changeCallbackNoRef { nullptr };
    std::vector<xref::weakref_ptr<CUIElement>> m_items;

    // ThemeShadows use a UIElementWeakCollection to specify its Receivers. It doesn't allow ancestor elements in the
    // receiver collection, which it marks using this flag.
    bool m_disallowAncestorsInCollection {false};
};

