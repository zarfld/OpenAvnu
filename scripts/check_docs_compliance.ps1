# OpenAvnu Documentation Compliance Checker (PowerShell)
# This script validates documentation against the governance rules

param(
    [string]$RepoPath = ".",
    [string]$OutputFile = "",
    [switch]$FailOnWarnings
)

# Initialize
$ErrorActionPreference = "Stop"
$complianceErrors = @()
$complianceWarnings = @()
$repoPath = Resolve-Path $RepoPath
$docsPath = Join-Path $repoPath "docs"

# Prohibited terms without hardware validation
$prohibitedTerms = @(
    "PRODUCTION READY",
    "CERTIFIED", 
    "MEETS ALL REQUIREMENTS",
    "FULLY VALIDATED",
    "COMPLETE IMPLEMENTATION",
    "READY FOR DEPLOYMENT"
)

# Required hardware validation evidence
$hardwareEvidence = @(
    "Intel i210",
    "Intel i219",
    "hardware timestamping", 
    "clock synchronization"
)

function Test-FileNaming {
    Write-Host "Checking file naming conventions..."
    
    $namingPatterns = @{
        "completed" = ".*_IMPLEMENTATION\.md$"
        "status" = ".*_STATUS\.md$"
        "work-in-progress" = ".*_DEVELOPMENT\.md$"
        "archive" = ".*_ARCHIVED_\d{4}-\d{2}-\d{2}\.md$"
    }
    
    foreach ($folder in $namingPatterns.Keys) {
        $folderPath = Join-Path $docsPath $folder
        if (Test-Path $folderPath) {
            $files = Get-ChildItem -Path $folderPath -Filter "*.md" -Recurse
            foreach ($file in $files) {
                if (-not ($file.Name -match $namingPatterns[$folder])) {
                    $script:complianceWarnings += "Naming convention violation: $($file.FullName.Replace($repoPath, '').TrimStart('\'))"
                }
            }
        }
    }
}

function Test-ProhibitedTerms {
    Write-Host "Checking for prohibited terms..."
    
    $mdFiles = Get-ChildItem -Path $docsPath -Filter "*.md" -Recurse -ErrorAction SilentlyContinue
    foreach ($file in $mdFiles) {
        try {
            $content = Get-Content -Path $file.FullName -Raw -Encoding UTF8 -ErrorAction SilentlyContinue
            
            foreach ($term in $prohibitedTerms) {
                if ($content -match $term) {
                    # Check if hardware validation evidence exists
                    $hasEvidence = $false
                    foreach ($evidence in $hardwareEvidence) {
                        if ($content -match $evidence) {
                            $hasEvidence = $true
                            break
                        }
                    }
                    
                    if (-not $hasEvidence) {
                        $relativePath = $file.FullName.Replace($repoPath, "").TrimStart('\')
                        $script:complianceErrors += "Prohibited term '$term' found without hardware validation evidence in $relativePath"
                    }
                }
            }
        }
        catch {
            $relativePath = $file.FullName.Replace($repoPath, "").TrimStart('\')
            $script:complianceWarnings += "Could not read $relativePath"
        }
    }
}

function New-ComplianceReport {
    $report = @()
    $report += "# OpenAvnu Documentation Compliance Report"
    $report += "Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
    $report += ""
    
    if ($complianceErrors.Count -gt 0) {
        $report += "## Critical Errors (Must Fix)"
        foreach ($complianceError in $complianceErrors) {
            $report += "- ERROR: $complianceError"
        }
        $report += ""
    }
    
    if ($complianceWarnings.Count -gt 0) {
        $report += "## Warnings (Should Fix)"
        foreach ($complianceWarning in $complianceWarnings) {
            $report += "- WARNING: $complianceWarning"
        }
        $report += ""
    }
    
    if ($complianceErrors.Count -eq 0 -and $complianceWarnings.Count -eq 0) {
        $report += "## All Checks Passed"
        $report += "Documentation is compliant with governance rules."
    }
    
    $report += ""
    $report += "## Summary"
    $report += "- Critical Errors: $($complianceErrors.Count)"
    $report += "- Warnings: $($complianceWarnings.Count)"
    $report += "- Total Issues: $($complianceErrors.Count + $complianceWarnings.Count)"
    
    return $report -join "`n"
}

# Main execution
Write-Host "OpenAvnu Documentation Compliance Checker" -ForegroundColor Green
Write-Host "Repository: $repoPath" -ForegroundColor Yellow
Write-Host ""

# Run all checks
Test-FileNaming
Test-ProhibitedTerms

# Generate report
$report = New-ComplianceReport

if ($OutputFile) {
    $report | Out-File -FilePath $OutputFile -Encoding UTF8
    Write-Host "Compliance report written to $OutputFile" -ForegroundColor Green
} else {
    Write-Host $report
}

# Exit with appropriate code
if ($complianceErrors.Count -gt 0) {
    Write-Host "Critical errors found! Please fix before proceeding." -ForegroundColor Red
    exit 1
} elseif ($FailOnWarnings -and $complianceWarnings.Count -gt 0) {
    Write-Host "Warnings found and fail-on-warnings enabled." -ForegroundColor Yellow
    exit 2
} else {
    Write-Host "All checks completed successfully!" -ForegroundColor Green
    exit 0
}
