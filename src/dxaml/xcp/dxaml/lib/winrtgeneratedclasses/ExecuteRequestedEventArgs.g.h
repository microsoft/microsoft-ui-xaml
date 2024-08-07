// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//------------------------------------------------------------------------
//
//  Abstract:
//
//      XAML types.
//      NOTE: This file was generated by a tool.
//
//------------------------------------------------------------------------

#pragma once

#include "ItemsControl.g.h"

#define __ExecuteRequestedEventArgs_GUID "5c7fe9ed-651e-49c3-8ef3-1caba3c7a38a"
#include <FeatureFlags.h>
#if WI_IS_FEATURE_PRESENT(Feature_CommandingImprovements) 
#define FEATURE_COMMANDINGIMPROVEMENTS_OVERRIDE override
#else
#define FEATURE_COMMANDINGIMPROVEMENTS_OVERRIDE
#endif
namespace DirectUI
{
    class ExecuteRequestedEventArgs;

    class __declspec(novtable) __declspec(uuid(__ExecuteRequestedEventArgs_GUID)) ExecuteRequestedEventArgs :
        public ABI::Microsoft::UI::Xaml::Input::IExecuteRequestedEventArgs,
#if WI_IS_FEATURE_PRESENT(Feature_CommandingImprovements)
        public ABI::Microsoft::UI::Xaml::Input::IExecuteRequestedEventArgsFeature_CommandingImprovements,
#endif
        public DirectUI::EventArgs
    {

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Input.ExecuteRequestedEventArgs");

        BEGIN_INTERFACE_MAP(ExecuteRequestedEventArgs, DirectUI::EventArgs)
            INTERFACE_ENTRY(ExecuteRequestedEventArgs, ABI::Microsoft::UI::Xaml::Input::IExecuteRequestedEventArgs)
#if WI_IS_FEATURE_PRESENT(Feature_CommandingImprovements)
            INTERFACE_ENTRY(ExecuteRequestedEventArgs, ABI::Microsoft::UI::Xaml::Input::IExecuteRequestedEventArgsFeature_CommandingImprovements)
#endif
        END_INTERFACE_MAP(ExecuteRequestedEventArgs, DirectUI::EventArgs)

    public:
        ExecuteRequestedEventArgs();
        ~ExecuteRequestedEventArgs() override;

        // Properties.
        IFACEMETHOD(get_Parameter)(_Outptr_result_maybenull_ IInspectable** ppValue) override;
        _Check_return_ HRESULT put_Parameter(_In_opt_ IInspectable* pValue);
        IFACEMETHOD(get_CommandTarget)(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::IDependencyObject** ppValue) FEATURE_COMMANDINGIMPROVEMENTS_OVERRIDE;
        _Check_return_ HRESULT put_CommandTarget(_In_opt_ ABI::Microsoft::UI::Xaml::IDependencyObject* pValue);
        IFACEMETHOD(get_ListCommandTarget)(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Controls::IItemsControl** ppValue) FEATURE_COMMANDINGIMPROVEMENTS_OVERRIDE;
        _Check_return_ HRESULT put_ListCommandTarget(_In_opt_ ABI::Microsoft::UI::Xaml::Controls::IItemsControl* pValue);

        // Methods.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:
        // Customized properties.

        // Customized methods.

        // Fields.
        TrackerPtr<IInspectable> m_pParameter;
        TrackerPtr<ABI::Microsoft::UI::Xaml::IDependencyObject> m_pCommandTarget;
        TrackerPtr<ABI::Microsoft::UI::Xaml::Controls::IItemsControl> m_pListCommandTarget;
    };
}


