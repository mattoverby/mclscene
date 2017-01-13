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


#ifndef MCLSCENE_MICROTIMER_H
#define MCLSCENE_MICROTIMER_H 1

#include <chrono>

// -Timer starts upon construction and doesn't stop until destruction.
// -It can be reset back to zero with reset().
// -It returns elapsed time in double precision.
//
// Example:
//
//	mcl::MicroTimer timer;
//
//	... do work ...
//
//	double elapsed_microseconds = timer.elapsed_us();
//	double elapsed_milliseconds = timer.elapsed_ms();
//	double elapsed_seconds = timer.elapsed_s();
//
//	   Note: Each elapsed_* value above includes the calls before
//	   it. If you require precision in multiple units it's more
//	   accurate to only call MicroTimer::elapsed_us() and convert
//	   the result to whatever units you need.
//

namespace mcl {

class MicroTimer {
	typedef std::chrono::steady_clock C; // clock type
	public:

		MicroTimer() : start_time( C::now() ){}

		// Resets the timer
		void reset() { start_time = C::now(); }

		// Return time elapsed in seconds
		double elapsed_s() const {
			curr_time = C::now();
			std::chrono::duration<double> durr = curr_time-start_time;
			return durr.count();
		}

		// Return time elapsed in milliseconds
		double elapsed_ms() const {
			curr_time = C::now();
			std::chrono::duration<double, std::milli> durr = curr_time-start_time;
			return durr.count();
		}

		// Return time elapsed in microseconds
		double elapsed_us() const {
			curr_time = C::now();
			std::chrono::duration<double, std::micro> durr = curr_time-start_time;
			return durr.count();
		}

	private:
		std::chrono::time_point<std::chrono::steady_clock> start_time;
		mutable std::chrono::time_point<std::chrono::steady_clock> curr_time;

}; // end class MicroTimer


} // end namespace mcl


#endif
