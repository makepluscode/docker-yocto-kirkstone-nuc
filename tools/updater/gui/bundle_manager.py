#!/usr/bin/env python3
"""
Bundle Manager for Updater Application
Handles bundle file discovery and latest file selection
"""

import os
import glob
from pathlib import Path
from datetime import datetime
from typing import List, Dict, Optional


class BundleManager:
    """Manages bundle files and deployment candidates."""
    
    def __init__(self, bundle_dir: str = "bundle"):
        self.bundle_dir = Path(bundle_dir)
        self.bundle_files = []
        self.latest_bundle = None
        
    def scan_bundles(self) -> List[Dict]:
        """Scan bundle directory for .raucb files."""
        if not self.bundle_dir.exists():
            return []
        
        bundle_files = []
        pattern = str(self.bundle_dir / "*.raucb")
        
        for file_path in glob.glob(pattern):
            file_info = self._get_file_info(file_path)
            if file_info:
                bundle_files.append(file_info)
        
        # Sort by modification time (newest first)
        bundle_files.sort(key=lambda x: x['mtime'], reverse=True)
        
        self.bundle_files = bundle_files
        
        # Set latest bundle
        if bundle_files:
            self.latest_bundle = bundle_files[0]
        
        return bundle_files
    
    def _get_file_info(self, file_path: str) -> Optional[Dict]:
        """Get information about a bundle file."""
        try:
            path = Path(file_path)
            stat = path.stat()
            
            # Extract version from filename if possible
            version = self._extract_version_from_filename(path.name)
            
            return {
                'name': path.name,
                'path': str(path),
                'size': stat.st_size,
                'mtime': stat.st_mtime,
                'mtime_str': datetime.fromtimestamp(stat.st_mtime).strftime('%Y-%m-%d %H:%M:%S'),
                'version': version,
                'size_mb': round(stat.st_size / (1024 * 1024), 2)
            }
        except Exception as e:
            print(f"Error getting file info for {file_path}: {e}")
            return None
    
    def _extract_version_from_filename(self, filename: str) -> str:
        """Extract version information from filename."""
        # Try to extract version from common patterns
        if 'bundle-' in filename:
            # Pattern: nuc-image-qt5-bundle-intel-corei7-64-20250816051855.raucb
            parts = filename.split('-')
            if len(parts) >= 2:
                # Look for timestamp pattern
                for part in parts:
                    if len(part) == 14 and part.isdigit():
                        # Convert timestamp to readable format
                        try:
                            dt = datetime.strptime(part, '%Y%m%d%H%M%S')
                            return dt.strftime('%Y.%m.%d-%H%M%S')
                        except ValueError:
                            pass
        
        # Fallback: use filename without extension
        return Path(filename).stem
    
    def get_latest_bundle(self) -> Optional[Dict]:
        """Get the latest bundle file."""
        if not self.bundle_files:
            self.scan_bundles()
        
        return self.latest_bundle
    
    def get_bundle_by_name(self, name: str) -> Optional[Dict]:
        """Get bundle by filename."""
        for bundle in self.bundle_files:
            if bundle['name'] == name:
                return bundle
        return None
    
    def list_bundles(self) -> List[Dict]:
        """List all available bundles."""
        if not self.bundle_files:
            self.scan_bundles()
        
        return self.bundle_files
    
    def get_bundle_summary(self) -> Dict:
        """Get summary of bundle directory."""
        bundles = self.list_bundles()
        
        total_size = sum(b['size'] for b in bundles)
        total_size_mb = round(total_size / (1024 * 1024), 2)
        
        return {
            'total_files': len(bundles),
            'total_size_mb': total_size_mb,
            'latest_bundle': self.latest_bundle,
            'bundles': bundles
        }
    
    def create_deployment_from_latest(self, name: str = None) -> Optional[Dict]:
        """Create deployment configuration from latest bundle."""
        latest = self.get_latest_bundle()
        if not latest:
            return None
        
        if not name:
            # Generate name from bundle info
            name = f"Deployment-{latest['version']}"
        
        return {
            'name': name,
            'version': latest['version'],
            'filename': latest['name'],
            'size_mb': latest['size_mb'],
            'created': latest['mtime_str'],
            'active': True
        }


# Global instance
bundle_manager = BundleManager() 