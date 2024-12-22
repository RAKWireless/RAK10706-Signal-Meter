/**
 * @file meshtastic.h
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Definitions for the Meshtastic protocol
 * @version 0.1
 * @date 2024-12-19
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <Arduino.h>

typedef uint32_t NodeNum;
typedef uint32_t PacketId; // A packet sequence number
#define MAX_LORA_PAYLOAD_LEN 255 // max length of 255 per Semtech's datasheets on SX12xx

typedef struct
{
	NodeNum to, from; // four bytes
	PacketId id; // 4 bytes
	uint8_t flags; // he bottom three bits of flags are use to store hop_limit when sent over the wire.
	uint8_t channel; // The channel hash - used as a hint for the decoder to limit which channels we consider
	uint8_t next_hop; // For future use
	uint8_t relay_node; // For future use
} PacketHeader;

typedef struct
{
	/** The header, as defined just before */
	PacketHeader header;

	/** The payload, of maximum length minus the header, aligned just to be sure */
	uint8_t payload[MAX_LORA_PAYLOAD_LEN + 1 - sizeof(PacketHeader)] __attribute__((__aligned__));

} RadioBuffer;

#define PB_BYTES_ARRAY_T(n) \
	struct                  \
	{                       \
		uint32_t size;      \
		uint8_t bytes[n];   \
	}

typedef PB_BYTES_ARRAY_T(233) meshtastic_Data_payload_t;
typedef PB_BYTES_ARRAY_T(256) meshtastic_MeshPacket_encrypted_t;
typedef PB_BYTES_ARRAY_T(32) meshtastic_MeshPacket_public_key_t;

typedef enum _meshtastic_PortNum {
	meshtastic_PortNum_UNKNOWN_APP = 0,					// Deprecated
	meshtastic_PortNum_TEXT_MESSAGE_APP = 1,			// A simple UTF-8 text message
	meshtastic_PortNum_REMOTE_HARDWARE_APP = 2,			// Reserved for built-in GPIO/example app
	meshtastic_PortNum_POSITION_APP = 3,				// Position message
	meshtastic_PortNum_NODEINFO_APP = 4,				// User message
	meshtastic_PortNum_ROUTING_APP = 5,					// Routing message
	meshtastic_PortNum_ADMIN_APP = 6,					// AdminMessage message
	meshtastic_PortNum_TEXT_MESSAGE_COMPRESSED_APP = 7, // Compressed TEXT_MESSAGE
	meshtastic_PortNum_WAYPOINT_APP = 8,				// Waypoint message
	meshtastic_PortNum_AUDIO_APP = 9,					// Encapsulated codec2 packet
	meshtastic_PortNum_DETECTION_SENSOR_APP = 10,		// Text Message but originating from Detection Sensor Module
	meshtastic_PortNum_REPLY_APP = 32,					// 'ping' service
	meshtastic_PortNum_IP_TUNNEL_APP = 33,				// python IP tunnel feature
	meshtastic_PortNum_PAXCOUNTER_APP = 34,				// Paxcounter
	meshtastic_PortNum_SERIAL_APP = 64,					// hardware serial interface
	meshtastic_PortNum_STORE_FORWARD_APP = 65,			// STORE_FORWARD_APP (Work in Progress)
	meshtastic_PortNum_RANGE_TEST_APP = 66,				// messages for the range test module
	meshtastic_PortNum_TELEMETRY_APP = 67,				// telemetry data
	meshtastic_PortNum_ZPS_APP = 68,					// Experimental tools for estimating node position without a GPS
	meshtastic_PortNum_SIMULATOR_APP = 69,				// Linux native applications
	meshtastic_PortNum_TRACEROUTE_APP = 70,				// RouteDiscovery message
	meshtastic_PortNum_NEIGHBORINFO_APP = 71,			// list of each node's neighbors
	meshtastic_PortNum_ATAK_PLUGIN = 72,				// Meshtastic ATAK
	meshtastic_PortNum_MAP_REPORT_APP = 73,				// map via MQTT
	meshtastic_PortNum_POWERSTRESS_APP = 74,			// PowerStress based monitoring
	meshtastic_PortNum_PRIVATE_APP = 256,				// Private application
	meshtastic_PortNum_ATAK_FORWARDER = 257,			// ATAK Forwarder
	meshtastic_PortNum_MAX = 511
} meshtastic_PortNum;

typedef struct _meshtastic_Data
{
	meshtastic_PortNum portnum; // Formerly named typ and of type Type
	meshtastic_Data_payload_t payload;
	bool want_response;	 // Response required
	uint32_t dest;		 // address of the destination node
	uint32_t source;	 // address of the original sender
	uint32_t request_id; // Only used in routing or response messages.
	uint32_t reply_id;	 // only for reply
	uint32_t emoji;		 // always false
	bool has_bitfield;	 // Has bit fields
	uint8_t bitfield;	 // Bitfield for extra flags. First use is to indicate that user approves the packet being uploaded to MQTT.
} meshtastic_Data;

typedef struct _meshtastic_MeshPacket
{
	uint32_t from;					// The sending node number.
	uint32_t to;					// The (immediate) destination
	uint8_t channel;				// Channel index
	uint8_t which_payload_variant; // Payload variant
	// meshtastic_Data decoded;		// Simplified, we do not handle encrypted data
	union
	{
		meshtastic_Data decoded;
		meshtastic_MeshPacket_encrypted_t encrypted;
	};
	uint32_t id;								   // unique ID for this packet
	uint32_t rx_time;							   // ignore, only ESP32
	float rx_snr;								   // RX packet SNR
	uint8_t hop_limit;							   // limit of hops, ignore
	bool want_ack;								   // request ACK
	uint8_t priority;							   // packet priority, ignore
	int32_t rx_rssi;							   // RX packet RSSI
	uint8_t delayed;							   // delayed flag, ignore
	bool via_mqtt;								   // Flag for MQTT, ignore
	uint8_t hop_start;							   // first hop
	meshtastic_MeshPacket_public_key_t public_key; // Public key for encrypted packets
	bool pki_encrypted;							   // Flag whether packet is encrypted
	uint8_t next_hop;							   // next hop
	uint8_t relay_node;							   // relay id
} meshtastic_MeshPacket;