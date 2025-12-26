# PlatformIO Build - Run from Windows
# Usage: .\pio-build.ps1 [COM_PORT]
# Example: .\pio-build.ps1 COM5

param(
    [string]$Port = ""
)

$ProjectName = "MeteoCharts"
$WinPath = "C:\Pio\$ProjectName"
$SourcePath = $PSScriptRoot

Write-Host "=== MeteoCharts PlatformIO Build ===" -ForegroundColor Magenta

# Auto-detect or use provided port
if (!$Port) {
    $ports = [System.IO.Ports.SerialPort]::GetPortNames() | Where-Object { $_ -match "COM\d+" }
    if ($ports.Count -eq 1) {
        $Port = $ports[0]
        Write-Host "Auto-detected port: $Port" -ForegroundColor Cyan
    } elseif ($ports.Count -gt 1) {
        Write-Host "Available ports: $($ports -join ', ')" -ForegroundColor Yellow
        $Port = Read-Host "Enter COM port"
    } else {
        Write-Host "No COM ports found!" -ForegroundColor Red
        exit 1
    }
}

# Find pio.exe
$pioPaths = @(
    "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe",
    "$env:LOCALAPPDATA\Programs\Python\Python*\Scripts\pio.exe",
    "$env:LOCALAPPDATA\Packages\PythonSoftwareFoundation.Python.3.12*\LocalCache\local-packages\Python312\Scripts\pio.exe",
    "$env:LOCALAPPDATA\Packages\PythonSoftwareFoundation.Python.3.11*\LocalCache\local-packages\Python311\Scripts\pio.exe",
    "C:\Users\*\.platformio\penv\Scripts\pio.exe"
)
$pioExe = $null
foreach ($p in $pioPaths) {
    $found = Get-Item $p -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($found) { $pioExe = $found.FullName; break }
}
if (!$pioExe) {
    Write-Host "pio.exe not found! Run in PowerShell:" -ForegroundColor Red
    Write-Host "  where.exe pio" -ForegroundColor Yellow
    exit 1
}
Write-Host "Using: $pioExe" -ForegroundColor Gray

# Create C:\Pio if needed
if (!(Test-Path "C:\Pio")) {
    New-Item -ItemType Directory -Path "C:\Pio" -Force | Out-Null
    Write-Host "Created C:\Pio" -ForegroundColor Gray
}

# Copy project files
Write-Host "Copying files..." -ForegroundColor Cyan

if (!(Test-Path $WinPath)) {
    New-Item -ItemType Directory -Path $WinPath -Force | Out-Null
}

# Main source files
$filesToCopy = @("MeteoCharts.ino", "conf.h", "images.h", "platformio.ini")
foreach ($file in $filesToCopy) {
    $src = Join-Path $SourcePath $file
    if (Test-Path $src) {
        Copy-Item $src $WinPath -Force
        Write-Host "  $file" -ForegroundColor Gray
    } else {
        Write-Host "  $file (missing!)" -ForegroundColor Red
    }
}

# Copy Configuration folder (for modular config)
$configSrc = Join-Path (Split-Path $SourcePath -Parent) "Configuration"
$configDst = Join-Path $WinPath "Configuration"
if (Test-Path $configSrc) {
    if (!(Test-Path $configDst)) {
        New-Item -ItemType Directory -Path $configDst -Force | Out-Null
    }
    Copy-Item "$configSrc\*" $configDst -Recurse -Force
    Write-Host "  Configuration/" -ForegroundColor Gray
}

# Build and upload
Write-Host "`nBuilding and uploading to $Port..." -ForegroundColor Green
Set-Location $WinPath
& $pioExe run -t upload --upload-port $Port

if ($LASTEXITCODE -eq 0) {
    Write-Host "`nUpload OK - Starting monitor on $Port..." -ForegroundColor Green
    & $pioExe device monitor --port $Port
} else {
    Write-Host "`nBuild failed!" -ForegroundColor Red
}
