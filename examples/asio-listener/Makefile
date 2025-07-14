# ASIO Audio Listener Makefile for Windows
# Requires ASIO SDK and WinPcap/Npcap development libraries

CC = cl
CFLAGS = /D_CRT_SECURE_NO_WARNINGS /DWIN32_LEAN_AND_MEAN /I. /I$(ASIO_SDK_DIR) /I$(PCAP_DIR)\Include
LDFLAGS = /link ws2_32.lib winmm.lib $(PCAP_DIR)\Lib\x64\wpcap.lib $(PCAP_DIR)\Lib\x64\Packet.lib

# Directories (set these to match your environment)
ASIO_SDK_DIR = C:\ASIOSDK2.3
PCAP_DIR = C:\WpdPack

TARGET = asio_listener.exe
SOURCES = asio_listener.c

# ASIO SDK source files (typically needed)
ASIO_SOURCES = $(ASIO_SDK_DIR)\common\asio.cpp \
               $(ASIO_SDK_DIR)\host\asiodrivers.cpp \
               $(ASIO_SDK_DIR)\host\pc\asiolist.cpp

all: $(TARGET)

$(TARGET): $(SOURCES)
	@echo Building ASIO AVB Audio Listener...
	@echo.
	@echo Prerequisites:
	@echo   1. ASIO SDK 2.3 or later (set ASIO_SDK_DIR)
	@echo   2. WinPcap Developer Pack or Npcap SDK (set PCAP_DIR)
	@echo   3. Visual Studio Build Tools or full Visual Studio
	@echo.
	@if not exist "$(ASIO_SDK_DIR)" (echo ERROR: ASIO SDK not found at $(ASIO_SDK_DIR) && exit /b 1)
	@if not exist "$(PCAP_DIR)" (echo ERROR: WinPcap/Npcap SDK not found at $(PCAP_DIR) && exit /b 1)
	$(CC) $(CFLAGS) $(SOURCES) $(ASIO_SOURCES) $(LDFLAGS) /Fe:$(TARGET)
	@echo.
	@echo Build complete: $(TARGET)
	@echo.
	@echo Usage: $(TARGET) "Network Interface Name" ["ASIO Driver Name"]
	@echo Example: $(TARGET) "Intel I219" "ASIO4ALL v2"

clean:
	@if exist *.obj del *.obj
	@if exist *.exe del *.exe
	@if exist *.pdb del *.pdb
	@echo Cleaned build files

help:
	@echo ASIO AVB Audio Listener Build System
	@echo.
	@echo Targets:
	@echo   all     - Build the ASIO listener (default)
	@echo   clean   - Remove build artifacts
	@echo   help    - Show this help message
	@echo.
	@echo Environment Variables:
	@echo   ASIO_SDK_DIR - Path to ASIO SDK (default: C:\ASIOSDK2.3)
	@echo   PCAP_DIR     - Path to WinPcap/Npcap SDK (default: C:\WpdPack)
	@echo.
	@echo Download Requirements:
	@echo   ASIO SDK: https://www.steinberg.net/developers/
	@echo   Npcap SDK: https://npcap.com/dist/npcap-sdk-1.13.zip
	@echo   WinPcap SDK: https://www.winpcap.org/install/bin/WpdPack_4_1_2.zip

.PHONY: all clean help
