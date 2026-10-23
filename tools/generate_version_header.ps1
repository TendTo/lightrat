#!/usr/bin/env pwsh
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Template,

    [Parameter(Position = 1)]
    [string]$Name = "unknown",

    [Parameter(Position = 2)]
    [string]$Version = "unknown.unknown.unknown",

    [Parameter(Position = 3)]
    [string]$Description = "description"
)

$ErrorActionPreference = "Stop"
$statusFile = Join-Path "bazel-out" "stable-status.txt"

if (-not (Test-Path -LiteralPath $Template -PathType Leaf)) {
    throw "Version header template not found: $Template"
}
if (-not (Test-Path -LiteralPath $statusFile -PathType Leaf)) {
    throw "Bazel stable status file not found: $statusFile"
}

$versionParts = $Version.Split(".")
$major = if ($versionParts.Count -gt 0) { $versionParts[0] } else { "unknown" }
$minor = if ($versionParts.Count -gt 1) { $versionParts[1] } else { "unknown" }
$revision = if ($versionParts.Count -gt 2) { $versionParts[2] } else { "unknown" }

$repositoryStatus = Get-Content -LiteralPath $statusFile |
    Where-Object { $_ -match "^STABLE_REPOSITORY_STATUS\s+" } |
    Select-Object -First 1
if ($null -ne $repositoryStatus) {
    $repositoryStatus = $repositoryStatus -replace "^STABLE_REPOSITORY_STATUS\s+", ""
} else {
    $repositoryStatus = ""
}

$replacements = @{
    "@PROJECT_NAME@" = $Name
    "@PROJECT_VERSION@" = $Version
    "@PROJECT_VERSION_FULL@" = $Version
    "@PROJECT_VERSION_MAJOR@" = $major
    "@PROJECT_VERSION_MINOR@" = $minor
    "@PROJECT_VERSION_PATCH@" = $revision
    "@PROJECT_VERSION_REPOSTAT@" = $repositoryStatus
    "@PROJECT_VERSION_REVISION@" = $repositoryStatus
    "@PROJECT_DESCRIPTION@" = $Description
}

Get-Content -LiteralPath $Template -Raw |
    ForEach-Object {
        $result = $_
        foreach ($placeholder in $replacements.Keys) {
            $result = $result.Replace($placeholder, [string]$replacements[$placeholder])
        }
        $result
    }
