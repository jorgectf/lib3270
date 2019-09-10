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
 #include <lib3270-internals.h>
 #include <string.h>
 #include <lib3270.h>
 #include <lib3270/properties.h>
 #include <lib3270/keyboard.h>

 unsigned int lib3270_get_kybdlock_as_int(const H3270 *hSession)
 {
	return (unsigned int) lib3270_get_keyboard_lock_state(hSession);
 }

 const LIB3270_UINT_PROPERTY * lib3270_get_unsigned_properties_list(void)
 {

	static const LIB3270_UINT_PROPERTY properties[] = {

		{
			"cursor_address",							//  Property name.
			N_( "Cursor address" ),						//  Property description.
			lib3270_get_cursor_address,					//  Get value.
			lib3270_set_cursor_address					//  Set value.
		},

		{
			"width",									//  Property name.
			N_( "Current screen width in columns" ),	//  Property description.
			lib3270_get_width,							//  Get value.
			NULL										//  Set value.
		},

		{
			"height",									//  Property name.
			N_( "Current screen height in rows" ),		//  Property description.
			lib3270_get_height,							//  Get value.
			NULL										//  Set value.
		},

		{
			"max_width",								//  Property name.
			N_( "Maximum screen width in columns" ),	//  Property description.
			lib3270_get_max_width,						//  Get value.
			NULL										//  Set value.
		},

		{
			"max_height",								//  Property name.
			N_( "Maximum screen height in rows" ),		//  Property description.
			lib3270_get_max_height,						//  Get value.
			NULL										//  Set value.
		},

		{
			"length",									//  Property name.
			N_( "Screen buffer length in bytes" ),		//  Property description.
			lib3270_get_length,							//  Get value.
			NULL										//  Set value.
		},

		{
			"unlock_delay",																			//  Property name.
			N_( "The delay between the host unlocking the keyboard and the actual unlock" ),		//  Property description.
			lib3270_get_unlock_delay,																//  Get value.
			lib3270_set_unlock_delay																//  Set value.
		},

		{
			"kybdlock",																				//  Property name.
			N_( "Keyboard lock status" ),															//  Property description.
			lib3270_get_kybdlock_as_int,															//  Get value.
			NULL																					//  Set value.
		},

		/*
		{
			"",						//  Property name.
			"",				//  Property description.
			NULL,					//  Get value.
			NULL					//  Set value.
		},
		*/

		{
			NULL,
			NULL,
			NULL,
			NULL
		}
	};

	return properties;
 }


