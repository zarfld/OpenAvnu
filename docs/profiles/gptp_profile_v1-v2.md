# gPTP Profile Documentation (v1 vs v2)

**Profile:** Generalized Precision Time Protocol (gPTP, IEEE 802.1AS)

---

## Overview
- gPTP is used for precise time synchronization in AVB/TSN networks.
- OpenAvnu supports multiple gPTP profile versions (v1, v2).

## Version History and Differences
| Feature/Aspect         | gPTP v1 (IEEE 802.1AS-2011) | gPTP v2 (IEEE 802.1AS-2020) |
|-----------------------|-----------------------------|-----------------------------|
| Standard              | 802.1AS-2011                | 802.1AS-2020                |
| Time Sync Accuracy    | Sub-microsecond             | Improved, sub-microsecond   |
| Link Types            | Ethernet only               | Ethernet, Wi-Fi, others     |
| Path Delay Mechanism  | Peer delay only             | Peer & end-to-end           |
| Domain Number         | Single domain               | Multiple domains supported  |
| TLV Extensions        | Limited                     | Expanded, more flexible     |
| Security              | Not specified               | Optional, improved          |
| Management            | Basic                       | Enhanced, YANG models       |
| Interoperability      | v1 only                     | v1/v2 coexistence possible  |

## Profile Parameters (Consolidated)
- **Clock Class, Accuracy, Variance**: Table of supported values per version
- **Announce, Sync, Follow_Up, Pdelay_Req, Pdelay_Resp, etc.**: Message formats and fields
- **Configuration Options**: CLI, config file, and runtime options for each version

## Implementation Notes
- Reference to relevant hardware registers (see insights for I210/I225/I219/I226)
- Known issues and errata for each version
- Test requirements and validation procedures

## References
- IEEE 802.1AS-2011, IEEE 802.1AS-2020
- OpenAvnu insights and test results

---

## Detailed Specification

### Message Formats
// Add tables and diagrams for Announce, Sync, etc. (per version)

### Configuration Examples
// Add CLI/config file examples for v1 and v2

### Version-Specific Implementation Differences
// Highlight any code/config/behavior differences

---

*This document is a living reference. Update as new versions or errata are published.*
