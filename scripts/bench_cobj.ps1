param(
  [Parameter(Mandatory=$true)][string]$Rsp,
  [Parameter(Mandatory=$true)][string]$Output,
  [string]$Radlink = '',
  [string]$WorkingDirectory = '',
  [string]$LogDirectory = '',
  [int]$CacheGiB = 17,
  [int]$CacheShrinkGiB = 6,
  [switch]$CacheFreeze = $true,
  [ValidateSet('','types','except_debug_s','except_debug_s_runs','all')][string]$CacheFreezeTrim = '',
  [int]$SkipCleanup = 1,
  [string]$Tag = 'v2',
  [string]$ExtraArgs = '',
  [switch]$TrimWorkingSet = $true,
  [switch]$TrimProcessWorkingSet,
  [switch]$TraceMemory
)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (!$Radlink) { $Radlink = Join-Path $root 'out_cobj\radlink.exe' }
if (!$WorkingDirectory) { $WorkingDirectory = (Get-Location).Path }
if (!$LogDirectory) { $LogDirectory = Join-Path $root 'cobj_bench_output' }
$Radlink = [IO.Path]::GetFullPath($Radlink)
$WorkingDirectory = [IO.Path]::GetFullPath($WorkingDirectory)
$LogDirectory = [IO.Path]::GetFullPath($LogDirectory)
[IO.Directory]::CreateDirectory($LogDirectory) | Out-Null
$log = Join-Path $LogDirectory "cobj_$Tag.log"
$phase = Join-Path $LogDirectory "cobj_$Tag.phase.log"
Remove-Item -LiteralPath $log,$phase -Force -ErrorAction SilentlyContinue
$psi = [Diagnostics.ProcessStartInfo]::new()
$psi.FileName = $Radlink
$psi.WorkingDirectory = $WorkingDirectory
$psi.Arguments = '@"' + [IO.Path]::GetFullPath($Rsp) + '" /RAD_LOG:timers ' + $ExtraArgs
$psi.UseShellExecute = $false
$psi.CreateNoWindow = $true
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true
$psi.Environment['RAD_COBJ_CACHE_GIB'] = [string]$CacheGiB
$psi.Environment['RAD_COBJ_SKIP_CLEANUP'] = [string]$SkipCleanup
if ($CacheShrinkGiB -gt 0) { $psi.Environment['RAD_COBJ_CACHE_SHRINK_GIB'] = [string]$CacheShrinkGiB }
else { [void]$psi.Environment.Remove('RAD_COBJ_CACHE_SHRINK_GIB') }
if ($CacheFreeze) { $psi.Environment['RAD_COBJ_CACHE_FREEZE'] = '1' }
else { [void]$psi.Environment.Remove('RAD_COBJ_CACHE_FREEZE') }
if ($CacheFreezeTrim) {
  $psi.Environment['RAD_COBJ_CACHE_FREEZE_TRIM'] = if ($CacheFreezeTrim -eq 'all') { '1' } else { $CacheFreezeTrim }
} else { [void]$psi.Environment.Remove('RAD_COBJ_CACHE_FREEZE_TRIM') }
if ($TrimWorkingSet) { $psi.Environment['RAD_COBJ_TRIM_WS'] = '1' }
if ($TrimProcessWorkingSet) { $psi.Environment['RAD_COBJ_TRIM_WS'] = '2' }
$psi.Environment['RADLINK_PHASE_LOG'] = $phase
$p = [Diagnostics.Process]::new(); $p.StartInfo = $psi
$sw = [Diagnostics.Stopwatch]::StartNew(); [void]$p.Start()
$stdoutTask = $p.StandardOutput.ReadToEndAsync()
$stderrTask = $p.StandardError.ReadToEndAsync()
$peakPrivate = 0L; $peakCommit = 0L; $peakWs = 0L
$memoryTrace = [Collections.Generic.List[string]]::new()
if ($TraceMemory) { $memoryTrace.Add('ElapsedSeconds,PrivateGiB,WorkingSetGiB') }
while (!$p.HasExited) {
  try {
    $p.Refresh()
    $private = $p.PrivateMemorySize64
    $commit = $p.PeakPagedMemorySize64
    $ws = $p.WorkingSet64
    $peakPrivate=[Math]::Max($peakPrivate,$private)
    $peakCommit=[Math]::Max($peakCommit,$commit)
    $peakWs=[Math]::Max($peakWs,$ws)
    if ($TraceMemory) {
      $memoryTrace.Add(('{0:F3},{1:F6},{2:F6}' -f $sw.Elapsed.TotalSeconds,($private/1GB),($ws/1GB)))
    }
  } catch {}
  Start-Sleep -Milliseconds 50
}
$stdout=$stdoutTask.GetAwaiter().GetResult(); $stderr=$stderrTask.GetAwaiter().GetResult(); $sw.Stop()
[IO.File]::WriteAllText($log, $stdout + $stderr)
if ($TraceMemory) { [IO.File]::WriteAllLines((Join-Path $LogDirectory "cobj_$Tag.memory.csv"), $memoryTrace) }
$p.Refresh(); $peakPrivate=[Math]::Max($peakPrivate,$p.PrivateMemorySize64); $peakCommit=[Math]::Max($peakCommit,$p.PeakPagedMemorySize64); $peakWs=[Math]::Max($peakWs,$p.WorkingSet64)
$hash = if ($p.ExitCode -eq 0 -and (Test-Path -LiteralPath $Output)) { (Get-FileHash -LiteralPath $Output -Algorithm SHA256).Hash } else { '' }
[pscustomobject]@{Tag=$Tag;ExitCode=$p.ExitCode;WallSeconds=$sw.Elapsed.TotalSeconds;PeakCommitGiB=$peakCommit/1GB;PeakPrivateGiB=$peakPrivate/1GB;PeakWorkingSetGiB=$peakWs/1GB;UserSeconds=$p.UserProcessorTime.TotalSeconds;KernelSeconds=$p.PrivilegedProcessorTime.TotalSeconds;Sha256=$hash;Log=$log;PhaseLog=$phase} | ConvertTo-Json -Compress
Get-Content -LiteralPath $log -Tail 30
