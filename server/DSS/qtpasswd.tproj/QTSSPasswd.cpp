/*
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * Copyright (c) 1999-2001 Apple Computer, Inc.  All Rights Reserved. The
 * contents of this file constitute Original Code as defined in and are
 * subject to the Apple Public Source License Version 1.2 (the 'License').
 * You may not use this file except in compliance with the License.  Please
 * obtain a copy of the License at http://www.apple.com/publicsource and
 * read it before using this file.
 *
 * This Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.  Please
 * see the License for the specific language governing rights and
 * limitations under the License.
 *
 *
 * @APPLE_LICENSE_HEADER_END@
 *
 */
 /*
	Modified htpasswd.c
	format of the file:
	username:crypt(password):MD5(username:realm:password)
*/

/******************************************************************************
 ******************************************************************************
 * NOTE! This program is not safe as a setuid executable!  Do not make it
 * setuid!
 ******************************************************************************
 *****************************************************************************/


#ifndef __Win32__
#include <sys/types.h>
#include <sys/stat.h>
#ifdef __linux__
	#include <time.h>
#endif
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#else
#include "OSHeaders.h"
#include <conio.h>
#include <time.h>
#endif


#ifdef __solaris__
        #include <crypt.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "StrPtrLen.h"
#include "md5digest.h"
#include "md5.h"
#include "../revision.h"	//this include should be fixed

#ifndef CHARSET_EBCDIC
#define LF 10
#define CR 13
#else /*CHARSET_EBCDIC*/
#define LF '\n'
#define CR '\r'
#endif /*CHARSET_EBCDIC*/

#define MAX_STRING_LEN 256

char *tn;


static char *strd(char *s)
{
    char *d;

    d = (char *) malloc(strlen(s) + 1);
    strcpy(d, s);
    return (d);
}

static void getword(char *word, char *line, char stop)
{
    int x = 0, y;

    for (x = 0; ((line[x]) && (line[x] != stop)); x++)
	word[x] = line[x];

    word[x] = '\0';
    if (line[x])
	++x;
    y = 0;

    while ((line[y++] = line[x++]));
}

static int getline(char *s, int n, FILE *f)
{
    register int i = 0;

    while (1) {
	s[i] = (char) fgetc(f);

	if (s[i] == CR)
	    s[i] = fgetc(f);

	if ((s[i] == 0x4) || (s[i] == LF) || (i == (n - 1))) {
	    s[i] = '\0';
	    return (feof(f) ? 1 : 0);
	}
	++i;
    }
}

static void putline(FILE *f, char *l)
{
    int x;

    for (x = 0; l[x]; x++)
	fputc(l[x], f);
    fputc('\n', f);
}


#if MPE 
/* MPE lacks getpass() and a way to suppress stdin echo.  So for now, just
 * issue the prompt and read the results with echo.  (Ugh).
 */

static char *getpass(const char *prompt)
{

    static char password[81];

    fputs(prompt, stderr);
    fgets((char *) &password, 80, stdin);

    if (strlen((char *) &password) > 8) {
	password[8] = '\0';
    }

    return (char *) &password;
}

#elif __Win32__
/*
 * Windows lacks getpass().  So we'll re-implement it here.
 */

static char *getpass(const char *prompt)
{
  static char password[81];
  int n = 0;
  
  fputs(prompt, stderr);
  
  while ((password[n] = _getch()) != '\r') {
    
    if(n == 80) {
      fputs("password can't be longer than 80 chars.\n", stderr);
      fputs(prompt, stderr);
      for(n = 0; n < 81; n++)
	password[n] = '\0';
      n = 0;
      continue;
    }
      
    if (password[n] >= ' ' && password[n] <= '~') {
      n++;
      printf("*");
    }
    else {
      printf("\n");
      fputs(prompt, stderr);
      n = 0;
    }
  }

  password[n] = '\0';
  printf("\n");

  return (char *) &password;
}

#endif

static void create_file(char *realm, FILE *f)
{
	fprintf(f, "realm %s\n", realm);
}

static char* digest(char *user, char *passwd, char *realm) 
{
	StrPtrLen userSPL(user), passwdSPL(passwd), realmSPL(realm), hashHex16Bit, hashSPL;
	CalcMD5HA1(&userSPL, &realmSPL, &passwdSPL, &hashHex16Bit); // memory allocated for hashHex16Bit.Ptr
	HashToString((unsigned char *)hashHex16Bit.Ptr, &hashSPL);	// memory allocated for hashSPL.Ptr
	char* digestStr = hashSPL.GetAsCString();
	delete [] hashSPL.Ptr;				// freeing memory allocated in above calls		
	delete [] hashHex16Bit.Ptr;
	return digestStr;
}

static void add_password_noprompt(char *user, char* password, char* realm, FILE *f)
{
    char *crpw, cpw[120], salt[9], *dpw;
    int crpwLen = 0;

    (void) srand((int) time((time_t *) NULL));
    to64(&salt[0], rand(), 8);
    salt[8] = '\0';

#ifdef __Win32__
    MD5Encode((const char *)password, (const char *)salt, cpw, sizeof(cpw));
#else
    crpw = (char *)crypt(password, salt); // cpw is crypt of password
    crpwLen = ::strlen(crpw);
    strncpy(cpw, crpw, crpwLen);
    cpw[crpwLen] = '\0';
#endif
    
    dpw = (char *)digest(user, password, realm); // dpw is digest of password
    
    fprintf(f, "%s:%s:%s\n", user, cpw, dpw);
}

static void add_password(char *user, char* realm, FILE *f)
{
    char *pw, *crpw, cpw[120], salt[9], *dpw;
    int len = 0, i = 0, crpwLen = 0;
    char *checkw;

    pw = strd((char *) getpass("New password:"));
    /* check for a blank password */    
    len = strlen(pw);
    checkw = new char[len+1];
    for(i = 0; i < len; i++)
      checkw[i] = ' ';
    checkw[len] = '\0';
    if(strcmp(pw, checkw) == 0) {
    	fprintf(stderr, "Password cannot be blank, sorry.\n");
		if (tn)
		  unlink(tn);
		delete(checkw);
		exit(1);
    }
    delete(checkw);
    
    if (strcmp(pw, (char *) getpass("Re-type new password:"))) {
	fprintf(stderr, "They don't match, sorry.\n");
	if (tn)
	    unlink(tn);
	exit(1);
    }
    (void) srand((int) time((time_t *) NULL));
    to64(&salt[0], rand(), 8);
    salt[8] = '\0';

#ifdef __Win32__
    MD5Encode((const char *)pw, (const char *)salt, cpw, sizeof(cpw));
#else
    crpw = (char *)crypt(pw, salt); // cpw is crypt of password
    crpwLen = ::strlen(crpw);
    strncpy(cpw, crpw, crpwLen);
    cpw[crpwLen] = '\0';
#endif
    
    dpw = (char *)digest(user, pw, realm); // dpw is digest of password
    
    fprintf(f, "%s:%s:%s\n", user, cpw, dpw);
    free(pw); // Do after cpw and dpw are used. 
}


static void usage(void)
{
    fprintf(stderr, "  qtpasswd %s built on: %s\n", kVersionString, __DATE__ ", "__TIME__);
    fprintf(stderr, "  Usage: qtpasswd [-c realm] passwordfile username\n");
    fprintf(stderr, "  The -c flag creates a new file.\n");
    exit(1);
}

static void interrupted(void)
{
    fprintf(stderr, "Interrupted.\n");
    if (tn)
	unlink(tn);
    exit(1);
}

int main(int argc, char *argv[])
{
    FILE *tfp, *f;
    char user[MAX_STRING_LEN];
    char line[MAX_STRING_LEN];
    char l[MAX_STRING_LEN];
    char w[MAX_STRING_LEN];
    char s[MAX_STRING_LEN];
    int found;
    int result;
    static char choice[81];
	
    tn = NULL;
#ifndef __Win32__
	//signal(SIGINT, (void (*)()) interrupted);
    signal(SIGINT, (void(*)(int))interrupted);
	//create file with owner RW permissions only
	umask(S_IRWXO|S_IRWXG);
#endif

    if ((argc == 5) || (argc == 6)) {
		if (strcmp(argv[1], "-c"))
	    	usage();
		if(tfp = fopen(argv[3], "r")) {
		  printf("File already exists. Do you wish to overwrite it? y or n [y] ");
		  fgets( (char *) &choice, 80, stdin);
		  if((choice[0] == 'n') || (choice[0] == 'N')) {
		    fclose(tfp);
		    exit(0);
		  }
		  else {
		    fclose(tfp);
		  }
		}
		if (!(tfp = fopen(argv[3], "w"))) {
	    	fprintf(stderr, "Could not open passwd file %s for writing.\n", argv[3]);
	    	perror("fopen");
	    	exit(1);
		}
		printf("Creating password file for realm %s.\n", argv[2]);
		create_file(argv[2], tfp);
		
		/* check for : in name before adding password */
		if(strchr(argv[4], ':') != NULL) {
		  printf("Username cannot contain a ':' character. Sorry!");
		  fclose(tfp);
		  exit(1);
		}

		printf("Adding password for %s.\n", argv[4]);
		if(argc == 6)
			add_password_noprompt(argv[4], argv[5], argv[2], tfp);
		else 
			add_password(argv[4], argv[2], tfp);
		fclose(tfp);
		exit(0);
    }
    else if ((argc != 3) && (argc != 4))
		usage();

#ifdef __Win32__
	char separator = '\\';
#else
	char separator = '/';
#endif
	char* tempDir;
	char* lastOccurOfSeparator = strrchr(argv[1], separator);
	int pathLength = strlen(argv[1]);
	if(lastOccurOfSeparator != NULL) 
	{
		int filenameLength = strlen(lastOccurOfSeparator + sizeof(char));
		tempDir = new char[pathLength - filenameLength];
		memcpy(tempDir, argv[1], (pathLength - filenameLength - sizeof(char)));
		tempDir[pathLength - filenameLength -1] = '\0';
		tn = tempnam(tempDir, NULL);
		//printf("tempDir is %s\n", tempDir);
		delete [] tempDir;
	}
	else 
	{
		tn = tempnam(".", NULL);
	}
	 	
    //tn = tmpnam(NULL);
    if (!(tfp = fopen(tn, "w"))) {
		printf("failed\n");
		fprintf(stderr, "Could not open temp file.\n");
		exit(1);
    }

    if (!(f = fopen(argv[1], "r"))) {
		fprintf(stderr, "Could not open passwd file %s for reading.\n", argv[1]);
		fprintf(stderr, "Use -c option to create new one.\n");
		exit(1);
    }
    strcpy(user, argv[2]);
    // Get the realm from the first line
	while (!(getline(line, MAX_STRING_LEN, f))) {
		if ((line[0] == '#') || (!line[0])) {
		    putline(tfp, line);
		    continue;
		}
		else {
			// line is "realm somename"
			strcpy(s ,line + 6); 
			putline(tfp, line);
			break;	
		}
	}
	// Look for an existing entry with the username
    found = 0;
    while (!(getline(line, MAX_STRING_LEN, f))) {
		if (found || (line[0] == '#') || (!line[0])) {
		    putline(tfp, line);
		    continue;
		}
		strcpy(l, line);
		getword(w, l, ':');
		if (strcmp(user, w)) {
		    putline(tfp, line);
		    continue;
		}
		else {
		    printf("Changing password for user %s\n", user);
		    if(argc == 4)
		    	add_password_noprompt(user, argv[3], s, tfp);
		    else 
			    add_password(user, s, tfp);
		    found = 1;
		}
    }
    if (!found) {
		/* check for : in name before adding password */
		if(strchr(user, ':') != NULL) {
			printf("Username cannot contain a ':' character. Sorry!");
			fclose(f);
			fclose(tfp);
			exit(1);
		}
      
		printf("Adding user %s\n", user);
		if(argc == 4)
			add_password_noprompt(user, argv[3], s, tfp);
		else 
			add_password(user, s, tfp);
	}
    fclose(f);
    fclose(tfp);
    
    // Remove old file and change name of temp file to match new file
    remove(argv[1]);
    
    result = rename(tn, argv[1]);
    if(result != 0)
    	perror("rename failed with error");
	 
  	return 0;
}
