# Do an iteration

Push-Location
Set-Location $PSScriptRoot

.\Bootstrap-Vapor

Copy-Item ..\Vapor_stage3.aqua .\Vapor.aqua

Write-Host "Iteration done."

Pop-Location
