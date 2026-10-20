Param(
    [Parameter(Mandatory = $false)]
    [Switch] $clean,

    [Parameter(Mandatory = $false)]
    [Switch] $skipBuild,

    [Parameter(Mandatory = $false)]
    [Switch] $help
)

if ($help -eq $true) {
    Write-Output "`"Build`" - Copiles your mod into a `".so`" or a `".a`" library"
    Write-Output "`n-- Arguments --`n"

    Write-Output "-Clean `t`t Deletes the `"build`" folder, so that the entire library is rebuilt"
    Write-Output "-SkipBuild `t`t Skips build, combine this with -Clean"

    exit
}

$sharedQpmFilePath = "qpm.shared.json"
$defaultQpmFilePath = "qpm.json"

if (Test-Path $sharedQpmFilePath) {
    $qpmJson = (Get-Content $sharedQpmFilePath | ConvertFrom-Json).config
}
elseif (Test-Path $defaultQpmFilePath) {
    $qpmJson = Get-Content $defaultQpmFilePath | ConvertFrom-Json
}
else {
    Write-Error "Neither qpm.shared.json nor qpm.json exists."
    exit 1
}

if (-not $skipBuild.IsPresent -and (-not (Test-Path -Path $qpmJson.dependenciesDir))) {
    & qpm restore
}

if ($clean.IsPresent) {
    if (Test-Path -Path "build") {
        remove-item build -R -Force
    }

    if (Test-Path $qpmJson.workspace.qmodOutput) {
        Remove-Item $qpmJson.workspace.qmodOutput -Force
    }
}

if ($skipBuild.IsPresent) {
    exit 0
}

if (Test-Path "$PSScriptRoot/../extern/includes/bs-cordl/include/version.txt") {
    $modTemplateRaw = Get-Content "$PSScriptRoot/../mod.template.json" -Raw
    $modTemplateOriginal = $modTemplateRaw | ConvertFrom-Json
    $modTemplate = $modTemplateRaw | ConvertFrom-Json
    $bsversion = Get-Content "$PSScriptRoot/../extern/includes/bs-cordl/include/version.txt"
    if (-not [string]::IsNullOrWhitespace($bsversion)) {
        Write-Output "Setting Package Version to $bsversion"
        $modTemplate.packageVersion = $bsversion

        if (($modTemplate | ConvertTo-Json -Depth 32) -ne ($modTemplateOriginal | ConvertTo-Json -Depth 32)) {
            Write-Output "Writing updated mod.template.json"
            $modTemplate | ConvertTo-Json -Depth 32 | Set-Content -Path "$PSScriptRoot/../mod.template.json"
        }
    }
    else {
        Write-Output "Empty bs-cordl version.txt, skipping package version update."
    }
}
else {
    Write-Output "Missing bs-cordl version.txt, skipping package version update."
}

if (-not (Test-Path -Path "build")) {
    new-item -Path build -ItemType Directory
}

& cmake -G "Ninja" -DCMAKE_BUILD_TYPE="RelWithDebInfo" -B build
& cmake --build ./build
exit $LASTEXITCODE
