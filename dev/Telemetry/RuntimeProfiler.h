// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.


namespace RuntimeProfiler
{
    typedef enum
    {
        PG_Class = 0
    } ProfileGroup;

    //  We use these id's on reports so please don't remove or move these
    //  definitions.  Add any new id's immediately preceding ProfId_Size
    //  since it's used to determine the size of this list.
    typedef enum
    {
        ProfId_ColorPicker = 0,
        ProfId_NavigationView,
        ProfId_ParallaxView,
        ProfId_PersonPicture,
        ProfId_RefreshContainer,
        ProfId_RefreshVisualizer,
        ProfId_RatingControl,
        ProfId_SwipeControl,
        ProfId_SwipeItem,
        ProfId_TreeView,
        ProfId_TwoPaneView,
        ProfId_Reveal,
        ProfId_Acrylic,
        ProfId_SplitButton,
        ProfId_DropDownButton,
        ProfId_CommandBarFlyout,
        ProfId_TextCommandBarFlyout,
        ProfId_RadioButtons,
        ProfId_RadioMenuFlyoutItem,
        ProfId_ItemsRepeater,
        ProfId_TeachingTip,
        ProfId_AnimatedVisualPlayer,
        ProfId_NonVirtualizingLayout,
        ProfId_StackLayout,
        ProfId_UniformGridLayout,
        ProfId_VirtualizingLayout,
        ProfId_ItemsRepeaterScrollHost,
        ProfId_TabView,
        ProfId_TabViewItem,
        ProfId_ProgressBar,
        ProfId_ProgressRing,
        ProfId_NumberBox,
        ProfId_RadialGradientBrush,
        ProfId_InfoBar,
        ProfId_Expander,
        ProfId_PagerControl,
        ProfId_PipsControl,
        ProfId_Size // ProfId_Size is the last always. 
    } ProfilerClassId;

    //  Ditto above...
    typedef enum
    {
        ProfMemberId_Acrylic_TintLuminosityOpacity_Changed = 0,
        ProfMemberId_TeachingTip_F6AccessKey_FirstInvocation,
        ProfMemberId_TeachingTip_F6AccessKey_SubsequentInvocation,
        ProfMemberId_TeachingTip_TipDidNotOpenDueToSize,
        ProfMemberId_Size
    } ProfilerMemberId;

    void FireEvent(bool Suspend) noexcept;
    void RegisterMethod(ProfileGroup group, UINT16 TypeIndex, UINT16 MethodIndex, volatile LONG *Count) noexcept;
}

#define __RP_Marker_ClassById(typeindex) \
    { \
        __pragma (warning ( suppress : 28112)) \
        static volatile LONG __RuntimeProfiler_Counter = -1; \
        if (0 == ::InterlockedIncrement(&__RuntimeProfiler_Counter)) \
        { \
            RuntimeProfiler::RegisterMethod(RuntimeProfiler::PG_Class, (UINT16)typeindex, 9999, &__RuntimeProfiler_Counter); \
        } \
    }
    
#define __RP_Marker_ClassMemberById(typeindex, memberindex) \
    { \
        __pragma (warning ( suppress : 28112)) \
        static volatile LONG __RuntimeProfiler_Counter = -1; \
        if (0 == ::InterlockedIncrement(&__RuntimeProfiler_Counter)) \
        { \
            RuntimeProfiler::RegisterMethod(RuntimeProfiler::PG_Class, (UINT16)typeindex, (UINT16)memberindex, &__RuntimeProfiler_Counter); \
        } \
    }


