#!/usr/bin/env bash

master --beta=2 --resolution=1024x1024 --parallel models/Bearings.blend --UPG \
	--radius=0.1 --output=Bearings.UPG2.exr --snapshot=360 --batch --num-minutes=180 \
	--reference=reference/Bearings.BPT.exr \
	--trace=490x180 \
	--trace=260x310 \
	--trace=220x690 \
	--trace=290x690 \
	--trace=700x420 \
	--trace=900x900

master --beta=2 --resolution=1024x1024 --parallel models/Bearings.blend --BPT \
               --output=Bearings.BPT2.exr --snapshot=360 --batch --num-minutes=180 \
  --reference=reference/Bearings.BPT.exr \
  --trace=490x180 \
	--trace=260x310 \
	--trace=220x690 \
	--trace=290x690 \
	--trace=700x420 \
	--trace=900x900
