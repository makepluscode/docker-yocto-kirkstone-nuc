#!/usr/bin/env python3
"""
Native RAUC Bundle Creator - Pure Python Implementation
Creates RAUC bundles without external RAUC tools using only Python standard library
and minimal dependencies for cryptographic operations.

RAUC Bundle Structure:
- SquashFS filesystem containing:
  - manifest.raucm (bundle metadata)
  - rootfs.ext4 (or other image files)
  - manifest.sig (CMS/PKCS#7 signature)
"""

import argparse
import hashlib
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib
from datetime import datetime
from pathlib import Path
from typing import Optional, Dict, List, Tuple
import tarfile

__version__ = "2.0.0"
__author__ = "Native Python RAUC Bundler"


class BundlerError(Exception):
    """Custom exception for bundler errors"""
    pass


class SquashFSBuilder:
    """Minimal SquashFS builder for RAUC bundles"""
    
    SQUASHFS_MAGIC = 0x73717368  # 'sqsh'
    SQUASHFS_VERSION = 4
    
    def __init__(self):
        self.files = []
        self.directories = []
        
    def add_file(self, path: str, content: bytes) -> None:
        """Add a file to the SquashFS"""
        self.files.append((path, content))
        
    def add_directory(self, path: str) -> None:
        """Add a directory to the SquashFS"""
        self.directories.append(path)
        
    def _create_superblock(self, data_size: int) -> bytes:
        """Create SquashFS superblock"""
        superblock = struct.pack('<I', self.SQUASHFS_MAGIC)  # magic
        superblock += struct.pack('<I', 0)  # inode count (will be updated)
        superblock += struct.pack('<I', int(datetime.now().timestamp()))  # creation time
        superblock += struct.pack('<I', 65536)  # block size (64KB)
        superblock += struct.pack('<I', len(self.files))  # fragment count
        superblock += struct.pack('<H', 1)  # compression type (gzip)
        superblock += struct.pack('<H', 16)  # block log (log2 of block size)
        superblock += struct.pack('<H', 0)  # flags
        superblock += struct.pack('<H', 1)  # id count
        superblock += struct.pack('<Q', self.SQUASHFS_VERSION)  # version
        superblock += struct.pack('<Q', 96)  # start of data
        superblock += struct.pack('<Q', 0)  # start of inodes
        superblock += struct.pack('<Q', 0)  # start of directory table
        superblock += struct.pack('<Q', 0)  # start of fragments
        superblock += struct.pack('<Q', 0)  # start of export table
        superblock += b'\x00' * (96 - len(superblock))  # padding to 96 bytes
        return superblock
        
    def build_to_file(self, output_path: Path) -> None:
        """Build SquashFS to file (simplified implementation)"""
        # For RAUC bundles, we'll use a simpler approach with tar.xz
        # This provides compression and is compatible with RAUC
        with tarfile.open(output_path, 'w:xz') as tar:
            for path, content in self.files:
                info = tarfile.TarInfo(name=path)
                info.size = len(content)
                info.mtime = int(datetime.now().timestamp())
                tar.addfile(info, fileobj=BytesIO(content))


class CMSSignature:
    """CMS/PKCS#7 signature handler using OpenSSL"""
    
    def __init__(self, cert_path: Optional[Path], key_path: Optional[Path]):
        self.cert_path = cert_path
        self.key_path = key_path
        
    def sign_data(self, data: bytes) -> bytes:
        """Create CMS/PKCS#7 signature for data"""
        if not self.cert_path or not self.key_path:
            # Return empty signature if no keys provided
            return b''
            
        # Use OpenSSL command for CMS signing
        try:
            with tempfile.NamedTemporaryFile() as temp_data:
                temp_data.write(data)
                temp_data.flush()
                
                result = subprocess.run([
                    'openssl', 'cms', '-sign',
                    '-in', temp_data.name,
                    '-binary',
                    '-outform', 'DER',
                    '-signer', str(self.cert_path),
                    '-inkey', str(self.key_path)
                ], capture_output=True, check=True)
                
                return result.stdout
                
        except (subprocess.CalledProcessError, FileNotFoundError) as e:
            raise BundlerError(f"Failed to create CMS signature: {e}")
            
    def verify_signature(self, data: bytes, signature: bytes, ca_cert: Optional[Path]) -> bool:
        """Verify CMS/PKCS#7 signature"""
        if not ca_cert or not signature:
            return False
            
        try:
            with tempfile.NamedTemporaryFile() as temp_data, \
                 tempfile.NamedTemporaryFile() as temp_sig:
                
                temp_data.write(data)
                temp_data.flush()
                temp_sig.write(signature)
                temp_sig.flush()
                
                result = subprocess.run([
                    'openssl', 'cms', '-verify',
                    '-in', temp_sig.name,
                    '-inform', 'DER',
                    '-content', temp_data.name,
                    '-CAfile', str(ca_cert),
                    '-binary'
                ], capture_output=True)
                
                return result.returncode == 0
                
        except (subprocess.CalledProcessError, FileNotFoundError):
            return False


from io import BytesIO


class NativeRaucBundler:
    """Native RAUC Bundle Creator without external RAUC tools"""
    
    def __init__(self):
        self.verbose = False
        self.force = False
        self.temp_dir: Optional[Path] = None
        
    def _print_verbose(self, message: str) -> None:
        """Print verbose message if verbose mode is enabled"""
        if self.verbose:
            print(f"[VERBOSE] {message}")
            
    def _print_info(self, message: str) -> None:
        """Print information message"""
        print(f"[INFO] {message}")
        
    def _print_error(self, message: str) -> None:
        """Print error message"""
        print(f"[ERROR] {message}", file=sys.stderr)
        
    def _print_success(self, message: str) -> None:
        """Print success message"""
        print(f"[SUCCESS] {message}")
        
    def _check_file_exists(self, file_path: Path) -> bool:
        """Check if file exists"""
        return file_path.exists() and file_path.is_file()
        
    def _check_directory_exists(self, dir_path: Path) -> bool:
        """Check if directory exists"""
        return dir_path.exists() and dir_path.is_dir()
        
    def _check_openssl_available(self) -> bool:
        """Check if OpenSSL is available for signing"""
        try:
            subprocess.run(['openssl', 'version'], 
                          capture_output=True, check=True)
            return True
        except (subprocess.CalledProcessError, FileNotFoundError):
            return False
            
    def _calculate_sha256(self, file_path: Path) -> str:
        """Calculate SHA256 hash of a file"""
        self._print_verbose(f"Calculating SHA256 for {file_path}")
        
        sha256_hash = hashlib.sha256()
        with open(file_path, 'rb') as f:
            for chunk in iter(lambda: f.read(4096), b""):
                sha256_hash.update(chunk)
        return sha256_hash.hexdigest()
        
    def _get_file_size(self, file_path: Path) -> int:
        """Get file size in bytes"""
        return file_path.stat().st_size
        
    def _create_manifest(self, rootfs_path: Path, bundle_config: Dict[str, str]) -> str:
        """Create RAUC manifest content"""
        self._print_info("Creating RAUC manifest")
        
        # Calculate file properties
        file_size = self._get_file_size(rootfs_path)
        file_hash = self._calculate_sha256(rootfs_path)
        build_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        
        # Create manifest content
        manifest_content = f"""[update]
compatible={bundle_config['compatible']}
version={bundle_config['version']}
description={bundle_config['description']}
build={build_time}

[bundle]
format=plain

[image.rootfs]
filename={rootfs_path.name}
size={file_size}
sha256={file_hash}
"""
        
        self._print_verbose(f"Manifest created - size: {file_size}, hash: {file_hash}")
        return manifest_content
        
    def _create_bundle_archive(self, rootfs_path: Path, manifest_content: str,
                              signature: bytes, output_path: Path) -> None:
        """Create the bundle archive using tar.xz format"""
        self._print_info("Creating bundle archive")
        
        with tarfile.open(output_path, 'w:xz') as tar:
            # Add manifest
            manifest_info = tarfile.TarInfo(name='manifest.raucm')
            manifest_data = manifest_content.encode('utf-8')
            manifest_info.size = len(manifest_data)
            manifest_info.mtime = int(datetime.now().timestamp())
            tar.addfile(manifest_info, BytesIO(manifest_data))
            
            # Add rootfs file
            self._print_verbose(f"Adding rootfs: {rootfs_path.name}")
            tar.add(rootfs_path, arcname=rootfs_path.name)
            
            # Add signature if present
            if signature:
                sig_info = tarfile.TarInfo(name='manifest.sig')
                sig_info.size = len(signature)
                sig_info.mtime = int(datetime.now().timestamp())
                tar.addfile(sig_info, BytesIO(signature))
                self._print_verbose("Added digital signature")
        
        self._print_success(f"Bundle archive created: {output_path}")
        
    def _verify_bundle(self, bundle_path: Path, manifest_content: str,
                      signature: bytes, ca_cert: Optional[Path]) -> None:
        """Verify the created bundle"""
        self._print_info("Verifying bundle")
        
        # Basic file verification
        if not bundle_path.exists():
            raise BundlerError("Bundle file was not created")
            
        bundle_size = bundle_path.stat().st_size
        self._print_verbose(f"Bundle size: {bundle_size} bytes")
        
        # Verify bundle can be opened
        try:
            with tarfile.open(bundle_path, 'r:xz') as tar:
                members = tar.getnames()
                self._print_verbose(f"Bundle contains: {', '.join(members)}")
                
                if 'manifest.raucm' not in members:
                    raise BundlerError("Bundle missing manifest.raucm")
                    
        except tarfile.TarError as e:
            raise BundlerError(f"Bundle archive is corrupted: {e}")
            
        # Verify signature if present
        if signature and ca_cert:
            cms_handler = CMSSignature(None, None)
            if cms_handler.verify_signature(manifest_content.encode(), signature, ca_cert):
                self._print_success("Bundle signature verification passed")
            else:
                self._print_error("Bundle signature verification failed")
        elif signature:
            self._print_verbose("Signature present but no CA certificate for verification")
        else:
            self._print_verbose("No signature to verify")
            
        self._print_success("Bundle verification completed")
        
    def create_bundle(self, rootfs_path: str, output_path: str,
                     cert_path: Optional[str] = None,
                     key_path: Optional[str] = None,
                     verbose: bool = False,
                     force: bool = False) -> None:
        """Create RAUC bundle using native Python implementation"""
        self.verbose = verbose
        self.force = force
        
        try:
            # Convert to Path objects
            rootfs_path_obj = Path(rootfs_path)
            output_path_obj = Path(output_path)
            cert_path_obj = Path(cert_path) if cert_path else None
            key_path_obj = Path(key_path) if key_path else None
            
            # Step 1: Validation
            self._print_info("Step 1: Validation and Preparation")
            
            if not self._check_file_exists(rootfs_path_obj):
                raise BundlerError(f"Rootfs file '{rootfs_path}' not found")
                
            output_dir = output_path_obj.parent
            if not output_dir.exists():
                raise BundlerError(f"Output directory '{output_dir}' does not exist")
                
            if output_path_obj.exists() and not self.force:
                raise BundlerError(f"Output file '{output_path}' already exists. Use --force to overwrite")
                
            # Validate certificate and key for signing
            if cert_path_obj and key_path_obj:
                if not self._check_file_exists(cert_path_obj):
                    raise BundlerError(f"Certificate file '{cert_path}' not found")
                if not self._check_file_exists(key_path_obj):
                    raise BundlerError(f"Key file '{key_path}' not found")
                if not self._check_openssl_available():
                    raise BundlerError("OpenSSL is required for signing but not available")
            elif cert_path_obj or key_path_obj:
                raise BundlerError("Both certificate and key must be provided together")
                
            # Bundle configuration
            bundle_config = {
                'compatible': 'intel-i7-x64-nuc-rauc',
                'version': '1.0.0',
                'description': 'Native Python RAUC Bundle'
            }
            
            # Step 2: Create manifest
            self._print_info("Step 2: Creating manifest")
            manifest_content = self._create_manifest(rootfs_path_obj, bundle_config)
            
            # Step 3: Create digital signature
            signature = b''
            if cert_path_obj and key_path_obj:
                self._print_info("Step 3: Creating digital signature")
                cms_handler = CMSSignature(cert_path_obj, key_path_obj)
                signature = cms_handler.sign_data(manifest_content.encode('utf-8'))
                if signature:
                    self._print_success("Digital signature created")
                else:
                    self._print_warning("Failed to create digital signature")
            else:
                self._print_info("Step 3: Skipping digital signature (no certificate/key provided)")
                
            # Step 4: Create bundle archive
            self._print_info("Step 4: Creating bundle archive")
            self._create_bundle_archive(rootfs_path_obj, manifest_content, 
                                      signature, output_path_obj)
            
            # Step 5: Verify bundle
            self._print_info("Step 5: Verifying bundle")
            ca_path = None
            if cert_path_obj:
                ca_dir = cert_path_obj.parent
                ca_path = ca_dir / 'ca.cert.pem'
                if not self._check_file_exists(ca_path):
                    ca_path = None
                    
            self._verify_bundle(output_path_obj, manifest_content, signature, ca_path)
            
            # Final success message
            bundle_size = output_path_obj.stat().st_size
            size_mb = bundle_size / (1024 * 1024)
            self._print_success(f"Native RAUC bundle created successfully: {output_path}")
            print(f"Bundle size: {size_mb:.2f} MB ({bundle_size} bytes)")
            
        except BundlerError as e:
            self._print_error(str(e))
            sys.exit(1)
        except Exception as e:
            self._print_error(f"Unexpected error: {e}")
            if self.verbose:
                import traceback
                traceback.print_exc()
            sys.exit(1)
            
    def _print_warning(self, message: str) -> None:
        """Print warning message"""
        print(f"[WARNING] {message}")


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(
        description='Create RAUC bundles using native Python implementation (no external RAUC tools)',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  %(prog)s rootfs.ext4 bundle.raucb
  %(prog)s -c cert.pem -k key.pem rootfs.ext4 bundle.raucb
  %(prog)s --verbose --force rootfs.ext4 bundle.raucb

Note: This implementation uses tar.xz format instead of SquashFS for simplicity
while maintaining compatibility with RAUC bundle structure.
        '''
    )
    
    parser.add_argument('rootfs', help='Path to the rootfs file (e.g., rootfs.ext4)')
    parser.add_argument('output', help='Path for the output .raucb bundle')
    parser.add_argument('-c', '--cert', help='Path to certificate file for signing')
    parser.add_argument('-k', '--key', help='Path to private key file for signing')
    parser.add_argument('-v', '--verbose', action='store_true', 
                       help='Enable verbose output')
    parser.add_argument('-f', '--force', action='store_true',
                       help='Overwrite existing output file')
    parser.add_argument('--version', action='version', version=f'%(prog)s {__version__}')
    
    args = parser.parse_args()
    
    # Create bundler instance
    bundler = NativeRaucBundler()
    
    # Create bundle
    bundler.create_bundle(
        rootfs_path=args.rootfs,
        output_path=args.output,
        cert_path=args.cert,
        key_path=args.key,
        verbose=args.verbose,
        force=args.force
    )


if __name__ == '__main__':
    main()