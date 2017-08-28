/*
 * Localization test program for CUPS.
 *
 * Copyright 2007-2017 by Apple Inc.
 * Copyright 1997-2006 by Easy Software Products.
 *
 * These coded instructions, statements, and computer programs are the
 * property of Apple Inc. and are protected by Federal copyright
 * law.  Distribution and use rights are outlined in the file "LICENSE.txt"
 * which should have been included with this file.  If this file is
 * missing or damaged, see the license at "http://www.cups.org/".
 *
 * This file is subject to the Apple OS-Developed Software exception.
 */

/*
 * Include necessary headers...
 */

#include "cups-private.h"
#include "ppd-private.h"
#include <sys/stat.h>
#ifdef WIN32
#  include <io.h>
#else
#  include <unistd.h>
#  include <fcntl.h>
#endif /* WIN32 */
#ifdef __APPLE__
#  include <CoreFoundation/CoreFoundation.h>
#endif /* __APPLE__ */


/*
 * 'main()' - Load the specified language and show the strings for yes and no.
 */

int					/* O - Exit status */
main(int  argc,				/* I - Number of command-line arguments */
     char *argv[])			/* I - Command-line arguments */
{
  int			i;		/* Looping var */
  int			errors = 0;	/* Number of errors */
  cups_lang_t		*language;	/* Message catalog */
  cups_lang_t		*language2;	/* Message catalog */
  struct lconv		*loc;		/* Locale data */
  const char            *msgid,         /* String identifier */
                        *msgstr;        /* Localized string */
  char			buffer[1024],	/* String buffers */
                        localedir[1024],/* Directory for language locale file */
                        realfilename[1024],/* Filename for language locale file */
                        filepath[1024];	/* Filename for language locale file */
  double		number;		/* Number */
  static const char * const tests[] =	/* Test strings */
  {
    "1",
    "-1",
    "3",
    "5.125"
  };


  if (argc == 1)
  {
    language  = cupsLangDefault();
    language2 = cupsLangDefault();
  }
  else
  {
    language  = cupsLangGet(argv[1]);
    language2 = cupsLangGet(argv[1]);

    setenv("LANG", argv[1], 1);
    setenv("SOFTWARE", "CUPS/" CUPS_SVERSION, 1);
  }

  /*
  * Setup directories for locale stuff...
  */

  if (access("locale", 0))
  {
    mkdir("locale", 0777);

    snprintf(realfilename, sizeof(realfilename), "../locale/cups_%s.po", language->language);
    snprintf(filepath, sizeof(filepath), "../../../locale/cups_%s.po", language->language);
    snprintf(localedir, sizeof(localedir), "locale/%s", language->language);
    snprintf(buffer, sizeof(buffer), "locale/%s/cups_%s.po", language->language, language->language);

    if (strchr(language->language, '_') && access(realfilename, 0))
    {
      /*
      * Country localization not available, look for generic localization...
      */

      snprintf(realfilename, sizeof(realfilename), "../locale/cups_%.2s.po", language->language);
      snprintf(filepath, sizeof(filepath), "../../../locale/cups_%.2s.po", language->language);
      snprintf(localedir, sizeof(localedir), "locale/%.2s", language->language);
      snprintf(buffer, sizeof(buffer), "locale/%.2s/cups_%.2s.po", language->language, language->language);

      if (access(realfilename, 0))
      {
          /*
          * No generic localization, so use POSIX...
          */
          snprintf(filepath, sizeof(filepath), "../../../locale/cups_C.po");
          snprintf(localedir, sizeof(localedir), "locale/C");
          snprintf(buffer, sizeof(buffer), "locale/C/cups_C.po");
        }
    }

    mkdir(localedir, 0777);
    symlink(filepath, buffer);
  }

  putenv("LOCALEDIR=locale");

  _cupsSetLocale(argv);

  if (language != language2)
  {
    errors ++;

    puts("**** ERROR: Language cache did not work! ****");
    puts("First result from cupsLangGet:");
  }

  printf("Language = \"%s\"\n", language->language);
  printf("Encoding = \"%s\"\n", _cupsEncodingName(language->encoding));

  msgid  = "No";
  msgstr = _cupsLangString(language, msgid);
  if (msgid == msgstr)
  {
    printf("%-8s = \"%s\" (FAIL)\n", msgid, msgstr);
    errors ++;
  }
  else
    printf("%-8s = \"%s\" (PASS)\n", msgid, msgstr);

  msgid  = "Yes";
  msgstr = _cupsLangString(language, msgid);
  if (msgid == msgstr)
  {
    printf("%-8s = \"%s\" (FAIL)\n", msgid, msgstr);
    errors ++;
  }
  else
    printf("%-8s = \"%s\" (PASS)\n", msgid, msgstr);

  if (language != language2)
  {
    puts("Second result from cupsLangGet:");

    printf("Language = \"%s\"\n", language2->language);
    printf("Encoding = \"%s\"\n", _cupsEncodingName(language2->encoding));
    printf("No       = \"%s\"\n", _cupsLangString(language2, "No"));
    printf("Yes      = \"%s\"\n", _cupsLangString(language2, "Yes"));
  }

  loc = localeconv();

  for (i = 0; i < (int)(sizeof(tests) / sizeof(tests[0])); i ++)
  {
    number = _cupsStrScand(tests[i], NULL, loc);

    printf("_cupsStrScand(\"%s\") number=%f\n", tests[i], number);

    _cupsStrFormatd(buffer, buffer + sizeof(buffer), number, loc);

    printf("_cupsStrFormatd(%f) buffer=\"%s\"\n", number, buffer);

    if (strcmp(buffer, tests[i]))
    {
      errors ++;
      puts("**** ERROR: Bad formatted number! ****");
    }
  }

  if (argc == 3)
  {
    ppd_file_t		*ppd;		/* PPD file */
    ppd_option_t	*option;	/* PageSize option */
    ppd_choice_t	*choice;	/* PageSize/Letter choice */

    if ((ppd = ppdOpenFile(argv[2])) == NULL)
    {
      printf("Unable to open PPD file \"%s\".\n", argv[2]);
      errors ++;
    }
    else
    {
      ppdLocalize(ppd);

      if ((option = ppdFindOption(ppd, "PageSize")) == NULL)
      {
        puts("No PageSize option.");
        errors ++;
      }
      else
      {
        printf("PageSize: %s\n", option->text);

        if ((choice = ppdFindChoice(option, "Letter")) == NULL)
        {
	  puts("No Letter PageSize choice.");
	  errors ++;
        }
        else
        {
	  printf("Letter: %s\n", choice->text);
        }
      }

      printf("media-empty: %s\n", ppdLocalizeIPPReason(ppd, "media-empty", NULL, buffer, sizeof(buffer)));

      ppdClose(ppd);
    }
  }
#ifdef __APPLE__
  else
  {
   /*
    * Test all possible language IDs for compatibility with _cupsAppleLocale...
    */

    CFIndex     j,                      /* Looping var */
                num_locales;            /* Number of locales */
    CFArrayRef  locales;                /* Locales */
    CFStringRef locale_id,              /* Current locale ID */
                language_id;            /* Current language ID */
    char        locale_str[256],        /* Locale ID C string */
                language_str[256],      /* Language ID C string */
                *bufptr;                /* Pointer to ".UTF-8" in POSIX locale */
    size_t      buflen;                 /* Length of POSIX locale */
#  if TEST_COUNTRY_CODES
    CFIndex     k,                      /* Looping var */
                num_country_codes;      /* Number of country codes */
    CFArrayRef  country_codes;          /* Country codes */
    CFStringRef country_code,           /* Current country code */
                temp_id;                /* Temporary language ID */
    char        country_str[256];       /* Country code C string */
#  endif /* TEST_COUNTRY_CODES */

    locales     = CFLocaleCopyAvailableLocaleIdentifiers();
    num_locales = CFArrayGetCount(locales);

#  if TEST_COUNTRY_CODES
    country_codes     = CFLocaleCopyISOCountryCodes();
    num_country_codes = CFArrayGetCount(country_codes);
#  endif /* TEST_COUNTRY_CODES */

    printf("%d locales are available:\n", (int)num_locales);

    for (j = 0; j < num_locales; j ++)
    {
      locale_id   = CFArrayGetValueAtIndex(locales, j);
      language_id = CFLocaleCreateCanonicalLanguageIdentifierFromString(kCFAllocatorDefault, locale_id);

      if (!locale_id || !CFStringGetCString(locale_id, locale_str, (CFIndex)sizeof(locale_str), kCFStringEncodingASCII))
      {
        printf("%d: FAIL (unable to get locale ID string)\n", (int)j + 1);
        errors ++;
        continue;
      }

      if (!language_id || !CFStringGetCString(language_id, language_str, (CFIndex)sizeof(language_str), kCFStringEncodingASCII))
      {
        printf("%d %s: FAIL (unable to get language ID string)\n", (int)j + 1, locale_str);
        errors ++;
        continue;
      }

      if (!_cupsAppleLocale(language_id, buffer, sizeof(buffer)))
      {
        printf("%d %s(%s): FAIL (unable to convert language ID string to POSIX locale)\n", (int)j + 1, locale_str, language_str);
        errors ++;
        continue;
      }

      if ((bufptr = strstr(buffer, ".UTF-8")) != NULL)
        buflen = (size_t)(bufptr - buffer);
      else
        buflen = strlen(buffer);

      if ((language = cupsLangGet(buffer)) == NULL)
      {
        printf("%d %s(%s): FAIL (unable to load POSIX locale \"%s\")\n", (int)j + 1, locale_str, language_str, buffer);
        errors ++;
        continue;
      }

      if (strncasecmp(language->language, buffer, buflen))
      {
        printf("%d %s(%s): FAIL (unable to load POSIX locale \"%s\", got \"%s\")\n", (int)j + 1, locale_str, language_str, buffer, language->language);
        errors ++;
        continue;
      }

      printf("%d %s(%s): PASS (POSIX locale is \"%s\")\n", (int)j + 1, locale_str, language_str, buffer);
    }

    CFRelease(locales);

#  if TEST_COUNTRY_CODES
    CFRelease(country_codes);
#  endif /* TEST_COUNTRY_CODES */
  }
#endif /* __APPLE__ */

  return (errors > 0);
}
