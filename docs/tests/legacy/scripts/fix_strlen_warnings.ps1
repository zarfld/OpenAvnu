# Fix all strlen warnings in test files
$files = @(
    "daemons/mrpd/tests/simple/msrp_pruning_tests.cpp",
    "daemons/mrpd/tests/simple/msrp_tests.cpp"
)

foreach ($file in $files) {
    if (Test-Path $file) {
        $content = Get-Content $file -Raw
        # Fix strlen() calls that aren't already cast to (int)
        $content = $content -replace 'strlen\(([^)]+)\) \+ 1, &', '(int)strlen($1) + 1, &'
        # Fix cases where (int)strlen is already there but repeated
        $content = $content -replace '\(int\)\(int\)strlen', '(int)strlen'
        Set-Content $file -Value $content
        Write-Host "Fixed $file"
    }
}
