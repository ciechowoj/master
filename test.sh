#!/usr/bin/env bash

master --beta=2 --parallel models/Bearings.d.blend --UPG --radius=0.1 --output=Bearings.Diffuse.UPG2.exr --snapshot=360 --batch --num-samples=10000
master --beta=2 --parallel models/Bearings.d.blend --BPT              --output=Bearings.Diffuse.BPT2.exr --snapshot=360 --batch --num-samples=10000
master --beta=2 --parallel models/Bearings.s.blend --UPG --radius=0.1 --output=Bearings.Directional.UPG2.exr --snapshot=360 --batch --num-samples=10000
master --beta=2 --parallel models/Bearings.s.blend --BPT              --output=Bearings.Directional.BPT2.exr --snapshot=360 --batch --num-samples=10000
