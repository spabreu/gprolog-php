/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999, 2000 The PHP Group                   |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors:                                                             |
   |                                                                      |
   +----------------------------------------------------------------------+
 */

#ifndef PHP_GPROLOG_H
#define PHP_GPROLOG_H

extern zend_module_entry gprolog_module_entry;
#define phpext_gprolog_ptr &gprolog_module_entry

#ifdef PHP_WIN32
#define PHP_GPROLOG_API __declspec(dllexport)
#else
#define PHP_GPROLOG_API
#endif

#define COMPILE_DL_GPROLOG 1

#define STATE_INIT      0
#define STATE_QUERY     1
#define STATE_SINGLE    2
#define STATE_QSOL	3
#define STATE_SSOL	4

ZEND_MINIT_FUNCTION(gprolog);
ZEND_MSHUTDOWN_FUNCTION(gprolog);
ZEND_RINIT_FUNCTION(gprolog);
ZEND_RSHUTDOWN_FUNCTION(gprolog);
ZEND_MINFO_FUNCTION(gprolog);

ZEND_FUNCTION(pl_open);
ZEND_FUNCTION(pl_popen);
ZEND_FUNCTION(pl_close);
ZEND_FUNCTION(pl_pclose);
ZEND_FUNCTION(pl_query_single);
ZEND_FUNCTION(pl_query_all);
ZEND_FUNCTION(pl_query);
ZEND_FUNCTION(pl_more);
ZEND_FUNCTION(pl_done);
ZEND_FUNCTION(pl_atach);
ZEND_FUNCTION(pl_detach);
ZEND_FUNCTION(pl_show_table);
ZEND_FUNCTION(pl_debug);


typedef struct _php_gprolog_result_handle {
    int pid;
    int fd_to_kid, fd_from_kid;
    int ready;
} gprolog_result_handle;

#define GPROLOG_MAX_LINKS 16

typedef struct {
    char *buffer, *next;
    char qnext;
    int size, count;
    int at_eof;
} FB_buffer;

#define FB_BUFFER_SIZE 1

#define GPROLOG_COMMAND_CHAR 1

/*
  	Declare any global variables you may need between the BEGIN
	and END macros here:     
*/
ZEND_BEGIN_MODULE_GLOBALS(gprolog)
    FB_buffer             gpg_fb[256];
    gprolog_result_handle gpg_link[GPROLOG_MAX_LINKS];
    int                   gpg_gprolog_debug;
ZEND_END_MODULE_GLOBALS(gprolog)

#define fb      GPROLOGG(gpg_fb)
#define gp_link GPROLOGG(gpg_link)
#define php_gprolog_debug GPROLOGG(gpg_gprolog_debug)


/* In every function that needs to use variables in php_gprolog_globals,
   do call GPROLOGLS_FETCH(); after declaring other variables used by
   that function, and always refer to them as GPROLOGG(variable).
   You are encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define GPROLOGG(v) (gprolog_globals->v)
#define GPROLOGLS_FETCH() php_gprolog_globals *gprolog_globals = ts_resource(gprolog_globals_id)
#else
#define GPROLOGG(v) (gprolog_globals.v)
#define GPROLOGLS_FETCH()
#endif

#endif	/* PHP_GPROLOG_H */


/*
 * Local variables:
 * tab-width: 8
 * c-basic-offset: 4
 * End:
 */







