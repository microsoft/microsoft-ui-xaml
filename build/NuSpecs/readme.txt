Thanks for installing the WinUI NuGet package!

Don't forget to set XamlControlsResources as your Application resources in App.xaml:

    <Application>
        <Application.Resources>
            <XamlControlsResources xmlns="using:Microsoft.UI.Xaml.Controls" /> 
        </Application.Resources>
    </Application>

or if you have other resources then add XamlControlsResources at the top as a merged dictionary:

    <Application.Resources>
        <ResourceDictionary>
            <ResourceDictionary.MergedDictionaries>
                <XamlControlsResources xmlns="using:Microsoft.UI.Xaml.Controls" />
            </ResourceDictionary.MergedDictionaries>
                       
        </ResourceDictionary>
    </Application.Resources>

See http://aka.ms/winui for more information.
