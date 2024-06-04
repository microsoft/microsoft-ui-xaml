// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RadioButton.g.h"
#include "RadioButtonAutomationPeer.g.h"
#include "KeyboardNavigation.h"
#include "KeyRoutedEventArgs.g.h"
#include "focusmgr.h"
#include "VisualTreeHelper.h"

using namespace DirectUI;
using namespace Focus;

// Initializes a new instance of the RadioButton class.
RadioButton::RadioButton()
{
}

// Destructor
RadioButton::~RadioButton()
{
}

_Check_return_ HRESULT
RadioButton::Initialize()
{
    IFC_RETURN(RadioButtonGenerated::Initialize());
    // Ignore the ENTER key by default
    SetAcceptsReturn(false);
    IFC_RETURN(Register(NULL, this));

    return S_OK;
}

// Disconnect framework peer and remove RadioButton from Group
_Check_return_
HRESULT
RadioButton::DisconnectFrameworkPeerCore()
{
    HRESULT hr = S_OK;

    if (GetHandle() == NULL)
    {
        goto Cleanup;
    }

    // If we run off-thread, then don't worry about unregistering ourselves... We're using
    // weak references anyway, so they'll just stop resolving at this point.
    if (DXamlServices::IsDXamlCoreInitialized())
    {
        IFC(UnregisterSafe(this));
    }

    IFC(RadioButtonGenerated::DisconnectFrameworkPeerCore());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT RadioButton::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spOldValue;
    ctl::ComPtr<IInspectable> spNewValue;

    IFC(RadioButtonGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::RadioButton_GroupName:
            IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, /* pTargetType */ nullptr, &spOldValue));
            IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, /* pTargetType */ nullptr, &spNewValue));
            IFC(OnGroupNamePropertyChanged(spOldValue.Get(), spNewValue.Get()));
            break;
        case KnownPropertyIndex::ToggleButton_IsChecked:
            {
                ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;

                BOOLEAN bOldValue = FALSE;
                BOOLEAN bNewValue = FALSE;

                BOOLEAN bListenerExistsForPropertyChangedEvent = FALSE;
                BOOLEAN bListenerExistsForElementSelectedEvent = FALSE;
                BOOLEAN bListenerExistsForElementRemovedFromSelectionEvent = FALSE;

                IFC(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_PropertyChanged, &bListenerExistsForPropertyChangedEvent));
                IFC(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_SelectionItemPatternOnElementSelected, &bListenerExistsForElementSelectedEvent));
                IFC(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_SelectionItemPatternOnElementRemovedFromSelection, &bListenerExistsForElementRemovedFromSelectionEvent));

                if (bListenerExistsForPropertyChangedEvent || bListenerExistsForElementSelectedEvent || bListenerExistsForElementRemovedFromSelectionEvent)
                {
                    IFC(GetOrCreateAutomationPeer(&spAutomationPeer));

                    IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, /* pTargetType */ nullptr, &spOldValue));
                    IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, /* pTargetType */ nullptr, &spNewValue));

                    if (spOldValue != nullptr)
                    {
                        IFC(ctl::do_get_value(bOldValue, spOldValue.Get()));
                    }

                    if (spNewValue)
                    {
                        IFC(ctl::do_get_value(bNewValue, spNewValue.Get()));
                    }
                }

                if (spAutomationPeer)
                {
                    if (bListenerExistsForPropertyChangedEvent)
                    {
                        ctl::ComPtr<xaml_automation_peers::IRadioButtonAutomationPeer> spRadioButtonAutomationPeer;

                        spRadioButtonAutomationPeer = spAutomationPeer.AsOrNull<xaml_automation_peers::IRadioButtonAutomationPeer>();
                        if (spRadioButtonAutomationPeer)
                        {
                            IFC(spRadioButtonAutomationPeer.Cast<RadioButtonAutomationPeer>()->RaiseIsSelectedPropertyChangedEvent(bOldValue, bNewValue));
                        }
                    }

                    if (bOldValue != bNewValue)
                    {
                        if (bListenerExistsForElementSelectedEvent && bNewValue)
                        {
                            IFC(spAutomationPeer->RaiseAutomationEvent(xaml_automation_peers::AutomationEvents_SelectionItemPatternOnElementSelected));
                        }

                        if (bListenerExistsForElementRemovedFromSelectionEvent && !bNewValue)
                        {
                            IFC(spAutomationPeer->RaiseAutomationEvent(xaml_automation_peers::AutomationEvents_SelectionItemPatternOnElementRemovedFromSelection));
                        }
                    }
                }
            }
            break;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
RadioButton::OnGroupNamePropertyChanged(
    _In_ IInspectable* pOldValue,
    _In_ IInspectable* pNewValue)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strOldValue;
    wrl_wrappers::HString strNewValue;

    if (pOldValue)
    {
        IFC(ctl::do_get_value(*strOldValue.GetAddressOf(), pOldValue));
    }
    if (pNewValue)
    {
        IFC(ctl::do_get_value(*strNewValue.GetAddressOf(), pNewValue));
    }

    IFC(Unregister(strOldValue.Get(), this));
    IFC(Register(strNewValue.Get(), this));

    IFC(UpdateRadioButtonGroup());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT RadioButton::EnterImpl(
    _In_ bool bLive,
    _In_ bool bSkipNameRegistration,
    _In_ bool bCoercedIsEnabled,
    _In_ bool bUseLayoutRounding)
{
    IFC_RETURN(__super::EnterImpl(bLive, bSkipNameRegistration, bCoercedIsEnabled, bUseLayoutRounding));
    if (bLive)
    {
        IFC_RETURN(UpdateRadioButtonGroup());
    }
    return S_OK;
}

_Check_return_ HRESULT RadioButton::OnChecked()
{
    HRESULT hr = S_OK;
    IFC(UpdateRadioButtonGroup());
    IFC(RadioButtonGenerated::OnChecked());

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP RadioButton::OnToggle()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spNewValue;
    ctl::ComPtr<wf::IReference<bool>> spNewValueReference;

    IFC(PropertyValue::CreateFromBoolean(TRUE, &spNewValue));
    IFC(spNewValue.As(&spNewValueReference));
    IFC(put_IsChecked(spNewValueReference.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
RadioButton::Register(_In_opt_ HSTRING hGroupName, _In_ RadioButton* pRadioButton)
{
    HRESULT hr = S_OK;
    std::list<ctl::WeakRefPtr> *groupElements = nullptr;
    xstring_ptr groupName;
    ctl::ComPtr<IWeakReference> weakRef;
    xchainedmap<xstring_ptr, std::list<ctl::WeakRefPtr>*> *groupsByName;

    IFCEXPECT(pRadioButton);
    if (hGroupName)
    {
        IFC(xstring_ptr::CloneRuntimeStringHandle(hGroupName, &groupName));
    }

    // When registering, require DXamlCore to create the table of radio button group names if it doesn't exist.
    IFC(DXamlCore::GetCurrent()->GetRadioButtonGroupsByName(TRUE, groupsByName));
    IFC(groupsByName->Get(groupName, groupElements)); // will return S_FALSE if key not found

    if (!groupElements)
    {
        groupElements = new std::list<ctl::WeakRefPtr>();

        IFC(groupsByName->Add(groupName, groupElements));
    }

    // Keep a weak ref to the button; we don't want this list to cause the button to leak, and IWeakReference is
    // automatically updated during GC.
    // Note that we are calling GetWeakReference here instead of using as_weakref. This is done because as_weakref does a QI
    // and Register() is called during Initialize() so in case of controls deriving from RadioButton,
    // the QI might be querying the outer object which has not been constructed yet.
    IFC(pRadioButton->GetWeakReference(&weakRef));
    groupElements->push_front(weakRef);

Cleanup:

    RRETURN(hr);
}

// Unregister by searching for the instance in all groups. This is safer during shutdown, because
// get_GroupName may access an external (CLR) string which may have been GC'ed.
_Check_return_ HRESULT
RadioButton::UnregisterSafe(_In_ RadioButton * pRadioButton)
{
    std::list<ctl::WeakRefPtr> *groupElements = nullptr;
    xchainedmap<xstring_ptr, std::list<ctl::WeakRefPtr>*> *groupsByName;
    bool found = false;
    bool fRestart = false;

    IFCPTR_RETURN(pRadioButton);

    IFC_RETURN(DXamlCore::GetCurrent()->GetRadioButtonGroupsByName(FALSE, groupsByName));

    // DXamlCore may have already deleted the list of names depending on the order of operations during shutdown,
    // so it may be a nullptr.
    if (groupsByName)
    {
        for (auto map_it = groupsByName->begin();
             map_it != groupsByName->end();
             fRestart ? map_it = groupsByName->begin() : ++map_it)
        {
            fRestart = false;

            groupElements = map_it->second;
            IFC_RETURN(UnregisterFromGroup(groupElements, pRadioButton, &found));

            if (groupElements->empty())
            {
                IFC_RETURN(groupsByName->Remove(map_it->first, groupElements));
                delete groupElements;

                fRestart = true;
            }

            if (found)
            {
                break;
            }
        }
    }
    return S_OK;
}

// Unregister within a specific group name. This is faster, but not safe during shutdown.
_Check_return_ HRESULT
RadioButton::Unregister(_In_opt_ HSTRING hGroupName, _In_ RadioButton * pRadioButton)
{
    std::list<ctl::WeakRefPtr> *groupElements = nullptr;
    xstring_ptr groupName;
    xchainedmap<xstring_ptr, std::list<ctl::WeakRefPtr>*> *groupsByName;
    bool found = false;

    IFCPTR_RETURN(pRadioButton);

    if (hGroupName)
    {
        IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(hGroupName, &groupName));
    }

    IFC_RETURN(DXamlCore::GetCurrent()->GetRadioButtonGroupsByName(FALSE, groupsByName));

    // DXamlCore may have already deleted the list of names depending on the order of operations during shutdown,
    // so it may be a nullptr.
    if (groupsByName)
    {
        if (S_OK == groupsByName->Get(groupName, groupElements))
        {
            IFC_RETURN(UnregisterFromGroup(groupElements, pRadioButton, &found));

            if (groupElements->empty())
            {
                IFC_RETURN(groupsByName->Remove(groupName, groupElements));
                delete groupElements;
            }
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
RadioButton::UnregisterFromGroup(std::list<ctl::WeakRefPtr> *groupElements, _In_ RadioButton * pRadioButton, _Out_ bool* found)
{
    *found = false;

    // Remove pRadioButton from the list
    bool fRestart = false;

    std::list<ctl::WeakRefPtr>::iterator list_it;
    for(
        list_it = groupElements->begin();
        list_it != groupElements->end();
        fRestart ? list_it = groupElements->begin() : ++list_it)
    {
        fRestart = false;

        // Resolve the weak ref to a RadioButton
        ctl::WeakRefPtr pRadioButtonRef = *list_it;
        ctl::ComPtr<IRadioButton> radioButtonCheck;
        IFC_RETURN(pRadioButtonRef.As(&radioButtonCheck));

        // If this is the radio button we're looking for, or it's a dead weak reference, we want
        // to remove this item from the list.
        if (radioButtonCheck == nullptr || radioButtonCheck.Get() == pRadioButton)
        {
            // Remove the item
            groupElements->erase(list_it);

            // If this was the item we were looking for, we're done.
            if (radioButtonCheck.Get() == pRadioButton)
            {
                *found = true;
                break;
            }

            // Otherwise, we removed this because it was a dead weak ref, but it might not have been the item
            // we were looking for.  There could be more dead weak refs in this list, or the actual
            // live item still in this list.  For that case, restart the enumeration and continue.
            else
            {
                ASSERT(radioButtonCheck == nullptr);
                fRestart = true;
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
RadioButton::UpdateRadioButtonGroup()
{
    BOOLEAN bIsChecked = FALSE;
    ctl::ComPtr<wf::IReference<bool>> spIsCheckedReference;

    // If we are not checked then we don't have any effect on other buttons
    IFC_RETURN(get_IsChecked(&spIsCheckedReference));
    if (spIsCheckedReference)
    {
        IFC_RETURN(spIsCheckedReference->get_Value(&bIsChecked));
        if (!bIsChecked)
        {
            return S_OK;
        }
    }

    RadioButton *pRadioButtonNoRef = nullptr;
    std::list<ctl::WeakRefPtr> *groupElements = nullptr;
    bool groupNameExists = false;
    xstring_ptr groupName;
    xref_ptr<CDependencyObject> spParent;

    IFC_RETURN(GetGroupName(&groupNameExists, &groupName));
    IFC_RETURN(GetParentForGroup(groupNameExists, this, spParent.ReleaseAndGetAddressOf()));

    // If we don't get a parent then don't do anything and let it be handled on enter impl.  Without
    // a parent we don't know what the scope of a named group item is and for unnamed items, they
    // will all share the same group preventing different groups from having buttons selected.
    if (!spParent)
    {
        return S_OK;
    }
    
    // We need to make sure the DXaml peer exists for our parent because this can propagate a new
    // DataContext and cause an app FrameworkElement.DataContextChanged event handler to affect
    // the results of GetGroupName, GetParentForGroup and GetRadioButtonGroupsByName.
    // So the GetGroupName & GetParentForGroup methods are called a second time if we need to
    // create a peer to make sure the resulting groupName, spParent, groupsByName
    // and groupElements variables are valid.
    {
        ctl::ComPtr<DependencyObject> dxamlPeer;

        IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(spParent, dxamlPeer.ReleaseAndGetAddressOf()));
        if (!dxamlPeer)
        {
            IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(spParent, dxamlPeer.ReleaseAndGetAddressOf()));
            IFC_RETURN(GetGroupName(&groupNameExists, &groupName));
            IFC_RETURN(GetParentForGroup(groupNameExists, this, spParent.ReleaseAndGetAddressOf()));
        }
    }

    xchainedmap<xstring_ptr, std::list<ctl::WeakRefPtr>*> *groupsByName;
    IFC_RETURN(DXamlCore::GetCurrent()->GetRadioButtonGroupsByName(FALSE, groupsByName));

    // DXamlCore will delete RadioButton name table during shutdown, so check against it being a nullptr or having 0 elements.
    if (groupsByName && groupsByName->Count() > 0)
    {
        if (S_OK == groupsByName->Get(groupName, groupElements))
        {
            if (groupElements && !groupElements->empty())
            {
                std::vector<ctl::WeakRefPtr> groupElementsCache(groupElements->begin(), groupElements->end());

                for (auto vector_it = groupElementsCache.begin(); vector_it != groupElementsCache.end(); ++vector_it)
                {
                    // Resolve the weak reference to get an actual IRadioButton
                    ctl::WeakRefPtr pRadioButtonRef = *vector_it;
                    ctl::ComPtr<IRadioButton> radioButton;
                    IFC_RETURN(pRadioButtonRef.As(&radioButton));

                    // If the weak reference is dead, it's not a match, we can move on (it will get cleaned up
                    // later when the RadioButton is destructed).
                    if (radioButton == nullptr)
                        continue;

                    // Cast to a class so we can call all the members
                    pRadioButtonNoRef = static_cast<RadioButton*>(radioButton.Get());

                    IFC_RETURN(pRadioButtonNoRef->get_IsChecked(spIsCheckedReference.ReleaseAndGetAddressOf()));
                    if (spIsCheckedReference)
                    {
                        IFC_RETURN(spIsCheckedReference->get_Value(&bIsChecked));
                    }

                    if (pRadioButtonNoRef != this && (!spIsCheckedReference || bIsChecked))
                    {
                        xref_ptr<CDependencyObject> spCurrentParent;
                        IFC_RETURN(GetParentForGroup(groupNameExists, pRadioButtonNoRef, spCurrentParent.ReleaseAndGetAddressOf()));

                        if (spParent == spCurrentParent)
                        {
                            ctl::ComPtr<IInspectable> spNewIsCheckedValue;
                            ctl::ComPtr<wf::IReference<bool>> spNewIsCheckedValueReference;

                            IFC_RETURN(PropertyValue::CreateFromBoolean(FALSE, &spNewIsCheckedValue));
                            IFC_RETURN(spNewIsCheckedValue.As(&spNewIsCheckedValueReference));
                            IFC_RETURN(pRadioButtonNoRef->put_IsChecked(spNewIsCheckedValueReference.Get()));
                        }
                    }
                }
            }
        }
        else
        {
            IFCEXPECT_RETURN(!groupNameExists);
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
RadioButton::GetGroupName(_Out_ bool* groupNameExists, _Out_ xstring_ptr* groupName)
{
    *groupNameExists = false;
    wrl_wrappers::HString strGroupName;
    IFC_RETURN(get_GroupName(strGroupName.GetAddressOf()));

    if (strGroupName.Get())
    {
        IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(strGroupName.Get(), groupName));
        *groupNameExists = true;
    }
    else
    {
        groupName->Reset();
    }
    return S_OK;
}

_Check_return_ HRESULT
RadioButton::GetParentForGroup(_In_ bool groupNameExists, _In_ RadioButton* radioButton, _Outptr_ CDependencyObject** parent)
{
    *parent = nullptr;

    // Previously we used to return the DXaml peer but that requires us to be able to get the peer of the parent and
    // there are scenarios when we are trying to get the parent for an item not in the tree that is the process
    // of being destructed.  In this case we will try to resurrect the peer and will fail.  In addition, the methods
    // used to get the DXaml peer always return null if the element is not in the active tree.  This caused us to
    // improperly group all radio buttons not in the tree into a single group and we end up unchecking the wrong buttons.
    // So currently we only walk the core side which ensure we always have some kind of a parent AND does not get
    // the Dxaml peer when it isn't needed.
    CValue result;
    IFC_RETURN(CoreImports::DependencyObject_GetVisualRelative(
        static_cast<CUIElement*>(radioButton->GetHandle()),
        groupNameExists ? CoreImports::VisualRelativeKind_Root : CoreImports::VisualRelativeKind_Parent,
        &result));

    *parent = result.DetachObject().detach();

     return S_OK;
}

_Check_return_ HRESULT RadioButton::AutomationRadioButtonOnToggle()
{
    HRESULT hr = S_OK;

    // OnToggle through UIAutomation
    IFC(OnClick());

Cleanup:
    RRETURN(hr);
}

// Create RadioButtonAutomationPeer to represent the RadioButton.
IFACEMETHODIMP RadioButton::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IRadioButtonAutomationPeer> spRadioButtonAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IRadioButtonAutomationPeerFactory> spRadioButtonAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::RadioButtonAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spRadioButtonAPFactory));

    IFC(spRadioButtonAPFactory.Cast<RadioButtonAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spRadioButtonAutomationPeer));
    IFC(spRadioButtonAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
RadioButton::OnKeyDown(
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    IFC_RETURN(RadioButtonGenerated::OnKeyDown(pArgs));

    BOOLEAN handled = FALSE;
    IFC_RETURN(pArgs->get_Handled(&handled));
    if (!handled)
    {
        bool groupNameExists = false;
        xstring_ptr groupName;
        IFC_RETURN(GetGroupName(&groupNameExists, &groupName));

        // We need to get OriginalKey here and not the "mapped" key because we want to focus
        // the next element in RadioButton "group" for Up/Down and Left/Right keys only for Keyboard,
        // but for other input devices, like Gamepad or Remote, we want the default focus behavior.
        auto originalKey = wsy::VirtualKey_None;
        IFC_RETURN(static_cast<KeyRoutedEventArgs*>(pArgs)->get_OriginalKey(&originalKey));

        bool wasFocused = false;
        switch (originalKey)
        {
        case wsy::VirtualKey_Down:
        case wsy::VirtualKey_Right:
            IFC_RETURN(FocusNextElementInGroup(/* moveForward */ true, &wasFocused));
            handled = TRUE;
            break;
        case wsy::VirtualKey_Up:
        case wsy::VirtualKey_Left:
            IFC_RETURN(FocusNextElementInGroup(/* moveForward */ false, &wasFocused));
            handled = TRUE;
            break;
        }
        IFC_RETURN(pArgs->put_Handled(handled));
    }
    return S_OK;
}

_Check_return_ HRESULT
RadioButton::FocusNextElementInGroup(
    _In_ bool moveForward,
    _Out_ bool* wasFocused)
{
    *wasFocused = false;

    bool currentGroupNameExists = false;
    xstring_ptr currentGroupName;
    IFC_RETURN(GetGroupName(&currentGroupNameExists, &currentGroupName));

    ctl::ComPtr<DependencyObject> currentParent;
    xref_ptr<CDependencyObject> currentParentDO;
    IFC_RETURN(GetParentForGroup(currentGroupNameExists, this, currentParentDO.ReleaseAndGetAddressOf()));
    if (currentParentDO)
    {
        IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(currentParentDO, &currentParent));
    }

    auto pFocusManager = VisualTree::GetFocusManagerForElement(GetHandle());
    if (currentParentDO && pFocusManager)
    {
        CDependencyObject* nextFocusCandidate = nullptr;
        CDependencyObject* currentRadioButtonDO = static_cast<CDependencyObject*>(this->GetHandle());
        CDependencyObject* firstFocusCandidate = pFocusManager->GetFirstFocusableElement(currentParentDO); // First focusable element, given current parent.
        CDependencyObject* lastFocusCandidate = pFocusManager->GetLastFocusableElement(currentParentDO); // Last focusable element, given current parent.
        DirectUI::FocusNavigationDirection navigationDirection = DirectUI::FocusNavigationDirection::None;

        // Focus candidate is to set to Next or Previous Tab Stop, depending on movement direction, unless we have reached the first/last element, in which case,
        // we loop around.
        if (moveForward)
        {
            nextFocusCandidate = (currentRadioButtonDO == lastFocusCandidate) ? firstFocusCandidate : pFocusManager->GetNextTabStop(currentRadioButtonDO, FALSE);
            navigationDirection = DirectUI::FocusNavigationDirection::Next;
        }
        else
        {
            nextFocusCandidate = (currentRadioButtonDO == firstFocusCandidate) ? lastFocusCandidate : pFocusManager->GetPreviousTabStop(currentRadioButtonDO);
            navigationDirection = DirectUI::FocusNavigationDirection::Previous;
        }

        std::vector<CDependencyObject*> dosConsidered;
        dosConsidered.push_back(currentRadioButtonDO);

        // Search for next focus candidate until we have looked at all focusable elements in the group.
        while (nextFocusCandidate != nullptr &&
            std::find(dosConsidered.begin(), dosConsidered.end(), nextFocusCandidate) == dosConsidered.end())
        {
            // Check to see if the nextFocusCandidate is a RadioButton.
            if (nextFocusCandidate->OfTypeByIndex<KnownTypeIndex::RadioButton>())
            {
                BOOLEAN haveCommonAncestor = FALSE;
                ctl::ComPtr<DependencyObject> nextFocusCandidatePeer;
                IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(nextFocusCandidate, &nextFocusCandidatePeer));
                if (currentParent)
                {
                    IFC_RETURN(static_cast<DependencyObject*>(currentParent.Get())->IsAncestorOf(nextFocusCandidatePeer.Get(), &haveCommonAncestor));
                }

                if (haveCommonAncestor)
                {
                    bool nextGroupNameExists = false;
                    xstring_ptr nextGroupName;
                    IFC_RETURN(nextFocusCandidatePeer.AsOrNull<RadioButton>()->GetGroupName(&nextGroupNameExists, &nextGroupName));

                    if (currentGroupName.Equals(nextGroupName))
                    {
                        const Focus::FocusMovementResult result = pFocusManager->SetFocusedElement(FocusMovement(nextFocusCandidate, navigationDirection, DirectUI::FocusState::Keyboard));
                        IFC_RETURN(result.GetHResult());
                        *wasFocused = true;
                        break;
                    }
                }
            }

            dosConsidered.push_back(nextFocusCandidate);

            if (moveForward)
            {
                nextFocusCandidate = pFocusManager->GetNextTabStop(nextFocusCandidate, FALSE);
                navigationDirection = DirectUI::FocusNavigationDirection::Next;
            }
            else
            {
                nextFocusCandidate = pFocusManager->GetPreviousTabStop(nextFocusCandidate);
                navigationDirection = DirectUI::FocusNavigationDirection::Previous;
            }
        }
    }
    return S_OK;
}

// Change to the correct visual state for the RadioButton.
_Check_return_ HRESULT
RadioButton::ChangeVisualState(
    // true to use transitions when updating the visual state, false
    // to snap directly to the new visual state.
    _In_ bool bUseTransitions)
{
    ctl::ComPtr<wf::IReference<bool>> spIsCheckedReference;
    HRESULT hr = S_OK;
    BOOLEAN bIsEnabled = FALSE;
    BOOLEAN bIsPressed = FALSE;
    BOOLEAN bIsPointerOver = FALSE;
    BOOLEAN bIsChecked = FALSE;
    BOOLEAN bIgnored = FALSE;
    xaml::FocusState focusState = xaml::FocusState_Unfocused;

    IFC(get_IsEnabled(&bIsEnabled));
    IFC(get_IsPressed(&bIsPressed));
    IFC(get_IsPointerOver(&bIsPointerOver));
    IFC(get_FocusState(&focusState));

    IFC(this->get_IsChecked(&spIsCheckedReference));
    if (spIsCheckedReference)
    {
        IFC(spIsCheckedReference->get_Value(&bIsChecked));
    }

    // Update the Interaction state group
    if (!bIsEnabled)
    {
        IFC(GoToState(bUseTransitions, L"Disabled", &bIgnored));
    }
    else if (bIsPressed)
    {
        IFC(GoToState(bUseTransitions, L"Pressed", &bIgnored));
    }
    else if (bIsPointerOver)
    {
        IFC(GoToState(bUseTransitions, L"PointerOver", &bIgnored));
    }
    else
    {
        IFC(GoToState(bUseTransitions, L"Normal", &bIgnored));
    }

    //Update the Check state group
    if (!spIsCheckedReference)
    {
        // Indeterminate
        IFC(GoToState(bUseTransitions, L"Indeterminate", &bIgnored));
    }
    else if (bIsChecked)
    {
        // Checked
        IFC(GoToState(bUseTransitions, L"Checked", &bIgnored));
    }
    else
    {
        // Unchecked
        IFC(GoToState(bUseTransitions, L"Unchecked", &bIgnored));
    }

    // Update the Focus group
    if (xaml::FocusState_Unfocused != focusState && bIsEnabled)
    {
        if (xaml::FocusState_Pointer == focusState)
        {
            IFC(GoToState(bUseTransitions, L"PointerFocused", &bIgnored));
        }
        else
        {
            IFC(GoToState(bUseTransitions, L"Focused", &bIgnored));
        }
    }
    else
    {
        IFC(GoToState(bUseTransitions, L"Unfocused", &bIgnored));
    }

Cleanup:
    RRETURN(hr);
}

