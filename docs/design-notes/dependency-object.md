# DependencyObject

## Table of Contents

- [Overview](#overview)
- [DependencyProperty](#dependencyproperty)
  - [Multi-parent DOs and associations](#multi-parent-dos-and-associations)
    - [Checking associations](#checking-associations)
    - [Setting associations](#setting-associations)

## Overview

`CDependencyObject` (DO) is the base class for many types in Xaml. This document describes some implementation details
behind DOs and `DependencyProperties` (DPs).

## DependencyProperty

### Multi-parent DOs and associations

Most DOs, like UIElements, are only allowed a single parent. Adding the same `Canvas` to multiple `Grid`s results in an
error. Some DOs, like `Brush`, can have multiple parents. For example, the same `SolidColorBrush` can be used to fill
the background of multiple `Grid`s. These parent-child links are referred to as _associations_. An object has an association if it's connected to a parent in the tree.

#### Checking associations

Many Xaml APIs that put DOs in the tree also check whether the object is already associated, or whether the new
connection would give an object multiple associations. There are two mechanisms for this:

1. **Via the property system.** When an object is set on a `DependencyProperty`, it goes through
   `CDependencyObject::SetValue`. The `SetValue` call then checks whether the new value has association problems. The
   stack is

```
Microsoft_UI_Xaml!CDependencyObject::VerifyCanAssociate
Microsoft_UI_Xaml!CDependencyObject::SetEffectiveValue
Microsoft_UI_Xaml!CDependencyObject::UpdateEffectiveValue
Microsoft_UI_Xaml!CDependencyObject::SetValue
...
```

`CDependencyObject::VerifyCanAssociate` (in `dxaml/xcp/components/DependencyObject/PropertySystem.cpp`)
has an if statement that is triggered when:
1. The new object is already associated (i.e. already has a parent),
2. The property being set is has association restrictions (i.e. an unassociated object being set on this property becomes associated), and
3. The new object doesn't allow multiple associations (i.e. it's single-parent only).

If all three conditions hold then there's a problem - we're trying to give a second parent to an object that only allows
one. `VerifyCanAssociate` will confirm that we're not setting the same object, try to back it out, and if all else fails
it will raise an error.

Note that not all properties have association restrictions. Those that care are annotated with a
`[RequiresMultipleAssociationCheck]` tag in codegen (see `dxaml/xcp/tools/XCPTypesAutoGen/XamlOM/Model/Microsoft.UI.Xaml.Media.cs`).
One example is `Border.Child`. If an element is the child of one Border, it can't be the child of
another. Some properties, like
`ItemsControl.ItemsHost` (see `dxaml/xcp/tools/XCPTypesAutoGen/XamlOM/Model/Microsoft.UI.Xaml.Controls.cs`),
don't have such a restriction. These are typically properties that don't actually put the object in the tree.
[`ItemsControl.ItemsHost`](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.itemscontrol.itemspanel?view=windows-app-sdk-1.1)
isn't in the tree; it's just a template for the panel that controls the layout of the items. The same `Grid` can serve
as the template of many different `ItemsControl`s.

2. **Via `CDOCollection`.** When an object is added to a `CDOCollection`, the collection will check the new object's
   associations automatically. The stack is

```
Microsoft_UI_Xaml!CDOCollection::PreAddToCollection
Microsoft_UI_Xaml!CCollection::Add
...
```

`CDOCollection::PreAddToCollection` (in `dxaml/xcp/components/Collection/DOCollection.cpp`)
checks whether the new object is associated already, and if so raises an error.

Note that unlike the property system code path, all CDOCollections enforce that their contents can't have multiple
associations. There's no way to opt-out by leaving out the `[RequiresMultipleAssociationCheck]` tag.

#### Setting associations

When it comes to marking an object as associated, there are the same two cases:

1. **Via the property system.**
   `CDependencyObject::EnterEffectiveValue` and
   `CDependencyObject::LeaveEffectiveValue` (in `dxaml/xcp/components/DependencyObject/PropertySystem.cpp`)
   are the places that set and clear associations. They only set the association if either the property cares about
   multiple associations or if the object is allowed to have multiple parents. This excludes cases like
   `ItemsControl.ItemsHost` from touching associations.

2. **Via `CDOCollection`.**
   `CDOCollection::SetChildParent` (in `dxaml/xcp/components/Collection/DOCollection.cpp`)
   is the method that does this, and it's called from the modification APIs like `Append`, `Insert`, `RemoveAt`, and
   `Neat` (which clears the collection).
