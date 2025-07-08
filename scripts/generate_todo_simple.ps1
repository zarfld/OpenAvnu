# OpenAvnu TODO Generator (PowerShell) - Simple Version
# Automatically generates TODO items from compliance reports

param(
    [string]$RepoPath = ".",
    [string]$OutputFile = "docs\TODO_AUTO_GENERATED.md"
)

$repoPath = Resolve-Path $RepoPath
$docsPath = Join-Path $repoPath "docs"
$complianceFile = Join-Path $docsPath "compliance_report.md"

# Initialize TODO list
$todos = @()

# Generate TODOs from compliance report
if (Test-Path $complianceFile) {
    Write-Host "Reading compliance report..."
    $content = Get-Content -Path $complianceFile -Raw -Encoding UTF8
    
    # Extract critical errors
    $criticalMatches = [regex]::Matches($content, '- ERROR: (.*)')
    foreach ($match in $criticalMatches) {
        $todos += @{
            Priority = "P0"
            Type = "Critical"
            Description = "Fix: $($match.Groups[1].Value)"
            DueDate = (Get-Date).AddDays(2).ToString("yyyy-MM-dd")
            Owner = "TBD"
            Status = "Not Started"
        }
    }
    
    # Extract warnings
    $warningMatches = [regex]::Matches($content, '- WARNING: (.*)')
    foreach ($match in $warningMatches) {
        $todos += @{
            Priority = "P1"
            Type = "Warning"
            Description = "Fix: $($match.Groups[1].Value)"
            DueDate = (Get-Date).AddDays(7).ToString("yyyy-MM-dd")
            Owner = "TBD"
            Status = "Not Started"
        }
    }
}

# Add hardware validation tasks
$hardwareTasks = @(
    @{
        Priority = "P0"
        Type = "Hardware"
        Description = "Test Intel i210 NIC on Windows 10 - clock synchronization"
        DueDate = (Get-Date).AddDays(30).ToString("yyyy-MM-dd")
        Owner = "TBD"
        Status = "Blocked - Need hardware"
    },
    @{
        Priority = "P0"
        Type = "Hardware"
        Description = "Test Intel i210 NIC on Windows 11 - clock synchronization"
        DueDate = (Get-Date).AddDays(30).ToString("yyyy-MM-dd")
        Owner = "TBD"
        Status = "Blocked - Need hardware"
    },
    @{
        Priority = "P0"
        Type = "Hardware"
        Description = "Test Intel i219 NIC on Windows 10 - clock synchronization"
        DueDate = (Get-Date).AddDays(30).ToString("yyyy-MM-dd")
        Owner = "TBD"
        Status = "Blocked - Need hardware"
    },
    @{
        Priority = "P0"
        Type = "Hardware"
        Description = "Test Intel i219 NIC on Windows 11 - clock synchronization"
        DueDate = (Get-Date).AddDays(30).ToString("yyyy-MM-dd")
        Owner = "TBD"
        Status = "Blocked - Need hardware"
    }
)

$todos += $hardwareTasks

# Generate markdown
$markdown = @()
$markdown += "# OpenAvnu Auto-Generated TODO List"
$markdown += "Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
$markdown += ""

# Group by priority
$p0Tasks = $todos | Where-Object { $_.Priority -eq "P0" }
$p1Tasks = $todos | Where-Object { $_.Priority -eq "P1" }

if ($p0Tasks.Count -gt 0) {
    $markdown += "## CRITICAL PRIORITY (P0)"
    $markdown += ""
    
    foreach ($todo in $p0Tasks) {
        $markdown += "- [ ] **$($todo.Description)**"
        $markdown += "  - Status: $($todo.Status)"
        $markdown += "  - Owner: $($todo.Owner)"
        $markdown += "  - Due: $($todo.DueDate)"
        $markdown += "  - Type: $($todo.Type)"
        $markdown += ""
    }
}

if ($p1Tasks.Count -gt 0) {
    $markdown += "## HIGH PRIORITY (P1)"
    $markdown += ""
    
    foreach ($todo in $p1Tasks) {
        $markdown += "- [ ] **$($todo.Description)**"
        $markdown += "  - Status: $($todo.Status)"
        $markdown += "  - Owner: $($todo.Owner)"
        $markdown += "  - Due: $($todo.DueDate)"
        $markdown += "  - Type: $($todo.Type)"
        $markdown += ""
    }
}

$markdown += "## SUMMARY"
$markdown += "- Total Tasks: $($todos.Count)"
$markdown += "- Critical (P0): $($p0Tasks.Count)"
$markdown += "- High (P1): $($p1Tasks.Count)"
$markdown += ""
$markdown += "---"
$markdown += "*This file is auto-generated. Do not edit manually.*"
$markdown += "*Run 'powershell scripts\generate_todo_simple.ps1' to update.*"

# Write to file
$outputPath = Join-Path $repoPath $OutputFile
$markdown | Out-File -FilePath $outputPath -Encoding UTF8

Write-Host "Generated TODO list with $($todos.Count) items" -ForegroundColor Green
Write-Host "- Critical (P0): $($p0Tasks.Count)" -ForegroundColor Red
Write-Host "- High (P1): $($p1Tasks.Count)" -ForegroundColor Yellow
Write-Host "Written to: $outputPath" -ForegroundColor Green
