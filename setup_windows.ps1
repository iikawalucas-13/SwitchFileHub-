$ErrorActionPreference = "Stop"

# Cria a pasta switch_file_hub (estrutura nativa Windows/Android) e copia o codigo deste pacote.
$sourceRoot  = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectPath = Join-Path $sourceRoot "switch_file_hub"

if (-not (Get-Command flutter -ErrorAction SilentlyContinue)) {
    Write-Host "Flutter SDK não foi encontrado no PATH. Instale o Flutter e tente novamente." -ForegroundColor Red
    exit 1
}

if (Test-Path $projectPath) {
    Write-Host "A pasta switch_file_hub já existe. Renomeie ou remova essa pasta antes de executar novamente." -ForegroundColor Yellow
    exit 1
}

flutter config --enable-windows-desktop | Out-Null
flutter create --platforms=windows,android --org com.switchfilehub --project-name switch_file_hub $projectPath
if ($LASTEXITCODE -ne 0) { throw "flutter create falhou (código $LASTEXITCODE)." }

Copy-Item -Recurse -Force (Join-Path $sourceRoot "lib")  $projectPath
Copy-Item -Recurse -Force (Join-Path $sourceRoot "test") $projectPath
Copy-Item -Force (Join-Path $sourceRoot "pubspec.yaml") (Join-Path $projectPath "pubspec.yaml")
Copy-Item -Force (Join-Path $sourceRoot "analysis_options.yaml") (Join-Path $projectPath "analysis_options.yaml")

$manifestSource = Join-Path $sourceRoot "tool\android\AndroidManifest.xml"
$manifestTarget = Join-Path $projectPath "android\app\src\main\AndroidManifest.xml"
if ((Test-Path $manifestSource) -and (Test-Path $manifestTarget)) {
    Copy-Item -Force $manifestSource $manifestTarget
}

# Nome da janela e metadados do .exe
$mainCpp = Join-Path $projectPath "windows\runner\main.cpp"
if (Test-Path $mainCpp) {
    $c = Get-Content -Raw -Encoding UTF8 $mainCpp
    $c = $c -replace 'L"switch_file_hub"', 'L"Switch File Hub"'
    Set-Content -Encoding UTF8 -NoNewline -Path $mainCpp -Value $c
}
$rc = Join-Path $projectPath "windows\runner\Runner.rc"
if (Test-Path $rc) {
    $r = Get-Content -Raw $rc
    $r = $r -replace '(VALUE "FileDescription", ")[^"]*(")',  '${1}Switch File Hub${2}'
    $r = $r -replace '(VALUE "ProductName", ")[^"]*(")',      '${1}Switch File Hub${2}'
    Set-Content -NoNewline -Path $rc -Value $r
}

Write-Host ""
Write-Host "Estrutura criada em: $projectPath" -ForegroundColor Green
