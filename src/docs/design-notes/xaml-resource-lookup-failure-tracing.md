# Improving Diagnosability of Missing XAML Resources

## Introduction
Failure to resolve a XAML resource reference (`{StaticResource}` or `{ThemeResource}`) is one of the most
common causes of app crashes. Such failures manifest as a stowed exception with the message "Cannot find a 
Resource with the Name/Key *foo*", and generally fall into one of two buckets:

1. The referenced resource legitimately does not exist, e.g. the referenced key is misspelled or the intended
matching resource has not been added to the app
2. The referenced resource *does* exist in the app, but it is not reachable from the reference at run-time

Unfortunately, the stowed exception message is the *only* information provided about the error, which makes it 
difficult or, more often, outright impossible to debug. Fundamentally, the lack of useful debugging information 
is due to the fact that the parser can only report *after the fact* that the referenced resource cannot be 
found; that is, the "point of failure" is well after all useful information has already been popped off the stack.

There are two general approaches that can be taken to address this shortcoming:

1. Compile-time static analysis to detect unresolveable resource references
2. Run-time recording of resource search

This document will examine both approaches in order to explain why the second approach should be pursued.

## Compile-time static analysis
It's easy to imagine that XamlCompiler could do static analysis at compile-time to identify XAML resource 
references in an app's markup and verify that they will be resolvable at run-time. Being able to tell developers 
at compile-time that their app contains XAML resource references that will fail to resolve is the holy grail as it 
allows developers to preemptively identify and address issues before they cause crashes for customers. 
Additionally, it would increase our own productivity by reducing the number of Watson hits caused by unresolvable
XAML resource references.

Unfortunately, this is significantly easier said than done. XAML's `ResourceDictionary`s are fully mutable at 
run-time which makes it impossible to statically determine ahead of time what resources will be reachable at
run-time when the parser is attempting to resolve a resource reference. A concrete example of this scenario is 
Shell: they use a configuration key to determine what version of the WinUI 2 styles should be used at run-time, and 
dynamically insert an appropriately configured instance of [`XamlControlsResources`](https://docs.microsoft.com/en-us/windows/winui/api/microsoft.ui.xaml.controls.xamlcontrolsresources?view=winui-3.0) into `Application.Current().Resources`. 
There is no way for a static analyzer to realize that this insertion of `XamlControlsResources` into the app's global resources is even happening, not to speak of guaranteeing that it will succeed before any attempts are made 
to reference resources contained therein.

While in principle we could perform best effort static analysis, this would be of limited value because the most 
interesting failures are precisely the ones we would be unable to detect: a resource is *expected* to be resolvable
but inexplicably is not. 

## Run-time recording
An alternative to compile-time static analysis is to record at run-time the actual resource search; in the event of 
a failure, such a recording could be examined by a developer during postmortem investigation to determine why the 
referenced resource is not present in the expected search location. A sufficiently detailed recording would
effectively allow for (discount) Time-travel Debugging of XAML resource reference lookups. Consider the 
aforementioned example of Shell: if the parser failed to resolve a reference to a WinUI 2 resource, a Shell 
developer might notice that the recording indicates that a `XamlControlsResources` instance was not visited during
the search for the resource which could indicate that an error occurred earlier during the dynamic insertion of the 
resource dictionary into the app's global resources.

Given that compile-time static analysis was previously determined to be nonviable, this is clearly the
only suitable approach.

## Proposed design
Since the vast majority of resource lookups are successful (failure is, after all, exceptional), we cannot just 
record every search as that would incur intolerable performance overhead. However, resource lookup is both 
deterministic and idempotent which means that in the event of a resource lookup failure, the parser can rerun the 
search with recording turned on and the result will be the same as if it had been recording during the initial 
search.

Instead of immediately logging a stowed exception in the event of failure to resolve a resource reference, the 
parser will do the following:

1. Rerun the resource lookup, but this time passing in a `isTracing` flag as a parameter which will be plumbed 
through to the individual `ResourceDictionary`s. This flag serves two purposes: first, it informs the search that 
actions related to tracing should be performed, and second it disables the `ResourceDictionary` caching mechanism 
for non-existent keys (we want to ensure that no shortcuts are taken and all `ResourceDictionary`s that had been 
visited by the original search are visited once again).
2.  Log tracing information
    - When the `ResourceDictionary` is queried for the resource key (`CResourceDictionary::GetKeyNoRefImpl()`), 
    log the base URI of the `ResourceDictionary` (special cases will need to be handled; known special 
    dictionaries like the framework's `generic.xaml` will be hard-coded while others, like dictionaries 
    constructed in code-behind, will simply be noted as `<anonymous dictionary>` (there is not much value in 
    assigning them a unique ID as (a) it wouldn't be stable without additional bookkeeping, (b) there would be no 
    correlation to the source code anyway).
    - A message will be logged before and after performing lookups into subsidiary dictionaries (`MergedDictionaries
    ` and `ThemeDictionaries`).
    - Tracing information is stored in several different places:
        - If a new property, `IsXamlResourceReferenceTracingEnabled`, on [`DebugSettings`](https://docs.microsoft.com/en-us/windows/winui/api/microsoft.ui.xaml.debugsettings?view=winui-3.0)
        is set to `true`, then the logged information will be output to the debugger and an event,
        `XamlResourceReferenceFailed` (also defined on `DebugSettings`), will be raised
        - Logged information will always be sent to the XAML ETW provider
        - Logged information will always be stored in a data structure (i.e. glorified gigantic string) that is 
        allocated on the heap so that it is available in heap dumps
3. Raise stowed exception
