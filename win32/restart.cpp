// restart.cpp : Defines the entry point for the console application.
//

#include <string.h>
#include <time.h>
#include <process.h>
#include <fstream.h>
#include <iostream.h>
#include <stdio.h>
#include <malloc.h>


#define RESTART_VERSION "Restart Version 2.2"

/////////////////////////////////////////////////////////////////////////////
// The one and only application object


int newrename(const char *oldfile, const char *newfile);
void UpdateStatus(char *update_string);
void create_ini();

FILE   *inifile, *dbinr, *dbout, *dbold, *deltas, *panic, *logfile, *dbinw, *mpanic;
   char      buf[ 2048 ];
   char      dbinf[ 2048 ];
   char      dboutf[ 2048 ];
   char      dboldf[ 2048 ];
   char      deltasf[ 2048 ];
   char      port[ 2048 ];
   char      logf[ 2048 ];
   char      garbage[ 2056 ];
   char      mpanicf[ 2056 ];
   char      holding[256];
   char      dbcheck[ 20 ];
   char      chrin;
   char      portstore[10];
   char		 *cmdline;
   char      *args[40];
   char      *portara[40];
   int       i;
   int       bufctr=0;
   int       count;
   int       nologging=0;
   int       strtot=0;
   int       cmdcount=0;


int main (int argc, char **argv) {
   time_t        st_t;
   struct tm     *st_tm;
   char*         DTarray[2];

   st_t  = time(0);
   st_tm = localtime(&st_t);
   bufctr=0;
   DTarray[0] = (char*)malloc(sizeof(char)*20);
   strftime(DTarray[0],15,"%m-%d-%Y",st_tm);
   DTarray[1] = (char*)malloc(sizeof(char)*20);
   strftime(DTarray[1],15,"%H:%M:%S",st_tm);

// Start the restart

   inifile = fopen("restart.ini", "r");
   if (!inifile) {
      printf("Error opening RESTART.INI. One will now be created for you with the default values.\n",stderr);
      (void) create_ini();
      inifile = fopen("restart.ini", "r");
      if (!inifile) {
         printf("There was an error creating the RESTART.INI file. Please obtain one from the author of this software.\nError Code: 1\n", stderr);
         exit(0);
      }
   }

// Get the log file from the INI file.

   do {
      chrin = fgetc(inifile);
      if ((char)chrin == '\n') {
         buf[ bufctr ] = '\0';
         break;
      } else {
         buf[ bufctr ] = (char)chrin;
      }
	  if (!chrin || feof(inifile)) {
         printf("There was an error reading RESTART.INI. It is most likely corrupted and will have to be replaced. Please delete the file in the muck directory and a new one will be created for you upon restart.\nError Code: 3\n",stderr);
         exit(0);
      }
      bufctr++;
      if (bufctr == 2048) break;
   } while (bufctr != 2048);
   bufctr=0;
   strcpy(logf, buf);
 
// Open the log file for writing.

   logfile = fopen(logf,"a+");
   if (!logfile) {
      printf("There was an error creating the log file for writing. Logging will not be enabled until you have changed the location of the log file.\nError Code:4\n",stderr);
      nologging=1;
   }

// Get the port number.

   do {
      chrin = fgetc(inifile);
      if ((char)chrin == '\n') {
         buf[ bufctr ] = '\0';
         break;
      } else {
         buf[ bufctr ] = (char)chrin;
      }
	  if (!chrin || feof(inifile)) {
         printf("There was an error reading RESTART.INI. It is most likely corrupted and will\nhave to be replaced. Please delete the file in the muck directory and a new\none will be created for you upon restart.\nError Code: 5\n",stderr);
         exit(0);
      }
      bufctr++;
      if (bufctr == 2048) break;
   } while (bufctr != 2048);
   bufctr=0;
   strcpy(port, buf);

// Get the DBIN file name.

   do {
      chrin = fgetc(inifile);
      if ((char)chrin == '\n') {
         buf[ bufctr ] = '\0';
         break;
      } else {
         buf[ bufctr ] = (char)chrin;
      }
	  if (!chrin || feof(inifile)) {
         printf("There was an error reading RESTART.INI. It is most likely corrupted and will\nhave to be replaced. Please delete the file in the muck directory and a new\none will be created for you upon restart.\nError Code: 6\n",stderr);
         exit(0);
      }
      bufctr++;
      if (bufctr == 2048) break;
   } while (bufctr != 2048);
   bufctr=0;
   strcpy(dbinf, buf);

// Get the DBOUT file name.

   do {
      chrin = fgetc(inifile);
      if ((char)chrin == '\n') {
         buf[ bufctr ] = '\0';
         break;
      } else {
         buf[ bufctr ] = (char)chrin;
      }
	  if (!chrin || feof(inifile)) {
         printf("There was an error reading RESTART.INI. It is most likely corrupted and will\nhave to be replaced. Please delete the file in the muck directory and a new\none will be created for you upon restart.\nError Code: 7\n",stderr);
         exit(0);
      }
      bufctr++;
      if (bufctr == 2048) break;
   } while (bufctr != 2048);
   bufctr=0;
   strcpy(dboutf, buf);

// Get the DBOLD file name.

   do {
      chrin = fgetc(inifile);
      if ((char)chrin == '\n') {
         buf[ bufctr ] = '\0';
         break;
      } else {
         buf[ bufctr ] = (char)chrin;
      }
	  if (!chrin || feof(inifile)) {
         printf("There was an error reading RESTART.INI. It is most likely corrupted and will\nhave to be replaced. Please delete the file in the muck directory and a new\none will be created for you upon restart.\nError Code: 8\n",stderr);
         exit(0);
      }
      bufctr++;
      if (bufctr == 2048) break;
   } while (bufctr != 2048);
   bufctr=0;
   strcpy(dboldf, buf);

// Get the DELTAS file name.

   do {
      chrin = fgetc(inifile);
      if ((char)chrin == '\n') {
         buf[ bufctr ] = '\0';
         break;
      } else {
         buf[ bufctr ] = (char)chrin;
      }
	  if (!chrin || feof(inifile)) {
         printf("There was an error reading RESTART.INI. It is most likely corrupted and will\nhave to be replaced. Please delete the file in the muck directory and a new\none will be created for you upon restart.\nError Code: 9\n",stderr);
         exit(0);
      }
      bufctr++;
      if (bufctr == 2048) break;
   } while (bufctr != 2048);
   bufctr=0;
   strcpy(deltasf, buf);

   fclose(inifile);

   if (strlen(dbinf) <= 2) {
           printf("The value of the DB input file is invalid.\nError Code: 10\n",stderr);
	   exit(1);
   }
   if (strlen(dboutf) <= 2) {
           printf("The value of the DB output file is invalid.\nError Code: 11\n",stderr);
	   exit(1);
   }
   if (strlen(dboldf) <= 2) {
           printf("The value of the DB old file is invalid.\nError Code: 12\n",stderr);
	   exit(1);
   }
   if (strlen(deltasf) <= 2) {
           printf("The value of the deltas file is invalid.\nError Code: 13\n",stderr);
	   exit(1);
   }
   if (strlen(logf) <= 2) {
           printf("The value of the log file is invalid.\nError Code: 14\n",stderr);
	   exit(1);
   }
   if (strlen(port) == 0) {
           printf("The value of the port address is invalid.\nError Code: 15\n",stderr);
	   exit(1);
   }

// Do some restart stuff

   sprintf(holding,"------------------STARTING SERVER: %s at %s------------------\n",DTarray[0],DTarray[1]);
   UpdateStatus(holding);
   UpdateStatus("Starting ");
   UpdateStatus(RESTART_VERSION);
   UpdateStatus("\n");
   if (!nologging) {
      fputs("------------------STARTING SERVER: ",logfile);
      fputs(DTarray[0],logfile);
      fputs(" at ",logfile);
      fputs(DTarray[1],logfile);
      fputs("------------------\n",logfile);
   }

// Test for a DBOLD file to backup.
   dbold = fopen(dboldf, "r");
   if(dbold)
   {
      fclose(dbold);
      strcpy(garbage,dboldf);
      strcat(garbage,".");
      strcat(garbage,DTarray[0]);
      sprintf(holding,"%s --> %s...",dboldf,garbage);
      UpdateStatus(holding);
      if (!nologging) {
         fputs(dboldf,logfile);
         fputs(" --> ",logfile);
         fputs(garbage, logfile);
         fputs("\n",logfile);
      }
      unlink(garbage);
      if (newrename(dboldf,garbage)) {
         sprintf(holding,"Failed!\n***ERROR: Cannot rename file. Restart aborted.\n");
         UpdateStatus(holding);
         if (!nologging) {
            fputs("ERROR: Cannot rename file. Restart aborted.\n",logfile);
            fclose(logfile);
         }
         return 0;
      }
      UpdateStatus("Successful.\n");
   }

// Test for a DBIN to rename to DBOLD.
   dbinr = fopen(dbinf, "r");
   if (dbinr) {
      dbout = fopen(dboutf, "r");
      if (dbout) {
         fclose(dbout);
         fclose(dbinr);
         sprintf(holding,"%s --> %s...",dbinf,dboldf);
         UpdateStatus(holding);
         if (!nologging) {
            fputs(dbinf,logfile);
            fputs(" --> ",logfile);
            fputs(dboldf, logfile);
            fputs("\n",logfile);
         }
         if (newrename(dbinf,dboldf)) {
            sprintf(holding, "Failed!\n*** ERROR: Cannot rename file. Restart aborted.\n");
            UpdateStatus(holding);
            if (!nologging) {
               fputs("ERROR: Cannot rename file. Restart aborted.\n",logfile);
               fclose(logfile);
            }
            fclose(dbinr);
            return 0;
         }
         UpdateStatus("Successful.\n");
      } else {
         fclose(dbinr);
      }
   }

   strcpy(garbage,dboutf);
   strcat(garbage,".PANIC");
   strcpy(mpanicf, ".\\muf\\macros.PANIC");

   panic = fopen(garbage,"r");
   mpanic = fopen(mpanicf, "r");
   if (panic) {
      fclose(panic);
      sprintf(holding,"PANIC file exists. Using this to restart.\n");
      UpdateStatus(holding);
      if (!nologging) {
         fputs("PANIC file exists. Using this to restart.\n",logfile);
      }
      sprintf(holding, "%s --> %s\n", garbage, dboutf);
      UpdateStatus(holding);
      if (!nologging) {
         fputs(garbage,logfile);
         fputs(" --> ",logfile);
         fputs(dboutf, logfile);
         fputs("\n",logfile);
      }

      if(newrename(garbage,dboutf)) {
         sprintf(holding, "***ERROR: Cannot rename file. Restart aborted.\n");
         UpdateStatus(holding);
         if (!nologging) {
            fputs("ERROR: Cannot rename file. Restart aborted.\n",logfile);
            fclose(logfile);
         }
         return 0;
      }
      (void) unlink(deltasf);
      if (mpanic) {
         fclose(mpanic);
         strcpy(garbage, ".\\muf\\macros.");
         strcpy(mpanicf, ".\\muf\\macros.PANIC");
         (void) unlink(garbage);
         sprintf(holding, "%s --> %s\n", mpanicf, garbage);
         UpdateStatus(holding);
         if (!nologging) {
            fputs(".\\muf\\macros.PANIC --> .\\muf\\macros\n",logfile);
         }

         if(newrename(mpanicf,garbage)) {
            sprintf(holding, "***ERROR: Cannot rename file. Restart aborted.\n");
            UpdateStatus(holding);
            if (!nologging) {
               fputs("ERROR: Cannot rename file. Restart aborted.\n",logfile);
               fclose(logfile);
            }
            return 0;
         }
      }
   }

//  Check and rename for DBOUT file.

   dbout = fopen(dboutf, "r");
   if (dbout) {
      fclose(dbout);
      sprintf(holding,"%s --> %s...", dboutf, dbinf);
      UpdateStatus(holding);
      if (!nologging) {
         fputs(dboutf,logfile);
         fputs(" --> ",logfile);
         fputs(dbinf, logfile);
         fputs("\n",logfile);
      }
      if (newrename(dboutf,dbinf)) {
         sprintf(holding, "Failed!\n***ERROR: Cannot rename file. Restart aborted.\n");
         UpdateStatus(holding);
         if (!nologging) {
            fputs("ERROR: Cannot rename file. Restart aborted.\n",logfile);
            fclose(logfile);
         }
         return 0;
      }
      UpdateStatus("Successful.\n");
   }

// Check for a DBIN file to load.

   sprintf(holding,"Verifying %s...", dbinf);
   UpdateStatus(holding);
   dbinr = fopen(dbinf, "rb");
   if (!dbinr) {
      UpdateStatus("Failed!\nNo input file. Restart aborted.\n");
      if (!nologging) {
         fputs("No input file. Restart Aborted.\n",logfile);
         fclose(logfile);
      }
      return 0;
   }

// Check to make sure it is a valid DB.

   fseek(dbinr,-18L, SEEK_END);
   do {
      chrin = fgetc(dbinr);
      buf[ bufctr ] = (char)chrin;
      bufctr++;
      if (bufctr == 17) break;
   } while (bufctr != 17 );
   strcpy(dbcheck, buf);
   bufctr=0;
   dbcheck[17] = '\0';

   if(strcmp(dbcheck,"***END OF DUMP***")) {
      UpdateStatus("Failed!\nInput file is corrupt.\n");
      if (!nologging) {
         fputs("Input file is corrupt. Restart aborted.\n",logfile);
         fclose(logfile);
      }
      fclose(dbinr);
      return 0;
   }

   fclose(dbinr);

   UpdateStatus("Successful.\n");

// Check for delta existance.

   deltas = fopen(deltasf,"r");
   if (deltas) {
      UpdateStatus("Found a delta. Attempting to merge.\n");
      fseek(deltas, -18L, SEEK_END);
      do {
         chrin = fgetc(deltas);
         buf[ bufctr ] = (char)chrin;
         bufctr++;
         if (bufctr == 17) break;
      } while (bufctr != 17 );
      bufctr=0;
      strcpy(dbcheck, buf);
      dbcheck[17] = '\0';

      if(strcmp(dbcheck,"***END OF DUMP***")) {
         UpdateStatus("Deltadump is incomplete. Discarding it.\n");
         if (!nologging) {
            fputs("Deltadump is incomplete. Discarding it.\n",logfile);
         }
         fclose(deltas);
         (void) unlink(deltasf);
      } else {
         dbinw = fopen(dbinf, "a+b");
         if (dbinw) {
            fseek(deltas, 0L, SEEK_SET);
            do {
               chrin = fgetc(deltas);
               if (!chrin || feof(deltas)) {
                  break;
               }
               fputc(chrin, dbinw);
            } while (chrin);
            fclose(deltas);
            fclose(dbinw);
         } else {
            UpdateStatus("Can't open database file to append to. Restart aborted.\n");
            if (!nologging) {
               fputs("Can't open database to append delta to. Aborting.\n",logfile);
               fclose(logfile);
            }
            fclose(deltas);
            return 0;
         }
      }
   }
   fclose(logfile);

// Setup the command line
   args[0]=(char *) malloc(2048);
   args[0]="fbmuck";
   cmdcount = 0;
   for (i = 1; i < argc; i++) {
      cmdcount++;
      args[cmdcount] = (char *) malloc(strlen(argv[i]));
      args[cmdcount] = argv[i];
   }
   cmdcount++;
   args[cmdcount]=(char *) malloc(strlen(dbinf));
   args[cmdcount]=dbinf;
   cmdcount++;
   args[cmdcount]=(char *) malloc(strlen(dboutf));
   args[cmdcount]=dboutf;
   do {
      cmdcount++;
      args[cmdcount] = (char *) malloc(6);
      i = 0;
      do {
         if (port[count] == 0) break;
         if (port[count] == '\n') break;
         if (port[count] == '\r') break;
         if (port[count] == ' ') break;
         args[cmdcount][i]=port[count];
         i++;
         count++;
      } while (1);
      args[cmdcount][i] = '\0';
      if (port[count] == 0) break;
      if (port[count] == '\n') break;
      if (port[count] == '\r') break;
      count++;
   } while (1) ;
   cmdcount++;
   args[cmdcount]=0;
   
   execv("fbmuck", args);

   printf("Error spawning server. Cannot restart.\n");

   return 1;

}

void create_ini() {
	inifile = fopen("restart.ini", "w");
	if(inifile)
		fputs(".\\logs\\restart.\n8888\ndata\\minimal.db\ndata\\minimal.out\ndata\\minimal.old\ndata\\deltas-file\n",inifile);
	fclose(inifile);
	return;
}

void UpdateStatus(char* update_string) {
    printf(update_string, stderr);
    return;
}

int newrename(const char *oldfile, const char *newfile){
	int result;

	result = rename(oldfile, newfile);
	if (result != 0) {
		(void) _unlink(newfile);
		return rename(oldfile,newfile);
	} else {
		return result;
	}
}

