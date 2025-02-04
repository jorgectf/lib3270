/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2008 Banco do Brasil S.A.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ANDROID
#include <stdlib.h>
#endif // !ANDROID

#include <internals.h>
#include "kybdc.h"
#include "ansic.h"
#include "togglesc.h"
#include "screen.h"
#include "screenc.h"
#include "ctlrc.h"
#include "ftc.h"
#include "kybdc.h"
#include "3270ds.h"
#include "popupsc.h"
#include <lib3270/trace.h>
#include <lib3270/log.h>
#include <lib3270/properties.h>

/*---[ Globals ]--------------------------------------------------------------------------------------------------------------*/

static H3270 *default_session = NULL;

/*---[ Statics ]--------------------------------------------------------------------------------------------------------------*/

/*---[ Implement ]------------------------------------------------------------------------------------------------------------*/

/**
 * @brief Closes a TN3270 session releasing resources.
 *
 * @param h handle of the session to close.
 *
 */
void lib3270_session_free(H3270 *h) {
	size_t f;

	if(!h)
		return;

	if(lib3270_is_connected(h)) {
		// Connected, disconnect
		lib3270_disconnect(h);
	} else if(lib3270_get_connection_state(h) == LIB3270_CONNECTING) {
		// Connecting, disconnect
		debug("%s: Stopping while connecting",__FUNCTION__);
		lib3270_disconnect(h);

	}

	// Do we have pending tasks?
	if(h->tasks) {
		lib3270_write_log(h,LIB3270_STRINGIZE_VALUE_OF(PRODUCT_NAME),"Destroying session with %u active task(s)",h->tasks);
	}

	shutdown_toggles(h);

	// Release network module
	if(h->network.module) {
		h->network.module->finalize(h);
		h->network.module = NULL;
	}

	// Release state change callbacks
	for(f=0; f<LIB3270_STATE_USER; f++)
		lib3270_linked_list_free(&h->listeners.state[f]);

	// Release toggle change listeners.
	for(f=0; f<LIB3270_TOGGLE_COUNT; f++)
		lib3270_linked_list_free(&h->listeners.toggle[f]);

	// Release action listeners.
	for(f=0; f<LIB3270_ACTION_GROUP_CUSTOM; f++)
		lib3270_linked_list_free(&h->listeners.actions[f]);

	// Release Lu names
	if(h->lu.names) {
		lib3270_free(h->lu.names);
		h->lu.names = NULL;
	}


	// Release memory
#define release_pointer(x) lib3270_free(x); x = NULL;

	// release_pointer(h->charset.display);
	release_pointer(h->paste_buffer);

	release_pointer(h->ibuf);
	h->ibuf_size = 0;

	for(f=0; f<(sizeof(h->buffer)/sizeof(h->buffer[0])); f++) {
		release_pointer(h->buffer[f]);
	}

	if(h == default_session)
		default_session = NULL;

	// Release hostname info
	release_pointer(h->host.current);
	release_pointer(h->host.url);
	release_pointer(h->host.srvc);
	release_pointer(h->host.qualified);

	release_pointer(h->charset.host);
	release_pointer(h->charset.display);

	release_pointer(h->text);
	release_pointer(h->zero_buf);

	release_pointer(h->output.base);

	release_pointer(h->sbbuf);
	release_pointer(h->tabs);

	// Release timeouts
	lib3270_linked_list_free(&h->timeouts);

	// Release inputs;
	lib3270_linked_list_free(&h->input.list);

	// Release logfile
	release_pointer(h->log.file);
	release_pointer(h->trace.file);
	lib3270_free(h);

}

static void nop_void() {
}

static int default_action(H3270 GNUC_UNUSED(*hSession), const char GNUC_UNUSED(*name)) {
	return errno = ENOENT;
}


/*
static void update_char(H3270 GNUC_UNUSED(*session), int GNUC_UNUSED(addr), unsigned char GNUC_UNUSED(chr), unsigned short GNUC_UNUSED(attr), unsigned char GNUC_UNUSED(cursor)) {
}

static void update_model(H3270 GNUC_UNUSED(*session), const char GNUC_UNUSED(*name), int GNUC_UNUSED(model), int GNUC_UNUSED(rows), int GNUC_UNUSED(cols)) {
}

static void changed(H3270 GNUC_UNUSED(*session), int GNUC_UNUSED(bstart), int GNUC_UNUSED(bend)) {
}

static void update_cursor(H3270 GNUC_UNUSED(*session), unsigned short GNUC_UNUSED(row), unsigned short GNUC_UNUSED(col), unsigned char GNUC_UNUSED(c), unsigned short GNUC_UNUSED(attr)) {
}

static void update_oia(H3270 GNUC_UNUSED(*session), LIB3270_FLAG GNUC_UNUSED(id), unsigned char GNUC_UNUSED(on)) {
}

static void update_selection(H3270 GNUC_UNUSED(*session), int GNUC_UNUSED(start), int GNUC_UNUSED(end)) {
}

static void set_cursor(H3270 GNUC_UNUSED(*session), LIB3270_POINTER GNUC_UNUSED(id)) {
}

static void update_ssl(H3270 GNUC_UNUSED(*session), LIB3270_SSL_STATE GNUC_UNUSED(state)) {
}

static void set_timer(H3270 GNUC_UNUSED(*session), unsigned char GNUC_UNUSED(on)) {
}

static void default_update_luname(H3270 GNUC_UNUSED(*session), const char GNUC_UNUSED(*name)) {
}

static void default_update_url(H3270 GNUC_UNUSED(*session), const char GNUC_UNUSED(*url)) {
}

*/

static int print(H3270 *session, LIB3270_CONTENT_OPTION GNUC_UNUSED(mode)) {
	lib3270_write_log(session, "print", "%s", "Printing is unavailable");
	lib3270_popup_dialog(session, LIB3270_NOTIFY_WARNING, _( "Can't print" ), _( "Unable to print" ), "%s", strerror(ENOTSUP));
	return errno = ENOTSUP;
}

static int save(H3270 *session, LIB3270_CONTENT_OPTION GNUC_UNUSED(mode), const char GNUC_UNUSED(*filename)) {
	lib3270_write_log(session, "save", "%s", "Saving is unavailable");
	lib3270_popup_dialog(session, LIB3270_NOTIFY_WARNING, _( "Can't save" ), _( "Unable to save" ), "%s", strerror(ENOTSUP));
	return errno = ENOTSUP;
}

static int load(H3270 *session, const char GNUC_UNUSED(*filename)) {
	lib3270_write_log(session, "load", "%s", "Loading from file is unavailable");
	lib3270_popup_dialog(session, LIB3270_NOTIFY_WARNING, _( "Can't load" ), _( "Unable to load from file" ), "%s", strerror(ENOTSUP));
	return errno = ENOTSUP;
}

static void screen_disp(H3270 *session) {
	CHECK_SESSION_HANDLE(session);
	screen_update(session,0,session->view.rows*session->view.cols);
}

void lib3270_reset_callbacks(H3270 *hSession) {
	// Default calls
	memset(&hSession->cbk,0,sizeof(hSession->cbk));

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

	hSession->cbk.update 				= nop_void;
	hSession->cbk.update_model			= nop_void;
	hSession->cbk.update_cursor			= nop_void;
	hSession->cbk.set_selection 		= nop_void;
	hSession->cbk.ctlr_done				= nop_void;
	hSession->cbk.changed				= nop_void;
	hSession->cbk.erase					= screen_disp;
	hSession->cbk.suspend				= nop_void;
	hSession->cbk.resume				= screen_disp;
	hSession->cbk.update_oia			= nop_void;
	hSession->cbk.update_selection		= nop_void;
	hSession->cbk.cursor 				= nop_void;
	hSession->cbk.update_ssl			= nop_void;
	hSession->cbk.display				= screen_disp;
	hSession->cbk.set_width				= nop_void;
	hSession->cbk.update_status			= nop_void;
	hSession->cbk.autostart				= nop_void;
	hSession->cbk.set_timer				= nop_void;
	hSession->cbk.print					= print;
	hSession->cbk.save					= save;
	hSession->cbk.load					= load;
	hSession->cbk.update_luname			= nop_void;
	hSession->cbk.update_url			= nop_void;
	hSession->cbk.action				= default_action;
	hSession->cbk.reconnect				= lib3270_reconnect;
	hSession->cbk.word_selected			= nop_void;

	#pragma GCC diagnostic pop

	lib3270_set_popup_handler(hSession, NULL);
	lib3270_set_log_handler(hSession,NULL,NULL);
	lib3270_set_trace_handler(hSession,NULL,NULL);

}

static void lib3270_session_init(H3270 *hSession, const char *model, const char *charset) {
	int f;

	memset(hSession,0,sizeof(H3270));
	lib3270_set_default_network_module(hSession);

#if defined(SSL_ENABLE_CRL_CHECK)
	hSession->ssl.download_crl = 1;
#endif // SSL_ENABLE_CRL_CHECK

	lib3270_set_host_charset(hSession,charset);
	lib3270_reset_callbacks(hSession);

	// Set the defaults.
	hSession->extended  			=  1;
	hSession->typeahead				=  1;
	hSession->oerr_lock 			=  1;
	hSession->unlock_delay			=  1;
	hSession->icrnl 				=  1;
	hSession->onlcr					=  1;
	hSession->model_num				= -1;
	hSession->connection.state		= LIB3270_NOT_CONNECTED;
	hSession->connection.timeout	= 10000;
	hSession->connection.retry		= 5000;
	hSession->oia.status			= LIB3270_MESSAGE_DISCONNECTED;
	hSession->kybdlock 				= KL_NOT_CONNECTED;
	hSession->aid 					= AID_NO;
	hSession->reply_mode 			= SF_SRM_FIELD;
	hSession->linemode				= 1;
	hSession->tn3270e_submode		= E_NONE;
	hSession->scroll_top 			= -1;
	hSession->scroll_bottom			= -1;
	hSession->wraparound_mode 		= 1;
	hSession->saved_wraparound_mode	= 1;
	hSession->once_cset 			= -1;
	hSession->state					= LIB3270_ANSI_STATE_DATA;
	hSession->host_type				= LIB3270_HOSTTYPE_DEFAULT;
	hSession->colors				= 16;
	hSession->m3279					= 1;
	hSession->pointer				= (unsigned short) LIB3270_POINTER_LOCKED;

#ifdef UNLOCK_MS
	lib3270_set_unlock_delay(hSession,UNLOCK_MS);
#else
	lib3270_set_unlock_delay(hSession,350);
#endif // UNLOCK_MS

	// CSD
	for(f=0; f<4; f++)
		hSession->csd[f] = hSession->saved_csd[f] = LIB3270_ANSI_CSD_US;

#ifdef _WIN32
	// Get defaults from registry.
	{
		lib3270_auto_cleanup(HKEY) hKey = 0;
		DWORD disp = 0;
		LSTATUS	rc = RegCreateKeyEx(
		                 HKEY_LOCAL_MACHINE,
		                 "Software\\" LIB3270_STRINGIZE_VALUE_OF(PRODUCT_NAME) "\\protocol",
		                 0,
		                 NULL,
		                 REG_OPTION_NON_VOLATILE,
		                 KEY_QUERY_VALUE|KEY_READ,
		                 NULL,
		                 &hKey,
		                 &disp);

		if(rc == ERROR_SUCCESS) {
			size_t property;
			const LIB3270_UINT_PROPERTY * uiProps = lib3270_get_unsigned_properties_list();

			for(property = 0; uiProps[property].name; property++) {
				if(!(uiProps[property].set && uiProps[property].group == LIB3270_ACTION_GROUP_OFFLINE))
					continue;

				DWORD val = (DWORD) uiProps[property].default_value;
				DWORD cbData = sizeof(DWORD);
				DWORD dwRet = RegQueryValueEx(hKey, uiProps[property].name, NULL, NULL, (LPBYTE) &val, &cbData);

				if(dwRet == ERROR_SUCCESS) {
					debug("%s=%u",uiProps[property].name,(unsigned int) val);
					uiProps[property].set(hSession,(unsigned int) val);
				} else {
					uiProps[property].set(hSession,uiProps[property].default_value);
				}

			}

		}
	}
#endif // _WIN32

	// Initialize toggles
	initialize_toggles(hSession);

	lib3270_set_model_name(hSession,model);

}

H3270 * lib3270_session_new(const char *model) {
	H3270 * hSession;

	trace("%s - configured=%s",__FUNCTION__,default_session ? "Yes" : "No");

	hSession = lib3270_malloc(sizeof(H3270));
	lib3270_session_init(hSession, model, "bracket" );

	if(!default_session)
		default_session = hSession;

	if(screen_init(hSession))
		return NULL;

	trace("%s: Initializing KYBD",__FUNCTION__);
	lib3270_register_schange(hSession,LIB3270_STATE_CONNECT,kybd_connect,NULL);
	lib3270_register_schange(hSession,LIB3270_STATE_3270_MODE,kybd_in3270,NULL);

#if defined(X3270_ANSI)
	trace("%s: Initializing ANSI",__FUNCTION__);
	lib3270_register_schange(hSession,LIB3270_STATE_3270_MODE,ansi_in3270,NULL);
#endif // X3270_ANSI


#if defined(X3270_FT)
	ft_init(hSession);
#endif

	lib3270_set_url(hSession,NULL);	// Set default URL (if available).

	trace("%s finished",__FUNCTION__);

	errno = 0;
	return hSession;
}

#if defined(DEBUG)
void check_session_handle(H3270 **hSession, const char *fname)
#else
void check_session_handle(H3270 **hSession)
#endif // DEBUG
{
	if(*hSession)
		return;

//	*hSession = lib3270_get_default_session_handle();

#if defined(DEBUG)
	lib3270_write_log(NULL, "lib3270", "%s called with empty session from %s",__FUNCTION__,fname);
#else
	lib3270_write_log(NULL, "lib3270", "%s called with empty session",__FUNCTION__);
#endif // ANDROID
}

LIB3270_INTERNAL int check_online_session(const H3270 *hSession) {

	// Is the session valid?
	if(!hSession)
		return errno = EINVAL;

	// Is it connected?
	if((int) hSession->connection.state < (int)LIB3270_CONNECTED_INITIAL)
		return errno = ENOTCONN;

	return 0;
}

LIB3270_INTERNAL int check_offline_session(const H3270 *hSession) {

	// Is the session valid?
	if(!hSession)
		return errno = EINVAL;

	// Is it connected?
	if((int) hSession->connection.state >= (int)LIB3270_CONNECTED_INITIAL)
		return errno = EISCONN;

	return 0;
}

LIB3270_EXPORT H3270 * lib3270_get_default_session_handle(void) {
	if(default_session)
		return default_session;

	return lib3270_session_new("");
}

LIB3270_EXPORT void lib3270_set_user_data(H3270 *h, void *ptr) {
	CHECK_SESSION_HANDLE(h);
	h->user_data = ptr;
}

LIB3270_EXPORT void * lib3270_get_user_data(H3270 *h) {
	CHECK_SESSION_HANDLE(h);
	return h->user_data;
}

LIB3270_EXPORT void lib3270_set_session_id(H3270 *hSession, char id) {
	CHECK_SESSION_HANDLE(hSession);
	hSession->id = id;
}

LIB3270_EXPORT char lib3270_get_session_id(H3270 *hSession) {
	CHECK_SESSION_HANDLE(hSession);
	return hSession->id;
}

struct lib3270_session_callbacks * lib3270_get_session_callbacks(H3270 *hSession, const char *revision, unsigned short sz) {

	#define REQUIRED_REVISION "20211118"

	if(revision && strcasecmp(revision,REQUIRED_REVISION) < 0) {
		errno = EINVAL;
		lib3270_write_log(hSession,PACKAGE_NAME,"Invalid revision %s when setting callback table",revision);
		return NULL;
	}

	if(sz != sizeof(struct lib3270_session_callbacks)) {

		lib3270_write_log(hSession,PACKAGE_NAME,"Invalid callback table (sz=%u expected=%u)",sz,(unsigned int) sizeof(struct lib3270_session_callbacks));
		errno = EINVAL;
		return NULL;
	}

	return &hSession->cbk;
}


