/**
 * @file    read_shmim.c
 * @brief   read shared memory stream
 */

#include <sys/stat.h>
#include <fcntl.h> // open
#include <unistd.h> // close
#include <sys/mman.h> // mmap

#include "CommandLineInterface/CLIcore.h"
#include "image_ID.h"
#include "list_image.h"
#include "image_keyword_list.h"



// Local variables pointers
static char *insname;


// List of arguments to function
static CLICMDARGDEF farg[] =
{
    {
        CLIARG_STR_NOT_IMG, ".in_sname", "input stream", "ims1",
        CLIARG_VISIBLE_DEFAULT,
        (void **) &insname
    }
};




// flag CLICMDFLAG_FPS enabled FPS capability
static CLICMDDATA CLIcmddata =
{
    "readshmim",
    "read shared memory image",
    CLICMD_FIELDS_DEFAULTS
};




// detailed help
static errno_t help_function()
{
    return RETURN_SUCCESS;
}



imageID read_sharedmem_image(
    const char *sname
)
{
    imageID ID = -1;
    imageID IDmem = 0;
    IMAGE *image;

    DEBUG_TRACEPOINT("looking for next ID");
    IDmem = next_avail_image_ID();
    DEBUG_TRACEPOINT("Next ID = %ld", IDmem);

    image = &data.image[IDmem];

    if(ImageStreamIO_read_sharedmem_image_toIMAGE(sname, image) != IMAGESTREAMIO_SUCCESS)
    {
        printf("read shared mem image failed -> ID = -1\n");
        fflush(stdout);
        ID = -1;
    }
    else
    {
        IMGID img = makeIMGID(sname);
        //DEBUG_TRACEPOINT("resolving image");
        //ID = resolveIMGID(&img, ERRMODE_ABORT);
        img.ID = IDmem;

        //ID = image_ID(sname);
        printf("read shared mem image success -> ID = %ld\n", img.ID);
        fflush(stdout);

        image_keywords_list(img);
    }

    if(data.MEM_MONITOR == 1)
    {
        list_image_ID_ncurses();
    }

    return ID;
}



// adding INSERT_STD_PROCINFO statements enables processinfo support
static errno_t compute_function()
{
    INSERT_STD_PROCINFO_COMPUTEFUNC_START

    read_sharedmem_image(insname);

    INSERT_STD_PROCINFO_COMPUTEFUNC_END

    return RETURN_SUCCESS;
}





INSERT_STD_FPSCLIfunctions

errno_t CLIADDCMD_COREMOD_memory__read_sharedmem_image()
{
    INSERT_STD_CLIREGISTERFUNC
    return RETURN_SUCCESS;
}



