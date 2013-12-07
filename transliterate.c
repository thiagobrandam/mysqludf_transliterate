/*
  mysqludf_transliterate - A MySQL UDF to transliterate strings
  Copyright © 2013  Thiago Brandão <thiagobrandam@gmail.com>
  The MIT License (MIT)

  Copyright (c) 2013 Thiago Brandão Damasceno

  Permission is hereby granted, free of charge, to any person obtaining a copy of
  this software and associated documentation files (the "Software"), to deal in
  the Software without restriction, including without limitation the rights to
  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
  the Software, and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
  FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <my_global.h>
#include <mysql.h>
#include <m_ctype.h>
#include <fcntl.h>
#include <unistd.h>

/*
#ifdef __WIN__
  #define DLLEXP __declspec(dllexport)
#else
  #define DLLEXP
#endif
*/

#ifdef __GNUC__
  /* From Check:  http://check.svn.sourceforge.net/viewvc/check/trunk/src/check.h.in?revision=HEAD
     License: LGPL 2.1 or later */
  #ifdef __GNUC_MINOR__
    #define GCC_VERSION_AT_LEAST(major, minor) \
      ((__GNUC__ > (major)) || \
      (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
  #else
    #define GCC_VERSION_AT_LEAST(major, minor) 0
  #endif /* __GNUC_MINOR__ */

  #if GCC_VERSION_AT_LEAST(2, 95)
    #define ATTRIBUTE_UNUSED __attribute__ ((unused))
  #else
    #define ATTRIBUTE_UNUSED
  #endif
#else
  #define ATTRIBUTE_UNUSED
#endif

#ifndef SIZE_MAX
  #define SIZE_MAX ((size_t) -1)
#endif

#ifdef HAVE_DLOPEN /* dynamic loading */

  #define LIBVERSION ("mysqludf_transliterate version 0.1")

  /* Function declarations */
  #ifdef  __cplusplus
    extern "C" {
  #endif

  my_bool str_transliterate_init(UDF_INIT *, UDF_ARGS *, char *);
  void str_transliterate_deinit(UDF_INIT *);
  char *str_transliterate(UDF_INIT *, UDF_ARGS *, char *, unsigned long *, char *, char *);

  #ifdef  __cplusplus
    }
  #endif

  /* Function definitions */
  my_bool str_transliterate_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
  {

    unsigned char *original;
    unsigned char *transliterated;

    /* Check if user provided 1 argument and is of type string */
    if (args->arg_count != 1)
    {
      snprintf(message, MYSQL_ERRMSG_SIZE, "wrong argument count: str_transliterate requires one string argument, got %d arguments", args->arg_count);
      return 1;
    }

    if (args->arg_type[0] != STRING_RESULT)
    {
      snprintf(message, MYSQL_ERRMSG_SIZE, "wrong argument type: str_transliterate requires one string argument. Expected type %d, got type %d.", STRING_RESULT, args->arg_type[0]);
      return 1;
    }

    original = args->args[0];

    transliterated = (unsigned char *)(malloc(sizeof(unsigned char) * strlen(original)));

    if (transliterated == NULL)
    {
      snprintf(message, MYSQL_ERRMSG_SIZE, "failed to allocate memory for transliteration");
      return 1;
    }

    initid->ptr = (unsigned char *) transliterated;

    initid->maybe_null = 1;

    return 0;
  }

  void str_transliterate_deinit(UDF_INIT *initid)
  {
    unsigned char *transliterated = (unsigned char *) initid->ptr;
    free(transliterated);
  }

  char *str_transliterate(UDF_INIT *initid, UDF_ARGS *args,
        char *result, unsigned long *res_length,
        char *null_value, char *error)
  {

    unsigned char *original = args->args[0];
    unsigned char *transliterated = initid->ptr;

    int original_counter = 0, transliterated_counter = 0;

    if(original == NULL)
    {
      result = NULL;
      *res_length = 0;
      *null_value = 1;
      return result;
    }

    for(; original[original_counter] != '\0'; original_counter++){
      if(original[original_counter] > 0x7F)
      {
        if(original[original_counter] == 0xC3 && original[original_counter+1] == 0xA1)
        {
          transliterated[transliterated_counter] = 'a';
          transliterated_counter += 1;
          original_counter += 1;
        }
        else
        {
          transliterated[transliterated_counter] = original[original_counter];
          transliterated_counter += 1;
        }
      }
      else
      {
        transliterated[transliterated_counter] = original[original_counter];
        transliterated_counter += 1;
      }
    }

    transliterated[transliterated_counter] = '\0';

    *res_length = transliterated_counter;
    return transliterated;
  }

#endif /* HAVE_DLOPEN */
