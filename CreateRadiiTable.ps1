#!/usr/bin/env powershell

function Get-Images() {
  Get-ChildItem radii `
    | ForEach-Object { `
      if ($_ -match "^(?<Scene>\w*)\.(?<Method>\w*)\d\.(?<Radius>\w*)\.cam(?<Camera>\d*)(?<FromCamera>\.from\.camera\.|\.)exr") {
        $properties = @{
          Path = $_.FullName;
          Scene = $Matches.Scene + "^$($Matches.Camera)";
          Method = $Matches.Method + $(If ($Matches.FromCamera -eq ".from.camera.") { "^*" } Else { "" });
          Radius = [single]($Matches.Radius -replace '_', '.');
          Camera = $Matches.Camera;
        }

        New-Object -TypeName PSObject -Prop $properties
      }
    }
}

function Get-Error([string]$path) {
  (. master measurements "$path" | Select-Object -Last 1) -split '\s+' `
    | Select-Object -First 3
}

function Get-ImagesWithErrors() {
  Get-Images `
    | ForEach-Object { `
      $error = Get-Error $_.Path
      Add-Member -InputObject $_ -MemberType NoteProperty -Name Time -Value ([single]$error[0])
      Add-Member -InputObject $_ -MemberType NoteProperty -Name RmsError -Value ([single]$error[1])
      Add-Member -InputObject $_ -MemberType NoteProperty -Name AbsError -Value ([single]$error[2])
      $_
    }
}

function Get-RadiiList($images) {
  $set = New-Object System.Collections.Generic.HashSet[single]

  foreach ($image in $images) {
    $set.Add($image.Radius) | Out-Null
  }

  $set | Sort-Object
}

function Group-Radii($images) {
  function Convert-ToAssociative($group) {
    $array = @{}

    foreach ($_ in $group) {
      $properties = @{
        Radius = $_.Radius;
        Time = $_.Time;
        RmsError = $_.RmsError;
        AbsError = $_.AbsError;
      }

      $array[$_.Radius] = New-Object -TypeName PSObject -Prop $properties
    }

    $array
  }

  $images `
    | Group-Object -Property Scene,Camera,Method `
    | ForEach-Object {
      $radii = Convert-ToAssociative $_.Group;
      $radiiKeys = @($radii.Keys)
      $radiiValues = @($radii.Values | ForEach-Object { $_.AbsError })
      $minError = [single]($radiiValues | Measure-Object -Minimum).Minimum;
      $minRadius = $radiiKeys[$radiiValues.IndexOf($minError)]

      $properties = @{
        Path = $_.Group[0].Path;
        Scene = $_.Group[0].Scene;
        Method = $_.Group[0].Method;
        Radii = $radii;
        MinError = $minError;
        MinRadius = $minRadius;
        Camera = $_.Group[0].Camera;
        FromCamera = $_.Group[0].FromCamera -eq ".from.camera.";
      }

      New-Object -TypeName PSObject -Prop $properties
    }
}

function Format-RadiiTable($images) {
  function Format-Radii($expected, $actual) {
    $minimum = ($actual.Values | Measure-Object -Minimum -Property AbsError).Minimum

    $exponent = [int][system.math]::Floor([system.math]::Log10($minimum))

    foreach ($radius in $expected) {
      if ($actual.Contains($radius)) {
        $errorRaw = $actual[$radius].AbsError
        $error = $errorRaw * [system.math]::Pow(10, -$exponent)
        $errorString = $error.ToString("0.0000")
        if ($errorRaw -eq $minimum) {
          "\cellcolor{green} " + $errorString
        }
        else {
          $errorString
        }
      }
      else {
        "-"
      }
    }

    "$\times 10^{$exponent}$"
  }

  $radii = Get-RadiiList $images
  $images = Group-Radii $images

  "\begin{table}[ht]"
  "\tiny"
  "\setlength\tabcolsep{2pt}"
  "\begin{center}"
  "\begin{adjustbox}{angle=90}"
  "\begin{tabular}{ |l|l|" + [string]::Join("|", ($radii | ForEach-Object { "c" })) + "|l| }"
  "  \hline"

  "  Alg. & Scene & " + [string]::Join(" & ", $radii) + " & Exp. \\ [0.5ex] \hline\hline"
  foreach ($group in $($images | Group-Object -Property Method)) {
    foreach ($image in $group.Group) {
      $radiiList = " & " + [string]::Join(" & ", (Format-Radii $radii $image.Radii)) + " \\ \hline"

      "  $" + $image.Method + "$ & $" + $image.Scene + "^" + $image.Camera + "$" + $radiiList
    }
  }

  "\end{tabular}"
  "\end{adjustbox}"
  "\end{center}"
  "\end{table}"
}


function Format-BestRadiiTable($images) {
  $images = Group-Radii $images
  $radii = @{}

  foreach ($image in $images) {
    $radii[[System.Tuple]::Create($image.Scene, $image.Method)] = $image.MinRadius
  }

  $scenes = $images | Foreach-Object { $_.Scene } | Select-Object -Unique
  $methods = $images | Foreach-Object { $_.Method } | Select-Object -Unique

  "\begin{table}[ht]"
  "\setlength\tabcolsep{2pt}"
  "\begin{center}"
  "\begin{tabular}{ |l|" + [string]::Join("|", ($methods | ForEach-Object { "l" })) + "| } \hline"

  "  Scene & " + [string]::Join(" & ", ($methods | ForEach-Object { "$" + $_ + "$" })) + " \\ [0.5ex] \hline\hline"

  foreach ($scene in $scenes) {
    $row = $methods | ForEach-Object { $key = [System.Tuple]::Create($scene, $_); $(if ($radii.Contains($key)) { $radii[$key] } else { "-" }) }
    "  $" + $scene + "$ & " + [string]::Join(" & ", $row) + " \\ \hline"
  }

  "\end{tabular}"
  "\end{center}"
  "\end{table}"
}

$images = Get-ImagesWithErrors `
  | Where-Object { (-not ($_.Scene -like "BreakfastRoom*")) -or ($_.Camera -ne 2) }

Format-BestRadiiTable $images
