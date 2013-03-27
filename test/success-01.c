/*
 *  Copyright (c) 2009-2013 Alper Akcan <alper.akcan@gmail.com>
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (int argc, char *argv[])
{
	void *rc;
	(void) argc;
	(void) argv;
	rc = strdup(argv[0]);
	if (rc == NULL) {
		fprintf(stderr, "strdup failed\n");
		exit(-1);
	}
	free(rc);
	rc = strndup(argv[0], 1024);
	if (rc == NULL) {
		fprintf(stderr, "strndup failed\n");
		exit(-1);
	}
	free(rc);
	return 0;
}
