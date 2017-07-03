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
 * The NetBSD audio_info structure consists of two sub-structures with 12 ints
 * and 8 chars each, followed by another 6 ints (for a total of 30 ints and 16
 * chars).  For Solaris compatibility, I've also appended an int64_t to this
 * structure to ensure a buffer size divisible by 64 bits.  Refer to the file
 * <sys/audio.h> to see the actual structure.
 *
 * This structure has too many different data types and too many fields to be
 * encoded by Hurd's usual _IOT calculation and MIG ioctl prototypes, so an
 * int64_t buffer is used in its place.
 *
 * The MIG language requires a constant value for the length of the buffer we
 * need to define.  "Constant" means an integer at preprocessor time, not the
 * usual constant expressions that C handles at compile time.
 */

#include <bits/wordsize.h>
#if __WORDSIZE == 64
#define AUDIO_INFO_BUFFER_INT64S 33
#elif __WORDSIZE == 32
#define AUDIO_INFO_BUFFER_INT64S 18
#else
#error "What kind of computer are you using?"
#endif
