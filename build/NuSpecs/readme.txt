Thanks for installing the WinUI NuGet package!

Don't forget to set XamlControlsResources as your Application resources in App.xaml:

    <Application>
        <Application.Resources>
            <XamlControlsResources xmlns="using:Microsoft.UI.Xaml.Controls" />
        </Application.Resources>
    </Application>

If you have other resources then we recommend you add those to the XamlControlsResources' MergedDictionaries.
This works with the platform's resource system to allow overrides of the XamlControlsResources resources.

<ResourceDictionary
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:controls="using:Microsoft.UI.Xaml.Controls"/>

    <Application>
        <Application.Resources>
            <controls:XamlControlsResources>
                <controls:XamlControlsResources.MergedDictionaries>
                    <!-- Other app resources here -->
                </controls:XamlControlsResources.MergedDictionaries>
            </controls:XamlControlsResources>
        </Application.Resources>
    </Application>

See http://aka.ms/winui for more information.
