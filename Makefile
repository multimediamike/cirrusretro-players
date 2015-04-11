TARGET=cr-spc.js
.DEFAULT_GOAL=cr-spc.js

CXX:=em++

THINGS_TO_CLEAN:=$(TARGET)

EXPORT_LIST="['_crPlayerContextSize', '_crPlayerInitialize', '_crPlayerLoadFile', '_crPlayerSetTrack', '_crPlayerGenerateStereoFrames', '_crPlayerVoicesCanBeToggled', '_crPlayerGetVoiceCount', '_crPlayerSetVoiceState', '_crPlayerCleanup']";

GME_CXX_SOURCES:=gme_interface.cpp \
	gme-source-0.6.0/Ay_Apu.cpp \
	gme-source-0.6.0/Ay_Cpu.cpp \
	gme-source-0.6.0/Ay_Emu.cpp \
	gme-source-0.6.0/Blip_Buffer.cpp \
	gme-source-0.6.0/Classic_Emu.cpp \
	gme-source-0.6.0/Data_Reader.cpp \
	gme-source-0.6.0/Dual_Resampler.cpp \
	gme-source-0.6.0/Effects_Buffer.cpp \
	gme-source-0.6.0/Fir_Resampler.cpp \
	gme-source-0.6.0/Gb_Apu.cpp \
	gme-source-0.6.0/Gb_Cpu.cpp \
	gme-source-0.6.0/Gb_Oscs.cpp \
	gme-source-0.6.0/Gbs_Emu.cpp \
	gme-source-0.6.0/gme.cpp \
	gme-source-0.6.0/Gme_File.cpp \
	gme-source-0.6.0/Gym_Emu.cpp \
	gme-source-0.6.0/Hes_Apu.cpp \
	gme-source-0.6.0/Hes_Cpu.cpp \
	gme-source-0.6.0/Hes_Emu.cpp \
	gme-source-0.6.0/Kss_Cpu.cpp \
	gme-source-0.6.0/Kss_Emu.cpp \
	gme-source-0.6.0/Kss_Scc_Apu.cpp \
	gme-source-0.6.0/M3u_Playlist.cpp \
	gme-source-0.6.0/Multi_Buffer.cpp \
	gme-source-0.6.0/Music_Emu.cpp \
	gme-source-0.6.0/Nes_Apu.cpp \
	gme-source-0.6.0/Nes_Cpu.cpp \
	gme-source-0.6.0/Nes_Fme7_Apu.cpp \
	gme-source-0.6.0/Nes_Namco_Apu.cpp \
	gme-source-0.6.0/Nes_Oscs.cpp \
	gme-source-0.6.0/Nes_Vrc6_Apu.cpp \
	gme-source-0.6.0/Nsfe_Emu.cpp \
	gme-source-0.6.0/Nsf_Emu.cpp \
	gme-source-0.6.0/Sap_Apu.cpp \
	gme-source-0.6.0/Sap_Cpu.cpp \
	gme-source-0.6.0/Sap_Emu.cpp \
	gme-source-0.6.0/Sms_Apu.cpp \
	gme-source-0.6.0/Snes_Spc.cpp \
	gme-source-0.6.0/Spc_Cpu.cpp \
	gme-source-0.6.0/Spc_Dsp.cpp \
	gme-source-0.6.0/Spc_Emu.cpp \
	gme-source-0.6.0/Spc_Filter.cpp \
	gme-source-0.6.0/Vgm_Emu.cpp \
	gme-source-0.6.0/Vgm_Emu_Impl.cpp \
	gme-source-0.6.0/Ym2413_Emu.cpp \
	gme-source-0.6.0/Ym2612_Emu.cpp
#GME_CXXFLAGS:=-Wall -g -Os -DHAVE_CONFIG_H -Igme-source-0.6.0 -Igme-source-0.6.0/config-headers/spc
GME_CXXFLAGS:=-Wall -g -Os -Igme-source-0.6.0 -Igme-source-0.6.0/config-headers/spc
GME_CXX_OBJECTS:=$(patsubst %.cpp,%.js.o,$(GME_CXX_SOURCES))
# C++ rule
%.js.o : %.cpp
	$(CXX) -o $@ -c $< $(GME_CXXFLAGS)
THINGS_TO_CLEAN:=$(THINGS_TO_CLEAN) $(GME_C_OBJECTS) $(GME_CXX_OBJECTS)

$(TARGET): $(GME_CXX_OBJECTS)
	$(CXX) -o $(TARGET) $^ $(GME_CXX_FLAGS) -s EXPORTED_FUNCTIONS=$(EXPORT_LIST)

clean:
	rm -f $(TARGET) $(THINGS_TO_CLEAN)
