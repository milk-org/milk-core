/**
 * @file CLIcore_UI.c
 *
 * @brief User input (UI) functions
 *
 */



#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>


#include "CommandLineInterface/CLIcore.h"
#include "CommandLineInterface/calc.h"
#include "CommandLineInterface/calc_bison.h"


#include "COREMOD_memory/COREMOD_memory.h"





extern void yy_scan_string(const char *);
extern int yylex_destroy(void);





/**
 * @brief Readline callback
 *
 **/
void rl_cb_linehandler(char *linein)
{
    if(NULL == linein)
    {
        return;
    }

    data.CLIexecuteCMDready = 1;

    // copy input into data.CLIcmdline
    strcpy(data.CLIcmdline, linein);

    CLI_execute_line();

    free(linein);
}





errno_t runCLI_prompt(
    char *promptstring,
    char *prompt
)
{
	int color_cyan = 36;


    if(data.quiet == 0) {

        if(strlen(promptstring) > 0)
        {
            if(data.processnameflag == 0)
            {
                sprintf(prompt, "%c[%d;%dm%s >%c[%dm ", 0x1B, 1, color_cyan, promptstring, 0x1B, 0);
            }
            else
            {
                sprintf(prompt, "%c[%d;%dm%s-%s >%c[%dm ", 0x1B, 1, color_cyan, promptstring,
                        data.processname, 0x1B, 0);
            }
        }
        else
        {
            sprintf(prompt, "%c[%d;%dm%s >%c[%dm ", 0x1B, 1, color_cyan, data.processname, 0x1B, 0);
        }
    }
    else
    {
		sprintf(prompt," ");
    }



    return RETURN_SUCCESS;
}









static void *xmalloc(int size)
{
    void *buf;

    buf = malloc(size);
    if(!buf)
    {
        fprintf(stderr, "Error: Out of memory. Exiting.'n");
        exit(1);
    }

    return buf;
}



static char *dupstr(char *s)
{
    char *r;

    r = (char *) xmalloc((strlen(s) + 1));
    strcpy(r, s);
    return (r);
}




static char *CLI_generator(
    const char *text,
    int         state
)
{
    static unsigned int list_index;
    static unsigned int list_index1;
    static unsigned int len;
    char      *name;

	//printf("[generator %d %d %d]\n", state, data.CLImatchMode, list_index);

    if(!state)
    {
        list_index = 0;
        list_index1 = 0;
        len = strlen(text);
    }

    if(data.CLImatchMode == 0)
        while(list_index < data.NBcmd)
        {
            name = data.cmd[list_index].key;
            list_index++;
            if(strncmp(name, text, len) == 0)
            {
                return (dupstr(name));
            }
        }

    while(list_index1 < data.NB_MAX_IMAGE)
    {
        int iok;
        iok = data.image[list_index1].used;
        if(iok == 1)
        {
            name = data.image[list_index1].name;
            //	  printf("  name %d = %s %s\n", list_index1, data.image[list_index1].name, name);
        }
        list_index1++;
        if(iok == 1)
        {
            if(strncmp(name, text, len) == 0)
            {
                return (dupstr(name));
            }
        }
    }
    return ((char *)NULL);

}




/** @brief readline custom completion
 *
 * Invoked when pressing TAB
 */

char **CLI_completion(
    const char *text,
    int start,
    int __attribute__((unused)) end
)
{
    char **matches;

	//printf("[%d %s]", start, rl_line_buffer);
	//rl_message("[%d %s]", start, rl_line_buffer);
	//rl_redisplay();
	//rl_forced_update_display();

    matches = (char **)NULL;

    if(start == 0)
    {
        data.CLImatchMode = 0;    // try to match string with command name
    }
    else
    {
        data.CLImatchMode = 1;    // do not try to match with command
    }

    matches = rl_completion_matches((char *)text, &CLI_generator);

    //    else
    //  rl_bind_key('\t',rl_abort);

    return (matches);

}







errno_t CLI_execute_line()
{
    long    i;
    char   *cmdargstring;
    char    str[200];
    FILE   *fp;
    time_t  t;
    struct  tm *uttime;
    struct  timespec *thetime = (struct timespec *)malloc(sizeof(struct timespec));



	add_history(data.CLIcmdline);

    //
    // If line starts with !, use system()
    //
    if(data.CLIcmdline[0] == '!')
    {
        data.CLIcmdline[0] = ' ';
        if(system(data.CLIcmdline) != 0)
        {
            PRINT_ERROR("system call error");
            exit(4);
        }
        data.CMDexecuted = 1;
    }
    else if(data.CLIcmdline[0] == '#')
    {
        // do nothing... this is a comment
        data.CMDexecuted = 1;
    }
    else
    {
        // some initialization
        data.parseerror = 0;
        data.calctmp_imindex = 0;
        for(i = 0; i < NB_ARG_MAX; i++)
        {
            data.cmdargtoken[0].type = CMDARGTOKEN_TYPE_UNSOLVED;
        }


        // log command if CLIlogON active
        if(data.CLIlogON == 1)
        {
            t = time(NULL);
            uttime = gmtime(&t);
            clock_gettime(CLOCK_REALTIME, thetime);

            sprintf(data.CLIlogname,
                    "%s/logdir/%04d%02d%02d/%04d%02d%02d_CLI-%s.log",
                    getenv("HOME"),
                    1900 + uttime->tm_year,
                    1 + uttime->tm_mon,
                    uttime->tm_mday,
                    1900 + uttime->tm_year,
                    1 + uttime->tm_mon,
                    uttime->tm_mday,
                    data.processname);

            fp = fopen(data.CLIlogname, "a");
            if(fp == NULL)
            {
                printf("ERROR: cannot log into file %s\n", data.CLIlogname);
                EXECUTE_SYSTEM_COMMAND(
                    "mkdir -p %s/logdir/%04d%02d%02d\n",
                    getenv("HOME"),
                    1900 + uttime->tm_year,
                    1 + uttime->tm_mon,
                    uttime->tm_mday);
            }
            else
            {
                fprintf(fp,
                        "%04d/%02d/%02d %02d:%02d:%02d.%09ld %10s %6ld %s\n",
                        1900 + uttime->tm_year,
                        1 + uttime->tm_mon,
                        uttime->tm_mday,
                        uttime->tm_hour,
                        uttime->tm_min,
                        uttime->tm_sec,
                        thetime->tv_nsec,
                        data.processname,
                        (long) getpid(),
                        data.CLIcmdline);
                fclose(fp);
            }
        }



        //
        data.cmdNBarg = 0;
        // extract first word
        cmdargstring = strtok(data.CLIcmdline, " ");

        while(cmdargstring != NULL)   // iterate on words
        {
            // always copy word in string, so that arg can be processed as string if needed
            sprintf(data.cmdargtoken[data.cmdNBarg].val.string, "%s", cmdargstring);
            //printf("PROCESSING WORD \"%s\"  -> \"%s\"\n", cmdargstring, data.cmdargtoken[data.cmdNBarg].val.string);

            if((cmdargstring[0] == '\"')
                    && (cmdargstring[strlen(cmdargstring) - 1] == '\"'))
            {
                // if within quotes, store as raw string
                unsigned int stri;
                for(stri = 0; stri < strlen(cmdargstring) - 2; stri++)
                {
                    cmdargstring[stri] = cmdargstring[stri + 1];
                }
                cmdargstring[stri] = '\0';
                printf("%s\n", cmdargstring);
                data.cmdargtoken[data.cmdNBarg].type = CMDARGTOKEN_TYPE_RAWSTRING;
                sprintf(data.cmdargtoken[data.cmdNBarg].val.string, "%s", cmdargstring);
            }
            else     // otherwise, process it
            {
                sprintf(str, "%s\n", cmdargstring);
                yy_scan_string(str);
                data.calctmp_imindex = 0;
                yyparse();
                yylex_destroy();
            }

            cmdargstring = strtok(NULL, " ");
            data.cmdNBarg++;
        }
        data.cmdargtoken[data.cmdNBarg].type = CMDARGTOKEN_TYPE_UNSOLVED;


        i = 0;
        if(data.Debug == 1)
        {
            while(data.cmdargtoken[i].type != 0)
            {

                printf("TOKEN %ld/%ld   \"%s\"  type : %d\n", i, data.cmdNBarg, data.cmdargtoken[i].val.string, data.cmdargtoken[i].type);
                if(data.cmdargtoken[i].type == CMDARGTOKEN_TYPE_FLOAT)   // double
                {
                    printf("\t double : %g\n", data.cmdargtoken[i].val.numf);
                }
                if(data.cmdargtoken[i].type == CMDARGTOKEN_TYPE_LONG)   // long
                {
                    printf("\t long   : %ld\n", data.cmdargtoken[i].val.numl);
                }
                if(data.cmdargtoken[i].type == CMDARGTOKEN_TYPE_STRING)   // new variable/image
                {
                    printf("\t string : %s\n", data.cmdargtoken[i].val.string);
                }
                if(data.cmdargtoken[i].type == CMDARGTOKEN_TYPE_EXISTINGIMAGE)   // existing image
                {
                    printf("\t string : %s\n", data.cmdargtoken[i].val.string);
                }
                if(data.cmdargtoken[i].type == CMDARGTOKEN_TYPE_COMMAND)   // command
                {
                    printf("\t string : %s\n", data.cmdargtoken[i].val.string);
                }
                if(data.cmdargtoken[i].type == CMDARGTOKEN_TYPE_RAWSTRING)   // unprocessed string
                {
                    printf("\t string : %s\n", data.cmdargtoken[i].val.string);
                }

                i++;
            }
        }

        if(data.parseerror == 0)
        {
            if(data.cmdargtoken[0].type == CMDARGTOKEN_TYPE_COMMAND)
            {
                if(data.Debug == 1)
                {
                    printf("EXECUTING COMMAND %ld (%s)\n", data.cmdindex,
                           data.cmd[data.cmdindex].key);
                }
                data.cmd[data.cmdindex].fp();
                data.CMDexecuted = 1;
            }
        }

        for(i = 0; i < data.calctmp_imindex; i++)
        {
            CREATE_IMAGENAME(calctmpimname, "_tmpcalc%ld", i);
            //sprintf(calctmpimname, "_tmpcalc%ld", i);
            if(image_ID(calctmpimname) != -1)
            {
                if(data.Debug == 1)
                {
                    printf("Deleting %s\n", calctmpimname);
                }
                delete_image_ID(calctmpimname);
            }
        }


        if(!((data.cmdargtoken[0].type == CMDARGTOKEN_TYPE_STRING) || (data.cmdargtoken[0].type == CMDARGTOKEN_TYPE_RAWSTRING)))
        {
            data.CMDexecuted = 1;
        }

    }

    if((data.CMDexecuted == 0) && (data.CLIloopON == 1))
    {
        printf("Command not found, or command with no effect\n");
    }


    free(thetime);

    return RETURN_SUCCESS;
}
