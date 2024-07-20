// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// This file exists for dubious reasons. For things like many of the collections their header files aren't
// included anywhere else and must be present here if they were only referenced in code gen. For other header
// files there's no reason they should be here.

class CDependencyObject;

#include "EnumDefs.h"
#include "Indexes.g.h"
#include "depends.h"
#include "ModifiedValue.h"
#include "xvector.h"
#include "xref_ptr.h"

// Require depends.h

// Layout
#include "LayoutManager.h"
#include "LayoutStorage.h"

#include "collectionbase.h"
#include "DOCollection.h"
#include "HubSectionCollection.h"
#include "CommandBarElementCollection.h"
#include "MenuFlyoutItemBaseCollection.h"
#include "StoryboardCollection.h"
#include "double.h"
#include "doublecollection.h"
#include "point.h"
#include "pointcollection.h"
#include "brushcollection.h"
#include "size.h"
#include "primitives.h"
#include "CPropertyPath.h"
#include "simple.h"
#include "Enums.g.h"
#include "brush.h"
#include "tilebrush.h"
#include "imagebrush.h"
#include "figure.h"
#include "gradient.h"
#include "geometry.h"
#include "segment.h"
#include "transforms.h"
#include "cachemode.h"
#include "MarkupExtension.h"
#include "resources.h"
#include "deferredkeys.h"
#include "GridLength.h"
#include "GridDefinitions.h"
#include "XamlPredicate.h"

#include "DependencyPropertyProxy.h"
#include "SetterBase.h"
#include "SetterBaseCollection.h"
#include "Setter.h"
#include "Style.h"
#include "TargetPropertyPath.h"

#include "EventArgs.h"

// DirectManipulation
#include "DirectManipulationContainerHandler.h"
#include "DirectManipulationContainer.h"

// UIAutomation Framework
#include "automation.h"

#include "HitTestPolygon.h"

// Require visual.h and AutomationPeer.h
#include "RenderWalkType.h"
#include "UIElementStructs.h"
#include "RenderParams.h"
#include "UIElementCollection.h"
#include "UIElement.h"
#include "LayoutTransitionElement.h"

// Require uielement.h
#include "VisualStateGroupCollection.h"
#include "fontfamily.h"
#include "framework.h"
#include "layoutelement.h"

// Require framework.h

#include "TemplateBindingExtension.h"
#include "Binding.h"
#include "ThemeResourceExtension.h"
#include "NullExtension.h"
#include "TemplateContent.h"
#include "DeferredElement.h"
#include "type.h"
#include "template.h"
#include "ccontrol.h"
#include "panel.h"
#include "shape.h"
#include "mediabase.h"
#include "CornerRadius.h"
#include "Border.h"
#include "Viewbox.h"
#include "perspectiveplane.h"
#include "transitiontarget.h"
#include "StateTriggerBase.h"
#include "StateTrigger.h"
#include "StateTriggerCollection.h"
#include "AdaptiveTrigger.h"
#include "VisualState.h"
#include "VisualStateGroup.h"
#include "VisualTransition.h"
#include "VisualTransitionCollection.h"
#include "VisualStateManager.h"
#include "VisualStateChangedEventArgs.h"
#include "layouttransition.h"
#include "TransitionCollection.h"
#include "RangeBase.h"
#include "NullKeyedResource.h"

// Require mediabase.h

#include "imagebase.h"
#include "TiledSurface.h"
#include "ImageSurfaceWrapper.h"
#include "imagesource.h"

// Require panel.h

#include "rootvisual.h"
#include "canvas.h"
#include "StackPanel.h"
#include "Grid.h"
#include "RelativePanel.h"
#include "ItemsPresenter.h"
#include "ContentControl.h"
#include "ScrollContentControl.h"
#include "ContentPresenter.h"
#include "ItemsStackPanel.h"
#include "ListViewBaseItem.h"
#include "ListViewBaseItemChrome.h"
#include "ListViewItemChrome.h"
#include "GridViewItemChrome.h"
#include "CalendarViewBaseItemChrome.h"
#include "CalendarView.h"
#include "XAMLItemCollection.h"
#include "ItemsControl.h"
#include "popup.h"
#include "XamlIslandRoot.h"
#include "XamlIslandRootCollection.h"

// Require shape.h

#include "ellipse.h"
#include "glyphs.h"
#include "line.h"
#include "path.h"
#include "polygon.h"
#include "polyline.h"
#include "rect.h"
#include "rectangle.h"
#include "Thickness.h"
#include "TouchableRectangle.h"
#include "fontfamily.h"
#include "IBackingStore.h"
#include "textcontainer.h"
#include "TextElement.h"
#include "TextElementCollection.h"
#include "UIATextRange.h"
#include "TextPointerWrapper.h"
#include "inline.h"
#include "InlineUIContainer.h"
#include "EmbeddedElementHost.h"
#include "Hyperlink.h"
#include "blocktextelement.h"
#include "textview.h"
#include "TextAdapter.h"
#include "TextRangeAdapter.h"
#include "textblock.h"
#include "richtextblock.h"
#include "richtextblockoverflow.h"
#include "Gripper.h"
#include "RichEditGripper.h"
#include "CaretBrowsingCaret.h"
#include "WriteableBitmap.h"
#include "SurfaceImageSource.h"
#include "VirtualSurfaceImageSource.h"
#include "RenderTargetBitmapRoot.h"
#include "RenderTargetElement.h"
#include "RenderTargetBitmap.h"
#include "ListViewBaseItemSecondaryChrome.h"
#include "duration.h"

#include "mediaqueue.h"

#include "NavigationEventArgs.h"

#include "SwapChainElement.h"
#include "SwapChainBackgroundPanel.h"
#include "SwapChainPanel.h"

#include "PrintPageEventArgs.h"
#include "BeginPrintEventArgs.h"
#include "EndPrintEventArgs.h"
#include "PrintDocument.h"
#include "PrintRoot.h"

#include "TransitionRoot.h"

#include "FullWindowMediaRoot.h"

#include "ConnectedAnimationRoot.h"

#include "DependencyObjectWrapper.h"

#include "FlyoutBase.h"
#include "Flyout.h"
#include "MenuFlyout.h"
#include "CommandBar.h"
#include "Button.h"

#include "icon.h"

#include "ScrollViewer.h"

#include "frameworkelementex.h"
#include "panelex.h"

// Core peer for Microsoft.UI.Xaml.Window
#include "CWindow.h"
