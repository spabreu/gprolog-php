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
   | Authors: Joao Conceicao <l11082@alunos.uevora.pt>                    |
   |          Salvador Abreu <spa@di.uevora.pt>                           |
   +----------------------------------------------------------------------+
 */

#include "php.h"
#include "php_ini.h"
#include "ext/standard/php_standard.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <linux/limits.h>
#include "php_gprolog.h"
// #include "config.h"

#define CHECK(x) if ((x) == -1) do { \
 static char embuf[256]; \
 sprintf (embuf, "%s: %s<br>\n", #x, strerror(errno)); \
 zend_error (E_ERROR, embuf); \
 RETURN_FALSE; } \
while (0)

#define RBUFF_SIZE 8192


/*
 * Declaracao da tabela (variavel global) que gere os diferentes handlers
 * guardando a informacao necessaria a cada um deles
 */

ZEND_DECLARE_MODULE_GLOBALS(gprolog);

/* True global resources - no need for thread safety here */
static int le_gprolog;


function_entry gprolog_functions[] = {
    ZEND_FE(pl_open,                    NULL)
    ZEND_FE(pl_close,                   NULL)

    ZEND_FE(pl_query_single,            NULL)
    ZEND_FE(pl_query_all,               NULL)

    ZEND_FE(pl_query,                   NULL)
    ZEND_FE(pl_more,                    NULL)
    ZEND_FE(pl_done,                    NULL)

    ZEND_FE(pl_show_table,              NULL)
    ZEND_FE(pl_debug,                   NULL)

    // ZEND_FE(pl_atach,                   NULL)
    // ZEND_FE(pl_detach,                  NULL)

    {NULL, NULL, NULL}
};


/* compiled module information */
zend_module_entry gprolog_module_entry = {
    STANDARD_MODULE_HEADER,
    "gprolog",
    gprolog_functions,
    ZEND_MINIT(gprolog),
    ZEND_MSHUTDOWN(gprolog),
    ZEND_RINIT(gprolog),
    ZEND_RSHUTDOWN(gprolog),
    ZEND_MINFO(gprolog),
    NO_VERSION_YET,
    STANDARD_MODULE_PROPERTIES
};


#ifdef COMPILE_DL_GPROLOG
ZEND_GET_MODULE(gprolog)
#endif


/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
PHP_INI_END()
*/


ZEND_MINIT_FUNCTION(gprolog)
{
    /* Remove comments if you have entries in php.ini
       REGISTER_INI_ENTRIES();
    */
    return SUCCESS;
}


ZEND_MSHUTDOWN_FUNCTION(gprolog)
{
    /* Remove comments if you have entries in php.ini
       UNREGISTER_INI_ENTRIES();
    */
    return SUCCESS;
}


/* Remove if there's nothing to do at request start */
ZEND_RINIT_FUNCTION(gprolog)
{
    int i;
    GPROLOGLS_FETCH();

    php_gprolog_debug = 0;

    for (i=0; i<GPROLOG_MAX_LINKS; ++i) {
	gp_link[i].pid = 0;
	gp_link[i].ready = 0;
	gp_link[i].fd_to_kid = 0;
	gp_link[i].fd_from_kid = 0;
    }

    for (i=0; i<256; ++i) {
	fb[i].buffer = 0;
	fb[i].next = 0;
	fb[i].qnext = 0;
	fb[i].size = 0;
	fb[i].count = 0;
	fb[i].at_eof = 0;
    }

    zend_printf ("<h1>PHP pid: %d</h1>\n", getpid());

    return SUCCESS;
}


/* Remove if there's nothing to do at request end */
ZEND_RSHUTDOWN_FUNCTION(gprolog)
{
    return SUCCESS;
}


ZEND_MINFO_FUNCTION(gprolog)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "gprolog support", "enabled");
    php_info_print_table_end();

    /* Remove comments if you have entries in php.ini
       DISPLAY_INI_ENTRIES();
    */
}

/*
 * implement functions that are meant to be made available to PHP
 */

/* A funcao seguinte e a responsavel por atribuir um indice a uma ligacao */
int get_index()
{
    int i=1;

    while((gp_link[i].pid != 0) && (i<GPROLOG_MAX_LINKS + 1))
	i++;

    return i;
}
/* }}} */


/* -- Buffered input ------------------------------------------------------- */

static inline int fb_available (int x)
{
    int fd;
    FB_buffer *p;
    GPROLOGLS_FETCH();

    fd = gp_link[x].fd_from_kid;
    p = fb+fd;

    return p->count + (p->qnext? 1: 0);
}

static inline int fb_getchar (int x)
{
    int fd;
    FB_buffer *p;
    GPROLOGLS_FETCH();

    fd = gp_link[x].fd_from_kid;
    p = fb+fd;

    for (;;) {
	if (p->at_eof)
	    return EOF;

	if (p->qnext) {
	    char result = p->qnext;
	    p->qnext = 0;
	    return result;
	}

	if (p->count-- > 0)
	    return *p->next++;

	if (!p->size &&
	    !(p->buffer = emalloc (1 + (p->size = FB_BUFFER_SIZE)))) {
	    zend_error(E_ERROR, "can't malloc FB buffer");
	    return EOF;
	}

	if (p->count = read (fd, p->buffer, p->size)) {
	    p->buffer[p->count] = 0;
//	    zend_printf ("read %d bytes from fd %d: \"%s\"<br>",
//			 p->count, x, p->buffer);
	    p->at_eof = 0;
	    p->next = p->buffer;
	}
	else {
	    p->at_eof = 1;
	    return EOF;
	}
    }
}

static inline void fb_ungetchar (int x, char c)
{
    int fd;
    FB_buffer *p;
    GPROLOGLS_FETCH();

    fd = gp_link[x].fd_from_kid;
    p = fb+fd;

    if (p->qnext)
	zend_error(E_ERROR, "fb_ungetchar: two calls in sequence");
    p->qnext = c;
}

static inline void fb_skipblanks (int x)
{
    int c;

    for (c = fb_getchar(x); c != EOF && isspace(c); c = fb_getchar(x))
	;
    if (c != EOF)
	fb_ungetchar(x, c);
}

static inline void fb_skiptoeol (int x)
{
    int c;

    for (c = fb_getchar(x); c != EOF && c != '\n'; c = fb_getchar(x))
	;
    if (php_gprolog_debug & 2)
	zend_printf ("fb_skiptoeol()<br>");
}

static inline char *fb_getline (int x, char *space)
{
    char *p = space;
    int c;

    fb_skipblanks (x);
    while (c != EOF && c != '\n') {
	*p++ = c;
	c = fb_getchar (x);
    }
    *p = 0;
    if (php_gprolog_debug & 2)
	zend_printf ("fb_getline -> <tt>\"%s\"</tt><br>", space);
    return space;
}

static inline char *fb_getword (int x, char *space)
{
    register char *p = space;
    int c;

    fb_skipblanks (x);
    c = fb_getchar (x);
    while (c != EOF && !isspace(c)) {
	*p++ = c;
	c = fb_getchar (x);
    }
    *p = 0;
    if (c != EOF)
	fb_ungetchar (x, c);
    if (php_gprolog_debug & 2)
	zend_printf ("fb_getword -> <tt>\"%s\"</tt><br>", space);
    return space;
}

/* ------------------------------------------------------------------------- */

/* {{{ proto int wait_for_prolog_ready (INTERNAL_FUNCTION_PARAMETERS, int)
 *
 * This function consumes all input from the child process (both HTML and
 * commands) and waits until it's ready to send something (a command) to the
 * child process.
 */

int wait_for_prolog_ready (INTERNAL_FUNCTION_PARAMETERS, int index)
{
    char buffer[FB_BUFFER_SIZE], *p = buffer;
    int  nbytes = 0, c;
    GPROLOGLS_FETCH();

    if (php_gprolog_debug & 1)
	zend_printf("wait_for_prolog_ready(%d)<br>", index);

    if (gp_link[index].ready) return 1;

    while ((c = fb_getchar(index)) != EOF) {
	if (c == GPROLOG_COMMAND_CHAR) { /* 1 */
	    if (php_gprolog_debug & 1)
		zend_printf("wait_for_prolog_ready: ready to parse<br>\n");

	    if (nbytes > 0) {
		zend_write (buffer, nbytes);
		p = buffer;
		nbytes = 0;
	    }

	    return parse_prolog(INTERNAL_FUNCTION_PARAM_PASSTHRU, index);
	}
	else {
	    *p++ = c;
	    if (++nbytes >= FB_BUFFER_SIZE) {
		zend_write (buffer, nbytes);
		p = buffer;
		nbytes = 0;
	    }
	}
    }
}
/* }}} */


/* ------------------------------------------------------------------------- */

/* {{{ proto int parse_result(INTERNAL_FUNCTION_PARAMETERS, int index)
 *
 * This function reads all input from the Prolog process, parsing and
 * executing it.
 *
 * It's called exclusively from within wait_for_prolog_ready(). Return values:
 *
 *  0: means keep looking for more input
 *
 *  other values: means return from the calling function as well.  PHP result
 *  codes must be set before returning, in this case.  the return value will
 *  be shifted right 1 bit, eg. return 1 for wait_for_prolog_ready() to return
 *  0 and 2 for wait_for_prolog_ready() to return 1.
 */

int parse_prolog(INTERNAL_FUNCTION_PARAMETERS, int index)
{
    zval *zvalue;
    char status[128], varname[128], type[128], value[128];
    int i, j, nbytes;
    GPROLOGLS_FETCH();

    if (php_gprolog_debug & 8)
	zend_printf ("parse_prolog(%d) called<br>\n", index);

    for (;;) {
	fb_getword (index, status);
	if (!strcmp (status, "var")) {
	    fb_getword (index, varname);
	    fb_getword (index, type);
	    if (php_gprolog_debug & 8)
		zend_printf ("<b>about to assign %s (type %s): ", varname, type);
	    if (!strcmp (type, "unbound")) {
		fb_skiptoeol (index);
		MAKE_STD_ZVAL (zvalue);
		ZVAL_STRING (zvalue, "_UNBOUND", 1);
		ZEND_SET_SYMBOL (EG(active_symbol_table), varname, zvalue);
		if (php_gprolog_debug & 8)
		    zend_printf ("(unbound)</b><br>\n");
	    }
	    else {
		zval *zvalue;

		MAKE_STD_ZVAL (zvalue);
		fb_getword (index, value);
		if (!strcmp (type, "int")) {
		    fb_skiptoeol (index);
		    ZVAL_LONG (zvalue, atoi (value));
		    if (php_gprolog_debug & 8)
			zend_printf ("%d</b><br>\n", atoi(value));
		}
		else if (!strcmp (type, "float")) {
		    fb_skiptoeol (index);
		    ZVAL_DOUBLE (zvalue, atof (value));
		    if (php_gprolog_debug & 8)
			zend_printf ("%g</b><br>\n", atof(value));
		}
		else if (!strcmp (type, "string")) {
		    int n, length = atoi (value);
		    char *p, *string = alloca (length+1);

		    (void) fb_getchar (index); /* skip 1 blank */
		    p=string, n=length;
		    while (n-- > 0)
			*p++ = fb_getchar (index);
		    *p = 0;

		    fb_skiptoeol (index);
		    ZVAL_STRING (zvalue, string, 1);
		    if (php_gprolog_debug & 8)
			zend_printf ("(length %d) \"%s\"</b><br>\n", length, string);
		}
		else {
		    fb_skiptoeol (index);
		    zend_error(E_WARNING, "bad type for variable %s", varname);
		}
		ZEND_SET_SYMBOL (EG(active_symbol_table), varname, zvalue);
	    }
	}
	else if (!strcmp (status, "end")) {
	    fb_skiptoeol (index);
	    RETVAL_TRUE;
	    return 1;
	}
	else if (!strcmp (status, "ok")) { /* state doesn't change */
	    char scratch[256];
	    int level = atoi (fb_getword (index, scratch));
	    fb_skiptoeol (index);
	    if (php_gprolog_debug & 8)
		zend_printf ("<b>parse_prolog: <tt>ok %d</tt></b><br>", level);
	}
	else if (!strcmp (status, "no")) { /* go back */
	    char scratch[256];
	    int level = atoi (fb_getword (index, scratch));
	    fb_skiptoeol (index);
	    if (php_gprolog_debug & 8)
		zend_printf ("<b>parse_prolog: <tt>no %d</tt></b><br>", level);
	    RETVAL_FALSE;
	    return 0;
	}
	else if (!strcmp (status, "error")) {
	    char scratch[256];
	    int level = atoi (fb_getword (index, scratch));
	    int length = atoi (fb_getword (index, scratch));
	    char *msg = alloca (length);
	    char *p = msg;

	    while (length-- > 0)
		*p++ = fb_getchar (index);
	    *p = 0;
	    fb_skiptoeol (index);

	    zend_error (E_WARNING, "<b>prolog(%d): %s</b><br>", level, msg);

	    RETVAL_FALSE;
	    return 0;
	}
	else {
	    zend_error (E_WARNING, "<b>gprolog: illegal input: \"%s\"</b><br>", status);
	    fb_skiptoeol (index);
	    RETVAL_FALSE;
	    return 0;
	}
    }
}
/* }}} */



/* {{{ proto int send_to_prolog(INTERNAL_FUNCTION_PARAMETERS, int, char *)
 *
 * This function sends its argument as a command to the Prolog process.
 */

int send_to_prolog (INTERNAL_FUNCTION_PARAMETERS, int index, char *command)
{
    GPROLOGLS_FETCH();

    if (php_gprolog_debug & 1)
	zend_printf("send_to_prolog(%d, <tt>%s</tt>)<br>", index, command);

    CHECK (write (gp_link[index].fd_to_kid, command, strlen(command)));
    return 1;
}
/* }}} */



/* {{{ proto int pl_query_internal(INTERNAL_FUNCTION_PARAMETERS, int, char *, char *)
 *
 * This function is used by both pl_query_all and pl_query_single.
 * It returns 
 */

int pl_qx (INTERNAL_FUNCTION_PARAMETERS, int *indexp, int consume_more)
{
    zval **zquery, **zindex, **zoptions = 0;
    int index;
    char *query, *options = 0, *oprefix, *osuffix, *command, *consume;
    int length;
    GPROLOGLS_FETCH();

    switch(ZEND_NUM_ARGS()) {
    case 2:
    case 3:
	if (zend_get_parameters_ex(ZEND_NUM_ARGS(),
				   &zindex, &zquery, &zoptions) == FAILURE) {
	    zend_error(E_WARNING, "parameter failure in pl_query");
	    RETVAL_FALSE;
	    return 0;
	}

	convert_to_string_ex(zindex); index = atoi(Z_STRVAL_PP(zindex));
	convert_to_string_ex(zquery); query = Z_STRVAL_PP(zquery);
	if (zoptions) {
	    convert_to_string_ex(zoptions); options = Z_STRVAL_PP(zoptions);
	}
	break;

    default:
	WRONG_PARAM_COUNT;
	RETVAL_FALSE;
	return 0;
    }

    *indexp = index;

    length = 0;
    length += 5;		/* "query" */
    length += 2;		/* " (" */
    if (consume_more) {
	consume = "read_token(_), ";
	length += strlen(consume);
    }
    else
	consume = "";
    length += strlen(query);
    length += 1;		/* ")" */
    if (options) {
	length += 4;		/* " / [" */
	length += strlen(options);
	length += 1;		/* "]" */
	oprefix = " / [";
	osuffix = "]";
    }
    else
	options = oprefix = "";
    length += 2;		/* ".\n" */

    command = (char *) alloca (length+1);
    sprintf (command, "query (%s%s)%s%s.\n", consume, query, oprefix, options);

    return send_to_prolog (INTERNAL_FUNCTION_PARAM_PASSTHRU, index, command);
}
/* }}} */


/* {{{ proto bool pl_query(int gp_link, char *wbuff[, char *optionlist])
 *
 *  Esta funcao recebe o gp_link de uma ligacao, a query a efectuar ao gprolog
 *  e uma lista de variaveis que se pretendem consultar apos a query ter sido
 *  executada.  A funcao devolve uma referencia para o primeiro tuplo
 *  resultado da query executada.
 */
ZEND_FUNCTION (pl_query)
{
    int index;
    GPROLOGLS_FETCH();

    if (php_gprolog_debug & 1) {
	zend_printf("pl_query()<br>");
    }

    if (pl_qx (INTERNAL_FUNCTION_PARAM_PASSTHRU, &index, 1)) {
	RETURN_TRUE;
    }
    else {
	RETURN_FALSE;
    }
}
/* }}} */


/* {{{ proto bool pl_query_all(int gp_link, char *wbuff[, char *optionlist])
 *
 *  Esta funcao recebe o gp_link de uma ligacao, a query a efectuar ao gprolog
 *  e uma lista de variaveis que se pretendem consultar apos a query ter sido
 *  executada.  A funcao devolve uma referencia para o primeiro tuplo
 *  resultado da query executada.
 */
ZEND_FUNCTION (pl_query_all)
{
    pl_query (INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */


/* {{{ proto bool pl_query_single(int gp_link, char *wbuff [, char *lista_de_opcoes])
 */

ZEND_FUNCTION(pl_query_single)
{
    int index;
    GPROLOGLS_FETCH();

    if (php_gprolog_debug & 1) {
	zend_printf("pl_query_single()<br>");
    }

    if (pl_qx (INTERNAL_FUNCTION_PARAM_PASSTHRU, &index, 0) &&
	wait_for_prolog_ready (INTERNAL_FUNCTION_PARAM_PASSTHRU, index) &&
	send_to_prolog (INTERNAL_FUNCTION_PARAM_PASSTHRU, index, "done\n")) {
	RETURN_TRUE;
    }
    else {
	RETURN_FALSE;
    }
}
/* }}} */


/* {{{ proto bool pl_more(int gp_link)
 */
ZEND_FUNCTION(pl_more)
{
    zval **zindex;
    int index;
    GPROLOGLS_FETCH();

    if (php_gprolog_debug & 1) {
	zend_printf("pl_more()<br>");
    }

    if (ZEND_NUM_ARGS() != 1) {
	WRONG_PARAM_COUNT;
	RETURN_FALSE;
    }

    if (zend_get_parameters_ex(1, &zindex) == FAILURE)
	RETURN_FALSE;

    convert_to_string_ex (zindex);
    index = atoi(Z_STRVAL_PP (zindex));

    if (send_to_prolog (INTERNAL_FUNCTION_PARAM_PASSTHRU, index, "more\n")) {
	wait_for_prolog_ready (INTERNAL_FUNCTION_PARAM_PASSTHRU, index);
	return;
    }

    RETURN_FALSE;
}
/* }}} */


/* {{{ proto bool pl_done(int gp_link)
 *
 */
ZEND_FUNCTION (pl_done)
{
    int index;
    GPROLOGLS_FETCH();

    if (php_gprolog_debug & 1) {
	zend_printf("pl_done()<br>");
    }

    if (send_to_prolog (INTERNAL_FUNCTION_PARAM_PASSTHRU, index, "done\n")) {
	RETURN_TRUE;
    }
    else {
	RETURN_FALSE;
    }
}
/* }}} */


/* {{{ proto int php_gprolog_connect(INTERNAL_FUNCTION_PARAMETERS, int)
 *
 * This function starts the Prolog process.
 */

int php_gprolog_connect(INTERNAL_FUNCTION_PARAMETERS, int persistent)
{
    char *path=0, *args=0, *host=0, *port=0, *options=0, *tty=0;
    pid_t childpid;
    int i, h_index;
    GPROLOGLS_FETCH();

    h_index=0;
    switch(ZEND_NUM_ARGS()) {
    case 2: { /* usar o caminho recebido recebido em 'path' e 'args' */
	zval **yypath;
	zval **yyargs;
	if (zend_get_parameters_ex(2, &yypath, &yyargs) == FAILURE)
	    zend_error(E_WARNING, "can't fetch parameters in php_gprolog_connnect.");

	convert_to_string_ex(yypath);
	path = Z_STRVAL_PP(yypath);
	convert_to_string_ex(yyargs);
	args = Z_STRVAL_PP(yyargs);

	if (!strlen(path) || !strlen(args))
	    zend_error(E_WARNING, "no arguments in 'path' or 'args'.");
	else {
	    int fd_to_kid[2], fd_from_kid[2], fd_html[2];
	    int retval, nbytes;

	    h_index = get_index();
	    CHECK (pipe (fd_to_kid) );
	    CHECK (pipe (fd_from_kid) );

	    switch (childpid = fork()) {
	    case -1:
		zend_error(E_ERROR, "forking");
		RETURN_LONG(0);
		break;

	    case 0:
		/*
		 * child process
		 * =============
		 */
		CHECK( setpgrp() );
		CHECK( dup2(fd_to_kid[0],0) );
		CHECK( dup2(fd_from_kid[1],1) );

		/* fechar todos os outros descritores (NR_OPEN = 1024) */
		for(i=3; i<1024; i++)
		    close(i);

		// close(fd_to_kid[1]);
		// close(fd_from_kid[0]);

		CHECK(execl(path, args, NULL));
		break;

	    default:
		/*
		 * parent process
		 * ==============
		 */
		gp_link[h_index].pid = childpid;
		close(fd_to_kid[0]);
		close(fd_from_kid[1]);
		gp_link[h_index].fd_to_kid = fd_to_kid[1];
		gp_link[h_index].fd_from_kid = fd_from_kid[0];

		CHECK (fcntl (fd_to_kid[1], F_SETFL,
			      fcntl (fd_to_kid[1], F_GETFL) | O_NONBLOCK));
		CHECK (fcntl (fd_from_kid[0], F_SETFL,
			      fcntl (fd_from_kid[0], F_GETFL) | O_NONBLOCK));

		/*
		 * Ligacoes persistentes
		 */
		if (php_gprolog_debug) {
		    zend_printf("persistent = %d <BR>", persistent);
		}

	    }
	}
    }
	return h_index;
	break;

    case 4: { /* usar 'host', 'port', 'tty' e 'options' */
	zval **yyhost, **yyport, **yytty, **yyoptions;

	if (zend_get_parameters_ex(4, &yyhost, &yyport, &yytty, &yyoptions) == FAILURE)
	    zend_error(E_WARNING, "can't fetch parameters in php_gprolog_connnect.");

	convert_to_string_ex(yyhost);
	convert_to_string_ex(yyport);
	convert_to_string_ex(yyoptions);
	convert_to_string_ex(yytty);
	host = Z_STRVAL_PP(yyhost);
	port = Z_STRVAL_PP(yyport);
	options = Z_STRVAL_PP(yyhost);
	tty = Z_STRVAL_PP(yyhost);

	// fazer a ligacao ao servico de rede
	zend_error(E_ERROR, "Service not available");
    }
	break;

    default:
	WRONG_PARAM_COUNT;
	RETURN_LONG(0);
	break;
    }

}
/* }}} */


/* {{{ proto int pl_open([string path, string args] | [string host, string port, s./doittring args])

 *  Esta funcao recebe o caminho (path) para o executavel do Gprolog, um array
 *  com argumentos (args) e devolve um gp_link que serve de ligacao
*/
ZEND_FUNCTION(pl_open)
{
    int h = php_gprolog_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
    RETURN_LONG(h);
}
/* }}} */


static void close_and_kill (INTERNAL_FUNCTION_PARAMETERS, int index)
{
    int status;
    GPROLOGLS_FETCH();

    send_to_prolog (INTERNAL_FUNCTION_PARAM_PASSTHRU, index, "quit\n");

    close(gp_link[index].fd_to_kid);
    close(gp_link[index].fd_from_kid);
    //    CHECK (kill(gp_link[index].pid, SIGTERM));
    CHECK (wait(&status) );
    gp_link[index].ready = 0;
    gp_link[index].pid = 0;
}
	


/* {{{ proto bool pl_close()
 *
 *  Esta funcao recebe o índice duma ligacao e fecha-a de modo a que nao possa
 *  haver mais ligacoes ao executavel respectivo. Retorna 0 caso tenha havido
 *  problemas no processo ou 1 se o canal foi fechado sem problemas.
 */
ZEND_FUNCTION(pl_close)
{
    zval **yyindex;
    int index;
    char closebuff[7];
    GPROLOGLS_FETCH();

    switch(ZEND_NUM_ARGS()) {
    case 1:
	if (zend_get_parameters_ex(1, &yyindex)==FAILURE) {
	    RETURN_FALSE;
	}
	else {
	    int status;

	    convert_to_string_ex(yyindex);
	    index = atoi(Z_STRVAL_PP(yyindex));

	    close_and_kill (INTERNAL_FUNCTION_PARAM_PASSTHRU, index);
	    RETURN_TRUE;
	}
	break;

    default:
	WRONG_PARAMTER_COUNT();
	RETURN_FALSE;
	break;
    }
}
/* }}} */



/* {{{ proto void pl_debug()
   Toggle debugging messages
*/
ZEND_FUNCTION(pl_debug)
{
    zval **yyindex;
    GPROLOGLS_FETCH();

    if (zend_get_parameters_ex (1, &yyindex) == FAILURE)
	RETURN_FALSE;
    convert_to_string_ex (yyindex);
    php_gprolog_debug = atoi (Z_STRVAL_PP(yyindex));
}
/* }}} */


/* {{{ proto void pl_show_table()
   Esta funcao percorre a tabela gp_link que contem todas as ligacoes e respectivos dados
   e mostra esse mesmos dados de todas as ligacoes que estao activas
*/
ZEND_FUNCTION(pl_show_table)
{
    int i;
    GPROLOGLS_FETCH();

    zend_printf("Tabela de Processos <BR>");
    zend_printf("=================== <BR>");
    zend_printf("(gp_link, pid, estado, { to_kid : from_kid : html})<BR>");
    for (i=1; i<(GPROLOG_MAX_LINKS + 1); i++) {
	if (gp_link[i].pid != 0)
	    zend_printf(" %d %d %d { %d : %d : %d } <BR>", i,
			gp_link[i].pid, gp_link[i].ready,
			gp_link[i].fd_to_kid,
			gp_link[i].fd_from_kid);
    }
}
/* }}} */



/*
 * Local variables:
 * mode: font-lock
 * tab-width: 8
 * c-basic-offset: 4
 * End:
 */
