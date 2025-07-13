# MILAN Profile Documentation (v1 vs v2)

**Profile:** MILAN (Media IP Networking Alliance)

---
> **Note:** This document contains only Milan-specific requirements. For Automotive, ProAV, or Industrial AVB/TSN requirements, see the corresponding profile files. Where Milan explicitly adopts or mandates Automotive AVB/TSN requirements, this is referenced in the text.

---

## Milan Protocol Requirements: Complete Reference

This section provides a detailed, self-contained specification for all AVB/TSN protocols required by the Milan profile. Each protocol area includes purpose, message formats, configuration, and Milan-specific requirements.

---
## Protocol Definitions, Tolerances, Limits, and Default Values

This section summarizes all key definitions, tolerances, limits, and default values required for Milan-compliant devices and networks. All values are normative unless otherwise stated.

### General Definitions
- **Talker:** Device that transmits AVB/TSN streams (audio/video)
- **Listener:** Device that receives AVB/TSN streams
- **Bridge:** Switch that forwards AVB/TSN traffic and enforces protocol constraints
- **Controller:** (Optional) Device that manages stream connections and configuration

---
### 1. gPTP (IEEE 802.1AS)
| Parameter                | Default Value | Tolerance/Limit         | Notes                                  |
|--------------------------|--------------|------------------------|----------------------------------------|
| Sync Interval            | 125ms        | ±10%                   | logSyncInterval = -3                   |
| Announce Interval        | 1s           | ±10%                   | logAnnounceInterval = 0                |
| PDelay Interval          | 1s           | ±10%                   | logPdelayReqInterval = 0               |
| ClockClass               | 248          | Fixed                   | Milan required                         |
| ClockAccuracy            | 0xFE         | Fixed                   | Milan required                         |
| OffsetScaledLogVariance  | 0x4E5D       | Fixed                   | Milan required                         |
| neighborPropDelayThresh  | 800,000 ns   | Max 800,000 ns          | 800μs                                  |
| Max Sync Jitter          | 1,000 ns     | ≤1,000 ns               | Peak-to-peak, measured at listener     |
| Max Path Delay Variation | 10,000 ns    | ≤10,000 ns              |                                        |
| Convergence Target       | <100 ms      | Must not exceed 100 ms  | Time to achieve sync after startup     |

---
### 2. AVTP (IEEE 1722)
| Parameter                | Default Value | Tolerance/Limit         | Notes                                  |
|--------------------------|--------------|------------------------|----------------------------------------|
| Audio Sample Format      | 16-bit int    | Required                | AAF only                               |
| Audio Sample Rate        | 48kHz/44.1kHz | Required                | At least one must be supported         |
| Max Audio Latency        | 2 ms (Class A)| ≤2 ms                   | End-to-end, talker to listener         |
| Max Video Latency        | 50 ms (Class B)| ≤50 ms                  | If video supported (v2)                |
| Channel Layouts          | Dynamic       | Must support change     | See AVTP event field                   |
| Presentation Time        | gPTP domain   | ±1,000 ns jitter        |                                        |

---
### 3. MSRP (IEEE 802.1Qat)
| Parameter                | Default Value | Tolerance/Limit         | Notes                                  |
|--------------------------|--------------|------------------------|----------------------------------------|
| SR Class                 | Class A       | Class A required        | Class B optional                       |
| Max Latency (Class A)    | 2 ms          | ≤2 ms                   | End-to-end                             |
| Max Latency (Class B)    | 50 ms         | ≤50 ms                  | If used                                |
| Bandwidth Reservation    | 75% (Class A+B)| ≤75% total              | Per port, per IEEE 802.1Q              |
| Talker Advertise Timeout | 200 ms        | ±10%                    |                                        |
| Listener Ready Timeout   | 200 ms        | ±10%                    |                                        |

---
### 4. MAAP (IEEE 1722)
| Parameter                | Default Value | Tolerance/Limit         | Notes                                  |
|--------------------------|--------------|------------------------|----------------------------------------|
| MAAP Address Range       | 91:E0:F0:00:00:00 - 91:E0:F0:FF:FF:FF | Fixed | Milan required range                  |
| Probe Interval           | 1s           | ±10%                    |                                        |
| Defend Interval          | 1s           | ±10%                    |                                        |
| Max Probes               | 3            | Fixed                   | Before claiming address                |

---
### 5. SRP (IEEE 802.1Qat)
| Parameter                | Default Value | Tolerance/Limit         | Notes                                  |
|--------------------------|--------------|------------------------|----------------------------------------|
| Stream Registration Timeout | 200 ms     | ±10%                    |                                        |
| Max Streams per Port     | 512          | ≥512                    | Device dependent, Milan minimum        |
| Admission Control        | Required     | Strict                  | No over-reservation allowed            |

---
### 6. AVDECC (IEEE 1722.1)
| Parameter                | Default Value | Tolerance/Limit         | Notes                                  |
|--------------------------|--------------|------------------------|----------------------------------------|
| Descriptor Set           | Milan Spec   | Must match              | ENTITY, CONFIGURATION, STREAM_INPUT/OUTPUT, AUDIO_UNIT |
| Connection Setup Time    | 1s           | ≤1s                     | From request to ready                  |
| Control Response Time    | 100 ms       | ≤100 ms                 | For all mandatory controls             |

---
### 7. Redundancy (Milan v2+)
| Parameter                | Default Value | Tolerance/Limit         | Notes                                  |
|--------------------------|--------------|------------------------|----------------------------------------|
| Failover Time            | 0 ms (seamless)| ≤0 ms (no interruption)| For required device classes            |
| Stream Duplication       | Required     | Strict                  | Where redundancy is mandated           |

---
### 8. Management and Diagnostics
| Parameter                | Default Value | Tolerance/Limit         | Notes                                  |
|--------------------------|--------------|------------------------|----------------------------------------|
| YANG Model Support       | Required     | Milan v2+               | For configuration and monitoring       |
| Log Retention            | 24h          | ≥24h                    | Device dependent                       |
| Diagnostic Response Time | 1s           | ≤1s                     | For all mandatory diagnostics          |

---

### 1. Generalized Precision Time Protocol (gPTP, IEEE 802.1AS)
**Purpose:** Provides precise time synchronization for all Milan devices.

- **Standard:** IEEE 802.1AS-2020 (required)
- **Profile:** Milan requires strict timing (see table below)

| Parameter                | Milan Required Value |
|--------------------------|---------------------|
| Sync Interval            | 125ms (logSyncInterval = -3) |
| Announce Interval        | 1s (logAnnounceInterval = 0) |
| PDelay Interval          | 1s (logPdelayReqInterval = 0) |
| ClockClass               | 248                 |
| ClockAccuracy            | 0xFE                |
| OffsetScaledLogVariance  | 0x4E5D              |
| neighborPropDelayThresh  | 800,000 ns (800us)  |

**Configuration Example:** See [ptp] section in INI example below.

**Milan-Specific:**
- All endpoints and bridges must support gPTP and enforce Milan timing.
- BMCA must be disabled; static GM configuration is required.
- All time-aware ports must set asCapable=TRUE when link is up.

---
### 2. AVTP (Audio Video Transport Protocol, IEEE 1722)
**Purpose:** Transports time-sensitive audio (and optionally video) streams over Ethernet.

- **Standard:** IEEE 1722-2016
- **Milan Requirements:**
  - All audio streams must use AVTP Audio Format (AAF)
  - 16-bit integer, 48kHz or 44.1kHz sample rates required
  - Video streams (Milan v2) must use AVTP-compliant formats
  - Dynamic channel layouts must be supported

**AVTPDU (AVTP Data Unit) Key Fields:**
| Field         | Description                        |
|---------------|------------------------------------|
| subtype       | 0x02 (AAF), 0x00 (61883/IIDC), etc.|
| sv            | Stream ID valid                    |
| stream_id     | Unique stream identifier           |
| timestamp     | Presentation time (gPTP domain)    |
| format        | AAF, 61883, etc.                   |
| event         | Channel layout change, etc.        |

**Milan-Specific:**
- All devices must support AAF for audio; video is optional (v2)
- Presentation time must be derived from gPTP
- Channel layout changes must be signaled using the event field

---
### 3. MSRP (Multiple Stream Reservation Protocol, IEEE 802.1Qat)
**Purpose:** Reserves bandwidth and resources for AVB streams across the network.

- **Standard:** IEEE 802.1Qat (integrated into IEEE 802.1Q)
- **Milan Requirements:**
  - All talkers and listeners must implement MSRP state machines
  - Streams must be registered before transmission
  - Class A (2ms latency) and Class B (50ms latency) supported; Milan typically uses Class A

**MSRP Message Types:**
| Message Type | Purpose                        |
|--------------|-------------------------------|
| Talker Advertise | Announces stream availability |
| Listener Ready   | Indicates listener is ready   |
| Domain Attribute | Declares supported SR classes |

**Milan-Specific:**
- All endpoints must register streams before sending/receiving
- Bridges must enforce bandwidth and class constraints
- MSRP attributes must be propagated per IEEE 802.1Qat

---
### 4. MAAP (Multiple Address Assignment Protocol, IEEE 1722)
**Purpose:** Dynamically allocates multicast MAC addresses for AVB streams.

- **Standard:** IEEE 1722 (MAAP section)
- **Milan Requirements:**
  - All talkers must use MAAP to obtain stream destination MAC addresses
  - MAAP range: 91:E0:F0:00:00:00 to 91:E0:F0:FF:FF:FF

**MAAP Message Types:**
| Message Type | Purpose                        |
|--------------|-------------------------------|
| Probe        | Checks address availability    |
| Defend       | Responds to address conflict   |
| Announce     | Claims address range           |

**Milan-Specific:**
- Devices must not use static multicast addresses outside the MAAP range
- MAAP state machine must be implemented per IEEE 1722

---
### 5. SRP (Stream Reservation Protocol, IEEE 802.1Qat)
**Purpose:** End-to-end reservation of network resources for AVB streams (MSRP is the protocol, SRP is the service).

- **Standard:** IEEE 802.1Qat
- **Milan Requirements:**
  - All streams must be reserved before transmission
  - Endpoints and bridges must implement SRP state machines

**SRP Key Concepts:**
- Stream registration, bandwidth calculation, and admission control
- Class A and B support (Milan: Class A required)

---
### 6. AVDECC (IEEE 1722.1)
**Purpose:** Device discovery, enumeration, connection management, and control.

- **Standard:** IEEE 1722.1
- **Milan Requirements:**
  - Devices must support AVDECC for stream connection and control
  - Entity model and descriptors must be implemented per Milan spec

**AVDECC Key Descriptors:**
| Descriptor Type | Purpose                        |
|----------------|-------------------------------|
| ENTITY         | Device identity                |
| CONFIGURATION  | Device configuration options   |
| STREAM_INPUT   | Stream input endpoints         |
| STREAM_OUTPUT  | Stream output endpoints        |
| AUDIO_UNIT     | Audio signal processing        |

**Milan-Specific:**
- All Milan devices must implement required AVDECC descriptors
- Stream connection and control must be AVDECC-compliant

---
### 7. Redundancy (Milan v2+)
**Purpose:** Ensures stream delivery in case of network or device failure.

- **Milan Requirements:**
  - Redundant stream support is optional in v1, required for some device classes in v2
  - Devices must support seamless failover and stream duplication where required

---
### 8. Management and Diagnostics
**Purpose:** Enables configuration, monitoring, and troubleshooting of Milan devices.

- **Milan Requirements:**
  - Devices must support YANG models for configuration (v2)
  - Enhanced diagnostics and logging required

---
## Overview
- MILAN is an application profile for deterministic audio networking based on AVB/TSN standards.
- Specifies required protocols (gPTP, AVTP, SRP, MAAP, etc.), device classes, and interoperability requirements.
- Multiple MILAN profile versions exist; differences must be clearly documented.

## Version History and Differences
| Feature/Aspect         | MILAN v1                        | MILAN v2                        |
|-----------------------|----------------------------------|----------------------------------|
| Protocol Baseline     | AVB/TSN (IEEE 802.1AS, 1722, etc.) | AVB/TSN (updated standards)      |
| Device Classes        | Talker, Listener, Bridge         | Expanded device classes          |
| Stream Format         | Audio only                       | Audio + video (optional)         |
| Redundancy            | Not required                     | Optional/required (per class)    |
| Certification         | v1 test suite                    | v2 test suite, new requirements  |
| Management            | Basic                            | Enhanced, YANG models, etc.      |


## Profile Parameters (Consolidated)

### Required Protocols
- gPTP (IEEE 802.1AS-2020, strict Milan timing)
- AVTP (IEEE 1722)
- SRP (IEEE 802.1Qat)
- MAAP (IEEE 1722.1)
- Redundancy (optional/required in v2)

### Device Classes and Capabilities
| Device Class | v1 Capabilities | v2 Capabilities |
|--------------|----------------|----------------|
| Talker       | Audio streams  | Audio, video, redundancy, secure channels |
| Listener     | Audio receive  | Audio, video, redundancy, secure channels |
| Bridge       | AVB/TSN switch | Enhanced redundancy, management, diagnostics |
| Controller   | -              | Optional, for advanced management |

### Milan Timing and Clock Requirements
| Parameter                | Value (Milan Required) |
|--------------------------|-----------------------|
| Sync Interval            | 125ms (logSyncInterval = -3) |
| Announce Interval        | 1s (logAnnounceInterval = 0) |
| PDelay Interval          | 1s (logPdelayReqInterval = 0) |
| Convergence Target       | < 100ms                |
| Max Sync Jitter          | 1000ns (1μs)           |
| Max Path Delay Variation | 10000ns (10μs)         |
| ClockClass               | 248                    |
| ClockAccuracy            | 0xFE                   |
| OffsetScaledLogVariance  | 0x4E5D                 |

### Configuration Options
- CLI: `-Milan` or `-profile milan` enables Milan mode
- Config file: `test_milan_config.ini` (see below)
- Runtime: All parameters validated at startup and runtime

#### Example Milan Configuration (INI)
```ini
[ptp]
priority1 = 248
clockClass = 248
clockAccuracy = 0xFE
offsetScaledLogVariance = 0x4E5D
profile = milan
sync_interval = -3      # 125ms - MILAN REQUIRED
announce_interval = 0   # 1s - MILAN REQUIRED  
pdelay_interval = 0     # 1s - MILAN REQUIRED

[port]
neighborPropDelayThresh = 800000  # 800μs - MILAN REQUIRED
announceReceiptTimeout = 3
syncReceiptTimeout = 3
allowNegativeCorrectionField = 0
```


## Implementation Notes

### Core Milan Profile Features
- Fast convergence (<100ms) with immediate asCapable logic
- Enhanced BMCA (stream-aware, redundancy-ready)
- Strict jitter and path delay monitoring
- Milan-specific configuration structure and validation
- Command-line and config file activation
- Comprehensive logging and compliance reporting

### Version Differences (v1 vs v2)
- v2 adds video stream support, secure channels, expanded device classes, and optional/required redundancy
- v2 test suite and certification requirements are stricter
- v2 supports enhanced management (YANG models, diagnostics)

### Known Issues and Errata
- Stream-aware BMCA and full redundancy: framework ready, implementation ongoing
- All timing and clock requirements are enforced at runtime

### Hardware and Protocol References
- See insights for I210/I225/I219/I226 for register-level details
- Protocol implementation: `common/milan_profile.hpp`, `common/milan_profile.cpp`

### Test Requirements and Validation
- Milan compliance test suite: see `MILAN_PROFILE_TEST_GUIDE.md`
- Interoperability: multi-vendor and cross-profile validation
- Performance: sub-microsecond accuracy, fast convergence, low jitter


## References
- MILAN specifications (v1, v2)
- AVnu Alliance Milan Baseline Interoperability Specification v2.0a
- IEEE 802.1AS-2020
- OpenAvnu insights and test results
- `MILAN_PROFILE_REQUIREMENTS.md`, `MILAN_PROFILE_TEST_GUIDE.md`, `MILAN_BASELINE_INTEROPERABILITY_IMPLEMENTATION.md`

---

## Detailed Specification


### Device Classes and Capabilities
| Device Class | v1 | v2 |
|--------------|----|----|
| Talker       | Audio | Audio, Video, Redundancy, Secure |
| Listener     | Audio | Audio, Video, Redundancy, Secure |
| Bridge       | AVB/TSN | Enhanced Redundancy, Diagnostics |
| Controller   | - | Optional |


### Configuration Examples
#### Command Line
```bash
# Enable Milan profile
gptp.exe MAC_ADDRESS -Milan

# With configuration file
gptp.exe MAC_ADDRESS -M test_milan_config.ini

# Debug mode
gptp.exe MAC_ADDRESS -profile milan -debug-packets
```

#### INI File (see above)


### Version-Specific Implementation Differences
- v2 supports video streams, secure channels, and enhanced redundancy
- v2 test suite is stricter and includes new certification requirements
- v2 supports advanced management (YANG, diagnostics)
- v1 is audio-only, with basic redundancy and management

---

---

*This document is a living reference. Update as new versions or errata are published. For full details, see the referenced implementation, requirements, and test guide documents.*
