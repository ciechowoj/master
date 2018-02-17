#!/usr/bin/env powershell

$numMinutes=360
$outputDirectory = "result"

function Invoke-Master() {
  $beta = 2
  $makeReference = $args.contains("-makeReference")
  $justPrint = $args.contains("-justPrint")

  $args = @($args | Where-Object { $_ -ne "-justPrint" -and $_ -ne "-makeReference" })

  $commonArguments = @(
    "--parallel",
    "--batch",
    "--beta=$beta",
    "--num-minutes=$global:numMinutes",
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

  $output = "--output=$global:outputDirectory/$basename.$technique.exr"
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

function Generate-Results([string]$model, [switch]$justPrint, [switch]$onlyUPG = $false, $traces, $camera, $radius) {
  New-Item -Name $global:outputDirectory -ItemType directory -Force | Out-Null

  Write-Host $model "camera" $camera
  if ($justPrint)
  {
    if (-not $onlyUPG)
    {
      Invoke-Master $model $camera --BPT @traces -justPrint
      Invoke-Master $model $camera --VCM $radius @traces -justPrint
      Invoke-Master $model $camera --UPG --from-camera $radius @traces -justPrint
    }

    Invoke-Master $model $camera --UPG $radius @traces -justPrint
  }
  else
  {
    if (-not $onlyUPG)
    {
      Invoke-Master $model $camera --BPT @traces
      Invoke-Master $model $camera --VCM $radius @traces
      Invoke-Master $model $camera --UPG --from-camera $radius @traces
    }

    Invoke-Master $model $camera --UPG $radius @traces
  }
}

function Bearings([switch]$justPrint, [switch]$onlyUPG = $false, [float]$radius)
{
  $model = "models/Bearings.blend"

  $traces = @(
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

  Generate-Results $model $traces --camera=0 --radius=$radius -justPrint:$justPrint -onlyUPG:$onlyUPG
}

function Bathroom([switch]$justPrint, [switch]$onlyUPG = $false, [float]$radius)
{
  $model = "models/Bathroom.blend"

  $traces = @(
    "--trace=150x380x8",
    "--trace=580x330x8",
    "--trace=870x550x8",
    "--trace=400x660x8",
    "--trace=850x575x8")

  Generate-Results $model $traces --camera=0 --radius=$radius -justPrint:$justPrint -onlyUPG:$onlyUPG
}

function BreakfastRoom1_Camera0([switch]$justPrint, [switch]$onlyUPG = $false, [float]$radius)
{
  $model = "models/BreakfastRoom1.blend"

  $traces = @(
    "--trace=300x850x8",
    "--trace=480x260x8",
    "--trace=550x550x8",
    "--trace=660x165x8",
    "--trace=700x250x8",
    "--trace=850x800x8",
    "--trace=140x900x8",
    "--trace=700x900x8",
    "--trace=200x550x8")

  Generate-Results $model $traces --camera=0 --radius=$radius -justPrint:$justPrint -onlyUPG:$onlyUPG # 0.02
}

function BreakfastRoom1_Camera1([switch]$justPrint, [switch]$onlyUPG = $false, [float]$radius)
{
  $model = "models/BreakfastRoom1.blend"

  $traces = @(
    "--trace=400x810x8",
    "--trace=420x170x8",
    "--trace=470x570x8",
    "--trace=630x485x8")

  Generate-Results $model $traces --camera=1 --radius=$radius -justPrint:$justPrint -onlyUPG:$onlyUPG # 0.02
}

function BreakfastRoom1_Camera2([switch]$justPrint, [switch]$onlyUPG = $false, [float]$radius)
{
  $model = "models/BreakfastRoom1.blend"

  $traces = @(
    "--trace=75x100x8",
    "--trace=435x400x8",
    "--trace=650x270x8",
    "--trace=800x270x8",
    "--trace=20x370x8",
    "--trace=850x310x8",
    "--trace=880x81x8",
    "--trace=360x750x8",
    "--trace=540x380x8")

  Generate-Results $model $traces --camera=2 --radius=$radius -justPrint:$justPrint -onlyUPG:$onlyUPG # 0.02
}


function BreakfastRoom2_Camera0([switch]$justPrint, [switch]$onlyUPG = $false, [float]$radius)
{
  $model = "models/BreakfastRoom2.blend"

  $traces = @(
    "--trace=190x930x8",
    "--trace=420x230x8",
    "--trace=460x410x8",
    "--trace=570x450x8",
    "--trace=670x260x8",
    "--trace=580x210x8",
    "--trace=690x185x8",
    "--trace=170x750x8",
    "--trace=500x820x8")

  Generate-Results $model $traces --camera=0 --radius=$radius -justPrint:$justPrint -onlyUPG:$onlyUPG # 0.02
}

function BreakfastRoom2_Camera1([switch]$justPrint, [switch]$onlyUPG = $false, [float]$radius)
{
  $model = "models/BreakfastRoom2.blend"

  $traces = @(
    "--trace=30x700x8",
    "--trace=75x660x8",
    "--trace=380x830x8",
    "--trace=400x450x8",
    "--trace=400x520x8",
    "--trace=450x790x8",
    "--trace=470x685x8",
    "--trace=500x730x8",
    "--trace=950x650x8")

  Generate-Results $model $traces --camera=1 --radius=$radius -justPrint:$justPrint -onlyUPG:$onlyUPG # 0.02
}

function BreakfastRoom2_Camera2([switch]$justPrint, [switch]$onlyUPG = $false, [float]$radius)
{
  $model = "models/BreakfastRoom2.blend"

  $traces = @(
    "--trace=780x340x8",
    "--trace=830x310x8",
    "--trace=255x395x8",
    "--trace=715x760x8",
    "--trace=460x130x8",
    "--trace=550x250x8",
    "--trace=940x180x8",
    "--trace=735x690x8",
    "--trace=20x170")

  Generate-Results $model $traces --camera=2 --radius=$radius -justPrint:$justPrint -onlyUPG:$onlyUPG # 0.03
}

function CrytekSponza_Camera0([switch]$justPrint, [switch]$onlyUPG = $false, [float]$radius)
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

  Generate-Results $model $traces --camera=0 --radius=$radius -justPrint:$justPrint -onlyUPG:$onlyUPG # 0.015
}

function CrytekSponza_Camera1([switch]$justPrint, [switch]$onlyUPG = $false, [float]$radius)
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

  Generate-Results $model $traces --camera=1 --radius=$radius -justPrint:$justPrint -onlyUPG:$onlyUPG # 0.015
}

function CrytekSponza_Camera2([switch]$justPrint, [switch]$onlyUPG = $false, [float]$radius)
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

  Generate-Results $model $traces --camera=2 --radius=$radius -justPrint:$justPrint -onlyUPG:$onlyUPG # 0.015
}

function Cluster1([switch]$justPrint)
{
  $global:numMinutes = 360
  $global:outputDirectory = "result"

  Bearings -justPrint:$justPrint -radius 0.01
  Bathroom -justPrint:$justPrint -radius 0.01
  BreakfastRoom1_Camera0 -justPrint:$justPrint -radius 0.02
  BreakfastRoom1_Camera1 -justPrint:$justPrint -radius 0.02
  BreakfastRoom1_Camera2 -justPrint:$justPrint -radius 0.02
  BreakfastRoom2_Camera0 -justPrint:$justPrint -radius 0.02
}

function Cluster2([switch]$justPrint)
{
  Cluster3 -justPrint:$justPrint

  $global:numMinutes = 360
  $global:outputDirectory = "result"

  BreakfastRoom2_Camera1 -justPrint:$justPrint -radius 0.02
  BreakfastRoom2_Camera2 -justPrint:$justPrint -radius 0.02
  CrytekSponza_Camera0 -justPrint:$justPrint -radius 0.015
  CrytekSponza_Camera1 -justPrint:$justPrint -radius 0.015
  CrytekSponza_Camera2 -justPrint:$justPrint -radius 0.015
}

function Cluster3([switch]$justPrint)
{
  $global:numMinutes = 60
  $global:outputDirectory = "radii"

  Bearings -justPrint:$justPrint -onlyUPG -radius 0.005
  Bearings -justPrint:$justPrint -onlyUPG -radius 0.010
  Bearings -justPrint:$justPrint -onlyUPG -radius 0.015
  Bearings -justPrint:$justPrint -onlyUPG -radius 0.020
  Bearings -justPrint:$justPrint -onlyUPG -radius 0.025
  Bearings -justPrint:$justPrint -onlyUPG -radius 0.030

  BreakfastRoom2_Camera0 -justPrint:$justPrint -onlyUPG -radius 0.010
  BreakfastRoom2_Camera0 -justPrint:$justPrint -onlyUPG -radius 0.015
  BreakfastRoom2_Camera0 -justPrint:$justPrint -onlyUPG -radius 0.020
  BreakfastRoom2_Camera0 -justPrint:$justPrint -onlyUPG -radius 0.025
  BreakfastRoom2_Camera0 -justPrint:$justPrint -onlyUPG -radius 0.030
  BreakfastRoom2_Camera0 -justPrint:$justPrint -onlyUPG -radius 0.035

  BreakfastRoom1_Camera1 -justPrint:$justPrint -onlyUPG -radius 0.010
  BreakfastRoom1_Camera1 -justPrint:$justPrint -onlyUPG -radius 0.015
  BreakfastRoom1_Camera1 -justPrint:$justPrint -onlyUPG -radius 0.020
  BreakfastRoom1_Camera1 -justPrint:$justPrint -onlyUPG -radius 0.025
  BreakfastRoom1_Camera1 -justPrint:$justPrint -onlyUPG -radius 0.030
  BreakfastRoom1_Camera1 -justPrint:$justPrint -onlyUPG -radius 0.035

  CrytekSponza_Camera1 -justPrint:$justPrint -onlyUPG -radius 0.010
  CrytekSponza_Camera1 -justPrint:$justPrint -onlyUPG -radius 0.015
  CrytekSponza_Camera1 -justPrint:$justPrint -onlyUPG -radius 0.020
  CrytekSponza_Camera1 -justPrint:$justPrint -onlyUPG -radius 0.025
  CrytekSponza_Camera1 -justPrint:$justPrint -onlyUPG -radius 0.030
  CrytekSponza_Camera1 -justPrint:$justPrint -onlyUPG -radius 0.035
}

function Cluster4([switch]$justPrint)
{
  $global:numMinutes = 180
  $global:outputDirectory = "radii"
  $radius = "--radius=0.03"

  Invoke-Master "models/TestCase40.blend" --BPT $(If ($justPrint) {"-justPrint"}) -makeReference
  Invoke-Master "models/TestCase40.blend" --UPG $(If ($justPrint) {"-justPrint"}) $radius -makeReference
  Invoke-Master "models/TestCase41.blend" --BPT $(If ($justPrint) {"-justPrint"}) -makeReference
  Invoke-Master "models/TestCase41.blend" --UPG $(If ($justPrint) {"-justPrint"}) $radius -makeReference
  Invoke-Master "models/TestCase42.blend" --BPT $(If ($justPrint) {"-justPrint"}) -makeReference
  Invoke-Master "models/TestCase42.blend" --UPG $(If ($justPrint) {"-justPrint"}) $radius -makeReference
  Invoke-Master "models/TestCase43.blend" --BPT $(If ($justPrint) {"-justPrint"}) -makeReference
  Invoke-Master "models/TestCase43.blend" --UPG $(If ($justPrint) {"-justPrint"}) $radius -makeReference
}

function Cluster5([switch]$justPrint)
{
  $global:numMinutes = 1200
  $global:outputDirectory = "result"

  Bearings -justPrint:$justPrint -onlyUPG -radius 0.020
}

function Cluster6([switch]$justPrint)
{
  $global:numMinutes = 480
  $global:outputDirectory = "result"
  $radius = "--radius=0.03"

  Invoke-Master "models/Bearings.blend" --BPT $(If ($justPrint) {"-justPrint"}) -makeReference --camera=2
  Invoke-Master "models/Bearings.blend" --UPG $(If ($justPrint) {"-justPrint"}) $radius --camera=2
}
