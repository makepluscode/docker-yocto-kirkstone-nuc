#!/bin/bash

# RAUC Example Key Generation Script
# Based on meta-nuc/create-example-keys.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KEYS_DIR="${SCRIPT_DIR}/example-ca"

echo "Creating RAUC example keys in ${KEYS_DIR}"

# Create directory structure
mkdir -p "${KEYS_DIR}/private"
mkdir -p "${KEYS_DIR}/newcerts"

# Create CA configuration
cat > "${KEYS_DIR}/openssl-ca.cnf" << EOF
[ ca ]
default_ca = CA_default

[ CA_default ]
dir               = ${KEYS_DIR}
certs             = \$dir
new_certs_dir     = \$dir/newcerts
database          = \$dir/index.txt
serial            = \$dir/serial
RANDFILE          = \$dir/private/.rand

private_key       = \$dir/private/ca.key.pem
certificate       = \$dir/ca.cert.pem

default_days      = 365
default_crl_days  = 30
default_md        = sha256
name_opt         = ca_default
cert_opt         = ca_default
new_certs_dir    = \$dir/newcerts
database         = \$dir/index.txt
serial           = \$dir/serial
RANDFILE         = \$dir/private/.rand

policy            = policy_strict

x509_extensions = ca_extensions

[ policy_strict ]
countryName             = match
stateOrProvinceName     = match
organizationName        = match
organizationalUnitName  = optional
commonName              = supplied
emailAddress            = optional

[ ca_extensions ]
subjectKeyIdentifier=hash
authorityKeyIdentifier=keyid:always,issuer
basicConstraints = CA:true

[ req ]
default_bits        = 2048
distinguished_name  = req_distinguished_name
string_mask        = utf8only
default_md         = sha256
x509_extensions    = v3_ca

[ req_distinguished_name ]
countryName                     = Country Name (2 letter code)
stateOrProvinceName            = State or Province Name
localityName                   = Locality Name
0.organizationName             = Organization Name
organizationalUnitName         = Organizational Unit Name
commonName                     = Common Name

[ v3_ca ]
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid:always,issuer
basicConstraints = critical, CA:true
keyUsage = critical, digitalSignature, cRLSign, keyCertSign

[ v3_leaf ]
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid:always,issuer
basicConstraints = critical, CA:false
keyUsage = critical, digitalSignature
EOF

# Initialize CA database
touch "${KEYS_DIR}/index.txt"
echo "01" > "${KEYS_DIR}/serial"

# Generate CA private key
echo "Generating CA private key..."
openssl genrsa -out "${KEYS_DIR}/private/ca.key.pem" 2048

# Generate CA certificate
echo "Generating CA certificate..."
openssl req -new -x509 -days 3650 -key "${KEYS_DIR}/private/ca.key.pem" \
    -out "${KEYS_DIR}/ca.cert.pem" \
    -subj "/C=US/ST=State/L=City/O=Example Org/CN=Example Org RAUC CA Development"

# Generate development signing key
echo "Generating development signing key..."
openssl genrsa -out "${KEYS_DIR}/private/development-1.key.pem" 2048

# Generate development certificate signing request
echo "Generating development certificate signing request..."
openssl req -new -key "${KEYS_DIR}/private/development-1.key.pem" \
    -out "${KEYS_DIR}/development-1.csr.pem" \
    -subj "/C=US/ST=State/L=City/O=Example Org/CN=Example Org Development-1"

# Sign development certificate
echo "Signing development certificate..."
openssl ca -batch -config "${KEYS_DIR}/openssl-ca.cnf" \
    -extensions v3_leaf \
    -in "${KEYS_DIR}/development-1.csr.pem" \
    -out "${KEYS_DIR}/development-1.cert.pem"

# Set proper permissions
chmod 600 "${KEYS_DIR}/private/"*.pem
chmod 644 "${KEYS_DIR}/"*.cert.pem

# Create site.conf template
cat > "${KEYS_DIR}/site.conf.template" << EOF
# RAUC Key Configuration Template
# Copy this to your build/conf/site.conf or local.conf

# RAUC keyring file (CA certificate)
RAUC_KEYRING_FILE = "\${TOPDIR}/example-ca/ca.cert.pem"

# RAUC signing key and certificate
RAUC_KEY_FILE = "\${TOPDIR}/example-ca/private/development-1.key.pem"
RAUC_CERT_FILE = "\${TOPDIR}/example-ca/development-1.cert.pem"

# Enable RAUC features
DISTRO_FEATURES += "rauc"
EOF

echo ""
echo "✅ RAUC example keys created successfully!"
echo ""
echo "📁 Key files created in: ${KEYS_DIR}"
echo "   - CA certificate: ${KEYS_DIR}/ca.cert.pem"
echo "   - CA private key: ${KEYS_DIR}/private/ca.key.pem"
echo "   - Development certificate: ${KEYS_DIR}/development-1.cert.pem"
echo "   - Development private key: ${KEYS_DIR}/private/development-1.key.pem"
echo ""
echo "📋 Configuration template: ${KEYS_DIR}/site.conf.template"
echo ""
echo "🔧 To use these keys in your Yocto build:"
echo "   1. Copy the site.conf.template content to your build/conf/site.conf"
echo "   2. Update the TOPDIR path to match your build directory"
echo "   3. Build your RAUC bundle recipe"
echo ""
echo "⚠️  WARNING: These are example keys for development only!"
echo "   Do not use these keys in production systems." 