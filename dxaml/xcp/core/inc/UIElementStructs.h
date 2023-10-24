// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Indexes.g.h"
#include "enumdefs.g.h"
#include "paltypes.h"
#include "uielement.h"

class HWRealization;
class HitTestPolygon;

static const XUINT16 EnteredInThisTick = 0;
static const XUINT16 LeftInThisTick = 0;

enum UnloadCleanup
{
    UC_Null                       = 0x00000000,
    UC_OnRemoveFromCollection     = 0x00000001,
    UC_ClearLogic                 = 0x00000002,
    UC_ContentControlLeave        = 0x00000004,
    UC_OnRemoveDOFromCollection   = 0x00000008,
    UC_UnlinkContainer            = 0x00000010,
    UC_RemoveLogicalParent        = 0x00000020,
    UC_ReleaseParentPopup         = 0x00000040,
    UC_REFERENCE_ThemeTransition     = 0x00000080,
    UC_REFERENCE_ConnectedAnimation  = 0x00000100,
    UC_REFERENCE_ImplicitAnimation   = 0x00000200,
};

struct IDirtyRegionAccumulator
{
    _Check_return_ virtual HRESULT AddPreDirtyRegion(_In_ const XRECTF_RB& Region) = 0;
    _Check_return_ virtual HRESULT AddPostDirtyRegion(_In_ const XRECTF_RB& Region) = 0;

    _Check_return_ virtual HRESULT PushPostClippingRegion(_In_ const XRECTF_RB& Region) = 0;
    _Check_return_ virtual HRESULT PopPostClippingRegion() = 0;
};

struct IDirtyRegionQuery
{
    virtual bool IsRegionDirty(_In_ const XRECTF_RB& Region) const = 0;
};

#include "RenderParams.h"

class UIElementCollectionUnloadingStorage
{
public:
    UIElementCollectionUnloadingStorage();
    UINT Count();
    bool ContainsKey(_In_ CUIElement* element);
    UnloadCleanup* Get(_In_ CUIElement* element);
    void Add(_In_ CUIElement* element, UnloadCleanup* unloadCleanup);
    UnloadCleanup* Remove(_In_ CUIElement* element);
    void Clear();

    CUIElement* m_pUnloadingElementBeingRemoved;
    CUIElement* m_pUnloadingRoot;
    typedef std::vector<std::pair<CUIElement*, UnloadCleanup*>> UnloadingMap;
    UnloadingMap m_unloadingElements;
};

//------------------------------------------------------------------------
//
//  Class:  CHitTestResults
//
//  Synopsis:
//      A list of CUIElement* that holds a hit-test result.
//
//------------------------------------------------------------------------
class CHitTestResults
{
public:
    ~CHitTestResults();

    XUINT32 GetCount() const { return m_cResults; };
    _Ret_maybenull_ CUIElement *First();

    _Check_return_ HRESULT Add(_In_ CUIElement* pResult);
    _Check_return_ HRESULT GetAnswer(
        _Out_ XUINT32 *pCount,
        _Outptr_result_buffer_(*pCount) CUIElement ***pppResults
        );

private:
    CXcpList<CUIElement> m_oResults;
    XUINT32 m_cResults = 0;
};


// Helper functions for the ManipulationModes enum.
//
// The low word (CustomManipulationModeMask) of this enum is used for Gesture flags.
// The high word (SystemManipulationModeMask) is used for DM flags.
//
// There are three cases:
//    1) mode == ManipulationModes.None == 0. No Gestures, No DM.
//    2) mode & CustomManipulationModeMask != 0. Yes Gestures, No DM.
//    3) mode & SystemManipulationModeMask != 0. No Gestures, Yes DM.
//
// Before Windows 8.1, having both (CustomManipulationModeMask & mode) AND (SystemManipulationModeMask & mode) is an invalid state.

const XUINT32 CustomManipulationModeMask = 0x0000FFFF;
const XUINT32 SystemManipulationModeMask = 0xFFFF0000;

inline DirectUI::ManipulationModes CustomManipulationModes(DirectUI::ManipulationModes manipulationMode)
{
    return static_cast<DirectUI::ManipulationModes>(CustomManipulationModeMask & static_cast<XUINT32>(manipulationMode));
}

inline DirectUI::ManipulationModes SystemManipulationModes(DirectUI::ManipulationModes manipulationMode)
{
    return static_cast<DirectUI::ManipulationModes>(SystemManipulationModeMask & static_cast<XUINT32>(manipulationMode));
}

template <typename HitType>
bool DoesRectIntersectHitType(
    _In_ const XRECTF& rect,
    _In_ const HitType& target
    );

template <typename HitType>
bool DoesRectIntersectHitType(
    _In_ const XRECTF_RB& rect,
    _In_ const HitType& target
    );

template <typename HitType>
_Check_return_ HRESULT ClipHitTypeToRect(
    _Inout_ HitType& target,
    _In_ const XRECTF& rect,
    _Out_ bool* pHit
    );

template <typename HitType>
_Check_return_ HRESULT ClipHitTypeToRect(
    _Inout_ HitType& target,
    _In_ const XRECTF_RB& rect,
    _Out_ bool* pHit
    );

template <typename HitType>
void ApplyTransformToHitType(
    _In_ CMILMatrix* pTransform,
    _In_ const HitType* pSrcTarget,
    _Out_ HitType* pDestTarget
    );

#include "BoundsWalkHitResult.h"

// Responds to bounds walk hit testing. Further refines hit test results by hit testing the element
// in its local coordinate space. Tracks the list of parent elements that were also hit.
class CBoundedHitTestVisitor
{
public:
    CBoundedHitTestVisitor(
        _In_ CHitTestResults* pHitElements,
        bool hitMultiple
        );

    _Check_return_ HRESULT OnElementHit(
        _In_ CUIElement* pElement,
        _In_ const XPOINTF& target,
        bool hitPostChildren,
        _Out_ BoundsWalkHitResult* pResult
        );

    _Check_return_ HRESULT OnElementHit(
        _In_ CUIElement* pElement,
        _In_ const HitTestPolygon& target,
        bool hitPostChildren,
        _Out_ BoundsWalkHitResult* pResult
        );

    _Check_return_ HRESULT OnParentIncluded(
        _In_ CUIElement* pElement,
        _In_ const XPOINTF& target,
        _Out_ BoundsWalkHitResult* pResult
        );

    _Check_return_ HRESULT OnParentIncluded(
        _In_ CUIElement* pElement,
        _In_ const HitTestPolygon& target,
        _Out_ BoundsWalkHitResult* pResult
        );

private:
    template <typename HitType>
    _Check_return_ HRESULT OnElementHitImpl(
        _In_ CUIElement* pElement,
        _In_ const HitType& target,
        bool hitPostChildren,
        _Out_ BoundsWalkHitResult* pResult
        );

    template <typename HitType>
    _Check_return_ HRESULT OnParentIncludedImpl(
        _In_ CUIElement* pElement,
        _In_ const HitType& target,
        _Out_ BoundsWalkHitResult* pResult
        );

    CHitTestResults* m_pHitElements;
    bool m_hitMultiple;
};
