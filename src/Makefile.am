bin_PROGRAMS=meshnet
#meshnet_SOURCES=aes.cpp arp.cpp mesh.cpp net.cpp periodic.cpp tap.cpp tap_dev.cpp
meshnet_SOURCES=mesh.c aes.c net.c arp.c tap.c periodic.c tap_dev.c
meshnet_LDFLAGS=-pthread

if DEBUG
AM_CFLAGS=-ggdb3 -DSYSCONF='"${sysconfdir}"' -DDEBUG
AM_CXXFLAGS=-ggdb3 -DSYSCONF='"${sysconfdir}"' -DDEBUG
else
AM_CFLAGS=-DSYSCONF='"${sysconfdir}"'
AM_CXXFLAGS=-DSYSCONF='"${sysconfdir}"'
endif
