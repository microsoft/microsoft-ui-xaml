# Remap shared MUXC xmlns values to Tabular namespaces for generated Tabular XAML.
param([Parameter(Mandatory = $true)][string]$Dir)

$files = @()
$files += Get-ChildItem -Path $Dir -Filter 'generic*.xaml' -File -ErrorAction SilentlyContinue
$files += Get-ChildItem -Path $Dir -Filter 'themeresources*.xaml' -File -ErrorAction SilentlyContinue

foreach ($f in $files) {
    $content = Get-Content -LiteralPath $f.FullName -Raw
    # Skip empty files so Replace() does not throw.
    if ([string]::IsNullOrEmpty($content)) { continue }
    $remapped = $content.Replace('using:Microsoft.UI.Xaml.Controls.Primitives"', 'using:Microsoft.UI.Xaml.Controls.Tabular.Primitives"').Replace('using:Microsoft.UI.Xaml.Controls"', 'using:Microsoft.UI.Xaml.Controls.Tabular"')
    if ($remapped -ne $content) {
        Set-Content -LiteralPath $f.FullName -Value $remapped -NoNewline
        Write-Host "RemapTabularXamlNamespaces: remapped Tabular xmlns in $($f.Name)"
    }
}
