
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

function Get-TestCases() {
    Get-ChildItem "models" `
        | Where-Object { $_.Name -match "TestCase2(1|2|3|4).blend$" } `
        | ForEach-Object {  @{ Input = (Resolve-Path $_.FullName -Relative); BaseOutput = ("test_results\" + $_.BaseName) } }
}

if ($printAverage)
{
    $testResults = Get-ChildItem "test_results" `
        | Where-Object { $_.Name -match "TestCase.*\.exr$" } `
        | ForEach-Object {  (Resolve-Path $_.FullName -Relative) }
    
    foreach ($testResult in $testResults) {
        Write-Host $testResult
        Invoke-Master avg $testResult
    }
}
else
{
    $testCases = Get-TestCases

    mkdir test_results -Force | Out-Null

    foreach ($testCase in $testCases) {
        $baseArguments = @("--parallel", "--batch", "--beta=2", "--num-minutes=240", "--snapshot=360")
        Invoke-Master $baseArguments --BPT $testCase.Input ("--output=" + $testCase.BaseOutput + ".BPT2.exr")
    }
}


# TestCase0.blend --radius=0.01
# TestCase1.blend --radius=0.05
# TestCase2.blend --radius=0.01
# TestCase3.blend --radius=0.01
# TestCase4.blend --radius=0.05
# TestCase5.blend --radius=0.05
# TestCase6.blend --radius=0.05
# TestCase7.blend --radius=0.05
# TestCase8.blend --radius=0.01
# TestCase9.blend --radius=0.01
# TestCase10.blend --radius=0.01
# TestCase11.blend --radius=0.01
# TestCase12.blend --radius=0.01
# TestCase13.blend --radius=0.03
# TestCase14.blend --radius=0.05
# TestCase15.blend --radius=0.01
# TestCase16.blend --radius=0.03
# TestCase17.blend --radius=0.05
# TestCase18.blend --radius=0.05
# TestCase19.blend --radius=0.1
# TestCase20.blend --radius=0.01
# TestCase21.blend --radius=0.05
# TestCase22.blend --radius=0.05
# TestCase23.blend --radius=0.05
# TestCase24.blend --radius=0.01


