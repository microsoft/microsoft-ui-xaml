// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "xstack.h"
#include "CAutomationPeerAnnotation.g.h"
#include "AutomationPeerAnnotationCollection.h"
#include "UIElement.h"
#include <ComPtr.h>
#include <windows.applicationmodel.core.h>
#include <XamlIslandRoot.h>
#include <Popup.h>
#include <CControl.h>
#include <focusmgr.h>
#include <DesktopUtility.h>
#include <DependencyObject.h>
#include <RootScale.h>
#include <xcpwindow.h>

HRESULT GetCurrentDisplayName(_Out_ BSTR *outString);

// Look up identifiers.
HRESULT CUIAWindow::InitIds()
{
    // When InfoBar gets created while a UIA listener is attached, we can hit a
    // reentrancy crash here. PauseNewDispatch will pause messages being
    // processed, so we won't hit the crash.
    IXcpBrowserHost *pBH = m_pHost->GetBrowserHost();
    CCoreServices *pCore = nullptr;
    if (pBH)
    {
        pCore = pBH->GetContextInterface();
    }
    PauseNewDispatch deferReentrancy(pCore);

    ASSERT(m_pUIAIds);
    m_pUIAIds->IsControlElement_Property        = UiaLookupId(AutomationIdentifierType_Property, &IsControlElement_Property_GUID);
    m_pUIAIds->ControlType_Property             = UiaLookupId(AutomationIdentifierType_Property, &ControlType_Property_GUID);
    m_pUIAIds->IsContentElement_Property        = UiaLookupId(AutomationIdentifierType_Property, &IsContentElement_Property_GUID);
    m_pUIAIds->LabeledBy_Property               = UiaLookupId(AutomationIdentifierType_Property, &LabeledBy_Property_GUID);
    m_pUIAIds->AutomationId_Property            = UiaLookupId(AutomationIdentifierType_Property, &AutomationId_Property_GUID);
    m_pUIAIds->ItemType_Property                = UiaLookupId(AutomationIdentifierType_Property, &ItemType_Property_GUID);
    m_pUIAIds->IsPassword_Property              = UiaLookupId(AutomationIdentifierType_Property, &IsPassword_Property_GUID);
    m_pUIAIds->LocalizedControlType_Property    = UiaLookupId(AutomationIdentifierType_Property, &LocalizedControlType_Property_GUID);
    m_pUIAIds->Name_Property                    = UiaLookupId(AutomationIdentifierType_Property, &Name_Property_GUID);
    m_pUIAIds->AcceleratorKey_Property          = UiaLookupId(AutomationIdentifierType_Property, &AcceleratorKey_Property_GUID);
    m_pUIAIds->AccessKey_Property               = UiaLookupId(AutomationIdentifierType_Property, &AccessKey_Property_GUID);
    m_pUIAIds->HasKeyboardFocus_Property        = UiaLookupId(AutomationIdentifierType_Property, &HasKeyboardFocus_Property_GUID);
    m_pUIAIds->IsKeyboardFocusable_Property     = UiaLookupId(AutomationIdentifierType_Property, &IsKeyboardFocusable_Property_GUID);
    m_pUIAIds->IsEnabled_Property               = UiaLookupId(AutomationIdentifierType_Property, &IsEnabled_Property_GUID);
    m_pUIAIds->BoundingRectangle_Property       = UiaLookupId(AutomationIdentifierType_Property, &BoundingRectangle_Property_GUID);
    m_pUIAIds->ProcessId_Property               = UiaLookupId(AutomationIdentifierType_Property, &ProcessId_Property_GUID);
    m_pUIAIds->RuntimeId_Property               = UiaLookupId(AutomationIdentifierType_Property, &RuntimeId_Property_GUID);
    m_pUIAIds->ClassName_Property               = UiaLookupId(AutomationIdentifierType_Property, &ClassName_Property_GUID);
    m_pUIAIds->HelpText_Property                = UiaLookupId(AutomationIdentifierType_Property, &HelpText_Property_GUID);
    m_pUIAIds->ClickablePoint_Property          = UiaLookupId(AutomationIdentifierType_Property, &ClickablePoint_Property_GUID);
    m_pUIAIds->Culture_Property                 = UiaLookupId(AutomationIdentifierType_Property, &Culture_Property_GUID);
    m_pUIAIds->IsOffscreen_Property             = UiaLookupId(AutomationIdentifierType_Property, &IsOffscreen_Property_GUID);
    m_pUIAIds->Orientation_Property             = UiaLookupId(AutomationIdentifierType_Property, &Orientation_Property_GUID);
    m_pUIAIds->FrameworkId_Property             = UiaLookupId(AutomationIdentifierType_Property, &FrameworkId_Property_GUID);
    m_pUIAIds->IsRequiredForForm_Property       = UiaLookupId(AutomationIdentifierType_Property, &IsRequiredForForm_Property_GUID);
    m_pUIAIds->ItemStatus_Property              = UiaLookupId(AutomationIdentifierType_Property, &ItemStatus_Property_GUID);
    m_pUIAIds->LiveSetting_Property             = UiaLookupId(AutomationIdentifierType_Property, &LiveSetting_Property_GUID);
    m_pUIAIds->ControlledPeers_Property         = UiaLookupId(AutomationIdentifierType_Property, &ControllerFor_Property_GUID);
    m_pUIAIds->FlowsFrom_Property               = UiaLookupId(AutomationIdentifierType_Property, &FlowsFrom_Property_GUID);
    m_pUIAIds->FlowsTo_Property                 = UiaLookupId(AutomationIdentifierType_Property, &FlowsTo_Property_GUID);
    m_pUIAIds->PositionInSet_Property           = UiaLookupId(AutomationIdentifierType_Property, &PositionInSet_Property_GUID);
    m_pUIAIds->SizeOfSet_Property               = UiaLookupId(AutomationIdentifierType_Property, &SizeOfSet_Property_GUID);
    m_pUIAIds->Level_Property                   = UiaLookupId(AutomationIdentifierType_Property, &Level_Property_GUID);
    m_pUIAIds->AnnotationTypes_Property         = UiaLookupId(AutomationIdentifierType_Property, &AnnotationTypes_Property_GUID);
    m_pUIAIds->AnnotationObjects_Property       = UiaLookupId(AutomationIdentifierType_Property, &AnnotationObjects_Property_GUID);
    m_pUIAIds->LandmarkType_Property            = UiaLookupId(AutomationIdentifierType_Property, &LandmarkType_Property_GUID);
    m_pUIAIds->LocalizedLandmarkType_Property   = UiaLookupId(AutomationIdentifierType_Property, &LocalizedLandmarkType_Property_GUID);
    m_pUIAIds->IsPeripheral_Property            = UiaLookupId(AutomationIdentifierType_Property, &IsPeripheral_Property_GUID);
    m_pUIAIds->IsDataValidForForm_Property      = UiaLookupId(AutomationIdentifierType_Property, &IsDataValidForForm_Property_GUID);
    m_pUIAIds->FullDescription_Property         = UiaLookupId(AutomationIdentifierType_Property, &FullDescription_Property_GUID);
    m_pUIAIds->DescribedBy_Property             = UiaLookupId(AutomationIdentifierType_Property, &DescribedBy_Property_GUID);
    m_pUIAIds->HeadingLevel_Property            = UiaLookupId(AutomationIdentifierType_Property, &HeadingLevel_Property_GUID);
    m_pUIAIds->IsDialog_Property                = UiaLookupId(AutomationIdentifierType_Property, &IsDialog_Property_GUID);

    m_pUIAIds->Annotation_AnnotationTypeId_Property         = UiaLookupId(AutomationIdentifierType_Property, &Annotation_AnnotationTypeId_Property_GUID);
    m_pUIAIds->Annotation_AnnotationTypeName_Property       = UiaLookupId(AutomationIdentifierType_Property, &Annotation_AnnotationTypeName_Property_GUID);
    m_pUIAIds->Annotation_Author_Property                   = UiaLookupId(AutomationIdentifierType_Property, &Annotation_Author_Property_GUID);
    m_pUIAIds->Annotation_DateTime_Property                 = UiaLookupId(AutomationIdentifierType_Property, &Annotation_DateTime_Property_GUID);
    m_pUIAIds->Annotation_Target_Property                   = UiaLookupId(AutomationIdentifierType_Property, &Annotation_Target_Property_GUID);
    m_pUIAIds->Dock_DockPosition_Property                   = UiaLookupId(AutomationIdentifierType_Property, &Dock_DockPosition_Property_GUID);
    m_pUIAIds->Drag_DropEffect_Property                     = UiaLookupId(AutomationIdentifierType_Property, &Drag_DropEffect_Property_GUID);
    m_pUIAIds->Drag_DropEffects_Property                    = UiaLookupId(AutomationIdentifierType_Property, &Drag_DropEffects_Property_GUID);
    m_pUIAIds->Drag_GrabbedItems_Property                   = UiaLookupId(AutomationIdentifierType_Property, &Drag_GrabbedItems_Property_GUID);
    m_pUIAIds->Drag_IsGrabbed_Property                      = UiaLookupId(AutomationIdentifierType_Property, &Drag_IsGrabbed_Property_GUID);
    m_pUIAIds->DropTarget_DropTargetEffect_Property         = UiaLookupId(AutomationIdentifierType_Property, &DropTarget_DropTargetEffect_Property_GUID);
    m_pUIAIds->DropTarget_DropTargetEffects_Property        = UiaLookupId(AutomationIdentifierType_Property, &DropTarget_DropTargetEffects_Property_GUID);
    m_pUIAIds->ExpandCollapse_ExpandCollapseState_Property  = UiaLookupId(AutomationIdentifierType_Property, &ExpandCollapse_ExpandCollapseState_Property_GUID);
    m_pUIAIds->GridItem_Column_Property                     = UiaLookupId(AutomationIdentifierType_Property, &GridItem_Column_Property_GUID);
    m_pUIAIds->GridItem_ColumnSpan_Property                 = UiaLookupId(AutomationIdentifierType_Property, &GridItem_ColumnSpan_Property_GUID);
    m_pUIAIds->GridItem_Parent_Property                     = UiaLookupId(AutomationIdentifierType_Property, &GridItem_Parent_Property_GUID);
    m_pUIAIds->GridItem_Row_Property                        = UiaLookupId(AutomationIdentifierType_Property, &GridItem_Row_Property_GUID);
    m_pUIAIds->GridItem_RowSpan_Property                    = UiaLookupId(AutomationIdentifierType_Property, &GridItem_RowSpan_Property_GUID);
    m_pUIAIds->Grid_ColumnCount_Property                    = UiaLookupId(AutomationIdentifierType_Property, &Grid_ColumnCount_Property_GUID);
    m_pUIAIds->Grid_RowCount_Property                       = UiaLookupId(AutomationIdentifierType_Property, &Grid_RowCount_Property_GUID);
    m_pUIAIds->MultipleView_CurrentView_Property            = UiaLookupId(AutomationIdentifierType_Property, &MultipleView_CurrentView_Property_GUID);
    m_pUIAIds->MultipleView_SupportedViews_Property         = UiaLookupId(AutomationIdentifierType_Property, &MultipleView_SupportedViews_Property_GUID);
    m_pUIAIds->RangeValue_IsReadOnly_Property               = UiaLookupId(AutomationIdentifierType_Property, &RangeValue_IsReadOnly_Property_GUID);
    m_pUIAIds->RangeValue_LargeChange_Property              = UiaLookupId(AutomationIdentifierType_Property, &RangeValue_LargeChange_Property_GUID);
    m_pUIAIds->RangeValue_Maximum_Property                  = UiaLookupId(AutomationIdentifierType_Property, &RangeValue_Maximum_Property_GUID);
    m_pUIAIds->RangeValue_Minimum_Property                  = UiaLookupId(AutomationIdentifierType_Property, &RangeValue_Minimum_Property_GUID);
    m_pUIAIds->RangeValue_SmallChange_Property              = UiaLookupId(AutomationIdentifierType_Property, &RangeValue_SmallChange_Property_GUID);
    m_pUIAIds->RangeValue_Value_Property                    = UiaLookupId(AutomationIdentifierType_Property, &RangeValue_Value_Property_GUID);
    m_pUIAIds->Scroll_HorizontallyScrollable_Property       = UiaLookupId(AutomationIdentifierType_Property, &Scroll_HorizontallyScrollable_Property_GUID);
    m_pUIAIds->Scroll_HorizontalScrollPercent_Property      = UiaLookupId(AutomationIdentifierType_Property, &Scroll_HorizontalScrollPercent_Property_GUID);
    m_pUIAIds->Scroll_HorizontalViewSize_Property           = UiaLookupId(AutomationIdentifierType_Property, &Scroll_HorizontalViewSize_Property_GUID);
    m_pUIAIds->Scroll_VerticallyScrollable_Property         = UiaLookupId(AutomationIdentifierType_Property, &Scroll_VerticallyScrollable_Property_GUID);
    m_pUIAIds->Scroll_VerticalScrollPercent_Property        = UiaLookupId(AutomationIdentifierType_Property, &Scroll_VerticalScrollPercent_Property_GUID);
    m_pUIAIds->Scroll_VerticalViewSize_Property             = UiaLookupId(AutomationIdentifierType_Property, &Scroll_VerticalViewSize_Property_GUID);
    m_pUIAIds->SelectionItem_IsSelected_Property            = UiaLookupId(AutomationIdentifierType_Property, &SelectionItem_IsSelected_Property_GUID);
    m_pUIAIds->SelectionItem_SelectionContainer_Property    = UiaLookupId(AutomationIdentifierType_Property, &SelectionItem_SelectionContainer_Property_GUID);
    m_pUIAIds->Selection_CanSelectMultiple_Property         = UiaLookupId(AutomationIdentifierType_Property, &Selection_CanSelectMultiple_Property_GUID);
    m_pUIAIds->Selection_IsSelectionRequired_Property       = UiaLookupId(AutomationIdentifierType_Property, &Selection_IsSelectionRequired_Property_GUID);
    m_pUIAIds->Selection_Selection_Property                 = UiaLookupId(AutomationIdentifierType_Property, &Selection_Selection_Property_GUID);
    m_pUIAIds->TableItem_ColumnHeaderItems_Property         = UiaLookupId(AutomationIdentifierType_Property, &TableItem_ColumnHeaderItems_Property_GUID);
    m_pUIAIds->TableItem_RowHeaderItems_Property            = UiaLookupId(AutomationIdentifierType_Property, &TableItem_RowHeaderItems_Property_GUID);
    m_pUIAIds->Table_ColumnHeaders_Property                 = UiaLookupId(AutomationIdentifierType_Property, &Table_ColumnHeaders_Property_GUID);
    m_pUIAIds->Table_RowHeaders_Property                    = UiaLookupId(AutomationIdentifierType_Property, &Table_RowHeaders_Property_GUID);
    m_pUIAIds->Table_RowOrColumn_Property                   = UiaLookupId(AutomationIdentifierType_Property, &Table_RowOrColumnMajor_Property_GUID);
    m_pUIAIds->Toggle_ToggleState_Property                  = UiaLookupId(AutomationIdentifierType_Property, &Toggle_ToggleState_Property_GUID);
    m_pUIAIds->Transform_CanMove_Property                   = UiaLookupId(AutomationIdentifierType_Property, &Transform_CanMove_Property_GUID);
    m_pUIAIds->Transform_CanResize_Property                 = UiaLookupId(AutomationIdentifierType_Property, &Transform_CanResize_Property_GUID);
    m_pUIAIds->Transform_CanRotate_Property                 = UiaLookupId(AutomationIdentifierType_Property, &Transform_CanRotate_Property_GUID);
    m_pUIAIds->Value_IsReadOnly_Property                    = UiaLookupId(AutomationIdentifierType_Property, &Value_IsReadOnly_Property_GUID);
    m_pUIAIds->Value_Value_Property                         = UiaLookupId(AutomationIdentifierType_Property, &Value_Value_Property_GUID);
    m_pUIAIds->Window_CanMaximize_Property                  = UiaLookupId(AutomationIdentifierType_Property, &Window_CanMaximize_Property_GUID);
    m_pUIAIds->Window_CanMinimize_Property                  = UiaLookupId(AutomationIdentifierType_Property, &Window_CanMinimize_Property_GUID);
    m_pUIAIds->Window_IsModal_Property                      = UiaLookupId(AutomationIdentifierType_Property, &Window_IsModal_Property_GUID);
    m_pUIAIds->Window_IsTopmost_Property                    = UiaLookupId(AutomationIdentifierType_Property, &Window_IsTopmost_Property_GUID);
    m_pUIAIds->Window_WindowInteractionState_Property       = UiaLookupId(AutomationIdentifierType_Property, &Window_WindowInteractionState_Property_GUID);
    m_pUIAIds->Window_WindowVisualState_Property            = UiaLookupId(AutomationIdentifierType_Property, &Window_WindowVisualState_Property_GUID);
    m_pUIAIds->Transform2_CanZoom_Property                  = UiaLookupId(AutomationIdentifierType_Property, &Transform2_CanZoom_Property_GUID);
    m_pUIAIds->Transform2_ZoomLevel_Property                = UiaLookupId(AutomationIdentifierType_Property, &Transform2_ZoomLevel_Property_GUID);
    m_pUIAIds->Transform2_MaxZoom_Property                  = UiaLookupId(AutomationIdentifierType_Property, &Transform2_ZoomMaximum_Property_GUID);
    m_pUIAIds->Transform2_MinZoom_Property                  = UiaLookupId(AutomationIdentifierType_Property, &Transform2_ZoomMinimum_Property_GUID);
    m_pUIAIds->SpreadsheetItem_Formula_Property             = UiaLookupId(AutomationIdentifierType_Property, &SpreadsheetItem_Formula_Property_GUID);
    m_pUIAIds->Styles_ExtendedProperties_Property           = UiaLookupId(AutomationIdentifierType_Property, &Styles_ExtendedProperties_Property_GUID);
    m_pUIAIds->Styles_FillColor_Property                    = UiaLookupId(AutomationIdentifierType_Property, &Styles_FillColor_Property_GUID);
    m_pUIAIds->Styles_FillPatternColor_Property             = UiaLookupId(AutomationIdentifierType_Property, &Styles_FillPatternColor_Property_GUID);
    m_pUIAIds->Styles_FillPatternStyle_Property             = UiaLookupId(AutomationIdentifierType_Property, &Styles_FillPatternStyle_Property_GUID);
    m_pUIAIds->Styles_Shape_Property                        = UiaLookupId(AutomationIdentifierType_Property, &Styles_Shape_Property_GUID);
    m_pUIAIds->Styles_StyleId_Property                      = UiaLookupId(AutomationIdentifierType_Property, &Styles_StyleId_Property_GUID);
    m_pUIAIds->Styles_StyleName_Property                    = UiaLookupId(AutomationIdentifierType_Property, &Styles_StyleName_Property_GUID);
    IFC_RETURN(GetPreventKeyboardDisplayOnProgrammaticFocusId());

    m_pUIAIds->Annotation_Pattern               = UiaLookupId(AutomationIdentifierType_Pattern, &Annotation_Pattern_GUID);
    m_pUIAIds->Invoke_Pattern                   = UiaLookupId(AutomationIdentifierType_Pattern, &Invoke_Pattern_GUID);
    m_pUIAIds->Dock_Pattern                     = UiaLookupId(AutomationIdentifierType_Pattern, &Dock_Pattern_GUID);
    m_pUIAIds->Drag_Pattern                     = UiaLookupId(AutomationIdentifierType_Pattern, &Drag_Pattern_GUID);
    m_pUIAIds->DropTarget_Pattern               = UiaLookupId(AutomationIdentifierType_Pattern, &DropTarget_Pattern_GUID);
    m_pUIAIds->ExpandCollapse_Pattern           = UiaLookupId(AutomationIdentifierType_Pattern, &ExpandCollapse_Pattern_GUID);
    m_pUIAIds->GridItem_Pattern                 = UiaLookupId(AutomationIdentifierType_Pattern, &GridItem_Pattern_GUID);
    m_pUIAIds->Grid_Pattern                     = UiaLookupId(AutomationIdentifierType_Pattern, &Grid_Pattern_GUID);
    m_pUIAIds->MultipleView_Pattern             = UiaLookupId(AutomationIdentifierType_Pattern, &MultipleView_Pattern_GUID);
    m_pUIAIds->RangeValue_Pattern               = UiaLookupId(AutomationIdentifierType_Pattern, &RangeValue_Pattern_GUID);
    m_pUIAIds->ScrollItem_Pattern               = UiaLookupId(AutomationIdentifierType_Pattern, &ScrollItem_Pattern_GUID);
    m_pUIAIds->Scroll_Pattern                   = UiaLookupId(AutomationIdentifierType_Pattern, &Scroll_Pattern_GUID);
    m_pUIAIds->SelectionItem_Pattern            = UiaLookupId(AutomationIdentifierType_Pattern, &SelectionItem_Pattern_GUID);
    m_pUIAIds->Selection_Pattern                = UiaLookupId(AutomationIdentifierType_Pattern, &Selection_Pattern_GUID);
    m_pUIAIds->TableItem_Pattern                = UiaLookupId(AutomationIdentifierType_Pattern, &TableItem_Pattern_GUID);
    m_pUIAIds->Table_Pattern                    = UiaLookupId(AutomationIdentifierType_Pattern, &Table_Pattern_GUID);
    m_pUIAIds->Toggle_Pattern                   = UiaLookupId(AutomationIdentifierType_Pattern, &Toggle_Pattern_GUID);
    m_pUIAIds->Transform_Pattern                = UiaLookupId(AutomationIdentifierType_Pattern, &Transform_Pattern_GUID);
    m_pUIAIds->Value_Pattern                    = UiaLookupId(AutomationIdentifierType_Pattern, &Value_Pattern_GUID);
    m_pUIAIds->Window_Pattern                   = UiaLookupId(AutomationIdentifierType_Pattern, &Window_Pattern_GUID);
    m_pUIAIds->Text_Pattern                     = UiaLookupId(AutomationIdentifierType_Pattern, &Text_Pattern_GUID);
    m_pUIAIds->ItemContainer_Pattern            = UiaLookupId(AutomationIdentifierType_Pattern, &ItemContainer_Pattern_GUID);
    m_pUIAIds->VirtualizedItem_Pattern          = UiaLookupId(AutomationIdentifierType_Pattern, &VirtualizedItem_Pattern_GUID);
    m_pUIAIds->Text_Pattern2                    = UiaLookupId(AutomationIdentifierType_Pattern, &Text_Pattern2_GUID);
    m_pUIAIds->TextChild_Pattern                = UiaLookupId(AutomationIdentifierType_Pattern, &TextChild_Pattern_GUID);
    m_pUIAIds->ObjectModel_Pattern              = UiaLookupId(AutomationIdentifierType_Pattern, &ObjectModel_Pattern_GUID);
    m_pUIAIds->Spreadsheet_Pattern              = UiaLookupId(AutomationIdentifierType_Pattern, &Spreadsheet_Pattern_GUID);
    m_pUIAIds->SpreadsheetItem_Pattern          = UiaLookupId(AutomationIdentifierType_Pattern, &SpreadsheetItem_Pattern_GUID);
    m_pUIAIds->Styles_Pattern                   = UiaLookupId(AutomationIdentifierType_Pattern, &Styles_Pattern_GUID);
    m_pUIAIds->Transform_Pattern2               = UiaLookupId(AutomationIdentifierType_Pattern, &Tranform_Pattern2_GUID);
    m_pUIAIds->SynchronizedInput_Pattern        = UiaLookupId(AutomationIdentifierType_Pattern, &SynchronizedInput_Pattern_GUID);
    m_pUIAIds->TextEdit_Pattern                 = UiaLookupId(AutomationIdentifierType_Pattern, &TextEdit_Pattern_GUID);
    m_pUIAIds->CustomNavigation_Pattern         = UiaLookupId(AutomationIdentifierType_Pattern, &CustomNavigation_Pattern_GUID);

    m_pUIAIds->Button_ControlType               = UiaLookupId(AutomationIdentifierType_ControlType, &Button_Control_GUID);
    m_pUIAIds->Calendar_ControlType             = UiaLookupId(AutomationIdentifierType_ControlType, &Calendar_Control_GUID);
    m_pUIAIds->CheckBox_ControlType             = UiaLookupId(AutomationIdentifierType_ControlType, &CheckBox_Control_GUID);
    m_pUIAIds->ComboBox_ControlType             = UiaLookupId(AutomationIdentifierType_ControlType, &ComboBox_Control_GUID);
    m_pUIAIds->Custom_ControlType               = UiaLookupId(AutomationIdentifierType_ControlType, &Custom_Control_GUID);
    m_pUIAIds->DataGrid_ControlType             = UiaLookupId(AutomationIdentifierType_ControlType, &DataGrid_Control_GUID);
    m_pUIAIds->DataItem_ControlType             = UiaLookupId(AutomationIdentifierType_ControlType, &DataItem_Control_GUID);
    m_pUIAIds->Document_ControlType             = UiaLookupId(AutomationIdentifierType_ControlType, &Document_Control_GUID);
    m_pUIAIds->Edit_ControlType                 = UiaLookupId(AutomationIdentifierType_ControlType, &Edit_Control_GUID);
    m_pUIAIds->Group_ControlType                = UiaLookupId(AutomationIdentifierType_ControlType, &Group_Control_GUID);
    m_pUIAIds->Header_ControlType               = UiaLookupId(AutomationIdentifierType_ControlType, &Header_Control_GUID);
    m_pUIAIds->HeaderItem_ControlType           = UiaLookupId(AutomationIdentifierType_ControlType, &HeaderItem_Control_GUID);
    m_pUIAIds->Hyperlink_ControlType            = UiaLookupId(AutomationIdentifierType_ControlType, &Hyperlink_Control_GUID);
    m_pUIAIds->Image_ControlType                = UiaLookupId(AutomationIdentifierType_ControlType, &Image_Control_GUID);
    m_pUIAIds->List_ControlType                 = UiaLookupId(AutomationIdentifierType_ControlType, &List_Control_GUID);
    m_pUIAIds->ListItem_ControlType             = UiaLookupId(AutomationIdentifierType_ControlType, &ListItem_Control_GUID);
    m_pUIAIds->Menu_ControlType                 = UiaLookupId(AutomationIdentifierType_ControlType, &Menu_Control_GUID);
    m_pUIAIds->MenuBar_ControlType              = UiaLookupId(AutomationIdentifierType_ControlType, &MenuBar_Control_GUID);
    m_pUIAIds->MenuItem_ControlType             = UiaLookupId(AutomationIdentifierType_ControlType, &MenuItem_Control_GUID);
    m_pUIAIds->Pane_ControlType                 = UiaLookupId(AutomationIdentifierType_ControlType, &Pane_Control_GUID);
    m_pUIAIds->ProgressBar_ControlType          = UiaLookupId(AutomationIdentifierType_ControlType, &ProgressBar_Control_GUID);
    m_pUIAIds->RadioButton_ControlType          = UiaLookupId(AutomationIdentifierType_ControlType, &RadioButton_Control_GUID);
    m_pUIAIds->ScrollBar_ControlType            = UiaLookupId(AutomationIdentifierType_ControlType, &ScrollBar_Control_GUID);
    m_pUIAIds->Separator_ControlType            = UiaLookupId(AutomationIdentifierType_ControlType, &Separator_Control_GUID);
    m_pUIAIds->Slider_ControlType               = UiaLookupId(AutomationIdentifierType_ControlType, &Slider_Control_GUID);
    m_pUIAIds->Spinner_ControlType              = UiaLookupId(AutomationIdentifierType_ControlType, &Spinner_Control_GUID);
    m_pUIAIds->SplitButton_ControlType          = UiaLookupId(AutomationIdentifierType_ControlType, &SplitButton_Control_GUID);
    m_pUIAIds->StatusBar_ControlType            = UiaLookupId(AutomationIdentifierType_ControlType, &StatusBar_Control_GUID);
    m_pUIAIds->SemanticZoom_ControlType         = UiaLookupId(AutomationIdentifierType_ControlType, &SemanticZoom_Control_GUID);
    m_pUIAIds->Tab_ControlType                  = UiaLookupId(AutomationIdentifierType_ControlType, &Tab_Control_GUID);
    m_pUIAIds->TabItem_ControlType              = UiaLookupId(AutomationIdentifierType_ControlType, &TabItem_Control_GUID);
    m_pUIAIds->Table_ControlType                = UiaLookupId(AutomationIdentifierType_ControlType, &Table_Control_GUID);
    m_pUIAIds->Text_ControlType                 = UiaLookupId(AutomationIdentifierType_ControlType, &Text_Control_GUID);
    m_pUIAIds->Thumb_ControlType                = UiaLookupId(AutomationIdentifierType_ControlType, &Thumb_Control_GUID);
    m_pUIAIds->TitleBar_ControlType             = UiaLookupId(AutomationIdentifierType_ControlType, &TitleBar_Control_GUID);
    m_pUIAIds->ToolBar_ControlType              = UiaLookupId(AutomationIdentifierType_ControlType, &ToolBar_Control_GUID);
    m_pUIAIds->ToolTip_ControlType              = UiaLookupId(AutomationIdentifierType_ControlType, &ToolTip_Control_GUID);
    m_pUIAIds->Tree_ControlType                 = UiaLookupId(AutomationIdentifierType_ControlType, &Tree_Control_GUID);
    m_pUIAIds->TreeItem_ControlType             = UiaLookupId(AutomationIdentifierType_ControlType, &TreeItem_Control_GUID);
    m_pUIAIds->Window_ControlType               = UiaLookupId(AutomationIdentifierType_ControlType, &Window_Control_GUID);
    m_pUIAIds->AppBar_ControlType               = UiaLookupId(AutomationIdentifierType_ControlType, &AppBar_Control_GUID);

    m_pUIAIds->AsyncContentLoaded_Event                                 = UiaLookupId(AutomationIdentifierType_Event, &AsyncContentLoaded_Event_GUID);
    m_pUIAIds->AutomationFocusChanged_Event                             = UiaLookupId(AutomationIdentifierType_Event, &AutomationFocusChanged_Event_GUID);
    m_pUIAIds->AutomationPropertyChanged_Event                          = UiaLookupId(AutomationIdentifierType_Event, &AutomationPropertyChanged_Event_GUID);
    m_pUIAIds->Drag_DragCancel_Event                                    = UiaLookupId(AutomationIdentifierType_Event, &Drag_DragCancel_Event_GUID);
    m_pUIAIds->Drag_DragComplete_Event                                  = UiaLookupId(AutomationIdentifierType_Event, &Drag_DragComplete_Event_GUID);
    m_pUIAIds->Drag_DragStart_Event                                     = UiaLookupId(AutomationIdentifierType_Event, &Drag_DragStart_Event_GUID);
    m_pUIAIds->DropTarget_DragEnter_Event                               = UiaLookupId(AutomationIdentifierType_Event, &DropTarget_DragEnter_Event_GUID);
    m_pUIAIds->DropTarget_DragLeave_Event                               = UiaLookupId(AutomationIdentifierType_Event, &DropTarget_DragLeave_Event_GUID);
    m_pUIAIds->DropTarget_Dropped_Event                                 = UiaLookupId(AutomationIdentifierType_Event, &DropTarget_Dropped_Event_GUID);
    m_pUIAIds->Invoke_Invoked_Event                                     = UiaLookupId(AutomationIdentifierType_Event, &Invoke_Invoked_Event_GUID);
    m_pUIAIds->MenuClosed_Event                                         = UiaLookupId(AutomationIdentifierType_Event, &MenuClosed_Event_GUID);
    m_pUIAIds->MenuOpened_Event                                         = UiaLookupId(AutomationIdentifierType_Event, &MenuOpened_Event_GUID);
    m_pUIAIds->Selection_InvalidatedEvent_Event                         = UiaLookupId(AutomationIdentifierType_Event, &Selection_InvalidatedEvent_Event_GUID);
    m_pUIAIds->SelectionItem_ElementAddedToSelectionEvent_Event         = UiaLookupId(AutomationIdentifierType_Event, &SelectionItem_ElementAddedToSelectionEvent_Event_GUID);
    m_pUIAIds->SelectionItem_ElementRemovedFromSelectionEvent_Event     = UiaLookupId(AutomationIdentifierType_Event, &SelectionItem_ElementRemovedFromSelectionEvent_Event_GUID);
    m_pUIAIds->SelectionItem_ElementSelectedEvent_Event                 = UiaLookupId(AutomationIdentifierType_Event, &SelectionItem_ElementSelectedEvent_Event_GUID);
    m_pUIAIds->StructureChanged_Event                                   = UiaLookupId(AutomationIdentifierType_Event, &StructureChanged_Event_GUID);
    m_pUIAIds->Text_TextChangedEvent_Event                              = UiaLookupId(AutomationIdentifierType_Event, &Text_TextChangedEvent_Event_GUID);
    m_pUIAIds->Text_TextSelectionChangedEvent_Event                     = UiaLookupId(AutomationIdentifierType_Event, &Text_TextSelectionChangedEvent_Event_GUID);
    m_pUIAIds->ToolTipClosed_Event                                      = UiaLookupId(AutomationIdentifierType_Event, &ToolTipClosed_Event_GUID);
    m_pUIAIds->ToolTipOpened_Event                                      = UiaLookupId(AutomationIdentifierType_Event, &ToolTipOpened_Event_GUID);
    m_pUIAIds->LiveRegionChanged_Event                                  = UiaLookupId(AutomationIdentifierType_Event, &LiveRegionChanged_Event_GUID);
    m_pUIAIds->InputReachedTarget_Event                                 = UiaLookupId(AutomationIdentifierType_Event, &InputReachedTarget_Event_GUID);
    m_pUIAIds->InputReachedOtherElement_Event                           = UiaLookupId(AutomationIdentifierType_Event, &InputReachedOtherElement_Event_GUID);
    m_pUIAIds->InputDiscarded_Event                                     = UiaLookupId(AutomationIdentifierType_Event, &InputDiscarded_Event_GUID);
    m_pUIAIds->WindowClosed_Event                                       = UiaLookupId(AutomationIdentifierType_Event, &Window_WindowClosed_Event_GUID);
    m_pUIAIds->WindowOpened_Event                                       = UiaLookupId(AutomationIdentifierType_Event, &Window_WindowOpened_Event_GUID);
    m_pUIAIds->TextEdit_TextChanged_Event                               = UiaLookupId(AutomationIdentifierType_Event, &TextEdit_TextChanged_Event_GUID);
    m_pUIAIds->TextEdit_ConversionTargetChanged_Event                   = UiaLookupId(AutomationIdentifierType_Event, &TextEdit_ConversionTargetChanged_Event_GUID);
    m_pUIAIds->LayoutInvalidated_Event                                  = UiaLookupId(AutomationIdentifierType_Event, &LayoutInvalidated_Event_GUID);
    m_pUIAIds->Notification_Event                                       = UiaLookupId(AutomationIdentifierType_Event, &Notification_Event_GUID);

    m_pUIAIds->Text_AnimationStyle_Attribute        = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_AnimationStyle_Attribute_GUID);
    m_pUIAIds->Text_BackgroundColor_Attribute       = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_BackgroundColor_Attribute_GUID);
    m_pUIAIds->Text_BulletStyle_Attribute           = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_BulletStyle_Attribute_GUID);
    m_pUIAIds->Text_CapStyle_Attribute              = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_CapStyle_Attribute_GUID);
    m_pUIAIds->Text_Culture_Attribute               = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_Culture_Attribute_GUID);
    m_pUIAIds->Text_FontName_Attribute              = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_FontName_Attribute_GUID);
    m_pUIAIds->Text_FontSize_Attribute              = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_FontSize_Attribute_GUID);
    m_pUIAIds->Text_FontWeight_Attribute            = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_FontWeight_Attribute_GUID);
    m_pUIAIds->Text_ForegroundColor_Attribute       = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_ForegroundColor_Attribute_GUID);
    m_pUIAIds->Text_HorizontalTextAlignment_Attribute   = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_HorizontalTextAlignment_Attribute_GUID);
    m_pUIAIds->Text_IndentationFirstLine_Attribute      = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_IndentationFirstLine_Attribute_GUID);
    m_pUIAIds->Text_IndentationLeading_Attribute        = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_IndentationLeading_Attribute_GUID);
    m_pUIAIds->Text_IndentationTrailing_Attribute       = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_IndentationTrailing_Attribute_GUID);
    m_pUIAIds->Text_IsHidden_Attribute                  = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_IsHidden_Attribute_GUID);
    m_pUIAIds->Text_IsItalic_Attribute                  = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_IsItalic_Attribute_GUID);
    m_pUIAIds->Text_IsReadOnly_Attribute                = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_IsReadOnly_Attribute_GUID);
    m_pUIAIds->Text_IsSubscript_Attribute               = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_IsSubscript_Attribute_GUID);
    m_pUIAIds->Text_IsSuperscript_Attribute             = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_IsSuperscript_Attribute_GUID);
    m_pUIAIds->Text_MarginBottom_Attribute              = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_MarginBottom_Attribute_GUID);
    m_pUIAIds->Text_MarginLeading_Attribute             = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_MarginLeading_Attribute_GUID);
    m_pUIAIds->Text_MarginTop_Attribute                 = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_MarginTop_Attribute_GUID);
    m_pUIAIds->Text_MarginTrailing_Attribute            = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_MarginTrailing_Attribute_GUID);
    m_pUIAIds->Text_OutlineStyles_Attribute             = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_OutlineStyles_Attribute_GUID);
    m_pUIAIds->Text_OverlineColor_Attribute             = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_OverlineColor_Attribute_GUID);
    m_pUIAIds->Text_OverlineStyle_Attribute             = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_OverlineStyle_Attribute_GUID);
    m_pUIAIds->Text_StrikethroughColor_Attribute        = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_StrikethroughColor_Attribute_GUID);
    m_pUIAIds->Text_StrikethroughStyle_Attribute        = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_StrikethroughStyle_Attribute_GUID);
    m_pUIAIds->Text_Tabs_Attribute                      = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_Tabs_Attribute_GUID);
    m_pUIAIds->Text_TextFlowDirections_Attribute        = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_TextFlowDirections_Attribute_GUID);
    m_pUIAIds->Text_UnderlineColor_Attribute            = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_UnderlineColor_Attribute_GUID);
    m_pUIAIds->Text_UnderlineStyle_Attribute            = UiaLookupId(AutomationIdentifierType_TextAttribute, &Text_UnderlineStyle_Attribute_GUID);

    return S_OK;
}
// Constructor.
CUIAWindow::CUIAWindow(_In_ const UIAHostEnvironmentInfo& uiaHostEnvironmentInfo, _In_ IXcpHostSite *pHost)
    : m_uiaHostEnvironmentInfo(uiaHostEnvironmentInfo)
{
    XCP_WEAK(&m_pHost);
    m_pHost = pHost;
}

CUIAWindow::~CUIAWindow()
{
    // Deinit is called from CJupiterControl when it is destroying.
    ASSERT(!m_fInitialized);
}

HRESULT CUIAWindow::Init()
{
    m_pUIAIds = new UIAIdentifiers;
    IFC_RETURN(InitIds());

    // Get UIA window validator
    m_pUIAWindowValidator = GetUIAWindowValidator();

    if (auto contentIslandAutomation = GetContentIslandAutomation())
    {
        IFCFAILFAST(contentIslandAutomation->get_AutomationOption(&m_automationOption));
    }

    m_fInitialized = true;
    return S_OK;
}

void CUIAWindow::Deinit()
{
    // Flush the existing events provider to avoid the memory leak
    FlushUiaBridgeEventTable();

    m_pHost = nullptr;
    m_uiaHostEnvironmentInfo = {};
    delete m_pUIAIds;
    m_pUIAIds = nullptr;

    if (m_pUIAWindowValidator)
    {
        m_pUIAWindowValidator->Invalidate();
    }
    ReleaseInterface(m_pUIAWindowValidator);

    m_fInitialized = false;
}

void CUIAWindow::UIADisconnectAllProviders()
{
    // Disconnecting this window provider from UIAutomationCore to help prevent leaking.
    // We switched to UiaDisconnectProvider late in Win8 GA and could not take the risk of an unimportant HR crashing the app, hence the IGNOREHR.
    IGNOREHR(UiaDisconnectProvider(this));
}

// IUnknown implementation.
ULONG STDMETHODCALLTYPE CUIAWindow::AddRef()
{
    return CInterlockedReferenceCount::AddRef();
}
ULONG STDMETHODCALLTYPE CUIAWindow::Release()
{
    return CInterlockedReferenceCount::Release();
}
HRESULT STDMETHODCALLTYPE CUIAWindow::QueryInterface(_In_ REFIID riid, _Outptr_ void** ppInterface)
{
    if (riid == __uuidof(IUnknown))
    {
        *ppInterface =(IUnknown*)((IRawElementProviderSimple*)this);
    }
    else if (riid == __uuidof(IInspectable))
    {
        *ppInterface =(IInspectable*)this;
    }
    else if (riid == __uuidof(IRawElementProviderSimple))
    {
        *ppInterface =(IRawElementProviderSimple*)this;
    }
    else if (riid == __uuidof(IRawElementProviderFragment))
    {
        *ppInterface =(IRawElementProviderFragment*)this;
    }
    else if (riid == __uuidof(IRawElementProviderFragmentRoot))
    {
        *ppInterface =(IRawElementProviderFragmentRoot*)this;
    }
    else if (riid == __uuidof(IRawElementProviderAdviseEvents))
    {
        *ppInterface =(IRawElementProviderAdviseEvents*)this;
    }
    else if (riid == __uuidof(IRawElementProviderHwndOverride))
    {
        *ppInterface =(IRawElementProviderHwndOverride*)this;
    }
    else
    {
        *ppInterface = nullptr;
        return E_NOINTERFACE;
    }
    ((IUnknown*)(*ppInterface))->AddRef();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CUIAWindow::GetIids(ULONG* iidCount, IID** iids)
{
    static const IID* const uiaWindowIIDs[] = {
        &__uuidof(IRawElementProviderSimple),
        &__uuidof(IRawElementProviderFragment),
        &__uuidof(IRawElementProviderFragmentRoot),
        &__uuidof(IRawElementProviderAdviseEvents),
        &__uuidof(IRawElementProviderHwndOverride),
    };

    IID* iidArray = static_cast<IID*>(::CoTaskMemAlloc(sizeof(IID) * ARRAY_SIZE(uiaWindowIIDs)));
    for (unsigned int i = 0; i < ARRAY_SIZE(uiaWindowIIDs); ++i)
    {
        iidArray[i] = *uiaWindowIIDs[i];
    }

    *iidCount = ARRAY_SIZE(uiaWindowIIDs);
    *iids = iidArray;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CUIAWindow::GetRuntimeClassName(HSTRING *className)
{
    constexpr wchar_t classNameStr[] = L"CUIAWindow";
    return wrl_wrappers::HStringReference(classNameStr, SZ_COUNT(classNameStr)).CopyTo(className);
}

HRESULT STDMETHODCALLTYPE CUIAWindow::GetTrustLevel(TrustLevel *trustLevel)
{
    *trustLevel = TrustLevel::BaseTrust;
    return S_OK;
}

// IRawElementProviderSimple implementation
// Get Provider options.
HRESULT STDMETHODCALLTYPE CUIAWindow::get_ProviderOptions( _Out_ ProviderOptions* pRetVal )
{
    if (pRetVal == nullptr) return E_INVALIDARG;

    *pRetVal = ProviderOptions_ServerSideProvider | ProviderOptions_UseComThreading;

    return S_OK;
}
// Get the object that supports IInvokePattern.
HRESULT STDMETHODCALLTYPE CUIAWindow::GetPatternProvider(_In_ PATTERNID patternId, _Out_ IUnknown** pRetVal)
{
    *pRetVal = nullptr;
    return S_OK;
}

// Gets custom properties.
HRESULT STDMETHODCALLTYPE CUIAWindow::GetPropertyValue(_In_ PROPERTYID propertyId, _Out_ VARIANT* pRetVal)
{
    if (!m_fInitialized)
    {
        IFC_NOTRACE_RETURN(static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
    }
    if (propertyId == m_pUIAIds->ControlType_Property)
    {
        pRetVal->vt = VT_I4;
        if (m_uiaHostEnvironmentInfo.IsHwndBased())
        {
            pRetVal->lVal = m_pUIAIds->Window_ControlType;
        }
        else
        {
            pRetVal->lVal = m_pUIAIds->Pane_ControlType;
        }
    }
    else if (propertyId == m_pUIAIds->RuntimeId_Property)
    {
        pRetVal = nullptr;
    }
    else
    {
        // Allow derived class to return values for additional properties
        IFC_RETURN(GetPropertyValueOverride(propertyId, pRetVal));
    }
    return S_OK;
}

// Gets the UI Automation CUIAWindow for the host window. This CUIAWindow supplies most properties.
HRESULT STDMETHODCALLTYPE CUIAWindow::get_HostRawElementProvider(_Out_ IRawElementProviderSimple** pRetVal)
{
    if (pRetVal == nullptr) IFC_RETURN(E_INVALIDARG);
    *pRetVal = nullptr;

    // This is the UWP case.
    if (m_uiaHostEnvironmentInfo.IsHwndBased())
    {
        IFC_RETURN(UiaHostProviderFromHwnd(m_uiaHostEnvironmentInfo.GetElementWindow(), pRetVal));
        return S_OK;
    }

    // This is the Win32 case.
    if (xref_ptr<ixp::IContentIsland> contentIsland = GetContentIsland())
    {
        if (IsAutomationOptionFragmentBased())
        {
            // Since we're not the root of a fragment tree, we return null here.
            return S_OK;
        }
        else
        {
            wrl::ComPtr<IInspectable> automationHostProvider;
            IFC_RETURN(contentIsland->GetAutomationHostProvider(&automationHostProvider));
            if (automationHostProvider)
            {
                IFC_RETURN(automationHostProvider->QueryInterface(IID_PPV_ARGS(pRetVal)));
            }
            return S_OK;
        }
    }

    return UIA_E_ELEMENTNOTAVAILABLE;
}

// IRawElementProviderFragment implementation
// Gets the bounding rectangle of this element
HRESULT STDMETHODCALLTYPE CUIAWindow::get_BoundingRectangle(_Out_ UiaRect * pRetVal)
{
    UIA_TRACE(L"CUIAWindow::get_BoundingRectangle");
    if (pRetVal == nullptr) return E_INVALIDARG;
    HRESULT hr = E_NOTIMPL;
    XRECTF rect = { 0, 0, 0, 0 };
    if (ShouldFrameworkProvideWindowProperties())
    {
        // On desktop, UIACore will provide this for us through HwndProxy
        // On other platforms however we need to return these properties explicitly since there's no backing hwnd
        XUINT32 value;
        hr = m_pHost->GetActualWidth(&value);
        if (SUCCEEDED(hr))
        {
            rect.Width = static_cast<float>(value);
            hr = m_pHost->GetActualHeight(&value);
            if (SUCCEEDED(hr))
            {
                rect.Height = static_cast<float>(value);
                hr = S_OK;
            }
        }
    }
    else
    {
        // This gives the wrong answer for the Popup case; we return the island's bounds instead of the Popup's bounds.
        // The UIA parent of the Popup has the correct bounds, and the UIA child has the correct bounds, so in practice
        // it doesn't seem to be causing big problems.
        xref_ptr<CXamlIslandRoot> islandRoot = m_uiaHostEnvironmentInfo.GetXamlIslandRoot();
        
        if (islandRoot)
        {
            const float scale = RootScale::GetRasterizationScaleForElementWithFallback(islandRoot.get());
            rect.Width = islandRoot->GetActualWidth() * scale;
            rect.Height = islandRoot->GetActualHeight() * scale;
            hr = S_OK;
        }
    }

    if (SUCCEEDED(hr))
    {
        VERIFY(TransformClientToScreen(GetHostEnvironmentInfo(), &rect));

        pRetVal->left = rect.X;
        pRetVal->top = rect.Y;
        pRetVal->width = rect.Width;
        pRetVal->height = rect.Height;
    }

    UIA_TRACE(L"CUIAWindow::get_BoundingRectangle return %x", hr);
    return hr;
}

// Gets the root node of the fragment
HRESULT STDMETHODCALLTYPE CUIAWindow::get_FragmentRoot(_Out_ IRawElementProviderFragmentRoot** pRetVal)
{
    UIA_TRACE(L"CUIAWindow::get_FragmentRoot");
    if (pRetVal == nullptr) return E_INVALIDARG;

    if (!m_pUIAWindowValidator || !m_pUIAWindowValidator->IsValid())
    {
        return E_FAIL;
    }

    *pRetVal = nullptr;

    if (IsAutomationOptionFragmentBased())
    {
        // In fragment-based automation trees, we're rooted by a fragment root owned by someone else.
        if (auto contentIslandAutomation = GetContentIslandAutomation())
        {
            wrl::ComPtr<IInspectable> fragmentRoot;
            IFCFAILFAST(contentIslandAutomation->get_FragmentRootAutomationProvider(&fragmentRoot));
            if (fragmentRoot)
            {
                fragmentRoot.CopyTo(pRetVal);
            }
        }
    }
    else
    {
        wrl::ComPtr<IRawElementProviderFragmentRoot> thisPtr(this);
        *pRetVal = thisPtr.Detach();
    }
    
    UIA_TRACE(L"CUIAWindow::get_FragmentRoot return S_OK");
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CUIAWindow::GetEmbeddedFragmentRoots(_Outptr_result_maybenull_ SAFEARRAY** retVal)
{
    *retVal = nullptr;
    return S_OK;
}

// Retrieves the runtime identifier of an element
HRESULT STDMETHODCALLTYPE CUIAWindow::GetRuntimeId(_Out_ SAFEARRAY ** pRetVal)
{
    HRESULT hr = S_OK;

    if (pRetVal == nullptr)
    {
        return E_INVALIDARG;
    }

    if (!m_fInitialized)
    {
        IFC_RETURN(E_UNEXPECTED);
    }

    // On the desktop, uiautomationcore provides an implicit RuntimeId for this
    // object because it is associated with an HWND, so there is no need to
    // return a value.
    //
    // The uiautomationcore implementation on non-desktop skus are different
    // and this provider node is not associated with an HWND in the same way
    // that it is on the desktop.  We do need to explicitly provide a RuntimeId
    // here.
    if (ShouldFrameworkProvideWindowProperties())
    {
        int rId[] = { UiaAppendRuntimeId, static_cast<int>(reinterpret_cast<uintptr_t>(m_uiaHostEnvironmentInfo.GetElementWindow())) };

        unique_safearray safeArray(SafeArrayCreateVector(VT_I4, 0, _countof(rId)));
        IFCOOMFAILFAST(safeArray.get());
        for (LONG i = 0; i < _countof(rId); i++)
        {
            IFC_RETURN(SafeArrayPutElement(safeArray.get(), &i, &(rId[i])));
        }

        *pRetVal = safeArray.release();
    }
    else
    {
        *pRetVal = nullptr;
    }

    return hr;
}
// Retrieves the UI Automation element in a specified direction within the UI Automation tree
HRESULT STDMETHODCALLTYPE CUIAWindow::Navigate(_In_ NavigateDirection direction, _Out_ IRawElementProviderFragment ** pRetVal)
{
    UIA_TRACE(L"CUIAWindow::Navigate %d", direction);

    HRESULT hr = S_OK;

    if (!m_fInitialized)
    {
        IFC_NOTRACE(static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
    }

    IFC(NavigateImpl(direction, pRetVal));

Cleanup:
    UIA_TRACE(L"CUIAWindow::Navigate return %x", hr);
    RRETURN(hr);
}

XUINT32 CUIAWindow::GetAutomationPeersForRoot(CUIElement* root, _Outptr_result_buffer_(return) CAutomationPeer*** rootPeersOut)
{
    CAutomationPeer* rootPeer = nullptr;
    CAutomationPeer** rootPeers = nullptr;
    XUINT32 childrenCount = 0;

    rootPeer = root->OnCreateAutomationPeer();

    if (rootPeer == nullptr)
    {
        // UIA takes a dependency on the fact that the root element (e.g. Frame) usually doesn't have an automation peer.
        // In that case, we can just call into GetAPChildren, which will return a list of the automation peers of the
        // nearest descendants with peers. The list returned by GetAPChildren will also include the peers of any open
        // unparented popups.
        childrenCount = root->GetAPChildren(&rootPeers);
    }
    else
    {
        bool isWindowedPopup = false;
        if (auto cpopup = do_pointer_cast<CPopup>(root))
        {
            isWindowedPopup = cpopup->IsWindowed();
        }

        // If the root is a windowed Popup, return its AP
        if (isWindowedPopup)
        {
            rootPeers = new CAutomationPeer*[1];
            rootPeers[0] = rootPeer;
            childrenCount = 1;
        }
        else
        {
            // Sometimes the root element does have an automation peer, because it had an automation peer property set on
            // it. In those cases we have to manually build a list of automation peers of open unparented popups.

            XINT32 popupCount = root->GetAPPopupChildrenCount();

            rootPeers = new CAutomationPeer*[popupCount + 1];
            XINT32 rootPeerIndex = 0;

            if (popupCount > 0)
            {
                // Note: We're putting popup peers before (under) the root peer to stay consistent with the case without
                // a peer on Frame. They should be placed after (on top).
                rootPeerIndex = root->AppendAPPopupChildren(popupCount, rootPeers);
                ASSERT(rootPeerIndex == popupCount);
            }

            rootPeers[rootPeerIndex] = rootPeer;
            childrenCount = rootPeerIndex + 1;
        }
    }

    *rootPeersOut = rootPeers;
    return childrenCount;
}

HRESULT STDMETHODCALLTYPE CUIAWindow::NavigateImpl(_In_ NavigateDirection direction, _Out_ IRawElementProviderFragment ** pRetVal)
{
    HRESULT hr = S_OK;
    xref_ptr<CUIAWrapper> wrapper;
    xref_ptr<CDependencyObject> pDO;
    IRawElementProviderFragment* pFrag = nullptr;
    CAutomationPeer** ppChildren = nullptr;

    if (pRetVal == nullptr)
    {
        IFC(E_INVALIDARG);
    }

    if (IsAutomationOptionFragmentBased())
    {
        // Xaml is being hosted as part of a larger fragment tree, so we need to ask the ContentIsland
        // interfaces who our parent and siblings are.
        if (auto contentIslandAutomation = GetContentIslandAutomation())
        {
            wrl::ComPtr<IInspectable> nextProvider;

            switch (direction)
            {
                case NavigateDirection_Parent:
                    IFCFAILFAST(contentIslandAutomation->get_ParentAutomationProvider(&nextProvider));
                    break;
                
                case NavigateDirection_NextSibling:
                    IFCFAILFAST(contentIslandAutomation->get_NextSiblingAutomationProvider(&nextProvider));
                    break;

                case NavigateDirection_PreviousSibling:
                    IFCFAILFAST(contentIslandAutomation->get_PreviousSiblingAutomationProvider(&nextProvider));
                    break;
            }

            if (nextProvider)
            {
                IFCFAILFAST(nextProvider.CopyTo(&pFrag));
            }
        }
    }

    if (pFrag == nullptr)
    {
        switch(direction)
        {
            case NavigateDirection_FirstChild:
            case NavigateDirection_LastChild:
                IFC(GetRootVisual(pDO));
                if (pDO != nullptr)
                {
                    ASSERT(pDO->OfTypeByIndex<KnownTypeIndex::UIElement>());
                    CUIElement* root = static_cast<CUIElement*>(pDO.get());

                    XUINT32 peerCount = GetAutomationPeersForRoot(root, &ppChildren);

                    if (peerCount > 0)
                    {
                        CAutomationPeer* pAP = (direction == NavigateDirection_FirstChild) ? ppChildren[0] : ppChildren[peerCount - 1];
                        ASSERT(pAP != nullptr);

                        CAutomationPeer* pAPEventsSource = pAP->GetAPEventsSource();
                        if (pAPEventsSource)
                        {
                            pAP = pAPEventsSource;
                        }
                        IFC(CreateProviderForAP(pAP, wrapper.ReleaseAndGetAddressOf()));
                        IFC(wrapper->QueryInterface(__uuidof(IRawElementProviderFragment), reinterpret_cast<void**>(&pFrag)));
                    }
                }
                break;
        }
    }

    *pRetVal = pFrag;

Cleanup:
    delete [] ppChildren;
    return hr;
}

// Sets the focus to this element
HRESULT STDMETHODCALLTYPE CUIAWindow::SetFocus()
{
    return S_OK;
}

// IRawElementProviderFragmentRoot implementation
// Gets the Provider at a given point
HRESULT STDMETHODCALLTYPE CUIAWindow::ElementProviderFromPoint(_In_ double x, _In_ double y, _Out_ IRawElementProviderFragment** pRetVal)
{
    if (!m_fInitialized)
    {
        IFC_NOTRACE_RETURN(static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
    }

    IFC_RETURN(ElementProviderFromPointImpl(x, y, pRetVal))

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CUIAWindow::ElementProviderFromPointImpl(_In_ double x, _In_ double y, _Out_ IRawElementProviderFragment** pRetVal)
{
    if (pRetVal == nullptr)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    xref_ptr<CDependencyObject> spHitTestDO;
    CAutomationPeer** pAPChildren = nullptr;

    // We walk up the automation peer tree twice, to do this
    // we need to keep track of the original first AP.
    CAutomationPeer* pFirstFoundAP = nullptr;

    // The AP we're actually going to return as the result of this function.
    xref_ptr<CAutomationPeer> spResultAP;

    xref_ptr<IRawElementProviderFragment> spFrag;
    xstack<xref_ptr<CAutomationPeer>> targetStack;

    // Should we return a pointer to this object if something goes wrong?
    bool shouldFallbackToSelf {true};

    XPOINTF xptPhysical = {0};
    {
        POINT pt;
        pt.x = (LONG)x;
        pt.y = (LONG)y;
        VERIFY(TransformScreenToClient(GetHostEnvironmentInfo(), &pt));
        xptPhysical.x = (XFLOAT)(pt.x);
        xptPhysical.y = (XFLOAT)(pt.y);
    }

    // xptPhysical is in physical coordinates so let's convert it to logical coordinates for hit-testing.
    xref_ptr<CXamlIslandRoot> root = GetHostEnvironmentInfo().GetXamlIslandRoot();
    const float scale = RootScale::GetRasterizationScaleForElementWithFallback(root.get());
    const XPOINTF xptLogical = { xptPhysical.x / scale, xptPhysical.y / scale };

    // Step 0: Check to see if the point is within the XamlIsland.  In Fragment-based navigation scenarios,
    // it's possible apps may be calling this function directly, and we may get points that are outside the
    // bounds of the XamlIsland.
    if (root && IsAutomationOptionFragmentBased())
    {
        const auto xamlIslandRootSize = root->GetSize();
        if (xptLogical.x < 0 || xptLogical.y < 0 || xptLogical.x > xamlIslandRootSize.Width || xptLogical.y > xamlIslandRootSize.Height)
        {
            shouldFallbackToSelf = false;
        }
    }

    // Step 1: Perform actual hit testing and get the UIElement directly under
    // the point passed in.
    {
        IXcpBrowserHost *pBH = nullptr;
        CCoreServices *pCore = nullptr;
        pBH = m_pHost->GetBrowserHost();
        if (pBH)
        {
            pCore = pBH->GetContextInterface();
        }

        if (pCore)
        {
            xref_ptr<CDependencyObject> spLinkDO;

            // NOTE: HitTest returns a ref on pDO that we must release. This is different
            // than most UIA code, which doesn't usually take refs. bHitTestTarget below
            // is used to ensure we release this ref.
            // root may be null -- that's OK, HitTest will tolerate null and fall back to the main content.
            IFC(pCore->HitTest(xptLogical, root, spHitTestDO.ReleaseAndGetAddressOf(), true /*fHitDisabledElement*/));

            if (spHitTestDO)
            {
                IFC(pCore->HitTestLinkFromTextControl(xptLogical, spHitTestDO, spLinkDO.ReleaseAndGetAddressOf()));
            }

            if (spLinkDO)
            {
                pFirstFoundAP = spLinkDO->OnCreateAutomationPeer();
            }
        }
    }

    // Step 2: Walk up the visual tree and look for the first UIElement
    // that has an automation peer.
    if (!pFirstFoundAP)
    {
        CAutomationPeer* pAP = nullptr;
        CDependencyObject *pDOParentNoRef = nullptr;
        CDependencyObject *pDONoRef = spHitTestDO;
        while (pDONoRef)
        {
            pAP = pDONoRef->OnCreateAutomationPeer();
            if (pAP)
            {
                pFirstFoundAP = pAP;
                break;
            }

            pDOParentNoRef = pDONoRef->GetParent();
            pDONoRef = pDOParentNoRef;
            pDOParentNoRef = nullptr;
        }
    }

    // Once we've found an automation peer we're going to validate that
    // it's part of a valid UIA tree. In scenarios where we override
    // the GetChildren method on the AutomationPeer we 'hide' APs. If that
    // AP was found via hit-testing we can't return it to the UIA client
    // because it isn't part of a self-consistent tree (its parent won't
    // report it in its children collection).

    // Step 3.1: Walk up the AutomationPeers and call GetChildren on each
    // AutomationPeer. This will sometimes cause reparenting to occur. When
    // an AutomationPeer hides an intermediate AutomationPeer between the
    // hit-test AP and the root this walk will retern a different set of
    // items after GetChildren has been called on every item. For example
    // ListView hides the intermediate ScrollViewer's AP. While ListViewItems
    // have special logic to account for this (APEventsSource), ListView Headers
    // and footers do not. Walking up the tree from a ListView Header's AP will
    // produce the intermediate ScrollViewer, until after ListView's GetChildren
    // method is called and sets the parent of the ListView Header's AP to be
    // the ListView itself.
    {
        xref_ptr<CAutomationPeer> spAP(pFirstFoundAP);
        while (spAP)
        {
            xref_ptr<IUnknown> spUnkParentNativeNode;
            xref_ptr<CAutomationPeer> spAPParent;

            CAutomationPeer* pAPEventsSourceNoRef = spAP->GetAPEventsSource();
            XINT32 cChildren = 0;
            if (pAPEventsSourceNoRef)
            {
                spAP = pAPEventsSourceNoRef;
            }

            cChildren = spAP->GetChildren(&pAPChildren);
            delete[] pAPChildren;
            pAPChildren = nullptr;

            IFC(spAP->Navigate(UIAXcp::AutomationNavigationDirection_Parent, spAPParent.ReleaseAndGetAddressOf(), spUnkParentNativeNode.ReleaseAndGetAddressOf()));
            spAP = spAPParent;

            if (spUnkParentNativeNode)
            {
                ASSERT(spAP == nullptr);
            }
        }
    }

    // Step 3.2: Again walk up the AutomationPeer tree and place each parent
    // on the stack. During step 3.1 when calling GetChildren it's possible that
    // an overridden version of GetChildren hid one of the AutomationPeers that
    // was in the target stack. In that case the stack will be different this time.
    {
        xref_ptr<CAutomationPeer> spAP(pFirstFoundAP);
        while (spAP)
        {
            xref_ptr<IUnknown> spUnkParentNativeNode;
            xref_ptr<CAutomationPeer> spAPEventsSource;
            xref_ptr<CAutomationPeer> spAPParent;

            spAPEventsSource = spAP->GetAPEventsSource();
            if (spAPEventsSource)
            {
                spAP = spAPEventsSource;
            }

            IFC(targetStack.push(spAP));

            IFC(spAP->Navigate(UIAXcp::AutomationNavigationDirection_Parent, spAPParent.ReleaseAndGetAddressOf(), spUnkParentNativeNode.ReleaseAndGetAddressOf()));
            spAP = spAPParent;

            if (spUnkParentNativeNode)
            {
                ASSERT(spAP == nullptr);
            }
        }
    }

    // Step 4: Walk down the stack of AutomationPeers, checking each child
    // collection for the next AP. If it's not found then we need to bail out
    // early: returning the next AP would mean giving an AP that is a part of an
    // inconsistent tree to the UIA client.
    while (!targetStack.empty())
    {
        XINT32 cChildren = 0;
        bool found = false;
        xref_ptr<CAutomationPeer> spAP;
        xref_ptr<CAutomationPeer> spTopAP;

        IFC(targetStack.top(spAP));
        IFC(targetStack.pop());

        if (targetStack.empty())
        {
            // Termination condition 1: If we're the last AP on the stack
            // we have no children to validate. We're the hit-tested AP.
            spResultAP = spAP;
            break;
        }

        IFC(targetStack.top(spTopAP));

        cChildren = spAP->GetChildren(&pAPChildren);
        for (int i = 0; i < cChildren; i++)
        {
            if(pAPChildren[i]->GetRuntimeId() == spTopAP->GetRuntimeId())
            {
                found = true;
                break;
            }
        }
        delete[] pAPChildren;
        pAPChildren = nullptr;

        if (!found)
        {
            // Termination condition 2: pTopAP isn't part of a well-formed tree. pAP is
            // as deep into the stack as we can traverse without giving UIA clients a AP
            // that is part of an inconsistent tree.
            spResultAP = spAP;
            break;
        }
    }

    // Step 5: Finally handle the case of hosted automation providers. There is a possibility
    // that it could either be managing it's own focus in the subtree or it could be hosting
    // windowless control.
    if (spResultAP)
    {
        if (spResultAP->IsHostedWindowlessElementFromPointProvider())
        {
            xref_ptr<IRawElementProviderFragmentRoot> spRawElementProviderFragmentRoot;
            xref_ptr<IRawElementProviderSimple> spRawElementProviderSimple;
            // Is this a hosted automation provider, like windowless WebView, query it for ElementProviderFromPoint
            spRawElementProviderSimple = reinterpret_cast<IRawElementProviderSimple*>(spResultAP->GetWindowlessRawElementProviderSimple());
            IFCPTR(spRawElementProviderSimple);

            IFC(spRawElementProviderSimple->QueryInterface(
                __uuidof(IRawElementProviderFragmentRoot),
                reinterpret_cast<void**>(spRawElementProviderFragmentRoot.ReleaseAndGetAddressOf())));
            IFC(spRawElementProviderFragmentRoot->ElementProviderFromPoint(x, y, spFrag.ReleaseAndGetAddressOf()));
        }
        else
        {
            // If its not a hosting windowless control there's quite a possibility that this control might be managing
            // its subtree by itself that includes navigation ElementFromPoint and Focus. Give this element a chance
            // to provide the correct possible descendant.

            xref_ptr<IUnknown> spUnkElementFromPoint;
            xref_ptr<CAutomationPeer> spAP;

            // The descndant could be a native UIA Node as well, handling that case.
            IFC(spResultAP->GetElementFromPoint(&xptPhysical, spAP.ReleaseAndGetAddressOf(), spUnkElementFromPoint.ReleaseAndGetAddressOf()));

            if (spUnkElementFromPoint)
            {
                IFC((spUnkElementFromPoint.get())->QueryInterface(__uuidof(IRawElementProviderFragment), reinterpret_cast<void**>(spFrag.ReleaseAndGetAddressOf())));
            }
            else
            {
                if (spAP)
                {
                    xref_ptr<CUIAWrapper> spWrapper;
                    xref_ptr<CAutomationPeer> spAPEventsSource;

                    // If EventsSource exist use that instead.
                    spAPEventsSource = spAP->GetAPEventsSource();
                    if (spAPEventsSource)
                    {
                        spAP = spAPEventsSource;
                    }

                    IFC(CreateProviderForAP(spAP, spWrapper.ReleaseAndGetAddressOf()));
                    IFC(spWrapper->QueryInterface(__uuidof(IRawElementProviderFragment), reinterpret_cast<void**>(spFrag.ReleaseAndGetAddressOf())));
                }
            }
        }
    }

    *pRetVal = spFrag.detach();

Cleanup:
    if (shouldFallbackToSelf && (FAILED(hr) || *pRetVal == nullptr))
    {
        AddRef();
        *pRetVal = (IRawElementProviderFragment*)this;
    }
    delete[] pAPChildren;
    RRETURN(hr);
}

// Gets the Provider currently focused on
HRESULT STDMETHODCALLTYPE CUIAWindow::GetFocus(_Out_ IRawElementProviderFragment** pRetVal)
{
    if (!m_fInitialized)
    {
        IFC_NOTRACE_RETURN(static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
    }

    IFC_RETURN(GetFocusImpl(pRetVal));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CUIAWindow::GetFocusImpl(_Out_ IRawElementProviderFragment ** pRetVal)
{
    if (pRetVal == nullptr)
    {
        return E_INVALIDARG;
    }

    IXcpBrowserHost *pBH = nullptr;

    *pRetVal = nullptr;
    pBH = m_pHost->GetBrowserHost();

    if (pBH != nullptr && pBH->GetContextInterface())
    {
        xref_ptr<IRawElementProviderFragment> spFrag;
        xref_ptr<CAutomationPeer> spResultAP;

        xref_ptr<CDependencyObject> rootVisual;
        IFC_RETURN(GetRootVisual(rootVisual));

        if (rootVisual != nullptr)
        {
            CFocusManager* focusManager = VisualTree::GetFocusManagerForElement(rootVisual);
            if (focusManager != nullptr)
            {
                spResultAP = focusManager->GetFocusedAutomationPeer();
            }
        }

        if (spResultAP)
        {
            xref_ptr<CAutomationPeer> spAPEventsSource(spResultAP->GetAPEventsSource());

            if (spAPEventsSource)
            {
                spResultAP = std::move(spAPEventsSource);
            }

            // Once focused element is found, there is a possibility that it could either be managing it's own focus
            // in the subtree or it could be hosting windowless control.
            if (spResultAP->IsHostedWindowlessElementFromPointProvider())
            {
                // If it's a hosting windowless control, get the fragment root and
                // ask the root to provide the focused element.
                xref_ptr<IRawElementProviderFragmentRoot> spRawElementProviderFragmentRoot;
                xref_ptr<IRawElementProviderSimple> spRawElementProviderSimple;

                // Is this a hosted automation provider, like windowless WebView, query it for ElementProviderFromPoint
                spRawElementProviderSimple = reinterpret_cast<IRawElementProviderSimple*>(spResultAP->GetWindowlessRawElementProviderSimple());
                IFCPTR_RETURN(spRawElementProviderSimple);

                IFC_RETURN(spRawElementProviderSimple->QueryInterface(
                    __uuidof(IRawElementProviderFragmentRoot),
                    reinterpret_cast<void**>(spRawElementProviderFragmentRoot.ReleaseAndGetAddressOf())));
                IFC_RETURN(spRawElementProviderFragmentRoot->GetFocus(spFrag.ReleaseAndGetAddressOf()));
            }
            else
            {
                // If its not a hosting windowless control there's quite a possibility that this control might be managing
                // its subtree by itself that includes navigation ElementFromPoint and Focus. Give this element a chance
                // to provide the correct possible descendant.

                xref_ptr<IUnknown> spUnkFocusedElement;
                xref_ptr<CAutomationPeer> spAP;

                // The descndant could be a native UIA Node as well, handling that case.
                IFC_RETURN(spResultAP->GetFocusedElement(spAP.ReleaseAndGetAddressOf(), spUnkFocusedElement.ReleaseAndGetAddressOf()));

                if (spUnkFocusedElement)
                {
                    IFC_RETURN((spUnkFocusedElement.get())->QueryInterface(__uuidof(IRawElementProviderFragment), reinterpret_cast<void**>(spFrag.ReleaseAndGetAddressOf())));
                }
                else
                {
                    if (spAP)
                    {
                        // If EventsSource exist use that instead.

                        spAPEventsSource = spAP->GetAPEventsSource();

                        if (spAPEventsSource)
                        {
                            spAP = std::move(spAPEventsSource);
                        }

                        xref_ptr<CUIAWrapper> spWrapper;
                        IFC_RETURN(CreateProviderForAP(spAP, spWrapper.ReleaseAndGetAddressOf()));
                        IFC_RETURN((spWrapper.get())->QueryInterface(__uuidof(IRawElementProviderFragment), reinterpret_cast<void**>(spFrag.ReleaseAndGetAddressOf())));
                    }
                }
            }
        }

        *pRetVal = spFrag.detach();
    }

    return S_OK;
}

// IRawElementProviderAdviseEvents implementation
// Notifies the UI Automation provider when a UI Automation client begins listening for a specific event, including a property-changed event
HRESULT STDMETHODCALLTYPE CUIAWindow::AdviseEventAdded(_In_ EVENTID eventId, _Out_ SAFEARRAY *propertyIDs)
{
    if (!m_fInitialized)
    {
        IFC_NOTRACE_RETURN(static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
    }

    IFCPTR_RETURN(m_pUIAIds);

    if ( eventId == m_pUIAIds->AsyncContentLoaded_Event)
    {
        m_cAdviseEventAsyncContentLoaded++;
    }
    else if ( eventId == m_pUIAIds->AutomationFocusChanged_Event)
    {
        m_cAdviseEventAutomationFocusChanged++;
    }
    else if (eventId == m_pUIAIds->AutomationPropertyChanged_Event)
    {
        m_cAdviseEventPropertyChanged++;
    }
    else if (eventId == m_pUIAIds->Invoke_Invoked_Event)
    {
        m_cAdviseEventInvokePatternOnInvoked++;
    }
    else if (eventId == m_pUIAIds->MenuClosed_Event)
    {
        m_cAdviseEventMenuClosed++;
    }
    else if (eventId == m_pUIAIds->MenuOpened_Event)
    {
        m_cAdviseEventMenuOpened++;
    }
    else if (eventId == m_pUIAIds->Selection_InvalidatedEvent_Event)
    {
        m_cAdviseEventSelectionPatternOnInvalidated++;
    }
    else if (eventId == m_pUIAIds->SelectionItem_ElementAddedToSelectionEvent_Event)
    {
        m_cAdviseEventSelectionItemPatternOnElementAddedToSelection++;
    }
    else if (eventId == m_pUIAIds->SelectionItem_ElementRemovedFromSelectionEvent_Event)
    {
        m_cAdviseEventSelectionItemPatternOnElementRemovedFromSelection++;
    }
    else if (eventId == m_pUIAIds->SelectionItem_ElementSelectedEvent_Event)
    {
        m_cAdviseEventSelectionItemPatternOnElementSelected++;
    }
    else if (eventId == m_pUIAIds->StructureChanged_Event)
    {
        m_cAdviseEventStructureChanged++;
    }
    else if (eventId == m_pUIAIds->Text_TextChangedEvent_Event)
    {
        m_cAdviseEventTextPatternOnTextChanged++;
    }
    else if (eventId == m_pUIAIds->Text_TextSelectionChangedEvent_Event)
    {
        m_cAdviseEventTextPatternOnTextSelectionChanged++;
    }
    else if (eventId == m_pUIAIds->ToolTipClosed_Event)
    {
        m_cAdviseEventToolTipClosed++;
    }
    else if (eventId == m_pUIAIds->ToolTipOpened_Event)
    {
        m_cAdviseEventToolTipOpened++;
    }
    else if (eventId == m_pUIAIds->Drag_DragStart_Event)
    {
        m_cAdviseEventDragStart++;
    }
    else if (eventId == m_pUIAIds->Drag_DragCancel_Event)
    {
        m_cAdviseEventDragCancel++;
    }
    else if (eventId == m_pUIAIds->Drag_DragComplete_Event)
    {
        m_cAdviseEventDragComplete++;
    }
    else if (eventId == m_pUIAIds->DropTarget_DragEnter_Event)
    {
        m_cAdviseEventDragEnter++;
    }
    else if (eventId == m_pUIAIds->DropTarget_DragLeave_Event)
    {
        m_cAdviseEventDragLeave++;
    }
    else if (eventId == m_pUIAIds->DropTarget_Dropped_Event)
    {
        m_cAdviseEventDropped++;
    }
    else if (eventId == m_pUIAIds->LiveRegionChanged_Event)
    {
        m_cAdviseEventLiveRegionChanged++;
    }
    else if (eventId == m_pUIAIds->InputReachedTarget_Event)
    {
        m_cAdviseEventInputReachedTarget++;
    }
    else if (eventId == m_pUIAIds->InputReachedOtherElement_Event)
    {
        m_cAdviseEventInputReachedOtherElement++;
    }
    else if (eventId == m_pUIAIds->InputDiscarded_Event)
    {
        m_cAdviseEventInputDiscarded++;
    }
    else if (eventId == m_pUIAIds->WindowClosed_Event)
    {
        m_cAdviseEventWindowClosed++;
    }
    else if (eventId == m_pUIAIds->WindowOpened_Event)
    {
        m_cAdviseEventWindowOpened++;
    }
    else if (eventId == m_pUIAIds->TextEdit_TextChanged_Event)
    {
        m_cAdviseEventTextEditTextChanged++;
    }
    else if (eventId == m_pUIAIds->TextEdit_ConversionTargetChanged_Event)
    {
        m_cAdviseEventConversionTargetChanged++;
    }
    else if (eventId == m_pUIAIds->LayoutInvalidated_Event)
    {
        m_cLayoutInvalidated++;
    }
    else if (eventId == m_pUIAIds->Notification_Event)
    {
        m_cAdviseNotificationEvent++;
    }

    return S_OK;
}
// Notifies the UI Automation provider when a UI Automation client stops listening for a specific event, including a property-changed event
HRESULT STDMETHODCALLTYPE CUIAWindow::AdviseEventRemoved(_In_ EVENTID eventId, _Out_ SAFEARRAY *propertyIDs)
{
    if (!m_fInitialized)
    {
        IFC_NOTRACE_RETURN(static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
    }

    IFCPTR_RETURN(m_pUIAIds);
    if ( eventId == m_pUIAIds->AsyncContentLoaded_Event && m_cAdviseEventAsyncContentLoaded > 0)
    {
        m_cAdviseEventAsyncContentLoaded--;
    }
    else if ( eventId == m_pUIAIds->AutomationFocusChanged_Event && m_cAdviseEventAutomationFocusChanged > 0)
    {
        m_cAdviseEventAutomationFocusChanged--;
    }
    else if (eventId == m_pUIAIds->AutomationPropertyChanged_Event && m_cAdviseEventPropertyChanged > 0)
    {
        m_cAdviseEventPropertyChanged--;
    }
    else if (eventId == m_pUIAIds->Invoke_Invoked_Event && m_cAdviseEventInvokePatternOnInvoked > 0)
    {
        m_cAdviseEventInvokePatternOnInvoked--;
    }
    else if (eventId == m_pUIAIds->MenuClosed_Event && m_cAdviseEventMenuClosed > 0)
    {
        m_cAdviseEventMenuClosed--;
    }
    else if (eventId == m_pUIAIds->MenuOpened_Event && m_cAdviseEventMenuOpened > 0)
    {
        m_cAdviseEventMenuOpened--;
    }
    else if (eventId == m_pUIAIds->Selection_InvalidatedEvent_Event &&
             m_cAdviseEventSelectionPatternOnInvalidated > 0)
    {
        m_cAdviseEventSelectionPatternOnInvalidated--;
    }
    else if (eventId == m_pUIAIds->SelectionItem_ElementAddedToSelectionEvent_Event &&
             m_cAdviseEventSelectionItemPatternOnElementAddedToSelection > 0)
    {
        m_cAdviseEventSelectionItemPatternOnElementAddedToSelection--;
    }
    else if (eventId == m_pUIAIds->SelectionItem_ElementRemovedFromSelectionEvent_Event &&
             m_cAdviseEventSelectionItemPatternOnElementRemovedFromSelection > 0)
    {
        m_cAdviseEventSelectionItemPatternOnElementRemovedFromSelection--;
    }
    else if (eventId == m_pUIAIds->SelectionItem_ElementSelectedEvent_Event &&
             m_cAdviseEventSelectionItemPatternOnElementSelected > 0)
    {
        m_cAdviseEventSelectionItemPatternOnElementSelected--;
    }
    else if (eventId == m_pUIAIds->StructureChanged_Event && m_cAdviseEventStructureChanged > 0)
    {
        m_cAdviseEventStructureChanged--;
    }
    else if (eventId == m_pUIAIds->Text_TextChangedEvent_Event && m_cAdviseEventTextPatternOnTextChanged > 0)
    {
        m_cAdviseEventTextPatternOnTextChanged--;
    }
    else if (eventId == m_pUIAIds->Text_TextSelectionChangedEvent_Event &&
             m_cAdviseEventTextPatternOnTextSelectionChanged > 0)
    {
        m_cAdviseEventTextPatternOnTextSelectionChanged--;
    }
    else if (eventId == m_pUIAIds->ToolTipClosed_Event && m_cAdviseEventToolTipClosed > 0)
    {
        m_cAdviseEventToolTipClosed--;
    }
    else if (eventId == m_pUIAIds->ToolTipOpened_Event && m_cAdviseEventToolTipOpened > 0)
    {
        m_cAdviseEventToolTipOpened--;
    }
    else if (eventId == m_pUIAIds->Drag_DragStart_Event && m_cAdviseEventDragStart > 0)
    {
        m_cAdviseEventDragStart--;
    }
    else if (eventId == m_pUIAIds->Drag_DragCancel_Event && m_cAdviseEventDragCancel > 0)
    {
        m_cAdviseEventDragCancel--;
    }
    else if (eventId == m_pUIAIds->Drag_DragComplete_Event && m_cAdviseEventDragComplete > 0)
    {
        m_cAdviseEventDragComplete--;
    }
    else if (eventId == m_pUIAIds->DropTarget_DragEnter_Event && m_cAdviseEventDragEnter > 0)
    {
        m_cAdviseEventDragEnter--;
    }
    else if (eventId == m_pUIAIds->DropTarget_DragLeave_Event && m_cAdviseEventDragLeave > 0)
    {
        m_cAdviseEventDragLeave--;
    }
    else if (eventId == m_pUIAIds->DropTarget_Dropped_Event && m_cAdviseEventDropped > 0)
    {
        m_cAdviseEventDropped--;
    }
    else if (eventId == m_pUIAIds->LiveRegionChanged_Event && m_cAdviseEventLiveRegionChanged > 0)
    {
        m_cAdviseEventLiveRegionChanged--;
    }
    else if (eventId == m_pUIAIds->InputReachedTarget_Event && m_cAdviseEventInputReachedTarget > 0)
    {
        m_cAdviseEventInputReachedTarget--;
    }
    else if (eventId == m_pUIAIds->InputReachedOtherElement_Event && m_cAdviseEventInputReachedOtherElement > 0)
    {
        m_cAdviseEventInputReachedOtherElement--;
    }
    else if (eventId == m_pUIAIds->InputDiscarded_Event && m_cAdviseEventInputDiscarded > 0)
    {
        m_cAdviseEventInputDiscarded--;
    }
    else if (eventId == m_pUIAIds->WindowClosed_Event && m_cAdviseEventWindowClosed > 0)
    {
        m_cAdviseEventWindowClosed--;
    }
    else if (eventId == m_pUIAIds->WindowOpened_Event && m_cAdviseEventWindowOpened > 0)
    {
        m_cAdviseEventWindowOpened--;
    }
    else if (eventId == m_pUIAIds->TextEdit_TextChanged_Event && m_cAdviseEventTextEditTextChanged > 0)
    {
        m_cAdviseEventTextEditTextChanged--;
    }
    else if (eventId == m_pUIAIds->TextEdit_ConversionTargetChanged_Event && m_cAdviseEventConversionTargetChanged > 0)
    {
        m_cAdviseEventConversionTargetChanged--;
    }
    else if (eventId == m_pUIAIds->LayoutInvalidated_Event && m_cLayoutInvalidated > 0)
    {
        m_cLayoutInvalidated--;
    }
    else if (eventId == m_pUIAIds->Notification_Event && m_cAdviseNotificationEvent > 0)
    {
        m_cAdviseNotificationEvent--;
    }

    return S_OK;
}

HRESULT CUIAWindow::ReleaseInterfaceOffThread(_In_ IObject *pObject)
{
    if (!m_fInitialized)
    {
        IFC_RETURN(E_FAIL);
    }

    ReleaseInterface(pObject);

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CUIAWindow::UIAClientsAreListening
//
//  Synopsis:
//      Checks if clients are listening to a specific event
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CUIAWindow::UIAClientsAreListening(_In_ UIAXcp::APAutomationEvents eAutomationEvent) const
{
    if (m_isEnabledMockUIAClientsListening)
    {
        return S_OK;
    }

    XUINT32 bListening = FALSE;

    if (UiaClientsAreListening())
    {
        switch(eAutomationEvent)
        {
        case UIAXcp::AEToolTipOpened:
            bListening = m_cAdviseEventToolTipOpened;
            break;
        case UIAXcp::AEToolTipClosed:
            bListening = m_cAdviseEventToolTipClosed;
            break;
        case UIAXcp::AEMenuOpened:
            bListening = m_cAdviseEventMenuOpened;
            break;
        case UIAXcp::AEMenuClosed:
            bListening = m_cAdviseEventMenuClosed;
            break;
        case UIAXcp::AEAutomationFocusChanged:
            bListening = m_cAdviseEventAutomationFocusChanged;
            break;
        case UIAXcp::AEInvokePatternOnInvoked:
            bListening = m_cAdviseEventInvokePatternOnInvoked;
            break;
        case UIAXcp::AESelectionItemPatternOnElementAddedToSelection:
            bListening = m_cAdviseEventSelectionItemPatternOnElementAddedToSelection;
            break;
        case UIAXcp::AESelectionItemPatternOnElementRemovedFromSelection:
            bListening = m_cAdviseEventSelectionItemPatternOnElementRemovedFromSelection;
            break;
        case UIAXcp::AESelectionItemPatternOnElementSelected:
            bListening = m_cAdviseEventSelectionItemPatternOnElementSelected;
            break;
        case UIAXcp::AESelectionPatternOnInvalidated:
            bListening = m_cAdviseEventSelectionPatternOnInvalidated;
            break;
        case UIAXcp::AETextPatternOnTextSelectionChanged:
            bListening = m_cAdviseEventTextPatternOnTextSelectionChanged;
            break;
        case UIAXcp::AETextPatternOnTextChanged:
            bListening = m_cAdviseEventTextPatternOnTextChanged;
            break;
        case UIAXcp::AEAsyncContentLoaded:
            bListening = m_cAdviseEventAsyncContentLoaded;
            break;
        case UIAXcp::AEPropertyChanged:
            bListening = m_cAdviseEventPropertyChanged;
            break;
        case UIAXcp::AEStructureChanged:
            bListening = m_cAdviseEventStructureChanged;
            break;
        case UIAXcp::AEDragStart:
            bListening = m_cAdviseEventDragStart;
            break;
        case UIAXcp::AEDragCancel:
            bListening = m_cAdviseEventDragCancel;
            break;
        case UIAXcp::AEDragComplete:
            bListening = m_cAdviseEventDragComplete;
            break;
        case UIAXcp::AEDragEnter:
            bListening = m_cAdviseEventDragEnter;
            break;
        case UIAXcp::AEDragLeave:
            bListening = m_cAdviseEventDragLeave;
            break;
        case UIAXcp::AEDropped:
            bListening = m_cAdviseEventDropped;
            break;
        case UIAXcp::AELiveRegionChanged:
            bListening = m_cAdviseEventLiveRegionChanged;
            break;
        case UIAXcp::AEInputReachedTarget:
            bListening = m_cAdviseEventInputReachedTarget;
            break;
        case UIAXcp::AEInputReachedOtherElement:
            bListening = m_cAdviseEventInputReachedOtherElement;
            break;
        case UIAXcp::AEInputDiscarded:
            bListening = m_cAdviseEventInputDiscarded;
            break;
        case UIAXcp::AEWindowClosed:
            bListening = m_cAdviseEventWindowClosed;
            break;
        case UIAXcp::AEWindowOpened:
            bListening = m_cAdviseEventWindowOpened;
            break;
        case UIAXcp::AETextEditTextChanged:
            bListening = m_cAdviseEventTextEditTextChanged;
            break;
        case UIAXcp::AEConversionTargetChanged:
            bListening = m_cAdviseEventConversionTargetChanged;
            break;
        case UIAXcp::AELayoutInvalidated:
            bListening = m_cLayoutInvalidated;
            break;
        case UIAXcp::AENotification:
            bListening = m_cAdviseNotificationEvent;
            break;

        }
        if (bListening)
        {
            return S_OK;
        }
    }
    return S_FALSE;
}
//-------------------------------------------------------------------------
//
//  Function:   CUIAWindow::UIARaiseAutomationEvent
//
//  Synopsis:
//      Raises a UIA Event
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CUIAWindow::UIARaiseAutomationEvent(
    _In_ CAutomationPeer *pAP,
    _In_ UIAXcp::APAutomationEvents eAutomationEvent)
{
    xref_ptr<CUIAWrapper> wrapper;
    xref_ptr<IRawElementProviderSimple> provider;

    if (S_FALSE == UIAClientsAreListening(eAutomationEvent))
    {
        return S_OK;
    }

    if (pAP)
    {
        IFC_RETURN(CreateProviderForAP(pAP, wrapper.ReleaseAndGetAddressOf()));
        IFC_RETURN(wrapper->QueryInterface(__uuidof(IRawElementProviderSimple), reinterpret_cast<void**>(provider.ReleaseAndGetAddressOf())));
    }
    else
    {
        IFC_RETURN(E_FAIL);
    }
    IFC_RETURN(UiaRaiseAutomationEvent(provider, ConvertEnumToId(eAutomationEvent)));

    return S_OK;
}
//-------------------------------------------------------------------------
//
//  Function:   CUIAWindow::UIARaiseAutomationPropertyChangedEvent
//
//  Synopsis:
//      Raises a UIA Property Changed Event
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CUIAWindow::UIARaiseAutomationPropertyChangedEvent(
    _In_ CAutomationPeer *pAP,
   _In_ UIAXcp::APAutomationProperties eAutomationProperty,
   _In_ const CValue& oldValue,
   _In_ const CValue& newValue)
{
    HRESULT hr = S_OK;
    VARIANT oldVariant;
    VARIANT newVariant;
    VariantInit(&oldVariant);
    VariantInit(&newVariant);
    xref_ptr<CUIAWrapper> wrapper;
    CAutomationPeer *pAPEventsSource = nullptr;
    xref_ptr<IRawElementProviderSimple> provider;

    pAPEventsSource = pAP->GetAPEventsSource();
    if (pAPEventsSource)
    {
        pAP = pAPEventsSource;
    }

    if (pAP)
    {
        IFC(CreateProviderForAP(pAP, wrapper.ReleaseAndGetAddressOf()));
        IFC(wrapper->QueryInterface(__uuidof(IRawElementProviderSimple), reinterpret_cast<void**>(provider.ReleaseAndGetAddressOf())));
    }
    else
    {
        IFC(E_FAIL);
    }
    switch (eAutomationProperty)
    {
        case UIAXcp::APStructureChangeType_ChildrenBulkRemovedProperty:
            ASSERT(newValue.GetType() == valueNull || newValue.GetType() == valueSignedArray);
            IFC(UiaRaiseStructureChangedEvent(provider, StructureChangeType_ChildrenBulkRemoved, newValue.AsSignedArray(), newValue.GetArrayElementCount()));
            break;
        case UIAXcp::APStructureChangeType_ChildrenBulkAddedProperty:
            ASSERT(newValue.GetType() == valueNull || newValue.GetType() == valueSignedArray);
            IFC(UiaRaiseStructureChangedEvent(provider, StructureChangeType_ChildrenBulkAdded, newValue.AsSignedArray(), newValue.GetArrayElementCount()));
            break;
        case UIAXcp::APStructureChangeType_ChildrenInvalidatedProperty:
            ASSERT(newValue.GetType() == valueNull || newValue.GetType() == valueSignedArray);
            IFC(UiaRaiseStructureChangedEvent(provider, StructureChangeType_ChildrenInvalidated, newValue.AsSignedArray(), newValue.GetArrayElementCount()));
            break;
        case UIAXcp::APStructureChangeType_ChildAddedProperty:
            ASSERT(newValue.GetType() == valueNull || newValue.GetType() == valueSignedArray);
            IFC(UiaRaiseStructureChangedEvent(provider, StructureChangeType_ChildAdded, newValue.AsSignedArray(), newValue.GetArrayElementCount()));
            break;
        case UIAXcp::APStructureChangeType_ChildRemovedProperty:
            ASSERT(newValue.GetType() == valueNull || newValue.GetType() == valueSignedArray);
            IFC(UiaRaiseStructureChangedEvent(provider, StructureChangeType_ChildRemoved, newValue.AsSignedArray(), newValue.GetArrayElementCount()));
            break;
        case UIAXcp::APStructureChangeType_ChildernReorderedProperty:
            ASSERT(newValue.GetType() == valueNull || newValue.GetType() == valueSignedArray);
            IFC(UiaRaiseStructureChangedEvent(provider, StructureChangeType_ChildrenReordered, newValue.AsSignedArray(), newValue.GetArrayElementCount()));
            break;
        case UIAXcp::APAutomationProperties::APAnnotationsProperty:
        {
            // This is a special case, the XAML Annotations property maps to two UIA properties:
            // AnnotationTypes and AnnotationObjects
            CAutomationPeerAnnotationCollection* pOldAnnotations = static_cast<CAutomationPeerAnnotationCollection*>(oldValue.AsObject());
            CAutomationPeerAnnotationCollection* pNewAnnotations = static_cast<CAutomationPeerAnnotationCollection*>(newValue.AsObject());

            if (pOldAnnotations)
            {
                IFC(AutomationPeerAnnotationCollectionToVariant(pOldAnnotations, m_pUIAIds->AnnotationTypes_Property, &oldVariant));
            }
            if (pNewAnnotations)
            {
                IFC(AutomationPeerAnnotationCollectionToVariant(pOldAnnotations, m_pUIAIds->AnnotationTypes_Property, &newVariant));
            }
            IFC(UiaRaiseAutomationPropertyChangedEvent(provider, m_pUIAIds->AnnotationTypes_Property, oldVariant, newVariant));

            VariantClear(&oldVariant);
            VariantClear(&newVariant);
            if (pOldAnnotations)
            {
                IFC(AutomationPeerAnnotationCollectionToVariant(pOldAnnotations, m_pUIAIds->AnnotationObjects_Property, &oldVariant));
            }
            if (pNewAnnotations)
            {
                IFC(AutomationPeerAnnotationCollectionToVariant(pOldAnnotations, m_pUIAIds->AnnotationObjects_Property, &oldVariant));
            }
            IFC(UiaRaiseAutomationPropertyChangedEvent(provider, m_pUIAIds->AnnotationObjects_Property, oldVariant, newVariant));

            break;
        }
        default:
            if (!oldValue.IsUnset())
            {
                IFC(CValueToVariant(oldValue, &oldVariant));
            }
            if (!newValue.IsUnset())
            {
                IFC(CValueToVariant(newValue, &newVariant));
            }
            IFC(UiaRaiseAutomationPropertyChangedEvent(provider, ConvertEnumToId(eAutomationProperty), oldVariant, newVariant));
            break;
    }


Cleanup:
    VariantClear(&oldVariant);
    VariantClear(&newVariant);
    return hr;
}
//-------------------------------------------------------------------------
//
//  Function:   CUIAWindow::UIARaiseFocusChangedEventOnUIAWindow
//
//  Synopsis:
//      Raises a UIA Focus Changed Event
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CUIAWindow::UIARaiseFocusChangedEventOnUIAWindow()
{
    xref_ptr<IRawElementProviderSimple> provider;

    IFC_RETURN(this->QueryInterface(__uuidof(IRawElementProviderSimple), reinterpret_cast<void**>(provider.ReleaseAndGetAddressOf())));
    IFC_RETURN(UiaRaiseAutomationEvent(provider, m_pUIAIds->AutomationFocusChanged_Event));

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CUIAWindow::UiaRaiseTextEditTextChangedEvent
//
//  Synopsis:
//      Raises a UIA Text Edit Event
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CUIAWindow::UIARaiseTextEditTextChangedEvent(
    _In_ CAutomationPeer *pAP,
    _In_ UIAXcp::AutomationTextEditChangeType eAutomationProperty,
    _In_ CValue *pChangedData
    )
{
    xref_ptr<CUIAWrapper> spWrapper;
    xref_ptr<CAutomationPeer> spAPEventsSource;
    xref_ptr<IRawElementProviderSimple> spProvider;
    UINT size = 0;

    spAPEventsSource = pAP->GetAPEventsSource();
    if (spAPEventsSource)
    {
        pAP = spAPEventsSource;
    }

    if (pAP)
    {
        IFC_RETURN(CreateProviderForAP(pAP, spWrapper.ReleaseAndGetAddressOf()));
        IFC_RETURN(spWrapper->QueryInterface(__uuidof(IRawElementProviderSimple), reinterpret_cast<void**>(spProvider.ReleaseAndGetAddressOf())));
    }
    else
    {
        IFC_RETURN(E_FAIL);
    }

    xref_ptr<wfc::IVectorView<HSTRING>> spChangedData;
    xref_ptr<IInspectable> spChangedDataAsInspectable(pChangedData->AsIInspectable());
    IFCPTR_RETURN(spChangedDataAsInspectable);
    IFC_RETURN(spChangedDataAsInspectable->QueryInterface(__uuidof(wfc::IVectorView<HSTRING>), reinterpret_cast<void**> (spChangedData.ReleaseAndGetAddressOf())));
    IFCPTR_RETURN(spChangedData);
    IFC_RETURN(spChangedData->get_Size(&size));

    unique_safearray safeArray(SafeArrayCreateVector(VT_BSTR, 0, size));
    IFCOOMFAILFAST(safeArray.get());

    for (LONG i = 0; i < (LONG)size; ++i)
    {
        HSTRING str;
        IFC_RETURN(spChangedData->GetAt(i, &str));
        BSTR bstr = SysAllocString(WindowsGetStringRawBuffer(str, nullptr));
        IFC_RETURN(SafeArrayPutElement(safeArray.get(), &i, static_cast<void*>(bstr)));
    }

    switch (eAutomationProperty)
    {
    case UIAXcp::AutomationTextEditChangeType::AutomationTextEditChangeType_None:
        break;
    case UIAXcp::AutomationTextEditChangeType::AutomationTextEditChangeType_AutoCorrect:
    case UIAXcp::AutomationTextEditChangeType::AutomationTextEditChangeType_Composition:
    case UIAXcp::AutomationTextEditChangeType::AutomationTextEditChangeType_CompositionFinalized:
        IFC_RETURN(UiaRaiseTextEditTextChangedEvent(spProvider.get(), static_cast<TextEditChangeType>(eAutomationProperty), safeArray.get()));
        break;
    default:
        IFC_RETURN(E_INVALIDARG);
        break;
    }
    return S_OK;
}

_Check_return_ HRESULT CUIAWindow::UIARaiseNotificationEvent(
    _In_ CAutomationPeer* ap,
    UIAXcp::AutomationNotificationKind notificationKind,
    UIAXcp::AutomationNotificationProcessing notificationProcessing,
    const xstring_ptr& displayString,
    const xstring_ptr& activityId)
{
    CAutomationPeer* const eventSource = ap->GetAPEventsSource();
    if (eventSource)
    {
        ap = eventSource;
    }

    wrl::ComPtr<IRawElementProviderSimple> provider;
    if (ap)
    {
        wrl::ComPtr<CUIAWrapper> wrapper;
        IFC_RETURN(CreateProviderForAP(ap, &wrapper));
        IFC_RETURN(wrapper.As(&provider));
    }
    else
    {
        IFC_RETURN(E_FAIL);
    }

    wil::unique_bstr bstrDisplayString;
    if (!displayString.IsNullOrEmpty())
    {
        bstrDisplayString.reset(SysAllocStringLen(displayString.GetBuffer(), displayString.GetCount()));
        IFCOOM_RETURN(bstrDisplayString);
    }
    wil::unique_bstr bstrActivityId(SysAllocStringLen(activityId.GetBuffer(), activityId.GetCount()));
    IFCOOM_RETURN(bstrActivityId);

    IFC_RETURN(UiaRaiseNotificationEvent(
        provider.Get(),
        static_cast<NotificationKind>(notificationKind),
        static_cast<NotificationProcessing>(notificationProcessing),
        bstrDisplayString.get(),
        bstrActivityId.get()));

    return S_OK;
}

EVENTID CUIAWindow::ConvertEnumToId(_In_ UIAXcp::APAutomationEvents eEvent)
{
    switch (eEvent)
    {
        case UIAXcp::AEToolTipOpened:
            return m_pUIAIds->ToolTipOpened_Event;
        case UIAXcp::AEToolTipClosed:
            return m_pUIAIds->ToolTipClosed_Event;
        case UIAXcp::AEMenuOpened:
            return m_pUIAIds->MenuOpened_Event;
        case UIAXcp::AEMenuClosed:
            return m_pUIAIds->MenuClosed_Event;
        case UIAXcp::AEAutomationFocusChanged:
            return m_pUIAIds->AutomationFocusChanged_Event;
        case UIAXcp::AEInvokePatternOnInvoked:
            return m_pUIAIds->Invoke_Invoked_Event;
        case UIAXcp::AESelectionItemPatternOnElementAddedToSelection:
            return m_pUIAIds->SelectionItem_ElementAddedToSelectionEvent_Event;
        case UIAXcp::AESelectionItemPatternOnElementRemovedFromSelection:
            return m_pUIAIds->SelectionItem_ElementRemovedFromSelectionEvent_Event;
        case UIAXcp::AESelectionItemPatternOnElementSelected:
            return m_pUIAIds->SelectionItem_ElementSelectedEvent_Event;
        case UIAXcp::AESelectionPatternOnInvalidated:
            return m_pUIAIds->Selection_InvalidatedEvent_Event;
        case UIAXcp::AETextPatternOnTextSelectionChanged:
            return m_pUIAIds->Text_TextSelectionChangedEvent_Event;
        case UIAXcp::AETextPatternOnTextChanged:
            return m_pUIAIds->Text_TextChangedEvent_Event;
        case UIAXcp::AEAsyncContentLoaded:
            return m_pUIAIds->AsyncContentLoaded_Event;
        case UIAXcp::AEPropertyChanged:
            return m_pUIAIds->AutomationPropertyChanged_Event;
        case UIAXcp::AEStructureChanged:
            return m_pUIAIds->StructureChanged_Event;
        case UIAXcp::AEDragStart:
            return m_pUIAIds->Drag_DragStart_Event;
        case UIAXcp::AEDragCancel:
            return m_pUIAIds->Drag_DragCancel_Event;
        case UIAXcp::AEDragComplete:
            return m_pUIAIds->Drag_DragComplete_Event;
        case UIAXcp::AEDragEnter:
            return m_pUIAIds->DropTarget_DragEnter_Event;
        case UIAXcp::AEDragLeave:
            return m_pUIAIds->DropTarget_DragLeave_Event;
        case UIAXcp::AEDropped:
            return m_pUIAIds->DropTarget_Dropped_Event;
        case UIAXcp::AELiveRegionChanged:
            return m_pUIAIds->LiveRegionChanged_Event;
        case UIAXcp::AEInputReachedTarget:
            return m_pUIAIds->InputReachedTarget_Event;
        case UIAXcp::AEInputReachedOtherElement:
            return m_pUIAIds->InputReachedOtherElement_Event;
        case UIAXcp::AEInputDiscarded:
            return m_pUIAIds->InputDiscarded_Event;
        case UIAXcp::AEWindowClosed:
            return m_pUIAIds->WindowClosed_Event;
        case UIAXcp::AEWindowOpened:
            return m_pUIAIds->WindowOpened_Event;
        case UIAXcp::AETextEditTextChanged:
            return m_pUIAIds->TextEdit_TextChanged_Event;
        case UIAXcp::AEConversionTargetChanged:
            return m_pUIAIds->TextEdit_ConversionTargetChanged_Event;
        case UIAXcp::AELayoutInvalidated:
            return m_pUIAIds->LayoutInvalidated_Event;
    };

    return (EVENTID)0;
}
CONTROLTYPEID CUIAWindow::ConvertEnumToId(_In_ UIAXcp::APAutomationControlType eControlType)
{
    switch (eControlType)
        {
        case UIAXcp::ACTButton:
            return m_pUIAIds->Button_ControlType;
        case UIAXcp::ACTCalendar:
            return m_pUIAIds->Calendar_ControlType;
        case UIAXcp::ACTCheckBox:
            return m_pUIAIds->CheckBox_ControlType;
        case UIAXcp::ACTComboBox:
            return m_pUIAIds->ComboBox_ControlType;
        case UIAXcp::ACTEdit:
            return m_pUIAIds->Edit_ControlType;
        case UIAXcp::ACTHyperlink:
            return m_pUIAIds->Hyperlink_ControlType;
        case UIAXcp::ACTImage:
            return m_pUIAIds->Image_ControlType;
        case UIAXcp::ACTListItem:
            return m_pUIAIds->ListItem_ControlType;
        case UIAXcp::ACTList:
            return m_pUIAIds->List_ControlType;
        case UIAXcp::ACTMenu:
            return m_pUIAIds->Menu_ControlType;
        case UIAXcp::ACTMenuBar:
            return m_pUIAIds->MenuBar_ControlType;
        case UIAXcp::ACTMenuItem:
            return m_pUIAIds->MenuItem_ControlType;
        case UIAXcp::ACTProgressBar:
            return m_pUIAIds->ProgressBar_ControlType;
        case UIAXcp::ACTRadioButton:
            return m_pUIAIds->RadioButton_ControlType;
        case UIAXcp::ACTScrollBar:
            return m_pUIAIds->ScrollBar_ControlType;
        case UIAXcp::ACTSlider:
            return m_pUIAIds->Slider_ControlType;
        case UIAXcp::ACTSpinner:
            return m_pUIAIds->Spinner_ControlType;
        case UIAXcp::ACTStatusBar:
            return m_pUIAIds->StatusBar_ControlType;
        case UIAXcp::ACTTab:
            return m_pUIAIds->Tab_ControlType;
        case UIAXcp::ACTTabItem:
            return m_pUIAIds->TabItem_ControlType;
        case UIAXcp::ACTText:
            return m_pUIAIds->Text_ControlType;
        case UIAXcp::ACTToolBar:
            return m_pUIAIds->ToolBar_ControlType;
        case UIAXcp::ACTToolTip:
            return m_pUIAIds->ToolTip_ControlType;
        case UIAXcp::ACTTree:
            return m_pUIAIds->Tree_ControlType;
        case UIAXcp::ACTTreeItem:
            return m_pUIAIds->TreeItem_ControlType;
        case UIAXcp::ACTCustom:
            return m_pUIAIds->Custom_ControlType;
        case UIAXcp::ACTGroup:
            return m_pUIAIds->Group_ControlType;
        case UIAXcp::ACTThumb:
            return m_pUIAIds->Thumb_ControlType;
        case UIAXcp::ACTDataGrid:
            return m_pUIAIds->DataGrid_ControlType;
        case UIAXcp::ACTDataItem:
            return m_pUIAIds->DataItem_ControlType;
        case UIAXcp::ACTDocument:
            return m_pUIAIds->Document_ControlType;
        case UIAXcp::ACTSplitButton:
            return m_pUIAIds->SplitButton_ControlType;
        case UIAXcp::ACTWindow:
            return m_pUIAIds->Window_ControlType;
        case UIAXcp::ACTPane:
            return m_pUIAIds->Pane_ControlType;
        case UIAXcp::ACTHeader:
            return m_pUIAIds->Header_ControlType;
        case UIAXcp::ACTHeaderItem:
            return m_pUIAIds->HeaderItem_ControlType;
        case UIAXcp::ACTTable:
            return m_pUIAIds->Table_ControlType;
        case UIAXcp::ACTTitleBar:
            return m_pUIAIds->TitleBar_ControlType;
        case UIAXcp::ACTSeparator:
            return m_pUIAIds->Separator_ControlType;
        case UIAXcp::ACTSemanticZoom:
            return m_pUIAIds->SemanticZoom_ControlType;
        case UIAXcp::ACTAppBar:
            return m_pUIAIds->AppBar_ControlType;
    }
    return (CONTROLTYPEID)0;
}
PROPERTYID CUIAWindow::ConvertEnumToId(_In_ UIAXcp::APAutomationProperties ePropertyType)
{
    switch (ePropertyType)
    {
        case UIAXcp::APAcceleratorKeyProperty:
            return m_pUIAIds->AcceleratorKey_Property;
        case UIAXcp::APAccessKeyProperty:
            return m_pUIAIds->AccessKey_Property;
        case UIAXcp::APAutomationControlTypeProperty:
            return m_pUIAIds->ControlType_Property;
        case UIAXcp::APAutomationIdProperty:
            return m_pUIAIds->AutomationId_Property;
        case UIAXcp::APBoundingRectangleProperty:
            return m_pUIAIds->BoundingRectangle_Property;
        case UIAXcp::APClassNameProperty:
            return m_pUIAIds->ClassName_Property;
        case UIAXcp::APClickablePointProperty:
            return m_pUIAIds->ClickablePoint_Property;
        case UIAXcp::APHelpTextProperty:
            return m_pUIAIds->HelpText_Property;
        case UIAXcp::APItemStatusProperty:
            return m_pUIAIds->ItemStatus_Property;
        case UIAXcp::APItemTypeProperty:
            return m_pUIAIds->ItemType_Property;
        case UIAXcp::APLabeledByProperty:
            return m_pUIAIds->LabeledBy_Property;
        case UIAXcp::APLocalizedControlTypeProperty:
            return m_pUIAIds->LocalizedControlType_Property;
        case UIAXcp::APNameProperty:
            return m_pUIAIds->Name_Property;
        case UIAXcp::APOrientationProperty:
            return m_pUIAIds->Orientation_Property;
        case UIAXcp::APHasKeyboardFocusProperty:
            return m_pUIAIds->HasKeyboardFocus_Property;
        case UIAXcp::APIsContentElementProperty:
            return m_pUIAIds->IsControlElement_Property;
        case UIAXcp::APIsControlElementProperty:
            return m_pUIAIds->IsContentElement_Property;
        case UIAXcp::APIsEnabledProperty:
            return m_pUIAIds->IsEnabled_Property;
        case UIAXcp::APIsKeyboardFocusableProperty:
            return m_pUIAIds->IsKeyboardFocusable_Property;
        case UIAXcp::APIsOffscreenProperty:
            return m_pUIAIds->IsOffscreen_Property;
        case UIAXcp::APIsPasswordProperty:
            return m_pUIAIds->IsPassword_Property;
        case UIAXcp::APIsRequiredForFormProperty:
            return m_pUIAIds->IsRequiredForForm_Property;
        case UIAXcp::APAnnotationTypeIdProperty:
            return m_pUIAIds->Annotation_AnnotationTypeId_Property;
        case UIAXcp::APAnnotationTypeNameProperty:
            return m_pUIAIds->Annotation_AnnotationTypeName_Property;
        case UIAXcp::APAuthorProperty:
            return m_pUIAIds->Annotation_Author_Property;
        case UIAXcp::APDateTimeProperty:
            return m_pUIAIds->Annotation_DateTime_Property;
        case UIAXcp::APTargetProperty:
            return m_pUIAIds->Annotation_Target_Property;
        case UIAXcp::APDockPositionProperty:
            return m_pUIAIds->Dock_DockPosition_Property;
        case UIAXcp::APDropEffectProperty:
            return m_pUIAIds->Drag_DropEffect_Property;
        case UIAXcp::APDropEffectsProperty:
            return m_pUIAIds->Drag_DropEffects_Property;
        case UIAXcp::APGrabbedItemsProperty:
            return m_pUIAIds->Drag_GrabbedItems_Property;
        case UIAXcp::APIsGrabbedProperty:
            return m_pUIAIds->Drag_IsGrabbed_Property;
        case UIAXcp::APDropTargetEffectProperty:
            return m_pUIAIds->DropTarget_DropTargetEffect_Property;
        case UIAXcp::APDropTargetEffectsProperty:
            return m_pUIAIds->DropTarget_DropTargetEffects_Property;
        case UIAXcp::APExpandCollapseStateProperty:
            return m_pUIAIds->ExpandCollapse_ExpandCollapseState_Property;
        case UIAXcp::APColumnProperty:
            return m_pUIAIds->GridItem_Column_Property;
        case UIAXcp::APColumnSpanProperty:
            return m_pUIAIds->GridItem_ColumnSpan_Property;
        case UIAXcp::APContainingGridProperty:
            return m_pUIAIds->GridItem_Parent_Property;
        case UIAXcp::APRowProperty:
            return m_pUIAIds->GridItem_Row_Property;
        case UIAXcp::APRowSpanProperty:
            return m_pUIAIds->GridItem_RowSpan_Property;
        case UIAXcp::APColumnCountProperty:
            return m_pUIAIds->Grid_ColumnCount_Property;
        case UIAXcp::APRowCountProperty:
            return m_pUIAIds->Grid_RowCount_Property;
        case UIAXcp::APCurrentViewProperty:
            return m_pUIAIds->MultipleView_CurrentView_Property;
        case UIAXcp::APSupportedViewsProperty:
            return m_pUIAIds->MultipleView_SupportedViews_Property;
        case UIAXcp::APRangeValueIsReadOnlyProperty:
            return m_pUIAIds->RangeValue_IsReadOnly_Property;
        case UIAXcp::APLargeChangeProperty:
            return m_pUIAIds->RangeValue_LargeChange_Property;
        case UIAXcp::APMaximumProperty:
            return m_pUIAIds->RangeValue_Maximum_Property;
        case UIAXcp::APMinimumProperty:
            return m_pUIAIds->RangeValue_Minimum_Property;
        case UIAXcp::APSmallChangeProperty:
            return m_pUIAIds->RangeValue_SmallChange_Property;
        case UIAXcp::APRangeValueValueProperty:
            return m_pUIAIds->RangeValue_Value_Property;
        case UIAXcp::APHorizontallyScrollableProperty:
            return m_pUIAIds->Scroll_HorizontallyScrollable_Property;
        case UIAXcp::APHorizontalScrollPercentProperty:
            return m_pUIAIds->Scroll_HorizontalScrollPercent_Property;
        case UIAXcp::APHorizontalViewSizeProperty:
            return m_pUIAIds->Scroll_HorizontalViewSize_Property;
        case UIAXcp::APVerticallyScrollableProperty:
            return m_pUIAIds->Scroll_VerticallyScrollable_Property;
        case UIAXcp::APVerticalScrollPercentProperty:
            return m_pUIAIds->Scroll_VerticalScrollPercent_Property;
        case UIAXcp::APVerticalViewSizeProperty:
            return m_pUIAIds->Scroll_VerticalViewSize_Property;
        case UIAXcp::APIsSelectedProperty:
            return m_pUIAIds->SelectionItem_IsSelected_Property;
        case UIAXcp::APSelectionContainerProperty:
            return m_pUIAIds->SelectionItem_SelectionContainer_Property;
        case UIAXcp::APCanSelectMultipleProperty:
            return m_pUIAIds->Selection_CanSelectMultiple_Property;
        case UIAXcp::APIsSelectionRequiredProperty:
            return m_pUIAIds->Selection_IsSelectionRequired_Property;
        case UIAXcp::APSelectionProperty:
            return m_pUIAIds->Selection_Selection_Property;
        case UIAXcp::APColumnHeaderItemsProperty:
            return m_pUIAIds->TableItem_ColumnHeaderItems_Property;
        case UIAXcp::APRowHeaderItemsProperty:
            return m_pUIAIds->TableItem_RowHeaderItems_Property;
        case UIAXcp::APColumnHeadersProperty:
            return m_pUIAIds->Table_ColumnHeaders_Property;
        case UIAXcp::APRowHeadersProperty:
            return m_pUIAIds->Table_RowHeaders_Property;
        case UIAXcp::APRowOrColumnMajorProperty:
            return m_pUIAIds->Table_RowOrColumn_Property;
        case UIAXcp::APToggleStateProperty:
            return m_pUIAIds->Toggle_ToggleState_Property;
        case UIAXcp::APCanMoveProperty:
            return m_pUIAIds->Transform_CanMove_Property;
        case UIAXcp::APCanResizeProperty:
            return m_pUIAIds->Transform_CanResize_Property;
        case UIAXcp::APCanRotateProperty:
            return m_pUIAIds->Transform_CanRotate_Property;
        case UIAXcp::APValueIsReadOnlyProperty:
            return m_pUIAIds->Value_IsReadOnly_Property;
        case UIAXcp::APValueValueProperty:
            return m_pUIAIds->Value_Value_Property;
        case UIAXcp::APCanMaximizeProperty:
            return m_pUIAIds->Window_CanMaximize_Property;
        case UIAXcp::APCanMinimizeProperty:
            return m_pUIAIds->Window_CanMinimize_Property;
        case UIAXcp::APIsModalProperty:
            return m_pUIAIds->Window_IsModal_Property;
        case UIAXcp::APIsTopmostProperty:
            return m_pUIAIds->Window_IsTopmost_Property;
        case UIAXcp::APWindowInteractionStateProperty:
            return m_pUIAIds->Window_WindowInteractionState_Property;
        case UIAXcp::APWindowVisualStateProperty:
            return m_pUIAIds->Window_WindowVisualState_Property;
        case UIAXcp::APLiveSettingProperty:
            return m_pUIAIds->LiveSetting_Property;
        case UIAXcp::APControlledPeersProperty:
            return m_pUIAIds->ControlledPeers_Property;
        case UIAXcp::APCanZoomProperty:
            return m_pUIAIds->Transform2_CanZoom_Property;
        case UIAXcp::APZoomLevelProperty:
            return m_pUIAIds->Transform2_ZoomLevel_Property;
        case UIAXcp::APMaxZoomProperty:
            return m_pUIAIds->Transform2_MaxZoom_Property;
        case UIAXcp::APMinZoomProperty:
            return m_pUIAIds->Transform2_MinZoom_Property;
        case UIAXcp::APFormulaProperty:
            return m_pUIAIds->SpreadsheetItem_Formula_Property;
        case UIAXcp::APExtendedPropertiesProperty:
            return m_pUIAIds->Styles_ExtendedProperties_Property;
        case UIAXcp::APFillColorProperty:
            return m_pUIAIds->Styles_FillColor_Property;
        case UIAXcp::APFillPatternColorProperty:
            return m_pUIAIds->Styles_FillPatternColor_Property;
        case UIAXcp::APFillPatternStyleProperty:
            return m_pUIAIds->Styles_FillPatternStyle_Property;
        case UIAXcp::APShapeProperty:
            return m_pUIAIds->Styles_Shape_Property;
        case UIAXcp::APStyleIdProperty:
            return m_pUIAIds->Styles_StyleId_Property;
        case UIAXcp::APStyleNameProperty:
            return m_pUIAIds->Styles_StyleName_Property;
        case UIAXcp::APFlowsFromProperty:
            return m_pUIAIds->FlowsFrom_Property;
        case UIAXcp::APFlowsToProperty:
            return m_pUIAIds->FlowsTo_Property;
        case UIAXcp::APPositionInSetProperty:
            return m_pUIAIds->PositionInSet_Property;
        case UIAXcp::APSizeOfSetProperty:
            return m_pUIAIds->SizeOfSet_Property;
        case UIAXcp::APLevelProperty:
            return m_pUIAIds->Level_Property;
        case UIAXcp::APIsPeripheralProperty:
            return m_pUIAIds->IsPeripheral_Property;
        case UIAXcp::APIsDataValidForFormProperty:
            return m_pUIAIds->IsDataValidForForm_Property;
        case UIAXcp::APFullDescriptionProperty:
            return m_pUIAIds->FullDescription_Property;
        case UIAXcp::APDescribedByProperty:
            return m_pUIAIds->DescribedBy_Property;
        case UIAXcp::APCultureProperty:
            return m_pUIAIds->Culture_Property;
        case UIAXcp::APIsDialogProperty:
            return m_pUIAIds->IsDialog_Property;
    }
    return (PROPERTYID)0;
}

LANDMARKTYPEID CUIAWindow::ConvertEnumToId(_In_ UIAXcp::AutomationLandmarkType eLandmarkType)
{
    switch (eLandmarkType)
    {
        case UIAXcp::AutomationLandmarkType_None:
            return (LANDMARKTYPEID)0;
        case UIAXcp::AutomationLandmarkType_Custom:
            return UIA_CustomLandmarkTypeId;
        case UIAXcp::AutomationLandmarkType_Form:
            return UIA_FormLandmarkTypeId;
        case UIAXcp::AutomationLandmarkType_Main:
            return UIA_MainLandmarkTypeId;
        case UIAXcp::AutomationLandmarkType_Navigation:
            return UIA_NavigationLandmarkTypeId;
        case UIAXcp::AutomationLandmarkType_Search:
            return UIA_SearchLandmarkTypeId;
    }
    return (LANDMARKTYPEID)0;
}

HEADINGLEVELID CUIAWindow::ConvertEnumToId(_In_ UIAXcp::AutomationHeadingLevel eHeadingLevel)
{
    switch (eHeadingLevel)
    {
    case UIAXcp::AutomationHeadingLevel_None:
        return HeadingLevel_None;
    case UIAXcp::AutomationHeadingLevel_HeadingLevel1:
        return HeadingLevel1;
    case UIAXcp::AutomationHeadingLevel_HeadingLevel2:
        return HeadingLevel2;
    case UIAXcp::AutomationHeadingLevel_HeadingLevel3:
        return HeadingLevel3;
    case UIAXcp::AutomationHeadingLevel_HeadingLevel4:
        return HeadingLevel4;
    case UIAXcp::AutomationHeadingLevel_HeadingLevel5:
        return HeadingLevel5;
    case UIAXcp::AutomationHeadingLevel_HeadingLevel6:
        return HeadingLevel6;
    case UIAXcp::AutomationHeadingLevel_HeadingLevel7:
        return HeadingLevel7;
    case UIAXcp::AutomationHeadingLevel_HeadingLevel8:
        return HeadingLevel8;
    case UIAXcp::AutomationHeadingLevel_HeadingLevel9:
        return HeadingLevel9;
    }

    return HeadingLevel_None;
}

UIAXcp::APAutomationProperties CUIAWindow::ConvertIdToAPAutomationProperties(_In_ PROPERTYID ePropertyId)
{
    if (ePropertyId == m_pUIAIds->ControlType_Property)
    {
        return UIAXcp::APAutomationControlTypeProperty;
    }
    else if(ePropertyId == m_pUIAIds->AutomationId_Property)
    {
        return UIAXcp::APAutomationIdProperty;
    }
    else if(ePropertyId == m_pUIAIds->Name_Property)
    {
        return UIAXcp::APNameProperty;
    }
    else if(ePropertyId == m_pUIAIds->SelectionItem_IsSelected_Property)
    {
        return UIAXcp::APIsSelectedProperty;
    }
    else if(0 == ePropertyId)
    {
        return UIAXcp::APEmptyProperty;
    }

    return (UIAXcp::APAutomationProperties)0;
}

UIAXcp::APAutomationProperties CUIAWindow::GetAutomationComponentProperty(_In_ PROPERTYID ePropertyId)
{
    if (ePropertyId == m_pUIAIds->ControlledPeers_Property || ePropertyId == UIA_ControllerForPropertyId)
    {
        return UIAXcp::APControlledPeersProperty;
    }
    else if (ePropertyId == m_pUIAIds->DescribedBy_Property)
    {
        return UIAXcp::APDescribedByProperty;
    }
    else if (ePropertyId == m_pUIAIds->LabeledBy_Property)
    {
        return UIAXcp::APLabeledByProperty;
    }

    return (UIAXcp::APAutomationProperties)((int)-1);
}

UIAXcp::APAutomationControlType CUIAWindow::ConvertIdToAPAutomationControlType(_In_ CONTROLTYPEID eControlTypeId)
{
    if (eControlTypeId == m_pUIAIds->Button_ControlType)
        return UIAXcp::ACTButton;
    else if (eControlTypeId == m_pUIAIds->Calendar_ControlType)
        return UIAXcp::ACTCalendar;
    else if (eControlTypeId == m_pUIAIds->CheckBox_ControlType)
        return UIAXcp::ACTCheckBox;
    else if (eControlTypeId == m_pUIAIds->ComboBox_ControlType)
        return UIAXcp::ACTComboBox;
    else if (eControlTypeId == m_pUIAIds->Edit_ControlType)
        return UIAXcp::ACTEdit;
    else if (eControlTypeId == m_pUIAIds->Hyperlink_ControlType)
        return UIAXcp::ACTHyperlink;
    else if (eControlTypeId == m_pUIAIds->Image_ControlType)
        return UIAXcp::ACTImage;
    else if (eControlTypeId == m_pUIAIds->ListItem_ControlType)
        return UIAXcp::ACTListItem;
    else if (eControlTypeId == m_pUIAIds->List_ControlType)
        return UIAXcp::ACTList;
    else if (eControlTypeId == m_pUIAIds->Menu_ControlType)
        return UIAXcp::ACTMenu;
    else if (eControlTypeId == m_pUIAIds->MenuBar_ControlType)
        return UIAXcp::ACTMenuBar;
    else if (eControlTypeId == m_pUIAIds->MenuItem_ControlType)
        return UIAXcp::ACTMenuItem;
    else if (eControlTypeId == m_pUIAIds->ProgressBar_ControlType)
        return UIAXcp::ACTProgressBar;
    else if (eControlTypeId == m_pUIAIds->RadioButton_ControlType)
        return UIAXcp::ACTRadioButton;
    else if (eControlTypeId == m_pUIAIds->ScrollBar_ControlType)
        return UIAXcp::ACTScrollBar;
    else if (eControlTypeId == m_pUIAIds->Slider_ControlType)
        return UIAXcp::ACTSlider;
    else if (eControlTypeId == m_pUIAIds->Spinner_ControlType)
        return UIAXcp::ACTSpinner;
    else if (eControlTypeId == m_pUIAIds->StatusBar_ControlType)
        return UIAXcp::ACTStatusBar;
    else if (eControlTypeId == m_pUIAIds->Tab_ControlType)
        return UIAXcp::ACTTab;
    else if (eControlTypeId == m_pUIAIds->TabItem_ControlType)
        return UIAXcp::ACTTabItem;
    else if (eControlTypeId == m_pUIAIds->Text_ControlType)
        return UIAXcp::ACTText;
    else if (eControlTypeId == m_pUIAIds->ToolBar_ControlType)
        return UIAXcp::ACTToolBar;
    else if (eControlTypeId == m_pUIAIds->ToolTip_ControlType)
        return UIAXcp::ACTToolTip;
    else if (eControlTypeId == m_pUIAIds->Tree_ControlType)
        return UIAXcp::ACTTree;
    else if (eControlTypeId == m_pUIAIds->TreeItem_ControlType)
        return UIAXcp::ACTTreeItem;
    else if (eControlTypeId == m_pUIAIds->Custom_ControlType)
        return UIAXcp::ACTCustom;
    else if (eControlTypeId == m_pUIAIds->Group_ControlType)
        return UIAXcp::ACTGroup;
    else if (eControlTypeId == m_pUIAIds->Thumb_ControlType)
        return UIAXcp::ACTThumb;
    else if (eControlTypeId == m_pUIAIds->DataGrid_ControlType)
        return UIAXcp::ACTDataGrid;
    else if (eControlTypeId == m_pUIAIds->DataItem_ControlType)
        return UIAXcp::ACTDataItem;
    else if (eControlTypeId == m_pUIAIds->Document_ControlType)
        return UIAXcp::ACTDocument;
    else if (eControlTypeId == m_pUIAIds->SplitButton_ControlType)
        return UIAXcp::ACTSplitButton;
    else if (eControlTypeId == m_pUIAIds->Window_ControlType)
        return UIAXcp::ACTWindow;
    else if (eControlTypeId == m_pUIAIds->Pane_ControlType)
        return UIAXcp::ACTPane;
    else if (eControlTypeId == m_pUIAIds->Header_ControlType)
        return UIAXcp::ACTHeader;
    else if (eControlTypeId == m_pUIAIds->HeaderItem_ControlType)
        return UIAXcp::ACTHeaderItem;
    else if (eControlTypeId == m_pUIAIds->Table_ControlType)
        return UIAXcp::ACTTable;
    else if (eControlTypeId == m_pUIAIds->TitleBar_ControlType)
        return UIAXcp::ACTTitleBar;
    else if (eControlTypeId == m_pUIAIds->Separator_ControlType)
        return UIAXcp::ACTSeparator;
    else if (eControlTypeId == m_pUIAIds->SemanticZoom_ControlType)
        return UIAXcp::ACTSemanticZoom;
    else if (eControlTypeId == m_pUIAIds->AppBar_ControlType)
        return UIAXcp::ACTAppBar;
    else
        return (UIAXcp::APAutomationControlType)0;
}

//-------------------------------------------------------------------------
//
//  Function:   CValueToVariant
//
//  Synopsis:
//     Convert one  VARIANT to CValue
//
//-------------------------------------------------------------------------
HRESULT CUIAWindow::CValueToVariant(
    _In_ const CValue& value,
    _Out_ VARIANT* pVAR)
{
    // The return type from this call can be boxed or unboxed so account for both
    switch (value.GetType())
    {
    case valueAny:
    case valueNull:
        {
             pVAR->vt = VT_EMPTY;
        }
        break;
    case valueFloat:
        {
             pVAR->vt = VT_R4;
             pVAR->fltVal = value.AsFloat();
        }
        break;
    case valueString:
        {
            xstring_ptr strValue = value.AsString();
            pVAR->vt = VT_BSTR;
            pVAR->bstrVal = SysAllocStringLen(strValue.GetBuffer(), strValue.GetCount());
            IFCOOM_RETURN(pVAR->bstrVal);
        }
        break;
    case valueColor:
    case valueSigned:
        {
             pVAR->vt = VT_INT;
             pVAR->intVal = value.AsSigned();
        }
        break;
    case valueBool:
        {
             pVAR->vt = VT_BOOL;
             pVAR->boolVal = value.AsBool() ? VARIANT_TRUE : VARIANT_FALSE;
        }
        break;
    case valueEnum:
    case valueEnum8:
        {
             pVAR->vt = VT_I4;
             pVAR->lVal = value.AsEnum();
        }
        break;
    case valueDouble:
        {
            pVAR->vt = VT_R8;
            pVAR->dblVal = value.AsDouble();
        }
        break;
    case valueIUnknown:
        {
            pVAR->vt = VT_UNKNOWN;
            pVAR->punkVal = value.AsIUnknown();
        }
        break;
    case valueSignedArray:
        {
            IFC_RETURN(ArrayToSafeArray(value.AsSignedArray(), value.GetArrayElementCount(), VT_I4, &pVAR->parray));
            pVAR->vt = VT_ARRAY | VT_I4;
        }
        break;
    case valueDoubleArray:
        {
            IFC_RETURN(ArrayToSafeArray(value.AsDoubleArray(), value.GetArrayElementCount(), VT_R8, &pVAR->parray));
            pVAR->vt = VT_ARRAY | VT_R8;
        }
        break;
    default:
        {
            IFC_RETURN(E_FAIL);
        }
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   VariantToCValue
//
//  Synopsis:
//     Convert one  CValue to VARIANT
//
//-------------------------------------------------------------------------
HRESULT CUIAWindow::VariantToCValue(
    _In_ const VARIANT* pVAR,
    _Out_ CValue *pValue)
{
    IFCPTR_RETURN(pVAR);

    // The return type from this call can be boxed or unboxed so account for both
    switch (pVAR->vt)
    {
    case VT_UNKNOWN:
    case VT_EMPTY:
    case VT_NULL:
        {
            pValue->SetNull();
        }
        break;
    case VT_R4:
        {
            pValue->SetFloat(pVAR->fltVal);
        }
        break;
    case VT_BSTR:
        {
            xstring_ptr strValue;
            IFC_RETURN(xstring_ptr::CloneBuffer(pVAR->bstrVal, SysStringLen(pVAR->bstrVal), &strValue));
            pValue->SetString(std::move(strValue));
        }
        break;
    case VT_INT:
        {
            pValue->SetSigned(pVAR->intVal);
        }
        break;
    case VT_BOOL:
        {
            if(pVAR->boolVal == VARIANT_TRUE)
            {
                pValue->SetBool(TRUE);
            }
            else
            {
                pValue->SetBool(FALSE);
            }
        }
        break;
    case VT_I4:
        {
            pValue->SetEnum(static_cast<XUINT32>(pVAR->lVal));
        }
        break;
    case VT_R8:
        {
            pValue->SetDouble(pVAR->dblVal);
        }
        break;
    case (VT_ARRAY | VT_R8) :
        {
            XDOUBLE *rgData = nullptr;
            int count = 0;
            IFC_RETURN(SafeArrayToArray(pVAR->parray, &rgData, &count, VT_R8));
            pValue->SetDoubleArray(count, rgData);
        }
        break;
    case (VT_ARRAY | VT_I4) :
        {
            XINT32 *rgData = nullptr;
            int count = 0;
            IFC_RETURN(SafeArrayToArray(pVAR->parray, &rgData, &count, VT_I4));
            pValue->SetSignedArray(count, rgData);
        }
        break;
    default:
        {
            IFC_RETURN(E_FAIL);
        }
    }

    return S_OK;
}

HRESULT CUIAWindow::AutomationPeerAnnotationCollectionToVariant(
    _In_opt_ CAutomationPeerAnnotationCollection* pAnnotations,
    _In_ PROPERTYID propertyId,
    _Out_ VARIANT* pResult)
{
    HRESULT hr = S_OK;
    CDependencyObject* pDO = nullptr;
    CAutomationPeerAnnotation* pAutomationAnnotation = nullptr;
    int count = 0;
    VARTYPE vt = static_cast<VARTYPE>((propertyId == m_pUIAIds->AnnotationTypes_Property) ? VT_I4 : VT_UNKNOWN);
    bool fHaveNonNullsInObjects = false;

    ASSERT(pResult);
    VariantInit(pResult);

    count = pAnnotations ? pAnnotations->GetCount() : 0;
    if (count == 0)
    {
        return S_OK;
    }

    unique_safearray safeArray(SafeArrayCreateVector(vt, 0, count));
    IFCOOMFAILFAST(safeArray.get());

    for (LONG i = 0; i < count; i++)
    {
        bool good = false;
        pDO = pAnnotations->GetItemImpl(i);
        hr = DoPointerCast(pAutomationAnnotation, pDO);

        if (hr == S_OK && pAutomationAnnotation)
        {
            if (propertyId == m_pUIAIds->AnnotationTypes_Property)
            {
                good = true;
                CValue typeValue;
                hr = pAutomationAnnotation->GetValueByIndex(KnownPropertyIndex::AutomationPeerAnnotation_Type, &typeValue);
                if (hr == S_OK)
                {
                    XUINT32 type;
                    hr = typeValue.GetEnum(type);
                    if (hr == S_OK)
                    {
                        hr = SafeArrayPutElement(safeArray.get(), &i, &type);
                    }
                }
            }
            else if (propertyId == m_pUIAIds->AnnotationObjects_Property)
            {
                CValue peerValue;
                hr = pAutomationAnnotation->GetValueByIndex(KnownPropertyIndex::AutomationPeerAnnotation_Peer, &peerValue);
                if (hr == S_OK && peerValue.GetType() == valueObject)
                {
                    CDependencyObject *peerObject = nullptr;
                    hr = peerValue.GetObject(peerObject);
                    if (hr == S_OK)
                    {
                        CAutomationPeer *pPeer = static_cast<CAutomationPeer*>(peerObject);
                        if (pPeer)
                        {
                            Microsoft::WRL::ComPtr<CUIAWrapper> spWrapper;
                            hr = CreateProviderForAP(pPeer, spWrapper.GetAddressOf());
                            if (hr == S_OK && spWrapper)
                            {
                                Microsoft::WRL::ComPtr<IUnknown> spUnk;
                                hr = spWrapper.Get()->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(spUnk.GetAddressOf()));
                                if (hr == S_OK && spUnk)
                                {
                                    good = true;
                                    fHaveNonNullsInObjects = true;
                                    hr = SafeArrayPutElement(safeArray.get(), &i, spUnk.Get());
                                }
                            }
                        }
                    }
                }
                else if (hr == S_OK && peerValue.GetType() == valueNull)
                {
                    // nullptr is a valid value for annotation object
                    good = true;
                    hr = SafeArrayPutElement(safeArray.get(), &i, nullptr);
                }
            }

            if (hr != S_OK || !good)
            {
                IFC_RETURN(FAILED(hr) ? hr : E_FAIL);
            }
        }
    }

    // If we only have NULLs in annotation objects array, return nothing
    if (!(propertyId == m_pUIAIds->AnnotationObjects_Property && !fHaveNonNullsInObjects))
    {
        pResult->vt = VT_ARRAY | vt;
        pResult->parray = safeArray.release();
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetUIAWindowValidator
//
//  Synopsis: Returns a shared objects that is used to validate an UIAWindow
//            for a given wrapper
//
//------------------------------------------------------------------------

IUIAWindowValidator* CUIAWindow::GetUIAWindowValidator()
{
    if (!m_pUIAWindowValidator)
    {
        m_pUIAWindowValidator = new CUIAWindowValidator(this);
    }

    AddRefInterface(m_pUIAWindowValidator);

    return m_pUIAWindowValidator;
}

//------------------------------------------------------------------------
//
//  Method:   FlushUiaBridgeEventTable
//
//  Synopsis:
//  When a window that previously returned providers has been destroyed, we should notify UI Automation by
//  calling the UiaReturnRawElementProvider function as follows : UiaReturnRawElementProvider(hwnd, 0, 0, nullptr).
//  This call tells UI Automation that it can safely remove all map entries that refer to the specified window.
//
//------------------------------------------------------------------------
void CUIAWindow::FlushUiaBridgeEventTable()
{
    if (m_uiaHostEnvironmentInfo.IsHwndBased())
    {
        UiaReturnRawElementProvider(m_uiaHostEnvironmentInfo.GetElementWindow(), 0, 0, nullptr);
    }
}


//------------------------------------------------------------------------
//
//  Method: GetOverrideProviderForHwnd
//
//  Synopsis:
//       Used for Hwnd provider repositioning. Returns the provider that overrides the
//       default Hwnd automation provider
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAWindow::GetOverrideProviderForHwnd(_In_ HWND hwnd, _Outptr_result_maybenull_ IRawElementProviderSimple **pRetVal)
{
    if (!m_fInitialized)
    {
        IFC_NOTRACE_RETURN(static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
    }

    IFC_RETURN(GetOverrideProviderForHwndImpl(hwnd, pRetVal));

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method: GetOverrideProviderForHwndImpl
//
//  Synopsis:
//       Used for Hwnd provider repositioning. Returns the provider that overrides the
//       default Hwnd automation provider. Must execute on the UI-thread.
//
//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CUIAWindow::GetOverrideProviderForHwndImpl(_In_ HWND hwnd, _Outptr_result_maybenull_ IRawElementProviderSimple **ppRetVal)
{
    *ppRetVal = nullptr;
    // AP returned by GetAPForHwnd is not ref counted, use raw pointer
    CAutomationPeer* hwndAP = nullptr;
    xref_ptr<IRawElementProviderSimple> provider;
    xref_ptr<CUIAWrapper> wrapper;

    if ((m_uiaHostEnvironmentInfo.IsHwndBased() && hwnd != m_uiaHostEnvironmentInfo.GetElementWindow())
        || m_uiaHostEnvironmentInfo.GetXamlIslandRoot() != nullptr)
    {
        IFC_RETURN(GetAPForHwnd(hwnd, nullptr, &hwndAP));
        if (hwndAP)
        {
            IFC_RETURN(CreateProviderForAP(hwndAP, wrapper.ReleaseAndGetAddressOf()));
            IFC_RETURN(wrapper->QueryInterface(__uuidof(IRawElementProviderSimple), reinterpret_cast<void**>(provider.ReleaseAndGetAddressOf())));
        }
    }

    *ppRetVal = provider.detach();

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method: GetAPForHwnd
//
//  Synopsis:
//       Gets the Interop automation peer corresponding to hwnd.
//
//------------------------------------------------------------------------
HRESULT CUIAWindow::GetAPForHwnd(HWND hwnd, CAutomationPeer* pAPRoot, CAutomationPeer** ppHwndAP)
{
    HRESULT hr = S_OK;
    CAutomationPeer** ppChildren = nullptr;

    IFCPTR(ppHwndAP);
    *ppHwndAP = nullptr;

    if(pAPRoot)
    {
        ctl::ComPtr<IAutomationPeerHwndInterop> interopPeer;
        HRESULT result = S_OK;
        if (pAPRoot->GetDXamlPeer())
        {
            result = ctl::iinspectable_cast(pAPRoot->GetDXamlPeer())->QueryInterface<IAutomationPeerHwndInterop>(&interopPeer);
        }
        if (result == S_OK && interopPeer != nullptr)
        {
            bool foundPeer = false;
            IFC(interopPeer->IsCorrectPeerForHwnd(hwnd, &foundPeer));
            // Can't have an element using hwnds inside another element using hwnds,
            // so it's ok if this returns false but we don't continue checking the children.
            if (foundPeer)
            {
                *ppHwndAP = pAPRoot;
            }
        }
        else
        {
            XINT32 cChildren = pAPRoot->GetChildren(&ppChildren);
            for(XINT32 iChild = 0; iChild < cChildren && !*ppHwndAP; iChild++)
            {
                IFC(GetAPForHwnd(hwnd, ppChildren[iChild], ppHwndAP));
            }
        }
    }
    else
    {
        // We are not give an automation peer to start our search from. Start from the visual root.
        xref_ptr<CDependencyObject> pDO;
        CAutomationPeer *pAP = nullptr;
        IFC(GetRootVisual(pDO));

        if (pDO != nullptr)
        {
            pAP = pDO->OnCreateAutomationPeer();
            if (pAP)
            {
                IFC(GetAPForHwnd(hwnd, pAP, ppHwndAP));
            }
            else
            {
                XINT32 cChildren = pDO->GetAPChildren(&ppChildren);
                for (XINT32 iChild = 0; iChild < cChildren && !*ppHwndAP; iChild++)
                {
                    IFC(GetAPForHwnd(hwnd, ppChildren[iChild], ppHwndAP));
                }
            }
        }
    }

Cleanup:
    delete[] ppChildren;
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Method: CreateProviderForAP
//
//  Synopsis:
//       Helper method to create CUIAWrapper for a given automation peer
//
//------------------------------------------------------------------------
HRESULT CUIAWindow::CreateProviderForAP(CAutomationPeer* pAP, CUIAWrapper** ppRet)
{
    xref_ptr<CUIAWrapper> wrapper;

    IFCPTR_RETURN(pAP);

    IFC_RETURN(CUIAWrapper::Create(m_uiaHostEnvironmentInfo, m_pHost, this, pAP, m_pUIAIds, wrapper.ReleaseAndGetAddressOf()));

    *ppRet = wrapper.detach();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: GetDisableKeyboardDisplayOnProgrammaticFocusId
//
//  Synopsis:
//       Get Property ID for custom UIA property DisableKeyboardDisplayOnProgrammaticFocus
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CUIAWindow::GetPreventKeyboardDisplayOnProgrammaticFocusId()
 {
    Microsoft::WRL::ComPtr<IUIAutomationRegistrar> registrar;
    PROPERTYID customPropertyId;
    UIAutomationPropertyInfo customPropertyInfo =
    {
        GUID_PreventKeyboardDisplayOnProgrammaticFocus,
        L"DisableKeyboardDisplayOnProgrammaticFocus ",
        UIAutomationType_Bool
    };
    IFC_RETURN(CoCreateInstance(CLSID_CUIAutomationRegistrar, nullptr, CLSCTX_INPROC_SERVER, IID_IUIAutomationRegistrar, (void **)&registrar));
    IFC_RETURN(registrar->RegisterProperty(&customPropertyInfo, &customPropertyId));
    m_pUIAIds->PreventKeyboardDisplayOnProgrammaticFocus_Property = customPropertyId;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Transform given point from screen coordinates to client coordinates
//      Uses phone's orientation transform if needed
//
//------------------------------------------------------------------------
BOOL CUIAWindow::TransformClientToScreen(_In_ UIAHostEnvironmentInfo& uiaHostEnvironmentInfo, _Inout_ POINT* pPoint)
{
    BOOL fResult = TRUE;

    fResult = uiaHostEnvironmentInfo.GlobalSpaceToUiaSpace(pPoint);

    return fResult;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Transform given rect from screen coordinates to client coordinates
//      Uses phone's orientation transform if needed
//
//------------------------------------------------------------------------
BOOL CUIAWindow::TransformClientToScreen(_In_ UIAHostEnvironmentInfo& uiaHostEnvironmentInfo, _Inout_ XRECTF* pRect)
{
    BOOL fResult = TRUE;
    POINT ptTopLeft = { };

    fResult = uiaHostEnvironmentInfo.GlobalSpaceToUiaSpace(pRect);

    ptTopLeft.x = static_cast<LONG>(pRect->X);
    ptTopLeft.y = static_cast<LONG>(pRect->Y);

    pRect->X = static_cast<XFLOAT>(ptTopLeft.x);
    pRect->Y = static_cast<XFLOAT>(ptTopLeft.y);

    return fResult;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Transform given point from client coordinates to screen coordinates
//      Uses phone's orientation transform if needed
//
//------------------------------------------------------------------------
BOOL CUIAWindow::TransformScreenToClient(_In_ const UIAHostEnvironmentInfo& uiaHostEnvironmentInfo, _Inout_ POINT* pPoint)
{
    return uiaHostEnvironmentInfo.UiaSpaceToGlobalSpace(pPoint);
}


//-------------------------------------------------------------------------
//
//  Function:   ConvertToVariant
//
//  Synopsis:
//     Helper functions to convert a collections of AutomationPeer into
//     a SAFEARRAY of IUnknown's
//
//-------------------------------------------------------------------------

HRESULT CUIAWindow::ConvertToVariant(_In_ CAutomationPeerCollection* pAutomationPeerCollection, _Out_ VARIANT* pResult)
{
    xref_ptr<CDependencyObject> spDO;
    xref_ptr<CAutomationPeer> spAP;
    xref_ptr<CUIAWrapper> spWrapper;
    xref_ptr<IUnknown> spUnk;
    LONG safeArrayCount = 0;
    XUINT32 count = 0;
    if (!m_pUIAWindowValidator || !m_pUIAWindowValidator->IsValid())
    {
        return E_FAIL;
    }

    count = pAutomationPeerCollection ? pAutomationPeerCollection->GetCount() : 0;
    if (count == 0)
    {
        VariantInit(pResult);
        return S_OK;
    }

    unique_safearray safeArray(SafeArrayCreateVector(VT_UNKNOWN, 0, count));
    IFCOOMFAILFAST(safeArray.get());

    while (0 < count--)
    {
        spDO = pAutomationPeerCollection->GetItemImpl(safeArrayCount);
        spAP = do_pointer_cast <CAutomationPeer>(spDO.get());
        IFCPTR_RETURN(spAP.get());

        IFC_RETURN(CreateProviderForAP(spAP.get(), spWrapper.ReleaseAndGetAddressOf()));
        IFCPTR_RETURN(spWrapper.get());
        IFC_RETURN(spWrapper->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**> (spUnk.ReleaseAndGetAddressOf())));
        IFCPTR_RETURN(spUnk.get());

        IFC_RETURN(SafeArrayPutElement(safeArray.get(), &safeArrayCount, spUnk.get()));
        ++safeArrayCount;
    }

    pResult->vt = VT_ARRAY | VT_UNKNOWN;
    pResult->parray = safeArray.release();
    return S_OK;
}

//static
bool CUIAWindow::ShouldFrameworkProvideWindowProperties()
{
    // Supported only if not desktop. Phone & OneCore don't support full HWND.
    // Hence XAML needs to provide all application window related properties
    return !DesktopUtility::IsOnDesktop();
}

// This may return null when running in UWP or when called as the island is shutting down.
xref_ptr<ixp::IContentIslandAutomation> CUIAWindow::GetContentIslandAutomation()
{
    if (auto contentIsland = GetContentIsland())
    {
        xref_ptr<ixp::IContentIslandAutomation> contentIslandAutomation;
        IFCFAILFAST(contentIsland->QueryInterface(IID_PPV_ARGS(contentIslandAutomation.ReleaseAndGetAddressOf())));
        return contentIslandAutomation;
    }
    return nullptr;
}

CUIAHostWindow::CUIAHostWindow(
    _In_ const UIAHostEnvironmentInfo& uiaHostEnvironmentInfo,
    _In_ IXcpHostSite *pHost,
    _In_opt_ CDependencyObject *pPopup)
    : CUIAWindow(uiaHostEnvironmentInfo, pHost)
{
    if (pPopup)
    {
       m_popupWeakRef = xref::get_weakref(pPopup);
    }
}

//-------------------------------------------------------------------------
//
// CUIAHostWindow::Create
//
//-------------------------------------------------------------------------
HRESULT CUIAHostWindow::Create(
    _In_ const UIAHostEnvironmentInfo& uiaHostEnvironmentInfo,
    _In_ IXcpHostSite *pHost,
    _In_opt_ CDependencyObject *pPopup,
    _Outptr_ CUIAHostWindow** ppUIAWindow)
{
    *ppUIAWindow = new CUIAHostWindow(uiaHostEnvironmentInfo, pHost, pPopup);
    IFC_RETURN((*ppUIAWindow)->Init());
    return S_OK;
}

HRESULT CUIAHostWindow::GetRootVisual(_Out_ xref_ptr<CDependencyObject>& rootVisual)
{
    rootVisual = nullptr;

    if (m_popupWeakRef)
    {
        // Popup case
        rootVisual = m_popupWeakRef.lock();
    }
    else
    {
        if (m_uiaHostEnvironmentInfo.IsHwndBased())
        {
            // UWP case (deprecated)
            IXcpBrowserHost *pBH = m_pHost->GetBrowserHost();
            if (pBH != nullptr)
            {
                rootVisual = pBH->GetPublicOrFullScreenRootVisual();
            }
        }
        else
        {
            // Island case
            rootVisual = m_uiaHostEnvironmentInfo.GetXamlIslandRoot()->GetPublicRootVisual();
        }
    }

    return S_OK; //RRETURN_REMOVAL
}

HRESULT CUIAHostWindow::GetPropertyValueOverride(_In_ PROPERTYID propertyId, _Out_ VARIANT * pRetVal)
{
    HRESULT hr = S_OK;
    HSTRING strSessionName = nullptr;

    if (propertyId == m_pUIAIds->ClickablePoint_Property)
    {
        POINT ptScreen;
        XUINT32 x = 0;
        XUINT32 y = 0;
        LONG lLbound = 0;
        double ptValue = 0;

        IFC(m_pHost->GetActualWidth(&x));
        IFC(m_pHost->GetActualHeight(&y));
        ptScreen.x = static_cast<LONG>(x / 2);
        ptScreen.y = static_cast<LONG>(y / 2);

        // Convert the point from client to screen coordinate
        VERIFY(TransformClientToScreen(GetHostEnvironmentInfo(), &ptScreen));

        pRetVal->vt = VT_R8 | VT_ARRAY;
        unique_safearray safeArray(SafeArrayCreateVector(VT_R8, 0, 2));
        IFCOOMFAILFAST(safeArray.get());

        // Add x Point value
        ptValue = static_cast<XDOUBLE>(ptScreen.x);
        IFC(SafeArrayPutElement(safeArray.get(), &lLbound, static_cast<void*>(&ptValue)));

        // Add y Point value
        lLbound = 1;
        ptValue = static_cast<XDOUBLE>(ptScreen.y);
        IFC(SafeArrayPutElement(safeArray.get(), &lLbound, static_cast<void*>(&ptValue)));

        pRetVal->parray = safeArray.release();
    }
    else if (propertyId == m_pUIAIds->HasKeyboardFocus_Property)
    {
        ctl::ComPtr<IRawElementProviderFragment> spFocusedElement;
        IFC(GetFocus(&spFocusedElement));
        // if no element owns focus, then this window has focus.
        if (spFocusedElement == nullptr)
        {
            // if no element owns focus, return a value to represent the whole window.  Historically we always returned
            // true here, but found a case in SV2 with the ALT+tab menu where focus was landing on an empty XamlIslandRoot.
            // So in this case, let's return false since there's nothing interesting for UIA to focus on.  In the future
            // it might be a more appropriate fix to always return FALSE when the XamlIslandRoot isn't focused.
            pRetVal->vt = VT_BOOL;
            pRetVal->boolVal = VARIANT_TRUE;
            if (xref_ptr<CXamlIslandRoot> xamlIslandRoot = m_uiaHostEnvironmentInfo.GetXamlIslandRoot())
            {
                if (xamlIslandRoot->GetPublicRootVisual() == nullptr && !xamlIslandRoot->HasFocus())
                {
                    // The island doesn't have content or focus, just return false
                    pRetVal->boolVal = VARIANT_FALSE;
                }
            }
        }
        else
        {
            // otherwise the focused element should already handle the focus.
            pRetVal->vt = VT_EMPTY;
        }
    }
    else if (propertyId == m_pUIAIds->FrameworkId_Property)
    {
        // The FrameworkId property needs to be specified for XAML window and doesn't need to be localized,
        pRetVal->vt = VT_BSTR;
        pRetVal->bstrVal = SysAllocString(L"XAML");
    }
    else if (propertyId == m_pUIAIds->ClassName_Property && ShouldFrameworkProvideWindowProperties())
    {
        // On the desktop the class name will be provided by the uiacore hwndproxy
        pRetVal->vt = VT_BSTR;
        pRetVal->bstrVal = SysAllocString(L"Windows.UI.Core.CoreWindow");
    }
    else if (propertyId == m_pUIAIds->IsEnabled_Property && ShouldFrameworkProvideWindowProperties())
    {
        pRetVal->vt = VT_BOOL;
        pRetVal->boolVal = VARIANT_TRUE;
    }
    else if (propertyId == m_pUIAIds->Name_Property && ShouldFrameworkProvideWindowProperties())
    {
        if (!DesktopUtility::IsOnDesktop())
        {
            BSTR currentDisplayName = nullptr;

            if (FAILED(GetCurrentDisplayName(&currentDisplayName)))
            {
                // We once had a fallback to phone's INavigationClientPhone here
                IFC(E_FAIL);
            }
            else
            {
                pRetVal->bstrVal = currentDisplayName;
                pRetVal->vt = VT_BSTR;
                currentDisplayName = nullptr; // relinquish ownership
            }
        }
        else
        {
            pRetVal->vt = VT_EMPTY;
        }
    }
    else
    {
        pRetVal->vt = VT_EMPTY;
    }

Cleanup:
    WindowsDeleteString(strSessionName);
    return hr;
}

//-------------------------------------------------------------------------
//
// Helper functions
//
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
//  Synopsis:
//      Retrieves the MRT-resolved (i.e., session URI -> localized app name)
//      DisplayName for the current application
//-------------------------------------------------------------------------
HRESULT GetCurrentDisplayName(_Out_ BSTR *outString)
{
    // Get the MRT-resolved value for the DisplayName

    wrl_wrappers::HString stringSelected;
    ctl::ComPtr<wa::IPackageStatics> spIpackage;

    IFC_RETURN(ctl::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_ApplicationModel_Package).Get(),
        &spIpackage));

    ctl::ComPtr<wa::IPackage> currentPackage;
    IFC_RETURN(spIpackage->get_Current(&currentPackage));

    ctl::ComPtr<wa::IPackage2> currentPackage2;
    IFC_RETURN(currentPackage.As(&currentPackage2));

    IFC_RETURN(currentPackage2->get_DisplayName(stringSelected.GetAddressOf()));

    *outString = SysAllocString(stringSelected.GetRawBuffer(nullptr));

    return S_OK;
}

namespace UIA { namespace Private {

xref_ptr<CDependencyObject> FindElementByName(
    _In_ CDependencyObject *pDO,
    _In_ const xstring_ptr_view& strName);

} }

void CUIAWindow::SetMockUIAClientsListening(bool isEnabledMockUIAClientsListening)
{
    m_isEnabledMockUIAClientsListening = isEnabledMockUIAClientsListening;
}

// This may return null when running in UWP or when called as the island is shutting down.
xref_ptr<ixp::IContentIsland> CUIAHostWindow::GetContentIsland()
{
    // For the Popup case, return the ContentIsland of the Popup.
    if (m_popupWeakRef)
    {
        if (auto popup = do_pointer_cast<CPopup>(m_popupWeakRef.lock()))
        {
            return xref_ptr<ixp::IContentIsland>{popup->GetContentIslandNoRef()};
        }
    }

    // Otherwise, the XamlIsland's ContentIsland. 
    if (auto xamlIsland = m_uiaHostEnvironmentInfo.GetXamlIslandRoot())
    {
        return xref_ptr<ixp::IContentIsland>{xamlIsland->GetContentIsland()};
    }

    // For UWP, we'll return null.
    return nullptr;
}
