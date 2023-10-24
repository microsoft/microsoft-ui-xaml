// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// FlyoutBase is DependencyObject and x:Name registration for
// framework objects is performed on UIElement level, flyout needs to
// have an object in core which will override EnterImpl method to register
// its children names.
// Alternative approach of moving the registration call to CDependencyObject
// was rejected because of its negative effect on performance.

class CFlyoutBase : public CDependencyObject
{
public:
    enum class MajorPlacementMode
    {
        Top,
        Bottom,
        Left,
        Right,
        Full
    };

    DECLARE_CREATE(CFlyoutBase);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CFlyoutBase>::Index;
    }

    xref_ptr<CXamlIslandRoot> GetIslandContext();

    _Check_return_ HRESULT OnPlacementUpdated(_In_ const MajorPlacementMode majorPlacementMode);
    void AddPlacementUpdatedSubscriber(_In_ CDependencyObject* pTarget,
        const std::function<HRESULT(CDependencyObject*, MajorPlacementMode)>& callbackFunc);
    _Check_return_ HRESULT RemovePlacementUpdatedSubscriber(_In_ CDependencyObject* pTarget);

protected:
    CFlyoutBase(_In_ CCoreServices *pCore);

    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params) override;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) override;

private:
    struct OnPlacementUpdatedSubscriber
    {
        OnPlacementUpdatedSubscriber(xref::weakref_ptr<CDependencyObject> ipTarget,
            const std::function<HRESULT(CDependencyObject*, MajorPlacementMode)>& icallbackFunc):pTarget(ipTarget), callbackFunc(icallbackFunc)
        {
        }

        xref::weakref_ptr<CDependencyObject> pTarget;
        std::function<HRESULT(CDependencyObject*, MajorPlacementMode)> callbackFunc;
    };
    std::vector<OnPlacementUpdatedSubscriber> m_placementUpdatedSubscribers;
};

