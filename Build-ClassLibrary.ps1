# Aqua class library builder

# Search all files in "CoreTemp\"

Push-Location
Set-Location $PSScriptRoot

$params = New-Object System.Collections.Generic.List[System.String]

$list = Get-ChildItem "CoreTemp\" -Recurse

foreach ($file in $list)
{
	$params.Add($file.FullName)
}

& ..\Assembler $params ..\Core.aqua

Pop-Location
