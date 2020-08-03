/**
 * @file    fps_save2disk.c
 * @brief   Save FPS content to disk
 */


#include <sys/stat.h> // fstat
#include <sys/syscall.h> // needed for tid = syscall(SYS_gettid);


#include "CommandLineInterface/CLIcore.h"
#include "COREMOD_tools/timeutils.h"
#include "COREMOD_iofits/COREMOD_iofits.h"

#include "fps_GetParamIndex.h"
#include "fps_printparameter_valuestring.h"
#include "fps_WriteParameterToDisk.h"





int functionparameter_SaveParam2disk(
    FUNCTION_PARAMETER_STRUCT *fpsentry,
    const char *paramname
)
{
    int pindex;

    pindex = functionparameter_GetParamIndex(fpsentry, paramname);
    functionparameter_WriteParameterToDisk(fpsentry, pindex, "setval",
                                           "SaveParam2disk");

    return RETURN_SUCCESS;
}







int functionparameter_SaveFPS2disk_dir(
    FUNCTION_PARAMETER_STRUCT *fpsentry,
    char *dirname
)
{
    char fname[STRINGMAXLEN_FULLFILENAME];
    FILE *fpoutval;
    int stringmaxlen = 500;
    char outfpstring[stringmaxlen];


    struct stat st = {0};
    if (stat(dirname, &st) == -1) {
        mkdir(dirname, 0700);
    }


    sprintf(fname, "%s/%s.fps", dirname, fpsentry->md->name);
    fpoutval = fopen(fname, "w");

    pid_t tid;
    tid = syscall(SYS_gettid);

    // Get GMT time
    char timestring[200];
    struct timespec tnow;
    time_t now;

    clock_gettime(CLOCK_REALTIME, &tnow);
    now = tnow.tv_sec;
    struct tm *uttime;
    uttime = gmtime(&now);

	
    sprintf(timestring, "%04d%02d%02dT%02d%02d%02d.%09ld",
            1900 + uttime->tm_year, 1 + uttime->tm_mon, uttime->tm_mday, uttime->tm_hour,
            uttime->tm_min,  uttime->tm_sec, tnow.tv_nsec);

	
    fprintf(fpoutval, "# TIMESTRING %s\n", timestring);
    fprintf(fpoutval, "# PID        %d\n", getpid());
    fprintf(fpoutval, "# TID        %d\n", (int) tid);
    fprintf(fpoutval, "#\n");

    for ( int pindex = 0; pindex < fpsentry->md->NBparamMAX; pindex++)
    {
        errno_t ret = functionparameter_PrintParameter_ValueString(&fpsentry->parray[pindex], outfpstring, stringmaxlen);
        if(ret == RETURN_SUCCESS)
            fprintf(fpoutval, "%s\n", outfpstring);

    }
    fclose(fpoutval);


    return RETURN_SUCCESS;
}



/** @brief save entire FPS to disk
 * 
 * Writes in subdirectory fpslog
 */
int functionparameter_SaveFPS2disk(
    FUNCTION_PARAMETER_STRUCT *fps
)
{
	char outdir[STRINGMAXLEN_DIRNAME];
	WRITE_DIRNAME(outdir, "%s", fps->md->datadir);
	functionparameter_SaveFPS2disk_dir(fps, outdir);
	
	char timestring[100];
    char timestringnow[100];
    
    // assemble timestrings
    mkUTtimestring_microsec(timestring, fps->md->runpidstarttime);
    mkUTtimestring_microsec_now(timestringnow);	
	
	
	char ffname[STRINGMAXLEN_FULLFILENAME];
    FILE *fpout;
    WRITE_FULLFILENAME(ffname, "%s/%s.outlog", fps->md->datadir, fps->md->name);
    fpout = fopen(ffname, "w");
    fprintf(fpout, "%s %s %s fps\n", timestring, timestringnow, fps->md->name);
    fclose(fpout);
    	
	
	return RETURN_SUCCESS;
}




/** @brief Write archive script to .log2fps entry
 *
 * Funciton to be executed inside fps->md->datadir.
 * 
 * Writes a script to be executed to archive most recent data
 * in ../datadir/ (usually a sym link)
 *
 * takes fps as input
 * 
 * Optional input:
 * 
 * File loglist.dat in directory .conf.dirname
 * 
 */
errno_t	functionparameter_write_archivescript(
    FUNCTION_PARAMETER_STRUCT *fps
)
{	
    // Write archive script
    // to be executed to archive most recent calibration data
    // takes fpsname as input
    //
    FILE *fplogscript;
    char ffname[STRINGMAXLEN_FULLFILENAME];
    
    char datadirname[STRINGMAXLEN_DIRNAME];


	char timestring[FUNCTION_PARAMETER_STRMAXLEN];
	strncpy(timestring, functionparameter_GetParamPtr_STRING(fps, ".conf.timestring"), FUNCTION_PARAMETER_STRMAXLEN);    
    
	

    WRITE_FULLFILENAME(ffname, "archlogscript.bash");

    fplogscript = fopen(ffname, "w");
    fprintf(fplogscript, "#!/bin/bash\n");
    fprintf(fplogscript, "\n");
    fprintf(fplogscript, "# %s fps.%s.dat\n", timestring, fps->md->name);

    char datestring[9];
    strncpy(datestring, timestring, 8);
    datestring[8] = '\0';

    // save FPS
    WRITE_DIRNAME(datadirname, "../datadir/%s/%s/fps.%s", datestring, fps->md->name, fps->md->name);
    fprintf(fplogscript, "mkdir -p %s\n", datadirname);
    fprintf(fplogscript, "cp fps.%s.dat %s/fps.%s.%s.dat\n", fps->md->name, datadirname, fps->md->name, timestring);

    // save files listed in loglist.dat
    FILE *fploglist;
    char loglistfname[STRINGMAXLEN_FULLFILENAME];
    WRITE_FULLFILENAME(loglistfname, "loglist.dat");
    fploglist = fopen(loglistfname, "r");
    if (fploglist != NULL)
    {
        char *line = NULL;
        size_t llen = 0;
        char logfname[STRINGMAXLEN_FILENAME];

        while(getline(&line, &llen, fploglist) != -1) {
            sscanf(line, "%s", logfname);
            WRITE_DIRNAME(datadirname, "../datadir/%s/%s/%s", datestring, fps->md->name, logfname);
            fprintf(fplogscript, "mkdir -p %s\n", datadirname);
            fprintf(fplogscript, "cp -r %s %s/%s.%s\n", logfname, datadirname, logfname, timestring);
        }
        fclose(fploglist);
    }

    fclose(fplogscript);
    chmod(ffname, S_IRWXU | S_IRWXG  | S_IROTH );

//    functionparameter_SetParamValue_STRING(fps, ".conf.archivescript", ffname);
    
    return RETURN_SUCCESS;
}





/** @brief Save image as FITS 
 * 
 * Standard function to save output of FPS RUN function.
 * 
 * imagename is the in-memory image to be saved to disk, written as
 * outname.fits.
 * 
 */
errno_t fps_write_RUNoutput_image(
	FUNCTION_PARAMETER_STRUCT *fps,
	const char *imagename,
	const char *outname
)
{
	char ffname[STRINGMAXLEN_FULLFILENAME];
	char timestring[100];
    char timestringnow[100];
    
    // assemble timestrings
    mkUTtimestring_microsec(timestring, fps->md->runpidstarttime);
    mkUTtimestring_microsec_now(timestringnow);
    
    WRITE_FULLFILENAME(ffname, "!%s/%s.fits", fps->md->datadir, outname);
    save_fits(imagename, ffname);
    
    FILE *fpout;
    WRITE_FULLFILENAME(ffname, "%s/%s.outlog", fps->md->datadir, outname);
    fpout = fopen(ffname, "w");
    fprintf(fpout, "%s %s %s fits\n", timestring, timestringnow, outname);
    fclose(fpout);
    
	
	return RETURN_SUCCESS;
}




/** @brief Save text file 
 * 
 * Standard function to save output of FPS RUN function.
 * 
 * 
 */
FILE *fps_write_RUNoutput_file(
	FUNCTION_PARAMETER_STRUCT *fps,
	const char *filename,
	const char *extension
)
{
	FILE *fp;
	
	char ffname[STRINGMAXLEN_FULLFILENAME];
	char timestring[100];
    char timestringnow[100];
    
    // assemble timestrings
    mkUTtimestring_microsec(timestring, fps->md->runpidstarttime);
    mkUTtimestring_microsec_now(timestringnow);
    
    WRITE_FULLFILENAME(ffname, "%s/%s.%s", fps->md->datadir, filename, extension);
    fp = fopen(ffname, "w");
    
    FILE *fpout;
    WRITE_FULLFILENAME(ffname, "%s/%s.outlog", fps->md->datadir, filename);
    fpout = fopen(ffname, "w");
    fprintf(fpout, "%s %s %s %s\n", timestring, timestringnow, filename, extension);
    fclose(fpout);
    
	
	return fp;
}
