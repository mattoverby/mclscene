# mclscene

	By Matt Overby
	http://www.mattoverby.net

	mclscene is a generic scene loader for some of my test projects. It parses an XML file
	and stores metadata, which can then be extracted by another application for whatever purpose.
	It uses Trimesh2 as its base (http://gfx.cs.princeton.edu/proj/trimesh2) included in deps/.

	There is a viewer sample which can render the scene, but requires OpenGL and SFML 2.

# linking

	The best way to include mclscene in your project is to check it out in a subdirectory,
	call add_subdirectory, then use the ${MCLSCENE_LIBRARIES} and ${MCLSCENE_INCLUDE_DIRS}
	that are set during compilation.

# TODO

	- Cleanup headers
	- Render generic objects (sphere/box/plane/etc...)
	- Mesh instancing
	- Use parsed cameras in viewer
	- Use parsed lights in viewer
	- TetMesh::save
	- SceneManager::save

# license

Copyright 2014 Matthew Overby.

MCLSCENE Uses the BSD 2-Clause License (http://www.opensource.org/licenses/BSD-2-Clause)
Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list
of conditions and the following disclaimer in the documentation and/or other materials
provided with the distribution.
THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR  A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE UNIVERSITY OF MINNESOTA, DULUTH OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

By Matt Overby (http://www.mattoverby.net)
