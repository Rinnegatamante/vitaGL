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

extern "C" {
#include "vitaGL.h"
#include "shared.h"
#ifdef HAVE_RAZOR
extern razor_results razor_metrics;
#endif
};

#ifdef HAVE_DEBUG_INTERFACE
#ifndef HAVE_RAZOR_INTERFACE
#include "debug_font.h"
int dbg_y = 8;
uint32_t *frame_buf;

void vgl_debugger_draw_character(int character, int x, int y) {
    for (int yy = 0; yy < 10; yy++) {
        int xDisplacement = x;
        int yDisplacement = (y + (yy<<1)) * DISPLAY_STRIDE;
        uint32_t* screenPos = frame_buf + xDisplacement + yDisplacement;

        uint8_t charPos = font[character * 10 + yy];
        for (int xx = 7; xx >= 2; xx--) {
			uint32_t clr = ((charPos >> xx) & 1) ? 0xFFFFFFFF : 0x00000000;
			*(screenPos) = clr;
			*(screenPos+1) = clr;
			*(screenPos+DISPLAY_STRIDE) = clr;
			*(screenPos+DISPLAY_STRIDE+1) = clr;			
			screenPos += 2;
        }
    }
}

void vgl_debugger_draw_string(int x, int y, const char *str) {
    for (size_t i = 0; i < strlen(str); i++)
        vgl_debugger_draw_character(str[i], x + i * 12, y);
}

void vgl_debugger_draw_string_format(int x, int y, const char *format, ...) {
    char str[512] = { 0 };
    va_list va;

    va_start(va, format);
    vsnprintf(str, 512, format, va);
    va_end(va);

    for (char* text = strtok(str, "\n"); text != NULL; text = strtok(NULL, "\n"), y += 20)
        vgl_debugger_draw_string(x, y, text);
}
#endif

void vgl_debugger_draw_mem_usage(const char *str, vglMemType type) {
	uint32_t tot = vgl_mem_get_total_space(type) / (1024 * 1024);
	uint32_t used = tot - (vgl_mem_get_free_space(type) / (1024 * 1024));
	float ratio = ((float)used / (float)tot) * 100.0f;
#ifdef HAVE_RAZOR_INTERFACE
	ImGui::Text("%s: %luMBs / %luMBs (%.2f%%)", str, used, tot, ratio);
#else
	vgl_debugger_draw_string_format(5, dbg_y, "%s: %luMBs / %luMBs (%.2f%%)", str, used, tot, ratio);
	dbg_y += 20;
#endif
}

#ifndef HAVE_RAZOR_INTERFACE
void vgl_debugger_light_draw(int stride, uint32_t *fb) {
	frame_buf = fb;
	dbg_y = 8;
	vgl_debugger_draw_mem_usage("RAM Usage", VGL_MEM_RAM);
	vgl_debugger_draw_mem_usage("VRAM Usage", VGL_MEM_VRAM);
#ifndef PHYCONT_ON_DEMAND
	vgl_debugger_draw_mem_usage("Phycont RAM Usage", VGL_MEM_SLOW);
#endif
}
#endif
#endif

#ifdef HAVE_RAZOR_INTERFACE
#include <imgui_vita.h>
bool razor_dbg_window = true; // Current state for sceRazor debugger window
int metrics_mode = SCE_RAZOR_GPU_LIVE_METRICS_GROUP_PBUFFER_USAGE; // Current live metrics to show

#ifdef HAVE_DEVKIT
void vgl_debugger_set_metrics(int mode) {
	metrics_mode = mode;
	sceRazorGpuLiveStop();
	sceRazorGpuLiveSetMetricsGroup(metrics_mode);
	sceRazorGpuLiveStart();
	frame_idx = 0;
}
#endif

void vgl_debugger_init() {
	// Initializing dear ImGui
	ImGui::CreateContext();
	ImGui_ImplVitaGL_Init();
	ImGui_ImplVitaGL_TouchUsage(GL_TRUE);
	ImGui_ImplVitaGL_UseIndirectFrontTouch(GL_TRUE);
	ImGui::StyleColorsDark();
}

void vgl_debugger_draw() {
	// Initializing a new ImGui frame
	ImGui_ImplVitaGL_NewFrame();
		
	// Drawing debugging window
	ImGui::SetNextWindowPos(ImVec2(150, 20), ImGuiSetCond_Once);
	ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiSetCond_Once);
	ImGui::Begin("vitaGL debugger", &razor_dbg_window);
	if (ImGui::Button("Perform GPU capture")) {
		SceDateTime date;
		sceRtcGetCurrentClockLocalTime(&date);
		char titleid[16];
		sceAppMgrAppParamGetString(0, 12, titleid , 256);
		char fname[256];
		sprintf(fname, "ux0:data/cap_%s-%02d_%02d_%04d-%02d_%02d_%02d.sgx", titleid, date.day, date.month, date.year, date.hour, date.minute, date.second);
		sceRazorGpuCaptureSetTriggerNextFrame(fname);
	}
#ifdef HAVE_DEVKIT
	if (has_razor_live) {
		if (ImGui::BeginMenu("Change metrics type")) {
			if (ImGui::MenuItem("Param Buffer", nullptr, metrics_mode == SCE_RAZOR_GPU_LIVE_METRICS_GROUP_PBUFFER_USAGE)) {
				vgl_debugger_set_metrics(SCE_RAZOR_GPU_LIVE_METRICS_GROUP_PBUFFER_USAGE);
			}
			if (ImGui::MenuItem("USSE", nullptr, metrics_mode == SCE_RAZOR_GPU_LIVE_METRICS_GROUP_OVERVIEW_1)) {
				vgl_debugger_set_metrics(SCE_RAZOR_GPU_LIVE_METRICS_GROUP_OVERVIEW_1);
			}
			if (ImGui::MenuItem("I/O", nullptr, metrics_mode == SCE_RAZOR_GPU_LIVE_METRICS_GROUP_OVERVIEW_2)) {
				vgl_debugger_set_metrics(SCE_RAZOR_GPU_LIVE_METRICS_GROUP_OVERVIEW_2);
			}
			if (ImGui::MenuItem("Memory", nullptr, metrics_mode == SCE_RAZOR_GPU_LIVE_METRICS_GROUP_OVERVIEW_3)) {
				vgl_debugger_set_metrics(SCE_RAZOR_GPU_LIVE_METRICS_GROUP_OVERVIEW_3);
			}
			ImGui::EndMenu();
		}
		ImGui::Separator();
	
		ImGui::Text("Frame number: %d", razor_metrics.frameNumber);
		ImGui::Text("Frame duration: %dus", razor_metrics.frameDuration);
		ImGui::Text("GPU activity: %dus (%.0f%%)", razor_metrics.frameGpuActive, 100.f * razor_metrics.frameGpuActive / razor_metrics.frameDuration);
		ImGui::Text("Scenes per frame: %lu", razor_metrics.sceneCount);
		ImGui::Separator();
	
		switch (metrics_mode) {
		case SCE_RAZOR_GPU_LIVE_METRICS_GROUP_PBUFFER_USAGE:
			ImGui::Text("Partial Rendering: %s", razor_metrics.partialRender ? "Yes" : "No");
			ImGui::Text("Param Buffer Outage: %s", razor_metrics.vertexJobPaused ? "Yes" : "No");
			ImGui::Text("Param Buffer Peak Usage: %lu Bytes", razor_metrics.peakUsage);
			break;
		case SCE_RAZOR_GPU_LIVE_METRICS_GROUP_OVERVIEW_1:
			ImGui::Text("Vertex jobs: %d (Time: %lluus)", razor_metrics.vertexJobCount, razor_metrics.vertexJobTime / 4);
			ImGui::Text("USSE Vertex Processing: %.2f%%", razor_metrics.usseVertexProcessing / razor_metrics.vertexJobCount);
			ImGui::Separator();
			ImGui::Text("Fragment jobs: %d (Time: %lluus)", razor_metrics.fragmentJobCount, razor_metrics.fragmentJobTime / 4);
			ImGui::Text("USSE Fragment Processing: %.2f%%", razor_metrics.usseFragmentProcessing / razor_metrics.fragmentJobCount);
			ImGui::Separator();
			ImGui::Text("USSE Dependent Texture Read: %.2f%%", razor_metrics.usseDependentTextureReadRequest / razor_metrics.fragmentJobCount);
			ImGui::Text("USSE Non-Dependent Texture Read: %.2f%%", razor_metrics.usseNonDependentTextureReadRequest / razor_metrics.fragmentJobCount);
			ImGui::Separator();
			ImGui::Text("Firmware jobs: %d (Time: %lluus)", razor_metrics.firmwareJobCount, razor_metrics.firmwareJobTime / 4);
			break;
		case SCE_RAZOR_GPU_LIVE_METRICS_GROUP_OVERVIEW_2:
			ImGui::Text("Vertex jobs: %d (Time: %lluus)", razor_metrics.vertexJobCount, razor_metrics.vertexJobTime / 4);
			ImGui::Text("VDM primitives (Input): %d", razor_metrics.vdmPrimitivesInput);
			ImGui::Text("MTE primitives (Output): %d", razor_metrics.mtePrimitivesOutput);
			ImGui::Text("VDM vertices (Input): %d", razor_metrics.vdmVerticesInput);
			ImGui::Text("MTE vertices (Output): %d", razor_metrics.mteVerticesOutput);
			ImGui::Separator();
			ImGui::Text("Fragment jobs: %d (Time: %lluus)", razor_metrics.fragmentJobCount, razor_metrics.fragmentJobTime / 4);
			ImGui::Text("Rasterized pixels before HSR: %d", razor_metrics.rasterizedPixelsBeforeHsr);
			ImGui::Text("Rasterized output pixels: %d", razor_metrics.rasterizedOutputPixels);
			ImGui::Text("Rasterized output samples: %d", razor_metrics.rasterizedOutputSamples);
			ImGui::Separator();
			ImGui::Text("Firmware jobs: %d (Time: %lluus)", razor_metrics.firmwareJobCount, razor_metrics.firmwareJobTime / 4);
			break;
		case SCE_RAZOR_GPU_LIVE_METRICS_GROUP_OVERVIEW_3:
			ImGui::Text("Vertex jobs: %d (Time: %lluus)", razor_metrics.vertexJobCount, razor_metrics.vertexJobTime / 4);
			ImGui::Text("BIF: Tiling accelerated memory writes: %d bytes", razor_metrics.bifTaMemoryWrite);
			ImGui::Separator();
			ImGui::Text("Fragment jobs: %d (Time: %lluus)", razor_metrics.fragmentJobCount, razor_metrics.fragmentJobTime / 4);
			ImGui::Text("BIF: ISP parameter fetch memory reads: %d bytes", razor_metrics.bifIspParameterFetchMemoryRead);
			ImGui::Separator();
			ImGui::Text("Firmware jobs: %d (Time: %lluus)", razor_metrics.firmwareJobCount, razor_metrics.firmwareJobTime / 4);
			break;
		default:
			break;
		}
		ImGui::Separator();
	}
#endif
	
	vgl_debugger_draw_mem_usage("RAM Usage", VGL_MEM_RAM);
	vgl_debugger_draw_mem_usage("VRAM Usage", VGL_MEM_VRAM);
#ifndef PHYCONT_ON_DEMAND
	vgl_debugger_draw_mem_usage("Phycont RAM Usage", VGL_MEM_SLOW);
#endif
		
	ImGui::End();
	
	// Invalidating current GL machine state
	GLuint program_bkp = 0;
	if (cur_program) {
		program_bkp = cur_program;
		glUseProgram(0);
	}
	GLboolean blend_state_bkp = blend_state;
	SceGxmBlendFactor sfactor_bkp = blend_sfactor_rgb;
	SceGxmBlendFactor dfactor_bkp = blend_dfactor_rgb;
	GLboolean cull_face_state_bkp = cull_face_state;
	GLboolean depth_test_state_bkp = depth_test_state;
	GLboolean scissor_test_state_bkp = scissor_test_state;
	
	// Sending job to ImGui renderer
	glViewport(0, 0, static_cast<int>(ImGui::GetIO().DisplaySize.x), static_cast<int>(ImGui::GetIO().DisplaySize.y));
	ImGui::Render();
	ImGui_ImplVitaGL_RenderDrawData(ImGui::GetDrawData());
	
	// Restoring invalidated GL machine state
	if (program_bkp)
		glUseProgram(program_bkp);
	blend_sfactor_rgb = blend_sfactor_a = sfactor_bkp;
	blend_dfactor_rgb = blend_dfactor_a = dfactor_bkp;
	if (!blend_state_bkp)
		glDisable(GL_BLEND);
	else
		change_blend_factor();
	if (!scissor_test_state_bkp)
		glDisable(GL_SCISSOR_TEST);
	if (depth_test_state_bkp)
		glEnable(GL_DEPTH_TEST);
	if (cull_face_state_bkp)
		glEnable(GL_CULL_FACE);
}
#endif

#ifdef FILE_LOG
void vgl_file_log(const char *format, ...) {
	__gnuc_va_list arg;
	va_start(arg, format);
	char msg[512];
	vsprintf(msg, format, arg);
	va_end(arg);
	FILE *log = fopen("ux0:/data/vitaGL.log", "a+");
	if (log != NULL) {
		fwrite(msg, 1, strlen(msg), log);
		fclose(log);
	}
}
#endif

#ifdef LOG_ERRORS
char *get_gl_error_literal(uint32_t code) {
	switch (code) {
	case GL_INVALID_VALUE:
		return "GL_INVALID_VALUE";
	case GL_INVALID_ENUM:
		return "GL_INVALID_ENUM";
	case GL_INVALID_OPERATION:
		return "GL_INVALID_OPERATION";
	case GL_OUT_OF_MEMORY:
		return "GL_OUT_OF_MEMORY";
	case GL_STACK_OVERFLOW:
		return "GL_STACK_OVERFLOW";
	case GL_STACK_UNDERFLOW:
		return "GL_STACK_UNDERFLOW";
	default:
		return "Unknown Error";
	}
}

char *get_gxm_error_literal(uint32_t code) {
	switch (code) {
	case SCE_GXM_ERROR_UNINITIALIZED:
		return "SCE_GXM_ERROR_UNINITIALIZED";
	case SCE_GXM_ERROR_ALREADY_INITIALIZED:
		return "SCE_GXM_ERROR_ALREADY_INITIALIZED";
	case SCE_GXM_ERROR_OUT_OF_MEMORY:
		return "SCE_GXM_ERROR_OUT_OF_MEMORY";
	case SCE_GXM_ERROR_INVALID_VALUE:
		return "SCE_GXM_ERROR_INVALID_VALUE";
	case SCE_GXM_ERROR_INVALID_POINTER:
		return "SCE_GXM_ERROR_INVALID_POINTER";
	case SCE_GXM_ERROR_INVALID_ALIGNMENT:
		return "SCE_GXM_ERROR_INVALID_ALIGNMENT";
	case SCE_GXM_ERROR_NOT_WITHIN_SCENE:
		return "SCE_GXM_ERROR_NOT_WITHIN_SCENE";
	case SCE_GXM_ERROR_WITHIN_SCENE:
		return "SCE_GXM_ERROR_WITHIN_SCENE";
	case SCE_GXM_ERROR_NULL_PROGRAM:
		return "SCE_GXM_ERROR_NULL_PROGRAM";
	case SCE_GXM_ERROR_UNSUPPORTED:
		return "SCE_GXM_ERROR_UNSUPPORTED";
	case SCE_GXM_ERROR_PATCHER_INTERNAL:
		return "SCE_GXM_ERROR_PATCHER_INTERNAL";
	case SCE_GXM_ERROR_RESERVE_FAILED:
		return "SCE_GXM_ERROR_RESERVE_FAILED";
	case SCE_GXM_ERROR_PROGRAM_IN_USE:
		return "SCE_GXM_ERROR_PROGRAM_IN_USE";
	case SCE_GXM_ERROR_INVALID_INDEX_COUNT:
		return "SCE_GXM_ERROR_INVALID_INDEX_COUNT";
	case SCE_GXM_ERROR_INVALID_POLYGON_MODE:
		return "SCE_GXM_ERROR_INVALID_POLYGON_MODE";
	case SCE_GXM_ERROR_INVALID_SAMPLER_RESULT_TYPE_PRECISION:
		return "SCE_GXM_ERROR_INVALID_SAMPLER_RESULT_TYPE_PRECISION";
	case SCE_GXM_ERROR_INVALID_SAMPLER_RESULT_TYPE_COMPONENT_COUNT:
		return "SCE_GXM_ERROR_INVALID_SAMPLER_RESULT_TYPE_COMPONENT_COUNT";
	case SCE_GXM_ERROR_UNIFORM_BUFFER_NOT_RESERVED:
		return "SCE_GXM_ERROR_UNIFORM_BUFFER_NOT_RESERVED";
	case SCE_GXM_ERROR_INVALID_AUXILIARY_SURFACE:
		return "SCE_GXM_ERROR_INVALID_AUXILIARY_SURFACE";
	case SCE_GXM_ERROR_INVALID_PRECOMPUTED_DRAW:
		return "SCE_GXM_ERROR_INVALID_PRECOMPUTED_DRAW";
	case SCE_GXM_ERROR_INVALID_PRECOMPUTED_VERTEX_STATE:
		return "SCE_GXM_ERROR_INVALID_PRECOMPUTED_VERTEX_STATE";
	case SCE_GXM_ERROR_INVALID_PRECOMPUTED_FRAGMENT_STATE:
		return "SCE_GXM_ERROR_INVALID_PRECOMPUTED_FRAGMENT_STATE";
	case SCE_GXM_ERROR_DRIVER:
		return "SCE_GXM_ERROR_DRIVER";
	case SCE_GXM_ERROR_INVALID_TEXTURE:
		return "SCE_GXM_ERROR_INVALID_TEXTURE";
	case SCE_GXM_ERROR_INVALID_TEXTURE_DATA_POINTER:
		return "SCE_GXM_ERROR_INVALID_TEXTURE_DATA_POINTER";
	case SCE_GXM_ERROR_INVALID_TEXTURE_PALETTE_POINTER:
		return "SCE_GXM_ERROR_INVALID_TEXTURE_PALETTE_POINTER";
	default:
		return "Unknown Error";
	}
}
#endif
