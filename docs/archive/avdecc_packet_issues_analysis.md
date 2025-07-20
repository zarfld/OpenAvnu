# AVDECC ADP Packet Issues Analysis

## CRITICAL ISSUES FOUND:

### 1. **INCORRECT ADP PDU STRUCTURE** ❌
**Line 173:** `packet->adp_pdu.control_data_length = 56;`
**Problem:** This field should be `control_data_length` in **header** format, not as a direct byte value.

### 2. **MISSING IEEE 1722.1 AVTP HEADER** ❌
**Lines 32-37:** The `avdecc_common_header_t` is incomplete!
**Missing fields:**
- Stream ID (8 bytes)
- AVTP timestamp (4 bytes) 
- Gateway info (4 bytes)
- Stream data length (2 bytes)

### 3. **INCORRECT COMMON HEADER VALUES** ❌
**Line 166:** `packet->adp_pdu.common.sv_ver_mr_gv_tv = 0x00;`
**Should be:** 
- `sv` = 0 (stream valid)
- `ver` = 0 (version)
- `mr` = 0 (media reset) 
- `gv` = 1 (gateway valid for ADP)
- `tv` = 1 (timestamp valid)

### 4. **WRONG CONTROL DATA LENGTH CALCULATION** ❌
**Line 173:** `control_data_length = 56`
**Problem:** This should be calculated as:
- Total ADP PDU size - AVTP header size
- Must be in network byte order

### 5. **INCORRECT MESSAGE TYPE FIELD POSITION** ❌
**Line 171:** Message type directly after common header
**Problem:** Missing AVTP header fields between common header and ADP data

## CORRECT IEEE 1722.1 ADP PACKET STRUCTURE:

```
Ethernet Header (14 bytes)
├── dest_mac[6]
├── src_mac[6] 
└── ethertype (0x22F0)

AVTP Header (12 bytes)
├── subtype (0x7A)
├── sv_ver_mr_gv_tv
├── sequence_num
├── reserved_tu
├── stream_id[8]

ADP PDU (56 bytes)
├── message_type
├── valid_time
├── control_data_length 
├── entity_id[8]
├── entity_model_id[8]
├── entity_capabilities[4]
├── talker_stream_sources[2]
├── talker_capabilities[2]
├── listener_stream_sinks[2] 
├── listener_capabilities[2]
├── controller_capabilities[2]
├── available_index[4]
├── gptp_grandmaster_id[8]
├── gptp_domain_number[1]
├── reserved[3]
├── identify_control_index[4]
├── interface_index[4]
└── association_id[8]
```

## WHY HIVE-AVDECC DOESN'T SEE IT:
1. **Invalid AVTP header** - Hive rejects malformed packets
2. **Wrong control_data_length** - Field size calculation incorrect
3. **Missing stream ID** - Required for IEEE 1722.1 compliance
4. **Incorrect gateway/timestamp flags** - ADP requires specific flag values

## FIX REQUIRED:
Complete rewrite of packet structure with proper IEEE 1722.1 AVTP header format.
