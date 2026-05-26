# Script to delete entries from themeresources files for apis not available in final release. 
# This will let us use existing files for pre-release builds and for final release we can use updated files. 
# Updated files are stored under RepoRoot\BuildOutput\Temp\ThemeResources

$themesDir = "$($env:RepoRoot)\controls\dev"
$outDir = "$($env:RepoRoot)\BuildOutput\Temp\ThemeResources"

$AutoSuggestBox = "AutoSuggestBox_themeresources.xaml"
$AutoSuggestBox_perf2026 = "AutoSuggestBox_themeresources_perf2026.xaml"
$ComboBox = "ComboBox_themeresources.xaml"
$ComboBox_perf2026 = "ComboBox_themeresources_perf2026.xaml"
$TextBox = "TextBox_themeresources.xaml"
$TextBox_perf2026 = "TextBox_themeresources_perf2026.xaml"

Function DeleteLineFromFile($PathIn, $File, $ExperimentalApis)
{
    Get-Content $themesDir\$PathIn\$File | 
    Where-Object { $_ -notmatch $ExperimentalApis } | 
    Set-Content $outDir\$File
}

function Run-Main
{
    Write-Output("Input themesDir: " + $themesDir)

    # Clean-up if already exists and create clean folder
    if (Test-Path -Path $outDir)
    {
        rm $outDir -r -fo
    }
    md $outDir

    DeleteLineFromFile "AutoSuggestBox" $AutoSuggestBox "InputValidation|ErrorTemplate|ValidationContext"
    DeleteLineFromFile "AutoSuggestBox" $AutoSuggestBox_perf2026 "InputValidation|ErrorTemplate|ValidationContext"
    DeleteLineFromFile "ComboBox" $ComboBox "ErrorTemplate"
    DeleteLineFromFile "ComboBox" $ComboBox_perf2026 "ErrorTemplate"
    DeleteLineFromFile "CommonStyles" $TextBox "ErrorTemplate"
    DeleteLineFromFile "CommonStyles" $TextBox_perf2026 "ErrorTemplate"

    Write-Output("Output themeresources files are stored at : " + $outDir)
}

Run-Main 
