# Code architecture documentation

This is a collection of documentations which provide a high level design oriented overview of different parts of WinUI codebase. It is an ongoing effort, so expect large scale changes to content as well as format.

## Contents:

### Concepts

* [Journey of a control](./control-overview.md) - a vertical slice of what makes up a WinUI control
* [Xaml compiler overview](./xamlcompiler.md)
* [Codegen](./codegen.md)
* [Xaml/C# Object Lifetime Design](./xaml-object-lifetime.md)
* [Surfaces in Xaml](./surfaces-overview.md) - Use of Composition and Direct3D surfaces in Xaml
* [Dxaml vs Core layers / Peer objects](./dxamlvscore.md) - A writeup explaning difference between Dxaml and core layers, peer objects and how to transition between the two objects for a given type 
* [XAML Rendering Architecture](./rendering.md) -  This document gives a high-level overview of how the XAML rendering engine works, primarily covering integration with the system compositor
* [UI Thread ticking](./ui-thread-ticking.md) - A writeup on ticks in ui thread and how layout, animation and other parts of UI depend on it
* [Xaml theming resources](./resources.md) - A writeup on everthing about Xaml theming resources and how they are created and used
* [Read-Only Text Controls Architecture](./text-controls.md) - This document describes the architecture of XAML’s read-only text controls, and supporting functionality in the XAML platform to make them fully functional in a XAML application.
* [Custom titlebar](./customtitlebar.md) - Explains the inner working of custom titlebar feature in Desktop WinUI 3 apps, including glass window concept
* [Unconstrained Popups](./unconstrained-popup.md) - A spec about a new(er) option for ContentDialogPlacement: UnconstrainedPopup

### Coding resources

* [Guide to WinUI codebase pointers](./pointers.md)
* [Startup path for WinUI application](./startup-overview.md)
* [Xaml Behaviors](./xamlbehaviors.md) - Lists deprecated features which are still in codebase and getting slowly phased out