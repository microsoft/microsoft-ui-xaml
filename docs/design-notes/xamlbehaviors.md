# XamlBehavoirs

XamlBehaviors are a deprecated feature that we still have a few instances of in WinUI 3. Once they are removed, 
this document should be deleted.

XamlBehaviors are used to alter behavior based on SKU (desktop vs phone vs onecore). They are espeically useless, since 
Xaml does not support phone or OneCore today. A XamlBehavior looks similar to a compatibility quirk, but whereas Quirks turn on and off based on the runtime 
version an app is targeting or on a per-app basis (and maybe additional ways), XamlBehaviors look at the platform the 
app is running on.
``` c++
if (IsXamlBehaviorEnabledForCurrentSku(Foo_UsePhoneVersionOfFoo))
{
       // On phone we need to call into FooPhone.dll instead of Foo.dll here, since
       // the phone-specific binaries were not yet converged at the time of this code.
       szDllToLoad = L”FooPhone.dll”;
}
```
So, we used XamlBehaviors when we needed code to have special logic on Windows desktop vs. phone. This should be rare, 
we’re trying to remove XamlBehaviors wherever we can.  XamlBehaviors are deprecated.

Each XamlBehavior used to be represented by an enum value defined in XamlBehaviorMode.h, but now the only check done is 
``` c++
bool IsXamlBehaviorEnabledForCurrentSku(enum XamlBehavior behavior)
{
    return !DesktopUtility::IsOnDesktop();
}
```