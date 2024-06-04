if ($env:RepoRoot)
{
    $repoRoot = $env:RepoRoot
}
else
{
    $repoRoot = [System.IO.Path]::GetFullPath("$PSScriptRoot\..\..")
}

Get-ChildItem "$repoRoot\controls\dev" -Filter "*.vcxitems" -Recurse | ForEach-Object {
    $xmlDocument = [System.Xml.XmlDocument]::new()
    $xmlDocument.Load($_.FullName)

    [System.Collections.Generic.List[System.Xml.XmlNode]]$nodesToRemove = @()

    $documentUpdated = $false

    foreach ($pageNode in $xmlDocument.Project.ItemGroup.Where({$_.Page}).Page)
    {
        [System.Xml.XmlNode]$pageNode = $pageNode

        # We'll always want to remove the OS version metadata, since that's not valid in WinUI 3.
        if ($pageNode.Version)
        {
            [System.Xml.XmlNode]$versionNode = $pageNode.ChildNodes.Where({$_.'#text' -eq $pageNode.Version})[0]
            $pageNode.RemoveChild($versionNode)

            $documentUpdated = $true
        }

        # Ditto for the ControlsResourcesVersion metadata.
        if ($pageNode.ControlsResourcesVersion)
        {
            [System.Xml.XmlNode]$controlsResourcesVersionNode = $pageNode.ChildNodes.Where({$_.'#text' -eq $pageNode.ControlsResourcesVersion})[0]
            $pageNode.RemoveChild($controlsResourcesVersionNode)

            $documentUpdated = $true
        }

        [string]$pagePath = $pageNode.SelectSingleNode("@Include").'#text'
        $pagePathWithoutOsVersion = $pagePath.Replace("_rs1", "").Replace("_rs2", "").Replace("_rs3", "").Replace("_rs4", "").Replace("_rs5", "").Replace("_19h1", "").Replace("_21h1", "")
        
        # If this is an page without an OS version, then we'll keep it as-is.
        if ($pagePath -eq $pagePathWithoutOsVersion)
        {
            continue
        }
        # If this is not a page without an OS version, and there is an existing page without an OS version, then we'll remove this node entirely.
        elseif ($xmlDocument.Project.ItemGroup.Where({$_.Page}).Page.Where({$_.SelectSingleNode("@Include").'#text' -eq $pagePathWithoutOsVersion}))
        {
            $nodesToRemove.Add($pageNode)
        }
        # Otherwise, if there's no page without an OS version, we'll update this to be one without an OS version.
        else
        {
            $pageNode.SelectSingleNode("@Include").'#text' = $pagePathWithoutOsVersion
        }

        $documentUpdated = $true
    }

    foreach ($node in $nodesToRemove)
    {
        [System.Xml.XmlNode]$node = $node
        $node.ParentNode.RemoveChild($node)

        $documentUpdated = $true
    }
    
    if ($documentUpdated)
    {
        $xmlDocument.Save($_.FullName)
    }
}