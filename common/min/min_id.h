/**
 * @file min_id.h
 * @brief MIN protocol frame ID definitions for UD3
 *
 * Defines MIN frame IDs and command codes for UD3 communication protocol.
 */

#ifndef MIN_ID_H
#define MIN_ID_H

/** @name MIN Frame IDs
 * Frame identifiers for different message types
 * @{
 */
#define MIN_ID_TERM 0     //!< Terminal/CLI data
#define MIN_ID_WD 10      //!< Watchdog
#define MIN_ID_RESET 11   //!< Reset command
#define MIN_ID_COMMAND 12 //!< Command frame
#define MIN_ID_SOCKET 13  //!< Socket status
#define MIN_ID_SYNTH 14   //!< Synthesizer control
#define MIN_ID_FEATURE 15 //!< Feature flags
#define MIN_ID_MIDI 20    //!< MIDI data
#define MIN_ID_SID 21     //!< SID data

#define MIN_ID_EVENT 40   //!< Event notifications
#define MIN_ID_ALARM 41   //!< Alarm messages
#define MIN_ID_DEBUG 42   //!< Debug output
#define MIN_ID_VMS 43     //!< Voice Music System data
#define MIN_ID_OS_INFO 44 //!< Operating system info
/** @} */

/** @name Synthesizer Commands
 * Commands sent via MIN_ID_SYNTH
 * @{
 */
#define SYNTH_CMD_FLUSH 1 //!< Flush synthesizer queues
#define SYNTH_CMD_SID 2   //!< SID mode
#define SYNTH_CMD_MIDI 3  //!< MIDI mode
#define SYNTH_CMD_OFF 4   //!< Synthesizer off
/** @} */

/** @name Socket Status
 * Socket connection states
 * @{
 */
#define SOCKET_DISCONNECTED 0 //!< Socket disconnected
#define SOCKET_CONNECTED 1    //!< Socket connected
/** @} */

/** @name Command Frame Types
 * Command codes for MIN_ID_COMMAND frames
 * @{
 */
#define CMD_HELLO_WORLD 0x01   //!< Hello handshake
#define CMD_FEATURE_FRAME 0x02 //!< Feature negotiation
#define CMD_LINK 0x03          //!< Link control
/** @} */

/** @name Event Types
 * Event codes for MIN_ID_EVENT frames
 * @{
 */
#define EVENT_GET_INFO 1             //!< Request system info
#define EVENT_ETH_INIT_FAIL 2        //!< Ethernet init failed
#define EVENT_ETH_INIT_DONE 3        //!< Ethernet init complete
#define EVENT_ETH_LINK_UP 4          //!< Ethernet link up
#define EVENT_ETH_LINK_DOWN 5        //!< Ethernet link down
#define EVENT_ETH_DHCP_SUCCESS 6     //!< DHCP succeeded
#define EVENT_ETH_DHCP_FAIL 7        //!< DHCP failed
#define EVENT_FS_CARD_CONNECTED 8    //!< Filesystem card connected
#define EVENT_FS_CARD_REMOVED 9      //!< Filesystem card removed
/** @} */

/** @name Structure Versions
 * Version identifiers for protocol structures
 * @{
 */
#define EVENT_STRUCT_VERSION 1   //!< Event structure version
#define OS_INFO_STRUCT_VERSION 1 //!< OS info structure version
/** @} */

#endif