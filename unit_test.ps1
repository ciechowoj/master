
param (
    [switch]$justPrint = $false
)

$testCases = Get-ChildItem "models" `
    | Where-Object { $_.Name -match "TestCase.*\.blend$" } `
    | ForEach-Object {  @{ Input = (Resolve-Path $_.FullName -Relative); BaseOutput = ("test_results\" + $_.BaseName) } }

$masterPath = "./master.exe"

function Invoke-Master() {
    if ($justPrint) {
        Write-Host master @args
    }
    else {
        if ([System.IO.File]::Exists($masterPath)) {
            . $masterPath @args
        }
        else {
            . master @args
        }
    }
}

mkdir test_results -Force | Out-Null

foreach ($testCase in $testCases) {
    $baseArguments = @("--parallel", "--batch", "--beta=2", "--num-minutes=20")
    Invoke-Master $baseArguments --BPT $testCase.Input ("--output=" + $testCase.BaseOutput + ".BPT2.exr")
    Invoke-Master $baseArguments --UPG $testCase.Input ("--output=" + $testCase.BaseOutput + ".UPG2.exr")
}
