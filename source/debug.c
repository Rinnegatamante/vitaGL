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

#include "vitaGL.h"
#include "shared.h"
#ifdef HAVE_RAZOR
extern razor_results razor_metrics;
#endif

void glPushGroupMarker(GLsizei length, const GLchar *marker) {
	sceGxmPushUserMarker(gxm_context, marker);
}

void glPopGroupMarker(void) {
	sceGxmPopUserMarker(gxm_context);
}

#ifdef HAVE_DEBUG_INTERFACE
#include "utils/font_utils.h"
static int dbg_y = -18;
static uint32_t *frame_buf;
#if !defined(DISABLE_CIRCULAR_POOL) && !defined(CIRCULAR_POOL_SPEEDHACK)
uint32_t vgl_circular_pool_frame_peak = 0;
uint32_t vgl_circular_pool_global_peak = 0;
#endif

#ifdef HAVE_DEVKIT
int metrics_mode = SCE_RAZOR_GPU_LIVE_METRICS_GROUP_PBUFFER_USAGE; // Current live metrics to show

void vgl_debugger_set_metrics(int mode) {
	metrics_mode = mode;
	sceRazorGpuLiveStop();
	sceRazorGpuLiveSetMetricsGroup(mode);
	sceRazorGpuLiveStart();
}
#endif

static void vgl_debugger_draw_character(int character, int x, int y, uint32_t color) {
	for (int yy = 0; yy < 10; yy++) {
		int xDisplacement = x;
		int yDisplacement = (y + (yy<<1)) * DISPLAY_STRIDE;
		uint32_t* screenPos = frame_buf + xDisplacement + yDisplacement;

		uint8_t charPos = font[character * 10 + yy];
		for (int xx = 7; xx >= 2; xx--) {
			uint32_t clr = ((charPos >> xx) & 1) ? color : 0x00000000;
			*(screenPos) = clr;
			*(screenPos+1) = clr;
			*(screenPos+DISPLAY_STRIDE) = clr;
			*(screenPos+DISPLAY_STRIDE+1) = clr;			
			screenPos += 2;
		}
	}
}

static void vgl_debugger_draw_string(int x, int y, const char *str, uint32_t color) {
	for (size_t i = 0; i < strlen(str); i++)
		vgl_debugger_draw_character(str[i], x + i * 12, y, color);
}

static void vgl_debugger_draw_string_format(int x, int y, uint32_t color, const char *format, ...) {
	char str[512] = { 0 };
	va_list va;

	va_start(va, format);
	vsnprintf(str, 512, format, va);
	va_end(va);

	for (char* text = strtok(str, "\n"); text != NULL; text = strtok(NULL, "\n"), y += 20)
		vgl_debugger_draw_string(x, y, text, color);
}

static inline __attribute__((always_inline)) uint32_t vgl_debugger_get_color_by_percentage(uint32_t percent) {
	if (percent > 80) {
		return 0xFF0000FF;
	} else if (percent > 55) {
		return 0xFF00FFFF;
	}
	return 0xFFFFFFFF;
}

static void vgl_debugger_draw_mem_usage(const char *str, vglMemType type) {
	uint32_t tot = vgl_mem_get_total_space(type) / (1024 * 1024);
	uint32_t used = tot - (vgl_mem_get_free_space(type) / (1024 * 1024));
	float ratio = ((float)used / (float)tot) * 100.0f;
	vgl_debugger_draw_string_format(5, dbg_y += 20, vgl_debugger_get_color_by_percentage(ratio), "%s: %luMBs / %luMBs (%.2f%%)", str, used, tot, ratio);
}

static inline __attribute__((always_inline)) void vgl_debugger_draw_mem_usage_metrics() {
	vgl_debugger_draw_mem_usage("RAM Usage", VGL_MEM_RAM);
	vgl_debugger_draw_mem_usage("VRAM Usage", VGL_MEM_VRAM);
	vgl_debugger_draw_mem_usage("Phycont RAM Usage", VGL_MEM_SLOW);
	vgl_debugger_draw_mem_usage("CDLG RAM Usage", VGL_MEM_BUDGET);
}

void vgl_debugger_draw(uint32_t *fb) {
	frame_buf = fb;
	dbg_y = -18;
	float percentage;
#ifdef HAVE_DEVKIT
	if (has_razor_live) {
		static uint32_t param_buf_peak = 0;
		vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFF00FF, "Page %d/%d", metrics_mode + 1, SCE_RAZOR_GPU_LIVE_METRICS_GROUP_NUM);
		switch (metrics_mode) {
		case SCE_RAZOR_GPU_LIVE_METRICS_GROUP_PBUFFER_USAGE:
			if (razor_metrics.peak_usage_value > param_buf_peak) {
				param_buf_peak = razor_metrics.peak_usage_value;
			}
			vgl_debugger_draw_mem_usage_metrics();
#if !defined(DISABLE_CIRCULAR_POOL) && !defined(CIRCULAR_POOL_SPEEDHACK)
			percentage = 100.f * ((float)vgl_circular_pool_frame_peak / (circular_data_pool_size / gxm_display_buffer_count));
			vgl_debugger_draw_string_format(5, dbg_y += 20, vgl_debugger_get_color_by_percentage(percentage), "Circular Pool Usage: %lu Bytes (%.0f%%)", vgl_circular_pool_frame_peak, percentage);
			percentage = 100.f * ((float)vgl_circular_pool_global_peak / (circular_data_pool_size / gxm_display_buffer_count));
			vgl_debugger_draw_string_format(5, dbg_y += 20, vgl_debugger_get_color_by_percentage(percentage), "Circular Pool Peak Usage: %lu Bytes (%.0f%%)", vgl_circular_pool_global_peak, percentage);
#endif
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "SP Buffer Mem Usage: %luKBs", sceGxmShaderPatcherGetBufferMemAllocated(gxm_shader_patcher) / 1024);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "SP Fragment USSE Mem Usage: %luKBs", sceGxmShaderPatcherGetFragmentUsseMemAllocated(gxm_shader_patcher) / 1024);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "SP Vertex USSE Mem Usage: %luKBs", sceGxmShaderPatcherGetVertexUsseMemAllocated(gxm_shader_patcher) / 1024);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "SP Host Mem Usage: %luKBs", sceGxmShaderPatcherGetHostMemAllocated(gxm_shader_patcher) / 1024);
			percentage = 100.f * razor_metrics.gpu_activity_duration_time / razor_metrics.frame_duration;
			vgl_debugger_draw_string_format(5, dbg_y += 20, vgl_debugger_get_color_by_percentage(percentage), "GPU activity: %dus (%.0f%%)", razor_metrics.gpu_activity_duration_time, percentage);
			vgl_debugger_draw_string_format(5, dbg_y += 20, razor_metrics.partial_render ? 0xFF0000FF : 0xFFFFFFFF, "Partial Rendering: %s", razor_metrics.partial_render ? "Yes" : "No");
			vgl_debugger_draw_string_format(5, dbg_y += 20, razor_metrics.vertex_job_paused ? 0xFF0000FF : 0xFFFFFFFF, "Param Buffer Outage: %s", razor_metrics.vertex_job_paused ? "Yes" : "No");
			percentage = 100.f * ((float)param_buf_peak / (float)gxm_param_buf_size);
			vgl_debugger_draw_string_format(5, dbg_y += 20, vgl_debugger_get_color_by_percentage(percentage), "Param Buffer Peak Usage: %lu Bytes (%.0f%%)", param_buf_peak, percentage);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "Scenes per frame: %lu", razor_metrics.scene_count);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "Frame Number: %lu", razor_metrics.frame_number);
			break;
		case SCE_RAZOR_GPU_LIVE_METRICS_GROUP_OVERVIEW_1:
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "Vertex jobs: %d (Time: %lluus)", razor_metrics.vertex_job_count, razor_metrics.vertex_job_time / 4);
			percentage = razor_metrics.usse_vertex_processing_percent / razor_metrics.vertex_job_count;
			vgl_debugger_draw_string_format(5, dbg_y += 20, vgl_debugger_get_color_by_percentage(percentage), "USSE Vertex Processing: %.2f%%", percentage);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "Fragment jobs: %d (Time: %lluus)", razor_metrics.fragment_job_count, razor_metrics.fragment_job_time / 4);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "USSE Fragment Processing: %.2f%%", razor_metrics.usse_fragment_processing_percent / razor_metrics.fragment_job_count);
			percentage = razor_metrics.usse_dependent_texture_reads_percent / razor_metrics.fragment_job_count;
			vgl_debugger_draw_string_format(5, dbg_y += 20, vgl_debugger_get_color_by_percentage(percentage), "USSE Dependent Texture Read: %.2f%%", percentage);
			percentage = razor_metrics.usse_non_dependent_texture_reads_percent / razor_metrics.fragment_job_count;
			vgl_debugger_draw_string_format(5, dbg_y += 20, vgl_debugger_get_color_by_percentage(percentage), "USSE Non-Dependent Texture Read: %.2f%%", percentage);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "Firmware jobs: %d (Time: %lluus)", razor_metrics.firmware_job_count, razor_metrics.firmware_job_time / 4);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "Scenes per frame: %lu", razor_metrics.scene_count);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "Frame Number: %lu", razor_metrics.frame_number);
			break;
		case SCE_RAZOR_GPU_LIVE_METRICS_GROUP_OVERVIEW_2:
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "Vertex jobs: %d (Time: %lluus)", razor_metrics.vertex_job_count, razor_metrics.vertex_job_time / 4);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "VDM primitives (Input): %d", razor_metrics.vdm_primitives_input_num);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "MTE primitives (Output): %d", razor_metrics.mte_primitives_output_num);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "VDM vertices (Input): %d", razor_metrics.vdm_vertices_input_num);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "MTE vertices (Output): %d", razor_metrics.mte_vertices_output_num);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "Fragment jobs: %d (Time: %lluus)", razor_metrics.fragment_job_count, razor_metrics.fragment_job_time / 4);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "Rasterized pixels before HSR: %d", razor_metrics.rasterized_pixels_before_hsr_num);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "Rasterized output pixels: %d", razor_metrics.rasterized_output_pixels_num);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "Rasterized output samples: %d", razor_metrics.rasterized_output_samples_num);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "Firmware jobs: %d (Time: %lluus)", razor_metrics.firmware_job_count, razor_metrics.firmware_job_time / 4);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "Scenes per frame: %lu", razor_metrics.scene_count);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "Frame Number: %lu", razor_metrics.frame_number);
			break;
		case SCE_RAZOR_GPU_LIVE_METRICS_GROUP_OVERVIEW_3:
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "Vertex jobs: %d (Time: %lluus)", razor_metrics.vertex_job_count, razor_metrics.vertex_job_time / 4);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "BIF: Tiling accelerated memory writes: %d bytes", razor_metrics.tiling_accelerated_mem_writes);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "Fragment jobs: %d (Time: %lluus)", razor_metrics.fragment_job_count, razor_metrics.fragment_job_time / 4);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "BIF: ISP parameter fetch memory reads: %d bytes", razor_metrics.isp_parameter_fetches_mem_reads);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "Firmware jobs: %d (Time: %lluus)", razor_metrics.firmware_job_count, razor_metrics.firmware_job_time / 4);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "Scenes per frame: %lu", razor_metrics.scene_count);
			vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "Frame Number: %lu", razor_metrics.frame_number);
			break;
		}
		static uint32_t oldpad = 0;
		static SceCtrlData pad;
		sceCtrlPeekBufferPositive(0, &pad, 1);
		if ((pad.buttons & SCE_CTRL_LEFT) && !(oldpad & SCE_CTRL_LEFT)) {
			metrics_mode--;
			if (metrics_mode < 0) {
				metrics_mode = SCE_RAZOR_GPU_LIVE_METRICS_GROUP_NUM - 1;
			}
			vgl_debugger_set_metrics(metrics_mode);
		} else if ((pad.buttons & SCE_CTRL_RIGHT) && !(oldpad & SCE_CTRL_RIGHT)) {
			metrics_mode = (metrics_mode + 1) % SCE_RAZOR_GPU_LIVE_METRICS_GROUP_NUM;
			vgl_debugger_set_metrics(metrics_mode);
		}
		oldpad = pad.buttons;
		return;
	}
#endif
	vgl_debugger_draw_mem_usage_metrics();
#if !defined(DISABLE_CIRCULAR_POOL) && !defined(CIRCULAR_POOL_SPEEDHACK)
	percentage = 100.f * ((float)vgl_circular_pool_frame_peak / (circular_data_pool_size / gxm_display_buffer_count));
	vgl_debugger_draw_string_format(5, dbg_y += 20, vgl_debugger_get_color_by_percentage(percentage), "Circular Pool Usage: %lu Bytes (%.0f%%)", vgl_circular_pool_frame_peak, percentage);
	percentage = 100.f * ((float)vgl_circular_pool_global_peak / (circular_data_pool_size / gxm_display_buffer_count));
	vgl_debugger_draw_string_format(5, dbg_y += 20, vgl_debugger_get_color_by_percentage(percentage), "Circular Pool Peak Usage: %lu Bytes (%.0f%%)", vgl_circular_pool_global_peak, percentage);
#endif	
	vgl_debugger_draw_string_format(5, dbg_y += 20, 0xFFFFFFFF, "Frame Number: %lu", vgl_framecount);
}
#endif

#ifdef FILE_LOG
static char msg[512 * 1024];
void vgl_file_log(const char *format, ...) {
	__gnuc_va_list arg;
	va_start(arg, format);
	vsnprintf(msg, sizeof(msg), format, arg);
	va_end(arg);
	SceUID log = sceIoOpen("ux0:/data/vitaGL.log", SCE_O_WRONLY | SCE_O_APPEND | SCE_O_CREAT, 0777);
	if (log >= 0) {
		sceIoWrite(log, msg, strlen(msg));
		sceIoClose(log);
	}
}
#endif

#ifdef LOG_ERRORS
#define ERROR_CASE(x) \
	case x: \
		return #x;
char *get_gxm_error_literal(uint32_t code) {
	switch (code) {
	ERROR_CASE(SCE_GXM_ERROR_UNINITIALIZED)
	ERROR_CASE(SCE_GXM_ERROR_ALREADY_INITIALIZED)
	ERROR_CASE(SCE_GXM_ERROR_OUT_OF_MEMORY)
	ERROR_CASE(SCE_GXM_ERROR_INVALID_VALUE)
	ERROR_CASE(SCE_GXM_ERROR_INVALID_POINTER)
	ERROR_CASE(SCE_GXM_ERROR_INVALID_ALIGNMENT)
	ERROR_CASE(SCE_GXM_ERROR_NOT_WITHIN_SCENE)
	ERROR_CASE(SCE_GXM_ERROR_WITHIN_SCENE)
	ERROR_CASE(SCE_GXM_ERROR_NULL_PROGRAM)
	ERROR_CASE(SCE_GXM_ERROR_UNSUPPORTED)
	ERROR_CASE(SCE_GXM_ERROR_PATCHER_INTERNAL)
	ERROR_CASE(SCE_GXM_ERROR_RESERVE_FAILED)
	ERROR_CASE(SCE_GXM_ERROR_PROGRAM_IN_USE)
	ERROR_CASE(SCE_GXM_ERROR_INVALID_INDEX_COUNT)
	ERROR_CASE(SCE_GXM_ERROR_INVALID_POLYGON_MODE)
	ERROR_CASE(SCE_GXM_ERROR_INVALID_SAMPLER_RESULT_TYPE_PRECISION)
	ERROR_CASE(SCE_GXM_ERROR_INVALID_SAMPLER_RESULT_TYPE_COMPONENT_COUNT)
	ERROR_CASE(SCE_GXM_ERROR_UNIFORM_BUFFER_NOT_RESERVED)
	ERROR_CASE(SCE_GXM_ERROR_INVALID_PRECOMPUTED_DRAW)
	ERROR_CASE(SCE_GXM_ERROR_INVALID_PRECOMPUTED_VERTEX_STATE)
	ERROR_CASE(SCE_GXM_ERROR_INVALID_PRECOMPUTED_FRAGMENT_STATE)
	ERROR_CASE(SCE_GXM_ERROR_DRIVER)
	ERROR_CASE(SCE_GXM_ERROR_INVALID_TEXTURE)
	ERROR_CASE(SCE_GXM_ERROR_INVALID_TEXTURE_DATA_POINTER)
	ERROR_CASE(SCE_GXM_ERROR_INVALID_TEXTURE_PALETTE_POINTER)
	ERROR_CASE(SCE_GXM_ERROR_OUT_OF_RENDER_TARGETS)
	default:
		return "Unknown Error";
	}
}
#endif
