Param(
    [string]$Destination
)

$picturesFolder = [Environment]::GetFolderPath('MyPictures')
Copy-Item $picturesFolder\*.xml $Destination