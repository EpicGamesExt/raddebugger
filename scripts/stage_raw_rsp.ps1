param(
  [Parameter(Mandatory=$true)][string]$ResponseFile,
  [Parameter(Mandatory=$true)][string]$OutputDirectory,
  [int]$Workers = 16
)

$ErrorActionPreference = 'Stop'
$ResponseFile = [IO.Path]::GetFullPath($ResponseFile)
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
[IO.Directory]::CreateDirectory($OutputDirectory) | Out-Null

$lines = [IO.File]::ReadAllLines($ResponseFile)
$rewritten = New-Object string[] $lines.Length
$sources = [Collections.Generic.List[string]]::new()
$destinations = [Collections.Generic.List[string]]::new()
for ($lineIndex = 0; $lineIndex -lt $lines.Length; ++$lineIndex) {
  $path = $lines[$lineIndex].Trim().Trim('"')
  if ($path -match '(?i)\.obj$' -and [IO.File]::Exists($path)) {
    $dest = Join-Path $OutputDirectory ('{0:D5}_{1}' -f $sources.Count,[IO.Path]::GetFileName($path))
    $sources.Add($path)
    $destinations.Add($dest)
    $rewritten[$lineIndex] = '"' + ($dest -replace '\\','/') + '"'
  } else {
    $rewritten[$lineIndex] = $lines[$lineIndex]
  }
}

Add-Type -TypeDefinition @'
using System;
using System.IO;
using System.Threading.Tasks;
public static class RawObjStager {
  public static void Copy(string[] sources, string[] destinations, int workers) {
    Parallel.For(0, sources.Length, new ParallelOptions { MaxDegreeOfParallelism = workers }, i => {
      File.Copy(sources[i], destinations[i], true);
    });
  }
}
'@

$timer = [Diagnostics.Stopwatch]::StartNew()
[RawObjStager]::Copy($sources.ToArray(), $destinations.ToArray(), $Workers)
[IO.File]::WriteAllLines((Join-Path $OutputDirectory 'raw.rsp'), $rewritten)
$bytes = ($destinations | ForEach-Object { [IO.FileInfo]$_ } | Measure-Object Length -Sum).Sum
Write-Host ("staged {0} independent raw objects, {1:n2} GiB in {2:n1}s" -f
            $sources.Count,($bytes/1GB),$timer.Elapsed.TotalSeconds)
