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

__thread msg_queue_err_t msg_queue_err = MSG_ERR_NONE;

/*------------------------------------------------------------------------*/

msg_queue_t* msg_queue_create(size_t max_size)
{
	msg_queue_t* res;

	if(!(res = malloc(sizeof(*res)))) {
		msg_queue_err = MSG_ERR_NO_MEMORY;

		return(NULL);
	}

	res->max_size = max_size;
	res->size = 0;
	res->first = NULL;
	res->last = NULL;

	msg_queue_err = MSG_ERR_NONE;

	pthread_mutex_init(&res->mutex, NULL);

	pthread_mutex_init(&res->cond_mutex, NULL);

	pthread_cond_init(&res->cond, NULL);

	return(res);
}

/*------------------------------------------------------------------------*/

void msg_queue_destroy(msg_queue_t* q)
{
	msg_t* m;
	msg_t* n;

	if(!q) {
		msg_queue_err = MSG_ERR_INVALID_ARG;

		return;
	}

	m = q->first;

	while(m) {
		n = m;
		m = m->next;

		msg_free(n);
	}

	pthread_mutex_destroy(&q->mutex);

	pthread_mutex_destroy(&q->cond_mutex);

	pthread_cond_destroy(&q->cond);

	free(q);

	msg_queue_err = MSG_ERR_NONE;
}

/*------------------------------------------------------------------------*/

int msg_queue_put(msg_queue_t* q, msg_t* m)
{
	int res = -1;

	pthread_mutex_lock(&q->mutex);

	if(q->size >= q->max_size) {
		msg_queue_err = MSG_ERR_QUEUE_IS_FULL;

		goto exit;
	}

	m->prev = NULL;
	m->next = q->first;

	if(q->first)
		q->first->prev = m;

	if(!q->last)
		q->last = m;

	q->first = m;

	++ q->size;

	res = 0;

	msg_queue_err = MSG_ERR_NONE;

	pthread_mutex_lock(&q->cond_mutex);
	pthread_cond_signal(&q->cond);
	pthread_mutex_unlock(&q->cond_mutex);

exit:
	pthread_mutex_unlock(&q->mutex);

	return(res);
}

/*------------------------------------------------------------------------*/

static msg_t* msg_queue_fetch_internal(msg_queue_t* q)
{
	msg_t* res = NULL;

	if(!q->last) {
		msg_queue_err = MSG_ERR_QUEUE_IS_EMPTY;

		return(res);
	}

	res = q->last;

	q->last = q->last->prev;

	-- q->size;

	if(q->last)
		q->last->next = NULL;
	else
		q->first = NULL;

	msg_queue_err = MSG_ERR_NONE;

	return(res);
}

/*------------------------------------------------------------------------*/

msg_t* msg_queue_fetch(msg_queue_t* q)
{
	msg_t* res = NULL;

	pthread_mutex_lock(&q->mutex);
	res = msg_queue_fetch_internal(q);
	pthread_mutex_unlock(&q->mutex);

	return(res);
}

/*------------------------------------------------------------------------*/

msg_t* msg_queue_fetch_wait(msg_queue_t* q, int sec)
{
	struct timespec timeout;
	int res_cond;
	msg_t* res;

	if((res = msg_queue_fetch(q)))
		return(res);

	timeout.tv_sec = time(NULL) + sec;
	timeout.tv_nsec = 0;

	pthread_mutex_lock(&q->cond_mutex);
	res_cond = pthread_cond_timedwait(&q->cond, &q->cond_mutex, &timeout);
	pthread_mutex_unlock(&q->cond_mutex);

	if(!res_cond)
		return(msg_queue_fetch(q));

	msg_queue_err = MSG_ERR_TIMEOUT;

	return(res);
}

/*------------------------------------------------------------------------*/

msg_queue_err_t msg_queue_errno(void)
{
	return(msg_queue_err);
}

/*------------------------------------------------------------------------*/

const char* msg_queue_errno2str(msg_queue_err_t err)
{
	static const char* msg_queue_errstr[] = {
		"MSG_ERR_NONE",
		"MSG_ERR_NO_MEMORY",
		"MSG_ERR_MUTEX_FAILED",
		"MSG_ERR_INVALID_ARG",
		"MSG_ERR_QUEUE_IS_FULL",
		"MSG_ERR_QUEUE_IS_EMPTY",
		"MSG_ERR_TIMEOUT",
	};

	if(err >= 0 && err < ARRAY_COUNT(msg_queue_errstr))
		return(msg_queue_errstr[err]);

	return(NULL);
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
