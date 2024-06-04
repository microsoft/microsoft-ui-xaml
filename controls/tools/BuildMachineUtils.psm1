function Get-AdalPath
{
    $adalVersion = "3.17.2"
    $adalPath = "$env:USERPROFILE\.nuget\packages\microsoft.identitymodel.clients.activedirectory\$adalVersion"

    if (-not (Test-Path $adalPath))
    {
        $installLocation = Get-Package Microsoft.IdentityModel.Clients.ActiveDirectory -MaximumVersion $adalVersion -MinimumVersion $adalVersion -ErrorAction Ignore
        if (!$installLocation)
        {
            Install-package Microsoft.IdentityModel.Clients.ActiveDirectory -Source https://nuget.org/api/v2/ -ProviderName nuget -MaximumVersion $adalVersion -Scope CurrentUser
            $installLocation = Get-Package Microsoft.IdentityModel.Clients.ActiveDirectory -MaximumVersion $adalVersion
        }

        $adalPath = Split-Path $installLocation.Source
    }

    $adalPath
}

function Get-AuthenticationContext
{
    # Get an Access Token with ADAL

    $authContext = New-Object Microsoft.IdentityModel.Clients.ActiveDirectory.AuthenticationContext("https://login.windows.net/common")

    if ($authContext.TokenCache.Count -gt 0)
    {
        $homeTenant = $authContext.TokenCache.ReadItems()[0].TenantId;
        $authContext = New-Object Microsoft.IdentityModel.Clients.ActiveDirectory.AuthenticationContext("https://login.microsoftonline.com/$homeTenant");
    }
    else
    {
        $authContext = New-Object Microsoft.IdentityModel.Clients.ActiveDirectory.AuthenticationContext("https://login.microsoftonline.com/microsoft.onmicrosoft.com");
    }

    $authContext;
}

function Get-AccessToken
{
    $promptBehavior = [Microsoft.IdentityModel.Clients.ActiveDirectory.PromptBehavior]::Auto

    $resourceId = "499b84ac-1321-427f-aa17-267ca6975798";
    $clientId = "872cd9fa-d31f-45e0-9eab-6e460a02d1f1";
    
    $authCtx = Get-AuthenticationContext
    $authResultTask = $authCtx.AcquireTokenAsync($resourceId, $clientId, [System.Uri]"urn:ietf:wg:oauth:2.0:oob", 
        (new-object Microsoft.IdentityModel.Clients.ActiveDirectory.PlatformParameters($promptBehavior, $null)));

    while (!$authResultTask.IsCompleted) {
        Start-Sleep -Seconds 1
    }

    $authResultTask.Result.AccessToken
}

function Queue-BuildOnMachine
{
    Param(
        [string]$MachineName,
        [string]$ClientAlias,
        [string]$BuildId)

    $token = Get-AccessToken

    $headers = @{ 
            "Authorization" = ("Bearer {0}" -f $token);
            "Content-Type" = "application/json";
        }

    $root = @{
        "sourceBranch" = "refs/heads/main";
        "definition" = @{
            "id" = $BuildId
        };
        "project" = @{
            "id" = "8d47e068-03c8-4cdc-aa9b-fc6929290322" # OS
        };
        "repository" = @{
            "id" = "d39af991-db55-43c3-bdef-c6333a2b3264" # dep.controls
        };
        "demands" = @(
            "ClientAlias -equals $ClientAlias";
            "COMPUTERNAME -equals {0}" -f $machineName
        )
    };

    $jsonPayload = ConvertTo-JSon $root

    Write-Verbose "Payload = $jsonPayload"

    $result = Invoke-RestMethod -Method Post -Uri "https://microsoft.visualstudio.com/DefaultCollection/OS/_apis/build/builds?api-version=4.1-preview" -Headers $headers -Body $jsonPayload

    $result
}

# Initialize ADAL when this module is imported

$adalPath = Get-AdalPath

Write-Verbose "ADALPath: $adalPath"
Add-Type -Path "$adalPath\lib\net45\Microsoft.IdentityModel.Clients.ActiveDirectory.dll"
