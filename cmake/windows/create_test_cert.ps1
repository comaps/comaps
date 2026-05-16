# Creates a self-signed code-signing certificate for local MSIX testing.
# The subject CN must match Identity/@Publisher in cmake/windows/AppxManifest.xml.
#
# Usage:
#   .\create_test_cert.ps1 [-Subject "CN=CoMaps"] [-OutDir .]
#
# Outputs:
#   CoMaps-test.pfx  — private key + cert (no password; for CI use only)
#   CoMaps-test.cer  — public cert to import into Trusted People store once

param(
    [string]$Subject = "CN=CoMaps Test",
    [string]$OutDir  = "."
)

$cert = New-SelfSignedCertificate `
    -Subject $Subject `
    -KeyAlgorithm RSA `
    -KeyLength 2048 `
    -CertStoreLocation "Cert:\CurrentUser\My" `
    -Type CodeSigningCert `
    -NotAfter (Get-Date).AddYears(1)

$pfx = Join-Path $OutDir "CoMaps-test.pfx"
$cer = Join-Path $OutDir "CoMaps-test.cer"

$pfxPw = ConvertTo-SecureString -String "comaps-test" -Force -AsPlainText
Export-PfxCertificate -Cert $cert -FilePath $pfx -Password $pfxPw | Out-Null

Export-Certificate -Cert $cert -FilePath $cer -Type CERT | Out-Null

Write-Host "Created:"
Write-Host "  $pfx  (test-only private key - safe to commit for CI/local testing)"
Write-Host "  $cer  (public cert - run as admin to trust:)"
Write-Host ""
Write-Host "  certutil -addstore TrustedPeople `"$cer`""
Write-Host ""
Write-Host "Thumbprint: $($cert.Thumbprint)"
