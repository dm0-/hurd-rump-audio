/*
 * Copyright (C) 2017 David Michael <fedora.dm0@gmail.com>
 *
 * This file is part of the Hurd rump audio translator.
 *
 * The Hurd rump audio translator is free software: you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * The Hurd rump audio translator is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * the Hurd rump audio translator.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _IOCTL_H_
#define _IOCTL_H_

#include "buffer.h"

/* This data type is passed to Hurd ioctls in place of struct audio_info.  */
typedef int64_t audio_info_buffer_t[AUDIO_INFO_BUFFER_INT64S];

extern int sioctl_server (mach_msg_header_t *, mach_msg_header_t *);

#endif
