# Script to extract all resources related to controls and generate xaml files per control per rs release.
#   . Estimate all the keys and styles would be impacted
#   . generate files like TimePicker_rs1_themeresources.xaml, TimePicker_rs2_themeresources.xaml, TimePicker_rs3_themeresources.xaml...
#   . remove the duplicated Items. If TimePicker style in rs2 is the same with rs1, rs2 will not include this style
#   . generate new generic.xaml and the node to be removed would be marked with NodeToBeReplaced
param (
    [Parameter(Mandatory=$true)][string]$controlLists
)
$skippedkeyLists = @()

$themesDir = "D:\os\src\onecoreuap\windows\dxaml\xcp\dxaml\themes"
$targetDir = "D:\temp"

# false mean to remove impacted nodes directly when generate generic.xaml. true mean to comment the impacted nodes and it provide a chance to review it
$CommentMatchedNodeInGenericXAML = $false

$rsPath = Join-Path $themesDir "generic.xaml"
$rs5Path = Join-Path $themesDir "generic.xaml"
$rs4Path = Join-Path $themesDir "generic.rs4.xaml"
$rs3Path = Join-Path $themesDir "generic.rs3.xaml"
$rs2Path = Join-Path $themesDir "generic.rs2.xaml"
$rs1Path = Join-Path $themesDir "generic.rs1.xaml"

$generatedThemeResourcesBeginMessage = "Begin Windows.UI.Xaml.Controls.dll resources - DO NOT MANUALLY EDIT BELOW THIS LINE!"
$generatedThemeResourcesEndMessage = "End Windows.UI.Xaml.Controls.dll resources - DO NOT MANUALLY EDIT ABOVE THIS LINE!"

$generatedThemeResourcesRegexString = "(?m)($generatedThemeResourcesBeginMessage)[\s\S]+?($generatedThemeResourcesEndMessage)"

# xmlDocument.Load/Save converted &#xE0E5 to uncode character and it's hard to recover it to the orignal string
# replace the "escape characters" in the form &#xE0E5 in loading and convert them back in saving
function Get-XmlDocument
{
    Param($file)
    $tempFile = New-TemporaryFile
    (Get-Content $file -Raw).Replace("&", "&amp;") |Set-Content $tempFile
    $xmlDocument =  New-Object System.Xml.XmlDocument
    $xmlDocument.PreserveWhitespace = $true
    $xmlDocument.Load($tempFile)
    Remove-Item $tempFile
    return $xmlDocument
}

function Save-XmlDocument
{
    Param($file, $xmlDocument)
    $tempFile = New-TemporaryFile
    $xmlDocument.Save($tempFile)
    $content = (Get-Content $tempFile -Raw).Replace("&amp;","&") 
    #Write-Output($tempFile.FullName)
    Remove-Item $tempFile
    # just keep one space line
    $content -replace "(?m)(^\s*\r\n)+","`r`n" |Set-Content $file
}

function Get-IsInSkipList
{
    Param($node)

    if (!$skippedkeyLists -or $skippedkeyLists.Count -eq 0)    
    {
        return $false
    }
    
    $shouldSkip = $false
    if ($($node.Key)) # x:Key
    {
        $shouldSkip = $shouldSkip -or $skippedkeyLists.Contains($node.Key)
    } 
    if ($($node.Name)) # x:Name=
    {
        $shouldSkip = $shouldSkip -or $skippedkeyLists.Contains($node.Name)
    }
    if ($($node.TargetType)) # target exists
    {
        $shouldSkip = $shouldSkip -or $skippedkeyLists.Contains($node.TargetType)
    } 
    if ($($node.LocalName))
    {
         $shouldSkip = $shouldSkip -or $skippedkeyLists.Contains($node.LocalName)
    }
    return $shouldSkip
}

function Get-IsRelatedToControl
{
    Param($node, $controlLists)
    
    if (Get-IsInSkipList $node)
    {
        return $false
    }
    
    if ($($node.TargetType)) # target exists
    {
        if ($controlLists.Contains($node.TargetType) -or  $($controlLists |Where-Object { $node.TargetType.StartsWith( $_ ) }))
        {
            return $true
        }
    }
    
    if ($($node.Key)) # x:Key
    {
        $key = $node.Key
    } elseif ($($node.Name)) # x:Name=
    {
        $key = $node.Name
    }
    if ($key)
    {
        return $($controlLists |Where-Object { $key.StartsWith( $_ ) })
    }
    return $false
}

enum KeyPrefix
{
    Default
    HighContrast
    Light
    ResourceDictionary
}

function To-String
{
    Param($node, [KeyPrefix] $keyPrefix)

    $localName = $keyPrefix.ToString() + ":"
    if ($node.LocalName)
    {
        $localName += $node.LocalName + ":"
    } 
    else
    {
        $localName += ""
    }

    if ($($node.Key)) # x:Key
    {
        return $localName + $node.Key
    }     
    elseif ($($node.TargetType))
    {
        return $localName + $node.TargetType
    }
    elseif ($($node.Name)) # x:Name=
    {
        return $localName + $node.Name
    } 

    return "Unknown"
}


function Evaluate-ImpactedKeys
{
    param($dict, $controlLists)
    foreach ($node in $dict.'ResourceDictionary.ThemeDictionaries'.ChildNodes[0].ChildNodes)
    {
        if (Get-IsRelatedToControl $node $controlLists)
        {
            Write-Output("Default has Key " + (To-String $node ([KeyPrefix]::Default)))
        }
    }

    foreach ($node in $dict.'ResourceDictionary.ThemeDictionaries'.ChildNodes[1].ChildNodes)
    {
        if (Get-IsRelatedToControl $node $controlLists)
        {
            Write-Output("HighContrast has key " + (To-String $node ([KeyPrefix]::HighContrast)))
        }
    }

    foreach ($node in $dict.'ResourceDictionary.ThemeDictionaries'.ChildNodes[2].ChildNodes)
    {
        if (Get-IsRelatedToControl $node $controlLists)
        {
            Write-Output("Light has key " + (To-String $node ([KeyPrefix]::Light)))
        }
    }

    foreach ($node in $dict.ChildNodes)
    {
        if (Get-IsRelatedToControl $node $controlLists)
        {
            Write-Output("Root has key " + (To-String $node ([KeyPrefix]::ResourceDictionary)))
        }
    }
}

function Build-ImpactedMap
{
    param($dict, $controlLists)
    $map = @{}

    foreach ($node in $dict.'ResourceDictionary.ThemeDictionaries'.ChildNodes[0].ChildNodes)
    {
        if (Get-IsRelatedToControl $node $controlLists)
        {
            $map.Add((To-String $node ([KeyPrefix]::Default)), $node.OuterXml)
        }
    }

    foreach ($node in $dict.'ResourceDictionary.ThemeDictionaries'.ChildNodes[1].ChildNodes)
    {
        if (Get-IsRelatedToControl $node $controlLists)
        {
            $map.Add((To-String $node ([KeyPrefix]::HighContrast)), $node.OuterXml)
        }
    }

    foreach ($node in $dict.'ResourceDictionary.ThemeDictionaries'.ChildNodes[2].ChildNodes)
    {
        if (Get-IsRelatedToControl $node $controlLists)
        {
           $map.Add( (To-String $node ([KeyPrefix]::Light)), $node.OuterXml)
        }
    }

    foreach ($node in $dict.ChildNodes)
    {
        if (Get-IsRelatedToControl $node $controlLists)
        {
            $map.Add((To-String $node ([KeyPrefix]::ResourceDictionary)), $node.OuterXml)
        }
    }
    return $map
}

# The DO NOT MANUALLY EDIT part is used by depcontrol porting script and should be ignored.
# the curMap keeps all the keys, it we can't find the key, then it should be in DepControlAutoPortingPart
function Get-IsDepControlAutoPortingPart
{
    Param($node, $keyPrefix, $curMap)
    $key = To-String $node $keyPrefix
    return !$curMap.ContainsKey($key)
}

function Get-IsSameNodeWithOldRelease
{
    Param($node, $keyPrefix, $preMap, $curMap)
    
    if (!$preMap)
    {
        return $false
    }

    $key = To-String $node $keyPrefix
    $oldValue = $preMap[$key]
    $curValue = $curMap[$key]
    if (!$curValue)
    {
        Write-Error("Can't find key in cur map: " + $key)
        Write-Debug($curMap)
    }

    if ($oldValue -and $oldValue.Equals($curValue))
    {
        return $true
    }
    return $false
}

function Replace-ChildNodes
{
    Param($parentNode, [KeyPrefix] $keyPrefix, $controlLists, $xmlDocument, $curMap)
    

    if (!$($parentNode.ChildNodes))
    {
        return
    }

    $nodeToBeReplaced = New-Object System.Collections.ArrayList

    foreach ($node in $parentNode.ChildNodes)
    {
        if (Is-ReservedNode $node)
        {
            continue #skip 'ResourceDictionary.ThemeDictionaries'
        }

        if (Get-IsRelatedToControl $node $controlLists)
        {
            if (Get-IsDepControlAutoPortingPart $node $keyPrefix $curMap)
            {
                continue # ignore ManuallyPart
            }
            $nodeToBeReplaced.Add($node) | Out-Null
        }        
    }
    foreach ($node in $nodeToBeReplaced)
    {
        if ($CommentMatchedNodeInGenericXAML)
        {
            # Comment the matched Element
            $node.SetAttribute("NodeToBeReplaced","True") | Out-Null
        }
        else
        {
            $parentNode.RemoveChild($node) | Out-Null
        }
    }
}

# In order to keep the old format of generic*.xaml, we didn't create a new file and copy all concerned nodes to the new file
# But we copy generic*.xaml to a new file, then remove items which should not belongs to this file
function Clean-ChildNodes
{
    Param($parentNode, [KeyPrefix] $keyPrefix, $controlLists, $preMap, $curMap)

    if (!$($parentNode.ChildNodes))
    {
        return
    }

    $nodeToBeRemoved = New-Object System.Collections.ArrayList

    foreach ($node in $parentNode.ChildNodes)
    {
        if (Is-ReservedNode $node)
        {
            continue #skip 'ResourceDictionary.ThemeDictionaries'
        }

        if (Get-IsRelatedToControl $node $controlLists)
        {
            if (Get-IsDepControlAutoPortingPart $node $keyPrefix $curMap)
            {
                # remove it because it's in ManuallyPart
                $nodeToBeRemoved.Add($node) | Out-Null
            } 
            elseif (Get-IsSameNodeWithOldRelease $node $keyPrefix $preMap $curMap)
            {
                Write-Output("Remove duplicate " + (To-String $node $keyPrefix))
                $nodeToBeRemoved.Add($node) | Out-Null
            }
        }
        else
        {
            $nodeToBeRemoved.Add($node) | Out-Null
        }
    }
    foreach ($node in $nodeToBeRemoved)
    {
        $parentNode.RemoveChild($node) | Out-Null
    }
}

function Is-ReservedNode
{
    Param($node)
    
    $reservedKey = @("ResourceDictionary.ThemeDictionaries", "#whitespace", "ResourceDictionary")
    return $($node.LocalName) -and $reservedKey.Contains($node.LocalName)
}


function Get-ThemeSubDict
{
    Param($themesDir, [KeyPrefix] $keyPrefix)
    $key = $keyPrefix.ToString()
    foreach ($node in $themesDir.ChildNodes)
    {
        if ($node.Key)
        {
            if ($key.Equals($node.Key))
            {
                return $node
            }
        }
    }
    Write-Error("This should not happen")
    Write-Debug($themeDict.ChildNodes)
    return $null
}

function Handle-Control
{
    Param($xmlPath, $controlName, $preMap, $curMap, $postfix)

    $xmlDocument = Get-XmlDocument $xmlPath

    $controlLists = New-Object System.Collections.ArrayList
    $controlLists.Add($controlName) | Out-Null

    $themeDict = $xmlDocument.ResourceDictionary.'ResourceDictionary.ThemeDictionaries'

    # Remove Items in ThemeDictionaries
    $subDict0 = Get-ThemeSubDict $themeDict ([KeyPrefix]::Default)
    Clean-ChildNodes $subDict0 ([KeyPrefix]::Default) $controlLists $preMap $curMap
    $subDict1 = Get-ThemeSubDict $themeDict ([KeyPrefix]::HighContrast)
    Clean-ChildNodes $subDict1 ([KeyPrefix]::HighContrast) $controlLists $preMap $curMap
    $subDict2 = Get-ThemeSubDict $themeDict ([KeyPrefix]::Light)
    Clean-ChildNodes $subDict2 ([KeyPrefix]::Light) $controlLists $preMap $curMap

    # Remove Items in ResourceDictionary
    
    Clean-ChildNodes $xmlDocument.ResourceDictionary ([KeyPrefix]::ResourceDictionary) $controlLists $preMap $curMap
    
    $file = Join-Path $targetDir ($controlName + "_" + $postfix + "_themeresources.xaml")
    Write-Output("Generate File " + $file)
    Save-XmlDocument $file $xmlDocument
}

function Produce-GenericXAML
{
    Param($xmlPath, $controlLists, $curMap)
                                                                                                                         
    $xmlDocument = Get-XmlDocument $xmlPath

    $themeDict = $xmlDocument.ResourceDictionary.'ResourceDictionary.ThemeDictionaries'

    # Replace Items in ThemeDictionaries
    $subDict0 = Get-ThemeSubDict $themeDict ([KeyPrefix]::Default)
    Replace-ChildNodes $subDict0 ([KeyPrefix]::Default) $controlLists $xmlDocument $curMap
    $subDict1 = Get-ThemeSubDict $themeDict ([KeyPrefix]::HighContrast)
    Replace-ChildNodes $subDict1 ([KeyPrefix]::HighContrast) $controlLists $xmlDocument $curMap
    $subDict2 = Get-ThemeSubDict $themeDict ([KeyPrefix]::Light)
    Replace-ChildNodes $subDict2 ([KeyPrefix]::Light) $controlLists $xmlDocument $curMap

    # Replace Items in ResourceDictionary   
    Replace-ChildNodes $xmlDocument.ResourceDictionary ([KeyPrefix]::ResourceDictionary) $controlLists $xmlDocument $curMap
    
    $file = Join-Path $targetDir  "generic.xaml"
    Write-Output("Generate File " + $file)
    Save-XmlDocument $file $xmlDocument
}

function Run-Main
{
    param($rs1Path, $rs2Path, $rs3Path, $rs4Path, $rs5Path, $rsPath, $controlLists)
    
    Start-Transcript -path (Join-Path $targetDir "main.log") -Append
    New-Item -Path $targetDir -Type Directory
    Write-Output("themesDir: " + $themesDir)
    Write-Output("targetDir: " + $targetDir)
    Write-Output("Generate files for Controls: " + $controlLists)
    Write-Output("Skipped Keys " + $skippedkeyLists)
    
    [string]$rsThemeResouces = Get-Content $rsPath -Raw
    [string]$rsOriginalThemeResources = $rsThemeResouces -replace $generatedThemeResourcesRegexString,""

    [string]$rs5ThemeResouces = Get-Content $rs5Path -Raw
    [string]$rs5OriginalThemeResources = $rs5ThemeResouces -replace $generatedThemeResourcesRegexString,""

    [string]$rs4ThemeResouces = Get-Content $rs4Path -Raw
    [string]$rs4OriginalThemeResources = $rs4ThemeResouces -replace $generatedThemeResourcesRegexString,""

    [string]$rs3ThemeResouces = Get-Content $rs3Path -Raw
    [string]$rs3OriginalThemeResources = $rs3ThemeResouces -replace $generatedThemeResourcesRegexString,""

    [string]$rs2ThemeResouces = Get-Content $rs2Path -Raw
    [string]$rs2OriginalThemeResources = $rs2ThemeResouces -replace $generatedThemeResourcesRegexString,""

    [string]$rs1ThemeResouces = Get-Content $rs1Path -Raw
    [string]$rs1OriginalThemeResources = $rs1ThemeResouces -replace $generatedThemeResourcesRegexString,""

    Write-Output("Load generic.rs*.xaml and build quick lookup map")
    $rsResourceDictionary = ([xml] $rsOriginalThemeResources).ResourceDictionary
    $rs5ResourceDictionary = ([xml] $rs5OriginalThemeResources).ResourceDictionary
    $rs4ResourceDictionary = ([xml] $rs4OriginalThemeResources).ResourceDictionary
    $rs3ResourceDictionary = ([xml] $rs3OriginalThemeResources).ResourceDictionary
    $rs2ResourceDictionary = ([xml] $rs2OriginalThemeResources).ResourceDictionary
    $rs1ResourceDictionary = ([xml] $rs1OriginalThemeResources).ResourceDictionary
    
    $rsMap = Build-ImpactedMap $rsResourceDictionary $controlLists
    $rs5Map = Build-ImpactedMap $rs5ResourceDictionary $controlLists
    $rs4Map = Build-ImpactedMap $rs4ResourceDictionary $controlLists
    $rs3Map = Build-ImpactedMap $rs3ResourceDictionary $controlLists
    $rs2Map = Build-ImpactedMap $rs2ResourceDictionary $controlLists
    $rs1Map = Build-ImpactedMap $rs1ResourceDictionary $controlLists
    
    Write-Output("Evaluate the Changes related to Controls")
    Evaluate-ImpactedKeys $rsResourceDictionary $controlLists
    
    $oldGenericXamlFile = Join-Path  $targetDir "generic.orig.xaml"
    Write-Output("Backup $rsPath to $oldGenericXamlFile")
    Copy-Item -Path $rsPath -Destination $oldGenericXamlFile

    Write-Output("Generate new generic.xaml template")
    Produce-GenericXAML $rsPath $controlLists $rsMap

    foreach ($controlName in $controlLists)
    {
        Write-Output("Processing " + $controlName)
        Handle-Control $rs1Path $controlName $rsMapNotExist $rs1Map "rs1"
        Handle-Control $rs2Path $controlName $rs1Map $rs2Map "rs2"
        Handle-Control $rs3Path $controlName $rs2Map $rs3Map "rs3"
        Handle-Control $rs4Path $controlName $rs3Map $rs4Map "rs4"
        Handle-Control $rs5Path $controlName $rs4Map $rs5Map "rs5"
        Handle-Control $rsPath $controlName $rs5Map $rsMap "rs6"
    }
    Stop-Transcript
}

Run-Main $rs1Path $rs2Path $rs3Path $rs4Path $rs5Path $rsPath $controlLists