#!/usr/bin/env powershell

param (
    [switch]$justPrint = $false,
    [switch]$printAverage = $false,
    [int]$beta = 2,
    [int]$numMinutes=240,
    [switch]$makeReference = $false
)

$masterPath = "./master.exe"

$commonArguments = @(
  "--parallel",
  "--batch",
  "--beta=$beta",
  "--num-minutes=$numMinutes",
  "--resolution=1024x1024",
  "--snapshot=360")

function Invoke-Master() {
    $baseName = [io.path]::GetFileNameWithoutExtension($args[0])
    $reference = "--reference=reference/$baseName.exr"
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

    if ($args.contains("--BPT")) {
      $technique = "BPT$beta$camera"
    }
    elseif ($args.contains("--UPG")) {
      $technique = "UPG$beta$radius$camera"
    }
    elseif ($args.contains("--VCM")) {
      $technique = "VCM$beta$radius$camera"
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

# Bearings.blend

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

New-Item -Name result -ItemType directory -Force | Out-Null
#Invoke-Master models/Bearings.blend --BPT $bearingsTraces
#Invoke-Master models/Bearings.blend --UPG --radius=0.1 $bearingsTraces

Invoke-Master models/Bathroom.blend --BPT
Invoke-Master models/BathroomDiscrete.blend --BPT
Invoke-Master models/Bearings.blend --BPT
Invoke-Master models/BreakfastRoom0.blend --BPT --camera=0
Invoke-Master models/BreakfastRoom0.blend --BPT --camera=1
Invoke-Master models/BreakfastRoom0.blend --BPT --camera=2
Invoke-Master models/BreakfastRoom1.blend --BPT --camera=0
Invoke-Master models/BreakfastRoom1.blend --BPT --camera=1
Invoke-Master models/BreakfastRoom1.blend --BPT --camera=2
Invoke-Master models/CrytekSponza.blend --BPT --camera=0
Invoke-Master models/CrytekSponza.blend --BPT --camera=1
Invoke-Master models/CrytekSponza.blend --BPT --camera=2

# Invoke-Master models/BreakfastRoom.blend --BPT
# Invoke-Master models/Bathroom.blend --UPG --radius=0.05
# Invoke-Master models/BathroomDiscrete.blend --UPG --radius=0.03
