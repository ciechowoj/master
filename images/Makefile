
MASTER=../master
SAMPLES=10000

PREFIX=

BLEND_FILES = \
	../models/CornellBoxDiffuse.blend \
	../models/CornellBoxSpecular.blend \
	../models/FurnaceTest.blend \
	../models/MetalRings.blend

# BLEND_FILES = $(wildcard ../models/*.blend)
PT_EXR_FILES = $(BLEND_FILES:../models/%.blend=%.PT.exr)
BPT_EXR_FILES = $(BLEND_FILES:../models/%.blend=%.BPT.exr)
VCM_EXR_FILES = $(BLEND_FILES:../models/%.blend=%.VCM.exr)
UPG_EXR_FILES = $(BLEND_FILES:../models/%.blend=%.UPG.exr)

# $(info $(EXR_FILES))

%.PT.exr: ../models/%.blend ../PT.hpp ../PT.cpp
	$(PREFIX) $(MASTER) $< --output=$@ --num-samples=$(SAMPLES) --batch --parallel --PT --snapshot=1000

%.BPT.exr: ../models/%.blend ../BPT.hpp ../BPT.cpp
	$(PREFIX) $(MASTER) $< --output=$@ --num-samples=$(SAMPLES) --batch --parallel --BPT --snapshot=1000

%.VCM.exr: ../models/%.blend ../UPG.hpp ../UPG.cpp
	$(PREFIX) $(MASTER) $< --output=$@ --num-samples=$(SAMPLES) --batch --parallel --VCM --max-radius=0.01 --snapshot=1000

%.UPG.exr: ../models/%.blend ../UPG.hpp ../UPG.cpp
	$(PREFIX) $(MASTER) $< --output=$@ --num-samples=$(SAMPLES) --batch --parallel --UPG --max-radius=0.01 --snapshot=1000

.PHONY: all pt bpt vcm upg

pt: $(PT_EXR_FILES)

bpt: $(BPT_EXR_FILES)

vcm: $(VCM_EXR_FILES)

upg: $(UPG_EXR_FILES)

all: pt bpt vcm upg
