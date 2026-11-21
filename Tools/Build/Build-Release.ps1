# 배포 빌드 (9-4): 유료 에셋(Content/ThirdParty)이 있는 작업 폴더에서 Shipping으로 패키징한다.
#  - 시작 맵: 빌드하는 동안만 Config/DefaultEngine.ini의 GameDefaultMap을 배포 맵으로 바꾸고, 끝나면(실패해도) 원래 바이트로 되돌린다.
#    윈도우 패키징은 -cmdline(UECommandLine.txt)을 쓰지 않고(엔진 WinPlatform.ShouldStageCommandLine = false),
#    Shipping은 명령줄 ini 덮어쓰기를 막으며, 설치형 엔진은 CustomConfig 타깃을 막는다 (PLAN 9장 "배포 빌드 시작 맵")
#  - 쿡할 맵은 배포 맵 하나. 그 맵이 참조하는 에셋만 들어간다(Agora 샘플 맵 전체, 기본 맵은 들어가지 않는다)
#  - 결과: Builds/Release/Windows/FirstAction.exe
#  - 에디터는 닫고 실행한다(Live Coding이 켜져 있으면 빌드가 막힌다)
#
#   powershell -ExecutionPolicy Bypass -File Tools/Build/Build-Release.ps1
param(
    [string]$EngineRoot = "F:\Epic Games\Games\UE_5.7",
    [string]$OutDir = ""
)
$ErrorActionPreference = "Stop"
$ProjectDir = (Resolve-Path "$PSScriptRoot\..\..").Path
$Project = Join-Path $ProjectDir "FirstAction.uproject"
if (-not $OutDir) { $OutDir = Join-Path $ProjectDir "Builds\Release" }
$ReleaseMap = "/Game/ThirdParty/Release/Maps/Lvl_Release_Agora"
$DefaultMapLine = "GameDefaultMap=/Game/Portfolio/Maps/Lvl_TestEnemies.Lvl_TestEnemies"
$ReleaseMapLine = "GameDefaultMap=$ReleaseMap.Lvl_Release_Agora"
$EngineIni = Join-Path $ProjectDir "Config\DefaultEngine.ini"

if (-not (Test-Path (Join-Path $ProjectDir "Content\ThirdParty\Release\Maps\Lvl_Release_Agora.umap"))) {
    throw "배포 맵이 없습니다(Content/ThirdParty). 유료 에셋이 있는 작업 폴더에서 실행하세요"
}
$RunUAT = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"
if (-not (Test-Path $RunUAT)) { throw "RunUAT를 찾을 수 없습니다: $RunUAT (-EngineRoot로 엔진 경로 지정)" }
if (Get-Process UnrealEditor -ErrorAction SilentlyContinue) { throw "에디터를 닫고 실행하세요(Live Coding이 빌드를 막습니다)" }

# 시작 맵 바꾸기. 지난 실행이 도중에 끊겨 배포 맵이 남아 있으면 먼저 기본 맵으로 되돌린다
$Utf8 = [Text.UTF8Encoding]::new($false)
$IniText = [IO.File]::ReadAllText($EngineIni, $Utf8)
if ($IniText.Contains($ReleaseMapLine)) {
    Write-Warning "DefaultEngine.ini에 배포 시작 맵이 남아 있습니다(지난 빌드가 끊김). 기본 맵으로 되돌립니다"
    $IniText = $IniText.Replace($ReleaseMapLine, $DefaultMapLine)
    [IO.File]::WriteAllText($EngineIni, $IniText, $Utf8)
}
if (-not $IniText.Contains($DefaultMapLine)) { throw "DefaultEngine.ini에서 '$DefaultMapLine' 줄을 찾지 못했습니다" }
$OriginalBytes = [IO.File]::ReadAllBytes($EngineIni)

try {
    [IO.File]::WriteAllText($EngineIni, $IniText.Replace($DefaultMapLine, $ReleaseMapLine), $Utf8)
    Write-Host "시작 맵(빌드 중에만): $ReleaseMap"
    & $RunUAT BuildCookRun `
        "-project=$Project" -platform=Win64 -clientconfig=Shipping `
        -build -cook "-map=$ReleaseMap" -stage -pak -archive "-archivedirectory=$OutDir" `
        -prereqs -nodebuginfo -nop4 -utf8output -unattended
    $Code = $LASTEXITCODE
}
finally {
    [IO.File]::WriteAllBytes($EngineIni, $OriginalBytes)
    Write-Host "DefaultEngine.ini 시작 맵을 되돌렸습니다"
}
if ($Code -ne 0) { throw "BuildCookRun 실패 (종료 코드 $Code)" }
Write-Host "배포 빌드 완료: $OutDir\Windows\FirstAction.exe"
