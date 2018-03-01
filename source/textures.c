/* 
 * textures.c:
 * Implementation for textures related functions
 */
 
 #include "shared.h"
 
 texture_unit texture_units[GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS]; // Available texture units
 palette *color_table = NULL; // Current in-use color table
 
 /*
 * ------------------------------
 * - IMPLEMENTATION STARTS HERE -
 * ------------------------------
 */
 
 void glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * data){
	
	// Setting some aliases to make code more readable
	texture_unit *tex_unit = &texture_units[server_texture_unit];
	int texture2d_idx = tex_unit->tex_id;
	texture *tex = &tex_unit->textures[texture2d_idx];
	
	SceGxmTextureFormat tex_format;
	uint8_t data_bpp = 0;
	
	// Support for legacy GL1.0 internalFormat
	switch (internalFormat){
	case 1:
		internalFormat = GL_RED;
		break;
	case 2:
		internalFormat = GL_RG;
		break;
	case 3:
		internalFormat = GL_RGB;
		break;
	case 4:
		internalFormat = GL_RGBA;
		break;
	}
	
	/*
	 * Callbacks are actually used to just perform down/up-sampling
	 * between U8 texture formats. Reads are expected to give as result
	 * a RGBA sample that will be wrote depending on texture format
	 * by the write callback
	 */
	void (*write_cb)(void*, uint32_t) = NULL;
	uint32_t (*read_cb)(void*) = NULL;
	
	// Detecting proper read callaback and source bpp
	switch (format){
	case GL_RED:
	case GL_ALPHA:
		switch (type){
		case GL_UNSIGNED_BYTE:
			read_cb = readR;
			data_bpp = 1;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
		}
		break;
	case GL_RG:
		switch (type){
		case GL_UNSIGNED_BYTE:
			read_cb = readRG;
			data_bpp = 2;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
		}
		break;
	case GL_RGB:
		switch (type){
		case GL_UNSIGNED_BYTE:
			data_bpp = 3;
			read_cb = readRGB;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
		}
		break;
	case GL_RGBA:
		switch (type){
		case GL_UNSIGNED_BYTE:
			data_bpp = 4;
			read_cb = readRGBA;
			break;
		case GL_UNSIGNED_SHORT_5_5_5_1:
			data_bpp = 2;
			read_cb = readRGBA5551;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
		}
		break;
	}
	
	
	switch (target){
		case GL_TEXTURE_2D:
		
			// Detecting proper write callback and texture format
			switch (internalFormat){
			case GL_RGB:
				write_cb = writeRGB;
				tex_format = SCE_GXM_TEXTURE_FORMAT_U8U8U8_BGR;
				break;
			case GL_RGBA:
				write_cb = writeRGBA;
				tex_format = SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR;
				break;
			case GL_LUMINANCE:
				write_cb = writeR;
				tex_format = SCE_GXM_TEXTURE_FORMAT_L8;
				break;
			case GL_LUMINANCE_ALPHA:
				write_cb = writeRA;
				tex_format = SCE_GXM_TEXTURE_FORMAT_A8L8;
				break;
			case GL_INTENSITY:
				write_cb = writeR;
				tex_format = SCE_GXM_TEXTURE_FORMAT_U8_RRRR;
				break;
			case GL_ALPHA:
				write_cb = writeR;
				tex_format = SCE_GXM_TEXTURE_FORMAT_A8;
				break;
			case GL_COLOR_INDEX8_EXT:
				write_cb = writeR; // TODO: This is a hack
				tex_format = SCE_GXM_TEXTURE_FORMAT_P8_ABGR;
				break;
			default:
				error = GL_INVALID_ENUM;
				break;
			}
			
			// Checking if texture is too big for sceGxm
			if (width > GXM_TEX_MAX_SIZE || height > GXM_TEX_MAX_SIZE){
				error = GL_INVALID_VALUE;
				return;
			}
			
			// Allocating texture/mipmaps depending on user call
			tex->type = internalFormat;
			tex->write_cb = write_cb;
			if (level == 0) gpu_alloc_texture(width, height, tex_format, data, tex, data_bpp, read_cb, write_cb);
			else gpu_alloc_mipmaps(level, tex);
			
			// Setting texture parameters
			sceGxmTextureSetUAddrMode(&tex->gxm_tex, tex_unit->u_mode);
			sceGxmTextureSetVAddrMode(&tex->gxm_tex, tex_unit->v_mode);
			sceGxmTextureSetMinFilter(&tex->gxm_tex, tex_unit->min_filter);
			sceGxmTextureSetMagFilter(&tex->gxm_tex, tex_unit->mag_filter);
			
			// Setting palette if the format requests one
			if (tex->valid && tex->palette_UID) sceGxmTextureSetPalette(&tex->gxm_tex, color_table->data);
			
			break;
		default:
			error = GL_INVALID_ENUM;
			break;	
	}
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels){
	
	// Setting some aliases to make code more readable
	texture_unit* tex_unit = &texture_units[server_texture_unit];
	int texture2d_idx = tex_unit->tex_id;
	texture* target_texture = &tex_unit->textures[texture2d_idx];
	
	// Calculating implicit texture stride and start address of requested texture modification
	SceGxmTextureFormat tex_format = sceGxmTextureGetFormat(&target_texture->gxm_tex);
	uint8_t bpp = tex_format_to_bytespp(tex_format);
	uint32_t stride = ALIGN(sceGxmTextureGetWidth(&target_texture->gxm_tex), 8) * bpp;
	uint8_t* ptr = (uint8_t*)sceGxmTextureGetData(&target_texture->gxm_tex) + xoffset * bpp + yoffset * stride;
	uint8_t* ptr_line = ptr;
	uint8_t data_bpp = 0;
	int i,j;
	
	/*
	 * Callbacks are actually used to just perform down/up-sampling
	 * between U8 texture formats. Reads are expected to give as result
	 * a RGBA sample that will be wrote depending on texture format
	 * by the write callback
	 */
	void (*write_cb)(void*, uint32_t) = NULL;
	uint32_t (*read_cb)(void*) = NULL;
	
	// Detecting proper read callback and source bpp
	switch (format){
	case GL_RED:
	case GL_ALPHA:
		switch (type){
		case GL_UNSIGNED_BYTE:
			read_cb = readR;
			data_bpp = 1;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
		}
		break;
	case GL_RG:
		switch (type){
		case GL_UNSIGNED_BYTE:
			read_cb = readRG;
			data_bpp = 2;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
		}
		break;
	case GL_RGB:
		switch (type){
		case GL_UNSIGNED_BYTE:
			data_bpp = 3;
			read_cb = readRGB;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
		}
		break;
	case GL_RGBA:
		switch (type){
		case GL_UNSIGNED_BYTE:
			data_bpp = 4;
			read_cb = readRGBA;
			break;
		case GL_UNSIGNED_SHORT_5_5_5_1:
			data_bpp = 2;
			read_cb = readRGBA5551;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
		}
		break;
	}
	
	switch (target){
	case GL_TEXTURE_2D:
	
		// Detecting proper write callback
		switch (target_texture->type){
		case GL_RGB:
			write_cb = writeRGB;
			break;
		case GL_RGBA:
			write_cb = writeRGBA;
			break;
		case GL_LUMINANCE:
			write_cb = writeR;
			break;
		case GL_LUMINANCE_ALPHA:
			write_cb = writeRA;
			break;
		case GL_INTENSITY:
			write_cb = writeR;
			break;
		case GL_ALPHA:
			write_cb = writeR;
			break;
		}
		
		// Executing texture modification via callbacks
		uint8_t *data = (uint8_t*)pixels;
		for (i=0;i<height;i++){
			for (j=0;j<width;j++){
				uint32_t clr = read_cb((uint8_t*)data);
				write_cb(ptr, clr);
				data += data_bpp;
				ptr += bpp;
			}
			ptr = ptr_line + stride;
			ptr_line = ptr;
		}
		
		break;
		default:
			error = GL_INVALID_ENUM;
			break;	
	}
	
}

void glColorTable(GLenum target,  GLenum internalformat,  GLsizei width,  GLenum format,  GLenum type,  const GLvoid * data){
	
	// Checking if a color table is already enabled, if so, deallocating it
	if (color_table != NULL){
		gpu_free_palette(color_table);
		color_table = NULL;
	}
	
	// Calculating color table bpp
	uint8_t bpp = 0;
	switch (target){
		case GL_COLOR_TABLE:
			switch (format){
				case GL_RGBA:
					bpp = 4;
					break;
				default:
					error = GL_INVALID_ENUM;
					break;
			}
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	
	// Allocating and initializing color table
	color_table = gpu_alloc_palette(data, width, bpp);
	
}

void glTexParameteri(GLenum target, GLenum pname, GLint param){
	
	// Setting some aliases to make code more readable
	texture_unit* tex_unit = &texture_units[server_texture_unit];
	int texture2d_idx = tex_unit->tex_id;
	texture* tex = &tex_unit->textures[texture2d_idx];
	
	switch (target){
		case GL_TEXTURE_2D:
			switch (pname){
				case GL_TEXTURE_MIN_FILTER: // Min filter
					switch (param){
						case GL_NEAREST: // Point
							tex_unit->min_filter = SCE_GXM_TEXTURE_FILTER_POINT;
							break;
						case GL_LINEAR: // Linear
							tex_unit->min_filter = SCE_GXM_TEXTURE_FILTER_LINEAR;
							break;
						case GL_NEAREST_MIPMAP_NEAREST: // TODO: Implement this
							break;
						case GL_LINEAR_MIPMAP_NEAREST: // TODO: Implement this
							break;
						case GL_NEAREST_MIPMAP_LINEAR: // TODO: Implement this
							break;
						case GL_LINEAR_MIPMAP_LINEAR: // TODO: Implement this
							break;
						default:
							error = GL_INVALID_ENUM;
							break;
					}
					sceGxmTextureSetMinFilter(&tex->gxm_tex, tex_unit->min_filter);
					break;
				case GL_TEXTURE_MAG_FILTER: // Mag Filter
					switch (param){
						case GL_NEAREST: // Point
							tex_unit->mag_filter = SCE_GXM_TEXTURE_FILTER_POINT;
							break;
						case GL_LINEAR: // Linear
							tex_unit->mag_filter = SCE_GXM_TEXTURE_FILTER_LINEAR;
							break;
						case GL_NEAREST_MIPMAP_NEAREST: // TODO: Implement this
							break;
						case GL_LINEAR_MIPMAP_NEAREST: // TODO: Implement this
							break;
						case GL_NEAREST_MIPMAP_LINEAR: // TODO: Implement this
							break;
						case GL_LINEAR_MIPMAP_LINEAR: // TODO: Implement this
							break;
						default:
							error = GL_INVALID_ENUM;
							break;
					}
					sceGxmTextureSetMagFilter(&tex->gxm_tex, tex_unit->mag_filter);
					break;
				case GL_TEXTURE_WRAP_S: // U Mode
					switch (param){
						case GL_CLAMP_TO_EDGE: // Clamp
							tex_unit->u_mode = SCE_GXM_TEXTURE_ADDR_CLAMP;
							break;
						case GL_REPEAT: // Repeat
							tex_unit->u_mode = SCE_GXM_TEXTURE_ADDR_REPEAT;
							break;
						case GL_MIRRORED_REPEAT: // Mirror
							tex_unit->u_mode = SCE_GXM_TEXTURE_ADDR_MIRROR;
							break;
						default:
							error = GL_INVALID_ENUM;
							break;
					}
					sceGxmTextureSetUAddrMode(&tex->gxm_tex, tex_unit->u_mode);
					break;
				case GL_TEXTURE_WRAP_T: // V Mode
					switch (param){
						case GL_CLAMP_TO_EDGE: // Clamp
							tex_unit->v_mode = SCE_GXM_TEXTURE_ADDR_CLAMP;
							break;
						case GL_REPEAT: // Repeat
							tex_unit->v_mode = SCE_GXM_TEXTURE_ADDR_REPEAT;
							break;
						case GL_MIRRORED_REPEAT: // Mirror
							tex_unit->v_mode = SCE_GXM_TEXTURE_ADDR_MIRROR;
							break;
						default:
							error = GL_INVALID_ENUM;
							break;
					}
					sceGxmTextureSetVAddrMode(&tex->gxm_tex, tex_unit->v_mode);
					break;
				default:
					error = GL_INVALID_ENUM;
					break;
			}
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
}

void glTexParameterf(GLenum target, GLenum pname, GLfloat param){
	
	// Setting some aliases to make code more readable
	texture_unit* tex_unit = &texture_units[server_texture_unit];
	int texture2d_idx = tex_unit->tex_id;
	texture* tex = &tex_unit->textures[texture2d_idx];
	
	switch (target){
		case GL_TEXTURE_2D:
			switch (pname){
				case GL_TEXTURE_MIN_FILTER: // Min Filter
					if (param == GL_NEAREST) tex_unit->min_filter = SCE_GXM_TEXTURE_FILTER_POINT; // Point
					if (param == GL_LINEAR) tex_unit->min_filter = SCE_GXM_TEXTURE_FILTER_LINEAR; // Linear
					sceGxmTextureSetMinFilter(&tex->gxm_tex, tex_unit->min_filter);
					break;
				case GL_TEXTURE_MAG_FILTER: // Mag filter
					if (param == GL_NEAREST) tex_unit->mag_filter = SCE_GXM_TEXTURE_FILTER_POINT; // Point
					else if (param == GL_LINEAR) tex_unit->mag_filter = SCE_GXM_TEXTURE_FILTER_LINEAR; // Linear
					sceGxmTextureSetMagFilter(&tex->gxm_tex, tex_unit->mag_filter);
					break;
				case GL_TEXTURE_WRAP_S: // U Mode
					if (param == GL_CLAMP_TO_EDGE) tex_unit->u_mode = SCE_GXM_TEXTURE_ADDR_CLAMP; // Clamp
					else if (param == GL_REPEAT) tex_unit->u_mode = SCE_GXM_TEXTURE_ADDR_REPEAT; // Repeat
					else if (param == GL_MIRRORED_REPEAT) tex_unit->u_mode = SCE_GXM_TEXTURE_ADDR_MIRROR; // Mirror
					sceGxmTextureSetUAddrMode(&tex->gxm_tex, tex_unit->u_mode);
					break;
				case GL_TEXTURE_WRAP_T: // V Mode
					if (param == GL_CLAMP_TO_EDGE) tex_unit->v_mode = SCE_GXM_TEXTURE_ADDR_CLAMP; // Clamp
					else if (param == GL_REPEAT) tex_unit->v_mode = SCE_GXM_TEXTURE_ADDR_REPEAT; // Repeat
					else if (param == GL_MIRRORED_REPEAT) tex_unit->v_mode = SCE_GXM_TEXTURE_ADDR_MIRROR; // Mirror
					sceGxmTextureSetVAddrMode(&tex->gxm_tex, tex_unit->v_mode);
					break;
				default:
					error = GL_INVALID_ENUM;
					break;
			}
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
}

void glActiveTexture(GLenum texture){
	
	// Changing current in use server texture unit
#ifndef SKIP_ERROR_HANDLING
	if ((texture < GL_TEXTURE0) && (texture > GL_TEXTURE31)) error = GL_INVALID_ENUM;
	else 
#endif
		server_texture_unit = texture - GL_TEXTURE0;
	
}
