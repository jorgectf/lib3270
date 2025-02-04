/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270. Registro no INPI sob o nome G3270.
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

#include <config.h>
#include <internals.h>
#include <string.h>
#include <lib3270.h>
#include <lib3270/properties.h>
#include <lib3270/keyboard.h>

unsigned int lib3270_get_kybdlock_as_int(const H3270 *hSession) {
	return (unsigned int) lib3270_get_keyboard_lock_state(hSession);
}

const LIB3270_UINT_PROPERTY * lib3270_unsigned_property_get_by_name(const char *name) {
	size_t ix;
	const LIB3270_UINT_PROPERTY * list = lib3270_get_unsigned_properties_list();

	for(ix = 0; list[ix].name; ix++) {
		if(!strcasecmp(list[ix].name,name))
			return &list[ix];
	}

	errno = ENOENT;
	return NULL;
}

int lib3270_set_host_type_number(H3270 *hSession, unsigned int host_type) {
	FAIL_IF_ONLINE(hSession);
	hSession->host_type = host_type;
	return 0;
}

static unsigned int lib3270_get_host_type_number(const H3270 *hSession) {
	return (unsigned int) hSession->host_type;
}

LIB3270_EXPORT unsigned int lib3270_get_auto_reconnect(const H3270 *hSession) {
	return hSession->connection.retry;
}

LIB3270_EXPORT int lib3270_set_auto_reconnect(H3270 *hSession, unsigned int timer) {
	hSession->connection.retry = timer;
	return 0;
}

const LIB3270_UINT_PROPERTY * lib3270_get_unsigned_properties_list(void) {

	static const LIB3270_UINT_PROPERTY properties[] = {

		{
			.name = "color_type",									// Property name.
			.default_value = 16,									// Default value for the property.
			.group = LIB3270_ACTION_GROUP_OFFLINE,					// Property group.
			.description = N_( "The color type" ),					// Property description.
			.get = lib3270_get_color_type,							// Get value.
			.set = lib3270_set_color_type							// Set value.
		},

		{
			.name = "host_type",									// Property name.
			.default_value = (unsigned int) LIB3270_HOST_S390,
			.group = LIB3270_ACTION_GROUP_OFFLINE,					// Property group.
			.description = N_( "Host type number" ),				// Property description.
			.get = lib3270_get_host_type_number,					// Get value.
			.set = lib3270_set_host_type_number						// Set value.
		},

		{
			.name = "model_number",									// Property name.
			.group = LIB3270_ACTION_GROUP_OFFLINE,					// Property group.
			.icon = "video-display",								// Property Icon.
			.label = N_("Terminal model"),							// Property label.
			.description = N_( "The model number" ),				// Property description.
			.min = 2,												// Minimum allowable value.
			.max = 5,												// Maximum allowable value.
			.default_value = 2,										// Default value for the property.
			.get = lib3270_get_model_number,						// Get value.
			.set = lib3270_set_model_number							// Set value.
		},

		{
			.name = "width",										//  Property name.
			.description = N_( "Current screen width in columns" ),	//  Property description.
			.get = lib3270_get_width,								//  Get value.
			.set = NULL												//  Set value.
		},

		{
			.name = "height",										//  Property name.
			.description = N_( "Current screen height in rows" ),	//  Property description.
			.get = lib3270_get_height,								//  Get value.
			.set = NULL												//  Set value.
		},

		{
			.name = "max_width",									//  Property name.
			.description = N_( "Maximum screen width in columns" ),	//  Property description.
			.get = lib3270_get_max_width,							//  Get value.
			.set = NULL												//  Set value.
		},

		{
			.name = "max_height",									//  Property name.
			.description = N_( "Maximum screen height in rows" ),	//  Property description.
			.get = lib3270_get_max_height,							//  Get value.
			.set = NULL												//  Set value.
		},

		{
			.name = "length",										//  Property name.
			.description = N_( "Screen buffer length in bytes" ),	//  Property description.
			.get = lib3270_get_length,								//  Get value.
			.set = NULL												//  Set value.
		},

		{
			.name = "auto_reconnect",								// Property name.
			.default_value = 5000,									// Default value for the property.
			.description = N_( "Time for auto-reconnect" ),			// Property description.
			.get = lib3270_get_auto_reconnect,						// Get value.
			.set = lib3270_set_auto_reconnect						// Set value.
		},

		{
			.name = "unlock_delay",																				//  Property name.
			.group = LIB3270_ACTION_GROUP_OFFLINE,																// Property group.
#ifdef UNLOCK_MS
			.default_value = UNLOCK_MS,
#else
			.default_value = 350,
#endif // UNLOCK_MS
			.min = 0,
			.max = 1000000,
			.label = N_("Unlock delay"),
			.description = N_( "The delay between the host unlocking the keyboard and the actual unlock" ),		//  Property description.
			.get = lib3270_get_unlock_delay,																	//  Get value.
			.set = lib3270_set_unlock_delay																		//  Set value.
		},

		{
			.name = "kybdlock",																					//  Property name.
			.description = N_( "Keyboard lock status" ),														//  Property description.
			.get = lib3270_get_kybdlock_as_int,																	//  Get value.
			.set = NULL																							//  Set value.
		},

		{
			.name = NULL,
			.description = NULL,
			.get = NULL,
			.set = NULL
		}
	};

	return properties;
}

int lib3270_set_uint_property(H3270 *hSession, const char *name, unsigned int value, int seconds) {
	size_t ix;
	const LIB3270_UINT_PROPERTY * properties;

	if(seconds)
		lib3270_wait_for_ready(hSession, seconds);

	// Check for INT Properties
	properties = lib3270_get_unsigned_properties_list();

	for(ix = 0; properties[ix].name; ix++) {
		if(!strcasecmp(name,properties[ix].name)) {
			if(properties[ix].set)
				return properties[ix].set(hSession, value);
			else
				return errno = EPERM;
		}

	}

	return errno = ENOENT;

}

LIB3270_EXPORT unsigned int lib3270_get_task_count(const H3270 *h) {
	return h->tasks;
}

