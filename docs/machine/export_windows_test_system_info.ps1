# Windows Test System Info Exporter (Markdown)
$hostname = $env:COMPUTERNAME
$date = Get-Date -Format "yyyy-MM-dd"
$cpu = (Get-CimInstance Win32_Processor | Select-Object -First 1 -ExpandProperty Name)
$ram = "{0} GB" -f [math]::Round((Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory / 1GB)
$adapters = Get-NetAdapter | Where-Object { $_.Status -eq "Up" } | Select-Object -ExpandProperty Name
$os = (Get-CimInstance Win32_OperatingSystem)
$osver = "$($os.Caption), Version $($os.Version) (Build $($os.BuildNumber))"
$arch = if ($os.OSArchitecture) { $os.OSArchitecture } else { "64-bit" }
# Visual Studio robust auslesen
$vsReg = Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\VisualStudio\SxS\VS7' -ErrorAction SilentlyContinue
if ($vsReg) {
    $vs = $vsReg | Get-Member -MemberType NoteProperty | ForEach-Object { $_.Name }
} else {
    $vs = @()
}
$cmake = (cmake --version 2>$null | Select-String -Pattern "cmake version" | ForEach-Object { $_.Line }) -replace "cmake version ",""
$python = (python --version 2>&1)
$git = (git --version 2>&1)
$psver = $PSVersionTable.PSVersion.ToString()
$npcap = if (Test-Path Env:NPCAP_DIR) { $env:NPCAP_DIR } elseif (Test-Path Env:NPCAP_SDK_DIR) { $env:NPCAP_SDK_DIR } else { "Not installed" }
$winpcap = if (Test-Path Env:WPCAP_DIR) { $env:WPCAP_DIR } else { "Not installed" }

# Schreibe die Ausgabe in eine Datei im Markdown-Format
$outFile = "windows_test_system_$hostname.md"

$lines = @()
$lines += "# Machine Environment Description: Windows Test System ($hostname)"
$lines += ""
$lines += "**Date:** $date"
$lines += ""
$lines += "## Hardware"
$lines += "- CPU: $cpu"
$lines += "- RAM: $ram"
$lines += "- Network Adapters:"
# Adapter-Details: ALLE Adapter (ohne Status-Filter), robustes Matching für Treiberinfos
$allDrivers = Get-CimInstance Win32_PnPSignedDriver
$adapterDetails = Get-NetAdapter | ForEach-Object {
    $mac = $_.MacAddress
    $desc = $_.InterfaceDescription
    $status = $_.Status
    $name = $_.Name
    $pnpid = $_.PnpDeviceID
    # Suche nach passendem Treiber (Teilstring-Match auf DeviceID)
    $drv = $allDrivers | Where-Object { $_.DeviceID -and $pnpid -and ($_.DeviceID -like "*$pnpid*") }
    if (-not $drv) {
        # Fallback: Suche nach Vendor/Device-IDs
        if ($pnpid -match "VEN_([0-9A-Fa-f]{4})&DEV_([0-9A-Fa-f]{4})") {
            $ven = $matches[1]; $dev = $matches[2]
            $drv = $allDrivers | Where-Object { $_.DeviceID -match "VEN_$ven" -and $_.DeviceID -match "DEV_$dev" }
        }
    }
    $drvVersion = if ($drv) { $drv[0].DriverVersion } else { "n/a" }
    $drvProvider = if ($drv) { $drv[0].DriverProviderName } else { "n/a" }
    "  - Name: $name | Description: $desc | MAC: $mac | Status: $status | Driver: $drvVersion | Provider: $drvProvider | PNP: $pnpid"
}
foreach ($line in $adapterDetails) { $lines += $line }
$lines += ""
$lines += "## Operating System"
$lines += "- $osver"
$lines += "- $arch"
$lines += ""
$lines += "## Software & Drivers"
$lines += "- Visual Studio: $($vs -join ', ')"
$lines += "- CMake: $cmake"
$lines += "- WinPcap SDK: $winpcap"
$lines += "- Npcap: $npcap"
$lines += "- Intel Ethernet drivers: [Please fill in version]"
$lines += ""
$lines += "## Dependencies"
$lines += "- Python: $python"
$lines += "- Git: $git"
$lines += "- PowerShell: $psver"
$lines += ""
$lines += "## Known Issues / Limitations"
$lines += "- MAC address cannot be read by HAL on Windows (see insights)"
$lines += "- NDIS timestamping only partially supported (fallback mode)"
$lines += "- Only one adapter per type detected in test logs"
$lines += ""
$lines += "## Custom Configuration"
$lines += "- [Please fill in any custom network, BIOS, or driver settings relevant for AVB/TSN]"
$lines += ""
$lines += "---"
$lines += "*Please update this file with any hardware or software changes relevant for reproducibility or debugging.*"

$lines | Set-Content -Encoding UTF8 $outFile
Write-Output "System info written to $outFile"
