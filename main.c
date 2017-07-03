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
 * This file defines the initialization code of a Hurd translator for the
 * Solaris audio device /dev/audio, converting operations to the NetBSD audio
 * device /dev/audio in a rump kernel server.
 */

#define _GNU_SOURCE

#include <argp.h>
#include <error.h>
#include <nullauth.h>
#include <stdlib.h>
#include <unistd.h>

#include <hurd.h>
#include <hurd/fsys.h>
#include <hurd/ports.h>
#include <hurd/trivfs.h>

#include <rump/rumpclient.h>
#include <rump/rumpdefs.h>
#include <rump/rumperrno2host.h>
#include <rump/rump_syscalls.h>

#include "ioctl.h"

int rump_fd;


const char *argp_program_version = "audio 0.1";

static const struct argp_option options[] =
{
  {"url", 'u', "URL", 0, "Specify where to connect to the rump server"},
  {0}
};

static error_t
parse_opt (int opt, char *arg, struct argp_state *state)
{
  int rc;

  switch (opt)
    {
    case 'u':
      rc = setenv ("RUMP_SERVER", arg, 1);
      if (rc < 0)
        return errno;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }

  return 0;
}

static const struct argp argp =
{options, parse_opt, 0, "Map to the NetBSD audio device."};


static int
audio_demuxer (mach_msg_header_t *inp, mach_msg_header_t *outp)
{
  return trivfs_demuxer (inp, outp) || sioctl_server (inp, outp);
}

int
main (int argc, char *argv[])
{
  error_t err;
  mach_port_t bootstrap;
  struct trivfs_control *fsys;
  char *url;
  int rc;

  argp_parse (&argp, argc, argv, 0, 0, 0);

  /* Test whether the socket path exists before client setup.  */
  url = getenv ("RUMP_SERVER");
  if (url == NULL)
    error (1, 0, "The rump server URL must be specified");
  if (strncmp (url, "unix://", 7) == 0 && access (url + 7, F_OK) != 0)
    error (1, 0, "The rump server socket path does not exist");

  /* Get the bootstrap port.  */
  task_get_bootstrap_port (mach_task_self (), &bootstrap);
  if (bootstrap == MACH_PORT_NULL)
    error (1, 0, "Must be started as a translator");

  /* Reply to our parent.  */
  err = trivfs_startup (bootstrap, 0, 0, 0, 0, 0, &fsys);
  mach_port_deallocate (mach_task_self (), bootstrap);
  if (err)
    error (2, err, "Contacting parent");

  /* Connect to the rump server.  */
  rc = rumpclient_init ();
  if (rc < 0)
    error (3, rump_errno2host (errno), "Connecting to the rump kernel server");
  rumpclient_setconnretry (RUMPCLIENT_RETRYCONN_DIE);

  /* Get a rump file descriptor for the audio device.  */
  rump_fd = rump_sys_open ("/dev/audio", RUMP_O_RDWR | RUMP_O_CLOEXEC);
  if (rump_fd < 0)
    error (4, rump_errno2host (errno), "Opening the rump /dev/audio");

  /* Drop Unix privileges.  */
  err = setnullauth ();
  if (err)
    error (5, err, "Dropping privileges");

  /* Launch.  */
  ports_manage_port_operations_multithread (fsys->pi.bucket, audio_demuxer,
                                            2 * 60 * 1000, 0, 0);

  /* Close the rump device.  */
  rc = rump_sys_close (rump_fd);
  if (rc < 0)
    error (6, rump_errno2host (errno), "Closing the rump /dev/audio");

  return 0;
}
