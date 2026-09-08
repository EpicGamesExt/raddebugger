param(
  [Parameter(Mandatory=$true)][string]$Rsp,
  [Parameter(Mandatory=$true)][string]$Output,
  [Parameter(Mandatory=$true)][string]$Pdb,
  [int]$Iterations = 5,
  [string]$ExtraArgs = '',
  [string]$WorkingDirectory = '',
  [string]$Radlink = ''
)

$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (!$Radlink) { $Radlink = Join-Path $root 'out_cobj\radlink.exe' }
if (!$WorkingDirectory) { $WorkingDirectory = (Get-Location).Path }
$Radlink = [IO.Path]::GetFullPath($Radlink)
$Rsp = [IO.Path]::GetFullPath($Rsp)
$Output = [IO.Path]::GetFullPath($Output)
$Pdb = [IO.Path]::GetFullPath($Pdb)

$expectedOutputHash = $null
$expectedPdbHash = $null
$results = [Collections.Generic.List[object]]::new()

for ($iteration = 1; $iteration -le $Iterations; ++$iteration) {
  # Never let a stale artifact make a successful-but-nonproducing invocation look valid.
  [IO.File]::Delete($Output)
  [IO.File]::Delete($Pdb)

  $psi = [Diagnostics.ProcessStartInfo]::new()
  $psi.FileName = $Radlink
  $psi.WorkingDirectory = $WorkingDirectory
  $psi.Arguments = '@"' + $Rsp + '" ' + $ExtraArgs
  $psi.UseShellExecute = $false
  $psi.CreateNoWindow = $true
  $psi.RedirectStandardOutput = $true
  $psi.RedirectStandardError = $true

  $process = [Diagnostics.Process]::new()
  $process.StartInfo = $psi
  $timer = [Diagnostics.Stopwatch]::StartNew()
  [void]$process.Start()
  $stdout = $process.StandardOutput.ReadToEndAsync()
  $stderr = $process.StandardError.ReadToEndAsync()
  $process.WaitForExit()
  $timer.Stop()
  $combined = $stdout.GetAwaiter().GetResult() + $stderr.GetAwaiter().GetResult()
  if ($process.ExitCode -ne 0) {
    throw "link iteration $iteration failed with exit code $($process.ExitCode):`n$combined"
  }
  if (!(Test-Path -LiteralPath $Output) -or !(Test-Path -LiteralPath $Pdb)) {
    throw "link iteration $iteration did not produce both requested outputs"
  }

  $outputHash = (Get-FileHash -LiteralPath $Output -Algorithm SHA256).Hash
  $pdbHash = (Get-FileHash -LiteralPath $Pdb -Algorithm SHA256).Hash
  if ($iteration -eq 1) {
    $expectedOutputHash = $outputHash
    $expectedPdbHash = $pdbHash
  } elseif ($outputHash -ne $expectedOutputHash -or $pdbHash -ne $expectedPdbHash) {
    throw "non-deterministic output at iteration $iteration (image $outputHash, PDB $pdbHash)"
  }

  $results.Add([pscustomobject]@{
    Iteration = $iteration
    WallSeconds = $timer.Elapsed.TotalSeconds
    ImageSha256 = $outputHash
    PdbSha256 = $pdbHash
  })
}

$results | ConvertTo-Json
