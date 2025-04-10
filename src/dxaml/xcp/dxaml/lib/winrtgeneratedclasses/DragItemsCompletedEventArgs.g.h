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


#define __DragItemsCompletedEventArgs_GUID "31f1244f-4b3d-4ec9-bbca-1a8062ba5fdd"

namespace DirectUI
{
    class DragItemsCompletedEventArgs;

    class __declspec(novtable) __declspec(uuid(__DragItemsCompletedEventArgs_GUID)) DragItemsCompletedEventArgs :
        public ABI::Microsoft::UI::Xaml::Controls::IDragItemsCompletedEventArgs,
        public DirectUI::EventArgs
    {

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Controls.DragItemsCompletedEventArgs");

        BEGIN_INTERFACE_MAP(DragItemsCompletedEventArgs, DirectUI::EventArgs)
            INTERFACE_ENTRY(DragItemsCompletedEventArgs, ABI::Microsoft::UI::Xaml::Controls::IDragItemsCompletedEventArgs)
        END_INTERFACE_MAP(DragItemsCompletedEventArgs, DirectUI::EventArgs)

    public:
        DragItemsCompletedEventArgs();
        ~DragItemsCompletedEventArgs() override;

        // Properties.
        IFACEMETHOD(get_Items)(_Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IVectorView<IInspectable*>** ppValue) override;
        _Check_return_ HRESULT put_Items(_In_opt_ ABI::Windows::Foundation::Collections::IVectorView<IInspectable*>* pValue);
        IFACEMETHOD(get_DropResult)(_Out_ ABI::Windows::ApplicationModel::DataTransfer::DataPackageOperation* pValue) override;
        _Check_return_ HRESULT put_DropResult(ABI::Windows::ApplicationModel::DataTransfer::DataPackageOperation value);

        // Methods.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:
        // Customized properties.

        // Customized methods.

        // Fields.
        TrackerPtr<ABI::Windows::Foundation::Collections::IVectorView<IInspectable*>> m_pItems;
        ABI::Windows::ApplicationModel::DataTransfer::DataPackageOperation m_dropResult;
    };
}


