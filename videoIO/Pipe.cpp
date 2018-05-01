/**
* @file Pipe.cpp
* @ingroup DataManagement
* @author Dominique Vaufreydaz, Grenoble Alpes University, Inria
* @copyright All right reserved.
*/

/* 
	This code is reproduced from https://github.com/Vaufreyd/ReadWriteVideosWithOpenCV in accordance with its GNU public license. Thanks!
*/

#include "Pipe.h"

/** @brief Constructor, empty but defined. Values are set in c++11 way.
*/
Pipe::Pipe() :
	DebugMode(false)		// Do not show command line before executing them
{
	// by default,
	Mode = UNK_MODE;
	InternalFile = nullptr;
}

/** @brief Virtual destructor always
	*/
Pipe::~Pipe()
{
	// Close everything
	Close();
}


/** @brief Open a pipe to a command
	*
*/
bool Pipe::Open(const char *Command, int eMode /* = READ_MODE */ )
{
#if defined WIN32 || defined WIN64
	const char * ModeRead = "rb";
	const char * ModeWrite = "wb";
#else
	// Linux and MacOS does not support ReadMode
	const char * ModeRead = "r";
	const char * ModeWrite = "w";
#endif
	// Read mode
	if ( eMode == READ_MODE )
	{
		Mode = READ_MODE;
		InternalFile = popen( Command, ModeRead );
	}
	// Or write mode
	else if ( eMode == WRITE_MODE )
	{
		Mode = WRITE_MODE;
		InternalFile = popen( Command, ModeWrite );
	}
	// Bad param, unkown mode
	else
	{
		fprintf( stderr, "Unkown opening mode\n" );
		Mode = UNK_MODE;
		return false;
	}

	// Check if we manage to open pipe
	if ( InternalFile == nullptr )
	{
		// no...
		Mode = UNK_MODE;
		return false;
	}
	else
	{
		// pipe is opened
		return true;	
	}
}

/** @brief Close the Pipe (if opened).
	*/
void Pipe::Close()
{
	if ( InternalFile != nullptr )
	{
		pclose(InternalFile);
		InternalFile = nullptr;
	}
}