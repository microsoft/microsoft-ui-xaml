# This project is for framework package only, and the Microsoft.UI.Xaml.pri is picked up by the framework package.
# Allow single URI to access Compact.xaml from both framework package and nuget package

<ResourceDictionary xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation">
    <ResourceDictionary.MergedDictionaries>
        <ResourceDictionary Source="ms-appx:///Microsoft.UI.Xaml/DensityStyles/Compact.xaml"/>
    </ResourceDictionary.MergedDictionaries>
</ResourceDictionary>