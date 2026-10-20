Param(
    [Parameter(Mandatory = $false)]
    [String] $packageId = "com.beatgames.beatsaber"
)

if (Test-Path "$PSScriptRoot/../package_id.txt") {
    $packageId = Get-Content "$PSScriptRoot/../package_id.txt"
    Write-Output "Using package ID from package_id.txt: $packageId"
}

adb shell am force-stop "$packageId"

adb shell am start "$packageId/com.unity3d.player.UnityPlayerActivity"
