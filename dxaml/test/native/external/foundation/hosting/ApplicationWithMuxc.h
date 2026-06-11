// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

ref class XamlIslandTests_ApplicationWithMuxc sealed
    : public Microsoft::UI::Xaml::Application
    , public IXamlMetadataProvider
{
public:
    XamlIslandTests_ApplicationWithMuxc()
    {
        LOG_OUTPUT(L"    > [XamlIslandTests_ApplicationWithMuxc] Set MUXC MetadataProvider");
        m_metadataProvider = ref new Microsoft::UI::Xaml::XamlTypeInfo::XamlControlsXamlMetaDataProvider();

        LOG_OUTPUT(L"    > [XamlIslandTests_ApplicationWithMuxc] Create WindowsXamlManager");
        m_manager = Microsoft::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();

        s_didDestructorRun = false;
    }

    virtual ~XamlIslandTests_ApplicationWithMuxc()
    {
        LOG_OUTPUT(L"    > [XamlIslandTests_ApplicationWithMuxc] Destructor");
        s_didDestructorRun = true;
    }

    void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs^) override
    {
        LOG_OUTPUT(L"    > [XamlIslandTests_ApplicationWithMuxc] Load MUXC Resources");
        this->Resources->MergedDictionaries->Append(ref new Microsoft::UI::Xaml::Controls::XamlControlsResources());
    }

    // IXamlMetadataProvider
    virtual IXamlType^ GetXamlType(::Windows::UI::Xaml::Interop::TypeName type) /*override*/
    {
        return m_metadataProvider->GetXamlType(type);
    }

    virtual IXamlType^ GetXamlType(Platform::String^ fullName) /*override*/
    {
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

    static bool DidDestructorRun()
    {
        return s_didDestructorRun;
    }

private:
    IXamlMetadataProvider^ m_metadataProvider;
    Microsoft::UI::Xaml::Hosting::WindowsXamlManager^ m_manager;

    inline static bool s_didDestructorRun {false};
};
