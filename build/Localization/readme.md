# Localization process

This doc is now located in the OneNote (I'm not deleting as I go because it's more... remote)

This directory contains everything necessary to process our english-only .resw files and receive back from the localization
service the localized files.

The localization process is asynchronous so what happens is:
1. Run devcmd.cmd
2. For now we will run the RunLocWorkflow script manually. In the future we will have a daily scheduled run to submit our strings for localization. 
3. The script also retrieves from the localization service the localized (or partially localized) .resw files. These files
are staged in the source tree but when run on the CI machine, they're then abandoned.
4. We will manually email egutierr;wdglocengsup and request them to process our localization request (this will eventually
happen automatically but for now we need to manually request).
    ```
    For this reach project http://tdbuild:40/Home/ConfigSettings?TeamID=104

    Please let us know when it's complete.
    ```
Once all the strings are localized on the service then someone needs to run the loc workflow locally and submit the 
strings to master. 

Run the workflow locally with a string that uniquely identifies the version of the strings. Today's date usually works well:
```
RunLocWorkflow.cmd 20161202-manual
```

## If you're doing this for a new control
Add a new section to [..\build\Localization\Settings\LocConfig.xml](..\build\Localization\Settings\LocConfig.xml) for your control.

```
    <File
      location="MyWidgetControl"
      path="%LocRoot%\..\..\dev\MyWidgetControl\Strings\en-us\Resources.resw"/>
```

Make sure that your Resources.resw file is located in Strings\en-us because the localization process will create sibling directories for all other languages.