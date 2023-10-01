/*
 * This file is part of vitaGL
 * Copyright 2017, 2018, 2019, 2020 Rinnegatamante
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 *shaders.h:
 *Header file for default shaders related stuffs
 */

#ifndef _SHADERS_H_
#define _SHADERS_H_

// Clear shader
extern SceGxmShaderPatcherId clear_vertex_id;
extern SceGxmShaderPatcherId clear_fragment_id;
extern const SceGxmProgramParameter *clear_position;
extern const SceGxmProgramParameter *clear_depth;
extern const SceGxmProgramParameter *clear_color;
extern SceGxmVertexProgram *clear_vertex_program_patched;
extern SceGxmFragmentProgram *clear_fragment_program_patched;
extern SceGxmFragmentProgram *clear_fragment_program_float_patched;

// Framebuffer blit shader
extern SceGxmShaderPatcherId blit_vertex_id;
extern SceGxmShaderPatcherId blit_fragment_id;
extern const SceGxmProgramParameter *blit_position;
extern const SceGxmProgramParameter *blit_texcoord;
extern SceGxmVertexProgram *blit_vertex_program_patched;
extern SceGxmFragmentProgram *blit_fragment_program_patched;
extern SceGxmFragmentProgram *blit_fragment_program_float_patched;
#endif
