# Windows-Header-Konflikte und Build-Probleme bei Intel Ethernet HAL

**Datum:** 2025-07-10

## Kontext
Test und Validierung der Intel Ethernet HAL (Hardware Abstraction Layer) für I210/I219/I225 Adapter unter Windows. Ziel war ein vollständiger automatisierter Testlauf aller Adapter und HAL-Funktionen.

## Beobachtung
- Beim Kompilieren des Testprogramms traten Fehler und Warnungen auf, die auf Header-Konflikte zwischen `winsock.h` und `winsock2.h` sowie auf doppelte Definitionen von Makros wie `WIN32_LEAN_AND_MEAN` und `INTEL_HAL_WINDOWS` zurückzuführen waren.
- Zusätzlich wurde die statische Bibliothek `intel-ethernet-hal-static.lib` zunächst nicht gefunden, weil das Build-Verzeichnis falsch initialisiert war.
- Legacy-Testdateien verursachten Build-Abbrüche durch fehlende Header.

## Ursache
- Die HAL-API und die eigenen Tests binden Windows-Header ein, wobei die Reihenfolge und Mehrfachdefinitionen zu Konflikten führen können.
- CMake wurde im falschen Verzeichnis initialisiert, sodass Abhängigkeiten nicht korrekt erkannt wurden.
- Alte Testdateien waren noch im Build enthalten, obwohl sie nicht mehr benötigt werden.

## Lösung
- Vor dem Einbinden der HAL-API werden nun explizit `WIN32_LEAN_AND_MEAN` und die richtigen Windows-Header in der richtigen Reihenfolge eingebunden.
- Das Build-Verzeichnis wurde gelöscht und CMake korrekt aus dem Hauptverzeichnis initialisiert.
- Legacy-Tests wurden aus dem Build entfernt.

## Takeaways
- Windows-Header immer in der richtigen Reihenfolge und mit passenden Defines einbinden, um Konflikte zu vermeiden.
- CMake-Builds immer aus dem Hauptverzeichnis initialisieren, um alle Abhängigkeiten korrekt zu erfassen.
- Legacy-Tests und nicht mehr benötigte Dateien konsequent aus dem Build entfernen.
- Lessons Learned immer in `docs/insights/` dokumentieren, um wiederkehrende Fehler zu vermeiden.
