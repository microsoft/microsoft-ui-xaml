# Runtime Enabled Features

Runtime enabled features are hidden behind a feature flag. They can be disabled
by default and easily enabled by developers, or vice versa. This is useful in
cases where a feature may not be compatible for all users, so Xaml can enable
that functionality for staging and testing prior to turning it on.

## Adding a new runtime enabled feature

First, add the new feature to `RuntimeEnabledFeaturesEnum.h` as well as
`RuntimeFeatureData.cpp`, and specify a default value.

Then, all code associated with the feature should be enclosed in an if block
that checks whether the feature is enabled. This can be found through the
following code--replacing FeatureName with the name added in the header files
earlier.

``` cpp
RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector()->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::FeatureName);
```

Now, all code associated with the new feature should only run when the feature
flag is enabled. 

### Turning on the feature via the registry

We can look up a dword regkey under `HKLM\Software\Microsoft\WinUI\Xaml`, and
use its value to set the RuntimeEnabledFeature. 

An example of this can be found in
`RuntimeEnabledFeatureDetector::IsFeatureEnabled`:

``` cpp
bool defaultOrRegKeyValue = featureData.DefaultEnabledState;
DWORD data = 0;
HKEY hkXaml = NULL;

// It's possible the XAML registry key doesn't exist at all. That's just fine.
if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, rootXamlKey, 0, KEY_READ, &hkXaml) == ERROR_SUCCESS)
{
    DWORD dwSize = sizeof(DWORD);

    // It's also possible the key for our feature doesn't exist at all. That's just fine too.
    if (RegQueryValueEx(hkXaml, keyname, 0, NULL, reinterpret_cast<LPBYTE>(&data), &dwSize) == ERROR_SUCCESS)
    {
        defaultOrRegKeyValue = (data != featureData.DisableValue);
    }

    RegCloseKey(hkXaml);
}
```

`defaultOrRegKeyValue` is now set according to the registry key.

#### Setting a key for local testing

Developers can set a key in an admin command prompt, by running a command like
the following:

```
reg add HKLM\Software\Microsoft\WinUI\XAML /v FeatureNameHere /t REG_DWORD /d 1
```

### Turning on the feature explicitly in a test

A test can explicitly enable or disable a RuntimeEnabledFeature to test the
feature/fallback, through RuntimeEnabledFeatureOverride. The following code is
an example of enabling the feature ForceProjectedShadowsOnByDefault.

``` cpp
RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
```
