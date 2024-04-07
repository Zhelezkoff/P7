////////////////////////////////////////////////////////////////////////////////
//                                                                             /
// 2012-2020 (c) Baical                                                        /
//                                                                             /
// This library is free software; you can redistribute it and/or               /
// modify it under the terms of the GNU Lesser General Public                  /
// License as published by the Free Software Foundation; either                /
// version 3.0 of the License, or (at your option) any later version.          /
//                                                                             /
// This library is distributed in the hope that it will be useful,             /
// but WITHOUT ANY WARRANTY; without even the implied warranty of              /
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU           /
// Lesser General Public License for more details.                             /
//                                                                             /
// You should have received a copy of the GNU Lesser General Public            /
// License along with this library.                                            /
//                                                                             /
////////////////////////////////////////////////////////////////////////////////
#ifndef PPROCESS_H
#define PPROCESS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <stdint.h>
#include <unix.h>
#include <sys/procfs.h>
#include <libgen.h>
#include <sys/debug.h>

class CProc
{
public:
    ///////////////////////////////////////////////////////////////////////////////
    //Get_ArgV
    static tXCHAR **Get_ArgV(tINT32 *o_pCount)
    {
        const tUINT16 l_wLen    = 4096;
        int           l_pFile   = -1;
        tUINT16       pathLength = l_wLen;
    	procfs_info   pinfo;
    	tINT32        argc;
    	tINT32        i, l;
    	tXCHAR       *argv;
    	tXCHAR        l_pCmdLine[l_wLen];
    	tXCHAR       *p = l_pCmdLine;

        if (NULL == o_pCount)
        {
            goto l_lblExit;
        }

        l_pFile = open("/proc/self/as", O_RDONLY);
        if (-1 == l_pFile)
        {
            goto l_lblExit;
        }

    	if (EOK != devctl(l_pFile, DCMD_PROC_INFO, &pinfo, sizeof(pinfo), NULL))
    	{
            goto l_lblExit;
    	}

    	//calculate count of items//////////////////////////////////////////////////
    	if (lseek(l_pFile, pinfo.initial_stack, SEEK_SET) == -1)
    	{
            goto l_lblExit;
    	}

    	if (read(l_pFile, &argc, sizeof(argc)) < (int)sizeof(argc))
    	{
            goto l_lblExit;
    	}

    	*o_pCount = argc;

        if (0 >= *o_pCount)
        {
            goto l_lblExit;
        }

        //fill result///////////////////////////////////////////////////////////////
        pathLength--; // leave room for the terminating byte
    	for (i = 0; (i < argc) && (pathLength > 0); i++)
    	{
    		// iterate through argv[0..argc-1] by reading argv[i] and seeking to
    		// location argv[i] in the addr space, and then reading the string
    		// at that location in the addr space
    		if (lseek(l_pFile, pinfo.initial_stack + (1 + i) * sizeof (void *), SEEK_SET) == -1)
    		{
                goto l_lblExit;
    		}
    		if (read(l_pFile, &argv, sizeof(argv)) < (int)sizeof(argv))
    		{
                goto l_lblExit;
    		}
    		if (lseek(l_pFile, (uintptr_t)argv, SEEK_SET) == -1)
    		{
                goto l_lblExit;
    		}

    		l = read(l_pFile, p, pathLength);
    		if (l == -1)
    		{
                goto l_lblExit;
    		}

    		// we need to find the end of the string because read() will
    		// read past the '\0'
    		while (*p && l)
    		{
    			p++;
    			l--;
    			pathLength--;
    		}

    		if (pathLength > 0){
    			*p++ = ' ';   // add a space between arguments
    			pathLength--;
    		}
    		if (pathLength > 0)
    			*p = 0; // preserve our command line now in case we have an error
    	}
    	p[-1] = 0; 	// remove the trailing space

    l_lblExit:

        if (-1 != l_pFile)
        {
            close(l_pFile);
        }

        return Get_ArgV(l_pCmdLine, o_pCount);
    }//Get_ArgV


    ///////////////////////////////////////////////////////////////////////////////
    //Get_ArgV
    static tXCHAR **Get_ArgV(const tXCHAR *i_pCmdLine, tINT32 *o_pCount)
    {
        tINT32        l_iLen    = 0;//strlen(i_pCmdLine);
        tXCHAR       *l_pBuffer = NULL;//new tXCHAR[l_iLen];
        tXCHAR      **l_pReturn = NULL;
        tBOOL         l_bNew    = TRUE;
        tBOOL         l_bStr    = FALSE;
        tINT32        l_iIDX    = 0;

        if (    (NULL == i_pCmdLine)
             || (NULL == o_pCount)    
        )
        {
            goto l_lblExit;
        }

        l_iLen = (tINT32)strlen(i_pCmdLine) + 1;
        l_pBuffer = new tXCHAR[l_iLen];

        if (NULL == l_pBuffer)
        {
            goto l_lblExit;
        }

        for (tINT32 l_iI = 0; l_iI < l_iLen; l_iI++)
        {
            if (TM('\"') == i_pCmdLine[l_iI])
            {
                l_bStr = ! l_bStr;
            }
            else
            {
                if (    (TM(' ') == i_pCmdLine[l_iI])
                     && (!l_bStr)
                   )
                {
                    l_pBuffer[l_iIDX++] = 0;
                }
                else
                {
                    l_pBuffer[l_iIDX++] = i_pCmdLine[l_iI];
                }
                
            }
        }  

        l_iLen = l_iIDX;

        //calculate count of items//////////////////////////////////////////////////
        *o_pCount = 0;
        for (tINT32 l_iI = 0; l_iI < l_iLen; l_iI++)
        {
            if (    (0 == l_pBuffer[l_iI])
                 || ((l_iI + 1) == l_iLen)    
               )
            {
                (*o_pCount) ++;
            }
        }

        if (0 >= *o_pCount)
        {
            goto l_lblExit;
        }

        //allocate result///////////////////////////////////////////////////////////
        l_pReturn = new tXCHAR*[*o_pCount];

        if (NULL == l_pReturn)
        {
            goto l_lblExit;
        }

        //fill result///////////////////////////////////////////////////////////////
        l_bNew = TRUE;
        l_iIDX = 0;
        for (tINT32 l_iI = 0; l_iI < l_iLen; l_iI++)
        {
            if (l_bNew)
            {
                l_pReturn[l_iIDX++] = &l_pBuffer[l_iI];
                l_bNew = FALSE;
            }

            if (0 == l_pBuffer[l_iI])
            {
                l_bNew = TRUE;
            }
        }

    l_lblExit:

        return l_pReturn;
    }//Get_ArgV


    ///////////////////////////////////////////////////////////////////////////////
    //Free_ArgV
    static void Free_ArgV(tXCHAR **i_pArgV)
    {
        if (NULL == i_pArgV)
        {
            return;
        }

        if (i_pArgV[0])
        {
            delete [] i_pArgV[0];
        }

        delete [] i_pArgV; 
        //LocalFree(i_pArgV);
    }//Free_ArgV


    ////////////////////////////////////////////////////////////////////////////////
    //Get_Process_Time()
    //return a 64-bit value divided into 2 parts representing the number of 
    //100-nanosecond intervals since January 1, 1601 (UTC).
    static tBOOL Get_Process_Time(tUINT32 *o_pHTime, tUINT32 *o_pLTime)
    {
        long long  l_llStime  = 0;
        tBOOL      l_bResult  = FALSE;
        int        fd;
        debug_process_t info;

        if ( (NULL == o_pHTime)
            || (NULL == o_pLTime)   
        )
        {
            goto l_lblExit;
        }

        *o_pHTime = 0;
        *o_pLTime = 0;

    	fd = open("/proc/self/as", O_RDONLY);
    	if (-1 == fd)
    	{
            goto l_lblExit;
    	}

    	/* Использование процессора */
    	if (EOK != devctl(fd, DCMD_PROC_INFO, &info, sizeof(info), 0))
    	{
    		close(fd);
            goto l_lblExit;
    	}

    	close(fd);

    	l_llStime  = info.start_time / 100;
        l_llStime += TIME_OFFSET_1601_1970; //defined in "PTime.h"

        *o_pHTime = (l_llStime >> 32);
        *o_pLTime = (l_llStime & 0xFFFFFFFF);

        l_bResult = TRUE;

    l_lblExit:
	
        return l_bResult;
    }//Get_Process_Time()


    ////////////////////////////////////////////////////////////////////////////////
    //Get_Process_Name
    static tBOOL Get_Process_Name(tACHAR *o_pName, tINT32 i_iMax_Len)
    {
        tBOOL l_bReturn = FALSE;
    	int fd;
    	static struct
    	{
    		procfs_debuginfo info;
    		char buff[PATH_MAX];
    	} name;

        if (    (NULL == o_pName)
            || (32   >= i_iMax_Len)
        )
        {
            goto l_lblExit;
        }

    	fd = open("/proc/self/as", O_RDONLY);
    	if (-1 == fd)
    	{
            goto l_lblExit;
    	}

    	if (EOK != devctl(fd, DCMD_PROC_MAPDEBUG_BASE, &name, sizeof(name), 0))
    	{
    		close(fd);
            goto l_lblExit;
    	}

		close(fd);

		l_bReturn = TRUE;

    	strncpy(o_pName, basename(name.info.path), i_iMax_Len - 1);
    	o_pName[i_iMax_Len] = '\0';

    l_lblExit:
        return l_bReturn;
    }//Get_Process_Name

    ////////////////////////////////////////////////////////////////////////////////
    //Get_Process_Name
    static tBOOL Get_Process_Name(tWCHAR *o_pName, tINT32 i_iMax_Len)
        {
        int   l_iLen    = 4096;
        char *l_pBuffer = new char[l_iLen];
        tBOOL l_bReturn = FALSE;


        if (    (l_pBuffer)
             && (Get_Process_Name(l_pBuffer, l_iLen))
           )
        {
            l_bReturn = TRUE;
            Convert_UTF8_To_UTF16(l_pBuffer, o_pName, i_iMax_Len);
        }
        else
        {
            Convert_UTF8_To_UTF16("Unknown", o_pName, i_iMax_Len);
        }
    
        if (l_pBuffer)
        {
            delete [] l_pBuffer;
            l_pBuffer = NULL;
        }

        return l_bReturn;
    }//Get_Process_Name


    ////////////////////////////////////////////////////////////////////////////////
    //Get_Process_Path
    static tBOOL Get_Process_Path(tXCHAR *o_pPath, tINT32 i_iMax_Len)
    {
        tBOOL l_bReturn = FALSE;
    	int fd;
    	static struct
    	{
    		procfs_debuginfo info;
    		char buff[PATH_MAX];
    	} name;

        if (    (NULL == o_pPath)
            || (32   >= i_iMax_Len)
        )
        {
            goto l_lblExit;
        }

    	fd = open("/proc/self/as", O_RDONLY);
    	if (-1 == fd)
    	{
            goto l_lblExit;
    	}

    	if (EOK != devctl(fd, DCMD_PROC_MAPDEBUG_BASE, &name, sizeof(name), 0))
    	{
    		close(fd);
            goto l_lblExit;
    	}

		close(fd);

		l_bReturn = TRUE;

    	strncpy(o_pPath, name.info.path, i_iMax_Len - 1);
    	o_pPath[i_iMax_Len] = '\0';

    l_lblExit:
        return l_bReturn;
    }//Get_Process_Path


    ////////////////////////////////////////////////////////////////////////////////
    //Get_Process_ID
    static tUINT32 Get_Process_ID()
    {
        return getpid();
    }//Get_Process_ID
    
    
    ////////////////////////////////////////////////////////////////////////////////
    //Get_Thread_Id
    static tUINT32 Get_Thread_Id()
    {
        return pthread_self();
    }//Get_Thread_Id
    
    
    ////////////////////////////////////////////////////////////////////////////////
    //Get_Processor
    static __forceinline tUINT32 Get_Processor()
    {
    	procfs_status status;
    	int fd;
    	tUINT32 l_bReturn = -1;

    	fd = open("/proc/self/as", O_RDONLY);
    	if (-1 == fd)
    	{
            goto l_lblExit;
    	}

    	if (EOK != devctl(fd, DCMD_PROC_STATUS, &status, sizeof(status), NULL))
    	{
    		close(fd);
            goto l_lblExit;
    	}

		close(fd);

		l_bReturn = status.last_cpu;

l_lblExit:
		return l_bReturn;
    }//Get_Processor
};

#endif //PPROCESS_H
