#Requires -RunAsAdministrator
# Install Chaz Velvet.vst3 into the system VST3 folder from OneDrive\Chaz Velvet zip.
# Paste (elevated):
#   Start-Process powershell -Verb RunAs -ArgumentList '-NoProfile -ExecutionPolicy Bypass -File', "$env:USERPROFILE\OneDrive\Chaz Velvet\Install-ChazVelvet.ps1"
$ErrorActionPreference = 'Stop'

$TargetBundle = 'C:\Program Files\Common Files\VST3\Chaz Velvet.vst3'
$TargetBinary = Join-Path $TargetBundle 'Contents\x86_64-win\Chaz Velvet.vst3'
$ExpectedSha  = '7d8862c31073e8fa9db34be6e752c77b446a49f348677f520f493855151e3957'
$ZipName      = 'ChazVelvet-VST3-Windows-x64.zip'

function Find-Zip {
  $roots = New-Object System.Collections.Generic.List[string]
  if ($PSScriptRoot) { [void]$roots.Add($PSScriptRoot) }
  if ($env:OneDrive) { [void]$roots.Add((Join-Path $env:OneDrive 'Chaz Velvet')) }
  if ($env:OneDriveConsumer) { [void]$roots.Add((Join-Path $env:OneDriveConsumer 'Chaz Velvet')) }
  if ($env:OneDriveCommercial) { [void]$roots.Add((Join-Path $env:OneDriveCommercial 'Chaz Velvet')) }
  Get-ChildItem -LiteralPath $env:USERPROFILE -Directory -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -like 'OneDrive*' } |
    ForEach-Object { [void]$roots.Add((Join-Path $_.FullName 'Chaz Velvet')) }

  foreach ($r in ($roots | Select-Object -Unique)) {
    $p = Join-Path $r $ZipName
    if (Test-Path -LiteralPath $p) { return (Get-Item -LiteralPath $p).FullName }
  }
  return $null
}

$Zip = Find-Zip
if (-not $Zip) {
  throw "Missing $ZipName. Expected next to this script or under OneDrive\Chaz Velvet."
}

Write-Host "Zip: $Zip"
Unblock-File -LiteralPath $Zip -ErrorAction SilentlyContinue
$hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $Zip).Hash.ToLowerInvariant()
if ($hash -ne $ExpectedSha) {
  throw "SHA256 mismatch.`n  got:      $hash`n  expected: $ExpectedSha"
}
Write-Host "SHA256 OK"

$Staging = Join-Path $env:TEMP ('ChazVelvet-install-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $Staging | Out-Null
try {
  Expand-Archive -LiteralPath $Zip -DestinationPath $Staging -Force

  $Bundle = Get-ChildItem -LiteralPath $Staging -Recurse -Directory -Filter 'Chaz Velvet.vst3' -ErrorAction SilentlyContinue |
    Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'Contents\x86_64-win\Chaz Velvet.vst3') } |
    Select-Object -First 1
  if (-not $Bundle) {
    throw "Chaz Velvet.vst3 bundle (with Contents\x86_64-win\Chaz Velvet.vst3) not found inside zip."
  }

  $DestParent = Split-Path $TargetBundle -Parent
  if (-not (Test-Path -LiteralPath $DestParent)) {
    New-Item -ItemType Directory -Path $DestParent -Force | Out-Null
  }
  if (Test-Path -LiteralPath $TargetBundle) {
    Remove-Item -LiteralPath $TargetBundle -Recurse -Force
  }
  Copy-Item -LiteralPath $Bundle.FullName -Destination $TargetBundle -Recurse -Force

  Get-ChildItem -LiteralPath $TargetBundle -Recurse -File | ForEach-Object {
    Unblock-File -LiteralPath $_.FullName -ErrorAction SilentlyContinue
  }

  if (-not (Test-Path -LiteralPath $TargetBinary)) {
    throw "Install incomplete — binary missing: $TargetBinary"
  }

  Write-Host "Installed bundle: $TargetBundle"
  Write-Host "Binary OK:        $TargetBinary"
  Write-Host "Test-Path result: $((Test-Path -LiteralPath $TargetBinary))"
  Write-Host "Next: FL Studio → Options → Manage plugins → Find plugins / Rescan"
}
finally {
  Remove-Item -LiteralPath $Staging -Recurse -Force -ErrorAction SilentlyContinue
}
