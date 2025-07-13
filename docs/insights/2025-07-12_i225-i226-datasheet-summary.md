# Insight: Intel I225/I226 Ethernet Controller Datasheet

**Context:**
Summary of Intel® Ethernet Controller I225/I226 Datasheet for OpenAvnu hardware and driver development.

**Observation:**
- Integrated MAC/PHY supporting 10BASE-Te, 100BASE-TX, 1000BASE-T, and 2500BASE-T.
- Supports IEEE 802.3u auto-negotiation, carrier extension, half/full duplex, and advanced diagnostics.
- Power and performance features: smart speed, power optimizer, integrated voltage regulator, jumbo frames, RSS, MSI-X, and multiple Tx/Rx queues.
- Advanced features: ECC in packet buffers, checksum offload, TSO, SCTP offload, PXE support, and SIPP.
- TSN/AVB features: IEEE 1588, 802.1AS-Rev, 802.1Qav, 802.1Qbu, 802.1Qbv, 802.3br, PCIe PTM for time sync.
- Package options for commercial and industrial temperature ranges.

**Root Cause:**
- Expanding requirements for TSN/AVB, diagnostics, and power management in modern Ethernet controllers.

**Resolution:**
- Use the datasheet for reference when designing, validating, or troubleshooting I225/I226-based systems.
- Leverage TSN/AVB features for time-sensitive networking applications.

**Takeaways:**
- Always check the latest datasheet for supported features and integration details.
- Document any field issues and cross-reference with official documentation.


**Reference:**
- `spec/2407151103_Intel-Altera-KTI226V-S-RKTU_C26159200.md` (Intel I225/I226 Datasheet)

---

## Register and Address Reference (TODO)

// Extract and summarize key register addresses, bitfields, and configuration options from the full datasheet as needed for development.
