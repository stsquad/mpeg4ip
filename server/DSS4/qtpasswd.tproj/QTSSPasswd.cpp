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
#include <sys/time.h>
#include <signal.h>
#include "getopt.h"
#include <unistd.h>
#else
#include "getopt.h"
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
#include "revision.h"

#ifdef __linux__
	#include <time.h>
	#include <crypt.h>
#endif

#ifndef CHARSET_EBCDIC
#define LF 10
#define CR 13
#else /*CHARSET_EBCDIC*/
#define LF '\n'
#define CR '\r'
#endif /*CHARSET_EBCDIC*/

#define MAX_STRING_LEN 255
#define MAX_LINE_LEN 5120
#define MAX_PASSWORD_LEN 80

#if __MacOSX__
const char* kDefaultQTPasswdFilePath = "/Library/QuickTimeStreaming/Config/qtusers";
#elif __Win32__
const char* kDefaultQTPasswdFilePath = "C:\\Program Files\\Darwin Streaming Server\\qtusers";
#else
const char* kDefaultQTPasswdFilePath = "/etc/streaming/qtusers";
#endif


const char* kDefaultRealmString = "Streaming Server";


char *tn;

/*
 * CopyString: 	Returns a malloc'd copy
 *              of inString
 */
static char *CopyString(char *inString)
{
    char *outString = (char *) malloc(strlen(inString) + 1);
    strcpy(outString, inString);
    return outString;
}

/*
 * GetWord: 	Reads all characters from the
 *              line until the stop character
 *              and returns it in word
 */
static void GetWord(char *word, char *line, char stop)
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

/*
 * GetLine: 	Reads from file f, n characters
 *              or until newline and returns
 *              puts the line in s.
 */
static int GetLine(char *s, int n, FILE *f)
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

/*
 * PutLine: 	Writes string l to f and 
 *              puts a '\n' at the end
 */
static void PutLine(FILE *f, char *l)
{
    int x;

    for (x = 0; l[x]; x++)
	fputc(l[x], f);
    fputc('\n', f);
}


#if __Win32__
/*
 * Windows lacks getpass().  So we'll re-implement it here.
 */

static char *getpass(const char *prompt)
{
  static char password[MAX_PASSWORD_LEN + 1];
  int n = 0;
  
  fputs(prompt, stderr);
  
  while ((password[n] = _getch()) != '\r') {
    
    if(n == MAX_PASSWORD_LEN) {
      fputs("password can't be longer than MAX_PASSWORD_LEN chars.\n", stderr);
      fputs(prompt, stderr);
      for(n = 0; n < (MAX_PASSWORD_LEN + 1); n++)
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

/*
 * Digest:	Returns the MD5 hash of user:realm:password 
 */
static char* Digest(char *user, char *passwd, char *realm) 
{
	StrPtrLen userSPL(user), passwdSPL(passwd), realmSPL(realm), hashHex16Bit, hashSPL;
	CalcMD5HA1(&userSPL, &realmSPL, &passwdSPL, &hashHex16Bit); // memory allocated for hashHex16Bit.Ptr
	HashToString((unsigned char *)hashHex16Bit.Ptr, &hashSPL);	// memory allocated for hashSPL.Ptr
	char* digestStr = hashSPL.GetAsCString();
	delete [] hashSPL.Ptr;				// freeing memory allocated in above calls		
	delete [] hashHex16Bit.Ptr;
	return digestStr;
}

/*
 * AddPasswordWithoutPrompt: 	Adds the entry in the file
 *                              user:crpytofpassword:md5hash(user:realm:password) 
 */
static void AddPasswordWithoutPrompt(char *user, char* password, char* realm, FILE *f)
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
    
    dpw = (char *)Digest(user, password, realm); // dpw is digest of password
    
    fprintf(f, "%s:%s:%s\n", user, cpw, dpw);
}

/*
 * AddPassword:	Prompts the user for a password
 *              and adds the entry in the file
 *              user:crpytofpassword:md5hash(user:realm:password) 
 */
static void AddPassword(char *user, char* realm, FILE *f)
{
    char *pw, *crpw, cpw[120], salt[9], *dpw;
    int len = 0, i = 0, crpwLen = 0;
    char *checkw;

    pw = CopyString((char *) getpass("New password:"));
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
    
    dpw = (char *)Digest(user, pw, realm); // dpw is digest of password
    
    fprintf(f, "%s:%s:%s\n", user, cpw, dpw);
    free(pw); // Do after cpw and dpw are used. 
}

/*
 * Usage:	Prints the usage and calls exit
 */
static void usage(void)
{
	fprintf(stderr, "  qtpasswd %s built on: %s\n", kVersionString, __DATE__ ", "__TIME__);
	fprintf(stderr, "  Usage: qtpasswd [-F] [-f filename] [-c] [-r realm] [-p password] [-d] username\n");
	fprintf(stderr, "  -F   Don't ask for confirmation when deleting users or overwriting existing files.\n");
	fprintf(stderr, "  -f   Password file to manipulate (Default is \"%s\").\n", kDefaultQTPasswdFilePath);
	fprintf(stderr, "  -c   Create new file.\n");
	fprintf(stderr, "  -r   The realm name to use when creating a new file via \"-c\" (Default is \"%s\").\n", kDefaultRealmString);
	fprintf(stderr, "  -p   Allows entry of password at command line rather than prompting for it.\n");
	fprintf(stderr, "  -P   File to read the password from rather than prompting for it.\n");
	fprintf(stderr, "  -d   Delete the specified user.\n");
	fprintf(stderr, "  -h   Displays usage.\n");
        fprintf(stderr, "  Note:\n");
        fprintf(stderr, "  Usernames cannot be more than %d characters long and must not include a colon [:].\n", MAX_STRING_LEN);
        fprintf(stderr, "  Passwords cannot be more than %d characters long.\n", MAX_PASSWORD_LEN);
        fprintf(stderr, "  If the username/password contains whitespace or characters that may be\n");
        fprintf(stderr, "  interpreted by the shell please enclose it in single quotes,\n");
        fprintf(stderr, "  to prevent it from being interpolated.\n");
        fprintf(stderr, "\n");
	exit(1);
}

/*
 * Interrupted: signal handler for SIGINT
 */
static void Interrupted(void)
{
    fprintf(stderr, "Interrupted.\n");
    if (tn)
	unlink(tn);
    exit(1);
}

int main(int argc, char *argv[])
{
    FILE *tfp, *f, *passFilePtr;
    //char line[MAX_STRING_LEN + 1];
    char line[MAX_LINE_LEN];
    char lineFromFile[MAX_LINE_LEN];
    char usernameFromFile[MAX_STRING_LEN + 1];
    char realmFromFile[MAX_STRING_LEN + 1];
    int found;
    int result;
    static char choice[81];

    int doCreateNewFile = 0;
    int doDeleteUser = 0;
    int confirmPotentialDamage = 1;
    char* qtusersFilePath = NULL;
    char* userName = NULL;
    char* realmString = NULL;
    char* password = NULL;
    char* passwordFilePath = NULL;
    int ch;
    extern char* optarg;
    extern int optind;

    while ((ch = getopt(argc, argv, "cf:r:p:P:dFh")) != EOF)
    {
        switch(ch) 
        {

            case 'f':
                    qtusersFilePath = CopyString(optarg);
            break;

            case 'c':
                    doCreateNewFile = 1;
           	break;

            case 'r':
                    realmString = CopyString(optarg);
                    if (::strlen(realmString) > MAX_STRING_LEN)
                    {
                        fprintf(stderr, "Realm cannot have more than %d characters.\n", MAX_STRING_LEN);
                        printf("Exiting! \n");
                        exit(1);
                    }
            break;

            case 'p':
                    password = CopyString(optarg);
                    if (::strlen(password) > MAX_PASSWORD_LEN)
                    {
                        fprintf(stderr, "Password cannot have more than %d characters.\n", MAX_PASSWORD_LEN);
                        printf("Exiting! \n");
                        exit(1);
                    }
            break;

            case 'P':
                    passwordFilePath = CopyString(optarg);
            break;

            case 'd':
                    doDeleteUser = 1;
           	break;

            case 'F':
                    confirmPotentialDamage = 0;
            break;

	    case 'h':
	            usage(); 
		    break;

            case '?':
            default:
                    usage();
            break;
        }
    }

        if ((password == NULL) && (passwordFilePath != NULL))
	{
	        if ((passFilePtr = fopen(passwordFilePath, "r")) != NULL ) 
	        {
                        char passline[MAX_STRING_LEN];
                        char passFromFile[MAX_STRING_LEN];
		        int passLen = 0;
                        
                        ::memset(passline, 0, MAX_STRING_LEN);
                        ::memset(passFromFile, 0, MAX_STRING_LEN);
                        
                        GetLine(passline, MAX_STRING_LEN, passFilePtr);

                        if (passline[0] == '\'')		// if it is single quoted, read until the end single quote
                            GetWord(passFromFile, (passline + 1), '\'');
                        else if (passline[0] == '"')		// if it is double quoted, read until the end double quote
                            GetWord(passFromFile, (passline + 1), '"');
                        else					// if it is not quoted, read until the first whitespace
                            GetWord(passFromFile, passline, ' ');
                           
                        passLen = ::strlen(passFromFile);
                            
                        if (passLen == 0)
                        {
                            fprintf(stderr, "Password in file %s is blank.\n", passwordFilePath);
                            printf("Exiting! \n");
                            exit(1);
                        }
                        else if (passLen > MAX_PASSWORD_LEN)
                        {
                            fprintf(stderr, "Password in file %s has more than %d characters. Cannot accept password.\n", passwordFilePath, MAX_PASSWORD_LEN);
                            printf("Exiting! \n");
                            exit(1);
                        }
                        else
                            password = CopyString(passFromFile);

			fclose(passFilePtr);
		}
	}

	//deleting a user and (creating a file or setting a password) don't make sense together
	if ( doDeleteUser && (doCreateNewFile || password != NULL) )
	{
		usage();
	}

	//realm name only makes sense when creating a new password file
	if ( !doCreateNewFile && (realmString != NULL) )
	{
		usage();
	}


    if (argv[optind] != NULL)
    {
        userName = CopyString(argv[optind]);
        if (::strlen(userName) > MAX_STRING_LEN)
        {
	       fprintf(stderr, "Username cannot have more than %d characters.\n", MAX_STRING_LEN);
	       printf("Exiting! \n");
	       exit(1);
	}
    }
    else
    {
		
		usage();
    }

	if (confirmPotentialDamage && doDeleteUser)
	{
        printf("Delete user %s? y or n [y] ", userName);
        fgets( (char*)&choice, 80, stdin);
        if( choice[0] == 'n' || choice[0] == 'N' ) 
            exit(0);
	}

    if (qtusersFilePath == NULL)
    {
        qtusersFilePath = new char[strlen(kDefaultQTPasswdFilePath)+1];
        strcpy(qtusersFilePath, kDefaultQTPasswdFilePath);
    }

    if (realmString  == NULL)
    {
        char* kDefaultRealmString = "Streaming Server";

        realmString = new char[strlen(kDefaultRealmString)+1];
        strcpy(realmString, kDefaultRealmString);
    }

	
    tn = NULL;
#ifndef __Win32__
	//signal(SIGINT, (void (*)()) Interrupted);
    signal(SIGINT, (void(*)(int))Interrupted);
	//create file with owner RW permissions only
	umask(S_IRWXO|S_IRWXG);
#endif

    if (doCreateNewFile)
    {
		if (confirmPotentialDamage)
	        if( (tfp = fopen(qtusersFilePath, "r")) != NULL ) 
	        {
	            fclose(tfp);
	
	            printf("File already exists. Do you wish to overwrite it? y or n [y] ");
	            fgets( (char*)&choice, 80, stdin);
	            if( choice[0] == 'n' || choice[0] == 'N' ) 
	                exit(0);
	                
	        }

        //create new file or truncate existing one to 0
        if ( (tfp = fopen(qtusersFilePath, "w")) == NULL)
        {
            fprintf(stderr, "Could not open password file %s for writing. (err=%d %s)\n", qtusersFilePath, errno, strerror(errno));
            perror("fopen");
            exit(1);
        }

        printf("Creating password file for realm %s.\n", realmString);

        //write the realm into the file
        fprintf(tfp, "realm %s\n", realmString);

        fclose(tfp);
    }

#ifdef __Win32__
	char separator = '\\';
#else
	char separator = '/';
#endif
	char* tempDir;
	char* lastOccurOfSeparator = strrchr(qtusersFilePath, separator);
	int pathLength = strlen(qtusersFilePath);
	if(lastOccurOfSeparator != NULL) 
	{
            int filenameLength = strlen(lastOccurOfSeparator + sizeof(char));
            tempDir = new char[pathLength - filenameLength];
            memcpy(tempDir, qtusersFilePath, (pathLength - filenameLength - sizeof(char)));
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
    if (!(tfp = fopen(tn, "w"))) 
    {
        printf("failed\n");
        fprintf(stderr, "Could not open temp file. (err=%d %s)\n", errno, strerror(errno));
        exit(1);
    }

    if (!(f = fopen(qtusersFilePath, "r")))
    {
        fprintf(stderr, "Could not open passwd file %s for reading. (err=%d:%s)\n", qtusersFilePath, errno, strerror(errno));
        fprintf(stderr, "Use -c option to create new one.\n");
        exit(1);
    }

    // Get the realm from the first line
    while (!(GetLine(line, MAX_LINE_LEN, f))) 
    {
        if ((line[0] == '#') || (!line[0]))
        {
            PutLine(tfp, line);
            continue;
        }
        else
        {
            // line is "realm somename"
            if( strncmp(line, "realm", strlen("realm")) != 0 )
            {
                    fprintf(stderr, "Invalid users file.\n");
                    fprintf(stderr, "The first non-comment non-blank line must be the realm line\n");
                    fprintf(stderr, "The file may have been tampered manually!\n");
                    exit(1);
            }
            strcpy(realmFromFile ,line + strlen("realm")+1); 
            PutLine(tfp, line);
            break;	
        }
    }
	// Look for an existing entry with the username
    found = 0;
    while (!(GetLine(line, MAX_LINE_LEN, f))) 
    {
		//write comments and blank lines out to temp file
        if (found || (line[0] == '#') || (line[0] == 0)) 
        {
            PutLine(tfp, line);
            continue;
        }
        strcpy(lineFromFile, line);
        GetWord(usernameFromFile, lineFromFile, ':');

		//if not the user we're looking for, write the line out to the temp file
        if (strcmp(userName, usernameFromFile) != 0) 
        {
            PutLine(tfp, line);
            continue;
        }
        else 
        {
			if (doDeleteUser)
			{	//to delete a user - just don't write it out to the temp file
           		printf("Deleting userName %s\n", userName);
			}
			else
			{
	            printf("Changing password for userName %s\n", userName);
	            if(password != NULL)
	                AddPasswordWithoutPrompt(userName, password, realmFromFile, tfp);
	            else 
	                AddPassword(userName, realmFromFile, tfp);
			}
            found = 1;
        }
    }
    
    if (!found) 
    {
		if (doDeleteUser)
		{
            printf("Username %s not found.\n", userName);
			exit(0);
		}

        /* check for : in name before adding password */
        if(strchr(userName, ':') != NULL) 
        {
            printf("Username cannot contain a ':' character.");
            fclose(f);
            fclose(tfp);
            exit(1);
        }
    
        printf("Adding userName %s\n", userName);
        if(password != NULL)
            AddPasswordWithoutPrompt(userName, password, realmFromFile, tfp);
        else 
            AddPassword(userName, realmFromFile, tfp);
    }
    
    fclose(f);
    fclose(tfp);
    
    // Remove old file and change name of temp file to match new file
    remove(qtusersFilePath);
    
    result = rename(tn, qtusersFilePath);
    if(result != 0)
    	perror("rename failed with error");
	 
  	return 0;
}
