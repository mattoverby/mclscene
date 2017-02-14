// Copyright (c) 2016 University of Minnesota
// 
// MCLSCENE Uses the BSD 2-Clause License (http://www.opensource.org/licenses/BSD-2-Clause)
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other materials
//    provided with the distribution.
// THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR  A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE UNIVERSITY OF MINNESOTA, DULUTH OR CONTRIBUTORS BE 
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
// OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// By Matt Overby (http://www.mattoverby.net)


#ifndef MCLSCENE_LOGGER_H
#define MCLSCENE_LOGGER_H 1

#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string>
#include <unordered_map>
#include <sstream>

namespace mcl {

///
///	TODO make thread safe
///
class Logger {

	public:
		/// @brief Default Constructor
		///
		Logger() : last_appended("")  {}

		/// @brief Open a file to start logging to. This must be done
		/// before append(...) can be called.
		///
		bool open( std::string label, std::string filename, bool delete_if_exists=false ){

			bool exists = open_files.count( label );
			if( delete_if_exists && exists ){
				remove( filename.c_str() );
				remove( open_files[label].c_str() );
			}

			std::ofstream filestream;
			filestream.open( filename.c_str() );
			filestream.close();
			last_appended = label;

			open_files[ label ] = filename;

			return true;

		} // end open file

		/// @brief writes to the end of a file
		/// TODO: Overload stream operator
		///
		bool append( std::string label, std::string text ){
			if( !open_files.count( label ) ){ return false; }

			std::ofstream filestream;
			filestream.open( open_files[label].c_str(), std::ios_base::app );
			filestream << text;
			filestream.close();
			last_appended = label;

			return true;

		} // end append to file

	private:
		/// Label -> file
		std::unordered_map< std::string, std::string > open_files;
		std::string last_appended;


}; // end class Logger

} // end namespace mcl

#endif

