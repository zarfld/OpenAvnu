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
    
    $mdFiles = Get-ChildItem -Path $docsPath -Filter "*.md" -Recurse
    foreach ($file in $mdFiles) {
        try {
            $content = Get-Content -Path $file.FullName -Raw -Encoding UTF8
            
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
            $script:complianceWarnings += "Could not read $relativePath`: $_"
        }
    }
}

function Test-RequiredSections {
    Write-Host "Checking required sections..."
    
    $requiredSections = @{
        "completed" = @("Hardware configuration tested", "Test results", "Validation date")
        "status" = @("Current status", "Recent progress", "Next steps")
        "work-in-progress" = @("Current development status", "Remaining tasks", "Target completion")
    }
    
    foreach ($folder in $requiredSections.Keys) {
        $folderPath = Join-Path $docsPath $folder
        if (Test-Path $folderPath) {
            $files = Get-ChildItem -Path $folderPath -Filter "*.md" -Recurse
            foreach ($file in $files) {
                try {
                    $content = Get-Content -Path $file.FullName -Raw -Encoding UTF8
                    
                    foreach ($section in $requiredSections[$folder]) {
                        if ($content -notmatch $section) {
                            $relativePath = $file.FullName.Replace($repoPath, "").TrimStart('\')
                            $script:complianceWarnings += "Missing required section '$section' in $relativePath"
                        }
                    }
                }
                catch {
                    $relativePath = $file.FullName.Replace($repoPath, "").TrimStart('\')
                    $script:complianceWarnings += "Could not read $relativePath`: $_"
                }
            }
        }
    }
}

function Test-StatusFreshness {
    Write-Host "Checking status document freshness..."
    
    $statusPath = Join-Path $docsPath "status"
    if (Test-Path $statusPath) {
        $cutoffDate = (Get-Date).AddDays(-45)
        
        $files = Get-ChildItem -Path $statusPath -Filter "*.md" -Recurse
        foreach ($file in $files) {
            try {
                if ($file.LastWriteTime -lt $cutoffDate) {
                    $relativePath = $file.FullName.Replace($repoPath, "").TrimStart('\')
                    $lastModified = $file.LastWriteTime.ToString("yyyy-MM-dd")
                    $script:complianceWarnings += "Status document may be outdated (last modified $lastModified): $relativePath"
                }
            }
            catch {
                $relativePath = $file.FullName.Replace($repoPath, "").TrimStart('\')
                $script:complianceWarnings += "Could not check modification time for $relativePath`: $_"
            }
        }
    }
}

function Test-CrossReferences {
    Write-Host "Checking cross-references..."
    
    $mdFiles = Get-ChildItem -Path $docsPath -Filter "*.md" -Recurse
    foreach ($file in $mdFiles) {
        try {
            $content = Get-Content -Path $file.FullName -Raw -Encoding UTF8
            
            # Find markdown links
            $links = [regex]::Matches($content, '\[([^\]]+)\]\(([^)]+)\)')
            foreach ($link in $links) {
                $linkUrl = $link.Groups[2].Value
                
                if ($linkUrl -match '^https?://') {
                    continue  # Skip external links
                }
                
                # Check if internal link exists
                if ($linkUrl.StartsWith('/')) {
                    $targetPath = Join-Path $repoPath $linkUrl.TrimStart('/')
                } else {
                    $targetPath = Join-Path $file.Directory.FullName $linkUrl
                }
                
                if (-not (Test-Path $targetPath)) {
                    $relativePath = $file.FullName.Replace($repoPath, "").TrimStart('\')
                    $script:complianceWarnings += "Broken link '$linkUrl' in $relativePath"
                }
            }
        }
        catch {
            $relativePath = $file.FullName.Replace($repoPath, "").TrimStart('\')
            $script:complianceWarnings += "Could not check links in $relativePath`: $_"
        }
    }
}

function New-ComplianceReport {
    $report = @()
    $report += "# OpenAvnu Documentation Compliance Report"
    $report += "Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
    $report += ""
    
    if ($complianceErrors.Count -gt 0) {
        $report += "## 🚨 Critical Errors (Must Fix)"
        foreach ($complianceError in $complianceErrors) {
            $report += "- ❌ $complianceError"
        }
        $report += ""
    }
    
    if ($complianceWarnings.Count -gt 0) {
        $report += "## ⚠️ Warnings (Should Fix)"
        foreach ($complianceWarning in $complianceWarnings) {
            $report += "- ⚠️ $complianceWarning"
        }
        $report += ""
    }
    
    if ($complianceErrors.Count -eq 0 -and $complianceWarnings.Count -eq 0) {
        $report += "## ✅ All Checks Passed"
        $report += "Documentation is compliant with governance rules."
    }
    
    $report += ""
    $report += "## 📊 Summary"
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
Test-RequiredSections
Test-StatusFreshness
Test-CrossReferences

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
    Write-Host "`nCritical errors found! Please fix before proceeding." -ForegroundColor Red
    exit 1
} elseif ($FailOnWarnings -and $complianceWarnings.Count -gt 0) {
    Write-Host "`nWarnings found and fail-on-warnings enabled." -ForegroundColor Yellow
    exit 2
} else {
    Write-Host "`nAll checks completed successfully!" -ForegroundColor Green
    exit 0
}
