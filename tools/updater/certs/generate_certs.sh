#!/bin/bash

# Generate SSL certificates for Updater Server
# This script creates self-signed certificates for development/testing

CERT_DIR="certs"
DAYS=365
COUNTRY="US"
STATE="State"
CITY="City"
ORG="Updater Server"
OU="IT Department"
CN="localhost"

echo "=== Generating SSL Certificates for Updater Server ==="
echo "Certificate directory: $CERT_DIR"
echo "Common Name: $CN"
echo "Valid for: $DAYS days"
echo ""

# Create certificate directory
mkdir -p $CERT_DIR

# Generate CA private key
echo "1. Generating CA private key..."
openssl genrsa -out $CERT_DIR/ca.key 4096

# Generate CA certificate
echo "2. Generating CA certificate..."
openssl req -new -x509 -days $DAYS -key $CERT_DIR/ca.key -out $CERT_DIR/ca.crt -subj "/C=$COUNTRY/ST=$STATE/L=$CITY/O=$ORG/OU=$OU/CN=Updater-CA"

# Generate server private key
echo "3. Generating server private key..."
openssl genrsa -out $CERT_DIR/server.key 4096

# Generate server certificate signing request
echo "4. Generating server certificate signing request..."
openssl req -new -key $CERT_DIR/server.key -out $CERT_DIR/server.csr -subj "/C=$COUNTRY/ST=$STATE/L=$CITY/O=$ORG/OU=$OU/CN=$CN"

# Create extensions file for SAN (Subject Alternative Names)
echo "5. Creating certificate extensions..."
cat > $CERT_DIR/server.ext << EOF
authorityKeyIdentifier=keyid,issuer
basicConstraints=CA:FALSE
keyUsage = digitalSignature, nonRepudiation, keyEncipherment, dataEncipherment
subjectAltName = @alt_names

[alt_names]
DNS.1 = localhost
DNS.2 = *.localhost
DNS.3 = updater-server
DNS.4 = *.updater-server
IP.1 = 127.0.0.1
IP.2 = 192.168.1.101
IP.3 = ::1
EOF

# Generate server certificate signed by CA
echo "6. Generating server certificate..."
openssl x509 -req -in $CERT_DIR/server.csr -CA $CERT_DIR/ca.crt -CAkey $CERT_DIR/ca.key -CAcreateserial -out $CERT_DIR/server.crt -days $DAYS -extensions v3_req -extfile $CERT_DIR/server.ext

# Set appropriate permissions
echo "7. Setting certificate permissions..."
chmod 600 $CERT_DIR/*.key
chmod 644 $CERT_DIR/*.crt
chmod 644 $CERT_DIR/*.csr 2>/dev/null || true

# Clean up temporary files
rm -f $CERT_DIR/server.csr $CERT_DIR/server.ext

echo ""
echo "✅ SSL certificates generated successfully!"
echo ""
echo "Files created:"
echo "- $CERT_DIR/ca.crt (Certificate Authority)"
echo "- $CERT_DIR/ca.key (CA Private Key)"
echo "- $CERT_DIR/server.crt (Server Certificate)"
echo "- $CERT_DIR/server.key (Server Private Key)"
echo ""
echo "To enable HTTPS, set environment variable:"
echo "export UPDATER_ENABLE_HTTPS=true"
echo ""
echo "⚠️  Note: These are self-signed certificates for development only."
echo "   For production, use certificates from a trusted CA."
echo ""
echo "📋 Certificate Information:"
openssl x509 -in $CERT_DIR/server.crt -noout -text | grep -A 10 "Subject Alternative Name"