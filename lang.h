/*
 * Geany-Plugin AStyle
 * 2011
 * 
 * Language file
 */

#include <stddef.h>

typedef enum {	LANG_DE, 
				LANG_EN, 
				LANG_COUNT} Language;

// German
const char * STR_DE[] = {   "Astyle Optionen datei:",
                            "AStyle Optionen:",
                            "Dokumentation",
                            "Ungültige oder unbekannte Sprache"
                        };

// English
const char * STR_EN[] = {   "AStyle option file:",
                            "AStyle options:",
                            "Documentation",
                            "Invalid or unknown language"
                        };

#define LANG_NUM_ENTRIES sizeof(STR_DE)/sizeof(const char *)


const char * lang_getString(int lang, int id)
{
    if (id >= LANG_NUM_ENTRIES) return NULL;

    switch (lang)
        {
        case LANG_DE:
            return STR_DE[id];
        case LANG_EN:
            return STR_EN[id];
        default:
            return NULL;
        }
}
