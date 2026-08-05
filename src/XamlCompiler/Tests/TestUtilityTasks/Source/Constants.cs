// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------------
// Copyright(c) 2014 Microsoft Corporation
//--------------------------------------------------------------------------------------------
namespace XamlCompilerTestsUtilityTasks
{
    internal static class Constants
    {
        internal const string uapManagedManifestSubPath = @"\vsproject\Templates\Windows\WinRTCommon\ProjectTemplates\Windows_UAP_Common\Package\Package-managed.appxmanifest";
        internal const string xamlCompilerRegressionTestsRootSubPath = @"\vsproject\XamlCompiler\Tests\RegressionProjects";
        internal const string packageManifestFileName = "Package.appxmanifest";
        internal const string WindowsSDKsString = "Windows";

        /* Do not change values without also changing the corresponding values in CSharp/Simple/App.xaml & MainPage.xaml.
        These values are used to test incremental build and runtime behavior*/

        internal const string CSharpSimpleMainGridWhiteColor = "CSharpSimpleMainGridWhiteColor";
        internal const string CSharpSimpleMainGridBlackColor = "CSharpSimpleMainGridBlackColor";
    }
}
