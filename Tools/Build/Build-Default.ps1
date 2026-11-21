# 기본 빌드 (9-4): 깃 저장소를 새로 클론한 폴더에서 Development로 패키징한다.
#  - 클론에는 Content/ThirdParty가 없으므로 유료 에셋이 섞이지 않는다 (9-2 결정: 기본 빌드는 깨끗한 클론에서만)
#  - 원격(origin)에 push된 커밋을 받으므로, 빌드 전에 커밋·push 한다
#  - 쿡할 맵은 Lvl_TestEnemies(시작 맵, DefaultEngine.ini GameDefaultMap)와 Lvl_Training
#  - 결과: Builds/Default/Windows/FirstAction.exe (작업 폴더 쪽에 둔다)
#
#   powershell -ExecutionPolicy Bypass -File Tools/Build/Build-Default.ps1
param(
    [string]$EngineRoot = "F:\Epic Games\Games\UE_5.7",
    [string]$Source = "",
    [string]$Branch = "main",
    [string]$CloneDir = "",
    [string]$OutDir = ""
)
$ErrorActionPreference = "Stop"
$ProjectDir = (Resolve-Path "$PSScriptRoot\..\..").Path
if (-not $Source) { $Source = (git -C $ProjectDir remote get-url origin).Trim() }
if (-not $CloneDir) { $CloneDir = Join-Path (Split-Path $ProjectDir -Parent) "FirstAction_DefaultBuild" }
if (-not $OutDir) { $OutDir = Join-Path $ProjectDir "Builds\Default" }
$Maps = "/Game/Portfolio/Maps/Lvl_TestEnemies+/Game/Portfolio/Maps/Lvl_Training"

$RunUAT = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"
if (-not (Test-Path $RunUAT)) { throw "RunUAT를 찾을 수 없습니다: $RunUAT (-EngineRoot로 엔진 경로 지정)" }

# 깨끗한 클론 (이전 클론은 지우고 새로)
if (Test-Path $CloneDir) {
    Write-Host "이전 클론을 지웁니다: $CloneDir"
    Remove-Item -Recurse -Force $CloneDir
}
git clone --branch $Branch $Source $CloneDir
if ($LASTEXITCODE -ne 0) { throw "git clone 실패" }
git -C $CloneDir lfs pull
if ($LASTEXITCODE -ne 0) { throw "git lfs pull 실패" }
if (Test-Path (Join-Path $CloneDir "Content\ThirdParty")) { throw "클론에 Content/ThirdParty가 있습니다. 유료 에셋이 저장소에 들어갔는지 확인하세요" }

& $RunUAT BuildCookRun `
    "-project=$(Join-Path $CloneDir 'FirstAction.uproject')" -platform=Win64 -clientconfig=Development `
    -build -cook "-map=$Maps" -stage -pak -archive "-archivedirectory=$OutDir" `
    -prereqs -nop4 -utf8output -unattended
if ($LASTEXITCODE -ne 0) { throw "BuildCookRun 실패 (종료 코드 $LASTEXITCODE)" }
Write-Host "기본 빌드 완료: $OutDir\Windows\FirstAction.exe"
