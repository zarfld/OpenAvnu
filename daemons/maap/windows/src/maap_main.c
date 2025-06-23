/*******************************************************************************
Copyright (c) 2016-2017, Harman International Industries, Incorporated
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS LISTED "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS LISTED BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <pcap.h>
#include <io.h>
#include <fcntl.h>

#include "platform.h"
#include "maap.h"
#include "maap_packet.h"
#include "maap_parse.h"

#define MAAP_LOG_COMPONENT "Init"
#include "maap_log.h"

#define MAX_CLIENT_CONNECTIONS 32
#define DEFAULT_PORT           "15364"
#define VERSION_STR            "0.1"

static pcap_t *init_maap_networking(const char *iface,
                                    uint8_t src_mac[ETH_ALEN],
                                    uint8_t dest_mac[ETH_ALEN]);
static SOCKET get_listener_socket(const char *listenport);
static int act_as_client(const char *listenport);
static int act_as_server(const char *listenport, char *iface, int daemonize,
                        char *logfile);

static void log_print_notify_result(void *callback_data, int logLevel, const char *notifyText);
static void display_print_notify_result(void *callback_data, int logLevel, const char *notifyText);
static void send_print_notify_result(void *callback_data, int logLevel, const char *notifyText);

static const char *version_str =
        "maap_daemon v" VERSION_STR "\n"
        "Copyright (c) 2016-2017, Harman International Industries, Inc.\n";

static void usage(void)
{
        fprintf(stderr,
                "\n%s"
                "\n"
                "usage: maap_daemon [ -c | -i interface-name [-d log_file] ] [-p port_num]"
                "\n"
                "options:\n"
                "\t-c  Run as a client (sends commands to the daemon)\n"
                "\t-i  Run as a server monitoring the specified interface\n"
                "\t-d  Daemonize the server and log to log_file\n"
                "\t-p  Specify the control port to connect to (client) or\n"
                "\t    listen to (server).  The default port is " DEFAULT_PORT "\n"
                "\n",
                version_str);
        exit(1);
}

/* get sockaddr, IPv4 or IPv6 */
static void *get_in_addr(struct sockaddr *sa)
{
        if (sa->sa_family == AF_INET) {
                return &(((struct sockaddr_in*)sa)->sin_addr);
        }
        return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
        int as_client = 0, daemonize = 0;
        char *iface = NULL;
        char *listenport = NULL;
        char *logfile = NULL;
        int i;
        WSADATA wsaData;
        int ret;

        for (i = 1; i < argc; ++i) {
                if (argv[i][0] != '-') usage();
                switch (argv[i][1]) {
                case 'c':
                        as_client = 1;
                        break;
                case 'd':
                        if (++i >= argc) usage();
                        daemonize = 1;
                        logfile = _strdup(argv[i]);
                        break;
                case 'i':
                        if (++i >= argc) usage();
                        iface = _strdup(argv[i]);
                        break;
                case 'p':
                        if (++i >= argc) usage();
                        listenport = _strdup(argv[i]);
                        break;
                case 'h':
                default:
                        usage();
                        break;
                }
        }

        if (as_client && daemonize) {
                fprintf(stderr, "Cannot run as both a client and a daemon\n");
                usage();
        }
        if (!as_client && iface == NULL) {
                fprintf(stderr, "A network interface is required as a daemon\n");
                usage();
        }
        if (as_client && iface != NULL) {
                fprintf(stderr, "A network interface is not supported as a client\n");
                usage();
        }

        if (listenport == NULL) {
                listenport = _strdup(DEFAULT_PORT);
        }

        if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
                fprintf(stderr, "Failed to initialize Winsock\n");
                return -1;
        }

        maapLogInit();

        if (as_client) {
                ret = act_as_client(listenport);
        } else {
                ret = act_as_server(listenport, iface, daemonize, logfile);
        }

        maapLogExit();
        WSACleanup();
        free(listenport);
        free(logfile);
        return ret;
}

/* Initialize WinPcap and return a handle. */
static pcap_t *init_maap_networking(const char *iface,
                                    uint8_t src_mac[ETH_ALEN],
                                    uint8_t dest_mac[ETH_ALEN])
{
        pcap_if_t *alldevs, *d;
        char errbuf[PCAP_ERRBUF_SIZE];
        pcap_t *handle = NULL;
        struct bpf_program fcode;
        char filter[64];

        if (pcap_findalldevs(&alldevs, errbuf) == -1) {
                MAAP_LOGF_ERROR("pcap_findalldevs failed: %s", errbuf);
                return NULL;
        }

        for (d = alldevs; d; d = d->next) {
                if (strcmp(d->name, iface) == 0 ||
                    (d->description && strcmp(d->description, iface) == 0)) {
                        break;
                }
        }
        if (!d) {
                MAAP_LOG_ERROR("Interface not found");
                pcap_freealldevs(alldevs);
                return NULL;
        }

        handle = pcap_open_live(d->name, 65536, 1, 1, errbuf);
        if (!handle) {
                MAAP_LOGF_ERROR("Unable to open interface %s", d->name);
                pcap_freealldevs(alldevs);
                return NULL;
        }

        sprintf(filter, "ether proto 0x%04x", MAAP_TYPE);
        if (pcap_compile(handle, &fcode, filter, 1, 0) < 0 ||
            pcap_setfilter(handle, &fcode) < 0) {
                MAAP_LOG_ERROR("Unable to set pcap filter");
                pcap_close(handle);
                pcap_freealldevs(alldevs);
                return NULL;
        }

        /* Get MAC address */
        ULONG len = 0;
        IP_ADAPTER_INFO *info = NULL, *cur;
        if (GetAdaptersInfo(NULL, &len) == ERROR_BUFFER_OVERFLOW) {
                info = (IP_ADAPTER_INFO *)malloc(len);
                if (GetAdaptersInfo(info, &len) == NO_ERROR) {
                        for (cur = info; cur; cur = cur->Next) {
                                if (strstr(d->name, cur->AdapterName)) {
                                        memcpy(src_mac, cur->Address, ETH_ALEN);
                                        break;
                                }
                        }
                }
                free(info);
        }

        pcap_freealldevs(alldevs);
        (void)dest_mac;
        return handle;
}

/* Initialize the listener socket. */
static SOCKET get_listener_socket(const char *listenport)
{
        struct addrinfo hints, *ai, *p;
        SOCKET listener = INVALID_SOCKET;
        int yes = 1;
        int ret;

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;
        if ((ret = getaddrinfo(NULL, listenport, &hints, &ai)) != 0) {
                MAAP_LOGF_ERROR("getaddrinfo failure %s", gai_strerror(ret));
                return INVALID_SOCKET;
        }

        for (p = ai; p != NULL; p = p->ai_next) {
                listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
                if (listener == INVALID_SOCKET) {
                        continue;
                }
                setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(int));
                if (bind(listener, p->ai_addr, (int)p->ai_addrlen) == 0) {
                        break;
                }
                closesocket(listener);
                listener = INVALID_SOCKET;
        }

        freeaddrinfo(ai);
        if (p == NULL) {
                MAAP_LOG_ERROR("Socket failed to bind");
                return INVALID_SOCKET;
        }
        if (listen(listener, 10) != 0) {
                MAAP_LOG_ERROR("Socket listen error");
                closesocket(listener);
                return INVALID_SOCKET;
        }
        return listener;
}

/* Client side networking. */
static int act_as_client(const char *listenport)
{
        SOCKET socketfd;
        struct addrinfo hints, *ai, *p;
        int ret;

        fd_set master, read_fds;
        int fdmax;

        char recvbuffer[200];
        int recvbytes;
        Maap_Cmd recvcmd;
        int exit_received = 0;

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = 0;
        if ((ret = getaddrinfo("localhost", listenport, &hints, &ai)) != 0) {
                MAAP_LOGF_ERROR("getaddrinfo failure %s", gai_strerror(ret));
                return -1;
        }
        for (p = ai; p != NULL; p = p->ai_next) {
                socketfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
                if (socketfd == INVALID_SOCKET) continue;
                ret = connect(socketfd, p->ai_addr, (int)p->ai_addrlen);
                if (ret == 0) break;
                closesocket(socketfd);
        }
        if (p == NULL) {
                MAAP_LOG_ERROR("Unable to connect to the daemon");
                freeaddrinfo(ai);
                return -1;
        }
        freeaddrinfo(ai);

        u_long nb = 1;
        ioctlsocket(socketfd, FIONBIO, &nb);

        FD_ZERO(&read_fds);
        FD_ZERO(&master);
        HANDLE stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
        FD_SET(socketfd, &master);
        fdmax = (int)socketfd;

        puts("Client started");
        puts("Enter \"help\" for a list of valid commands.");

        while (!exit_received) {
                read_fds = master;
                ret = select(fdmax+1, &read_fds, NULL, NULL, NULL);
                if (ret <= 0) {
                        MAAP_LOG_ERROR("select() error");
                        break;
                }

                if (FD_ISSET(socketfd, &read_fds)) {
                        while ((recvbytes = recv(socketfd, recvbuffer, sizeof(Maap_Notify), 0)) > 0) {
                                recvbuffer[recvbytes] = '\0';
                                if (recvbytes == sizeof(Maap_Notify)) {
                                        print_notify((Maap_Notify *)recvbuffer, display_print_notify_result, NULL);
                                }
                        }
                        if (recvbytes == 0) {
                                MAAP_LOG_INFO("MAAP daemon exited.  Closing application.");
                                exit_received = 1;
                        }
                }
                if (WaitForSingleObject(stdin_handle, 0) == WAIT_OBJECT_0) {
                        recvbytes = (int)_read(_fileno(stdin), recvbuffer, sizeof(recvbuffer)-1);
                        if (recvbytes > 0) {
                                Maap_Cmd *bufcmd = (Maap_Cmd *)recvbuffer;
                                int rv = 0;
                                recvbuffer[recvbytes] = '\0';
                                switch (bufcmd->kind) {
                                case MAAP_CMD_INIT:
                                case MAAP_CMD_RESERVE:
                                case MAAP_CMD_RELEASE:
                                case MAAP_CMD_STATUS:
                                case MAAP_CMD_YIELD:
                                case MAAP_CMD_EXIT:
                                        memcpy(&recvcmd, bufcmd, sizeof(Maap_Cmd));
                                        rv = 1;
                                        break;
                                default:
                                        memset(&recvcmd, 0, sizeof(Maap_Cmd));
                                        rv = parse_text_cmd(recvbuffer, &recvcmd);
                                        if (!rv) {
                                                if (strncmp(recvbuffer, "help", 4) != 0) {
                                                        puts("Invalid command type");
                                                }
                                                parse_usage(display_print_notify_result, NULL);
                                        }
                                        break;
                                }
                                if (rv) {
                                        if (send(socketfd, (char *)&recvcmd, sizeof(Maap_Cmd), 0) < 0) {
                                                MAAP_LOG_ERROR("Error writing to network socket");
                                                break;
                                        }
                                }
                        }
                }
        }
        closesocket(socketfd);
        return (exit_received ? 0 : -1);
}

/* Server side networking. */
static int act_as_server(const char *listenport, char *iface, int daemonize,
                         char *logfile)
{
        Maap_Client mc;
        uint8_t dest_mac[ETH_ALEN] = MAAP_DEST_MAC;
        uint8_t src_mac[ETH_ALEN];
        pcap_t *pcap_handle;
        SOCKET listener;
        SOCKET clientfd[MAX_CLIENT_CONNECTIONS];
        int client_wants_text[MAX_CLIENT_CONNECTIONS];
        int i, nextclientindex;
        fd_set master, read_fds;
        int fdmax;
        void *packet_data;
        int64_t waittime;
        struct timeval tv;
        char recvbuffer[1600];
        int recvbytes;
        Maap_Cmd recvcmd;
        Maap_Notify recvnotify;
        uintptr_t notifysocket;
        int exit_received = 0;
        int ret;
        HANDLE log_handle = INVALID_HANDLE_VALUE;
        int log_fd = -1;

        pcap_handle = init_maap_networking(iface, src_mac, dest_mac);
        if (!pcap_handle) {
                return -1;
        }
        free(iface);
        iface = NULL;

        if (daemonize && logfile) {
                FreeConsole();
                log_handle = CreateFileA(logfile, FILE_APPEND_DATA,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                                        NULL, OPEN_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL, NULL);
                if (log_handle != INVALID_HANDLE_VALUE) {
                        SetFilePointer(log_handle, 0, NULL, FILE_END);
                        log_fd = _open_osfhandle((intptr_t)log_handle,
                                                _O_APPEND | _O_WRONLY);
                        if (log_fd != -1) {
                                _dup2(log_fd, _fileno(stderr));
                                SetStdHandle(STD_ERROR_HANDLE,
                                            (HANDLE)_get_osfhandle(_fileno(stderr)));
                                _close(log_fd);
                                log_handle = INVALID_HANDLE_VALUE;
                        } else {
                                CloseHandle(log_handle);
                                log_handle = INVALID_HANDLE_VALUE;
                        }
                }
        }


        FD_ZERO(&read_fds);
        FD_ZERO(&master);
        HANDLE stdin_handle = NULL;
        if (!daemonize) {
                stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
        }
        fdmax = 0;

        listener = get_listener_socket(listenport);
        if (listener == INVALID_SOCKET) {
                pcap_close(pcap_handle);
                return -1;
        }
        FD_SET(listener, &master);
        if ((int)listener > fdmax) fdmax = (int)listener;

        for (i = 0; i < MAX_CLIENT_CONNECTIONS; ++i) {
                clientfd[i] = INVALID_SOCKET;
                client_wants_text[i] = 0;
        }
        nextclientindex = 0;

        memset(&mc, 0, sizeof(mc));
        mc.dest_mac = convert_mac_address(dest_mac);
        mc.src_mac  = convert_mac_address(src_mac);

        srand((unsigned int)mc.src_mac + (unsigned int)time(NULL));

        MAAP_LOG_STATUS("Server started");
        if (!daemonize) {
                puts("Enter \"help\" for a list of valid commands.");
        }

        while (!exit_received) {
                while (mc.net != NULL && (packet_data = Net_getNextQueuedPacket(mc.net)) != NULL) {
                        if (pcap_sendpacket(pcap_handle, packet_data, MAAP_NET_BUFFER_SIZE) != 0) {
                                MAAP_LOG_ERROR("Error writing to network");
                                Net_freeQueuedPacket(mc.net, packet_data);
                                break;
                        }
                        Net_freeQueuedPacket(mc.net, packet_data);
                }

                while (get_notify(&mc, (void *)&notifysocket, &recvnotify) > 0) {
                        if ((int)notifysocket == -1) {
                                print_notify(&recvnotify, display_print_notify_result, NULL);
                        } else {
                                print_notify(&recvnotify, log_print_notify_result, NULL);
                                for (i = 0; i < MAX_CLIENT_CONNECTIONS; ++i) {
                                        if (clientfd[i] == (SOCKET)notifysocket) {
                                                if (client_wants_text[i]) {
                                                        print_notify(&recvnotify, send_print_notify_result, (void *)&clientfd[i]);
                                                } else {
                                                        send(clientfd[i], (char *)&recvnotify, sizeof(recvnotify), 0);
                                                }
                                                break;
                                        }
                                }
                        }
                }

                waittime = maap_get_delay_to_next_timer(&mc);
                if (waittime > 0) {
                        tv.tv_sec = (long)(waittime / 1000000000);
                        tv.tv_usec = (long)((waittime % 1000000000) / 1000);
                } else {
                        tv.tv_sec = 0; tv.tv_usec = 0;
                }

                read_fds = master;
                ret = select(fdmax+1, &read_fds, NULL, NULL, &tv);
                if (ret < 0) {
                        MAAP_LOG_ERROR("select() error");
                        break;
                }
                if (ret == 0) {
                        maap_handle_timer(&mc);
                        continue;
                }

                /* Poll pcap for packets */
                do {
                        const u_char *pkt;
                        struct pcap_pkthdr *hdr;
                        ret = pcap_next_ex(pcap_handle, &hdr, &pkt);
                        if (ret > 0) {
                                maap_handle_packet(&mc, (uint8_t *)pkt, hdr->len);
                        }
                } while (ret > 0);

                if (FD_ISSET(listener, &read_fds)) {
                        SOCKET newfd;
                        struct sockaddr_storage remoteaddr;
                        int addrlen = sizeof remoteaddr;
                        char remoteIP[INET6_ADDRSTRLEN];
                        newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
                        if (newfd == INVALID_SOCKET) {
                                MAAP_LOG_ERROR("accept failed");
                        } else {
                                MAAP_LOGF_INFO("New connection from %s on socket %d",
                                        inet_ntop(remoteaddr.ss_family,
                                                get_in_addr((struct sockaddr*)&remoteaddr),
                                                remoteIP, INET6_ADDRSTRLEN),
                                        (int)newfd);
                                if (clientfd[nextclientindex] != INVALID_SOCKET) {
                                        for (i = (nextclientindex + 1) % MAX_CLIENT_CONNECTIONS;
                                             i != nextclientindex; i = (i + 1) % MAX_CLIENT_CONNECTIONS) {
                                                if (clientfd[i] == INVALID_SOCKET) break;
                                        }
                                        if (i == nextclientindex) {
                                                MAAP_LOG_ERROR("Out of client connection slots. Connection rejected.");
                                                closesocket(newfd);
                                                newfd = INVALID_SOCKET;
                                        }
                                }
                                if (newfd != INVALID_SOCKET) {
                                        clientfd[nextclientindex] = newfd;
                                        nextclientindex = (nextclientindex + 1) % MAX_CLIENT_CONNECTIONS;
                                        FD_SET(newfd, &master);
                                        if ((int)newfd > fdmax) fdmax = (int)newfd;
                                }
                        }
                }

                if (!daemonize && WaitForSingleObject(stdin_handle, 0) == WAIT_OBJECT_0) {
                        recvbytes = (int)_read(_fileno(stdin), recvbuffer, sizeof(recvbuffer)-1);
                        if (recvbytes > 0) {
                                recvbuffer[recvbytes] = '\0';
                                switch (((Maap_Cmd *)recvbuffer)->kind) {
                                case MAAP_CMD_INIT:
                                case MAAP_CMD_RESERVE:
                                case MAAP_CMD_RELEASE:
                                case MAAP_CMD_STATUS:
                                case MAAP_CMD_YIELD:
                                case MAAP_CMD_EXIT:
                                        memcpy(&recvcmd, recvbuffer, sizeof(Maap_Cmd));
                                        break;
                                default:
                                        if (!parse_text_cmd(recvbuffer, &recvcmd)) {
                                                if (strncmp(recvbuffer, "help", 4) != 0) {
                                                        puts("Invalid command type");
                                                }
                                                parse_usage(display_print_notify_result, NULL);
                                                continue;
                                        }
                                        break;
                                }
                                if (parse_write(&mc, (const void *)(uintptr_t)-1, (char *)&recvcmd, NULL) == 1) {
                                        exit_received = 1;
                                }
                        }
                }

                for (i = 0; i < MAX_CLIENT_CONNECTIONS; ++i) {
                        if (clientfd[i] != INVALID_SOCKET && FD_ISSET(clientfd[i], &read_fds)) {
                                recvbytes = recv(clientfd[i], recvbuffer, sizeof(recvbuffer)-1, 0);
                                if (recvbytes <= 0) {
                                        closesocket(clientfd[i]);
                                        FD_CLR(clientfd[i], &master);
                                        clientfd[i] = INVALID_SOCKET;
                                } else {
                                        recvbuffer[recvbytes] = '\0';
                                        if (strncmp(recvbuffer, "text", 4) == 0) {
                                                client_wants_text[i] = 1;
                                        } else {
                                                if (parse_write(&mc, (const void *)(uintptr_t)clientfd[i], recvbuffer, NULL) == 1) {
                                                        exit_received = 1;
                                                }
                                        }
                                }
                        }
                }
        }

        pcap_close(pcap_handle);
        closesocket(listener);
        for (i = 0; i < MAX_CLIENT_CONNECTIONS; ++i) {
                if (clientfd[i] != INVALID_SOCKET) closesocket(clientfd[i]);
        }
        if (daemonize && logfile) {
                fflush(stderr);
                _close(_fileno(stderr));
        }
        return 0;
}

static void log_print_notify_result(void *callback_data, int logLevel, const char *notifyText)
{
        switch (logLevel) {
        case MAAP_LOG_LEVEL_ERROR:
                MAAP_LOG_ERROR(notifyText);
                break;
        case MAAP_LOG_LEVEL_WARNING:
                MAAP_LOG_WARNING(notifyText);
                break;
        case MAAP_LOG_LEVEL_INFO:
                MAAP_LOG_INFO(notifyText);
                break;
        case MAAP_LOG_LEVEL_STATUS:
                MAAP_LOG_STATUS(notifyText);
                break;
        case MAAP_LOG_LEVEL_DEBUG:
                MAAP_LOG_DEBUG(notifyText);
                break;
        case MAAP_LOG_LEVEL_VERBOSE:
                MAAP_LOG_VERBOSE(notifyText);
                break;
        }
}

static void format_print_notify_result(int logLevel, const char *notifyText, char *szOutputText)
{
        int i, nLastSpace;
        int nInitial = -1;
        char *pszOut = szOutputText;

        while (*notifyText) {
                if (nInitial < 0) {
                        if (logLevel == MAAP_LOG_LEVEL_ERROR) {
                                strcpy(pszOut, "Error:  ");
                                nInitial = (int) strlen(pszOut);
                                pszOut += nInitial;
                        } else if (logLevel == MAAP_LOG_LEVEL_WARNING) {
                                strcpy(pszOut, "Warning:  ");
                                nInitial = (int) strlen(pszOut);
                                pszOut += nInitial;
                        } else {
                                nInitial = 0;
                        }
                } else {
                        nInitial = 0;
                }

                nLastSpace = -1;
                for (i = 0; (i < MAAP_LOG_STDOUT_CONSOLE_WIDTH - nInitial || nLastSpace <= 0) && notifyText[i]; ++i) {
                        if (isspace((unsigned char)notifyText[i])) nLastSpace = i;
                }
                if (notifyText[i] == '\0') {
                        strcpy(pszOut, notifyText);
                        pszOut += strlen(pszOut);
                        *pszOut++ = '\r';
                        *pszOut++ = '\n';
                        break;
                }

                for (i = 0; i < nLastSpace; ++i) {
                        *pszOut++ = *notifyText++;
                }
                *pszOut++ = '\r';
                *pszOut++ = '\n';

                while (isspace((unsigned char)*notifyText)) notifyText++;
        }

        *pszOut = '\0';
}

static void display_print_notify_result(void *callback_data, int logLevel, const char *notifyText)
{
        char szOutputText[300];
        format_print_notify_result(logLevel, notifyText, szOutputText);
        fputs(szOutputText, stdout);
        fflush(stdout);
}

static void send_print_notify_result(void *callback_data, int logLevel, const char *notifyText)
{
        char szOutputText[300];
        format_print_notify_result(logLevel, notifyText, szOutputText);
        send(*(SOCKET *)callback_data, szOutputText, (int)strlen(szOutputText), 0);
}
