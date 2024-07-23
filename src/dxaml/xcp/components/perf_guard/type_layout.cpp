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
VALIDATE_TYPE_LAYOUT(                       CDependencyObject,   48,   64,   88) // 1
VALIDATE_TYPE_LAYOUT(                            CVisualState,   84,  100,  160) // 2
VALIDATE_TYPE_LAYOUT(                               CTimeSpan,   56,   72,   96) // 2
VALIDATE_TYPE_LAYOUT(                                 CMatrix,   72,   88,  112) // 2
VALIDATE_TYPE_LAYOUT(                              CUIElement,  260,  276,  360) // 2
VALIDATE_TYPE_LAYOUT(                     CTargetPropertyPath,   76,   92,  144) // 2
VALIDATE_TYPE_LAYOUT(              CShareableDependencyObject,   52,   68,   96) // 2
VALIDATE_TYPE_LAYOUT(                                CPointer,   60,   76,  104) // 2
VALIDATE_TYPE_LAYOUT(                             CEnumerated,   56,   72,   96) // 2
VALIDATE_TYPE_LAYOUT(                              CKeySpline,   68,   84,  112) // 2
VALIDATE_TYPE_LAYOUT(                      CRectangleGeometry,   84,  100,  136) // 3
VALIDATE_TYPE_LAYOUT(                                CKeyTime,   64,   80,  104) // 3
VALIDATE_TYPE_LAYOUT(      CNoParentShareableDependencyObject,   56,   72,  104) // 3
VALIDATE_TYPE_LAYOUT(                       CFrameworkElement,  304,  320,  440) // 3
VALIDATE_TYPE_LAYOUT(                              CTextBlock,  424,  440,  640) // 4
VALIDATE_TYPE_LAYOUT(                     CResourceDictionary,  188,  208,  352) // 4
VALIDATE_TYPE_LAYOUT(                          CRichTextBlock,  444,  460,  672) // 4
VALIDATE_TYPE_LAYOUT(          CObjectAnimationUsingKeyFrames,  288,  304,  456) // 4
VALIDATE_TYPE_LAYOUT(                                CBinding,   52,   68,   96) // 4
VALIDATE_TYPE_LAYOUT(                                 CBorder,  364,  380,  512) // 4
VALIDATE_TYPE_LAYOUT(                 CManagedObjectReference,   72,   88,  128) // 4
VALIDATE_TYPE_LAYOUT(                       CContentPresenter,  344,  360,  504) // 4
VALIDATE_TYPE_LAYOUT(                        CDoubleAnimation,  240,  256,  384) // 4
VALIDATE_TYPE_LAYOUT(   CMultiParentShareableDependencyObject,   72,   88,  136) // 4
VALIDATE_TYPE_LAYOUT(                     CTimelineCollection,   84,  100,  160) // 5
VALIDATE_TYPE_LAYOUT(                                  CBrush,   80,   96,  152) // 5
VALIDATE_TYPE_LAYOUT(                         CContentControl,  408,  424,  608) // 5
VALIDATE_TYPE_LAYOUT(                             CStoryboard,  240,  256,  360) // 5
VALIDATE_TYPE_LAYOUT(         HWCompWinRTVisualRenderDataNode,   60,   76,  112) // 5
VALIDATE_TYPE_LAYOUT(          CDoubleAnimationUsingKeyFrames,  240,  256,  384) // 5
VALIDATE_TYPE_LAYOUT(                                   CGrid,  340,  356,  504) // 5
VALIDATE_TYPE_LAYOUT(                              CRectangle,  392,  408,  544) // 5
VALIDATE_TYPE_LAYOUT(                             CStackPanel,  332,  348,  480) // 5
VALIDATE_TYPE_LAYOUT(                                 CSetter,  108,  124,  200) // 6
VALIDATE_TYPE_LAYOUT(               CDoubleKeyFrameCollection,   88,  104,  168) // 6
VALIDATE_TYPE_LAYOUT(               CObjectKeyFrameCollection,   88,  104,  168) // 6
VALIDATE_TYPE_LAYOUT(                        CSolidColorBrush,   92,  108,  176) // 6
VALIDATE_TYPE_LAYOUT(                        CMatrixTransform,   84,  100,  160) // 7
