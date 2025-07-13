
# Automotive AVB/TSN Profile Specification

**Scope:**
This document defines the Automotive AVB/TSN profile for in-vehicle Ethernet networks, focusing on infotainment, driver assistance, and control systems. It consolidates requirements from relevant IEEE standards, automotive industry practice, and OpenAvnu implementation experience. This profile is distinct from Milan/ProAV and is tailored for automotive use cases.

---

## 1. Application Scope and Use Cases

- In-vehicle audio/video streaming (infotainment, rear-seat entertainment)
- Advanced driver-assistance systems (ADAS) sensor data transport
- Control and diagnostics data exchange between ECUs
- Time-synchronized multi-ECU applications

---

## 2. Device Classes

| Device Class | Description |
|--------------|-------------|
| ECU          | Electronic Control Unit (talker, listener, or both) |
| Gateway      | Bridges multiple network segments or domains |
| Switch       | AVB/TSN bridge enforcing protocol constraints |
| Endpoint     | Edge device (audio/video/control) |

---

## 3. Protocol Requirements (Automotive Profile)

### 3.1 gPTP (IEEE 802.1AS, Avnu Automotive Profile 1.6)
| Parameter                   | Value/Range           | Tolerance/Limit         | Notes/Definition & Source              |
|-----------------------------|-----------------------|------------------------|----------------------------------------|
| BMCA                        | Disabled              | Required               | Fixed grandmaster, preconfigured roles [Avnu 1.6 §6.2.1.1, 6.3] |
| asCapable                   | TRUE on link up       | Required               | For time-aware ports [Avnu 1.6 §6.2.1.2] |
| Announce                    | Disabled              | Required               | No announce messages sent/processed [Avnu 1.6 §6.3] |
| initialLogSyncInterval      | -5 to -3 (31.25–125ms)| Required               | Time critical ports [Avnu 1.6 Table 12] |
| operLogSyncInterval         | -3 to 0 (125ms–1s)    | Required               | Time critical ports [Avnu 1.6 Table 12] |
| initialLogPdelayReqInterval | 0 (1s)                | Required               | [Avnu 1.6 Table 12]                    |
| operLogPdelayReqInterval    | 0–3 (1s–8s)           | Required               | [Avnu 1.6 Table 12]                    |
| Persistent Storage          | neighborPropDelay, rateRatio, neighborRateRatio | Required | Update if >100ns diff [Avnu 1.6 §6.2.2] |
| Signaling                   | Required              | ≤250ms/2 intervals     | Process within 2 intervals or 250ms, send within 60s [Avnu 1.6 §6.2.4] |
| Follow_Up TLVs              | Required              | Required               | All Follow_Up messages [Avnu 1.6 §6.2.5] |
| Sync Jitter                 | ≤1,000ns              | Required               | Measured at listener [Avnu 1.6 §6.2.6]  |
| Path Delay Variation        | ≤10,000ns             | Required               | [Avnu 1.6 §6.2.6]                       |
| Convergence Time            | ≤100ms                | Required               | After startup [Avnu 1.6 §6.2.6]         |

### 3.2 AVTP (IEEE 1722, Avnu Automotive Spec)
| Parameter                   | Value/Range           | Tolerance/Limit         | Notes/Definition & Source              |
|-----------------------------|-----------------------|------------------------|----------------------------------------|
| Audio Format                | AAF only              | Required               | Sample format/rate is application/OEM-specific [Avnu 1.6, AUTOSAR] |
| Max Audio Latency           | ≤2ms                  | Required               | Class A, end-to-end [Avnu 1.6]         |
| Max Video Latency           | ≤50ms                 | Required (if video)    | Class B, end-to-end [Avnu 1.6]         |
| Channel Layouts             | Dynamic               | Must support change    | Runtime reconfiguration [Avnu 1.6]     |
| Presentation Time           | gPTP domain           | ±1,000ns jitter (typ.) | Jitter typical, not strictly mandated [Avnu 1.6] |

### 3.3 MSRP/SRP (IEEE 802.1Qat)
| Parameter                   | Value/Range           | Tolerance/Limit         | Notes/Definition & Source              |
|-----------------------------|-----------------------|------------------------|----------------------------------------|
| SR Class                    | Class A required      | Class B optional        | [Avnu 1.6]                             |
| Max Latency (Class A)       | ≤2ms                  | Required                | End-to-end [Avnu 1.6]                  |
| Max Latency (Class B)       | ≤50ms                 | Required (if used)      | End-to-end [Avnu 1.6]                  |
| Bandwidth Reservation       | ≤75% per port         | Required                | Class A+B [IEEE 802.1Q]                |
| Admission Control           | Strict                | Required                | No over-reservation [Avnu 1.6]         |

### 3.4 MAAP (IEEE 1722)
| Parameter                   | Value/Range           | Tolerance/Limit         | Notes/Definition & Source              |
|-----------------------------|-----------------------|------------------------|----------------------------------------|
| Address Range               | 91:E0:F0:00:00:00–91:E0:F0:FF:FF:FF | Fixed | [IEEE 1722]                        |
| Probe/Defend Interval       | 1s (±10%)             | Required                | [IEEE 1722]                            |
| Max Probes                  | 3                     | Required                | [IEEE 1722]                            |

### 3.5 AVDECC (IEEE 1722.1)
| Parameter                   | Value/Range           | Tolerance/Limit         | Notes/Definition & Source              |
|-----------------------------|-----------------------|------------------------|----------------------------------------|
| Descriptor Set              | System-specific        | Must match              | ENTITY, CONFIGURATION, STREAM_INPUT/OUTPUT, AUDIO_UNIT [IEEE 1722.1] |
| Connection Setup            | ≤1s                   | Required                | [IEEE 1722.1]                          |
| Control Response            | ≤100ms                | Required                | [IEEE 1722.1]                          |

### 3.6 Redundancy
| Parameter                   | Value/Range           | Tolerance/Limit         | Notes/Definition & Source              |
|-----------------------------|-----------------------|------------------------|----------------------------------------|
| Failover                    | Seamless (≤0ms)       | Required                | For required device classes [Avnu 1.6] |
| Stream Duplication          | Required              | Strict                  | Where mandated [Avnu 1.6]              |

### 3.7 Management and Diagnostics
| Parameter                   | Value/Range           | Tolerance/Limit         | Notes/Definition & Source              |
|-----------------------------|-----------------------|------------------------|----------------------------------------|
| YANG Model Support          | Required (v2+)        | Required                | For configuration/monitoring [Avnu 1.6]|
| Log Retention               | ≥24h                  | Device dependent        | [Avnu 1.6]                             |
| Diagnostic Response         | ≤1s                   | Required                | [Avnu 1.6]                             |

---

**Hardware Note:**
Intel I210/I225/I219 controllers: Confirmed IEEE 1588/802.1AS support, hardware timestamping, TSN features (Qav, Qbv, Qbu), 1Gbps/2.5Gbps, sub-microsecond accuracy. [Intel datasheets]

### 3.2 AVTP (IEEE 1722)
| Parameter                   | Default Value | Tolerance/Limit         | Notes/Definition                       |
|-----------------------------|---------------|------------------------|----------------------------------------|
| Audio Sample Rate           | 48kHz         | Not mandated           | 48kHz typical, others allowed          |
| Max Audio Latency           | 2 ms          | ≤2 ms (Class A)        | End-to-end, talker to listener         |
| Max Video Latency           | 50 ms         | ≤50 ms (Class B)       | If video supported                     |
| Channel Layouts             | Dynamic       | Must support change    | Runtime reconfiguration                |
| Presentation Time           | gPTP domain   | ±1,000 ns jitter (typ.)| Jitter typical, not strictly mandated  |

> **Note:** The Automotive profile does not mandate any specific audio sample format. Format selection (e.g., 16-bit PCM, 24-bit, float) is application- or OEM-specific. Only AVTP Audio Format (AAF) support is required for audio streams.

### 3.3 MSRP/SRP (IEEE 802.1Qat)
| Parameter                   | Default Value | Tolerance/Limit         | Notes/Definition                       |
|-----------------------------|---------------|------------------------|----------------------------------------|
| SR Class                    | Class A       | Class A required        | Class B optional                       |
| Max Latency (Class A)       | 2 ms          | ≤2 ms                   | End-to-end                             |
| Max Latency (Class B)       | 50 ms         | ≤50 ms                  | If used                                |
| Bandwidth Reservation       | 75% (A+B)     | ≤75% total              | Per port, per IEEE 802.1Q              |
| Talker Advertise Timeout    | 200 ms        | ±10%                    |                                        |
| Listener Ready Timeout      | 200 ms        | ±10%                    |                                        |
| Admission Control           | Required      | Strict                  | No over-reservation allowed            |

### 3.4 MAAP (IEEE 1722)
| Parameter                   | Default Value | Tolerance/Limit         | Notes/Definition                       |
|-----------------------------|---------------|------------------------|----------------------------------------|
| MAAP Address Range          | 91:E0:F0:00:00:00 - 91:E0:F0:FF:FF:FF | Fixed | Automotive required range              |
| Probe Interval              | 1 s           | ±10%                    |                                        |
| Defend Interval             | 1 s           | ±10%                    |                                        |
| Max Probes                  | 3             | Fixed                   | Before claiming address                |

### 3.5 AVDECC (IEEE 1722.1)
| Parameter                   | Default Value | Tolerance/Limit         | Notes/Definition                       |
|-----------------------------|---------------|------------------------|----------------------------------------|
| Descriptor Set              | System-specific| Must match             | ENTITY, CONFIGURATION, STREAM_INPUT/OUTPUT, AUDIO_UNIT |
| Connection Setup Time       | 1 s           | ≤1 s                    | From request to ready                  |
| Control Response Time       | 100 ms        | ≤100 ms                 | For all mandatory controls             |

### 3.6 Redundancy
| Parameter                   | Default Value | Tolerance/Limit         | Notes/Definition                       |
|-----------------------------|---------------|------------------------|----------------------------------------|
| Failover Time               | 0 ms (seamless)| ≤0 ms (no interruption)| For required device classes            |
| Stream Duplication          | Required      | Strict                  | Where redundancy is mandated           |

### 3.7 Management and Diagnostics
| Parameter                   | Default Value | Tolerance/Limit         | Notes/Definition                       |
|-----------------------------|---------------|------------------------|----------------------------------------|
| YANG Model Support          | Required      | Automotive v2+          | For configuration and monitoring       |
| Log Retention               | 24 h          | ≥24 h                   | Device dependent                       |
| Diagnostic Response Time    | 1 s           | ≤1 s                    | For all mandatory diagnostics          |

---

## 4. Mandatory vs. Optional Features

| Feature                | Mandatory | Optional/Notes |
|------------------------|-----------|---------------|
| gPTP (time sync)       | Yes       |               |
| AVTP (audio/video)     | Yes       | Video optional |
| MSRP/SRP               | Yes       |               |
| MAAP                   | Yes       |               |
| AVDECC                 | Rec.      | Not always required |
| Redundancy             | If safety | For critical ECUs |
| Diagnostics (YANG)     | Yes (v2+) |               |

---

## 5. Configuration and Implementation Guidance

- Use 16-bit PCM/48kHz for audio unless otherwise required by the application.
- Ensure all ECUs and switches are configured for correct gPTP domain and sync parameters.
- Validate end-to-end latency and bandwidth reservation in real hardware.
- Document all configuration and test results in accordance with automotive process standards (e.g., ASPICE).

---

## 6. References

- IEEE 802.1AS, 802.1Qat, 802.1Qav, 1722, 1722.1
- Automotive AVB/TSN specifications (OEM-specific)
- OpenAvnu insights and test results

---

*This document is a living reference. Update as new standards, errata, or OEM requirements are published.*
