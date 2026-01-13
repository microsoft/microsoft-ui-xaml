param(
    [string]$downloadurlx86,
    [string]$downloadurlx64,
    [string]$downloadurlarm64
)

$ProgressPreference = "SilentlyContinue"

$arch = $env:_BuildArch;

if ($arch -eq "ARM64EC")
{
    $arch = "amd64"
}

$reporoot = Join-Path $PSScriptRoot ".." -resolve
$outputDir = Join-Path $reporoot (Join-Path ".tools" $arch)
$outputPath = Join-Path $outputDir "dotnet-windowsdesktop-runtime-installer.exe"

if(!(Test-Path $outputDir))
{
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

if (([string]::IsNullOrEmpty($downloadurlx86)) -or ([string]::IsNullOrEmpty($downloadurlx64)) -or ([string]::IsNullOrEmpty($downloadurlarm64)))
{
    # Find .NET runtime direct download links at https://dotnet.microsoft.com/en-us/download/dotnet
    switch($env:_DotNetMoniker)
    {
        "net8.0"
        {
            Write-Host "Downloading .NET 8 runtimes"
            $downloadurlx86 = "https://download.visualstudio.microsoft.com/download/pr/c629f243-5125-4751-a5ff-e78fa45646b1/85777e3e3f58f863d884fd4b8a1453f2/windowsdesktop-runtime-8.0.3-win-x86.exe"
            $downloadurlx64 = "https://download.visualstudio.microsoft.com/download/pr/51bc18ac-0594-412d-bd63-18ece4c91ac4/90b47b97c3bfe40a833791b166697e67/windowsdesktop-runtime-8.0.3-win-x64.exe"
            $downloadurlarm64 = "https://download.visualstudio.microsoft.com/download/pr/bd4bf739-106e-44af-9f0d-a6a777976512/e9f077b8cb33b574df2f5cf986acddd8/windowsdesktop-runtime-8.0.3-win-arm64.exe"
        }
        "net7.0"
        {
            Write-Host "Downloading .NET 7 runtimes"
            $downloadurlx86 = "https://download.visualstudio.microsoft.com/download/pr/8a184836-3d12-41c7-b509-7d0d8d63dbf8/5d3bb50e730873808363dea2e9b8a2fd/windowsdesktop-runtime-7.0.7-win-x86.exe"
            $downloadurlx64 = "https://download.visualstudio.microsoft.com/download/pr/342ba160-3776-4ffa-91dd-e3cd9dc0f817/ba649d6b80b27ca164d80bd488cdb51f/windowsdesktop-runtime-7.0.7-win-x64.exe"
            $downloadurlarm64 = "https://download.visualstudio.microsoft.com/download/pr/00f57df4-c604-46e1-8d32-f87a77f1d233/cd28e125d6f777c6d567a344da1fff2d/windowsdesktop-runtime-7.0.7-win-arm64.exe"
        }
        "net6.0"
        {
            Write-Host "Downloading .NET 6 runtimes..."
            $downloadurlx86 = "https://download.visualstudio.microsoft.com/download/pr/68574b0b-3242-46f1-a406-9ef9aeeec3e5/d45d732e846f306889f41579104b1a33/windowsdesktop-runtime-6.0.18-win-x86.exe"
            $downloadurlx64 = "https://download.visualstudio.microsoft.com/download/pr/f76bace5-6cf4-41d8-ab54-fb7a3766b673/1cbc047d4547dfa9ecd59d5a71402186/windowsdesktop-runtime-6.0.18-win-x64.exe"
            $downloadurlarm64 = "https://download.visualstudio.microsoft.com/download/pr/efc902e6-6c71-42d2-b9d7-ad7c1d104d52/2f88aed465962b5f495c98536d6371c5/windowsdesktop-runtime-6.0.18-win-arm64.exe"
        }
        default
        {
            Write-Host "Downloading .NET 5 runtimes..."
            $downloadurlx86 = "https://download.visualstudio.microsoft.com/download/pr/b6fe5f2a-95f4-46f1-9824-f5994f10bc69/db5ec9b47ec877b5276f83a185fdb6a0/windowsdesktop-runtime-5.0.17-win-x86.exe"
            $downloadurlx64 = "https://download.visualstudio.microsoft.com/download/pr/3aa4e942-42cd-4bf5-afe7-fc23bd9c69c5/64da54c8864e473c19a7d3de15790418/windowsdesktop-runtime-5.0.17-win-x64.exe"
            $downloadurlarm64 = "https://download.visualstudio.microsoft.com/download/pr/be25784a-4231-4c53-ba6e-869166ef523f/9602c6c0d358d31dc710fd0573fc39e0/windowsdesktop-runtime-5.0.17-win-arm64.exe"
        }
    }
}

if($arch -eq "x86")
{
    $downloadurl = $downloadurlx86
}
elseif($arch -eq "amd64")
{
    $downloadurl = $downloadurlx64
} 
elseif($arch -eq "arm64" -or $arch -eq "ARM64")
{
    $downloadurl = $downloadurlarm64
}
else
{
    Write-Host "dotnet Desktop runtime not available for arch $arch"
    return
}


if(!(Test-Path $outputPath))
{
    Write-Host "Downloading $downloadurl to $outputPath"
    Invoke-WebRequest $downloadurl -OutFile $outputPath
}
