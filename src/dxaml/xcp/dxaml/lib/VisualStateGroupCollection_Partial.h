// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <VisualStateGroupCollection.g.h>
#include <IVisualStateGroupCollectionTestHooks.h>

namespace DirectUI
{
    PARTIAL_CLASS(VisualStateGroupCollection)
        , IVisualStateGroupCollectionTestHooks
    {
    public:
#pragma region IVisualStateGroupCollectionTestHooks
        std::shared_ptr<std::vector<std::wstring>> GetVisualStateGroupNames() override;
        std::shared_ptr<std::vector<std::wstring>> GetVisualStateNamesForGroup(_In_ unsigned int groupIdx) override;
        bool DoesVisualStateGroupHaveTransitions(_In_ unsigned int groupIdx) const override;
        Microsoft::WRL::ComPtr<IInspectable> CreateStoryboard(_In_ unsigned int groupIdx, _In_ unsigned int storyboardIdx) override;
#pragma endregion

        CVisualStateGroupCollection* GetHandle() const;

    protected:
        _Check_return_  HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void **ppvObject) override;
    };
}