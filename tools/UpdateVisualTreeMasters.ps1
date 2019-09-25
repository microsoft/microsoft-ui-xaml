Param(
    [parameter(Mandatory=$true)][string]$HtmlFilePath
)
$html = New-Object -ComObject "HTMLFile"
$rawContent = Get-Content -Path $HtmlFilePath -Raw
$content = [System.Text.Encoding]::Unicode.GetBytes($rawContent)
$html.write($content)

$webClient = New-Object System.Net.WebClient
$visualTreeDumpDivs = $html.body.getElementsByClassName("VisualTreeMasters")
foreach($div in $visualTreeDumpDivs)
{
    $links = $div.getElementsByTagName("a")
    foreach ($link in $links)
    {
        $url = $link.href
        $fileName = [System.IO.Path]::GetFileName($url)
        $destination = "$PSScriptRoot\..\test\MUXControlsTestApp\master\$Filename"
        $webClient.DownloadFile($url, $destination)
    }
}
