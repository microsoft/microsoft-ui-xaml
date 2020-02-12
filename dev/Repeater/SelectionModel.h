// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SelectionModel.g.h"
#include "SelectionModel.properties.h"

struct SelectedItemInfo
{
    std::weak_ptr<SelectionNode> Node;
    winrt::IndexPath Path;
};

class SelectionModel :
    public ReferenceTracker<SelectionModel, winrt::implementation::SelectionModelT, winrt::Windows::UI::Xaml::Data::ICustomPropertyProvider, winrt::composing>,
    public SelectionModelProperties
{
public:
    SelectionModel();
    ~SelectionModel();

#pragma region ISelectionModel
    winrt::IInspectable Source();
    void Source(winrt::IInspectable const& value);

    bool SingleSelect();
    void SingleSelect(bool value); 

    winrt::IndexPath AnchorIndex();
    void AnchorIndex(winrt::IndexPath const& value);

    winrt::IndexPath SelectedIndex();
    void SelectedIndex(winrt::IndexPath const& value);

    winrt::IInspectable SelectedItem();
    winrt::IVectorView<winrt::IInspectable> SelectedItems();
    winrt::IVectorView<winrt::IndexPath> SelectedIndices();

    void SetAnchorIndex(int32_t index);
    void SetAnchorIndex(int groupIndex, int itemIndex);

    void Select(int index);
    void Select(int groupIndex, int itemIndex);
    void SelectAt(winrt::IndexPath const& index);

    void Deselect(int index);
    void Deselect(int groupIndex, int itemIndex);
    void DeselectAt(winrt::IndexPath const& index);

    winrt::IReference<bool> IsSelected(int index);
    winrt::IReference<bool> IsSelected(int groupIndex, int itemIndex);
    winrt::IReference<bool> IsSelectedAt(winrt::IndexPath const& index);

    void SelectRangeFromAnchor(int32_t index);
    void SelectRangeFromAnchor(int groupIndex, int itemIndex);
    void SelectRangeFromAnchorTo(winrt::IndexPath const& index);

    void DeselectRangeFromAnchor(int32_t index);
    void DeselectRangeFromAnchor(int groupIndex, int itemIndex);
    void DeselectRangeFromAnchorTo(winrt::IndexPath const& index);

    void SelectRange(winrt::IndexPath const& start, winrt::IndexPath const& end);
    void DeselectRange(winrt::IndexPath const& start, winrt::IndexPath const& end);

    void SelectAll();
    void ClearSelection();

#pragma endregion

#pragma region ICustomPropertyProvider
    winrt::TypeName Type();
    winrt::ICustomProperty GetCustomProperty(hstring const& name);
    winrt::ICustomProperty GetIndexedProperty(hstring const& name, winrt::TypeName const& type);
    hstring GetStringRepresentation();
#pragma endregion

#pragma region INotifyPropertyChanged
    winrt::event_token PropertyChanged(winrt::PropertyChangedEventHandler const& value);
    void PropertyChanged(winrt::event_token const& token);
#pragma endregion

#pragma region ISelectionModelProtected
    void OnPropertyChanged(winrt::hstring const& propertyName);
#pragma endregion

    winrt::IInspectable ResolvePath(const winrt::IInspectable& data, const winrt::IndexPath& dataIndexPath);
    void OnSelectionInvalidatedDueToCollectionChange();
    std::shared_ptr<SelectionNode> SharedLeafNode() { return m_leafNode; }

private:
    void RaisePropertyChanged(std::wstring_view const& name);
    void ClearSelection(bool resetAnchor, bool raiseSelectionChanged);
    void OnSelectionChanged();

    void SelectImpl(int index, bool select);
    void SelectWithGroupImpl(int groupIndex, int itemIndex, bool select);
    void SelectWithPathImpl(const winrt::IndexPath& index, bool select, bool raiseSelectionChanged);
    void SelectRangeFromAnchorImpl(int index, bool select);
    void SelectRangeFromAnchorWithGroupImpl(int groupIndex, int itemIndex, bool select);
    void SelectRangeImpl(const winrt::IndexPath& start, const winrt::IndexPath& end, bool select);

    std::shared_ptr<SelectionNode> m_rootNode{ nullptr };
    bool m_singleSelect{ false };

    winrt::IVectorView<winrt::IndexPath> m_selectedIndicesCached{ nullptr };
    winrt::IVectorView<winrt::IInspectable> m_selectedItemsCached{ nullptr };

    event_source<winrt::PropertyChangedEventHandler> m_propertyChangedEventSource{ this };

    // Cached Event args to avoid creation cost every time
    tracker_ref<winrt::SelectionModelChildrenRequestedEventArgs> m_childrenRequestedEventArgs{ this };
    tracker_ref<winrt::SelectionModelSelectionChangedEventArgs> m_selectionChangedEventArgs{ this };

    // use just one instance of a leaf node to avoid creating a bunch of these.
    std::shared_ptr<SelectionNode> m_leafNode;
};
