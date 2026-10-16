# Monitor topology test driver

This directory contains the VM-only virtual display driver used by the WinUI
mixed-DPI interaction test. It is development test infrastructure, not a
production driver.

The driver is an x64 UMDF2/IddCx root-enumerated display adapter. A controller
opens its exclusive device interface, adds virtual monitors, and removes them
before closing the handle. The current WinUI test adds one 2560x1440 monitor,
sets it to 150% scale, and validates WinUI after a window moves to that monitor.

## Build

The driver builds only from `MonitorTopologyTestDriver.sln` or
`Build-Driver.ps1` in this directory. Normal `build.cmd` builds do not
include it, so they do not require the WDK. To build the driver explicitly
from the repository root, install Visual Studio 2022 C++, Windows SDK
10.0.26100.0, and the matching WDK, then run:

```powershell
.\dxaml\test\MonitorTopologyTestDriver\Build-Driver.ps1
```

The build produces an unsigned package in
`MonitorTopologyTestDriver\out\x64\<Debug-or-Release>\MonitorTopologyTestDriver`.
It does not install the driver, create or trust certificates, or change
signing policy.
The WDK is also available as a NuGet package (`Microsoft.Windows.WDK.x64`),
but this project currently uses the installed WDK toolset. NuGet does not
remove the need for Visual Studio driver build components.

## Opt-in local VM provisioning

Provision only a disposable Windows 11 x64 VM with a recovery checkpoint and
an interactive console session. This test currently requires Windows build
26660, test signing enabled, and a guest administrator account. Do not enable
test signing or turn off Secure Boot on a development machine.
If the VM has an older version of this test driver, restore its pre-driver
checkpoint before installing the renamed driver. Its hardware ID and control
interface have changed; the installer does not replace or remove old adapters.

From the repository root, **explicitly** run:

```powershell
.\initrun.ps1 .\build.cmd
.\dxaml\test\MonitorTopologyTestDriver\Build-Driver.ps1
.\dxaml\test\MonitorTopologyTestDriver\Install-DriverOnVM.ps1 -VMName <vm-name>
.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName <vm-name> 'Microsoft.UI.Xaml.Tests.Controls.MonitorTopologyTests.WindowUpdatesDpiAfterMonitorDrag' -AdditionalArgs '-RunIgnoredTests'
```

The installer defaults to the Debug driver package. For a Release build,
pass `-DriverPackageDir` with the Release package path. Building WinUI does
not build or install the driver; the separate driver build is required once
before provisioning a fresh VM.

The driver grants read/write control-interface access to local users and app
packages so the packaged managed test host can control it. Install it only on
the disposable test VM described above.

`Install-DriverOnVM.ps1` is never called by a build, test payload, or shared
test setup. It uses PowerShell Direct to connect to the named, running Hyper-V
VM. It checks the VM and OS build, then signs a *copy* of the unsigned driver
catalog with a temporary development certificate. It trusts only the public
certificate in the VM, then installs or updates the driver package. It creates
one root adapter only when none is present, and confirms that the adapter starts.
If an update fails, it leaves the existing adapter in place. Each run creates a
new temporary certificate and adds its public certificate to the VM's Root and
TrustedPublisher stores; those certificates remain in the disposable VM. The
installer does not change boot policy or add certificate trust on the host. The
guest needs test signing enabled before running the installer; the installer
will not change that policy.

The installer uses the same encrypted per-VM credential cache as
`tools\run-tests-on-vm.ps1`, or prompts for credentials. You can also pass
`-Credential` or `-WhatIf`. The caller must be a local administrator or a
member of Hyper-V Administrators. The guest account must be an administrator.
The WDK tools and SDK version used to build the driver must be installed on
the host; use `-WdkToolsDir` and `-SdkVersion` for other paths.

`MonitorTopologyTests` has `Ignore=True`, so ordinary test runs do not select
it. Specify both the test name and `-RunIgnoredTests` to run it deliberately.
Using `-RunIgnoredTests` with a broad selector can also run other ignored tests.
The test requires an elevated interactive desktop and fails if no driver is
installed; it never installs the driver itself.

Do not disable the VM's normal display adapter. The driver does not arrange
monitors, set the primary display, or restore the desktop layout. If Windows
does not activate the new monitor as an extended display, the test fails.

The test uses private display-config DPI packets and is pinned to the Windows
build named in `MonitorTopologyTests.cs`. A new build must be qualified before
updating that pin.

## Licensing

The sample-derived native driver is under the Microsoft Public License. See
`MonitorTopologyTestDriver\LICENSE` and `THIRD-PARTY-NOTICES.txt`. The shared
controller protocol is original MIT-licensed code.
