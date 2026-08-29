# .NET Validation for WinUI Applications

## Why Validation Is Needed

Validation is a critical step in the development lifecycle of WinUI applications built on the .NET platform. It ensures that:

- Application components behave reliably across different .NET versions (e.g., .NET 8, 9, and 10 RC2), especially when integrating WinUI 3 features.
- Developers can confidently upgrade SDKs without breaking existing functionality, such as hot reload, live preview, or packaging workflows.
- Component libraries are correctly referenced and consumed, which was a recurring issue in earlier validation cycles until updated steps were introduced.
- Internal documentation and signoff processes remain accurate and actionable, reducing delays and miscommunication during release planning.
- NuGet feed availability and SDK compatibility are verified before rollout, preventing build failures and developer frustration.

In short, validation helps maintain quality, consistency, and developer productivity—especially for teams working on WinUI-based desktop applications in enterprise environments.

---

## .NET Release Cadence

Microsoft follows a predictable annual release cadence for .NET, typically launching new versions every November. These releases alternate between:

- **LTS (Long-Term Support):** Supported for 3 years.
- **STS (Standard-Term Support):** Supported for 18 months.

For example:

- .NET 8 (LTS) was released in November 2023.
- .NET 9 (STS) followed in November 2024.
- .NET 10 (LTS) is expected in November 2025.

Each release introduces new runtime features, SDK tooling updates, and changes to project templates and compatibility layers—especially relevant for WinUI 3 applications. Validation during these cycles ensures that existing workflows, component libraries, and developer tooling remain functional and performant across the updated platform.

---

## Monthly Update Cadence

In addition to major annual releases, Microsoft maintains a consistent **monthly update cadence**, typically releasing servicing updates on the second Tuesday of each month. These updates include:

- Security patches
- Bug fixes
- Minor enhancements to the runtime, SDKs, and tooling

While these updates may not introduce breaking changes, they can affect build stability, NuGet feed availability, and compatibility with Visual Studio previews—especially for WinUI applications that rely on tightly coupled SDK behavior. Validation during these monthly cycles ensures that:

- Existing projects continue to function as expected.
- New SDK versions are correctly integrated into internal workflows.
- Any regressions or feed issues are identified early.

This proactive validation helps maintain developer confidence and minimizes disruption across engineering teams working on production-grade WinUI apps.

---

# WinUI Validation Tests and Setup Guide

## Prerequisites

Before conducting validation tests for WinUI applications, ensure the following components are installed:

- Latest Visual Studio ([download here](https://visualstudio.microsoft.com/downloads/))
- Latest Project Reunion NuGet package and its VSIX extension
- **.NET Desktop Development** workload
- **WinUI Application development** workload
---

## Setting Up .NET build 

### Configure NuGet Feed

#### Option 1: From Release Manager

1. Visit the **Release Manager - .NET Release Tracker**.
2. Copy the feed URL from the **Feed** link.
3. In Visual Studio, go to `Tools > NuGet Package Manager > Package Manager Settings`.
4. Under **Package Sources**, click the green `+` icon and add the feed URL.

#### Option 2: From ADO Pipeline

1. Open the ADO Pipeline run for the desired build.
2. Download all `.nupkg` files under **Final Packages**.
3. In Visual Studio, go to `Manage NuGet Packages for Solution`.
4. Add a new package source pointing to the local folder containing the `.nupkg` files.

---

## Validation Test Scenarios

### Create New C# Desktop Project

- Template: **Blank App, Packaged (WinUI 3 in Desktop)**
- Expected Result: App is launched successfully

### Create New C++ Desktop Project

- Template: **Blank App, Packaged (WinUI 3 in Desktop)**
- Expected Result: Same behavior as C# project.

### Create New C# Class Library

- Template: **Class Library (WinUI 3 in Desktop)**
- Expected Result: Library builds successfully.

### Create New Windows Runtime Component

- Template: **Windows Runtime Component (WinUI 3)**
- Expected Result: Component builds successfully.

---

## C# Project Referencing a C# Library User Control

Steps include:

- Adding a UserControl to the class library.
- Creating a `Person` class.
- Updating `UserControl1.xaml` and `UserControl1.xaml.cs`.
- Referencing the library in a desktop project.

**Expected Result:**  
The app displays input fields and a greeting message using the `Person` class.

---

## C# Project Referencing a WinUI 3 Component Library

- Add reference to a C++/WinRT component (e.g., `ThermometerWRC`).
- Use `cswinrt` NuGet package.
- Instantiate and use the `Thermometer` class in C#.

**Expected Result:**  
Component is instantiated and method is invoked successfully.

---

## C++ Project Referencing a WinUI 3 Component Library

- Add reference to `ThermometerWRC` from a C++ desktop project.
- Use the component in a button click handler.

**Expected Result:**  
`Thermometer` object is created and temperature adjusted.

---

