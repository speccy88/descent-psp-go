[CmdletBinding(SupportsShouldProcess=$true,ConfirmImpact='Medium')]
param(
 [ValidateSet('Plan','Install','Verify','CollectLogs')][string]$Mode='Plan',
 [Parameter(Mandatory)][string]$PspRoot,
 [string]$Package=(Join-Path $PSScriptRoot '..\dist\Descent-Go'),
 [string]$ExpectedVolumeId,
 [string]$ExpectedDeviceId,
 [switch]$DestinationConfirmed,
 [switch]$Update,
 [string]$BackupRoot=(Join-Path $PSScriptRoot '..\backups'),
 [string]$Output=(Join-Path $PSScriptRoot '..\collected-logs')
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
function Assert-NoReparse([string]$Path){
 $item=Get-Item -LiteralPath $Path -Force
 if($item.Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Reparse point rejected: $Path"}
}
function Assert-Tree([string]$Path){
 Assert-NoReparse $Path
 foreach($entry in Get-ChildItem -LiteralPath $Path -Force){
  Assert-NoReparse $entry.FullName
  if($entry.PSIsContainer){Assert-Tree $entry.FullName}
 }
}
$root=[IO.Path]::GetFullPath($PspRoot)
if($root -notmatch '^[A-Za-z]:\\$'){throw 'PspRoot must be an explicit drive root, such as F:\.'}
if($root.Substring(0,2) -eq $env:SystemDrive){throw 'System volume excluded.'}
$letter=$root.Substring(0,1)
function Get-Identity {
 $vol=Get-Volume -DriveLetter $letter
 if($vol.FileSystemLabel -match '(?i)lexar'){throw 'Lexar backup storage is excluded.'}
 if($vol.FileSystem -ne 'FAT32'){throw 'Expected PSP FAT32 storage.'}
 $association=@()
 foreach($disk in Get-CimInstance Win32_DiskDrive){
  foreach($part in Get-CimAssociatedInstance -InputObject $disk -Association Win32_DiskDriveToDiskPartition){
   foreach($logical in Get-CimAssociatedInstance -InputObject $part -Association Win32_LogicalDiskToPartition){
    if($logical.DeviceID -eq "${letter}:"){$association+= [pscustomobject]@{Model=$disk.Model;DeviceId=$disk.PNPDeviceID;DiskSerial=$disk.SerialNumber;VolumeSerial=$logical.VolumeSerialNumber;DiskIndex=$disk.Index}}
   }
  }
 }
 if($association.Count -ne 1){throw 'Ambiguous or unavailable disk association; device writes prohibited.'}
 $a=$association[0]
 if($a.Model -match '(?i)Lexar' -or $a.DeviceId -notmatch '(?i)^USBSTOR\\DISK&VEN_SONY&PROD_.*PSP'){throw 'Storage is not positively associated with a Sony PSP USB disk.'}
 [pscustomobject]@{Root=$root;VolumeId=$vol.UniqueId;Label=$vol.FileSystemLabel;Filesystem=$vol.FileSystem;Capacity=$vol.Size;FreeBytes=$vol.SizeRemaining;DeviceId=$a.DeviceId;Model=$a.Model;DiskSerial=$a.DiskSerial;VolumeSerial=$(if($a.VolumeSerial){$a.VolumeSerial}else{'UNKNOWN'});Target=(Join-Path $root 'PSP\GAME\Descent-Go')}
}
function Check-Identity {
 $current=Get-Identity
 if(!$ExpectedVolumeId -or !$ExpectedDeviceId){throw 'Writes require ExpectedVolumeId and ExpectedDeviceId.'}
 if($current.VolumeId -cne $ExpectedVolumeId -or $current.DeviceId -cne $ExpectedDeviceId){throw 'Volume/device identity changed. No write performed.'}
 return $current
}
function Relative-Safe([string]$base,[string]$name){
 if([IO.Path]::IsPathRooted($name) -or $name -match '(^|[\\/])\.\.([\\/]|$)' -or $name.Contains(':')){throw 'Unsafe manifest path.'}
 $result=[IO.Path]::GetFullPath((Join-Path $base $name))
 if(!$result.StartsWith($base.TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Path escaped authorized directory.'}
 return $result
}
function Verify-Files([string]$base,$manifest){
 Assert-Tree $base
 foreach($file in $manifest.files){
  $p=Relative-Safe $base $file.path
  if(!(Test-Path -LiteralPath $p -PathType Leaf)){throw "Missing $($file.path)"}
  if((Get-Item -LiteralPath $p).Length -ne $file.bytes -or (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLowerInvariant() -cne $file.sha256){throw "Hash mismatch $($file.path)"}
 }
}
function Snapshot([string]$base){
 $files=@(Get-ChildItem -LiteralPath $base -File -Recurse -Force)
 foreach($f in $files){[pscustomobject]@{path=$f.FullName.Substring($base.TrimEnd('\').Length+1);bytes=$f.Length;sha256=(Get-FileHash -LiteralPath $f.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}}
}
$identity=Get-Identity
$identity | Format-List
foreach($p in @($root,(Join-Path $root 'PSP'),(Join-Path $root 'PSP\GAME'))){Assert-NoReparse $p}
$target=$identity.Target
$packagePath=(Resolve-Path -LiteralPath $Package).Path
Assert-Tree $packagePath
$manifest=Get-Content -LiteralPath (Join-Path $packagePath 'manifest.json') -Raw | ConvertFrom-Json
Verify-Files $packagePath $manifest
# Parse PBP/SFO/data edition before copying; verifier is bundled with this source tree.
& python (Join-Path $PSScriptRoot 'verify-package.py') $packagePath
if($LASTEXITCODE -ne 0){throw 'Package validation failed.'}
if($Mode -eq 'Plan'){
 [pscustomobject]@{Mode='Plan';DeviceWrites=0;PackageBytes=($manifest.files|Measure-Object bytes -Sum).Sum;ExistingInstall=(Test-Path -LiteralPath $target);ExpectedVolumeId=$identity.VolumeId;ExpectedDeviceId=$identity.DeviceId}
 return
}
if($Mode -eq 'Verify'){
 $null=Check-Identity
 Verify-Files $target $manifest
 'READBACK VERIFIED: '+$target+' / '+$manifest.version+' / '+$manifest.edition
 return
}
if($Mode -eq 'CollectLogs'){
 Assert-Tree $target
 $destination=[IO.Path]::GetFullPath($Output)
 if($destination.StartsWith($root,[StringComparison]::OrdinalIgnoreCase)){throw 'CollectLogs output must be on local storage.'}
 if($PSCmdlet.ShouldProcess($destination,'Copy PSP logs locally')){New-Item -ItemType Directory -Path $destination -Force|Out-Null;Copy-Item -LiteralPath (Join-Path $target 'logs') -Destination (Join-Path $destination ([guid]::NewGuid().ToString('N'))) -Recurse}
 return
}
$identity=Check-Identity
if(!$DestinationConfirmed){throw 'Obtain one user confirmation of the displayed destination, then pass -DestinationConfirmed.'}
$exists=Test-Path -LiteralPath $target
if($exists -and !$Update){throw 'Existing Descent-Go left intact. Explicit -Update is required after review.'}
if($exists){Assert-Tree $target}
$game=[IO.Path]::GetFullPath((Join-Path $root 'PSP\GAME'))
$token=[guid]::NewGuid().ToString('N')
$stage=Relative-Safe $game ('.Descent-Go.stage-'+$token)
$previous=Relative-Safe $game ('.Descent-Go.previous-'+$token)
$packageBytes=(Get-ChildItem -LiteralPath $packagePath -Recurse -File|Measure-Object Length -Sum).Sum
$oldBytes=0
if($exists){$oldBytes=(Get-ChildItem -LiteralPath $target -Recurse -File|Measure-Object Length -Sum).Sum}
if($identity.FreeBytes -lt ($packageBytes+$oldBytes+8MB)){throw 'Insufficient free space for staging and retained data.'}
$backup=[IO.Path]::GetFullPath((Join-Path $BackupRoot ('Descent-Go-'+$token)))
if($backup.StartsWith($root,[StringComparison]::OrdinalIgnoreCase)){throw 'Backup must be on local storage.'}
if(!$PSCmdlet.ShouldProcess($target,"Install $($manifest.version) ($($manifest.edition)); SHA-256 verified staging")){return}
$journal=[ordered]@{id=$token;started=(Get-Date).ToString('o');identity=$identity;stage=$stage;target=$target;status='started';backup=$null}
$journalPath=Join-Path $PSScriptRoot ('..\install-'+$token+'.json')
$journal|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $journalPath -Encoding utf8
try{
 if($exists){
  $oldManifest=[pscustomobject]@{files=@(Snapshot $target)}
  New-Item -ItemType Directory -Path $backup -Force|Out-Null
  foreach($entry in Get-ChildItem -LiteralPath $target -Force){Copy-Item -LiteralPath $entry.FullName -Destination $backup -Recurse}
  Verify-Files $backup $oldManifest
  $journal.backup=$backup
 }
 $null=Check-Identity
 New-Item -ItemType Directory -Path $stage|Out-Null
 if($exists){foreach($entry in Get-ChildItem -LiteralPath $target -Force){Copy-Item -LiteralPath $entry.FullName -Destination $stage -Recurse}}
 foreach($file in $manifest.files){
  $null=Check-Identity
  $to=Relative-Safe $stage $file.path
  if($exists -and $file.path -match '^(userdata|logs)[/\\]'){continue}
  New-Item -ItemType Directory -Path (Split-Path -Parent $to) -Force|Out-Null
  Copy-Item -LiteralPath (Relative-Safe $packagePath $file.path) -Destination $to -Force
 }
 Copy-Item -LiteralPath (Join-Path $packagePath 'manifest.json') -Destination (Join-Path $stage 'manifest.json')
 foreach($dir in @('userdata','logs')){New-Item -ItemType Directory -Path (Join-Path $stage $dir) -Force|Out-Null}
 Verify-Files $stage $manifest
 $null=Check-Identity
 Assert-NoReparse $game
 if($exists){Move-Item -LiteralPath $target -Destination $previous; $journal.previous=$previous}
 elseif(Test-Path -LiteralPath $target){throw 'Target appeared during staging; refusing replacement.'}
 Move-Item -LiteralPath $stage -Destination $target
 Verify-Files $target $manifest
 $journal.status='readback-verified';$journal.finished=(Get-Date).ToString('o')
 $journal|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $journalPath -Encoding utf8
 'INSTALLED AND READBACK VERIFIED: '+$target+' / '+$manifest.version+' / '+$manifest.edition
 'Safely eject, leave USB mode, then launch Game > System Storage > Descent PSP Go.'
}catch{
 $journal.status='stopped';$journal.error=$_.Exception.Message
 $journal|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $journalPath -Encoding utf8
 throw # No automatic filesystem repair, deletion, or further device writes.
}
