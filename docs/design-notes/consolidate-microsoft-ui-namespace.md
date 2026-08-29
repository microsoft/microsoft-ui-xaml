
# Consolidating the Microsoft.UI namespace to one package

Xaml and IXP both contribute to the Windows.UI namespace in _system_ WinUI. 
That namespace has 5 types directly in it:
* [Color](https://docs.microsoft.com/uwp/api/Windows.UI.Color)
* [ColorHelper](https://docs.microsoft.com/uwp/api/Windows.UI.ColorHelper)
* [Colors](https://docs.microsoft.com/uwp/api/Windows.UI.Colors)
* [UIContentRoot](https://docs.microsoft.com/uwp/api/Windows.UI.UIContentRoot)
* [UIContext](https://docs.microsoft.com/uwp/api/Windows.UI.UIContext)

We’ve left Color in the `Windows.UI` namespace, because there are non-WinUI types that use it too. 
And we haven’t lifted UIContentRoot/UIContext to WinUI3. So today we just have Colors and ColorHelper 
in `Microsoft.UI`.

But IXP is adding more types to it (WindowId and DisplayId), and as we lift from one Windows 
to two packages, we can no longer both contribute to the same namespace 
(two packages can’t both have `Microsoft.UI` types). 

So the solution is to move all of Microsoft.UI to the IXP package, which means moving Colors and ColorHelper.

Note that none of this affects apps; it’s still the same types in the same namespace, 
and you still get all the types when you make a NuGet reference in your project to WinUI3.

Background on why the two packages can't both contribute to the Microsoft.UI namespace:
* The closure of dependent package DLLs go into the same directory in both the app’s build and deployment, 
so can’t conflict in name
* The DLL and WinMD need to have the same base filename
    * This is because the appx manifest generator finds the WinMD then assumes the DLL has the same name
* The WinMD filename needs to be a prefix of all type namespace names therein
    * Ex Foo.Bar.Banana type needs to be in Foo.Bar.Bananna.dll, or Foo.Bar.dll, or Foo.dll
    * Again this is a requirement of the appx manifest generator, but it’s also a requirement of 
    language runtimes that don’t use the appx manifest (ex JS), 
    and is also a requirement of manifest-free registration.
