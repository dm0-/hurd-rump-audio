# Hurd rump audio translator

Enable audio on GNU Hurd with a rump kernel server and PulseAudio.

This project provides a translator to be run at `/dev/audio` which is partially
compatible with the Solaris audio device.  It is not a complete implementation,
but it supports enough of the functionality for a PulseAudio sink.  This allows
any application that plays sounds through PulseAudio to output audio on a GNU
Hurd system.


## Installation

This project itself depends on Hurd libraries and the rump client library, and
it is a dependency of the PulseAudio build.  The order should go as follows:

 1. Build and install everything needed for a Hurd development environment
 2. Build and install the rump kernel's `buildrump.sh` and `pci-userspace`
 3. Build and install this
 4. Build and install PulseAudio with the Solaris module enabled

The first two steps can be done with the standard upstream build processes.

### Building the translator

The `Makefile` here is very basic.  It supports many of the usual variables for
configuring the build.  Just running `make && sudo make install` should be
enough to handle most needs.  It will install the translator at `/hurd/audio`
and its `ioctl` definitions at `/usr/include/sys/audio.h` by default.

### Building PulseAudio

PulseAudio versions older than 11.0 require a minor edit to compile, since the
Solaris module was only expected to be built with Solaris system headers.

    sed -i -e 's,<sys/conf.h>,<sys/poll.h>,' src/modules/module-solaris.c

After this edit, it should be able to be built and installed using the normal
upstream procedure.  Ensure the `configure` command does not use the option
`--disable-solaris` so it can detect `<sys/audio.h>`.  Alternatively, append
the `--enable-solaris` option to the `configure` command to require it.


## System configuration

With the translator and PulseAudio's Solaris module installed, the following
component configurations should result in an audible Hurd system.

### Hardware configuration

The rump kernel described here expects to drive an AC97 sound card.  For use
with QEMU, include the `-soundhw ac97` option to emulate such a device.

### Rump kernel configuration

The translator acts as a rump client that accesses `/dev/audio` in the rump
server's VFS.  The following command starts a server with enough modules to
support this and drive AC97 sound cards.

    rump_server -s -v \
        -lrumpvfs \
        -lrumpdev \
        -lrumpdev_audio \
        -lrumpdev_audio_ac97 \
        -lrumpdev_pci \
        -lrumpdev_pci_auich \
        unix:///run/rump.sock

Note the socket it creates at `/run/rump.sock` must be writable for the UID of
the translator process, which should be `0` (`root`) if it was started from a
translator record on the file system.

### Translator configuration

The translator command only has a single option, the URL of the rump server's
socket.  It is equivalent to using the `RUMP_SERVER` environment variable, but
the command-line option has higher precedence.  Assuming the translator was
installed as `/hurd/audio`, the following command will start it for the above
server and write the translator record to run it automatically on future boots.

    settrans -acfgp /dev/audio /hurd/audio --url=unix:///run/rump.sock

Make sure `/dev/audio` is writable for whatever users should have permission to
play sounds.  At this point `*.au` files should be playable (or recordable) by
direct writes (or reads) to this device.

### PulseAudio configuration

To account for the translator's incomplete compatibility with Solaris, a few
settings in PulseAudio's configuration should be defined to avoid unsupported
`ioctl` calls, etc.  If using `default.pa`, make the following modifications.

  * Avoid suspending, so don't load `module-suspend-on-idle`
  * Since `module-detect` isn't needed, replace it with `module-solaris`
  * Set `record=no` on `module-solaris` to avoid using streams
  * Set `channel_map=mono channels=1 format=ulaw rate=8000` on `module-solaris`
    to pick some known-supported playback parameters

Any applications that use PulseAudio should now be able to play sounds without
modifying them.  A user's PulseAudio server should start automatically when it
is needed, e.g. by running `paplay` on a sound file.


## Implementation details

The translator provides a device node supporting four important operations:

  * Writing data, i.e. playing audio out of speakers
  * Reading data, i.e. recording audio from a microphone
  * The `AUDIO_GETINFO` ioctl, for retrieving device information
  * The `AUDIO_SETINFO` ioctl, for changing device information

The installed header `<sys/audio.h>` is a modified version of NetBSD's header
`<sys/audioio.h>` to be more compatible with Solaris.  However, the changes
required to build on Hurd make it incompatible with both NetBSD and Solaris.

The Solaris device node uses the old POSIX streams interface which is not
implemented here, but PulseAudio's usage of the device is limited enough for it
to bypass the streams ioctls with the aforementioned configuration changes.

The audio ioctl character is `A` on both Solaris and NetBSD, but this value is
invalid on Hurd, so the installed `audio.h` uses `s` instead.  Also, Hurd's
ioctl definitions for structured parameters are normally defined by pairs of
the member data type and the count of consecutive members of that type.  The
`struct audio_info` members are too many to fit into that encoding scheme, so
the ioctl definitions in `audio.h` pretend all members are 64-bit integers,
with enough of them to match the same structure size.

The Solaris data structure has an `output_muted` member used by PulseAudio, but
NetBSD does not define it.  It was appended to the structure in `audio.h` as a
64-bit integer (to keep things evenly divisble) for source compatibility, but
the value is never actually used by the translator.

As a final note:  This is just a silly toy program.  If anyone really wants to
have PulseAudio use a rump server, it would be better to write a new PulseAudio
module that directly connects to it for platform independence and less IPC.
