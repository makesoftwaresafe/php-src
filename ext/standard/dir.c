/*
   +----------------------------------------------------------------------+
   | Copyright (c) The PHP Group                                          |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | https://www.php.net/license/3_01.txt                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Thies C. Arntzen <thies@thieso.net>                          |
   +----------------------------------------------------------------------+
 */

/* {{{ includes/startup/misc */

#include "php.h"
#include "fopen_wrappers.h"
#include "file.h"
#include "php_dir.h"
#include "php_dir_int.h"
#include "php_scandir.h"
#include "basic_functions.h"
#include "dir_arginfo.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <errno.h>

#ifdef PHP_WIN32
#include "win32/readdir.h"
#endif

typedef struct {
	zend_resource *default_dir;
} php_dir_globals;

#ifdef ZTS
#define DIRG(v) ZEND_TSRMG(dir_globals_id, php_dir_globals *, v)
int dir_globals_id;
#else
#define DIRG(v) (dir_globals.v)
php_dir_globals dir_globals;
#endif

static zend_class_entry *dir_class_entry_ptr;
static zend_object_handlers dir_class_object_handlers;

#define Z_DIRECTORY_PATH_P(zv) OBJ_PROP_NUM(Z_OBJ_P(zv), 0)
#define Z_DIRECTORY_HANDLE_P(zv) OBJ_PROP_NUM(Z_OBJ_P(zv), 1)

static zend_function *dir_class_get_constructor(zend_object *object)
{
	zend_throw_error(NULL, "Cannot directly construct Directory, use dir() instead");
	return NULL;
}

static void php_set_default_dir(zend_resource *res)
{
	if (DIRG(default_dir)) {
		zend_list_delete(DIRG(default_dir));
	}

	if (res) {
		GC_ADDREF(res);
	}

	DIRG(default_dir) = res;
}

PHP_RINIT_FUNCTION(dir)
{
	DIRG(default_dir) = NULL;
	return SUCCESS;
}

PHP_MINIT_FUNCTION(dir)
{
	dirsep_str[0] = DEFAULT_SLASH;
	dirsep_str[1] = '\0';

	pathsep_str[0] = ZEND_PATHS_SEPARATOR;
	pathsep_str[1] = '\0';

	register_dir_symbols(module_number);

	dir_class_entry_ptr = register_class_Directory();
	dir_class_entry_ptr->default_object_handlers = &dir_class_object_handlers;

	memcpy(&dir_class_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	dir_class_object_handlers.get_constructor = dir_class_get_constructor;
	dir_class_object_handlers.clone_obj = NULL;
	dir_class_object_handlers.compare = zend_objects_not_comparable;

#ifdef ZTS
	ts_allocate_id(&dir_globals_id, sizeof(php_dir_globals), NULL, NULL);
#endif

	return SUCCESS;
}
/* }}} */

/* {{{ internal functions */
static void _php_do_opendir(INTERNAL_FUNCTION_PARAMETERS, int createobject)
{
	char *dirname;
	size_t dir_len;
	zval *zcontext = NULL;
	php_stream_context *context = NULL;
	php_stream *dirp;

	ZEND_PARSE_PARAMETERS_START(1, 2)
		Z_PARAM_PATH(dirname, dir_len)
		Z_PARAM_OPTIONAL
		Z_PARAM_RESOURCE_OR_NULL(zcontext)
	ZEND_PARSE_PARAMETERS_END();

	context = php_stream_context_from_zval(zcontext, 0);

	dirp = php_stream_opendir(dirname, REPORT_ERRORS, context);

	if (dirp == NULL) {
		RETURN_FALSE;
	}

	dirp->flags |= PHP_STREAM_FLAG_NO_FCLOSE;

	php_set_default_dir(dirp->res);

	if (createobject) {
		object_init_ex(return_value, dir_class_entry_ptr);
		ZVAL_STRINGL(Z_DIRECTORY_PATH_P(return_value), dirname, dir_len);
		ZVAL_RES(Z_DIRECTORY_HANDLE_P(return_value), dirp->res);
		php_stream_auto_cleanup(dirp); /* so we don't get warnings under debug */
	} else {
		php_stream_to_zval(dirp, return_value);
	}
}
/* }}} */

/* {{{ Open a directory and return a dir_handle */
PHP_FUNCTION(opendir)
{
	_php_do_opendir(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */

/* {{{ Directory class with properties, handle and class and methods read, rewind and close */
PHP_FUNCTION(dir)
{
	_php_do_opendir(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */


static php_stream* php_dir_get_directory_stream_from_user_arg(php_stream *dir_stream)
{
	if (dir_stream == NULL) {
		if (UNEXPECTED(DIRG(default_dir) == NULL)) {
			zend_type_error("No resource supplied");
			return NULL;
		}
		zend_resource *res = DIRG(default_dir);
		ZEND_ASSERT(res->type == php_file_le_stream());
		dir_stream = (php_stream*) res->ptr;
	}

	if (UNEXPECTED((dir_stream->flags & PHP_STREAM_FLAG_IS_DIR)) == 0) {
		zend_argument_type_error(1, "must be a valid Directory resource");
		return NULL;
	}
	return dir_stream;
}

static php_stream* php_dir_get_directory_stream_from_this(zval *this_z)
{
	zval *handle_zv = Z_DIRECTORY_HANDLE_P(this_z);
	if (UNEXPECTED(Z_TYPE_P(handle_zv) != IS_RESOURCE)) {
		zend_throw_error(NULL, "Internal directory stream has been altered");
		return NULL;
	}
	zend_resource *res = Z_RES_P(handle_zv);
	/* Assume the close() method was called
	 * (instead of the hacky case where a different resource would have been set via the ArrayObject "hack") */
	if (UNEXPECTED(res->type != php_file_le_stream())) {
		/* TypeError is used for BC, TODO: Use base Error in PHP 9 */
		zend_type_error("Directory::%s(): cannot use Directory resource after it has been closed", get_active_function_name());
		return NULL;
	}
	php_stream *dir_stream = (php_stream*) res->ptr;
	if (UNEXPECTED((dir_stream->flags & PHP_STREAM_FLAG_IS_DIR)) == 0) {
		zend_throw_error(NULL, "Internal directory stream has been altered");
		return NULL;
	}
	return dir_stream;
}

/* {{{ Close directory connection identified by the dir_handle */
PHP_FUNCTION(closedir)
{
	php_stream *dirp = NULL;

	ZEND_PARSE_PARAMETERS_START(0, 1)
		Z_PARAM_OPTIONAL
		PHP_Z_PARAM_STREAM_OR_NULL(dirp)
	ZEND_PARSE_PARAMETERS_END();

	dirp = php_dir_get_directory_stream_from_user_arg(dirp);
	if (UNEXPECTED(dirp == NULL)) {
		RETURN_THROWS();
	}
	zend_resource *res = dirp->res;
	zend_list_close(res);

	if (res == DIRG(default_dir)) {
		php_set_default_dir(NULL);
	}
}
/* }}} */

PHP_METHOD(Directory, close)
{
	ZEND_PARSE_PARAMETERS_NONE();

	php_stream *dirp = php_dir_get_directory_stream_from_this(ZEND_THIS);
	if (UNEXPECTED(dirp == NULL)) {
		RETURN_THROWS();
	}

	zend_resource *res = dirp->res;
	zend_list_close(res);

	if (res == DIRG(default_dir)) {
		php_set_default_dir(NULL);
	}
}

/* {{{ Rewind dir_handle back to the start */
PHP_FUNCTION(rewinddir)
{
	php_stream *dirp = NULL;

	ZEND_PARSE_PARAMETERS_START(0, 1)
		Z_PARAM_OPTIONAL
		PHP_Z_PARAM_STREAM_OR_NULL(dirp)
	ZEND_PARSE_PARAMETERS_END();

	dirp = php_dir_get_directory_stream_from_user_arg(dirp);
	if (UNEXPECTED(dirp == NULL)) {
		RETURN_THROWS();
	}

	php_stream_rewinddir(dirp);
}
/* }}} */

PHP_METHOD(Directory, rewind)
{
	ZEND_PARSE_PARAMETERS_NONE();

	php_stream *dirp = php_dir_get_directory_stream_from_this(ZEND_THIS);
	if (UNEXPECTED(dirp == NULL)) {
		RETURN_THROWS();
	}

	php_stream_rewinddir(dirp);
}

/* {{{ Read directory entry from dir_handle */
PHP_FUNCTION(readdir)
{
	php_stream *dirp = NULL;

	ZEND_PARSE_PARAMETERS_START(0, 1)
		Z_PARAM_OPTIONAL
		PHP_Z_PARAM_STREAM_OR_NULL(dirp)
	ZEND_PARSE_PARAMETERS_END();

	dirp = php_dir_get_directory_stream_from_user_arg(dirp);
	if (UNEXPECTED(dirp == NULL)) {
		RETURN_THROWS();
	}

	php_stream_dirent entry;
	if (php_stream_readdir(dirp, &entry)) {
		RETURN_STRING(entry.d_name);
	}
	RETURN_FALSE;
}
/* }}} */

PHP_METHOD(Directory, read)
{
	ZEND_PARSE_PARAMETERS_NONE();

	php_stream *dirp = php_dir_get_directory_stream_from_this(ZEND_THIS);
	if (UNEXPECTED(dirp == NULL)) {
		RETURN_THROWS();
	}

	php_stream_dirent entry;
	if (php_stream_readdir(dirp, &entry)) {
		RETURN_STRING(entry.d_name);
	}
	RETURN_FALSE;
}

#if defined(HAVE_CHROOT) && !defined(ZTS) && defined(ENABLE_CHROOT_FUNC)
/* {{{ Change root directory */
PHP_FUNCTION(chroot)
{
	char *str;
	int ret;
	size_t str_len;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_PATH(str, str_len)
	ZEND_PARSE_PARAMETERS_END();

	ret = chroot(str);
	if (ret != 0) {
		php_error_docref(NULL, E_WARNING, "%s (errno %d)", strerror(errno), errno);
		RETURN_FALSE;
	}

	php_clear_stat_cache(1, NULL, 0);

	ret = chdir("/");

	if (ret != 0) {
		php_error_docref(NULL, E_WARNING, "%s (errno %d)", strerror(errno), errno);
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */
#endif

/* {{{ Change the current directory */
PHP_FUNCTION(chdir)
{
	char *str;
	int ret;
	size_t str_len;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_PATH(str, str_len)
	ZEND_PARSE_PARAMETERS_END();

	if (php_check_open_basedir(str)) {
		RETURN_FALSE;
	}
	ret = VCWD_CHDIR(str);

	if (ret != 0) {
		php_error_docref(NULL, E_WARNING, "%s (errno %d)", strerror(errno), errno);
		RETURN_FALSE;
	}

	if (BG(CurrentStatFile) && !IS_ABSOLUTE_PATH(ZSTR_VAL(BG(CurrentStatFile)), ZSTR_LEN(BG(CurrentStatFile)))) {
		zend_string_release(BG(CurrentStatFile));
		BG(CurrentStatFile) = NULL;
	}
	if (BG(CurrentLStatFile) && !IS_ABSOLUTE_PATH(ZSTR_VAL(BG(CurrentLStatFile)), ZSTR_LEN(BG(CurrentLStatFile)))) {
		zend_string_release(BG(CurrentLStatFile));
		BG(CurrentLStatFile) = NULL;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ Gets the current directory */
PHP_FUNCTION(getcwd)
{
	char path[MAXPATHLEN];
	char *ret=NULL;

	ZEND_PARSE_PARAMETERS_NONE();

#ifdef HAVE_GETCWD
	ret = VCWD_GETCWD(path, MAXPATHLEN);
#elif defined(HAVE_GETWD)
	ret = VCWD_GETWD(path);
#endif

	if (ret) {
		RETURN_STRING(path);
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ Find pathnames matching a pattern */
PHP_FUNCTION(glob)
{
	size_t cwd_skip = 0;
#ifdef ZTS
	char cwd[MAXPATHLEN];
	char work_pattern[MAXPATHLEN];
	char *result;
#endif
	char *pattern = NULL;
	size_t pattern_len;
	zend_long flags = 0;
	php_glob_t globbuf;
	size_t n;
	int ret;
	bool basedir_limit = 0;
	zval tmp;

	ZEND_PARSE_PARAMETERS_START(1, 2)
		Z_PARAM_PATH(pattern, pattern_len)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG(flags)
	ZEND_PARSE_PARAMETERS_END();

	if (pattern_len >= MAXPATHLEN) {
		php_error_docref(NULL, E_WARNING, "Pattern exceeds the maximum allowed length of %d characters", MAXPATHLEN);
		RETURN_FALSE;
	}

	if ((PHP_GLOB_AVAILABLE_FLAGS & flags) != flags) {
		php_error_docref(NULL, E_WARNING, "At least one of the passed flags is invalid or not supported on this platform");
		RETURN_FALSE;
	}

#ifdef ZTS
	if (!IS_ABSOLUTE_PATH(pattern, pattern_len)) {
		result = VCWD_GETCWD(cwd, MAXPATHLEN);
		if (!result) {
			cwd[0] = '\0';
		}
#ifdef PHP_WIN32
		if (IS_SLASH(*pattern)) {
			cwd[2] = '\0';
		}
#endif
		cwd_skip = strlen(cwd)+1;

		snprintf(work_pattern, MAXPATHLEN, "%s%c%s", cwd, DEFAULT_SLASH, pattern);
		pattern = work_pattern;
	}
#endif


	memset(&globbuf, 0, sizeof(globbuf));
	globbuf.gl_offs = 0;
	if (0 != (ret = php_glob(pattern, flags & PHP_GLOB_FLAGMASK, NULL, &globbuf))) {
#ifdef PHP_GLOB_NOMATCH
		if (PHP_GLOB_NOMATCH == ret) {
			/* Some glob implementation simply return no data if no matches
			   were found, others return the PHP_GLOB_NOMATCH error code.
			   We don't want to treat PHP_GLOB_NOMATCH as an error condition
			   so that PHP glob() behaves the same on both types of
			   implementations and so that 'foreach (glob() as ...'
			   can be used for simple glob() calls without further error
			   checking.
			*/
			goto no_results;
		}
#endif
		RETURN_FALSE;
	}

	/* now catch the FreeBSD style of "no matches" */
	if (!globbuf.gl_pathc || !globbuf.gl_pathv) {
#ifdef PHP_GLOB_NOMATCH
no_results:
#endif
		array_init(return_value);
		return;
	}

	array_init(return_value);
	for (n = 0; n < (size_t)globbuf.gl_pathc; n++) {
		if (PG(open_basedir) && *PG(open_basedir)) {
			if (php_check_open_basedir_ex(globbuf.gl_pathv[n], 0)) {
				basedir_limit = 1;
				continue;
			}
		}
		/* we need to do this every time since PHP_GLOB_ONLYDIR does not guarantee that
		 * all directories will be filtered. GNU libc documentation states the
		 * following:
		 * If the information about the type of the file is easily available
		 * non-directories will be rejected but no extra work will be done to
		 * determine the information for each file. I.e., the caller must still be
		 * able to filter directories out.
		 */
		if (flags & PHP_GLOB_ONLYDIR) {
			zend_stat_t s = {0};

			if (0 != VCWD_STAT(globbuf.gl_pathv[n], &s)) {
				continue;
			}

			if (S_IFDIR != (s.st_mode & S_IFMT)) {
				continue;
			}
		}
		ZVAL_STRING(&tmp, globbuf.gl_pathv[n]+cwd_skip);
		zend_hash_next_index_insert_new(Z_ARRVAL_P(return_value), &tmp);
	}

	php_globfree(&globbuf);

	if (basedir_limit && !zend_hash_num_elements(Z_ARRVAL_P(return_value))) {
		zend_array_destroy(Z_ARR_P(return_value));
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ List files & directories inside the specified path */
PHP_FUNCTION(scandir)
{
	char *dirn;
	size_t dirn_len;
	zend_long flags = PHP_SCANDIR_SORT_ASCENDING;
	zend_string **namelist;
	int n, i;
	zval *zcontext = NULL;
	php_stream_context *context = NULL;

	ZEND_PARSE_PARAMETERS_START(1, 3)
		Z_PARAM_PATH(dirn, dirn_len)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG(flags)
		Z_PARAM_RESOURCE_OR_NULL(zcontext)
	ZEND_PARSE_PARAMETERS_END();

	if (dirn_len < 1) {
		zend_argument_must_not_be_empty_error(1);
		RETURN_THROWS();
	}

	if (zcontext) {
		context = php_stream_context_from_zval(zcontext, 0);
	}

	if (flags == PHP_SCANDIR_SORT_ASCENDING) {
		n = php_stream_scandir(dirn, &namelist, context, (void *) php_stream_dirent_alphasort);
	} else if (flags == PHP_SCANDIR_SORT_NONE) {
		n = php_stream_scandir(dirn, &namelist, context, NULL);
	} else {
		n = php_stream_scandir(dirn, &namelist, context, (void *) php_stream_dirent_alphasortr);
	}
	if (n < 0) {
		php_error_docref(NULL, E_WARNING, "(errno %d): %s", errno, strerror(errno));
		RETURN_FALSE;
	}

	array_init(return_value);

	for (i = 0; i < n; i++) {
		add_next_index_str(return_value, namelist[i]);
	}

	if (n) {
		efree(namelist);
	}
}
/* }}} */
