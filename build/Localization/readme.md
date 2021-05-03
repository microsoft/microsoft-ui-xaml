# Localization process

This directory contains everything necessary to process our english-only .resw files and receive back from the localization
service the localized files.

Uploading english-only .resw files to Touchdown for localization:
1. MUX-LocalizationHandoff is a scheduled pipeline that runs regularly. It will automatically upload the lastest english .resw files to Touchdown for localization.

Checking-in the localized .resw files requires some manual steps. The plan is to automate this in the future.
1. When there are newly available localized strings, Touchdown notifies the team via email.
2. Each run of MUX-LocalizationHandoff retrieves the latest localized files and publishes them as a build artifact under 'LocalizationDrop'
3. Download & unzip this artifact (either from a pipeline run that happened after the reciept of the aforementioned email or from a manually scheduled pipeline run).
4. Run the script `build\Localization\CopyBackLocalizedFiles.ps1`. Either place the downloaded folder under `\BuildOutput\` or specify its location using /`-LocalizedFilesLocation`.
5. Check-in the changes to main

## If you're doing this for a new control
Make sure that your Resources.resw file is located in *ControlName*\Strings\en-us because the localization process will create sibling directories for all other languages.