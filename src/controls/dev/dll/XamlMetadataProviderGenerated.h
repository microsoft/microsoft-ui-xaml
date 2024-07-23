 

//////////////////////////////////////////////////////////////
/// WARNING: Generated File: Please do not modify manually ///
//////////////////////////////////////////////////////////////

#include "pch.h"
#include "common.h"
#include "XamlControlsResources.h"

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
        L"Microsoft.UI.Xaml.Controls.AnimatedIcon",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.AnimatedIcon",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.IconElement",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IAnimatedIconFactory>(L"Microsoft.UI.Xaml.Controls.AnimatedIcon"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IAnimatedIconStatics statics = GetFactory<winrt::IAnimatedIconStatics>(L"Microsoft.UI.Xaml.Controls.AnimatedIcon");
                    {
                        xamlType.AddDPMember(L"FallbackIconSource", L"Microsoft.UI.Xaml.Controls.IconSource", statics.FallbackIconSourceProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MirroredWhenRightToLeft", L"Boolean", statics.MirroredWhenRightToLeftProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Source", L"Microsoft.UI.Xaml.Controls.IAnimatedVisualSource2", statics.SourceProperty(), true /* isContent */);
                        xamlType.AddDPMember(L"State", L"String", statics.StateProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.AnimatedIconSource",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.AnimatedIconSource",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.IconSource",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IAnimatedIconSourceFactory>(L"Microsoft.UI.Xaml.Controls.AnimatedIconSource"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IAnimatedIconSourceStatics statics = GetFactory<winrt::IAnimatedIconSourceStatics>(L"Microsoft.UI.Xaml.Controls.AnimatedIconSource");
                    {
                        xamlType.AddDPMember(L"FallbackIconSource", L"Microsoft.UI.Xaml.Controls.IconSource", statics.FallbackIconSourceProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MirroredWhenRightToLeft", L"Boolean", statics.MirroredWhenRightToLeftProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Source", L"Microsoft.UI.Xaml.Controls.IAnimatedVisualSource2", statics.SourceProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.AnimatedVisualPlayer",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.AnimatedVisualPlayer",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.FrameworkElement",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IAnimatedVisualPlayerFactory>(L"Microsoft.UI.Xaml.Controls.AnimatedVisualPlayer"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IAnimatedVisualPlayerStatics statics = GetFactory<winrt::IAnimatedVisualPlayerStatics>(L"Microsoft.UI.Xaml.Controls.AnimatedVisualPlayer");
                    {
                        xamlType.AddDPMember(L"AutoPlay", L"Boolean", statics.AutoPlayProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Diagnostics", L"Object", statics.DiagnosticsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Duration", L"TimeSpan", statics.DurationProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"FallbackContent", L"Microsoft.UI.Xaml.DataTemplate", statics.FallbackContentProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsAnimatedVisualLoaded", L"Boolean", statics.IsAnimatedVisualLoadedProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsPlaying", L"Boolean", statics.IsPlayingProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PlaybackRate", L"Double", statics.PlaybackRateProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Source", L"Microsoft.UI.Xaml.Controls.IAnimatedVisualSource", statics.SourceProperty(), true /* isContent */);
                        xamlType.AddDPMember(L"Stretch", L"Microsoft.UI.Xaml.Media.Stretch", statics.StretchProperty(), false /* isContent */);
                    }

                    winrt::IAnimatedVisualPlayerStatics2 statics2 = GetFactory<winrt::IAnimatedVisualPlayerStatics2>(L"Microsoft.UI.Xaml.Controls.AnimatedVisualPlayer");
                    {
                        xamlType.AddDPMember(L"AnimationOptimization", L"Microsoft.UI.Xaml.Controls.PlayerAnimationOptimization", statics2.AnimationOptimizationProperty(), false /* isContent */);
                    }

                    xamlType.AddMember(
                        L"ProgressObject", /* propertyName */
                        L"Microsoft.UI.Composition.CompositionObject", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::AnimatedVisualPlayer>().ProgressObject(); },
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
        L"Microsoft.UI.Xaml.Controls.AnimatedVisuals.AnimatedAcceptVisualSource",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.AnimatedVisuals.AnimatedAcceptVisualSource",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstance(L"Microsoft.UI.Xaml.Controls.AnimatedVisuals.AnimatedAcceptVisualSource"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"Markers", /* propertyName */
                        L"Windows.Foundation.Collections.IReadOnlyDictionary`2<String,Double>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::AnimatedAcceptVisualSource>().Markers(); },
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
        L"Microsoft.UI.Xaml.Controls.AnimatedVisuals.AnimatedBackVisualSource",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.AnimatedVisuals.AnimatedBackVisualSource",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstance(L"Microsoft.UI.Xaml.Controls.AnimatedVisuals.AnimatedBackVisualSource"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"Markers", /* propertyName */
                        L"Windows.Foundation.Collections.IReadOnlyDictionary`2<String,Double>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::AnimatedBackVisualSource>().Markers(); },
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
        L"Microsoft.UI.Xaml.Controls.AnimatedVisuals.AnimatedChevronDownSmallVisualSource",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.AnimatedVisuals.AnimatedChevronDownSmallVisualSource",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstance(L"Microsoft.UI.Xaml.Controls.AnimatedVisuals.AnimatedChevronDownSmallVisualSource"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"Markers", /* propertyName */
                        L"Windows.Foundation.Collections.IReadOnlyDictionary`2<String,Double>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::AnimatedChevronDownSmallVisualSource>().Markers(); },
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
        L"Microsoft.UI.Xaml.Controls.AnimatedVisuals.AnimatedChevronRightDownSmallVisualSource",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.AnimatedVisuals.AnimatedChevronRightDownSmallVisualSource",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstance(L"Microsoft.UI.Xaml.Controls.AnimatedVisuals.AnimatedChevronRightDownSmallVisualSource"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"Markers", /* propertyName */
                        L"Windows.Foundation.Collections.IReadOnlyDictionary`2<String,Double>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::AnimatedChevronRightDownSmallVisualSource>().Markers(); },
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
        L"Microsoft.UI.Xaml.Controls.AnimatedVisuals.AnimatedChevronUpDownSmallVisualSource",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.AnimatedVisuals.AnimatedChevronUpDownSmallVisualSource",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstance(L"Microsoft.UI.Xaml.Controls.AnimatedVisuals.AnimatedChevronUpDownSmallVisualSource"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"Markers", /* propertyName */
                        L"Windows.Foundation.Collections.IReadOnlyDictionary`2<String,Double>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::AnimatedChevronUpDownSmallVisualSource>().Markers(); },
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
        L"Microsoft.UI.Xaml.Controls.AnimatedVisuals.AnimatedFindVisualSource",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.AnimatedVisuals.AnimatedFindVisualSource",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstance(L"Microsoft.UI.Xaml.Controls.AnimatedVisuals.AnimatedFindVisualSource"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"Markers", /* propertyName */
                        L"Windows.Foundation.Collections.IReadOnlyDictionary`2<String,Double>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::AnimatedFindVisualSource>().Markers(); },
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
        L"Microsoft.UI.Xaml.Controls.AnimatedVisuals.AnimatedGlobalNavigationButtonVisualSource",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.AnimatedVisuals.AnimatedGlobalNavigationButtonVisualSource",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstance(L"Microsoft.UI.Xaml.Controls.AnimatedVisuals.AnimatedGlobalNavigationButtonVisualSource"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"Markers", /* propertyName */
                        L"Windows.Foundation.Collections.IReadOnlyDictionary`2<String,Double>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::AnimatedGlobalNavigationButtonVisualSource>().Markers(); },
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
        L"Microsoft.UI.Xaml.Controls.AnimatedVisuals.AnimatedSettingsVisualSource",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.AnimatedVisuals.AnimatedSettingsVisualSource",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstance(L"Microsoft.UI.Xaml.Controls.AnimatedVisuals.AnimatedSettingsVisualSource"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"Markers", /* propertyName */
                        L"Windows.Foundation.Collections.IReadOnlyDictionary`2<String,Double>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::AnimatedSettingsVisualSource>().Markers(); },
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
        L"Microsoft.UI.Xaml.Controls.AnnotatedScrollBar",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.AnnotatedScrollBar",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IAnnotatedScrollBarFactory>(L"Microsoft.UI.Xaml.Controls.AnnotatedScrollBar"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IAnnotatedScrollBarStatics statics = GetFactory<winrt::IAnnotatedScrollBarStatics>(L"Microsoft.UI.Xaml.Controls.AnnotatedScrollBar");
                    {
                        xamlType.AddDPMember(L"DetailLabelTemplate", L"Microsoft.UI.Xaml.IElementFactory", statics.DetailLabelTemplateProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"LabelTemplate", L"Microsoft.UI.Xaml.IElementFactory", statics.LabelTemplateProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Labels", L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.AnnotatedScrollBarLabel>", statics.LabelsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SmallChange", L"Double", statics.SmallChangeProperty(), false /* isContent */);
                    }

                    xamlType.AddMember(
                        L"ScrollController", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.Primitives.IScrollController", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::AnnotatedScrollBar>().ScrollController(); },
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
        L"Microsoft.UI.Xaml.Controls.AnnotatedScrollBarLabel",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.AnnotatedScrollBarLabel",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"Content", /* propertyName */
                        L"Object", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::AnnotatedScrollBarLabel>().Content(); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ScrollOffset", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::AnnotatedScrollBarLabel>().ScrollOffset()); },
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
        L"Microsoft.UI.Xaml.Controls.AnnotatedScrollBarScrollingEventKind",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.AnnotatedScrollBarScrollingEventKind",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Click") return box_value(winrt::AnnotatedScrollBarScrollingEventKind::Click);
                    if (fromString == L"Drag") return box_value(winrt::AnnotatedScrollBarScrollingEventKind::Drag);
                    if (fromString == L"IncrementButton") return box_value(winrt::AnnotatedScrollBarScrollingEventKind::IncrementButton);
                    if (fromString == L"DecrementButton") return box_value(winrt::AnnotatedScrollBarScrollingEventKind::DecrementButton);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.BreadcrumbBar",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.BreadcrumbBar",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IBreadcrumbBarFactory>(L"Microsoft.UI.Xaml.Controls.BreadcrumbBar"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IBreadcrumbBarStatics statics = GetFactory<winrt::IBreadcrumbBarStatics>(L"Microsoft.UI.Xaml.Controls.BreadcrumbBar");
                    {
                        xamlType.AddDPMember(L"ItemTemplate", L"Object", statics.ItemTemplateProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ItemsSource", L"Object", statics.ItemsSourceProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.BreadcrumbBarItem",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.BreadcrumbBarItem",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ContentControl",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IBreadcrumbBarItemFactory>(L"Microsoft.UI.Xaml.Controls.BreadcrumbBarItem"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ColorPicker",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ColorPicker",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IColorPickerFactory>(L"Microsoft.UI.Xaml.Controls.ColorPicker"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IColorPickerStatics statics = GetFactory<winrt::IColorPickerStatics>(L"Microsoft.UI.Xaml.Controls.ColorPicker");
                    {
                        xamlType.AddDPMember(L"Color", L"Windows.UI.Color", statics.ColorProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ColorSpectrumComponents", L"Microsoft.UI.Xaml.Controls.ColorSpectrumComponents", statics.ColorSpectrumComponentsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ColorSpectrumShape", L"Microsoft.UI.Xaml.Controls.ColorSpectrumShape", statics.ColorSpectrumShapeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsAlphaEnabled", L"Boolean", statics.IsAlphaEnabledProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsAlphaSliderVisible", L"Boolean", statics.IsAlphaSliderVisibleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsAlphaTextInputVisible", L"Boolean", statics.IsAlphaTextInputVisibleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsColorChannelTextInputVisible", L"Boolean", statics.IsColorChannelTextInputVisibleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsColorPreviewVisible", L"Boolean", statics.IsColorPreviewVisibleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsColorSliderVisible", L"Boolean", statics.IsColorSliderVisibleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsColorSpectrumVisible", L"Boolean", statics.IsColorSpectrumVisibleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsHexInputVisible", L"Boolean", statics.IsHexInputVisibleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsMoreButtonVisible", L"Boolean", statics.IsMoreButtonVisibleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MaxHue", L"Int32", statics.MaxHueProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MaxSaturation", L"Int32", statics.MaxSaturationProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MaxValue", L"Int32", statics.MaxValueProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MinHue", L"Int32", statics.MinHueProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MinSaturation", L"Int32", statics.MinSaturationProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MinValue", L"Int32", statics.MinValueProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PreviousColor", L"Windows.Foundation.IReference`1<Windows.UI.Color>", statics.PreviousColorProperty(), false /* isContent */);
                    }

                    winrt::IColorPickerStatics2 statics2 = GetFactory<winrt::IColorPickerStatics2>(L"Microsoft.UI.Xaml.Controls.ColorPicker");
                    {
                        xamlType.AddDPMember(L"Orientation", L"Microsoft.UI.Xaml.Controls.Orientation", statics2.OrientationProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ColorPickerHsvChannel",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ColorPickerHsvChannel",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Hue") return box_value(winrt::ColorPickerHsvChannel::Hue);
                    if (fromString == L"Saturation") return box_value(winrt::ColorPickerHsvChannel::Saturation);
                    if (fromString == L"Value") return box_value(winrt::ColorPickerHsvChannel::Value);
                    if (fromString == L"Alpha") return box_value(winrt::ColorPickerHsvChannel::Alpha);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ColorSpectrumComponents",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ColorSpectrumComponents",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"HueValue") return box_value(winrt::ColorSpectrumComponents::HueValue);
                    if (fromString == L"ValueHue") return box_value(winrt::ColorSpectrumComponents::ValueHue);
                    if (fromString == L"HueSaturation") return box_value(winrt::ColorSpectrumComponents::HueSaturation);
                    if (fromString == L"SaturationHue") return box_value(winrt::ColorSpectrumComponents::SaturationHue);
                    if (fromString == L"SaturationValue") return box_value(winrt::ColorSpectrumComponents::SaturationValue);
                    if (fromString == L"ValueSaturation") return box_value(winrt::ColorSpectrumComponents::ValueSaturation);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ColorSpectrumShape",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ColorSpectrumShape",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Box") return box_value(winrt::ColorSpectrumShape::Box);
                    if (fromString == L"Ring") return box_value(winrt::ColorSpectrumShape::Ring);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.CommandBarFlyout",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.CommandBarFlyout",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ICommandBarFlyoutFactory>(L"Microsoft.UI.Xaml.Controls.CommandBarFlyout"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"PrimaryCommands", /* propertyName */
                        L"Windows.Foundation.Collections.IObservableVector`1<Microsoft.UI.Xaml.Controls.ICommandBarElement>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::CommandBarFlyout>().PrimaryCommands(); },
                        nullptr, /* setter */
                        true, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"SecondaryCommands", /* propertyName */
                        L"Windows.Foundation.Collections.IObservableVector`1<Microsoft.UI.Xaml.Controls.ICommandBarElement>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::CommandBarFlyout>().SecondaryCommands(); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"AlwaysExpanded", /* propertyName */
                        L"Boolean", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::CommandBarFlyout>().AlwaysExpanded()); },
                        [](winrt::IInspectable instance, winrt::IInspectable value) { instance.as<winrt::CommandBarFlyout>().AlwaysExpanded(unbox_value<bool>(value)); },
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.DropDownButton",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.DropDownButton",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Button",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IDropDownButtonFactory>(L"Microsoft.UI.Xaml.Controls.DropDownButton"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ElementFactory",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ElementFactory",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IElementFactoryFactory>(L"Microsoft.UI.Xaml.Controls.ElementFactory"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ElementRealizationOptions",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ElementRealizationOptions",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"None") return box_value(winrt::ElementRealizationOptions::None);
                    if (fromString == L"ForceCreate") return box_value(winrt::ElementRealizationOptions::ForceCreate);
                    if (fromString == L"SuppressAutoRecycle") return box_value(winrt::ElementRealizationOptions::SuppressAutoRecycle);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ExpandDirection",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ExpandDirection",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Down") return box_value(winrt::ExpandDirection::Down);
                    if (fromString == L"Up") return box_value(winrt::ExpandDirection::Up);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Expander",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Expander",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ContentControl",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IExpanderFactory>(L"Microsoft.UI.Xaml.Controls.Expander"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IExpanderStatics statics = GetFactory<winrt::IExpanderStatics>(L"Microsoft.UI.Xaml.Controls.Expander");
                    {
                        xamlType.AddDPMember(L"ExpandDirection", L"Microsoft.UI.Xaml.Controls.ExpandDirection", statics.ExpandDirectionProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Header", L"Object", statics.HeaderProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"HeaderTemplate", L"Microsoft.UI.Xaml.DataTemplate", statics.HeaderTemplateProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"HeaderTemplateSelector", L"Microsoft.UI.Xaml.Controls.DataTemplateSelector", statics.HeaderTemplateSelectorProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsExpanded", L"Boolean", statics.IsExpandedProperty(), false /* isContent */);
                    }

                    xamlType.AddMember(
                        L"TemplateSettings", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.ExpanderTemplateSettings", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::Expander>().TemplateSettings(); },
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
        L"Microsoft.UI.Xaml.Controls.ExpanderTemplateSettings",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ExpanderTemplateSettings",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"ContentHeight", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ExpanderTemplateSettings>().ContentHeight()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"NegativeContentHeight", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ExpanderTemplateSettings>().NegativeContentHeight()); },
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
        L"Microsoft.UI.Xaml.Controls.Layout",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Layout",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"IndexBasedLayoutOrientation", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.IndexBasedLayoutOrientation", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value<int>((int)instance.as<winrt::Layout>().IndexBasedLayoutOrientation()); },
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
        L"Microsoft.UI.Xaml.Controls.VirtualizingLayout",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.VirtualizingLayout",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Layout",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IVirtualizingLayoutFactory>(L"Microsoft.UI.Xaml.Controls.VirtualizingLayout"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.FlowLayout",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.FlowLayout",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.VirtualizingLayout",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IFlowLayoutFactory>(L"Microsoft.UI.Xaml.Controls.FlowLayout"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IFlowLayoutStatics statics = GetFactory<winrt::IFlowLayoutStatics>(L"Microsoft.UI.Xaml.Controls.FlowLayout");
                    {
                        xamlType.AddDPMember(L"LineAlignment", L"Microsoft.UI.Xaml.Controls.FlowLayoutLineAlignment", statics.LineAlignmentProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MinColumnSpacing", L"Double", statics.MinColumnSpacingProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MinRowSpacing", L"Double", statics.MinRowSpacingProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Orientation", L"Microsoft.UI.Xaml.Controls.Orientation", statics.OrientationProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.FlowLayoutAnchorInfo",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.FlowLayoutAnchorInfo",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"ValueType",
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
        L"Microsoft.UI.Xaml.Controls.FlowLayoutLineAlignment",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.FlowLayoutLineAlignment",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Start") return box_value(winrt::FlowLayoutLineAlignment::Start);
                    if (fromString == L"Center") return box_value(winrt::FlowLayoutLineAlignment::Center);
                    if (fromString == L"End") return box_value(winrt::FlowLayoutLineAlignment::End);
                    if (fromString == L"SpaceAround") return box_value(winrt::FlowLayoutLineAlignment::SpaceAround);
                    if (fromString == L"SpaceBetween") return box_value(winrt::FlowLayoutLineAlignment::SpaceBetween);
                    if (fromString == L"SpaceEvenly") return box_value(winrt::FlowLayoutLineAlignment::SpaceEvenly);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.FlowLayoutState",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.FlowLayoutState",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IFlowLayoutStateFactory>(L"Microsoft.UI.Xaml.Controls.FlowLayoutState"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.IAnimatedVisual",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.IAnimatedVisual",
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
        L"Microsoft.UI.Xaml.Controls.IAnimatedVisual2",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.IAnimatedVisual2",
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
        L"Microsoft.UI.Xaml.Controls.IAnimatedVisualSource",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.IAnimatedVisualSource",
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
        L"Microsoft.UI.Xaml.Controls.IAnimatedVisualSource2",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.IAnimatedVisualSource2",
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
        L"Microsoft.UI.Xaml.Controls.IAnimatedVisualSource3",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.IAnimatedVisualSource3",
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
        L"Microsoft.UI.Xaml.Controls.IApplicationViewSpanningRects",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.IApplicationViewSpanningRects",
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
        L"Microsoft.UI.Xaml.Controls.IDynamicAnimatedVisualSource",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.IDynamicAnimatedVisualSource",
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
        L"Microsoft.UI.Xaml.Controls.IKeyIndexMapping",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.IKeyIndexMapping",
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
        L"Microsoft.UI.Xaml.Controls.ISelfPlayingAnimatedVisual",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ISelfPlayingAnimatedVisual",
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
        L"Microsoft.UI.Xaml.Controls.ImageIcon",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ImageIcon",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.IconElement",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IImageIconFactory>(L"Microsoft.UI.Xaml.Controls.ImageIcon"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IImageIconStatics statics = GetFactory<winrt::IImageIconStatics>(L"Microsoft.UI.Xaml.Controls.ImageIcon");
                    {
                        xamlType.AddDPMember(L"Source", L"Microsoft.UI.Xaml.Media.ImageSource", statics.SourceProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ImageIconSource",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ImageIconSource",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.IconSource",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IImageIconSourceFactory>(L"Microsoft.UI.Xaml.Controls.ImageIconSource"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IImageIconSourceStatics statics = GetFactory<winrt::IImageIconSourceStatics>(L"Microsoft.UI.Xaml.Controls.ImageIconSource");
                    {
                        xamlType.AddDPMember(L"ImageSource", L"Microsoft.UI.Xaml.Media.ImageSource", statics.ImageSourceProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.IndexBasedLayoutOrientation",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.IndexBasedLayoutOrientation",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"None") return box_value(winrt::IndexBasedLayoutOrientation::None);
                    if (fromString == L"TopToBottom") return box_value(winrt::IndexBasedLayoutOrientation::TopToBottom);
                    if (fromString == L"LeftToRight") return box_value(winrt::IndexBasedLayoutOrientation::LeftToRight);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.IndexPath",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.IndexPath",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
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
        L"Microsoft.UI.Xaml.Controls.InfoBadge",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.InfoBadge",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IInfoBadgeFactory>(L"Microsoft.UI.Xaml.Controls.InfoBadge"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IInfoBadgeStatics statics = GetFactory<winrt::IInfoBadgeStatics>(L"Microsoft.UI.Xaml.Controls.InfoBadge");
                    {
                        xamlType.AddDPMember(L"IconSource", L"Microsoft.UI.Xaml.Controls.IconSource", statics.IconSourceProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TemplateSettings", L"Microsoft.UI.Xaml.Controls.InfoBadgeTemplateSettings", statics.TemplateSettingsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Value", L"Int32", statics.ValueProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.InfoBadgeTemplateSettings",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.InfoBadgeTemplateSettings",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IInfoBadgeTemplateSettingsFactory>(L"Microsoft.UI.Xaml.Controls.InfoBadgeTemplateSettings"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IInfoBadgeTemplateSettingsStatics statics = GetFactory<winrt::IInfoBadgeTemplateSettingsStatics>(L"Microsoft.UI.Xaml.Controls.InfoBadgeTemplateSettings");
                    {
                        xamlType.AddDPMember(L"IconElement", L"Microsoft.UI.Xaml.Controls.IconElement", statics.IconElementProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"InfoBadgeCornerRadius", L"Microsoft.UI.Xaml.CornerRadius", statics.InfoBadgeCornerRadiusProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.InfoBar",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.InfoBar",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IInfoBarFactory>(L"Microsoft.UI.Xaml.Controls.InfoBar"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IInfoBarStatics statics = GetFactory<winrt::IInfoBarStatics>(L"Microsoft.UI.Xaml.Controls.InfoBar");
                    {
                        xamlType.AddDPMember(L"ActionButton", L"Microsoft.UI.Xaml.Controls.Primitives.ButtonBase", statics.ActionButtonProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"CloseButtonCommandParameter", L"Object", statics.CloseButtonCommandParameterProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"CloseButtonCommand", L"Microsoft.UI.Xaml.Input.ICommand", statics.CloseButtonCommandProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"CloseButtonStyle", L"Microsoft.UI.Xaml.Style", statics.CloseButtonStyleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Content", L"Object", statics.ContentProperty(), true /* isContent */);
                        xamlType.AddDPMember(L"ContentTemplate", L"Microsoft.UI.Xaml.DataTemplate", statics.ContentTemplateProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IconSource", L"Microsoft.UI.Xaml.Controls.IconSource", statics.IconSourceProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsClosable", L"Boolean", statics.IsClosableProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsIconVisible", L"Boolean", statics.IsIconVisibleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsOpen", L"Boolean", statics.IsOpenProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Message", L"String", statics.MessageProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Severity", L"Microsoft.UI.Xaml.Controls.InfoBarSeverity", statics.SeverityProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TemplateSettings", L"Microsoft.UI.Xaml.Controls.InfoBarTemplateSettings", statics.TemplateSettingsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Title", L"String", statics.TitleProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.InfoBarCloseReason",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.InfoBarCloseReason",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"CloseButton") return box_value(winrt::InfoBarCloseReason::CloseButton);
                    if (fromString == L"Programmatic") return box_value(winrt::InfoBarCloseReason::Programmatic);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.InfoBarSeverity",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.InfoBarSeverity",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Informational") return box_value(winrt::InfoBarSeverity::Informational);
                    if (fromString == L"Success") return box_value(winrt::InfoBarSeverity::Success);
                    if (fromString == L"Warning") return box_value(winrt::InfoBarSeverity::Warning);
                    if (fromString == L"Error") return box_value(winrt::InfoBarSeverity::Error);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.InfoBarTemplateSettings",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.InfoBarTemplateSettings",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IInfoBarTemplateSettingsFactory>(L"Microsoft.UI.Xaml.Controls.InfoBarTemplateSettings"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IInfoBarTemplateSettingsStatics statics = GetFactory<winrt::IInfoBarTemplateSettingsStatics>(L"Microsoft.UI.Xaml.Controls.InfoBarTemplateSettings");
                    {
                        xamlType.AddDPMember(L"IconElement", L"Microsoft.UI.Xaml.Controls.IconElement", statics.IconElementProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ItemCollectionTransition",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ItemCollectionTransition",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"HasStarted", /* propertyName */
                        L"Boolean", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ItemCollectionTransition>().HasStarted()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"NewBounds", /* propertyName */
                        L"Windows.Foundation.Rect", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ItemCollectionTransition>().NewBounds()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"OldBounds", /* propertyName */
                        L"Windows.Foundation.Rect", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ItemCollectionTransition>().OldBounds()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"Operation", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.ItemCollectionTransitionOperation", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value<int>((int)instance.as<winrt::ItemCollectionTransition>().Operation()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"Triggers", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.ItemCollectionTransitionTriggers", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value<int>((int)instance.as<winrt::ItemCollectionTransition>().Triggers()); },
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
        L"Microsoft.UI.Xaml.Controls.ItemCollectionTransitionOperation",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ItemCollectionTransitionOperation",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Add") return box_value(winrt::ItemCollectionTransitionOperation::Add);
                    if (fromString == L"Remove") return box_value(winrt::ItemCollectionTransitionOperation::Remove);
                    if (fromString == L"Move") return box_value(winrt::ItemCollectionTransitionOperation::Move);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ItemCollectionTransitionProgress",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ItemCollectionTransitionProgress",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"Element", /* propertyName */
                        L"Microsoft.UI.Xaml.UIElement", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::ItemCollectionTransitionProgress>().Element(); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"Transition", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.ItemCollectionTransition", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::ItemCollectionTransitionProgress>().Transition(); },
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
        L"Microsoft.UI.Xaml.Controls.ItemCollectionTransitionProvider",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ItemCollectionTransitionProvider",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IItemCollectionTransitionProviderFactory>(L"Microsoft.UI.Xaml.Controls.ItemCollectionTransitionProvider"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ItemCollectionTransitionTriggers",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ItemCollectionTransitionTriggers",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"CollectionChangeAdd") return box_value(winrt::ItemCollectionTransitionTriggers::CollectionChangeAdd);
                    if (fromString == L"CollectionChangeRemove") return box_value(winrt::ItemCollectionTransitionTriggers::CollectionChangeRemove);
                    if (fromString == L"CollectionChangeReset") return box_value(winrt::ItemCollectionTransitionTriggers::CollectionChangeReset);
                    if (fromString == L"LayoutTransition") return box_value(winrt::ItemCollectionTransitionTriggers::LayoutTransition);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ItemContainer",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ItemContainer",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IItemContainerFactory>(L"Microsoft.UI.Xaml.Controls.ItemContainer"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IItemContainerStatics statics = GetFactory<winrt::IItemContainerStatics>(L"Microsoft.UI.Xaml.Controls.ItemContainer");
                    {
                        xamlType.AddDPMember(L"Child", L"Microsoft.UI.Xaml.UIElement", statics.ChildProperty(), true /* isContent */);
                        xamlType.AddDPMember(L"IsSelected", L"Boolean", statics.IsSelectedProperty(), false /* isContent */);
                    }

                    winrt::IItemContainerStatics2 statics2 = GetFactory<winrt::IItemContainerStatics2>(L"Microsoft.UI.Xaml.Controls.ItemContainer");
                    {
                        xamlType.AddDPMember(L"CanUserInvoke", L"Microsoft.UI.Xaml.Controls.ItemContainerUserInvokeMode", statics2.CanUserInvokeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"CanUserSelect", L"Microsoft.UI.Xaml.Controls.ItemContainerUserSelectMode", statics2.CanUserSelectProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MultiSelectMode", L"Microsoft.UI.Xaml.Controls.ItemContainerMultiSelectMode", statics2.MultiSelectModeProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ItemContainerInteractionTrigger",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ItemContainerInteractionTrigger",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"PointerPressed") return box_value(winrt::ItemContainerInteractionTrigger::PointerPressed);
                    if (fromString == L"PointerReleased") return box_value(winrt::ItemContainerInteractionTrigger::PointerReleased);
                    if (fromString == L"Tap") return box_value(winrt::ItemContainerInteractionTrigger::Tap);
                    if (fromString == L"DoubleTap") return box_value(winrt::ItemContainerInteractionTrigger::DoubleTap);
                    if (fromString == L"EnterKey") return box_value(winrt::ItemContainerInteractionTrigger::EnterKey);
                    if (fromString == L"SpaceKey") return box_value(winrt::ItemContainerInteractionTrigger::SpaceKey);
                    if (fromString == L"AutomationInvoke") return box_value(winrt::ItemContainerInteractionTrigger::AutomationInvoke);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ItemContainerMultiSelectMode",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ItemContainerMultiSelectMode",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Auto") return box_value(winrt::ItemContainerMultiSelectMode::Auto);
                    if (fromString == L"Single") return box_value(winrt::ItemContainerMultiSelectMode::Single);
                    if (fromString == L"Extended") return box_value(winrt::ItemContainerMultiSelectMode::Extended);
                    if (fromString == L"Multiple") return box_value(winrt::ItemContainerMultiSelectMode::Multiple);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ItemContainerUserInvokeMode",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ItemContainerUserInvokeMode",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Auto") return box_value(winrt::ItemContainerUserInvokeMode::Auto);
                    if (fromString == L"UserCanInvoke") return box_value(winrt::ItemContainerUserInvokeMode::UserCanInvoke);
                    if (fromString == L"UserCannotInvoke") return box_value(winrt::ItemContainerUserInvokeMode::UserCannotInvoke);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ItemContainerUserSelectMode",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ItemContainerUserSelectMode",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Auto") return box_value(winrt::ItemContainerUserSelectMode::Auto);
                    if (fromString == L"UserCanSelect") return box_value(winrt::ItemContainerUserSelectMode::UserCanSelect);
                    if (fromString == L"UserCannotSelect") return box_value(winrt::ItemContainerUserSelectMode::UserCannotSelect);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ItemsRepeater",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ItemsRepeater",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.FrameworkElement",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IItemsRepeaterFactory>(L"Microsoft.UI.Xaml.Controls.ItemsRepeater"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IItemsRepeaterStatics statics = GetFactory<winrt::IItemsRepeaterStatics>(L"Microsoft.UI.Xaml.Controls.ItemsRepeater");
                    {
                        xamlType.AddDPMember(L"Background", L"Microsoft.UI.Xaml.Media.Brush", statics.BackgroundProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"HorizontalCacheLength", L"Double", statics.HorizontalCacheLengthProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ItemTemplate", L"Object", statics.ItemTemplateProperty(), true /* isContent */);
                        xamlType.AddDPMember(L"ItemsSource", L"Object", statics.ItemsSourceProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Layout", L"Microsoft.UI.Xaml.Controls.Layout", statics.LayoutProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"VerticalCacheLength", L"Double", statics.VerticalCacheLengthProperty(), false /* isContent */);
                    }

                    winrt::IItemsRepeaterStatics2 statics2 = GetFactory<winrt::IItemsRepeaterStatics2>(L"Microsoft.UI.Xaml.Controls.ItemsRepeater");
                    {
                        xamlType.AddDPMember(L"ItemTransitionProvider", L"Microsoft.UI.Xaml.Controls.ItemCollectionTransitionProvider", statics2.ItemTransitionProviderProperty(), false /* isContent */);
                    }

                    xamlType.AddMember(
                        L"ItemsSourceView", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.ItemsSourceView", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::ItemsRepeater>().ItemsSourceView(); },
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
        L"Microsoft.UI.Xaml.Controls.ItemsRepeaterScrollHost",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ItemsRepeaterScrollHost",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.FrameworkElement",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstance(L"Microsoft.UI.Xaml.Controls.ItemsRepeaterScrollHost"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"VerticalAnchorRatio", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ItemsRepeaterScrollHost>().VerticalAnchorRatio()); },
                        [](winrt::IInspectable instance, winrt::IInspectable value) { instance.as<winrt::ItemsRepeaterScrollHost>().VerticalAnchorRatio(unbox_value<double>(value)); },
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ScrollViewer", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.ScrollViewer", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::ItemsRepeaterScrollHost>().ScrollViewer(); },
                        [](winrt::IInspectable instance, winrt::IInspectable value) { instance.as<winrt::ItemsRepeaterScrollHost>().ScrollViewer(value.as<winrt::Microsoft::UI::Xaml::Controls::ScrollViewer>()); },
                        true, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"HorizontalAnchorRatio", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ItemsRepeaterScrollHost>().HorizontalAnchorRatio()); },
                        [](winrt::IInspectable instance, winrt::IInspectable value) { instance.as<winrt::ItemsRepeaterScrollHost>().HorizontalAnchorRatio(unbox_value<double>(value)); },
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"CurrentAnchor", /* propertyName */
                        L"Microsoft.UI.Xaml.UIElement", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::ItemsRepeaterScrollHost>().CurrentAnchor(); },
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
        L"Microsoft.UI.Xaml.Controls.ItemsSourceView",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ItemsSourceView",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"Count", /* propertyName */
                        L"Int32", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ItemsSourceView>().Count()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"HasKeyIndexMapping", /* propertyName */
                        L"Boolean", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ItemsSourceView>().HasKeyIndexMapping()); },
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
        L"Microsoft.UI.Xaml.Controls.ItemsView",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ItemsView",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IItemsViewFactory>(L"Microsoft.UI.Xaml.Controls.ItemsView"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IItemsViewStatics statics = GetFactory<winrt::IItemsViewStatics>(L"Microsoft.UI.Xaml.Controls.ItemsView");
                    {
                        xamlType.AddDPMember(L"CurrentItemIndex", L"Int32", statics.CurrentItemIndexProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsItemInvokedEnabled", L"Boolean", statics.IsItemInvokedEnabledProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ItemTemplate", L"Microsoft.UI.Xaml.IElementFactory", statics.ItemTemplateProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ItemTransitionProvider", L"Microsoft.UI.Xaml.Controls.ItemCollectionTransitionProvider", statics.ItemTransitionProviderProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ItemsSource", L"Object", statics.ItemsSourceProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Layout", L"Microsoft.UI.Xaml.Controls.Layout", statics.LayoutProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ScrollView", L"Microsoft.UI.Xaml.Controls.ScrollView", statics.ScrollViewProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SelectedItem", L"Object", statics.SelectedItemProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SelectionMode", L"Microsoft.UI.Xaml.Controls.ItemsViewSelectionMode", statics.SelectionModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"VerticalScrollController", L"Microsoft.UI.Xaml.Controls.Primitives.IScrollController", statics.VerticalScrollControllerProperty(), false /* isContent */);
                    }

                    xamlType.AddMember(
                        L"SelectedItems", /* propertyName */
                        L"Windows.Foundation.Collections.IReadOnlyList`1<Object>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::ItemsView>().SelectedItems(); },
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
        L"Microsoft.UI.Xaml.Controls.ItemsViewSelectionMode",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ItemsViewSelectionMode",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"None") return box_value(winrt::ItemsViewSelectionMode::None);
                    if (fromString == L"Single") return box_value(winrt::ItemsViewSelectionMode::Single);
                    if (fromString == L"Multiple") return box_value(winrt::ItemsViewSelectionMode::Multiple);
                    if (fromString == L"Extended") return box_value(winrt::ItemsViewSelectionMode::Extended);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.LayoutContext",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.LayoutContext",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"LayoutState", /* propertyName */
                        L"Object", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::LayoutContext>().LayoutState(); },
                        [](winrt::IInspectable instance, winrt::IInspectable value) { instance.as<winrt::LayoutContext>().LayoutState(value); },
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.LayoutPanel",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.LayoutPanel",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Panel",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ILayoutPanelFactory>(L"Microsoft.UI.Xaml.Controls.LayoutPanel"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ILayoutPanelStatics statics = GetFactory<winrt::ILayoutPanelStatics>(L"Microsoft.UI.Xaml.Controls.LayoutPanel");
                    {
                        xamlType.AddDPMember(L"BorderBrush", L"Microsoft.UI.Xaml.Media.Brush", statics.BorderBrushProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"BorderThickness", L"Microsoft.UI.Xaml.Thickness", statics.BorderThicknessProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"CornerRadius", L"Microsoft.UI.Xaml.CornerRadius", statics.CornerRadiusProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Layout", L"Microsoft.UI.Xaml.Controls.Layout", statics.LayoutProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Padding", L"Microsoft.UI.Xaml.Thickness", statics.PaddingProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.LinedFlowLayout",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.LinedFlowLayout",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.VirtualizingLayout",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ILinedFlowLayoutFactory>(L"Microsoft.UI.Xaml.Controls.LinedFlowLayout"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ILinedFlowLayoutStatics statics = GetFactory<winrt::ILinedFlowLayoutStatics>(L"Microsoft.UI.Xaml.Controls.LinedFlowLayout");
                    {
                        xamlType.AddDPMember(L"ActualLineHeight", L"Double", statics.ActualLineHeightProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ItemsJustification", L"Microsoft.UI.Xaml.Controls.LinedFlowLayoutItemsJustification", statics.ItemsJustificationProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ItemsStretch", L"Microsoft.UI.Xaml.Controls.LinedFlowLayoutItemsStretch", statics.ItemsStretchProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"LineHeight", L"Double", statics.LineHeightProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"LineSpacing", L"Double", statics.LineSpacingProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MinItemSpacing", L"Double", statics.MinItemSpacingProperty(), false /* isContent */);
                    }

                    xamlType.AddMember(
                        L"RequestedRangeLength", /* propertyName */
                        L"Int32", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::LinedFlowLayout>().RequestedRangeLength()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"RequestedRangeStartIndex", /* propertyName */
                        L"Int32", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::LinedFlowLayout>().RequestedRangeStartIndex()); },
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
        L"Microsoft.UI.Xaml.Controls.LinedFlowLayoutItemCollectionTransitionProvider",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.LinedFlowLayoutItemCollectionTransitionProvider",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ItemCollectionTransitionProvider",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ILinedFlowLayoutItemCollectionTransitionProviderFactory>(L"Microsoft.UI.Xaml.Controls.LinedFlowLayoutItemCollectionTransitionProvider"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.LinedFlowLayoutItemsJustification",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.LinedFlowLayoutItemsJustification",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Start") return box_value(winrt::LinedFlowLayoutItemsJustification::Start);
                    if (fromString == L"Center") return box_value(winrt::LinedFlowLayoutItemsJustification::Center);
                    if (fromString == L"End") return box_value(winrt::LinedFlowLayoutItemsJustification::End);
                    if (fromString == L"SpaceAround") return box_value(winrt::LinedFlowLayoutItemsJustification::SpaceAround);
                    if (fromString == L"SpaceBetween") return box_value(winrt::LinedFlowLayoutItemsJustification::SpaceBetween);
                    if (fromString == L"SpaceEvenly") return box_value(winrt::LinedFlowLayoutItemsJustification::SpaceEvenly);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.LinedFlowLayoutItemsStretch",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.LinedFlowLayoutItemsStretch",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"None") return box_value(winrt::LinedFlowLayoutItemsStretch::None);
                    if (fromString == L"Fill") return box_value(winrt::LinedFlowLayoutItemsStretch::Fill);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.MapControl",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.MapControl",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IMapControlFactory>(L"Microsoft.UI.Xaml.Controls.MapControl"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IMapControlStatics statics = GetFactory<winrt::IMapControlStatics>(L"Microsoft.UI.Xaml.Controls.MapControl");
                    {
                        xamlType.AddDPMember(L"Center", L"Windows.Devices.Geolocation.Geopoint", statics.CenterProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"InteractiveControlsVisible", L"Boolean", statics.InteractiveControlsVisibleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Layers", L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.MapLayer>", statics.LayersProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MapServiceToken", L"String", statics.MapServiceTokenProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ZoomLevel", L"Double", statics.ZoomLevelProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.MapElement",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.MapElement",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
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
        L"Microsoft.UI.Xaml.Controls.MapLayer",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.MapLayer",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
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
        L"Microsoft.UI.Xaml.Controls.MapElementsLayer",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.MapElementsLayer",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.MapLayer",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IMapElementsLayerFactory>(L"Microsoft.UI.Xaml.Controls.MapElementsLayer"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IMapElementsLayerStatics statics = GetFactory<winrt::IMapElementsLayerStatics>(L"Microsoft.UI.Xaml.Controls.MapElementsLayer");
                    {
                        xamlType.AddDPMember(L"MapElements", L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.MapElement>", statics.MapElementsProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.MapIcon",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.MapIcon",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.MapElement",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IMapIconFactory>(L"Microsoft.UI.Xaml.Controls.MapIcon"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IMapIconStatics statics = GetFactory<winrt::IMapIconStatics>(L"Microsoft.UI.Xaml.Controls.MapIcon");
                    {
                        xamlType.AddDPMember(L"Location", L"Windows.Devices.Geolocation.Geopoint", statics.LocationProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.MenuBar",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.MenuBar",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IMenuBarFactory>(L"Microsoft.UI.Xaml.Controls.MenuBar"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IMenuBarStatics statics = GetFactory<winrt::IMenuBarStatics>(L"Microsoft.UI.Xaml.Controls.MenuBar");
                    {
                        xamlType.AddDPMember(L"Items", L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.MenuBarItem>", statics.ItemsProperty(), true /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.MenuBarItem",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.MenuBarItem",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IMenuBarItemFactory>(L"Microsoft.UI.Xaml.Controls.MenuBarItem"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IMenuBarItemStatics statics = GetFactory<winrt::IMenuBarItemStatics>(L"Microsoft.UI.Xaml.Controls.MenuBarItem");
                    {
                        xamlType.AddDPMember(L"Items", L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.MenuFlyoutItemBase>", statics.ItemsProperty(), true /* isContent */);
                        xamlType.AddDPMember(L"Title", L"String", statics.TitleProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.MenuBarItemFlyout",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.MenuBarItemFlyout",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.MenuFlyout",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IMenuBarItemFlyoutFactory>(L"Microsoft.UI.Xaml.Controls.MenuBarItemFlyout"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.NavigationView",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.NavigationView",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ContentControl",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::INavigationViewFactory>(L"Microsoft.UI.Xaml.Controls.NavigationView"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::INavigationViewStatics statics = GetFactory<winrt::INavigationViewStatics>(L"Microsoft.UI.Xaml.Controls.NavigationView");
                    {
                        xamlType.AddDPMember(L"AlwaysShowHeader", L"Boolean", statics.AlwaysShowHeaderProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"AutoSuggestBox", L"Microsoft.UI.Xaml.Controls.AutoSuggestBox", statics.AutoSuggestBoxProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"CompactModeThresholdWidth", L"Double", statics.CompactModeThresholdWidthProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"CompactPaneLength", L"Double", statics.CompactPaneLengthProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"DisplayMode", L"Microsoft.UI.Xaml.Controls.NavigationViewDisplayMode", statics.DisplayModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ExpandedModeThresholdWidth", L"Double", statics.ExpandedModeThresholdWidthProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"FooterMenuItems", L"Windows.Foundation.Collections.IVector`1<Object>", statics.FooterMenuItemsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"FooterMenuItemsSource", L"Object", statics.FooterMenuItemsSourceProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Header", L"Object", statics.HeaderProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"HeaderTemplate", L"Microsoft.UI.Xaml.DataTemplate", statics.HeaderTemplateProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsPaneOpen", L"Boolean", statics.IsPaneOpenProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsPaneToggleButtonVisible", L"Boolean", statics.IsPaneToggleButtonVisibleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsSettingsVisible", L"Boolean", statics.IsSettingsVisibleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsTitleBarAutoPaddingEnabled", L"Boolean", statics.IsTitleBarAutoPaddingEnabledProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MenuItemContainerStyle", L"Microsoft.UI.Xaml.Style", statics.MenuItemContainerStyleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MenuItemContainerStyleSelector", L"Microsoft.UI.Xaml.Controls.StyleSelector", statics.MenuItemContainerStyleSelectorProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MenuItemTemplate", L"Microsoft.UI.Xaml.DataTemplate", statics.MenuItemTemplateProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MenuItemTemplateSelector", L"Microsoft.UI.Xaml.Controls.DataTemplateSelector", statics.MenuItemTemplateSelectorProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MenuItems", L"Windows.Foundation.Collections.IVector`1<Object>", statics.MenuItemsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MenuItemsSource", L"Object", statics.MenuItemsSourceProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"OpenPaneLength", L"Double", statics.OpenPaneLengthProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PaneFooter", L"Microsoft.UI.Xaml.UIElement", statics.PaneFooterProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PaneToggleButtonStyle", L"Microsoft.UI.Xaml.Style", statics.PaneToggleButtonStyleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SelectedItem", L"Object", statics.SelectedItemProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SettingsItem", L"Object", statics.SettingsItemProperty(), false /* isContent */);
                    }

                    winrt::INavigationViewStatics2 statics2 = GetFactory<winrt::INavigationViewStatics2>(L"Microsoft.UI.Xaml.Controls.NavigationView");
                    {
                        xamlType.AddDPMember(L"ContentOverlay", L"Microsoft.UI.Xaml.UIElement", statics2.ContentOverlayProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsBackButtonVisible", L"Microsoft.UI.Xaml.Controls.NavigationViewBackButtonVisible", statics2.IsBackButtonVisibleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsBackEnabled", L"Boolean", statics2.IsBackEnabledProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsPaneVisible", L"Boolean", statics2.IsPaneVisibleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"OverflowLabelMode", L"Microsoft.UI.Xaml.Controls.NavigationViewOverflowLabelMode", statics2.OverflowLabelModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PaneCustomContent", L"Microsoft.UI.Xaml.UIElement", statics2.PaneCustomContentProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PaneDisplayMode", L"Microsoft.UI.Xaml.Controls.NavigationViewPaneDisplayMode", statics2.PaneDisplayModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PaneHeader", L"Microsoft.UI.Xaml.UIElement", statics2.PaneHeaderProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PaneTitle", L"String", statics2.PaneTitleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SelectionFollowsFocus", L"Microsoft.UI.Xaml.Controls.NavigationViewSelectionFollowsFocus", statics2.SelectionFollowsFocusProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ShoulderNavigationEnabled", L"Microsoft.UI.Xaml.Controls.NavigationViewShoulderNavigationEnabled", statics2.ShoulderNavigationEnabledProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TemplateSettings", L"Microsoft.UI.Xaml.Controls.NavigationViewTemplateSettings", statics2.TemplateSettingsProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.NavigationViewBackButtonVisible",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.NavigationViewBackButtonVisible",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Collapsed") return box_value(winrt::NavigationViewBackButtonVisible::Collapsed);
                    if (fromString == L"Visible") return box_value(winrt::NavigationViewBackButtonVisible::Visible);
                    if (fromString == L"Auto") return box_value(winrt::NavigationViewBackButtonVisible::Auto);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.NavigationViewDisplayMode",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.NavigationViewDisplayMode",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Minimal") return box_value(winrt::NavigationViewDisplayMode::Minimal);
                    if (fromString == L"Compact") return box_value(winrt::NavigationViewDisplayMode::Compact);
                    if (fromString == L"Expanded") return box_value(winrt::NavigationViewDisplayMode::Expanded);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.NavigationViewItemBase",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.NavigationViewItemBase",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ContentControl",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::INavigationViewItemBaseStatics statics = GetFactory<winrt::INavigationViewItemBaseStatics>(L"Microsoft.UI.Xaml.Controls.NavigationViewItemBase");
                    {
                        xamlType.AddDPMember(L"IsSelected", L"Boolean", statics.IsSelectedProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.NavigationViewItem",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.NavigationViewItem",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.NavigationViewItemBase",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::INavigationViewItemFactory>(L"Microsoft.UI.Xaml.Controls.NavigationViewItem"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::INavigationViewItemStatics statics = GetFactory<winrt::INavigationViewItemStatics>(L"Microsoft.UI.Xaml.Controls.NavigationViewItem");
                    {
                        xamlType.AddDPMember(L"CompactPaneLength", L"Double", statics.CompactPaneLengthProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Icon", L"Microsoft.UI.Xaml.Controls.IconElement", statics.IconProperty(), false /* isContent */);
                    }

                    winrt::INavigationViewItemStatics2 statics2 = GetFactory<winrt::INavigationViewItemStatics2>(L"Microsoft.UI.Xaml.Controls.NavigationViewItem");
                    {
                        xamlType.AddDPMember(L"HasUnrealizedChildren", L"Boolean", statics2.HasUnrealizedChildrenProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsChildSelected", L"Boolean", statics2.IsChildSelectedProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsExpanded", L"Boolean", statics2.IsExpandedProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MenuItems", L"Windows.Foundation.Collections.IVector`1<Object>", statics2.MenuItemsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MenuItemsSource", L"Object", statics2.MenuItemsSourceProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SelectsOnInvoked", L"Boolean", statics2.SelectsOnInvokedProperty(), false /* isContent */);
                    }

                    winrt::INavigationViewItemStatics3 statics3 = GetFactory<winrt::INavigationViewItemStatics3>(L"Microsoft.UI.Xaml.Controls.NavigationViewItem");
                    {
                        xamlType.AddDPMember(L"InfoBadge", L"Microsoft.UI.Xaml.Controls.InfoBadge", statics3.InfoBadgeProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.NavigationViewItemHeader",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.NavigationViewItemHeader",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.NavigationViewItemBase",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::INavigationViewItemHeaderFactory>(L"Microsoft.UI.Xaml.Controls.NavigationViewItemHeader"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.NavigationViewItemSeparator",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.NavigationViewItemSeparator",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.NavigationViewItemBase",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::INavigationViewItemSeparatorFactory>(L"Microsoft.UI.Xaml.Controls.NavigationViewItemSeparator"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.NavigationViewOverflowLabelMode",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.NavigationViewOverflowLabelMode",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"MoreLabel") return box_value(winrt::NavigationViewOverflowLabelMode::MoreLabel);
                    if (fromString == L"NoLabel") return box_value(winrt::NavigationViewOverflowLabelMode::NoLabel);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.NavigationViewPaneDisplayMode",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.NavigationViewPaneDisplayMode",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Auto") return box_value(winrt::NavigationViewPaneDisplayMode::Auto);
                    if (fromString == L"Left") return box_value(winrt::NavigationViewPaneDisplayMode::Left);
                    if (fromString == L"Top") return box_value(winrt::NavigationViewPaneDisplayMode::Top);
                    if (fromString == L"LeftCompact") return box_value(winrt::NavigationViewPaneDisplayMode::LeftCompact);
                    if (fromString == L"LeftMinimal") return box_value(winrt::NavigationViewPaneDisplayMode::LeftMinimal);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.NavigationViewSelectionFollowsFocus",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.NavigationViewSelectionFollowsFocus",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Disabled") return box_value(winrt::NavigationViewSelectionFollowsFocus::Disabled);
                    if (fromString == L"Enabled") return box_value(winrt::NavigationViewSelectionFollowsFocus::Enabled);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.NavigationViewShoulderNavigationEnabled",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.NavigationViewShoulderNavigationEnabled",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"WhenSelectionFollowsFocus") return box_value(winrt::NavigationViewShoulderNavigationEnabled::WhenSelectionFollowsFocus);
                    if (fromString == L"Always") return box_value(winrt::NavigationViewShoulderNavigationEnabled::Always);
                    if (fromString == L"Never") return box_value(winrt::NavigationViewShoulderNavigationEnabled::Never);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.NavigationViewTemplateSettings",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.NavigationViewTemplateSettings",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::INavigationViewTemplateSettingsFactory>(L"Microsoft.UI.Xaml.Controls.NavigationViewTemplateSettings"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::INavigationViewTemplateSettingsStatics statics = GetFactory<winrt::INavigationViewTemplateSettingsStatics>(L"Microsoft.UI.Xaml.Controls.NavigationViewTemplateSettings");
                    {
                        xamlType.AddDPMember(L"BackButtonVisibility", L"Microsoft.UI.Xaml.Visibility", statics.BackButtonVisibilityProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"LeftPaneVisibility", L"Microsoft.UI.Xaml.Visibility", statics.LeftPaneVisibilityProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"OverflowButtonVisibility", L"Microsoft.UI.Xaml.Visibility", statics.OverflowButtonVisibilityProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PaneToggleButtonVisibility", L"Microsoft.UI.Xaml.Visibility", statics.PaneToggleButtonVisibilityProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PaneToggleButtonWidth", L"Double", statics.PaneToggleButtonWidthProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SingleSelectionFollowsFocus", L"Boolean", statics.SingleSelectionFollowsFocusProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SmallerPaneToggleButtonWidth", L"Double", statics.SmallerPaneToggleButtonWidthProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TopPadding", L"Double", statics.TopPaddingProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TopPaneVisibility", L"Microsoft.UI.Xaml.Visibility", statics.TopPaneVisibilityProperty(), false /* isContent */);
                    }

                    winrt::INavigationViewTemplateSettingsStatics2 statics2 = GetFactory<winrt::INavigationViewTemplateSettingsStatics2>(L"Microsoft.UI.Xaml.Controls.NavigationViewTemplateSettings");
                    {
                        xamlType.AddDPMember(L"OpenPaneLength", L"Double", statics2.OpenPaneLengthProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.NonVirtualizingLayout",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.NonVirtualizingLayout",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Layout",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::INonVirtualizingLayoutFactory>(L"Microsoft.UI.Xaml.Controls.NonVirtualizingLayout"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.NonVirtualizingLayoutContext",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.NonVirtualizingLayoutContext",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.LayoutContext",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::INonVirtualizingLayoutContextFactory>(L"Microsoft.UI.Xaml.Controls.NonVirtualizingLayoutContext"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"Children", /* propertyName */
                        L"Windows.Foundation.Collections.IReadOnlyList`1<Microsoft.UI.Xaml.UIElement>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::NonVirtualizingLayoutContext>().Children(); },
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
        L"Microsoft.UI.Xaml.Controls.NumberBox",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.NumberBox",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::INumberBoxFactory>(L"Microsoft.UI.Xaml.Controls.NumberBox"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::INumberBoxStatics statics = GetFactory<winrt::INumberBoxStatics>(L"Microsoft.UI.Xaml.Controls.NumberBox");
                    {
                        xamlType.AddDPMember(L"AcceptsExpression", L"Boolean", statics.AcceptsExpressionProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Description", L"Object", statics.DescriptionProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Header", L"Object", statics.HeaderProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"HeaderTemplate", L"Microsoft.UI.Xaml.DataTemplate", statics.HeaderTemplateProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsWrapEnabled", L"Boolean", statics.IsWrapEnabledProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"LargeChange", L"Double", statics.LargeChangeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Maximum", L"Double", statics.MaximumProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Minimum", L"Double", statics.MinimumProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"NumberFormatter", L"Windows.Globalization.NumberFormatting.INumberFormatter2", statics.NumberFormatterProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PlaceholderText", L"String", statics.PlaceholderTextProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PreventKeyboardDisplayOnProgrammaticFocus", L"Boolean", statics.PreventKeyboardDisplayOnProgrammaticFocusProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SelectionFlyout", L"Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase", statics.SelectionFlyoutProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SelectionHighlightColor", L"Microsoft.UI.Xaml.Media.SolidColorBrush", statics.SelectionHighlightColorProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SmallChange", L"Double", statics.SmallChangeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SpinButtonPlacementMode", L"Microsoft.UI.Xaml.Controls.NumberBoxSpinButtonPlacementMode", statics.SpinButtonPlacementModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Text", L"String", statics.TextProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TextReadingOrder", L"Microsoft.UI.Xaml.TextReadingOrder", statics.TextReadingOrderProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ValidationMode", L"Microsoft.UI.Xaml.Controls.NumberBoxValidationMode", statics.ValidationModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Value", L"Double", statics.ValueProperty(), false /* isContent */);
                    }

                    winrt::INumberBoxStatics2 statics2 = GetFactory<winrt::INumberBoxStatics2>(L"Microsoft.UI.Xaml.Controls.NumberBox");
                    {
                        xamlType.AddDPMember(L"InputScope", L"Microsoft.UI.Xaml.Input.InputScope", statics2.InputScopeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TextAlignment", L"Microsoft.UI.Xaml.TextAlignment", statics2.TextAlignmentProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.NumberBoxSpinButtonPlacementMode",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.NumberBoxSpinButtonPlacementMode",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Hidden") return box_value(winrt::NumberBoxSpinButtonPlacementMode::Hidden);
                    if (fromString == L"Compact") return box_value(winrt::NumberBoxSpinButtonPlacementMode::Compact);
                    if (fromString == L"Inline") return box_value(winrt::NumberBoxSpinButtonPlacementMode::Inline);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.NumberBoxValidationMode",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.NumberBoxValidationMode",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"InvalidInputOverwritten") return box_value(winrt::NumberBoxValidationMode::InvalidInputOverwritten);
                    if (fromString == L"Disabled") return box_value(winrt::NumberBoxValidationMode::Disabled);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.PagerControl",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.PagerControl",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IPagerControlFactory>(L"Microsoft.UI.Xaml.Controls.PagerControl"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IPagerControlStatics statics = GetFactory<winrt::IPagerControlStatics>(L"Microsoft.UI.Xaml.Controls.PagerControl");
                    {
                        xamlType.AddDPMember(L"ButtonPanelAlwaysShowFirstLastPageIndex", L"Boolean", statics.ButtonPanelAlwaysShowFirstLastPageIndexProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"DisplayMode", L"Microsoft.UI.Xaml.Controls.PagerControlDisplayMode", statics.DisplayModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"FirstButtonCommand", L"Microsoft.UI.Xaml.Input.ICommand", statics.FirstButtonCommandProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"FirstButtonStyle", L"Microsoft.UI.Xaml.Style", statics.FirstButtonStyleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"FirstButtonVisibility", L"Microsoft.UI.Xaml.Controls.PagerControlButtonVisibility", statics.FirstButtonVisibilityProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"LastButtonCommand", L"Microsoft.UI.Xaml.Input.ICommand", statics.LastButtonCommandProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"LastButtonStyle", L"Microsoft.UI.Xaml.Style", statics.LastButtonStyleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"LastButtonVisibility", L"Microsoft.UI.Xaml.Controls.PagerControlButtonVisibility", statics.LastButtonVisibilityProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"NextButtonCommand", L"Microsoft.UI.Xaml.Input.ICommand", statics.NextButtonCommandProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"NextButtonStyle", L"Microsoft.UI.Xaml.Style", statics.NextButtonStyleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"NextButtonVisibility", L"Microsoft.UI.Xaml.Controls.PagerControlButtonVisibility", statics.NextButtonVisibilityProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"NumberOfPages", L"Int32", statics.NumberOfPagesProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PagerInputCommand", L"Microsoft.UI.Xaml.Input.ICommand", statics.PagerInputCommandProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PrefixText", L"String", statics.PrefixTextProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PreviousButtonCommand", L"Microsoft.UI.Xaml.Input.ICommand", statics.PreviousButtonCommandProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PreviousButtonStyle", L"Microsoft.UI.Xaml.Style", statics.PreviousButtonStyleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PreviousButtonVisibility", L"Microsoft.UI.Xaml.Controls.PagerControlButtonVisibility", statics.PreviousButtonVisibilityProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SelectedPageIndex", L"Int32", statics.SelectedPageIndexProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SuffixText", L"String", statics.SuffixTextProperty(), false /* isContent */);
                    }

                    xamlType.AddMember(
                        L"TemplateSettings", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.PagerControlTemplateSettings", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::PagerControl>().TemplateSettings(); },
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
        L"Microsoft.UI.Xaml.Controls.PagerControlButtonVisibility",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.PagerControlButtonVisibility",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Visible") return box_value(winrt::PagerControlButtonVisibility::Visible);
                    if (fromString == L"HiddenOnEdge") return box_value(winrt::PagerControlButtonVisibility::HiddenOnEdge);
                    if (fromString == L"Hidden") return box_value(winrt::PagerControlButtonVisibility::Hidden);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.PagerControlDisplayMode",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.PagerControlDisplayMode",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Auto") return box_value(winrt::PagerControlDisplayMode::Auto);
                    if (fromString == L"ComboBox") return box_value(winrt::PagerControlDisplayMode::ComboBox);
                    if (fromString == L"NumberBox") return box_value(winrt::PagerControlDisplayMode::NumberBox);
                    if (fromString == L"ButtonPanel") return box_value(winrt::PagerControlDisplayMode::ButtonPanel);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.PagerControlTemplateSettings",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.PagerControlTemplateSettings",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IPagerControlTemplateSettingsFactory>(L"Microsoft.UI.Xaml.Controls.PagerControlTemplateSettings"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"NumberPanelItems", /* propertyName */
                        L"Windows.Foundation.Collections.IVector`1<Object>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::PagerControlTemplateSettings>().NumberPanelItems(); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"Pages", /* propertyName */
                        L"Windows.Foundation.Collections.IVector`1<Object>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::PagerControlTemplateSettings>().Pages(); },
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
        L"Microsoft.UI.Xaml.Controls.ParallaxSourceOffsetKind",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ParallaxSourceOffsetKind",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Absolute") return box_value(winrt::ParallaxSourceOffsetKind::Absolute);
                    if (fromString == L"Relative") return box_value(winrt::ParallaxSourceOffsetKind::Relative);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ParallaxView",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ParallaxView",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.FrameworkElement",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IParallaxViewFactory>(L"Microsoft.UI.Xaml.Controls.ParallaxView"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IParallaxViewStatics statics = GetFactory<winrt::IParallaxViewStatics>(L"Microsoft.UI.Xaml.Controls.ParallaxView");
                    {
                        xamlType.AddDPMember(L"Child", L"Microsoft.UI.Xaml.UIElement", statics.ChildProperty(), true /* isContent */);
                        xamlType.AddDPMember(L"HorizontalShift", L"Double", statics.HorizontalShiftProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"HorizontalSourceEndOffset", L"Double", statics.HorizontalSourceEndOffsetProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"HorizontalSourceOffsetKind", L"Microsoft.UI.Xaml.Controls.ParallaxSourceOffsetKind", statics.HorizontalSourceOffsetKindProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"HorizontalSourceStartOffset", L"Double", statics.HorizontalSourceStartOffsetProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsHorizontalShiftClamped", L"Boolean", statics.IsHorizontalShiftClampedProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsVerticalShiftClamped", L"Boolean", statics.IsVerticalShiftClampedProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MaxHorizontalShiftRatio", L"Double", statics.MaxHorizontalShiftRatioProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MaxVerticalShiftRatio", L"Double", statics.MaxVerticalShiftRatioProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Source", L"Microsoft.UI.Xaml.UIElement", statics.SourceProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"VerticalShift", L"Double", statics.VerticalShiftProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"VerticalSourceEndOffset", L"Double", statics.VerticalSourceEndOffsetProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"VerticalSourceOffsetKind", L"Microsoft.UI.Xaml.Controls.ParallaxSourceOffsetKind", statics.VerticalSourceOffsetKindProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"VerticalSourceStartOffset", L"Double", statics.VerticalSourceStartOffsetProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.PersonPicture",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.PersonPicture",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IPersonPictureFactory>(L"Microsoft.UI.Xaml.Controls.PersonPicture"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IPersonPictureStatics statics = GetFactory<winrt::IPersonPictureStatics>(L"Microsoft.UI.Xaml.Controls.PersonPicture");
                    {
                        xamlType.AddDPMember(L"BadgeGlyph", L"String", statics.BadgeGlyphProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"BadgeImageSource", L"Microsoft.UI.Xaml.Media.ImageSource", statics.BadgeImageSourceProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"BadgeNumber", L"Int32", statics.BadgeNumberProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"BadgeText", L"String", statics.BadgeTextProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Contact", L"Windows.ApplicationModel.Contacts.Contact", statics.ContactProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"DisplayName", L"String", statics.DisplayNameProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Initials", L"String", statics.InitialsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsGroup", L"Boolean", statics.IsGroupProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PreferSmallImage", L"Boolean", statics.PreferSmallImageProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ProfilePicture", L"Microsoft.UI.Xaml.Media.ImageSource", statics.ProfilePictureProperty(), false /* isContent */);
                    }

                    xamlType.AddMember(
                        L"TemplateSettings", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.PersonPictureTemplateSettings", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::PersonPicture>().TemplateSettings(); },
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
        L"Microsoft.UI.Xaml.Controls.PersonPictureTemplateSettings",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.PersonPictureTemplateSettings",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"ActualImageBrush", /* propertyName */
                        L"Microsoft.UI.Xaml.Media.ImageBrush", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::PersonPictureTemplateSettings>().ActualImageBrush(); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ActualInitials", /* propertyName */
                        L"String", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::PersonPictureTemplateSettings>().ActualInitials()); },
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
        L"Microsoft.UI.Xaml.Controls.PipsPager",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.PipsPager",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IPipsPagerFactory>(L"Microsoft.UI.Xaml.Controls.PipsPager"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IPipsPagerStatics statics = GetFactory<winrt::IPipsPagerStatics>(L"Microsoft.UI.Xaml.Controls.PipsPager");
                    {
                        xamlType.AddDPMember(L"MaxVisiblePips", L"Int32", statics.MaxVisiblePipsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"NextButtonStyle", L"Microsoft.UI.Xaml.Style", statics.NextButtonStyleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"NextButtonVisibility", L"Microsoft.UI.Xaml.Controls.PipsPagerButtonVisibility", statics.NextButtonVisibilityProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"NormalPipStyle", L"Microsoft.UI.Xaml.Style", statics.NormalPipStyleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"NumberOfPages", L"Int32", statics.NumberOfPagesProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Orientation", L"Microsoft.UI.Xaml.Controls.Orientation", statics.OrientationProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PreviousButtonStyle", L"Microsoft.UI.Xaml.Style", statics.PreviousButtonStyleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PreviousButtonVisibility", L"Microsoft.UI.Xaml.Controls.PipsPagerButtonVisibility", statics.PreviousButtonVisibilityProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SelectedPageIndex", L"Int32", statics.SelectedPageIndexProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SelectedPipStyle", L"Microsoft.UI.Xaml.Style", statics.SelectedPipStyleProperty(), false /* isContent */);
                    }

                    winrt::IPipsPagerStatics2 statics2 = GetFactory<winrt::IPipsPagerStatics2>(L"Microsoft.UI.Xaml.Controls.PipsPager");
                    {
                        xamlType.AddDPMember(L"WrapMode", L"Microsoft.UI.Xaml.Controls.PipsPagerWrapMode", statics2.WrapModeProperty(), false /* isContent */);
                    }

                    xamlType.AddMember(
                        L"TemplateSettings", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.PipsPagerTemplateSettings", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::PipsPager>().TemplateSettings(); },
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
        L"Microsoft.UI.Xaml.Controls.PipsPagerButtonVisibility",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.PipsPagerButtonVisibility",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Visible") return box_value(winrt::PipsPagerButtonVisibility::Visible);
                    if (fromString == L"VisibleOnPointerOver") return box_value(winrt::PipsPagerButtonVisibility::VisibleOnPointerOver);
                    if (fromString == L"Collapsed") return box_value(winrt::PipsPagerButtonVisibility::Collapsed);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.PipsPagerTemplateSettings",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.PipsPagerTemplateSettings",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"PipsPagerItems", /* propertyName */
                        L"Windows.Foundation.Collections.IVector`1<Int32>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::PipsPagerTemplateSettings>().PipsPagerItems(); },
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
        L"Microsoft.UI.Xaml.Controls.PipsPagerWrapMode",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.PipsPagerWrapMode",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"None") return box_value(winrt::PipsPagerWrapMode::None);
                    if (fromString == L"Wrap") return box_value(winrt::PipsPagerWrapMode::Wrap);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.PlayerAnimationOptimization",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.PlayerAnimationOptimization",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Latency") return box_value(winrt::PlayerAnimationOptimization::Latency);
                    if (fromString == L"Resources") return box_value(winrt::PlayerAnimationOptimization::Resources);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Primitives.AutoSuggestBoxHelper",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.AutoSuggestBoxHelper",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IAutoSuggestBoxHelperStatics statics = GetFactory<winrt::IAutoSuggestBoxHelperStatics>(L"Microsoft.UI.Xaml.Controls.Primitives.AutoSuggestBoxHelper");
                    {
                        xamlType.AddDPMember(L"KeepInteriorCornersSquare", L"Boolean", statics.KeepInteriorCornersSquareProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Primitives.ColorPickerSlider",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.ColorPickerSlider",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Slider",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IColorPickerSliderFactory>(L"Microsoft.UI.Xaml.Controls.Primitives.ColorPickerSlider"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IColorPickerSliderStatics statics = GetFactory<winrt::IColorPickerSliderStatics>(L"Microsoft.UI.Xaml.Controls.Primitives.ColorPickerSlider");
                    {
                        xamlType.AddDPMember(L"ColorChannel", L"Microsoft.UI.Xaml.Controls.ColorPickerHsvChannel", statics.ColorChannelProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Primitives.ColorSpectrum",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.ColorSpectrum",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IColorSpectrumFactory>(L"Microsoft.UI.Xaml.Controls.Primitives.ColorSpectrum"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IColorSpectrumStatics statics = GetFactory<winrt::IColorSpectrumStatics>(L"Microsoft.UI.Xaml.Controls.Primitives.ColorSpectrum");
                    {
                        xamlType.AddDPMember(L"Color", L"Windows.UI.Color", statics.ColorProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Components", L"Microsoft.UI.Xaml.Controls.ColorSpectrumComponents", statics.ComponentsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"HsvColor", L"Windows.Foundation.Numerics.Vector4", statics.HsvColorProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MaxHue", L"Int32", statics.MaxHueProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MaxSaturation", L"Int32", statics.MaxSaturationProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MaxValue", L"Int32", statics.MaxValueProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MinHue", L"Int32", statics.MinHueProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MinSaturation", L"Int32", statics.MinSaturationProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MinValue", L"Int32", statics.MinValueProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Shape", L"Microsoft.UI.Xaml.Controls.ColorSpectrumShape", statics.ShapeProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Primitives.ColumnMajorUniformToLargestGridLayout",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.ColumnMajorUniformToLargestGridLayout",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.NonVirtualizingLayout",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IColumnMajorUniformToLargestGridLayoutFactory>(L"Microsoft.UI.Xaml.Controls.Primitives.ColumnMajorUniformToLargestGridLayout"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IColumnMajorUniformToLargestGridLayoutStatics statics = GetFactory<winrt::IColumnMajorUniformToLargestGridLayoutStatics>(L"Microsoft.UI.Xaml.Controls.Primitives.ColumnMajorUniformToLargestGridLayout");
                    {
                        xamlType.AddDPMember(L"ColumnSpacing", L"Double", statics.ColumnSpacingProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MaxColumns", L"Int32", statics.MaxColumnsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"RowSpacing", L"Double", statics.RowSpacingProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Primitives.ComboBoxHelper",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.ComboBoxHelper",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IComboBoxHelperStatics statics = GetFactory<winrt::IComboBoxHelperStatics>(L"Microsoft.UI.Xaml.Controls.Primitives.ComboBoxHelper");
                    {
                        xamlType.AddDPMember(L"KeepInteriorCornersSquare", L"Boolean", statics.KeepInteriorCornersSquareProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Primitives.CommandBarFlyoutCommandBar",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.CommandBarFlyoutCommandBar",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.CommandBar",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ICommandBarFlyoutCommandBarFactory>(L"Microsoft.UI.Xaml.Controls.Primitives.CommandBarFlyoutCommandBar"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ICommandBarFlyoutCommandBarStatics statics = GetFactory<winrt::ICommandBarFlyoutCommandBarStatics>(L"Microsoft.UI.Xaml.Controls.Primitives.CommandBarFlyoutCommandBar");
                    {
                        xamlType.AddDPMember(L"SystemBackdrop", L"Microsoft.UI.Xaml.Media.SystemBackdrop", statics.SystemBackdropProperty(), false /* isContent */);
                    }

                    xamlType.AddMember(
                        L"FlyoutTemplateSettings", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.Primitives.CommandBarFlyoutCommandBarTemplateSettings", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::CommandBarFlyoutCommandBar>().FlyoutTemplateSettings(); },
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
        L"Microsoft.UI.Xaml.Controls.Primitives.CommandBarFlyoutCommandBarAutomationProperties",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.CommandBarFlyoutCommandBarAutomationProperties",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ICommandBarFlyoutCommandBarAutomationPropertiesStatics statics = GetFactory<winrt::ICommandBarFlyoutCommandBarAutomationPropertiesStatics>(L"Microsoft.UI.Xaml.Controls.Primitives.CommandBarFlyoutCommandBarAutomationProperties");
                    {
                        xamlType.AddDPMember(L"ControlType", L"Microsoft.UI.Xaml.Automation.Peers.AutomationControlType", statics.ControlTypeProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Primitives.CommandBarFlyoutCommandBarTemplateSettings",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.CommandBarFlyoutCommandBarTemplateSettings",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"CloseAnimationEndPosition", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::CommandBarFlyoutCommandBarTemplateSettings>().CloseAnimationEndPosition()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ContentClipRect", /* propertyName */
                        L"Windows.Foundation.Rect", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::CommandBarFlyoutCommandBarTemplateSettings>().ContentClipRect()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"CurrentWidth", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::CommandBarFlyoutCommandBarTemplateSettings>().CurrentWidth()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ExpandDownAnimationEndPosition", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::CommandBarFlyoutCommandBarTemplateSettings>().ExpandDownAnimationEndPosition()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ExpandDownAnimationHoldPosition", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::CommandBarFlyoutCommandBarTemplateSettings>().ExpandDownAnimationHoldPosition()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ExpandDownAnimationStartPosition", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::CommandBarFlyoutCommandBarTemplateSettings>().ExpandDownAnimationStartPosition()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ExpandDownOverflowVerticalPosition", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::CommandBarFlyoutCommandBarTemplateSettings>().ExpandDownOverflowVerticalPosition()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ExpandUpAnimationEndPosition", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::CommandBarFlyoutCommandBarTemplateSettings>().ExpandUpAnimationEndPosition()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ExpandUpAnimationHoldPosition", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::CommandBarFlyoutCommandBarTemplateSettings>().ExpandUpAnimationHoldPosition()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ExpandUpAnimationStartPosition", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::CommandBarFlyoutCommandBarTemplateSettings>().ExpandUpAnimationStartPosition()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ExpandUpOverflowVerticalPosition", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::CommandBarFlyoutCommandBarTemplateSettings>().ExpandUpOverflowVerticalPosition()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ExpandedWidth", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::CommandBarFlyoutCommandBarTemplateSettings>().ExpandedWidth()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"OpenAnimationEndPosition", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::CommandBarFlyoutCommandBarTemplateSettings>().OpenAnimationEndPosition()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"OpenAnimationStartPosition", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::CommandBarFlyoutCommandBarTemplateSettings>().OpenAnimationStartPosition()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"OverflowContentClipRect", /* propertyName */
                        L"Windows.Foundation.Rect", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::CommandBarFlyoutCommandBarTemplateSettings>().OverflowContentClipRect()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"WidthExpansionAnimationEndPosition", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::CommandBarFlyoutCommandBarTemplateSettings>().WidthExpansionAnimationEndPosition()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"WidthExpansionAnimationStartPosition", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::CommandBarFlyoutCommandBarTemplateSettings>().WidthExpansionAnimationStartPosition()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"WidthExpansionDelta", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::CommandBarFlyoutCommandBarTemplateSettings>().WidthExpansionDelta()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"WidthExpansionMoreButtonAnimationEndPosition", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::CommandBarFlyoutCommandBarTemplateSettings>().WidthExpansionMoreButtonAnimationEndPosition()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"WidthExpansionMoreButtonAnimationStartPosition", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::CommandBarFlyoutCommandBarTemplateSettings>().WidthExpansionMoreButtonAnimationStartPosition()); },
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
        L"Microsoft.UI.Xaml.Controls.Primitives.CornerRadiusFilterConverter",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.CornerRadiusFilterConverter",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstance(L"Microsoft.UI.Xaml.Controls.Primitives.CornerRadiusFilterConverter"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ICornerRadiusFilterConverterStatics statics = GetFactory<winrt::ICornerRadiusFilterConverterStatics>(L"Microsoft.UI.Xaml.Controls.Primitives.CornerRadiusFilterConverter");
                    {
                        xamlType.AddDPMember(L"Filter", L"Microsoft.UI.Xaml.Controls.Primitives.CornerRadiusFilterKind", statics.FilterProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Scale", L"Double", statics.ScaleProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Primitives.CornerRadiusFilterKind",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.CornerRadiusFilterKind",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"None") return box_value(winrt::CornerRadiusFilterKind::None);
                    if (fromString == L"Top") return box_value(winrt::CornerRadiusFilterKind::Top);
                    if (fromString == L"Right") return box_value(winrt::CornerRadiusFilterKind::Right);
                    if (fromString == L"Bottom") return box_value(winrt::CornerRadiusFilterKind::Bottom);
                    if (fromString == L"Left") return box_value(winrt::CornerRadiusFilterKind::Left);
                    if (fromString == L"TopLeftValue") return box_value(winrt::CornerRadiusFilterKind::TopLeftValue);
                    if (fromString == L"BottomRightValue") return box_value(winrt::CornerRadiusFilterKind::BottomRightValue);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Primitives.CornerRadiusToThicknessConverter",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.CornerRadiusToThicknessConverter",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstance(L"Microsoft.UI.Xaml.Controls.Primitives.CornerRadiusToThicknessConverter"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ICornerRadiusToThicknessConverterStatics statics = GetFactory<winrt::ICornerRadiusToThicknessConverterStatics>(L"Microsoft.UI.Xaml.Controls.Primitives.CornerRadiusToThicknessConverter");
                    {
                        xamlType.AddDPMember(L"ConversionKind", L"Microsoft.UI.Xaml.Controls.Primitives.CornerRadiusToThicknessConverterKind", statics.ConversionKindProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Multiplier", L"Double", statics.MultiplierProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Primitives.CornerRadiusToThicknessConverterKind",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.CornerRadiusToThicknessConverterKind",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"FilterTopAndBottomFromLeft") return box_value(winrt::CornerRadiusToThicknessConverterKind::FilterTopAndBottomFromLeft);
                    if (fromString == L"FilterTopAndBottomFromRight") return box_value(winrt::CornerRadiusToThicknessConverterKind::FilterTopAndBottomFromRight);
                    if (fromString == L"FilterLeftAndRightFromTop") return box_value(winrt::CornerRadiusToThicknessConverterKind::FilterLeftAndRightFromTop);
                    if (fromString == L"FilterLeftAndRightFromBottom") return box_value(winrt::CornerRadiusToThicknessConverterKind::FilterLeftAndRightFromBottom);
                    if (fromString == L"FilterTopFromTopLeft") return box_value(winrt::CornerRadiusToThicknessConverterKind::FilterTopFromTopLeft);
                    if (fromString == L"FilterTopFromTopRight") return box_value(winrt::CornerRadiusToThicknessConverterKind::FilterTopFromTopRight);
                    if (fromString == L"FilterRightFromTopRight") return box_value(winrt::CornerRadiusToThicknessConverterKind::FilterRightFromTopRight);
                    if (fromString == L"FilterRightFromBottomRight") return box_value(winrt::CornerRadiusToThicknessConverterKind::FilterRightFromBottomRight);
                    if (fromString == L"FilterBottomFromBottomRight") return box_value(winrt::CornerRadiusToThicknessConverterKind::FilterBottomFromBottomRight);
                    if (fromString == L"FilterBottomFromBottomLeft") return box_value(winrt::CornerRadiusToThicknessConverterKind::FilterBottomFromBottomLeft);
                    if (fromString == L"FilterLeftFromBottomLeft") return box_value(winrt::CornerRadiusToThicknessConverterKind::FilterLeftFromBottomLeft);
                    if (fromString == L"FilterLeftFromTopLeft") return box_value(winrt::CornerRadiusToThicknessConverterKind::FilterLeftFromTopLeft);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Primitives.IScrollController",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.IScrollController",
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
        L"Microsoft.UI.Xaml.Controls.Primitives.IScrollControllerPanningInfo",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.IScrollControllerPanningInfo",
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
        L"Microsoft.UI.Xaml.Controls.Primitives.InfoBarPanel",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.InfoBarPanel",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Panel",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IInfoBarPanelFactory>(L"Microsoft.UI.Xaml.Controls.Primitives.InfoBarPanel"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IInfoBarPanelStatics statics = GetFactory<winrt::IInfoBarPanelStatics>(L"Microsoft.UI.Xaml.Controls.Primitives.InfoBarPanel");
                    {
                        xamlType.AddDPMember(L"HorizontalOrientationMargin", L"Microsoft.UI.Xaml.Thickness", statics.HorizontalOrientationMarginProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"HorizontalOrientationPadding", L"Microsoft.UI.Xaml.Thickness", statics.HorizontalOrientationPaddingProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"VerticalOrientationMargin", L"Microsoft.UI.Xaml.Thickness", statics.VerticalOrientationMarginProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"VerticalOrientationPadding", L"Microsoft.UI.Xaml.Thickness", statics.VerticalOrientationPaddingProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Primitives.MonochromaticOverlayPresenter",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.MonochromaticOverlayPresenter",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Grid",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IMonochromaticOverlayPresenterFactory>(L"Microsoft.UI.Xaml.Controls.Primitives.MonochromaticOverlayPresenter"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IMonochromaticOverlayPresenterStatics statics = GetFactory<winrt::IMonochromaticOverlayPresenterStatics>(L"Microsoft.UI.Xaml.Controls.Primitives.MonochromaticOverlayPresenter");
                    {
                        xamlType.AddDPMember(L"ReplacementColor", L"Windows.UI.Color", statics.ReplacementColorProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SourceElement", L"Microsoft.UI.Xaml.UIElement", statics.SourceElementProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Primitives.NavigationViewItemPresenter",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.NavigationViewItemPresenter",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ContentControl",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::INavigationViewItemPresenterFactory>(L"Microsoft.UI.Xaml.Controls.Primitives.NavigationViewItemPresenter"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::INavigationViewItemPresenterStatics statics = GetFactory<winrt::INavigationViewItemPresenterStatics>(L"Microsoft.UI.Xaml.Controls.Primitives.NavigationViewItemPresenter");
                    {
                        xamlType.AddDPMember(L"Icon", L"Microsoft.UI.Xaml.Controls.IconElement", statics.IconProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TemplateSettings", L"Microsoft.UI.Xaml.Controls.Primitives.NavigationViewItemPresenterTemplateSettings", statics.TemplateSettingsProperty(), false /* isContent */);
                    }

                    winrt::INavigationViewItemPresenterStatics2 statics2 = GetFactory<winrt::INavigationViewItemPresenterStatics2>(L"Microsoft.UI.Xaml.Controls.Primitives.NavigationViewItemPresenter");
                    {
                        xamlType.AddDPMember(L"InfoBadge", L"Microsoft.UI.Xaml.Controls.InfoBadge", statics2.InfoBadgeProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Primitives.NavigationViewItemPresenterTemplateSettings",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.NavigationViewItemPresenterTemplateSettings",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::INavigationViewItemPresenterTemplateSettingsFactory>(L"Microsoft.UI.Xaml.Controls.Primitives.NavigationViewItemPresenterTemplateSettings"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::INavigationViewItemPresenterTemplateSettingsStatics statics = GetFactory<winrt::INavigationViewItemPresenterTemplateSettingsStatics>(L"Microsoft.UI.Xaml.Controls.Primitives.NavigationViewItemPresenterTemplateSettings");
                    {
                        xamlType.AddDPMember(L"IconWidth", L"Double", statics.IconWidthProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SmallerIconWidth", L"Double", statics.SmallerIconWidthProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Primitives.SnapPointBase",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.SnapPointBase",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
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
        L"Microsoft.UI.Xaml.Controls.Primitives.ScrollSnapPointBase",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.ScrollSnapPointBase",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.SnapPointBase",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"Alignment", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.Primitives.ScrollSnapPointsAlignment", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value<int>((int)instance.as<winrt::ScrollSnapPointBase>().Alignment()); },
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
        L"Microsoft.UI.Xaml.Controls.Primitives.RepeatedScrollSnapPoint",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.RepeatedScrollSnapPoint",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.ScrollSnapPointBase",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"End", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::RepeatedScrollSnapPoint>().End()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"Interval", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::RepeatedScrollSnapPoint>().Interval()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"Offset", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::RepeatedScrollSnapPoint>().Offset()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"Start", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::RepeatedScrollSnapPoint>().Start()); },
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
        L"Microsoft.UI.Xaml.Controls.Primitives.ZoomSnapPointBase",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.ZoomSnapPointBase",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.SnapPointBase",
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
        L"Microsoft.UI.Xaml.Controls.Primitives.RepeatedZoomSnapPoint",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.RepeatedZoomSnapPoint",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.ZoomSnapPointBase",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"End", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::RepeatedZoomSnapPoint>().End()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"Interval", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::RepeatedZoomSnapPoint>().Interval()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"Offset", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::RepeatedZoomSnapPoint>().Offset()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"Start", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::RepeatedZoomSnapPoint>().Start()); },
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
        L"Microsoft.UI.Xaml.Controls.Primitives.ScrollPresenter",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.ScrollPresenter",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.FrameworkElement",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IScrollPresenterFactory>(L"Microsoft.UI.Xaml.Controls.Primitives.ScrollPresenter"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IScrollPresenterStatics statics = GetFactory<winrt::IScrollPresenterStatics>(L"Microsoft.UI.Xaml.Controls.Primitives.ScrollPresenter");
                    {
                        xamlType.AddDPMember(L"Background", L"Microsoft.UI.Xaml.Media.Brush", statics.BackgroundProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ComputedHorizontalScrollMode", L"Microsoft.UI.Xaml.Controls.ScrollingScrollMode", statics.ComputedHorizontalScrollModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ComputedVerticalScrollMode", L"Microsoft.UI.Xaml.Controls.ScrollingScrollMode", statics.ComputedVerticalScrollModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ContentOrientation", L"Microsoft.UI.Xaml.Controls.ScrollingContentOrientation", statics.ContentOrientationProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Content", L"Microsoft.UI.Xaml.UIElement", statics.ContentProperty(), true /* isContent */);
                        xamlType.AddDPMember(L"HorizontalAnchorRatio", L"Double", statics.HorizontalAnchorRatioProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"HorizontalScrollChainMode", L"Microsoft.UI.Xaml.Controls.ScrollingChainMode", statics.HorizontalScrollChainModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"HorizontalScrollMode", L"Microsoft.UI.Xaml.Controls.ScrollingScrollMode", statics.HorizontalScrollModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"HorizontalScrollRailMode", L"Microsoft.UI.Xaml.Controls.ScrollingRailMode", statics.HorizontalScrollRailModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IgnoredInputKinds", L"Microsoft.UI.Xaml.Controls.ScrollingInputKinds", statics.IgnoredInputKindsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MaxZoomFactor", L"Double", statics.MaxZoomFactorProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MinZoomFactor", L"Double", statics.MinZoomFactorProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"VerticalAnchorRatio", L"Double", statics.VerticalAnchorRatioProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"VerticalScrollChainMode", L"Microsoft.UI.Xaml.Controls.ScrollingChainMode", statics.VerticalScrollChainModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"VerticalScrollMode", L"Microsoft.UI.Xaml.Controls.ScrollingScrollMode", statics.VerticalScrollModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"VerticalScrollRailMode", L"Microsoft.UI.Xaml.Controls.ScrollingRailMode", statics.VerticalScrollRailModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ZoomChainMode", L"Microsoft.UI.Xaml.Controls.ScrollingChainMode", statics.ZoomChainModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ZoomMode", L"Microsoft.UI.Xaml.Controls.ScrollingZoomMode", statics.ZoomModeProperty(), false /* isContent */);
                    }

                    xamlType.AddMember(
                        L"CurrentAnchor", /* propertyName */
                        L"Microsoft.UI.Xaml.UIElement", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::ScrollPresenter>().CurrentAnchor(); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"HorizontalScrollController", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.Primitives.IScrollController", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::ScrollPresenter>().HorizontalScrollController(); },
                        [](winrt::IInspectable instance, winrt::IInspectable value) { instance.as<winrt::ScrollPresenter>().HorizontalScrollController(value.as<winrt::Microsoft::UI::Xaml::Controls::Primitives::IScrollController>()); },
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"VerticalScrollController", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.Primitives.IScrollController", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::ScrollPresenter>().VerticalScrollController(); },
                        [](winrt::IInspectable instance, winrt::IInspectable value) { instance.as<winrt::ScrollPresenter>().VerticalScrollController(value.as<winrt::Microsoft::UI::Xaml::Controls::Primitives::IScrollController>()); },
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ExpressionAnimationSources", /* propertyName */
                        L"Microsoft.UI.Composition.CompositionPropertySet", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::ScrollPresenter>().ExpressionAnimationSources(); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ExtentHeight", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ScrollPresenter>().ExtentHeight()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ExtentWidth", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ScrollPresenter>().ExtentWidth()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"HorizontalOffset", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ScrollPresenter>().HorizontalOffset()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"HorizontalSnapPoints", /* propertyName */
                        L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.Primitives.ScrollSnapPointBase>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::ScrollPresenter>().HorizontalSnapPoints(); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ScrollableHeight", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ScrollPresenter>().ScrollableHeight()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ScrollableWidth", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ScrollPresenter>().ScrollableWidth()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"State", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.ScrollingInteractionState", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value<int>((int)instance.as<winrt::ScrollPresenter>().State()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"VerticalOffset", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ScrollPresenter>().VerticalOffset()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"VerticalSnapPoints", /* propertyName */
                        L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.Primitives.ScrollSnapPointBase>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::ScrollPresenter>().VerticalSnapPoints(); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ViewportHeight", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ScrollPresenter>().ViewportHeight()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ViewportWidth", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ScrollPresenter>().ViewportWidth()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ZoomFactor", /* propertyName */
                        L"Single", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ScrollPresenter>().ZoomFactor()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ZoomSnapPoints", /* propertyName */
                        L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.Primitives.ZoomSnapPointBase>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::ScrollPresenter>().ZoomSnapPoints(); },
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
        L"Microsoft.UI.Xaml.Controls.Primitives.ScrollSnapPoint",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.ScrollSnapPoint",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.ScrollSnapPointBase",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"Value", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ScrollSnapPoint>().Value()); },
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
        L"Microsoft.UI.Xaml.Controls.Primitives.ScrollSnapPointsAlignment",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.ScrollSnapPointsAlignment",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Near") return box_value(winrt::ScrollSnapPointsAlignment::Near);
                    if (fromString == L"Center") return box_value(winrt::ScrollSnapPointsAlignment::Center);
                    if (fromString == L"Far") return box_value(winrt::ScrollSnapPointsAlignment::Far);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Primitives.TabViewListView",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.TabViewListView",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ListView",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ITabViewListViewFactory>(L"Microsoft.UI.Xaml.Controls.Primitives.TabViewListView"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Primitives.ZoomSnapPoint",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.ZoomSnapPoint",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.ZoomSnapPointBase",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"Value", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ZoomSnapPoint>().Value()); },
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
        L"Microsoft.UI.Xaml.Controls.ProgressBar",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ProgressBar",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.RangeBase",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IProgressBarFactory>(L"Microsoft.UI.Xaml.Controls.ProgressBar"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IProgressBarStatics statics = GetFactory<winrt::IProgressBarStatics>(L"Microsoft.UI.Xaml.Controls.ProgressBar");
                    {
                        xamlType.AddDPMember(L"IsIndeterminate", L"Boolean", statics.IsIndeterminateProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ShowError", L"Boolean", statics.ShowErrorProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ShowPaused", L"Boolean", statics.ShowPausedProperty(), false /* isContent */);
                    }

                    xamlType.AddMember(
                        L"TemplateSettings", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.ProgressBarTemplateSettings", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::ProgressBar>().TemplateSettings(); },
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
        L"Microsoft.UI.Xaml.Controls.ProgressBarTemplateSettings",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ProgressBarTemplateSettings",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"ClipRect", /* propertyName */
                        L"Microsoft.UI.Xaml.Media.RectangleGeometry", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::ProgressBarTemplateSettings>().ClipRect(); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"Container2AnimationEndPosition", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ProgressBarTemplateSettings>().Container2AnimationEndPosition()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"Container2AnimationStartPosition", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ProgressBarTemplateSettings>().Container2AnimationStartPosition()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ContainerAnimationEndPosition", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ProgressBarTemplateSettings>().ContainerAnimationEndPosition()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ContainerAnimationMidPosition", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ProgressBarTemplateSettings>().ContainerAnimationMidPosition()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ContainerAnimationStartPosition", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ProgressBarTemplateSettings>().ContainerAnimationStartPosition()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"EllipseAnimationEndPosition", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ProgressBarTemplateSettings>().EllipseAnimationEndPosition()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"EllipseAnimationWellPosition", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ProgressBarTemplateSettings>().EllipseAnimationWellPosition()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"EllipseDiameter", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ProgressBarTemplateSettings>().EllipseDiameter()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"EllipseOffset", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ProgressBarTemplateSettings>().EllipseOffset()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"IndicatorLengthDelta", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ProgressBarTemplateSettings>().IndicatorLengthDelta()); },
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
        L"Microsoft.UI.Xaml.Controls.ProgressRing",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ProgressRing",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IProgressRingFactory>(L"Microsoft.UI.Xaml.Controls.ProgressRing"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IProgressRingStatics statics = GetFactory<winrt::IProgressRingStatics>(L"Microsoft.UI.Xaml.Controls.ProgressRing");
                    {
                        xamlType.AddDPMember(L"IsActive", L"Boolean", statics.IsActiveProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsIndeterminate", L"Boolean", statics.IsIndeterminateProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Maximum", L"Double", statics.MaximumProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Minimum", L"Double", statics.MinimumProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Value", L"Double", statics.ValueProperty(), false /* isContent */);
                    }

                    winrt::IProgressRingStatics2 statics2 = GetFactory<winrt::IProgressRingStatics2>(L"Microsoft.UI.Xaml.Controls.ProgressRing");
                    {
                        xamlType.AddDPMember(L"DeterminateSource", L"Microsoft.UI.Xaml.Controls.IAnimatedVisualSource", statics2.DeterminateSourceProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IndeterminateSource", L"Microsoft.UI.Xaml.Controls.IAnimatedVisualSource", statics2.IndeterminateSourceProperty(), false /* isContent */);
                    }

                    xamlType.AddMember(
                        L"TemplateSettings", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.ProgressRingTemplateSettings", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::ProgressRing>().TemplateSettings(); },
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
        L"Microsoft.UI.Xaml.Controls.ProgressRingTemplateSettings",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ProgressRingTemplateSettings",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"EllipseDiameter", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ProgressRingTemplateSettings>().EllipseDiameter()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"EllipseOffset", /* propertyName */
                        L"Microsoft.UI.Xaml.Thickness", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ProgressRingTemplateSettings>().EllipseOffset()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"MaxSideLength", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ProgressRingTemplateSettings>().MaxSideLength()); },
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
        L"Microsoft.UI.Xaml.Controls.RadioButtons",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.RadioButtons",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IRadioButtonsFactory>(L"Microsoft.UI.Xaml.Controls.RadioButtons"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IRadioButtonsStatics statics = GetFactory<winrt::IRadioButtonsStatics>(L"Microsoft.UI.Xaml.Controls.RadioButtons");
                    {
                        xamlType.AddDPMember(L"Header", L"Object", statics.HeaderProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"HeaderTemplate", L"Microsoft.UI.Xaml.DataTemplate", statics.HeaderTemplateProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ItemTemplate", L"Object", statics.ItemTemplateProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Items", L"Windows.Foundation.Collections.IVector`1<Object>", statics.ItemsProperty(), true /* isContent */);
                        xamlType.AddDPMember(L"ItemsSource", L"Object", statics.ItemsSourceProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MaxColumns", L"Int32", statics.MaxColumnsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SelectedIndex", L"Int32", statics.SelectedIndexProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SelectedItem", L"Object", statics.SelectedItemProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.RadioMenuFlyoutItem",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.RadioMenuFlyoutItem",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.MenuFlyoutItem",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IRadioMenuFlyoutItemFactory>(L"Microsoft.UI.Xaml.Controls.RadioMenuFlyoutItem"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IRadioMenuFlyoutItemStatics statics = GetFactory<winrt::IRadioMenuFlyoutItemStatics>(L"Microsoft.UI.Xaml.Controls.RadioMenuFlyoutItem");
                    {
                        xamlType.AddDPMember(L"GroupName", L"String", statics.GroupNameProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsChecked", L"Boolean", statics.IsCheckedProperty(), false /* isContent */);
                    }

                    winrt::IRadioMenuFlyoutItemStatics2 statics2 = GetFactory<winrt::IRadioMenuFlyoutItemStatics2>(L"Microsoft.UI.Xaml.Controls.RadioMenuFlyoutItem");
                    {
                        xamlType.AddDPMember(L"AreCheckStatesEnabled", L"Boolean", statics2.AreCheckStatesEnabledProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.RatingControl",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.RatingControl",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IRatingControlFactory>(L"Microsoft.UI.Xaml.Controls.RatingControl"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IRatingControlStatics statics = GetFactory<winrt::IRatingControlStatics>(L"Microsoft.UI.Xaml.Controls.RatingControl");
                    {
                        xamlType.AddDPMember(L"Caption", L"String", statics.CaptionProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"InitialSetValue", L"Int32", statics.InitialSetValueProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsClearEnabled", L"Boolean", statics.IsClearEnabledProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsReadOnly", L"Boolean", statics.IsReadOnlyProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ItemInfo", L"Microsoft.UI.Xaml.Controls.RatingItemInfo", statics.ItemInfoProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MaxRating", L"Int32", statics.MaxRatingProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PlaceholderValue", L"Double", statics.PlaceholderValueProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Value", L"Double", statics.ValueProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.RatingItemInfo",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.RatingItemInfo",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IRatingItemInfoFactory>(L"Microsoft.UI.Xaml.Controls.RatingItemInfo"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.RatingItemFontInfo",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.RatingItemFontInfo",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.RatingItemInfo",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IRatingItemFontInfoFactory>(L"Microsoft.UI.Xaml.Controls.RatingItemFontInfo"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IRatingItemFontInfoStatics statics = GetFactory<winrt::IRatingItemFontInfoStatics>(L"Microsoft.UI.Xaml.Controls.RatingItemFontInfo");
                    {
                        xamlType.AddDPMember(L"DisabledGlyph", L"String", statics.DisabledGlyphProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Glyph", L"String", statics.GlyphProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PlaceholderGlyph", L"String", statics.PlaceholderGlyphProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PointerOverGlyph", L"String", statics.PointerOverGlyphProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PointerOverPlaceholderGlyph", L"String", statics.PointerOverPlaceholderGlyphProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"UnsetGlyph", L"String", statics.UnsetGlyphProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.RatingItemImageInfo",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.RatingItemImageInfo",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.RatingItemInfo",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IRatingItemImageInfoFactory>(L"Microsoft.UI.Xaml.Controls.RatingItemImageInfo"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IRatingItemImageInfoStatics statics = GetFactory<winrt::IRatingItemImageInfoStatics>(L"Microsoft.UI.Xaml.Controls.RatingItemImageInfo");
                    {
                        xamlType.AddDPMember(L"DisabledImage", L"Microsoft.UI.Xaml.Media.ImageSource", statics.DisabledImageProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Image", L"Microsoft.UI.Xaml.Media.ImageSource", statics.ImageProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PlaceholderImage", L"Microsoft.UI.Xaml.Media.ImageSource", statics.PlaceholderImageProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PointerOverImage", L"Microsoft.UI.Xaml.Media.ImageSource", statics.PointerOverImageProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PointerOverPlaceholderImage", L"Microsoft.UI.Xaml.Media.ImageSource", statics.PointerOverPlaceholderImageProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"UnsetImage", L"Microsoft.UI.Xaml.Media.ImageSource", statics.UnsetImageProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.RecyclePool",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.RecyclePool",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IRecyclePoolFactory>(L"Microsoft.UI.Xaml.Controls.RecyclePool"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IRecyclePoolStatics statics = GetFactory<winrt::IRecyclePoolStatics>(L"Microsoft.UI.Xaml.Controls.RecyclePool");
                    {
                        xamlType.AddDPMember(L"PoolInstance", L"Microsoft.UI.Xaml.Controls.RecyclePool", statics.PoolInstanceProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.RecyclingElementFactory",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.RecyclingElementFactory",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ElementFactory",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IRecyclingElementFactoryFactory>(L"Microsoft.UI.Xaml.Controls.RecyclingElementFactory"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"Templates", /* propertyName */
                        L"Windows.Foundation.Collections.IMap`2<String,Microsoft.UI.Xaml.DataTemplate>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::RecyclingElementFactory>().Templates(); },
                        [](winrt::IInspectable instance, winrt::IInspectable value) { instance.as<winrt::RecyclingElementFactory>().Templates(value.as<winrt::Windows::Foundation::Collections::IMap<winrt::hstring, winrt::Microsoft::UI::Xaml::DataTemplate>>()); },
                        true, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"RecyclePool", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.RecyclePool", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::RecyclingElementFactory>().RecyclePool(); },
                        [](winrt::IInspectable instance, winrt::IInspectable value) { instance.as<winrt::RecyclingElementFactory>().RecyclePool(value.as<winrt::Microsoft::UI::Xaml::Controls::RecyclePool>()); },
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.RefreshContainer",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.RefreshContainer",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ContentControl",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IRefreshContainerFactory>(L"Microsoft.UI.Xaml.Controls.RefreshContainer"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IRefreshContainerStatics statics = GetFactory<winrt::IRefreshContainerStatics>(L"Microsoft.UI.Xaml.Controls.RefreshContainer");
                    {
                        xamlType.AddDPMember(L"PullDirection", L"Microsoft.UI.Xaml.Controls.RefreshPullDirection", statics.PullDirectionProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Visualizer", L"Microsoft.UI.Xaml.Controls.RefreshVisualizer", statics.VisualizerProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.RefreshPullDirection",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.RefreshPullDirection",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"LeftToRight") return box_value(winrt::RefreshPullDirection::LeftToRight);
                    if (fromString == L"TopToBottom") return box_value(winrt::RefreshPullDirection::TopToBottom);
                    if (fromString == L"RightToLeft") return box_value(winrt::RefreshPullDirection::RightToLeft);
                    if (fromString == L"BottomToTop") return box_value(winrt::RefreshPullDirection::BottomToTop);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.RefreshVisualizer",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.RefreshVisualizer",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IRefreshVisualizerFactory>(L"Microsoft.UI.Xaml.Controls.RefreshVisualizer"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IRefreshVisualizerStatics statics = GetFactory<winrt::IRefreshVisualizerStatics>(L"Microsoft.UI.Xaml.Controls.RefreshVisualizer");
                    {
                        xamlType.AddDPMember(L"Content", L"Microsoft.UI.Xaml.UIElement", statics.ContentProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"InfoProvider", L"Object", statics.InfoProviderProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Orientation", L"Microsoft.UI.Xaml.Controls.RefreshVisualizerOrientation", statics.OrientationProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"State", L"Microsoft.UI.Xaml.Controls.RefreshVisualizerState", statics.StateProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.RefreshVisualizerOrientation",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.RefreshVisualizerOrientation",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Auto") return box_value(winrt::RefreshVisualizerOrientation::Auto);
                    if (fromString == L"Normal") return box_value(winrt::RefreshVisualizerOrientation::Normal);
                    if (fromString == L"Rotate90DegreesCounterclockwise") return box_value(winrt::RefreshVisualizerOrientation::Rotate90DegreesCounterclockwise);
                    if (fromString == L"Rotate270DegreesCounterclockwise") return box_value(winrt::RefreshVisualizerOrientation::Rotate270DegreesCounterclockwise);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.RefreshVisualizerState",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.RefreshVisualizerState",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Idle") return box_value(winrt::RefreshVisualizerState::Idle);
                    if (fromString == L"Peeking") return box_value(winrt::RefreshVisualizerState::Peeking);
                    if (fromString == L"Interacting") return box_value(winrt::RefreshVisualizerState::Interacting);
                    if (fromString == L"Pending") return box_value(winrt::RefreshVisualizerState::Pending);
                    if (fromString == L"Refreshing") return box_value(winrt::RefreshVisualizerState::Refreshing);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.RevealListViewItemPresenter",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.RevealListViewItemPresenter",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.ListViewItemPresenter",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IRevealListViewItemPresenterFactory>(L"Microsoft.UI.Xaml.Controls.RevealListViewItemPresenter"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ScrollView",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ScrollView",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IScrollViewFactory>(L"Microsoft.UI.Xaml.Controls.ScrollView"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IScrollViewStatics statics = GetFactory<winrt::IScrollViewStatics>(L"Microsoft.UI.Xaml.Controls.ScrollView");
                    {
                        xamlType.AddDPMember(L"ComputedHorizontalScrollBarVisibility", L"Microsoft.UI.Xaml.Visibility", statics.ComputedHorizontalScrollBarVisibilityProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ComputedHorizontalScrollMode", L"Microsoft.UI.Xaml.Controls.ScrollingScrollMode", statics.ComputedHorizontalScrollModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ComputedVerticalScrollBarVisibility", L"Microsoft.UI.Xaml.Visibility", statics.ComputedVerticalScrollBarVisibilityProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ComputedVerticalScrollMode", L"Microsoft.UI.Xaml.Controls.ScrollingScrollMode", statics.ComputedVerticalScrollModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ContentOrientation", L"Microsoft.UI.Xaml.Controls.ScrollingContentOrientation", statics.ContentOrientationProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Content", L"Microsoft.UI.Xaml.UIElement", statics.ContentProperty(), true /* isContent */);
                        xamlType.AddDPMember(L"HorizontalAnchorRatio", L"Double", statics.HorizontalAnchorRatioProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"HorizontalScrollBarVisibility", L"Microsoft.UI.Xaml.Controls.ScrollingScrollBarVisibility", statics.HorizontalScrollBarVisibilityProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"HorizontalScrollChainMode", L"Microsoft.UI.Xaml.Controls.ScrollingChainMode", statics.HorizontalScrollChainModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"HorizontalScrollMode", L"Microsoft.UI.Xaml.Controls.ScrollingScrollMode", statics.HorizontalScrollModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"HorizontalScrollRailMode", L"Microsoft.UI.Xaml.Controls.ScrollingRailMode", statics.HorizontalScrollRailModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IgnoredInputKinds", L"Microsoft.UI.Xaml.Controls.ScrollingInputKinds", statics.IgnoredInputKindsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MaxZoomFactor", L"Double", statics.MaxZoomFactorProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MinZoomFactor", L"Double", statics.MinZoomFactorProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ScrollPresenter", L"Microsoft.UI.Xaml.Controls.Primitives.ScrollPresenter", statics.ScrollPresenterProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"VerticalAnchorRatio", L"Double", statics.VerticalAnchorRatioProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"VerticalScrollBarVisibility", L"Microsoft.UI.Xaml.Controls.ScrollingScrollBarVisibility", statics.VerticalScrollBarVisibilityProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"VerticalScrollChainMode", L"Microsoft.UI.Xaml.Controls.ScrollingChainMode", statics.VerticalScrollChainModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"VerticalScrollMode", L"Microsoft.UI.Xaml.Controls.ScrollingScrollMode", statics.VerticalScrollModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"VerticalScrollRailMode", L"Microsoft.UI.Xaml.Controls.ScrollingRailMode", statics.VerticalScrollRailModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ZoomChainMode", L"Microsoft.UI.Xaml.Controls.ScrollingChainMode", statics.ZoomChainModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ZoomMode", L"Microsoft.UI.Xaml.Controls.ScrollingZoomMode", statics.ZoomModeProperty(), false /* isContent */);
                    }

                    xamlType.AddMember(
                        L"CurrentAnchor", /* propertyName */
                        L"Microsoft.UI.Xaml.UIElement", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::ScrollView>().CurrentAnchor(); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ExpressionAnimationSources", /* propertyName */
                        L"Microsoft.UI.Composition.CompositionPropertySet", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::ScrollView>().ExpressionAnimationSources(); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ExtentHeight", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ScrollView>().ExtentHeight()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ExtentWidth", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ScrollView>().ExtentWidth()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"HorizontalOffset", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ScrollView>().HorizontalOffset()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ScrollableHeight", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ScrollView>().ScrollableHeight()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ScrollableWidth", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ScrollView>().ScrollableWidth()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"State", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.ScrollingInteractionState", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value<int>((int)instance.as<winrt::ScrollView>().State()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"VerticalOffset", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ScrollView>().VerticalOffset()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ViewportHeight", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ScrollView>().ViewportHeight()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ViewportWidth", /* propertyName */
                        L"Double", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ScrollView>().ViewportWidth()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ZoomFactor", /* propertyName */
                        L"Single", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::ScrollView>().ZoomFactor()); },
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
        L"Microsoft.UI.Xaml.Controls.ScrollingAnimationMode",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ScrollingAnimationMode",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Disabled") return box_value(winrt::ScrollingAnimationMode::Disabled);
                    if (fromString == L"Enabled") return box_value(winrt::ScrollingAnimationMode::Enabled);
                    if (fromString == L"Auto") return box_value(winrt::ScrollingAnimationMode::Auto);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ScrollingChainMode",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ScrollingChainMode",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Auto") return box_value(winrt::ScrollingChainMode::Auto);
                    if (fromString == L"Always") return box_value(winrt::ScrollingChainMode::Always);
                    if (fromString == L"Never") return box_value(winrt::ScrollingChainMode::Never);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ScrollingContentOrientation",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ScrollingContentOrientation",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Vertical") return box_value(winrt::ScrollingContentOrientation::Vertical);
                    if (fromString == L"Horizontal") return box_value(winrt::ScrollingContentOrientation::Horizontal);
                    if (fromString == L"None") return box_value(winrt::ScrollingContentOrientation::None);
                    if (fromString == L"Both") return box_value(winrt::ScrollingContentOrientation::Both);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ScrollingInputKinds",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ScrollingInputKinds",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"None") return box_value(winrt::ScrollingInputKinds::None);
                    if (fromString == L"Touch") return box_value(winrt::ScrollingInputKinds::Touch);
                    if (fromString == L"Pen") return box_value(winrt::ScrollingInputKinds::Pen);
                    if (fromString == L"MouseWheel") return box_value(winrt::ScrollingInputKinds::MouseWheel);
                    if (fromString == L"Keyboard") return box_value(winrt::ScrollingInputKinds::Keyboard);
                    if (fromString == L"Gamepad") return box_value(winrt::ScrollingInputKinds::Gamepad);
                    if (fromString == L"All") return box_value(winrt::ScrollingInputKinds::All);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ScrollingInteractionState",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ScrollingInteractionState",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Idle") return box_value(winrt::ScrollingInteractionState::Idle);
                    if (fromString == L"Interaction") return box_value(winrt::ScrollingInteractionState::Interaction);
                    if (fromString == L"Inertia") return box_value(winrt::ScrollingInteractionState::Inertia);
                    if (fromString == L"Animation") return box_value(winrt::ScrollingInteractionState::Animation);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ScrollingRailMode",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ScrollingRailMode",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Enabled") return box_value(winrt::ScrollingRailMode::Enabled);
                    if (fromString == L"Disabled") return box_value(winrt::ScrollingRailMode::Disabled);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ScrollingScrollBarVisibility",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ScrollingScrollBarVisibility",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Auto") return box_value(winrt::ScrollingScrollBarVisibility::Auto);
                    if (fromString == L"Visible") return box_value(winrt::ScrollingScrollBarVisibility::Visible);
                    if (fromString == L"Hidden") return box_value(winrt::ScrollingScrollBarVisibility::Hidden);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ScrollingScrollMode",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ScrollingScrollMode",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Enabled") return box_value(winrt::ScrollingScrollMode::Enabled);
                    if (fromString == L"Disabled") return box_value(winrt::ScrollingScrollMode::Disabled);
                    if (fromString == L"Auto") return box_value(winrt::ScrollingScrollMode::Auto);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ScrollingScrollOptions",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ScrollingScrollOptions",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"SnapPointsMode", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.ScrollingSnapPointsMode", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value<int>((int)instance.as<winrt::ScrollingScrollOptions>().SnapPointsMode()); },
                        [](winrt::IInspectable instance, winrt::IInspectable value) { instance.as<winrt::ScrollingScrollOptions>().SnapPointsMode(unbox_value<winrt::ScrollingSnapPointsMode>(value)); },
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"AnimationMode", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.ScrollingAnimationMode", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value<int>((int)instance.as<winrt::ScrollingScrollOptions>().AnimationMode()); },
                        [](winrt::IInspectable instance, winrt::IInspectable value) { instance.as<winrt::ScrollingScrollOptions>().AnimationMode(unbox_value<winrt::ScrollingAnimationMode>(value)); },
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ScrollingSnapPointsMode",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ScrollingSnapPointsMode",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Default") return box_value(winrt::ScrollingSnapPointsMode::Default);
                    if (fromString == L"Ignore") return box_value(winrt::ScrollingSnapPointsMode::Ignore);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ScrollingZoomMode",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ScrollingZoomMode",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Enabled") return box_value(winrt::ScrollingZoomMode::Enabled);
                    if (fromString == L"Disabled") return box_value(winrt::ScrollingZoomMode::Disabled);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ScrollingZoomOptions",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ScrollingZoomOptions",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"SnapPointsMode", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.ScrollingSnapPointsMode", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value<int>((int)instance.as<winrt::ScrollingZoomOptions>().SnapPointsMode()); },
                        [](winrt::IInspectable instance, winrt::IInspectable value) { instance.as<winrt::ScrollingZoomOptions>().SnapPointsMode(unbox_value<winrt::ScrollingSnapPointsMode>(value)); },
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"AnimationMode", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.ScrollingAnimationMode", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value<int>((int)instance.as<winrt::ScrollingZoomOptions>().AnimationMode()); },
                        [](winrt::IInspectable instance, winrt::IInspectable value) { instance.as<winrt::ScrollingZoomOptions>().AnimationMode(unbox_value<winrt::ScrollingAnimationMode>(value)); },
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.SelectionModel",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.SelectionModel",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ISelectionModelFactory>(L"Microsoft.UI.Xaml.Controls.SelectionModel"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"Source", /* propertyName */
                        L"Object", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::SelectionModel>().Source(); },
                        [](winrt::IInspectable instance, winrt::IInspectable value) { instance.as<winrt::SelectionModel>().Source(value); },
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"SingleSelect", /* propertyName */
                        L"Boolean", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::SelectionModel>().SingleSelect()); },
                        [](winrt::IInspectable instance, winrt::IInspectable value) { instance.as<winrt::SelectionModel>().SingleSelect(unbox_value<bool>(value)); },
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"SelectedIndex", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.IndexPath", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::SelectionModel>().SelectedIndex(); },
                        [](winrt::IInspectable instance, winrt::IInspectable value) { instance.as<winrt::SelectionModel>().SelectedIndex(value.as<winrt::Microsoft::UI::Xaml::Controls::IndexPath>()); },
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"AnchorIndex", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.IndexPath", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::SelectionModel>().AnchorIndex(); },
                        [](winrt::IInspectable instance, winrt::IInspectable value) { instance.as<winrt::SelectionModel>().AnchorIndex(value.as<winrt::Microsoft::UI::Xaml::Controls::IndexPath>()); },
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"SelectedIndices", /* propertyName */
                        L"Windows.Foundation.Collections.IReadOnlyList`1<Microsoft.UI.Xaml.Controls.IndexPath>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::SelectionModel>().SelectedIndices(); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"SelectedItem", /* propertyName */
                        L"Object", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::SelectionModel>().SelectedItem(); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"SelectedItems", /* propertyName */
                        L"Windows.Foundation.Collections.IReadOnlyList`1<Object>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::SelectionModel>().SelectedItems(); },
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
        L"Microsoft.UI.Xaml.Controls.SelectorBar",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.SelectorBar",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ISelectorBarFactory>(L"Microsoft.UI.Xaml.Controls.SelectorBar"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ISelectorBarStatics statics = GetFactory<winrt::ISelectorBarStatics>(L"Microsoft.UI.Xaml.Controls.SelectorBar");
                    {
                        xamlType.AddDPMember(L"Items", L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.SelectorBarItem>", statics.ItemsProperty(), true /* isContent */);
                        xamlType.AddDPMember(L"SelectedItem", L"Microsoft.UI.Xaml.Controls.SelectorBarItem", statics.SelectedItemProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.SelectorBarItem",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.SelectorBarItem",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ItemContainer",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ISelectorBarItemFactory>(L"Microsoft.UI.Xaml.Controls.SelectorBarItem"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ISelectorBarItemStatics statics = GetFactory<winrt::ISelectorBarItemStatics>(L"Microsoft.UI.Xaml.Controls.SelectorBarItem");
                    {
                        xamlType.AddDPMember(L"Icon", L"Microsoft.UI.Xaml.Controls.IconElement", statics.IconProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Text", L"String", statics.TextProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.SplitButton",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.SplitButton",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ContentControl",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ISplitButtonFactory>(L"Microsoft.UI.Xaml.Controls.SplitButton"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ISplitButtonStatics statics = GetFactory<winrt::ISplitButtonStatics>(L"Microsoft.UI.Xaml.Controls.SplitButton");
                    {
                        xamlType.AddDPMember(L"CommandParameter", L"Object", statics.CommandParameterProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Command", L"Microsoft.UI.Xaml.Input.ICommand", statics.CommandProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Flyout", L"Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase", statics.FlyoutProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.StackLayout",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.StackLayout",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.VirtualizingLayout",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IStackLayoutFactory>(L"Microsoft.UI.Xaml.Controls.StackLayout"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IStackLayoutStatics statics = GetFactory<winrt::IStackLayoutStatics>(L"Microsoft.UI.Xaml.Controls.StackLayout");
                    {
                        xamlType.AddDPMember(L"Orientation", L"Microsoft.UI.Xaml.Controls.Orientation", statics.OrientationProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Spacing", L"Double", statics.SpacingProperty(), false /* isContent */);
                    }

                    winrt::IStackLayoutStatics2 statics2 = GetFactory<winrt::IStackLayoutStatics2>(L"Microsoft.UI.Xaml.Controls.StackLayout");
                    {
                        xamlType.AddDPMember(L"IsVirtualizationEnabled", L"Boolean", statics2.IsVirtualizationEnabledProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.StackLayoutState",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.StackLayoutState",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IStackLayoutStateFactory>(L"Microsoft.UI.Xaml.Controls.StackLayoutState"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.SwipeBehaviorOnInvoked",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.SwipeBehaviorOnInvoked",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Auto") return box_value(winrt::SwipeBehaviorOnInvoked::Auto);
                    if (fromString == L"Close") return box_value(winrt::SwipeBehaviorOnInvoked::Close);
                    if (fromString == L"RemainOpen") return box_value(winrt::SwipeBehaviorOnInvoked::RemainOpen);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.SwipeControl",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.SwipeControl",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ContentControl",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ISwipeControlFactory>(L"Microsoft.UI.Xaml.Controls.SwipeControl"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ISwipeControlStatics statics = GetFactory<winrt::ISwipeControlStatics>(L"Microsoft.UI.Xaml.Controls.SwipeControl");
                    {
                        xamlType.AddDPMember(L"BottomItems", L"Microsoft.UI.Xaml.Controls.SwipeItems", statics.BottomItemsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"LeftItems", L"Microsoft.UI.Xaml.Controls.SwipeItems", statics.LeftItemsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"RightItems", L"Microsoft.UI.Xaml.Controls.SwipeItems", statics.RightItemsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TopItems", L"Microsoft.UI.Xaml.Controls.SwipeItems", statics.TopItemsProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.SwipeItem",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.SwipeItem",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ISwipeItemFactory>(L"Microsoft.UI.Xaml.Controls.SwipeItem"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ISwipeItemStatics statics = GetFactory<winrt::ISwipeItemStatics>(L"Microsoft.UI.Xaml.Controls.SwipeItem");
                    {
                        xamlType.AddDPMember(L"Background", L"Microsoft.UI.Xaml.Media.Brush", statics.BackgroundProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"BehaviorOnInvoked", L"Microsoft.UI.Xaml.Controls.SwipeBehaviorOnInvoked", statics.BehaviorOnInvokedProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"CommandParameter", L"Object", statics.CommandParameterProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Command", L"Microsoft.UI.Xaml.Input.ICommand", statics.CommandProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Foreground", L"Microsoft.UI.Xaml.Media.Brush", statics.ForegroundProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IconSource", L"Microsoft.UI.Xaml.Controls.IconSource", statics.IconSourceProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Text", L"String", statics.TextProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.SwipeItems",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.SwipeItems",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ISwipeItemsFactory>(L"Microsoft.UI.Xaml.Controls.SwipeItems"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ISwipeItemsStatics statics = GetFactory<winrt::ISwipeItemsStatics>(L"Microsoft.UI.Xaml.Controls.SwipeItems");
                    {
                        xamlType.AddDPMember(L"Mode", L"Microsoft.UI.Xaml.Controls.SwipeMode", statics.ModeProperty(), false /* isContent */);
                    }

                });

            xamlType->SetCollectionAddFunc((std::function<void(winrt::IInspectable const&, winrt::IInspectable const&)>)[](winrt::IInspectable const& collection, winrt::IInspectable const& value)
            {
                collection.as<winrt::Microsoft::UI::Xaml::Controls::SwipeItems>().Append(unbox_value<winrt::Microsoft::UI::Xaml::Controls::SwipeItem>(value));
            });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.SwipeMode",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.SwipeMode",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Reveal") return box_value(winrt::SwipeMode::Reveal);
                    if (fromString == L"Execute") return box_value(winrt::SwipeMode::Execute);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.TabView",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.TabView",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ITabViewFactory>(L"Microsoft.UI.Xaml.Controls.TabView"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ITabViewStatics statics = GetFactory<winrt::ITabViewStatics>(L"Microsoft.UI.Xaml.Controls.TabView");
                    {
                        xamlType.AddDPMember(L"AddTabButtonCommandParameter", L"Object", statics.AddTabButtonCommandParameterProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"AddTabButtonCommand", L"Microsoft.UI.Xaml.Input.ICommand", statics.AddTabButtonCommandProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"AllowDropTabs", L"Boolean", statics.AllowDropTabsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"CanDragTabs", L"Boolean", statics.CanDragTabsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"CanReorderTabs", L"Boolean", statics.CanReorderTabsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"CloseButtonOverlayMode", L"Microsoft.UI.Xaml.Controls.TabViewCloseButtonOverlayMode", statics.CloseButtonOverlayModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsAddTabButtonVisible", L"Boolean", statics.IsAddTabButtonVisibleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SelectedIndex", L"Int32", statics.SelectedIndexProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SelectedItem", L"Object", statics.SelectedItemProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TabItemTemplate", L"Microsoft.UI.Xaml.DataTemplate", statics.TabItemTemplateProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TabItemTemplateSelector", L"Microsoft.UI.Xaml.Controls.DataTemplateSelector", statics.TabItemTemplateSelectorProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TabItems", L"Windows.Foundation.Collections.IVector`1<Object>", statics.TabItemsProperty(), true /* isContent */);
                        xamlType.AddDPMember(L"TabItemsSource", L"Object", statics.TabItemsSourceProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TabStripFooter", L"Object", statics.TabStripFooterProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TabStripFooterTemplate", L"Microsoft.UI.Xaml.DataTemplate", statics.TabStripFooterTemplateProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TabStripHeader", L"Object", statics.TabStripHeaderProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TabStripHeaderTemplate", L"Microsoft.UI.Xaml.DataTemplate", statics.TabStripHeaderTemplateProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TabWidthMode", L"Microsoft.UI.Xaml.Controls.TabViewWidthMode", statics.TabWidthModeProperty(), false /* isContent */);
                    }

                    winrt::ITabViewStatics2 statics2 = GetFactory<winrt::ITabViewStatics2>(L"Microsoft.UI.Xaml.Controls.TabView");
                    {
                        xamlType.AddDPMember(L"CanTearOutTabs", L"Boolean", statics2.CanTearOutTabsProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.TabViewCloseButtonOverlayMode",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.TabViewCloseButtonOverlayMode",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Auto") return box_value(winrt::TabViewCloseButtonOverlayMode::Auto);
                    if (fromString == L"OnPointerOver") return box_value(winrt::TabViewCloseButtonOverlayMode::OnPointerOver);
                    if (fromString == L"Always") return box_value(winrt::TabViewCloseButtonOverlayMode::Always);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.TabViewItem",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.TabViewItem",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ListViewItem",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ITabViewItemFactory>(L"Microsoft.UI.Xaml.Controls.TabViewItem"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ITabViewItemStatics statics = GetFactory<winrt::ITabViewItemStatics>(L"Microsoft.UI.Xaml.Controls.TabViewItem");
                    {
                        xamlType.AddDPMember(L"Header", L"Object", statics.HeaderProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"HeaderTemplate", L"Microsoft.UI.Xaml.DataTemplate", statics.HeaderTemplateProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IconSource", L"Microsoft.UI.Xaml.Controls.IconSource", statics.IconSourceProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsClosable", L"Boolean", statics.IsClosableProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TabViewTemplateSettings", L"Microsoft.UI.Xaml.Controls.TabViewItemTemplateSettings", statics.TabViewTemplateSettingsProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.TabViewItemTemplateSettings",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.TabViewItemTemplateSettings",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ITabViewItemTemplateSettingsFactory>(L"Microsoft.UI.Xaml.Controls.TabViewItemTemplateSettings"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ITabViewItemTemplateSettingsStatics statics = GetFactory<winrt::ITabViewItemTemplateSettingsStatics>(L"Microsoft.UI.Xaml.Controls.TabViewItemTemplateSettings");
                    {
                        xamlType.AddDPMember(L"IconElement", L"Microsoft.UI.Xaml.Controls.IconElement", statics.IconElementProperty(), false /* isContent */);
                    }

                    winrt::ITabViewItemTemplateSettingsStatics2 statics2 = GetFactory<winrt::ITabViewItemTemplateSettingsStatics2>(L"Microsoft.UI.Xaml.Controls.TabViewItemTemplateSettings");
                    {
                        xamlType.AddDPMember(L"TabGeometry", L"Microsoft.UI.Xaml.Media.Geometry", statics2.TabGeometryProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.TabViewWidthMode",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.TabViewWidthMode",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Equal") return box_value(winrt::TabViewWidthMode::Equal);
                    if (fromString == L"SizeToContent") return box_value(winrt::TabViewWidthMode::SizeToContent);
                    if (fromString == L"Compact") return box_value(winrt::TabViewWidthMode::Compact);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.TeachingTip",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.TeachingTip",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ContentControl",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ITeachingTipFactory>(L"Microsoft.UI.Xaml.Controls.TeachingTip"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ITeachingTipStatics statics = GetFactory<winrt::ITeachingTipStatics>(L"Microsoft.UI.Xaml.Controls.TeachingTip");
                    {
                        xamlType.AddDPMember(L"ActionButtonCommandParameter", L"Object", statics.ActionButtonCommandParameterProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ActionButtonCommand", L"Microsoft.UI.Xaml.Input.ICommand", statics.ActionButtonCommandProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ActionButtonContent", L"Object", statics.ActionButtonContentProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ActionButtonStyle", L"Microsoft.UI.Xaml.Style", statics.ActionButtonStyleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"CloseButtonCommandParameter", L"Object", statics.CloseButtonCommandParameterProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"CloseButtonCommand", L"Microsoft.UI.Xaml.Input.ICommand", statics.CloseButtonCommandProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"CloseButtonContent", L"Object", statics.CloseButtonContentProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"CloseButtonStyle", L"Microsoft.UI.Xaml.Style", statics.CloseButtonStyleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"HeroContentPlacement", L"Microsoft.UI.Xaml.Controls.TeachingTipHeroContentPlacementMode", statics.HeroContentPlacementProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"HeroContent", L"Microsoft.UI.Xaml.UIElement", statics.HeroContentProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IconSource", L"Microsoft.UI.Xaml.Controls.IconSource", statics.IconSourceProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsLightDismissEnabled", L"Boolean", statics.IsLightDismissEnabledProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsOpen", L"Boolean", statics.IsOpenProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PlacementMargin", L"Microsoft.UI.Xaml.Thickness", statics.PlacementMarginProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"PreferredPlacement", L"Microsoft.UI.Xaml.Controls.TeachingTipPlacementMode", statics.PreferredPlacementProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ShouldConstrainToRootBounds", L"Boolean", statics.ShouldConstrainToRootBoundsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Subtitle", L"String", statics.SubtitleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TailVisibility", L"Microsoft.UI.Xaml.Controls.TeachingTipTailVisibility", statics.TailVisibilityProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Target", L"Microsoft.UI.Xaml.FrameworkElement", statics.TargetProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TemplateSettings", L"Microsoft.UI.Xaml.Controls.TeachingTipTemplateSettings", statics.TemplateSettingsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Title", L"String", statics.TitleProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.TeachingTipCloseReason",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.TeachingTipCloseReason",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"CloseButton") return box_value(winrt::TeachingTipCloseReason::CloseButton);
                    if (fromString == L"LightDismiss") return box_value(winrt::TeachingTipCloseReason::LightDismiss);
                    if (fromString == L"Programmatic") return box_value(winrt::TeachingTipCloseReason::Programmatic);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.TeachingTipHeroContentPlacementMode",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.TeachingTipHeroContentPlacementMode",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Auto") return box_value(winrt::TeachingTipHeroContentPlacementMode::Auto);
                    if (fromString == L"Top") return box_value(winrt::TeachingTipHeroContentPlacementMode::Top);
                    if (fromString == L"Bottom") return box_value(winrt::TeachingTipHeroContentPlacementMode::Bottom);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.TeachingTipPlacementMode",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.TeachingTipPlacementMode",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Auto") return box_value(winrt::TeachingTipPlacementMode::Auto);
                    if (fromString == L"Top") return box_value(winrt::TeachingTipPlacementMode::Top);
                    if (fromString == L"Bottom") return box_value(winrt::TeachingTipPlacementMode::Bottom);
                    if (fromString == L"Left") return box_value(winrt::TeachingTipPlacementMode::Left);
                    if (fromString == L"Right") return box_value(winrt::TeachingTipPlacementMode::Right);
                    if (fromString == L"TopRight") return box_value(winrt::TeachingTipPlacementMode::TopRight);
                    if (fromString == L"TopLeft") return box_value(winrt::TeachingTipPlacementMode::TopLeft);
                    if (fromString == L"BottomRight") return box_value(winrt::TeachingTipPlacementMode::BottomRight);
                    if (fromString == L"BottomLeft") return box_value(winrt::TeachingTipPlacementMode::BottomLeft);
                    if (fromString == L"LeftTop") return box_value(winrt::TeachingTipPlacementMode::LeftTop);
                    if (fromString == L"LeftBottom") return box_value(winrt::TeachingTipPlacementMode::LeftBottom);
                    if (fromString == L"RightTop") return box_value(winrt::TeachingTipPlacementMode::RightTop);
                    if (fromString == L"RightBottom") return box_value(winrt::TeachingTipPlacementMode::RightBottom);
                    if (fromString == L"Center") return box_value(winrt::TeachingTipPlacementMode::Center);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.TeachingTipTailVisibility",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.TeachingTipTailVisibility",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Auto") return box_value(winrt::TeachingTipTailVisibility::Auto);
                    if (fromString == L"Visible") return box_value(winrt::TeachingTipTailVisibility::Visible);
                    if (fromString == L"Collapsed") return box_value(winrt::TeachingTipTailVisibility::Collapsed);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.TeachingTipTemplateSettings",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.TeachingTipTemplateSettings",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ITeachingTipTemplateSettingsFactory>(L"Microsoft.UI.Xaml.Controls.TeachingTipTemplateSettings"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ITeachingTipTemplateSettingsStatics statics = GetFactory<winrt::ITeachingTipTemplateSettingsStatics>(L"Microsoft.UI.Xaml.Controls.TeachingTipTemplateSettings");
                    {
                        xamlType.AddDPMember(L"IconElement", L"Microsoft.UI.Xaml.Controls.IconElement", statics.IconElementProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TopLeftHighlightMargin", L"Microsoft.UI.Xaml.Thickness", statics.TopLeftHighlightMarginProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TopRightHighlightMargin", L"Microsoft.UI.Xaml.Thickness", statics.TopRightHighlightMarginProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.TextCommandBarFlyout",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.TextCommandBarFlyout",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.CommandBarFlyout",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ITextCommandBarFlyoutFactory>(L"Microsoft.UI.Xaml.Controls.TextCommandBarFlyout"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.TitleBar",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.TitleBar",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ITitleBarFactory>(L"Microsoft.UI.Xaml.Controls.TitleBar"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ITitleBarStatics statics = GetFactory<winrt::ITitleBarStatics>(L"Microsoft.UI.Xaml.Controls.TitleBar");
                    {
                        xamlType.AddDPMember(L"Content", L"Object", statics.ContentProperty(), true /* isContent */);
                        xamlType.AddDPMember(L"Footer", L"Object", statics.FooterProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Header", L"Object", statics.HeaderProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IconSource", L"Microsoft.UI.Xaml.Controls.IconSource", statics.IconSourceProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsBackButtonVisible", L"Boolean", statics.IsBackButtonVisibleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsBackEnabled", L"Boolean", statics.IsBackEnabledProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsPaneToggleButtonVisible", L"Boolean", statics.IsPaneToggleButtonVisibleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Subtitle", L"String", statics.SubtitleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TemplateSettings", L"Microsoft.UI.Xaml.Controls.TitleBarTemplateSettings", statics.TemplateSettingsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Title", L"String", statics.TitleProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.TitleBarTemplateSettings",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.TitleBarTemplateSettings",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ITitleBarTemplateSettingsFactory>(L"Microsoft.UI.Xaml.Controls.TitleBarTemplateSettings"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ITitleBarTemplateSettingsStatics statics = GetFactory<winrt::ITitleBarTemplateSettingsStatics>(L"Microsoft.UI.Xaml.Controls.TitleBarTemplateSettings");
                    {
                        xamlType.AddDPMember(L"IconElement", L"Microsoft.UI.Xaml.Controls.IconElement", statics.IconElementProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ToggleSplitButton",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ToggleSplitButton",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.SplitButton",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IToggleSplitButtonFactory>(L"Microsoft.UI.Xaml.Controls.ToggleSplitButton"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IToggleSplitButtonStatics statics = GetFactory<winrt::IToggleSplitButtonStatics>(L"Microsoft.UI.Xaml.Controls.ToggleSplitButton");
                    {
                        xamlType.AddDPMember(L"IsChecked", L"Boolean", statics.IsCheckedProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.TreeView",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.TreeView",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ITreeViewFactory>(L"Microsoft.UI.Xaml.Controls.TreeView"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ITreeViewStatics statics = GetFactory<winrt::ITreeViewStatics>(L"Microsoft.UI.Xaml.Controls.TreeView");
                    {
                        xamlType.AddDPMember(L"SelectedItem", L"Object", statics.SelectedItemProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SelectionMode", L"Microsoft.UI.Xaml.Controls.TreeViewSelectionMode", statics.SelectionModeProperty(), false /* isContent */);
                    }

                    winrt::ITreeViewStatics2 statics2 = GetFactory<winrt::ITreeViewStatics2>(L"Microsoft.UI.Xaml.Controls.TreeView");
                    {
                        xamlType.AddDPMember(L"CanDragItems", L"Boolean", statics2.CanDragItemsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"CanReorderItems", L"Boolean", statics2.CanReorderItemsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ItemContainerStyle", L"Microsoft.UI.Xaml.Style", statics2.ItemContainerStyleProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ItemContainerStyleSelector", L"Microsoft.UI.Xaml.Controls.StyleSelector", statics2.ItemContainerStyleSelectorProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ItemContainerTransitions", L"Microsoft.UI.Xaml.Media.Animation.TransitionCollection", statics2.ItemContainerTransitionsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ItemTemplate", L"Microsoft.UI.Xaml.DataTemplate", statics2.ItemTemplateProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ItemTemplateSelector", L"Microsoft.UI.Xaml.Controls.DataTemplateSelector", statics2.ItemTemplateSelectorProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ItemsSource", L"Object", statics2.ItemsSourceProperty(), false /* isContent */);
                    }

                    xamlType.AddMember(
                        L"RootNodes", /* propertyName */
                        L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.TreeViewNode>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::TreeView>().RootNodes(); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"SelectedNodes", /* propertyName */
                        L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.TreeViewNode>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::TreeView>().SelectedNodes(); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"SelectedNode", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.TreeViewNode", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::TreeView>().SelectedNode(); },
                        [](winrt::IInspectable instance, winrt::IInspectable value) { instance.as<winrt::TreeView>().SelectedNode(value.as<winrt::Microsoft::UI::Xaml::Controls::TreeViewNode>()); },
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"SelectedItems", /* propertyName */
                        L"Windows.Foundation.Collections.IVector`1<Object>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::TreeView>().SelectedItems(); },
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
        L"Microsoft.UI.Xaml.Controls.TreeViewItem",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.TreeViewItem",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ListViewItem",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ITreeViewItemFactory>(L"Microsoft.UI.Xaml.Controls.TreeViewItem"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ITreeViewItemStatics statics = GetFactory<winrt::ITreeViewItemStatics>(L"Microsoft.UI.Xaml.Controls.TreeViewItem");
                    {
                        xamlType.AddDPMember(L"CollapsedGlyph", L"String", statics.CollapsedGlyphProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ExpandedGlyph", L"String", statics.ExpandedGlyphProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"GlyphBrush", L"Microsoft.UI.Xaml.Media.Brush", statics.GlyphBrushProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"GlyphOpacity", L"Double", statics.GlyphOpacityProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"GlyphSize", L"Double", statics.GlyphSizeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsExpanded", L"Boolean", statics.IsExpandedProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TreeViewItemTemplateSettings", L"Microsoft.UI.Xaml.Controls.TreeViewItemTemplateSettings", statics.TreeViewItemTemplateSettingsProperty(), false /* isContent */);
                    }

                    winrt::ITreeViewItemStatics2 statics2 = GetFactory<winrt::ITreeViewItemStatics2>(L"Microsoft.UI.Xaml.Controls.TreeViewItem");
                    {
                        xamlType.AddDPMember(L"HasUnrealizedChildren", L"Boolean", statics2.HasUnrealizedChildrenProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ItemsSource", L"Object", statics2.ItemsSourceProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.TreeViewItemTemplateSettings",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.TreeViewItemTemplateSettings",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ITreeViewItemTemplateSettingsFactory>(L"Microsoft.UI.Xaml.Controls.TreeViewItemTemplateSettings"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ITreeViewItemTemplateSettingsStatics statics = GetFactory<winrt::ITreeViewItemTemplateSettingsStatics>(L"Microsoft.UI.Xaml.Controls.TreeViewItemTemplateSettings");
                    {
                        xamlType.AddDPMember(L"CollapsedGlyphVisibility", L"Microsoft.UI.Xaml.Visibility", statics.CollapsedGlyphVisibilityProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"DragItemsCount", L"Int32", statics.DragItemsCountProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ExpandedGlyphVisibility", L"Microsoft.UI.Xaml.Visibility", statics.ExpandedGlyphVisibilityProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Indentation", L"Microsoft.UI.Xaml.Thickness", statics.IndentationProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.TreeViewList",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.TreeViewList",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.ListView",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ITreeViewListFactory>(L"Microsoft.UI.Xaml.Controls.TreeViewList"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.TreeViewNode",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.TreeViewNode",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.DependencyObject",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ITreeViewNodeFactory>(L"Microsoft.UI.Xaml.Controls.TreeViewNode"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ITreeViewNodeStatics statics = GetFactory<winrt::ITreeViewNodeStatics>(L"Microsoft.UI.Xaml.Controls.TreeViewNode");
                    {
                        xamlType.AddDPMember(L"Content", L"Object", statics.ContentProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Depth", L"Int32", statics.DepthProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"HasChildren", L"Boolean", statics.HasChildrenProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"IsExpanded", L"Boolean", statics.IsExpandedProperty(), false /* isContent */);
                    }

                    xamlType.AddMember(
                        L"HasUnrealizedChildren", /* propertyName */
                        L"Boolean", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::TreeViewNode>().HasUnrealizedChildren()); },
                        [](winrt::IInspectable instance, winrt::IInspectable value) { instance.as<winrt::TreeViewNode>().HasUnrealizedChildren(unbox_value<bool>(value)); },
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"Children", /* propertyName */
                        L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.TreeViewNode>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::TreeViewNode>().Children(); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"Parent", /* propertyName */
                        L"Microsoft.UI.Xaml.Controls.TreeViewNode", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::TreeViewNode>().Parent(); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                });

            xamlType->SetIsBindable(true);

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.TreeViewSelectionMode",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.TreeViewSelectionMode",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"None") return box_value(winrt::TreeViewSelectionMode::None);
                    if (fromString == L"Single") return box_value(winrt::TreeViewSelectionMode::Single);
                    if (fromString == L"Multiple") return box_value(winrt::TreeViewSelectionMode::Multiple);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.TwoPaneView",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.TwoPaneView",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.Control",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::ITwoPaneViewFactory>(L"Microsoft.UI.Xaml.Controls.TwoPaneView"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::ITwoPaneViewStatics statics = GetFactory<winrt::ITwoPaneViewStatics>(L"Microsoft.UI.Xaml.Controls.TwoPaneView");
                    {
                        xamlType.AddDPMember(L"MinTallModeHeight", L"Double", statics.MinTallModeHeightProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MinWideModeWidth", L"Double", statics.MinWideModeWidthProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Mode", L"Microsoft.UI.Xaml.Controls.TwoPaneViewMode", statics.ModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Pane1Length", L"Microsoft.UI.Xaml.GridLength", statics.Pane1LengthProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Pane1", L"Microsoft.UI.Xaml.UIElement", statics.Pane1Property(), false /* isContent */);
                        xamlType.AddDPMember(L"Pane2Length", L"Microsoft.UI.Xaml.GridLength", statics.Pane2LengthProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Pane2", L"Microsoft.UI.Xaml.UIElement", statics.Pane2Property(), false /* isContent */);
                        xamlType.AddDPMember(L"PanePriority", L"Microsoft.UI.Xaml.Controls.TwoPaneViewPriority", statics.PanePriorityProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TallModeConfiguration", L"Microsoft.UI.Xaml.Controls.TwoPaneViewTallModeConfiguration", statics.TallModeConfigurationProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"WideModeConfiguration", L"Microsoft.UI.Xaml.Controls.TwoPaneViewWideModeConfiguration", statics.WideModeConfigurationProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.TwoPaneViewMode",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.TwoPaneViewMode",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"SinglePane") return box_value(winrt::TwoPaneViewMode::SinglePane);
                    if (fromString == L"Wide") return box_value(winrt::TwoPaneViewMode::Wide);
                    if (fromString == L"Tall") return box_value(winrt::TwoPaneViewMode::Tall);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.TwoPaneViewPriority",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.TwoPaneViewPriority",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Pane1") return box_value(winrt::TwoPaneViewPriority::Pane1);
                    if (fromString == L"Pane2") return box_value(winrt::TwoPaneViewPriority::Pane2);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.TwoPaneViewTallModeConfiguration",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.TwoPaneViewTallModeConfiguration",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"SinglePane") return box_value(winrt::TwoPaneViewTallModeConfiguration::SinglePane);
                    if (fromString == L"TopBottom") return box_value(winrt::TwoPaneViewTallModeConfiguration::TopBottom);
                    if (fromString == L"BottomTop") return box_value(winrt::TwoPaneViewTallModeConfiguration::BottomTop);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.TwoPaneViewWideModeConfiguration",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.TwoPaneViewWideModeConfiguration",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"SinglePane") return box_value(winrt::TwoPaneViewWideModeConfiguration::SinglePane);
                    if (fromString == L"LeftRight") return box_value(winrt::TwoPaneViewWideModeConfiguration::LeftRight);
                    if (fromString == L"RightLeft") return box_value(winrt::TwoPaneViewWideModeConfiguration::RightLeft);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.UniformGridLayout",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.UniformGridLayout",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.VirtualizingLayout",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IUniformGridLayoutFactory>(L"Microsoft.UI.Xaml.Controls.UniformGridLayout"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IUniformGridLayoutStatics statics = GetFactory<winrt::IUniformGridLayoutStatics>(L"Microsoft.UI.Xaml.Controls.UniformGridLayout");
                    {
                        xamlType.AddDPMember(L"ItemsJustification", L"Microsoft.UI.Xaml.Controls.UniformGridLayoutItemsJustification", statics.ItemsJustificationProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"ItemsStretch", L"Microsoft.UI.Xaml.Controls.UniformGridLayoutItemsStretch", statics.ItemsStretchProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MaximumRowsOrColumns", L"Int32", statics.MaximumRowsOrColumnsProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MinColumnSpacing", L"Double", statics.MinColumnSpacingProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MinItemHeight", L"Double", statics.MinItemHeightProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MinItemWidth", L"Double", statics.MinItemWidthProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MinRowSpacing", L"Double", statics.MinRowSpacingProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Orientation", L"Microsoft.UI.Xaml.Controls.Orientation", statics.OrientationProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.UniformGridLayoutItemsJustification",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.UniformGridLayoutItemsJustification",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Start") return box_value(winrt::UniformGridLayoutItemsJustification::Start);
                    if (fromString == L"Center") return box_value(winrt::UniformGridLayoutItemsJustification::Center);
                    if (fromString == L"End") return box_value(winrt::UniformGridLayoutItemsJustification::End);
                    if (fromString == L"SpaceAround") return box_value(winrt::UniformGridLayoutItemsJustification::SpaceAround);
                    if (fromString == L"SpaceBetween") return box_value(winrt::UniformGridLayoutItemsJustification::SpaceBetween);
                    if (fromString == L"SpaceEvenly") return box_value(winrt::UniformGridLayoutItemsJustification::SpaceEvenly);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.UniformGridLayoutItemsStretch",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.UniformGridLayoutItemsStretch",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"None") return box_value(winrt::UniformGridLayoutItemsStretch::None);
                    if (fromString == L"Fill") return box_value(winrt::UniformGridLayoutItemsStretch::Fill);
                    if (fromString == L"Uniform") return box_value(winrt::UniformGridLayoutItemsStretch::Uniform);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.UniformGridLayoutState",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.UniformGridLayoutState",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IUniformGridLayoutStateFactory>(L"Microsoft.UI.Xaml.Controls.UniformGridLayoutState"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.VirtualizingLayoutContext",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.VirtualizingLayoutContext",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.LayoutContext",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IVirtualizingLayoutContextFactory>(L"Microsoft.UI.Xaml.Controls.VirtualizingLayoutContext"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    xamlType.AddMember(
                        L"LayoutOrigin", /* propertyName */
                        L"Windows.Foundation.Point", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::VirtualizingLayoutContext>().LayoutOrigin()); },
                        [](winrt::IInspectable instance, winrt::IInspectable value) { instance.as<winrt::VirtualizingLayoutContext>().LayoutOrigin(unbox_value<winrt::Point>(value)); },
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"ItemCount", /* propertyName */
                        L"Int32", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::VirtualizingLayoutContext>().ItemCount()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"RealizationRect", /* propertyName */
                        L"Windows.Foundation.Rect", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::VirtualizingLayoutContext>().RealizationRect()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"RecommendedAnchorIndex", /* propertyName */
                        L"Int32", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::VirtualizingLayoutContext>().RecommendedAnchorIndex()); },
                        nullptr, /* setter */
                        false, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                    xamlType.AddMember(
                        L"VisibleRect", /* propertyName */
                        L"Windows.Foundation.Rect", /* propertyType */
                        [](winrt::IInspectable instance) { return box_value(instance.as<winrt::VirtualizingLayoutContext>().VisibleRect()); },
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
        L"Microsoft.UI.Xaml.Controls.WebView2",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.WebView2",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.FrameworkElement",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IWebView2Factory>(L"Microsoft.UI.Xaml.Controls.WebView2"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IWebView2Statics statics = GetFactory<winrt::IWebView2Statics>(L"Microsoft.UI.Xaml.Controls.WebView2");
                    {
                        xamlType.AddDPMember(L"CanGoBack", L"Boolean", statics.CanGoBackProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"CanGoForward", L"Boolean", statics.CanGoForwardProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"DefaultBackgroundColor", L"Windows.UI.Color", statics.DefaultBackgroundColorProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Source", L"Uri", statics.SourceProperty(), false /* isContent */);
                    }

                    xamlType.AddMember(
                        L"CoreWebView2", /* propertyName */
                        L"Microsoft.Web.WebView2.Core.CoreWebView2", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::WebView2>().CoreWebView2(); },
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
        L"Microsoft.UI.Xaml.Controls.XamlControlsResources",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Controls.XamlControlsResources",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.ResourceDictionary",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstance(L"Microsoft.UI.Xaml.Controls.XamlControlsResources"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IXamlControlsResourcesStatics statics = GetFactory<winrt::IXamlControlsResourcesStatics>(L"Microsoft.UI.Xaml.Controls.XamlControlsResources");
                    {
                        xamlType.AddDPMember(L"UseCompactResources", L"Boolean", statics.UseCompactResourcesProperty(), false /* isContent */);
                    }

                });

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
        L"Microsoft.UI.Xaml.Media.AcrylicBrush",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Media.AcrylicBrush",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Media.XamlCompositionBrushBase",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IAcrylicBrushFactory>(L"Microsoft.UI.Xaml.Media.AcrylicBrush"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IAcrylicBrushStatics statics = GetFactory<winrt::IAcrylicBrushStatics>(L"Microsoft.UI.Xaml.Media.AcrylicBrush");
                    {
                        xamlType.AddDPMember(L"AlwaysUseFallback", L"Boolean", statics.AlwaysUseFallbackProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TintColor", L"Windows.UI.Color", statics.TintColorProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TintOpacity", L"Double", statics.TintOpacityProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TintTransitionDuration", L"TimeSpan", statics.TintTransitionDurationProperty(), false /* isContent */);
                    }

                    winrt::IAcrylicBrushStatics2 statics2 = GetFactory<winrt::IAcrylicBrushStatics2>(L"Microsoft.UI.Xaml.Media.AcrylicBrush");
                    {
                        xamlType.AddDPMember(L"TintLuminosityOpacity", L"Windows.Foundation.IReference`1<Double>", statics2.TintLuminosityOpacityProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Media.DesktopAcrylicBackdrop",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Media.DesktopAcrylicBackdrop",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Media.SystemBackdrop",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IDesktopAcrylicBackdropFactory>(L"Microsoft.UI.Xaml.Media.DesktopAcrylicBackdrop"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Media.MicaBackdrop",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Media.MicaBackdrop",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Media.SystemBackdrop",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IMicaBackdropFactory>(L"Microsoft.UI.Xaml.Media.MicaBackdrop"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IMicaBackdropStatics statics = GetFactory<winrt::IMicaBackdropStatics>(L"Microsoft.UI.Xaml.Media.MicaBackdrop");
                    {
                        xamlType.AddDPMember(L"Kind", L"Microsoft.UI.Composition.SystemBackdrops.MicaKind", statics.KindProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Media.RadialGradientBrush",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Media.RadialGradientBrush",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Media.XamlCompositionBrushBase",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IRadialGradientBrushFactory>(L"Microsoft.UI.Xaml.Media.RadialGradientBrush"); },
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IRadialGradientBrushStatics statics = GetFactory<winrt::IRadialGradientBrushStatics>(L"Microsoft.UI.Xaml.Media.RadialGradientBrush");
                    {
                        xamlType.AddDPMember(L"Center", L"Windows.Foundation.Point", statics.CenterProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"GradientOrigin", L"Windows.Foundation.Point", statics.GradientOriginProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"InterpolationSpace", L"Microsoft.UI.Composition.CompositionColorSpace", statics.InterpolationSpaceProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"MappingMode", L"Microsoft.UI.Xaml.Media.BrushMappingMode", statics.MappingModeProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"RadiusX", L"Double", statics.RadiusXProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"RadiusY", L"Double", statics.RadiusYProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"SpreadMethod", L"Microsoft.UI.Xaml.Media.GradientSpreadMethod", statics.SpreadMethodProperty(), false /* isContent */);
                    }

                    xamlType.AddMember(
                        L"GradientStops", /* propertyName */
                        L"Windows.Foundation.Collections.IObservableVector`1<Microsoft.UI.Xaml.Media.GradientStop>", /* propertyType */
                        [](winrt::IInspectable instance) { return instance.as<winrt::RadialGradientBrush>().GradientStops(); },
                        nullptr, /* setter */
                        true, /* isContent */
                        false, /* isDependencyProperty */
                        false /* isAttachable */);
                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Media.RevealBrush",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Media.RevealBrush",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Media.XamlCompositionBrushBase",
                /* Arg 3 - Activator func */ 
                nullptr,
                /* Arg 4 - Populate properties func */ 
                (std::function<void(XamlTypeBase&)>)[](XamlTypeBase& xamlType)
                {
                    winrt::IRevealBrushStatics statics = GetFactory<winrt::IRevealBrushStatics>(L"Microsoft.UI.Xaml.Media.RevealBrush");
                    {
                        xamlType.AddDPMember(L"AlwaysUseFallback", L"Boolean", statics.AlwaysUseFallbackProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"Color", L"Windows.UI.Color", statics.ColorProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"State", L"Microsoft.UI.Xaml.Media.RevealBrushState", statics.StateProperty(), false /* isContent */);
                        xamlType.AddDPMember(L"TargetTheme", L"Microsoft.UI.Xaml.ApplicationTheme", statics.TargetThemeProperty(), false /* isContent */);
                    }

                });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Media.RevealBackgroundBrush",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Media.RevealBackgroundBrush",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Media.RevealBrush",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IRevealBackgroundBrushFactory>(L"Microsoft.UI.Xaml.Media.RevealBackgroundBrush"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Media.RevealBorderBrush",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Media.RevealBorderBrush",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Media.RevealBrush",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstanceWithFactory<winrt::IRevealBorderBrushFactory>(L"Microsoft.UI.Xaml.Media.RevealBorderBrush"); },
                /* Arg 4 - Populate properties func */ 
                nullptr
            );

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Media.RevealBrushState",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.Media.RevealBrushState",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Normal") return box_value(winrt::RevealBrushState::Normal);
                    if (fromString == L"PointerOver") return box_value(winrt::RevealBrushState::PointerOver);
                    if (fromString == L"Pressed") return box_value(winrt::RevealBrushState::Pressed);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.XamlTypeInfo.XamlControlsXamlMetaDataProvider",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Xaml.XamlTypeInfo.XamlControlsXamlMetaDataProvider",
                /* Arg 2 - BaseTypeName */ 
                (PCWSTR)L"Object",
                /* Arg 3 - Activator func */ 
                (std::function<winrt::IInspectable()>)[](){ return ActivateInstance(L"Microsoft.UI.Xaml.XamlTypeInfo.XamlControlsXamlMetaDataProvider"); },
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
        L"Microsoft.UI.Composition.CompositionColorSpace",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Composition.CompositionColorSpace",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Auto") return box_value(winrt::CompositionColorSpace::Auto);
                    if (fromString == L"Hsl") return box_value(winrt::CompositionColorSpace::Hsl);
                    if (fromString == L"Rgb") return box_value(winrt::CompositionColorSpace::Rgb);
                    if (fromString == L"HslLinear") return box_value(winrt::CompositionColorSpace::HslLinear);
                    if (fromString == L"RgbLinear") return box_value(winrt::CompositionColorSpace::RgbLinear);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Composition.CompositionObject",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Microsoft.UI.Composition.CompositionObject", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Composition.CompositionPropertySet",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Microsoft.UI.Composition.CompositionPropertySet", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Composition.SystemBackdrops.MicaKind",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make<EnumXamlType>(
                /* Arg 1 - TypeName */ 
                (PCWSTR)L"Microsoft.UI.Composition.SystemBackdrops.MicaKind",
                /* Arg 2 - CreateFromString func */ 
                (std::function<winrt::IInspectable(hstring)>)[](hstring fromString)
                {
                    if (fromString == L"Base") return box_value(winrt::MicaKind::Base);
                    if (fromString == L"BaseAlt") return box_value(winrt::MicaKind::BaseAlt);
                    throw winrt::hresult_invalid_argument();
                });

            return xamlType;
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.ApplicationTheme",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.ApplicationTheme"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Automation.Peers.AutomationControlType",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Automation.Peers.AutomationControlType"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.AutoSuggestBox",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Controls.AutoSuggestBox"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Button",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Controls.Button"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.CommandBar",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Controls.CommandBar"); }
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
        L"Microsoft.UI.Xaml.Controls.Grid",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Controls.Grid"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.IconElement",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Controls.IconElement"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.IconSource",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Controls.IconSource"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ListView",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Controls.ListView"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ListViewItem",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Controls.ListViewItem"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.MenuFlyout",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Controls.MenuFlyout"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.MenuFlyoutItem",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Controls.MenuFlyoutItem"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Orientation",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Controls.Orientation"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Panel",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Controls.Panel"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Primitives.ButtonBase",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.ButtonBase"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Primitives.ListViewItemPresenter",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.ListViewItemPresenter"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Primitives.RangeBase",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Controls.Primitives.RangeBase"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.ScrollViewer",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Controls.ScrollViewer"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.Slider",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Controls.Slider"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Controls.StyleSelector",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Controls.StyleSelector"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.CornerRadius",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.CornerRadius"); }
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
        L"Microsoft.UI.Xaml.FrameworkElement",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.FrameworkElement"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.GridLength",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.GridLength"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.IElementFactory",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.IElementFactory"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Input.ICommand",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Input.ICommand"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Input.InputScope",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Input.InputScope"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Media.Animation.TransitionCollection",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Media.Animation.TransitionCollection"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Media.Brush",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Media.Brush"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Media.BrushMappingMode",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Media.BrushMappingMode"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Media.Geometry",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Media.Geometry"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Media.GradientSpreadMethod",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Media.GradientSpreadMethod"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Media.ImageBrush",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Media.ImageBrush"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Media.ImageSource",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Media.ImageSource"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Media.RectangleGeometry",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Media.RectangleGeometry"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Media.SolidColorBrush",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Media.SolidColorBrush"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Media.Stretch",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Media.Stretch"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Media.SystemBackdrop",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Media.SystemBackdrop"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Media.XamlCompositionBrushBase",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Media.XamlCompositionBrushBase"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.ResourceDictionary",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.ResourceDictionary"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Style",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Style"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.TextAlignment",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.TextAlignment"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.TextReadingOrder",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.TextReadingOrder"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Thickness",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Thickness"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.UIElement",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.UIElement"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.UI.Xaml.Visibility",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Microsoft.UI.Xaml.Visibility"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Microsoft.Web.WebView2.Core.CoreWebView2",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Microsoft.Web.WebView2.Core.CoreWebView2", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Object",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Object"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Single",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"Single"); }
    },
    {
        /* Arg1 TypeName */ 
        L"String",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<PrimitiveXamlType>((PCWSTR)L"String"); }
    },
    {
        /* Arg1 TypeName */ 
        L"TimeSpan",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"TimeSpan", (PCWSTR)L"ValueType" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Uri",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Uri", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            return static_cast<winrt::IXamlType>(*xamlType);
        }
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
        L"Windows.ApplicationModel.Contacts.Contact",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.ApplicationModel.Contacts.Contact", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Devices.Geolocation.Geopoint",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.Devices.Geolocation.Geopoint", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Foundation.Collections.IMap`2<String,Microsoft.UI.Xaml.DataTemplate>",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.Foundation.Collections.IMap`2<String,Microsoft.UI.Xaml.DataTemplate>", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            xamlType->SetAddToMapFunc((std::function<void(winrt::IInspectable const&, winrt::IInspectable const&, winrt::IInspectable const&)>)[](winrt::IInspectable const& map, winrt::IInspectable const& key, winrt::IInspectable const& value)
            {
                map.as<winrt::Windows::Foundation::Collections::IMap<winrt::hstring, winrt::Microsoft::UI::Xaml::DataTemplate>>().Insert(unbox_value<winrt::hstring>(key), unbox_value<winrt::Microsoft::UI::Xaml::DataTemplate>(value));
            });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Foundation.Collections.IObservableVector`1<Microsoft.UI.Xaml.Controls.ICommandBarElement>",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.Foundation.Collections.IObservableVector`1<Microsoft.UI.Xaml.Controls.ICommandBarElement>", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            xamlType->SetCollectionAddFunc((std::function<void(winrt::IInspectable const&, winrt::IInspectable const&)>)[](winrt::IInspectable const& collection, winrt::IInspectable const& value)
            {
                collection.as<winrt::Windows::Foundation::Collections::IObservableVector<winrt::Microsoft::UI::Xaml::Controls::ICommandBarElement>>().Append(unbox_value<winrt::Microsoft::UI::Xaml::Controls::ICommandBarElement>(value));
            });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Foundation.Collections.IObservableVector`1<Microsoft.UI.Xaml.Media.GradientStop>",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.Foundation.Collections.IObservableVector`1<Microsoft.UI.Xaml.Media.GradientStop>", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            xamlType->SetCollectionAddFunc((std::function<void(winrt::IInspectable const&, winrt::IInspectable const&)>)[](winrt::IInspectable const& collection, winrt::IInspectable const& value)
            {
                collection.as<winrt::Windows::Foundation::Collections::IObservableVector<winrt::Microsoft::UI::Xaml::Media::GradientStop>>().Append(unbox_value<winrt::Microsoft::UI::Xaml::Media::GradientStop>(value));
            });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Foundation.Collections.IReadOnlyDictionary`2<String,Double>",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.Foundation.Collections.IReadOnlyDictionary`2<String,Double>", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Foundation.Collections.IReadOnlyList`1<Microsoft.UI.Xaml.Controls.IndexPath>",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.Foundation.Collections.IReadOnlyList`1<Microsoft.UI.Xaml.Controls.IndexPath>", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Foundation.Collections.IReadOnlyList`1<Microsoft.UI.Xaml.UIElement>",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.Foundation.Collections.IReadOnlyList`1<Microsoft.UI.Xaml.UIElement>", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Foundation.Collections.IReadOnlyList`1<Object>",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.Foundation.Collections.IReadOnlyList`1<Object>", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Foundation.Collections.IVector`1<Int32>",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.Foundation.Collections.IVector`1<Int32>", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            xamlType->SetCollectionAddFunc((std::function<void(winrt::IInspectable const&, winrt::IInspectable const&)>)[](winrt::IInspectable const& collection, winrt::IInspectable const& value)
            {
                collection.as<winrt::Windows::Foundation::Collections::IVector<int>>().Append(unbox_value<int>(value));
            });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.AnnotatedScrollBarLabel>",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.AnnotatedScrollBarLabel>", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            xamlType->SetCollectionAddFunc((std::function<void(winrt::IInspectable const&, winrt::IInspectable const&)>)[](winrt::IInspectable const& collection, winrt::IInspectable const& value)
            {
                collection.as<winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::UI::Xaml::Controls::AnnotatedScrollBarLabel>>().Append(unbox_value<winrt::Microsoft::UI::Xaml::Controls::AnnotatedScrollBarLabel>(value));
            });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.MapElement>",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.MapElement>", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            xamlType->SetCollectionAddFunc((std::function<void(winrt::IInspectable const&, winrt::IInspectable const&)>)[](winrt::IInspectable const& collection, winrt::IInspectable const& value)
            {
                collection.as<winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::UI::Xaml::Controls::MapElement>>().Append(unbox_value<winrt::Microsoft::UI::Xaml::Controls::MapElement>(value));
            });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.MapLayer>",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.MapLayer>", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            xamlType->SetCollectionAddFunc((std::function<void(winrt::IInspectable const&, winrt::IInspectable const&)>)[](winrt::IInspectable const& collection, winrt::IInspectable const& value)
            {
                collection.as<winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::UI::Xaml::Controls::MapLayer>>().Append(unbox_value<winrt::Microsoft::UI::Xaml::Controls::MapLayer>(value));
            });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.MenuBarItem>",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.MenuBarItem>", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            xamlType->SetCollectionAddFunc((std::function<void(winrt::IInspectable const&, winrt::IInspectable const&)>)[](winrt::IInspectable const& collection, winrt::IInspectable const& value)
            {
                collection.as<winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::UI::Xaml::Controls::MenuBarItem>>().Append(unbox_value<winrt::Microsoft::UI::Xaml::Controls::MenuBarItem>(value));
            });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.MenuFlyoutItemBase>",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.MenuFlyoutItemBase>", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            xamlType->SetCollectionAddFunc((std::function<void(winrt::IInspectable const&, winrt::IInspectable const&)>)[](winrt::IInspectable const& collection, winrt::IInspectable const& value)
            {
                collection.as<winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItemBase>>().Append(unbox_value<winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItemBase>(value));
            });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.Primitives.ScrollSnapPointBase>",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.Primitives.ScrollSnapPointBase>", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            xamlType->SetCollectionAddFunc((std::function<void(winrt::IInspectable const&, winrt::IInspectable const&)>)[](winrt::IInspectable const& collection, winrt::IInspectable const& value)
            {
                collection.as<winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::UI::Xaml::Controls::Primitives::ScrollSnapPointBase>>().Append(unbox_value<winrt::Microsoft::UI::Xaml::Controls::Primitives::ScrollSnapPointBase>(value));
            });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.Primitives.ZoomSnapPointBase>",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.Primitives.ZoomSnapPointBase>", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            xamlType->SetCollectionAddFunc((std::function<void(winrt::IInspectable const&, winrt::IInspectable const&)>)[](winrt::IInspectable const& collection, winrt::IInspectable const& value)
            {
                collection.as<winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::UI::Xaml::Controls::Primitives::ZoomSnapPointBase>>().Append(unbox_value<winrt::Microsoft::UI::Xaml::Controls::Primitives::ZoomSnapPointBase>(value));
            });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.SelectorBarItem>",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.SelectorBarItem>", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            xamlType->SetCollectionAddFunc((std::function<void(winrt::IInspectable const&, winrt::IInspectable const&)>)[](winrt::IInspectable const& collection, winrt::IInspectable const& value)
            {
                collection.as<winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::UI::Xaml::Controls::SelectorBarItem>>().Append(unbox_value<winrt::Microsoft::UI::Xaml::Controls::SelectorBarItem>(value));
            });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.TreeViewNode>",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.Foundation.Collections.IVector`1<Microsoft.UI.Xaml.Controls.TreeViewNode>", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            xamlType->SetCollectionAddFunc((std::function<void(winrt::IInspectable const&, winrt::IInspectable const&)>)[](winrt::IInspectable const& collection, winrt::IInspectable const& value)
            {
                collection.as<winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::UI::Xaml::Controls::TreeViewNode>>().Append(unbox_value<winrt::Microsoft::UI::Xaml::Controls::TreeViewNode>(value));
            });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Foundation.Collections.IVector`1<Object>",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.Foundation.Collections.IVector`1<Object>", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            xamlType->SetCollectionAddFunc((std::function<void(winrt::IInspectable const&, winrt::IInspectable const&)>)[](winrt::IInspectable const& collection, winrt::IInspectable const& value)
            {
                collection.as<winrt::Windows::Foundation::Collections::IVector<winrt::IInspectable>>().Append(unbox_value<winrt::IInspectable>(value));
            });

            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Foundation.IReference`1<Double>",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<NullableXamlType>((PCWSTR)L"Windows.Foundation.IReference`1<Double>", (PCWSTR)L"Double"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Foundation.IReference`1<Windows.UI.Color>",
        /* Arg2 CreateXamlTypeCallback */ 
        []() { return winrt::make<NullableXamlType>((PCWSTR)L"Windows.Foundation.IReference`1<Windows.UI.Color>", (PCWSTR)L"Windows.UI.Color"); }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Foundation.Numerics.Vector4",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.Foundation.Numerics.Vector4", (PCWSTR)L"ValueType" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Foundation.Point",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.Foundation.Point", (PCWSTR)L"ValueType" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Foundation.Rect",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.Foundation.Rect", (PCWSTR)L"ValueType" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.Globalization.NumberFormatting.INumberFormatter2",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.Globalization.NumberFormatting.INumberFormatter2", (PCWSTR)L"Object" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
    {
        /* Arg1 TypeName */ 
        L"Windows.UI.Color",
        /* Arg2 CreateXamlTypeCallback */ 
        []()
        {
            auto xamlType = winrt::make_self<XamlType>((PCWSTR)L"Windows.UI.Color", (PCWSTR)L"ValueType" /* BaseTypeName */ , nullptr /* Activator Func */, nullptr /* PopulatePropertiesFunc */ );
            return static_cast<winrt::IXamlType>(*xamlType);
        }
    },
};

hstring c_knownNamespacePrefixes[] =
{
    L"Microsoft.UI.Composition.",
    L"Microsoft.UI.Xaml.",
    L"Microsoft.Web.WebView2.",
    L"Windows.ApplicationModel.Contacts.",
    L"Windows.Devices.Geolocation.",
    L"Windows.Foundation.",
    L"Windows.Foundation.Collections.",
    L"Windows.Foundation.Numerics.",
    L"Windows.Globalization.NumberFormatting.",
    L"Windows.UI.",
};
}

#include "AcrylicBrush.properties.h"
#include "AnimatedIcon.properties.h"
#include "AnimatedIconSource.properties.h"
#include "AnimatedVisualPlayer.properties.h"
#include "AnnotatedScrollBar.properties.h"
#include "AutoSuggestBoxHelper.properties.h"
#include "BreadcrumbBar.properties.h"
#include "ColorPicker.properties.h"
#include "ColorPickerSlider.properties.h"
#include "ColorSpectrum.properties.h"
#include "ColumnMajorUniformToLargestGridLayout.properties.h"
#include "ComboBoxHelper.properties.h"
#include "CommandBarFlyoutCommandBar.properties.h"
#include "CommandBarFlyoutCommandBarAutomationProperties.properties.h"
#include "CommandBarFlyoutCommandBarTemplateSettings.properties.h"
#include "CornerRadiusFilterConverter.properties.h"
#include "CornerRadiusToThicknessConverter.properties.h"
#include "Expander.properties.h"
#include "FlowLayout.properties.h"
#include "ImageIcon.properties.h"
#include "ImageIconSource.properties.h"
#include "InfoBadge.properties.h"
#include "InfoBadgeTemplateSettings.properties.h"
#include "InfoBar.properties.h"
#include "InfoBarPanel.properties.h"
#include "InfoBarTemplateSettings.properties.h"
#include "ItemContainer.properties.h"
#include "ItemsRepeater.properties.h"
#include "ItemsView.properties.h"
#include "LayoutPanel.properties.h"
#include "LinedFlowLayout.properties.h"
#include "MapControl.properties.h"
#include "MapElementsLayer.properties.h"
#include "MapIcon.properties.h"
#include "MenuBar.properties.h"
#include "MenuBarItem.properties.h"
#include "MicaBackdrop.properties.h"
#include "MonochromaticOverlayPresenter.properties.h"
#include "NavigationView.properties.h"
#include "NavigationViewItem.properties.h"
#include "NavigationViewItemBase.properties.h"
#include "NavigationViewItemPresenter.properties.h"
#include "NavigationViewItemPresenterTemplateSettings.properties.h"
#include "NavigationViewTemplateSettings.properties.h"
#include "NumberBox.properties.h"
#include "PagerControl.properties.h"
#include "ParallaxView.properties.h"
#include "PersonPicture.properties.h"
#include "PipsPager.properties.h"
#include "ProgressBar.properties.h"
#include "ProgressBarTemplateSettings.properties.h"
#include "ProgressRing.properties.h"
#include "RadialGradientBrush.properties.h"
#include "RadioButtons.properties.h"
#include "RadioMenuFlyoutItem.properties.h"
#include "RatingControl.properties.h"
#include "RatingItemFontInfo.properties.h"
#include "RatingItemImageInfo.properties.h"
#include "RecyclePool.properties.h"
#include "RefreshContainer.properties.h"
#include "RefreshVisualizer.properties.h"
#include "RevealBrush.properties.h"
#include "ScrollPresenter.properties.h"
#include "ScrollView.properties.h"
#include "SelectorBar.properties.h"
#include "SelectorBarItem.properties.h"
#include "SplitButton.properties.h"
#include "StackLayout.properties.h"
#include "SwipeControl.properties.h"
#include "SwipeItem.properties.h"
#include "SwipeItems.properties.h"
#include "TabView.properties.h"
#include "TabViewItem.properties.h"
#include "TabViewItemTemplateSettings.properties.h"
#include "TeachingTip.properties.h"
#include "TeachingTipTemplateSettings.properties.h"
#include "TitleBar.properties.h"
#include "TitleBarTemplateSettings.properties.h"
#include "ToggleSplitButton.properties.h"
#include "TreeView.properties.h"
#include "TreeViewItem.properties.h"
#include "TreeViewItemTemplateSettings.properties.h"
#include "TreeViewNode.properties.h"
#include "TwoPaneView.properties.h"
#include "UniformGridLayout.properties.h"
#include "WebView2.properties.h"
#include "XamlControlsResources.properties.h"
#include "AutoSuggestBoxHelper.h"
#include "ComboBoxHelper.h"
#include "RecyclePool.h"
#include "RevealBrush.h"

namespace {

void ClearTypeProperties()
{
    AcrylicBrushProperties::ClearProperties();
    AnimatedIconProperties::ClearProperties();
    AnimatedIconSourceProperties::ClearProperties();
    AnimatedVisualPlayerProperties::ClearProperties();
    AnnotatedScrollBarProperties::ClearProperties();
    AutoSuggestBoxHelperProperties::ClearProperties();
    BreadcrumbBarProperties::ClearProperties();
    ColorPickerProperties::ClearProperties();
    ColorPickerSliderProperties::ClearProperties();
    ColorSpectrumProperties::ClearProperties();
    ColumnMajorUniformToLargestGridLayoutProperties::ClearProperties();
    ComboBoxHelperProperties::ClearProperties();
    CommandBarFlyoutCommandBarProperties::ClearProperties();
    CommandBarFlyoutCommandBarAutomationPropertiesProperties::ClearProperties();
    CommandBarFlyoutCommandBarTemplateSettingsProperties::ClearProperties();
    CornerRadiusFilterConverterProperties::ClearProperties();
    CornerRadiusToThicknessConverterProperties::ClearProperties();
    ExpanderProperties::ClearProperties();
    FlowLayoutProperties::ClearProperties();
    ImageIconProperties::ClearProperties();
    ImageIconSourceProperties::ClearProperties();
    InfoBadgeProperties::ClearProperties();
    InfoBadgeTemplateSettingsProperties::ClearProperties();
    InfoBarProperties::ClearProperties();
    InfoBarPanelProperties::ClearProperties();
    InfoBarTemplateSettingsProperties::ClearProperties();
    ItemContainerProperties::ClearProperties();
    ItemsRepeaterProperties::ClearProperties();
    ItemsViewProperties::ClearProperties();
    LayoutPanelProperties::ClearProperties();
    LinedFlowLayoutProperties::ClearProperties();
    MapControlProperties::ClearProperties();
    MapElementsLayerProperties::ClearProperties();
    MapIconProperties::ClearProperties();
    MenuBarProperties::ClearProperties();
    MenuBarItemProperties::ClearProperties();
    MicaBackdropProperties::ClearProperties();
    MonochromaticOverlayPresenterProperties::ClearProperties();
    NavigationViewProperties::ClearProperties();
    NavigationViewItemProperties::ClearProperties();
    NavigationViewItemBaseProperties::ClearProperties();
    NavigationViewItemPresenterProperties::ClearProperties();
    NavigationViewItemPresenterTemplateSettingsProperties::ClearProperties();
    NavigationViewTemplateSettingsProperties::ClearProperties();
    NumberBoxProperties::ClearProperties();
    PagerControlProperties::ClearProperties();
    ParallaxViewProperties::ClearProperties();
    PersonPictureProperties::ClearProperties();
    PipsPagerProperties::ClearProperties();
    ProgressBarProperties::ClearProperties();
    ProgressBarTemplateSettingsProperties::ClearProperties();
    ProgressRingProperties::ClearProperties();
    RadialGradientBrushProperties::ClearProperties();
    RadioButtonsProperties::ClearProperties();
    RadioMenuFlyoutItemProperties::ClearProperties();
    RatingControlProperties::ClearProperties();
    RatingItemFontInfoProperties::ClearProperties();
    RatingItemImageInfoProperties::ClearProperties();
    RecyclePoolProperties::ClearProperties();
    RefreshContainerProperties::ClearProperties();
    RefreshVisualizerProperties::ClearProperties();
    RevealBrushProperties::ClearProperties();
    ScrollPresenterProperties::ClearProperties();
    ScrollViewProperties::ClearProperties();
    SelectorBarProperties::ClearProperties();
    SelectorBarItemProperties::ClearProperties();
    SplitButtonProperties::ClearProperties();
    StackLayoutProperties::ClearProperties();
    SwipeControlProperties::ClearProperties();
    SwipeItemProperties::ClearProperties();
    SwipeItemsProperties::ClearProperties();
    TabViewProperties::ClearProperties();
    TabViewItemProperties::ClearProperties();
    TabViewItemTemplateSettingsProperties::ClearProperties();
    TeachingTipProperties::ClearProperties();
    TeachingTipTemplateSettingsProperties::ClearProperties();
    TitleBarProperties::ClearProperties();
    TitleBarTemplateSettingsProperties::ClearProperties();
    ToggleSplitButtonProperties::ClearProperties();
    TreeViewProperties::ClearProperties();
    TreeViewItemProperties::ClearProperties();
    TreeViewItemTemplateSettingsProperties::ClearProperties();
    TreeViewNodeProperties::ClearProperties();
    TwoPaneViewProperties::ClearProperties();
    UniformGridLayoutProperties::ClearProperties();
    WebView2Properties::ClearProperties();
    XamlControlsResourcesProperties::ClearProperties();
    AutoSuggestBoxHelper::ClearProperties();
    ComboBoxHelper::ClearProperties();
    RecyclePool::ClearProperties();
    RevealBrush::ClearProperties();
}

}
