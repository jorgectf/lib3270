/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270. Registro no INPI sob
 * o nome G3270.
 *
 * Copyright (C) <2008> <Banco do Brasil S.A.>
 *
 * Este programa é software livre. Você pode redistribuí-lo e/ou modificá-lo sob
 * os termos da GPL v.2 - Licença Pública Geral  GNU,  conforme  publicado  pela
 * Free Software Foundation.
 *
 * Este programa é distribuído na expectativa de  ser  útil,  mas  SEM  QUALQUER
 * GARANTIA; sem mesmo a garantia implícita de COMERCIALIZAÇÃO ou  de  ADEQUAÇÃO
 * A QUALQUER PROPÓSITO EM PARTICULAR. Consulte a Licença Pública Geral GNU para
 * obter mais detalhes.
 *
 * Você deve ter recebido uma cópia da Licença Pública Geral GNU junto com este
 * programa; se não, escreva para a Free Software Foundation, Inc., 51 Franklin
 * St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Este programa está nomeado como - e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

/**
 * @brief Utility functions.
 */

#define _GNU_SOURCE

#include <internals.h>
#include "utilc.h"
#include "popupsc.h"
#include <lib3270/selection.h>
#include <lib3270/log.h>
#include <sys/stat.h>
#include <fcntl.h>

#if defined(HAVE_LIBSSL)
#include <openssl/opensslv.h>
#endif // HAVE_LIBSSL

#if defined(HAVE_MALLOC_H)
#include <malloc.h>
#endif // defined

#include <stdlib.h>

#define my_isspace(c)	isspace((unsigned char)c)

///  @brief Cheesy internal version of sprintf that allocates its own memory.
char * lib3270_vsprintf(const char *fmt, va_list args) {
	char *r = NULL;

#if defined(HAVE_VASPRINTF)

	if(vasprintf(&r, fmt, args) < 0 || !r)
		lib3270_write_log(NULL, "lib3270", "Error in vasprintf");

#else

	char buf[16384];
	int nc;

	nc = vsnprintf(buf, sizeof(buf), fmt, args);
	if(nc < 0) {
		lib3270_write_log(NULL, "lib3270", "Error on vsnprintf");
	} else if (nc < sizeof(buf)) {
		r = lib3270_malloc(nc + 1);
		strcpy(r, buf);

	} else {
		r = lib3270_malloc(nc + 1);
		if(vsnprintf(r, nc, fmt, args) < 0) {
			lib3270_write_log(NULL, "lib3270", "Error on vsnprintf");
			free(r);
			return NULL;
		}

	}

#endif

	return r;
}

LIB3270_EXPORT char * lib3270_strdup_printf(const char *fmt, ...) {
	va_list args;
	char *r;

	va_start(args, fmt);
	r = lib3270_vsprintf(fmt, args);
	va_end(args);
	return r;
}

/**
 * @brief Common helper functions to insert strings, through a template, into a new  buffer.
 *
 * @param format	A printf format string with '%s's in it.
 *
 */
char * xs_buffer(const char *fmt, ...) {
	va_list args;
	char *r;

	va_start(args, fmt);
	r = lib3270_vsprintf(fmt, args);
	va_end(args);
	return r;
}

/**
 *	@brief Expands a character in the manner of "cat -v".
 */
char *
ctl_see(int c) {
	static char	buf[64];
	char	*p = buf;

	c &= 0xff;
	if ((c & 0x80) && (c <= 0xa0)) {
		*p++ = 'M';
		*p++ = '-';
		c &= 0x7f;
	}
	if (c >= ' ' && c != 0x7f) {
		*p++ = c;
	} else {
		*p++ = '^';
		if (c == 0x7f) {
			*p++ = '?';
		} else {
			*p++ = c + '@';
		}
	}
	*p = '\0';
	return buf;
}

LIB3270_EXPORT void * lib3270_free(void *p) {
	if(p)
		free(p);
	return NULL;
}

LIB3270_EXPORT void lib3270_autoptr_cleanup_char(char **ptr) {
	if(ptr && *ptr) {
		free((void *) *ptr);
		*ptr = NULL;
	}
}

LIB3270_EXPORT void lib3270_autoptr_cleanup_LIB3270_POPUP(LIB3270_POPUP **ptr) {
	if(ptr && *ptr) {
		free((void *) *ptr);
		*ptr = NULL;
	}
}

LIB3270_EXPORT void * lib3270_realloc(void *p, int len) {
	p = realloc(p, len);
	if(p == NULL)
		perror("realloc");
	return p;
}

LIB3270_EXPORT void * lib3270_calloc(int elsize, int nelem, void *ptr) {
	size_t sz = ((size_t) nelem) * ((size_t) elsize);

	if(ptr)
		ptr = realloc(ptr,sz);
	else
		ptr = malloc(sz);

	if(!ptr)
		perror("calloc");

	memset(ptr,0,sz);
	return ptr;
}

LIB3270_EXPORT void * lib3270_malloc(int len) {
	char *r = malloc(len);
	if(r)
		memset(r,0,len);

	return r;
}

LIB3270_EXPORT void * lib3270_strdup(const char *str) {
	return strdup(str);
}

LIB3270_EXPORT char * lib3270_chomp(char *str) {

	size_t len = strlen(str);

	while(len--) {

		if(isspace(str[len])) {
			str[len] = 0;
		} else {
			break;
		}
	}

	return str;

}

LIB3270_EXPORT char * lib3270_chug(char *str) {

	char *start;

	for (start = (char*) str; *start && isspace(*start); start++);

	memmove(str, start, strlen ((char *) start) + 1);

	return str;
}

LIB3270_EXPORT char * lib3270_strip(char *str) {
	return lib3270_chomp(lib3270_chug(str));
}


LIB3270_EXPORT const char * lib3270_get_version(void) {
	return PACKAGE_VERSION;
}

LIB3270_EXPORT const char * lib3270_get_revision(void) {
	return RPQ_REVISION;
}

LIB3270_EXPORT const char * lib3270_get_product_name(void) {
	return LIB3270_STRINGIZE_VALUE_OF(PRODUCT_NAME);
}

LIB3270_EXPORT char * lib3270_get_version_info(void) {
#if defined(HAVE_LIBSSL)
	return lib3270_strdup_printf(
	           "%s version %s-%s build %s (%s)",
	           PACKAGE_NAME,
	           PACKAGE_VERSION,
	           PACKAGE_RELEASE,
	           RPQ_TIMESTAMP_VALUE,
	           OPENSSL_VERSION_TEXT
	       );
#else
	return lib3270_strdup_printf("%s version %s-%s build %s",PACKAGE_NAME,PACKAGE_VERSION,PACKAGE_RELEASE,RPQ_TIMESTAMP_VALUE);
#endif // HAVE_LIBSSL
}

void lib3270_popup_an_errno(H3270 *hSession, int errn, const char *fmt, ...) {
	va_list	  args;

	va_start(args, fmt);
	lib3270_autoptr(char) summary = lib3270_vsprintf(fmt, args);
	lib3270_autoptr(char) body = lib3270_strdup_printf( _( "The system error was '%s' (rc=%d)" ),strerror(errn),errn);
	va_end(args);

	LIB3270_POPUP popup = {
		.type = LIB3270_NOTIFY_ERROR,
		.summary = summary,
		.body = body
	};

	lib3270_popup(hSession,&popup,0);

}

LIB3270_EXPORT int lib3270_print(H3270 *hSession) {
	if(check_online_session(hSession))
		return errno = ENOTCONN;

	return hSession->cbk.print(hSession, (hSession->selected ? LIB3270_CONTENT_SELECTED : LIB3270_CONTENT_ALL));
}

LIB3270_EXPORT int lib3270_print_all(H3270 *hSession) {
	if(check_online_session(hSession))
		return errno = ENOTCONN;

	return hSession->cbk.print(hSession,LIB3270_CONTENT_ALL);
}

LIB3270_EXPORT int lib3270_print_selected(H3270 *hSession) {
	if(check_online_session(hSession))
		return errno = ENOTCONN;

	if(hSession->selected)
		return hSession->cbk.print(hSession,LIB3270_CONTENT_SELECTED);

	return errno = ENODATA;
}

LIB3270_EXPORT int lib3270_print_copy(H3270 *hSession) {
	if(check_online_session(hSession))
		return errno = ENOTCONN;

	return hSession->cbk.print(hSession,LIB3270_CONTENT_COPY);
}

LIB3270_EXPORT int lib3270_load(H3270 *hSession, const char *filename) {
	if(check_online_session(hSession))
		return errno = ENOTCONN;

	return hSession->cbk.load(hSession, filename);
}

LIB3270_EXPORT int lib3270_save(H3270 *hSession, LIB3270_CONTENT_OPTION mode, const char *filename) {
	return hSession->cbk.save(hSession, mode, filename);
}

LIB3270_EXPORT int lib3270_save_all(H3270 *hSession, const char *filename) {
	if(check_online_session(hSession))
		return errno = ENOTCONN;

	return lib3270_save(hSession,LIB3270_CONTENT_ALL,filename);
}

LIB3270_EXPORT int lib3270_save_selected(H3270 *hSession, const char *filename) {
	if(hSession->selected)
		return lib3270_save(hSession,LIB3270_CONTENT_SELECTED,filename);

	return errno = ENODATA;
}

LIB3270_EXPORT int lib3270_save_copy(H3270 *hSession, const char *filename) {
	return lib3270_save(hSession,LIB3270_CONTENT_COPY,filename);
}

LIB3270_EXPORT LIB3270_POINTER lib3270_get_pointer(H3270 *hSession, int baddr) {
	static const struct _ptr {
		unsigned short	id;
		LIB3270_POINTER	value;
	} ptr[] = {
		{ 0x80,	LIB3270_POINTER_MOVE_SELECTION			},
		{ 0x82,	LIB3270_POINTER_SELECTION_TOP			},
		{ 0x86,	LIB3270_POINTER_SELECTION_TOP_RIGHT		},
		{ 0x84,	LIB3270_POINTER_SELECTION_RIGHT			},
		{ 0x81,	LIB3270_POINTER_SELECTION_LEFT			},
		{ 0x89,	LIB3270_POINTER_SELECTION_BOTTOM_LEFT	},
		{ 0x88,	LIB3270_POINTER_SELECTION_BOTTOM		},
		{ 0x8c,	LIB3270_POINTER_SELECTION_BOTTOM_RIGHT	},
		{ 0x83,	LIB3270_POINTER_SELECTION_TOP_LEFT		}
	};

	size_t f;
	unsigned short id = lib3270_get_selection_flags(hSession,baddr) & 0x8f;

	if(!lib3270_is_connected(hSession) || baddr < 0)
		return LIB3270_POINTER_LOCKED;

	for(f = 0; f < (sizeof(ptr)/sizeof(ptr[0])); f++) {
		if(ptr[f].id == id) {
			return ptr[f].value;
		}
	}

	return hSession->pointer;

}

static int xdigit_value(const char scanner) {

	if(scanner >= '0' && scanner <= '9') {
		return scanner - '0';
	}

	if(scanner >= 'A' && scanner <= 'F') {
		return 10 + (scanner - 'A');
	}

	if(scanner >= 'a' && scanner <= 'f') {
		return 10 + (scanner - 'a');
	}

	return -1;
}

static int unescape_character(const char *scanner) {

	int first_digit 	= xdigit_value(*scanner++);
	int second_digit 	= xdigit_value(*scanner++);

	if (first_digit < 0)
		return -1;

	if (second_digit < 0)
		return -1;

	return (first_digit << 4) | second_digit;

}

char * lib3270_unescape(const char *text) {
	if(!text)
		return NULL;

	size_t		  sz = strlen(text);
	char 		* outString = lib3270_malloc(sz+1);
	char		* dst = outString;
	const char	* src = text;
	char 		* ptr = strchr(src,'%');

	memset(outString,0,sz+1);

	while(ptr) {
		if(ptr[1] == '%') {
			src = ptr+2;
		} else {
			size_t sz = (ptr - src);
			memcpy(dst,src,sz);
			dst += sz;

			int chr = unescape_character(ptr+1);
			if(chr < 0) {
				*(dst++) = '?';
			} else {
				*(dst++) = (char) chr;
			}

			src += (sz+3);
		}

		ptr = strchr(src,'%');
	}

	strcpy(dst,src);

	return outString;
}

int lib3270_compare_alnum(const char *s1, const char *s2) {
	while(*s1 && *s2) {

		char c1 = toupper(*s1);
		if(!isalnum(c1)) {
			s1++;
			continue;
		}

		char c2 = toupper(*s2);
		if(!isalnum(c2)) {
			s2++;
			continue;
		}

		if(c1 != c2)
			return 1;

		s1++;
		s2++;

	}

	return 0;
}

LIB3270_EXPORT const char * lib3270_get_translation_domain() {
	return GETTEXT_PACKAGE;
}

LIB3270_INTERNAL char * lib3270_file_get_contents(H3270 GNUC_UNUSED(*hSession), const char *filename) {

	int fd = open(filename,O_RDONLY);
	if(fd < 0)
		return NULL;

	// Get file size.
	struct stat st;

	if(fstat(fd,&st) < 0) {
		int err = errno;
		close(fd);
		errno = err;
		return NULL;
	}

	char * text = lib3270_malloc(st.st_size+1);
	memset(text,0,st.st_size+1);

	char *ptr = text;
	while(st.st_size) {

		ssize_t bytes = read(fd,ptr,st.st_size);
		if(bytes < 0) {
			int err = errno;
			close(fd);
			lib3270_free(text);
			errno = err;
			return NULL;
		}

		if(!bytes)
			break;

		ptr += bytes;
		st.st_size -= bytes;

	}

	close(fd);
	return text;
}
