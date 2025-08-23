#!/bin/bash

set -e

if [ -z $BBPATH ]; then
  printf "Please call from within a set-up bitbake environment!\nRun 'source oe-init-build-env <builddir>' first\n"
  exit 1
fi

# Use fixed CA from meta-nuc layer instead of generating new ones
FIXED_CA_DIR="$BBPATH/../meta-nuc/recipes-core/rauc/files/ca-fixed"
BASE="$BBPATH/example-ca"

if [ -e $BASE ]; then
# Always use fixed CA - remove existing if present
if [ -e $BASE ]; then
  echo "🔄 Removing existing CA to use fixed CA..."
  rm -rf $BASE
fi

# Check if fixed CA files exist
if [ ! -f "$FIXED_CA_DIR/ca.cert.pem" ]; then
  echo "❌ Error: Fixed CA files not found in $FIXED_CA_DIR"
  echo "Please ensure bundler CA files are copied to meta-nuc layer"
  exit 1
fi

echo "🔑 Using fixed CA from meta-nuc layer instead of generating new ones..."

mkdir -p $BASE/{private,certs}

# Copy fixed CA files instead of generating
echo "📋 Copying fixed CA certificates..."
cp "$FIXED_CA_DIR/ca.cert.pem" "$BASE/"
cp "$FIXED_CA_DIR/development-1.cert.pem" "$BASE/"
cp "$FIXED_CA_DIR/ca.key.pem" "$BASE/private/"
cp "$FIXED_CA_DIR/development-1.key.pem" "$BASE/private/"

# Create required CA database files for OpenSSL
touch $BASE/index.txt
echo 01 > $BASE/serial

# Fixed CA files are already copied, no need to generate
echo "✅ Fixed CA certificates copied successfully"


CONFFILE=${BUILDDIR}/conf/site.conf

echo ""
echo "Writing RAUC key configuration to site.conf ..."

if test -f $CONFFILE; then
if grep -q "^RAUC_KEYRING_FILE.*=" $CONFFILE; then
	echo "RAUC_KEYRING_FILE already configured, aborting key configuration"
	exit 0
fi
if grep -q "^RAUC_KEY_FILE.*=" $CONFFILE; then
	echo "RAUC_KEY_FILE already configured, aborting key configuration"
	exit 0
fi
if grep -q "^RAUC_CERT_FILE.*=" $CONFFILE; then
	echo "RAUC_CERT_FILE already configured, aborting key configuration"
	exit 0
fi
fi

echo "RAUC_KEYRING_FILE=\"${BUILDDIR}/example-ca/ca.cert.pem\"" >> $CONFFILE
echo "RAUC_KEY_FILE=\"${BUILDDIR}/example-ca/private/development-1.key.pem\"" >> $CONFFILE
echo "RAUC_CERT_FILE=\"${BUILDDIR}/example-ca/development-1.cert.pem\"" >> $CONFFILE

echo "Key configuration successfully written to ${BUILDDIR}/conf/site.conf" 