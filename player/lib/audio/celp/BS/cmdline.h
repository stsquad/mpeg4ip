/**********************************************************************
MPEG-4 Audio VM
Command line module



This software module was originally developed by

Charalampos Ferkidis (University of Hannover / ACTS-MoMuSys)
Heiko Purnhagen (University of Hannover / ACTS-MoMuSys)
partially based on a concept by FhG IIS, Erlangen

and edited by

in the course of development of the MPEG-2 NBC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 NBC/MPEG-4 Audio tools
as specified by the MPEG-2 NBC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 NBC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 NBC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
NBC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 NBC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1996.



Header file: cmdline.h

$Id: cmdline.h,v 1.1 2002/05/13 18:57:42 wmaycisco Exp $

Required modules:
common.o		common module

Authors:
      partially based on a concept by FHG <iis.fhg.de>
CF    Charalampos Ferekidis, Uni Hannover <ferekidi@tnt.uni-hannover.de>
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>

Changes:
28-may-96   CF    added functions for parsing of init files and strings
05-jun-96   HP    several changes
                  moved ErrorMsg() to seperat module
06-jun-96   HP    setDefault added to CmdLineEval()
07-jun-96   HP    use CommonProgName(), CommonWarning(), CommonExit()
10-jun-96   HP    ...
13-jun-96   HP    changed ComposeFileName()
19-jun-96   HP    changed ComposeFileName()
20-jun-96   HP    added NOTE re CmdLineParseString()
08-aug-96   HP    changed string handling in CmdLineParseString()
                  changed handling of variable length argument list
                  added CmdLineEvalFree(), CmdLineParseFree()
09-aug-96   HP    ...
26-aug-96   HP    CVS
07-apr-97   HP    "-" support in CmdLineEval() and ComposeFileName()
04-may-98   HP/BT unsigned in ComposeFileName()
**********************************************************************/


#ifndef _cmdline_h_
#define _cmdline_h_


/* ---------- declarations ---------- */

typedef struct {	/* cmdline parameter info list element */
  void *argument;	/* ptr to argument */
			/*  NULL to mark end of paraList !!! */
  char *format;		/* format of argument (e.g. "%d") */
			/*  "%s"    char *string -> *argument */
			/*  NULL    int varArgIdx[] -> *argument */
			/*           variable length argument list: */
			/*           varArgIdx[] contains the indices of all */
			/*           remaining entries in argv[] that are */
			/*           not switches. */
			/*           varArgIdx[] is terminated by -1. */
			/*           (The next entry in paraList must be */
			/*           the end mark *argument=NULL.) */
			/*  others  value -> *argument */
  char *help;		/* description of parameter for help */
			/*  (e.g. "<filename>") */
} CmdLinePara;  

typedef struct {	/* cmdline switch info list element */
  char *switchName;	/* switch name (case sensitive, without '-') */
			/*  NULL to mark end of switchList !!! */
  void *argument;	/* ptr to argument */
			/*  NULL for help switch !!! */
			/*  (format, defaultValue, usedFlag are ignored) */
  char *format;		/* format of argument (e.g. "%d") */
			/*  "%s"    char* -> *argument */
			/*  NULL    switch used flag -> *(int*)argument */
			/*           (0=not used  1=used) */
			/*           (for switch without argument) */
			/*  others  value -> *argument */
  char *defaultValue;	/* default value string for argument */
			/*  argument value unchanged if defaultValue==NULL */
			/*  default value ignored if format==NULL */
  int  *usedFlag;	/* ptr to switch used flag */
			/*  0=not used  1=used */
			/*  NULL if not required */
  char *help;		/* description of switch for help */
			/*  '\n' can be used to separate multiple lines */
			/*  if defaultValue!=NULL the */
			/*  " (dftl: <defaultValue>)" is appended */
			/*  (appending "\n\b" places "dflt:..." on next line */
} CmdLineSwitch;


/* ---------- functions ---------- */

#ifdef __cplusplus
extern "C" {
#endif


/* CmdLineInit() */
/* Init command line module. */

void CmdLineInit (
  int debugLevel);		/* in: debug level */
				/*     0=off  1=basic  2=full */


/* CmdLineEval() */
/* Evaluate parameters and switches in argv[]. */
/* Command line mode (progNamePtr!=NULL): */
/*   Evaluate command line in argv[] from main() and extract program name */
/*   from argv[0]. Pass program name to CommonProgName(). */
/*   Switches are identified by preceding '-' in the command line. */
/* Token list mode (progNamePtr==NULL): */
/*   Evaluate token list in argv[] (as generated by CmdLineParseString() or */
/*   CmdLineParseFile()). Switches are identified by preceding '-' if */
/*   paraList!=NULL. Switches don't have a preceding '-' if paraList==NULL. */

int CmdLineEval (
  int argc,			/* in: num command line args */
  char *argv[],			/* in: command line args */
  CmdLinePara *paraList,	/* in: parameter info list */
				/*     or NULL */
  CmdLineSwitch *switchList,	/* in: switch info list */
				/*     or NULL */
  int setDefault,		/* in: 0 = leave switch used flags and args */
				/*         unchanged */
				/*     1 = init switch used flags and args */
				/*         with defaultValue */
  char **progNamePtr);		/* out: program name */
				/*      or NULL */
				/* returns: */
				/*  0=OK  1=help switch  2=error */


/* CmdLineEvalFree() */
/* Free memory allocated by CmdLineEval() for variable length */
/* argument list. */

void CmdLineEvalFree (
  CmdLinePara *paraList);	/* in: parameter info list */
				/*     or NULL */


/* CmdLineHelp() */
/* Print help text about program usage including description of */
/* command line parameters and switches. */

void CmdLineHelp (
  char *progName,		/* in: program name */
				/*     or NULL */
  CmdLinePara *paraList,	/* in: parameter info list */
				/*     or NULL */
  CmdLineSwitch *switchList,	/* in: switch info list */
				/*     or NULL */
  FILE *outStream);		/* in: output stream */
				/*     (e.g. stdout) */


/* CmdLineParseString() */
/* Parse a copy of string into tokens separated by sepaChar. */
/* Resulting token list can be evaluated by CmdLineEval(). */

char **CmdLineParseString (
  char *string,			/* in: string to be parsed */
				/*     NOTE: string is not modified */
  char *sepaChar,		/* in: token separator characters */
  int *count);			/* out: number of tokens generated by parser */
				/*      (corresponds to argc) */
				/* returns: */
				/*  list of tokens generated by parser */
				/*  (corresponds to argv[]) */


/* CmdLineParseFile() */
/* Parse init file into tokens separated by sepaChar. */
/* Comments preceded by a commentSepaChar are ingnored. */
/* Resulting token list can be evaluated by CmdLineEval(). */

char **CmdLineParseFile (
  char *fileName,		/* in: file name of init file */
  char *sepaChar,		/* in: token separator characters */
  char *commentSepaChar,	/* in: comment separator characters */
  int *count);			/* out: number of tokens generated by parser */
				/*      (corresponds to argc) */
				/* returns: */
				/*  list of tokens generated by parser */
				/*  (corresponds to argv[]) */
				/*  or NULL if file error */


/* CmdLineParseFree() */
/* Free memory allocated by CmdLineParseString() or CmdLineParseFile(). */

void CmdLineParseFree (
  char **tokenList);		/* in: token list returned by */
				/*     CmdLineParseString() or */
				/*     CmdLineParseFile() */


/* ComposeFileName() */
/* Compose filename using default path and extension if required. */
/* Handles Unix & DOS paths. "-" is passed through directly. */

int ComposeFileName (
  char *inName,			/* in: input filename */
  int forceDefault,		/* in: 0=keep input path and/or extension if */
				/*       available, otherwise use default(s) */
				/*     1=force usage of default */
				/*       path and extension */
  char *defaultPath,		/* in: default path */
				/*     or NULL */
  char *defaultExt,		/* in: default extension */
				/*     or NULL */
  char *fileName,		/* out: composed filename */
  unsigned int fileNameMaxLen);	/* in: fileName max length */
				/* returns: */
				/*  0=OK  1=result too long */


#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _cmdline_h_ */

/* end of cmdline.h */

