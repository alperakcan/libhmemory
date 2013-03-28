/*
 *  Copyright (c) 2008-2013 Alper Akcan <alper.akcan@gmail.com>
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
	char *rc;
	(void) argc;
	(void) argv;
	r = asprintf(&rc, "%s", argv[0]);
	if (r < 0) {
		fprintf(stderr, "asprintf failed\n");
		exit(-1);
	}
	free(rc);
	return 0;
}
