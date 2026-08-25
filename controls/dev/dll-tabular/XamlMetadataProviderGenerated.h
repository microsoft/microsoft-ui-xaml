 

//////////////////////////////////////////////////////////////
/// WARNING: Generated File: Please do not modify manually ///
//////////////////////////////////////////////////////////////

#include "pch.h"
#include "common.h"
#include "TabularControlsResources.h"

namespace {

template <typename Factory>
winrt::IInspectable ActivateInstanceWithFactory(_In_ PCWSTR typeName)
{
    auto factory = GetFactory<Factory>(typeName);
    winrt::IInspectable inner;
    return factory.as<Factory>().CreateInstance(nullptr, inner);
}

template <typename Factory>
Factory GetFactory(_In_ PCWSTR typeName)
{
    winrt::IActivationFactory _activationFactory{ nullptr };
    winrt::hstring activatableClassId{ typeName };

    if (FAILED(WINRT_GetActivationFactory(winrt::get_abi(activatableClassId), winrt::put_abi(_activationFactory))))
    {
        return nullptr;
    }
    else
    {
        return _activationFactory.as<Factory>();
    }
}

winrt::IInspectable ActivateInstance(_In_ PCWSTR typeName)
{
    winrt::IActivationFactory _activationFactory{ nullptr };
    winrt::hstring activatableClassId{ typeName };
    winrt::check_hresult(WINRT_GetActivationFactory(winrt::get_abi(activatableClassId), winrt::put_abi(_activationFactory)));

    return _activationFactory.ActivateInstance<winrt::IInspectable>();
}

struct Entry
{
    hstring typeName;
    std::function<winrt::IXamlType()> createXamlTypeCallback;
    winrt::IXamlType xamlType;
};

Entry c_typeEntries[] =
{
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Private.Controls.SortIndicator",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Private.Controls.SortIndicator",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ISortIndicatorFactory>(L"Microsoft.UI.Private.Controls.SortIndicator"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ISortIndicatorStatics statics = GetFactory<winrt::ISortIndicatorStatics>(L"Microsoft.UI.Private.Controls.SortIndicator");
                    {
                        xamlType.AddDPMember(L"Direction", L"Microsoft.UI.Private.Controls.SortIndicatorDirection", statics.DirectionProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Tabular.ITableViewSortComparer",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Tabular.ITableViewSortComparer",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Tabular.SortDirection",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Tabular.SortDirection",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"None") return box_value(winrt::SortDirection::None);
                    if (fromString == L"Ascending") return box_value(winrt::SortDirection::Ascending);
                    if (fromString == L"Descending") return box_value(winrt::SortDirection::Descending);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Tabular.TableView",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Tabular.TableView",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ITableViewFactory>(L"Microsoft.UI.Xaml.Controls.Tabular.TableView"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ITableViewStatics statics = GetFactory<winrt::ITableViewStatics>(L"Microsoft.UI.Xaml.Controls.Tabular.TableView");
                    {
                        xamlType.AddDPMember(L"AlternatingRowBackground", L"Microsoft.UI.Xaml.Media.Brush", statics.AlternatingRowBackgroundProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"CanUserSortColumns", L"Boolean", statics.CanUserSortColumnsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Columns", L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn>", statics.ColumnsProperty(), true /* isContent */);
                        xamlType.AddDPMember(L"Density", L"Microsoft.UI.Xaml.Controls.Tabular.TableViewDensity", statics.DensityProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"EmptyTemplate", L"Microsoft.UI.Xaml.DataTemplate", statics.EmptyTemplateProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"GridLinesVisibility", L"Microsoft.UI.Xaml.Controls.Tabular.TableViewGridLinesVisibility", statics.GridLinesVisibilityProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"GroupHeaderTemplate", L"Microsoft.UI.Xaml.DataTemplate", statics.GroupHeaderTemplateProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"HeadersVisibility", L"Microsoft.UI.Xaml.Controls.Tabular.TableViewHeadersVisibility", statics.HeadersVisibilityProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsReadOnly", L"Boolean", statics.IsReadOnlyProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ItemsSource", L"Object", statics.ItemsSourceProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"RowBackground", L"Microsoft.UI.Xaml.Media.Brush", statics.RowBackgroundProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SelectedIndex", L"Int32", statics.SelectedIndexProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SelectedItem", L"Object", statics.SelectedItemProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SelectionMode", L"Microsoft.UI.Xaml.Controls.Tabular.TableViewSelectionMode", statics.SelectionModeProperty(), false /* isContent */);
                    }

                    xamlType.AddMember(
                        L"IsEditing", /* propertyName */
                        L"Boolean", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::TableView>().IsEditing()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Tabular.TableViewCellsPanel",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Tabular.TableViewCellsPanel",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Panel",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ITableViewCellsPanelFactory>(L"Microsoft.UI.Xaml.Controls.Tabular.TableViewCellsPanel"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ITableViewColumnFactory>(L"Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ITableViewColumnStatics statics = GetFactory<winrt::ITableViewColumnStatics>(L"Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn");
                    {
                        xamlType.AddDPMember(L"ActualWidth", L"Double", statics.ActualWidthProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"CanSort", L"Boolean", statics.CanSortProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"CellEditingTemplate", L"Microsoft.UI.Xaml.DataTemplate", statics.CellEditingTemplateProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"FrozenEdge", L"Microsoft.UI.Xaml.Controls.Tabular.TableViewFrozenEdge", statics.FrozenEdgeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Header", L"Object", statics.HeaderProperty(), true /* isContent */);
                        xamlType.AddDPMember(L"HeaderTemplate", L"Microsoft.UI.Xaml.DataTemplate", statics.HeaderTemplateProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"HeaderTemplateSelector", L"Microsoft.UI.Xaml.Controls.DataTemplateSelector", statics.HeaderTemplateSelectorProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsReadOnly", L"Boolean", statics.IsReadOnlyProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MaxWidth", L"Double", statics.MaxWidthProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MinWidth", L"Double", statics.MinWidthProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SortCycle", L"Microsoft.UI.Xaml.Controls.Tabular.TableViewSortCycle", statics.SortCycleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SortDirection", L"Microsoft.UI.Xaml.Controls.Tabular.SortDirection", statics.SortDirectionProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SortMemberPath", L"String", statics.SortMemberPathProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Visibility", L"Microsoft.UI.Xaml.Visibility", statics.VisibilityProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Width", L"Microsoft.UI.Xaml.GridLength", statics.WidthProperty(), false /* isContent */);
                    }

                    xamlType.AddMember(
                        L"CustomSortComparer", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.Tabular.ITableViewSortComparer", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::TableViewColumn>().CustomSortComparer(); },
                        [](winrt::IInspectable instance, winrt::IInspectable value) { instance.as<winrt::TableViewColumn>().CustomSortComparer(value ? value.as<winrt::Microsoft::UI::Xaml::Controls::Tabular::ITableViewSortComparer>() : nullptr); },
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Tabular.TableViewDensity",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Tabular.TableViewDensity",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Compact") return box_value(winrt::TableViewDensity::Compact);
                    if (fromString == L"Standard") return box_value(winrt::TableViewDensity::Standard);
                    if (fromString == L"Comfortable") return box_value(winrt::TableViewDensity::Comfortable);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Tabular.TableViewEditAction",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Tabular.TableViewEditAction",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Commit") return box_value(winrt::TableViewEditAction::Commit);
                    if (fromString == L"Cancel") return box_value(winrt::TableViewEditAction::Cancel);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Tabular.TableViewFrozenEdge",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Tabular.TableViewFrozenEdge",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"None") return box_value(winrt::TableViewFrozenEdge::None);
                    if (fromString == L"Leading") return box_value(winrt::TableViewFrozenEdge::Leading);
                    if (fromString == L"Trailing") return box_value(winrt::TableViewFrozenEdge::Trailing);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Tabular.TableViewGridLinesVisibility",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Tabular.TableViewGridLinesVisibility",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"All") return box_value(winrt::TableViewGridLinesVisibility::All);
                    if (fromString == L"Horizontal") return box_value(winrt::TableViewGridLinesVisibility::Horizontal);
                    if (fromString == L"None") return box_value(winrt::TableViewGridLinesVisibility::None);
                    if (fromString == L"Vertical") return box_value(winrt::TableViewGridLinesVisibility::Vertical);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Tabular.TableViewGroupHeader",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Tabular.TableViewGroupHeader",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ContentControl",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ITableViewGroupHeaderFactory>(L"Microsoft.UI.Xaml.Controls.Tabular.TableViewGroupHeader"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ITableViewGroupHeaderStatics statics = GetFactory<winrt::ITableViewGroupHeaderStatics>(L"Microsoft.UI.Xaml.Controls.Tabular.TableViewGroupHeader");
                    {
                        xamlType.AddDPMember(L"IsExpandable", L"Boolean", statics.IsExpandableProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsExpanded", L"Boolean", statics.IsExpandedProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Tabular.TableViewGroupInfo",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Tabular.TableViewGroupInfo",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"IsExpandable", /* propertyName */
                        L"Boolean", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::TableViewGroupInfo>().IsExpandable()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"IsExpanded", /* propertyName */
                        L"Boolean", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::TableViewGroupInfo>().IsExpanded()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ItemCount", /* propertyName */
                        L"Int32", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::TableViewGroupInfo>().ItemCount()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ItemCountText", /* propertyName */
                        L"String", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::TableViewGroupInfo>().ItemCountText()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"Key", /* propertyName */
                        L"Object", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::TableViewGroupInfo>().Key(); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"KeyText", /* propertyName */
                        L"String", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::TableViewGroupInfo>().KeyText()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"Level", /* propertyName */
                        L"Int32", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::TableViewGroupInfo>().Level()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Tabular.TableViewHeadersVisibility",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Tabular.TableViewHeadersVisibility",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"None") return box_value(winrt::TableViewHeadersVisibility::None);
                    if (fromString == L"Column") return box_value(winrt::TableViewHeadersVisibility::Column);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Tabular.TableViewRow",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Tabular.TableViewRow",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ITableViewRowFactory>(L"Microsoft.UI.Xaml.Controls.Tabular.TableViewRow"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ITableViewRowStatics statics = GetFactory<winrt::ITableViewRowStatics>(L"Microsoft.UI.Xaml.Controls.Tabular.TableViewRow");
                    {
                        xamlType.AddDPMember(L"IsSelected", L"Boolean", statics.IsSelectedProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Tabular.TableViewSelectionMode",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Tabular.TableViewSelectionMode",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"None") return box_value(winrt::TableViewSelectionMode::None);
                    if (fromString == L"Single") return box_value(winrt::TableViewSelectionMode::Single);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Tabular.TableViewSortCycle",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Tabular.TableViewSortCycle",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"AscendingDescending") return box_value(winrt::TableViewSortCycle::AscendingDescending);
                    if (fromString == L"AscendingDescendingNone") return box_value(winrt::TableViewSortCycle::AscendingDescendingNone);
                    if (fromString == L"DescendingAscending") return box_value(winrt::TableViewSortCycle::DescendingAscending);
                    if (fromString == L"DescendingAscendingNone") return box_value(winrt::TableViewSortCycle::DescendingAscendingNone);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Tabular.TableViewSource",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Tabular.TableViewSource",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"View", /* propertyName */
                        L"Windows.Foundation.Collections.IObservableVector`1<Object>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::TableViewSource>().View(); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Tabular.TableViewTemplateColumn",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Tabular.TableViewTemplateColumn",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ITableViewTemplateColumnFactory>(L"Microsoft.UI.Xaml.Controls.Tabular.TableViewTemplateColumn"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ITableViewTemplateColumnStatics statics = GetFactory<winrt::ITableViewTemplateColumnStatics>(L"Microsoft.UI.Xaml.Controls.Tabular.TableViewTemplateColumn");
                    {
                        xamlType.AddDPMember(L"CellTemplate", L"Microsoft.UI.Xaml.DataTemplate", statics.CellTemplateProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Tabular.TableViewTextColumn",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Tabular.TableViewTextColumn",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ITableViewTextColumnFactory>(L"Microsoft.UI.Xaml.Controls.Tabular.TableViewTextColumn"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"Binding", /* propertyName */
                        L"Microsoft.UI.Xaml.Data.Binding", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::TableViewTextColumn>().Binding(); },
                        [](winrt::IInspectable instance, winrt::IInspectable value) { instance.as<winrt::TableViewTextColumn>().Binding(value ? value.as<winrt::Microsoft::UI::Xaml::Data::Binding>() : nullptr); },
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Tabular.TabularControlsResources",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Tabular.TabularControlsResources",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.ResourceDictionary",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstance(L"Microsoft.UI.Xaml.Controls.Tabular.TabularControlsResources"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.CustomAttributes.MUXHasCustomActivationFactoryAttribute",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.CustomAttributes.MUXHasCustomActivationFactoryAttribute",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Attribute",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstance(L"Microsoft.UI.Xaml.CustomAttributes.MUXHasCustomActivationFactoryAttribute"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.CustomAttributes.MUXOverrideEnsurePropertiesAttribute",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.CustomAttributes.MUXOverrideEnsurePropertiesAttribute",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Attribute",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstance(L"Microsoft.UI.Xaml.CustomAttributes.MUXOverrideEnsurePropertiesAttribute"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.CustomAttributes.MUXPropertyChangedCallbackAttribute",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.CustomAttributes.MUXPropertyChangedCallbackAttribute",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Attribute",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstance(L"Microsoft.UI.Xaml.CustomAttributes.MUXPropertyChangedCallbackAttribute"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.CustomAttributes.MUXPropertyChangedCallbackMethodNameAttribute",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.CustomAttributes.MUXPropertyChangedCallbackMethodNameAttribute",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Attribute",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstance(L"Microsoft.UI.Xaml.CustomAttributes.MUXPropertyChangedCallbackMethodNameAttribute"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.CustomAttributes.MUXPropertyDefaultValueAttribute",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.CustomAttributes.MUXPropertyDefaultValueAttribute",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Attribute",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstance(L"Microsoft.UI.Xaml.CustomAttributes.MUXPropertyDefaultValueAttribute"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.CustomAttributes.MUXPropertyNeedsDependencyPropertyFieldAttribute",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.CustomAttributes.MUXPropertyNeedsDependencyPropertyFieldAttribute",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Attribute",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstance(L"Microsoft.UI.Xaml.CustomAttributes.MUXPropertyNeedsDependencyPropertyFieldAttribute"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.CustomAttributes.MUXPropertyTypeAttribute",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.CustomAttributes.MUXPropertyTypeAttribute",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Attribute",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstance(L"Microsoft.UI.Xaml.CustomAttributes.MUXPropertyTypeAttribute"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.CustomAttributes.MUXPropertyValidationCallbackAttribute",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.CustomAttributes.MUXPropertyValidationCallbackAttribute",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Attribute",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstance(L"Microsoft.UI.Xaml.CustomAttributes.MUXPropertyValidationCallbackAttribute"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.XamlTypeInfo.XamlControlsTabularXamlMetaDataProvider",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.XamlTypeInfo.XamlControlsTabularXamlMetaDataProvider",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstance(L"Microsoft.UI.Xaml.XamlTypeInfo.XamlControlsTabularXamlMetaDataProvider"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    // Register types encountered 
    {
        /* Arg1 TypeName */ 
        L"Attribute",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Attribute", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Boolean",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Boolean"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Double",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Double"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Int32",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Int32"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Private.Controls.SortIndicatorDirection",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Private.Controls.SortIndicatorDirection",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"None") return box_value(winrt::SortIndicatorDirection::None);
                    if (fromString == L"Ascending") return box_value(winrt::SortIndicatorDirection::Ascending);
                    if (fromString == L"Descending") return box_value(winrt::SortIndicatorDirection::Descending);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ContentControl",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Controls.ContentControl"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Control",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Controls.Control"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.DataTemplateSelector",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Controls.DataTemplateSelector"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Panel",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Controls.Panel"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Data.Binding",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Data.Binding"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.DataTemplate",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.DataTemplate"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.DependencyObject",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.DependencyObject"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.GridLength",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.GridLength"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Media.Brush",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Media.Brush"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.ResourceDictionary",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.ResourceDictionary"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Visibility",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Visibility"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Object",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Object"); }
    },
    {
        /* Arg1 TypeName */ 
        L"String",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"String"); }
    },
    {
        /* Arg1 TypeName */ 
        L"ValueType",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"ValueType", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Foundation.Collections.IObservableVector`1<Object>",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.Foundation.Collections.IObservableVector`1<Object>", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            xamlType->SetCollectionAddFunc((std::function<void(winrt::IInspectable const&, winrt::IInspectable const&)>)[](winrt::IInspectable const& collection, winrt::IInspectable const& value)
            {
                collection.as<winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable>>().Append(unbox_value<winrt::IInspectable>(value));
            });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn>",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn>", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            xamlType->SetCollectionAddFunc((std::function<void(winrt::IInspectable const&, winrt::IInspectable const&)>)[](winrt::IInspectable const& collection, winrt::IInspectable const& value)
            {
                collection.as<winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::UI::Xaml::Controls::Tabular::TableViewColumn>>().Append(unbox_value<winrt::Microsoft::UI::Xaml::Controls::Tabular::TableViewColumn>(value));
            });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
};

std::wstring_view c_knownNamespacePrefixes[] =
{
    L"Microsoft.UI.Private.",
    L"Microsoft.UI.Xaml.",
    L"Windows.Foundation.Collections.",
};
}

#include "SortIndicator.properties.h"
#include "TableView.properties.h"
#include "TableViewColumn.properties.h"
#include "TableViewGroupHeader.properties.h"
#include "TableViewRow.properties.h"
#include "TableViewTemplateColumn.properties.h"

namespace {

void ClearTypeProperties()
{
    SortIndicatorProperties::ClearProperties();
    TableViewProperties::ClearProperties();
    TableViewColumnProperties::ClearProperties();
    TableViewGroupHeaderProperties::ClearProperties();
    TableViewRowProperties::ClearProperties();
    TableViewTemplateColumnProperties::ClearProperties();
}

}
