[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$IdlPath
)

$ErrorActionPreference = "Stop"

$compositionPath = Join-Path $IdlPath "Windows.UI.Composition.idl"
$uiDependencyPath = Join-Path $IdlPath "Windows.UI.idl"
$renamedUiDependencyPath = Join-Path $IdlPath "Windows.UI.Composition.Dependencies.idl"
$systemDependencyPath = Join-Path $IdlPath "Windows.System.idl"

if (-not (Test-Path $compositionPath -PathType Leaf))
{
    throw "winmdidl did not produce the Composition IDL in '$IdlPath'."
}

if (Test-Path $uiDependencyPath -PathType Leaf)
{
    Move-Item $uiDependencyPath $renamedUiDependencyPath -Force
}
elseif (-not (Test-Path $renamedUiDependencyPath -PathType Leaf))
{
    throw "winmdidl did not produce the Windows.UI dependency IDL in '$IdlPath'."
}

$windowContextPartnerIdl = @'
import "inspectable.idl";
import "windowscontracts.idl";
import "Windows.Foundation.idl";

namespace Windows
{
    namespace Foundation
    {
        apicontract UniversalApiContract;
    }
}

namespace Windows
{
    namespace UI
    {
        [contract(Windows.Foundation.UniversalApiContract, 8.0)]
        [uuid(AB9BABFD-B632-4214-B698-D0E5EB9EE566)]
        interface IWindowContextPartner : IInspectable
        {
        }
    }
}
'@
Set-Content $renamedUiDependencyPath $windowContextPartnerIdl -Encoding utf8

Remove-Item $systemDependencyPath -Force -ErrorAction SilentlyContinue

$composition = Get-Content $compositionPath -Raw
$windowsUiImport = 'import "Windows.UI.idl";'
$dependencyImport = 'import "Windows.UI.Composition.Dependencies.idl";'
if ($composition.IndexOf($dependencyImport, [StringComparison]::Ordinal) -lt 0)
{
    if ($composition.IndexOf($windowsUiImport, [StringComparison]::Ordinal) -lt 0)
    {
        throw "The generated Composition IDL does not import Windows.UI.idl."
    }

    $composition = $composition.Replace(
        $windowsUiImport,
        "$windowsUiImport`r`n$dependencyImport"
    )
    Set-Content $compositionPath $composition -Encoding utf8
}
