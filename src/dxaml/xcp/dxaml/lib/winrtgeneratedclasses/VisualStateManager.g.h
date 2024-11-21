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


#define __VisualStateManager_GUID "85a77c1b-a554-4d4c-abc1-6d603c68a50f"

namespace DirectUI
{
    class VisualStateManager;
    class Control;
    class FrameworkElement;
    class VisualState;
    class VisualStateGroup;

    class __declspec(novtable) VisualStateManagerGenerated:
        public DirectUI::DependencyObject
        , public ABI::Microsoft::UI::Xaml::IVisualStateManager
        , public ABI::Microsoft::UI::Xaml::IVisualStateManagerProtected
        , public ABI::Microsoft::UI::Xaml::IVisualStateManagerOverrides
    {
        friend class DirectUI::VisualStateManager;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.VisualStateManager");

        BEGIN_INTERFACE_MAP(VisualStateManagerGenerated, DirectUI::DependencyObject)
            INTERFACE_ENTRY(VisualStateManagerGenerated, ABI::Microsoft::UI::Xaml::IVisualStateManager)
            INTERFACE_ENTRY(VisualStateManagerGenerated, ABI::Microsoft::UI::Xaml::IVisualStateManagerProtected)
            INTERFACE_ENTRY(VisualStateManagerGenerated, ABI::Microsoft::UI::Xaml::IVisualStateManagerOverrides)
        END_INTERFACE_MAP(VisualStateManagerGenerated, DirectUI::DependencyObject)

    public:
        VisualStateManagerGenerated();
        ~VisualStateManagerGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::VisualStateManager;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::VisualStateManager;
        }

        // Properties.

        // Events.

        // Methods.
        IFACEMETHOD(GoToStateCore)(_In_opt_ ABI::Microsoft::UI::Xaml::Controls::IControl* pControl, _In_ ABI::Microsoft::UI::Xaml::IFrameworkElement* pTemplateRoot, _In_opt_ HSTRING stateName, _In_opt_ ABI::Microsoft::UI::Xaml::IVisualStateGroup* pGroup, _In_opt_ ABI::Microsoft::UI::Xaml::IVisualState* pState, BOOLEAN useTransitions, _Out_ BOOLEAN* pReturnValue) override;
        _Check_return_ HRESULT GoToStateCoreProtected(_In_opt_ ABI::Microsoft::UI::Xaml::Controls::IControl* pControl, _In_ ABI::Microsoft::UI::Xaml::IFrameworkElement* pTemplateRoot, _In_opt_ HSTRING stateName, _In_opt_ ABI::Microsoft::UI::Xaml::IVisualStateGroup* pGroup, _In_opt_ ABI::Microsoft::UI::Xaml::IVisualState* pState, BOOLEAN useTransitions, _Out_ BOOLEAN* pReturnValue);
        IFACEMETHOD(RaiseCurrentStateChanged)(_In_ ABI::Microsoft::UI::Xaml::IVisualStateGroup* pStateGroup, _In_ ABI::Microsoft::UI::Xaml::IVisualState* pOldState, _In_ ABI::Microsoft::UI::Xaml::IVisualState* pNewState, _In_ ABI::Microsoft::UI::Xaml::Controls::IControl* pControl) override;
        IFACEMETHOD(RaiseCurrentStateChanging)(_In_ ABI::Microsoft::UI::Xaml::IVisualStateGroup* pStateGroup, _In_ ABI::Microsoft::UI::Xaml::IVisualState* pOldState, _In_ ABI::Microsoft::UI::Xaml::IVisualState* pNewState, _In_ ABI::Microsoft::UI::Xaml::Controls::IControl* pControl) override;


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "VisualStateManager_Partial.h"

namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) VisualStateManagerFactory:
       public ctl::BetterAggregableCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::IVisualStateManagerFactory
        , public ABI::Microsoft::UI::Xaml::IVisualStateManagerStatics
    {
        BEGIN_INTERFACE_MAP(VisualStateManagerFactory, ctl::BetterAggregableCoreObjectActivationFactory)
            INTERFACE_ENTRY(VisualStateManagerFactory, ABI::Microsoft::UI::Xaml::IVisualStateManagerFactory)
            INTERFACE_ENTRY(VisualStateManagerFactory, ABI::Microsoft::UI::Xaml::IVisualStateManagerStatics)
        END_INTERFACE_MAP(VisualStateManagerFactory, ctl::BetterAggregableCoreObjectActivationFactory)

    public:
        // Factory methods.
        IFACEMETHOD(CreateInstance)(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::IVisualStateManager** ppInstance);

        // Static properties.

        // Dependency properties.

        // Attached properties.
        static _Check_return_ HRESULT GetVisualStateGroupsStatic(_In_ ABI::Microsoft::UI::Xaml::IFrameworkElement* pObj, _Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IVector<ABI::Microsoft::UI::Xaml::VisualStateGroup*>** ppValue);
        static _Check_return_ HRESULT SetVisualStateGroupsStatic(_In_ ABI::Microsoft::UI::Xaml::IFrameworkElement* pObj, _In_opt_ ABI::Windows::Foundation::Collections::IVector<ABI::Microsoft::UI::Xaml::VisualStateGroup*>* pValue);
        IFACEMETHOD(GetVisualStateGroups)(_In_ ABI::Microsoft::UI::Xaml::IFrameworkElement* pObj, _Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IVector<ABI::Microsoft::UI::Xaml::VisualStateGroup*>** ppValue);
        IFACEMETHOD(SetVisualStateGroups)(_In_ ABI::Microsoft::UI::Xaml::IFrameworkElement* pObj, _In_opt_ ABI::Windows::Foundation::Collections::IVector<ABI::Microsoft::UI::Xaml::VisualStateGroup*>* pValue);
        static _Check_return_ HRESULT GetCustomVisualStateManagerStatic(_In_ ABI::Microsoft::UI::Xaml::IFrameworkElement* pObj, _Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::IVisualStateManager** ppValue);
        static _Check_return_ HRESULT SetCustomVisualStateManagerStatic(_In_ ABI::Microsoft::UI::Xaml::IFrameworkElement* pObj, _In_opt_ ABI::Microsoft::UI::Xaml::IVisualStateManager* pValue);
        IFACEMETHOD(get_CustomVisualStateManagerProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetCustomVisualStateManager)(_In_ ABI::Microsoft::UI::Xaml::IFrameworkElement* pObj, _Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::IVisualStateManager** ppValue);
        IFACEMETHOD(SetCustomVisualStateManager)(_In_ ABI::Microsoft::UI::Xaml::IFrameworkElement* pObj, _In_opt_ ABI::Microsoft::UI::Xaml::IVisualStateManager* pValue);

        // Static methods.
        IFACEMETHOD(GoToState)(_In_opt_ ABI::Microsoft::UI::Xaml::Controls::IControl* pControl, _In_opt_ HSTRING stateName, BOOLEAN useTransitions, _Out_ BOOLEAN* pReturnValue) override;

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::VisualStateManager;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
         _Check_return_ HRESULT GoToStateImpl(_In_opt_ ABI::Microsoft::UI::Xaml::Controls::IControl* pControl, _In_opt_ HSTRING stateName, BOOLEAN useTransitions, _Out_ BOOLEAN* pReturnValue); 
    };
}
