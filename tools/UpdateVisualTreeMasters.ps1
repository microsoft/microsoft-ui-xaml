Param(
    [parameter(Mandatory=$true)][string]$HtmlFilePath
)
$html = New-Object -ComObject "HTMLFile"
$rawContent = Get-Content -Path $HtmlFilePath -Raw
$content = [System.Text.Encoding]::Unicode.GetBytes($rawContent)
$html.write($content)

$webClient = New-Object System.Net.WebClient
$visualTreeDumpDivs = $html.body.getElementsByClassName("VisualTreeMasters")
$newFiles = @()
foreach($div in $visualTreeDumpDivs)
{
    $links = $div.getElementsByTagName("a")
    foreach ($link in $links)
    {
        $url = $link.href
        $fileName = [System.IO.Path]::GetFileName($url)
        $destination = "$PSScriptRoot\..\test\MUXControlsTestApp\master\$fileName"
        Write-Host "Copying $fileName"
        $webClient.DownloadFile($url, $destination)
        $newFiles += Get-Item $destination
    }
}

Write-Host "Merging duplicates..."

$prefixList = @()
foreach($file in $newFiles)
{
    $prefix = $file.BaseName.Split('-')[0]
    if($prefixList -NotContains $prefix)
    {
        $prefixList += $prefix
    }
}

foreach($prefix in $prefixList)
{
    $filesToDelete = @()
    $versionedMasters = $newFiles | Where { $_.BaseName.StartsWith($prefix) } | Sort-Object -Property Name -Descending
    for ($i=0; $i -lt $versionedMasters.Length-1; $i++)
    {
        $v1 = Get-Content $versionedMasters[$i].FullName
        $v2 = Get-Content $versionedMasters[$i+1].FullName
        $diff = Compare-Object $v1 $v2
        if($diff.Length -eq 0)
        {
            $filesToDelete += $versionedMasters[$i]
        }
    }
    $filesToDelete | ForEach-Object {
        Write-Host "Deleting $($_.Name)"
        Remove-Item $_.FullName
    }

    Write-Host "Renaming $($versionedMasters[-1].Name) to $prefix.xml"
    Move-Item $versionedMasters[-1].FullName "$PSScriptRoot\..\test\MUXControlsTestApp\master\$prefix.xml" -Force
}
