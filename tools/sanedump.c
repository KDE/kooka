/************************************************************************/
/*									*/
/*  This file is part of the KDE Project				*/
/*  Copyright (c) 2004 Jonathan Marten <jjm@keelhaul.me.uk>		*/
/*									*/
/*  This program is free software; you can redistribute it and/or	*/
/*  modify it under the terms of the GNU General Public License as	*/
/*  published by the Free Software Foundation; either version 2 of	*/
/*  the License, or (at your option) any later version.			*/
/*									*/
/*  It is distributed in the hope that it will be useful, but		*/
/*  WITHOUT ANY WARRANTY; without even the implied warranty of		*/
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*/
/*  GNU General Public License for more details.			*/
/*									*/
/*  You should have received a copy of the GNU General Public		*/
/*  License along with this program; see the file COPYING for further	*/
/*  details.  If not, write to the Free Software Foundation, Inc.,	*/
/*  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.	*/
/*									*/
/************************************************************************/


/************************************************************************/
/*  Include files							*/
/************************************************************************/

#include <stdio.h>					/* Standard I/O library */
#include <unistd.h>					/* Unix(TM) system routines */
#include <string.h>					/* String handling routines */
#include <stdarg.h>					/* Variable parameter handling */
#include <errno.h>					/* Library error numbers */
#include <stdlib.h>					/* Standard library routines */

#include <sane/sane.h>					/* SANE scanner access */

/************************************************************************/
/*  Global variables							*/
/************************************************************************/

const char *myname;					/* my program name */

static int listmode = 0;				/* option 'L' -> probe and list */
static char devname[256] = "";				/* option 'd' -> device name */
static int longdesc = 0;				/* option 'l' -> long descriptions */
static int advopt = 0;					/* option 'a' -> advanced options */

SANE_Status sanerr;					/* SANE error status */

static const char *fmt = "%-14s  ";			/* format for output */

/************************************************************************/
/*  Message keys for cmderr() - sync with 'errstrings'			*/
/************************************************************************/

enum cemsg
{
	CEERROR		= 0,				/* exit */
	CENONE		= 1,				/* benign */
	CEWARNING	= 2,				/* continue */
	CENOTICE	= 3,				/* continue */
	CEINFO		= 4,				/* continue */
	CESUCCESS	= 5,				/* continue */
	CEFATAL		= 6,				/* exit */
	CEUNKNOWN	= 7				/* must be last */
};

/************************************************************************/
/*  Error source keys for cmderr()					*/
/************************************************************************/

enum cesys
{
	CSNONE		= 0,				/* no message */
	CSERRNO		= 1,				/* decode 'errno' */
	CSSANE		= 2				/* decode SANE status */
};

/************************************************************************/
/*  Severity strings for cmderr() reports				*/
/************************************************************************/

static const char *errstrings[] =
{
    "ERROR",						/* 0 */
    NULL,						/* 1 */
    "WARNING",						/* 2 */
    "NOTICE",						/* 3 */
    "INFO",						/* 4 */
    "SUCCESS",						/* 5 */
    "FATAL",						/* 6 */
    NULL						/* 7 */
};

/************************************************************************/
/*  cmderr -- Display an error message with parameters, optionally	*/
/*  adding the system or SANE error as well.				*/
/************************************************************************/

void 
#if defined(__GNUC__)
__attribute__((format (printf,3,4)))
#endif
cmderr(enum cesys src,enum cemsg sev,const char *fmtstr,...)
{
    va_list args;					/* parameters for format */
    const char *ss;					/* severity string */
    const char *mm = NULL;				/* textual error message*/

    if (src==CSERRNO) mm = strerror(errno);		/* system error message */
    else if (src==CSSANE) mm = sane_strstatus(sanerr);	/* SANE error message */

    fflush(stdout); fflush(stderr);			/* flush current output */
							/* catch bad parameter */
    if (((int) sev)<0 || sev>CEUNKNOWN) sev = CEUNKNOWN;
    ss = errstrings[sev];

    fprintf(stderr,"%s",myname);			/* report our program name */
    if (ss!=NULL) fprintf(stderr," (%s)",ss);		/* report severity if there is one */
    fprintf(stderr,": ");

    va_start(args,fmtstr);				/* locate variable parameters */
    vfprintf(stderr,fmtstr,args);			/* user message and parameters */
    if (mm!=NULL) fprintf(stderr,", %s",mm);		/* system error message */
    va_end(args);					/* finished with parameters */
    fprintf(stderr,"\n");				/* end of message */
							/* exit from application */
    if (sev==CEERROR || sev==CEFATAL) exit(EXIT_FAILURE);
}

/************************************************************************/
/*  usage -- Display the command line and option help.			*/
/************************************************************************/

void usage()
{
	fprintf(stderr,"Usage:    %s -L                  probe and list scanner devices\n",myname);
	fprintf(stderr,"          %s [-la] [-d] device   display information for scanner device\n",myname);
	fprintf(stderr,"\n");
	fprintf(stderr,"Options:  -d     specify SANE device (backend) name\n");
	fprintf(stderr,"          -l     display long SANE option description\n");
	fprintf(stderr,"          -a     display advanced SANE options\n");
	exit(2);
}

/************************************************************************/
/*  do_init - Initialise SANE and report its version.			*/
/************************************************************************/

void do_init()
{
	SANE_Int sanevers;

	sanerr = sane_init(&sanevers,NULL);
	if (sanerr!=SANE_STATUS_GOOD) cmderr(CSSANE,CEFATAL,"cannot initialise SANE");
	printf(fmt,"SANE version:");
	printf("%d.%d.%d\n",SANE_VERSION_MAJOR(sanevers),
	       SANE_VERSION_MINOR(sanevers),SANE_VERSION_BUILD(sanevers));
}

/************************************************************************/
/*  do_listmode -- List all SANE devices known.				*/
/************************************************************************/

void do_listmode()
{
	const SANE_Device **devlist;
	const SANE_Device *dev;
	int i;

	do_init();

	sanerr = sane_get_devices(&devlist,SANE_FALSE);	/* local and network */
	if (sanerr!=SANE_STATUS_GOOD) cmderr(CSSANE,CEFATAL,"SANE cannot get device list");

	printf("\n");
	for (i = 0; devlist[i]!=NULL; ++i)
	{
		dev = devlist[i];

		printf(fmt,"Device:",dev->name);
		printf(fmt,"Vendor:",dev->vendor);
		printf(fmt,"Model:",dev->model);
		printf(fmt,"Type:",dev->type);
		printf("\n");
	}

	if (i==0)
	{
		cmderr(CSNONE,CENOTICE,"No devices found");
		cmderr(CSNONE,CEINFO,"Try sane-find-scanner(1), or hp-probe(1) for HP devices");
	}
}

/************************************************************************/
/*  do_describe -- Given a SANE device, decode and display all of its	*/
/*  option descriptors.							*/
/************************************************************************/

void do_describe(const char *dev)
{
	SANE_Handle *hand;
	const SANE_Option_Descriptor *desc;
	SANE_Int numopt;
	int opt;
	double min,max,q;
	const SANE_String_Const *sp;
	const SANE_Word *wp;
	int nw;
	int i;


	do_init();

	sanerr = sane_open(dev,&hand);
	if (sanerr!=SANE_STATUS_GOOD) cmderr(CSSANE,CEFATAL,"SANE cannot open '%s'",dev);

	/* get option index 0 = number of options */
	sanerr = sane_control_option(hand,0,SANE_ACTION_GET_VALUE,&numopt,NULL);
	if (sanerr!=SANE_STATUS_GOOD) cmderr(CSSANE,CEFATAL,"SANE cannot get option 0");

	printf(fmt,"Found:");
	printf("%d options\n",numopt);

	for (opt = 1; opt<numopt; ++opt)
	{
		desc = sane_get_option_descriptor(hand,opt);
		if (desc==NULL)
		{
			cmderr(CSNONE,CEWARNING,"SANE option %d is not valid",opt);
			continue;
		}

		if (!advopt && (desc->cap & SANE_CAP_ADVANCED)) continue;

		printf("\n");
		printf(fmt,"Option:");
		printf("%d\n",opt);

		if (desc->name!=NULL && desc->name[0]!='\0')
		{
			printf(fmt,"Name:");
			printf("%s\n",desc->name);
		}

		if (desc->title!=NULL)
		{
			printf(fmt,"Title:");
			printf("\"%s\"\n",desc->title);
		}

		if (desc->desc!=NULL && desc->desc[0]!='\0' && longdesc)
		{
			printf(fmt,"Description:");
			printf("\"%s\"\n",desc->desc);
		}

		printf(fmt,"Value type:");
		switch (desc->type)
		{
case SANE_TYPE_BOOL:	printf("BOOL");			break;
case SANE_TYPE_INT:	printf("INT");			break;
case SANE_TYPE_FIXED:	printf("FIXED");		break;
case SANE_TYPE_STRING:	printf("STRING");		break;
case SANE_TYPE_BUTTON:	printf("BUTTON");		break;
case SANE_TYPE_GROUP:	printf("GROUP");		break;
default:		printf("%d",desc->type);	break;
		}
		printf("\n");

		if (desc->unit!=SANE_UNIT_NONE)
		{
			printf(fmt,"Unit:");
			switch (desc->unit)
			{
case SANE_UNIT_NONE:		printf("NONE");			break;
case SANE_UNIT_PIXEL:		printf("PIXELS");		break;
case SANE_UNIT_BIT:		printf("BITS");			break;
case SANE_UNIT_MM:		printf("MM");			break;
case SANE_UNIT_DPI:		printf("DPI");			break;
case SANE_UNIT_PERCENT:		printf("PERCENT");		break;
case SANE_UNIT_MICROSECOND:	printf("MICROSECOND");		break;
default:			printf("%d",desc->unit);	break;
			}
			printf("\n");
		}

		if (desc->cap!=0)
		{
			printf(fmt,"Capabilities:");
			if (desc->cap & SANE_CAP_SOFT_SELECT) printf("SOFT_SELECT ");
			if (desc->cap & SANE_CAP_HARD_SELECT) printf("HARD_SELECT ");
			if (desc->cap & SANE_CAP_SOFT_DETECT) printf("SOFT_DETECT ");
			if (desc->cap & SANE_CAP_EMULATED) printf("EMULATED ");
			if (desc->cap & SANE_CAP_AUTOMATIC) printf("AUTOMATIC ");
			if (desc->cap & SANE_CAP_INACTIVE) printf("INACTIVE ");
			if (desc->cap & SANE_CAP_ADVANCED) printf("ADVANCED ");
#ifdef SANE_CAP_ALWAYS_SETTABLE
			if (desc->cap & SANE_CAP_ALWAYS_SETTABLE) printf("ALWAYS_SETTABLE ");
#endif
			printf("\n");
		}

		if (desc->constraint_type!=SANE_CONSTRAINT_NONE)
		{
			printf(fmt,"Constraint:");
			switch (desc->constraint_type)
			{
case SANE_CONSTRAINT_NONE:	printf("NONE");
				break;

case SANE_CONSTRAINT_RANGE:	switch (desc->type)
				{
case SANE_TYPE_INT:			min = desc->constraint.range->min;
					max = desc->constraint.range->max;
					q = desc->constraint.range->quant;
					break;

case SANE_TYPE_FIXED:			min = SANE_UNFIX(desc->constraint.range->min);
					max = SANE_UNFIX(desc->constraint.range->max);
					q = SANE_UNFIX(desc->constraint.range->quant);
					break;

default:				min = max = q = 0;
					break;
				}

				if (q==0) q = 1.0;
				if (q==1.0)
				{
					printf("RANGE %g - %g",min,max);
				}
				else
				{
					printf("RANGE %g - %g by %g = %g - %g",
					       min,max,q,min/q,max/q);
				}
				break;

case SANE_CONSTRAINT_STRING_LIST:
				printf("STRING_LIST [");
				for (sp = desc->constraint.string_list; *sp!=NULL; ++sp)
				{
					printf("\"%s\"",*sp);
					if (sp[1]!=NULL) printf(" ");
				}
				printf("]");
				break;

case SANE_CONSTRAINT_WORD_LIST:
				printf("WORD_LIST ");
				wp = desc->constraint.word_list;
				nw = *wp++;
				printf("%d [",nw);
				for (i = 0; i<nw; ++i)
				{
					if (desc->type==SANE_TYPE_FIXED) printf("%g",SANE_UNFIX(*wp));
					else printf("%d",*wp);
					++wp;
					if ((i+1)!=nw) printf(" ");
				}
				printf("]");
				break;

default:			printf("%d",desc->constraint_type);
				break;
			}

			printf("\n");
		}

		if (desc->size>0 && desc->size!=sizeof(SANE_Word))
		{
			printf(fmt,"Data size:");
			printf("%d\n",desc->size);
		}
	}

	printf("\n");
	sane_close(hand);
	sane_exit();
}

/************************************************************************/
/*  Main -- Handle command line and options			 	*/
/************************************************************************/

int main(int argc, char **argv)
{
	int opt;
	int i;

	myname = strrchr(argv[0],'/');			/* extract our program name */
	if (myname==NULL) myname = argv[0];
	else ++myname;

	while ((opt = getopt(argc,argv,":halLd:"))!=-1)
	{
		switch (opt)
		{
case 'L':		listmode = 1;			/* probe and list devices */
			break;

case 'd':		strcpy(devname,optarg);		/* specify device name */
			break;

case 'l':		longdesc = 1;			/* show long descriptions */
			break;

case 'a':		advopt = 1;			/* show advanced options */
			break;

case 'h':		usage();			/* display help */
			break;

default:
case '?':		cmderr(CSNONE,CEERROR,"Unrecognised option '%c' (use '-h' for help)",optopt);
			break;

case ':':		cmderr(CSNONE,CEERROR,"Option '%c' requires an argument (use '-h' for help)",optopt);
			break;
		}
	}

	argc -= optind;					/* adjust for parsed options */
	argv += optind;

	if (listmode)
	{
		if (longdesc || advopt) cmderr(CSNONE,CEWARNING,"Options '-l'/-a' igored in listing mode ('-L')");
	}

	if (devname[0]=='\0')
	{
		if (!listmode)
		{
			if (argc==0) cmderr(CSNONE,CEERROR,"No options or devices specified (use '-h' for help)");
			for (i = 0; argv[i]!=NULL; ++i) do_describe(argv[i]);
		}
		else
		{
			if (argc>0) cmderr(CSNONE,CEWARNING,"Arguments ignored for listing mode ('-L')");
			do_listmode();
		}
	}
	else
	{
		if (listmode) cmderr(CSNONE,CEERROR,"Only one of the '-d'/'-L' options may be specified");
		if (argc>0) cmderr(CSNONE,CEWARNING,"Additional arguments with '-d' ignored");
		do_describe(devname);
	}

	return (EXIT_SUCCESS);
}
