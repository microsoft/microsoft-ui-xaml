// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class VisualStateGroupCollectionCustomRuntimeData;

DECLARE_INTERFACE_IID_(IVisualStateGroupCollectionTestHooks, IUnknown, "2911a2b6-ba62-41cc-8b1c-3f20011c050b")
{
    // Because we use a custom allocator in debug builds we can't
    // simply return a std::vector and hope that the test code knows how to
    // destruct it properly. It will call HeapFree on a pointer to a block
    // that has additional header/tail added to it to enable the leak detector.
    // To get around this we return a shared_ptr instead, which virtualizes the
    // destructor and allows our deallocator to run over the array and the wstrings
    // contained within it.
    virtual std::shared_ptr<std::vector<std::wstring>> GetVisualStateGroupNames() = 0;
    virtual std::shared_ptr<std::vector<std::wstring>> GetVisualStateNamesForGroup(_In_ unsigned int groupIdx) = 0;
    virtual bool DoesVisualStateGroupHaveTransitions(_In_ unsigned int groupIdx) const = 0;
    virtual Microsoft::WRL::ComPtr<IInspectable> CreateStoryboard(_In_ unsigned int groupIdx, _In_ unsigned int storyboardIdx) = 0;
    
};

