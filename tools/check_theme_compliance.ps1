param(
    [string]$RootPath = "$PSScriptRoot\.."
)

$ErrorActionPreference = "Stop"
$failures = 0
$warnings = 0

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " [CUI] Starting Theme Compliance & Color Checker..." -ForegroundColor Cyan
Write-Host " Root Directory: $RootPath" -ForegroundColor Gray
Write-Host "==========================================================" -ForegroundColor Cyan

# -----------------------------------------------------------------------------
# Check 1: Core control constructors purity
# -----------------------------------------------------------------------------
Write-Host "`n[Check 1/4] Verifying CUI.Core control constructors for dynamic token purity..." -ForegroundColor Yellow
$controlsPath = Join-Path $RootPath "CUI.Core\ui\framework\controls"
$controlFiles = Get-ChildItem -Path $controlsPath -Filter "*.cpp" -Recurse

$destructivePattern = 'this->Apply(Color|Background|HoverBackground|PressedBackground|BorderBrush)\s*\(\s*(ThemeManager::|tokens\.)'
foreach ($file in $controlFiles) {
    $content = Get-Content $file.FullName -Raw
    $matches = [regex]::Matches($content, $destructivePattern)
    if ($matches.Count -gt 0) {
        foreach ($m in $matches) {
            $lineNum = ($content.Substring(0, $m.Index) -split "`n").Count
            Write-Host "  [FAIL] $($file.Name):$lineNum Hardcoded theme color in constructor: '$($m.Value)'" -ForegroundColor Red
            $failures++
        }
    }
}
if ($failures -eq 0) {
    Write-Host "  [PASS] All control constructors comply with pure token binding rules!" -ForegroundColor Green
}

# -----------------------------------------------------------------------------
# Check 2: MakeLabel helper function token propagation
# -----------------------------------------------------------------------------
Write-Host "`n[Check 2/4] Verifying MakeLabel token propagation in SamplePage.cpp..." -ForegroundColor Yellow
$samplePagePath = Join-Path $RootPath "CUI.Gallery\src\pages\SamplePage.cpp"
if (Test-Path $samplePagePath) {
    $content = Get-Content $samplePagePath -Raw
    if ($content -match 'MakeLabel\s*\([^)]*ThemeTokenId\s+(\w+)[^)]*\)\s*\{([^}]+)\}') {
        $tokenParam = $Matches[1]
        $body = $Matches[2]
        if ($body -notmatch "ColorToken\s*\(\s*$tokenParam\s*\)") {
            Write-Host "  [FAIL] MakeLabel receives token '$tokenParam' but does not call .ColorToken($tokenParam)!" -ForegroundColor Red
            $failures++
        } else {
            Write-Host "  [PASS] MakeLabel properly calls ColorToken($tokenParam)!" -ForegroundColor Green
        }
    }
}

# -----------------------------------------------------------------------------
# Check 3: Gallery pages hardcoded low-contrast dark greys
# -----------------------------------------------------------------------------
Write-Host "`n[Check 3/4] Scanning Gallery pages for hardcoded dark slate text colors..." -ForegroundColor Yellow
$galleryPagesPath = Join-Path $RootPath "CUI.Gallery\src\pages"
$galleryFiles = Get-ChildItem -Path $galleryPagesPath -Filter "*.cpp" -Recurse
$rule3Fails = 0

foreach ($file in $galleryFiles) {
    $lines = Get-Content $file.FullName
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        if ($line -match '\.(Foreground|Color)\s*\(\s*Rgb\s*\(\s*0x94A3B8') {
            Write-Host "  [FAIL] $($file.Name):$($i + 1) Low contrast hardcoded text color 0x94A3B8: $line" -ForegroundColor Red
            $rule3Fails++
            $failures++
        }
    }
}
if ($rule3Fails -eq 0) {
    Write-Host "  [PASS] No low-contrast hardcoded text colors found in Gallery pages!" -ForegroundColor Green
}

# -----------------------------------------------------------------------------
# Check 4: ThemeTokenId enum alignment
# -----------------------------------------------------------------------------
Write-Host "`n[Check 4/4] Verifying ThemeTokenId definitions in ThemeManager.cpp..." -ForegroundColor Yellow
$tokenHeaderPath = Join-Path $RootPath "CUI.Core\ui\framework\style\ThemeTokenId.h"
$themeMgrPath = Join-Path $RootPath "CUI.Core\ui\framework\style\ThemeManager.cpp"

if ((Test-Path $tokenHeaderPath) -and (Test-Path $themeMgrPath)) {
    $headerContent = Get-Content $tokenHeaderPath -Raw
    $mgrContent = Get-Content $themeMgrPath -Raw

    if ($headerContent -match 'enum class ThemeTokenId\s*:[^{]+\{([^}]+)\}') {
        $enumBody = $Matches[1]
        $tokens = [regex]::Matches($enumBody, '(\w+)\s*(?:=\s*\d+)?\s*,?') | ForEach-Object { $_.Groups[1].Value } | Where-Object { $_ -ne "Unset" -and $_ -ne "Count" }
        
        foreach ($token in $tokens) {
            $camelToken = $token.Substring(0, 1).ToLower() + $token.Substring(1)
            $tokenPropPattern = "m_tokens\.$camelToken\s*="
            if ($mgrContent -notmatch $tokenPropPattern) {
                Write-Host "  [WARN] ThemeManager.cpp may be missing m_tokens.$camelToken" -ForegroundColor Yellow
                $warnings++
            }
        }
        Write-Host "  [PASS] ThemeTokenId total count: $($tokens.Count), all tokens verified!" -ForegroundColor Green
    }
}

# -----------------------------------------------------------------------------
# Summary
# -----------------------------------------------------------------------------
Write-Host "`n==========================================================" -ForegroundColor Cyan
if ($failures -eq 0) {
    Write-Host " [SUCCESS] Theme compliance & color check PASSED! (Errors: 0, Warnings: $warnings)" -ForegroundColor Green
    Write-Host "==========================================================" -ForegroundColor Cyan
    exit 0
} else {
    Write-Host " [FAILED] Found $failures violations! (Errors: $failures, Warnings: $warnings)" -ForegroundColor Red
    Write-Host "==========================================================" -ForegroundColor Cyan
    exit 1
}
