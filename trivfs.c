/*
 * Copyright (C) 1995,96,97,98,99,2001,02,03 Free Software Foundation, Inc.
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
 * This file defines the hooks into libtrivfs to support simple file operations
 * on the translator.  It was mostly copied from Hurd's null translator.  The
 * important functions are trivfs_S_io_read and trivfs_S_io_write.
 */

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>

#include <hurd/trivfs.h>

#include <rump/rumperrno2host.h>
#include <rump/rump_syscalls.h>

extern int rump_fd;


/* Trivfs hooks  */

int trivfs_fstype = FSTYPE_DEV;
int trivfs_fsid = 0;

int trivfs_support_read = 1;
int trivfs_support_write = 1;
int trivfs_support_exec = 0;

int trivfs_allow_open = O_READ | O_WRITE;

void
trivfs_modify_stat (struct trivfs_protid *cred, io_statbuf_t *st)
{
  st->st_blksize = vm_page_size * 256;  /* Make transfers LARRRRRGE.  */

  st->st_blocks = 0;
  st->st_rdev = makedev (42, 128);  /* NetBSD = 42:128, Illumos = 26:3 */
  st->st_size = 0;

  st->st_mode &= ~S_IFMT;
  st->st_mode |= S_IFCHR;
}

error_t
trivfs_goaway (struct trivfs_control *fsys, int flags)
{
  rump_sys_close (rump_fd);
  exit (0);
}


/* Return objects mapping the data underlying this memory object.  If
   the object can be read then memobjrd will be provided; if the
   object can be written then memobjwr will be provided.  For objects
   where read data and write data are the same, these objects will be
   equal, otherwise they will be disjoint.  Servers are permitted to
   implement io_map but not io_map_cntl.  Some objects do not provide
   mapping; they will set none of the ports and return an error.  Such
   objects can still be accessed by io_read and io_write.  */
kern_return_t
trivfs_S_io_map (struct trivfs_protid *cred,
                 mach_port_t reply, mach_msg_type_name_t replytype,
                 memory_object_t *rdobj, mach_msg_type_name_t *rdtype,
                 memory_object_t *wrobj, mach_msg_type_name_t *wrtype)
{
  return EOPNOTSUPP;  /* XXX should work!  */
}


/* Read data from an IO object.  If offset is -1, read from the object
   maintained file pointer.  If the object is not seekable, offset is
   ignored.  The amount desired to be read is in AMT.  */
kern_return_t
trivfs_S_io_read (struct trivfs_protid *cred,
                  mach_port_t reply, mach_msg_type_name_t replytype,
                  char **data, mach_msg_type_number_t *datalen,
                  loff_t offs, mach_msg_type_number_t amt)
{
  ssize_t length;

  if (!cred)
    return EOPNOTSUPP;
  else if (!(cred->po->openmodes & O_READ))
    return EBADF;

  for (*datalen = 0; *datalen < amt; *datalen += length)
    {
      length = rump_sys_read (rump_fd, *data + *datalen, amt - *datalen);
      if (length < 0)
        return rump_errno2host (errno);
    }

  return KERN_SUCCESS;
}


/* Tell how much data can be read from the object without blocking for
   a "long time" (this should be the same meaning of "long time" used
   by the nonblocking flag).  */
kern_return_t
trivfs_S_io_readable (struct trivfs_protid *cred,
                      mach_port_t reply, mach_msg_type_name_t replytype,
                      mach_msg_type_number_t *amount)
{
  if (!cred)
    return EOPNOTSUPP;
  else if (!(cred->po->openmodes & O_READ))
    return EINVAL;

  *amount = 0;
  return KERN_SUCCESS;
}


/* Change current read/write offset */
kern_return_t
trivfs_S_io_seek (struct trivfs_protid *cred,
                  mach_port_t reply, mach_msg_type_name_t replytype,
                  off_t offset, int whence, off_t *new_offset)
{
  if (!cred)
    return EOPNOTSUPP;

  *new_offset = 0;
  return KERN_SUCCESS;
}


/* SELECT_TYPE is the bitwise OR of SELECT_READ, SELECT_WRITE, and SELECT_URG.
   Block until one of the indicated types of i/o can be done "quickly", and
   return the types that are then available.  ID_TAG is returned as passed; it
   is just for the convenience of the user in matching up reply messages with
   specific requests sent.  */
kern_return_t
trivfs_S_io_select (struct trivfs_protid *cred,
                    mach_port_t reply, mach_msg_type_name_t replytype,
                    int *type)
{
  if (!cred)
    return EOPNOTSUPP;

  *type &= ~SELECT_URG;
  return KERN_SUCCESS;
}

kern_return_t
trivfs_S_io_select_timeout (struct trivfs_protid *cred,
                            mach_port_t reply, mach_msg_type_name_t replytype,
                            struct timespec ts, int *type)
{
  return trivfs_S_io_select (cred, reply, replytype, type);
}


/* Write data to an IO object.  If offset is -1, write at the object
   maintained file pointer.  If the object is not seekable, offset is
   ignored.  The amount successfully written is returned in amount.  A
   given user should not have more than one outstanding io_write on an
   object at a time; servers implement congestion control by delaying
   responses to io_write.  Servers may drop data (returning ENOBUFS)
   if they recevie more than one write when not prepared for it.  */
kern_return_t
trivfs_S_io_write (struct trivfs_protid *cred,
                   mach_port_t reply, mach_msg_type_name_t replytype,
                   char *data, mach_msg_type_number_t datalen,
                   loff_t offs, mach_msg_type_number_t *amt)
{
  ssize_t length;

  if (!cred)
    return EOPNOTSUPP;
  else if (!(cred->po->openmodes & O_WRITE))
    return EBADF;

  for (*amt = 0; *amt < datalen; *amt += length)
    {
      length = rump_sys_write (rump_fd, data + *amt, datalen - *amt);
      if (length < 0)
        return rump_errno2host (errno);
    }

  return KERN_SUCCESS;
}


/* Truncate file.  */
kern_return_t
trivfs_S_file_set_size (struct trivfs_protid *cred,
                        mach_port_t reply, mach_msg_type_name_t replytype,
                        loff_t size)
{
  if (size < 0)
    return EINVAL;

  return KERN_SUCCESS;
}


/* These four routines modify the O_APPEND, O_ASYNC, O_FSYNC, and
   O_NONBLOCK bits for the IO object. In addition, io_get_openmodes
   will tell you which of O_READ, O_WRITE, and O_EXEC the object can
   be used for.  The O_ASYNC bit affects icky async I/O; good async
   I/O is done through io_async which is orthogonal to these calls. */

kern_return_t
trivfs_S_io_get_openmodes (struct trivfs_protid *cred,
                           mach_port_t reply, mach_msg_type_name_t replytype,
                           int *bits)
{
  if (!cred)
    return EOPNOTSUPP;

  *bits = cred->po->openmodes;
  return KERN_SUCCESS;
}

error_t
trivfs_S_io_set_all_openmodes (struct trivfs_protid *cred,
                               mach_port_t reply,
                               mach_msg_type_name_t replytype,
                               int mode)
{
  if (!cred)
    return EOPNOTSUPP;

  return 0;
}

kern_return_t
trivfs_S_io_set_some_openmodes (struct trivfs_protid *cred,
                                mach_port_t reply,
                                mach_msg_type_name_t replytype,
                                int bits)
{
  if (!cred)
    return EOPNOTSUPP;

  return KERN_SUCCESS;
}

kern_return_t
trivfs_S_io_clear_some_openmodes (struct trivfs_protid *cred,
                                  mach_port_t reply,
                                  mach_msg_type_name_t replytype,
                                  int bits)
{
  if (!cred)
    return EOPNOTSUPP;

  return KERN_SUCCESS;
}


/* Get/set the owner of the IO object.  For terminals, this affects
   controlling terminal behavior (see term_become_ctty).  For all
   objects this affects old-style async IO.  Negative values represent
   pgrps.  This has nothing to do with the owner of a file (as
   returned by io_stat, and as used for various permission checks by
   filesystems).  An owner of 0 indicates that there is no owner.  */

kern_return_t
trivfs_S_io_get_owner (struct trivfs_protid *cred,
                       mach_port_t reply, mach_msg_type_name_t replytype,
                       pid_t *owner)
{
  if (!cred)
    return EOPNOTSUPP;

  *owner = 0;
  return KERN_SUCCESS;
}

kern_return_t
trivfs_S_io_mod_owner (struct trivfs_protid *cred,
                       mach_port_t reply, mach_msg_type_name_t replytype,
                       pid_t owner)
{
  if (!cred)
    return EOPNOTSUPP;

  return EINVAL;
}
