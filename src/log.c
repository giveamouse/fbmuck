
/* $Header$ */

/*
 * Handles logging of tinymuck
 *
 * Copyright (c) 1990 Chelsea Dyerman
 * University of California, Berkeley (XCF)
 *
 */

#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <stdarg.h>
#include <time.h>

#include "fbstrings.h"
#include "externs.h"
#include "interface.h"

/* cks: these are varargs routines. We are assuming ANSI C. We could at least
   USE ANSI C varargs features, no? Sigh. */

void
log2file(char *myfilename, char *format, ...)
{
	va_list args;
	FILE *fp;

	va_start(args, format);

	if ((fp = fopen(myfilename, "a")) == NULL) {
		fprintf(stderr, "Unable to open %s!\n", myfilename);
		vfprintf(stderr, format, args);
	} else {
		vfprintf(fp, format, args);
		fprintf(fp, "\n");
		fclose(fp);
	}
	va_end(args);
}

void
log_sanity(char *format, ...)
{
	va_list args;
	FILE *fp;
	time_t lt;
	char buf[40];

	va_start(args, format);
	lt = time(NULL);

	*buf = '\0';
	if ((fp = fopen(LOG_SANITY, "a")) == NULL) {
		fprintf(stderr, "Unable to open %s!\n", LOG_STATUS);
		fprintf(stderr, "%.16s: ", ctime(&lt));
		vfprintf(stderr, format, args);
	} else {
		format_time(buf, 32, "%c", localtime(&lt));
		fprintf(fp, "%.32s: ", buf);
		vfprintf(fp, format, args);
		fclose(fp);
	}
	va_end(args);
}

void
log_status(char *format, ...)
{
	va_list args;
	FILE *fp;
	time_t lt;
	char buf[40];

	va_start(args, format);
	lt = time(NULL);

	*buf = '\0';
	if ((fp = fopen(LOG_STATUS, "a")) == NULL) {
		fprintf(stderr, "Unable to open %s!\n", LOG_STATUS);
		fprintf(stderr, "%.16s: ", ctime(&lt));
		vfprintf(stderr, format, args);
	} else {
		format_time(buf, 32, "%c", localtime(&lt));
		fprintf(fp, "%.32s: ", buf);
		vfprintf(fp, format, args);
		fclose(fp);
	}
	va_end(args);
}

void
log_muf(char *format, ...)
{
	va_list args;
	FILE *muflog;
	time_t lt;
	char buf[40];

	va_start(args, format);
	lt = time(NULL);

	if ((muflog = fopen(LOG_MUF, "a")) == NULL) {
		fprintf(stderr, "Unable to open %s!\n", LOG_MUF);
		fprintf(stderr, "%.16s: ", ctime(&lt));
		vfprintf(stderr, format, args);
	} else {
		format_time(buf, 32, "%c", localtime(&lt));
		fprintf(muflog, "%.32s: ", buf);
		vfprintf(muflog, format, args);
		fclose(muflog);
	}
	va_end(args);
}

void
log_gripe(char *format, ...)
{
	va_list args;
	FILE *fp;
	time_t lt;
	char buf[40];

	va_start(args, format);
	lt = time(NULL);

	*buf = '\0';
	if ((fp = fopen(LOG_GRIPE, "a")) == NULL) {
		fprintf(stderr, "Unable to open %s!\n", LOG_GRIPE);
		fprintf(stderr, "%.16s: ", ctime(&lt));
		vfprintf(stderr, format, args);
	} else {
		format_time(buf, 32, "%c", localtime(&lt));
		fprintf(fp, "%.32s: ", buf);
		vfprintf(fp, format, args);
		fclose(fp);
	}
	va_end(args);
}

void
log_command(char *format, ...)
{
	va_list args;
	char buf[40];
	FILE *fp;
	time_t lt;

	va_start(args, format);
	lt = time(NULL);

	*buf = '\0';
	if ((fp = fopen(COMMAND_LOG, "a")) == NULL) {
		fprintf(stderr, "Unable to open %s!\n", COMMAND_LOG);
		vfprintf(stderr, format, args);
	} else {
		format_time(buf, 32, "%c", localtime(&lt));
		fprintf(fp, "%.32s: ", buf);
		vfprintf(fp, format, args);
		fclose(fp);
	}
	va_end(args);
}

void
strip_evil_characters(char *badstring)
{
 	unsigned char *s = (unsigned char*)badstring;
 
 	if (!s)
  		return;
 
 	for (; *s; ++s) {
 		*s &= 0x7f; /* No high ascii */
 
 		if (*s == 0x1b)
 			*s = '['; /* No escape (Aieeee!) */
 
 		if (!isprint(*s))
 			*s = '_';
  	}
  	return;
}

void
log_user(dbref player, dbref program, char *logmessage)
{
	char logformat[BUFFER_LEN];
	char buf[40];
	time_t lt = 0;
	int len = 0;

	*buf='\0';
	*logformat='\0';

	lt=time(NULL);	
	format_time(buf, 32, "%c", localtime(&lt));

	snprintf(logformat,BUFFER_LEN,"%s(#%d) [%s(#%d)] at %.32s: ", NAME(player), player, NAME(program), program, buf);
	len = BUFFER_LEN - strlen(logformat)-1;
	strncat (logformat, logmessage, len);
	strip_evil_characters(logformat);
	log2file(USER_LOG,"%s",logformat);
}

void
notify_fmt(dbref player, char *format, ...)
{
	va_list args;
	char bufr[BUFFER_LEN];

	va_start(args, format);
	vsnprintf(bufr, sizeof(bufr), format, args);
	bufr[sizeof(bufr)-1] = '\0';
	notify(player, bufr);
	va_end(args);
}
