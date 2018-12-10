# First we'll make sure that we're running in a razzle context, and store the path to the
# repository if we are.
if ($($env:SDXROOT).Length -eq 0)
{
    Write-Host "This script must be run from a razzle environment."
    exit 1
}

$windowsPath = "$env:SDXROOT\onecoreuap\windows\dxaml\controls"
$outDir = "$env:OBJECT_ROOT\onecoreuap\windows\dxaml\controls\dev\dll\$env:_BuildAlt"

$env:ERRORLEVEL = "0"
$LASTEXITCODE = 0

# In order to ensure that we don't unnecessarily do lengthy processing when nothing has actually changed,
# we'll maintain a list of hashes for files associated with operations that take a while.
# That way, if the hashes match, we'll know that nothing has changed and that we can skip the task in question.
Import-Module .\HashingHelpers.psm1 -DisableNameChecking
Load-FileHashes .cachedFileHashes

function Get-WasError
{
    return ($env:ERRORLEVEL -ne $null -and $env:ERRORLEVEL.Length -gt 0 -and $env:ERRORLEVEL -ne "0") -or ($LASTEXITCODE -ne 0)
}

function Report-FailedStep
{
    Param(
        [Parameter(Mandatory = $true)] 
        [string]$stepName
    )

    Write-Error "$stepName FAILED."
    exit 1
}

if (-not (Test-Path $outDir))
{
    New-Item $outDir -ItemType Directory 2>&1> $null
}
else
{
    Get-ChildItem "$outDir" -File -Recurse | ForEach-Object { Remove-Item $_.FullName }
}

$scriptPath = "$windowsPath\tools"
$codeGenPath = "$env:SDXROOT\onecoreuap\windows\dxaml\xcp\tools\XCPTypesAutoGen\Modules\DEPControls\"
$codeGennedFilePath = "$($env:OBJECT_ROOT)\onecoreuap\windows\dxaml\xcp\tools\xcptypesautogen\runcodegen\$($env:_BuildAlt)\DEPControls"
$genericWuxcXamlPath = "$outDir\GenericWuxcXaml"
$genericXamlPath = "$env:SDXROOT\onecoreuap\windows\dxaml\xcp\dxaml\themes\generic.xaml"

[System.Collections.Generic.List[string]]$publicPageList = @()

Get-ChildItem dev\*.vcxitems -Recurse | ForEach-Object {
    $xmlStringReader = New-Object System.IO.StringReader -ArgumentList (Get-Content $_.FullName -Raw)
    $xmlReader = New-Object System.Xml.XmlTextReader -ArgumentList $xmlStringReader
    $xmlReader.Namespaces = $false # We don't care if there are unresolved namespaces - we're just interested in the text.
    $xmlDocument = New-Object System.Xml.XmlDocument
    $xmlDocument.Load($xmlReader)

    $directory = (Split-Path $_.FullName)
    $pageElements = $xmlDocument.GetElementsByTagName("Page")

    $pageElements | ForEach-Object {
        if ($_.IsPublic -eq $null -or $_.IsPublic -inotlike "*false*")
        {
            $publicPageList.Add($_.Include.Replace("`$(MSBuildThisFileDirectory)","$directory\"))
        }
    }
}

if (-not (Test-Path $genericWuxcXamlPath))
{
    New-Item $genericWuxcXamlPath -ItemType Directory 2>&1> $null
}

[System.Collections.Generic.List[string]]$pagesChanged = @()

$publicPageList | ForEach-Object {
    if (Get-FileHasChanged $_)
    {
        $pagesChanged.Add($_)
    }
}

if ($pagesChanged.Length -gt 0)
{
    Write-Host
    Write-Host "WUXC XAML files have changed:"
    Write-Host 
    Write-Host $($pagesChanged -join [System.Environment]::NewLine)
    Write-Host
    Write-Host "Merging them together..."
    Write-Host
    Write-Host "msbuild.cmd `"$windowsPath\dev\dll\Microsoft.UI.Xaml.vcxproj`" /nologo /p:BuildingWithBuildExe=true /clp:NoSummary /verbosity:normal /Target:GenerateWUXCGenericXamlFile"
    & msbuild.cmd "$windowsPath\dev\dll\Microsoft.UI.Xaml.vcxproj" /nologo /p:BuildingWithBuildExe=true /clp:NoSummary /verbosity:normal /Target:GenerateWUXCGenericXamlFile

    if (Get-WasError)
    {
        Report-FailedStep "Merge"
    }
}
else
{
    Write-Host "WUXC XAML files have not changed. Skipping merge."
}

$publicPageList | ForEach-Object {
    Save-FileHash $_ -fileNameOnly
}

[System.Collections.Generic.List[string]]$idlOrWinmdFilesChanged = @()
$idlFiles = (Get-ChildItem .\*.idl -Recurse)

$idlFiles | ForEach-Object {
    if (Get-FileHasChanged $_.FullName)
    {
        $idlOrWinmdFilesChanged.Add($_.FullName)
    }
}

$localWinmdPath = "$env:OBJECT_ROOT\onecoreuap\windows\dxaml\controls\idl\$env:_BuildAlt"
$builtIdlFiles = (Get-ChildItem idl\*.idl -Recurse)

[System.Collections.Generic.List[string]]$localWinmdFiles = @()

$builtIdlFiles | ForEach-Object {
    $localWinmdFiles.Add("$localWinmdPath\$($_.Name.Replace(".idl", ".winmd"))")
}

$localWinmdFiles | ForEach-Object {
    if (Get-FileHasChanged $_)
    {
        $idlOrWinmdFilesChanged.Add($_)
    }
}

if ($idlOrWinmdFilesChanged.Length -gt 0)
{
    Write-Host
    Write-Host "IDL or WinMD files have changed:"
    Write-Host 
    Write-Host $($idlOrWinmdFilesChanged -join [System.Environment]::NewLine)
    Write-Host
    Write-Host "Building WinMD files..."
    Write-Host
    $buildCommand = "build /c /dir `"$env:SDXROOT\onecoreuap\windows\dxaml\controls\idl;$env:SDXROOT\onecoreuap\merged\winmetadata\ContractMetadata;$env:SDXROOT\onecoreuap\merged\winmetadata\InternalMetadata;$env:SDXROOT\onecoreuap\merged\winmetadata\SdkMetadata;$env:SDXROOT\onecoreuap\merged\winmetadata\SystemMetadata`""
    Write-Host $buildCommand
    Invoke-Expression $buildCommand

    if (Get-WasError)
    {
        Report-FailedStep "Building"
    }
}
else
{
    Write-Host "IDL files have not changed. Skipping WinMD file build."
}

$idlFiles | ForEach-Object {
    Save-FileHash $_.FullName -fileNameOnly
}

$localWinmdFiles | ForEach-Object {
    Save-FileHash $_ -fileNameOnly
}

[System.Collections.Generic.List[string]]$internalWinmds = @()

Get-ChildItem $env:PUBLIC_ROOT\internal\onecoreuap\internal\buildmetadata\internal\*.winmd | ForEach-Object {
    $internalWinmds.Add($_.FullName)
}

[string[]]$metadataWinmds = @(
    "$env:PUBLIC_ROOT\internal\onecoreuap\internal\buildmetadata\windows.winmd"
    "$env:PUBLIC_ROOT\internal\onecoreuap\internal\buildmetadata\internal\windows.ui.winmd")
[string[]]$referenceWinmds = @(
    "$localWinmdPath\microsoft.ui.xaml.winmd",
    "$localWinmdPath\effects.winmd") + $metadataWinmds + $internalWinmds
[string[]]$typeHintWinmds = @(
    "$localWinmdPath\microsoft.ui.xaml.winmd")
$parameterValuesString = "'MetadataWinmdPaths=$($metadataWinmds -join ';'),ReferenceWinmds=$($referenceWinmds -join ';'),TypeHintWinmds=$($typeHintWinmds -join ';')'"

[System.Collections.Generic.List[string]]$winmdFilesChanged = @()

$metadataWinmds | ForEach-Object {
    if (Get-FileHasChanged $_)
    {
        $winmdFilesChanged.Add($_)
    }
}

$referenceWinmds | ForEach-Object {
    if (Get-FileHasChanged $_)
    {
        $winmdFilesChanged.Add($_)
    }
}

$typeHintWinmds | ForEach-Object {
    if (Get-FileHasChanged $_)
    {
        $winmdFilesChanged.Add($_)
    }
}

[System.Collections.Generic.List[string]]$codeGenInputFilesChanged = $winmdFilesChanged

if (Get-FileHasChanged $windowsPath\dev\dll\XamlMetadataProviderWindowsCodeGen.tt)
{
    $codeGenInputFilesChanged.Add("$windowsPath\dev\dll\XamlMetadataProviderWindowsCodeGen.tt")
}

if (Get-FileHasChanged $windowsPath\dev\dll\CommonHelpers.tt)
{
    $codeGenInputFilesChanged.Add("$windowsPath\dev\dll\CommonHelpers.tt")
}

if ($codeGenInputFilesChanged.Length -gt 0)
{
    Write-Host
    Write-Host "WUXC code gen input has changed:"
    Write-Host 
    Write-Host $($codeGenInputFilesChanged -join [System.Environment]::NewLine)
    Write-Host
    Write-Host "Generating WUXC code gen file..."
    Write-Host

    $expression = "$scriptPath\ProcessTextTemplate.ps1 -FilePath $windowsPath\dev\dll\XamlMetadataProviderWindowsCodeGen.tt -OutputPath $codeGenPath\DEPControls.cs -ParameterValuesString $parameterValuesString"
    Write-Host $expression
    Write-Host
    Invoke-Expression $expression
    Write-Host

    if (Get-WasError)
    {
        Report-FailedStep "Generation"
    }

    # Run the DependencyPropertyCodeGen script by compiling from source (grabbing the nuget or pre-built dll are
    # both a pain to do from OS repo and importing and running works ok).

    Write-Host
    Write-Host "Running DependencyPropertyCodeGen custom task..."
    Write-Host

    $source = [System.IO.File]::ReadAllText("$windowsPath\dev\generated\MetadataSummary.cs");
    Add-Type -TypeDefinition $source
    $metadataSummary = new-object CustomTasks.MetadataSummary

    $source = [System.IO.File]::ReadAllText("$windowsPath\tools\CustomTasks\DependencyPropertyCodeGen.cs");
    Add-Type -TypeDefinition $source

    $task = New-Object -TypeName CustomTasks.DependencyPropertyCodeGen
    $task.WinMDInput = $metadataWinmds
    $task.References = $referenceWinmds
    $task.OutputDirectory = "$windowsPath\dev\generated"
    $task.IncludedTypesMetadata = $metadataSummary.IncludedTypesMetadata
    $task.HasCustomActivationFactoryMetadata = $metadataSummary.HasCustomActivationFactoryMetadata
	$task.NeedsDependencyPropertyFieldMetadata = $metadataSummary.NeedsDependencyPropertyFieldMetadata
    $task.NeedsPropChangedCallbackMetadata = $metadataSummary.NeedsPropChangedCallbackMetadata
    $task.PropChangedCallbackMethodNameMetadata = $metadataSummary.PropChangedCallbackMethodNameMetadata
	$task.PropValidationCallbackMetadata = $metadataSummary.PropValidationCallbackMetadata
	$task.PropertyTypeOverrideMetadata = $metadataSummary.PropertyTypeOverrideMetadata
    $task.DefaultValueMetadata = $metadataSummary.DefaultValueMetadata

    $result = $task.Execute()

    if (!$result -or (Get-WasError))
    {
        Report-FailedStep "DependencyPropertyCodeGen"
    }

}
else
{
    Write-Host "WUXC code gen input has not changed. Skipping WUXC code gen file generation."
}

Save-FileHash $windowsPath\dev\dll\XamlMetadataProviderWindowsCodeGen.tt -fileNameOnly

[System.Collections.Generic.List[string]]$filterTypesGenerationInputFilesChanged = $winmdFilesChanged

if (Get-FileHasChanged $windowsPath\dev\dll\CppWinRTFilterTypes.tt)
{
    $filterTypesGenerationInputFilesChanged.Add("$windowsPath\dev\dll\CppWinRTFilterTypes.tt")
}

if ($filterTypesGenerationInputFilesChanged.Length -gt 0)
{
    Write-Host
    Write-Host "C++/WinRT filter types generation input has changed:"
    Write-Host 
    Write-Host $($filterTypesGenerationInputFilesChanged -join [System.Environment]::NewLine)
    Write-Host
    Write-Host "Generating file..."
    Write-Host

    $expression = "$scriptPath\ProcessTextTemplate.ps1 -FilePath $windowsPath\dev\dll\CppWinRTFilterTypes.tt -OutputPath $windowsPath\dev\dll\CppWinRTFilterTypes.txt -ParameterValuesString $parameterValuesString"
    Write-Host $expression
    Write-Host
    Invoke-Expression $expression
    Write-Host

    if (Get-WasError)
    {
        Report-FailedStep "Generation"
    }
}
else
{
    Write-Host "C++/WinRT filter types file has not changed. Skipping generation."
}

Save-FileHash $windowsPath\dev\dll\CppWinRTFilterTypes.tt -fileNameOnly

$metadataWinmds | ForEach-Object {
    Save-FileHash $_ -fileNameOnly
}

$referenceWinmds | ForEach-Object {
    Save-FileHash $_ -fileNameOnly
}

$typeHintWinmds | ForEach-Object {
    Save-FileHash $_ -fileNameOnly
}

[System.Collections.Generic.List[string]]$codeGenFilesChanged = @()

if (Get-FileHasChanged "$codeGenPath\DEPControls.cs")
{
    $codeGenFilesChanged.Add("$codeGenPath\DEPControls.cs")
}

if (Get-FileHasChanged "$codeGennedFilePath\Microsoft-Windows-UI-Xaml-DEPControls.man")
{
    $codeGenFilesChanged.Add("$codeGennedFilePath\Microsoft-Windows-UI-Xaml-DEPControls.man")
}

if (Get-FileHasChanged "$codeGennedFilePath\XamlTypeInfo.g.h")
{
    $codeGenFilesChanged.Add("$codeGennedFilePath\XamlTypeInfo.g.h")
}

if (Get-FileHasChanged "$codeGennedFilePath\XamlTypeInfo.g.rc")
{
    $codeGenFilesChanged.Add("$codeGennedFilePath\XamlTypeInfo.g.rc")
}

if ($codeGenFilesChanged.Length -gt 0)
{
    Write-Host
    Write-Host "Code gen files have changed:"
    Write-Host 
    Write-Host $($codeGenFilesChanged -join [System.Environment]::NewLine)
    Write-Host
    Write-Host "Running code gen..."
    Write-Host

    Push-Location "$env:SDXROOT\onecoreuap\windows\dxaml\xcp"
    & .\runcodegen.cmd
    Pop-Location

    Write-Host

    if (Test-Path "$env:SDXROOT\onecoreuap\windows\dxaml\xcp\tools\XCPTypesAutoGen\RunCodeGen\build$($env:_BuildType).err")
    {
        Report-FailedStep "Code gen"
    }
}
else
{
    Write-Host "Code gen files have not changed. Skipping code gen and copying already-generated files..."
    Copy-Item "$codeGennedFilePath\Microsoft-Windows-UI-Xaml-DEPControls.man" "$windowsPath\manifest\"
    Copy-Item "$codeGennedFilePath\XamlTypeInfo.g.h" "$windowsPath\dev\dll\"
    Copy-Item "$codeGennedFilePath\XamlTypeInfo.g.rc" "$windowsPath\dev\dll\"
}

Save-FileHash "$codeGenPath\DEPControls.cs" -fileNameOnly
Save-FileHash "$codeGennedFilePath\Microsoft-Windows-UI-Xaml-DEPControls.man" -fileNameOnly
Save-FileHash "$codeGennedFilePath\XamlTypeInfo.g.h" -fileNameOnly
Save-FileHash "$codeGennedFilePath\XamlTypeInfo.g.rc" -fileNameOnly

[System.Collections.Generic.List[string]]$xamlFilesToTest = @(
    "$genericWuxcXamlPath\generic_wuxc_rs1.xaml",
    "$genericWuxcXamlPath\generic_wuxc_rs2.xaml",
    "$genericWuxcXamlPath\generic_wuxc_rs3.xaml",
    "$genericWuxcXamlPath\generic_wuxc_rs4.xaml",
    "$genericWuxcXamlPath\generic_wuxc_rs5.xaml",
    "$genericWuxcXamlPath\generic_wuxc_19h1.xaml",
    "$genericWuxcXamlPath\generic_wuxc_rs1_themeresources.xaml",
    "$genericWuxcXamlPath\generic_wuxc_rs2_themeresources.xaml",
    "$genericWuxcXamlPath\generic_wuxc_rs3_themeresources.xaml",
    "$genericWuxcXamlPath\generic_wuxc_rs4_themeresources.xaml",
    "$genericWuxcXamlPath\generic_wuxc_rs5_themeresources.xaml",
    "$genericWuxcXamlPath\generic_wuxc_19h1_themeresources.xaml")
    
[System.Collections.Generic.List[string]]$xamlFilesChanged = @()

$xamlFilesExist = $false
$xamlFilesToTest | ForEach-Object {
    $xamlFilesExist = $xamlFilesExist -or (Test-Path $_) 
}

if ($xamlFilesExist)
{
    $xamlFilesToTest | ForEach-Object {
        if (Get-FileHasChanged $_)
        {
            $xamlFilesChanged.Add($_)
        }
    }

    if (Get-FileHasChanged $genericXamlPath)
    {
        $xamlFilesChanged.Add($genericXamlPath)
    }

    if ($xamlFilesChanged.Length -gt 0)
    {
        Write-Host
        Write-Host "XAML has changed:"
        Write-Host 
        Write-Host $($xamlFilesChanged -join [System.Environment]::NewLine)
        Write-Host
        Write-Host "Merging XAML files into generic.xaml..."
        Write-Host
    
        Write-Host "$scriptPath\GenerateMergedXaml.cmd -MergedXamlFilePath $genericXamlPath -BaseXamlFile $genericXamlPath -XamlFileList `"$($xamlFilesToTest -join ';')`" -MergedXamlName Windows.UI.Xaml.Controls.dll -RemoveUsings"
        Write-Host
        & $scriptPath\GenerateMergedXaml.cmd -MergedXamlFilePath $genericXamlPath -BaseXamlFile $genericXamlPath -XamlFileList "$($xamlFilesToTest -join ';')" -MergedXamlName Windows.UI.Xaml.Controls.dll -RemoveUsings
        Write-Host

        if (Get-WasError)
        {
            Report-FailedStep "Merge"
        }
        
        Write-Host
        Write-Host "Building resources..."
        Write-Host
        
        Push-Location "$env:SDXROOT\onecoreuap\windows\dxaml\xcp"
        & .\buildresources.cmd
        Pop-Location

        if (Get-WasError)
        {
            Report-FailedStep "Building resources"
        }
    }
    else
    {
        Write-Host "XAML has not changed. Skipping XAML merge into generic.xaml."
    }
}
else
{
    Write-Host "XAML has not changed. Skipping XAML merge into generic.xaml."
}

Save-FileHash $genericXamlPath -fileNameOnly

$xamlFilesToTest | ForEach-Object {
    if (Test-Path $_)
    {
        Save-FileHash $_ -fileNameOnly
    }
    else
    {
        Preserve-FileHash $_ -fileNameOnly
    }
}

Write-Host
Write-Host "Committing hashes of code-gen files..."
Write-Host

Commit-FileHashes .cachedFileHashes

& git add $codeGenPath\DEPControls.cs
& git add $windowsPath\dev\dll\CppWinRTFilterTypes.txt
& git add $genericXamlPath
& git add $themeResourcesPath
& git add $windowsPath\dev\Generated\*

Write-Host "Code gen complete!"
Write-Host
