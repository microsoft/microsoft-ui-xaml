// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// IF YOU GET A COMPILE ERROR IN THIS FILE IT MEANS YOU'VE CHANGED SIZE OR ALIGNMENT REQUIREMENTS
// OF ONE OF CLASSES WHICH CAN HAVE SIGNIFICANT EFFECT ON HEAP CONSUMPTION.
//
// Checked type sizes include space occupied by ancestor types, so if you change base class which has a subtype
// listed below, it will also trigger a failure.
// There might be instances where class layout will depend on state of build environment, e.g. value of velocity key.
// If your change modified sizes, you will need to update all flavor / build env combinations.
//
// To get type's size and alignment, either get them from SizeBench tool (see https://osgwiki.com/wiki/SizeBench) or look at what follows the compiler error:
//
// D:\src\dxaml\xcp\components\perf_guard\type_layout.h(9,28): error C2338: Size of type changed. [D:\src\dxaml\xcp\components\perf_guard\Microsoft.UI.Xaml.PerfGuard.vcxproj]
// D:\src\dxaml\xcp\components\perf_guard\type_layout.cpp(93): message : see reference to function template instantiation 'void validate_size<CContentControl,624,616>(void)' being compiled
//
// In this example size of CContentControl on amd64fre has increased from 616 (expected) to 624 (actual) bytes:
// validate_size<CContentControl,624,616>(void)
//
// The check is performed on x86chk, x86fre and amd64fre.
// To update:
// -build x86chk, update the affected entries for x86chk using instructions above
// -repeat for x86fre
// -repeat for amd64fre

#include "precomp.h"

#include "type_layout.h"

#include "CDependencyObject.h"
#include "Binding.h"
#include "Border.h"
#include "ContentControl.h"
#include "DoubleAnimation.h"
#include "DoubleAnimationUsingKeyFrames.h"
#include "DoubleKeyFrame.h"
#include "primitives.h"
#include "grid.h"
#include "KeySpline.h"
#include "ManagedObjectReference.h"
#include "CMatrix.h"
#include "MatrixTransform.h"
#include "ObjectAnimationUsingKeyFrames.h"
#include "ObjectKeyFrame.h"
#include "Pointer.h"
#include "Rectangle.h"
#include "Resources.h"
#include "Setter.h"
#include "Storyboard.h"
#include "TargetPropertyPath.h"
#include "TextBlock.h"
#include "TimelineCollection.h"
#include "VisualState.h"
#include "hwcompnode.h"
#include "StackPanel.h"
#include "RichTextBlock.h"

// 32f = x86fre size
// 32c = x86chk size
// 64f = amd64fre size
// inheritance level - level on class hierarchy (need to be sorted to point to class which caused change)

//                                                  type_name,  32f,  32c,  64f, inheritance level
VALIDATE_TYPE_LAYOUT(                       CDependencyObject,   52,   68,   96) // 1
VALIDATE_TYPE_LAYOUT(                            CVisualState,   88,  104,  168) // 2
VALIDATE_TYPE_LAYOUT(                               CTimeSpan,   64,   80,  104) // 2
VALIDATE_TYPE_LAYOUT(                                 CMatrix,   76,   92,  120) // 2
VALIDATE_TYPE_LAYOUT(                              CUIElement,  264,  280,  368) // 2
VALIDATE_TYPE_LAYOUT(                     CTargetPropertyPath,   80,   96,  152) // 2
VALIDATE_TYPE_LAYOUT(              CShareableDependencyObject,   56,   72,  104) // 2
VALIDATE_TYPE_LAYOUT(                                CPointer,   64,   80,  112) // 2
VALIDATE_TYPE_LAYOUT(                             CEnumerated,   60,   76,  104) // 2
VALIDATE_TYPE_LAYOUT(                              CKeySpline,   72,   88,  120) // 2
VALIDATE_TYPE_LAYOUT(                      CRectangleGeometry,   88,  104,  144) // 3
VALIDATE_TYPE_LAYOUT(                                CKeyTime,   72,   88,  112) // 3
VALIDATE_TYPE_LAYOUT(      CNoParentShareableDependencyObject,   60,   76,  112) // 3
VALIDATE_TYPE_LAYOUT(                       CFrameworkElement,  308,  324,  448) // 3
VALIDATE_TYPE_LAYOUT(                              CTextBlock,  428,  444,  648) // 4
VALIDATE_TYPE_LAYOUT(                     CResourceDictionary,  192,  212,  360) // 4
VALIDATE_TYPE_LAYOUT(                          CRichTextBlock,  448,  464,  680) // 4
VALIDATE_TYPE_LAYOUT(          CObjectAnimationUsingKeyFrames,  296,  312,  464) // 4
VALIDATE_TYPE_LAYOUT(                                CBinding,   56,   72,  104) // 4
VALIDATE_TYPE_LAYOUT(                                 CBorder,  368,  384,  520) // 4
VALIDATE_TYPE_LAYOUT(                 CManagedObjectReference,   76,   92,  136) // 4
VALIDATE_TYPE_LAYOUT(                       CContentPresenter,  348,  364,  512) // 4
VALIDATE_TYPE_LAYOUT(                        CDoubleAnimation,  248,  264,  392) // 4
VALIDATE_TYPE_LAYOUT(   CMultiParentShareableDependencyObject,   76,   92,  144) // 4
VALIDATE_TYPE_LAYOUT(                     CTimelineCollection,   88,  104,  168) // 5
VALIDATE_TYPE_LAYOUT(                                  CBrush,   84,  100,  160) // 5
VALIDATE_TYPE_LAYOUT(                         CContentControl,  412,  428,  616) // 5
VALIDATE_TYPE_LAYOUT(                             CStoryboard,  248,  264,  368) // 5
VALIDATE_TYPE_LAYOUT(         HWCompWinRTVisualRenderDataNode,   64,   80,  120) // 5
VALIDATE_TYPE_LAYOUT(          CDoubleAnimationUsingKeyFrames,  248,  264,  392) // 5
VALIDATE_TYPE_LAYOUT(                                   CGrid,  344,  360,  512) // 5
VALIDATE_TYPE_LAYOUT(                              CRectangle,  396,  412,  552) // 5
VALIDATE_TYPE_LAYOUT(                             CStackPanel,  336,  352,  488) // 5
VALIDATE_TYPE_LAYOUT(                                 CSetter,  112,  128,  208) // 6
VALIDATE_TYPE_LAYOUT(               CDoubleKeyFrameCollection,   92,  108,  176) // 6
VALIDATE_TYPE_LAYOUT(               CObjectKeyFrameCollection,   92,  108,  176) // 6
VALIDATE_TYPE_LAYOUT(                        CSolidColorBrush,   96,  112,  184) // 6
VALIDATE_TYPE_LAYOUT(                        CMatrixTransform,   88,  104,  168) // 7