##############################################################################  
##  
## Add-Aliases.ps1  
##
## Sets some common aliases for PowerShell consoles.
## Mostly based on aliases found in the ..\aliases file
##
##############################################################################  

function global:root { pushd $env:reporoot }
function global:native { pushd $env:reporoot\dxaml\xcp\dxaml\dllsrv\winrt\native\$args }
function global:tfgr { pushd $env:reporoot\dxaml\test\native\external\foundation\graphics\rendering\$args }
function global:dxaml { pushd $env:reporoot\dxaml\$args }
function global:xcp { pushd $env:reporoot\dxaml\xcp\$args }
function global:text { pushd $env:reporoot\dxaml\xcp\core\native\text\Controls\$args }
function global:scripts { pushd $env:reporoot\scripts\$args }
function global:idl { pushd $env:reporoot\dxaml\xcp\dxaml\idl\winrt\$args }
function global:elements { pushd $env:reporoot\dxaml\xcp\core\core\elements\$args }
function global:core { pushd $env:reporoot\dxaml\xcp\core\$args }
function global:codegen { pushd $env:reporoot\dxaml\xcp\tools\XCPTypesAutoGen\XamlOM\Model\$args }
function global:masters { pushd $env:reporoot\dxaml\test\resources\masters\$args }
function global:cb { git branch | select-string "\*" -raw }

function global:ctp { & "$env:reporoot\test\CreateTestPayload.cmd" $args; pushd "$env:reporoot\TestPayload\$env:BUILDPLATFORM$env:_BuildType" }
function global:ctps { & "$env:reporoot\test\CreateTestPayload.cmd" -mode ScenarioTestSuit $args; pushd "$env:reporoot\TestPayload\$env:BUILDPLATFORM$env:_BuildType" }
function global:dbo { & taskkill /f /im msbuild.exe; & taskkill /f /im vbcscompiler.exe; & rd $env:reporoot\BuildOutput -Force -Recurse }
function global:tp { pushd $env:reporoot\TestPayload }

# Specialized chdir commands
function global:up      { pushd ..\$args }
function global:up1     { pushd ..\$args }
function global:up2     { pushd ..\..\$args }
function global:up3     { pushd ..\..\..\$args }
function global:up4     { pushd ..\..\..\..\$args }
function global:up5     { pushd ..\..\..\..\..\$args }
function global:up6     { pushd ..\..\..\..\..\..\$args }
function global:up7     { pushd ..\..\..\..\..\..\..\$args }
function global:up8     { pushd ..\..\..\..\..\..\..\..\$args }
function global:up9     { pushd ..\..\..\..\..\..\..\..\..\$args }
function global:..      { pushd ..\$args }
function global:...     { pushd ..\$args }
function global:....     { pushd ..\..\$args }
function global:.....     { pushd ..\..\..\$args }
function global:......     { pushd ..\..\..\..\$args }
function global:.......     { pushd ..\..\..\..\..\$args }
function global:........     { pushd ..\..\..\..\..\..\$args }
function global:.........     { pushd ..\..\..\..\..\..\..\$args }
function global:..........     { pushd ..\..\..\..\..\..\..\..\$args }
function global:...........     { pushd ..\..\..\..\..\..\..\..\..\$args }
