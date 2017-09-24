
param (
    [switch]$justPrint = $false,
    [switch]$printAverage = $false,
    [int]$beta = 2
)

$masterPath = "./master.exe"

$commonArguments = @(
  "--parallel",
  #"--batch",
  "--beta=$beta",
  "--num-minutes=240",
  "--resolution=1024x1024",
  "--snapshot=360")

function Invoke-Master() {
    $baseName = [io.path]::GetFileNameWithoutExtension($args[0])
    $reference = "--reference=reference/$baseName.exr"
    $technique = $null
    $extra = ""

    $radius = ""
    $args | where-object { $_ -match "--radius=((\d|\.)*)" } | out-null
    if ($matches.length -ge 1) {
      $radius = "." + ($matches[1] -replace "\.","_")
    }

    if ($args.contains("--BPT")) {
      $technique = "BPT$beta"
    }
    elseif ($args.contains("--UPG")) {
      $technique = "UPG$beta$radius"
    }
    elseif ($args.contains("--VCM")) {
      $technique = "VCM$beta$radius"
    }

    $output = "--output=result/$basename.$technique.exr"

    $finalArguments = $commonArguments, $reference, $output, $args

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
Invoke-Master models/Bearings.blend --BPT $bearingsTraces
Invoke-Master models/Bearings.blend --UPG --radius=0.1 $bearingsTraces

