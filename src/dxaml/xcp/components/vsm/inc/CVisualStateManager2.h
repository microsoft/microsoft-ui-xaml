// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <vsm\inc\VisualStateToken.h>

class CControl;
class CVisualStateGroupCollection;
class CustomWriterRuntimeObjectCreator;
class CDependencyObject;

template<typename T>
class xref_ptr;

template<typename T, typename U>
class VariantMap;

using StateTriggerVariantMap = VariantMap<VisualStateToken, xref_ptr<CDependencyObject>>;

#include "VisualStateSetterHelper.h"

namespace Resources {
    class ResourceResolver;
}

namespace Diagnostics {
    class ElementStateChangedBuilder;
}

class CVisualStateManager2
{
    friend class Resources::ResourceResolver;
    friend class Diagnostics::ElementStateChangedBuilder;
    friend class Diagnostics::DiagnosticsInterop;

public:
    // Performs the GoToState operation using the new CustomRuntimeData
    // optimzied data structures. We're calling this a rewrite because eventually
    // the normal VSM logic will follow this control path as well, using the
    // abstractions introduced for the OptimizedVSGC.
    static _Check_return_ HRESULT GoToStateOptimized(
        _In_ CControl *pControl,
        _In_z_ const WCHAR* pStateName,
        _In_ bool useTransitions,
        _Out_ bool* succeeded);

    static _Check_return_ HRESULT GoToStateOptimized(
        _In_ CControl *pControl,
        _In_ VisualStateToken token,
        _In_ bool useTransitions,
        _Out_ bool* succeeded);

    // Returns a bool specifying whether a visual state with a given name exists. Note that
    // in the undeferred VSM case some of the called methods have signatures that suggest they
    // can throw, but in practice they can't, they're just Getters.
    static bool DoesVisualStateExist(
        _In_ CControl *pControl, _In_z_ const WCHAR* pStateName);

    static _Check_return_ HRESULT FaultInChildren(_In_ CVisualStateGroupCollection* groupCollection);

    // Called when a VisualStateGroupCollection leaves the live visual tree. This method
    // will ensure the proper clean-up of Storyboards happens by synchronously snapping all
    // pending storyboards to their end value, causing pending VisualTransitions to complete.
    static HRESULT OnVisualStateGroupCollectionLeave(_In_ CVisualStateGroupCollection* groupCollection);
    static HRESULT __stdcall OnStoryboardCompleted(_In_ CDependencyObject* sender, _In_ CEventArgs* eventArgs);

    // Called when a VisualStateGroupCollection gets a theme change notification
    static HRESULT OnVisualStateGroupCollectionNotifyThemeChanged(_In_ CVisualStateGroupCollection* groupCollection);

    // Initializes all VisualState.StateTriggers in a control's VisualStateGroupCollection
    static HRESULT InitializeStateTriggers(_In_ CDependencyObject* pDO, const bool forceUpdate = false);
    static void AssignStateTriggerChangedCallback(_In_ CControl* pControl, StateTriggerVariantMap& vm);

    // Returns the group index for a VisualState name, or -1 if it doesn't exist
    static int GetGroupIndexFromVisualState(
        _In_ CControl *pControl, _In_z_ const WCHAR* pStateName);

    static int GetGroupIndexFromVisualState(
        _In_ CControl *pControl, _In_ VisualStateToken token);

    // Resets a VSM Group back to its NULL state using an index. In the future this
    // might need to be updated to use either the string of the group name or some other
    // unique token that doesn't change when the collection is modified. A separate work
    // item exists to harden VSM against collection changes.
    static _Check_return_ HRESULT ResetVisualStateGroupToNullState(_In_ CControl *pControl, _In_ int groupIndex);

    static xref_ptr<CDependencyObject> GetCustomVisualStateManager(_In_ CControl *pControl);
    static xref_ptr<CDependencyObject> GetVisualState(_In_ CControl* pControl, _In_ VisualStateToken token);
    static xstring_ptr GetVisualStateName(_In_ CControl* pControl, _In_ VisualStateToken token);

    // We require the state to be active because otherwise the changes will be applied on the next state transition.
    static HRESULT TryRemoveStoryboardFromState(_In_ CVisualState* state, _In_ const xref_ptr<CStoryboard>& storyboard);
    static HRESULT TryAddStoryboardToState(_In_ CVisualState* state, _In_ const xref_ptr<CStoryboard>& storyboard);

    static bool IsActiveVisualState(_In_ CVisualState* visualState);
private:
    static xref_ptr<CVisualStateGroupCollection> GetGroupCollectionFromControl(_In_ CControl* pControl);
    static xref_ptr<CVisualStateGroupCollection> GetGroupCollectionFromVisualState(_In_ const CVisualState* state);
    static std::shared_ptr<StateTriggerVariantMap> CreateStateTriggerVariantMap(_In_ CControl* pControl, int groupIndex); 
    static HRESULT FaultInStateTriggers(_In_ CVisualStateGroupCollection* groupCollection, CustomWriterRuntimeObjectCreator& creator);
    static _Check_return_ HRESULT GoToStateOptimizedImpl(
        _In_ CControl *pControl,
        _In_z_ const WCHAR* pStateName,
        _In_ VisualStateToken token,
        _In_ bool useTransitions,
        _Out_ bool* succeeded);

};
