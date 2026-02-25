Write-Host "Building tests..."
cmake --build C:\Users\jwitz\Dev\screenqt\build --target pageview_tests --config Debug 2>&1 | Select-Object -Last 10

Write-Host "`n`nRunning tests..."
Set-Location C:\Users\jwitz\Dev\screenqt\build
ctest -C Debug --output-on-failure --timeout 30 2>&1 | Select-Object -Last 50

Write-Host "`nTest run complete"
