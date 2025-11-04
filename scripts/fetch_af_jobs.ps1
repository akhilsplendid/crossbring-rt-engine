Param(
  [string]$OutFile = "data/af_jobs.json",
  [int]$MaxRecords = 25
)

$ErrorActionPreference = 'Stop'

function Ensure-Dir($path) {
  $dir = Split-Path -Parent $path
  if (-not [string]::IsNullOrWhiteSpace($dir)) {
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
  }
}

$uri = "https://platsbanken-api.arbetsformedlingen.se/jobs/v1/search"
$payload = @{ filters = @(@{ type = "occupationField"; value = "apaJ_2ja_LuF" }); fromDate = $null; order = "relevance"; maxRecords = $MaxRecords; startIndex = 0; toDate = (Get-Date).ToString("s") + "Z"; source = "pb" } | ConvertTo-Json -Depth 5

Write-Host "Fetching AF jobs..." -ForegroundColor Cyan
$resp = Invoke-RestMethod -Method Post -Uri $uri -ContentType 'application/json' -Body $payload

Ensure-Dir $OutFile
$resp | ConvertTo-Json -Depth 8 | Set-Content -Path $OutFile -Encoding UTF8
Write-Host "Saved $($resp.ads.Count) ads to $OutFile" -ForegroundColor Green

<#
# Optionally fetch details of first N ads and save alongside
$detailsOut = [System.IO.Path]::ChangeExtension($OutFile, '.details.json')
$detailList = @()
foreach ($ad in $resp.ads | Select-Object -First 5) {
  $jid = $ad.id
  $duri = "https://platsbanken-api.arbetsformedlingen.se/jobs/v1/job/$jid"
  try {
    $detail = Invoke-RestMethod -Method Get -Uri $duri
    $detailList += $detail
  } catch { Write-Warning "Failed details for $jid: $_" }
}
$detailList | ConvertTo-Json -Depth 8 | Set-Content -Path $detailsOut -Encoding UTF8
Write-Host "Saved details to $detailsOut" -ForegroundColor DarkGreen
#>

