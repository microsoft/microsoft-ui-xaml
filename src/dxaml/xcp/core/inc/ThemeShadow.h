// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <FeatureFlags.h>
#include "Shadow.h"
#include "UIElementWeakCollection.h"
#include "ICollectionChangeCallback.h"

class CThemeShadow final
    : public CShadow
    , public ICollectionChangeCallback
{
private:
    CThemeShadow(_In_ CCoreServices *pCore);
    ~CThemeShadow() override;

    void SetDirtyFlagOnPopupChild(_In_ CDependencyObject* element);

protected:
    CThemeShadow(_In_ const CThemeShadow& original, _Out_ HRESULT& hr);

public:
// Creation method
    DECLARE_CREATE(CThemeShadow);

// CNoParentShareableDependencyObject overrides
    DECLARE_SHAREABLEDEPENDENCYOBJECT_CLONE(CThemeShadow);

// CMultiParentShareableDependencyObject overrides
    void NWPropagateDirtyFlag(DirtyFlags flags) override;

// CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CThemeShadow>::Index;
    }

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    CUIElementWeakCollection* GetReceiversNoRef();

    bool HasCustomReceivers();

    // Checks whether there are any ancestors in the Receivers collection.
    // Optionally accepts the an element that will be using this CThemeShadow. It won't be in the owner collection,
    // so we have to check it explicitly. The alternative is to check after setting the Shadow property, but then we
    // would have to unset it if it's invalid.
    _Check_return_ HRESULT CheckForAncestorReceivers(_In_opt_ CUIElement* newParent);

    _Check_return_ HRESULT ElementInserted(UINT32 indexInChildrenCollection) override;
    _Check_return_ HRESULT ElementRemoved(UINT32 indexInChildrenCollection) override;
    _Check_return_ HRESULT ElementMoved(UINT32 oldIndexInChildrenCollection, UINT32 newIndexInChildrenCollection) override;
    _Check_return_ HRESULT CollectionCleared() override;

    _Check_return_ HRESULT GetMask(_Outptr_result_maybenull_ WUComp::ICompositionBrush** maskBrush);
    void SetMask(_In_opt_ WUComp::ICompositionBrush* maskBrush);
    bool HasMaskBrush();

    static bool IsDropShadowMode();
    enum class DropShadowDepthClass
    {
        Small,
        Medium,
        Large
    };
    static XTHICKNESS GetInsetsForWindowedPopup(DropShadowDepthClass depthClass);

public:
    wrl::ComPtr<WUComp::ICompositionBrush> m_maskBrush;
};
