<p align="center"><img width="50%" height="50%" src="./vitagl.png"></p>
vitaGL is an opensource openGL driver for PSVITA development. It acts as a wrapper between openGL and sceGxm and allows to use a subset of openGL functions with fully hardware acceleration by translating the code to sceGxm equivalent.

# Prerequisites
In order to run an homebrew made with vitaGL, you are going to need libshacccg.suprx extracted and decrypted on your console. You can refer to this guide for more details about its extraction: https://samilops2.gitbook.io/vita-troubleshooting-guide/shader-compiler/extract-libshacccg.suprx<br>
If you want your homebrew to not be hard dependant from libshacccg.suprx, you can either:
- Stick to precompiled shaders usage only (glShaderBinary).
- Stick to fixed function pipeline features (GL1) while using this old legacy version of the library: https://github.com/Rinnegatamante/vitaGL/tree/legacy_precompiled_ffp

# Build Instructions
In order to build vitaGL use the following command: `make install`.
<br>These are all the available flags usable when compiling the library:<br>
`HAVE_SHARK_LOG=1` Enables logging support in runtime shader compiler.<br>
`HAVE_CUSTOM_HEAP=1` Replaces sceClib heap implementation with custom one (Less efficient but safer).<br>
`LOG_ERRORS=1` Errors will be logged with sceClibPrintf.<br>
`LOG_ERRORS=2` Errors will be logged to ux0:data/vitaGL.log.<br>
`NO_DEBUG=1` Disables most of the error handling features (Faster CPU code execution but code may be non compliant to all OpenGL standards).<br>
`NO_TEX_COMBINER=1` Disables texture combiner support (GL_COMBINE) for faster fixed function pipeline code execution.<br>
`NO_SHADER_CACHE=1` Disables extra shader cache layer on filesystem (ux0:data/shader_cache) for fixed function pipeline.<br>
`SOFTFP_ABI=1` Compiles the library in soft floating point compatibility mode.<br>
`DRAW_SPEEDHACK=1` Enables faster code for draw calls. May cause crashes.<br>
`MATH_SPEEDHACK=1` Enables faster code for matrix math calls. May cause glitches.<br>
`SAMPLERS_SPEEDHACK=1` Enables faster code for samplers resolution during shaders usage. May cause glitches.<br>
`SHADER_COMPILER_SPEEDHACK=1` Enables faster code for glShaderSource. May cause errors.<br>
`HAVE_HIGH_FFP_TEXUNITS=1` Enables support for more than 2 texunits for fixed function pipeline at the cost of some performance loss.<br>
`HAVE_UNFLIPPED_FBOS=1` Framebuffers objects won't be internally flipped to match OpenGL standards.<br>
`SHARED_RENDERTARGETS=1` Makes small framebuffers objects use shared rendertargets instead of dedicated ones.<br>
`CIRCULAR_VERTEX_POOL=1` Makes temporary data buffers being handled with a circular pool.<br>
`SINGLE_THREADED_GC=1` Makes the garbage collector run on main thread.<br>
`PHYCONT_ON_DEMAND=1` Makes the physically contiguous RAM be handled with separate memblocks instead of an heap.<br>
`UNPURE_TEXTURES=1` Makes legal to upload textures without base level.<br>
`HAVE_DEBUGGER=1` Enables lightweighted on screen debugger interface.<br>
`HAVE_DEBUGGER=2` Enables lightweighted on screen debugger interface with extra information (devkit only).<br>
`HAVE_RAZOR=1` Enables debugging features through Razor debugger (retail and devkit compatible).<br>
`HAVE_RAZOR=2` Enables debugging features through Razor debugger (retail and devkit compatible) with ImGui interface.<br>
`HAVE_DEVKIT=1` Enables extra debugging features through Razor debugger available only for devkit users.<br>
`HAVE_DEVKIT=2` Enables extra debugging features through Razor debugger available only for devkit users with ImGui interface.<br>
# Samples

You can find samples in the *samples* folder in this repository.

# Help and Troubleshooting

If you plan to use vitaGL for one of your projects, you can find an official channel to get help with it on Vita Nuova discord server: https://discord.gg/PyCaBx9

# Projects actually using vitaGL

Here you can find a list of projects using vitaGL:

Direct OpenGL Usage:<br>
[vitaQuake](https://vitadb.rinnegatamante.it/#/info/10) - Port of Quake I and mission packs<br>
[vitaQuakeII](https://vitadb.rinnegatamante.it/#/info/278) -Port of Quake II and mission packs<br>
[vitaQuakeIII](https://vitadb.rinnegatamante.it/#/info/375) - Port of ioquake3 (Quake III: Arena, Quake III: Team Arena, OpenArena, Urban Terror)<br>
[vitaRTCW](https://vitadb.rinnegatamante.it/#/info/459) - Port of iortcw (Return to Castle Wolfenstein)<br>
[vitaHexenII](https://vitadb.rinnegatamante.it/#/info/196) - Port of Hexen II<br>
[vitaXash3D](https://vitadb.rinnegatamante.it/#/info/365) - Port of Xash3D (Half Life, Counter Strike 1.6)<br>
[Fade to Black](https://vitadb.rinnegatamante.it/#/info/367) - Port of Fade to Black<br>
[vitaVoyager](https://vitadb.rinnegatamante.it/#/info/367) - Port of lilium-voyager (Star Trek Voyager: Elite Force)<br>
[Daedalus X64](https://github.com/Rinnegatamante/daedalusx64-vitagl) - Port of Daedalus X64 (N64 Emulator)<br>
[RetroArch](https://github.com/libretro/RetroArch) - Vita's GL1 video driver of RetroArch<br>
[vitaET](https://github.com/Rinnegatamante/vitaET) - Port of ET: Legacy (Wolfenstein: Enemy Territory)<br>
[flycast](https://github.com/Rinnegatamante/flycast) - Port of flycast (Dreamcast Emulator)<br>
[AvP Gold](https://github.com/Rinnegatamante/AvP-Gold-Vita) - Port of Aliens versus Predator: Gold Edition<br>
[prboom-plus](https://vitadb.rinnegatamante.it/#/info/591) - Port of PrBoom Plus (Doom engine sourceport)<br>
[VITAlbum](https://vitadb.rinnegatamante.it/#/info/566) - Filebrowser and image viewer app<br>
[sm64-vita](https://github.com/bythos14/sm64-vita) - Port of Super Mario 64<br>
[srb2-vita](https://github.com/Rinnegatamante/srb2-vita) - Port of Sonic Robo Blast 2<br>
[rvm_soniccd-vitagl](https://github.com/fgsfdsfgs/rvm_soniccd/tree/vitagl) - Port of rvm_soniccd (Sonic CD decompilation)<br>
[Hurrican](https://vitadb.rinnegatamante.it/#/info/617) - Port of Hurrican<br>
[VITA Homebrew Sorter](https://vitadb.rinnegatamante.it/#/info/655) - App to sort your app.db<br>
[jfsw-vita](https://vitadb.rinnegatamante.it/#/info/705) - Port of JFSW (Shadow Warrior Classic sourceport)<br>
[jfduke3d-vita](https://vitadb.rinnegatamante.it/#/info/711) - Port of JFDuke3D (Duke Nukem 3D sourceport)<br>
[d3es-vita](https://github.com/Rinnegatamante/d3es-vita) - Port of Doom 3<br>
[bc2_vita](https://vitadb.rinnegatamante.it/#/info/714) - Port of Battlefield Bad Company 2 Mobile<br>
[JetMan 3D](https://vitadb.rinnegatamante.it/#/info/719) - Fanmade remake of Jetpac for ZX Spectrum<br>
[FF3_Vita](https://vitadb.rinnegatamante.it/#/info/725) - Port of Final Fantasy III (3D Remake)<br>
[FF4_Vita](https://vitadb.rinnegatamante.it/#/info/726) - Port of Final Fantasy IV (3D Remake)<br>
[FF5_Vita](https://vitadb.rinnegatamante.it/#/info/733) - Port of Final Fantasy V<br>
[TheXTech Vita](https://vitadb.rinnegatamante.it/#/info/727) - Port of Super Mario Bros X<br>
[World of Goo Vita](https://vitadb.rinnegatamante.it/#/info/806) - Port of World of Goo<br>
[Baba Is You Vita](https://vitadb.rinnegatamante.it/#/info/828) - Port of Baba Is You<br>
[Crazy Taxi Vita](https://vitadb.rinnegatamante.it/#/info/728) - Port of Crazy Taxi Classic<br>
[YoYo Loader](https://vitadb.rinnegatamante.it/#/info/815) - Loader for Game Maker Studio made games<br>
[TWoM Vita](https://vitadb.rinnegatamante.it/#/info/802) - Port of This War of Mine and This War of Mine: Stories - Father's Promise<br>
[StaticJK](https://github.com/Rinnegatamante/StaticJK) - Port of Star Wars: Jedi Academy<br>
[Nazi Zombies Portable](https://vitadb.rinnegatamante.it/#/info/757) - Port of Nazi Zombies Portable<br>
[Quakespasm-Spiked](https://vitadb.rinnegatamante.it/#/info/716) - Port of Quakespasm Spiked (Limit removed Quake Engine sourceport)<br>

Libraries:<br>
[sdl12_gl](https://github.com/Rinnegatamante/SDL-Vita/tree/sdl12_gl/src) - SDL 1.2 Vita port adapted to work with vitaGL as renderer backend<br>
[SDL2_vitagl](https://github.com/Northfear/SDL/tree/vitagl) - SDL2 Vita port adapted to work with vitaGL as renderer backend<br>
[imgui_vita](https://github.com/Rinnegatamante/imgui-vita) - Port of dear imGui <br>

sdl12_gl Apps:<br>
[SuperMarioWar](https://vitadb.rinnegatamante.it/#/info/422) - Port of Super Mario War<br>
[ZeldaOLB](https://vitadb.rinnegatamante.it/#/info/265) - Port of Zelda: Oni Link Begins<br>
[ZeldaROTH](https://vitadb.rinnegatamante.it/#/info/109) - Port of Zelda: Return of the Hylian<br>
[Zelda3T](https://vitadb.rinnegatamante.it/#/info/334) - Port of Zelda: Time to Triumph<br>
[ZeldaNSQ](https://vitadb.rinnegatamante.it/#/info/350) - Port of Zelda: Navi's Quest<br>
[vitaWolfen](https://vitadb.rinnegatamante.it/#/info/31) - Port of Wolf4SDL (Wolfenstein 3D)<br>
[meritous](https://vitadb.rinnegatamante.it/#/info/411) - Port of meritous<br>
[Dstroy Vita](https://vitadb.rinnegatamante.it/#/info/383) - Port of Dstroy<br>
