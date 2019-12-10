// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SplitDataSourceBase.h"
enum class NavigationViewSplitVectorID
{
    NotInitialized = 0,
    PrimaryList = 1,
    OverflowList = 2,
    SkippedList = 3,
    Size = 4
};

using SplitDataSourceT = typename SplitDataSourceBase<winrt::IInspectable, NavigationViewSplitVectorID, float>;
using SplitVectorT = typename SplitVector<winrt::IInspectable, NavigationViewSplitVectorID>;

class TopNavigationViewDataProvider: public SplitDataSourceT
{
public:
    
    TopNavigationViewDataProvider(const ITrackerHandleManager* m_owner);

    winrt::IVector<winrt::IInspectable> GetPrimaryItems();
    winrt::IVector<winrt::IInspectable> GetOverflowItems();

    // The raw data is from MenuItems or MenuItemsSource
    void SetDataSource(const winrt::IInspectable& rawData);
    bool ShouldChangeDataSource(winrt::IInspectable const& rawData);

    void OnRawDataChanged(std::function<void(winrt::NotifyCollectionChangedEventArgs const& args)> const& dataChangeCallback);

    // override SplitDataSourceBase
    int IndexOf(const winrt::IInspectable& value) override;
    winrt::IInspectable GetAt(int index) override;
    int Size() override;
    NavigationViewSplitVectorID DefaultVectorIDOnInsert() override;
    float DefaultAttachedData() override;

    void MoveAllItemsToPrimaryList();
    std::vector<int> ConvertPrimaryIndexToIndex(std::vector<int> const& indexesInPrimary);
    int ConvertOriginalIndexToIndex(int originalIndex);
    void MoveItemsOutOfPrimaryList(std::vector<int> const& indexes);
    void MoveItemsToPrimaryList(std::vector<int> const& indexes);
    void MoveItemsToList(std::vector<int> const& indexes, NavigationViewSplitVectorID vectorID);

    int IndexOf(const winrt::IInspectable& value, NavigationViewSplitVectorID vectorID);

    int GetPrimaryListSize();
    int GetNavigationViewItemCountInPrimaryList();
    int GetNavigationViewItemCountInTopNav();

    void UpdateWidthForPrimaryItem(int indexInPrimary, float width);
    float WidthRequiredToRecoveryAllItemsToPrimary();
    float CalculateWidthForItems(std::vector<int> &items);
    float GetWidthForItem(int index);
    void InvalidWidthCache();
    float OverflowButtonWidth();
    void OverflowButtonWidth(float width);
    bool IsItemInPrimaryList(int index);
    bool HasInvalidWidth(std::vector<int> & items);
    bool IsValidWidthForItem(int index);

    // If value is not in the raw data set or can't be move to primarylist, then return false
    bool IsItemSelectableInPrimaryList(const winrt::IInspectable& value);
protected:
    void OnDataSourceChanged(const winrt::IInspectable& sender, const winrt::NotifyCollectionChangedEventArgs& args);

private:
    bool IsValidWidth(float width);
    void SetWidthForItem(int index, float width);
    void ChangeDataSource(winrt::ItemsSourceView dataSource);
    bool IsContainerNavigationViewItem(int index);
    bool IsContainerNavigationViewHeader(int index);

    tracker_ref<winrt::ItemsSourceView> m_dataSource;
    // If the raw datasource is the same, we don't need to create new winrt::ItemsSourceView object.
    tracker_ref<winrt::IInspectable> m_rawDataSource;
    // Event tokens
    winrt::event_token m_dataSourceChanged{};
    std::function<void(const winrt::NotifyCollectionChangedEventArgs& args)> m_dataChangeCallback;
    float m_overflowButtonCachedWidth{};
};

