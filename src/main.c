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

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#include "msg.h"

/*------------------------------------------------------------------------*/

#ifndef ARRAY_COUNT
#	define ARRAY_COUNT(x) (sizeof(x) / sizeof(x[0]))
#endif /* ARRAY_COUNT */

/*------------------------------------------------------------------------*/

static int terminate = 0;

/*------------------------------------------------------------------------*/

static const char *names[] = {
	"Jacob"
	"Michael",
	"Ethan",
	"Joshua",
	"Daniel",
	"Christopher",
	"Anthony",
	"William",
	"Matthew",
	"Andrew",
	"Alexander",
	"David",
	"Joseph",
	"Noah",
	"Emily",
	"Isabella",
	"Emma",
	"Ava",
	"Madison",
	"Sophia",
	"Olivia",
	"Abigail",
	"Hannah",
	"Elizabeth",
};

/*------------------------------------------------------------------------*/

void on_sigterm(int prm)
{
	terminate = 1;
}

/*------------------------------------------------------------------------*/

void* put_thread(void* prm)
{
	msg_queue_t* q = prm;
	const char* name;
	msg_t* m;

	while(!terminate) {
		usleep(1000);

		/* getting random name */
		name = names[rand() % ARRAY_COUNT(names)];

		if(!(m = msg_create(name, strlen(name) + 1)))
			continue;

		if(msg_queue_put(q, m)) {
			puts("Dropped");
			msg_free(m);
		}
	}

	return(NULL);
}

/*------------------------------------------------------------------------*/

void* fetch_thread(void* prm)
{
	msg_queue_t* q = prm;
	char name[0x100];
	msg_t* m;

	while(!terminate) {
		if(!(m = msg_queue_fetch(q)))
			continue;

		/* safety copying string */
		strncpy(name, m->payload, sizeof(name) - 1);
		name[sizeof(name) - 1] = 0;

		puts(name);

		msg_free(m);

		sleep(1);
	}

	return(NULL);
}

/*------------------------------------------------------------------------*/

int main(void)
{
	pthread_t t_put[5] = {0};
	pthread_t t_fetch;
	msg_queue_t* q;
	void* t_ret;
	int i;

	signal(SIGTERM, on_sigterm);
	signal(SIGINT, on_sigterm);

	srand(time(NULL));

	q = msg_queue_create(5);

	for(i = 0; i < ARRAY_COUNT(t_put); ++ i) {
		if(pthread_create(&t_put[i], NULL, put_thread, q)) {
			terminate = 1;
			break;
		}
	}

	if(!pthread_create(&t_fetch, NULL, fetch_thread, q)) {
		sleep(5);
		terminate = 1;
		pthread_join(t_fetch, &t_ret);
	}

	terminate = 1;

	for(i = 0; t_put[i] && i < ARRAY_COUNT(t_put); ++ i) {
		pthread_join(t_put[i], &t_ret);
	}

	msg_queue_destroy(q);

	return(0);
}
