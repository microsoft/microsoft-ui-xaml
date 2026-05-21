Set-StrictMode -Version 3.0
$ErrorActionPreference = "Stop"

# Processes the Intellisense .xml files in Intellisense/drop and generates new Intellisense
# .xml files in Intellisense/generated.
# See Intellisense.md for more information.

$StartTime = Get-Date
Write-Host "Generating Intellisense XML files." -ForegroundColor Cyan

$IntellisenseRoot = $PSScriptRoot
$RepoRoot = Join-Path $PSScriptRoot "..\.." -Resolve

$IntellisenseDir = "$IntellisenseRoot\drop"
if ($env:Configuration -eq '')
{
    Write-Error "Environment variable 'Configuration' not set. Run init.cmd first."
    exit 1;
}

$NugetLibDir = "$RepoRoot\BuildOutput\packaging\$($env:Configuration)\lib"
$OutputDir = "$IntellisenseRoot\generated"
$WorkingDir = "$RepoRoot\BuildOutput\Intellisense"

if (Test-Path $WorkingDir)
{
    Remove-Item $WorkingDir -Recurse
}

if (-not (Test-Path $WorkingDir))
{
    New-Item -ItemType Directory -Path $WorkingDir | Out-Null
}

# All directories in the Nuget package that contain managed DLLs and WinMDs.
# We only do the latest versions of each target framework, since they'll contain the most info.
$AssemblyDirs = @(
    "$NugetLibDir\uap10.0",
    "$NugetLibDir\net6.0-windows10.0.17763.0")

# FullXml will contain all the intellisense xml files combined.
[xml]$FullXml = New-Object System.Xml.XmlDocument
$FullXmlDoc = $FullXml.AppendChild($FullXml.CreateElement("doc"))
$FullXmlMembers = $FullXmlDoc.AppendChild($FullXml.CreateElement("members"))

function ReadIntellisenseFiles()
{
    # Keep track of the APIs we've seen before so we can avoid duplicates.
    $VisitedApi = @{}

    foreach ($F in Get-ChildItem $intellisenseDir -Filter "*.xml")
    {
        $Filename = $F.FullName
        Write-Host "Reading $Filename"
    
        # Load as xml
        [xml]$Xml = Get-Content $Filename

        $Members = $Xml.DocumentElement.SelectSingleNode("members")

        $CurrentType = ''

        # Iterate over members
        # The member names are like this:
        #   T:Namespace.Type
        #   M:Namespace.Type.MethodName
        #   P:Namespace.Type.PropertyName
        #   E:Namespace.Type.EventName
        foreach ($Member in $Members.ChildNodes)
        {
            $Name = $Member.GetAttribute("name")
            if ($Name.StartsWith("T:"))  # T: means "type"
            {        
                $Name = $Name.SubString(2)
                $CurrentType = $Name
            }
            else
            {
                if (-not $Name.SubString(2).StartsWith($CurrentType))
                {
                    # This script can be simple because all the members of a given type are grouped together.
                    # If that ever stops being true, we'll detect it here and bail.
                    throw "Unexpected member name: $Name"
                }
            }

            if ($VisitedApi[$Name] -eq $null)
            {
                $FullXmlMembers.AppendChild($FullXml.ImportNode($Member, $true)) | Out-Null
                $VisitedApi[$Name] = $true;
            }
        }
    }

    Write-Host "Intellisense Xml files contain $($VisitedApi.Count) unique members."
    Write-Host "Saving full intellisense xml $WorkingDir\FullIntellisenseRaw.xml for debugging purposes"
    $FullXml.Save("$WorkingDir\FullIntellisenseRaw.xml")
}

# Given "Namespace1.Namespace2.TypeName", return "TypeName"
function GetTypeNameFromFullName()
{
    param (
        [string]$FullName
    )
    if ($FullName -match "^.*\.(.*)$")
    {
        return $Matches[1]
    }
    return $FullName
}

# Fast way to check if a string needs to be cleaned up.
function NeedsToBeCleaned()
{
    param (
        [string]$Message
    )

    if ($Message.contains('<xref:')) { return $true }
    if ($Message.contains('<!--')) { return $true }
    return $false
}

# Remove some known problematic patterns from an intellisense string.
function CleanMessage()
{
    param (
        [string]$Message
    )

    $Iterations = 0

    # Remove HTML comments (\s\S will match across newlines too)
    $Message = $Message -replace "<!--[\s\S]*-->", ''

    # The intellisense drops we get have strings like this that don't render correctly in VS:
    #   ... <xref:Microsoft.UI.Xaml.Media.GradientStop?text=GradientStop> ...
    # We'll remove that and just keep the text.
    $Message = $Message -replace "<xref:\S*\?text=(.*?)>", '$1'

    # Remove the query string from strings like this:
    #   <xref:Microsoft.Windows.Widgets.Providers.IWidgetProvider.OnActionInvoked(Microsoft.Windows.Widgets.Providers.WidgetActionInvokedArgs)?displayProperty=nameWithType>
    $Message = $Message -replace "<xref:(\S*)\?.*>", ("<xref:" + '$1' + ">")

    # Also, a string line this:
    #   <xref:Microsoft.Windows.Widgets.Feeds.Providers.IFeedProvider.OnFeedEnabled(Microsoft.Windows.Widgets.Feeds.Providers.FeedEnabledArgs)>
    # Renders on learn.microsoft.com as:
    #   OnFeedEnabled(FeedEnabledArgs)
    # We need to loop because it will only match one at a time.
    while ($Message -match "(<xref:(\S*)\((\S*)\)>)" -and $Iterations++ -lt 100)
    {
        $Message = $Message.Replace($Matches[1], ((GetTypeNameFromFullName $Matches[2]) + "(" + (GetTypeNameFromFullName $Matches[3]) + ")"))
    }

    # Also, this pattern:
    #   ... <xref:Microsoft.Windows.Widgets.Providers.WidgetActionInvokedArgs> ...
    # Renders on learn.microsoft.com as:
    #   WidgetActionInvokedArgs
    # Keep this last because it'll match on some other patterns.
    # We need to loop because it will only match one at a time.
    while ($Message -match "(<xref:(\S*)>)" -and $Iterations++ -lt 100)
    {
        $Message = $Message.Replace($Matches[1], (GetTypeNameFromFullName $Matches[2]))
    }

    if ($Iterations -ge 100)
    {
        throw "CleanMessage hit 100 iterations.  This is unexpected."
    }

    return $Message
}

function CleanIntellisenseData()
{
    Write-Host "Cleaning intellisense strings"

    [string]$ModifiedStrings = ''

    # Examine all nodes of $FullXml
    foreach ($Member in $FullXml.DocumentElement.SelectNodes("members/member"))
    {
        foreach ($Node in $Member.ChildNodes)
        {
            [string]$rawString = $Node.InnerText
            if (NeedsToBeCleaned $Node.InnerText)
            {
                $newString = CleanMessage $rawString
                if ($newString -ne $Node.InnerText)
                {
                    $ModifiedStrings += "$($Member.name)`n"
                    $ModifiedStrings += "  $($Node.InnerText)`n"
                    $ModifiedStrings += "  $newString`n"
                    $Node.InnerText = $newString
                }
            }

            # Make sure we got rid of all the xrefs, etc.
            if (NeedsToBeCleaned $Node.InnerText)
            {
                Write-Host "Warning: Node appears to not be fully cleaned: $($Member.name)" -ForegroundColor Yellow
                Write-Host "Raw string:" -ForegroundColor Yellow
                Write-Host "  $($rawString)" -ForegroundColor Yellow
                Write-Host "Cleaned string:" -ForegroundColor Yellow
                Write-Host "  $($Node.InnerText)" -ForegroundColor Yellow
            }

            # Fix the line endings to CRLF
            $Node.InnerText = $Node.InnerText -replace "`r`n", "`n"
            $Node.InnerText = $Node.InnerText -replace "`n", "`r`n"
        }
    }

    Write-Host "Saving full intellisense xml $WorkingDir\FullIntellisenseCleaned.xml for debugging purposes"
    $FullXml.Save("$WorkingDir\FullIntellisenseCleaned.xml")

    Write-Host "Saving string changes to $WorkingDir\ModifiedStrings.txt for debugging purposes"
    $ModifiedStrings > $WorkingDir\ModifiedStrings.txt
}

# Gets all types in an assembly, using ildasm
function GetTypesFromAssembly()
{
    param (
        [string]$InputFile
    )
    
    [array]$ClassStrings = & cmd /c call $env:VS170COMNTOOLS\VsDevCmd.bat `&`& ildasm $InputFile /all /pubonly /text | Select-String -Pattern "^\.class.*\s(\S*)" | Select-String -Pattern "^\.class extern\s" -NotMatch |% { $_.Matches[0].Groups[1].Value }
    if ($LastExitCode -ne 0)
    {
        throw "Failed to run ildasm.exe."
    }
    # Remove strings that start with "ABI.", we don't care about this namespace.
    $ClassStrings = $ClassStrings | Where-Object { $_ -notlike "ABI.*" }
    $ClassStrings
}

# Loop through all assemblies (DLLs and WinMDs) and generate an intellisense xml file for each one.
function GenerateXmlFilesForAllAssemblies()
{
    $GeneratedDir = "$WorkingDir\generated"
    Remove-Item $GeneratedDir -Recurse -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Path $GeneratedDir | Out-Null

    $CoverageTable = @()
    $MissingTypes = @()

    $TotalTypes = 0
    $TotalTypesWithIntellisense = 0

    $AssembliesToIgnore = @(
        "Microsoft.Foundation.winmd",
        "Microsoft.Graphics.winmd",
        "Microsoft.UI.winmd",
        "Microsoft.Windows.ApplicationModel.Resources.winmd",
        "Microsoft.Security.Authentication.OAuth.Projection.dll",
        "Microsoft.Windows.ApplicationModel.Background.Projection.dll",
        "Microsoft.Windows.ApplicationModel.DynamicDependency.Projection.dll",
        "Microsoft.Windows.ApplicationModel.Resources.Projection.dll",
        "Microsoft.Windows.ApplicationModel.WindowsAppRuntime.Projection.dll",
        "Microsoft.Windows.AppLifecycle.Projection.dll",
        "Microsoft.Windows.AppNotifications.Builder.Projection.dll",
        "Microsoft.Windows.AppNotifications.Projection.dll",
        "Microsoft.Windows.BadgeNotifications.Projection.dll",
        "Microsoft.Windows.Foundation.Projection.dll",
        "Microsoft.Windows.Management.Deployment.Projection.dll",
        "Microsoft.Windows.Media.Capture.Projection.dll",
        "Microsoft.Windows.PushNotifications.Projection.dll",
        "Microsoft.Windows.Security.AccessControl.Projection.dll",
        "Microsoft.Windows.Storage.Pickers.Projection.dll",
        "Microsoft.Windows.Storage.Projection.dll",
        "Microsoft.Windows.System.Power.Projection.dll",
        "Microsoft.Windows.System.Projection.dll",
        "Microsoft.WindowsAppRuntime.Bootstrap.Net.dll")

    foreach ($AssemblyFile in Get-ChildItem $AssemblyDirs)
    {
        if ($AssemblyFile.Extension -ne ".dll" -and $AssemblyFile.Extension -ne ".winmd")
        {
            continue
        }
        # Skip these non-WinUI assemblies.
        if ($AssembliesToIgnore -contains $AssemblyFile.Name)
        {
            continue
        }

        Write-Host "Reading $($AssemblyFile.FullName)"

        $assemblyName = [io.path]::GetFileNameWithoutExtension($AssemblyFile.FullName)

        [xml]$ProjectionXml = New-Object System.Xml.XmlDocument
        $ProjectionXml.AppendChild($ProjectionXml.CreateElement("doc")) | Out-Null
        $ProjectionXml.DocumentElement.AppendChild($ProjectionXml.CreateElement("assembly")) | Out-Null
        $ProjectionXml.DocumentElement.SelectSingleNode("assembly").AppendChild($ProjectionXml.CreateElement("name")) | Out-Null
        $ProjectionXml.DocumentElement.SelectSingleNode("assembly/name").InnerText = $assemblyName

        $MembersElement = $ProjectionXml.DocumentElement.AppendChild($ProjectionXml.CreateElement("members"))

        $Types = GetTypesFromAssembly $AssemblyFile.FullName
        $TypesWithIntellisense = 0

        foreach ($TypeName in $Types)
        {
            # Select the type node.
            $Nodes = $FullXml.DocumentElement.SelectNodes("members/member[@name='T:$typeName']")

            # Select all the members that belong to this type.
            #   M:Namespace.Type.MethodName
            #   P:Namespace.Type.PropertyName
            #   E:Namespace.Type.EventName
            $Nodes += $FullXml.DocumentElement.SelectNodes("members/member[contains(@name, ':$typeName.')]")
            foreach ($Node in $Nodes)
            {
                $MembersElement.AppendChild($ProjectionXml.ImportNode($Node, $true)) | Out-Null
            }

            if ($Nodes.Count -eq 0)
            {
                $MissingTypes += "No strings for $TypeName (in $($AssemblyFile.Name))"
            }
            else
            {
                $TypesWithIntellisense++ 
            }
        }

        $CoverageTable += [PSCustomObject]@{
            Assembly = $AssemblyFile.Name
            Coverage = [math]::Round(($TypesWithIntellisense / $Types.Count) * 100, 2)
            TypesWithIntellisense = $TypesWithIntellisense
            TotalTypes = $Types.Count
        }
        $TotalTypes += $Types.Count
        $TotalTypesWithIntellisense += $TypesWithIntellisense

        if ($typesWithIntellisense -gt 0)
        {
            $ProjectionXmlFileName = Join-Path $GeneratedDir ($AssemblyFile.Name -replace "\.[a-z]*$", ".xml")
            if (Test-Path $ProjectionXmlFileName)
            {
                throw "File already exists: $ProjectionXmlFileName"
            }
            Write-Host "Writing $ProjectionXmlFileName"
            $ProjectionXml.Save($ProjectionXmlFileName)
        }
    }

    Write-Host "Copying xml files from $GeneratedDir to $OutputDir"

    Remove-Item $OutputDir\*.xml -ErrorAction SilentlyContinue
    Copy-Item $GeneratedDir\*.xml $OutputDir\

    $CoverageFile = Join-Path $OutputDir "Coverage.txt"

    Write-Host "Saving $CoverageFile"

    "This file is auto-generated.  It describes how well our API surface is covered by Intellisense strings." > $CoverageFile
    "" >> $CoverageFile
    $CoverageTable | Sort-Object Assembly | Format-Table -AutoSize | Out-String >> $CoverageFile
    "Total types: $TotalTypes" >> $CoverageFile
    "Total types with intellisense: $TotalTypesWithIntellisense" >> $CoverageFile
    "Coverage: $([math]::Round(($TotalTypesWithIntellisense / $TotalTypes) * 100, 2))%" >> $CoverageFile
    "" >> $CoverageFile
    "Types without intellisense strings:" >> $CoverageFile
    $MissingTypes | Sort-Object | Out-String >> $CoverageFile
}

ReadIntellisenseFiles

CleanIntellisenseData

GenerateXmlFilesForAllAssemblies

$EndTime = Get-Date
$Duration = ($EndTime - $StartTime).TotalSeconds
Write-Host "Done generating Intellisense XML files. (Took $Duration seconds)" -ForegroundColor Green
