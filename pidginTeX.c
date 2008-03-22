/* pidginTeX.c
 * Mikael Öhman (micketeer@gmail.com)
 * Built from pidgin-latex plugin
 * PLEASE, send any comment, bug report, etc. to the trackers at googlecode
 *
 * original pidgin-latex plugin writers:
 * Copyright (C) 2006-2007 Benjamin Moll (qjuh@users.sourceforge.net)
 * some portions : Copyright (C) 2004-2006 Nicolas Schoonbroodt (nicolas@ffsa.be)
 *                 Copyright (C) 2004-2006 GRIm@ (thegrima@altern.org).
 *         Copyright (C) 2004-2006 Eric Betts (bettse@onid.orst.edu).
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

#include "pidginTeX.h"

static void open_log(PurpleConversation *conv)
{
    conv->logs = g_list_append(NULL, purple_log_new(conv->type == PURPLE_CONV_TYPE_CHAT ? 
                PURPLE_LOG_CHAT : PURPLE_LOG_IM, 
                conv->name, conv->account, conv, time(NULL), NULL));
}

char* searchPATH(const char *file)
{
    char* searchexpr = g_strdup_printf("which %s > /dev/null",file);
    int found = system(searchexpr);
    free(searchexpr);
    return found ? NULL : g_strdup(file);
}

static int execute(char *cmd)
{
#ifdef _WIN32
    DWORD exitcode = 0;
    STARTUPINFO sup;
    PROCESS_INFORMATION pi;
    ZeroMemory( &sup, sizeof(sup) );
    ZeroMemory( &pi,  sizeof(pi) );
    sup.cb          = sizeof(sup);
    sup.wShowWindow = SW_HIDE;
    sup.dwFlags     = STARTF_USESHOWWINDOW;

    if (!CreateProcess(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &sup, &pi))
    {
        purple_notify_error(NULL, PLUGIN_NAME, cmd, NULL);
        return -1;
    }
    if (WAIT_OBJECT_0!=WaitForSingleObjectEx(pi.hProcess, INFINITE, FALSE))
    {
        purple_notify_error(NULL, PLUGIN_NAME, cmd, NULL);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return -1;
    }
    do
    {
        if (!GetExitCodeThread(pi.hThread, &exitcode))
        {
            purple_notify_error(NULL, PLUGIN_NAME, cmd, NULL);
            return -1;
        }
        Sleep(10);
    } while(exitcode==STILL_ACTIVE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return exitcode;
#else
    return system(cmd);
#endif
}

static gboolean latex_to_image(char *tex, char **file_img)
{
    FILE* tmpfile = purple_mkstemp(file_img,TRUE);
    if(!*file_img)
    {
        purple_notify_error(NULL, PLUGIN_NAME, _("Couldn't create temporary file"), NULL);
        unlink(*file_img);
        free(*file_img);
        return FALSE;
    }
    fclose(tmpfile);

#ifndef _WIN32
    const char *renderer = purple_prefs_get_string(PREFS_RENDERER);
    char *cmdTeX = searchPATH(renderer);
    if (!cmdTeX)
    {
        char *err_msg = !strcmp(renderer,"mimetex") ? 
            _("Failed to find: mimetex\n"
            "Make sure you have it installed or change renderer.") :
            _("Failed to find: mathtex\n"
            "Make sure you have it installed or change renderer.");
        // This didn't work for no good reason.
        //g_strdup_printf("Failed to execute command: %s."
        //" Make sure you have it installed or change renderer.",cmdTeX); 
        purple_notify_error(NULL, PLUGIN_NAME, err_msg, NULL);
        free(cmdTeX);
        return FALSE;
    }
#endif

    // Fixes the unusual escaped apostrophe (rest can be handled by mathtex)
    //tex = unescape_apos(tex);
    GRegex* regex = g_regex_new("&apos;", 0, 0, NULL);
    tex = g_regex_replace(regex, tex, -1, 0, "'", 0, NULL);
    g_regex_unref(regex);

    // Build the options the options
    int  fontsize         = purple_prefs_get_int(PREFS_FONT_SIZE);
    const char *prepend   = purple_prefs_get_string(PREFS_PREPEND);
    const char *fontcolor = purple_prefs_get_string(PREFS_FONT_COLOR);
    const char *style     = purple_prefs_get_string(PREFS_STYLE);
    const char *reverse   = purple_prefs_get_bool(PREFS_NEGATE) ? "\\reverse" : "";
    const char *smash     = purple_prefs_get_string(PREFS_SMASH);
    char* cmdparam;

    int usecolor = strlen(fontcolor);
#ifdef _WIN32
    cmdparam = g_strdup_printf("cmd.exe /C mimetex -d -s %d \"%s%s%s%s%s{%s %s}\" %s",
        fontsize, usecolor ? "\\":"", usecolor ? fontcolor:"",
        reverse, style, smash, prepend, tex, *file_img);
#else
    if (!strcmp(renderer, "mimetex"))
    {
        cmdparam = g_strdup_printf("\"%s\" -d -s %d \"%s%s%s%s%s{%s %s}\" > %s",
            cmdTeX, fontsize, usecolor ? "\\":"", usecolor ? fontcolor:"", 
            reverse, style, smash, prepend, tex, *file_img);
    }
    else //if (!strcmp(renderer,"mathtex"))
    {
        cmdparam = g_strdup_printf( 
            "\"%s\" -m 0 \"\\png\\usepackage{color}\\color{%s}%s\\%s %s %s\" -o %s",
            cmdTeX, usecolor ? fontcolor : "black", style,
            mathfont[fontsize], prepend, tex, *file_img);
    }
    free(cmdTeX);
#endif
    free(tex);

    DEBUG_PRINT(stderr, "%s\n",cmdparam);

    if(execute(cmdparam))
    {
#ifdef _WIN32
        purple_notify_error(NULL, PLUGIN_NAME, _("Failed to execute renderer,"
            " something might be wrong in the latex expression"), NULL);
#else
        char *err_msg = !strcmp(renderer,"mimetex") ? 
            _("Failed to execute: mimetex\n"
            "Something might be wrong in the latex expression.") :
            _("Failed to execute: mathtex\n"
            "Something might be wrong in the latex expression.");
            // This didn't work for no good reason.
            //g_strdup_printf("Failed to execute command: %s."
            //" Make sure you have it installed or change renderer.",cmdTeX); 
        purple_notify_error(NULL, PLUGIN_NAME, err_msg, NULL);
#endif
        free(cmdparam);
        //free(err_msg);
        return FALSE;
    }
    free(cmdparam);

#ifndef _WIN32
    // Ugly hack becuase mathtex makes up it's own filename (appends fileext). sigh
    if (!strcmp(renderer,"mathtex"))
    {
        char* file_img2 = g_strdup_printf("%s.png",*file_img);
        unlink(*file_img);
        free(*file_img);
        *file_img = file_img2;
    }
#endif
    return TRUE;
}

static gboolean analyse(char **msg, char *startdelim, char *enddelim)
{
    int imgcounter = 0;
    char *ptr1, *ptr2 = NULL;
    while(ptr1 = strstr(*msg, startdelim))
    {
        char *tex, *file_img;
        gchar *filedata;
        size_t size;
        GError *error = NULL;

        int msglen = strlen(*msg);
        tex = &ptr1[strlen(startdelim)];
        if (!(ptr2 = strstr(tex, enddelim))) return FALSE;
        *ptr2 = '\0';

        if (!latex_to_image(tex, &file_img)) return FALSE;

        if (!g_file_get_contents(file_img, &filedata, &size, &error))
        {
            purple_notify_error(NULL, PLUGIN_NAME, error->message, NULL);
            g_error_free(error);
            *ptr2 = *startdelim;
            return FALSE;
        }
        unlink(file_img);
        free(file_img);

        char* name = g_strdup_printf("pidginTeX-%s-%d.png",purple_date_format_long(NULL),
            imgcounter++);
        int idimg = purple_imgstore_add_with_id(filedata, size, name);
        free(name);
        if (idimg == 0)
        {
            purple_notify_error(NULL, PLUGIN_NAME, _("Failed to store image."), NULL);
            *ptr2 = *startdelim;
            return FALSE;
        }

        *ptr1 = '\0';
        char* tmpmsg;
        tmpmsg = g_strdup_printf("%s<img id=\"%d\" alt=\"%s\"> %s%s", *msg,idimg,tex,
                purple_prefs_get_bool(PREFS_PRINTEXPR) ? tex : "", &ptr2[strlen(enddelim)]);
        free(*msg);
        *msg = tmpmsg;
    }
    return TRUE;
}

static gboolean message_write(PurpleAccount *account, const char *sender, 
    char **message, PurpleConversation *conv, PurpleMessageFlags flags)
{
    DEBUG_PRINT(stderr, "message_write = \n%s\n",*message);
    if (!modifiedmsg && strstr(*message,TEX_DELIMITER) && (modifiedmsg = strdup(*message)) &&
        !analyse(&modifiedmsg, TEX_DELIMITER, TEX_DELIMITER))
    {
        free(modifiedmsg);
        modifiedmsg = NULL;
    }
    /*if (!modifiedmsg && strstr(*message,TEX_DELIMITER))
    {
        modifiedmsg = strdup(*message);
        analyse(&modifiedmsg, TEX_DELIMITER, TEX_DELIMITER);
    }*/
    if (modifiedmsg)
    {
        logflag = purple_conversation_is_logging(conv);
        purple_conversation_set_logging(conv, FALSE);
        originalmsg = *message;
        *message    = modifiedmsg;
        modifiedmsg = NULL;
        if (conv->features & PURPLE_CONNECTION_NO_IMAGES && purple_prefs_get_bool(PREFS_SENDIMAGE))
            purple_notify_warning(NULL, PLUGIN_NAME, 
                _("Image was NOT sent, this conversation does not support images."), NULL);
    }
    return FALSE;
}

static void message_wrote(PurpleAccount *account, const char *sender, 
    const char *message, PurpleConversation *conv, PurpleMessageFlags flags)
{
    DEBUG_PRINT(stderr, "message_wrote = \n%s\n",message);
    if (originalmsg && logflag)
    {
        purple_conversation_set_logging(conv, logflag);
        if (!conv->logs)
            open_log(conv);
        GList *log = conv->logs;
        while (log)
        {
            purple_log_write((PurpleLog *)log->data, flags, sender, time(NULL), originalmsg);
            log = log->next;
        }
        free(originalmsg);
        originalmsg = NULL;
    }
}

static void message_send(PurpleAccount *account, char *recipient, char **message)
{
    DEBUG_PRINT(stderr, "message_send = \n%s\n",*message);
    if (!purple_prefs_get_bool(PREFS_SENDIMAGE) || !strstr(*message, TEX_DELIMITER) ||
        !(modifiedmsg = strdup(*message)))
        return;
    else if (analyse(&modifiedmsg, TEX_DELIMITER, TEX_DELIMITER))
        *message = strdup(modifiedmsg);
    else
    {
        free(modifiedmsg);
        modifiedmsg = NULL;
    }
}

static gboolean plugin_load(PurplePlugin *plugin)
{
    void *conv_handle = purple_conversations_get_handle();
    purple_signal_connect(conv_handle,"writing-im-msg",  plugin,PURPLE_CALLBACK(message_write), NULL);
    purple_signal_connect(conv_handle,"writing-chat-msg",plugin,PURPLE_CALLBACK(message_write), NULL);
    purple_signal_connect(conv_handle,"wrote-im-msg",    plugin,PURPLE_CALLBACK(message_wrote), NULL);
    purple_signal_connect(conv_handle,"wrote-chat-msg",  plugin,PURPLE_CALLBACK(message_wrote), NULL);
    purple_signal_connect(conv_handle,"sending-im-msg",  plugin,PURPLE_CALLBACK(message_send), NULL);
    purple_signal_connect(conv_handle,"sending-chat-msg",plugin,PURPLE_CALLBACK(message_send), NULL);
    purple_debug(PURPLE_DEBUG_INFO, PLUGIN_NAME, PLUGIN_NAME " loaded\n");
    modifiedmsg = NULL;
    return TRUE;
}

static gboolean plugin_unload(PurplePlugin * plugin)
{
    void *conv_handle = purple_conversations_get_handle();
    purple_signal_disconnect(conv_handle,"writing-im-msg",  plugin,PURPLE_CALLBACK(message_write));
    purple_signal_disconnect(conv_handle,"writing-chat-mg", plugin,PURPLE_CALLBACK(message_write));
    purple_signal_disconnect(conv_handle,"wrote-im-msg",    plugin,PURPLE_CALLBACK(message_wrote));
    purple_signal_disconnect(conv_handle,"wrote-chat-msg",  plugin,PURPLE_CALLBACK(message_wrote));
    purple_signal_disconnect(conv_handle,"sending-im-msg",  plugin,PURPLE_CALLBACK(message_send));
    purple_signal_disconnect(conv_handle,"sending-chat-msg",plugin,PURPLE_CALLBACK(message_send));
    return TRUE;
}

static PurplePluginPrefFrame * get_plugin_pref_frame(PurplePlugin *plugin)
{
    PurplePluginPrefFrame *frame = purple_plugin_pref_frame_new();
    PurplePluginPref *ppref;

    purple_plugin_pref_frame_add(frame,purple_plugin_pref_new_with_label(
                _("All of these choices will only change your image, not your message.")));

    // Do send image
    ppref = purple_plugin_pref_new_with_name_and_label(
            PREFS_SENDIMAGE,
            _("Send image. Make sure images can be sent before selecting!"));
    purple_plugin_pref_frame_add(frame, ppref);

#ifndef _WIN32
    // Select renderer
    ppref = purple_plugin_pref_new_with_name_and_label(
            PREFS_RENDERER,
            _("Choose a renderer. Program must be installed in path."));
    purple_plugin_pref_set_type(ppref, PURPLE_PLUGIN_PREF_CHOICE);
    purple_plugin_pref_add_choice(ppref, "mimeTeX", "mimetex");
    purple_plugin_pref_add_choice(ppref, "mathTeX", "mathtex");
    purple_plugin_pref_frame_add(frame, ppref);
#endif

    // Print expression
    ppref = purple_plugin_pref_new_with_name_and_label(
            PREFS_PRINTEXPR,
            _("Print the expression along with the image"));
    purple_plugin_pref_frame_add(frame, ppref);

#ifndef _WIN32
    purple_plugin_pref_frame_add(frame,purple_plugin_pref_new_with_label(
                _("Only mimeTeX")));
#endif

    // Reverse image
    ppref = purple_plugin_pref_new_with_name_and_label(
            PREFS_NEGATE,
            _("Negate image. Select for dark background."));
    purple_plugin_pref_frame_add(frame, ppref);

    // Smash expression
    ppref = purple_plugin_pref_new_with_name_and_label(
            PREFS_SMASH,
            _("Smash expression. Horizontally compact."));
    purple_plugin_pref_set_type(ppref, PURPLE_PLUGIN_PREF_CHOICE);
    purple_plugin_pref_add_choice(ppref, _("Default"), "");
    purple_plugin_pref_add_choice(ppref, _("Smash"),    "\\smash");
    purple_plugin_pref_add_choice(ppref, _("No smash"), "\\nosmash");
    purple_plugin_pref_frame_add(frame, ppref);

#ifndef _WIN32
    purple_plugin_pref_frame_add(frame,purple_plugin_pref_new_with_label(
                _("For both renderers")));
#endif

    // Font size
    ppref = purple_plugin_pref_new_with_name_and_label(
            PREFS_FONT_SIZE,
            _("Font size"));
    purple_plugin_pref_set_type(ppref, PURPLE_PLUGIN_PREF_CHOICE);
    purple_plugin_pref_add_choice(ppref, "0", GINT_TO_POINTER(1));
    purple_plugin_pref_add_choice(ppref, "1", GINT_TO_POINTER(1));
    purple_plugin_pref_add_choice(ppref, "2", GINT_TO_POINTER(2));
    purple_plugin_pref_add_choice(ppref, "3", GINT_TO_POINTER(3));
    purple_plugin_pref_add_choice(ppref, "4", GINT_TO_POINTER(4));
    purple_plugin_pref_add_choice(ppref, "5", GINT_TO_POINTER(5));
    purple_plugin_pref_add_choice(ppref, "6", GINT_TO_POINTER(6));
    purple_plugin_pref_add_choice(ppref, "7", GINT_TO_POINTER(7));
    purple_plugin_pref_frame_add(frame, ppref);

    // Style
    ppref = purple_plugin_pref_new_with_name_and_label(
            PREFS_STYLE,
            _("Force style"));
    purple_plugin_pref_set_type(ppref, PURPLE_PLUGIN_PREF_CHOICE);
    purple_plugin_pref_add_choice(ppref, "Auto", "");
    purple_plugin_pref_add_choice(ppref, "Displaystyle", "\\displaystyle");
    purple_plugin_pref_add_choice(ppref, "Textstyle",    "\\textstyle");
    purple_plugin_pref_frame_add(frame, ppref);

    // Font Color
    ppref = purple_plugin_pref_new_with_name_and_label(
            PREFS_FONT_COLOR,
            _("Font color, use default if you have an old version of mimeTeX."));
    purple_plugin_pref_set_type(ppref, PURPLE_PLUGIN_PREF_CHOICE);
    purple_plugin_pref_add_choice(ppref, _("Default"), "");
    purple_plugin_pref_add_choice(ppref, _("Black"),   "black");
    purple_plugin_pref_add_choice(ppref, _("White"),   "white");
    purple_plugin_pref_add_choice(ppref, _("Red"),     "red");
    purple_plugin_pref_add_choice(ppref, _("Cyan"),    "cyan");
    purple_plugin_pref_add_choice(ppref, _("Blue"),    "blue");
    purple_plugin_pref_add_choice(ppref, _("Yellow"),  "yellow");
    purple_plugin_pref_add_choice(ppref, _("Green"),   "green");
    purple_plugin_pref_add_choice(ppref, _("Magenta"), "magenta");
    purple_plugin_pref_frame_add(frame, ppref);

    // Prepend
    ppref = purple_plugin_pref_new_with_name_and_label(
            PREFS_PREPEND,
            _("Prepend all expressions: (example \\gammacorrection{3.5})"));
    purple_plugin_pref_frame_add(frame, ppref);

    return frame;
}

static PurplePluginUiInfo prefs_info = {
    get_plugin_pref_frame,
    0,
    NULL,
    /* Padding */
    NULL,
    NULL,
    NULL,
    NULL
};

static PurplePluginInfo info =
{
    PURPLE_PLUGIN_MAGIC,
    PURPLE_MAJOR_VERSION,
    PURPLE_MINOR_VERSION,
    PURPLE_PLUGIN_STANDARD,                           /**< type           */
    NULL,                                             /**< ui_requirement */
    0,                                                /**< flags          */
    NULL,                                             /**< dependencies   */
    PURPLE_PRIORITY_DEFAULT,                          /**< priority       */
    "core-micket-" PLUGIN_NAME,                       /**< id             */
    PLUGIN_NAME,                                      /**< name           */
    PLUGIN_VERSION,                                   /**< version        */
    /**<  summary        */
    // I'm not allowed to translate this?
    ("To display LaTeX formula into Pidgin conversation."),
    /**<  description    */
    ("Put LaTeX-code between $$ ... $$ markup to have it displayed as a "
        "picture in your conversation.\n"
        "Remember that your contact needs an similar plugin or else he will "
        "just see the pure LaTeX-code.\n"
        "You must have mimeTeX or mathTeX installed (in your PATH)"),
    /**< author */
    "Mikael Öhman <micketeer@gmail.com>\n"
        "Based on LaTeX-plugin by:\n"
        "Benjamin Moll <qjuh@users.sourceforge.net>\n"
        "Nicolas Schoonbroodt <nicolas@ffsa.be>\n"
        "Nicolai Stange <nic-stange@t-online.de>\n",
    "http://pidgintex.googlecode.com",                /**< homepage       */
    plugin_load,                                      /**< load           */
    plugin_unload,                                    /**< unload         */
    NULL,                                             /**< destroy        */
    NULL,                                             /**< ui_info        */
    "http://www.forkosh.com/mimetex.html\n"
    "http://www.forkosh.com/mathtex.html",            /**< extra_info     */
    &prefs_info,
    NULL,
    /* padding */
    NULL,
    NULL,
    NULL,
    NULL
};

static void init_plugin(PurplePlugin *plugin)
{
#ifdef ENABLE_NLS
	bindtextdomain(PLUGIN_NAME, PP_LOCALEDIR);
	bind_textdomain_codeset(PLUGIN_NAME, "UTF-8");
#endif
    purple_prefs_add_none  (PREFS_BASE);
    purple_prefs_add_bool  (PREFS_SENDIMAGE,  FALSE);
    purple_prefs_add_bool  (PREFS_PRINTEXPR,  TRUE);
    purple_prefs_add_bool  (PREFS_NEGATE,     FALSE);
    purple_prefs_add_int   (PREFS_FONT_SIZE,  3);
    purple_prefs_add_string(PREFS_SMASH,      "");
    purple_prefs_add_string(PREFS_STYLE,      "");
    purple_prefs_add_string(PREFS_FONT_COLOR, "");
#ifndef _WIN32
    purple_prefs_add_string(PREFS_RENDERER,   "mimetex");
#endif
    purple_prefs_add_string(PREFS_PREPEND,    "");
}

PURPLE_INIT_PLUGIN(pidginTeX, init_plugin, info)

