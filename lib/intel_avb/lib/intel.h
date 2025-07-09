/******************************************************************************

  Copyright (c) 2025, Intel Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

   3. Neither the name of the Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

******************************************************************************/

#ifndef _INTEL_H_DEFINED_
#define _INTEL_H_DEFINED_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <stdint.h>
#include <time.h>

/* Define clockid_t if not available */
#ifndef _WIN32
#include <unistd.h>
#else
typedef int clockid_t;
#endif

/* Intel Vendor ID */
#define INTEL_VENDOR_ID 0x8086

/* Device capability flags */
#define INTEL_CAP_BASIC_1588    (1 << 0)  /* Basic IEEE 1588 */
#define INTEL_CAP_ENHANCED_TS   (1 << 1)  /* Enhanced timestamping */
#define INTEL_CAP_TSN_TAS       (1 << 2)  /* Time Aware Shaper (802.1Qbv) */
#define INTEL_CAP_TSN_FP        (1 << 3)  /* Frame Preemption (802.1Qbu) */
#define INTEL_CAP_PCIe_PTM      (1 << 4)  /* PCIe Precision Time Measurement */
#define INTEL_CAP_2_5G          (1 << 5)  /* 2.5 Gbps support */
#define INTEL_CAP_MDIO          (1 << 6)  /* MDIO register access */
#define INTEL_CAP_MMIO          (1 << 7)  /* Memory-mapped I/O */
#define INTEL_CAP_EEE           (1 << 8)  /* Energy Efficient Ethernet */

/* Intel device types */
typedef enum {
    INTEL_DEVICE_I210,
    INTEL_DEVICE_I219,
    INTEL_DEVICE_I225,
    INTEL_DEVICE_I226,
    INTEL_DEVICE_UNKNOWN
} intel_device_type_t;

/* Resource mapping structure */
struct resource {
    uint64_t paddr;
    uint32_t mmap_size;
};

/* Intel packet structure for timed transmission */
#define INTEL_PACKET_LAUNCHTIME   1   /* control when packet transmitted */
#define INTEL_PACKET_LATCHTIME    2   /* grab a timestamp of transmission */

struct intel_packet {
    struct resource map;        /* bus_dma map for packet */
    unsigned int offset;        /* offset into physical page */
    void *vaddr;
    uint32_t len;
    uint32_t flags;
    uint64_t attime;          /* launchtime */
    uint64_t dmatime;         /* when dma tx desc wb*/
    struct intel_packet *next;  /* used in the clean routine */
};

/* Device structure */
typedef struct _device_t {
    void *private_data;
    uint16_t pci_vendor_id;
    uint16_t pci_device_id;
    uint16_t domain;
    uint8_t bus;
    uint8_t dev;
    uint8_t func;
    intel_device_type_t device_type;
    uint32_t capabilities;
} device_t;

/* DMA allocation structure */
struct intel_dma_alloc {
    uint64_t dma_paddr;
    void *dma_vaddr;
    unsigned int mmap_size;
};

/* TSN Configuration structures */
struct tsn_tas_config {
    uint64_t base_time_s;      /* Base time seconds */
    uint32_t base_time_ns;     /* Base time nanoseconds */
    uint32_t cycle_time_s;     /* Cycle time seconds */
    uint32_t cycle_time_ns;    /* Cycle time nanoseconds */
    uint8_t gate_states[8];    /* Gate states for each traffic class */
    uint32_t gate_durations[8]; /* Duration for each gate state */
};

struct tsn_fp_config {
    uint8_t preemptable_queues; /* Bitfield of preemptable queues */
    uint32_t min_fragment_size; /* Minimum fragment size */
    uint8_t verify_disable;     /* Disable verification */
};

/* PTM Configuration */
struct ptm_config {
    uint8_t enabled;           /* PTM enable/disable */
    uint32_t clock_granularity; /* Clock granularity */
};

/* Core API Functions */
int intel_probe(device_t *dev);
int intel_attach(char *dev_path, device_t *pdev);
int intel_attach_rx(device_t *pdev);
int intel_attach_tx(device_t *pdev);
int intel_detach(device_t *dev);
int intel_init(device_t *dev);

/* Memory Management */
int intel_dma_malloc_page(device_t *dev, struct intel_dma_alloc *page);
void intel_dma_free_page(device_t *dev, struct intel_dma_alloc *page);

/* Packet Operations */
int intel_xmit(device_t *dev, unsigned int queue_index, struct intel_packet *packet);
int intel_refresh_buffers(device_t *dev, uint32_t idx,
                         struct intel_packet **rxbuf_packets, uint32_t num_bufs);
int intel_receive(device_t *dev, unsigned int queue_index, 
                 struct intel_packet **received_packets, uint32_t *count);
void intel_clean(device_t *dev, struct intel_packet **cleaned_packets);

/* Time and Clock Functions */
int intel_get_wallclock(device_t *dev, uint64_t *curtime, uint64_t *rdtsc);
int intel_gettime(device_t *dev, clockid_t clk_id, uint64_t *curtime,
                 struct timespec *system_time);
int intel_get_systime(device_t *dev, uint64_t *systime);
int intel_set_systime(device_t *dev, uint64_t systime);
int intel_adjust_systime(device_t *dev, int32_t ppb);

/* TSN Functions (I225/I226 specific) */
int intel_setup_time_aware_shaper(device_t *dev, struct tsn_tas_config *config);
int intel_setup_frame_preemption(device_t *dev, struct tsn_fp_config *config);
int intel_setup_ptm(device_t *dev, struct ptm_config *config);

/* Register Access Functions */
int intel_read_reg(device_t *dev, uint32_t offset, uint32_t *value);
int intel_write_reg(device_t *dev, uint32_t offset, uint32_t value);

/* MDIO Functions (I219 specific) */
int intel_mdio_read(device_t *dev, uint32_t page, uint32_t reg, uint16_t *value);
int intel_mdio_write(device_t *dev, uint32_t page, uint32_t reg, uint16_t value);

/* Device Information */
int intel_get_device_info(device_t *dev, char *info_buffer, size_t buffer_size);
uint32_t intel_get_capabilities(device_t *dev);
const char *intel_get_device_name(device_t *dev);
int intel_has_capability(device_t *dev, uint32_t capability);

#ifdef __cplusplus
}
#endif

#endif /* _INTEL_H_DEFINED_ */
