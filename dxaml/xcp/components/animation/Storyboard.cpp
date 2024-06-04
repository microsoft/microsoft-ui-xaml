// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Storyboard.h"
#include "TimelineCollection.h"
#include "DCompAnimationConversionContext.h"
#include "XStringBuilder.h"
#include "UIElement.h"
#include "VisualState.h"
#include "TypeTableStructs.h"
#include "DOPointerCast.h"

void CStoryboard::SynchronizeDCompAnimationAfterResume(double timeManagerTime)
{
    if (IsInActiveState())
    {
        SeekDCompAnimationInstances(timeManagerTime + m_rTimeDelta);
    }
}

void CStoryboard::FireDCompAnimationCompleted()
{
    OnDCompAnimationCompleted();

    for (auto item : *m_pChild)
    {
        static_cast<CTimeline*>(item)->OnDCompAnimationCompleted();
    }
}

bool CStoryboard::IsPaused() const
{
    return m_fIsPaused;
}

CompositionAnimationConversionResult CStoryboard::MakeCompositionAnimationVirtual(_Inout_ CompositionAnimationConversionContext* myContext)
{
    if (!m_storyboardTelemetryName.IsNull() && myContext->m_storyboardTelemetryName.IsNull())
    {
        myContext->m_storyboardTelemetryName = m_storyboardTelemetryName;
    }

    return __super::MakeCompositionAnimationVirtual(myContext);
}

xstring_ptr CStoryboard::GetNameForTracking(
    _In_opt_ CTransition* transition,
    _In_opt_ CUIElement* target,
    _In_opt_ CDependencyObject* dynamicTimeline,
    _Out_opt_ XUINT16* scenarioPriority)
{
    XStringBuilder nameBuilder;

    // Start with any name provided for the storyboard itself.
    if (!m_strName.IsNull())
    {
        IGNOREHR(nameBuilder.Append(m_strName));
    }

    // Add name from the target object.
    if (target != nullptr)
    {
        xstring_ptr targetName = target->GetUINameForTracing();
        IGNOREHR(nameBuilder.Append(targetName));
    }

    // Check if we are the child of a visual state.
    CVisualState* pVSParent = nullptr;
    CDependencyObject* pParent = GetParentInternal(/*fPublic*/false);
    pVSParent = do_pointer_cast<CVisualState>(pParent);

    // Add visual state info.
    if (pVSParent != nullptr && !pVSParent->m_strName.IsNull())
    {
        IGNOREHR(nameBuilder.Append(XSTRING_PTR_EPHEMERAL(L"+VS:")));
        IGNOREHR(nameBuilder.Append(pVSParent->m_strName));
    }

    // Add dynamic timeline name.
    if (dynamicTimeline != nullptr)
    {
        xstring_ptr strTypeName = dynamicTimeline->GetClassInformation()->GetFullName();
        IGNOREHR(nameBuilder.Append(XSTRING_PTR_EPHEMERAL(L"+DT:")));
        IGNOREHR(nameBuilder.Append(strTypeName));
    }

    // Add transition name.
    if (transition != nullptr && dynamicTimeline != nullptr)
    {
        xstring_ptr strTypeName = dynamicTimeline->GetClassInformation()->GetFullName();
        IGNOREHR(nameBuilder.Append(XSTRING_PTR_EPHEMERAL(L"+T:")));
        IGNOREHR(nameBuilder.Append(strTypeName));

        if (scenarioPriority != nullptr)
        {
            // Favor tracking transition animations.
            *scenarioPriority = c_AnimationTrackingTransitionPriority;

            if (strTypeName.Equals(XSTRING_PTR_EPHEMERAL(L"EntranceThemeTransition")))
            {
                // Further favor the entrance animations which tend to define the longer
                // launch and UI navigation experiences at a higher level. We may be starting
                // a number of accompanying content theme transitions for sub-elements
                // and we'd like to be tracking the higher level element/animation.
                *scenarioPriority = c_AnimationTrackingTransitionEntrancePriority;
            }
        }
    }

    xstring_ptr name;
    IGNOREHR(nameBuilder.DetachString(&name));
    return name;
}
