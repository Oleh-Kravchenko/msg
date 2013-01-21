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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "msg.h"

/*------------------------------------------------------------------------*/

msg_queue_t* msg_queue_create(size_t max_size)
{
	msg_queue_t* res;

	if(!(res = malloc(sizeof(*res))))
		return(NULL);

	res->max_size = max_size;
	res->size = 0;
	res->first = NULL;
	res->last = NULL;

	pthread_mutex_init(&res->mutex, NULL);

	return(res);
}

/*------------------------------------------------------------------------*/

void msg_queue_destroy(msg_queue_t* q)
{
	msg_t* m;
	msg_t* n;

	if(!q)
		return;

	m = q->first;

	while(m) {
		n = m;
		m = m->next;

		msg_free(n);
	}

	pthread_mutex_destroy(&q->mutex);

	free(q);
}

/*------------------------------------------------------------------------*/

int msg_queue_put(msg_queue_t* q, msg_t* m)
{
	int res = -1;

	pthread_mutex_lock(&q->mutex);

	if(q->size >= q->max_size)
		goto exit;

	m->prev = NULL;
	m->next = q->first;

	if(q->first)
		q->first->prev = m;

	if(!q->last)
		q->last = m;

	q->first = m;

	++ q->size;

	res = 0;

exit:
	pthread_mutex_unlock(&q->mutex);

	return(res);
}

/*------------------------------------------------------------------------*/

msg_t* msg_queue_fetch(msg_queue_t* q)
{
	msg_t* res = NULL;

	pthread_mutex_lock(&q->mutex);

	if(!q->last)
		goto exit;

	res = q->last;

	q->last = q->last->prev;

	-- q->size;

	if(q->last)
		q->last->next = NULL;
	else
		q->first = NULL;

exit:
	pthread_mutex_unlock(&q->mutex);

	return(res);
}

/*------------------------------------------------------------------------*/

msg_t* msg_create(const void* payload, size_t len)
{
	msg_t* res;

	if(!(res = malloc(sizeof(*res))))
		return(NULL);

	if(!(res->payload = malloc(len)))
		goto err_payload;

	memcpy(res->payload, payload, len);
	res->len = len;
	res->prev = NULL;
	res->next = NULL;

	return(res);

err_payload:
	free(res);

	return(NULL);
}

/*------------------------------------------------------------------------*/

void msg_free(msg_t* m)
{
	if(!m)
		return;

	free(m->payload);
	free(m);
}
