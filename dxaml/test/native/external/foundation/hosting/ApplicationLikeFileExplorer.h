// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// This is an Application class that's more like what FileExplorer does.  Notably, it doesn't create a
// WindowsXamlManager in the Application ctor, and it synchronizes some of the metadataProvider lookups.
ref class ApplicationLikeFileExplorer sealed
    : public Microsoft::UI::Xaml::Application
    , public IXamlMetadataProvider
{
public:
    ApplicationLikeFileExplorer()
    {
        m_metadataProvider = ref new Microsoft::UI::Xaml::XamlTypeInfo::XamlControlsXamlMetaDataProvider();
    }

    virtual ~ApplicationLikeFileExplorer()
    {
    }

    void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs^) override
    {
        LOG_OUTPUT(L"    > [ApplicationLikeFileExplorer] OnLaunched: Load MUXC Resources");

        Microsoft::UI::Xaml::Controls::XamlControlsResources^ resources = nullptr;
        {
            // Adding a lock acquisition here (and removing the other lines we acquire m_metadataLock)
            // makes the stress test much more stable.
            //auto lockGuard = m_metadataLock.lock_exclusive(); 
            resources = ref new Microsoft::UI::Xaml::Controls::XamlControlsResources();
        }
        this->Resources->MergedDictionaries->Append(resources);
    }

    // IXamlMetadataProvider
    virtual IXamlType^ GetXamlType(::Windows::UI::Xaml::Interop::TypeName type) /*override*/
    {
        // The lock here mimics what FileExplorer's Application does.
        auto lockGuard = m_metadataLock.lock_exclusive();
        return m_metadataProvider->GetXamlType(type);
    }

    virtual IXamlType^ GetXamlType(Platform::String^ fullName) /*override*/
    {
        // The lock here mimics what FileExplorer's Application does.
        auto lockGuard = m_metadataLock.lock_exclusive();
        return m_metadataProvider->GetXamlType(fullName);
    }

    virtual ::Platform::Array<XmlnsDefinition>^ GetXmlnsDefinitions() /*override*/
    {
        return m_metadataProvider->GetXmlnsDefinitions();
    }

    // This call is needed to cleanup some MUXC globals before MUXC is unloaded
    void DeinitMuxc()
    {
        LOG_OUTPUT(L"  > Call DeinitializeMUXC.");
        HMODULE hModuleMuxc = GetModuleHandle(L"Microsoft.UI.Xaml.Controls.dll");
        typedef void(__stdcall* PfnDeinitializeMUXC)();
        PfnDeinitializeMUXC pfnDeinitializeMUXC = reinterpret_cast<PfnDeinitializeMUXC>(GetProcAddress(hModuleMuxc, "DeinitializeMUXC"));
        pfnDeinitializeMUXC();
    }

    void Cleanup()
    {
        m_metadataProvider = nullptr;
        m_manager = nullptr;
        DeinitMuxc();
    }

    void Reset()
    {
        DeinitMuxc();
        m_metadataProvider = ref new Microsoft::UI::Xaml::XamlTypeInfo::XamlControlsXamlMetaDataProvider();
    }

private:
    wil::srwlock m_metadataLock;
    IXamlMetadataProvider^ m_metadataProvider;
    Microsoft::UI::Xaml::Hosting::WindowsXamlManager^ m_manager;
};
