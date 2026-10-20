Param(
    [Parameter(Mandatory = $false)]
    [Switch] $clean,

    [Parameter(Mandatory = $false)]
    [Switch] $log,

    [Parameter(Mandatory = $false)]
    [Switch] $useDebug,

    [Parameter(Mandatory = $false)]
    [Switch] $self,

    [Parameter(Mandatory = $false)]
    [Switch] $all,

    [Parameter(Mandatory = $false)]
    [String] $custom = "",

    [Parameter(Mandatory = $false)]
    [String] $file = "",

    [Parameter(Mandatory = $false)]
    [Switch] $help,

    [Parameter(Mandatory = $false)]
    [String] $packageId = "com.beatgames.beatsaber"
)

if ($help -eq $true) {
    Write-Output "`"Copy`" - Builds and copies your mod to your quest, and also starts Beat Saber with optional logging"
    Write-Output "`n-- Arguments --`n"

    Write-Output "-Clean `t`t Performs a clean build (equvilant to running `"build -clean`")"
    Write-Output "-UseDebug `t Copies the debug version of the mod to your quest"
    Write-Output "-Log `t`t Logs Beat Saber using the `"Start-Logging`" command"
    Write-Output "-packageId `t Specifies the package name of the game (default: com.beatgames.beatsaber)"

    Write-Output "`n-- Logging Arguments --`n"

    & $PSScriptRoot/start-logging.ps1 -help -excludeHeader

    exit
}

if (Test-Path "$PSScriptRoot/../package_id.txt") {
    $packageId = Get-Content "$PSScriptRoot/../package_id.txt"
    Write-Output "Using package ID from package_id.txt: $packageId"
}

& $PSScriptRoot/build.ps1 -clean:$clean

if ($LASTEXITCODE -ne 0) {
    Write-Output "Failed to build, exiting..."
    exit $LASTEXITCODE
}

& $PSScriptRoot/validate-modjson.ps1
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
$modJson = Get-Content "./mod.json" -Raw | ConvertFrom-Json

foreach ($fileName in $modJson.modFiles) {
    if ($useDebug -eq $true) {
        & adb push "build/debug/$fileName" "/sdcard/ModData/$packageId/Modloader/early_mods/$fileName"
    }
    else {
        & adb push "build/$fileName" "/sdcard/ModData/$packageId/Modloader/early_mods/$fileName"
    }
}

foreach ($fileName in $modJson.lateModFiles) {
    if ($useDebug -eq $true) {
        & adb push "build/debug/$fileName" "/sdcard/ModData/$packageId/Modloader/mods/$fileName"
    }
    else {
        & adb push "build/$fileName" "/sdcard/ModData/$packageId/Modloader/mods/$fileName"
    }
}

& $PSScriptRoot/restart-game.ps1 "-packageId:$packageId"

if ($log -eq $true) {
    & adb logcat -c
    & $PSScriptRoot/start-logging.ps1 -self:$self -all:$all -custom:$custom -file:$file "-packageId:$packageId"
}
