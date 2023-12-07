// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CCoreSerivces;
class CDependencyObject;
class CVisualTransition;
class CFrameworkElement;
class CVisualState;
class CVisualStateGroup;

// Data Abstraction containing the objects necessary to complete a visual
// transition after the intermediate storyboards have completed
class VisualTransitionCompletedData
{
public:
    static _Check_return_ HRESULT Create(
        _In_ int groupIndex,
        _In_ int stateIndex,
        _In_ CVisualTransition* transition,
        _Outptr_ VisualTransitionCompletedData** transitionCompletedData);

    CVisualTransition* GetVisualTransition() const;
    bool GetHasBeenProcessed() const { return m_hasBeenProcessed; }
    void SetHasBeenProcessed(_In_ bool processed) { m_hasBeenProcessed = processed; }

    int GetGroupIndex() const { return m_groupIndex; }
    int GetStateIndex() const { return m_stateIndex; }

private:
    VisualTransitionCompletedData() = default;

    xref::weakref_ptr<CVisualTransition> m_pTransitionWeakRef;
    int m_groupIndex = -1;
    int m_stateIndex = -1;
    bool m_hasBeenProcessed = false;
    bool m_peggedPeer = false;

#pragma region Legacy VSM Code
private:
    xref::weakref_ptr<CControl> m_pRootWeakRef;
    xref::weakref_ptr<CFrameworkElement> m_pImplRootWeakRef;
    xref::weakref_ptr<CVisualState> m_pOldVisualStateWeakRef;
    xref::weakref_ptr<CVisualState> m_pNewVisualStateWeakRef;
    xref::weakref_ptr<CVisualStateGroup> m_pGroupWeakRef;

public:
    CValue m_EventListenerToken;
    ~VisualTransitionCompletedData();

    static _Check_return_ HRESULT Create(
        _In_ CVisualTransition* pTransition,
        _In_ CControl*          pRoot,
        _In_ CFrameworkElement* pImplRoot,
        _In_opt_ CVisualState*  pOldVisualState,
        _In_ CVisualState*      pNewVisualState,
        _In_ CVisualStateGroup* pGroup,
        _Outptr_ VisualTransitionCompletedData** ppVisualTransitionCompletedData);

    CControl* GetRoot() const;

    CFrameworkElement* GetImplRoot() const;

    CVisualState* GetNewVisualState() const;

    CVisualState* GetOldVisualState() const;

    CVisualStateGroup* GetVisualStateGroup() const;
#pragma endregion
};