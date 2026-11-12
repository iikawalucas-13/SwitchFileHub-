# Compila o Switch File Hub para Windows 11 (Release) e monta a pasta distribuivel.
# Uso: .\build_windows.ps1   (ou de duplo clique em build_windows.bat)
$ErrorActionPreference = "Stop"
$sourceRoot  = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectPath = Join-Path $sourceRoot "switch_file_hub"
$distRoot    = Join-Path $sourceRoot "dist"
$distPath    = Join-Path $distRoot "SwitchFileHub_Windows_Release"

function Step($msg) { Write-Host "`n==> $msg" -ForegroundColor Cyan }
function Run($cmd, $arguments) {
    & $cmd @arguments
    if ($LASTEXITCODE -ne 0) { throw "'$cmd $($arguments -join ' ')' falhou (código $LASTEXITCODE)." }
}

# 0. Pré-requisitos
Step "Verificando pré-requisitos"
if (-not (Get-Command flutter -ErrorAction SilentlyContinue)) {
    throw "Flutter SDK não encontrado no PATH. Instale: https://docs.flutter.dev/get-started/install/windows/desktop"
}
$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
$hasVcTools = $false
if (Test-Path $vswhere) {
    $found = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($found) { $hasVcTools = $true }
}
if (-not $hasVcTools) {
    throw "Visual Studio 2022 com a carga 'Desenvolvimento para desktop com C++' não foi encontrado. Instale em https://visualstudio.microsoft.com/ e rode novamente."
}
Run flutter @("config", "--enable-windows-desktop")

# 1. Estrutura do projeto
if (-not (Test-Path $projectPath)) {
    Step "Criando estrutura nativa (flutter create)"
    & (Join-Path $sourceRoot "setup_windows.ps1")
    if ($LASTEXITCODE -ne 0) { throw "setup_windows.ps1 falhou." }
} else {
    Step "Pasta switch_file_hub já existe: sincronizando código-fonte"
    Copy-Item -Recurse -Force (Join-Path $sourceRoot "lib")  $projectPath
    Copy-Item -Recurse -Force (Join-Path $sourceRoot "test") $projectPath
    Copy-Item -Force (Join-Path $sourceRoot "pubspec.yaml") (Join-Path $projectPath "pubspec.yaml")
    Copy-Item -Force (Join-Path $sourceRoot "analysis_options.yaml") (Join-Path $projectPath "analysis_options.yaml")
}

Push-Location $projectPath
try {
    # 2. Dependências
    Step "flutter pub get"
    $out = & flutter pub get 2>&1
    $out | ForEach-Object { Write-Host $_ }
    if ($LASTEXITCODE -ne 0) {
        if (($out | Out-String) -match "symlink") {
            Write-Host "Ative o Modo de Desenvolvedor do Windows (Configurações > Para desenvolvedores) e rode de novo." -ForegroundColor Yellow
            Start-Process "ms-settings:developers"
        }
        throw "flutter pub get falhou."
    }

    # 3. Análise e testes
    Step "flutter analyze"
    Run flutter @("analyze")
    Step "flutter test"
    Run flutter @("test")

    # 4. Build Release
    Step "flutter build windows --release"
    Run flutter @("build", "windows", "--release")
}
finally { Pop-Location }

# 5. Localizar a saída
$releaseDir = Join-Path $projectPath "build\windows\x64\runner\Release"
if (-not (Test-Path (Join-Path $releaseDir "switch_file_hub.exe"))) {
    $exe = Get-ChildItem -Path (Join-Path $projectPath "build\windows") -Recurse -Filter "switch_file_hub.exe" -ErrorAction SilentlyContinue |
           Where-Object { $_.FullName -match "Release" } | Select-Object -First 1
    if (-not $exe) { throw "switch_file_hub.exe não foi encontrado após o build." }
    $releaseDir = $exe.DirectoryName
}

# 6. Pasta distribuível (exe + DLLs + data)
Step "Montando $distPath"
if (Test-Path $distPath) { Remove-Item -Recurse -Force $distPath }
New-Item -ItemType Directory -Force -Path $distPath | Out-Null
Copy-Item -Recurse -Force (Join-Path $releaseDir "*") $distPath

# Runtime do Visual C++ (uso local ao app) para PCs sem o Redistributable instalado
foreach ($dll in "msvcp140.dll", "vcruntime140.dll", "vcruntime140_1.dll") {
    $src = Join-Path $env:WINDIR "System32\$dll"
    if (Test-Path $src) { Copy-Item -Force $src $distPath }
}

# 7. Conferência e ZIP
foreach ($need in "switch_file_hub.exe", "flutter_windows.dll", "data\app.so", "data\icudtl.dat") {
    if (-not (Test-Path (Join-Path $distPath $need))) {
        Write-Host "AVISO: item esperado ausente: $need" -ForegroundColor Yellow
    }
}
$zip = Join-Path $distRoot "SwitchFileHub_Windows_Release.zip"
if (Test-Path $zip) { Remove-Item -Force $zip }
Compress-Archive -Path (Join-Path $distPath "*") -DestinationPath $zip

Write-Host "`nConcluído." -ForegroundColor Green
Write-Host "Executável: $(Join-Path $distPath 'switch_file_hub.exe')"
Write-Host "ZIP:        $zip"
