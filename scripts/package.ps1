[CmdletBinding()]
param(
    [string]$BuildDir = "build",
    [string]$DistRoot = "dist",
    [string]$QtBin = "",
    [switch]$SkipTests
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$buildPath = Join-Path $root $BuildDir
$distPath = Join-Path $root $DistRoot

if (-not $QtBin) {
    $windeployCommand = Get-Command windeployqt.exe -ErrorAction SilentlyContinue
    if ($windeployCommand) {
        $QtBin = Split-Path $windeployCommand.Source
    }
}
if ($QtBin) {
    $env:PATH = "$QtBin;$env:PATH"
}

function Copy-MingwRuntimeDependencies {
    param(
        [string]$Directory,
        [string]$QtBinDirectory
    )

    $objdump = Join-Path $QtBinDirectory "objdump.exe"
    if (-not (Test-Path $objdump)) {
        Write-Host "未找到 objdump，跳过 MinGW 运行库依赖复制（MSVC 构建通常不需要）。"
        return
    }

    $queue = [System.Collections.Generic.Queue[string]]::new()
    $files = @{}
    Get-ChildItem -Path $Directory -Recurse -File -Include *.exe, *.dll | ForEach-Object {
        $files[$_.Name] = $_.FullName
        $queue.Enqueue($_.FullName)
    }

    $seen = @{}
    $copied = 0
    while ($queue.Count -gt 0) {
        $file = $queue.Dequeue()
        if ($seen.ContainsKey($file)) {
            continue
        }
        $seen[$file] = $true

        $dependencies = & $objdump -p $file 2>$null |
            Select-String 'DLL Name:\s*(.+)$' |
            ForEach-Object { $_.Matches[0].Groups[1].Value.Trim() }

        foreach ($dependency in $dependencies) {
            if ($files.ContainsKey($dependency)) {
                $queue.Enqueue($files[$dependency])
                continue
            }

            $candidate = Join-Path $QtBinDirectory $dependency
            if (Test-Path $candidate) {
                $target = Join-Path $Directory $dependency
                Copy-Item -LiteralPath $candidate -Destination $target
                $files[$dependency] = $target
                $queue.Enqueue($target)
                ++$copied
            }
        }
    }

    Write-Host "已补充 $copied 个 MinGW/第三方依赖 DLL。"
}

$cache = Join-Path $buildPath "CMakeCache.txt"
if (-not (Test-Path $cache)) {
    throw "找不到 $cache，请先运行 .\dev.ps1 或手动 cmake 配置。"
}

$version = (Select-String -Path $cache -Pattern '^CMAKE_PROJECT_VERSION:STATIC=(.+)$').Matches[0].Groups[1].Value.Trim()
if (-not $version) {
    throw "无法从 CMakeCache.txt 读出项目版本号。"
}
Write-Host "== 版本 $version"

cmake --build $buildPath
if ($LASTEXITCODE -ne 0) {
    throw "构建失败。"
}

if (-not $SkipTests) {
    ctest --test-dir $buildPath --output-on-failure
    if ($LASTEXITCODE -ne 0) {
        throw "测试未通过，已停止打包。"
    }
}

$stageName = "algorithm-practice-notebook-$version-windows-x64"
$stage = Join-Path $distPath $stageName
if (Test-Path $stage) {
    Remove-Item -LiteralPath $stage -Recurse -Force
}
New-Item -ItemType Directory -Path $stage -Force | Out-Null

cmake --install $buildPath --prefix $stage --config Release
if ($LASTEXITCODE -ne 0) {
    throw "cmake --install 失败。"
}

$exe = Join-Path $stage "task_notebook_gui.exe"
if (-not (Test-Path $exe)) {
    throw "安装目录中缺少 task_notebook_gui.exe。"
}

windeployqt --release --no-translations --no-opengl-sw --compiler-runtime $exe
if ($LASTEXITCODE -ne 0) {
    throw "windeployqt 失败，请确认 Qt 的 bin 目录在 PATH 中。"
}

if ($QtBin) {
    Copy-MingwRuntimeDependencies -Directory $stage -QtBinDirectory $QtBin
}

# 便携版必须带说明和许可证，用户解压后能直接看到用法。
foreach ($document in @("README.md", "LICENSE", "CHANGELOG.md")) {
    $source = Join-Path $root $document
    if (Test-Path $source) {
        Copy-Item -LiteralPath $source -Destination (Join-Path $stage $document)
    }
}

# Qt 是 LGPLv3 动态链接，发布包里要带上许可证原文。
if ($QtBin) {
    $licenseSource = Join-Path (Split-Path $QtBin -Parent) "share\licenses\qt6-base"
    if (Test-Path $licenseSource) {
        $licenseTarget = Join-Path $stage "licenses"
        New-Item -ItemType Directory -Path $licenseTarget -Force | Out-Null
        Copy-Item -Path (Join-Path $licenseSource "*.txt") -Destination $licenseTarget
        Write-Host "已附带 Qt 许可证文本。"
    } else {
        Write-Warning "未找到 Qt 许可证文本（$licenseSource），发布前请手动补上。"
    }
}

# README 里引用了 docs 下的截图，一并带上，解压后文档不会出现坏图。
$docsSource = Join-Path $root "docs"
if (Test-Path $docsSource) {
    $docsTarget = Join-Path $stage "docs"
    New-Item -ItemType Directory -Path $docsTarget -Force | Out-Null
    Copy-Item -Path (Join-Path $docsSource "screenshot*.png") -Destination $docsTarget
}

$zip = Join-Path $distPath "$stageName.zip"
if (Test-Path $zip) {
    Remove-Item -LiteralPath $zip -Force
}
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zip -CompressionLevel Optimal

$files = Get-ChildItem -Path $stage -Recurse -File
$sizeMb = [math]::Round((($files | Measure-Object -Property Length -Sum).Sum / 1MB), 1)
$hash = (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash

Write-Host "== 完成"
Write-Host "目录：$stage（$($files.Count) 个文件，$sizeMb MB）"
Write-Host "压缩包：$zip"
Write-Host "SHA256：$hash"
Write-Host "目标机器无需安装 Qt 即可运行：解压后双击 task_notebook_gui.exe。"
# objdump 等外部命令可能留下非零的 LASTEXITCODE，显式收尾避免脚本被判失败。
exit 0
