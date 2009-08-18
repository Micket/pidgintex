/* pidginTeX.c
 * Copyright 2008 Mikael Öhman
 * Initially based on the pidgin-latex plugin
 *
 * This file is part of pidginTeX.
 *
 * pidginTeX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * pidginTeX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with pidginTeX. If not, see <http://www.gnu.org/licenses/>.
 */

#include "pidginTeX.h"

#ifdef G_OS_WIN32
static int execute(gchar *cmd)
{
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
        purple_debug_error(PLUGIN_NAME, "Failed to create process for: %s\n",cmd);
        return -1;
    }
    if (WAIT_OBJECT_0!=WaitForSingleObjectEx(pi.hProcess, INFINITE, FALSE))
    {
        purple_debug_error(PLUGIN_NAME, "Wait for single object failed: %s\n", cmd);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return -1;
    }
    do
    {
        if (!GetExitCodeThread(pi.hThread, &exitcode))
        {
            purple_debug_error(PLUGIN_NAME, "Failed to get exit code for: %s\n", cmd);
            return -1;
        }
        Sleep(10);
    } while(exitcode==STILL_ACTIVE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return exitcode;
}
#endif

static gboolean latex_to_image(gchar *tex, gchar** filedata, gsize* size)
{
    purple_debug_info(PLUGIN_NAME,"Rendering expression: %s\n",tex);
    // Build the options
    const gchar *renderer   = purple_prefs_get_string(PREFS_RENDERER);
    const gchar *prepend    = purple_prefs_get_string(PREFS_PREPEND);
    const gchar *fontcolor  = purple_prefs_get_string(PREFS_FONT_COLOR);
    const gchar *style      = purple_prefs_get_string(PREFS_STYLE);
    const gchar *smash      = purple_prefs_get_string(PREFS_SMASH);
    const gchar *reverse    = purple_prefs_get_bool(PREFS_NEGATE) ? "\\reverse" : "";
    const gint   fontsize   = purple_prefs_get_int(PREFS_FONT_SIZE);
    const gboolean usecolor = strlen(fontcolor) > 0;

    gchar* full_path = g_find_program_in_path(renderer);
    if (full_path)
        g_free(full_path);
    else
    {
        purple_debug_error(PLUGIN_NAME,"Failed to find renderer: %s\n", renderer);
        gchar *err_msg = g_strdup_printf(_("Failed to find: %s."
            " Make sure you have it installed or change renderer.\n"),renderer); 
        purple_notify_error(NULL, PLUGIN_NAME, err_msg, NULL);
        g_free(err_msg);
        return FALSE;
    }

    gchar *file_img;
    FILE* tmpfile = purple_mkstemp(&file_img,TRUE);
    if(!file_img)
    {
        purple_debug_error(PLUGIN_NAME, "Couldn't create temporary file.\n");
        return FALSE;
    }
    fclose(tmpfile);

    // Cleaning up the text a bit
    gchar *tex_fixed  = purple_markup_strip_html(tex);
    gchar *tex_fixed2 = purple_unescape_html(tex_fixed);
    g_free(tex_fixed); tex_fixed = tex_fixed2;
    tex_fixed2 = purple_strcasereplace(tex_fixed, "&apos;","'");
    g_free(tex_fixed); tex_fixed = tex_fixed2;
    tex_fixed2 = purple_strreplace(tex_fixed, "\"", "''"); 
    g_free(tex_fixed); tex_fixed = tex_fixed2;

    gchar* cmdparam = NULL;
    if (!strcmp(renderer, "mimetex"))
    {
        cmdparam = g_strdup_printf(
            "%s \"\\color{%s}%s\\%s%s%s{%s %s}\" -e %s",
            renderer, usecolor ? fontcolor : "black", style, mathfont[fontsize],
            reverse, smash, 
            prepend, tex_fixed, file_img);
    }
    else //if (!strcmp(renderer,"mathtex"))
    {
        cmdparam = g_strdup_printf( 
            "%s -m 0 \"\\png\\usepackage{color}\\color{%s}%s\\%s %s %s\" -o %s",
            renderer, usecolor ? fontcolor : "black", style, mathfont[fontsize], 
            prepend, tex_fixed, file_img);
    }
    g_free(tex_fixed);
#ifndef G_OS_WIN32
    gchar* cmdparam2 = purple_strreplace(cmdparam, "\\","\\\\");
    g_free(cmdparam); cmdparam = cmdparam2;
#endif

    purple_debug_info(PLUGIN_NAME,"Trying to execute command: %s\n",cmdparam);
    
#ifdef G_OS_WIN32
    gint rt = execute(cmdparam);
    gboolean success = rt == 0;
#else
    gint rt = 0;
    void(*old_handler)(int) = signal(SIGCHLD,SIG_DFL);
    gboolean success = g_spawn_command_line_sync(cmdparam,NULL,NULL,&rt,NULL);
    signal(SIGCHLD,old_handler);
#endif
    if (!success || (success && WEXITSTATUS(rt)) )
    {
        purple_debug_error(PLUGIN_NAME, "Failed to execute command: %s\n",cmdparam);
        gchar *err_msg = g_strdup_printf(_("Failed to execute command:\n%s\n"
            "Something might be wrong in the LaTeX expression.\n"),cmdparam);
        purple_notify_error(NULL, PLUGIN_NAME, err_msg, NULL);
        g_free(cmdparam);
        g_free(err_msg);
        g_unlink(file_img); g_free(file_img);
        return FALSE;
    }
    g_free(cmdparam);

    // Ugly hack becuase mathtex makes up it's own filename (appends fileext). sigh
    // It seems like I'll have to keep it, since the mathtex author seems to 
    // think this is a good idea.
    if (!strcmp(renderer,"mathtex"))
    {
        gchar* file_img2 = g_strdup_printf("%s.png",file_img);
        g_unlink(file_img);
        g_free(file_img);
        file_img = file_img2;
    }

    GError *error = NULL;
    success = g_file_get_contents(file_img, filedata, size, &error);
    if (!success)
    {
        purple_debug_error(PLUGIN_NAME, error->message);
        g_error_free(error);
    }
    g_unlink(file_img); g_free(file_img);
    return success;
}

static gboolean analyse(const gchar *msg, gchar** outmsg)
{
    if (!*msg) return FALSE; 
    gchar **split = g_strsplit(msg,TEX_DELIMITER,-1);
    if (!split[1]) return FALSE;
    gboolean print_expr = purple_prefs_get_bool(PREFS_PRINTEXPR);
    GString* out = g_string_sized_new(strlen(msg));
    gint i, imgcounter = 0;
    for (i = 0; split[i]; i++)
    {
        if (i%2 == 0) // Every odd is tex-expressions
            g_string_append(out,split[i]);
        else 
        {
            gchar* filedata;
            gsize size;
            if (!latex_to_image(split[i], &filedata, &size))
            {
                g_string_append_printf(out, "$$%s$$ (failed)", split[i]);
                continue;
            }
            gchar* name = g_strdup_printf("pidginTeX-%s-%d.png",
                purple_date_format_long(NULL), imgcounter++);
            gint idimg = purple_imgstore_add_with_id(filedata, size, name);
            g_free(name);
            if (idimg == 0)
            {
                purple_debug_error(PLUGIN_NAME, "Failed to store image. Data size = %d\n",(gint)size);
                g_string_append_printf(out, "$$%s$$ (failed)", split[i]);
                continue;
            }
            imageref = g_list_prepend(imageref,GINT_TO_POINTER(idimg));
            g_string_append_printf(out, "<img id=\"%d\" alt=\"%s\"> %s", 
                idimg, split[i], print_expr? split[i] : "");
        }
    }
    g_strfreev(split);
    *outmsg = g_string_free(out,FALSE);
    purple_debug_info(PLUGIN_NAME, "Modified message: %s\n",*outmsg);
    return TRUE;
}

////////////////////// Intercepting functions /////////////////////////////////
/* Emitted before sending an IM to a user. */
static void message_send(PurpleAccount *account, gchar *recipient, gchar **message)
{
    purple_debug_info(PLUGIN_NAME,"message_send:\n%s\n",*message);
    if (!purple_prefs_get_bool(PREFS_SENDIMAGE) || !analyse(*message, &modifiedmsg))
        return;
    PurpleConversation* conv = purple_find_conversation_with_account(
        PURPLE_CONV_TYPE_ANY, recipient, account);
    if (conv && conv->features & PURPLE_CONNECTION_NO_IMAGES)
    {
        purple_debug_error(PLUGIN_NAME, "Image is not sent. "
            "This conversation does not support images.\n");
        modoff = TRUE;
        // This triggers another "displaying_msg", thus the need to turn that off for a moment.
        purple_conv_present_error(recipient, account, 
            _("Image is not sent. This conversation does not support images."));
        modoff = FALSE;
    }
    else 
    {
        g_free(*message);
        *message = g_strdup(modifiedmsg);
    }
}

/* Emitted when a conversation is deleted. */
static void deleting_conv(PurpleConversation *conv)
{
    purple_debug_info(PLUGIN_NAME, "Clearing imgstore.\n");
    void fun(gpointer data, gpointer udata) {
        purple_imgstore_unref_by_id(GPOINTER_TO_INT(data));
    }
    g_list_foreach(imageref, fun, NULL);
    g_list_free(imageref);
    imageref = NULL;
}

/* Emitted when a conversation is created */
static void history_write(PurpleConversation *c)
{
	PidginConversation *gtkconv = PIDGIN_CONVERSATION(c);
	g_return_if_fail(gtkconv != NULL || gtkconv->imhtml != NULL);
    gchar* history = gtk_imhtml_get_markup(GTK_IMHTML(gtkconv->imhtml));
    gchar* modifiedhistory;
    if (analyse(history, &modifiedhistory))
	{
        gtk_imhtml_clear(GTK_IMHTML(gtkconv->imhtml));
		gtk_imhtml_append_text(GTK_IMHTML(gtkconv->imhtml), modifiedhistory, 0);
		g_free(modifiedhistory);
    }
}

/* Emitted when a message is about to be displayed */
static gboolean displaying_msg(PurpleAccount *account, const char *who,
    char **message, PurpleConversation *conv, PurpleMessageFlags flags)
{
    purple_debug_info(PLUGIN_NAME, "displaying_msg: %s\n",*message);
    if (!modoff && (modifiedmsg || analyse(*message, &modifiedmsg)))
    {
        g_free(*message);
        *message = modifiedmsg;
        modifiedmsg = NULL;
    }
    return FALSE;
}

static gboolean plugin_load(PurplePlugin *plugin)
{
    void *purple_handle = purple_conversations_get_handle();
    void *pidgin_handle = pidgin_conversations_get_handle();
    purple_signal_connect(purple_handle,"sending-im-msg",       plugin,PURPLE_CALLBACK(message_send), NULL);
    purple_signal_connect(purple_handle,"deleting-conversation",plugin,PURPLE_CALLBACK(deleting_conv), NULL);
    purple_signal_connect_priority(purple_handle, "conversation-created", plugin,
        PURPLE_CALLBACK(history_write), NULL, PURPLE_PRIORITY_HIGHEST); 
    purple_signal_connect(pidgin_handle,"displaying-im-msg",    plugin,PURPLE_CALLBACK(displaying_msg), NULL);
    purple_signal_connect(pidgin_handle,"displaying-chat-msg",  plugin,PURPLE_CALLBACK(displaying_msg), NULL);
    originalmsg = modifiedmsg = NULL;
    modoff = FALSE;
    return TRUE;
}

static gboolean plugin_unload(PurplePlugin * plugin)
{
    void *purple_handle = purple_conversations_get_handle();
    void *pidgin_handle = pidgin_conversations_get_handle();
    purple_signal_disconnect(purple_handle,"sending-im-msg",       plugin,PURPLE_CALLBACK(message_send));
    purple_signal_disconnect(purple_handle,"deleting-conversation",plugin,PURPLE_CALLBACK(deleting_conv));
    purple_signal_disconnect(purple_handle,"conversation-created", plugin,PURPLE_CALLBACK(history_write)); 
    purple_signal_disconnect(pidgin_handle,"displaying-im-msg",    plugin,PURPLE_CALLBACK(displaying_msg));
    purple_signal_disconnect(pidgin_handle,"displaying-chat-msg",  plugin,PURPLE_CALLBACK(displaying_msg));
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
            _("Send image in IM. Make sure images can be sent before selecting."));
    purple_plugin_pref_frame_add(frame, ppref);

    // Select renderer
    ppref = purple_plugin_pref_new_with_name_and_label(
            PREFS_RENDERER,
            _("Choose a renderer. Program must be installed in system path."));
    purple_plugin_pref_set_type(ppref, PURPLE_PLUGIN_PREF_CHOICE);
    purple_plugin_pref_add_choice(ppref, "mimeTeX", "mimetex");
#ifndef G_OS_WIN32
    purple_plugin_pref_add_choice(ppref, "mathTeX", "mathtex");
#endif
    purple_plugin_pref_frame_add(frame, ppref);

    // Print expression
    ppref = purple_plugin_pref_new_with_name_and_label(
            PREFS_PRINTEXPR,
            _("Print the expression along with the image"));
    purple_plugin_pref_frame_add(frame, ppref);

    purple_plugin_pref_frame_add(frame,purple_plugin_pref_new_with_label(
                _("Only mimeTeX")));

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

    purple_plugin_pref_frame_add(frame,purple_plugin_pref_new_with_label(
                _("For both renderers")));

    // Font size
    ppref = purple_plugin_pref_new_with_name_and_label(
            PREFS_FONT_SIZE,
            _("Font size"));
    purple_plugin_pref_set_type(ppref, PURPLE_PLUGIN_PREF_CHOICE);
    purple_plugin_pref_add_choice(ppref, "0", GINT_TO_POINTER(0));
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
            _("Font color. Use default if you have an old version of mimeTeX."));
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
    get_plugin_pref_frame, 0, NULL, NULL, NULL, NULL, NULL};

static PurplePluginInfo info =
{
    PURPLE_PLUGIN_MAGIC,
    PURPLE_MAJOR_VERSION,
    PURPLE_MINOR_VERSION,
    PURPLE_PLUGIN_STANDARD,                           /**< type           */
    PIDGIN_PLUGIN_TYPE,                               /**< ui_requirement */
    0,                                                /**< flags          */
    NULL,                                             /**< dependencies   */
    PURPLE_PRIORITY_DEFAULT,                          /**< priority       */
    PLUGIN_ID,                                        /**< id             */

    PLUGIN_NAME,                                      /**< name           */
    PLUGIN_VERSION,                                   /**< version        */
    NULL,                                             /**< summary        */
    NULL,                                             /**< description    */
    "Mikael Öhman <micketeer@gmail.com>",             /**< author         */
    "http://pidgintex.googlecode.com",                /**< homepage       */
    plugin_load,                                      /**< load           */
    plugin_unload,                                    /**< unload         */
    NULL,                                             /**< destroy        */
    NULL,                                             /**< ui_info        */
    "http://www.forkosh.com/mimetex.html\n"
    "http://www.forkosh.com/mathtex.html",            /**< extra_info     */
    &prefs_info,
    NULL, NULL, NULL, NULL, NULL
};

static void init_plugin(PurplePlugin *plugin)
{
#ifdef ENABLE_NLS
	bindtextdomain(PLUGIN_NAME, LOCALEDIR);
	bind_textdomain_codeset(PLUGIN_NAME, "UTF-8");
#endif

    info.summary = _("Renders LaTeX into a conversation.");
    info.description = _("Put LaTeX code between $$ ... $$ to have it display as a "
        "picture in your conversation.\n"
        "Remember that your contact needs an similar plugin or else he will "
        "just see the code.\n"
        "You must have mimeTeX or mathTeX installed (in your PATH).");

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

