$ErrorActionPreference='Stop'
if($env:VSCMD_ARG_TGT_ARCH -ne 'x86'){throw 'Run in an x86 Native Tools / developer shell for Visual Studio.'}
$r=$PSScriptRoot;$out=Join-Path $r 'build';$temp=Join-Path $out 'temp';[void][IO.Directory]::CreateDirectory($temp);$env:TEMP=$temp;$env:TMP=$temp
& cl.exe /nologo /utf-8 /O2 /MT /LD /W3 "$r/src/rr4k-isolation.c" "/Fo$out/rr4k-isolation.obj" /link advapi32.lib user32.lib gdi32.lib dbghelp.lib "/OUT:$out/rr4k-isolation.dll" "/IMPLIB:$out/rr4k-isolation.lib"
if($LASTEXITCODE){throw 'Core compilation failed'}
& cl.exe /nologo /utf-8 /O2 /MT /W3 "$r/tests/auto-display.c" "/Fo$out/auto-display.obj" "/Fe$out/auto-display.exe" /link user32.lib gdi32.lib
if($LASTEXITCODE){throw 'Config test compilation failed'}
& "$out/auto-display.exe" (Join-Path $out ('config-fixture-'+[guid]::NewGuid().ToString('N')))
if($LASTEXITCODE){throw 'Config regression failed'}
& cl.exe /nologo /utf-8 /O2 /MT /W3 "$r/tests/pixel-format.c" "/Fo$out/pixel-format.obj" "/Fe$out/pixel-format.exe" /link user32.lib gdi32.lib
if($LASTEXITCODE){throw 'Pixel-format test compilation failed'}
& "$out/pixel-format.exe" (Join-Path $out ('pixel-fixture-'+[guid]::NewGuid().ToString('N')))
if($LASTEXITCODE){throw 'Pixel-format regression failed'}

foreach($kind in @('legacy','candidate')){
 $defs=@();if($kind -eq 'legacy'){$defs=@('/DLEGACY')}
 & cl.exe /nologo /utf-8 /O2 /MT /W3 @defs "$r/tests/file-access.c" "/Fo$out/file-access-$kind.obj" "/Fe$out/file-access-$kind.exe" /link user32.lib
 if($LASTEXITCODE){throw "Save access test compilation failed: $kind"}
 & "$out/file-access-$kind.exe" (Join-Path $out ('save-fixture-'+[guid]::NewGuid().ToString('N')))
 if($LASTEXITCODE){throw "Save access regression failed: $kind"}
}
