# Testing WinUI 3 Changes with Sample C++ App (Dev Package)

## 1. Build the WinUI repo
1. Follow [developer guide](../building/developer-guide.md)


## 2. Get Visual Studio template

Use Visual Studio Installer:

- **Workload:** WinUI application development
- **Individual Components:** C++ WinUI app development tools


## 3. Create Sample Project

1. Launch Visual Studio.
2. New Project → search: **WinUI Windows C++**
3. Select: **WinUI Blank App (Packaged)**


## 4. Add Local Package Source

1. Open NuGet Manager Right-click project → **Manage NuGet Packages**.
2. Click **Gear** (Package Sources) icon. Click **+**.
3. **Name:** `LocalWinAppSDK`
4. **Source:** Full path to `PackageStore` built from the WinUI repo
5. Click **OK**.


## 5. Update Main Package

1. Choose `LocalWinAppSDK` in package source dropdown.
2. Go to **Installed** tab → `Microsoft.WindowsAppSDK`
3. Pick latest WinUI version


## 6. Remove Public Package Parts

1. You can do it from installed tab
2. Quicker way will be Edit `packages.config` and delete entries for `Microsoft.WindowsAppSDK.*` with public versions (e.g. 1.8.x).

## 7. Clean .vcxproj References

1. Unload project → edit `.vcxproj`
2. Remove Microsoft.WindowsAppSDK parts with public version
3. Add `<WindowsAppSDKSelfContained>true</WindowsAppSDKSelfContained>` under `<PropertyGroup Label="Globals">`
4. Reload project.

## 8. Delete Stale Package Folders

In project `packages` folder:

Remove `Microsoft.WindowsAppSDK.*` folders for public versions.

## 9. Clean & Build

Build → **Clean Solution** then **Rebuild**.
