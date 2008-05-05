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

# Translationtions aren't working yet.
#ENABLE_NLS = 1
PLUGIN = pidginTeX
PLUGIN_VERSION = 1.0.4

ifdef CROSS
############ Windows ###########
CC    = i586-mingw32msvc-gcc
STRIP = i586-mingw32msvc-strip

PIDGIN_CFLAGS = \
		-D"__GNUC_PREREQ(a,b)=0" \
		-I../pidgin-2.4.0/libpurple \
		-I../win32-dev/gtk_2_0/include \
		-I../win32-dev/gtk_2_0/include/glib-2.0 \
		-I../win32-dev/gtk_2_0/lib/glib-2.0/include

PIDGIN_LDFLAGS = \
	-L../win32-dev/gtk_2_0/lib -lglib-2.0 \
	-L. -lpurple 

ifdef ENABLE_NLS
 PIDGIN_LDFLAGS += -lintl
endif

PLUGIN_FILE = $(PLUGIN)-$(PLUGIN_VERSION).dll
else 
############ Linux ###########
CC = gcc
STRIP = strip
ifeq ($(PREFIX),)
 LIB_INSTALL_DIR = $(HOME)/.purple/plugins
else
 LIB_INSTALL_DIR = $(PREFIX)/lib/pidgin
endif
PIDGIN_CFLAGS  = $(shell pkg-config purple --cflags) -fPIC
PIDGIN_LDFLAGS = $(shell pkg-config purple --libs)
PLUGIN_FILE = $(PLUGIN).so
endif

############ Both ###########
ifdef DEBUG
 DEBUG_PRINT = fprintf
else
 DEBUG_PRINT = //
endif

CFLAGS     = -DDEBUG_PRINT=$(DEBUG_PRINT) \
			 -DPLUGIN_NAME=\"$(PLUGIN)\" \
			 -DPLUGIN_VERSION=\"$(PLUGIN_VERSION)\" \
			 $(PIDGIN_CFLAGS) -c
LDFLAGS    = $(PIDGIN_LDFLAGS) -shared -Wl,--export-dynamic -Wl,-soname
PLUGIN_DIR = $(PLUGIN)-$(PLUGIN_VERSION)

$(PLUGIN_FILE): $(PLUGIN).o
	@echo ======= Linking $(PLUGIN_FILE)
	$(CC) -o $(PLUGIN_FILE) $< $(LDFLAGS)
	@echo ======= Stripping $(PLUGIN_FILE)
	$(STRIP) $(PLUGIN_FILE)

install: $(PLUGIN_FILE)
	@echo ======= Installing to $(LIB_INSTALL_DIR)
	mkdir -p $(LIB_INSTALL_DIR)
	cp $(PLUGIN_FILE) $(LIB_INSTALL_DIR)

pot:
	@echo ======= Creating reference file
	mkdir -p po
	xgettext -k_ pidginTeX.c -o po/pidginTeX.pot

tar:
	@echo ======= Creating source package $(PLUGIN_DIR).tar.gz
	rm -rf $(PLUGIN_DIR) $(PLUGIN_DIR).tar.gz
	mkdir $(PLUGIN_DIR)
	cp $(PLUGIN).c $(PLUGIN).h Makefile CHANGELOG COPYING README TODO $(PLUGIN_DIR)
	tar -cv $(PLUGIN_DIR) | gzip -9 -c > $(PLUGIN_DIR).tar.gz
	rm -r $(PLUGIN_DIR)

clean:
	@echo ======= Clean
	rm -rf $(PLUGIN).o $(PLUGIN)*.so $(PLUGIN)*.dll
