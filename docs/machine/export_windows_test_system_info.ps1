# Windows Test System Info Exporter (Markdown)
$hostname = $env:COMPUTERNAME
$date = Get-Date -Format "yyyy-MM-dd"
$cpu = (Get-CimInstance Win32_Processor | Select-Object -First 1 -ExpandProperty Name)
$ram = "{0} GB" -f [math]::Round((Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory / 1GB)
$adapters = Get-NetAdapter | Where-Object { $_.Status -eq "Up" } | Select-Object -ExpandProperty Name
$os = (Get-CimInstance Win32_OperatingSystem)
$osver = "$($os.Caption), Version $($os.Version) (Build $($os.BuildNumber))"
$arch = if ($os.OSArchitecture) { $os.OSArchitecture } else { "64-bit" }
$vs = Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\VisualStudio\SxS\VS7' -ErrorAction SilentlyContinue | Get-Member -MemberType NoteProperty | ForEach-Object { $_.Name }
$cmake = (cmake --version 2>$null | Select-String -Pattern "cmake version" | ForEach-Object { $_.Line }) -replace "cmake version ",""
$python = (python --version 2>&1)
$git = (git --version 2>&1)
$psver = $PSVersionTable.PSVersion.ToString()
$npcap = if (Test-Path Env:NPCAP_DIR) { $env:NPCAP_DIR } elseif (Test-Path Env:NPCAP_SDK_DIR) { $env:NPCAP_SDK_DIR } else { "Not installed" }

Write-Output "# Machine Environment Description: Windows Test System ($hostname)"
Write-Output ""
Write-Output "**Date:** $date"
Write-Output ""
Write-Output "## Hardware"
Write-Output "- CPU: $cpu"
Write-Output "- RAM: $ram"
Write-Output "- Network Adapters:"
foreach ($a in $adapters) { Write-Output "  - $a" }
Write-Output ""
Write-Output "## Operating System"
Write-Output "- $osver"
Write-Output "- $arch"
Write-Output ""
Write-Output "## Software & Drivers"
Write-Output "- Visual Studio: $($vs -join ', ')"
Write-Output "- CMake: $cmake"
Write-Output "- WinPcap SDK (legacy, C:\\WpdPack)"
Write-Output "- Npcap: $npcap"
Write-Output "- Intel Ethernet drivers: [Please fill in version]"
Write-Output ""
Write-Output "## Dependencies"
Write-Output "- Python: $python"
Write-Output "- Git: $git"
Write-Output "- PowerShell: $psver"
Write-Output ""
Write-Output "## Known Issues / Limitations"
Write-Output "- MAC address cannot be read by HAL on Windows (see insights)"
Write-Output "- NDIS timestamping only partially supported (fallback mode)"
Write-Output "- Only one adapter per type detected in test logs"
Write-Output ""
Write-Output "## Custom Configuration"
Write-Output "- [Please fill in any custom network, BIOS, or driver settings relevant for AVB/TSN]"
Write-Output ""
Write-Output "---"
Write-Output "*Please update this file with any hardware or software changes relevant for reproducibility or debugging.*"
