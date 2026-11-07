# 开发循环：配置 -> 构建 -> 测试 -> 冒烟启动。
# 用法：
#   .\dev.ps1              增量构建并跑全部测试
#   .\dev.ps1 -Clean       删掉 build/ 重新配置
#   .\dev.ps1 -NoTest      只构建
#   .\dev.ps1 -NoSmoke     跳过 --smoke-test 启动检查
# 若提示“未对文件进行数字签名”，用：
#   powershell -NoProfile -ExecutionPolicy Bypass -File .\dev.ps1
param(
    [switch]$Clean,
    [switch]$NoTest,
    [switch]$NoSmoke
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$build = Join-Path $root 'build'

$cmake = (Get-Command cmake -ErrorAction SilentlyContinue).Source
if (-not $cmake) { $cmake = 'C:\msys64\ucrt64\bin\cmake.exe' }
if (-not (Test-Path $cmake)) { throw "找不到 cmake，请把 Qt/UCRT64 的 bin 加进 PATH" }

if ($Clean -and (Test-Path $build)) {
    Write-Host "== 清理 $build" -ForegroundColor Yellow
    Remove-Item -Recurse -Force $build
}

Write-Host "== 配置" -ForegroundColor Cyan
& $cmake -S $root -B $build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
if ($LASTEXITCODE -ne 0) { throw "cmake 配置失败" }

Write-Host "== 构建" -ForegroundColor Cyan
& $cmake --build $build
if ($LASTEXITCODE -ne 0) { throw "构建失败" }

if (-not $NoTest) {
    Write-Host "== 测试" -ForegroundColor Cyan
    & ctest --test-dir $build --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw "测试失败" }
}

if (-not $NoSmoke) {
    Write-Host "== 冒烟启动" -ForegroundColor Cyan
    $exe = Join-Path $build 'task_notebook_gui.exe'
    $proc = Start-Process -FilePath $exe -ArgumentList '--smoke-test' -PassThru -Wait
    if ($proc.ExitCode -ne 0) {
        throw "冒烟测试失败（退出码 $($proc.ExitCode)），诊断见 build\smoke.log"
    }
}

Write-Host "== 完成" -ForegroundColor Green
