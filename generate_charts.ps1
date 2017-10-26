#!/usr/bin/env powershell



master gnuplot --output=BreakfastRoom1.cam0.png --traces BreakfastRoom1.BPT2.cam0.exr BreakfastRoom1.UPG2.0_02.cam0.exr BreakfastRoom1.VCM2.0_02.cam0.exr
master gnuplot --output=BreakfastRoom1.cam1.png --traces BreakfastRoom1.BPT2.cam1.exr BreakfastRoom1.UPG2.0_02.cam1.exr BreakfastRoom1.VCM2.0_02.cam1.exr
master gnuplot --output=BreakfastRoom1.cam2.png --traces BreakfastRoom1.BPT2.cam2.exr BreakfastRoom1.UPG2.0_02.cam2.exr BreakfastRoom1.VCM2.0_02.cam2.exr

master gnuplot --output=BreakfastRoom2.cam0.png --traces BreakfastRoom2.BPT2.cam0.exr BreakfastRoom2.UPG2.0_02.cam0.exr BreakfastRoom2.VCM2.0_02.cam0.exr
master gnuplot --output=BreakfastRoom2.cam1.png --traces BreakfastRoom2.BPT2.cam1.exr BreakfastRoom2.UPG2.0_02.cam1.exr BreakfastRoom2.VCM2.0_02.cam1.exr
master gnuplot --output=BreakfastRoom2.cam2.png --traces BreakfastRoom2.BPT2.cam2.exr BreakfastRoom2.UPG2.0_02.cam2.exr BreakfastRoom2.VCM2.0_02.cam2.exr

master gnuplot --output=CrytekSponza.cam0.png --traces CrytekSponza.BPT2.cam0.exr CrytekSponza.UPG2.0_015.cam0.exr CrytekSponza.VCM2.0_015.cam0.exr
master gnuplot --output=CrytekSponza.cam1.png --traces CrytekSponza.BPT2.cam1.exr CrytekSponza.UPG2.0_015.cam1.exr CrytekSponza.VCM2.0_015.cam1.exr
master gnuplot --output=CrytekSponza.cam2.png --traces CrytekSponza.BPT2.cam2.exr CrytekSponza.UPG2.0_015.cam2.exr CrytekSponza.VCM2.0_015.cam2.exr

