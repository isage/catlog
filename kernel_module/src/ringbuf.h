/*
CatLog
Copyright (C) 2024 Cat
Copyright (C) 2020 Princess of Sleeping
Copyright (C) 2020 Asakura Reiko

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef RINGBUF_H
#define RINGBUF_H

#include <psp2kern/types.h>

int ringbuf_init(int size);
int ringbuf_term(void);

int ringbuf_put(char *c, int size);
int ringbuf_put_clobber(char *c, int size);
int ringbuf_get(char *c, int size);
int ringbuf_get_wait(char *c, int size, SceUInt *timeout);

#endif
