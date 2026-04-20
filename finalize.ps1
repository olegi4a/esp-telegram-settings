$base64 = (Get-Content -Path 'base64_image_backup.txt' -Raw).Trim()
$html = [System.IO.File]::ReadAllText('instruction.html')
$newHtml = $html.Replace('src="struc/схема edwic.png"', 'src="data:image/png;base64,' + $base64 + '"')
[System.IO.File]::WriteAllText('instruction.html', $newHtml)
