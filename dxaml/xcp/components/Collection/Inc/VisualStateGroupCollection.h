// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DOCollection.h"
#include "DependencyObjectTraits.h"
#include "DependencyObjectTraits.g.h"

#include <deferral\inc\ICustomWriterRuntimeDataReceiver.h>
#include <vsm\inc\VisualStateGroupContext.h>
#include <vsm\inc\DeferredNameScopeEntry.h>
#include <qualifiers\inc\VariantMap.h>
#include <CustomWriterRuntimeContext.h>

class CustomWriterRuntimeData;
class CVisualTransitionCollection;
class VisualStateGroupCollectionCustomRuntimeData;

class CVisualStateGroupCollection final
    : public CDOCollection
    , public ICustomWriterRuntimeDataReceiver
{
public:
    DECLARE_CREATE(CVisualStateGroupCollection);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CVisualStateGroupCollection>::Index;
    }

#pragma region DOCollection Overrides
    // To support the fault-in VSM logic we override the key publically-accessible
    // DOCollection methods to ensure that if someone tries to access this collection we
    // fault in the right stuff.
    XUINT32 GetCount() const override;
    _Check_return_ HRESULT Insert(_In_ XUINT32 index, _In_ CDependencyObject* object) override;
    _Check_return_ HRESULT Append(_In_ CDependencyObject* object, _Out_opt_ unsigned int* index) override;
    _Check_return_ HRESULT OnClear() override;
    _Check_return_ void* GetItemWithAddRef(_In_ unsigned int index) override;
    _Check_return_ HRESULT NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh) override;
#pragma endregion

    // When this collection is removed from the visual tree we do all the cleanup work needed
    // to ensure VSM stays in a consistent state, including skipping Transitions to the end.
    _Check_return_ HRESULT
    LeaveImpl(_In_ CDependencyObject* namescopeOwner, _In_ LeaveParams params) override;

#pragma region Optimized Storage and Data Accessors
    _Check_return_ HRESULT SetCustomWriterRuntimeData(std::shared_ptr<CustomWriterRuntimeData> data,
        std::unique_ptr<CustomWriterRuntimeContext> context) override;

    VisualStateGroupCollectionCustomRuntimeData* GetCustomRuntimeData() const
    {
        return m_runtimeData.get();
    }

    CustomWriterRuntimeContext* GetCustomRuntimeContext() const
    {
        return m_runtimeContext.get();
    }

    bool IsOptimizedGroupCollection() const
    {
        // If this group has any children we know that it has had the VSM
        // faulted in.
        return m_runtimeData && !m_faultedInChildren;
    }

    bool GetFaultedInChildren() const
    {
        return m_faultedInChildren;
    }

    void MarkFaultedInChildren()
    {
        ASSERT(!m_faultedInChildren);
        m_faultedInChildren = true;
    }

    void MarkVsmActivity()
    {
        m_vsmActivitySinceLastLeave = true;
    }

    std::vector<VisualStateGroupContext>& GetGroupContext()
    {
        return m_groupContext;
    }

    _Check_return_ HRESULT EnsureFaultedIn() const;

#pragma endregion

    // Queries the ViewManagement.UISettings.AnimationsEnabled class. Disabling animations
    // in response to the user being in accessibility mode is a behavior that we define not
    // at the level of individual animations and storyboards, but at the VSM and LayoutTransition
    // layer. To that end, to a VSM, this property is evaluated in the context of the control
    // it is attached to, via the VSGC which always has a strong connection. Today's implementation
    // queries directly through the DXamlCore.
    bool AreAnimationsEnabled() const;

    // Locates the control that this VisualStateGroupCollection belongs to, if any. In Jupiter a
    // VisualStateGroupCollection MUST belong to a root element of either a UserControl or Control
    // for it to be functional.
    xref_ptr<CControl> GetOwningControl() const;

    ~CVisualStateGroupCollection() override;

    CDependencyObjectCollection* m_pActiveStoryboards;
    CVisualTransitionCollection* m_pActiveTransitions;
    CStateTriggerCollection* m_pDeferredStateTriggers;
    CSetterBaseCollection* m_pDeferredSetters;

    std::vector<std::shared_ptr<StateTriggerVariantMap>>& GetStateTriggerVariantMaps() { return m_stateTriggerVariantMaps; }

protected:

    // Children will need to be able to Find named items relative to themselves, and as such
    // need to have an ensured path to a NamescopeOwner.
    bool ShouldEnsureNameResolution() override;

    CDependencyObject* GetTemplatedParent() final;

private:
    void RegisterDeferredStandardNameScopeEntries(_In_ CDependencyObject* namescopeOwner) final;
    void UnregisterDeferredStandardNameScopeEntries(_In_ CDependencyObject* namescopeOwner) final;

    CVisualStateGroupCollection(_In_ CCoreServices *pCore);

    std::shared_ptr<VisualStateGroupCollectionCustomRuntimeData> m_runtimeData;
    std::shared_ptr<Jupiter::VisualStateManager::DeferredNameScopeEntry> m_deferredNameScopeEntry;
    std::unique_ptr<CustomWriterRuntimeContext> m_runtimeContext;
    std::vector<VisualStateGroupContext> m_groupContext;
    bool m_faultedInChildren;

    // We only want to skip storyboards to their ending point once when leaving the tree for
    // efficiency and because it allows us to maintain stricter invarients inside the VSM. To
    // support this we set this bool every time a GoToState call is performed on this group, and
    // clear it when we leave the tree. This allows us to know for certain if actions have been
    // performed that could require a storyboard be skipped to end.
    bool m_vsmActivitySinceLastLeave;

    std::vector<std::shared_ptr<StateTriggerVariantMap>> m_stateTriggerVariantMaps;
};
