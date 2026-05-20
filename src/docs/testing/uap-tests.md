
# UAP Tests

A bunch of our tests are marked with "Hosting:Mode" "UAP", and this tells the infra to run them in a UWP context.
We want to reduce and remove our dependence on UWP, so we want to convert these tests to use the default "WPF" hosting mode instead.

Look for the tag `WPF_HOSTING_MODE_FAILURE`.  We put this on tests we recently found to be failing in WPF mode, they need more investigation.

## Progress

|Date|UAP-hosted tests|Total tests| % remaining from start |
|-|-|-|-|
| 12/1/23  | 1864 | 7035 | ██████████████████████████████████████████████████ 100% |
| 12/5/23  | 1715 | 7025 | ██████████████████████████████████████████████ 92% |
| 12/8/23  | 1569 | 7026 | ██████████████████████████████████████████ 84% |
| 12/12/23 | 1477 | 7026 | ████████████████████████████████████████ 79% |
| 12/15/23 | 1056 | 7025 | █████████████████████████████ 57% |
| 2/28/24 | 1048 | 7040 | █████████████████████████████ 56% |

## Common Issues

### Popup IsOpen="true" in markup doesn't work
Initialize Popup with IsOpen=true leads to crash in WinUI Desktop.

### Popups, Flyouts, FocusManager.GetFocusedElement() require a XamlRoot
Xaml needs a way to know which XamlRoot to use to show content from a Popup, Flyout, or ContentDialog.

Also, FocusManager.GetFocusedElement() without parameters is only meaningful in UWP, won't work in supported WinUI 3 scenarios.

### WindowContent doesn't accept a custom control
Xaml test infra doesn't support setting a custom C++/Cx control as WindowHelper->WindowContent.

Consider using **TreeHelper::WrapInGrid()** as a one-line way to wrap a custom control in a grid.

### GC Collect may be needed
Some tests validate that a weak ref got cleaned up.  But if the managed code in the WPF-hosting test infra got a ref to this object,
you'll need to run the garbage collector to ensure the object's refcount gets to zero.  Consider calling **WindowHelper.GCCollect()**.

### Text input  is slightly different
In UWP Xaml uses TSF ("Text Services Framework") 3, but in win32 we use TSF 1.  This can lead to small differences.