/*
PCAP Stub Implementation for Development
This file provides stub definitions when WinPcap/Npcap SDK is not available
*/

#ifndef PCAP_STUB_H
#define PCAP_STUB_H

#ifdef PCAP_STUB

#include <windows.h>
#include <stdio.h>

// PCAP types and constants (simplified)
typedef struct pcap pcap_t;
typedef struct pcap_if pcap_if_t;
typedef struct bpf_program bpf_program;

struct pcap_pkthdr {
    struct timeval ts;
    unsigned int caplen;
    unsigned int len;
};

struct pcap_if {
    struct pcap_if *next;
    char *name;
    char *description;
    struct pcap_addr *addresses;
    unsigned int flags;
};

// PCAP constants
#define PCAP_ERRBUF_SIZE 256
#define PCAP_NETMASK_UNKNOWN 0xffffffff

// PCAP function prototypes (stubs)
int pcap_findalldevs(pcap_if_t **alldevsp, char *errbuf);
void pcap_freealldevs(pcap_if_t *alldevs);
pcap_t* pcap_open_live(const char *device, int snaplen, int promisc, int to_ms, char *errbuf);
void pcap_close(pcap_t *p);
int pcap_compile(pcap_t *p, struct bpf_program *fp, const char *str, int optimize, unsigned int netmask);
int pcap_setfilter(pcap_t *p, struct bpf_program *fp);
void pcap_freecode(struct bpf_program *fp);
int pcap_next_ex(pcap_t *p, struct pcap_pkthdr **pkt_header, const unsigned char **pkt_data);
char* pcap_geterr(pcap_t *p);

// Stub implementations
static pcap_if_t stub_interface = {
    NULL,                    // next
    "stub_interface",       // name  
    "PCAP Stub Interface",  // description
    NULL,                   // addresses
    0                       // flags
};

static char stub_error[PCAP_ERRBUF_SIZE] = "PCAP Stub - Development Only";
static unsigned char stub_packet[1500] = {0};
static struct pcap_pkthdr stub_header = {{0, 0}, sizeof(stub_packet), sizeof(stub_packet)};

int pcap_findalldevs(pcap_if_t **alldevsp, char *errbuf) {
    printf("PCAP STUB: pcap_findalldevs() called\n");
    if (alldevsp) *alldevsp = &stub_interface;
    return 0;
}

void pcap_freealldevs(pcap_if_t *alldevs) {
    printf("PCAP STUB: pcap_freealldevs() called\n");
}

pcap_t* pcap_open_live(const char *device, int snaplen, int promisc, int to_ms, char *errbuf) {
    printf("PCAP STUB: pcap_open_live(%s) called\n", device ? device : "NULL");
    return (pcap_t*)0x12345678; // Dummy handle
}

void pcap_close(pcap_t *p) {
    printf("PCAP STUB: pcap_close() called\n");
}

int pcap_compile(pcap_t *p, struct bpf_program *fp, const char *str, int optimize, unsigned int netmask) {
    printf("PCAP STUB: pcap_compile(%s) called\n", str ? str : "NULL");
    return 0;
}

int pcap_setfilter(pcap_t *p, struct bpf_program *fp) {
    printf("PCAP STUB: pcap_setfilter() called\n");
    return 0;
}

void pcap_freecode(struct bpf_program *fp) {
    printf("PCAP STUB: pcap_freecode() called\n");
}

int pcap_next_ex(pcap_t *p, struct pcap_pkthdr **pkt_header, const unsigned char **pkt_data) {
    // Return timeout (0) to simulate no packets
    return 0;
}

char* pcap_geterr(pcap_t *p) {
    return stub_error;
}

#endif // PCAP_STUB

#endif // PCAP_STUB_H
