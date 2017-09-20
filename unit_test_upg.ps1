
param (
    [switch]$justPrint = $false,
    [switch]$printAverage = $false
)

$masterPath = "./master.exe"

function Invoke-Master() {
    if ($justPrint) {
        Write-Host master @args
    }
    else {
        if (Test-Path $masterPath) {
            . $masterPath @args
        }
        else {
            . master @args
        }
    }
}

$baseArguments = @("--UPG", "--parallel", "--batch", "--beta=2", "--num-minutes=120", "--snapshot=360")

Invoke-Master models\TestCaseFurnace.blend --radius=0.01 $baseArguments --output=test_results\TestCaseFurnace.UPG2.exr
Invoke-Master models\TestCase0.blend --radius=0.01 $baseArguments --output=test_results\TestCase0.UPG2.exr
Invoke-Master models\TestCase1.blend --radius=0.05 $baseArguments --output=test_results\TestCase1.UPG2.exr
Invoke-Master models\TestCase2.blend --radius=0.01 $baseArguments --output=test_results\TestCase2.UPG2.exr
Invoke-Master models\TestCase3.blend --radius=0.01 $baseArguments --output=test_results\TestCase3.UPG2.exr
Invoke-Master models\TestCase4.blend --radius=0.05 $baseArguments --output=test_results\TestCase4.UPG2.exr
Invoke-Master models\TestCase5.blend --radius=0.05 $baseArguments --output=test_results\TestCase5.UPG2.exr
Invoke-Master models\TestCase6.blend --radius=0.05 $baseArguments --output=test_results\TestCase6.UPG2.exr
Invoke-Master models\TestCase7.blend --radius=0.05 $baseArguments --output=test_results\TestCase7.UPG2.exr
Invoke-Master models\TestCase8.blend --radius=0.01 $baseArguments --output=test_results\TestCase8.UPG2.exr
Invoke-Master models\TestCase9.blend --radius=0.01 $baseArguments --output=test_results\TestCase9.UPG2.exr
Invoke-Master models\TestCase10.blend --radius=0.01 $baseArguments --output=test_results\TestCase10.UPG2.exr
Invoke-Master models\TestCase11.blend --radius=0.01 $baseArguments --output=test_results\TestCase11.UPG2.exr
Invoke-Master models\TestCase12.blend --radius=0.01 $baseArguments --output=test_results\TestCase12.UPG2.exr
Invoke-Master models\TestCase13.blend --radius=0.03 $baseArguments --output=test_results\TestCase13.UPG2.exr
Invoke-Master models\TestCase14.blend --radius=0.05 $baseArguments --output=test_results\TestCase14.UPG2.exr
Invoke-Master models\TestCase15.blend --radius=0.01 $baseArguments --output=test_results\TestCase15.UPG2.exr
Invoke-Master models\TestCase16.blend --radius=0.03 $baseArguments --output=test_results\TestCase16.UPG2.exr
Invoke-Master models\TestCase17.blend --radius=0.05 $baseArguments --output=test_results\TestCase17.UPG2.exr
Invoke-Master models\TestCase18.blend --radius=0.05 $baseArguments --output=test_results\TestCase18.UPG2.exr
Invoke-Master models\TestCase19.blend --radius=0.1 $baseArguments --output=test_results\TestCase19.UPG2.exr
Invoke-Master models\TestCase20.blend --radius=0.01 $baseArguments --output=test_results\TestCase20.UPG2.exr
Invoke-Master models\TestCase21.blend --radius=0.05 $baseArguments --output=test_results\TestCase21.UPG2.exr
Invoke-Master models\TestCase22.blend --radius=0.05 $baseArguments --output=test_results\TestCase22.UPG2.exr
Invoke-Master models\TestCase23.blend --radius=0.05 $baseArguments --output=test_results\TestCase23.UPG2.exr
Invoke-Master models\TestCase24.blend --radius=0.01 $baseArguments --output=test_results\TestCase24.UPG2.exr
