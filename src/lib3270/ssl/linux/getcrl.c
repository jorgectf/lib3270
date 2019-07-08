/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270.
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
 *
 * References:
 *
 * http://www.openssl.org/docs/ssl/
 * https://stackoverflow.com/questions/4389954/does-openssl-automatically-handle-crls-certificate-revocation-lists-now
 *
 */

#define CRL_DATA_LENGTH 2048

#include <config.h>

#if defined(HAVE_LIBSSL) && defined(SSL_ENABLE_CRL_CHECK)

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509_vfy.h>
#include <openssl/x509.h>

#ifdef HAVE_LDAP
	#define LDAP_DEPRECATED 1
	#include <ldap.h>
#endif // HAVE_LDAP

#ifdef HAVE_LIBCURL
	#include <curl/curl.h>
#endif // HAVE_LIBCURL

#include "../../private.h"
#include <trace_dsc.h>
#include <errno.h>
#include <lib3270.h>
#include <lib3270/trace.h>

/*--[ Implement ]------------------------------------------------------------------------------------*/

static inline void lib3270_autoptr_cleanup_FILE(FILE **file)
{
	if(*file)
		fclose(*file);
}

#ifdef HAVE_LDAP
static inline void lib3270_autoptr_cleanup_LDAPMessage(LDAPMessage **message)
{
	debug("%s(%p)",__FUNCTION__,*message);
	if(message)
		ldap_msgfree(*message);
	*message = NULL;
}

static inline void lib3270_autoptr_cleanup_LDAP(LDAP **ld)
{
	debug("%s(%p)",__FUNCTION__,*ld);
	if(*ld)
		ldap_unbind_ext(*ld, NULL, NULL);
	*ld = NULL;
}

static inline void lib3270_autoptr_cleanup_BerElement(BerElement **ber)
{
	debug("%s(%p)",__FUNCTION__,*ber);
	if(*ber)
		ber_free(*ber, 0);
	*ber = NULL;
}

static inline void lib3270_autoptr_cleanup_LDAPPTR(char **ptr)
{
	debug("%s(%p)",__FUNCTION__,*ptr);
	if(*ptr)
		ldap_memfree(*ptr);
	*ptr = NULL;
}

#endif // HAVE_LDAP

#ifdef HAVE_LIBCURL
static inline void lib3270_autoptr_cleanup_CURL(CURL **ptr)
{
	debug("%s(%p)",__FUNCTION__,*ptr);
	if(*ptr)
		curl_easy_cleanup(*ptr);
	*ptr = NULL;
}

typedef struct _curldata
{
	size_t		  		  length;
	H3270				* hSession;
	SSL_ERROR_MESSAGE	* message;
	char 				  errbuf[CURL_ERROR_SIZE];
	struct {
		size_t			  length;
		unsigned char	* contents;
	} data;
} CURLDATA;

static inline void lib3270_autoptr_cleanup_CURLDATA(CURLDATA **ptr)
{
	debug("%s(%p)",__FUNCTION__,*ptr);
	if(*ptr)
	{
		CURLDATA *cdata = *ptr;

		if(cdata->data.contents) {
			lib3270_free(cdata->data.contents);
			cdata->data.contents = NULL;
		}
		lib3270_free(cdata);
	}
	*ptr = NULL;
}

static inline void lib3270_autoptr_cleanup_BIO(BIO **ptr)
{
	debug("%s(%p)",__FUNCTION__,*ptr);
	if(*ptr)
		BIO_free_all(*ptr);
	*ptr = NULL;
}

static size_t internal_curl_write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
	CURLDATA * data = (CURLDATA *) userp;

	debug("%s",__FUNCTION__);

	size_t realsize = size * nmemb;

	debug("%s size=%d data->length=%d crldatalength=%d",__FUNCTION__,(int) size, (int) data->length, CRL_DATA_LENGTH);

	if((realsize + data->length) > data->data.length)
	{
		data->data.length += (CRL_DATA_LENGTH + realsize);
		data->data.contents = lib3270_realloc(data->data.contents,data->data.length);
	}

	debug("%s",__FUNCTION__);

	if(lib3270_get_toggle(data->hSession,LIB3270_TOGGLE_SSL_TRACE))
	{
		lib3270_trace_data(
			data->hSession,
			"Received",
			(const char *) contents,
			realsize
		);
	}

	debug("%s",__FUNCTION__);

	memcpy(&(data->data.contents[data->length]),contents,realsize);
	data->length += realsize;

	debug("%s",__FUNCTION__);

	return realsize;
}

static int internal_curl_trace_callback(CURL GNUC_UNUSED(*handle), curl_infotype type, char *data, size_t size, void *userp)
{
	const char * text = NULL;

	switch (type) {
	case CURLINFO_TEXT:
		lib3270_write_log(((CURLDATA *) userp)->hSession,"curl","%s",data);
		return 0;

	case CURLINFO_HEADER_OUT:
		text = "=> Send header";
		break;

	case CURLINFO_DATA_OUT:
		text = "=> Send data";
		break;

	case CURLINFO_SSL_DATA_OUT:
		text = "=> Send SSL data";
		break;

	case CURLINFO_HEADER_IN:
		text = "<= Recv header";
		break;

	case CURLINFO_DATA_IN:
		text = "<= Recv data";
		break;

	case CURLINFO_SSL_DATA_IN:
		text = "<= Recv SSL data";
		break;

	default:
		return 0;

	}

	lib3270_trace_data(
		((CURLDATA *) userp)->hSession,
		text,
		data,
		size
	);

	return 0;
}

#endif // HAVE_LIBCURL


int lib3270_get_X509_CRL(H3270 *hSession, SSL_ERROR_MESSAGE * message)
{
	const char	* consturl = lib3270_get_crl_url(hSession);

	if(!(consturl && *consturl))
	{
		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		message->text = _( "Can't open CRL File" );
		message->description = _("The URL for the CRL is undefined or empty");
		return errno = ENOENT;
	}

	trace_ssl(hSession, "crl=%s\n",consturl);

	if(strncasecmp(consturl,"file://",7) == 0)
	{
		lib3270_autoptr(FILE) hCRL = fopen(consturl+7,"r");

		if(!hCRL)
		{
			// Can't open CRL File.
			int err = errno;

			message->error = hSession->ssl.error = 0;
			message->title = _( "Security error" );
			message->text = _( "Can't open CRL File" );
			message->description = strerror(err);
			trace_ssl(hSession,"Can't open %s: %s\n",consturl,message->description);
			return err;

		}

		trace_ssl(hSession,"Loading CRL from %s\n",consturl+7);
		d2i_X509_CRL_fp(hCRL, &hSession->ssl.crl.cert);

	}
#ifdef HAVE_LDAP
	else if(strncasecmp(consturl,"ldap://",7) == 0 && strlen(consturl) > 8)
	{
		int	rc;
		lib3270_autoptr(char) url = strdup(consturl);
		char * base = strchr(url+7,'/');
		char * attrs[] = { NULL, NULL };

        if(!base)
		{
			message->error = hSession->ssl.error = 0;
			message->title = _( "Security error" );
			message->text = _( "No DN of the entry at which to start the search on the URL" );
			message->description = _( "The URL argument should be in the format ldap://[HOST]/[DN]?attribute" );
			return errno = EINVAL;
		}

		*(base++) = 0;
        attrs[0] = strchr(base,'?');

        if(!base)
		{
			message->error = hSession->ssl.error = 0;
			message->title = _( "Security error" );
			message->text = _( "No LDAP attribute on the URL" );
			message->description = _( "The URL argument should be in the format ldap://[HOST]/[DN]?attribute" );
			return errno = EINVAL;
		}

        *(attrs[0]++) = 0;

        debug("host: \"%s\"",url);
        debug("Base: \"%s\"",base);
        debug("Attr: \"%s\"",attrs[0]);

		// Do LDAP Query
		LDAP __attribute__ ((__cleanup__(lib3270_autoptr_cleanup_LDAP))) *ld = NULL;
		BerElement __attribute__ ((__cleanup__(lib3270_autoptr_cleanup_BerElement))) * ber = NULL;

		rc = ldap_initialize(&ld, url);
		if(rc != LDAP_SUCCESS)
		{
			message->error = hSession->ssl.error = 0;
			message->title = _( "Security error" );
			message->text = _( "Can't initialize LDAP" );
			message->description = ldap_err2string(rc);
			lib3270_write_log(hSession,"ssl","%s: %s",url, message->description);
			return -1;
		}

		unsigned long version = LDAP_VERSION3;
		rc = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION,(void *) &version);
		if(rc != LDAP_SUCCESS) {
			message->error = hSession->ssl.error = 0;
			message->title = _( "Security error" );
			message->text = _( "Can't set LDAP version" );
			message->description = ldap_err2string(rc);
			lib3270_write_log(hSession,"ssl","%s: %s",url, message->description);
			return -1;
		}

		rc = ldap_simple_bind_s(ld, "", "");
		if(rc != LDAP_SUCCESS)
		{
			message->error = hSession->ssl.error = 0;
			message->title = _( "Security error" );
			message->text = _( "Can't bind to LDAP server" );
			message->description = ldap_err2string(rc);
			lib3270_write_log(hSession,"ssl","%s: %s",url, message->description);
			return -1;
		}

		lib3270_autoptr(LDAPMessage) results = NULL;
		rc = ldap_search_ext_s(
					ld,						// Specifies the LDAP pointer returned by a previous call to ldap_init(), ldap_ssl_init(), or ldap_open().
					base,					// Specifies the DN of the entry at which to start the search.
					LDAP_SCOPE_BASE,		// Specifies the scope of the search.
					NULL,					// Specifies a string representation of the filter to apply in the search.
					(char **)  &attrs,		// Specifies a null-terminated array of character string attribute types to return from entries that match filter.
					0,						// Should be set to 1 to request attribute types only. Set to 0 to request both attributes types and attribute values.
					NULL,
					NULL,
					NULL,
					0,
					&results
				);

		if(rc != LDAP_SUCCESS)
		{
			message->error = hSession->ssl.error = 0;
			message->title = _( "Security error" );
			message->text = _( "Can't search LDAP server" );
			message->description = ldap_err2string(rc);
			lib3270_write_log(hSession,"ssl","%s: %s",url, message->description);
			return -1;
		}

		char __attribute__ ((__cleanup__(lib3270_autoptr_cleanup_LDAPPTR))) *attr = ldap_first_attribute(ld, results, &ber);
		if(!attr)
		{
			message->error = hSession->ssl.error = 0;
			message->title = _( "Security error" );
			message->text = _( "Can't get LDAP attribute" );
			message->description = _("Search did not produce any attributes.");
			lib3270_write_log(hSession,"ssl","%s: %s",url, message->description);
			return errno = ENOENT;
		}

		struct berval ** value = ldap_get_values_len(ld, results, attr);
		if(!value)
		{
			message->error = hSession->ssl.error = 0;
			message->title = _( "Security error" );
			message->text = _( "Can't get LDAP attribute" );
			message->description = _("Search did not produce any values.");
			lib3270_write_log(hSession,"ssl","%s: %s",url, message->description);
			return errno = ENOENT;
		}

		if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_SSL_TRACE))
		{
			lib3270_trace_data(
				hSession,
				"CRL Data received from LDAP server",
				(const char *) value[0]->bv_val,
				value[0]->bv_len
			);
		}

		// Precisa salvar uma cópia porque d2i_X509_CRL modifica o ponteiro.
		const unsigned char *crl_data = (const unsigned char *) value[0]->bv_val;

		if(!d2i_X509_CRL(&hSession->ssl.crl.cert, &crl_data, value[0]->bv_len))
		{
			message->error = hSession->ssl.error = ERR_get_error();
			message->title = _( "Security error" );
			message->text = _( "Can't decode CRL got from LDAP Search" );
			lib3270_write_log(hSession,"ssl","%s: %s",url, message->text);
			ldap_value_free_len(value);
			return -1;
		}

		ldap_value_free_len(value);

	}
#endif // HAVE_LDAP
	else
	{
#ifdef HAVE_LIBCURL

		// Use CURL to download the CRL
		lib3270_autoptr(CURLDATA)	crl_data		= lib3270_malloc(sizeof(CURLDATA));
		lib3270_autoptr(CURL)		hCurl			= curl_easy_init();

		memset(crl_data,0,sizeof(CURLDATA));
		crl_data->message		= message;
		crl_data->hSession		= hSession;
		crl_data->data.length	= CRL_DATA_LENGTH;
		crl_data->data.contents = lib3270_malloc(crl_data->data.length);

		if(hCurl)
		{
			CURLcode res;

			curl_easy_setopt(hCurl, CURLOPT_URL, consturl);
			curl_easy_setopt(hCurl, CURLOPT_FOLLOWLOCATION, 1L);

			curl_easy_setopt(hCurl, CURLOPT_ERRORBUFFER, crl_data->errbuf);

			curl_easy_setopt(hCurl, CURLOPT_WRITEFUNCTION, internal_curl_write_callback);
			curl_easy_setopt(hCurl, CURLOPT_WRITEDATA, (void *) crl_data);

			curl_easy_setopt(hCurl, CURLOPT_USERNAME, "");

			if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_SSL_TRACE))
			{
				curl_easy_setopt(hCurl, CURLOPT_VERBOSE, 1L);
				curl_easy_setopt(hCurl, CURLOPT_DEBUGFUNCTION, internal_curl_trace_callback);
				curl_easy_setopt(hCurl, CURLOPT_DEBUGDATA, (void *) crl_data);
			}

			res = curl_easy_perform(hCurl);

			if(res != CURLE_OK)
			{
				message->error = hSession->ssl.error = 0;
				message->title = _( "Security error" );

				if(crl_data->errbuf[0])
				{
					message->text = curl_easy_strerror(res);
					message->description =  crl_data->errbuf;
				}
				else
				{
					message->text = _( "Error loading CRL" );
					message->description =  curl_easy_strerror(res);
				}

				lib3270_write_log(hSession,"ssl","%s: %s",consturl, message->description);
				return -1;

			}

			char *ct = NULL;
			res = curl_easy_getinfo(hCurl, CURLINFO_CONTENT_TYPE, &ct);
			if(res != CURLE_OK)
			{
				message->error = hSession->ssl.error = 0;
				message->title = _( "Security error" );
				message->text = _( "Error loading CRL" );
				message->description =  curl_easy_strerror(res);
				lib3270_write_log(hSession,"ssl","%s: %s",consturl, message->description);
				return -1;
			}

			trace_ssl(hSession,"CRL Data has %u bytes",(unsigned int) crl_data->length);

			if(ct)
			{
				const unsigned char * data = crl_data->data.contents;


				if(strcasecmp(ct,"application/pkix-crl") == 0)
				{
					// CRL File, convert it
					if(!d2i_X509_CRL(&hSession->ssl.crl.cert, &data, crl_data->length))
					{
						message->error = hSession->ssl.error = ERR_get_error();
						message->title = _( "Security error" );
						message->text = _( "Got an invalid CRL from server" );
						lib3270_write_log(hSession,"ssl","%s: %s",consturl, message->text);
						return -1;
					}
				}
				else
				{
					message->error = hSession->ssl.error = ERR_get_error();
					message->title = _( "Security error" );
					message->text = _( "Got an invalid CRL from server" );
					lib3270_write_log(hSession,"ssl","%s: content-type unexpected: \"%s\"",consturl, ct);
					return -1;
				}
			}
			else if(strncasecmp(consturl,"ldap://",7) == 0)
			{
				// It's an LDAP query, assumes a base64 data.
				char * data = strstr((char *) crl_data->data.contents,":: ");
				if(!data)
				{
					message->error = hSession->ssl.error = ERR_get_error();
					message->title = _( "Security error" );
					message->text = _( "Got an invalid CRL from LDAP server" );
					lib3270_write_log(hSession,"ssl","%s: invalid format:\n%s\n",consturl, crl_data->data.contents);
					return -1;
				}
				data += 3;

				lib3270_autoptr(BIO) bio = BIO_new_mem_buf(data,-1);

				BIO * b64 = BIO_new(BIO_f_base64());
				bio = BIO_push(b64, bio);

				BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

				if(!d2i_X509_CRL_bio(bio, &hSession->ssl.crl.cert))
				{
					message->error = hSession->ssl.error = ERR_get_error();
					message->title = _( "Security error" );
					message->text = _( "Got an invalid CRL from server" );
					lib3270_write_log(hSession,"ssl","%s: %s",consturl, message->text);
					return -1;
				}

			}

		}

#else
		// Can't get CRL.

		message->error = hSession->ssl.error = 0;
		message->title = _( "Security error" );
		message->text = _( "Unexpected or invalid CRL URL" );
		message->description = _("The URL scheme is unknown");
		lib3270_write_log(hSession,"ssl","%s: %s",consturl, message->description);
		return errno = EINVAL;
#endif // HAVE_LIBCURL
	}

	return hSession->ssl.crl.cert == NULL ? -1 : 0;

}

#endif // HAVE_LIBSSL && SSL_ENABLE_CRL_CHECK
