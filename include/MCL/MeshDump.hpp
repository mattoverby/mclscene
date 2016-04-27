// Copyright 2016 Matthew Overby.
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


#ifndef MCLSCENE_MESHDUMP_H
#define MCLSCENE_MESHDUMP_H 1

#include "../../deps/pugixml/pugixml.hpp"
#include <sstream>
#include <boost/filesystem.hpp>
#include "TriMesh.h"
#include <iomanip>

namespace mcl {

class MeshDump {
public:
	struct entry {
		float dt;
		std::vector< std::string > mesh_files; // full path names 
	};

	// Creates a meshdump directory
	// If save is called and remove_existing_data is true, previous data is deleted
	MeshDump( std::string directory="meshdump/", bool remove_existing_dir=true ) :
		remove_existing_data(remove_existing_dir), dump_dir(directory), directory_created(false) {}

	~MeshDump(){ entries.clear(); dump_dir=""; }

	// Load meshes from a directory
	// returns true on success
	bool loadDirectory( std::string directory );

	// Returns the dump directory
	std::string getDirectory(){ return dump_dir; }

	// Set the values of data to the corresponding mesh
	// returns true on success
	bool get( int entry_id, entry *data );

	// Saves current frame to dump directory
	// Returns true on success
	bool save( float dt, const std::vector< trimesh::TriMesh* > &meshes );

	// Returns number of entries loaded after loadDirectory
	unsigned long numEntries() const { return entries.size(); }

private:
	// Settings
	bool directory_created;
	bool remove_existing_data; // Clear the default directory if it exists, default: true
	std::string dump_dir; // Directory of the dump, default: ./meshdump

	// Creates a dump directory and (if remove_existing_data) will delete the old one.
	void createDirectory( std::string directory );

	// The vector containing animation entries, filled by loadDirectory.
	// A call to save(...) does NOT add entries to this vector.
	std::vector< entry > entries;
};


} // end namespace mcl

#endif
