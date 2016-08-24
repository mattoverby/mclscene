# mclscene

By Matt Overby  
[http://www.mattoverby.net](http://www.mattoverby.net)

mclscene is a generic scene loader for prototyping my projects. It parses an XML scene file
and creates typical 3D related stuff (e.g. meshes, lights, materials, etc...)
Included are distributions of:
trimesh2 (Szymon Rusinkiewicz, gfx.cs.princeton.edu/proj/trimesh2),
tetgen (Hang Si, wias-berlin.de/software/tetgen),
pugixml (Arseny Kapoulkine, pugixml.org), and
soil (Jonathan Dummer, lonesock.net/soil.html).
Currently it only stores triangle/tetrahedral meshes and point clouds.

There is a viewer which can render the scene using OpenGL and GLFW3.

<img src="https://github.com/over0219/mclscene/raw/master/doc/dillo.png" width="512">

# linking

The best way to include mclscene in your project is to add it as a submodule with git,
use cmake to call add_subdirectory, then use the ${MCLSCENE_LIBRARIES} and
${MCLSCENE_INCLUDE_DIRS} that are set during the build. See CMakeLists.txt for more details.
I will improve this in the future with a proper FindMCLSCENE.cmake.

# todo

- Cleanup headers and make it header-only
- Cleanup object base class and hierarchy
- Support for texture wrapping
- Support for parsing new components (not just the 4 I use)
- Mesh instancing
- Export to other renderers (e.g., mitsuba, blender)

# license

Copyright 2016 Matthew Overby.

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
