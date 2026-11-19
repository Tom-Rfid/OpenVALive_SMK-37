# OpenVALive - manage the va/ submodule from Windows (PowerShell, needs git): same commands as scripts/va.sh
#
#   .\scripts\va.ps1 init | status | tags | update [BRANCH] | use REF | reset
#
# update and use only move va/: run the tests, then commit the new pin:  git add va; git commit -m "va: <version>"
param([string]$Command = "status", [string]$Ref = "")
$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")

function Pinned { ((git ls-files -s va) -split '\s+')[1] }
function Here   { git -C va rev-parse HEAD 2>$null }
function Name($c) { $n = git -C va describe --tags --always $c 2>$null; if ($n) { $n } else { $c } }

switch ($Command) {
    "init"   { git submodule update --init va; "va/ at $(Name (Here))" }
    "status" {
        $p = Pinned; $h = Here
        "pinned : $(Name $p)  ($p)"
        "va/    : $(Name $h)  ($h)"
        if ($p -ne $h) { "va/ has moved: test, then  git add va; git commit   (or  .\scripts\va.ps1 reset)" }
        if (git -C va status --porcelain) { "va/ has local changes (commit them in OpenVALive_CORE first)" }
    }
    "tags"   { git -C va fetch -q --tags origin; git -C va tag --sort=-v:refname | Select-Object -First 20 }
    "update" {
        $b = if ($Ref) { $Ref } else { "dev" }
        git -C va fetch -q origin $b; git -C va checkout -q --detach FETCH_HEAD
        "va/ now at $(Name (Here)) (latest $b). Test, then: git add va; git commit -m `"va: $(Name (Here))`""
    }
    "use"    {
        if (-not $Ref) { throw "usage: .\scripts\va.ps1 use TAG|BRANCH|COMMIT" }
        git -C va fetch -q --tags origin
        git -C va fetch -q origin $Ref 2>$null
        git -C va checkout -q --detach $Ref 2>$null
        if ($LASTEXITCODE -ne 0) { git -C va checkout -q --detach FETCH_HEAD }
        "va/ now at $(Name (Here)). Test, then: git add va; git commit -m `"va: $(Name (Here))`""
    }
    "reset"  { git submodule update --init va; "va/ back at the pinned $(Name (Here))" }
    default  { Get-Content $PSCommandPath -TotalCount 5 }
}
