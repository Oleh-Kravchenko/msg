/*
 * msg -- simple message queue
 * Copyright (C) 2013  Oleh Kravchenko <oleg@kaa.org.ua>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __MSG_H
#define __MSG_H

#include <stdlib.h>
#include <pthread.h>

/*------------------------------------------------------------------------*/

typedef struct msg {
	size_t len;

	char* payload;

	struct msg* prev;

	struct msg* next;
} msg_t;

/*------------------------------------------------------------------------*/

typedef struct msg_queue {
	msg_t* first;

	msg_t* last;

	size_t size;

	size_t max_size;

	pthread_mutex_t mutex;
} msg_queue_t;

/*------------------------------------------------------------------------*/

msg_queue_t* msg_queue_create(size_t max_size);

void msg_queue_destroy(msg_queue_t*);

int msg_queue_put(msg_queue_t* q, msg_t* m);

msg_t* msg_queue_fetch(msg_queue_t* q);

msg_t* msg_create(const void*, size_t len);

void msg_free(msg_t* m);

#endif /* __MSG_H */
