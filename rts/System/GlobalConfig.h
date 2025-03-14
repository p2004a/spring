/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GLOBAL_CONFIG_H
#define _GLOBAL_CONFIG_H


class GlobalConfig {
public:
	void Init();

public:
	/**
	 * @brief network loss factor
	 *
	 * Network loss factor, a higher factor will reconfigure the protocol
	 * to resend data more frequently, i.e. waste bandwidth to reduce lag
	 */
	int networkLossFactor = 0;

	/**
	 * @brief initial network timeout
	 *
	 * Network timeout in seconds, effective before the game has started
	 */
	int initialNetworkTimeout = 30;

	/**
	 * @brief network timeout
	 *
	 * Network timeout in seconds, effective after the game has started
	 */
	int networkTimeout = 120;

	/**
	 * @brief reconnect timeout
	 *
	 * Network timeout in seconds after which a player is allowed to reconnect
	 * with a different IP.
	 */
	int reconnectTimeout = 15;

	/**
	 * @brief MTU
	 *
	 * Maximum size of network packets to send
	 */
	unsigned mtu = 1400;


	/**
	 * @brief linkBandwidth
	 *
	 * Maximum outgoing bandwidth from server in bytes, per user
	 */
	int linkOutgoingBandwidth = 64 * 1024;

	/**
	 * @brief linkIncomingSustainedBandwidth
	 *
	 * Maximum incoming sustained bandwidth to server in bytes, per user
	 */
	int linkIncomingSustainedBandwidth = 2;

	/**
	 * @brief linkIncomingPeakBandwidth
	 *
	 * Maximum peak incoming bandwidth to server in bytes, per user
	 */
	int linkIncomingPeakBandwidth = 32;

	/**
	 * @brief linkIncomingMaxPacketRate
	 *
	 * Maximum number of incoming packets to server, per user and second
	 */
	int linkIncomingMaxPacketRate = 64;

	/**
	 * @brief linkIncomingMaxWaitingPackets
	 *
	 * Maximum number of queued incoming packets to server, per user
	 */
	int linkIncomingMaxWaitingPackets = 512;


	/**
	 * @brief useNetMessageSmoothingBuffer
	 *
	 * Whether client should try to keep a small buffer of unconsumed
	 * messages for smoothing network jitter at the cost of increased
	 * latency (running further behind the server)
	 */
	bool useNetMessageSmoothingBuffer = true;

	/**
	 * @brief luaWritableConfigFile
	 *
	 * Allows Lua to write to springsettings/springrc file
	 */
	bool luaWritableConfigFile = false;

	/**
	 * @brief vfsCacheArchiveFiles
	 *
	 * Whether the VFS should cache (BufferedArchive) files in memory
	 */
	bool vfsCacheArchiveFiles = true;

	/**
	 * @brief dumpGameStateOnDesync
	 *
	 * Whether the game state should be locally generated on all clients when
	 * A desync is first detected.
	 */
	bool dumpGameStateOnDesync = false;


	/**
	 * @brief teamHighlight
	 *
	 * Team highlighting for teams that are uncontrolled or have connection
	 * problems.
	 */
	int teamHighlight = 1;

	/**
	 * @brief simulation drawing balance
	 *
	 * Defines how much percent of the time for simulation is minimum spend for
	 * drawing. This is important when reconnecting,
	 * 
	 * For example: if set to 0.15 then 15% of the total cpu time is exclusively
	 * reserved for drawing and the remaining 85% for reconnecting/simulation.
	 */
	float minSimDrawBalance;

	/**
	 * @brief minimum Frames Per Second
	 *
	 * Defines how many frames per second should minimally be
	 * rendered. To reach this number we will delay simframes.
	 */
	int minDrawFPS;

	/**
	 * @brief selection ground penetration
	 *
	 * Defines how far beyond the ground to allow selecting objects.
	 */
	float selectThroughGround;
};

extern GlobalConfig globalConfig;

#endif // _GLOBAL_CONFIG_H
