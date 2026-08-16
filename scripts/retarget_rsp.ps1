param(
  [Parameter(Mandatory=$true)][string]$ResponseFile,
  [Parameter(Mandatory=$true)][string]$Destination,
  [Parameter(Mandatory=$true)][string]$Output,
  [Parameter(Mandatory=$true)][string]$Pdb
)

$ErrorActionPreference = 'Stop'
$ResponseFile = [IO.Path]::GetFullPath($ResponseFile)
$Destination = [IO.Path]::GetFullPath($Destination)
$Output = [IO.Path]::GetFullPath($Output)
$Pdb = [IO.Path]::GetFullPath($Pdb)

foreach ($directory in @([IO.Path]::GetDirectoryName($Destination),
                          [IO.Path]::GetDirectoryName($Output),
                          [IO.Path]::GetDirectoryName($Pdb))) {
  [IO.Directory]::CreateDirectory($directory) | Out-Null
}

$outputArg = '/OUT:"' + ($Output -replace '\\','/') + '"'
$pdbArg = '/PDB:"' + ($Pdb -replace '\\','/') + '"'
$foundOutput = $false
$foundPdb = $false
$rewritten = foreach ($line in [IO.File]::ReadAllLines($ResponseFile)) {
  if ($line -match '(?i)^\s*/OUT:') {
    $foundOutput = $true
    $outputArg
  } elseif ($line -match '(?i)^\s*/PDB:') {
    $foundPdb = $true
    $pdbArg
  } else {
    $line
  }
}

if (!$foundOutput) { throw "Response file has no /OUT argument: $ResponseFile" }
if (!$foundPdb) { throw "Response file has no /PDB argument: $ResponseFile" }
[IO.File]::WriteAllLines($Destination, $rewritten)

[pscustomobject]@{
  Source = $ResponseFile
  Destination = $Destination
  Output = $Output
  Pdb = $Pdb
} | ConvertTo-Json -Compress
