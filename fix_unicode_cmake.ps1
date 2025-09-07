# Fix Unicode characters in CMakeLists.txt files for CI compatibility

Write-Host "Fixing Unicode characters in CMakeLists.txt files for CI compatibility..."

# Function to replace Unicode characters with CI-friendly alternatives
function Fix-UnicodeInFile {
    param([string]$filePath)
    
    if (Test-Path $filePath) {
        $content = Get-Content $filePath -Raw -Encoding UTF8
        $originalContent = $content
        
        # Replace emoji characters with text equivalents
        $content = $content -replace '🔍', '[DETECT]'
        $content = $content -replace '✅', '[OK]'
        $content = $content -replace '❌', '[ERROR]'
        $content = $content -replace '⚠️', '[WARNING]'
        $content = $content -replace '🔧', '[CONFIG]'
        $content = $content -replace '📁', '[DIR]'
        $content = $content -replace '📚', '[LIB]'
        $content = $content -replace '📦', '[PKG]'
        $content = $content -replace '🎯', '[TARGET]'
        $content = $content -replace '⏱️', '[TIME]'
        $content = $content -replace '⏭️', '[SKIP]'
        
        if ($content -ne $originalContent) {
            Write-Host "Fixed Unicode in: $filePath"
            $content | Out-File $filePath -Encoding UTF8 -NoNewline
            return $true
        }
    }
    return $false
}

# Fix all CMakeLists.txt files
$fixedCount = 0
Get-ChildItem -Recurse -Filter "CMakeLists.txt" | ForEach-Object {
    if (Fix-UnicodeInFile $_.FullName) {
        $fixedCount++
    }
}

Write-Host "Fixed Unicode characters in $fixedCount CMakeLists.txt files"
