# OpenAvnu Daemon Dependencies Guide für ASIO Audio Listener

## 🎯 **Übersicht der Daemon-Abhängigkeiten**

Der **ASIO Audio Listener** benötigt eine koordinierte Startup-Sequenz mehrerer OpenAvnu-Daemons für korrekte AVB/TSN-Funktionalität.

### **Erforderliche Daemons (in Startposition-Reihenfolge):**

#### 1. **gPTP Daemon** (Optional, aber empfohlen)
- **Zweck:** Präzise Zeit-Synchronisation über IEEE 802.1AS
- **Abhängigkeiten:** Keine (kann als erstes starten)
- **Wichtig für:** Genaue Audio-Timing und Latenz-Kontrolle
- **Executable:** `build\gptp\windows_hal\Release\gptp2.exe`

#### 2. **MRPD Daemon** (Erforderlich)
- **Zweck:** Multiple Registration Protocol (MRP) für Stream-Reservierung
- **Abhängigkeiten:** Netzwerk-Interface
- **Wichtig für:** Bandwidth-Reservierung und QoS-Management 
- **Executable:** `build\daemons\mrpd\Release\mrpd.exe`
- **Intel HAL Integration:** Ja (für Hardware-Timestamping)

#### 3. **MAAP Daemon** (Erforderlich)
- **Zweck:** Multicast MAC Address Allocation Protocol
- **Abhängigkeiten:** Netzwerk verfügbar
- **Wichtig für:** Automatische MAC-Adressen-Zuweisung für Audio-Streams
- **Executable:** `build\daemons\maap\Release\maap_daemon.exe`
- **Intel HAL Integration:** Ja (für Hardware-Timestamping)

#### 4. **Windows Shaper Daemon** (Optional)
- **Zweck:** Traffic Shaping und QoS für Windows
- **Abhängigkeiten:** Andere Daemons laufend
- **Wichtig für:** Optimierte Netzwerk-Performance
- **Executable:** `build\daemons\shaper\Release\openavnu_shaper_windows.exe`

---

## 🚀 **Automatisierte Startup-Lösung**

### **Verwendung des Startup-Scripts:**

```powershell
# Alle Daemons mit Standard-Interface starten
.\start_openavnu_daemons.ps1

# Spezifisches Netzwerk-Interface verwenden  
.\start_openavnu_daemons.ps1 -NetworkInterface "Intel(R) Ethernet Controller I225-V"

# gPTP überspringen (falls nicht benötigt)
.\start_openavnu_daemons.ps1 -SkipGptp

# Verbose-Modus für Debugging
.\start_openavnu_daemons.ps1 -Verbose

# Dry-Run (zeigt nur was gemacht würde)
.\start_openavnu_daemons.ps1 -DryRun
```

### **Status-Überprüfung:**
```powershell
# Daemon-Status anzeigen
Show-DaemonStatus

# Alle Daemons beenden
Stop-AllDaemons
```

---

## 🔧 **Manuelle Startup-Sequenz**

Falls Sie die Daemons manuell starten möchten:

### **Schritt 1: gPTP starten (optional)**
```powershell
cd d:\Repos\OpenAvnu\build
.\gptp\windows_hal\Release\gptp2.exe -F ..\gptp_cfg.ini
```

### **Schritt 2: MRPD starten**
```powershell
cd d:\Repos\OpenAvnu\build
.\daemons\mrpd\Release\mrpd.exe -i "Intel(R) Ethernet Controller I225-V"
```

### **Schritt 3: MAAP starten**
```powershell
cd d:\Repos\OpenAvnu\build
.\daemons\maap\Release\maap_daemon.exe
```

### **Schritt 4: Windows Shaper starten (optional)**
```powershell
cd d:\Repos\OpenAvnu\build
.\daemons\shaper\Release\openavnu_shaper_windows.exe
```

---

## 🧪 **ASIO Listener Testing mit Daemon-Koordination**

### **Automatisierter Test-Workflow:**

```powershell
# 1. Alle Daemons starten
.\start_openavnu_daemons.ps1

# 2. Warten bis alle Daemons bereit sind
Show-DaemonStatus

# 3. ASIO Listener starten (falls alle Daemons laufen)
.\test_enhanced.bat

# 4. Nach dem Test alle Daemons beenden
Stop-AllDaemons
```

---

## 📊 **Daemon-Abhängigkeits-Matrix**

| Daemon | gPTP | MRPD | MAAP | Shaper | Intel HAL | Netzwerk |
|--------|------|------|------|--------|-----------|----------|
| **gPTP** | - | ❌ | ❌ | ❌ | ✅ | ✅ |
| **MRPD** | 🔶 | - | ❌ | ❌ | ✅ | ✅ |
| **MAAP** | 🔶 | ❌ | - | ❌ | ✅ | ✅ |
| **Shaper** | 🔶 | 🔶 | 🔶 | - | ❌ | ✅ |
| **ASIO Listener** | 🔶 | ✅ | ✅ | 🔶 | ❌ | ✅ |

**Legende:**
- ✅ = Erforderlich
- 🔶 = Empfohlen/Optional
- ❌ = Nicht erforderlich

---

## 🛠 **Troubleshooting**

### **Häufige Probleme:**

#### **"Network interface not found"**
```powershell
# Verfügbare Interfaces auflisten
Get-NetAdapter | Select-Object Name, InterfaceDescription

# Script mit korrektem Interface starten
.\start_openavnu_daemons.ps1 -NetworkInterface "Ihr Interface Name"
```

#### **"Daemon startup failed"**
```powershell
# Verbose-Modus für detaillierte Fehler
.\start_openavnu_daemons.ps1 -Verbose

# Einzelne Daemons manuell testen
cd d:\Repos\OpenAvnu\build
.\daemons\mrpd\Release\mrpd.exe --help
```

#### **"Intel HAL not available"**
- Überprüfen Sie Intel Ethernet Controller Treiber
- Intel HAL wird automatisch auf Software-Fallback umstellen

### **Performance-Optimierung:**

#### **Für beste ASIO-Performance:**
1. Starten Sie alle Daemons vor dem ASIO Listener
2. Verwenden Sie Intel HAL-kompatible Hardware (I219, I225, I226)
3. Aktivieren Sie gPTP für präzises Timing
4. Konfigurieren Sie Windows Shaper für QoS

#### **Minimale Konfiguration:**
```powershell
# Nur erforderliche Daemons starten
.\start_openavnu_daemons.ps1 -SkipGptp
```

---

## 📁 **Konfigurationsdateien**

### **gPTP Konfiguration:** `gptp_cfg.ini`
```ini
[General]
logLevel=INFO
interface=Intel(R) Ethernet Controller I225-V
```

### **MRPD Parameter:**
- `-i`: Netzwerk Interface
- `-v`: Verbose-Modus  
- `-d`: Daemon-Modus

### **MAAP Parameter:**
- Standard-Port: 15364
- Multicast Range: 91:E0:F0:00:00:00 - 91:E0:F0:00:FD:FF

---

## 🎵 **Integration mit ASIO Audio Listener**

Der **ASIO Audio Listener** nutzt die Daemons folgendermaßen:

1. **MAAP:** Automatische MAC-Adressen für Audio-Streams
2. **MRPD:** Bandwidth-Reservierung für garantierte Audio-Qualität  
3. **gPTP:** Synchronisierte Zeitbasis für Low-Latency Audio
4. **Shaper:** Optimierte Netzwerk-Performance

### **Empfohlener Workflow:**
```powershell
# 1. Daemons starten
.\start_openavnu_daemons.ps1 -Verbose

# 2. Status prüfen
Show-DaemonStatus

# 3. ASIO Listener starten
.\asio_listener.exe

# 4. Nach Nutzung Cleanup
Stop-AllDaemons
```

---

**📝 Hinweis:** Dieses Guide gewährleistet, dass alle erforderlichen OpenAvnu-Komponenten in der korrekten Reihenfolge gestartet werden und bereit sind, bevor der ASIO Audio Listener verwendet wird.
