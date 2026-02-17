param(
    [Parameter(Mandatory=$true)]
    [string]$InputFile,
    
    [Parameter(Mandatory=$true)]
    [string]$OutputFile,
    
    [Parameter(Mandatory=$true)]
    [string]$ArrayName
)

# Check if input file exists
if (-not (Test-Path $InputFile)) {
    Write-Error "Input file '$InputFile' not found!"
    exit 1
}

Write-Host "Converting $InputFile to $OutputFile..."
Write-Host "Array name: $ArrayName"

# Read the font file as bytes
$fontBytes = [System.IO.File]::ReadAllBytes($InputFile)
$totalBytes = $fontBytes.Length

Write-Host "Font size: $totalBytes bytes"

# Create output file
$output = New-Object System.Text.StringBuilder
[void]$output.AppendLine("const uint8_t ${ArrayName}[] =")
[void]$output.AppendLine("{")

# Write bytes in rows of 16
$bytesPerRow = 16
for ($i = 0; $i -lt $totalBytes; $i += $bytesPerRow) {
    $rowBytes = @()
    $endIndex = [Math]::Min($i + $bytesPerRow, $totalBytes)
    
    for ($j = $i; $j -lt $endIndex; $j++) {
        $rowBytes += "0x{0:x2}" -f $fontBytes[$j]
    }
    
    $line = $rowBytes -join ", "
    [void]$output.Append($line)
    
    # Add comma and newline unless it's the last line
    if ($endIndex -lt $totalBytes) {
        [void]$output.AppendLine(", ")
    } else {
        [void]$output.AppendLine("")
    }
    
    # Progress indicator
    if ($i % 1024 -eq 0) {
        $percent = [Math]::Round(($i / $totalBytes) * 100, 1)
        Write-Progress -Activity "Converting font" -Status "$percent% Complete" -PercentComplete $percent
    }
}

[void]$output.AppendLine("};")

# Write to file
[System.IO.File]::WriteAllText($OutputFile, $output.ToString())

Write-Progress -Activity "Converting font" -Completed
Write-Host "Conversion complete!" -ForegroundColor Green
Write-Host "Output file: $OutputFile"
