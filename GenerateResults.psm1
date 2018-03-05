#!/usr/bin/env powershell

. ./traces.ps1

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

  if ($justPrint)
  {
    if (-not $onlyUPG)
    {
      Invoke-Master $model $camera --BPT @traces -justPrint
      Invoke-Master $model $camera --VCM $radius @traces -justPrint
    }

    Invoke-Master $model $camera --UPG $radius @traces -justPrint
    Invoke-Master $model $camera --UPG --from-camera $radius @traces -justPrint
  }
  else
  {
    if (-not $onlyUPG)
    {
      Invoke-Master $model $camera --BPT @traces
      Invoke-Master $model $camera --VCM $radius @traces
    }

    Invoke-Master $model $camera --UPG $radius @traces
    Invoke-Master $model $camera --UPG --from-camera $radius @traces
  }
}

function Bearings([switch]$justPrint, [switch]$onlyUPG = $false, [float]$radius)
{
  $model = "models/Bearings.blend"
  Generate-Results $model $bearingsTraces --camera=0 --radius=$radius -justPrint:$justPrint -onlyUPG:$onlyUPG
}

function Bathroom([switch]$justPrint, [switch]$onlyUPG = $false, [float]$radius)
{
  $model = "models/Bathroom.blend"
  Generate-Results $model $bathroomTraces --camera=0 --radius=$radius -justPrint:$justPrint -onlyUPG:$onlyUPG
}

function BreakfastRoom1_Camera0([switch]$justPrint, [switch]$onlyUPG = $false, [float]$radius)
{
  $model = "models/BreakfastRoom1.blend"
  Generate-Results $model $breakfastRoom1Camera0Traces --camera=0 --radius=$radius -justPrint:$justPrint -onlyUPG:$onlyUPG # 0.02
}

function BreakfastRoom1_Camera1([switch]$justPrint, [switch]$onlyUPG = $false, [float]$radius)
{
  $model = "models/BreakfastRoom1.blend"
  Generate-Results $model $breakfastRoom1Camera1Traces --camera=1 --radius=$radius -justPrint:$justPrint -onlyUPG:$onlyUPG # 0.02
}

function BreakfastRoom1_Camera2([switch]$justPrint, [switch]$onlyUPG = $false, [float]$radius)
{
  $model = "models/BreakfastRoom1.blend"
  Generate-Results $model $breakfastRoom1Camera2Traces --camera=2 --radius=$radius -justPrint:$justPrint -onlyUPG:$onlyUPG # 0.02
}


function BreakfastRoom2_Camera0([switch]$justPrint, [switch]$onlyUPG = $false, [float]$radius)
{
  $model = "models/BreakfastRoom2.blend"
  Generate-Results $model $breakfastRoom2Camera0Traces --camera=0 --radius=$radius -justPrint:$justPrint -onlyUPG:$onlyUPG # 0.02
}

function BreakfastRoom2_Camera1([switch]$justPrint, [switch]$onlyUPG = $false, [float]$radius)
{
  $model = "models/BreakfastRoom2.blend"
  Generate-Results $model $breakfastRoom2Camera1Traces --camera=1 --radius=$radius -justPrint:$justPrint -onlyUPG:$onlyUPG # 0.02
}

function BreakfastRoom2_Camera2([switch]$justPrint, [switch]$onlyUPG = $false, [float]$radius)
{
  $model = "models/BreakfastRoom2.blend"
  Generate-Results $model $breakfastRoom2Camera2Traces --camera=2 --radius=$radius -justPrint:$justPrint -onlyUPG:$onlyUPG # 0.03
}

function CrytekSponza_Camera0([switch]$justPrint, [switch]$onlyUPG = $false, [float]$radius)
{
  $model = "models/CrytekSponza.blend"
  Generate-Results $model $crytekSponzaCamera0Traces --camera=0 --radius=$radius -justPrint:$justPrint -onlyUPG:$onlyUPG # 0.015
}

function CrytekSponza_Camera1([switch]$justPrint, [switch]$onlyUPG = $false, [float]$radius)
{
  $model = "models/CrytekSponza.blend"
  Generate-Results $model $crytekSponzaCamera1Traces --camera=1 --radius=$radius -justPrint:$justPrint -onlyUPG:$onlyUPG # 0.015
}

function CrytekSponza_Camera2([switch]$justPrint, [switch]$onlyUPG = $false, [float]$radius)
{
  $model = "models/CrytekSponza.blend"
  Generate-Results $model $crytekSponzaCamera2Traces --camera=2 --radius=$radius -justPrint:$justPrint -onlyUPG:$onlyUPG # 0.015
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

  # Invoke-Master "models/Bearings.blend" --BPT $(If ($justPrint) {"-justPrint"}) -makeReference --camera=2
  Invoke-Master "models/Bearings.blend" --UPG $(If ($justPrint) {"-justPrint"}) $radius --camera=2
}

function Invoke-MasterV2(
  [string]$path,
  [switch]$justPrint,
  [switch]$makeReference,
  [switch]$fromCamera,
  [single]$beta = 2,
  [single]$radius,
  [int]$camera,
  [int]$numMinutes,
  [string]$technique,
  $traces) {

  $commonArguments = @(
    $path,
    "--parallel",
    "--batch",
    "--beta=$beta",
    "--num-minutes=$numMinutes",
    "--resolution=1024x1024",
    "--snapshot=360",
    "--$technique")

  $masterPath = "./master.exe"

  $baseName = [io.path]::GetFileNameWithoutExtension($path)

  $radiusString = "." + (($radius).ToString("0.000") -replace "\.", "_")
  $cameraString = ".cam$camera"

  $techniqueString = "PT$beta$cameraString"
  $radiusArgument = $null

  if ($technique -eq "BPT") {
    $techniqueString = "BPT$beta$cameraString"
  }
  elseif ($technique -eq "UPG") {
    $techniqueString = "UPG$beta$radiusString$cameraString"
    $radiusArgument = "--radius=$radius"
  }
  elseif ($technique -eq "VCM") {
    $techniqueString = "VCM$beta$radiusString$cameraString"
    $radiusArgument = "--radius=$radius"
  }

  if ($fromCamera) {
    $techniqueString = $techniqueString + ".from.camera"
  }

  $output = "--output=$global:outputDirectory/$basename.$techniqueString.exr"

  $reference = "--reference=reference/$baseName$cameraString.exr"

  $finalArguments = $commonArguments, $reference, $output, $radiusArgument, $traces

  if ($makeReference) {
    $output = "--output=reference/$basename$cameraString.exr"
    $finalArguments = $commonArguments, $output, $radiusArgument
  }

  if ($justPrint) {
    Write-Host master @finalArguments
  }
  else {
    New-Item $global:outputDirectory -ItemType Directory -Force | Out-Null

    if (Test-Path $masterPath) {
        . $masterPath @finalArguments
    }
    else {
        . master @finalArguments
    }
  }
}

function Cluster7([switch]$justPrint) {
  function Invoke-MasterLocal(
    [string]$path,
    [switch]$justPrint,
    [single]$radius,
    [int]$camera,
    $traces) {

    Invoke-MasterV2 `
      $path `
      -justPrint:$justPrint `
      -beta 2 `
      -radius $radius `
      -camera $camera `
      -numMinutes 20 `
      -technique "VCM" `
      -traces $traces
  }

  function Invoke-MasterRadii(
    [string]$path,
    [switch]$justPrint,
    [single]$radius,
    [int]$camera,
    $traces) {
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.010
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.015
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.020
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.025
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.030
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.035
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.040
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.045
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.050
  }

  function Invoke-MasterRadiiExtra(
    [string]$path,
    [switch]$justPrint,
    [single]$radius,
    [int]$camera,
    $traces) {
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.055
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.060
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.065
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.070
  }

  $global:outputDirectory = "radii"

  Invoke-MasterRadii models/Bathroom.blend -justPrint:$justPrint -camera 0 -traces $bathroomTraces
  Invoke-MasterRadii models/Bearings.blend -justPrint:$justPrint -camera 0 -traces $bearingsTraces
  Invoke-MasterRadii models/BreakfastRoom1.blend -justPrint:$justPrint -camera 0 -traces $breakfastRoom1Camera0Traces
  Invoke-MasterRadii models/BreakfastRoom1.blend -justPrint:$justPrint -camera 1 -traces $breakfastRoom1Camera1Traces
  Invoke-MasterRadii models/BreakfastRoom1.blend -justPrint:$justPrint -camera 2 -traces $breakfastRoom1Camera2Traces
  Invoke-MasterRadii models/BreakfastRoom2.blend -justPrint:$justPrint -camera 0 -traces $breakfastRoom2Camera0Traces
  Invoke-MasterRadii models/BreakfastRoom2.blend -justPrint:$justPrint -camera 1 -traces $breakfastRoom2Camera1Traces
  Invoke-MasterRadii models/BreakfastRoom2.blend -justPrint:$justPrint -camera 2 -traces $breakfastRoom2Camera2Traces
  Invoke-MasterRadii models/CrytekSponza.blend -justPrint:$justPrint -camera 0 -traces $crytekSponzaCamera0Traces
  Invoke-MasterRadii models/CrytekSponza.blend -justPrint:$justPrint -camera 1 -traces $crytekSponzaCamera1Traces
  Invoke-MasterRadii models/CrytekSponza.blend -justPrint:$justPrint -camera 2 -traces $crytekSponzaCamera2Traces
}

function Cluster8([switch]$justPrint) {
  function Invoke-MasterLocal(
    [string]$path,
    [switch]$justPrint,
    [single]$radius,
    [int]$camera,
    $traces) {

    Invoke-MasterV2 `
      $path `
      -justPrint:$justPrint `
      -beta 2 `
      -radius $radius `
      -camera $camera `
      -numMinutes 20 `
      -technique "VCM" `
      -traces $traces
  }

  function Invoke-MasterRadiiExtra(
    [string]$path,
    [switch]$justPrint,
    [single]$radius,
    [int]$camera,
    $traces) {
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.055
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.060
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.065
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.070
  }

  $global:outputDirectory = "radii"

  Invoke-MasterRadiiExtra models/Bathroom.blend -justPrint:$justPrint -camera 0 -traces $bathroomTraces
  Invoke-MasterRadiiExtra models/BreakfastRoom1.blend -justPrint:$justPrint -camera 1 -traces $breakfastRoom1Camera1Traces
  Invoke-MasterRadiiExtra models/BreakfastRoom2.blend -justPrint:$justPrint -camera 1 -traces $breakfastRoom2Camera1Traces
}

function Cluster9([switch]$justPrint)
{
  $global:numMinutes = 60
  $global:outputDirectory = "radii"

  Bathroom -justPrint:$justPrint -onlyUPG -radius 0.050
  Bathroom -justPrint:$justPrint -onlyUPG -radius 0.055
  Bathroom -justPrint:$justPrint -onlyUPG -radius 0.060
  Bathroom -justPrint:$justPrint -onlyUPG -radius 0.065
  Bathroom -justPrint:$justPrint -onlyUPG -radius 0.070
}

function Cluster10([switch]$justPrint) {
  function Invoke-MasterLocal(
    [string]$path,
    [switch]$justPrint,
    [single]$radius,
    [int]$camera,
    $traces) {

    Invoke-MasterV2 `
      $path `
      -justPrint:$justPrint `
      -beta 2 `
      -radius $radius `
      -camera $camera `
      -numMinutes 60 `
      -technique "VCM" `
      -traces $traces
  }

  function Invoke-MasterRadiiExtra(
    [string]$path,
    [switch]$justPrint,
    [single]$radius,
    [int]$camera,
    $traces) {
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.07
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.08
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.09
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.10
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.11
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.12
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.13
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.14
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.2
    Invoke-MasterLocal $path -justPrint:$justPrint -camera $camera -traces $traces -radius 0.3
  }

  $global:outputDirectory = "radii"

  Invoke-MasterRadiiExtra models/Bathroom.blend -justPrint:$justPrint -camera 0 -traces $bathroomTraces
}

function Cluster11([switch]$justPrint)
{
  $global:numMinutes = 30
  $global:outputDirectory = "radii"

  Bathroom -justPrint:$justPrint -onlyUPG -radius 0.05
  Bathroom -justPrint:$justPrint -onlyUPG -radius 0.06
  Bathroom -justPrint:$justPrint -onlyUPG -radius 0.07
  Bathroom -justPrint:$justPrint -onlyUPG -radius 0.08
  Bathroom -justPrint:$justPrint -onlyUPG -radius 0.09
  Bathroom -justPrint:$justPrint -onlyUPG -radius 0.10
  Bathroom -justPrint:$justPrint -onlyUPG -radius 0.11
  Bathroom -justPrint:$justPrint -onlyUPG -radius 0.12
  Bathroom -justPrint:$justPrint -onlyUPG -radius 0.13
  Bathroom -justPrint:$justPrint -onlyUPG -radius 0.14
}

