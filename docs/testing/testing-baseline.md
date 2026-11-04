# Testing Baselines

## Overview

Rendering tests compare MockDComp XML tree dumps and PNG surfaces with checked-in expected outputs, called masters.
A mismatch fails verification and preserves the available comparison files for review.

## Where baselines are stored

- Source masters live in [test/resources/masters](../../dxaml/test/resources/masters):
  - XML: `*.master.xml`
  - Surfaces: `*.master.png`, with the same test, variation, and surface identifier as the output
- Failed comparisons copy available masters and `*.out.xml` / `*.out.png` files to `XamlTAEFOutput` under the
  Pictures library, typically `%USERPROFILE%\Pictures\XamlTAEFOutput`.
- Packaged tests read masters embedded in `Private.Infrastructure.Resources.dll`. Copying loose master files into an
  existing test payload does not update its comparison inputs.

## Updating baselines

1. Generate output with the current product and the intended test host. Review each difference and accept only changes
   attributable to the intended product or test change.
2. Replace the corresponding source master while preserving its full master filename. For bulk updates, point
   `scripts\UpdateMasterFiles.ps1` at a directory containing the reviewed output/master pairs:

   ```powershell
   .\scripts\UpdateMasterFiles.ps1 -NewMastersDirectory "<reviewed-output-directory>"
   ```

   The script replaces existing source masters; add masters for a new test explicitly. For a final-release baseline,
   pass `-release` so the matching file under `masters\release` is updated.
3. Preserve `$$Name$$` placeholders in XML masters. Tests supply OS-dependent values through
   `SetDCompXmlVariable`, and `VerificationComparer` substitutes them before comparison. A placeholder on its own line
   can represent an optional complete XML line.
4. Rebuild the resource project so packaged tests receive the updated masters. For x64 Debug:

   ```powershell
   .\initrun.ps1 -Flavor amd64chk msbuild dxaml\test\resources\masters\Private.Infrastructure.Resources.vcxproj /p:Configuration=Debug /p:Platform=x64
   ```

   When validating final-release masters, also pass `/p:MUXFinalRelease=true`.

5. Deploy the rebuilt `Private.Infrastructure.Resources.dll` with the matching test payload and rerun the affected
   tests under the same host, architecture, and OS configuration used to generate the outputs.