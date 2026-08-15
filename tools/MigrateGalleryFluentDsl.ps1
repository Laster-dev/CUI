param(
    [string]$Path = "CUI.Gallery\src",
    [switch]$WhatIf
)
$script = Join-Path $PSScriptRoot "MigrateGalleryFluentDsl.js"
if ($WhatIf) {
    node $script --what-if $Path
} else {
    node $script $Path
}
