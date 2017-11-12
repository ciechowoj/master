#!/usr/bin/env powershell

function Invoke-Master() {
  $beta = 2
  $numMinutes=360
  $makeReference = $false
  $justPrint = $args.contains("-justPrint")

  $args = @($args | Where-Object { $_ -ne "-justPrint" })

  $commonArguments = @(
    "--parallel",
    "--batch",
    "--beta=$beta",
    "--num-minutes=$numMinutes",
    "--resolution=1024x1024",
    "--snapshot=360")

  $masterPath = "./master.exe"

  $baseName = [io.path]::GetFileNameWithoutExtension($args[0])
  $technique = $null
  $extra = ""

  $radius = ""
  $args | where-object { $_ -match "--radius=((\d|\.)+)" } | out-null
  if ($matches.length -ge 1) {
    $radius = "." + ($matches[1] -replace "\.","_")
  }

  $camera = ""
  $args | where-object { $_ -match "--camera=((\d)+)" } | out-null
  if ($matches.length -ge 1) {
    $camera = ".cam" + $matches[1]
  }

  $reference = "--reference=reference/$baseName$camera.exr"

  if ($args.contains("--BPT")) {
    $technique = "BPT$beta$camera"
  }
  elseif ($args.contains("--UPG")) {
    $technique = "UPG$beta$radius$camera"
  }
  elseif ($args.contains("--VCM")) {
    $technique = "VCM$beta$radius$camera"
  }

  if ($args.contains("--from-camera")) {
    $technique = $technique + ".from.camera"
  }

  $output = "--output=result/$basename.$technique.exr"
  $finalArguments = $commonArguments, $reference, $output, $args

  if ($makeReference) {
    $output = "--output=reference/$basename$camera.exr"
    $finalArguments = $commonArguments, $output, $args
  }

  if ($justPrint) {
      Write-Host master @finalArguments
  }
  else {
      if (Test-Path $masterPath) {
          . $masterPath @finalArguments
      }
      else {
          . master @finalArguments
      }
  }
}

function Generate-Results([string]$model, [switch]$justPrint, $traces, $camera, $radius) {
  New-Item -Name result -ItemType directory -Force | Out-Null

  Write-Host $model "camera" $camera
  if ($justPrint)
  {
    Invoke-Master $model $camera --BPT @traces -justPrint
    Invoke-Master $model $camera --VCM $radius @traces -justPrint
    Invoke-Master $model $camera --UPG $radius @traces -justPrint
    Invoke-Master $model $camera --UPG --from-camera $radius @traces -justPrint
  }
  else
  {
    Invoke-Master $model $camera --BPT @traces
    Invoke-Master $model $camera --VCM $radius @traces
    Invoke-Master $model $camera --UPG $radius @traces
    Invoke-Master $model $camera --UPG --from-camera $radius @traces
  }
}

# Bearings.blend
# Invoke-Master models/Bearings.blend --BPT $bearingsTraces
# Invoke-Master models/Bathroom.blend --BPT
# Invoke-Master models/BathroomDiscrete.blend --BPT
# Invoke-Master models/Bearings.blend --BPT
# Invoke-Master models/BreakfastRoom1.blend --BPT --camera=0
# Invoke-Master models/BreakfastRoom1.blend --BPT --camera=1
# Invoke-Master models/BreakfastRoom1.blend --BPT --camera=2
# Invoke-Master models/BreakfastRoom2.blend --BPT --camera=0
# Invoke-Master models/BreakfastRoom2.blend --BPT --camera=1
# Invoke-Master models/BreakfastRoom2.blend --BPT --camera=2
# Invoke-Master models/CrytekSponza.blend --BPT --camera=0
# Invoke-Master models/CrytekSponza.blend --BPT --camera=1
# Invoke-Master models/CrytekSponza.blend --BPT --camera=2
# Invoke-Master models/BreakfastRoom.blend --BPT
# Invoke-Master models/Bathroom.blend --UPG --radius=0.05
# Invoke-Master models/BathroomDiscrete.blend --UPG --radius=0.03

function Bearings([switch]$justPrint)
{
  $bearingsTraces = @(
    "--trace=900x125x8",
    "--trace=505x200x8",
    "--trace=220x335x8",
    "--trace=290x335x8",
    "--trace=690x430x8",
    "--trace=700x605x8",
    "--trace=400x640x8",
    "--trace=520x710x8",
    "--trace=260x715x8",
    "--trace=210x805x8",
    "--trace=640x815x8",
    "--trace=490x845x8")
}

function BreakfastRoom1_Camera0([switch]$justPrint)
{
  $model = "models/BreakfastRoom1.blend"

  $traces = @(
    "--trace=300x850x8",
    "--trace=480x260x8",
    "--trace=550x550x8",
    "--trace=660x165x8",
    "--trace=700x250x8",
    "--trace=850x800x8")

  Generate-Results $model $traces --camera=0 --radius=0.02 -justPrint:$justPrint
}

function BreakfastRoom1_Camera1([switch]$justPrint)
{
  $model = "models/BreakfastRoom1.blend"
  
  $traces = @(
    "--trace=400x810x8",
    "--trace=420x170x8",
    "--trace=470x570x8",
    "--trace=630x485x8")

  Generate-Results $model $traces --camera=1 --radius=0.02 -justPrint:$justPrint
}

function BreakfastRoom1_Camera2([switch]$justPrint)
{
  $model = "models/BreakfastRoom1.blend"
  
  $traces = @(
    "--trace=75x100x8",
    "--trace=435x400x8",
    "--trace=650x270x8",
    "--trace=800x270x8")

  Generate-Results $model $traces --camera=2 --radius=0.02 -justPrint:$justPrint
}

function BreakfastRoom2_Camera0([switch]$justPrint)
{
  $model = "models/BreakfastRoom2.blend"

  $traces = @(
    "--trace=190x930x8",
    "--trace=420x230x8",
    "--trace=460x410x8",
    "--trace=570x450x8",
    "--trace=670x260x8",
    "--trace=580x210x8",
    "--trace=690x185x8")

  Generate-Results $model $traces --camera=0 --radius=0.02 -justPrint:$justPrint
}

function BreakfastRoom2_Camera1([switch]$justPrint)
{
  $model = "models/BreakfastRoom2.blend"

  $traces = @(
    "--trace=400x450x8",
    "--trace=400x520x8",
    "--trace=450x790x8",
    "--trace=470x685x8",
    "--trace=500x730x8")

  Generate-Results $model $traces --camera=1 --radius=0.02 -justPrint:$justPrint
}

function BreakfastRoom2_Camera2([switch]$justPrint)
{
  $model = "models/BreakfastRoom2.blend"

  $traces = @(
    "--trace=780x340x8",
    "--trace=830x310x8",
    "--trace=255x395x8",
    "--trace=715x760x8")

  Generate-Results $model $traces --camera=2 --radius=0.03 -justPrint:$justPrint
}

function CrytekSponza_Camera0([switch]$justPrint)
{
  $model = "models/CrytekSponza.blend"

  $traces = @(
    "--trace=156x575x8",
    "--trace=313x498x8",
    "--trace=464x302x8",
    "--trace=478x467x8",
    "--trace=789x865x8",
    "--trace=630x300x8",
    "--trace=800x540x8",
    "--trace=280x950x8",
    "--trace=750x200x8")

  Generate-Results $model $traces --camera=0 --radius=0.015 -justPrint:$justPrint
}

function CrytekSponza_Camera1([switch]$justPrint)
{
  $model = "models/CrytekSponza.blend"

  $traces = @(
    "--trace=20x335x8",
    "--trace=320x180x8",
    "--trace=470x813x8",
    "--trace=570x660x8",
    "--trace=720x440x8",
    "--trace=300x540x8",
    "--trace=375x430x8",
    "--trace=300x670x8",
    "--trace=590x700x8")

  Generate-Results $model $traces --camera=1 --radius=0.015 -justPrint:$justPrint
}

function CrytekSponza_Camera2([switch]$justPrint)
{
  $model = "models/CrytekSponza.blend"

  $traces = @(
    "--trace=500x370x8",
    "--trace=675x670x8",
    "--trace=840x880x8",
    "--trace=450x550x8",
    "--trace=450x250x8",
    "--trace=150x900x8",
    "--trace=290x680x8",
    "--trace=700x670x8",
    "--trace=600x600x8")
  
  Generate-Results $model $traces --camera=2 --radius=0.015 -justPrint:$justPrint
}

