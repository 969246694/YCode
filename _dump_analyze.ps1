# Minimal minidump analyzer: extract exception info and faulting module
param([string]$DumpPath = "F:\YiyangzaiCode\_dump.dmp")

$bytes = [System.IO.File]::ReadAllBytes($DumpPath)
$br = New-Object System.IO.BinaryReader([System.IO.MemoryStream]::new($bytes))

# MDMP header: 4 sig + 4 version + 4 numStreams + 4 streamDirectoryRva + 4 checksum + 4 timestamp + 4 flags
$sig = [System.Text.Encoding]::ASCII.GetString($br.ReadBytes(4))
Write-Output "Signature: $sig"
$br.BaseStream.Position = 4
$version = $br.ReadUInt32()
$numStreams = $br.ReadUInt32()
$dirRva = $br.ReadUInt32()
$br.BaseStream.Position = 24
$flags = $br.ReadUInt32()
Write-Output "streams=$numStreams dirRva=$dirRva"

# Stream directory: each entry = 4 type + 4 dataSize + 4 rva
$modules = @{}
$threads = @()
for ($i = 0; $i -lt $numStreams; $i++) {
    $pos = $dirRva + $i * 12
    $br.BaseStream.Position = $pos
    $type = $br.ReadUInt32()
    $size = $br.ReadUInt32()
    $rva = $br.ReadUInt32()
    switch ($type) {
        4 { # ModuleListStream
            $br.BaseStream.Position = $rva
            $count = $br.ReadUInt32()
            for ($m = 0; $m -lt $count; $m++) {
                $base = $br.ReadUInt64()
                $size2 = $br.ReadUInt32()
                $br.BaseStream.Position += 4
                $nameRva = $br.ReadUInt32()
                $save = $br.BaseStream.Position
                $br.BaseStream.Position = $rva + $nameRva
                $name = ""
                while ($true) {
                    $ch = $br.ReadByte()
                    if ($ch -eq 0) { break }
                    $name += [char]$ch
                }
                $br.BaseStream.Position = $save
                # skip the rest of MINIDUMP_MODULE (CvRecord, MiscRecord, Reserved)
                $br.BaseStream.Position += 4 + 4 + 8 + 8 + 4 + 4 + 4
                $modules[$base] = @{ Name = $name; Size = $size2 }
            }
        }
        3 { # ThreadListStream
            $br.BaseStream.Position = $rva
            $count = $br.ReadUInt32()
            for ($t = 0; $t -lt $count; $t++) {
                $threadId = $br.ReadUInt32()
                $suspendCount = $br.ReadUInt32()
                $priorityClass = $br.ReadUInt32()
                $priority = $br.ReadUInt32()
                $threadContextRva = $br.ReadUInt64()
                $threadContextSize = $br.ReadUInt32()
                # thread-specific context follows; record minimal info
                $threads += @{ Id = $threadId; ContextRva = $threadContextRva; CtxSize = $threadContextSize }
            }
        }
        6 { # ExceptionStream
            $br.BaseStream.Position = $rva
            $threadId = $br.ReadUInt32()
            $align = $br.ReadUInt32()
            $br.BaseStream.Position += 8 # exception record: code, flags, record
            $exCode = $br.ReadUInt32()
            $exFlags = $br.ReadUInt32()
            $exRecord = $br.ReadUInt64()
            $exAddress = $br.ReadUInt64()
            $exNumParams = $br.ReadUInt32()
            Write-Output "EXCEPTION on thread $threadId code=0x$('{0:X8}' -f $exCode) address=0x$('{0:X}' -f $exAddress) numParams=$exNumParams"
            for ($p = 0; $p -lt $exNumParams; $p++) {
                $pv = $br.ReadUInt64()
                Write-Output "  param$p = 0x$('{0:X}' -f $pv)"
            }
        }
    }
}

# Map exception address to module (approximate using previous dump if no exception stream printed, find modules covering RIP from thread contexts)
Write-Output "`nModules:"
foreach ($k in ($modules.Keys | Sort-Object)) {
    $m = $modules[$k]
    Write-Output ("  0x{0:X}  size={1}  {2}" -f $k, $m.Size, $m.Name)
}
