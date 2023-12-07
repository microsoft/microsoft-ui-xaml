// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PointerAnimationUsingKeyFrames.h"
#include "PointerKeyFrame.h"
#include "RoutedEventArgs.h"
#include "InputPointEventArgs.h"
#include <RootScale.h>

#undef min

using namespace DirectUI;

_Check_return_ HRESULT CPointerAnimationUsingKeyFrames::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
    )
{
    HRESULT hr = S_OK;
    CPointerKeyFrameCollection * pKeyFrames = NULL;
    CPointerKeyFrame * pPointerKeyFrame = NULL;

    // Create type
    CPointerAnimationUsingKeyFrames *pPointerAnimationUsingKeyFrames =
            new CPointerAnimationUsingKeyFrames(pCreate->m_pCore);

    // Create internal members
    IFC(CPointerKeyFrameCollection::Create(reinterpret_cast<CDependencyObject **>(&pKeyFrames), pCreate));
    IFC(pPointerAnimationUsingKeyFrames->SetValueByKnownIndex(KnownPropertyIndex::PointerAnimationUsingKeyFrames_KeyFrames, pKeyFrames));

    IFC(CPointerKeyFrame::Create(reinterpret_cast<CDependencyObject **>(&pPointerKeyFrame), pCreate ));
    pPointerAnimationUsingKeyFrames->m_pDPValue =
        pPointerKeyFrame->GetPropertyByIndexInline(KnownPropertyIndex::PointerKeyFrame_TargetValue);
    ASSERT(pPointerAnimationUsingKeyFrames->m_pDPValue != NULL);

    //do base-class initialization and return new object
    IFC(ValidateAndInit(pPointerAnimationUsingKeyFrames, ppObject));

    // Cleanup
    pPointerAnimationUsingKeyFrames = NULL;

Cleanup:
    ReleaseInterface(pKeyFrames);
    ReleaseInterface(pPointerAnimationUsingKeyFrames);
    ReleaseInterface(pPointerKeyFrame);
    RRETURN(hr);
}

// Compute the duration when one is not specified in the markup.
// Animations override this method to provide their own default.
void CPointerAnimationUsingKeyFrames::GetNaturalDuration(
    _Out_ DurationType *pDurationType,
    _Out_ XFLOAT *prDurationValue
    )
{
    ASSERT(pDurationType != NULL);
    ASSERT(prDurationValue != NULL);

    // Pointer animations last forever.
    *pDurationType = DirectUI::DurationType::Forever;
    *prDurationValue = 0.0f;
}

// Compute the duration to be used for the purposes of calculating progress.
void CPointerAnimationUsingKeyFrames::GetDurationForProgress(_Out_ DurationType *pDurationType, _Out_ XFLOAT *prDurationValue)
{
    ASSERT(pDurationType != NULL);
    ASSERT(prDurationValue != NULL);

    if (pDurationType == NULL || prDurationValue == NULL)
    {
        return;
    }

    *pDurationType = DirectUI::DurationType::TimeSpan;
    *prDurationValue = 1.0f;

    if (m_pKeyFrames != NULL)
    {
        // Find the largest source value and subtract the smallest source value from that.
        const auto& pointerKeyFrames = m_pKeyFrames->GetSortedCollection();
        XUINT32 frameCount = static_cast<XUINT32>(pointerKeyFrames.size());
        FAIL_FAST_ASSERT(frameCount == m_pKeyFrames->GetCount() && frameCount > 0);

        *prDurationValue = static_cast<CPointerKeyFrame*>(pointerKeyFrames[frameCount - 1])->m_pointerValue - static_cast<CPointerKeyFrame*>(pointerKeyFrames[0])->m_pointerValue;
    }
}

_Check_return_ HRESULT CPointerAnimationUsingKeyFrames::OnBegin()
{
    IFC_RETURN(CAnimation::OnBegin());

    // This is part of the newer behavior for Threshold, which is to disable the "scrub" animation.
    // Each time we start the animation, we reset this flag so that we query the InputManager only once
    // per run of the animation.  See ComputeLocalProgressAndTime() for the rest of the logic and comments.
    m_haveCachedPrimaryPointerPosition = false;
    m_haveCachedInputDeviceType = false;

    return S_OK;
}

// Sets the object to calculate the pointer position relative to.
_Check_return_ HRESULT CPointerAnimationUsingKeyFrames::SetRelativeToObject(
    _In_ CDependencyObject *pRelativeToObject)
{
    m_pRelativeToObject = xref::get_weakref(pRelativeToObject);

    return S_OK;
}

// Sets the name of the object to calculate the pointer position relative to.
_Check_return_ HRESULT CPointerAnimationUsingKeyFrames::SetRelativeToObjectName(
    _In_ const xstring_ptr_view& strRelativeToObjectName)
{
    IFC_RETURN(strRelativeToObjectName.Promote(
        &m_strRelativeToObjectName));

    return S_OK;
}

bool CPointerAnimationUsingKeyFrames::IsFinite()
{
    // Pointer animations are never finite.
    return false;
}

// Resolve target name, target property, and relative-to object name.
_Check_return_ HRESULT CPointerAnimationUsingKeyFrames::ResolveLocalTarget(
    _In_ CCoreServices *pCoreServices,
    _In_opt_ CTimeline *pParentTimeline
    )
{
    xref_ptr<CDependencyObject> pRelativeToObject;

    IFC_RETURN(CAnimation::ResolveLocalTarget(pCoreServices, pParentTimeline));

    if (!m_pRelativeToObject)
    {

        ResolveName(
            m_strRelativeToObjectName,
            pParentTimeline,
            pRelativeToObject.ReleaseAndGetAddressOf());

        m_pRelativeToObject = pRelativeToObject;
    }

    ASSERT(m_pRelativeToObject);

    return S_OK;
}

void CPointerAnimationUsingKeyFrames::ComputeLocalProgressAndTime(
    XDOUBLE rBeginTime,
    XDOUBLE rParentTime,
    DurationType durationType,
    XFLOAT rDurationValue,
    _In_ COptionalDouble *poptExpirationTime,
    bool hadIndependentAnimationLastTick,
    _Out_ bool *pIsInReverseSegment
    )
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    bool isInReverseSegment = false;
    XPOINTF lastPrimaryPointerPosition;
    XFLOAT value = 0.0f;
    XFLOAT startValue = 0.0f;
    xref_ptr<CDependencyObject> spRelativeToObject;
    ITransformer *pTransformer = nullptr;
    const auto& pointerKeyFrames = m_pKeyFrames->GetSortedCollection();

    CContentRoot* contentRoot = nullptr;

    if (CDependencyObject* target = GetTargetObjectWeakRef().lock())
    {
        contentRoot = VisualTree::GetContentRootForElement(GetTargetObjectWeakRef().lock());
    }
    else if(CDependencyObject* relative = m_pRelativeToObject.lock())
    {
        // If we don't have a target object, try to use the relative element as an anchor to
        // find the correct inputmanager
        contentRoot = VisualTree::GetContentRootForElement(relative);
    }
    else
    {
        // Fallback to using the pointeranimation itself to find the inputManger. If it isn't parented
        // to anything, we will fallback to the currently active content root
        contentRoot = VisualTree::GetContentRootForElement(this);
    }

    if (!m_haveCachedInputDeviceType)
    {
        m_inputDeviceType = contentRoot->GetInputManager().GetLastInputDeviceType();
        m_haveCachedInputDeviceType = true;
    }

    ASSERT(durationType == DirectUI::DurationType::TimeSpan);

    m_nIteration = 0;

    if (m_inputDeviceType == DirectUI::InputDeviceType::Mouse
        || m_inputDeviceType == DirectUI::InputDeviceType::Pen
        || m_inputDeviceType == DirectUI::InputDeviceType::Touch)
    {
        // The newer behavior starting with Threshold is to disable the "scrub" animation, which continues
        // tilting as the user drags the pointer around.  The scrub is disabled by caching the last primary
        // pointer position retrieved from the InputManager and using it for each frame.  The logic to query
        // InputManager is controlled by the m_haveCachedPrimaryPointerPosition flag.
        if (!m_haveCachedPrimaryPointerPosition)
        {
            IFC(contentRoot->GetInputManager().TryGetPrimaryPointerLastPosition(&m_cachedPrimaryPointerPosition, &m_haveCachedPrimaryPointerPosition))
        }

        // when using touch, ListViewItems/GridViewItems will delay the Pressed state
        // if the pointer is released early, then we go into the state but the InputManager has now destroyed the cached pointer information
        // in that case, we will apply a regular scale through the center
        if (m_inputDeviceType == DirectUI::InputDeviceType::Touch && !m_haveCachedPrimaryPointerPosition)
        {
            value = 0.5f;
        }

        if (m_haveCachedPrimaryPointerPosition)
        {
            lastPrimaryPointerPosition = m_cachedPrimaryPointerPosition;
            CFrameworkElement *pRelativeToElement = nullptr;

            // First, we turn the pointer position coordinates into values between 0 and 1, where 0 is the top-left corner and 1 is the bottom-right corner
            // of the element to calculate this relative to.
            spRelativeToObject = m_pRelativeToObject.lock();
            pRelativeToElement = do_pointer_cast<CFrameworkElement>(spRelativeToObject);

            if (pRelativeToElement != nullptr)
            {
                float width = pRelativeToElement->GetActualWidth();
                float height = pRelativeToElement->GetActualHeight();

                // if this the child of a ListViewItemPresenter, we use the minimum size of the child vs the parent
                CListViewBaseItemChrome* pListViewItemPresenter = do_pointer_cast<CListViewBaseItemChrome>(pRelativeToElement->GetTemplatedParent());
                if (pListViewItemPresenter)
                {
                    width = std::min(width, pListViewItemPresenter->GetActualWidth());
                    height = std::min(height, pListViewItemPresenter->GetActualHeight());
                }

                IFC(pRelativeToElement->TransformToRoot(&pTransformer));
                IFC(pTransformer->ReverseTransform(&lastPrimaryPointerPosition, &lastPrimaryPointerPosition, 1));

                lastPrimaryPointerPosition.x /= width;
                lastPrimaryPointerPosition.y /= height;

                XPOINTF normalizedPointerPosition;
                normalizedPointerPosition.x = MIN(MAX(lastPrimaryPointerPosition.x, 0), 1);
                normalizedPointerPosition.y = MIN(MAX(lastPrimaryPointerPosition.y, 0), 1);

                // The amount of tilt is distributed among the X and Y axes.
                // The way it is distributed depends on how much each axis contributes to the tilt.
                XFLOAT xMagnitude = XcpAbsF(normalizedPointerPosition.x - 0.5f);
                XFLOAT yMagnitude = XcpAbsF(normalizedPointerPosition.y - 0.5f);
                XFLOAT angleMagnitude = xMagnitude + yMagnitude;

                XINT32 angleDirection = 0;
                XFLOAT angleContribution = 0;

                if (m_pointerSource == PointerDirection::PointerDirection_BothAxes)
                {
                    value = (1 - angleMagnitude);
                }
                else
                {
                    if (m_pointerSource == PointerDirection::PointerDirection_YAxis)
                    {
                        angleDirection = normalizedPointerPosition.y > 0.5f ? 1 : -1;

                        if (angleMagnitude > 0)
                        {
                            angleContribution = yMagnitude / angleMagnitude;
                        }
                    }
                    else if (m_pointerSource == PointerDirection::PointerDirection_XAxis)
                    {
                        angleDirection = normalizedPointerPosition.x > 0.5f ? 1 : -1;

                        if (angleMagnitude > 0)
                        {
                            angleContribution = xMagnitude / angleMagnitude;
                        }
                    }

                    // A value of 0.5 means the element does not tilt.
                    // A value less than 0.5 means the element tilts to the left or the top, depending on the axis.
                    // A value greater than 0.5 means the element tilts to the right or the bottom, depending on the axis.
                    // Now, starting from the center, the angular magnitude is added after scaling it by the contribution of the appropriate axis.
                    value = 0.5f + angleMagnitude * angleContribution * angleDirection / 2.0f;
                }
            }
        }
    }
    else
    {
        // If the input device type is not mouse, pen or touch, the animation should appear
        // as if the element was pressed at its center. This is achieved by setting a magnitude of 0.5
        // for the rotation on the X or Y axis and a magnitude of 1.0 for the scale.
        value = (m_pointerSource == PointerDirection::PointerDirection_BothAxes) ? 1.0f : 0.5f;
    }

    // Find the smallest source value and subtract it from the value.
    FAIL_FAST_ASSERT(pointerKeyFrames.size() > 0);
    startValue = static_cast<CPointerKeyFrame*>(pointerKeyFrames[0])->m_pointerValue;

    // Clamp the current time such that it remains between 0 and the duration.
    m_rCurrentTime = MIN(MAX(value - startValue, 0), rDurationValue);
    m_rCurrentProgress = static_cast<XFLOAT>(m_rCurrentTime / rDurationValue);

    *pIsInReverseSegment = isInReverseSegment;

Cleanup:
    ReleaseInterface(pTransformer);
}

// Find the element that is a valid target for primitive composition
_Check_return_ HRESULT CPointerAnimationUsingKeyFrames::FindIndependentAnimationTargetsRecursive(
    _In_ CDependencyObject *pTargetObject,
    _In_opt_ CDependencyObject *pSender,
    _Inout_opt_ IATargetVector *pIATargets,
    _Out_ bool *pIsIndependentAnimation
    )
{
    // Pointer animations are always dependent.
    *pIsIndependentAnimation = FALSE;
    RRETURN(S_OK);
}

bool CPointerAnimationUsingKeyFrames::CanRequestTicksWhileActive()
{
    // Pointer animations are input-based rather than time-based and should not be requesting any frames. If input comes in,
    // then it should have dirtied the tree and requested a tick that way. If the target element changed size or position,
    // then we would have dirtied the tree for rendering and requested a tick that way. In either case, this animation would
    // be evaluated again when we update all animations as part of that requested tick.
    return false;
}