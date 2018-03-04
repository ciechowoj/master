#!/usr/bin/env powershell



function Get-Images() {
  Get-ChildItem radii `
    | ForEach-Object { `
      if ($_ -match "^(?<Scene>\w*)\.(?<Method>\w*)\d\.(?<Radius>\w*)\.cam(?<Camera>\d*)(?<FromCamera>\.from\.camera\.|\.)exr") {
        $properties = @{
          Path = $_.FullName;
          Scene = $Matches.Scene;
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
      $radii = $_.Group | Select-Object -Property Radius

      $properties = @{
        Path = $_.Group[0].Path;
        Scene = $_.Group[0].Scene;
        Method = $_.Group[0].Method;
        Radii = Convert-ToAssociative $_.Group;
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
  "\setlength\tabcolsep{2pt}"
  "\begin{center}"
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
  "\end{center}"
  "\end{table}"
}

$images = Get-ImagesWithErrors `
  | Where-Object { (-not ($_.Scene -like "BreakfastRoom*")) -or ($_.Camera -ne 2) }



Format-RadiiTable $images



# \begin{tabular}{ |r|l| }
#     \hline
#     \% time self & function name \\ [0.5ex] \hline\hline
#      23.16 & \verb+embree::avx::BVHNIntersector1<...>::occluded(...)+ \\ \hline
#      19.46 & \verb+embree::avx::BVHNIntersector1<...>::intersect(...)+ \\ \hline
#       8.91 & \verb+UPGBase<...>::_gather(...)+ \\ \hline
#       7.00 & \verb+UPGBase<...>::_connect(...)+ \\ \hline
#       4.88 & \verb+Scene::querySurface(...) const+ \\ \hline
#       3.43 & \verb+DiffuseBSDF::query(...) const+ \\ \hline
#       2.95 & \verb+Scene::occluded(...) const+ \\ \hline
#       2.88 & \verb+ska::detailv3::sherwood_v3_table<...>::grow()+ \\ \hline
#       2.02 & \verb+UPGBase<...>::_traceEye(...)+ \\ \hline
#       2.00 & \verb+HashGrid3D<...>::build(...)+ \\ \hline
#       1.94 & \verb+PhongBSDF::query(...) const+ \\ \hline
#       1.15 & \verb+std::mersenne_twister_engine<...>::operator()()+ \\ \hline
#       1.11 & \verb+std::sort<...>(...)+ \\ \hline
#       1.06 & \verb+Scene::intersect(...) const+ \\ \hline
#      18.05 & other \\ [0.5ex] \hline\hline
#      \% time total & function name \\ [0.5ex] \hline\hline
#      % \rowcolor{green}
#      15.32 & \verb+UPGBase<...>::scatter(...)+ \\ \hline
# \end{tabular}
