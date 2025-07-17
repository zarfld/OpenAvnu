#include <cstdint>
#include <iostream>

namespace IEEE{
namespace _1722_1{
namespace _2021{

    /*
    ATDECCEntitydiscoveryistheprocessinwhichATDECCControllersidentifyalloftheATDECCEntities
    currently availableonanetworkandidentifythemastheyareaddedtoorremovedfromthenetwork.The
    ATDECCDiscoveryProtocol(ADP)isusedforthispurpose.
    ADP isaprotocolbasedonIEEEStd17222016controlAVTPDUsallowingATDECCEntitiestobe
    discovered byeachother.ADPusesthreemessagetypestodothefollowing:
    a) Announce thatanATDECCEntityisavailable.
    b) Announce thatanATDECCEntityisdeparting.
    c) Discover oneoralloftheATDECCEntitiesonthenetwork.
    */
namespace ADPDU{
    struct ADPDU {
        /*The ADPprotocolisimplementedthroughthreestatemachines,anAdvertisingEntityStatemachineforeach
        ATDECCEntitybeingpublishedontheEndStation,anAdvertisingInterfaceStatemachineforeachAVB
        interface oftheATDECCEntitybeingpublishedintheEndStationandaDiscoveryStatemachineforeach
        ATDECCEntityimplementinganATDECCControllerorrequiringEntitydiscovery.
        There isoneinstanceoftheDiscoveryStatemachineforeachAVBInterfaceoftheATDECCEntity
        implementing discovery.TheATDECCEntitymayprovideitsowncoordinationbetweenthediscoverystate
        machines totrackATDECCEntitieswhichappearonmultipleinterfaces.*/
        const uint8_t AVDECC_MULTICAST_MAC[6] = {0x91, 0xE0, 0xF0, 0x01, 0x00, 0x00};
        const uint16_t AVDECC_ETHERTYPE = 0x22F0; // AVDECC Ethertype


        enum ADP_Message_type : uint8_t {
            ADP_ENTITY_AVAILABLE = 0x00,
            ADP_ENTITY_DEPARTING = 0x01,
            ADP_ENTITY_DISCOVERY_REQUEST = 0x02
        };

        enum class EntityCapabilities : uint32_t {
            None                                 = 0x00000000,
            EFU_MODE                             = 0x80000000, // Bit 31
            ADDRESS_ACCESS_SUPPORTED             = 0x40000000, // Bit 30
            GATEWAY_ENTITY                       = 0x20000000, // Bit 29
            AEM_SUPPORTED                        = 0x10000000, // Bit 28
            LEGACY_AVC                           = 0x08000000, // Bit 27
            ASSOCIATION_ID_SUPPORTED            = 0x04000000, // Bit 26
            ASSOCIATION_ID_VALID                = 0x02000000, // Bit 25
            VENDOR_UNIQUE_SUPPORTED             = 0x01000000, // Bit 24
            CLASS_A_SUPPORTED                   = 0x00800000, // Bit 23
            CLASS_B_SUPPORTED                   = 0x00400000, // Bit 22
            GPTP_SUPPORTED                      = 0x00200000, // Bit 21
            AEM_AUTHENTICATION_SUPPORTED        = 0x00100000, // Bit 20
            AEM_AUTHENTICATION_REQUIRED         = 0x00080000, // Bit 19
            AEM_PERSISTENT_ACQUIRE_SUPPORTED    = 0x00040000, // Bit 18
            AEM_IDENTIFY_CONTROL_INDEX_VALID    = 0x00020000, // Bit 17
            AEM_INTERFACE_INDEX_VALID           = 0x00010000, // Bit 16
            GENERAL_CONTROLLER_IGNORE           = 0x00008000, // Bit 15
            ENTITY_NOT_READY                    = 0x00004000, // Bit 14
            ACMP_ACQUIRE_WITH_AEM               = 0x00002000, // Bit 13
            ACMP_AUTHENTICATE_WITH_AEM          = 0x00001000, // Bit 12
            SUPPORTS_UDPv4_ATDECC               = 0x00000800, // Bit 11
            SUPPORTS_UDPv4_STREAMING            = 0x00000400, // Bit 10
            SUPPORTS_UDPv6_ATDECC               = 0x00000200, // Bit 9
            SUPPORTS_UDPv6_STREAMING            = 0x00000100, // Bit 8
            MULTIPLE_PTP_INSTANCES              = 0x00000080, // Bit 7
            AEM_CONFIGURATION_INDEX_VALID       = 0x00000040, // Bit 6
            // Bits 0–5: Reserved
        };

        enum class TalkerCapabilities : uint16_t {
            None               = 0x0000,
            VIDEO_SOURCE       = 0x0001, // Bit 0
            AUDIO_SOURCE       = 0x0002, // Bit 1
            MIDI_SOURCE        = 0x0004, // Bit 2
            SMPTE_SOURCE       = 0x0008, // Bit 3
            MEDIA_CLOCK_SOURCE = 0x0010, // Bit 4
            CONTROL_SOURCE     = 0x0020, // Bit 5
            OTHER_SOURCE       = 0x0040, // Bit 6
            IMPLEMENTED        = 0x8000  // Bit 15
            // Bits 7–14 are reserved
        };

        enum class ListenerCapabilities : uint16_t {
            None               = 0x0000,
            VIDEO_SINK         = 0x0001, // Bit 0
            AUDIO_SINK         = 0x0002, // Bit 1
            MIDI_SINK          = 0x0004, // Bit 2
            SMPTE_SINK         = 0x0008, // Bit 3
            MEDIA_CLOCK_SINK   = 0x0010, // Bit 4
            CONTROL_SINK       = 0x0020, // Bit 5
            OTHER_SINK         = 0x0040, // Bit 6
            IMPLEMENTED        = 0x8000  // Bit 15
            // Bits 7–14 are reserved
        };

        enum class ControllerCapabilities : uint32_t {
            None         = 0x00000000,
            IMPLEMENTED  = 0x00000001  // Bit 0
            // Bits 1–31 are reserved for future use
        };
        /*ATDECCDiscoveryProtocolDataUnit(ADPDU)isusedtocommunicatebetweenATDECCEntitiesand
        ATDECCControllers.TheADPDUisdefinedinIEEEStd17222016andcontainsfieldsforentity
        identification,capabilities,andstream information.*/

        /*ATDECCDiscoveryProtocolPDU 
        The ATDECCDiscoveryProtocolDataUnit(ADPDU)followstheIEEEStd17222016alternativeAVTPDU
        header.
        The ADPDUcontainsthefollowingfields:
        — subtype: 1octet
        — h (header specific):1bit
        — version: 3bits
        — message_type: 4bits
        — valid_time: 5bits
        — control_data_length: 11bits
        — entity_id: 64bits
        — entity_model_id: 64bits
        — entity_capabilities: 32bits
        — talker_stream_sources: 16bits
        — talker_capabilities: 16bits
        — listener_stream_sinks: 16bits
        — listener_capabilities: 16bits
        — controller_capabilities: 32bits
        — available_index: 32bits
        — gptp_grandmaster_id: 64bits
        — gptp_domain_number: 8bits
        — reserved0: 8bits
        — current_configuration_index: 16bits
        — identify_control_index: 16bits
        — interface_index: 16bits
        — association_id: 64bits
        — reserved1: 32bits
        */
       class ATDECCDiscoveryProtocolPDU{

        private:
        uint8_t raw_octets[68]; // 56 octets after entity_id (12+56)

        public:
            /**
 * @brief The IEEE Std 1722-2016 common header field subtype is set to the ADP subtype octet.
 */
uint8_t subtype; // 1 octet

/**
 * @brief The IEEE Std 1722-2016 common header h (header specific) bit is set to zero (0).
 */
bool h; // 1 bit

/**
 * @brief The IEEE Std 1722-2016 common header version field is set to zero (0).
 */
uint8_t version; // 3 bits

/**
 * @brief The IEEE Std 1722-2016 common header message_type field is set to ADP message_type.
 */
ADP_Message_type message_type; // 4 bits

/**
 * @brief Indicates how long the record will be valid for in two-second increments.
 * For ENTITY_AVAILABLE messages, this field is set to the ATDECC Entity's valid_time period.
 * For all other messages, this field is set to zero (0).
 */
uint16_t valid_time; // 5 bits

/**
 * @brief The length in octets of the PDU following the entity_id field. This is set to 56.
 */
uint16_t control_data_length; // 11 bits

/**
 * @brief The EUI64 identifier of the ATDECC Entity. Must be unique on the network.
        };
        
uint64_t entity_id; // 64 bits

/**
 * @brief EUI64 used to identify an ATDECC Entity data model from a vendor.
 * If the data model changes, a new unique entity_model_id must be used.
 */
uint64_t entity_model_id; // 64 bits


            EntityCapabilities entity_capabilities; // 32 bits

            /*
            The talker_stream_sources field isusedtoidentifythemaximumnumberofStreamsanATDECCTalker
            is capableofsourcingsimultaneously.Thisvalueshouldnotbeusedasthecurrentnumberofstream
            sources. ThecurrentnumberofstreamsourcesvalueisobtainedfromtheEntity’sdescription.For
            example, forAEM,usethenumberofSTREAM_OUTPUTdescriptorsinthecurrentCONFIGURATION
            descriptor; see 7.2.1 and 7.2.2.
            */
            uint16_t talker_stream_sources; // 16 bits

            /*
            The talker_capabilities field isa16bitbitfieldusedtoidentifythecapabilitiesofanATDECCTalker.This
            field issettoacombinationofthevalidbitsasdefinedby Table63.
            */
            TalkerCapabilities talker_capabilities; // 16 bits
            
            /*The listener_stream_sinks field isusedtoidentifythemaximumnumberofStreamsanATDECCListeneris
            capable ofsinkingsimultaneously.Thisvalueshouldnotbeusedasthecurrentnumberofstreamsinks.The
            current numberofstreamsinksvalueisobtainedfromtheEntity’sdescription.Forexample,forAEM,usethe
            number ofSTREAM_INPUTdescriptorsinthecurrentCONFIGURATIONdescriptor;see 7.2.1 and 7.2.2.*/
            uint16_t listener_stream_sinks; // 16 bits

            /*The listener_capabilities field isa16bitbitfieldusedtoidentifythecapabilitiesofanATDECCListener.
            This fieldissettoacombinationofthevalidbitsasdefinedby*/
            ListenerCapabilities listener_capabilities; // 16 bits

           /* The controller_capabilities field isusedtoidentifytheATDECCControllercapabilitiesofanATDECC
            Entity.Thisisabitfield,withthelowestbitindicatingthattheATDECCEntityimplementsanATDECC
            Controller.Thisfieldissettoacombinationofthevalidbits.*/
            ControllerCapabilities controller_capabilities; // 32 bits

            // constructor to initialize the PDU with raw data
            ATDECCDiscoveryProtocolPDU(uint8_t(68) data) {
                memcpy(raw_octets, data, sizeof(raw_octets));
            }

            // receive the raw octets
            uint8_t* get_raw_octets() {
                return raw_octets;
            }
        };



 
    }; // struct APPU
}
    /** ATDECCEntityModel */
    namespace AEM {


       /**
         * @brief ENTITY Descriptor for IEEE 1722.1-2021.
         *
         * This descriptor provides information about the ATDECC Entity, including its identity, capabilities, and configuration.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct EntityDescriptor {
            /**
             * @brief Unique identifier for the ATDECC Entity (EUI64).
             */
            uint64_t entity_id;

            /**
             * @brief Model identifier for the ATDECC Entity (EUI64).
             */
            uint64_t entity_model_id;

            /**
             * @brief Bitfield indicating the capabilities of the ATDECC Entity.
             */
            EntityCapabilities entity_capabilities;

            /**
             * @brief Current value of the available_index field, used for tracking changes in the entity's availability.
             */
            uint32_t available_index;

            /**
             * @brief Association ID for grouping related entities (EUI64).
             */
            uint64_t association_id;

            /**
             * @brief Current configuration index for the entity.
             */
            uint16_t current_configuration;

            /**
             * @brief Human-readable name for the entity.
             */
            char entity_name[64]; // Size per standard, adjust as needed

            /**
             * @brief Firmware version string.
             */
            char firmware_version[32]; // Size per standard, adjust as needed

            /**
             * @brief Serial number string.
             */
            char serial_number[32]; // Size per standard, adjust as needed

            /**
             * @brief Group name string.
             */
            char group_name[32]; // Size per standard, adjust as needed

            /**
             * @brief Current sampling rate (if applicable).
             */
            uint32_t current_sampling_rate;

            /**
             * @brief Current format (if applicable).
             */
            uint32_t current_format;

            /**
             * @brief Current aspect ratio (for video clusters, if applicable).
             */
            uint32_t current_aspect_ratio;

            /**
             * @brief Current size (for video clusters, if applicable).
             */
            uint32_t current_size;

            /**
             * @brief Current color space (for video clusters, if applicable).
             */
            uint32_t current_color_space;

            /**
             * @brief Length of memory object (if applicable).
             */
            uint32_t memory_object_length;

            /**
             * @brief MAC address for AVB interface (if applicable).
             */
            uint8_t mac_address[6];

            /**
             * @brief Clock identity for AVB interface (if applicable).
             */
            uint64_t clock_identity;

            /**
             * @brief Priority1 for AVB interface (if applicable).
             */
            uint8_t priority1;

            /**
             * @brief Clock class for AVB interface (if applicable).
             */
            uint8_t clock_class;

            /**
             * @brief Offset scaled log variance for AVB interface (if applicable).
             */
            uint16_t offset_scaled_log_variance;

            /**
             * @brief Clock accuracy for AVB interface (if applicable).
             */
            uint8_t clock_accuracy;

            /**
             * @brief Priority2 for AVB interface (if applicable).
             */
            uint8_t priority2;

            /**
             * @brief Domain number for AVB interface (if applicable).
             */
            uint8_t domain_number;

            /**
             * @brief Log sync interval for AVB interface (if applicable).
             */
            int8_t log_sync_interval;

            /**
             * @brief Log announce interval for AVB interface (if applicable).
             */
            int8_t log_announce_interval;

            /**
             * @brief Log pdelay interval for AVB interface (if applicable).
             */
            int8_t log_pdelay_interval;

            /**
             * @brief Reserved for future use.
             */
            uint8_t reserved[32];
        };

       /** @brief CONFIGURATIONDescriptor */
       struct CONFIGURATIONDescriptor {
           uint32_t descriptor_type; // 2 octets
           uint32_t descriptor_index; // 2 octets
           uint32_t object_name; // 64 octets
           uint32_t localized_description; // 2 octets
           uint32_t descriptor_counts_count; // 2 octets
           uint32_t descriptor_counts_offset; // 2 octets
           uint32_t descriptor_counts; // 4*Noctets
       };


       /** descriptor_countsformat */


       /**AUDIO_UNITDescriptor  */

       /** VIDEO_UNITDescriptor */

       /** SENSOR_UNITDescriptor */
        enum class StreamFlags : uint16_t {
            NONE                          = 0x0000,
            SUPPORTS_NO_SRP               = 0x8000, // Bit 0
            NO_MEDIA_CLOCK                = 0x4000, // Bit 1
            TIMING_FIELD_VALID            = 0x2000, // Bit 2
            NO_SUPPORT_AVTP_NATIVE        = 0x1000, // Bit 3
            SUPPORTS_AVTP_UDPV6           = 0x0800, // Bit 4
            SUPPORTS_AVTP_UDPV4           = 0x0400, // Bit 5
            TERTIARY_BACKUP_VALID         = 0x0200, // Bit 6
            TERTIARY_BACKUP_SUPPORTED     = 0x0100, // Bit 7
            SECONDARY_BACKUP_VALID        = 0x0080, // Bit 8
            SECONDARY_BACKUP_SUPPORTED    = 0x0040, // Bit 9
            PRIMARY_BACKUP_VALID          = 0x0020, // Bit 10
            PRIMARY_BACKUP_SUPPORTED      = 0x0010, // Bit 11
            SUPPORTS_ENCRYPTED            = 0x0008, // Bit 12
            CLASS_B                       = 0x0004, // Bit 13
            CLASS_A                       = 0x0002, // Bit 14
            CLOCK_SYNC_SOURCE             = 0x0001  // Bit 15
        };



       /** MATRIXDescriptor */

        /**
         * @brief MATRIX Descriptor for IEEE 1722.1-2021.
         *
         * Describes a matrix, including type, number of rows/columns, and name.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct MATRIXDescriptor {
            /** @brief Descriptor type (should be set to MATRIX type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Matrix type (standard-specific) */
            uint16_t matrix_type;
            /** @brief Number of rows */
            uint16_t number_of_rows;
            /** @brief Number of columns */
            uint16_t number_of_columns;
            /** @brief Matrix name (UTF-8, null-terminated, max 64 bytes) */
            char matrix_name[64];
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };
       /** JACK_INPUTandJACK_OUTPUTDescriptor */
        enum class JackFlags : uint16_t {
            None               = 0x0000,
            ClockSyncSource    = 0x0001, // Bit 15
            Captive            = 0x0002  // Bit 14
        };

        enum class JackType : uint16_t {
            Speaker              = 0x0000,
            Headphone            = 0x0001,
            AnalogMicrophone     = 0x0002,
            SPDIF                = 0x0003,
            ADAT                 = 0x0004,
            TDIF                 = 0x0005,
            MADI                 = 0x0006,
            UnbalancedAnalog     = 0x0007,
            BalancedAnalog       = 0x0008,
            Digital              = 0x0009,
            MIDI                 = 0x000A,
            AES_EBU              = 0x000B,
            CompositeVideo       = 0x000C,
            SVHSVideo            = 0x000D,
            ComponentVideo       = 0x000E,
            DVI                  = 0x000F,
            HDMI                 = 0x0010,
            UDI                  = 0x0011,
            DisplayPort          = 0x0012,
            Antenna              = 0x0013,
            AnalogTuner          = 0x0014,
            Ethernet             = 0x0015,
            WiFi                 = 0x0016,
            USB                  = 0x0017,
            PCI                  = 0x0018,
            PCIe                 = 0x0019,
            SCSI                 = 0x001A,
            ATA                  = 0x001B,
            Imager               = 0x001C,
            InfraRed             = 0x001D,
            Thunderbolt          = 0x001E,
            SATA                 = 0x001F,
            SMPTE_LTC            = 0x0020,
            DigitalMicrophone    = 0x0021,
            AudioMediaClock      = 0x0022,
            VideoMediaClock      = 0x0023,
            GNSSClock            = 0x0024,
            PPS                  = 0x0025,
            Expansion            = 0xFFFF
        };

        /**
         * @brief JACK_INPUT Descriptor for IEEE 1722.1-2021.
         *
         * Describes an input jack, including type, flags, and associated signal path.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct JACK_INPUTDescriptor {
            /** @brief Descriptor type (should be set to JACK_INPUT type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Jack type (see JackType enum) */
            JackType jack_type;
            /** @brief Jack flags (see JackFlags enum) */
            JackFlags jack_flags;
            /** @brief Jack name (UTF-8, null-terminated, max 64 bytes) */
            char jack_name[64];
            /** @brief Signal type (e.g., audio, video, MIDI) */
            uint16_t signal_type;
            /** @brief Signal index */
            uint16_t signal_index;
            /** @brief Associated clock domain index */
            uint16_t clock_domain_index;
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };

        /**
         * @brief JACK_OUTPUT Descriptor for IEEE 1722.1-2021.
         *
         * Describes an output jack, including type, flags, and associated signal path.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct JACK_OUTPUTDescriptor {
            /** @brief Descriptor type (should be set to JACK_OUTPUT type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Jack type (see JackType enum) */
            JackType jack_type;
            /** @brief Jack flags (see JackFlags enum) */
            JackFlags jack_flags;
            /** @brief Jack name (UTF-8, null-terminated, max 64 bytes) */
            char jack_name[64];
            /** @brief Signal type (e.g., audio, video, MIDI) */
            uint16_t signal_type;
            /** @brief Signal index */
            uint16_t signal_index;
            /** @brief Associated clock domain index */
            uint16_t clock_domain_index;
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };

       /** AVB_INTERFACEDescriptor */
        enum class InterfaceFlags : uint16_t {
            None                        = 0x0000,
            GPTP_GrandmasterSupported   = 0x0001, // Bit 15
            GPTP_Supported              = 0x0002, // Bit 14
            SRP_Supported               = 0x0004, // Bit 13
            FQTSS_NotSupported          = 0x0008, // Bit 12
            ScheduledTrafficSupported   = 0x0010, // Bit 11
            CanListenToSelf            = 0x0020, // Bit 10
            CanListenToOtherSelf       = 0x0040  // Bit 9
            // Bits 0-8 reserved
        };

        /**
         * @brief AVB_INTERFACE Descriptor for IEEE 1722.1-2021.
         *
         * Describes an AVB network interface, including flags, MAC address, and clock domain.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct AVB_INTERFACEDescriptor {
            /** @brief Descriptor type (should be set to AVB_INTERFACE type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Interface flags (see InterfaceFlags enum) */
            InterfaceFlags interface_flags;
            /** @brief MAC address for the AVB interface */
            uint8_t mac_address[6];
            /** @brief Clock domain index associated with this interface */
            uint16_t clock_domain_index;
            /** @brief Interface name (UTF-8, null-terminated, max 64 bytes) */
            char interface_name[64];
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };


       /** CLOCK_SOURCEDescriptor */
        enum class ClockSourceFlags : uint16_t {
            None     = 0x0000,
            StreamId = 0x0001, // Bit 15
            LocalId  = 0x0002  // Bit 14
            // Bits 0–13 are reserved
        };
        enum class ClockSourceType : uint16_t {
            Internal     = 0x0000,  // From crystal oscillator or internal source
            External     = 0x0001,  // From a Jack (external)
            InputStream  = 0x0002,  // From a media clock of an Input Stream
            Reserved     = 0x0003,  // Reserved (0x0003 to 0xFFFE)
            Expansion    = 0xFFFF   // Reserved for expansion
        };

        /**
         * @brief CLOCK_SOURCE Descriptor for IEEE 1722.1-2021.
         *
         * Describes a clock source, including type, flags, and name.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct CLOCK_SOURCEDescriptor {
            /** @brief Descriptor type (should be set to CLOCK_SOURCE type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Clock source flags (see ClockSourceFlags enum) */
            ClockSourceFlags clock_source_flags;
            /** @brief Clock source type (see ClockSourceType enum) */
            ClockSourceType clock_source_type;
            /** @brief Clock source name (UTF-8, null-terminated, max 64 bytes) */
            char clock_source_name[64];
            /** @brief Associated clock domain index */
            uint16_t clock_domain_index;
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };


       /** MEMORY_OBJECTDescriptor */


        typedef enum {
            MEMORY_OBJECT_FIRMWARE_IMAGE      = 0x0000,
            MEMORY_OBJECT_VENDOR_SPECIFIC     = 0x0001,
            MEMORY_OBJECT_CRASH_DUMP          = 0x0002,
            MEMORY_OBJECT_LOG_OBJECT          = 0x0003,
            MEMORY_OBJECT_AUTOSTART_SETTINGS  = 0x0004,
            MEMORY_OBJECT_SNAPSHOT_SETTINGS   = 0x0005,
            MEMORY_OBJECT_SVG_MANUFACTURER    = 0x0006,
            MEMORY_OBJECT_SVG_ENTITY          = 0x0007,
            MEMORY_OBJECT_SVG_GENERIC         = 0x0008,
            MEMORY_OBJECT_PNG_MANUFACTURER    = 0x0009,
            MEMORY_OBJECT_PNG_ENTITY          = 0x000A,
            MEMORY_OBJECT_PNG_GENERIC         = 0x000B,
            MEMORY_OBJECT_DAE_MANUFACTURER    = 0x000C,
            MEMORY_OBJECT_DAE_ENTITY          = 0x000D,
            MEMORY_OBJECT_DAE_GENERIC         = 0x000E
            // 0x000F–0xFFFF reserved
        } MemoryObjectType;

        typedef enum {
            MEMORY_OBJECT_OP_STORE            = 0x0000,
            MEMORY_OBJECT_OP_STORE_AND_REBOOT = 0x0001,
            MEMORY_OBJECT_OP_READ             = 0x0002,
            MEMORY_OBJECT_OP_ERASE            = 0x0003,
            MEMORY_OBJECT_OP_UPLOAD           = 0x0004
            // 0x0005–0xFFFF reserved
        } MemoryObjectOperationType;

        /**
         * @brief MEMORY_OBJECT Descriptor for IEEE 1722.1-2021.
         *
         * Describes a memory object, including type, length, and name.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct MEMORY_OBJECTDescriptor {
            /** @brief Descriptor type (should be set to MEMORY_OBJECT type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Memory object type (see MemoryObjectType enum) */
            uint16_t memory_object_type;
            /** @brief Memory object name (UTF-8, null-terminated, max 64 bytes) */
            char memory_object_name[64];
            /** @brief Length of the memory object in bytes */
            uint32_t memory_object_length;
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };


       /** LOCALEDescriptor */

        /**
         * @brief LOCALE Descriptor for IEEE 1722.1-2021.
         *
         * Describes a locale, including identifier, name, and associated strings.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct LOCALEDescriptor {
            /** @brief Descriptor type (should be set to LOCALE type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Locale identifier (e.g., language/country code) */
            uint16_t locale_identifier;
            /** @brief Locale name (UTF-8, null-terminated, max 64 bytes) */
            char locale_name[64];
            /** @brief Number of associated strings */
            uint16_t number_of_strings;
            /** @brief Offset to the strings descriptors */
            uint16_t strings_offset;
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };

       /** STRINGSDescriptor */

        /**
         * @brief STRINGS Descriptor for IEEE 1722.1-2021.
         *
         * Describes a set of strings, including descriptor type, index, and string data.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct STRINGSDescriptor {
            /** @brief Descriptor type (should be set to STRINGS type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Number of strings in this descriptor */
            uint16_t number_of_strings;
            /** @brief Offset to the string data */
            uint16_t strings_offset;
            /** @brief String data (UTF-8, null-terminated, max 256 bytes) */
            char string_data[256];
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };

       /** STREAM_PORT_INPUTandSTREAM_PORT_OUTPUTDescriptor */

        /**
         * @brief STREAM_PORT_INPUT Descriptor for IEEE 1722.1-2021.
         *
         * Describes an input stream port, including flags, signal type, and associated clock domain.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct STREAM_PORT_INPUTDescriptor {
            /** @brief Descriptor type (should be set to STREAM_PORT_INPUT type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Port flags (see PortFlags enum) */
            uint16_t port_flags;
            /** @brief Signal type (e.g., audio, video, MIDI) */
            uint16_t signal_type;
            /** @brief Signal index */
            uint16_t signal_index;
            /** @brief Associated clock domain index */
            uint16_t clock_domain_index;
            /** @brief Port name (UTF-8, null-terminated, max 64 bytes) */
            char port_name[64];
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };

        /**
         * @brief STREAM_PORT_OUTPUT Descriptor for IEEE 1722.1-2021.
         *
         * Describes an output stream port, including flags, signal type, and associated clock domain.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct STREAM_PORT_OUTPUTDescriptor {
            /** @brief Descriptor type (should be set to STREAM_PORT_OUTPUT type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Port flags (see PortFlags enum) */
            uint16_t port_flags;
            /** @brief Signal type (e.g., audio, video, MIDI) */
            uint16_t signal_type;
            /** @brief Signal index */
            uint16_t signal_index;
            /** @brief Associated clock domain index */
            uint16_t clock_domain_index;
            /** @brief Port name (UTF-8, null-terminated, max 64 bytes) */
            char port_name[64];
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };
        typedef enum {
            PORT_FLAG_CLOCK_SYNC_SOURCE        = 1 << 15, // Bit 15
            PORT_FLAG_ASYNC_SAMPLE_RATE_CONV  = 1 << 14, // Bit 14
            PORT_FLAG_SYNC_SAMPLE_RATE_CONV   = 1 << 13  // Bit 13
            // Bits 0–12 are reserved for future use
        } PortFlags;


       /** EXTERNAL_PORT_INPUTandEXTERNAL_PORT_OUTPUTDescriptor */

        /**
         * @brief EXTERNAL_PORT_INPUT Descriptor for IEEE 1722.1-2021.
         *
         * Describes an external input port, including flags, signal type, and associated clock domain.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct EXTERNAL_PORT_INPUTDescriptor {
            /** @brief Descriptor type (should be set to EXTERNAL_PORT_INPUT type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Port flags (see PortFlags enum) */
            uint16_t port_flags;
            /** @brief Signal type (e.g., audio, video, MIDI) */
            uint16_t signal_type;
            /** @brief Signal index */
            uint16_t signal_index;
            /** @brief Associated clock domain index */
            uint16_t clock_domain_index;
            /** @brief Port name (UTF-8, null-terminated, max 64 bytes) */
            char port_name[64];
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };

        /**
         * @brief EXTERNAL_PORT_OUTPUT Descriptor for IEEE 1722.1-2021.
         *
         * Describes an external output port, including flags, signal type, and associated clock domain.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct EXTERNAL_PORT_OUTPUTDescriptor {
            /** @brief Descriptor type (should be set to EXTERNAL_PORT_OUTPUT type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Port flags (see PortFlags enum) */
            uint16_t port_flags;
            /** @brief Signal type (e.g., audio, video, MIDI) */
            uint16_t signal_type;
            /** @brief Signal index */
            uint16_t signal_index;
            /** @brief Associated clock domain index */
            uint16_t clock_domain_index;
            /** @brief Port name (UTF-8, null-terminated, max 64 bytes) */
            char port_name[64];
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };

       /** INTERNAL_PORT_INPUTandINTERNAL_PORT_OUTPUTDescriptor */

        /**
         * @brief INTERNAL_PORT_INPUT Descriptor for IEEE 1722.1-2021.
         *
         * Describes an internal input port, including flags, signal type, and associated clock domain.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct INTERNAL_PORT_INPUTDescriptor {
            /** @brief Descriptor type (should be set to INTERNAL_PORT_INPUT type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Port flags (see PortFlags enum) */
            uint16_t port_flags;
            /** @brief Signal type (e.g., audio, video, MIDI) */
            uint16_t signal_type;
            /** @brief Signal index */
            uint16_t signal_index;
            /** @brief Associated clock domain index */
            uint16_t clock_domain_index;
            /** @brief Port name (UTF-8, null-terminated, max 64 bytes) */
            char port_name[64];
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };

        /**
         * @brief INTERNAL_PORT_OUTPUT Descriptor for IEEE 1722.1-2021.
         *
         * Describes an internal output port, including flags, signal type, and associated clock domain.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct INTERNAL_PORT_OUTPUTDescriptor {
            /** @brief Descriptor type (should be set to INTERNAL_PORT_OUTPUT type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Port flags (see PortFlags enum) */
            uint16_t port_flags;
            /** @brief Signal type (e.g., audio, video, MIDI) */
            uint16_t signal_type;
            /** @brief Signal index */
            uint16_t signal_index;
            /** @brief Associated clock domain index */
            uint16_t clock_domain_index;
            /** @brief Port name (UTF-8, null-terminated, max 64 bytes) */
            char port_name[64];
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };

       /** AUDIO_CLUSTERDescriptor */

        /**
         * @brief AUDIO_CLUSTER Descriptor for IEEE 1722.1-2021.
         *
         * Describes an audio cluster, including format, channel count, and name.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */

        enum class audio_cluster_format_t : uint8_t {
            AUDIO_FMT_IEC_60958 = 0x00,
            AUDIO_FMT_MBLA      = 0x04,
            AUDIO_FMT_MIDI      = 0x08,
            AUDIO_FMT_SMPTE     = 0x88
            // All other values reserved
        } ;

        struct AUDIO_CLUSTERDescriptor {
            /** @brief Descriptor type (should be set to AUDIO_CLUSTER type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Audio format (see audio_cluster_format_t enum) */
            audio_cluster_format_t audio_format;
            /** @brief Number of channels in the cluster */
            uint16_t channel_count;
            /** @brief Cluster name (UTF-8, null-terminated, max 64 bytes) */
            char cluster_name[64];
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };


       /** VIDEO_CLUSTERDescriptor */
        enum class video_cluster_format_t : uint8_t {
            VIDEO_FMT_MPEG_PES             = 0x00,
            VIDEO_FMT_IIDC                 = 0x01,
            VIDEO_FMT_RESERVED             = 0x02,
            VIDEO_FMT_BT601                = 0x03,
            VIDEO_FMT_MJPEG                = 0x04,
            VIDEO_FMT_H264                 = 0x05,
            VIDEO_FMT_JPEG2000             = 0x06,
            VIDEO_FMT_SDI                  = 0x07,
            VIDEO_FMT_RVF                  = 0x08,
            // 0x09 – 0xED reserved
            VIDEO_FMT_VENDOR_SPECIFIC      = 0xFE,
            VIDEO_FMT_EXPERIMENTAL         = 0xFF
        } ;


       /** SENSOR_CLUSTERDescriptor */
        /**
         * @brief VIDEO_CLUSTER Descriptor for IEEE 1722.1-2021.
         *
         * Describes a video cluster, including format, channel count, and name.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct VIDEO_CLUSTERDescriptor {
            /** @brief Descriptor type (should be set to VIDEO_CLUSTER type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Video format (see video_cluster_format_t enum) */
            video_cluster_format_t video_format;
            /** @brief Number of channels in the cluster */
            uint16_t channel_count;
            /** @brief Cluster name (UTF-8, null-terminated, max 64 bytes) */
            char cluster_name[64];
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };
        /**
         * @brief SENSOR_CLUSTER Descriptor for IEEE 1722.1-2021.
         *
         * Describes a sensor cluster, including format, channel count, and name.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct SENSOR_CLUSTERDescriptor {
            /** @brief Descriptor type (should be set to SENSOR_CLUSTER type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Sensor format (standard-specific, e.g., temperature, pressure, etc.) */
            uint8_t sensor_format;
            /** @brief Number of channels in the cluster */
            uint16_t channel_count;
            /** @brief Cluster name (UTF-8, null-terminated, max 64 bytes) */
            char cluster_name[64];
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };

       /** AUDIO_MAPDescriptor */
        /**
         * @brief AUDIO_MAP Descriptor for IEEE 1722.1-2021.
         *
         * Describes an audio map, including mapping between clusters and channels.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct AUDIO_MAPDescriptor {
            /** @brief Descriptor type (should be set to AUDIO_MAP type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Number of mappings in this descriptor */
            uint16_t number_of_mappings;
            /** @brief Offset to the mapping data */
            uint16_t mappings_offset;
            /** @brief Mapping data (implementation-specific, max 128 bytes) */
            uint8_t mapping_data[128];
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };

       /** VIDEO_MAPDescriptor */
        /**
         * @brief VIDEO_MAP Descriptor for IEEE 1722.1-2021.
         *
         * Describes a video map, including mapping between clusters and channels.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct VIDEO_MAPDescriptor {
            /** @brief Descriptor type (should be set to VIDEO_MAP type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Number of mappings in this descriptor */
            uint16_t number_of_mappings;
            /** @brief Offset to the mapping data */
            uint16_t mappings_offset;
            /** @brief Mapping data (implementation-specific, max 128 bytes) */
            uint8_t mapping_data[128];
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };


       /** SENSOR_MAPDescriptor */
        /**
         * @brief SENSOR_MAP Descriptor for IEEE 1722.1-2021.
         *
         * Describes a sensor map, including mapping between clusters and channels.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct SENSOR_MAPDescriptor {
            /** @brief Descriptor type (should be set to SENSOR_MAP type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Number of mappings in this descriptor */
            uint16_t number_of_mappings;
            /** @brief Offset to the mapping data */
            uint16_t mappings_offset;
            /** @brief Mapping data (implementation-specific, max 128 bytes) */
            uint8_t mapping_data[128];
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };


       /** CONTROLDescriptor */
        /**
         * @brief CONTROL Descriptor for IEEE 1722.1-2021.
         *
         * Describes a control, including type, value, and name.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct CONTROLDescriptor {
            /** @brief Descriptor type (should be set to CONTROL type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Control type (implementation-specific, e.g., gain, mute, etc.) */
            uint16_t control_type;
            /** @brief Current value of the control */
            int32_t current_value;
            /** @brief Control name (UTF-8, null-terminated, max 64 bytes) */
            char control_name[64];
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };

       /** SIGNAL_SELECTORDescriptor */

        /**
         * @brief SIGNAL_SELECTOR Descriptor for IEEE 1722.1-2021.
         *
         * Describes a signal selector, including type, current selection, and name.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct SIGNAL_SELECTORDescriptor {
            /** @brief Descriptor type (should be set to SIGNAL_SELECTOR type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Selector type (implementation-specific) */
            uint16_t selector_type;
            /** @brief Current selection index */
            uint16_t current_selection;
            /** @brief Selector name (UTF-8, null-terminated, max 64 bytes) */
            char selector_name[64];
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };

       /** MIXERDescriptor */

        /**
         * @brief MIXER Descriptor for IEEE 1722.1-2021.
         *
         * Describes a mixer, including type, number of inputs/outputs, and name.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct MIXERDescriptor {
            /** @brief Descriptor type (should be set to MIXER type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Mixer type (implementation-specific) */
            uint16_t mixer_type;
            /** @brief Number of inputs */
            uint16_t number_of_inputs;
            /** @brief Number of outputs */
            uint16_t number_of_outputs;
            /** @brief Mixer name (UTF-8, null-terminated, max 64 bytes) */
            char mixer_name[64];
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };

       /** MATRIXDescriptor */

        /**
         * @brief MATRIX Descriptor for IEEE 1722.1-2021.
         *
         * Describes a matrix, including type, number of inputs/outputs, and name.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct MATRIXDescriptor {
            /** @brief Descriptor type (should be set to MATRIX type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Matrix type (implementation-specific) */
            uint16_t matrix_type;
            /** @brief Number of inputs */
            uint16_t number_of_inputs;
            /** @brief Number of outputs */
            uint16_t number_of_outputs;
            /** @brief Matrix name (UTF-8, null-terminated, max 64 bytes) */
            char matrix_name[64];
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };

       /** MATRIX_SIGNALDescriptor */

        /**
         * @brief MATRIX_SIGNAL Descriptor for IEEE 1722.1-2021.
         *
         * Describes a matrix signal, including type, input/output indices, and name.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct MATRIX_SIGNALDescriptor {
            /** @brief Descriptor type (should be set to MATRIX_SIGNAL type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Signal type (implementation-specific) */
            uint16_t signal_type;
            /** @brief Input index */
            uint16_t input_index;
            /** @brief Output index */
            uint16_t output_index;
            /** @brief Signal name (UTF-8, null-terminated, max 64 bytes) */
            char signal_name[64];
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };

       /** SIGNAL_SPLITTERDescriptor */

        /**
         * @brief SIGNAL_SPLITTER Descriptor for IEEE 1722.1-2021.
         *
         * Describes a signal splitter, including type, input index, number of outputs, and name.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct SIGNAL_SPLITTERDescriptor {
            /** @brief Descriptor type (should be set to SIGNAL_SPLITTER type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Splitter type (implementation-specific) */
            uint16_t splitter_type;
            /** @brief Input index */
            uint16_t input_index;
            /** @brief Number of outputs */
            uint16_t number_of_outputs;
            /** @brief Splitter name (UTF-8, null-terminated, max 64 bytes) */
            char splitter_name[64];
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };

       /** SIGNAL_COMBINERDescriptor */

        /**
         * @brief SIGNAL_COMBINER Descriptor for IEEE 1722.1-2021.
         *
         * Describes a signal combiner, including type, number of inputs, output index, and name.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct SIGNAL_COMBINERDescriptor {
            /** @brief Descriptor type (should be set to SIGNAL_COMBINER type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Combiner type (implementation-specific) */
            uint16_t combiner_type;
            /** @brief Number of inputs */
            uint16_t number_of_inputs;
            /** @brief Output index */
            uint16_t output_index;
            /** @brief Combiner name (UTF-8, null-terminated, max 64 bytes) */
            char combiner_name[64];
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };

       /** SIGNAL_DEMULTIPLEXERDescriptor  
        * 
       */

        /**
         * @brief SIGNAL_DEMULTIPLEXER Descriptor for IEEE 1722.1-2021.
         *
         * Describes a signal demultiplexer, including type, input index, number of outputs, and name.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct SIGNAL_DEMULTIPLEXERDescriptor {
            /** @brief Descriptor type (should be set to SIGNAL_DEMULTIPLEXER type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Demultiplexer type (implementation-specific) */
            uint16_t demultiplexer_type;
            /** @brief Input index */
            uint16_t input_index;
            /** @brief Number of outputs */
            uint16_t number_of_outputs;
            /** @brief Demultiplexer name (UTF-8, null-terminated, max 64 bytes) */
            char demultiplexer_name[64];
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };

       /** SIGNAL_MULTIPLEXERDescriptor */

        /**
         * @brief SIGNAL_MULTIPLEXER Descriptor for IEEE 1722.1-2021.
         *
         * Describes a signal multiplexer, including type, number of inputs, output index, and name.
         * Doxygen comments are taken directly from the standard for clarity and compliance.
         */
        struct SIGNAL_MULTIPLEXERDescriptor {
            /** @brief Descriptor type (should be set to SIGNAL_MULTIPLEXER type value) */
            uint16_t descriptor_type;
            /** @brief Descriptor index */
            uint16_t descriptor_index;
            /** @brief Multiplexer type (implementation-specific) */
            uint16_t multiplexer_type;
            /** @brief Number of inputs */
            uint16_t number_of_inputs;
            /** @brief Output index */
            uint16_t output_index;
            /** @brief Multiplexer name (UTF-8, null-terminated, max 64 bytes) */
            char multiplexer_name[64];
            /** @brief Reserved for future use */
            uint8_t reserved[32];
        };

       /** SIGNAL_TRANSCODERDescriptor */
       
       /**
        * @brief SIGNAL_TRANSCODER Descriptor for IEEE 1722.1-2021.
        *
        * Describes a signal transcoder, including type, input index, output index, and name.
        * Doxygen comments are taken directly from the standard for clarity and compliance.
        */
       struct SIGNAL_TRANSCODERDescriptor {
           /** @brief Descriptor type (should be set to SIGNAL_TRANSCODER type value) */
           uint16_t descriptor_type;
           /** @brief Descriptor index */
           uint16_t descriptor_index;
           /** @brief Transcoder type (implementation-specific) */
           uint16_t transcoder_type;
           /** @brief Input index */
           uint16_t input_index;
           /** @brief Output index */
           uint16_t output_index;
           /** @brief Transcoder name (UTF-8, null-terminated, max 64 bytes) */
           char transcoder_name[64];
           /** @brief Reserved for future use */
           uint8_t reserved[32];
       };

       /** CLOCK_DOMAINDescriptor */
       /**
        * @brief CLOCK_DOMAIN Descriptor for IEEE 1722.1-2021.
        *
        * Describes a clock domain, including type, index, name, and associated sources.
        * Doxygen comments are taken directly from the standard for clarity and compliance.
        */
       struct CLOCK_DOMAINDescriptor {
           /** @brief Descriptor type (should be set to CLOCK_DOMAIN type value) */
           uint16_t descriptor_type;
           /** @brief Descriptor index */
           uint16_t descriptor_index;
           /** @brief Clock domain type (implementation-specific) */
           uint16_t clock_domain_type;
           /** @brief Clock domain name (UTF-8, null-terminated, max 64 bytes) */
           char clock_domain_name[64];
           /** @brief Number of associated clock sources */
           uint16_t number_of_sources;
           /** @brief Offset to the clock sources descriptors */
           uint16_t sources_offset;
           /** @brief Reserved for future use */
           uint8_t reserved[32];
       };

       /** CONTROL_BLOCKDescriptor */
       /**
        * @brief CONTROL_BLOCK Descriptor for IEEE 1722.1-2021.
        *
        * Describes a control block, including type, index, name, and associated controls.
        * Doxygen comments are taken directly from the standard for clarity and compliance.
        */
       struct CONTROL_BLOCKDescriptor {
           /** @brief Descriptor type (should be set to CONTROL_BLOCK type value) */
           uint16_t descriptor_type;
           /** @brief Descriptor index */
           uint16_t descriptor_index;
           /** @brief Control block type (implementation-specific) */
           uint16_t control_block_type;
           /** @brief Control block name (UTF-8, null-terminated, max 64 bytes) */
           char control_block_name[64];
           /** @brief Number of associated controls */
           uint16_t number_of_controls;
           /** @brief Offset to the controls descriptors */
           uint16_t controls_offset;
           /** @brief Reserved for future use */
           uint8_t reserved[32];
       };

       /** TIMINGDescriptor */
       /**
        * @brief TIMING Descriptor for IEEE 1722.1-2021.
        *
        * Describes timing information, including type, index, name, and timing parameters.
        * Doxygen comments are taken directly from the standard for clarity and compliance.
        */
       struct TIMINGDescriptor {
           /** @brief Descriptor type (should be set to TIMING type value) */
           uint16_t descriptor_type;
           /** @brief Descriptor index */
           uint16_t descriptor_index;
           /** @brief Timing type (implementation-specific) */
           uint16_t timing_type;
           /** @brief Timing name (UTF-8, null-terminated, max 64 bytes) */
           char timing_name[64];
           /** @brief Timing value (implementation-specific, e.g., nanoseconds, ticks) */
           uint32_t timing_value;
           /** @brief Reserved for future use */
           uint8_t reserved[32];
       };

       /** PTP_INSTANCEDescriptor */
       /**
        * @brief PTP_INSTANCE Descriptor for IEEE 1722.1-2021.
        *
        * Describes a PTP instance, including type, index, name, and associated ports.
        * Doxygen comments are taken directly from the standard for clarity and compliance.
        */
       struct PTP_INSTANCEDescriptor {
           /** @brief Descriptor type (should be set to PTP_INSTANCE type value) */
           uint16_t descriptor_type;
           /** @brief Descriptor index */
           uint16_t descriptor_index;
           /** @brief PTP instance type (implementation-specific) */
           uint16_t ptp_instance_type;
           /** @brief PTP instance name (UTF-8, null-terminated, max 64 bytes) */
           char ptp_instance_name[64];
           /** @brief Number of associated PTP ports */
           uint16_t number_of_ports;
           /** @brief Offset to the PTP port descriptors */
           uint16_t ports_offset;
           /** @brief Reserved for future use */
           uint8_t reserved[32];
       };

       /** PTP_PORTDescriptor */
       /**
        * @brief PTP_PORT Descriptor for IEEE 1722.1-2021.
        *
        * Describes a PTP port, including type, index, name, and port parameters.
        * Doxygen comments are taken directly from the standard for clarity and compliance.
        */
       struct PTP_PORTDescriptor {
           /** @brief Descriptor type (should be set to PTP_PORT type value) */
           uint16_t descriptor_type;
           /** @brief Descriptor index */
           uint16_t descriptor_index;
           /** @brief PTP port type (implementation-specific) */
           uint16_t ptp_port_type;
           /** @brief PTP port name (UTF-8, null-terminated, max 64 bytes) */
           char ptp_port_name[64];
           /** @brief Port address (implementation-specific, e.g., MAC or IP) */
           uint8_t port_address[16];
           /** @brief Port state (implementation-specific) */
           uint16_t port_state;
           /** @brief Reserved for future use */
           uint8_t reserved[32];
       };
 }

/** ATDECCConnectionManagementProtocol */
namespace ACMP {
struct ACMP{

};
};

/**ATDECCEnumerationandControlProtocol */
struct AECP{


};
} // namespace _2021
}; // namespace _1722_1
}; // namespace IEEE