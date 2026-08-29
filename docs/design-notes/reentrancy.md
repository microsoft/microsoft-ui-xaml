# Reentrancy

## Table of Contents

- [Background](#background)
  - [ASTA vs STA Threading models](#asta-vs-sta-threading-models)
  - [Lifted DispatcherQueue allows messages to pump re-entrantly](#lifted-dispatcherqueue-allows-messages-to-pump-re-entrantly)
- [Nested Message Pumps](#nested-message-pumps)
  - [EnableReentrancyChecks](#enablereentrancychecks)
  - [Example of a Xaml failfast due to app running a nested message pump](#example-of-a-xaml-failfast-due-to-app-running-a-nested-message-pump)
- [Reentrancy-protected messages](#reentrancy-protected-messages)
- [CReentrancyGuard](#creentrancyguard)
  - [Usage](#usage)
  - [Manifestation](#manifestation)
  - [Example](#example)
- [Reentrancy Checks](#reentrancy-checks)
- [Avoiding reentrancy](#avoiding-reentrancy)
- [Reentrant Pointer Input](#reentrant-pointer-input)
  - [Pointer Input Reentrancy Tracelogging and Debugging](#pointer-input-reentrancy-tracelogging-and-debugging)
- [See Also](#see-also)

## Background

### ASTA vs STA Threading models
As the [Windows App SDK Threading](https://docs.microsoft.com/en-us/windows/apps/windows-app-sdk/migrate-to-windows-app-sdk/guides/threading#summary-of-api-andor-feature-differences) guide points out:

>*UWP's threading model is ASTA, which blocks certain categories of reentrancy bugs and deadlocks.
The Windows App SDK's threading model is STA, and it doesn't have the same assurances around preventing reentrancy issues.*

To minimize risk, WinUI began by preventing any reentrancy, following the Silverlight model.  As needs arise,
constraints can then be gradually lifted and certain calls hardened so that reentrancy is permitted.
For example, the UIElement_Shown and UIElement_Hidden events are safe to invoke reentrantly.

### Lifted DispatcherQueue allows messages to pump re-entrantly
Both System and Lifted Xaml use CoreMessaging to send messages internally.  `CXcpDispatcher` uses a `DispatcherQueueTimer` to send
and receive messages; CoreMessaging.dll (and CoreMessagingXP.dll for lifted) implement `DispatcherQueue` and `DispatcherQueueTimer`.
CoreMessaging hooks into the win32 message pump, so anytime an app is calling GetMessage/PeekMessage, that could be a time when
CoreMessaging dispatches a message.

For System Xaml scenarios, System CoreMessaging is configured to **not** send messages re-entrantly.  That means in
normal System Xaml sceanrios you won't see a single stack where System CoreMessaging is sending two different messages.
Exception: if Lifted CoreMessagingXP is in the process, it will configure System CoreMessaging to also allow re-entrant
messages.  System Xaml was developed in an environment where it would never receive re-entrant messages from
CoreMessaging, so there may be places in Lifted Xaml we need to revisit/undo assumptions around this. (note even when
System CoreMessaging doesn't allow re-entrancy, there could be other kinds of re-entrancy, just none where CoreMessaging
is firing multiple messages).

For Lifted Xaml sceanrios, Lifted CoreMessagingXP.dll **MAY** send messaging re-entrantly.  This means that whenever an app runs
a nested message pump, Xaml could get a message from `DispatcherQueue`/`DispatcherQueueTimer` during that time.

## Nested Message Pumps
It is relatively common for win32 apps on an STA thread to make calls that pump messages while waiting for an expensive operation
to complete -- and they might not realize they're doing it.  For example:
* Modal dialogs, including `::MessageBox`, will run a nested message pump while the modal UI is active.
* RPC and remote COM calls may run a nested message pump while waiting for an operation to complete.
* Other expensive/complex functions, like `ShellExecute` for example, may run a nested message pump too.

### EnableReentrancyChecks
Xaml has an optional setting called `EnableReentrancyChecks` that will detect cases where the app is running a message
pump while Xaml is firing a re-entrancy-protected message:
``` cmd
rem Run from an admin cmd prompt to enable re-entrancy checks (machine-wide).
rem (The /reg:32 and 64 options ensure the value is set for both x64 processes and wow64/32-bit processes.)
reg add HKLM\Software\Microsoft\WinUI\Xaml /v EnableReentrancyChecks /t REG_DWORD /d 1 /reg:32
reg add HKLM\Software\Microsoft\WinUI\Xaml /v EnableReentrancyChecks /t REG_DWORD /d 1 /reg:64
rem To disable, set it back to 0 or delete the value.
rem The next time microsoft.ui.xaml.dll is loaded, it will have the updated value.

### EnableReentrancyChecksAllowPaused
Xaml has an optional setting called `EnableReentrancyChecksAllowPaused` that will detect cases where the app is running a message
pump while Xaml is firing a re-entrancy-protected message. This is different from `EnableReentrancyChecks`. In this case, 
if the XAML dispatch is paused, the process can continue without creating the reentrancy guard. Otherwise, the reentrancy checks are enabled.
``` cmd
rem Run from an admin cmd prompt to enable re-entrancy checks (machine-wide).
rem (The /reg:32 and 64 options ensure the value is set for both x64 processes and wow64/32-bit processes.)
reg add HKLM\Software\Microsoft\WinUI\Xaml /v EnableReentrancyChecksAllowPaused /t REG_DWORD /d 1 /reg:32
reg add HKLM\Software\Microsoft\WinUI\Xaml /v EnableReentrancyChecksAllowPaused /t REG_DWORD /d 1 /reg:64
rem To disable, set it back to 0 or delete the value.
rem The next time microsoft.ui.xaml.dll is loaded, it will have the updated value.
```

### Example of a Xaml failfast due to app running a nested message pump
Here's a stack in ths islands sample app where the app subscribes to the `UIElement.Loaded` event, and in the handler
of that event, it calls `ShellExecute`.  `ShellExecute` pumps messages, which trips the re-entrancy check
(EnableReentrancyChecks was ON).  This fails fast to show that a nested pump is running under a frame that's
re-entrancy protected (frame 41 below):

```
00 Microsoft_UI_Xaml!CReentrancyGuard::CheckReentrancy      <-- Failfast here b/c frame 41 enabled Reentrancy Guard
01 Microsoft_UI_Xaml!CXcpDispatcher::CreateReentrancyGuardAndCheckReentrancy
02 Microsoft_UI_Xaml!`CXcpDispatcher::QueueReentrancyCheck'::...::<lambda_1>... <-- Because EnableReentrancyChecks is on
...
04 CoreMessagingXP!Microsoft::UI::Dispatching::DispatcherQueue::DeferInvokeCallback
...
16 CoreMessagingXP!Microsoft::CoreUI::Dispatch::UserAdapter::WindowProc
17 USER32!UserCallWinProcCheckWow
18 USER32!DispatchClientMessage
19 USER32!__fnDWORD
...
1c USER32!_PeekMessage
1d USER32!PeekMessageW										<-- ShellExecute pumping messages with PeekMessage
1e SHELL32!SHProcessMessagesUntilEventsEx
1f SHELL32!SHProcessMessagesUntilEvents
20 SHELL32!CShellExecute::_RunThreadMaybeWait
21 SHELL32!CShellExecute::ExecuteNormal
22 SHELL32!ShellExecuteNormal
23 SHELL32!ShellExecuteExW
24 SHELL32!ShellExecuteW
25 WinUICppIslandsSampleApp!<lambda_0f97371552529a424546c6bd0f78dac2>::operator()<IInspectable,RoutedEventArgs>
26 WinUICppIslandsSampleApp!winrt::impl::delegate<...>::Invoke
27 Microsoft_UI_Xaml!DirectUI::CRoutedEventSourceBase<...>::Raise
28 Microsoft_UI_Xaml!DirectUI::CRoutedEventSourceBase<...>::UntypedRaise
29 Microsoft_UI_Xaml!DirectUI::DependencyObject::FireEvent
2a Microsoft_UI_Xaml!DirectUI::DXamlCore::FireEvent
2b Microsoft_UI_Xaml!AgCoreCallbacks::FireEvent
2c Microsoft_UI_Xaml!FxCallbacks::JoltHelper_FireEvent
2d Microsoft_UI_Xaml!CCoreServices::CLR_FireEvent
2e Microsoft_UI_Xaml!CommonBrowserHost::CLR_FireEvent
2f Microsoft_UI_Xaml!CControlBase::ScriptCallback
30 Microsoft_UI_Xaml!CXcpDispatcher::OnScriptCallback
31 Microsoft_UI_Xaml!CXcpDispatcher::OnWindowMessage		<-- UIElement.Loaded is firing (not protected)
32 Microsoft_UI_Xaml!CXcpDispatcher::ProcessMessage
33 Microsoft_UI_Xaml!CXcpDispatcher::WindowProc
34 USER32!UserCallWinProcCheckWow
35 USER32!SendMessageWorker
36 USER32!SendMessageW
37 Microsoft_UI_Xaml!CXcpDispatcher::SendMessageW
38 Microsoft_UI_Xaml!CXcpBrowserHost::SyncScriptCallbackRequest
39 Microsoft_UI_Xaml!CEventManager::RaiseHelper
3a Microsoft_UI_Xaml!CEventManager::RaiseLoadedEventForObject
3b Microsoft_UI_Xaml!CEventManager::RaiseLoadedEvent
3c Microsoft_UI_Xaml!CCoreServices::NWDrawTree
3d Microsoft_UI_Xaml!CCoreServices::NWDrawMainTree
3e Microsoft_UI_Xaml!CWindowRenderTarget::Draw
3f Microsoft_UI_Xaml!CXcpBrowserHost::OnTick
40 Microsoft_UI_Xaml!CXcpDispatcher::Tick
41 Microsoft_UI_Xaml!CXcpDispatcher::OnReentrancyProtectedWindowMessage		<-- Tick is firing, which is protected
42 Microsoft_UI_Xaml!CXcpDispatcher::ProcessMessage
43 Microsoft_UI_Xaml!CXcpDispatcher::WindowProc
44 Microsoft_UI_Xaml!CDeferredInvoke::DispatchQueuedMessage
45 Microsoft_UI_Xaml!CXcpDispatcher::MessageTimerCallback
46 Microsoft_UI_Xaml!CXcpDispatcher::MessageTimerCallbackStatic
47 Microsoft_UI_Xaml!`CXcpDispatcher::Init'::`67'::<lambda_1>::operator()
...
49 CoreMessagingXP!Microsoft::WRL::Details::CreateAgileHelper::__l2::<lambda_82cf8073f4f042d1a68771c460cb9f49>::operator()
...
60 CoreMessagingXP!Microsoft::CoreUI::Dispatch::UserAdapter::WindowProc
...
66 USER32!GetMessageW
67 WinUICppIslandsSampleApp!DesktopWindow::MessageLoop
...
...
```

## Reentrancy-protected messages
Xaml sends messages to itself to route various internal and external events.  Some of these messages are
*reentrancy-protected*. Only one reentrancy-protected message is allowed to be processed at a time -- if there's already
a reentrancy-protected message being processed on the stack, and Xaml needs to send another one, Xaml fails fast at that
point.
``` cpp
// In CXcpDispatcher::ProcessMessage
        // Messages that are reentrancy protected
        case WM_INTERNAL_TICK:
        case WM_APPLICATION_STARTUP_EVENT_COMPLETE:
        case WM_SCRIPT_SYNC_CALL_BACK:
        case WM_EXECUTE_ON_UI_CALLBACK:
        {
            lRet = OnReentrancyProtectedWindowMessage(
                            hwnd, msg, wParam, lParam);
            bDoDefault = FALSE;
            break;
        }

        // Messages that are not reentrancy protected
        case WM_XCP_ERROR_MSG:
        case WM_SCRIPT_CALL_BACK:
        case WM_SCRIPT_SYNC_INPUT_CALL_BACK:
        case WM_SCRIPT_SYNC_LOADED_CALL_BACK:
        case WM_SCRIPT_SYNC_ALLOW_REENTRANCY_CALL_BACK:
        case WM_DOWNLOAD_ASYNC_REQUESTS:
        case WM_FORCE_GC_COLLECT_EVENT:
        case WM_MINVER_NOTIFICATION:
        case WM_EXECUTE_ON_UI_CALLBACK_ALLOW_REENTRANCY:
        case WM_SCRIPT_SYNC_EFFECTIVEVIEWPORTCHANGED:
        case WM_REPLAY_PREVIOUS_POINTERUPDATE:
        case WM_SYNC_CHANGING_FOCUS_CALLBACK:
        {
            lRet = OnWindowMessage(hwnd, msg, wParam, lParam);
            bDoDefault = FALSE;
            break;
        }
```
So, this is OK because `WM_SCRIPT_SYNC_LOADED_CALL_BACK` is not protected:
```
Xaml sends WM_INTERNAL_TICK -> Xaml Ticks, runs layout -> Xaml fires UIElement.Loaded WM_SCRIPT_SYNC_LOADED_CALL_BACK
```
This will failfast because `WM_INTERNAL_TICK` and `WM_EXECUTE_ON_UI_CALLBACK` are both protected:
```
Xaml sends WM_INTERNAL_TICK -> ... -> App gets Loaded event, runs nested message pump -> WM_EXECUTE_ON_UI_CALLBACK
```

## CReentrancyGuard

This RAII class is used to detect and prevent unsupported reentrant execution. CReentrancyGuard is expected to be used
on a thread-local variable, either directly or indirectly.  Since m_bMessageReentrancyGuard is a member of a struct that
is unique per thread (CXcpDispatcher), CReentrancyGuard behavior is threadsafe in that scenario.

If concern does arise around misuse of CReentrancyGuard, tracepoints can be used in VS to detect multithreaded use:
```
CReentrancyGuard::CReentrancyGuard(): $TID, $CALLER
CReentrancyGuard::~CReentrancyGuard(): $TID, $CALLER
```

### Usage

CReentrancyGuard is instantiated in message dispatching logic that is expected not to recurse:
```
CXcpDispatcher::OnReentrancyProtectedWindowMessage(...)
{
    // Assert that m_bMessageReentrancyGuard is FALSE, before setting it to TRUE for this call.
    // Otherwise, we've detected reentrancy and the Xaml runtime will failfast the app.
    CReentrancyGuard reentrancyGuard(&m_bMessageReentrancyGuard);
    reentrancyGuard.CheckReentrancy(...);
}
```

### Manifestation

Reentrancy can result directly from application code accessing a property in an event handler. But it can also manifest
when visualizing a property in the debugger, for both C#/WinRT and C++/WinRT apps. This causes the debugger to invoke a
COM STA call back into the process to retrieve the property value. Many issues reported on the WinUI repo involve this
scenario.

It's also important to note that reentrancy itself is not deterministic, and failfasts may be intermittent.
For example, servicing a timer tick while an event handler calls back into Xaml will cause CReentrancyGuard to failfast.
But it's also a chance event that may take several attempts to repro.

### Example

In [issue 7231](https://github.com/microsoft/microsoft-ui-xaml/issues/7231), a DragOver event handler calls back into
Xaml to retrieve the event arg's DataView property, and while doing so pumps a timer tick message. The DragDrop operation
is initiated on a System COM call into WinUI's ICoreDropOperationTarget implementation. And the timer tick is also a COM
call. Both are serviced by OnReentrancyProtectedWindowMessage, so it is not permitted to nest one inside the other.

A sample stack (WRL frames omitted) demonstrating reentrancy detection leading to a failfast:

```
9.	Microsoft.ui.xaml.dll!CReentrancyGuard::CheckReentrancy
8. 	Microsoft.ui.xaml.dll!CXcpDispatcher::OnReentrancyProtectedWindowMessage
     Microsoft.ui.xaml.dll!CXcpDispatcher::ProcessMessage
     Microsoft.ui.xaml.dll!CXcpDispatcher::WindowProc
     Microsoft.ui.xaml.dll!CDeferredInvoke::DispatchQueuedMessage
     Microsoft.ui.xaml.dll!CXcpDispatcher::MessageTimerCallback
     Microsoft.ui.xaml.dll!CXcpDispatcher::MessageTimerCallbackStatic
     ...
     CoreMessagingXP.dll!CFlat::DelegateImpl<Microsoft::CoreUI::Dispatch::TimeoutHandler,0,void __stdcall
     CoreMessagingXP.dll!Microsoft::CoreUI::Dispatch::Timeout::CallHandlerWithActivationContext
     CoreMessagingXP.dll!Microsoft::CoreUI::Support::ActivationContext::CallbackWithActivationContext
7. 	CoreMessagingXP.dll!Microsoft::CoreUI::Dispatch::Timeout::CallHandler
     CoreMessagingXP.dll!Microsoft::CoreUI::Dispatch::TimeoutManager::Callback_OnDispatch
     CoreMessagingXP.dll!Microsoft::CoreUI::Dispatch::Dispatcher::Callback_DispatchNextItem
     CoreMessagingXP.dll!Microsoft::CoreUI::Dispatch::Dispatcher::Callback_DispatchLoop
     CoreMessagingXP.dll!Microsoft::CoreUI::Dispatch::EventLoop::Callback_RunCoreLoop
     CoreMessagingXP.dll!Microsoft::CoreUI::Dispatch::UserAdapter::DrainCoreMessagingQueue
     CoreMessagingXP.dll!Microsoft::CoreUI::Dispatch::UserAdapter::OnUserDispatch
     CoreMessagingXP.dll!Microsoft::CoreUI::Dispatch::UserAdapter::OnUserDispatchRaw
     CoreMessagingXP.dll!Microsoft::CoreUI::Dispatch::UserAdapter::DoWork
     CoreMessagingXP.dll!Microsoft::CoreUI::Dispatch::UserAdapter::HandleDispatchNotifyMessage
     CoreMessagingXP.dll!Microsoft::CoreUI::Dispatch::UserAdapter::WindowProc
     user32.dll!__InternalCallWinProc@20
     user32.dll!InternalCallWinProc
     user32.dll!UserCallWinProcCheckWow
     user32.dll!DispatchClientMessage
     user32.dll!__fnDWORD
     ...
     user32.dll!PeekMessageW
     combase.dll!CCliModalLoop::MyPeekMessage
     combase.dll!CCliModalLoop::PeekRPCAndDDEMessage
     combase.dll!CCliModalLoop::BlockFn
     combase.dll!ModalLoop
6. 	combase.dll!ClassicSTAThreadWaitForCall
     combase.dll!ThreadSendReceive
     combase.dll!CSyncClientCall::SwitchAptAndDispatchCall
     combase.dll!CSyncClientCall::SendReceive2
     combase.dll!SyncClientCallRetryContext::SendReceiveWithRetry
     combase.dll!CSyncClientCall::SendReceiveInRetryContext
     combase.dll!ClassicSTAThreadSendReceive
     combase.dll!CSyncClientCall::SendReceive
     combase.dll!CClientChannel::SendReceive
     combase.dll!NdrExtpProxySendReceive
     rpcrt4.dll!NdrpProxySendReceive
     rpcrt4.dll!NdrClientCall2
     combase.dll!ObjectStublessClient
     combase.dll!_ObjectStubless@0
     DataExchange.dll!DropTargetInternal::DragInfo::get_Data
     Microsoft.ui.xaml.dll!DirectUI::DragEventArgs::get_DataViewImpl
     Microsoft.ui.xaml.dll!DirectUI::DragEventArgsGenerated::get_DataView
     [Managed to Native Transition]
     Microsoft.WinUI.dll!ABI.Microsoft.UI.Xaml.IDragEventArgsMethods.get_DataView
5. 	Microsoft.WinUI.dll!Microsoft.UI.Xaml.DragEventArgs.DataView.get
4. 	WinUIcalendarDemos.dll!WinUIcalendarDemos.CustomStackPanel.Cell_DragOver
     Microsoft.WinUI.dll!WinRT._EventSource_global__Microsoft_UI_Xaml_DragEventHandler.EventState.GetEventInvoke.AnonymousMethod__1_0
     Microsoft.WinUI.dll!ABI.Microsoft.UI.Xaml.DragEventHandler.Do_Abi_Invoke
     [Native to Managed Transition]
     Microsoft.ui.xaml.dll!DirectUI::CRoutedEventSourceBase<...ABI::Microsoft::UI::Xaml::IDragEventArgs>::Raise
     Microsoft.ui.xaml.dll!DirectUI::CRoutedEventSourceBase<...ABI::Microsoft::UI::Xaml::IDragEventArgs>::UntypedRaise
     Microsoft.ui.xaml.dll!DirectUI::DependencyObject::FireEvent
     Microsoft.ui.xaml.dll!DirectUI::DXamlCore::FireEvent
     Microsoft.ui.xaml.dll!AgCoreCallbacks::FireEvent
     Microsoft.ui.xaml.dll!FxCallbacks::JoltHelper_FireEvent
     Microsoft.ui.xaml.dll!CCoreServices::CLR_FireEvent
     Microsoft.ui.xaml.dll!CommonBrowserHost::CLR_FireEvent
     Microsoft.ui.xaml.dll!CControlBase::ScriptCallback
     Microsoft.ui.xaml.dll!CXcpDispatcher::OnScriptCallback
3. 	Microsoft.ui.xaml.dll!CXcpDispatcher::OnReentrancyProtectedWindowMessage
     Microsoft.ui.xaml.dll!CXcpDispatcher::ProcessMessage
     Microsoft.ui.xaml.dll!CXcpDispatcher::WindowProc
     user32.dll!__InternalCallWinProc@20
     user32.dll!InternalCallWinProc
     user32.dll!UserCallWinProcCheckWow
     user32.dll!SendMessageWorker
     user32.dll!SendMessageInternal
     user32.dll!SendMessageW
     Microsoft.ui.xaml.dll!CXcpDispatcher::SendMessageW
2. 	Microsoft.ui.xaml.dll!CXcpBrowserHost::SyncScriptCallbackRequest
     Microsoft.ui.xaml.dll!CEventManager::RaiseHelper
     Microsoft.ui.xaml.dll!CEventManager::Raise
     Microsoft.ui.xaml.dll!CDragDropState::RaiseDragOverOrDropEvents
     Microsoft.ui.xaml.dll!CDragDropState::RaiseEvents
     Microsoft.ui.xaml.dll!ContentRootInput::DragDropProcessor::ProcessWinRtDragDrop
     Microsoft.ui.xaml.dll!CoreImports::DragDrop_RaiseEvent
     Microsoft.ui.xaml.dll!DirectUI::DropOperationTarget::RaiseDragDropEventActionAsync
     Microsoft.ui.xaml.dll!DirectUI::RaiseDragDropEventAsyncOperation::OnStart
    ...
     Microsoft.ui.xaml.dll!DirectUI::DropOperationTarget::RaiseDragDropEventOperationAsync
     Microsoft.ui.xaml.dll!DirectUI::DropOperationTarget::OverAsync
1. 	DataExchange.dll!DropTargetInternal::DragOver
```

Flow:
1. The system initiates a drag-drop DragOver event, dispatching into WinUI's DropOperationTarget implementation
2. WinUI routes the DragOver to the application's event handler via a synchronous SendMessage
3. While processing the message, WinUI instantiates a CReentrancyGuard to detect subsequent reentrance
4. The application's Cell_DragOver event handler is invoked
5. The event handler (or a debug visualizer) retrieves the DragEventArgs.DataView property
6. WinUI implements the incoming callback to a COM STA dispatch
7. While pumping messages, a timer tick elapses and is processed
8. A nested CReentrancyGuard is instantiated
9. The reentrancy check fails, causing a failfast crash

In this example, the propagation of ICoreDropOperationTarget methods using the STA SendMessage machinery has
the side effect of inserting a reentrancy guard with the call to OnReentrancyProtectedWindowMessage. Any calls back
into Xaml from the user handler run the risk of invoking another STA call, tripping the reentrancy guard check and
causing a failfast.

Observing the stack, all frames from the system's DataExchange call to the user's handler are simply propagating the
event along synchronously and without changing much (if any) Xaml state. The call to OnReentrancyProtectedWindowMessage
ends up being an accidental implementation detail that's fatal to user event handlers.

The solution is to exempt the DragDrop events from reentrancy checking by implementing them with
WM_SCRIPT_SYNC_ALLOW_REENTRANCY_CALL_BACK rather than WM_SCRIPT_SYNC_CALL_BACK. Note that using WM_SCRIPT_SYNC_ALLOW_REENTRANCY_CALL_BACK does not imply that the ICoreDropOperationTarget implementation is
reentrant - only that it doesn't need to be guarded.

## Reentrancy Checks

There are scenarios where Xaml needs to call out to the app during a reentrancy-protected message, and we're counting on
the app not doing anything that can pump messages and cause reentrancy. If the app _is_ pumping messages, and there
happens to be a CoreMessaging message in the queue that triggers a Xaml tick, then that message will be dispatched, Xaml
will try to tick, and we'll crash due to reentrancy during a reentrancy-protected message. However, this requires on
lucky timing and isn't guaranteed. In practice this produces crash dumps that aren't actionable because the repro is
very inconsistent.

To solve this problem and make the crashes consistent and reproducable, Xaml has the `QueueReentrancyCheck` method.
Before we process a reentrancy-protected message, we enqueue a task with high priority in the CoreMessaging dispatcher
queue. This guarantees that if anybody pumps the message, this task will get dispatched. Its high priority ensures that
it's at the head of the queue and is dispatched first, ahead of any input messages. If this task gets processed while
Xaml is still processing the reentrancy-protected message, we'll know the app pumped messages and we can fail fast.

## Avoiding reentrancy

In addition to detecting reentrancy and crashing, Xaml also has the ability to avoid reentrancy altogether. Xaml runs
code on its UI thread because of the CoreMessaging timer, so by delaying CoreMessaging processing we can prevent
anything new from running on the UI thread. This is done via the `IMessageLoopExtensions::PauseNewDispatch` (and
`ResumeDispatch`) method, and Xaml's `PauseNewDispatch` is an RAII object that wraps these calls to turn off
CoreMessaging dispatching anything.

A couple of places in Xaml currently make use of `PauseNewDispatch`:
1. `WM_SCRIPT_SYNC_CALL_BACK` messages. One such event that uses these is the `UIElement.ContextRequested` event.

   This was added to address Taskbar crashes from Xaml reentrancy.

2. Imaging tasks. These tasks are queued with `CXcpBrowserHost::FireExecuteOnUIThreadMessage` while specifying
   `ReentrancyBehavior::BlockReentrancy`. Other tasks that queue work on the UI thread via
   `FireExecuteOnUIThreadMessage` can use this mechanism as well.

## Reentrant Pointer Input

In Lifted Xaml, if an app runs a nested message loop it's possible for Xaml to receive re-entrant pointer messages.

Here's a stack in the islands sample app where we see two pointer-up/PointerPressed firing -- one on the outer message
pump and one on a nested/inner message pump:
```
03 WinUICppIslandsSampleApp!winrt::impl::delegate<PointerEventHandler,<lambda_e32ade7a43e6f023f17fc18dca4b20a9> >::Invoke
04 Microsoft_UI_Xaml!DirectUI::CRoutedEventSourceBase<...>::Raise
05 Microsoft_UI_Xaml!DirectUI::CRoutedEventSourceBase<...>::UntypedRaise
06 Microsoft_UI_Xaml!DirectUI::DependencyObject::FireEvent
07 Microsoft_UI_Xaml!DirectUI::DXamlCore::FireEvent
08 Microsoft_UI_Xaml!AgCoreCallbacks::FireEvent
09 Microsoft_UI_Xaml!FxCallbacks::JoltHelper_FireEvent
0a Microsoft_UI_Xaml!CCoreServices::CLR_FireEvent
0b Microsoft_UI_Xaml!CommonBrowserHost::CLR_FireEvent
0c Microsoft_UI_Xaml!CControlBase::ScriptCallback
0d Microsoft_UI_Xaml!CXcpDispatcher::OnScriptCallback
0e Microsoft_UI_Xaml!CXcpDispatcher::OnWindowMessage
0f Microsoft_UI_Xaml!CXcpDispatcher::ProcessMessage
10 Microsoft_UI_Xaml!CXcpDispatcher::WindowProc
11 USER32!UserCallWinProcCheckWow
12 USER32!SendMessageWorker
13 USER32!SendMessageW
14 Microsoft_UI_Xaml!CXcpDispatcher::SendMessageW
15 Microsoft_UI_Xaml!CXcpBrowserHost::SyncScriptCallbackRequest
16 Microsoft_UI_Xaml!CEventManager::RaiseHelper
17 Microsoft_UI_Xaml!CEventManager::Raise
18 Microsoft_UI_Xaml!CEventManager::RaiseRoutedEventBubbling
19 Microsoft_UI_Xaml!CEventManager::RaiseRoutedEvent
1a Microsoft_UI_Xaml!ContentRootInput::PointerInputProcessor::ProcessPointerInput
1b Microsoft_UI_Xaml!CInputServices::ProcessInput
1c Microsoft_UI_Xaml!CCoreServices::ProcessInput
1d Microsoft_UI_Xaml!CXcpBrowserHost::HandleInputMessage
1e Microsoft_UI_Xaml!CJupiterControl::HandlePointerMessage
1f Microsoft_UI_Xaml!CJupiterWindow::OnIslandPointerMessage
20 Microsoft_UI_Xaml!CXamlIslandRoot::InjectPointerMessage
21 Microsoft_UI_Xaml!CXamlIslandRoot::OnIslandPointerPressed        <-- Xaml receives PointerPressed from InputPointerSource
22 Microsoft_UI_Xaml!`CXamlIslandRoot::SubscribeToInputPointerSourceEvents'::`35'::<lambda_5>::operator()
84 Microsoft_UI_Xaml!Microsoft::WRL::Details::DelegateArgTraits<...>::Invoke
24 Microsoft_UI_Input!Microsoft::WRL::Details::CreateAgileHelper::__l2::<lambda_1>::operator()
...
3c Microsoft_UI_Input!Microsoft::BamoImpl::ConnectionIndirector::OnItemMessage
3d CoreMessagingXP!Microsoft::CoreUI::ICallbackMessageConversationHost::ImportAdapter$::OnItemMessage::__l2::<lambda...>::operator()
...
5c CoreMessagingXP!Microsoft::CoreUI::Dispatch::UserAdapter::WindowProc
5d USER32!UserCallWinProcCheckWow
5e USER32!DispatchClientMessage
5f USER32!__fnDWORD
...
62 USER32!GetMessageW                                       <-- App runs a nested message pump
63 WinUICppIslandsSampleApp!<lambda...>::operator()<IInspectable,PointerRoutedEventArgs>
64 WinUICppIslandsSampleApp!winrt::impl::delegate<PointerEventHandler,<lambda...> >::Invoke
65 Microsoft_UI_Xaml!DirectUI::CRoutedEventSourceBase<...>::Raise
66 Microsoft_UI_Xaml!DirectUI::CRoutedEventSourceBase<...>::UntypedRaise
67 Microsoft_UI_Xaml!DirectUI::DependencyObject::FireEvent
68 Microsoft_UI_Xaml!DirectUI::DXamlCore::FireEvent
69 Microsoft_UI_Xaml!AgCoreCallbacks::FireEvent
6a Microsoft_UI_Xaml!FxCallbacks::JoltHelper_FireEvent
6b Microsoft_UI_Xaml!CCoreServices::CLR_FireEvent
6c Microsoft_UI_Xaml!CommonBrowserHost::CLR_FireEvent
6d Microsoft_UI_Xaml!CControlBase::ScriptCallback
6e Microsoft_UI_Xaml!CXcpDispatcher::OnScriptCallback
6f Microsoft_UI_Xaml!CXcpDispatcher::OnWindowMessage
70 Microsoft_UI_Xaml!CXcpDispatcher::ProcessMessage
71 Microsoft_UI_Xaml!CXcpDispatcher::WindowProc
72 USER32!UserCallWinProcCheckWow
73 USER32!SendMessageWorker
74 USER32!SendMessageW
75 Microsoft_UI_Xaml!CXcpDispatcher::SendMessageW
76 Microsoft_UI_Xaml!CXcpBrowserHost::SyncScriptCallbackRequest
77 Microsoft_UI_Xaml!CEventManager::RaiseHelper
78 Microsoft_UI_Xaml!CEventManager::Raise
79 Microsoft_UI_Xaml!CEventManager::RaiseRoutedEventBubbling
7a Microsoft_UI_Xaml!CEventManager::RaiseRoutedEvent
7b Microsoft_UI_Xaml!ContentRootInput::PointerInputProcessor::ProcessPointerInput
7c Microsoft_UI_Xaml!CInputServices::ProcessInput
7d Microsoft_UI_Xaml!CCoreServices::ProcessInput
7e Microsoft_UI_Xaml!CXcpBrowserHost::HandleInputMessage
7f Microsoft_UI_Xaml!CJupiterControl::HandlePointerMessage
80 Microsoft_UI_Xaml!CJupiterWindow::OnIslandPointerMessage
81 Microsoft_UI_Xaml!CXamlIslandRoot::InjectPointerMessage
82 Microsoft_UI_Xaml!CXamlIslandRoot::OnIslandPointerPressed    <-- Xaml receives PointerPressed from InputPointerSource
83 Microsoft_UI_Xaml!`CXamlIslandRoot::SubscribeToInputPointerSourceEvents'::`35'::<lambda_5>::operator()
84 Microsoft_UI_Xaml!Microsoft::WRL::Details::DelegateArgTraits<...>::Invoke
85 Microsoft_UI_Input!Microsoft::WRL::Details::CreateAgileHelper::__l2::<lambda_1>::operator()
...
9d Microsoft_UI_Input!Microsoft::BamoImpl::ConnectionIndirector::OnItemMessage
9e CoreMessagingXP!Microsoft::CoreUI::ICallbackMessageConversationHost::ImportAdapter$::OnItemMessage::__l2::<lambda_de6cc8128f8f9f99144226758963eeb9>::operator()
...
bd CoreMessagingXP!Microsoft::CoreUI::Dispatch::UserAdapter::WindowProc
...
c3 USER32!GetMessageW
c4 WinUICppIslandsSampleApp!DesktopWindow::MessageLoop
...
...
```

We've done some hardening to make sure the functions that are repeated on the above stack (like `ProcessPointerInput`
and `ProcessInput`) don't crash in this situation, but there may be other situations that come up that need to be
hardeded further.

### Pointer Input Reentrancy Tracelogging and Debugging
To detect when pointer re-entrancy is happening in an ETW trace, if you have the XAML provider enabled, you can look for the
`PointerInputReentrancyDetected` event.

When pointer re-entrancy happens, the `InputMessage` that Xaml was in the middle of processing on the outer pump is marked
by setting `m_supersededByLaterMessage` to true.  This may be helpful when inspecting dump files.

## See Also

>*In rare cases it may be appropriate to allow reentrancy of COM calls; this should only be done if the API being used clearly set the expectations of the developer that this reentrancy is possible, DoDragDrop() is an example.*
