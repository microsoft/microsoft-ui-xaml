These are augmentations to ensure the graph is aligned across solutions.

In particular this aids in aligning the sets of targets. For example, WindowsAppContainer call GetPackagingOutputs on
their dependencies, and `Microsoft.UI.Xaml.sln` has a WindowsAppContainer project which depends on 
`dxaml\xcp\dxaml\manifest\manifest.vcxproj` while MUXControls.sln does not. This causes the two slns to call different
sets of targets on the project and causes Project Caching cache misses.

These dummy projects just claim that they will call specific targets on specific projects in order to help alignment.
The do not actually do anything at build time.
