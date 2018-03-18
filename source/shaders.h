/* 
 * shaders.h:
 * Header file for default shaders related stuffs
 */


// Disable color buffer shader
SceGxmShaderPatcherId disable_color_buffer_vertex_id;
SceGxmShaderPatcherId disable_color_buffer_fragment_id;
const SceGxmProgramParameter* disable_color_buffer_position;
const SceGxmProgramParameter* disable_color_buffer_matrix;
SceGxmVertexProgram* disable_color_buffer_vertex_program_patched;
SceGxmFragmentProgram* disable_color_buffer_fragment_program_patched;

// Clear shader
SceGxmShaderPatcherId clear_vertex_id;
SceGxmShaderPatcherId clear_fragment_id;
const SceGxmProgramParameter* clear_position;
const SceGxmProgramParameter* clear_color;
SceGxmVertexProgram *clear_vertex_program_patched; // Patched vertex program for clearing screen
SceGxmFragmentProgram* clear_fragment_program_patched;

// Color (RGBA/RGB) shader
SceGxmShaderPatcherId rgba_vertex_id;
SceGxmShaderPatcherId rgb_vertex_id;
SceGxmShaderPatcherId rgba_fragment_id;
const SceGxmProgramParameter* rgba_position;
const SceGxmProgramParameter* rgba_color;
const SceGxmProgramParameter* rgba_wvp;
const SceGxmProgramParameter* rgb_position;
const SceGxmProgramParameter* rgb_color;
const SceGxmProgramParameter* rgb_wvp;
SceGxmVertexProgram* rgba_vertex_program_patched;
SceGxmVertexProgram* rgb_vertex_program_patched;
SceGxmFragmentProgram* rgba_fragment_program_patched;
const SceGxmProgram* rgba_fragment_program;

// Texture2D shader
SceGxmShaderPatcherId texture2d_vertex_id;
SceGxmShaderPatcherId texture2d_fragment_id;
const SceGxmProgramParameter* texture2d_position;
const SceGxmProgramParameter* texture2d_texcoord;
const SceGxmProgramParameter* texture2d_wvp;
const SceGxmProgramParameter* texture2d_alpha_cut;
const SceGxmProgramParameter* texture2d_alpha_op;
const SceGxmProgramParameter* texture2d_tint_color;
const SceGxmProgramParameter* texture2d_tex_env;
const SceGxmProgramParameter* texture2d_tex_env_color;
SceGxmVertexProgram* texture2d_vertex_program_patched;
SceGxmFragmentProgram* texture2d_fragment_program_patched;
const SceGxmProgram* texture2d_fragment_program;

// Texture2D+RGBA shader
SceGxmShaderPatcherId texture2d_rgba_vertex_id;
SceGxmShaderPatcherId texture2d_rgba_fragment_id;
const SceGxmProgramParameter* texture2d_rgba_position;
const SceGxmProgramParameter* texture2d_rgba_texcoord;
const SceGxmProgramParameter* texture2d_rgba_wvp;
const SceGxmProgramParameter* texture2d_rgba_alpha_cut;
const SceGxmProgramParameter* texture2d_rgba_alpha_op;
const SceGxmProgramParameter* texture2d_rgba_color;
const SceGxmProgramParameter* texture2d_rgba_tex_env;
const SceGxmProgramParameter* texture2d_rgba_tex_env_color;
SceGxmVertexProgram* texture2d_rgba_vertex_program_patched;
SceGxmFragmentProgram* texture2d_rgba_fragment_program_patched;
const SceGxmProgram* texture2d_rgba_fragment_program;
