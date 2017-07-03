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

/*
 * Define some ioctl handlers for /dev/audio.
 */

#include <hurd/hurd_types.h>
#include <mach/kern_return.h>
#include <stdint.h>

#include <rump/rumpdefs.h>
#include <rump/rumperrno2host.h>
#include <rump/rump_syscalls.h>

#include "ioctl-audio.h"
#include "ioctl.h"

/* Define a type with the same size as the rump kernel's struct audio_info.  */
typedef int64_t audio_info_rump_buffer_t[AUDIO_INFO_BUFFER_INT64S - 1];

extern int rump_fd;


kern_return_t
sioctl_audio_getinfo (io_t reqport, audio_info_buffer_t info)
{
  int rc = rump_sys_ioctl (rump_fd,
                           _RUMP_IOR ('A', 21, audio_info_rump_buffer_t),
                           info);
  if (rc < 0)
    return rump_errno2host (rc);

  /* Set the appended compatibility 64 bits to zero.  */
  *(AUDIO_INFO_BUFFER_INT64S - 1 + (int64_t *) info) = 0;

  return KERN_SUCCESS;
}

kern_return_t
sioctl_audio_setinfo (io_t reqport, audio_info_buffer_t info)
{
  int rc = rump_sys_ioctl (rump_fd,
                           _RUMP_IOWR ('A', 22, audio_info_rump_buffer_t),
                           info);
  if (rc < 0)
    return rump_errno2host (rc);

  /* Set the appended compatibility 64 bits to zero.  */
  *(AUDIO_INFO_BUFFER_INT64S - 1 + (int64_t *) info) = 0;

  return KERN_SUCCESS;
}
