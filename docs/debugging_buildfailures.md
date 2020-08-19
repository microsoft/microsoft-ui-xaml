# Capturing binlogs
Capturing and providing binlog files can help with debugging build and packaging issues. In order to collect binlogs, please follow these steps:

1. Download the **[VS Project System Tools extension](https://marketplace.visualstudio.com/items?itemName=VisualStudioProductTeam.ProjectSystemTools)**
2. Set the **Build Log File** verbosity to `Diagnostics`: `Tools->Options->Projects and Solutions->MSBuild project build log file verbosity`:<br/>
![Screenshot of Build and run options](./images/binlog-images/buildandrunoptions.png)

3. Go to View->Other Windows->Build Logging:<br/>
![Screenshot of Build Logging menu item](./images/binlog-images/buildlogging_menuitem.png)

4. To start taking logs, press the play button:<br/>
![Screenshot of Build Logging window](./images/binlog-images/buildlogging_window.png)

5. Run the steps that resulted in errors, e.g. building your project. The steps that failed show up as "Failed". Those files have the file extension ".binlog" and can be shared to help debugging build and packaging issues.
