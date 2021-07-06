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
 *
 */
#include "vitaGL.h"
#include "shared.h"

// EGL implementation

EGLBoolean eglSwapInterval(EGLDisplay display, EGLint interval) {
	vsync_interval = interval;
}

EGLBoolean eglSwapBuffers(EGLDisplay display, EGLSurface surface) {
	vglSwapBuffers(GL_FALSE);
}
