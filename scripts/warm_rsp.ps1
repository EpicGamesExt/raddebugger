param([Parameter(Mandatory=$true)][string]$Rsp)

$ErrorActionPreference = 'Stop'
$Rsp = [IO.Path]::GetFullPath($Rsp)
$Buffer = New-Object byte[] (16MB)
$Files = [Collections.Generic.List[string]]::new()

foreach ($line in [IO.File]::ReadLines($Rsp)) {
  $path = $line.Trim().Trim('"')
  if ($path -notmatch '(?i)\.obj$' -or -not [IO.File]::Exists($path)) { continue }
  $Files.Add($path)
}

$total = 0L
$timer = [Diagnostics.Stopwatch]::StartNew()
foreach ($path in $Files) {
  $fs = [IO.FileStream]::new($path, [IO.FileMode]::Open, [IO.FileAccess]::Read,
                             [IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete,
                             16MB, [IO.FileOptions]::SequentialScan)
  try {
    while (($got = $fs.Read($Buffer, 0, $Buffer.Length)) -gt 0) { $total += $got }
  } finally { $fs.Dispose() }
}
$timer.Stop()
[pscustomobject]@{
  Rsp = $Rsp
  Files = $Files.Count
  LogicalGiBRead = $total / 1GB
  Seconds = $timer.Elapsed.TotalSeconds
  GiBPerSecond = if ($timer.Elapsed.TotalSeconds) { ($total / 1GB) / $timer.Elapsed.TotalSeconds } else { 0 }
} | ConvertTo-Json -Compress
