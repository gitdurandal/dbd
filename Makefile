CC = gcc
MAKE = make

# extra flags
CFLAGS=
LDFLAGS=

# cflags

WIN_CFLAGS 	= -Wall -Wshadow -O2 -DWIN32
WINMAIN_CFLAGS	= -mwindows -Wall -Wshadow -O2 -DWIN32 -DWINMAIN
UNIX_CFLAGS	= -Wall -Wshadow -O2

# ldflags

WIN_LDFLAGS	= -s -lwsock32
UNIX_LDFLAGS	= -s
SUNOS_LDFLAGS	= -s -lresolv -lsocket -lnsl

# make install (for unix-like only)
INSTALL = install
PREFIX = /usr/local
BINDIR = bin

#################################

out = dbd
outbg = dbdbg

files = pel.c aes.c sha1.c doexec.c dbd.c

#################################

none:
	@echo "usage:"
	@echo "  make unix     - Linux, NetBSD, FreeBSD, OpenBSD"
	@echo "  make sunos    - SunOS (Solaris)"
	@echo "  make win32    - native win32 console app (w/ Cygwin + MinGW)"
	@echo "  make win32bg  - create a native win32 no-console app (w/ Cygwin + MinGW)"
	@echo "  make win32bg CFLAGS=-DSTEALTH - stealthy no-console app"
	@echo "  make mingw    - native win32 console app (w/ MinGW MSYS)"
	@echo "  make mingwbg  - native win32 no-console app (w/ MinGW MSYS)"
	@echo "  make mingwbg CFLAGS=-DSTEALTH - stealthy no-console app (w/ MinGW MSYS)"
	@echo "  make cygwin   - Cygwin console app"
	@echo "  make darwin   - Darwin"

unix: clean
	$(CC) $(UNIX_CFLAGS) $(CFLAGS) -o $(out) $(files) $(UNIX_LDFLAGS) $(LDFLAGS)

sunos: clean
	@echo "*** tested on SunOS 5.9 x86 and r220 ***"
	$(CC) $(UNIX_CFLAGS) $(CFLAGS) -o $(out) $(files) $(SUNOS_LDFLAGS) $(LDFLAGS)

cygwin: unix

win32: cygmingw
windows: cygmingw
win32bg: cygmingwbg

cygmingw: clean
	$(CC) -mno-cygwin $(WIN_CFLAGS) $(CFLAGS) -o $(out) $(files) $(WIN_LDFLAGS) $(LDFLAGS)
cygmingwbg: cleanbg
	$(CC) -mno-cygwin $(WINMAIN_CFLAGS) $(CFLAGS) -o $(outbg) $(files) $(WIN_LDFLAGS) $(LDFLAGS)

mingw: clean
	$(CC) $(WIN_CFLAGS) $(CFLAGS) -o $(out) $(files) $(WIN_LDFLAGS) $(LDFLAGS)
mingwbg: cleanbg
	$(CC) $(WINMAIN_CFLAGS) $(CFLAGS) -o $(outbg) $(files) $(WIN_LDFLAGS) $(LDFLAGS)

darwin: clean
	$(CC) $(UNIX_CFLAGS) $(CFLAGS) -o $(out) $(files) $(LDFLAGS)
	strip $(out)

distclean: clean

clean:
	rm -f $(out) $(out).exe *.o core
cleanbg:
	rm -f $(outbg) $(outbg).exe *.o core

install:
	$(INSTALL) -m 755 -d $(PREFIX)/$(BINDIR)
	$(INSTALL) -c -m 755 $(out) $(PREFIX)/$(BINDIR)/

uninstall:
	rm -f $(PREFIX)/$(BINDIR)/$(out)

dist:
	@./mktarball.sh
