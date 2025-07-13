# ProAV AVB/TSN Profile Documentation

**Profile:** ProAV (Professional Audio/Video)

---

## Overview
- ProAV AVB/TSN profiles define requirements for professional audio/video networking in live sound, broadcast, and studio environments.
- Specifies required protocols, device classes, and interoperability requirements for ProAV use cases.

## Profile Features and Requirements
| Feature/Aspect         | ProAV Profile                    |
|-----------------------|----------------------------------|
| Protocol Baseline     | AVB/TSN (IEEE 802.1AS, 1722, 1722.1, etc.) |
| Device Classes        | Mixer, Speaker, Camera, Switch   |
| Stream Format         | Audio, video, control            |
| Redundancy            | Optional/required (per class)    |
| Certification         | ProAV test suite, interoperability|
| Management            | ProAV diagnostics, YANG, etc.    |
| Security              | Optional (MACsec, VLAN, etc.)    |

## Profile Parameters (Consolidated)
- **Required Protocols:** gPTP, AVTP, SRP, MAAP, IEEE 1722.1, etc.
- **Device Classes:** Table of supported classes and capabilities
- **Configuration Options:** ProAV-specific config and diagnostics

## Implementation Notes
- Reference to relevant hardware and protocol documentation
- Known issues and errata for ProAV use
- Test requirements and validation procedures

## References
- ProAV AVB/TSN specifications
- OpenAvnu insights and test results

---

## Detailed Specification

### Device Classes and Capabilities
// Add tables for Mixer, Speaker, Camera, Switch, etc.

### Configuration Examples
// Add config/diagnostic examples

### ProAV-Specific Implementation Notes
// Highlight any code/config/behavior differences

---

*This document is a living reference. Update as new versions or errata are published.*
