param([string]$OutputDirectory = (Join-Path $PSScriptRoot 'build'))
$ErrorActionPreference = 'Stop'
$nrRepo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$nrVswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$nrVs = & $nrVswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $nrVs) { throw 'Visual Studio C++ tools are required.' }
$nrBuild = @"
@echo off
call "$nrVs/Common7/Tools/VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /W4 /I"$PSScriptRoot" /I"$nrRepo/external/nvngx_dlss_sdk" /Fo"$OutputDirectory/MaskBlendModeTests.obj" /Fe"$OutputDirectory/MaskBlendModeTests.exe" "$PSScriptRoot/MaskBlendModeTests.cpp"
"@
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'build.cmd'), $nrBuild)
Push-Location -LiteralPath $OutputDirectory
try {
    & (Join-Path $OutputDirectory 'build.cmd') | Tee-Object -FilePath (Join-Path $OutputDirectory 'build.log')
    if ($LASTEXITCODE) { throw 'Mask blend mode suite did not compile.' }
    & (Join-Path $OutputDirectory 'MaskBlendModeTests.exe') | Tee-Object -FilePath (Join-Path $OutputDirectory 'results.log')
    $nrExit = $LASTEXITCODE
} finally { Pop-Location }
exit $nrExit
