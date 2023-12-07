// Defintion for IUISettings6 which is not available in the current version of Windows Platform SDK.
// This is used in DXamlCore.cpp which fall back code if IUISettings6 is not available on older Windows Versions 
// Added from (os.2020 publics internal\buildmetadata\abi\windows.ui.viewmanagement.h)
// TODO: Remove IUISettings6 when Windows Platform SDK is updated that will have IUISettings6

#pragma once

#include <abi/xaml_abi.h>

#pragma push_macro("MIDL_CONST_ID")
#undef MIDL_CONST_ID
#define MIDL_CONST_ID const __declspec(selectany)

#if !defined(____FITypedEventHandler_2_Windows__CUI__CViewManagement__CUISettings_Windows__CUI__CViewManagement__CUISettingsAnimationsEnabledChangedEventArgs_INTERFACE_DEFINED__)
#define ____FITypedEventHandler_2_Windows__CUI__CViewManagement__CUISettings_Windows__CUI__CViewManagement__CUISettingsAnimationsEnabledChangedEventArgs_INTERFACE_DEFINED__
#endif

typedef interface __FITypedEventHandler_2_Windows__CUI__CViewManagement__CUISettings_Windows__CUI__CViewManagement__CUISettingsAnimationsEnabledChangedEventArgs __FITypedEventHandler_2_Windows__CUI__CViewManagement__CUISettings_Windows__CUI__CViewManagement__CUISettingsAnimationsEnabledChangedEventArgs;

#if !defined(____FITypedEventHandler_2_Windows__CUI__CViewManagement__CUISettings_Windows__CUI__CViewManagement__CUISettingsMessageDurationChangedEventArgs_INTERFACE_DEFINED__)
#define ____FITypedEventHandler_2_Windows__CUI__CViewManagement__CUISettings_Windows__CUI__CViewManagement__CUISettingsMessageDurationChangedEventArgs_INTERFACE_DEFINED__
#endif

typedef interface __FITypedEventHandler_2_Windows__CUI__CViewManagement__CUISettings_Windows__CUI__CViewManagement__CUISettingsMessageDurationChangedEventArgs __FITypedEventHandler_2_Windows__CUI__CViewManagement__CUISettings_Windows__CUI__CViewManagement__CUISettingsMessageDurationChangedEventArgs;

/*
 *
 * Possibly Temporary declaration. 
 * 
 * Interface Windows.UI.ViewManagement.IUISettings6
 *
 * Introduced to Windows.Foundation.UniversalApiContract in version 10.0
 *
 * Interface is a part of the implementation of type Windows.UI.ViewManagement.UISettings
 *
 */

extern const __declspec(selectany) _Null_terminated_ WCHAR InterfaceName_Windows_UI_ViewManagement_IUISettings6[] = L"Windows.UI.ViewManagement.IUISettings6";
XAML_ABI_NAMESPACE_BEGIN
namespace Windows {
    namespace Foundation
    {
        template <>
        struct __declspec(uuid("deff0f90-59e1-5859-a245-3d275081c2ad"))
        ITypedEventHandler<ABI::Windows::UI::ViewManagement::UISettings*, ABI::Windows::UI::ViewManagement::UISettingsAnimationsEnabledChangedEventArgs*> : ITypedEventHandler_impl<ABI::Windows::Foundation::Internal::AggregateType<ABI::Windows::UI::ViewManagement::UISettings*, ABI::Windows::UI::ViewManagement::IUISettings*>, ABI::Windows::Foundation::Internal::AggregateType<ABI::Windows::UI::ViewManagement::UISettingsAnimationsEnabledChangedEventArgs*, ABI::Windows::UI::ViewManagement::IUISettingsAnimationsEnabledChangedEventArgs*>>
        {
            static const wchar_t* z_get_rc_name_impl()
            {
                return L"Windows.Foundation.TypedEventHandler`2<Windows.UI.ViewManagement.UISettings, Windows.UI.ViewManagement.UISettingsAnimationsEnabledChangedEventArgs>";
            }
        };
    }
    namespace UI {
        namespace ViewManagement {
            MIDL_INTERFACE("aef19bd7-fe31-5a04-ada4-469aaec6dfa9")
            IUISettings6 : public IInspectable
            {
            public:
                virtual HRESULT STDMETHODCALLTYPE add_AnimationsEnabledChanged(
                    wf::ITypedEventHandler<wuv::UISettings*, wuv::UISettingsAnimationsEnabledChangedEventArgs*>* handler,
                    EventRegistrationToken* token
                    ) = 0;
                virtual HRESULT STDMETHODCALLTYPE remove_AnimationsEnabledChanged(
                    EventRegistrationToken token
                    ) = 0;
                // virtual HRESULT STDMETHODCALLTYPE add_MessageDurationChanged(
                //     wf::ITypedEventHandler<wuv::UISettings*, wuv::UISettingsAnimationsEnabledChangedEventArgs*>* handler,
                //     EventRegistrationToken* token
                //     ) = 0;
                // virtual HRESULT STDMETHODCALLTYPE remove_MessageDurationChanged(
                //     EventRegistrationToken token
                //     ) = 0;
            };

            MIDL_CONST_ID IID& IID_IUISettings6 = __uuidof(IUISettings6);
        } /* ViewManagement */
    } /* UI */
} /* Windows */
XAML_ABI_NAMESPACE_END
