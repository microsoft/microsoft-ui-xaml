// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "KeytipAutomaticPlacementAlgorithm.h"
#include "KeyTip.h"
#include "Microsoft.UI.Windowing.h"
#include "Microsoft.UI.h"

using namespace DirectUI;
/*

        We try to place keytips in a positions such that cost is minimized. Costs are always positive and are
        computed as follows:

        KT : KeyTip

        Case 1: Keytip does not occlude anything. Cost is then 0.
        -----------
        |Parent   |---
        |Object   |KT|
        |_________|__|

        Case 2: Keytip occludes a focusable object that does not have access keys enabled.
        Cost is c_focusableElementCost

        -----------  ------------
        |Parent   |---           |
        |Object   |KT| Focusable |
        |_________|__| Object    |
                    |____________|

        Case 3: Keytip occludes its parent object.
        Cost is c_parentOcclusionCost

        ----------------
        |Parent   ---   |
        |Object   |KT|  |
        |_________|__|__|

        Case 4: Keytip occludes an object with access keys enabled.
        Cost is c_activeElementCost

        -----------  --------------
        |Parent   |--- Object with |
        |Object   |KT| Access Keys |
        |_________|__| Enabled     |
                    |______________|

        Case 5: Keytip occludes another keytip.
        Cost is c_keytipOcclusionCost

        -----------
        |Parent   |------
        |Object 1 |KT1  |   ------------
        |_________|___|-|--|   Parent  |
                      | KT2|  Object 2 |
                      |____|___________|


        If multiple possible keytip positions exist with equal costs, we look at keytip positions of nearby objects to help choose a
        'better' position.

*/

struct ElementWithCost {
    XRECTF_RB element;
    int cost;
};

static const float c_maxClosenessDistance = 50;
static const int c_activeElementCost = 10;
static const int c_focusableElementCost = 1;
static const int c_parentOcclusionCost = 3;
static const int c_keytipOcclusionCost = 10000;
static const int c_numIterations = 15;
static const int c_manualAlignmentBonus = 2; //Tie breaking Bonus for being aligned with a manually positioned element

// Determines if r1 is within c_maxClosenessDistance of r2 along either
// the horizontal or vertical axis.
bool AreClose(_In_ const XRECTF_RB& r1, _In_ const XRECTF_RB& r2, _In_ bool horizontal)
{
    XRECTF_RB r1Shifted;

    if (DoRectsIntersect(r1, r2))
    {
        return true;
    }

    //Look for horizontal closeness
    if (horizontal)
    {
        const bool closeInX =
            ((0 <= (r2.left - r1.right)) && ((r2.left - r1.right) < c_maxClosenessDistance)) || //r1 to the left of r2
            ((0 <= (r1.left - r2.right)) && (r1.left - r2.right < c_maxClosenessDistance));

        r1Shifted.left = r2.left;
        r1Shifted.right = r2.right;
        r1Shifted.top = r1.top;
        r1Shifted.bottom = r1.bottom;

        const bool inHorizontalShadow = DoRectsIntersect(r1Shifted, r2);

        return closeInX && inHorizontalShadow;
    }

    // If not horizontal, look for vertical closeness
    const bool closeInY =
        ((0 <= (r2.top - r1.bottom)) && ((r2.top - r1.bottom) < c_maxClosenessDistance)) || //r1 above r2
        ((0 <= (r1.top - r2.bottom)) && ((r1.top - r2.bottom) < c_maxClosenessDistance));   //r2 above r1

    r1Shifted.top = r2.top;
    r1Shifted.bottom = r2.bottom;
    r1Shifted.left = r1.left;
    r1Shifted.right = r1.right;

    const bool inVerticalShadow = DoRectsIntersect(r1Shifted, r2);

    return closeInY && inVerticalShadow;
}

struct ScreenBoundsHelper
{
    ScreenBoundsHelper(const XRECTF_RB& screenBounds, _In_opt_ CXamlIslandRoot* island)
        : m_screenBounds(screenBounds)
        , m_xamlIslandRoot(island)
        {}

    XRECTF_RB GetScreenBounds(const XRECTF_RB& elementBounds);

private:
    CXamlIslandRoot* m_xamlIslandRoot {};
    XRECTF_RB m_screenBounds {};

    // Cache of monitor bounds we know about, in screen-physical coordinates.  The cache only lasts for the duration
    // of one round of KeyTips; ever time the user moves to a new scope, we'll use a new cache.
    Jupiter::stack_vector<::RECT,2> m_knownMonitorBoundsScreenPhysical;
};

XRECTF_RB GetKeyTipPosition(
    _In_ const xref_ptr<CPopup> popUp,
    _In_ const XRECTF_RB& AKEnabledElement,
    _In_ const XRECTF_RB& KeyTip,
    _In_ const KeyTipPlacementMode position,
    _In_ float horizOffset,
    _In_ float vertOffset,
    _In_ bool isRightToLeft,
    _In_ ScreenBoundsHelper& screenBoundsHelper)
{
    const float keytipWidth = KeyTip.right - KeyTip.left;
    const float keytipHeight = KeyTip.bottom - KeyTip.top;

    XRECTF_RB finalPosition = {};

    // popupWindowbridge->get_DisplayID(&id);
    //Do not try and place a keytip with height or width 0
    if (keytipWidth == 0 || keytipHeight == 0)
    {
        return finalPosition;
    }

    switch (position)
    {
        case DirectUI::KeyTipPlacementMode::Left:
            finalPosition.left = isRightToLeft ? AKEnabledElement.right : AKEnabledElement.left - keytipWidth;
            finalPosition.top = (AKEnabledElement.top + AKEnabledElement.bottom) / 2.0f - keytipHeight / 2.0f;
            break;
        case DirectUI::KeyTipPlacementMode::Right:
            finalPosition.left = isRightToLeft ? AKEnabledElement.left - keytipWidth : AKEnabledElement.right;
            finalPosition.top = (AKEnabledElement.top + AKEnabledElement.bottom) / 2.0f - keytipHeight / 2.0f;
            break;
        case DirectUI::KeyTipPlacementMode::Top:
            finalPosition.left = (AKEnabledElement.left + AKEnabledElement.right) / 2.0f - keytipWidth / 2.0f;
            finalPosition.top = AKEnabledElement.top - keytipHeight;
            break;
        case DirectUI::KeyTipPlacementMode::Bottom:
            finalPosition.left = (AKEnabledElement.left + AKEnabledElement.right) / 2.0f - keytipWidth / 2.0f;
            finalPosition.top =  AKEnabledElement.bottom;
            break;

        case DirectUI::KeyTipPlacementMode::Center:
            finalPosition.left = (AKEnabledElement.left + AKEnabledElement.right) / 2.0f - keytipWidth / 2.0f;
            finalPosition.top =  (AKEnabledElement.top + AKEnabledElement.bottom) / 2.0f - keytipHeight / 2.0f;
            break;
        default:
            EmptyRectF(&finalPosition);
            return finalPosition;
    }

    finalPosition.right = finalPosition.left + keytipWidth;
    finalPosition.bottom = finalPosition.top + keytipHeight;

    horizOffset = (isRightToLeft ? (-1):1) * horizOffset;

    // Adjust for offsets
    finalPosition.left += horizOffset;
    finalPosition.right += horizOffset;
    finalPosition.top += vertOffset;
    finalPosition.bottom += vertOffset;

    const XRECTF_RB screenBounds = screenBoundsHelper.GetScreenBounds(AKEnabledElement);

    //If the keytip is off-screen to the left, adjust its position
    if (screenBounds.left > finalPosition.left)
    {
        finalPosition.left = screenBounds.left;
        finalPosition.right = finalPosition.left + keytipWidth;
    }

    //If the keytip is off-screen to the right, adjust its position
    if (screenBounds.right < finalPosition.right)
    {
        finalPosition.right = screenBounds.right;
        finalPosition.left = finalPosition.right - keytipWidth;
    }

    //If the keytip is off-screen to the top, adjust its position
    if (screenBounds.top > finalPosition.top)
    {
        finalPosition.top = screenBounds.top;
        finalPosition.bottom = finalPosition.top + keytipHeight;
    }

    //If the keytip is off-screen to the bottom, adjust its position
    if (screenBounds.bottom < finalPosition.bottom)
    {
        finalPosition.bottom = screenBounds.bottom;
        finalPosition.top = finalPosition.bottom - keytipHeight;
    }

    //If the keytip is still off-screen, set its width and height to 0
    // (aka do not display)
    if (screenBounds.left > finalPosition.left
        || screenBounds.right < finalPosition.right
        || screenBounds.top > finalPosition.top
        || screenBounds.bottom < finalPosition.bottom)
    {
        EmptyRectF(&finalPosition);
    }

    return finalPosition;
}

DirectUI::KeyTipPlacementMode ChoosePlacement(
    _In_ const int currentElementIndex,
    _In_ const KeyTipPlacementMode positionA,
    _In_ const KeyTipPlacementMode positionB,
    _Inout_ std::vector<KeyTip>& objectBounds,
    _In_ const bool tryHorizontalAndVertical)
{
    int positionAFreq = 0;
    int positionBFreq = 0;
    int numNearbyElements = 0;
    bool positionAAlignedToManual = false;
    bool positionBAlignedToManual = false;

    XRECTF_RB currentObj = objectBounds.at(currentElementIndex).ObjectBounds;

    for (const auto& element : objectBounds)
    {
        if (element.PlacementMode != KeyTipPlacementMode::Auto &&
            AreClose(currentObj, element.ObjectBounds, tryHorizontalAndVertical))
        {
            numNearbyElements++;
            if (element.PlacementMode == positionA)
            {
                positionAFreq++;
                if (element.AlignedWithManuallyPositioned)
                {
                    positionAFreq += c_manualAlignmentBonus;
                    positionAAlignedToManual = true;
                }
            }
            else if (element.PlacementMode == positionB)
            {
                positionBFreq++;
                if (element.AlignedWithManuallyPositioned)
                {
                    positionBFreq += c_manualAlignmentBonus;
                    positionBAlignedToManual = true;
                }
            }
        }
    }

    if (positionBFreq == 0
        && positionAFreq == 0)
    {
        if (tryHorizontalAndVertical)
        {
            return ChoosePlacement(currentElementIndex,
                positionA,
                positionB,
                objectBounds,
                false /*tryHorizontalAndVertical*/);
        }
        //If tryHorizontalAndVertical is false, we know we are looking at columns
        // In this case, try and favor left and right over up and down.
        else if (numNearbyElements > 0)
        {
            if (positionA == KeyTipPlacementMode::Left || positionA == KeyTipPlacementMode::Right)
            {
                return positionA;
            }
            else if (positionB == KeyTipPlacementMode::Left || positionB == KeyTipPlacementMode::Right)
            {
                return positionB;
            }
        }
        return KeyTipPlacementMode::Auto;
    }
    else if (positionBFreq > positionAFreq)
    {
        if (positionBAlignedToManual)
        {
            objectBounds[currentElementIndex].AlignedWithManuallyPositioned = true;
        }
        return positionB;
    }

    if (positionAAlignedToManual)
    {
        objectBounds[currentElementIndex].AlignedWithManuallyPositioned = true;
    }
    return positionA;
}

void AddScoresToBounds(
    _In_ const std::vector<KeyTip>& activeElementBounds,
    _In_ const std::vector<XRECTF_RB>& focusableElementBounds,
    _Inout_ std::vector<ElementWithCost>& rectsOnScreen)
{
    rectsOnScreen.reserve(activeElementBounds.size() + focusableElementBounds.size());

    for (const auto& activeElement : activeElementBounds)
    {
        ElementWithCost boundsWithCost = { activeElement.ObjectBounds, c_activeElementCost };
        rectsOnScreen.push_back(boundsWithCost);
    }

    for (const auto& focusableElement : focusableElementBounds)
    {
        ElementWithCost boundsWithCost = { focusableElement, c_focusableElementCost };
        rectsOnScreen.push_back(boundsWithCost);
    }
}

int Cost(
    _In_ const XRECTF_RB& position,
    _In_ std::vector <ElementWithCost>& elementsOnScreen,
    _In_ std::vector <ElementWithCost>& keyTipsOnScreen,
    _In_ const unsigned int autopositionedKeytipNumber)
{
    int score = 0;

    for (unsigned i = 0; i < elementsOnScreen.size(); i++)
    {
        XRECTF_RB objToCheck;
        objToCheck = elementsOnScreen.at(i).element;
        if (DoRectsIntersect(position, objToCheck))
        {
            //This comparison is dependent on elementsOnScreen corresponding with objectBounds
            if (i == autopositionedKeytipNumber)
            {
                score += c_parentOcclusionCost;
            }
            else
            {
                score += elementsOnScreen[i].cost;
            }
        }
    }

    for (unsigned i = 0; i < keyTipsOnScreen.size(); i++)
    {
        XRECTF_RB objToCheck;
        objToCheck = keyTipsOnScreen.at(i).element;
        if (DoRectsIntersect(position, objToCheck))
        {
            //If this keytip overlaps with its previous placement,
            // do not penalize it.
            if (i != autopositionedKeytipNumber)
            {
                score += elementsOnScreen[i].cost;
            }
        }
    }

    return score;
}

void PositionKeytipIteration(
    _Inout_ std::vector<KeyTip>& objectBounds,
    _Inout_ std::vector<ElementWithCost>& elementsOnScreenWithScores,
    _Inout_ std::vector<ElementWithCost>& keyTipsOnScreenWithScores,
    _In_ ScreenBoundsHelper& screenBoundsHelper
)
{

    for (unsigned int i = 0; i < objectBounds.size(); i++)
    {
        KeyTip& currentObject = objectBounds[i];
        if (!currentObject.ManuallyPositioned)
        {
            int placementCost = c_keytipOcclusionCost;
            DirectUI::KeyTipPlacementMode finalPos = KeyTipPlacementMode::Auto;
            XRECTF_RB finalKeytipPosition = { 0,0,0,0 };
            currentObject.PlacementMode = finalPos;

            for (int posAsInt = static_cast<int>(KeyTipPlacementMode::Bottom); posAsInt <= static_cast<int>(KeyTipPlacementMode::Center); posAsInt++)
            {
                const KeyTipPlacementMode positionToTry = static_cast<KeyTipPlacementMode>(posAsInt);
                XRECTF_RB keytipPosition = GetKeyTipPosition(
                    currentObject.Popup,
                    currentObject.ObjectBounds,
                    currentObject.KeytipBounds,
                    positionToTry,
                    0 /*horizontalOffset*/,
                    0 /*verticalOffset*/,
                    currentObject.IsRightToLeft,
                    screenBoundsHelper);

                const int oldCost = placementCost;
                placementCost = Cost(keytipPosition, elementsOnScreenWithScores, keyTipsOnScreenWithScores, i);
                if (placementCost < oldCost)
                {
                    finalKeytipPosition = keytipPosition;
                    finalPos = positionToTry;
                }
                //If there are multiple equally expensive options, choose one to best align with nearby placed keytips
                else if (placementCost == oldCost && placementCost < c_keytipOcclusionCost)
                {
                    const KeyTipPlacementMode chosenPlacement = ChoosePlacement(i, finalPos, positionToTry, objectBounds, true /*tryHorizontalAndVertical*/);

                    if (chosenPlacement == positionToTry)
                    {
                        finalKeytipPosition = keytipPosition;
                        finalPos = positionToTry;
                    }
                }
                else
                {
                    placementCost = oldCost;
                }
            }

            if (finalKeytipPosition.IsUniform() || c_keytipOcclusionCost <= placementCost)
            {
                finalPos = KeyTipPlacementMode::Hidden;
            }

            currentObject.KeytipBounds = finalKeytipPosition;
            currentObject.PlacementMode = finalPos;
            keyTipsOnScreenWithScores[i] = { finalKeytipPosition, c_keytipOcclusionCost };
        }
        else
        {
            keyTipsOnScreenWithScores[i] = { currentObject.KeytipBounds, 0 };
        }
    }
}

void KeytipAutomaticPlacementAlgorithm::PositionKeyTips(
    _Inout_ std::vector<KeyTip>& objectBounds,
    _In_ const XRECTF_RB& screenBounds,
    _In_ std::vector<XRECTF_RB>& focusableElementBounds,
    _In_opt_ CXamlIslandRoot* xamlIslandRoot,
    bool enableMonitorDetection)
{
    std::vector<ElementWithCost> fixedElementsOnScreenWithScores;
    AddScoresToBounds(objectBounds, focusableElementBounds, fixedElementsOnScreenWithScores);
    std::vector<ElementWithCost> movableKeyTipsOnScreenWithScores;

    movableKeyTipsOnScreenWithScores.resize(objectBounds.size());

    ScreenBoundsHelper screenBoundsHelper {screenBounds, enableMonitorDetection ? xamlIslandRoot : nullptr};

    // Place all explicitly positioned keytips
    for (auto& object : objectBounds)
    {
        if (object.PlacementMode != KeyTipPlacementMode::Auto)
        {
            XRECTF_RB keytipPosition = GetKeyTipPosition(
                object.Popup,
                object.ObjectBounds,
                object.KeytipBounds,
                object.PlacementMode,
                object.HorizontalOffset,
                object.VerticalOffset,
                object.IsRightToLeft,
                screenBoundsHelper);

            object.KeytipBounds = keytipPosition;

            ElementWithCost keytipWithCost;
            keytipWithCost.element = keytipPosition;
            keytipWithCost.cost = c_keytipOcclusionCost;
            fixedElementsOnScreenWithScores.push_back(keytipWithCost);
            object.ManuallyPositioned = true;
            //Any keytip that is manually positioned is trivially aligned with a manually positioned keytip
            object.AlignedWithManuallyPositioned = true;
        }
        else
        {
            object.ManuallyPositioned = false;
            object.AlignedWithManuallyPositioned = false;
        }
    }

    // For elements to be autoplaced, iteratively look for 'good' positions
    for (int tryCounter = 0; tryCounter < c_numIterations; tryCounter++)
    {
        PositionKeytipIteration(
            objectBounds,
            fixedElementsOnScreenWithScores,
            movableKeyTipsOnScreenWithScores,
            screenBoundsHelper
        );
    }
}

XRECTF_RB ToClientLogicalXRect(_In_ CXamlIslandRoot* xamlIslandRoot, const RECT& screenPhysical)
{
    POINT workingTopLeft = {screenPhysical.left, screenPhysical.top};
    xamlIslandRoot->ScreenPhysicalToClientLogical(workingTopLeft);

    POINT workingBottomRight = {screenPhysical.right, screenPhysical.bottom};
    xamlIslandRoot->ScreenPhysicalToClientLogical(workingBottomRight);

    return {
        static_cast<float>(workingTopLeft.x),
        static_cast<float>(workingTopLeft.y),
        static_cast<float>(workingBottomRight.x),
        static_cast<float>(workingBottomRight.y)};
}

XRECTF_RB ToScreenPhysicalXRect(_In_ CXamlIslandRoot* xamlIslandRoot, const RECT& clientLogical)
{
    POINT workingTopLeft = {clientLogical.left, clientLogical.top};
    xamlIslandRoot->ClientLogicalToScreenPhysical(workingTopLeft);

    POINT workingBottomRight = {clientLogical.right, clientLogical.bottom};
    xamlIslandRoot->ClientLogicalToScreenPhysical(workingBottomRight);

    return {
        static_cast<float>(workingTopLeft.x),
        static_cast<float>(workingTopLeft.y),
        static_cast<float>(workingBottomRight.x),
        static_cast<float>(workingBottomRight.y)};
}

RECT ToRect( const XRECTF_RB& xRect)
{
    return { static_cast<LONG>(xRect.left),
                static_cast<LONG>(xRect.top),
                static_cast<LONG>(xRect.right),
                static_cast<LONG>(xRect.bottom) };
}

bool operator==(const RECT& r1, const RECT& r2)
{
    return r1.left == r2.left && r1.top == r2.top && r1.right == r2.right && r1.bottom == r2.bottom;
}

XRECTF_RB ScreenBoundsHelper::GetScreenBounds(const XRECTF_RB& elementBounds)
{
    if (!m_xamlIslandRoot)
    {
        return m_screenBounds;
    }

    // We want to put each KeyTip within the bounds of the monitor its parent element appears on.  So, we use the
    // bounds of the parent element, figure out which monitor it's on, and then find the bounds of that monitor. The
    // placement algorithm will later ensure the KeyTip is placed entirely within the bounds this function returns.
    const XRECTF_RB elementBoundsScreenPhysical = ToScreenPhysicalXRect(m_xamlIslandRoot, ToRect(elementBounds));
    wgr::RectInt32 elementRect = { static_cast<int>(elementBoundsScreenPhysical.left),
        static_cast<int>(elementBoundsScreenPhysical.top),
        static_cast<int>(elementBoundsScreenPhysical.right - elementBoundsScreenPhysical.left),
        static_cast<int>(elementBoundsScreenPhysical.bottom - elementBoundsScreenPhysical.top) };

    for (RECT monitorRectScreenPhysical : m_knownMonitorBoundsScreenPhysical.m_vector)
    {
        POINT topLeft = {
            static_cast<int>(elementBounds.left),
            static_cast<int>(elementBounds.top)};
        m_xamlIslandRoot->ClientLogicalToScreenPhysical(topLeft);

        POINT botRight = {
            static_cast<int>(elementBounds.right),
            static_cast<int>(elementBounds.bottom)};
        m_xamlIslandRoot->ClientLogicalToScreenPhysical(botRight);

        // If both topLeft and bottomRight points of the rectangle are in Rect,
        // then the elements should be completely be in bounds
        if (::PtInRect(&monitorRectScreenPhysical, topLeft) && ::PtInRect(&monitorRectScreenPhysical, botRight))
        {
            // Cache hit -- return the monitor bounds we found earlier.
            return ToClientLogicalXRect(m_xamlIslandRoot, monitorRectScreenPhysical);
        }
    }

    ctl::ComPtr<ixp::IDisplayAreaStatics> displayAreaStatics;
    FAIL_FAST_IF_FAILED(ctl::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Windowing_DisplayArea).Get(),
        &displayAreaStatics));
    ctl::ComPtr<ixp::IDisplayArea> displayArea;
    FAIL_FAST_IF_FAILED(displayAreaStatics->GetFromRect(elementRect, ixp::DisplayAreaFallback::DisplayAreaFallback_None, &displayArea));
    wgr::RectInt32 workArea;
    FAIL_FAST_IF_FAILED(displayArea->get_WorkArea(&workArea));

    // Cache the monitor bounds for next time, so we don't have to keep calling MonitorFromPoint for each KeyTip.
    // Note there is a degenerate case here: if many of the AK elements are off-screen, we will keep missing the
    // cache, and call ::MonitorFromPoint a bunch of times.
    const ::RECT monitorRectScreenPhysical = { workArea.X, workArea.Y, workArea.X + workArea.Width, workArea.Y + workArea.Height };
    auto findIt = std::find(
        m_knownMonitorBoundsScreenPhysical.m_vector.begin(),
        m_knownMonitorBoundsScreenPhysical.m_vector.end(),
        monitorRectScreenPhysical);
    if (findIt == m_knownMonitorBoundsScreenPhysical.m_vector.end())
    {
        m_knownMonitorBoundsScreenPhysical.m_vector.push_back(monitorRectScreenPhysical);
    }
    return ToClientLogicalXRect(m_xamlIslandRoot, monitorRectScreenPhysical);

}
