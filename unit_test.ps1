
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
        | Where-Object { $_.Name -match "TestCase(20|24).blend$" } `
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