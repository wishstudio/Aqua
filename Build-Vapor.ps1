# Vapor stage 1 builder

Push-Location
Set-Location $PSScriptRoot

$params = New-Object System.Collections.Generic.List[System.String]

$list = Get-ChildItem "VaporBootstrap" -Filter "*.txt" -Recurse

foreach ($file in $list)
{
    $params.Add($file.FullName)
}

& ..\Aqua Vapor Vapor.aqua - $params ..\Vapor_stage1.txt
& ..\Assembler ..\Vapor_stage1.txt ..\Vapor_stage1.aqua

Pop-Location
