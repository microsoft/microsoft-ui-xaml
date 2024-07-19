$devDir = [System.IO.Path]::GetFullPath([System.IO.Path]::Combine($PSScriptRoot, "..", "dev"))

$v1XamlFiles = Get-ChildItem $devDir -Recurse -Filter *_v1.xaml
$vcxItemsFiles = Get-ChildItem $devDir -Recurse -Filter *.vcxitems

foreach ($v1XamlFile in $v1XamlFiles)
{
    Remove-Item $v1XamlFile.FullName
}

foreach ($vcxItemsFile in $vcxItemsFiles)
{
    $document = [System.Xml.XmlDocument]::new()
    $document.Load($vcxItemsFile.FullName)

    $changesMade = $false

    $page = $document.Project.ItemGroup.Where({$_.Page}).Page

    if ($page)
    {
        if ($page.GetType() -eq [System.Xml.XmlElement])
        {
            $page.ParentNode.RemoveChild($page)
        }
        else
        {
            foreach ($v1XamlFileInclude in $document.Project.ItemGroup.Where({$_.Page}).Page.Where({$_.Include -ilike "*_v1.xaml"}))
            {
                $v1XamlFileInclude.ParentNode.RemoveChild($v1XamlFileInclude)
                $changesMade = $true
            }
        }
    }

    if ($changesMade)
    {
        $document.Save($vcxItemsFile.FullName)
    }
}