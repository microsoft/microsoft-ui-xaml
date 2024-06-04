// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CDependencyObject.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <CPropertyPath.h>
#include <collectionbase.h>
#include <DOCollection.h>
#include <DXamlServices.h>
#include <AutoReentrantReferenceLock.h>
#include <CValue.h>
#include <UIElement.h>
#include <double.h>
#include <Point.h>
#include <dopointercast.h>
#include <corep.h>
#include "ICollectionChangeCallback.h"
#include <MultiParentShareableDependencyObject.h>
#include <Panel.h>
#include <Layouttransition.h>
#include <TransitionCollection.h>
#include <TrimWhitespace.h>
#include <StringConversions.h>
#include <UIElementCollection.h>
#include <UIElement.h>
#include <Framework.h>
#include <CControl.h>
#include <ContentControl.h>
#include <UIElementStructs.h>
#include <EventArgs.h>
#include <Canvas.h>
#include <TransitionRoot.h>
#include <EventMgr.h>
#include <Popup.h>
#include <LayoutTransitionElement.h>
#include <LayoutManager.h>
#include <StateTriggerCollection.h>
#include <DOCollection.h>
#include <SetterBaseCollection.h>
#include <VisualStateGroupCollection.h>
#include "VisualTransitionCollection.h"
#include <wil\result.h>
#include <FxCallbacks.h>

#include <VisualStateGroupCollectionCustomRuntimeData.h>
#include <CustomWriterRuntimeContext.h>
#include <CVisualStateManager2.h>
#include <CustomWriterRuntimeObjectCreator.h>
#include <StreamOffsetToken.h>
#include "theming\inc\Theme.h"

CVisualStateGroupCollection::CVisualStateGroupCollection(_In_ CCoreServices *pCore)
    : CDOCollection(pCore)
    , m_pActiveStoryboards(nullptr)
    , m_pActiveTransitions(nullptr)
    , m_pDeferredStateTriggers(nullptr)
    , m_pDeferredSetters(nullptr)
    , m_faultedInChildren(false)
    , m_vsmActivitySinceLastLeave(false)
{}

CVisualStateGroupCollection::~CVisualStateGroupCollection()
{
    if (m_vsmActivitySinceLastLeave)
    {
        CVisualStateManager2::OnVisualStateGroupCollectionLeave(this);
        m_vsmActivitySinceLastLeave = false;
    }

    ReleaseInterface(m_pActiveStoryboards);
    ReleaseInterface(m_pActiveTransitions);
    ReleaseInterface(m_pDeferredStateTriggers);
    ReleaseInterface(m_pDeferredSetters);
}

// DEAD_CODE_REMOVAL: remove this function override.
bool CVisualStateGroupCollection::ShouldEnsureNameResolution()
{
    return true;
}

CDependencyObject* CVisualStateGroupCollection::GetTemplatedParent()
{
    auto firstParent = GetParentInternal(false /* fPublic */);
    return firstParent ? firstParent->GetTemplatedParent() : nullptr;
}

#pragma region Optimized Storage and Data Accessors
_Check_return_ HRESULT
CVisualStateGroupCollection::SetCustomWriterRuntimeData(std::shared_ptr<CustomWriterRuntimeData> data,
    std::unique_ptr<CustomWriterRuntimeContext> context)
{
    // Depending on how the ObjectWriter is configured these invariants might not always be true.
    // It's better to capture this condition on origination than allow it to blow up later.
    ASSERT(data && context && context->GetReader());

    m_runtimeData = std::static_pointer_cast<VisualStateGroupCollectionCustomRuntimeData>(data);
    m_runtimeContext = std::move(context);

    auto templateParent = GetTemplatedParent();
    // The RuntimeData will determine we should bail out if it detected event registrations
    // or other oddness on the elements OR if we're a version of the VSM encoded before we stored
    // the list of x:Name elements in XBFv2.
    if (m_runtimeData->ShouldBailOut() ||
        (m_runtimeData->ShouldBailOutForUserControls() && !templateParent))
    {
        IFC_RETURN(CVisualStateManager2::FaultInChildren(this));
    }
    else
    {
        m_deferredNameScopeEntry = std::make_shared<Jupiter::VisualStateManager::DeferredNameScopeEntry>(xref::get_weakref(this));
        auto parseTimeNameScopeAdapter = m_runtimeContext->GetNameScope();
        auto core = GetContext();

        // We could use INameScope here, but instead we'll do something a little more direct. We're either
        // a template namescope member, in which case we should register these in our template parent's namescope,
        // or we're in a standard namescope, in which case we should register these with the current NameScope owner.
        for (const auto& name : m_runtimeData->GetSeenNames())
        {
            core->GetNameScopeRoot().SetNamedObject(name,
                parseTimeNameScopeAdapter->GetNamescopeOwner(),
                parseTimeNameScopeAdapter->GetNameScopeType(),
                std::weak_ptr<Jupiter::NameScoping::INameScopeDeferredElementCreator>(
                    std::static_pointer_cast<Jupiter::NameScoping::INameScopeDeferredElementCreator>(m_deferredNameScopeEntry)));
        }
    }

    return S_OK;
}
#pragma endregion

void CVisualStateGroupCollection::RegisterDeferredStandardNameScopeEntries(_In_ CDependencyObject* namescopeOwner)
{
    ASSERT(!IsTemplateNamescopeMember());
    if (m_deferredNameScopeEntry)
    {
        auto core = GetContext();
        core->GetNameScopeRoot().EnsureNameScope(namescopeOwner, nullptr);
        for (const auto& name : m_runtimeData->GetSeenNames())
        {
            core->GetNameScopeRoot().SetNamedObject(name, namescopeOwner,
                Jupiter::NameScoping::NameScopeType::StandardNameScope,
                std::weak_ptr<Jupiter::NameScoping::INameScopeDeferredElementCreator>(
                    std::static_pointer_cast<Jupiter::NameScoping::INameScopeDeferredElementCreator>(m_deferredNameScopeEntry)));
        }
    }
}

void CVisualStateGroupCollection::UnregisterDeferredStandardNameScopeEntries(_In_ CDependencyObject* namescopeOwner)
{
    if (m_deferredNameScopeEntry)
    {
        CCoreServices* core = GetContext();

        // Bug 15909774: [Class Issue][Compat][Store][UWP]
        //  [Microsoft Photos/MSN Weather/MSN News/Microsoft Edge][2018.18011.13438.0]:
        //  ShellExperience host crashes while clicking on share button multiple times
        //
        // If we're destroying the core, skip clearing the named object; the namescope
        // tables are going to be destroyed, if they haven't already been.
        if (!core->IsDestroyingCoreServices())
        {
            for (const auto& name : m_runtimeData->GetSeenNames())
            {
                core->GetNameScopeRoot().ClearNamedObjectIfExists(name, namescopeOwner);
            }
        }
    }
}


_Check_return_ HRESULT
CVisualStateGroupCollection::EnterImpl(_In_ CDependencyObject* namescopeOwner, _In_ EnterParams params)
{
    IFC_RETURN(CDOCollection::EnterImpl(namescopeOwner, params));

    if (params.fIsLive && m_vsmActivitySinceLastLeave)
    {
        // If a group has been modified while it is outside of the tree, then the setters may not have
        // the correct themed value, so when we re-enter, if there has been activity refresh the theme
        // resources.
        IFC_RETURN(NotifyThemeChanged(GetTheme(), true /* forceRefresh */));
    }
    return S_OK;
}


_Check_return_ HRESULT
CVisualStateGroupCollection::LeaveImpl(_In_ CDependencyObject* namescopeOwner, _In_ LeaveParams params)
{
    if (m_vsmActivitySinceLastLeave)
    {
        CVisualStateManager2::OnVisualStateGroupCollectionLeave(this);
        m_vsmActivitySinceLastLeave = false;
    }

    IFC_RETURN(CDOCollection::LeaveImpl(namescopeOwner, params));
    return S_OK;
}

#pragma region DOCollection Overrides
XUINT32 CVisualStateGroupCollection::GetCount() const
{
    if (m_faultedInChildren || !m_runtimeData)
    {
        return CDOCollection::GetCount();
    }
    else
    {
        return static_cast<unsigned int>(m_runtimeData->GetVisualStateGroupCount());
    }
}

_Check_return_ HRESULT CVisualStateGroupCollection::Insert(_In_ XUINT32 index, _In_ CDependencyObject* object)
{
    IFC_RETURN(EnsureFaultedIn());
    IFC_RETURN(CDOCollection::Insert(index, object));
    return S_OK;
}

_Check_return_ HRESULT CVisualStateGroupCollection::Append(_In_ CDependencyObject* object, _Out_opt_ unsigned int* index)
{
    IFC_RETURN(EnsureFaultedIn());
    IFC_RETURN(CDOCollection::Append(object, index));
    return S_OK;
}

_Check_return_ HRESULT CVisualStateGroupCollection::OnClear()
{
    IFC_RETURN(EnsureFaultedIn());
    IFC_RETURN(CDOCollection::OnClear());
    return S_OK;
}

_Check_return_ void* CVisualStateGroupCollection::GetItemWithAddRef(_In_ unsigned int index)
{
    IFCFAILFAST(EnsureFaultedIn());
    return CDOCollection::GetItemWithAddRef(index);
}

_Check_return_ HRESULT CVisualStateGroupCollection::NotifyThemeChangedCore(
    _In_ Theming::Theme theme,
    _In_ bool fForceRefresh)
{
    IFC_RETURN(CDOCollection::NotifyThemeChangedCore(theme, fForceRefresh));
    IFC_RETURN(CVisualStateManager2::OnVisualStateGroupCollectionNotifyThemeChanged(const_cast<CVisualStateGroupCollection*>(this)));

    return S_OK;
}

#pragma endregion

_Check_return_ HRESULT CVisualStateGroupCollection::EnsureFaultedIn() const
{
    if (!m_faultedInChildren && m_runtimeData)
    {
        // WARNING: const_cast ahead. We can't really change the contract of CDOCollection, but we absolutely
        // have to make sure that when GetCount() is called that we fault in the group collection. You
        // win some and you lose some I suppose...

        // Clear the one and only strong reference to our deferred namescope registration, which
        // will kill off all the weak pointers which are soon to be replaced by entries pointing to
        // the real thing.
        const_cast<CVisualStateGroupCollection*>(this)->m_deferredNameScopeEntry.reset();
        IFC_RETURN(CVisualStateManager2::FaultInChildren(const_cast<CVisualStateGroupCollection*>(this)));
    }
    return S_OK;
}

bool CVisualStateGroupCollection::AreAnimationsEnabled() const
{
    bool isAnimationsEnabled = false;
    // This API will sometimes return bad HRs if we're not running as a CoreApplication.
    // That's fine and the correct behavior is to return false here (assume we're running
    // under designer maybe?)
    IGNOREHR(FxCallbacks::FrameworkCallbacks_IsAnimationEnabled(&isAnimationsEnabled));
    return isAnimationsEnabled;
}

xref_ptr<CControl> CVisualStateGroupCollection::GetOwningControl() const
{
    // Our tree should look like this:
    // Control -> Root Element -> VisualStateGroupCollection
    auto parent = GetParentInternal(false);
    if (!parent) return nullptr;
    parent = parent->GetParentInternal(false);
    auto returnValue = xref_ptr<CControl>(do_pointer_cast<CControl>(parent));
    // Right now, because CVisualStateGroupCollection is RO, I can't imagine any scenario where VSGC should
    // be unable to resolve its owning control, except in the final stages of object deconstruction, at which
    // point we shouldn't be calling into this API. This ASSERT should expose any edge cases we need
    // to think carefully about.
    ASSERT(returnValue);
    return returnValue;
}