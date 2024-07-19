# Script to delete entries from themeresources files for apis not available in final release. 
# This will let us use existing files for pre-release builds and for final release we can use updated files. 
# Updated files are stored under RepoRoot\BuildOutput\Temp\ThemeResources

$themesDir = "$($env:RepoRoot)\controls\dev"
$outDir = "$($env:RepoRoot)\BuildOutput\Temp\ThemeResources"

$AutoSuggestBox = "AutoSuggestBox_themeresources.xaml"
$ComboBox = "ComboBox_themeresources.xaml"
$TextBox = "TextBox_themeresources.xaml"

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
    DeleteLineFromFile "ComboBox" $ComboBox "ErrorTemplate"
    DeleteLineFromFile "CommonStyles" $TextBox "ErrorTemplate"

    Write-Output("Output themeresources files are stored at : " + $outDir)
}

Run-Main 