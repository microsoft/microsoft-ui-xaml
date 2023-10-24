[CmdLetBinding()]
Param(
    [Parameter(mandatory=$true)]
    [string]$InputWinUIGalleryJsonFilePath = "$($env:RepoRoot)\Samples\WinUIGallery\WinUIGallery\DataModel\ControlInfoData.json",

    [Parameter(mandatory=$true)]
    [string]$OutputXmlFilePath
)

# This script generates WinUIGalleryTestData.xml from WinUIGallery's ControlInfoData.json
# The xml file is consumed by WinUIGallery test automation to create a test to navigate to each page in the app.

$xcgControlInfo = Get-Content $InputWinUIGalleryJsonFilePath | ConvertFrom-Json

[xml]$doc = New-Object System.Xml.XmlDocument
$root = $doc.CreateElement("Data")
$doc.AppendChild($root) | Out-Null

foreach($group in $xcgControlInfo.Groups)
{
    if($group.Items.Count -gt 0)
    {
        $table = $doc.CreateElement("Table")
        $table.SetAttribute("Id", $group.UniqueId)
        $root.AppendChild($table) | Out-Null

        foreach($item in $group.Items)
        {
            $row = $doc.CreateElement("Row")
            $row.SetAttribute("Name", $item.UniqueId)

            $param = $doc.CreateElement("Parameter")
            $param.SetAttribute("Name", "SectionName")
            $param.InnerText = $group.Title
            $row.AppendChild($param) | Out-Null

            $param = $doc.CreateElement("Parameter")
            $param.SetAttribute("Name", "PageName")
            $param.InnerText = $item.Title
            $row.AppendChild($param) | Out-Null

            $param = $doc.CreateElement("Parameter")
            $param.SetAttribute("Name", "TextOnPage")
            $param.InnerText = $item.Description
            $row.AppendChild($param) | Out-Null

            $table.AppendChild($row) | Out-Null
        }
    }
}

$doc.Normalize()
$doc.Save($OutputXmlFilePath)