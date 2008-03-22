/* pidgin-mimetex.h 
 * Based on:
 * LaTeX.c
 * pidgin-latex plugin
 *
 * This a plugin for Pidgin to display LaTeX formula in conversation
 * PLEASE, send any comment, bug report, etc. to the trackers at sourceforge.net
 * Copyright (C) 2006-2007 Benjamin Moll (qjuh@users.sourceforge.net)
 * some portions : Copyright (C) 2004-2006 Nicolas Schoonbroodt (nicolas@ffsa.be)
 *                 Copyright (C) 2004-2006 GRIm@ (thegrima@altern.org).
 * 		   Copyright (C) 2004-2006 Eric Betts (bettse@onid.orst.edu).
 * Windows port  : Copyright (C) 2005-2006 Nicolai Stange (nic-stange@t-online.de)
 * Other portions heavily inspired and copied from gaim sources
 * Copyright (C) 1998-2007 Pidgin developers pidgin.im
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; This document is under the scope of
 * the version 2 of the License, or any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program (see COPYING); if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef _PIDGINTEX_H_
#define _PIDGINTEX_H_
#ifndef G_GNUC_NULL_TERMINATED
#  if __GNUC__ >= 4
#    define G_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
#  else
#    define G_GNUC_NULL_TERMINATED
#  endif /* __GNUC__ >= 4 */
#endif /* G_GNUC_NULL_TERMINATED */

#define PURPLE_PLUGINS

#ifdef ENABLE_NLS
#  include <locale.h>
#  include <libintl.h>
#  define _(String) dgettext (PLUGIN_NAME, String)
// TODO:
#  define PP_LOCALEDIR "locale"
#else
#  define _(String) (String)
#endif

// Includes
#ifdef _WIN32
#define g_file_get_contents g_file_get_contents_utf8
#include <windows.h>
#endif

#include <conversation.h>
#include <debug.h>
#include <signals.h>
#include <imgstore.h>
#include <util.h>
#include <notify.h>
#include <server.h>
#include <log.h>
#include <version.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Constant
#define TEX_DELIMITER    "$$"
#define PREFS_BASE       "/plugins/core/" PLUGIN_NAME
#define PREFS_SENDIMAGE  (PREFS_BASE "/sendimage")
#define PREFS_PREPEND    (PREFS_BASE "/prepend")
#define PREFS_RENDERER   (PREFS_BASE "/renderer")
#define PREFS_PRINTEXPR  (PREFS_BASE "/printexpr")
#define PREFS_NEGATE     (PREFS_BASE "/negate")
#define PREFS_SMASH      (PREFS_BASE "/smash")
#define PREFS_STYLE      (PREFS_BASE "/style")
#define PREFS_FONT_SIZE  (PREFS_BASE "/fontsize")
#define PREFS_FONT_COLOR (PREFS_BASE "/fontcolor")

/* font sizes for mathTeX */
static char* mathfont[] = {"tiny","footnotesize","normalsize","large",
                 "Large","LARGE","huge","Huge"};

/* 
 * latex_to_image creates an image with the LaTeX code pointed by *latex
 * *latex points to the latex string
 * **file_img receives filename of the temporary generated image; 
 * file must be deleted and string must be freed by caller
 *
 * returns TRUE on success, false otherwise
 */
//static gboolean latex_to_image(char *latex, char **file_img);

/*
 * Transform *tmp2 extracting some *startdelim here *enddelim thing,
 * make an image from latex-input and tmp2 becomes 'some <img="number"> thing'
 *
 * returns TRUE on success, FALSE otherwise
 */
//static gboolean analyse(char **msg, char *startdelim, char *enddelim);

/*
 * log_write performs the effective write of the latex code in the IM windows
 * 	*conv is a pointer onto the conversation context
 *	*nom is the name of the correspondent
 *	*message is the modified message with the image
 *	*messFlags is Flags related to the messages
 *	*original is the original message unmodified
 *
 * returns TRUE.
 */
//static void pidgin_latex_write(PurpleConversation *conv, const char *nom, 
//    char *message, PurpleMessageFlags messFlag, char *original);

/* to intercept in- and outgoing messages */
static gboolean message_write(PurpleAccount *account, const char *sender, char **message, 
    PurpleConversation *conv, PurpleMessageFlags flags);
static void message_wrote(PurpleAccount *account, const char *sender, 
    const char *message, PurpleConversation *conv, PurpleMessageFlags flags);
static void message_send(PurpleAccount *account, char *recipient, char **message);

/* variables for callback functions */
static gboolean logflag;
static char *originalmsg;
static char *modifiedmsg;

/*
 * searchPATH searches the PATH-environment for the specified file file.
 * *file is the name of the executable to search for
 *
 * returns the right full path, e.g. with the executable's name appended, 
 * NULL on failure, must be freed with free()
 */
char* searchPATH(const char *file);

/*
 * execute executes the *cmd.
 * Advantage to system()-call under win32 ist that no console window pops up.
 * On systems other than windows this function is just a wrapper around the system()-call.
 * *cmd is the command to execute.
 *
 * returns -1 if execution failed, otherwise the return code of the executed program.
*/
static int execute(char *cmd);

/*
 * win32_purple_notify extracts error information of last occured 
 * WIN32-Error due to an API-call and asserts it to gaim via purple_notify_error
 * Error message will be prepended by *prep:
 */
//#ifdef _WIN32
//void win32_purple_notify_error(char *prep);
//#endif

#endif
