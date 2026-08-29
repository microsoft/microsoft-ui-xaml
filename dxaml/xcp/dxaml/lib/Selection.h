// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      A transaction-based selection tracker.

#pragma once

namespace DirectUI
{
    // Class for transaction-based selection changes
    class SelectionChanger
    {
    public:
        // End selection change.
        // This SelectionChanger instance should not be used again after this method completes.
        _Check_return_ virtual HRESULT End(
            _In_ wfc::IVector<IInspectable*>* pUnselectedItems,
            _In_ wfc::IVector<IInspectable*>* pSelectedItems,
            _In_ BOOLEAN canSelectMultiple) = 0;

        // Select the given item.
        _Check_return_ virtual HRESULT Select(
            _In_ int index,
            _In_ IInspectable* pItem,
            _In_ BOOLEAN canSelectMultiple) = 0;

        // Unselect a specific item.
        _Check_return_ virtual HRESULT Unselect(
            _In_ UINT index,
            _In_ IInspectable* pItem) = 0;

        // Unselect all selected items.
        _Check_return_ virtual HRESULT UnselectAll() = 0;

        // Cancels all changes and invalidates this SelectionChanger.
        // This SelectionChanger instance should not be used again after this method completes.
        _Check_return_ virtual HRESULT Cancel() = 0;

    protected:
        virtual ~SelectionChanger() { }
    };

    // Callback, used to notify interested parties of indexes being selected or unselected.
    // For example, used by Selector to notify SelectorItems of their Selected state changing.
    class SelectionChangeApplier
    {
    public:
        _Check_return_ virtual HRESULT SelectIndex(
            _In_ UINT index) = 0;

        _Check_return_ virtual HRESULT UnselectIndex(
            _In_ UINT index) = 0;

    protected:
        virtual ~SelectionChangeApplier() { }
    };

    class Selection
    {
    public:
        Selection();
        ~Selection();

        _Check_return_ HRESULT Initialize(_In_ SelectionChangeApplier* pChangeApplier);

        // Returns a copy of the list of selected indexes.
        std::vector<UINT> GetSelectedIndexes() const;

        // Obtains the index of the selected item at the given position in the selection.
        _Check_return_ HRESULT GetIndexAt(
            _In_ UINT position,
            _Out_ UINT& itemIndex);

        // Obtains the the selected item at the given position in the selection.
        _Check_return_ HRESULT GetAt(
            _In_ UINT position,
            _Outptr_ IInspectable** ppSelectedItem);

        // Obtains the number of selected items.
        _Check_return_ HRESULT GetNumItemsSelected(
            _Out_ UINT& size);

        _Check_return_ HRESULT AddSelectedIndex(
                _In_ UINT itemIndex);

        _Check_return_ HRESULT RemoveSelectedIndex(
                _In_ UINT itemIndex);

        _Check_return_ HRESULT Has(
                _In_ UINT itemIndex,
                _Out_ UINT& position,
                _Out_ BOOLEAN& hasItem);

        // Start selection change.
        _Check_return_ HRESULT BeginChange(
            _Outptr_ SelectionChanger** changer);

        // Whether or not we're in the middle of a selection change.
        BOOLEAN IsChangeActive();

    private:
        // A collection of selected items with their associated indexes in the items collection.
        // Conceptually, a List<Pair<IInspectable, UINT>>, where the UINT is the
        // index of the respective selected item. The Collection superclass is used for its ability
        // to properly handle IInspectable references.
        // Ideally, this would be replaced with a single list of IInspectable, UINT pairs for greater
        // performance.
        class InternalSelectedItemsStorage : public TrackerCollection<IInspectable*>
        {
        public:
            _Check_return_ HRESULT Add(
                _In_ UINT itemIndex,
                _In_ IInspectable* pItem);

            _Check_return_ HRESULT Has(
                _In_ UINT itemIndex,
                _Out_ UINT& position,
                _Out_ BOOLEAN& hasItem);

            _Check_return_ HRESULT GetIndexAt(
                _In_ UINT position,
                _Out_ UINT& itemIndex);

            _Check_return_ HRESULT Inserted(
                _In_ UINT itemIndex);

            _Check_return_ HRESULT Removed(
                 _In_ UINT itemIndex);

            // Returns a copy of the list of selected indexes.
            std::vector<UINT> GetSelectedIndexes() const;

            IFACEMETHODIMP SetAt(
                _In_ unsigned itemIndex,
                _In_ IInspectable* pItem) override
            {
                RRETURN(E_UNEXPECTED);
            }

            IFACEMETHODIMP InsertAt(
                _In_ unsigned itemIndex,
                _In_ IInspectable* pItem) override
            {
                RRETURN(E_UNEXPECTED);
            }

            IFACEMETHOD(RemoveAt)(_In_ unsigned position) override;

            IFACEMETHODIMP Append(
                _In_ IInspectable* pItem) override
            {
                RRETURN(E_UNEXPECTED);
            }

            IFACEMETHODIMP RemoveAtEnd() override
            {
                RRETURN(E_UNEXPECTED);
            }

            IFACEMETHOD(Clear)() override;

        private:
            std::vector<UINT> m_indexlist;
        };

        class SelectionChangerImpl final : public SelectionChanger
        {
        public:
            SelectionChangerImpl();
            ~SelectionChangerImpl() override;

            void BeginInternal();

            // End selection change.
            _Check_return_ HRESULT End(
                _In_ wfc::IVector<IInspectable*>* pUnselectedItems,
                _In_ wfc::IVector<IInspectable*>* pSelectedItems,
                _In_ BOOLEAN canSelectMultiple) override;

            // Select the given item.
            _Check_return_ HRESULT Select(
                _In_ int index,
                _In_ IInspectable* pItem,
                _In_ BOOLEAN canSelectMultiple) override;

            // Unselect a specific item.
            _Check_return_ HRESULT Unselect(
                _In_ UINT index,
                _In_ IInspectable* pItem) override;

            // Unselect all selected items.
            _Check_return_ HRESULT UnselectAll() override;

            // Cancels all changes and invalidates this SelectionChanger.
            // This SelectionChanger instance should not be used again after this method completes.
            _Check_return_ HRESULT Cancel() override;

            // We need to know if an item was removed, so we can correctly differentiate
            // removed items from items which simply ended up with the removed item's index.
            _Check_return_ HRESULT AccountForRemovedItem(_In_ UINT removedItemIndex);

            virtual BOOLEAN IsActive();

            // Prepares object's state
            _Check_return_ HRESULT Initialize(
                _In_ Selection* pOwner,
                _In_ SelectionChangeApplier* pChangeApplier);

        private:

            _Check_return_ HRESULT Cleanup();

            // Ensures that the selection change is valid for the current mode
            _Check_return_ HRESULT ApplyCanSelectMultiple(
                _In_ BOOLEAN canSelectMultiple);

            _Check_return_ HRESULT CreateDeltaSelectionChange(
                _In_ wfc::IVector<IInspectable*>* pUnselectedItems,
                _In_ wfc::IVector<IInspectable*>* pSelectedItems);

            // If m_unselectAllRequested is true, clear it and
            // unselect all items by adding them to m_pItemsToUnselect.
            // This is part of a perf optimization to rapidly handle
            // the UnselectAll case.
            _Check_return_ HRESULT UnrollUnselectAllRequest();

            // weak reference to owner Selection that this SelectionChanger belongs to.
            Selection* m_pOwnerNoRef;

            // List of items selected in current selection change
            ctl::ComPtr<InternalSelectedItemsStorage> m_spItemsToSelect;

            // List of items unselected in current selection change
            ctl::ComPtr<InternalSelectedItemsStorage> m_spItemsToUnselect;

            // Whether or not we're in the middle of a selection change
            BOOLEAN m_bIsActive;

            // Whether or not this transaction's only action is to clear
            // all the selected items. This is a perf optimization.
            BOOLEAN m_unselectAllRequested;

            // We need to know which items have been removed during this change, so when the change is applied
            // we can correctly differentiate removed items from items which simply ended up with the removed item's index.
            struct
            {
                UINT index;
                bool empty;
            } m_indexDeletedDuringThisChange;

            SelectionChangeApplier* m_pChangeApplier;
        };

        SelectionChangerImpl m_selectionChanger;

        // The current Selection.
        ctl::ComPtr<InternalSelectedItemsStorage> m_spSelectedItems;
    };
}
