param(
    [Parameter(Mandatory = $false)]
    [string]$Path = "CUI.Gallery\src",
    [switch]$WhatIf
)

$propertyMap = @{
    SetText = 'Text'; SetPlaceholder = 'Placeholder'; SetToolTip = 'ToolTip'; SetIcon = 'Icon'
    SetFontFamily = 'FontFamily'; SetFontSize = 'FontSize'; SetFontWeight = 'FontWeight'; SetFontStyle = 'FontStyle'
    SetIsUnderline = 'Underline'; SetIsStrikethrough = 'Strikethrough'
    SetBackground = 'Background'; SetBackgroundColor = 'Background'; SetHoverBackground = 'HoverBackground'; SetPressedBackground = 'PressedBackground'
    SetBorderBrush = 'BorderBrush'; SetColor = 'Foreground'; SetTextColor = 'Foreground'
    SetBackgroundToken = 'BackgroundToken'; SetHoverBackgroundToken = 'HoverBackgroundToken'; SetPressedBackgroundToken = 'PressedBackgroundToken'
    SetDisabledBackgroundToken = 'DisabledBackgroundToken'; SetBorderToken = 'BorderToken'; SetFocusedBorderToken = 'FocusedBorderToken'
    SetColorToken = 'ColorToken'; SetSecondaryColorToken = 'SecondaryColorToken'; SetPlaceholderColorToken = 'PlaceholderColorToken'
    SetSelectedBackgroundToken = 'SelectedBackgroundToken'; SetHeaderBackgroundToken = 'HeaderBackgroundToken'; SetPaneBackgroundToken = 'PaneBackgroundToken'
    SetIndicatorColorToken = 'IndicatorColorToken'; SetDropdownBackgroundToken = 'DropdownBackgroundToken'; SetSelectedItemBackgroundToken = 'SelectedItemBackgroundToken'
    SetFillColorToken = 'FillColorToken'; SetTrackColorToken = 'TrackColorToken'; SetActiveTrackColorToken = 'ActiveTrackColorToken'
    SetThumbColorToken = 'ThumbColorToken'; SetOnColorToken = 'OnColorToken'; SetOffColorToken = 'OffColorToken'; SetKnobColorToken = 'KnobColorToken'
    SetCheckedBackgroundToken = 'CheckedBackgroundToken'; SetAccentColorToken = 'AccentColorToken'; SetActiveColorToken = 'ActiveColorToken'
    SetUnderlineColorToken = 'UnderlineColorToken'; SetActiveUnderlineColorToken = 'ActiveUnderlineColorToken'
    SetActiveTabBackgroundToken = 'ActiveTabBackgroundToken'; SetInactiveTabBackgroundToken = 'InactiveTabBackgroundToken'
    SetGridLineBrushToken = 'GridLineBrushToken'; SetTitleColorToken = 'TitleColorToken'; SetMessageColorToken = 'MessageColorToken'; SetCaretColorToken = 'CaretColorToken'
    SetWidth = 'Width'; SetHeight = 'Height'; SetMinWidth = 'MinWidth'; SetMinHeight = 'MinHeight'; SetMaxWidth = 'MaxWidth'; SetMaxHeight = 'MaxHeight'
    SetMargin = 'Margin'; SetPadding = 'Padding'; SetVisibility = 'VisibilityProperty'; SetOpacity = 'Opacity'; SetCornerRadius = 'CornerRadius'; SetBorderThickness = 'BorderThickness'; SetFlexGrow = 'FlexGrow'
    SetAlign = 'Align'; SetAlignHorizontal = 'AlignHorizontal'; SetAlignVertical = 'AlignVertical'; SetOrientation = 'Orientation'; SetGap = 'Gap'; SetItemWidth = 'ItemWidth'; SetItemHeight = 'ItemHeight'
    SetLastChildFill = 'LastChildFill'; SetJustifyLines = 'JustifyLines'; SetFillLastLine = 'FillLastLine'; SetRows = 'Rows'; SetClipToBounds = 'ClipToBounds'
    SetCanvasLeft = 'CanvasLeft'; SetCanvasTop = 'CanvasTop'; SetCanvasRight = 'CanvasRight'; SetCanvasBottom = 'CanvasBottom'; SetZIndex = 'ZIndex'
    SetGridColumn = 'GridColumn'; SetGridRow = 'GridRow'; SetGridColumnSpan = 'GridColumnSpan'; SetGridRowSpan = 'GridRowSpan'; SetIsEnabled = 'IsEnabledProperty'
}

$files = Get-ChildItem -Path $Path -Recurse -File -Include *.cpp,*.h
$changes = 0
foreach ($file in $files) {
    $text = Get-Content -LiteralPath $file.FullName -Raw
    $updated = $text
    foreach ($entry in $propertyMap.GetEnumerator()) {
        $method = [regex]::Escape($entry.Key)
        $property = $entry.Value
        $updated = [regex]::Replace($updated, "(?m)(?<receiver>\b[A-Za-z_]\w*)->$method\((?<value>[^;\r\n]+)\);", { param($match) "$($match.Groups['receiver'].Value)->$property = $($match.Groups['value'].Value);" })
    }
    if ($updated -ne $text) {
        $changes++
        if (-not $WhatIf) { Set-Content -LiteralPath $file.FullName -Value $updated -NoNewline }
        Write-Output $file.FullName
    }
}
Write-Output "Migrated $changes file(s)."