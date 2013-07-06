# Ocean builder

Push-Location
Set-Location $PSScriptRoot

$params = New-Object System.Collections.Generic.List[System.String]

$list = Get-ChildItem "Core", "Ocean" -Filter "*.txt" -Recurse

foreach ($file in $list)
{
    $params.Add($file.FullName)
}

& ..\Aqua Vapor ..\Vapor_stage1.aqua - $params ..\Ocean.txt
& ..\Assembler ..\Ocean.txt ..\Ocean.aqua

Pop-Location
