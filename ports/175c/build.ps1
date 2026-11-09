param(
    [string]$Cli = "arduino-cli",
    [int]$Jobs = 4,
    [switch]$Clean
)
$ErrorActionPreference = "Stop"
Set-Location -LiteralPath $PSScriptRoot
$arguments = @("build.py", "--cli", $Cli, "--jobs", $Jobs)
if ($Clean) { $arguments += "--clean" }
python @arguments
if ($LASTEXITCODE -ne 0) { throw "Build failed ($LASTEXITCODE)" }
