param(
  [Parameter(Mandatory=$true)][string]$ResponseFile,
  [Parameter(Mandatory=$true)][string]$OutputDirectory,
  [Parameter(Mandatory=$true)][string]$Compressor,
  [int]$Workers = 16,
  [int]$SegmentKiB = 512,
  [ValidateSet('selkie','mermaid','kraken')][string]$Codec = 'kraken',
  [int]$SpaceSpeedTradeoff = 256,
  [ValidateSet('superfast','veryfast','fast','normal','optimal1','optimal2','optimal3','optimal4','optimal5')][string]$CompressionLevel = 'normal'
)

$ErrorActionPreference = 'Stop'
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class CObjFileSize {
  [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
  public static extern uint GetCompressedFileSizeW(string path, out uint high);
}
'@
$ResponseFile = [IO.Path]::GetFullPath($ResponseFile)
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
$Compressor = [IO.Path]::GetFullPath($Compressor)
[IO.Directory]::CreateDirectory($OutputDirectory) | Out-Null

$lines = [IO.File]::ReadAllLines($ResponseFile)
$work = New-Object System.Collections.Generic.List[object]
$rewritten = New-Object string[] $lines.Length
$objIndex = 0
for ($lineIndex = 0; $lineIndex -lt $lines.Length; ++$lineIndex) {
  $line = $lines[$lineIndex]
  $path = $line.Trim().Trim('"')
  if ($path -match '(?i)\.obj$' -and [IO.File]::Exists($path)) {
    $leaf = [IO.Path]::GetFileName($path)
    $dest = Join-Path $OutputDirectory ('{0:D5}_{1}' -f $objIndex, $leaf)
    $work.Add([pscustomobject]@{ Source=$path; Destination=$dest; Index=$objIndex })
    $rewritten[$lineIndex] = '"' + ($dest -replace '\\','/') + '"'
    ++$objIndex
  } else {
    $rewritten[$lineIndex] = $line
  }
}
[IO.File]::WriteAllLines((Join-Path $OutputDirectory 'compressed.rsp'), $rewritten)

$running = New-Object System.Collections.Generic.List[object]
$completed = 0
$failed = 0
$timer = [Diagnostics.Stopwatch]::StartNew()

function Reap-Compressors([bool]$waitForOne) {
  do {
    $reaped = $false
    for ($i = $running.Count - 1; $i -ge 0; --$i) {
      $job = $running[$i]
      if ($job.Process.HasExited) {
        $stdout = $job.Process.StandardOutput.ReadToEnd().Trim()
        $stderr = $job.Process.StandardError.ReadToEnd().Trim()
        if ($job.Process.ExitCode -ne 0) {
          ++$script:failed
          Write-Error "compression failed ($($job.Process.ExitCode)): $($job.Source)`n$stderr"
        }
        $job.Process.Dispose()
        $running.RemoveAt($i)
        ++$script:completed
        $reaped = $true
        if (($script:completed % 100) -eq 0 -or $script:completed -eq $work.Count) {
          Write-Host ("compressed {0}/{1} objects in {2:n1}s" -f $script:completed,$work.Count,$timer.Elapsed.TotalSeconds)
        }
      }
    }
    if ($waitForOne -and !$reaped) { Start-Sleep -Milliseconds 25 }
  } while ($waitForOne -and !$reaped)
}

foreach ($item in $work) {
  if ([IO.File]::Exists($item.Destination)) {
    ++$completed
    continue
  }
  while ($running.Count -ge $Workers) { Reap-Compressors $true }
  $psi = New-Object Diagnostics.ProcessStartInfo
  $psi.FileName = $Compressor
  $psi.Arguments = ('"{0}" "{1}" {2} {3} {4} {5}' -f $item.Source,$item.Destination,$SegmentKiB,$Codec,$SpaceSpeedTradeoff,$CompressionLevel)
  $psi.UseShellExecute = $false
  $psi.CreateNoWindow = $true
  $psi.RedirectStandardOutput = $true
  $psi.RedirectStandardError = $true
  $process = [Diagnostics.Process]::Start($psi)
  $running.Add([pscustomobject]@{ Process=$process; Source=$item.Source })
}
while ($running.Count) { Reap-Compressors $true }

if ($failed) { throw "$failed compression jobs failed" }
$rawBytes = ($work | ForEach-Object { [IO.FileInfo]$_.Source } | Measure-Object Length -Sum).Sum
$logicalBytes = ($work | ForEach-Object { [IO.FileInfo]$_.Destination } | Measure-Object Length -Sum).Sum
$allocatedBytes = 0L
foreach ($item in $work) {
  [uint32]$high = 0
  [uint32]$low = [CObjFileSize]::GetCompressedFileSizeW($item.Destination, [ref]$high)
  if ($low -eq [uint32]::MaxValue -and [Runtime.InteropServices.Marshal]::GetLastWin32Error() -ne 0) {
    throw "unable to query allocated size: $($item.Destination)"
  }
  $allocatedBytes += ([int64]$high -shl 32) -bor $low
}
Write-Host ("done: {0} independent objects, raw {1:n2} GiB, container logical {2:n2} GiB, allocated {3:n2} GiB ({4:n1}% of raw), {5:n1}s" -f
            $work.Count,($rawBytes/1GB),($logicalBytes/1GB),($allocatedBytes/1GB),
            (100.0*$allocatedBytes/$rawBytes),$timer.Elapsed.TotalSeconds)
