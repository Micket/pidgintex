/*
 * pidginTeX.c
 * Mikael Öhman (micketeer@gmail.com)
 *
 * Built from
 * LaTeX.c
 * pidgin-latex plugin
 *
 * PLEASE, send any comment, bug report, etc. to the trackers at sourceforge.net
 *
 * Copyright (C) 2006-2007 Benjamin Moll (qjuh@users.sourceforge.net)
 * some portions : Copyright (C) 2004-2006 Nicolas Schoonbroodt (nicolas@ffsa.be)
 *                 Copyright (C) 2004-2006 GRIm@ (thegrima@altern.org).
 *         Copyright (C) 2004-2006 Eric Betts (bettse@onid.orst.edu).
 * Windows port  : Copyright (C) 2005-2006 Nicolai Stange (nic-stange@t-online.de)
 * Other portions heavily inspired and copied from gaim sources
 *
 *
 * Copyright (C) 1998-2007 Pidgin developers pidgin.im
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; This document is under the scope of
 * the version 2 of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see COPYING); if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "pidginTeX.h"

static void open_log(PurpleConversation *conv)
{
    conv->logs = g_list_append(NULL, purple_log_new(conv->type == PURPLE_CONV_TYPE_CHAT ? 
                PURPLE_LOG_CHAT : PURPLE_LOG_IM, 
                conv->name, conv->account, conv, time(NULL), NULL));
}

#ifdef _WIN32
void win32_purple_notify_error(char *prep)
{
    char *errmsg=NULL;
    char *finalmsg=NULL;
    if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, 
                GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (void*)&errmsg, 0, NULL))
    {
        purple_notify_error(NULL, PLUGIN_NAME,"Can't display error message.", NULL);
        return;
    }

    if(prep)
    {
        finalmsg=malloc((strlen(errmsg)+ strlen(prep) + 3)*sizeof(char));
        if(!finalmsg)
        {
            purple_notify_error(NULL, PLUGIN_NAME, "Can't display error message.", NULL);
            LocalFree(errmsg); /* we can't do anything more for you */
            return;
        }
        strcpy(finalmsg, prep);
        strcat(finalmsg, ": ");
        strcat(finalmsg, errmsg);
        LocalFree(errmsg);
    }
    else
    {
        finalmsg = malloc((strlen(errmsg)+1)*sizeof(char));
        if(!finalmsg)
        {
            purple_notify_error(NULL, PLUGIN_NAME, "Can't display error message.", NULL);
            LocalFree(errmsg); /* we can't do anything more for you */
            return;
        }
        strcpy(finalmsg, errmsg);
        LocalFree(errmsg);
    }
    purple_notify_error(NULL, PLUGIN_NAME, finalmsg, NULL);
    free(finalmsg);
    return;
}
#endif

char* searchPATH(const char const *file)
{
    char *cmd=NULL;
#ifdef _WIN32
    DWORD sz = SearchPath(NULL, file, ".exe", 0, cmd, NULL);
    if (cmd = malloc(sz+1))
    {
        if (!SearchPath(NULL, file, ".exe", sz+1, cmd, NULL));
        {
            free(cmd);
            cmd=NULL;
        }
    }
#else
    cmd = g_strdup(file);
#endif
    return cmd;
}

static int execute(char *cmd)
{
#ifdef _WIN32
    int exitcode = 0;
    STARTUPINFO sup;
    PROCESS_INFORMATION pi;
    ZeroMemory( &sup, sizeof(sup) );
    ZeroMemory( &pi,   sizeof(pi) );
    sup.cb          = sizeof(sup);
    sup.wShowWindow = SW_HIDE;
    sup.dwFlags     = STARTF_USESHOWWINDOW;

    if(!CreateProcess(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &sup, &pi))
    {
        win32_purple_notify_error(cmd);
        free(cmd);
        return -1;
    }
    free(cmd);
    if(WAIT_OBJECT_0!=WaitForSingleObjectEx(pi.hProcess, INFINITE, FALSE))
    {
        win32_purple_notify_error(cmd);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return -1;
    }
    do
    {
        if(!GetExitCodeThread(pi.hThread, &exitcode))
        {
            win32_purple_notify_error(cmd);
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

static char* mathfont[] = {"tiny","footnotesize","normalsize","large",
                 "Large","LARGE","huge","Huge"};
static gboolean latex_to_image(char *latex, char **file_img)
{
    // the following is new and related to temporary-filename-generation
    FILE* tmpfile = purple_mkstemp(file_img,TRUE);
    if(!*file_img)
    {
        purple_notify_error(NULL, PLUGIN_NAME, "Couldn't creat temporary file", NULL);
        unlink(*file_img);
        free(*file_img);
        return FALSE;
    }
    fclose(tmpfile);

    const char *renderer = purple_prefs_get_string(PREFS_RENDERER);
    char *cmdTeX = searchPATH(renderer);
    if (!cmdTeX)
    {
        purple_notify_error(NULL, PLUGIN_NAME, "Couldn't find the selected renderer.", NULL);
        free(cmdTeX);
        return FALSE;
    }

    // Build the options the options
    int  fontsize   = purple_prefs_get_int(PREFS_FONT_SIZE);
    char *prepend   = (char*)purple_prefs_get_string(PREFS_PREPEND);
    char *fontcolor = (char*)purple_prefs_get_string(PREFS_FONT_COLOR);
    char *style     = (char*)purple_prefs_get_string(PREFS_STYLE);
    char *reverse   = purple_prefs_get_bool(PREFS_NEGATE) ? "\\reverse" : "";
    char *smash     = (char*)purple_prefs_get_string(PREFS_SMASH);
    char* cmdparam;

    if (!strcmp(renderer, "mimetex"))
    {
        if (strlen(fontcolor))
            cmdparam = g_strdup_printf(
                "\"%s\" -d -s %d \"\\%s%s%s%s{%s %s}\" > %s", \
                cmdTeX, fontsize, fontcolor, reverse, style, smash, prepend, latex, *file_img);
        else
            cmdparam = g_strdup_printf(
                "\"%s\" -d -s %d \"%s%s%s{%s %s}\" > %s", \
                cmdTeX, fontsize, reverse, style, smash, prepend, latex, *file_img);
    }
    else //if (!strcmp(renderer,"mathtex"))
    {
        if (strlen(fontcolor))
            cmdparam = g_strdup_printf( \
                "\"%s\" -m 0 \"\\png\\usepackage{color}\\color{%s}%s\\%s %s %s\" -o %s", \
                cmdTeX, fontcolor, style, mathfont[fontsize], prepend, latex, *file_img);
        else
            cmdparam = g_strdup_printf( \
                "\"%s\" -m 0 \"\\png%s%s\\%s %s %s\" -o %s", \
                cmdTeX, style, mathfont[fontsize], prepend, latex, *file_img);
    }
    free(cmdTeX);

    fprintf(stderr, "%s\n",cmdparam);
    if(execute(cmdparam))
    {
        purple_notify_error(NULL, PLUGIN_NAME, "Failed to execute command.", NULL);
        free(cmdparam);
        return FALSE;
    }
    free(cmdparam);

    // Ugly hack becuase mathtex makes up it's own filename (appends fileext). sigh
    if (!strcmp(renderer,"mathtex"))
    {
        char* file_img2 = g_strdup_printf("%s.png",*file_img);
        unlink(*file_img);
        free(*file_img);
        *file_img = file_img2;
    } 
    return TRUE;
}

static gboolean analyse(char **msg, char *startdelim, char *enddelim)
{
    char *ptr1, *ptr2;
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

        // Creates the image in file_img
        if (!latex_to_image(tex, &file_img)) { *ptr2 = *startdelim; return FALSE; };
        // loading image
        if (!g_file_get_contents(file_img, &filedata, &size, &error))
        {
            purple_notify_error(NULL, "pidginTeX", error->message, NULL);
            g_error_free(error);
            *ptr2 = *startdelim;
            return FALSE;
        }
        if (strcmp(purple_prefs_get_string(PREFS_RENDERER),"mathtex"))
            unlink(file_img);
        free(file_img);

        int idimg = purple_imgstore_add_with_id(filedata, size, "pidginTeX");
        if (idimg == 0)
        {
            purple_notify_error(NULL, PLUGIN_NAME, "Failed to store image.", NULL);
            *ptr2 = *startdelim;
            return FALSE;
        }

        *ptr1 = '\0';
        char* tmpmsg;
        if (purple_prefs_get_bool(PREFS_PRINTEXPR))
            tmpmsg = g_strdup_printf("%s<img id=\"%d\"> %s%s",
                *msg,idimg,tex,&ptr2[strlen(enddelim)]);
        else
            tmpmsg = g_strdup_printf("%s<img id=\"%d\">%s",
                *msg,idimg,&ptr2[strlen(enddelim)]);
        free(*msg);
        *msg = tmpmsg;
    }
    return TRUE;
}

static gboolean pidgin_latex_write(PurpleConversation *conv, const char *nom, 
        char *newmsg, PurpleMessageFlags messFlag, char *originalmsg)
{
    gboolean logflag = purple_conversation_is_logging(conv);

    if (logflag)
    {
        GList *log;
        if (!conv->logs)
            open_log(conv);
        log = conv->logs;
        while (log)
        {
            purple_log_write((PurpleLog *)log->data, messFlag, nom, time(NULL), originalmsg);
            log = log->next;
        }
        purple_conversation_set_logging(conv,FALSE);
    }

    if      (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_CHAT)
        purple_conv_chat_write(PURPLE_CONV_CHAT(conv), nom, newmsg, messFlag, time(NULL));
    else if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM)
        purple_conv_im_write(  PURPLE_CONV_IM(conv),   nom, newmsg, messFlag, time(NULL));

    if (logflag)
        purple_conversation_set_logging(conv,TRUE);

    return TRUE;
}

static gboolean message_write(PurpleAccount *account, const char *who, 
        char **buffer, PurpleConversation *conv, PurpleMessageFlags flags)
{
    char *tmp;
    if (!strstr(*buffer,TEX_DELIMITER) || !(tmp = strdup(*buffer))) 
        return FALSE;

    //fprintf(stderr, "Modify log = \n%s\n",*buffer);

    if (analyse(&tmp, TEX_DELIMITER, TEX_DELIMITER) )
    {
        //fprintf(stderr, "Writing log\n");
        pidgin_latex_write(conv, who, tmp, flags, *buffer);
        //free(*buffer);
        //*buffer = tmp;
        free(tmp);
        return TRUE;
    }
    free(tmp);
    return FALSE;
}

static gboolean message_send(PurpleAccount *account, const char *who, 
        char **buffer, PurpleConversation *conv, PurpleMessageFlags flags)
{
    //fprintf(stderr, "Modify message = \n%s\n",*buffer);
    if (purple_prefs_get_bool(PREFS_SENDIMAGE) && strstr(*buffer,TEX_DELIMITER)
        && analyse(buffer, TEX_DELIMITER, TEX_DELIMITER))
    {
        flags |= PURPLE_MESSAGE_IMAGES;
    }
    //fprintf(stderr, "Sending = \n%s\n",*buffer); 
    return FALSE;
}

static gboolean plugin_load(PurplePlugin *plugin)
{
    void *conv_handle = purple_conversations_get_handle();
    purple_signal_connect(conv_handle, "writing-im-msg",   plugin, PURPLE_CALLBACK(message_write), NULL);
    purple_signal_connect(conv_handle, "writing-chat-msg", plugin, PURPLE_CALLBACK(message_write), NULL);
    purple_signal_connect(conv_handle, "sending-im-msg",   plugin, PURPLE_CALLBACK(message_send), NULL);
    purple_signal_connect(conv_handle, "sending-chat-msg", plugin, PURPLE_CALLBACK(message_send), NULL);
    purple_debug(PURPLE_DEBUG_INFO, PLUGIN_NAME, PLUGIN_NAME " loaded\n");
    return TRUE;
}

static gboolean plugin_unload(PurplePlugin * plugin)
{
    void *conv_handle = purple_conversations_get_handle();
    purple_signal_disconnect(conv_handle, "writing-im-msg",   plugin, PURPLE_CALLBACK(message_write));
    purple_signal_disconnect(conv_handle, "writing-chat-mg",  plugin, PURPLE_CALLBACK(message_write));
    purple_signal_disconnect(conv_handle, "sending-im-msg",   plugin, PURPLE_CALLBACK(message_send));
    purple_signal_disconnect(conv_handle, "sending-chat-msg", plugin, PURPLE_CALLBACK(message_send));
    return TRUE;
}

static PurplePluginPrefFrame * get_plugin_pref_frame(PurplePlugin *plugin)
{
    PurplePluginPrefFrame *frame = purple_plugin_pref_frame_new();
    PurplePluginPref *ppref;
    

    ppref = purple_plugin_pref_new_with_label(
            "All of these choices will only change your image, not your message.");
    purple_plugin_pref_frame_add(frame, ppref);

    // Do send image
    ppref = purple_plugin_pref_new_with_name_and_label(
            PREFS_SENDIMAGE,
            "Send image. Make sure images can be sent before selecting!");
    purple_plugin_pref_frame_add(frame, ppref);

    // Select renderer
    ppref = purple_plugin_pref_new_with_name_and_label(
            PREFS_RENDERER,
            "Choose a renderer. Program must be installed in path.");
    purple_plugin_pref_set_type(ppref, PURPLE_PLUGIN_PREF_CHOICE);
    purple_plugin_pref_add_choice(ppref, "mimeTeX", "mimetex");
    purple_plugin_pref_add_choice(ppref, "mathTeX", "mathtex");
    purple_plugin_pref_frame_add(frame, ppref);

    // Print expression
    ppref = purple_plugin_pref_new_with_name_and_label(
            PREFS_PRINTEXPR,
            "Print the expression along with the image");
    purple_plugin_pref_frame_add(frame, ppref);

    ppref = purple_plugin_pref_new_with_label("Only mimeTeX");
    purple_plugin_pref_frame_add(frame, ppref);
    // Reverse image
    ppref = purple_plugin_pref_new_with_name_and_label(
            PREFS_NEGATE,
            "Negate image. Select for dark background.");
    purple_plugin_pref_frame_add(frame, ppref);

    // Smash expression
    ppref = purple_plugin_pref_new_with_name_and_label(
            PREFS_SMASH,
            "Smash expression. Horizontally compact.");
    purple_plugin_pref_set_type(ppref, PURPLE_PLUGIN_PREF_CHOICE);
    purple_plugin_pref_add_choice(ppref, "Default", "");
    purple_plugin_pref_add_choice(ppref, "Smash",    "\\smash");
    purple_plugin_pref_add_choice(ppref, "No smash", "\\nosmash");
    purple_plugin_pref_frame_add(frame, ppref);

    
    ppref = purple_plugin_pref_new_with_label("For both renderers");
    purple_plugin_pref_frame_add(frame, ppref);
    // Font size
    ppref = purple_plugin_pref_new_with_name_and_label(
            PREFS_FONT_SIZE,
            "Font size");
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
            "Force style");
    purple_plugin_pref_set_type(ppref, PURPLE_PLUGIN_PREF_CHOICE);
    purple_plugin_pref_add_choice(ppref, "Auto", "");
    purple_plugin_pref_add_choice(ppref, "Displaystyle", "\\displaystyle");
    purple_plugin_pref_add_choice(ppref, "Textstyle",    "\\textstyle");
    purple_plugin_pref_frame_add(frame, ppref);

    // Font Color
    ppref = purple_plugin_pref_new_with_name_and_label(
            PREFS_FONT_COLOR,
            "Font color, use default if you have an old version of mimeTeX.");
    purple_plugin_pref_set_type(ppref, PURPLE_PLUGIN_PREF_CHOICE);
    purple_plugin_pref_add_choice(ppref, "Default", "");
    purple_plugin_pref_add_choice(ppref, "Black",   "black");
    purple_plugin_pref_add_choice(ppref, "White",   "white");
    purple_plugin_pref_add_choice(ppref, "Red",     "red");
    purple_plugin_pref_add_choice(ppref, "Cyan",    "cyan");
    purple_plugin_pref_add_choice(ppref, "Blue",    "blue");
    purple_plugin_pref_add_choice(ppref, "Yellow",  "yellow");
    purple_plugin_pref_add_choice(ppref, "Green",   "green");
    purple_plugin_pref_add_choice(ppref, "Magenta", "magenta");
    purple_plugin_pref_frame_add(frame, ppref);
    
    // Prepend
    ppref = purple_plugin_pref_new_with_name_and_label(
            PREFS_PREPEND,
            "Prepend all expressions: (example \\gammacorrection{3.5})");
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
    "To display LaTeX formula into Pidgin conversation.",
    /**<  description    */
    "Put LaTeX-code between $$ ... $$ markup to have it displayed as a "
        "picture in your conversation.\n"
        "Remember that your contact needs an similar plugin or else he will "
        "just see the pure LaTeX-code.\n"
        "You must have mimeTeX or mathTeX installed (in your PATH)",
    /**< author */
    "Mikael Öhman <micketeer@gmail.com>\n"
        "Based on LaTeX-plugin by:\n"
        "Benjamin Moll <qjuh@users.sourceforge.net>\n"
        "Nicolas Schoonbroodt <nicolas@ffsa.be>\n"
        "Nicolai Stange <nic-stange@t-online.de>\n",
    "http://www.micket.com",                          /**< homepage       */
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
    purple_prefs_add_none  (PREFS_BASE);
    purple_prefs_add_bool  (PREFS_SENDIMAGE,  FALSE);
    purple_prefs_add_bool  (PREFS_PRINTEXPR,  TRUE);
    purple_prefs_add_bool  (PREFS_NEGATE,     FALSE);
    purple_prefs_add_int   (PREFS_FONT_SIZE,  3);
    purple_prefs_add_string(PREFS_SMASH,      "");
    purple_prefs_add_string(PREFS_STYLE,      "");
    purple_prefs_add_string(PREFS_FONT_COLOR, "");
    purple_prefs_add_string(PREFS_RENDERER,   "mimetex");
    purple_prefs_add_string(PREFS_PREPEND,    "");
}

PURPLE_INIT_PLUGIN(pidginTeX, init_plugin, info)

