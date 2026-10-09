param(
    [string]$WorkDirectory = "build/tfs1098-windows-smoke",
    [int]$StartupTimeoutSeconds = 120
)

$ErrorActionPreference = 'Stop'

$repoRoot = (Get-Location).Path
$work = [System.IO.Path]::GetFullPath((Join-Path $repoRoot $WorkDirectory))
$runtimeArchive = Join-Path $work 'tfs-v1.4.2-windows-vcpkg.zip'
$runtimeExtract = Join-Path $work 'runtime'
$fixtureOut = Join-Path $work 'mysql-fixture.out.log'
$fixtureErr = Join-Path $work 'mysql-fixture.err.log'
$fixtureQueries = Join-Path $work 'mysql-fixture.queries.log'
$tfsOut = Join-Path $work 'tfs-windows.out.log'
$tfsErr = Join-Path $work 'tfs-windows.err.log'

Remove-Item -Recurse -Force $work -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $work -Force | Out-Null

$gitOpenSsl = 'C:\Program Files\Git\usr\bin'
if ((Test-Path (Join-Path $gitOpenSsl 'openssl.exe')) -and -not (Get-Command openssl -ErrorAction SilentlyContinue)) {
    $env:Path = "$gitOpenSsl;$env:Path"
}
if (-not (Get-Command openssl -ErrorAction SilentlyContinue)) {
    throw 'OpenSSL is required by tfs1098_protocol_probe.py and was not found in PATH.'
}
if (-not (Get-Command python -ErrorAction SilentlyContinue)) {
    throw 'Python is required for the TFS1098 homologation tools.'
}

Write-Host 'Downloading official TFS 1.4.2 Windows runtime...'
Invoke-WebRequest `
    -Uri 'https://github.com/otland/forgottenserver/releases/download/v1.4.2/tfs-v1.4.2-windows-vcpkg.zip' `
    -OutFile $runtimeArchive
Expand-Archive -Path $runtimeArchive -DestinationPath $runtimeExtract -Force

$configDist = Get-ChildItem $runtimeExtract -Recurse -File -Filter 'config.lua.dist' | Select-Object -First 1
$key = Get-ChildItem $runtimeExtract -Recurse -File -Filter 'key.pem' | Select-Object -First 1
$executables = Get-ChildItem $runtimeExtract -Recurse -File -Filter '*.exe' | Where-Object {
    $_.BaseName -match '(?i)tfs|forgottenserver'
}

if (-not $configDist) { throw 'config.lua.dist was not found in the official TFS Windows package.' }
if (-not $key) { throw 'key.pem was not found in the official TFS Windows package.' }
if (-not $executables) { throw 'TFS executable was not found in the official TFS Windows package.' }

$runtime = $configDist.Directory.FullName
$tfsExe = $executables | Where-Object { $_.Directory.FullName -eq $runtime } | Select-Object -First 1
if (-not $tfsExe) { $tfsExe = $executables | Select-Object -First 1 }

Write-Host "TFS runtime: $runtime"
Write-Host "TFS executable: $($tfsExe.FullName)"

$configPath = Join-Path $runtime 'config.lua'
$config = Get-Content $configDist.FullName -Raw
$config = $config.Replace('mysqlPass = ""', 'mysqlPass = "fantasy-ci"')
Set-Content -Path $configPath -Value $config -Encoding utf8

$fixtureScript = Join-Path $repoRoot 'Tools/Tfs1098/tfs1098_mysql_fixture.py'
$probeScript = Join-Path $repoRoot 'Tools/Tfs1098/tfs1098_protocol_probe.py'

$fixture = $null
$tfs = $null
try {
    $fixture = Start-Process -FilePath 'python' -ArgumentList @(
        $fixtureScript, '--host', '127.0.0.1', '--port', '3306', '--log', $fixtureQueries
    ) -PassThru -RedirectStandardOutput $fixtureOut -RedirectStandardError $fixtureErr

    $fixtureReady = $false
    for ($i = 0; $i -lt 60; $i++) {
        if (Test-Path $fixtureOut) {
            $text = Get-Content $fixtureOut -Raw -ErrorAction SilentlyContinue
            if ($text -match 'TFS1098_MYSQL_FIXTURE READY') {
                $fixtureReady = $true
                break
            }
        }
        $fixture.Refresh()
        if ($fixture.HasExited) {
            Get-Content $fixtureErr -ErrorAction SilentlyContinue
            throw 'MySQL fixture exited before readiness.'
        }
        Start-Sleep -Milliseconds 250
    }
    if (-not $fixtureReady) { throw 'MySQL fixture did not become ready.' }

    $tfs = Start-Process `
        -FilePath $tfsExe.FullName `
        -WorkingDirectory $runtime `
        -PassThru `
        -RedirectStandardOutput $tfsOut `
        -RedirectStandardError $tfsErr

    $online = $false
    for ($i = 0; $i -lt $StartupTimeoutSeconds; $i++) {
        if (Test-Path $tfsOut) {
            $text = Get-Content $tfsOut -Raw -ErrorAction SilentlyContinue
            if ($text -match 'Forgotten Server Online!') {
                $online = $true
                break
            }
        }
        $tfs.Refresh()
        if ($tfs.HasExited) {
            Get-Content $tfsOut -ErrorAction SilentlyContinue
            Get-Content $tfsErr -ErrorAction SilentlyContinue
            throw 'Official Windows TFS exited before reaching Online.'
        }
        Start-Sleep -Seconds 1
    }
    if (-not $online) {
        Get-Content $tfsOut -ErrorAction SilentlyContinue
        throw 'Official Windows TFS did not reach Online within the startup gate.'
    }

    Write-Host 'TFS1098_WINDOWS_SERVER ONLINE'
    & python $probeScript `
        --host 127.0.0.1 `
        --port 7172 `
        --key $key.FullName `
        --account fantasy `
        --password fantasy `
        --character 'Fantasy Test' `
        --timeout 15
    if ($LASTEXITCODE -ne 0) { throw "Protocol probe failed with exit code $LASTEXITCODE" }

    Write-Host 'TFS1098_WINDOWS_PROTOCOL_SMOKE PASS'
}
finally {
    if ($tfs) {
        $tfs.Refresh()
        if (-not $tfs.HasExited) { Stop-Process -Id $tfs.Id -Force -ErrorAction SilentlyContinue }
    }
    if ($fixture) {
        $fixture.Refresh()
        if (-not $fixture.HasExited) { Stop-Process -Id $fixture.Id -Force -ErrorAction SilentlyContinue }
    }
}
