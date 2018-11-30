[CmdLetBinding()]
Param()

function Load-Adal
{
    $adalVersion = "3.17.2"
    $installLocation = Get-Package Microsoft.IdentityModel.Clients.ActiveDirectory -MaximumVersion $adalVersion -MinimumVersion $adalVersion -ErrorAction Ignore
    if (!$installLocation)
    {
        Install-Package Microsoft.IdentityModel.Clients.ActiveDirectory -Source https://nuget.org/api/v2/ -ProviderName nuget -MaximumVersion $adalMaxVersion -Scope CurrentUser
        $installLocation = Get-Package Microsoft.IdentityModel.Clients.ActiveDirectory -MaximumVersion $adalMaxVersion
    }

    $adalPath = Split-Path $installLocation.Source

    Write-Verbose "ADALPath: $adalPath"

    Add-Type -Path "$adalPath\lib\net45\Microsoft.IdentityModel.Clients.ActiveDirectory.dll"
}

function Get-AuthenticationContext
{
    Load-Adal

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
    $resourceId = "499b84ac-1321-427f-aa17-267ca6975798";
    $clientId = "872cd9fa-d31f-45e0-9eab-6e460a02d1f1";
    
    $authCtx = Get-AuthenticationContext

    $promptBehavior = [Microsoft.IdentityModel.Clients.ActiveDirectory.PromptBehavior]::Auto
    
    $authResultTask = $authCtx.AcquireTokenAsync($resourceId, $clientId, [System.Uri]"urn:ietf:wg:oauth:2.0:oob", 
        (new-object Microsoft.IdentityModel.Clients.ActiveDirectory.PlatformParameters($promptBehavior, $null)));

    while (!$authResultTask.IsCompleted) {
        Start-Sleep -Seconds 1
    }

    $authResultTask.Result.AccessToken
}

function Get-AuthorizationHeaders
{
    $token = Get-AccessToken

    $headers = @{ 
        "Authorization" = ("Bearer {0}" -f $token);
        "Content-Type" = "application/json";
    }

    $headers
}