# Prints the top-level layout of the current folder and the most common file types.
param([string]$Path = '.')
Get-ChildItem $Path -Force | Where-Object Name -notin '.git', 'bin', 'obj' | Select-Object Mode, Name | Format-Table -AutoSize
Get-ChildItem $Path -Recurse -File -ErrorAction SilentlyContinue |
    Where-Object FullName -notmatch '\\(\.git|bin|obj|node_modules)\\' |
    Group-Object Extension | Sort-Object Count -Descending | Select-Object -First 10 Count, Name | Format-Table -AutoSize
