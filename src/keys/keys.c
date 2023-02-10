// Copyright (c) 2010-2023 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include <stdbool.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <errno.h>
#include <poll.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include <nyx/nyx_module.h>
#include <nyx/module/nyx_utils.h>
#include <nyx/module/nyx_log.h>
#include "msgid.h"

enum
{
	F1 = 0x276C, /* Function keys */
	F2 = 0x276D,
	F3 = 0x276E,
	F4 = 0x276F,
	F5 = 0x2770,
	F6 = 0x2771,
	F7 = 0x2772,
	F8 = 0x2773,
	F9 = 0x2774,
	F10 = 0x2775,
	KEY_SYM = 0xf6,
	KEY_ORANGE = 0x64
};

int keypad_event_fd;

typedef struct
{
	nyx_device_t _parent;
	nyx_event_keys_t *current_event_ptr;
} keys_device_t;

NYX_DECLARE_MODULE(NYX_DEVICE_KEYS, "Keys");

/**
 * This is modeled after the linux input event interface events.
 * See linux/input.h for the original definition.
 */
typedef struct InputEvent
{
	struct timeval time;  /**< time event was generated */
	uint16_t type;        /**< type of event, EV_ABS, EV_MSC, etc. */
	uint16_t code;        /**< event code, ABS_X, ABS_Y, etc. */
	int32_t value;        /**< event value: coordinate, intensity,etc. */
} InputEvent_t;


static nyx_event_keys_t *keys_event_create()
{
	nyx_event_keys_t *event_ptr = (nyx_event_keys_t *) calloc(
	                                  sizeof(nyx_event_keys_t), 1);

	if (NULL == event_ptr)
	{
		return event_ptr;
	}

	((nyx_event_t *) event_ptr)->type = NYX_EVENT_KEYS;

	return event_ptr;
}

nyx_error_t keys_release_event(nyx_device_t *d, nyx_event_t *e)
{
	if (NULL == d)
	{
		return NYX_ERROR_INVALID_HANDLE;
	}

	if (NULL == e)
	{
		return NYX_ERROR_INVALID_HANDLE;
	}

	nyx_event_keys_t *a = (nyx_event_keys_t *) e;

	free(a);
	return NYX_ERROR_NONE;
}


static int
init_keypad(void)
{
#ifdef KEYPAD_INPUT_DEVICE
	keypad_event_fd = open(KEYPAD_INPUT_DEVICE, O_RDWR);

	if (keypad_event_fd < 0)
	{
		nyx_error(MSGID_NYX_QMUX_KEY_EVENT_ERR, 0, "Error in opening keypad event file");
		return -1;
	}

	return 0;
#else
	return -1;
#endif
}

nyx_error_t nyx_module_open(nyx_instance_t i, nyx_device_t **d)
{
	if (NULL == d)
	{
	    nyx_error(MSGID_NYX_QMUX_KEYS_OPEN_ERR, 0,"Keys device  open error.");
	    return NYX_ERROR_INVALID_VALUE;
	}

	keys_device_t *keys_device = (keys_device_t *) calloc(sizeof(keys_device_t),
	                             1);

	if (G_UNLIKELY(!keys_device))
	{
		nyx_error(MSGID_NYX_QMUX_KEY_OUT_OF_MEM, 0, "Out of memory");
		return NYX_ERROR_OUT_OF_MEMORY;
	}

	init_keypad();

	nyx_module_register_method(i, (nyx_device_t *) keys_device,
	                           NYX_GET_EVENT_SOURCE_MODULE_METHOD, "keys_get_event_source");
	nyx_module_register_method(i, (nyx_device_t *) keys_device,
	                           NYX_GET_EVENT_MODULE_METHOD, "keys_get_event");
	nyx_module_register_method(i, (nyx_device_t *) keys_device,
	                           NYX_RELEASE_EVENT_MODULE_METHOD, "keys_release_event");

	*d = (nyx_device_t *) keys_device;

	return NYX_ERROR_NONE;

}

nyx_error_t nyx_module_close(nyx_device_t *d)
{
	keys_device_t *keys_device = (keys_device_t *) d;

	if (NULL == d)
	{
		return NYX_ERROR_INVALID_HANDLE;
	}

	if (keys_device->current_event_ptr)
	{
		keys_release_event(d, (nyx_event_t *) keys_device->current_event_ptr);
	}

	nyx_debug("Freeing keys %p", d);
	free(d);

	return NYX_ERROR_NONE;
}

nyx_error_t keys_get_event_source(nyx_device_t *d, int *f)
{
	if (NULL == d)
	{
		return NYX_ERROR_INVALID_HANDLE;
	}

	if (NULL == f)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	*f = keypad_event_fd;

	return NYX_ERROR_NONE;
}

static int lookup_key(keys_device_t *d, uint16_t keyCode, int32_t keyValue,
                      nyx_key_type_t *key_type_out_ptr)
{
	int key = 0;

	switch (keyCode)
	{
		case KEY_Q:
			key = NYX_KEYS_CUSTOM_KEY_HOME;
			*key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
			break;

		case KEY_HOMEPAGE:
		case KEY_W:
			key = NYX_KEYS_CUSTOM_KEY_HOT;
			*key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
			break;

		case KEY_BACK:
		case KEY_E:
			key = NYX_KEYS_CUSTOM_KEY_BACK;
			*key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
			break;

		case KEY_HOME:
			key = NYX_KEYS_CUSTOM_KEY_HOME;
			*key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
			break;

		case KEY_VOLUMEUP:
			key = NYX_KEYS_CUSTOM_KEY_VOL_UP;
			*key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
			break;

		case KEY_VOLUMEDOWN:
			key = NYX_KEYS_CUSTOM_KEY_VOL_DOWN;
			*key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
			break;

		case KEY_END:
			key = NYX_KEYS_CUSTOM_KEY_POWER_ON;
			*key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
			break;

		case KEY_PLAY:
			key = NYX_KEYS_CUSTOM_KEY_MEDIA_PLAY;
			*key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
			break;

		case KEY_PAUSE:
			key = NYX_KEYS_CUSTOM_KEY_MEDIA_PAUSE;
			*key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
			break;

		case KEY_STOP:
			key = NYX_KEYS_CUSTOM_KEY_MEDIA_STOP;
			*key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
			break;

		case KEY_NEXT:
			key = NYX_KEYS_CUSTOM_KEY_MEDIA_NEXT;
			*key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
			break;

		case KEY_PREVIOUS:
			key = NYX_KEYS_CUSTOM_KEY_MEDIA_PREVIOUS;
			*key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
			break;

		// add keyboard function keys
		case KEY_SEARCH:
			key = NYX_KEYS_CUSTOM_KEY_SEARCH;
			*key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
			break;

		case KEY_BRIGHTNESSDOWN:
			key = NYX_KEYS_CUSTOM_KEY_BRIGHTNESS_DOWN;
			*key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
			break;

		case KEY_BRIGHTNESSUP:
			key = NYX_KEYS_CUSTOM_KEY_BRIGHTNESS_UP;
			*key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
			break;

		case KEY_MUTE:
			key = NYX_KEYS_CUSTOM_KEY_VOL_MUTE;
			*key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
			break;

		case KEY_REWIND:
			key = NYX_KEYS_CUSTOM_KEY_MEDIA_REWIND;
			*key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
			break;

		case KEY_FASTFORWARD:
			key = NYX_KEYS_CUSTOM_KEY_MEDIA_FASTFORWARD;
			*key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
			break;

		default:
			break;
	}

	if (*key_type_out_ptr != NYX_KEY_TYPE_CUSTOM)
	{
		if (keyCode == KEY_LEFTSHIFT)
		{
			key = KEY_LEFTSHIFT;
		}
		else
		{
			key = keyCode;
		}
	}

	return key;
}

struct pollfd fds[1];

int
read_input_event(InputEvent_t *pEvents, int maxEvents)
{
	int numEvents = 0;
	int rd = 0;

	if (pEvents == NULL)
	{
		return -1;
	}

	fds[0].fd = keypad_event_fd;
	fds[0].events = POLLIN;

	int ret_val = poll(fds, 1, 0);

	if (ret_val <= 0)
	{
		return 0;
	}

	if (fds[0].revents & POLLIN)
	{
		/* keep looping if get EINTR */
		for (;;)
		{
			rd = read(fds[0].fd, pEvents, sizeof(InputEvent_t) * maxEvents);

			if (rd > 0)
			{
				numEvents += rd / sizeof(InputEvent_t);
				break;
			}
			else if (rd < 0 && errno != EINTR)
			{
				nyx_error(MSGID_NYX_QMUX_KEY_EVENT_READ_ERR, 0, "Failed to read events from keypad event file");
				return -1;
			}
		}
	}

	return numEvents;
}

#define MAX_EVENTS      64

nyx_error_t keys_get_event(nyx_device_t *d, nyx_event_t **e)
{
	static InputEvent_t raw_events[MAX_EVENTS];

	static int event_count = 0;
	static int event_iter = 0;

	keys_device_t *keys_device = (keys_device_t *) d;

	/*
	 * Event bookkeeping...
	 */
	if (!event_iter)
	{
		event_count = read_input_event(raw_events, MAX_EVENTS);
		keys_device->current_event_ptr = NULL;
	}

	if (keys_device->current_event_ptr == NULL)
	{
		/*
		 * let's allocate new event and hold it here.
		 */
		keys_device->current_event_ptr = keys_event_create();
		assert(NULL != keys_device->current_event_ptr);
	}

	for (; event_iter < event_count;)
	{
		InputEvent_t *input_event_ptr;
		input_event_ptr = &raw_events[event_iter];
		event_iter++;

		if (input_event_ptr->type == EV_KEY)
		{
			keys_device->current_event_ptr->key_type = NYX_KEY_TYPE_STANDARD;
			keys_device->current_event_ptr->key = lookup_key(keys_device,
			                                      input_event_ptr->code, input_event_ptr->value,
			                                      &keys_device->current_event_ptr->key_type);
		}
		else
		{
			continue;
		}

		keys_device->current_event_ptr->key_is_press
		    = (input_event_ptr->value) ? true : false;
		keys_device->current_event_ptr->key_is_auto_repeat
		    = (input_event_ptr->value > 1) ? true : false;

		*e = (nyx_event_t *) keys_device->current_event_ptr;
		keys_device->current_event_ptr = NULL;

		/*
		 * Generated event, bail out and let the caller know.
		 */
		if (NULL != *e)
		{
			break;
		}
	}

	if (event_iter >= event_count)
	{
		event_iter = 0;
	}

	return NYX_ERROR_NONE;
}

