/*
 * Win32 support routines for PhysicsFS.
 *
 * Please see the file LICENSE in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#if (defined __STRICT_ANSI__)
#define __PHYSFS_DOING_STRICT_ANSI__
#endif

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define __PHYSICSFS_INTERNAL__
#include "physfs_internal.h"


const char *__PHYSFS_platformDirSeparator = "\\";


static const char *win32strerror(void)
{
    static TCHAR msgbuf[255];

    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
        msgbuf,
        sizeof (msgbuf) / sizeof (TCHAR),
        NULL 
    );

    return((const char *) msgbuf);
} /* win32strerror */


char **__PHYSFS_platformDetectAvailableCDs(void)
{
    char **retval = (char **) malloc(sizeof (char *));
    int cd_count = 1;  /* We count the NULL entry. */
    char drive_str[4] = "x:\\";

    for (drive_str[0] = 'A'; drive_str[0] <= 'Z'; drive_str[0]++)
    {
        if (GetDriveType(drive_str) == DRIVE_CDROM)
        {
            char **tmp = realloc(retval, sizeof (char *) * cd_count + 1);
            if (tmp)
            {
                retval = tmp;
                retval[cd_count-1] = (char *) malloc(4);
                if (retval[cd_count-1])
                {
                    strcpy(retval[cd_count-1], drive_str);
                    cd_count++;
                } /* if */
            } /* if */
        } /* if */
    } /* for */

    retval[cd_count - 1] = NULL;
    return(retval);
} /* __PHYSFS_detectAvailableCDs */


char *__PHYSFS_platformCalcBaseDir(const char *argv0)
{
    DWORD buflen = 0;
    char *retval = NULL;
    char *filepart = NULL;

    if (strchr(argv0, '\\') != NULL)   /* default behaviour can handle this. */
        return(NULL);

    buflen = SearchPath(NULL, argv0, NULL, buflen, NULL, NULL);
    retval = (char *) malloc(buflen);
    BAIL_IF_MACRO(!retval, ERR_OUT_OF_MEMORY, NULL);
    SearchPath(NULL, argv0, NULL, buflen, retval, &filepart);
    return(retval);
} /* __PHYSFS_platformCalcBaseDir */


char *__PHYSFS_platformGetUserName(void)
{
    DWORD bufsize = 0;
    LPTSTR retval = NULL;

    if (GetUserName(NULL, &bufsize) == 0)  /* This SHOULD fail. */
    {
        retval = (LPTSTR) malloc(bufsize);
        BAIL_IF_MACRO(retval == NULL, ERR_OUT_OF_MEMORY, NULL);
        if (GetUserName(retval, &bufsize) == 0)  /* ?! */
        {
            free(retval);
            retval = NULL;
        } /* if */
    } /* if */

    return((char *) retval);
} /* __PHYSFS_platformGetUserName */


static char *copyEnvironmentVariable(const char *varname)
{
    const char *envr = getenv(varname);
    char *retval = NULL;

    if (envr != NULL)
    {
        retval = malloc(strlen(envr) + 1);
        if (retval != NULL)
            strcpy(retval, envr);
    } /* if */

    return(retval);
} /* copyEnvironmentVariable */


char *__PHYSFS_platformGetUserDir(void)
{
    char *home = copyEnvironmentVariable("HOME");
    const char *homedrive = getenv("HOMEDRIVE");
    const char *homepath = getenv("HOMEPATH");

    if (home != NULL)
        return(home);

    if ((homedrive != NULL) && (homepath != NULL))
    {
        char *retval = (char *) malloc(strlen(homedrive)+strlen(homepath)+2);
        if (retval != NULL)
        {
            strcpy(retval, homedrive);
            if ((homepath[0] != '\\') &&
                (homedrive[strlen(homedrive)-1] != '\\'))
            {
                strcat(retval, "\\");
            } /* if */
            strcat(retval, homepath);
            return(retval);
        } /* if */
    } /* if */

    return(NULL);  /* fall through to default rules. */
} /* __PHYSFS_platformGetUserDir */


int __PHYSFS_platformGetThreadID(void)
{
    return((int) GetCurrentThreadId());
} /* __PHYSFS_platformGetThreadID */


/* ...make this Cygwin AND Visual C friendly... */
int __PHYSFS_platformStricmp(const char *x, const char *y)
{
    int ux, uy;

    do
    {
        ux = toupper((int) *x);
        uy = toupper((int) *y);
        if (ux > uy)
            return(1);
        else if (ux < uy)
            return(-1);
        x++;
        y++;
    } while ((ux) && (uy));

    return(0);
} /* __PHYSFS_platformStricmp */


int __PHYSFS_platformExists(const char *fname)
{
    return(GetFileAttributes(fname) != 0xffffffff);
} /* __PHYSFS_platformExists */


int __PHYSFS_platformIsSymLink(const char *fname)
{
    return(0);  /* no symlinks on win32. */
} /* __PHYSFS_platformIsSymlink */


int __PHYSFS_platformIsDirectory(const char *fname)
{
    return((GetFileAttributes(fname) & FILE_ATTRIBUTE_DIRECTORY) != 0);
} /* __PHYSFS_platformIsDirectory */


char *__PHYSFS_platformCvtToDependent(const char *prepend,
                                      const char *dirName,
                                      const char *append)
{
    int len = ((prepend) ? strlen(prepend) : 0) +
              ((append) ? strlen(append) : 0) +
              strlen(dirName) + 1;
    char *retval = malloc(len);
    char *p;

    BAIL_IF_MACRO(retval == NULL, ERR_OUT_OF_MEMORY, NULL);

    if (prepend)
        strcpy(retval, prepend);
    else
        retval[0] = '\0';

    strcat(retval, dirName);

    if (append)
        strcat(retval, append);

    for (p = strchr(retval, '/'); p != NULL; p = strchr(p + 1, '/'))
        *p = '\\';

    return(retval);
} /* __PHYSFS_platformCvtToDependent */


/* Much like my college days, try to sleep for 10 milliseconds at a time... */
void __PHYSFS_platformTimeslice(void)
{
    Sleep(10);
} /* __PHYSFS_platformTimeslice */


LinkedStringList *__PHYSFS_platformEnumerateFiles(const char *dirname,
                                                  int omitSymLinks)
{
    LinkedStringList *retval = NULL;
    LinkedStringList *l = NULL;
    LinkedStringList *prev = NULL;
    HANDLE dir;
    WIN32_FIND_DATA ent;

    dir = FindFirstFile(dirname, &ent);
    BAIL_IF_MACRO(dir == INVALID_HANDLE_VALUE, win32strerror(), NULL);

    for (; FindNextFile(dir, &ent) != 0; )
    {
        if (strcmp(ent.cFileName, ".") == 0)
            continue;

        if (strcmp(ent.cFileName, "..") == 0)
            continue;

        l = (LinkedStringList *) malloc(sizeof (LinkedStringList));
        if (l == NULL)
            break;

        l->str = (char *) malloc(strlen(ent.cFileName) + 1);
        if (l->str == NULL)
        {
            free(l);
            break;
        } /* if */

        strcpy(l->str, ent.cFileName);

        if (retval == NULL)
            retval = l;
        else
            prev->next = l;

        prev = l;
        l->next = NULL;
    } /* while */

    FindClose(dir);
    return(retval);
} /* __PHYSFS_platformEnumerateFiles */


int __PHYSFS_platformFileLength(FILE *handle)
{
    fpos_t curpos;
    int retval;

    fgetpos(handle, &curpos);
    fseek(handle, 0, SEEK_END);
    retval = ftell(handle);
    fsetpos(handle, &curpos);
    return(retval);
} /* __PHYSFS_platformFileLength */


char *__PHYSFS_platformCurrentDir(void)
{
    LPTSTR retval;
    DWORD buflen = 0;

    buflen = GetCurrentDirectory(buflen, NULL);
    retval = (LPTSTR) malloc(buflen);
    GetCurrentDirectory(buflen, retval);
    return((char *) retval);
} /* __PHYSFS_platformCurrentDir */


char *__PHYSFS_platformRealPath(const char *path)
{
    /* !!! FIXME: This isn't supported on CygWin! */
    return(_fullpath(NULL, path, _MAX_PATH));
} /* __PHYSFS_platformRealPath */


int __PHYSFS_platformMkDir(const char *path)
{
    DWORD rc = CreateDirectory(path, NULL);
    BAIL_IF_MACRO(rc == 0, win32strerror(), 0);
    return(1);
} /* __PHYSFS_platformMkDir */

/* end of win32.c ... */
