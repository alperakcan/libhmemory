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
	int r;
	void *rc;
	char *strp;
	(void) argc;
	(void) argv;
	rc = malloc(1024);
	if (rc == NULL) {
		fprintf(stderr, "malloc failed\n");
		exit(-1);
	}
	free(rc);
	rc = calloc(1, 1024);
	if (rc == NULL) {
		fprintf(stderr, "calloc failed\n");
		exit(-1);
	}
	free(rc);
	rc = realloc(NULL, 1024);
	if (rc == NULL) {
		fprintf(stderr, "realloc failed\n");
		exit(-1);
	}
	free(rc);
	rc = realloc(NULL, 1024);
	if (rc == NULL) {
		fprintf(stderr, "realloc failed\n");
		exit(-1);
	}
	rc = realloc(rc, 2048);
	if (rc == NULL) {
		fprintf(stderr, "realloc failed\n");
		exit(-1);
	}
	free(rc);
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
	r = asprintf(&strp, "%s", argv[0]);
	if (r < 0) {
		fprintf(stderr, "asprintf failed\n");
		exit(-1);
	}
	free(strp);
	return 0;
}
