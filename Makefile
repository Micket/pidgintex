# pidgin-mimetex Makefile
# Heavily inspired and copied from :
# LaTeX Makefile
# Copyright 2004 Edouard Geuten <thegrima AT altern DOT org>
#
# Heavily inspired and copied from :
# Gaim Extended Preferences Plugin Main Makefile
# Copyright 2004 Kevin Stange <extprefs@simguy.net>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

CC = gcc
STRIP = strip
PLUGIN = pidginTeX
PLUGIN_VERSION = 1.0.1

ifdef CROSS
############ Windows ###########
CC = $(CROSS)-$(CC)
PIDGIN_TREE_TOP = ../pidgin-2.3.1
PURPLE_TOP      = $(PIDGIN_TREE_TOP)/libpurple

PIDGIN_CFLAGS = \
		-I$(PIDGIN_TREE_TOP) \
		-I$(PURPLE_TOP) \
		-I$(PURPLE_TOP)/win32 \
		$(shell pkg-config glib-2.0 --cflags)

PIDGIN_LDFLAGS = -L$(PURPLE_TOP) -lpurple #-Lbin/ -lglib-2.0

PLUGIN_FILE = $(PLUGIN).dll
else 
############ Linux ###########
ifeq ($(PREFIX),)
 LIB_INSTALL_DIR = $(HOME)/.purple/plugins
else
 LIB_INSTALL_DIR = $(PREFIX)/lib/pidgin
endif
PIDGIN_CFLAGS  = $(shell pkg-config pidgin --cflags)
PIDGIN_LDFLAGS = $(shell pkg-config pidgin --libs)

PLUGIN_FILE = $(PLUGIN).so

endif

############ Both ###########
CFLAGS  = $(PIDGIN_CFLAGS) -fPIC -c \
	-DPLUGIN_NAME=\"$(PLUGIN)\" -DPLUGIN_VERSION=\"$(PLUGIN_VERSION)\"
LDFLAGS = $(PIDGIN_LDFLAGS) -shared -Wl,--export-dynamic -Wl,-soname
PLUGIN_DIR = $(PLUGIN)-$(PLUGIN_VERSION)

all: $(PLUGIN).o 
	$(CC) -o $(PLUGIN_FILE) $< $(LDFLAGS)
	$(STRIP) $(PLUGIN_FILE)

install: all
	mkdir -p $(LIB_INSTALL_DIR)
	cp $(PLUGIN).so $(LIB_INSTALL_DIR)

tar:
	rm -rf $(PLUGIN_DIR) $(PLUGIN_DIR).tar.gz
	mkdir $(PLUGIN_DIR)
	cp $(PLUGIN).c $(PLUGIN).h Makefile CHANGELOG COPYING README TODO $(PLUGIN_DIR)
	tar -cv $(PLUGIN_DIR) | gzip -9 -c > $(PLUGIN_DIR).tar.gz
	rm -r $(PLUGIN_DIR)

clean:
	rm -rf $(PLUGIN).o $(PLUGIN).so $(PLUGIN).dll
