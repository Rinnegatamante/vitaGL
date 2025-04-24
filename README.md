<p align="center"><img width="50%" height="50%" src="./vitagl.png"></p>
vitaGL is an opensource openGL driver for PSVITA development. It acts as a wrapper between openGL and sceGxm and allows to use a subset of openGL functions with full hardware acceleration by translating the code to sceGxm equivalent.

# Prerequisites
In order to run an homebrew made with vitaGL, you are going to need libshacccg.suprx extracted and decrypted on your console. You can refer to this guide for more details about its extraction: https://samilops2.gitbook.io/vita-troubleshooting-guide/shader-compiler/extract-libshacccg.suprx<br>
If you want your homebrew to not be hard dependant from libshacccg.suprx, you can either:
- Stick to fixed function pipeline features (GL1) while using this old legacy version of the library: https://github.com/Rinnegatamante/vitaGL/tree/legacy_precompiled_ffp

# Build Instructions
In order to build vitaGL use the following command: `make install`.
<br>These are all the available flags usable when compiling the library:<br>
### Debug Flags
| Flag | Description |
| --- | --- |
| `HAVE_SHARK_LOG=1`|  Enables logging support in runtime shader compiler.|
|`LOG_ERRORS=1`| Errors will be logged with sceClibPrintf.|
|`LOG_ERRORS=2`| Errors will be logged to ux0:data/vitaGL.log.|
|`HAVE_PROFILING=1`| Enables lightweighted profiler for CPU time spent in draw calls.|
|`HAVE_DEBUGGER=1`| Enables lightweighted on screen debugger interface.|
|`HAVE_DEBUGGER=2`| Enables lightweighted on screen debugger interface with extra information (devkit only).|
|`HAVE_RAZOR=1`| Enables debugging features through Razor debugger (retail and devkit compatible).|
|`HAVE_RAZOR=2`| Enables debugging features through Razor debugger (retail and devkit compatible) with ImGui interface.|
|`HAVE_DEVKIT=1`| Enables extra debugging features through Razor debugger available only for devkit users.|
|`HAVE_DEVKIT=2`| Enables extra debugging features through Razor debugger available only for devkit users with ImGui interface.|
### Compatibility Flags
| Flag | Description |
| --- | --- |
|`HAVE_CUSTOM_HEAP=1`| Replaces sceClib heap implementation with custom one (Less efficient but safer).|
|`HAVE_GLSL_SUPPORT=1`| Enables experimental GLSL to CG auto translation for shader sources with preprocessor pass (Recommended).|
|`HAVE_GLSL_SUPPORT=2`| Enables experimental GLSL to CG auto translation for shader sources without preprocessor pass.|
|`SOFTFP_ABI=1`| Compiles the library in soft floating point compatibility mode.|
|`STORE_DEPTH_STENCIL=1`| Makes all framebuffers depth/stencil surfaces to be load/stored on memory. Makes the rendering slower but more compliant with OpenGL standards.|
|`HAVE_HIGH_FFP_TEXUNITS=1`| Enables support for more than 2 texunits for fixed function pipeline at the cost of some performance loss.|
|`HAVE_DISPLAY_LISTS=1`| Enables support for display lists at the cost of some performance loss.|
|`SAFE_ETC1=1`| Disables hardware support for ETC1 textures. Makes ETC1 textures usage less efficient but allows for proper debugging in Razor.|
|`SAFE_DRAW=1`| Makes some optimizations in the drawing pipeline less efficient but can solve some glitches.|
|`SAFE_UNIFORMS=1`| Makes some optimizations in the shaders pipeline less efficient but makes uniform location indexing for basic type arrays compliant.|
|`UNPURE_TEXFORMATS=1`| Enables support for texture dimensions different than 2D (tex2D is still required in shader code).|
|`HAVE_VITA3K_SUPPORT=1`| Disables several features in order to make vitaGL compatible with Vita3K. Requires vitaShaRK compiled with https://github.com/Rinnegatamante/vitaShaRK/blob/master/source/vitashark.c#L24 uncommented.|
### Speedhack Flags
| Flag | Description |
| --- | --- |
|`NO_TEX_COMBINER=1`| Disables texture combiner support (GL_COMBINE) for faster fixed function pipeline code execution.|
|`NO_DEBUG=1`| Disables most of the error handling features (Faster CPU code execution but code may be non compliant to all OpenGL standards).|
|`BUFFERS_SPEEDHACK=1`| Enables faster vertex buffer copying. May cause crashes.|
|`DRAW_SPEEDHACK=1`| Enables faster code for draw calls. May cause crashes.|
|`DRAW_SPEEDHACK=2`| Enables faster code for draw calls only for large vertex data draws. May cause crashes.|
|`INDICES_SPEEDHACK=1`| Produces faster draw code but disables support for instanced draws and makes 32 bit (GL_UNSIGNED_INT) indexed draws potentially cause glitches.|
|`MATH_SPEEDHACK=1`| Enables faster code for matrix math calls. May cause glitches.|
|`TEXTURES_SPEEDHACK=1`| Makes glTexSubImage2D/glTexSubImage1D non fully OpenGL compliant but makes rendering pipeline slightly faster. Incompatible with HAVE_TEXTURE_CACHE=1.|
|`SAMPLERS_SPEEDHACK=1`| Enables faster code for samplers resolution during shaders usage. May cause glitches.|
|`SHADER_COMPILER_SPEEDHACK=1`| Enables faster code for glShaderSource. May cause errors.|
|`PRIMITIVES_SPEEDHACK=1`| Makes draw calls more efficient but GL_LINES and GL_POINTS primitives usage may cause glitches.|
### Misc Flags
| Flag | Description |
| --- | --- |
|`HAVE_TEXTURE_CACHE=1`| Adds file caching for textures not used since a lot of time, acting like a sort of swap implementation to increase effective available memory. (Experimental)|
|`NO_DMAC=1`| Disables sceDmacMemcpy usage. In some rare instances, it can improve framerate.|
|`HAVE_UNFLIPPED_FBOS=1`| Framebuffers objects won't be internally flipped to match OpenGL standards.|
|`HAVE_WVP_ON_GPU=1`| Moves calculation of the wvp in fixed function pipeline codepath to the GPU. Reduces CPU workload and increases GPU one.|
|`SHARED_RENDERTARGETS=1`| Makes small framebuffers objects use shared rendertargets instead of dedicated ones.|
|`SHARED_RENDERTARGETS=2`| Makes small framebuffers objects use shared rendertargets instead of dedicated ones and adds a mechanism for recycling older rendertargets.|
|`CIRCULAR_VERTEX_POOL=1`| Makes temporary data buffers being handled with a circular pool.|
|`CIRCULAR_VERTEX_POOL=2`| Makes temporary data buffers being handled with a circular pool with fallback to regular allocation if the pool gets overrun.|
|`USE_SCRATCH_MEMORY=1`| Makes GL_DYNAMIC and GL_STREAM vbos be configurable to use circular pool instead of regular allocations. Needs CIRCULAR_VERTEX_POOL.|
|`HAVE_PTHREAD=1`| Use pthread instead of sceKernel for starting garbage collector thread.|
|`SINGLE_THREADED_GC=1`| Makes the garbage collector run on main thread.|
|`PHYCONT_ON_DEMAND=1`| Makes the physically contiguous RAM be handled with separate memblocks instead of an heap.|
|`UNPURE_TEXTURES=1`| Makes legal to upload textures without base level.|
|`UNPURE_TEXCOORDS=1`| Makes legal to use multitexturing with fixed-function pipeline with GL_TEXTURE0 disabled.|
|`DISABLE_FFP_MULTITEXTURE=1`| Disables multitexture processing during draw calls performed with fixed function pipeline.|
|`HAVE_WRAPPED_ALLOCATORS=1`| Allows usage of vgl allocators inside wrapped allocators.|
|`HAVE_SHADER_CACHE=1`| Enables fast automatic file caching (based on XH3 xxHash algorithm) for application provided shaders.|
|`NO_CLIB=1`| Disables sceClib functions usage for easier debugging at the cost of slightly slower CPU code.|
|`DISABLE_W_CLAMPING=1`| Disables W clamping during viewport calculation. Might fix some glitches.|

# Samples

You can find samples in the *samples* folder in this repository.

# Help and Troubleshooting

If you plan to use vitaGL for one of your projects, you can find an official channel to get help with it on Vita Nuova discord server: https://discord.gg/PyCaBx9

# Projects actually using vitaGL

Here you can find a list of projects using vitaGL:

Direct OpenGL Usage:<br>
[Aerofoil Vita](https://vitadb.rinnegatamante.it/#/info/1040) - Port of Glider PRO<br>
[Anomaly 2 Vita](https://vitadb.rinnegatamante.it/#/info/1050) - Port of Anomaly 2<br>
[Anomaly Defenders Vita](https://vitadb.rinnegatamante.it/#/info/1051) - Port of Anomaly Defenders<br>
[Anomaly Korea](https://vitadb.rinnegatamante.it/#/info/1047) - Port of Anomaly Korea<br>
[Anomaly WE Vita](https://vitadb.rinnegatamante.it/#/info/1046) - Port of Anomaly Warzone Earth HD<br>
[AvP Gold](https://vitadb.rinnegatamante.it/#/info/569) - Port of Aliens versus Predator: Gold Edition<br>
[Baba Is You Vita](https://vitadb.rinnegatamante.it/#/info/828) - Port of Baba Is You<br>
[Billy Frontier Vita](https://vitadb.rinnegatamante.it/#/info/1001) - Port of Billy Frontier<br>
[Bugdom](https://vitadb.rinnegatamante.it/#/info/841) - Port of Bugdom<br>
[bc2_vita](https://vitadb.rinnegatamante.it/#/info/714) - Port of Battlefield Bad Company 2 Mobile<br>
[Crazy Taxi Vita](https://vitadb.rinnegatamante.it/#/info/728) - Port of Crazy Taxi Classic<br>
[Cro-Mag Rally Vita](https://vitadb.rinnegatamante.it/#/info/872) - Port of Cro-Mag Rally<br>
[CrossCraft Classic](https://vitadb.rinnegatamante.it/#/info/848) - Multiplatform Minecraft Classic clone<br>
[d3es-vita](https://github.com/Rinnegatamante/d3es-vita) - Port of Doom 3<br>
[Daedalus X64](https://vitadb.rinnegatamante.it/#/info/553) - Port of Daedalus X64 (N64 Emulator)<br>
[Dead Space Vita](https://vitadb.rinnegatamante.it/#/info/999) - Port of Dead Space (Android)<br>
[Death Road to Canada Lite](https://vitadb.rinnegatamante.it/#/info/846) - Port of Death Road to Canada<br>
[Doom64EX](https://vitadb.rinnegatamante.it/#/info/881) - Port of Doom64EX (Doom 64 sourceport)<br>
[Fade to Black](https://vitadb.rinnegatamante.it/#/info/367) - Port of Fade to Black<br>
[Fahrenheit Vita](https://vitadb.rinnegatamante.it/#/info/835) - Port of Fahrenheit: Indigo Prophecy<br>
[FF3-Vita](https://vitadb.rinnegatamante.it/#/info/725) - Port of Final Fantasy III (3D Remake)<br>
[FF4-Vita](https://vitadb.rinnegatamante.it/#/info/726) - Port of Final Fantasy IV (3D Remake)<br>
[FF4AY_Vita](https://vitadb.rinnegatamante.it/#/info/989) - Port of Final Fantasy IV: The After Years (3D Remake)<br>
[FF5-Vita](https://vitadb.rinnegatamante.it/#/info/733) - Port of Final Fantasy V<br>
[Flycast](https://vitadb.rinnegatamante.it/#/info/605) - Port of Flycast (Dreamcast Emulator)<br>
[Funky Smugglers Vita](https://vitadb.rinnegatamante.it/#/info/1044) - Port of Funky Smugglers<br>
[Hassey Collection](https://vitadb.rinnegatamante.it/#/info/1014) - Port of Galcon 2: Galactic Conquest and BREAKFINITY<br>
[Hurrican](https://vitadb.rinnegatamante.it/#/info/617) - Port of Hurrican<br>
[Isotope 244 Collection](https://vitadb.rinnegatamante.it/#/info/873) - Port of Machines at War 3 and Land Air Sea Warfare<br>
[JetMan 3D](https://vitadb.rinnegatamante.it/#/info/719) - Fanmade remake of Jetpac for ZX Spectrum<br>
[jfduke3d-vita](https://vitadb.rinnegatamante.it/#/info/711) - Port of JFDuke3D (Duke Nukem 3D sourceport)<br>
[jfsw-vita](https://vitadb.rinnegatamante.it/#/info/705) - Port of JFSW (Shadow Warrior Classic sourceport)<br>
[Lugaru HD](https://vitadb.rinnegatamante.it/#/info/853) - Port of Lugaru<br>
[Mass Effect Infiltrator Vita](https://vitadb.rinnegatamante.it/#/info/1019) - Port of Mass Effect: Infiltrator<br>
[Nanosaur Vita](https://vitadb.rinnegatamante.it/#/info/851) - Port of Nanosaur<br>
[Nazi Zombies Portable](https://vitadb.rinnegatamante.it/#/info/757) - Port of Nazi Zombies Portable<br>
[Neverball Vita](https://vitadb.rinnegatamante.it/#/info/873) - Port of Neverball<br>
[Neverputt Vita](https://vitadb.rinnegatamante.it/#/info/874) - Port of Neverputt<br>
[Pekka Kana 2 Vita](https://vitadb.rinnegatamante.it/#/info/847) - Port of Pekka Kana 2<br>
[Poppy Kart Collection](https://vitadb.rinnegatamante.it/#/info/1030) - Port of Poppy Kart and Poppy Kart 2<br>
[prboom-plus](https://vitadb.rinnegatamante.it/#/info/591) - Port of PrBoom Plus (Doom engine sourceport)<br>
[Quakespasm-Spiked](https://vitadb.rinnegatamante.it/#/info/716) - Port of Quakespasm Spiked (Limit removed Quake Engine sourceport)<br>
[Rigel Engine](https://vitadb.rinnegatamante.it/#/info/988) - Port of Duke Nukem II<br>
[RVGL Vita](https://vitadb.rinnegatamante.it/#/info/840) - Port of RVGL<br>
[rvm_soniccd-vitagl](https://github.com/fgsfdsfgs/rvm_soniccd/tree/vitagl) - Port of rvm_soniccd (Sonic CD decompilation)<br>
[Sleepwalker's Journey Vita](https://vitadb.rinnegatamante.it/#/info/1048) - Port of Sleepwalker's Journey<br>
[sm64-vita](https://github.com/bythos14/sm64-vita) - Port of Super Mario 64<br>
[srb2-vita](https://github.com/Rinnegatamante/srb2-vita) - Port of Sonic Robo Blast 2<br>
[StaticJK](https://github.com/Rinnegatamante/StaticJK) - Port of Star Wars: Jedi Academy<br>
[TheXTech Vita](https://vitadb.rinnegatamante.it/#/info/727) - Port of Super Mario Bros X<br>
[Tomb Raider 1 & 2 Classic Collection](https://vitadb.rinnegatamante.it/#/info/845) - Port of Tomb Raider and Tomb Raider 2<br>
[TWoM Vita](https://vitadb.rinnegatamante.it/#/info/802) - Port of This War of Mine and This War of Mine: Stories - Father's Promise<br>
[VITA Homebrew Sorter](https://vitadb.rinnegatamante.it/#/info/655) - App to sort your app.db<br>
[VitaDB Downloader](https://vitadb.rinnegatamante.it/#/info/877) - Homebrew store app<br>
[VITAlbum](https://vitadb.rinnegatamante.it/#/info/566) - Filebrowser and image viewer app<br>
[vitaET](https://github.com/Rinnegatamante/vitaET) - Port of ET: Legacy (Wolfenstein: Enemy Territory)<br>
[vitaHexenII](https://vitadb.rinnegatamante.it/#/info/196) - Port of Hexen II<br>
[vitaQuake](https://vitadb.rinnegatamante.it/#/info/10) - Port of Quake I and mission packs<br>
[vitaQuakeII](https://vitadb.rinnegatamante.it/#/info/278) -Port of Quake II and mission packs<br>
[vitaQuakeIII](https://vitadb.rinnegatamante.it/#/info/375) - Port of ioquake3 (Quake III: Arena, Quake III: Team Arena, OpenArena, Urban Terror)<br>
[vitaRTCW](https://vitadb.rinnegatamante.it/#/info/459) - Port of iortcw (Return to Castle Wolfenstein)<br>
[vitaVoyager](https://vitadb.rinnegatamante.it/#/info/491) - Port of lilium-voyager (Star Trek Voyager: Elite Force)<br>
[vitaXash3D](https://vitadb.rinnegatamante.it/#/info/365) - Port of Xash3D (Half Life, Counter Strike 1.6)<br>
[World of Goo Vita](https://vitadb.rinnegatamante.it/#/info/806) - Port of World of Goo<br>
[YoYo Loader](https://vitadb.rinnegatamante.it/#/info/815) - Loader for Game Maker Studio made games<br>

Libraries:<br>
[imgui_vita](https://github.com/Rinnegatamante/imgui-vita) - Port of dear imGui <br>
[sdl12_gl](https://github.com/Rinnegatamante/SDL-Vita/tree/sdl12_gl/src) - SDL 1.2 Vita port adapted to work with vitaGL as renderer backend<br>
[SDL2_vitagl](https://github.com/Northfear/SDL/tree/vitagl) - SDL2 Vita port adapted to work with vitaGL as renderer backend<br>

sdl12_gl Apps:<br>
[Dstroy Vita](https://vitadb.rinnegatamante.it/#/info/383) - Port of Dstroy<br>
[meritous](https://vitadb.rinnegatamante.it/#/info/411) - Port of meritous<br>
[SuperMarioWar](https://vitadb.rinnegatamante.it/#/info/422) - Port of Super Mario War<br>
[vitaWolfen](https://vitadb.rinnegatamante.it/#/info/31) - Port of Wolf4SDL (Wolfenstein 3D)<br>
[Zelda3T](https://vitadb.rinnegatamante.it/#/info/334) - Port of Zelda: Time to Triumph<br>
[ZeldaNSQ](https://vitadb.rinnegatamante.it/#/info/350) - Port of Zelda: Navi's Quest<br>
[ZeldaOLB](https://vitadb.rinnegatamante.it/#/info/265) - Port of Zelda: Oni Link Begins<br>
[ZeldaROTH](https://vitadb.rinnegatamante.it/#/info/109) - Port of Zelda: Return of the Hylian<br>
