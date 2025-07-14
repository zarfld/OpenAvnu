Write-Host "=== Multi-Intel Adapter Detection ===" -ForegroundColor Cyan
Write-Host ""

# Intel Adapter Priority: I226 > I225 > I210 > I219 > I217
$adapterPriorities = @{
    "I226" = 1
    "I225" = 2  
    "I210" = 3
    "I219" = 4
    "I217" = 5
}

Write-Host "Scanning for Intel Ethernet adapters..." -ForegroundColor Yellow

# Get all Intel Ethernet adapters
$intelAdapters = Get-NetAdapter | Where-Object { 
    $_.InterfaceDescription -like "*Intel*" -and 
    $_.InterfaceDescription -like "*Ethernet*" -and
    $_.Status -eq "Up"
}

if ($intelAdapters.Count -eq 0) {
    Write-Host "No Intel Ethernet adapters found!" -ForegroundColor Red
    exit 1
}

Write-Host "Found $($intelAdapters.Count) Intel Ethernet adapters:" -ForegroundColor Green
foreach ($adapter in $intelAdapters) {
    Write-Host "  - $($adapter.Name): $($adapter.InterfaceDescription)" -ForegroundColor White
}
Write-Host ""

# Find supported adapters with priority
$supportedAdapters = @()

foreach ($adapter in $intelAdapters) {
    $description = $adapter.InterfaceDescription
    $priority = 99
    $typeName = "Unknown Intel"
    
    if ($description -like "*I226*") {
        $priority = 1
        $typeName = "Intel I226"
    }
    elseif ($description -like "*I225*") {
        $priority = 2
        $typeName = "Intel I225"
    }
    elseif ($description -like "*I210*") {
        $priority = 3
        $typeName = "Intel I210"
    }
    elseif ($description -like "*I219*") {
        $priority = 4
        $typeName = "Intel I219"
    }
    elseif ($description -like "*I217*") {
        $priority = 5
        $typeName = "Intel I217"
    }
    
    if ($priority -lt 99) {
        $supportedAdapters += @{
            Adapter = $adapter
            Priority = $priority
            TypeName = $typeName
        }
        Write-Host "Found supported: $typeName (Priority: $priority)" -ForegroundColor Green
    }
}

if ($supportedAdapters.Count -eq 0) {
    Write-Host "No supported Intel adapters found!" -ForegroundColor Yellow
    Write-Host "Supported: I226, I225, I210, I219, I217" -ForegroundColor Yellow
    exit 1
}

# Select best adapter by priority
$bestAdapter = $supportedAdapters | Sort-Object { $_.Priority } | Select-Object -First 1

Write-Host ""
Write-Host "=== BEST ADAPTER SELECTED ===" -ForegroundColor Green
Write-Host "Type: $($bestAdapter.TypeName)" -ForegroundColor Green
Write-Host "Interface: $($bestAdapter.Adapter.Name)" -ForegroundColor White
Write-Host "Description: $($bestAdapter.Adapter.InterfaceDescription)" -ForegroundColor White
Write-Host "Priority: $($bestAdapter.Priority) (1=highest)" -ForegroundColor White
Write-Host "Link Speed: $($bestAdapter.Adapter.LinkSpeed)" -ForegroundColor White

# Show capabilities
Write-Host ""
Write-Host "Adapter Capabilities:" -ForegroundColor Cyan
$type = $bestAdapter.TypeName
if ($type -like "*I226*") {
    Write-Host "  AVB Support: Excellent" -ForegroundColor Green
    Write-Host "  TSN Support: Full" -ForegroundColor Green
    Write-Host "  Intel HAL: Yes" -ForegroundColor Green
    Write-Host "  Recommendation: Best choice for professional audio" -ForegroundColor Green
}
elseif ($type -like "*I225*") {
    Write-Host "  AVB Support: Excellent" -ForegroundColor Green
    Write-Host "  TSN Support: Full" -ForegroundColor Green
    Write-Host "  Intel HAL: Yes" -ForegroundColor Green
    Write-Host "  Recommendation: Excellent for professional audio" -ForegroundColor Green
}
elseif ($type -like "*I210*") {
    Write-Host "  AVB Support: Very Good" -ForegroundColor Green
    Write-Host "  TSN Support: Good" -ForegroundColor Green
    Write-Host "  Intel HAL: Yes" -ForegroundColor Green
    Write-Host "  Recommendation: Proven technology for AVB/TSN" -ForegroundColor Green
}
elseif ($type -like "*I219*") {
    Write-Host "  AVB Support: Good" -ForegroundColor Green
    Write-Host "  TSN Support: Limited" -ForegroundColor Yellow
    Write-Host "  Intel HAL: Yes" -ForegroundColor Green
    Write-Host "  Recommendation: Good for basic AVB audio" -ForegroundColor Yellow
}
elseif ($type -like "*I217*") {
    Write-Host "  AVB Support: Basic" -ForegroundColor Yellow
    Write-Host "  TSN Support: No" -ForegroundColor Red
    Write-Host "  Intel HAL: Yes" -ForegroundColor Green
    Write-Host "  Recommendation: Legacy AVB support only" -ForegroundColor Yellow
}

# Store results for daemon startup
$Global:SelectedAdapter = $bestAdapter.Adapter
$Global:SelectedAdapterType = $bestAdapter.TypeName

Write-Host ""
Write-Host "=== Multi-Adapter Detection Complete ===" -ForegroundColor Cyan
Write-Host "Ready to start OpenAvnu daemons with $($bestAdapter.TypeName)" -ForegroundColor Green
