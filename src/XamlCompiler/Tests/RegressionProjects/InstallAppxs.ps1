# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
# TODO: Dynamically generate the commands to remove and install app packages
"Removing existing app packages .."
remove-appxpackage -Package "f7bfb072-e040-4291-b2d0-e76b148fa8c5_1.0.0.0_x86__ps2m9tw0qshfy"
remove-appxpackage -Package "205487c2-7d3d-4003-967c-0a5e16ddae6f_1.0.0.0_x86__9tfqybwavkqx6"
remove-appxpackage -Package "a09efa80-7b73-456c-9bdb-98ca3c33e684_1.0.0.0_x86__ps2m9tw0qshfy"
remove-appxpackage -Package "4ab00967-c18b-44e8-a3f6-0e14ad6750cf_1.0.0.0_x86__ps2m9tw0qshfy"
"Installing new packages .."
$rootDrive = $env:_NTDRIVE
$rootSource = $env:_NTROOT
$command = $rootDrive+$rootSource+"\vsproject\XamlCompiler\Tests\RegressionProjects\Basic\CSharp\Simple\AppPackages\Simple_1.0.0.0_x86_Debug_Test\Add-AppDevPackage.ps1 -Force"
iex $command
$command = $rootDrive+$rootSource+"\vsproject\XamlCompiler\Tests\RegressionProjects\Features\CompiledBinding\BindTestbedCS\AppPackages\BindTestbedCS_1.0.0.0_x86_Debug_Test\Add-AppDevPackage.ps1 -Force"
iex $command
$command = $rootDrive+$rootSource+"\vsproject\XamlCompiler\Tests\RegressionProjects\Features\CompiledBinding\BindTestbedVB\AppPackages\BindTestbedVB_1.0.0.0_x86_Debug_Test\Add-AppDevPackage.ps1 -Force"
iex $command
$command = $rootDrive+$rootSource+"\vsproject\xamlcompiler\Tests\RegressionProjects\Features\CompiledBinding\AppPackages\BindTestbedCX\BindTestbedCX_1.0.0.0_Win32_Debug_Test\Add-AppDevPackage.ps1 -Force"
iex $command
"Installation Complete !"
