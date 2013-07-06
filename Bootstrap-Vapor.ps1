# Vapor stage 2 and stage 3 builder and comparer

Push-Location
Set-Location $PSScriptRoot

$params = New-Object System.Collections.Generic.List[System.String]

$list = Get-ChildItem "Core", "Vapor" -Filter "*.txt" -Recurse

foreach ($file in $list)
{
	$params.Add($file.FullName)
}

Write-Host "Building stage2..."
& ..\Aqua Vapor ..\Vapor_stage1.aqua - $params ..\Vapor_stage2.txt
& ..\Assembler ..\Vapor_stage2.txt ..\Vapor_stage2.aqua

Write-Host "Building stage3..."
& ..\Aqua Vapor ..\Vapor_stage2.aqua - $params ..\Vapor_stage3.txt
& ..\Assembler ..\Vapor_stage3.txt ..\Vapor_stage3.aqua

Write-Host "Comparing stage2 and stage3..."
if ((Compare-Object $(Get-Content ..\Vapor_stage2.txt) $(Get-Content ..\Vapor_stage3.txt)) -eq $null)
{
	Write-Host "Pass!" -ForegroundColor Green
}
else
{
	Write-Host "Fail!" -ForegroundColor Red
}

Pop-Location
