param (
    [string]$OutFile,    
    [string]$ChainedJobName,
    [string]$SourceBranch=$env:BUILD_SOURCEBRANCH
)

pushd build\BuildChainer\

(Get-Content ReleaseTestChainerTemplate.json) -replace '<CHAINED_JOB_NAME>',$ChainedJobName -Replace '<SOURCE_BRANCH>',$SourceBranch | Out-File -Encoding UTF8 $OutFile

popd