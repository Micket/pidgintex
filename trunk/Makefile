# Makefile for pidginTeX
# Copyright 2008 Mikael Öhman
# Initially based on the pidgin-latex plugin.
#
# This file is part of pidginTeX.
#
# pidginTeX is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# pidginTeX is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with pidginTeX. If not, see <http://www.gnu.org/licenses/>.

ENABLE_NLS = 1
#HISTORY = 1
PLUGIN = pidginTeX
PLUGIN_VERSION = 1.1.0

ifdef CROSS
############ Windows ###########
CC    = i586-mingw32msvc-cc
STRIP = i586-mingw32msvc-strip

CFLAGS = \
		-I../pidgin-2.5.5/libpurple \
		-I../win32-dev/gtk_2_0/include/glib-2.0 \
		-I../win32-dev/gtk_2_0/lib/glib-2.0/include \
		-I../win32-dev/gtk_2_0/include

LDFLAGS = \
	-L../win32-dev/gtk_2_0/lib -lglib-2.0 \
	-L../pidgin-2.5.5/libpurple/ -lpurple 
ifdef ENABLE_NLS
 LDFLAGS += -lintl
endif

PLUGIN_FILE = $(PLUGIN).dll
else 
############ Linux ###########
CC = gcc
STRIP = strip
CFLAGS  = $(shell pkg-config purple --cflags) -fPIC
LDFLAGS = $(shell pkg-config purple --libs)
PLUGIN_FILE = $(PLUGIN).so
endif

############ Both ###########
ifdef ENABLE_NLS
 CFLAGS  += -DENABLE_NLS
endif
CFLAGS   += -DPLUGIN_NAME=\"$(PLUGIN)\" \
            -DPLUGIN_VERSION=\"$(PLUGIN_VERSION)\" \
            -DPLUGIN_ID=\"core-micket-$(PLUGIN_NAME)\" \
            -Wall -c
LDFLAGS  += -shared -Wl,--export-dynamic -Wl,-soname
ifeq ($(PREFIX),)
 LIB_INSTALL_DIR = $(HOME)/.purple/plugins
else
 LIB_INSTALL_DIR = $(PREFIX)/lib/purple-2
endif
PLUGIN_DIR = $(PLUGIN)-$(PLUGIN_VERSION)

# Rules 
$(PLUGIN_FILE): $(PLUGIN).c
	@echo ========== Compiling $(PLUGIN_FILE)
	$(CC) $(PLUGIN).c -c $(CFLAGS)
	@echo ========== Linking $(PLUGIN_FILE)
	$(CC) $(PLUGIN).o -o $(PLUGIN_FILE) $(LDFLAGS)
	@echo ========== Stripping $(PLUGIN_FILE)
	$(STRIP) $(PLUGIN_FILE)

install: $(PLUGIN_FILE)
	@echo ========== Installing to $(LIB_INSTALL_DIR)
	mkdir -p $(LIB_INSTALL_DIR)
	cp $(PLUGIN_FILE) $(LIB_INSTALL_DIR)

po:
	@echo ========== Creating reference file
	mkdir po
	xgettext -k_ -p po -d pidginTeX pidginTeX.c

tar:
	@echo ========== Creating source package $(PLUGIN_DIR).tar.gz
	rm -rf $(PLUGIN_DIR) $(PLUGIN_DIR).tar.gz
	mkdir $(PLUGIN_DIR)
	cp $(PLUGIN).c $(PLUGIN).h Makefile CHANGELOG COPYING README TODO $(PLUGIN_DIR)
	tar -cv $(PLUGIN_DIR) | gzip -9 -c > $(PLUGIN_DIR).tar.gz
	rm -r $(PLUGIN_DIR)

clean:
	@echo ========== Clean
	rm -f $(PLUGIN)*.so $(PLUGIN)*.dll $(PLUGIN).o
