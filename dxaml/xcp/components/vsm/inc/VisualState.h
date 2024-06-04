// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CDependencyObject.h>
#include <ShareableDependencyObject.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <storyboard.h>
#include "VisualStateToken.h"
#include "VisualStateSetterHelper.h"

class CSetterBaseCollection;
class CTemplateContent;
class CStateTriggerCollection;

class CVisualState final
    : public CDependencyObject
{
public:

    CVisualState(CCoreServices *pCore);

    ~CVisualState() override;

    DECLARE_CREATE(CVisualState);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CVisualState>::Index;
    }

    _Check_return_ HRESULT GetValue(
        _In_ const CDependencyProperty *pdp,
        _Out_ CValue *pValue) override;

    _Check_return_ HRESULT GetStoryboard(_Outptr_result_maybenull_ CStoryboard**ppStoryboard);

    static _Check_return_ HRESULT Storyboard(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _In_reads_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult);

    // We override these three methods completely because our owned storyboard isn't
    // field-backed and isn't automatically Enter/Leaved by DO's own EnterImpl.
    _Check_return_ HRESULT NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh = false) override;
    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject* namescopeOwner, EnterParams params) override;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject* namescopeOwner, LeaveParams params) override;

    bool ReferenceTrackerWalkCore(
        _In_ DirectUI::EReferenceTrackerWalkType walkType,
        _In_ bool isRoot,
        _In_ bool shouldWalkPeer) override;

    // We override this because if an animation is added inside of a VisualState we need to be able to
    // grab the templated parent.
    CDependencyObject* GetTemplatedParent() override;

    CStoryboard *m_pStoryboard;
    CSetterBaseCollection *m_setters;
    CStateTriggerCollection* m_pStateTriggerCollection;

#pragma region Legacy VSM
public:
    CTemplateContent *m_pDeferredStoryboard;
    CTemplateContent *m_pDeferredSetters;

    CDependencyObject* GetStandardNameScopeParent() override {
        return m_pTemporaryNamescopeParent ? m_pTemporaryNamescopeParent : CDependencyObject::GetStandardNameScopeParent();
    }

    void SetTemporaryNamescopeParent(_In_ CDependencyObject* pTemporaryNamescopeParent)
    {
        ASSERT(!pTemporaryNamescopeParent || pTemporaryNamescopeParent && m_pTemporaryNamescopeParent == NULL);
        ReleaseInterface(m_pTemporaryNamescopeParent);
        m_pTemporaryNamescopeParent = pTemporaryNamescopeParent;
        AddRefInterface(m_pTemporaryNamescopeParent);
    }

    _Check_return_ VisualStateToken GetVisualStateToken();
    void SetOptimizedIndex(_In_ int index);

protected:
    // We override this because typically only FrameworkElements are aware of their
    // templated parent. We need to be to support namescope resolution from our children
    // optimized Storyboard and VisualTransition collections.
    void SetTemplatedParentImpl(_In_ CDependencyObject* parent) final;

private:
    xref::weakref_ptr<CDependencyObject> m_templatedParent;

    _Check_return_ HRESULT TryDelayCreateLegacyStoryboard();
    _Check_return_ HRESULT TryDelayCreateLegacyPropertySetters();

    _Check_return_ HRESULT CreateDeferredItemFromTemplateContent(
        _In_ CTemplateContent* pTemplateContent,
        _Outptr_ CDependencyObject** result);

    CDependencyObject* m_pTemporaryNamescopeParent;

    // Because we're using the method-based Getter/Setter in VisualState we unfortunately
    // find ourselves in the business of doing the Enter/Leave on our owned Storyboard ourselves.
    // We'll encapsalate that logic into these methods. In the future it would be awesome to
    // take this logic and make it a common part of DO, used by SetValue and derived classes
    // like CVisualState which do "fancy" stuff.
    void ParentAndEnterDO(_In_ CDependencyObject* object);
    void UnparentAndLeaveDO(_In_ CDependencyObject* object);

    VisualStateToken m_token;
#pragma endregion
};

