
MASTER=../master
SAMPLES=10000

PREFIX=timeout 2h

BLEND_FILES = \
	../models/CornellBoxDiffuse.blend \
	../models/CornellBoxSpecular.blend \
	../models/FurnaceTest.blend \
	../models/MetalRings.blend

# BLEND_FILES = $(wildcard ../models/*.blend)
EXR_FILES = \
	$(BLEND_FILES:../models/%.blend=%.PT.exr) \
	$(BLEND_FILES:../models/%.blend=%.BPT.exr) \
	$(BLEND_FILES:../models/%.blend=%.VCM.exr) \
	$(BLEND_FILES:../models/%.blend=%.UPG.exr)

# $(info $(EXR_FILES))

%.PT.exr: ../models/%.blend ../PT.hpp ../PT.cpp
	$(PREFIX) $(MASTER) $< --output=$@ --num-samples=$(SAMPLES) --batch --parallel --PT

%.BPT.exr: ../models/%.blend ../BPT.hpp ../BPT.cpp
	$(PREFIX) $(MASTER) $< --output=$@ --num-samples=$(SAMPLES) --batch --parallel --BPT

%.VCM.exr: ../models/%.blend ../UPG.hpp ../UPG.cpp
	$(PREFIX) $(MASTER) $< --output=$@ --num-samples=$(SAMPLES) --batch --parallel --VCM

%.UPG.exr: ../models/%.blend ../UPG.hpp ../UPG.cpp
	$(PREFIX) $(MASTER) $< --output=$@ --num-samples=$(SAMPLES) --batch --parallel --UPG

.PHONY: all

all: $(EXR_FILES)

