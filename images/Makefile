
MASTER=master
SAMPLES=10000

# PREFIX=nohop timeout 1h
PREFIX=

BLEND_FILES = ../models/CornellBoxDiffuse.blend ../models/FurnaceTest.blend
# BLEND_FILES = $(wildcard ../models/*.blend)
EXR_FILES = \
	$(BLEND_FILES:../models/%.blend=%.PT.exr) \
	$(BLEND_FILES:../models/%.blend=%.BPT.exr) \
	$(BLEND_FILES:../models/%.blend=%.VCM.exr) \
	$(BLEND_FILES:../models/%.blend=%.UPG.exr)

# $(info $(EXR_FILES))

%.PT.exr: ../models/%.blend ../PT.hpp ../PT.cpp
	$(PREFIX) $(MASTER) $< --output=$@ --num-samples=$(SAMPLES) --parallel --PT

%.BPT.exr: %.blend ../BPT.hpp ../BPT.cpp
	$(PREFIX) $(MASTER) $< --output=$@ --num-samples=$(SAMPLES) --parallel --BPT

%.VCM.exr: %.blend ../UPG.hpp ../UPG.cpp
	$(PREFIX) $(MASTER) $< --output=$@ --num-samples=$(SAMPLES) --parallel --VCM

%.UPG.exr: %.blend ../UPG.hpp ../UPG.cpp
	$(PREFIX) $(MASTER) $< --output=$@ --num-samples=$(SAMPLES) --parallel --UPG

.PHONY: all

all: $(EXR_FILES)

