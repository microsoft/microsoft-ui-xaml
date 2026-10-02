# Progress Styles Tables
These tables provides a comparison of current progress styles for the ProgressBar and ProgressRing controls in different modes and states.

## ProgressBar
ProgressBar has IsIndeterminate, Value, ShowPaused, and ShowError properties. 

### ProgressBar (Indeterminate)
IsIndeterminate is True

| ShowPaused | ShowError | Value | Image |
|:--| :-:|:-:|:-:| 
| False | False |    | ![](images/ProgressBar-indeterminate-active.PNG) |
| False | False | 85 | ![](images/ProgressBar-indeterminate-value-active.PNG) |
| False | True  |    | ![](images/ProgressBar-indeterminate-error.PNG) |
| False | True  | 85 | ![](images/ProgressBar-indeterminate-value-error.PNG) |
| True  | False |    | ![](images/ProgressBar-indeterminate-paused.PNG) |
| True  | False | 85 | ![](images/ProgressBar-indeterminate-value-paused.PNG) |
| True  | True  |    | ![](images/ProgressBar-indeterminate-paused-error.PNG) |
| True  | True  | 85 | ![](images/ProgressBar-indeterminate-value-paused-error.PNG) |

### ProgressBar (Determinate)
IsIndeterminate is False

| ShowPaused | ShowError | Value | Image |
|:--| :-:|:-:|:-:| 
| False | False |    | ![](images/ProgressBar-determinate-no-value.PNG) |
| False | False | 0  | ![](images/ProgressBar-determinate-no-value.PNG) |
| False | False | 85 | ![](images/ProgressBar-determinate-active.PNG) |
| False | True  |    | ![](images/ProgressBar-determinate-no-value.PNG) |
| False | True  | 85 | ![](images/ProgressBar-determinate-error.PNG) |
| True  | False |    | ![](images/ProgressBar-determinate-no-value.PNG) |
| True  | False | 85 | ![](images/ProgressBar-determinate-paused.PNG) |
| True  | True  |    | ![](images/ProgressBar-determinate-no-value.PNG) |
| True  | True  | 85 | ![](images/ProgressBar-determinate-paused-error.PNG) |

### ProgressBar Minimum and Maximum
ProgressBar indicates progress based on the Value, relative to the Minimum and Maximum.

| Properties | Image | 
|:--| :-:|
| Minimum=10 Maximum=20 Value=18.5| ![](images/ProgressBar-determinate-active.PNG) |

## ProgressRing
ProgressRing has IsActive property, and does not currently have IsIndeterminate, Value, ShowPaused, or ShowError properties.
### ProgressRing (Indeterminate)
| IsActive | Image |
|:--| :-:|
| False | ![](images/ProgressRing-indeterminate-not-active.PNG) |
| True | ![](images/ProgressRing-indeterminate.PNG) |





