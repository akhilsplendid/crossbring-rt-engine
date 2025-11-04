Param(
  [ValidateSet('Debug','Release')][string]$Configuration = 'Release',
  [switch]$EnableHttpServer = $true,
  [switch]$EnableSqlite = $true,
  [switch]$EnableZmq = $false,
  [switch]$EnableCpr = $false
)

$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot\..
try {
  $args = @('-S','.', '-B','build', "-DCMAKE_BUILD_TYPE=$Configuration")
  if ($EnableHttpServer) { $args += '-DENABLE_HTTP_SERVER=ON' } else { $args += '-DENABLE_HTTP_SERVER=OFF' }
  if ($EnableSqlite)     { $args += '-DENABLE_SQLITE=ON' } else { $args += '-DENABLE_SQLITE=OFF' }
  if ($EnableZmq)        { $args += '-DENABLE_ZEROMQ=ON' } else { $args += '-DENABLE_ZEROMQ=OFF' }
  if ($EnableCpr)        { $args += '-DENABLE_CPR=ON' } else { $args += '-DENABLE_CPR=OFF' }

  if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) { throw 'CMake not found in PATH.' }
  & cmake @args
  & cmake --build build --config $Configuration
}
finally { Pop-Location }

