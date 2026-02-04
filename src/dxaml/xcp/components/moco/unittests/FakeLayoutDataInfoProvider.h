// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls {

class FakeLayoutDataInfoProvider
    : public wrl::RuntimeClass<xaml_controls::ILayoutDataInfoProvider>
{
    InspectableClass(InterfaceName_Microsoft_UI_Xaml_Controls_ILayoutDataInfoProvider, TrustLevel::BaseTrust);

public:
    FakeLayoutDataInfoProvider()
        : _totalItemsCount(-1)
        , _totalGroupCount(-1)
    { }

    #pragma region SetData
    
    void SetData(int totalItemsCount)
    {
        _totalItemsCount = totalItemsCount;
        _totalGroupCount = -1;
        _groups.clear();
    }

    void SetData(std::vector<int> groupSizes)
    {
        int groupIndex = 0;
        int totalItemsCount = 0;
        _groups.clear();

        for (int size : groupSizes)
        {
            GroupInfo group = {};
            group.startItemIndex = totalItemsCount;
            group.endItemIndex = totalItemsCount + size;
            group.indexOfGroup = groupIndex;

            _groups.push_back(group);

            ++groupIndex;
            totalItemsCount += size;
        }

        _totalGroupCount = static_cast<int>(groupSizes.size());
        _totalItemsCount = totalItemsCount;
    }

    #pragma endregion

    #pragma region ILayoutDataInfoProvider methods.
    
    IFACEMETHOD(GetTotalItemCount)(INT* pReturnValue) override
    {
        *pReturnValue = _totalItemsCount;
        return S_OK;
    }

    IFACEMETHOD(GetTotalGroupCount)(INT* pReturnValue) override
    {
        ASSERT(IsGrouped());
        *pReturnValue = _totalGroupCount;
        return S_OK;
    }

    IFACEMETHOD(GetGroupInformationFromItemIndex)(
        INT itemIndex, 
        INT* pIndexOfGroup,
        INT* pIndexInsideGroup,
        INT* pItemCountInGroup) override
    {
        int indexOfGroup = -1;
        int indexInsideGroup = -1;
        int itemCountInGroup = -1;
        const int totalItemsCount = _totalItemsCount;

        if (IsGrouped() && 0 <= itemIndex && itemIndex < totalItemsCount)
        {
            auto foundGroup = std::upper_bound(begin(_groups), end(_groups), itemIndex,
                [](int workingIndex, const GroupInfo& current) -> bool
            {
                return (workingIndex < current.endItemIndex);
            });

            ASSERT(foundGroup != _groups.end());

            itemCountInGroup = foundGroup->endItemIndex - foundGroup->startItemIndex;
            indexInsideGroup = itemIndex - foundGroup->startItemIndex;
            indexOfGroup = foundGroup->indexOfGroup;
        }

        if (pIndexOfGroup)
        {
            *pIndexOfGroup = indexOfGroup;
        }
        if (pIndexInsideGroup)
        {
            *pIndexInsideGroup = indexInsideGroup;
        }
        if (pItemCountInGroup)
        {
            *pItemCountInGroup = itemCountInGroup;
        }

        return S_OK;
    }

    IFACEMETHOD(GetGroupInformationFromGroupIndex)(
        INT groupIndex, 
        INT* pStartItemIndex, 
        INT* pItemCountInGroup) override
    {
        ASSERT(IsGrouped() && 0 <= groupIndex && groupIndex < static_cast<int>(_groups.size()));

        int startItemIndex = -1;
        int itemCountInGroup = -1;

        const GroupInfo& group = _groups[groupIndex];
        startItemIndex = group.startItemIndex;
        itemCountInGroup = group.endItemIndex - startItemIndex;

        if (pStartItemIndex)
        {
            *pStartItemIndex = startItemIndex;
        }
        if (pItemCountInGroup)
        {
            *pItemCountInGroup = itemCountInGroup;
        }

        return S_OK;
    }

    #pragma endregion

private:
    bool IsGrouped() const { return _totalGroupCount != -1; }
        
    int _totalItemsCount;
    int _totalGroupCount;

    struct GroupInfo
    {
        int startItemIndex;
        int endItemIndex;
        int indexOfGroup;
    };

    std::vector<GroupInfo> _groups;
};

} } } } }