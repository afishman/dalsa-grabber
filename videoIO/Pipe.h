/**
 * @file Pipe.h
 * @ingroup DataManagement
 * @author Dominique Vaufreydaz, Grenoble Alpes University, Inria
 * @copyright All right reserved.
 */

#ifndef __PIPE_H__
#define __PIPE_H__

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#if !defined WIN32 && !defined WIN64
	#if __cplusplus < 201103L
		#ifndef nullptr
			#define nullptr NULL
		#endif
	#endif
#endif

#if defined  WIN32 || defined WIN64
	// Define null device
	#define NULL_OUTPUT "NUL"
#else
	// Define null device
	#define NULL_OUTPUT "/dev/null"
#endif

/**
 * @class Pipe Pipe.cpp Pipe.h
 * @brief Abtraction class for pipe functions under Windows/Linux/MacOSX.
 *
 * This classe is used to make consistancy calls for popen and pclose function accross
 * operating systems.
 * @author Dominique Vaufreydaz, Grenoble Alpes University, Inria
 */
class Pipe
{
public:
	/** @brief Constructor, empty but defined. Values are set in c++11 way.
	*/
	Pipe();

	/** @brief Virtual destructor always
	 */
	virtual ~Pipe();

	/** @brief cast operator to permit access to underlying FILE structure
	 */
	operator FILE *()
	{
		return InternalFile;
	}

	/** @enum Pipe::PipeMode
	 *  @brief Enum flags to select read *or* write pipe, i.e. if we want to pipe from (read) or to (write) an external program
	*/
	enum PipeMode {
		UNK_MODE,		/*!< Default value, mode is not known */
		READ_MODE,		/*!< Read from an external program */
		WRITE_MODE		/*!< Write to an exteran program */
	};

	/** @brief Open a pipe to a command
	 *
	*/
	bool Open(const char *Command, int eMode = READ_MODE );

	/** @brief Close the Pipe (if opened).
	 */
	void Close();

private:
	/** @brief Inline function to wrap the popen function. Not for external usage.
	 *
	 * @param Command [in] the command to be executed
	 * @param Type [in] the access type. 
	 * @return Pointer to a FILE * structure
	 */
	inline FILE * popen(const char *Command, const char *Type)
	{
#if defined WIN32 || defined WIN64
		return ::_popen(Command, Type);
#else
		return ::popen(Command, Type);
#endif
    }

	/** @brief inline function to wrap the popen function. Not for external usage.
	 *
	 * @param Command [in] the pipe stream to close
	 */
	inline int pclose(FILE *stream)
    {
#if defined WIN32 || defined WIN64
		return ::_pclose(stream);
#else
		return ::pclose(stream);
#endif
    }

protected:
	FILE * InternalFile;	/*!< A pointer to a FILE structure. In subclasses, this pointer can be re-used for usual files whenever there was no call for Pipe methods. */

public:
	int Mode;				/*!< Used to say if we want to open pip for reading or writing */
	bool DebugMode;			/*!< Boolean value to ask for Pipe debugging information. Default=false. */
};

#endif // __PIPE_H__
