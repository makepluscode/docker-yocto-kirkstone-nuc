#!/usr/bin/env python3
"""
RAUC Bundle Creator - Python Implementation
A Python-based tool for creating RAUC bundles with the same functionality as the C bundler.

This tool implements the 5-step RAUC bundle creation process:
1. Preparation: Validate manifest and certificate files
2. Manifest Generation: Create bundle metadata with image hash information
3. Bundle Directory Setup: Arrange rootfs and manifest in bundle directory
4. Signing and Bundle Creation: Create signed .raucb file using RAUC command
5. Verification: Validate bundle integrity and signature
"""

import argparse
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile
from datetime import datetime
from pathlib import Path
from typing import Optional, Tuple

__version__ = "1.0.0"
__author__ = "Python RAUC Bundler"


class BundlerError(Exception):
    """Custom exception for bundler errors"""
    pass


class RaucBundler:
    """RAUC Bundle Creator"""
    
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
        
    def _check_rauc_available(self) -> bool:
        """Check if RAUC command is available"""
        try:
            subprocess.run(['rauc', '--version'], 
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
        
    def _create_manifest(self, manifest_path: Path, rootfs_path: Path, 
                        bundle_config: dict) -> None:
        """Create RAUC manifest file"""
        self._print_info(f"Creating manifest: {manifest_path}")
        
        # Calculate file properties
        file_size = self._get_file_size(rootfs_path)
        file_hash = self._calculate_sha256(rootfs_path)
        build_time = datetime.now().strftime('%Y%m%d%H%M%S')
        
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
        
        # Write manifest file
        with open(manifest_path, 'w') as f:
            f.write(manifest_content)
            
        self._print_verbose(f"Manifest created with file size: {file_size}, hash: {file_hash}")
        
    def _prepare_bundle_directory(self, rootfs_path: Path, 
                                 bundle_config: dict) -> Path:
        """Step 3: Prepare bundle directory structure"""
        self._print_info("Preparing bundle directory structure")
        
        # Create temporary bundle directory
        self.temp_dir = Path(tempfile.mkdtemp(prefix='rauc_bundle_'))
        self._print_verbose(f"Created temporary directory: {self.temp_dir}")
        
        # Copy rootfs to bundle directory
        bundle_rootfs = self.temp_dir / rootfs_path.name
        self._print_verbose(f"Copying {rootfs_path} to {bundle_rootfs}")
        shutil.copy2(rootfs_path, bundle_rootfs)
        
        # Create manifest
        manifest_path = self.temp_dir / 'manifest.raucm'
        self._create_manifest(manifest_path, bundle_rootfs, bundle_config)
        
        return self.temp_dir
        
    def _execute_rauc_bundle(self, bundle_dir: Path, output_path: Path,
                           cert_path: Optional[Path] = None,
                           key_path: Optional[Path] = None) -> None:
        """Step 4: Execute RAUC bundle command"""
        self._print_info(f"Creating RAUC bundle: {output_path}")
        
        # Build rauc command
        cmd = ['rauc', 'bundle']
        
        # Add certificate and key if provided
        if cert_path and key_path:
            cmd.extend([f'--cert={cert_path}', f'--key={key_path}'])
            
        # Add bundle directory and output path
        cmd.extend([str(bundle_dir), str(output_path)])
        
        self._print_verbose(f"Executing: {' '.join(cmd)}")
        
        # Execute command
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, check=True)
            if self.verbose and result.stdout:
                print(result.stdout)
        except subprocess.CalledProcessError as e:
            self._print_error(f"RAUC bundle creation failed: {e}")
            if e.stderr:
                self._print_error(f"STDERR: {e.stderr}")
            raise BundlerError(f"Bundle creation failed with exit code {e.returncode}")
            
    def _verify_bundle(self, bundle_path: Path, 
                      keyring_path: Optional[Path] = None) -> None:
        """Step 5: Verify created bundle"""
        self._print_info(f"Verifying bundle: {bundle_path}")
        
        # Show bundle info
        try:
            result = subprocess.run(['rauc', 'info', str(bundle_path)],
                                  capture_output=True, text=True, check=True)
            if self.verbose:
                print("=== Bundle Information ===")
                print(result.stdout)
        except subprocess.CalledProcessError as e:
            self._print_error(f"Failed to get bundle info: {e}")
            
        # Verify bundle signature if keyring is provided
        if keyring_path and self._check_file_exists(keyring_path):
            try:
                result = subprocess.run(['rauc', 'verify', 
                                       f'--keyring={keyring_path}', 
                                       str(bundle_path)],
                                      capture_output=True, text=True, check=True)
                self._print_success("Bundle signature verification passed")
                if self.verbose and result.stdout:
                    print(result.stdout)
            except subprocess.CalledProcessError as e:
                self._print_error(f"Bundle verification failed: {e}")
                if e.stderr:
                    self._print_error(f"STDERR: {e.stderr}")
        else:
            self._print_verbose("No keyring provided, skipping signature verification")
            
    def _cleanup(self) -> None:
        """Clean up temporary files"""
        if self.temp_dir and self.temp_dir.exists():
            self._print_verbose(f"Cleaning up temporary directory: {self.temp_dir}")
            shutil.rmtree(self.temp_dir)
            self.temp_dir = None
            
    def create_bundle(self, manifest_path: str, output_path: str,
                     cert_path: Optional[str] = None,
                     key_path: Optional[str] = None,
                     verbose: bool = False,
                     force: bool = False) -> None:
        """Main function to create RAUC bundle"""
        self.verbose = verbose
        self.force = force
        
        try:
            # Convert to Path objects
            manifest_path_obj = Path(manifest_path)
            output_path_obj = Path(output_path)
            cert_path_obj = Path(cert_path) if cert_path else None
            key_path_obj = Path(key_path) if key_path else None
            
            # Step 1: Validation and Preparation
            self._print_info("Step 1: Validation and Preparation")
            
            # Check if RAUC is available
            if not self._check_rauc_available():
                raise BundlerError("RAUC is not installed or not available in PATH")
                
            # Check if manifest exists (in this implementation, we'll create it from rootfs)
            # For compatibility with C version, we accept rootfs file directly
            if not self._check_file_exists(manifest_path_obj):
                raise BundlerError(f"Input file '{manifest_path}' not found")
                
            # Check output directory
            output_dir = output_path_obj.parent
            if not output_dir.exists():
                raise BundlerError(f"Output directory '{output_dir}' does not exist")
                
            # Check if output file exists
            if output_path_obj.exists() and not self.force:
                raise BundlerError(f"Output file '{output_path}' already exists. Use --force to overwrite")
                
            # Validate certificate and key
            if cert_path_obj and key_path_obj:
                if not self._check_file_exists(cert_path_obj):
                    raise BundlerError(f"Certificate file '{cert_path}' not found")
                if not self._check_file_exists(key_path_obj):
                    raise BundlerError(f"Key file '{key_path}' not found")
            elif cert_path_obj or key_path_obj:
                raise BundlerError("Both certificate and key must be provided together")
                
            # Bundle configuration
            bundle_config = {
                'compatible': 'intel-i7-x64-nuc-rauc',
                'version': '1.0.0',
                'description': 'RAUC Bundle created by Python Bundler'
            }
            
            # Step 2-3: Prepare bundle directory
            self._print_info("Step 2-3: Preparing bundle directory and creating manifest")
            bundle_dir = self._prepare_bundle_directory(manifest_path_obj, bundle_config)
            
            # Step 4: Create bundle
            self._print_info("Step 4: Creating signed RAUC bundle")
            self._execute_rauc_bundle(bundle_dir, output_path_obj, cert_path_obj, key_path_obj)
            
            # Step 5: Verify bundle
            self._print_info("Step 5: Verifying bundle")
            # Try to find CA certificate for verification
            ca_path = None
            if cert_path_obj:
                ca_dir = cert_path_obj.parent
                ca_path = ca_dir / 'ca.cert.pem'
                if not self._check_file_exists(ca_path):
                    ca_path = None
                    
            self._verify_bundle(output_path_obj, ca_path)
            
            self._print_success(f"Bundle created successfully: {output_path}")
            
            # Print bundle size
            bundle_size = output_path_obj.stat().st_size
            size_mb = bundle_size / (1024 * 1024)
            print(f"Bundle size: {size_mb:.2f} MB ({bundle_size} bytes)")
            
        except BundlerError as e:
            self._print_error(str(e))
            sys.exit(1)
        except Exception as e:
            self._print_error(f"Unexpected error: {e}")
            sys.exit(1)
        finally:
            self._cleanup()


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(
        description='Create a RAUC bundle from a rootfs file (Python implementation)',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  %(prog)s rootfs.ext4 bundle.raucb
  %(prog)s -c cert.pem -k key.pem rootfs.ext4 bundle.raucb
  %(prog)s --verbose --force rootfs.ext4 bundle.raucb
        '''
    )
    
    parser.add_argument('rootfs', help='Path to the rootfs file (e.g., rootfs.ext4)')
    parser.add_argument('output', help='Path for the output .raucb bundle')
    parser.add_argument('-c', '--cert', help='Path to certificate file')
    parser.add_argument('-k', '--key', help='Path to private key file')
    parser.add_argument('-v', '--verbose', action='store_true', 
                       help='Enable verbose output')
    parser.add_argument('-f', '--force', action='store_true',
                       help='Overwrite existing output file')
    parser.add_argument('--version', action='version', version=f'%(prog)s {__version__}')
    
    args = parser.parse_args()
    
    # Create bundler instance
    bundler = RaucBundler()
    
    # Create bundle
    bundler.create_bundle(
        manifest_path=args.rootfs,  # Using rootfs as input (like C version)
        output_path=args.output,
        cert_path=args.cert,
        key_path=args.key,
        verbose=args.verbose,
        force=args.force
    )


if __name__ == '__main__':
    main()