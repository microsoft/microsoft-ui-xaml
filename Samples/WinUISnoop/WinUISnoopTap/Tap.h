#pragma once

#include <unknwn.h>
#include <ocidl.h>
#include <wil/com.h>
#include <wil/resource.h>
#include <xamlom.h>
#include <thread>
#include <winrt/windows.data.json.h>
#include <winrt/microsoft.UI.Dispatching.h>

struct __declspec(uuid("e6755080-f13e-4864-931e-06368e30a84a"))
    XamlSnoopTap : winrt::implements<XamlSnoopTap, IObjectWithSite, IVisualTreeServiceCallback2>
{
public:
    XamlSnoopTap() {}

    // IObjectWithSite
    STDMETHOD(SetSite)(IUnknown* pUnkSite);
    STDMETHOD(GetSite)(REFIID riid, void** ppvSite);

    // IVisualTreeServiceCallback
    STDMETHOD(OnVisualTreeChange)(ParentChildRelation relation, VisualElement element, VisualMutationType mutationType);

    // IVisualTreeServiceCallback2
    STDMETHOD(OnElementStateChanged)(InstanceHandle element, VisualElementState elementState, LPCWSTR context);

    static void SetModuleHandle(HMODULE hModule) { s_hModule = hModule; }

private:
    void RetrieveElementProperties(InstanceHandle element);
    void RetrieveSubTreeProperties(InstanceHandle rootElement);
    void RetrieveSubTreePropertiesRecursive(InstanceHandle element);

    void HighlightElement(InstanceHandle element);
    void ClearHighlight();

    winrt::hstring GetPropertyValueAsString(InstanceHandle element, unsigned int propertyIndex, BSTR propertyType);
    const wchar_t* SourceToString(BaseValueSource source);

    void WriteToPipe(const winrt::Windows::Data::Json::JsonObject& obj);
    void WriteLineToPipe(winrt::hstring string);

    static void ReaderThreadProc(wil::com_ptr<XamlSnoopTap> tap);

    // Json helpers
    void SetNamedPointerValue(winrt::Windows::Data::Json::JsonObject& obj, winrt::hstring name, InstanceHandle ptr);

    void Shutdown();

private:
    wil::com_ptr<IUnknown> m_site;
    wil::com_ptr<IXamlDiagnostics> m_xamlDiagnostics;
    wil::com_ptr<IVisualTreeService> m_visualTreeService;
    wil::unique_handle m_writePipe;
    wil::unique_handle m_readPipe;
    winrt::Microsoft::UI::Dispatching::DispatcherQueue m_dispatcherQueue = nullptr;
    bool m_suspendTreeChanges = false;
    bool m_shutdownCalled = false;

    winrt::Microsoft::UI::Composition::ContainerVisual m_highlightContainer{nullptr};
    winrt::weak_ref<winrt::Microsoft::UI::Xaml::UIElement> m_highlightHost;

    static HMODULE s_hModule;
};
