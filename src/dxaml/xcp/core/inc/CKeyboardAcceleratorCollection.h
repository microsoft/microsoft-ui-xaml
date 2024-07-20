// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CCoreServices;

class CKeyboardAcceleratorCollection final : public CDOCollection
{
private:
    CKeyboardAcceleratorCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
    { }

public:
// Creation method

    DECLARE_CREATE(CKeyboardAcceleratorCollection);

// CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const final
    {
        return DependencyObjectTraits<CKeyboardAcceleratorCollection>::Index;
    }

    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params) override
    {

        IFC_RETURN(__super::EnterImpl(pNamescopeOwner, params));

        if (params.fIsLive || params.fIsForKeyboardAccelerator)
        {
            CContentRoot *pContentRoot = VisualTree::GetContentRootForElement(this);
            if (nullptr != pContentRoot)
            {
                pContentRoot->AddToLiveKeyboardAccelerators(this);
            }
        }
        return S_OK;
    }

    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) override
    {
        IFC_RETURN(__super::LeaveImpl(pNamescopeOwner, params));
        if (params.fIsLive || params.fIsForKeyboardAccelerator)
        {
            CContentRoot *pContentRoot = VisualTree::GetContentRootForElement(this);
            if(nullptr != pContentRoot)
            {
                pContentRoot->RemoveFromLiveKeyboardAccelerators(this);
            }
        }
        return S_OK;
    }

    ~CKeyboardAcceleratorCollection() override
    { }
};
