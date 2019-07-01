function GetAzureDevOpsBaseUri
{
    return $env:SYSTEM_COLLECTIONURI + $env:SYSTEM_TEAMPROJECT
}

function GetQueryTestRunsUri
{
    $baseUri = GetAzureDevOpsBaseUri
    $queryUri = "$baseUri/_apis/test/runs?buildUri=$($env:BUILD_BUILDURI)"
    return $queryUri
}