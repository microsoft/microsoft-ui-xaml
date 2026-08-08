#requires -Version 5.1
<#
.SYNOPSIS
  Merges the WinAppSDK InteractiveExperiences component-package appxfragment
  into a side-by-side assembly manifest that augments the sample app's
  app.manifest (DPI awareness only) with the lifted-WinRT activatable-class
  registrations missing from the mock aggregator's WindowsAppRuntime MSIX.

.WHY
  TableViewSampleApp is built self-contained + unpackaged (WindowsAppSdkSelfContained=true,
  WindowsPackageType=None). The mock NuGet package
  `Microsoft.WindowsAppSDK.999.0.0-mock-3.0.0-dev-x64-Debug` ships a
  WindowsAppRuntime.1.8-Dev MSIX whose AppxManifest.xml lacks entries for the
  separately-shipped InteractiveExperiences component DLLs (CoreMessagingXP.dll,
  Microsoft.UI.Dispatching, Microsoft.UI.Input, Microsoft.UI.Windowing,
  Microsoft.Graphics.Display, Microsoft.UI.Designer, Microsoft.UI.dll, dcompi.dll,
  wuceffectsi.dll). Without these activatable-class entries the app crashes at
  startup with REGDB_E_CLASSNOTREG when DispatcherQueueController is activated.

.WHAT
  Reads the side-by-side assembly format expected by mt.exe (the same format
  emitted by GenerateAppManifestFromAppx in the mock pkg's SelfContained.targets,
  lines 177-306), produces an augmented manifest that mt.exe will then merge with
  the SDK-generated WindowsAppSDK.manifest to produce the final embedded manifest.

.PARAMETER Base
  Path to the sample's user app.manifest (DPI awareness etc.).

.PARAMETER Fragment
  Path to the IE component package's runtimes-framework\package.appxfragment.

.PARAMETER Out
  Path to write the augmented manifest.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Base,
    [Parameter(Mandatory=$true)][string]$Fragment,
    [Parameter(Mandatory=$true)][string]$Out
)

$ErrorActionPreference = 'Stop'

# Escapes a string for safe inclusion in an XML attribute value (single-quoted attributes).
function Escape-XmlAttr([string]$value)
{
    if ($null -eq $value) { return '' }
    return $value.Replace('&', '&amp;').Replace('<', '&lt;').Replace('>', '&gt;').Replace("'", '&apos;').Replace('"', '&quot;')
}

if (-not (Test-Path -LiteralPath $Base))     { throw "Base manifest not found: $Base" }
if (-not (Test-Path -LiteralPath $Fragment)) { throw "Component appxfragment not found: $Fragment" }

# Load fragment with namespace awareness.
[xml]$fragXml = Get-Content -LiteralPath $Fragment -Raw
$nsmgr = New-Object System.Xml.XmlNamespaceManager($fragXml.NameTable)
$nsmgr.AddNamespace('m', 'http://schemas.microsoft.com/appx/manifest/foundation/windows10')

$ipsNodes = $fragXml.SelectNodes('/m:Fragment/m:Extensions/m:Extension/m:InProcessServer', $nsmgr)
if ($null -eq $ipsNodes -or $ipsNodes.Count -eq 0) {
    $ipsNodes = $fragXml.SelectNodes('/m:Package/m:Extensions/m:Extension/m:InProcessServer', $nsmgr)
}
if ($null -eq $ipsNodes -or $ipsNodes.Count -eq 0) {
    throw "No <InProcessServer> elements found in fragment: $Fragment (XML namespace or schema mismatch?)"
}

# Sentinel class we must see in the output -- if missing, the merge produced nothing useful.
$requiredSentinel = 'Microsoft.UI.Dispatching.DispatcherQueueController'

# Build the augmented manifest as text (mirrors GenerateAppManifestFromAppx's emission style).
[System.Text.StringBuilder]$sb = [System.Text.StringBuilder]::new()
[void]$sb.AppendLine("<?xml version='1.0' encoding='utf-8' standalone='yes'?>")
[void]$sb.AppendLine("<assembly manifestVersion='1.0'")
[void]$sb.AppendLine("    xmlns:asmv3='urn:schemas-microsoft-com:asm.v3'")
[void]$sb.AppendLine("    xmlns:winrtv1='urn:schemas-microsoft-com:winrt.v1'")
[void]$sb.AppendLine("    xmlns='urn:schemas-microsoft-com:asm.v1'>")

# Re-emit the base manifest's <assemblyIdentity> only. We intentionally DROP the
# <application> block (dpiAware/dpiAwareness etc.) because SXS treats it as a
# config-manifest section that is invalid inside the binding-style assembly
# manifest that mt.exe produces for WinRT activation. Including it causes the
# OS loader to reject the EXE with ERROR_SXS_CANT_GEN_ACTCTX
# ("side-by-side configuration is incorrect"). DPI awareness for self-contained
# WinUI3 apps comes from the bootstrapper / Microsoft.UI.Windowing APIs.
[xml]$baseXml = Get-Content -LiteralPath $Base -Raw
$baseNsmgr = New-Object System.Xml.XmlNamespaceManager($baseXml.NameTable)
$baseNsmgr.AddNamespace('asm', 'urn:schemas-microsoft-com:asm.v1')

$baseIdentity = $baseXml.SelectSingleNode('/asm:assembly/asm:assemblyIdentity', $baseNsmgr)
if ($null -ne $baseIdentity) {
    # Hand-emit to avoid the redundant xmlns attribute that XmlElement.OuterXml
    # adds (SXS resolver is finicky about default-namespace duplication).
    $idVersion = $baseIdentity.GetAttribute('version')
    if ([string]::IsNullOrEmpty($idVersion)) { $idVersion = '1.0.0.0' }
    $idName = $baseIdentity.GetAttribute('name')
    if ([string]::IsNullOrEmpty($idName)) { $idName = 'TableViewSampleApp.app' }
    [void]$sb.AppendLine("  <assemblyIdentity version='$(Escape-XmlAttr $idVersion)' name='$(Escape-XmlAttr $idName)'/>")
} else {
    [void]$sb.AppendLine("  <assemblyIdentity version='1.0.0.0' name='TableViewSampleApp.app'/>")
}

$classCount = 0
$dllCount = 0
foreach ($ips in $ipsNodes) {
    $pathNode = $ips.SelectSingleNode('./m:Path', $nsmgr)
    if ($null -eq $pathNode) { continue }
    $dll = $pathNode.InnerText
    [void]$sb.AppendLine("  <asmv3:file name='$(Escape-XmlAttr $dll)'>")
    foreach ($cls in $ips.SelectNodes('./m:ActivatableClass', $nsmgr)) {
        $name = $cls.GetAttribute('ActivatableClassId')
        if ([string]::IsNullOrEmpty($name)) { continue }
        $threading = $cls.GetAttribute('ThreadingModel')
        if ([string]::IsNullOrEmpty($threading)) { $threading = 'both' } else { $threading = $threading.ToLowerInvariant() }
        [void]$sb.AppendLine("    <winrtv1:activatableClass name='$(Escape-XmlAttr $name)' threadingModel='$(Escape-XmlAttr $threading)'/>")
        $classCount++
    }
    [void]$sb.AppendLine("  </asmv3:file>")
    $dllCount++
}
# --- Split-binary Tabular.dll registration ---------------------------------
# The Tabular control ships in a SEPARATE lifted-WinRT binary
# (Microsoft.UI.Xaml.Controls.Tabular.dll) whose runtimeclasses are NOT present in
# the IXP component fragment above. Without these entries the app fail-fasts with
# 0x80040111 CLASS_E_CLASSNOTAVAILABLE the instant it activates
# XamlControlsTabularXamlMetaDataProvider / TableView. This list is the set of
# activatable runtimeclasses in the built Tabular winmds
# (controls\dev\dll-tabular\Merged\*.winmd); enums are not activatable and are omitted.
[void]$sb.AppendLine("  <asmv3:file name='Microsoft.UI.Xaml.Controls.Tabular.dll'>")
foreach ($tabularClass in @(
    'Microsoft.UI.Xaml.Controls.Tabular.TableView',
    'Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn',
    'Microsoft.UI.Xaml.Controls.Tabular.TableViewTextColumn',
    'Microsoft.UI.Xaml.Controls.Tabular.TableViewTemplateColumn',
    'Microsoft.UI.Xaml.Controls.Tabular.TableViewRow',
    'Microsoft.UI.Xaml.Controls.Tabular.TableViewCellsPanel',
    'Microsoft.UI.Xaml.Controls.Tabular.TabularControlsResources',
    'Microsoft.UI.Xaml.Controls.Tabular.TableViewAutomationPeer',
    'Microsoft.UI.Xaml.Controls.Tabular.TableViewRowAutomationPeer',
    'Microsoft.UI.Xaml.Controls.Tabular.TableViewColumnHeaderAutomationPeer',
    'Microsoft.UI.Xaml.Controls.Tabular.TableViewCellAutomationPeer',
    'Microsoft.UI.Xaml.XamlTypeInfo.XamlControlsTabularXamlMetaDataProvider'
)) {
    [void]$sb.AppendLine("    <winrtv1:activatableClass name='$(Escape-XmlAttr $tabularClass)' threadingModel='both'/>")
    $classCount++
}
[void]$sb.AppendLine("  </asmv3:file>")
$dllCount++

[void]$sb.AppendLine("</assembly>")

$result = $sb.ToString()

if ($result -notmatch [regex]::Escape($requiredSentinel)) {
    throw "Augmented manifest does not contain required activatable class '$requiredSentinel' -- fragment parse produced $classCount classes across $dllCount DLLs but the sentinel is missing."
}

$outDir = Split-Path -Parent $Out
if (-not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
[System.IO.File]::WriteAllText($Out, $result, [System.Text.Encoding]::UTF8)

Write-Host "MergeIxpAppManifest: wrote $Out -- $dllCount DLLs, $classCount activatable classes."
