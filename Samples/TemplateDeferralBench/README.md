# TemplateDeferralBench

Synthetic A/B benchmark for deferring rarely used control-template parts (`x:Load="False"`) in WinUI's built-in
control templates. Each scenario creates N instances of one control, measures the first frame, and then triggers
the feature that needs the deferred parts. Probes then check that those parts were realized and behave correctly.

## Build

Run from the repo root after a `product` build of the flavor you want to test:

```powershell
.\initrun.ps1 -Flavor amd64fre msb /q /restore "Samples\TemplateDeferralBench\TemplateDeferralBench.csproj" /p:Platform=x64 /p:Configuration=Release
```

`app.manifest` registers the locally built MUX/MUXC classes. A self-contained app built against the local
`3.0.0-dev` package doesn't get those registrations otherwise, and fails at startup with `0x80040111`. Regenerate it
with `python Samples\TemplateDeferralBench\gen-manifest.py` when the package's class list changes.

## Run

Copy the build output to a folder, then overlay `BuildOutput\packaging\Release\runtimes\win10-x64\native`. Then:

```powershell
TemplateDeferralBench.exe list                                  # writes scenarios.txt
TemplateDeferralBench.exe <label> <scenario> <count> <out.json> # one measurement
```

Run the exe from a clean shell, not one set up by `initrun.ps1`; its `DOTNET_ROOT` breaks startup (`0x80008096`).

## A/B

Templates come from the framework binaries next to the exe. Loose XAML ignores `x:Load`, so app-supplied
templates can't reproduce the deferral. For the eager baseline, build the product and this app with the added
`x:Load="False"` attributes removed, and deploy that output to a second folder. The app's `.pri` also contains the
MUXC templates, so build it once per variant too. Interleave runs of the two folders.

Output metrics: `FirstFrameWallMs`, `FirstFrameUiCpuMs`, `ElementsAtStartup`, `DeferredPartsAtStartup`,
`PrivateBytesMB`, `WorkingSetMB`, `TriggerMs`, `ElementsAfterTrigger`, `Probe.*`, `ProbesPass`.
