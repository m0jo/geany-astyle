/*
 * Geany-Plugin AStyle
 * 2011
 *
 * Code formatting of current document
 *
 */

#include <string.h>
#include <stdarg.h>
#include "geanyplugin.h"
#include "lang.h"
#include <stdlib.h>

#define VERSION_STR "0.2"

GeanyPlugin		*geany_plugin;
GeanyData		*geany_data;
GeanyFunctions	*geany_functions;

PLUGIN_VERSION_CHECK(147)

PLUGIN_SET_INFO("AStyle", "Plugin for Artistic Style code formatter.\nAStyle website: http://astyle.sourceforge.net/",
                VERSION_STR, "https://launchpad.net/~anhan10");


typedef struct Settings
{
    gchar * optStr;
    Language lang;

} Settings;



// allow for different calling conventions in Linux and Windows
#ifdef _WIN32
#define STDCALL __stdcall
#else
#define STDCALL
#endif

// functions to call AStyleMain
typedef void (STDCALL* fpError)(int, const char*);      // pointer to callback error handler
typedef char* (STDCALL* fpAlloc)(unsigned long);		// pointer to callback memory allocation

char* STDCALL AStyleMain(const char*, const char*, fpError, fpAlloc);
const char* STDCALL AStyleGetVersion (void);


// global variables
GtkWidget * main_menu_item;
GtkWidget * entryOptStr;
Settings settings;


const char * getString(int id)
{
    return lang_getString(settings.lang,id);
}

void msgBox( const gchar * format, ... )
{
    va_list args;
    va_start( args, format );
    const gchar * msg = g_strdup_vprintf(format,args);
    dialogs_show_msgbox(GTK_MESSAGE_INFO, msg);
    g_free((gpointer)msg);
    va_end( args );
}


// settings

gchar * getSettingsFileName()
{
    return g_build_filename(geany->app->configdir,"astyle-plugin.conf",NULL);
}

Language detectLanguage()
{
    char * lang = getenv("LANG");

    if (lang != NULL && strncmp(lang,"de",2)==0)
        {
            return LANG_DE;
        }
    else
        return LANG_EN;
}

void setDefaultSettings()
{
    g_free(settings.optStr);
    settings.optStr = g_strdup("--style=gnu");
    settings.lang = detectLanguage();
}


int saveSettings()
{
    int ret;

    GKeyFile * config = g_key_file_new();
    g_key_file_set_string(config,"General","version", VERSION_STR);
    g_key_file_set_integer(config,"General","language",settings.lang);
    g_key_file_set_string(config,"AStyle","optStr",settings.optStr);

    gchar * filename = getSettingsFileName();
    gchar * text = g_key_file_to_data(config,NULL,NULL);
    ret = utils_write_file(filename,text);
    g_free(text);
    g_free(filename);
    g_key_file_free (config);

    return ret;
}

int loadSettings()
{
    GKeyFile * config = g_key_file_new();
    gchar * filename = getSettingsFileName();
    int ret;

    config=g_key_file_new();
    if(g_key_file_load_from_file(config,filename, G_KEY_FILE_KEEP_COMMENTS,NULL))
        {
            gint lang = g_key_file_get_integer(config,"General","language",NULL);
            if (lang < 0 || lang >=LANG_COUNT) //Invalid or unknown language
                {
                    msgBox("%s: %i",getString(3),lang);
                    settings.lang = LANG_EN;
                }
            else settings.lang = lang;

            gchar * optStr = g_key_file_get_string(config,"AStyle","optStr",NULL);
            if (optStr != NULL)
                {
                    g_free(settings.optStr);
                    settings.optStr = optStr;
                }

            ret = 0;

        }
    else
        {
            ret = 1;
        }

    g_free(filename);
    g_key_file_free (config);
    return ret;
}


// AStyle

void  STDCALL ASErrorHandler(int errorNumber, const char* errorMessage)
{
    printf( "astyle error %i\n%s\n",errorNumber,errorMessage);
}

// Allocate memory for the Artistic Style formatter
char* STDCALL ASMemoryAlloc(unsigned long memoryNeeded)
{
    // error condition is checked after return from AStyleMain
    char* buffer = g_malloc(memoryNeeded);
    return buffer;
}


char * formatCode(const char * code)
{
    GeanyDocument *doc = document_get_current();
    if (doc == NULL) return NULL;

    gchar * mode = NULL;

    if ((strcmp(doc->file_type->name,"C")==0) || (strcmp(doc->file_type->name,"C++")==0))
        mode = "--mode=c";
    else if (strcmp(doc->file_type->name,"Java")==0)
        mode = "--mode=java";
    else if (strcmp(doc->file_type->name,"C#")==0)
        mode = "--mode=cs";
    else  mode = "";


    gchar * optstr = g_strconcat(mode," ",settings.optStr, NULL);

    return  AStyleMain(code,
                       optstr,
                       ASErrorHandler,
                       ASMemoryAlloc);

    g_free(optstr);
}


// Gui events

void on_menuitem_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    GeanyDocument *doc = document_get_current();
    gint len = sci_get_length(doc->editor->sci);
    gchar * code = sci_get_contents(doc->editor->sci,len);

    if (code == NULL) return;

    gchar * formatted = formatCode(code);

    if (formatted != NULL)
        {
            gint lineNr = sci_get_current_line(doc->editor->sci);
            sci_set_text(doc->editor->sci,formatted);
            sci_goto_line(doc->editor->sci,lineNr,FALSE);
            g_free(formatted);
        }

    g_free(code);
}


void on_configure_response(GtkDialog *dialog, gint response, gpointer user_data)
{
    if (response == GTK_RESPONSE_OK || response == GTK_RESPONSE_APPLY)
        {
            g_free(settings.optStr);
            settings.optStr = g_strdup(gtk_entry_get_text((GtkEntry*)entryOptStr));
            saveSettings();
        }
}


// Plugin

GtkWidget *plugin_configure(GtkDialog *dialog)
{
    GtkWidget * vbox = gtk_vbox_new(FALSE,0);
    GtkWidget * labelOptStr = gtk_label_new(getString(0));
    gtk_container_add((GtkContainer*)vbox,labelOptStr);
    entryOptStr = gtk_entry_new();
    gtk_entry_set_text((GtkEntry*)entryOptStr, settings.optStr);
    gtk_container_add((GtkContainer*)vbox,entryOptStr);
    GtkWidget * labelDoc = gtk_label_new(g_strconcat(getString(1),": ","http://astyle.sourceforge.net/astyle.html",NULL));
    gtk_label_set_selectable((GtkLabel *)labelDoc, TRUE);
    gtk_container_add((GtkContainer*)vbox,labelDoc);

    g_signal_connect(dialog, "response", G_CALLBACK(on_configure_response), NULL);

    return vbox;
}


void plugin_init(GeanyData *data)
{
    settings.optStr = NULL;
    setDefaultSettings();
    loadSettings();

    // Create a new menu item and show it
    main_menu_item = gtk_menu_item_new_with_mnemonic("AStyle");
    gtk_widget_show(main_menu_item);

    // Attach the new menu item to the Tools menu
    gtk_container_add(GTK_CONTAINER(geany->main_widgets->tools_menu),
                      main_menu_item);

    g_signal_connect(main_menu_item, "activate",
                     G_CALLBACK(on_menuitem_activate), NULL);

}


void plugin_cleanup(void)
{
    gtk_widget_destroy(main_menu_item);
}
