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

#include "MCL/MeshDump.hpp"

using namespace mcl;


void MeshDump::createDirectory( std::string directory ){

	if( directory.back() != '/' ){ directory += '/'; }

	namespace fs=boost::filesystem;

	// First, delete the directory and its contents if it exists
	fs::path dir( directory.c_str() );
	fs::remove_all( directory );

	// Recreate the directory and info XML
	fs::create_directory( dir );
	std::ofstream filestream; filestream.open( directory + "mesh_dump_info.xml" );
	filestream << "<meshdump>\n\t<info>\n\t\t<num_entries value=\"0\" />";
	filestream << "\n\t<num_meshes value=\"0\" />";
	filestream << "\n\t</info>\n</meshdump>";
	filestream.close();

}


bool MeshDump::loadDirectory( std::string directory ){

	if( directory.back() != '/' ){ directory += '/'; }
	dump_dir = directory;

	// First load and parse the XML file
	pugi::xml_document xml;
	std::string xml_filename = directory + "/mesh_dump_info.xml";
	if( !xml.load_file(xml_filename.c_str()) ){
		std::cerr << "\n**MeshDump::load Error: Could not load from directory " << directory << std::endl;
		return false;
	}

	int meshes_per_entry = 0;

	// Loop through the xml nodes to find a meshdump
	for( pugi::xml_node meshdump_data = xml.first_child(); meshdump_data;
		meshdump_data = meshdump_data.next_sibling()){

		// Make sure we've loaded a meshdump_data
		std::string nodename = meshdump_data.name();
		if( nodename.compare("meshdump")==0 ){
		for( pugi::xml_node info_data = meshdump_data.first_child(); info_data;
			info_data = info_data.next_sibling()){
			nodename = info_data.name();
			if( nodename.compare( "info" )==0 ){
				for( pugi::xml_node info_values = info_data.first_child(); info_values;
					info_values = info_values.next_sibling()){
					nodename = info_values.name();

					if( nodename.compare( "num_entries" )==0 ){
						entries.resize( info_values.attribute("value").as_int() );
					}

					if( nodename.compare( "num_meshes" )==0 ){
						meshes_per_entry = info_values.attribute("value").as_int();
					}

				} // end loop values in the info container

			} // end load info

		}} // end is meshdump_data and loop

	} // end loop xml children

	// Loop the entires and store
	for( pugi::xml_node meshdump_data = xml.first_child(); meshdump_data;
		meshdump_data = meshdump_data.next_sibling()){

		// Make sure we've loaded a meshdump_data
		std::string nodename = meshdump_data.name();
		if( nodename.compare("meshdump")==0 ){
		for( pugi::xml_node entry_data = meshdump_data.first_child(); entry_data;
			entry_data = entry_data.next_sibling()){
			nodename = entry_data.name();
			if( nodename.compare( "entry" )==0 ){

				int id = entry_data.attribute("id").as_int();
				if( id < 0 || id > entries.size()-1 ){
					std::cerr << "\n**MeshDump::load Error: entry id " << id << " too high." << std::endl;
					return false;
				}

				// resize the mesh_files
				entries[id].mesh_files.resize(meshes_per_entry);

				// Parse entry values
				for( pugi::xml_node entry_values = entry_data.first_child(); entry_values;
					entry_values = entry_values.next_sibling()){
					nodename = entry_values.name();

					if( nodename.compare( "timestep" )==0 ){
						entries[id].dt = entry_values.attribute("value").as_float();
					}

					if( nodename.compare( "mesh" )==0 ){
						int mesh_id = entry_values.attribute("id").as_int(); // TODO check
						entries[id].mesh_files[mesh_id] = entry_values.attribute("filename").as_string();
					}

				} // end loop values in the info container

			} // end load info

		}} // end is meshdump_data and loop

	} // end loop xml children

	return true;

} // end load a mesh dump


bool MeshDump::get( int entry_id, entry *data ){

	if( dump_dir.back() != '/' ){ dump_dir += '/'; }

	if( !entries.size() ){
		std::cerr << "\n**MeshDump::get Error: No entries. Call loadDirectory first." << std::endl;
		return false;
	}

	if( entry_id < 0 ){ entry_id = 0; }
	if( entry_id > entries.size()-1 ){ entry_id = entries.size()-1; }

	data->mesh_files.clear();
	data->dt = entries[entry_id].dt;
	for( int i=0; i<entries[entry_id].mesh_files.size(); ++i ){
		data->mesh_files.push_back( entries[entry_id].mesh_files[i] );
	}

	return true;

} // end get entry


bool MeshDump::save( float dt, const std::vector< trimesh::TriMesh* > &meshes ){

	if( dump_dir.back() != '/' ){ dump_dir += '/'; }

	static bool directory_created = false;
	if( !directory_created ){
		createDirectory( dump_dir );
		directory_created = true;
	}

	// Open up the XML info and update params
	pugi::xml_document xml;
	std::string xml_filename = dump_dir + "mesh_dump_info.xml";
	if( !xml.load_file(xml_filename.c_str()) ){
		std::cerr << "\n**MeshDump::save Error: Could not load " << dump_dir + "mesh_dump_info.xml" << std::endl;
		return false;
	}

	// Loop through the xml nodes to find a meshdump
	int n_mesh_entries = 0;
	for( pugi::xml_node meshdump_data = xml.first_child(); meshdump_data;
		meshdump_data = meshdump_data.next_sibling()){

		// Make sure we've loaded a meshdump_data
		std::string nodename = meshdump_data.name();
		if( nodename.compare("meshdump")==0 ){
		for( pugi::xml_node info_data = meshdump_data.first_child(); info_data;
			info_data = info_data.next_sibling()){

			nodename = info_data.name();
			if( nodename.compare( "info" )==0 ){

				for( pugi::xml_node info_values = info_data.first_child(); info_values;
					info_values = info_values.next_sibling()){

					nodename = info_values.name();
					if( nodename.compare( "num_entries" )==0 ){

						pugi::xml_attribute attr = info_values.attribute("value");
						std::stringstream ss;
						ss << attr.value();
						ss >> n_mesh_entries;

						// Update the num_entries var
						n_mesh_entries++;
						attr.set_value( n_mesh_entries );
					}

					else if( nodename.compare( "num_meshes" )==0 ){

						pugi::xml_attribute attr = info_values.attribute("value");
						attr.set_value( (int)meshes.size() );
					}

				} // end loop values in the info container

			} // end load info

		}} // end is meshdump_data and loop

	} // end loop xml children

	// Add another timestep entry
	for( pugi::xml_node meshdump_data = xml.first_child(); meshdump_data;
		meshdump_data = meshdump_data.next_sibling()){

		// Make sure we've loaded a meshdump_data
		std::string nodename = meshdump_data.name();
		if( nodename.compare("meshdump")==0 ){

			pugi::xml_node entry_node = meshdump_data.append_child("entry");
			entry_node.append_attribute("id") = (n_mesh_entries-1); 

			pugi::xml_node timestep_node = entry_node.append_child("dt");
			timestep_node.append_attribute("value") = dt;

			// Output mesh filenames
			for( int i=0; i<meshes.size(); ++i ){

				// Make sure this correlates with below ("save the meshes")
				std::stringstream mesh_filename_ss;
				mesh_filename_ss << dump_dir << "mesh_e";
				mesh_filename_ss << std::setfill('0') << std::setw(5) << (n_mesh_entries-1);
				mesh_filename_ss << "_m" << i << ".ply";
				std::string mesh_filename = mesh_filename_ss.str();

				pugi::xml_node mesh_node = entry_node.append_child("mesh");
				mesh_node.append_attribute("filename") = mesh_filename.c_str();
				mesh_node.append_attribute("id") = i;

			} // end add mesh filenames

		} // end is meshdump_data and loop

	} // end loop xml children

	// Update the xml doc
	xml.save_file(xml_filename.c_str());

	// Now save the meshes
	for( int i=0; i<meshes.size(); ++i ){

		// Make sure this correlates with above ("Output mesh filenames")
		std::stringstream mesh_filename_ss;
		mesh_filename_ss << dump_dir << "mesh_e";
		mesh_filename_ss << std::setfill('0') << std::setw(5) << (n_mesh_entries-1);
		mesh_filename_ss << "_m" << i << ".ply";
		std::string mesh_filename = mesh_filename_ss.str();
		meshes[i]->write(mesh_filename.c_str());

	} // end add mesh filenames

	return true;

} // end save


