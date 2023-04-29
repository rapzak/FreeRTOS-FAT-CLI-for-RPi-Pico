/* File-related-CLI-commands.c
Copyright 2021 Carl John Kugler III

Licensed under the Apache License, Version 2.0 (the License); you may not use
this file except in compliance with the License. You may obtain a copy of the
License at

   http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.
*/
/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* FreeRTOS+CLI includes. */
#include "FreeRTOS_CLI.h"

/* FreeRTOS+FAT includes. */
#include "ff_headers.h"
#include "ff_stdio.h"

#define cliNEW_LINE		"\r"

/*******************************************************************************
 * See the URL in the comments within main.c for the location of the online
 * documentation.
 ******************************************************************************/

/*
 * Print out information on a single file.
 */
static void prvCreateFileInfoString( char *pcBuffer, FF_FindData_t *pxFindStruct );

/*
 * Copies an existing file into a newly created file.
 */
static BaseType_t prvPerformCopy(const char *pcSourceFile, size_t lSourceFileLength, const char *pcDestinationFile,
		char *pxWriteBuffer, size_t xWriteBufferLen);

/*
 * Implements the DIR command.
 */
static BaseType_t prvDIRCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

/*
 * Implements the CD command.
 */
static BaseType_t prvCDCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

/*
 * Implements the DEL command.
 */
static BaseType_t prvDELCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

/*
 * Implements the DEL command.
 */
static BaseType_t prvRMDIRCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

/*
 * Implements the TYPE command.
 */
static BaseType_t prvTYPECommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

/*
 * Implements the COPY command.
 */
static BaseType_t prvCOPYCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

/*
 * Implements the REN command.
 */
static BaseType_t prvRENCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

/*
 * Implements the PWD (print working directory) command.
 */
static BaseType_t prvPWDCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

/* Structure that defines the DIR command line command, which lists all the
files in the current directory. */
static const CLI_Command_Definition_t xDIR =
{ "dir", /* The command string to type. */
"\rdir:\r Lists the files in the current directory\r", prvDIRCommand, /* The function to run. */
	0 /* No parameters are expected. */
};
static const CLI_Command_Definition_t xLS =
{ "ls", /* The command string to type. */
"ls: Alias for \"dir\"\r", prvDIRCommand, /* The function to run. */
	0 /* No parameters are expected. */
};

/* Structure that defines the CD command line command, which changes the
working directory. */
static const CLI_Command_Definition_t xCD =
{ "cd", /* The command string to type. */
"\rcd <dir name>:\r Changes the working directory\r", prvCDCommand, /* The function to run. */
	1 /* One parameter is expected. */
};

/* Structure that defines the TYPE command line command, which prints the
contents of a file to the console. */
static const CLI_Command_Definition_t xTYPE =
{ "type", /* The command string to type. */
"\rtype <filename>:\r Prints file contents to the terminal\r", prvTYPECommand, /* The function to run. */
	1 /* One parameter is expected. */
};

/* Structure that defines the DEL command line command, which deletes a file. */
static const CLI_Command_Definition_t xDEL =
{ "del", /* The command string to type. */
"\rdel <filename>:\r deletes a file (use rmdir to delete a directory)\r", prvDELCommand, /* The function to run. */
	1 /* One parameter is expected. */
};

/* Structure that defines the RMDIR command line command, which deletes a directory. */
static const CLI_Command_Definition_t xRMDIR =
{ "rmdir", /* The command string to type. */
"\rrmdir <directory name>:\r deletes a directory\r", prvRMDIRCommand, /* The function to run. */
	1 /* One parameter is expected. */
};

/* Structure that defines the COPY command line command, which deletes a file. */
static const CLI_Command_Definition_t xCOPY =
{ "copy", /* The command string to type. */
"\rcopy <source file> <dest file>:\r Copies <source file> to <dest file>\r", prvCOPYCommand, /* The function to run. */
	2 /* Two parameters are expected. */
};

/* Structure that defines the COPY command line command, which deletes a file. */
static const CLI_Command_Definition_t xREN =
{ "ren", /* The command string to type. */
"\rren <source file> <dest file>:\r Moves <source file> to <dest file>\r", prvRENCommand, /* The function to run. */
	2 /* Two parameters are expected. */
};

/* Structure that defines the pwd command line command, which prints the current working directory. */
static const CLI_Command_Definition_t xPWD =
{ "pwd", /* The command string to type. */
"\rpwd:\r Print Working Directory\r", prvPWDCommand, /* The function to run. */
	0 /* No parameters are expected. */
};

/*-----------------------------------------------------------*/

void vRegisterFileSystemCLICommands(void) {
	/* Register all the command line commands defined immediately above. */
	FreeRTOS_CLIRegisterCommand( &xDIR );
	FreeRTOS_CLIRegisterCommand( &xLS );
	FreeRTOS_CLIRegisterCommand( &xCD );
	FreeRTOS_CLIRegisterCommand( &xTYPE );
	FreeRTOS_CLIRegisterCommand( &xDEL );
	FreeRTOS_CLIRegisterCommand( &xRMDIR );
	FreeRTOS_CLIRegisterCommand( &xCOPY );
	FreeRTOS_CLIRegisterCommand( &xREN );
	FreeRTOS_CLIRegisterCommand( &xPWD );
}
/*-----------------------------------------------------------*/

static BaseType_t prvTYPECommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
const char *pcParameter;
BaseType_t xParameterStringLength, xReturn = pdTRUE;
static FF_FILE *pxFile = NULL;
int iChar;
size_t xByte;
size_t xColumns = 80U;

	/* Ensure there is always a null terminator after each character written. */
	memset( pcWriteBuffer, 0x00, xWriteBufferLen );

	/* Ensure the buffer leaves space for the \r. */
	configASSERT( xWriteBufferLen > ( strlen( cliNEW_LINE ) * 2 ) );
	xWriteBufferLen -= strlen( cliNEW_LINE );

	if (xWriteBufferLen < xColumns) {
		/* Ensure the loop that uses xColumns as an end condition does not
		write off the end of the buffer. */
		xColumns = xWriteBufferLen;
	}

	if (pxFile == NULL) {
		/* The file has not been opened yet.  Find the file name. */
		pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, /* The command string itself. */
							1,						/* Return the first parameter. */
							&xParameterStringLength	/* Store the parameter string length. */
						);

		/* Sanity check something was returned. */
		configASSERT( pcParameter );

		/* Attempt to open the requested file. */
		pxFile = ff_fopen( pcParameter, "r" );
	}

	if (pxFile != NULL) {
		/* Read the next chunk of data from the file. */
		for (xByte = 0; xByte < xColumns; xByte++) {
			iChar = ff_fgetc( pxFile );

			if (iChar == -1) {
				/* No more characters to return. */
				ff_fclose( pxFile );
				pxFile = NULL;
				break;
			} else {
				pcWriteBuffer[ xByte ] = ( char ) iChar;
			}
		}
	}

	if (pxFile == NULL) {
		/* Either the file was not opened, or all the data from the file has
		been returned and the file is now closed. */
		xReturn = pdFALSE;
	}

	strcat( pcWriteBuffer, cliNEW_LINE );

	return xReturn;
}
/*-----------------------------------------------------------*/

static BaseType_t prvCDCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
const char *pcParameter;
BaseType_t xParameterStringLength;
int iReturned;
size_t xStringLength;

	/* Obtain the parameter string. */
	pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, /* The command string itself. */
						1,						/* Return the first parameter. */
						&xParameterStringLength	/* Store the parameter string length. */
					);

	/* Sanity check something was returned. */
	configASSERT( pcParameter );

	/* Attempt to move to the requested directory. */
	iReturned = ff_chdir( pcParameter );

	if (iReturned == FF_ERR_NONE) {
		sprintf( pcWriteBuffer, "In: " );
		xStringLength = strlen( pcWriteBuffer );
		ff_getcwd( &( pcWriteBuffer[ xStringLength ] ), ( unsigned char ) ( xWriteBufferLen - xStringLength ) );
	} else {
		sprintf( pcWriteBuffer, "Error" );
	}

	strcat( pcWriteBuffer, cliNEW_LINE );

	return pdFALSE;
}
/*-----------------------------------------------------------*/

static BaseType_t prvDIRCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
static FF_FindData_t *pxFindStruct = NULL;
int iReturned;
BaseType_t xReturn = pdFALSE;

	/* This assumes pcWriteBuffer is long enough. */
	( void ) pcCommandString;

	/* Ensure the buffer leaves space for the \r. */
	configASSERT( xWriteBufferLen > ( strlen( cliNEW_LINE ) * 2 ) );
	xWriteBufferLen -= strlen( cliNEW_LINE );

	if (pxFindStruct == NULL) {
		/* This is the first time this function has been executed since the Dir
		command was run.  Create the find structure. */
		pxFindStruct = ( FF_FindData_t * ) pvPortMalloc( sizeof( FF_FindData_t ) );

		if (pxFindStruct != NULL) {
			memset( pxFindStruct, 0x00, sizeof( FF_FindData_t ) );
			iReturned = ff_findfirst( "", pxFindStruct );

			if (iReturned == FF_ERR_NONE) {
				prvCreateFileInfoString( pcWriteBuffer, pxFindStruct );
				xReturn = pdPASS;
			} else {
				snprintf( pcWriteBuffer, xWriteBufferLen, "Error: ff_findfirst() failed." );
				pxFindStruct = NULL;
			}
		} else {
			snprintf( pcWriteBuffer, xWriteBufferLen, "Failed to allocate RAM (using heap_4.c will prevent fragmentation)." );
		}
	} else {
		/* The find struct has already been created.  Find the next file in
		the directory. */
		iReturned = ff_findnext( pxFindStruct );

		if (iReturned == FF_ERR_NONE) {
			prvCreateFileInfoString( pcWriteBuffer, pxFindStruct );
			xReturn = pdPASS;
		} else {
			vPortFree( pxFindStruct );
			pxFindStruct = NULL;

			/* No string to return. */
			pcWriteBuffer[ 0 ] = 0x00;
		}
	}

	strcat( pcWriteBuffer, cliNEW_LINE );

	return xReturn;
}
/*-----------------------------------------------------------*/

static BaseType_t prvRMDIRCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
const char *pcParameter;
BaseType_t xParameterStringLength;
int iReturned;

	/* This function assumes xWriteBufferLen is large enough! */
	( void ) xWriteBufferLen;

	/* Obtain the parameter string. */
	pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, /* The command string itself. */
						1,						/* Return the first parameter. */
						&xParameterStringLength	/* Store the parameter string length. */
					);

	/* Sanity check something was returned. */
	configASSERT( pcParameter );

	/* Attempt to delete the directory. */
	iReturned = ff_rmdir( pcParameter );

	if (iReturned == FF_ERR_NONE) {
		sprintf( pcWriteBuffer, "%s was deleted", pcParameter );
	} else {
		sprintf( pcWriteBuffer, "Error.  %s was not deleted", pcParameter );
	}

	strcat( pcWriteBuffer, cliNEW_LINE );

	return pdFALSE;
}
/*-----------------------------------------------------------*/

static BaseType_t prvDELCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
const char *pcParameter;
BaseType_t xParameterStringLength;
int iReturned;

	/* Obtain the parameter string. */
	pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, /* The command string itself. */
						1,						/* Return the first parameter. */
						&xParameterStringLength	/* Store the parameter string length. */
					);

	/* Sanity check something was returned. */
	configASSERT( pcParameter );

	/* Attempt to delete the file. */
	iReturned = ff_remove( pcParameter );

	if (iReturned == FF_ERR_NONE) {
		snprintf(pcWriteBuffer, xWriteBufferLen, "%s was deleted", pcParameter);
	} else {
		snprintf(pcWriteBuffer, xWriteBufferLen, "Error.  %s was not deleted", pcParameter);
	}
	return pdFALSE;
}
/*-----------------------------------------------------------*/

static BaseType_t prvCOPYCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
char *pcSourceFile;
const char *pcDestinationFile;
BaseType_t xParameterStringLength;
long lSourceLength, lDestinationLength = 0;
FF_Stat_t xStat;

	/* Obtain the name of the destination file. */
	pcDestinationFile = FreeRTOS_CLIGetParameter(pcCommandString, /* The command string itself. */
							2,						/* Return the second parameter. */
							&xParameterStringLength	/* Store the parameter string length. */
						);

	/* Sanity check something was returned. */
	configASSERT( pcDestinationFile );

	/* Obtain the name of the source file. */
	pcSourceFile = (char *) FreeRTOS_CLIGetParameter(pcCommandString, /* The command string itself. */
									1,						/* Return the first parameter. */
									&xParameterStringLength	/* Store the parameter string length. */
								);

	/* Sanity check something was returned. */
	configASSERT( pcSourceFile );

	/* Terminate the string. */
	pcSourceFile[ xParameterStringLength ] = 0x00;

	/* See if the source file exists, obtain its length if it does. */
	if (ff_stat(pcSourceFile, &xStat) == FF_ERR_NONE) {
		lSourceLength = xStat.st_size;
	} else {
		lSourceLength = 0;
	}

	if (lSourceLength == 0) {
		sprintf( pcWriteBuffer, "Source file does not exist" );
	} else {
		/* See if the destination file exists. */
		if (ff_stat(pcDestinationFile, &xStat) == FF_ERR_NONE) {
			lDestinationLength = xStat.st_size;
		} else {
			lDestinationLength = 0;
		}

		if (xStat.st_mode == FF_IFDIR) {
			sprintf( pcWriteBuffer, "Error: Destination is a directory not a file" );

			/* Set lDestinationLength to a non-zero value just to prevent an
			attempt to copy the file. */
			lDestinationLength = 1;
		} else if (lDestinationLength != 0) {
			sprintf( pcWriteBuffer, "Error: Destination file already exists" );
		}
	}

	/* Continue only if the source file exists and the destination file does
	not exist. */
	if ((lSourceLength != 0) && (lDestinationLength == 0)) {
		if (prvPerformCopy(pcSourceFile, lSourceLength, pcDestinationFile, pcWriteBuffer, xWriteBufferLen) == pdPASS) {
			sprintf( pcWriteBuffer, "Copy made" );
		} else {
			sprintf( pcWriteBuffer, "Error during copy" );
		}
	}

	strcat( pcWriteBuffer, cliNEW_LINE );

	return pdFALSE;
}
/*-----------------------------------------------------------*/

static BaseType_t prvRENCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
char *pcSourceFile;
const char *pcDestinationFile;
BaseType_t xParameterStringLength;

	/* Obtain the name of the destination file. */
	pcDestinationFile = FreeRTOS_CLIGetParameter(pcCommandString, /* The command string itself. */
							2,						/* Return the second parameter. */
							&xParameterStringLength	/* Store the parameter string length. */
						);

	/* Sanity check something was returned. */
	configASSERT( pcDestinationFile );

	/* Obtain the name of the source file. */
	pcSourceFile = (char *) FreeRTOS_CLIGetParameter(pcCommandString, /* The command string itself. */
									1,						/* Return the first parameter. */
									&xParameterStringLength	/* Store the parameter string length. */
								);

	/* Sanity check something was returned. */
	configASSERT( pcSourceFile );

	/* Terminate the string. */
	pcSourceFile[ xParameterStringLength ] = 0x00;

    // int ff_rename( const char *pcOldName, const char *pcNewName );
    // If the file is moved successfully then zero is returned.
    // If the file could not be moved then -1 is returned and the task�s errno is set to indicate the reason. 
    // A task can obtain its errno value using the ff_errno() API function.    
    int ec = ff_rename(pcSourceFile, pcDestinationFile, false);
    if (ec) {
			int error = stdioGET_ERRNO();
		snprintf(pcWriteBuffer, xWriteBufferLen, "%s: ff_rename error: %s, (%d)", __FUNCTION__, strerror(error), error);
    } else {
		snprintf( pcWriteBuffer, xWriteBufferLen, "Rename succeeded" );
	}
	strncat( pcWriteBuffer, cliNEW_LINE, xWriteBufferLen );

	return pdFALSE;
}
/*-----------------------------------------------------------*/

static BaseType_t prvPWDCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
	( void ) pcCommandString;

	/* Copy the current working directory into the output buffer. */
	ff_getcwd( pcWriteBuffer, xWriteBufferLen );
	return pdFALSE;
}
/*-----------------------------------------------------------*/

static BaseType_t prvPerformCopy(const char *pcSourceFile, size_t lSourceFileLength, const char *pcDestinationFile,
		char *pxWriteBuffer, size_t xWriteBufferLen) {
size_t lBytesRead = 0, lBytesToRead, lBytesRemaining;
FF_FILE *pxSourceFile, *pxDestinationFile;
BaseType_t xReturn = pdPASS;

	/* NOTE:  Error handling has been omitted for clarity. */

	pxSourceFile = ff_fopen( pcSourceFile, "r" );
	pxDestinationFile = ff_fopen( pcDestinationFile, "a" );

	if ((pxSourceFile != NULL) && (pxDestinationFile != NULL)) {
		while (lBytesRead < lSourceFileLength) {
			/* How many bytes are left? */
			lBytesRemaining = lSourceFileLength - lBytesRead;

			/* How many bytes should be read this time around the loop.  Can't
			read more bytes than will fit into the buffer. */
			if (lBytesRemaining > xWriteBufferLen) {
				lBytesToRead = xWriteBufferLen;
			} else {
				lBytesToRead = lBytesRemaining;
			}

			ff_fread( pxWriteBuffer, lBytesToRead, 1, pxSourceFile );
			ff_fwrite( pxWriteBuffer, lBytesToRead, 1, pxDestinationFile );

			lBytesRead += lBytesToRead;
		}
	}

	if (pxSourceFile != NULL) {
		ff_fclose( pxSourceFile );
	}

	if (pxSourceFile != NULL) {
		ff_fclose( pxDestinationFile );
	}

	if (lBytesRead == lSourceFileLength) {
		xReturn = pdPASS;
	} else {
		xReturn = pdFAIL;
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

static void prvCreateFileInfoString(char *pcBuffer, FF_FindData_t *pxFindStruct) {
const char * pcWritableFile = "writable file", *pcReadOnlyFile = "read only file", *pcDirectory = "directory";
const char * pcAttrib;

	/* Point pcAttrib to a string that describes the file. */
	if ((pxFindStruct->ucAttributes & FF_FAT_ATTR_DIR) != 0) {
		pcAttrib = pcDirectory;
	} else if (pxFindStruct->ucAttributes & FF_FAT_ATTR_READONLY) {
		pcAttrib = pcReadOnlyFile;
	} else {
		pcAttrib = pcWritableFile;
	}

	/* Create a string that includes the file name, the file size and the
	attributes string. */
	sprintf( pcBuffer, "%s [%s] [size=%lu]", pxFindStruct->pcFileName, pcAttrib, (unsigned long)pxFindStruct->ulFileSize );
}

