<p align="center"><img width="50%" height="50%" src="./vitagl.png"></p>
vitaGL is an opensource openGL driver for PSVITA development. It acts as a wrapper between openGL and sceGxm and allows to use a subset of openGL functions with fully hardware acceleration by translating the code to sceGxm equivalent.

# Prerequisites
In order to run an homebrew made with vitaGL, you are going to need libshacccg.suprx extracted and decrypted on your console. You can refer to this guide for more details about its extraction: https://samilops2.gitbook.io/vita-troubleshooting-guide/shader-compiler/extract-libshacccg.suprx<br>
If you want your homebrew to not be hard dependant from libshaccg.suprx you can either:
- Stick to precompiled shaders usage only (glShaderBinary).
- Stick to fixed function pipeline features (GL1) while using this old legacy version of the library: https://github.com/Rinnegatamante/vitaGL/tree/legacy_precompiled_ffp

# Build Instructions
In order to build vitaGL use the following command: `make install`.
<br>These are all the available flags usable when compiling the library:<br>
`HAVE_SHARK_LOG=1` Enables logging support in runtime shader compiler.<br>
`LOG_ERRORS=1` Errors will be logged with sceClibPrintf.<br>
`LOG_ERRORS=2` Errors will be logged to ux0:data/vitaGL.log.<br>
`NO_DEBUG=1` Disables most of the error handling features (Faster CPU code execution but code may be non compliant to all OpenGL standards).<br>
`SOFTFP_ABI=1` Compiles the library in soft floating point compatibility mode.<br>
`HAVE_UNFLIPPED_FBOS=1` Framebuffers objects won't be internally flipped to match OpenGL standards.<br>
`SHARED_RENDERTARGETS=1` Makes small framebuffers objects use shared rendertargets instead of dedicated ones.<br>
`CIRCULAR_VERTEX_POOL=1` Makes temporary data buffers being handled with a circular pool.<br>
`SAMPLER_UNIFORMS=1` When enabled, shader samplers are treated as uniforms.<br>
`UNPURE_TEXTURES=1` Makes legal to upload textures without base level.<br>
`HAVE_RAZOR=1` Enables debugging features through Razor debugger (retail and devkit compatible).<br>
`HAVE_RAZOR=2` Enables debugging features through Razor debugger (retail and devkit compatible) with ImGui interface.<br>
`HAVE_DEVKIT=1` Enables extra debugging features through Razor debugger available only for devkit users.<br>
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
[re3-vita](https://vitadb.rinnegatamante.it/#/info/589) - Port of Grand Theft Auto III<br>
[reVC-vita](https://vitadb.rinnegatamante.it/#/info/616) - Port of Grand Theft Auto: Vice City<br>
[prboom-plus](https://vitadb.rinnegatamante.it/#/info/591) - Port of PrBoom Plus (Doom engine sourceport)<br>
[VITAlbum](https://vitadb.rinnegatamante.it/#/info/566) - Filebrowser and image viewer app<br>
[sm64-vita](https://github.com/bythos14/sm64-vita) - Port of Super Mario 64<br>
[srb2-vita](https://github.com/Rinnegatamante/srb2-vita) - Port of Sonic Robo Blast 2<br>
[rvm_soniccd-vitagl](https://github.com/fgsfdsfgs/rvm_soniccd/tree/vitagl) - Port of rvm_soniccd (Sonic CD decompilation)<br>
[Max Payne](https://vitadb.rinnegatamante.it/#/info/612) - Port of Max Payne Mobile<br>
[Hurrican](https://vitadb.rinnegatamante.it/#/info/617) - Port of Hurrican<br>
[gtasa-vita](https://vitadb.rinnegatamante.it/#/info/618) - Port of Grand Theft Auto: San Andreas Mobile<br>
[gtactw-vita](https://vitadb.rinnegatamante.it/#/info/625) - Port of Grand Theft Auto: Chinatown Wars Mobile<br>
[VITA Homebrew Sorter](https://vitadb.rinnegatamante.it/#/info/655) - App to sort your app.db<br>

Libraries:<br>
[sdl12_gl](https://github.com/Rinnegatamante/SDL-Vita/tree/sdl12_gl/src) - SDL 1.2 Vita port adapted to work with vitaGL as renderer<br>
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
