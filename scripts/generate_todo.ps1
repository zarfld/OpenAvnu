# OpenAvnu TODO Generator (PowerShell)
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
    $content = Get-Content -Path $complianceFile -Raw -Encoding UTF8
    
    # Extract critical errors
    if ($content -match '## Critical Errors.*?(?=##|$)') {
        $criticalSection = $matches[0]
        $errors = [regex]::Matches($criticalSection, '- ERROR: (.*)')
        foreach ($complianceError in $errors) {
            $todos += @{
                Priority = "P0"
                Type = "Critical"
                Description = "Fix: $($complianceError.Groups[1].Value)"
                DueDate = (Get-Date).AddDays(2).ToString("yyyy-MM-dd")
                Owner = "TBD"
                Status = "Not Started"
            }
        }
    }
    
    # Extract warnings
    if ($content -match '## Warnings.*?(?=##|$)') {
        $warningSection = $matches[0]
        $warnings = [regex]::Matches($warningSection, '- WARNING: (.*)')
        foreach ($warning in $warnings) {
            $todos += @{
                Priority = "P1"
                Type = "Warning"
                Description = "Fix: $($warning.Groups[1].Value)"
                DueDate = (Get-Date).AddDays(7).ToString("yyyy-MM-dd")
                Owner = "TBD"
                Status = "Not Started"
            }
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
$priorities = @{
    "P0" = @()
    "P1" = @()
    "P2" = @()
    "P3" = @()
}

foreach ($todo in $todos) {
    $priority = $todo.Priority
    if ($priorities.ContainsKey($priority)) {
        $priorities[$priority] += $todo
    }
}

$priorityNames = @{
    "P0" = "🚨 Critical Priority"
    "P1" = "⚠️ High Priority"
    "P2" = "📋 Medium Priority"
    "P3" = "💡 Low Priority"
}

$statusEmojis = @{
    "Not Started" = "🔴"
    "In Progress" = "🟡"
    "Completed" = "🟢"
    "Blocked" = "🔵"
}

foreach ($priority in @("P0", "P1", "P2", "P3")) {
    if ($priorities[$priority].Count -gt 0) {
        $markdown += "## $($priorityNames[$priority])"
        $markdown += ""
        
        foreach ($todo in $priorities[$priority]) {
            $statusEmoji = $statusEmojis[$todo.Status]
            if (-not $statusEmoji) { $statusEmoji = "🔴" }
            
            $markdown += "- [ ] **$($todo.Description)**"
            $markdown += "  - Status: $statusEmoji $($todo.Status)"
            $markdown += "  - Owner: $($todo.Owner)"
            $markdown += "  - Due: $($todo.DueDate)"
            $markdown += "  - Type: $($todo.Type)"
            $markdown += ""
        }
    }
}

$markdown += "## 📊 Summary"
$markdown += "- Total Tasks: $($todos.Count)"
$markdown += "- Critical (P0): $($priorities['P0'].Count)"
$markdown += "- High (P1): $($priorities['P1'].Count)"
$markdown += "- Medium (P2): $($priorities['P2'].Count)"
$markdown += "- Low (P3): $($priorities['P3'].Count)"
$markdown += ""
$markdown += "---"
$markdown += "*This file is auto-generated. Do not edit manually.*"
$markdown += "*Run ``powershell scripts\generate_todo.ps1`` to update.*"

# Write to file
$outputPath = Join-Path $repoPath $OutputFile
$markdown | Out-File -FilePath $outputPath -Encoding UTF8

Write-Host "Generated TODO list with $($todos.Count) items" -ForegroundColor Green
Write-Host "Written to: $outputPath" -ForegroundColor Green
