
$testCases = Get-ChildItem "models" `
    | Where-Object { $_.Name -match "TestCase1.+\.blend$" } `
    | ForEach-Object {  @{ Input = (Resolve-Path $_.FullName -Relative); BaseOutput = ("test_results\" + $_.BaseName) } }

$masterPath = "master.exe"

mkdir test_results -Force

foreach ($testCase in $testCases) {
    $baseArguments = @("--parallel", "--batch", "--beta=2", "--num-minutes=15")
    . $masterPath $baseArguments --BPT              $testCase.Input ("--output=" + $testCase.BaseOutput + ".BPT2.exr")
    . $masterPath $baseArguments --UPG --radius=0.1 $testCase.Input ("--output=" + $testCase.BaseOutput + ".UPG2.exr")
}
