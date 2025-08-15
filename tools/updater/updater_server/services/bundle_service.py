"""Bundle service for managing bundle files."""

import logging
import os
from pathlib import Path
from typing import Optional

from ..storage import storage
from ..exceptions import BundleNotFoundError

logger = logging.getLogger(__name__)


class BundleService:
    """Service for managing bundle files."""
    
    def get_bundle_path(self, filename: str) -> Path:
        """Get the path to a bundle file."""
        logger.info(f"Requested bundle: {filename}")
        
        bundle_path = storage.get_bundle_path(filename)
        if not bundle_path:
            logger.error(f"Bundle not found in storage: {filename}")
            raise BundleNotFoundError(f"Bundle not found: {filename}")
        
        if not bundle_path.exists():
            logger.error(f"Bundle file does not exist: {bundle_path}")
            raise BundleNotFoundError(f"Bundle file not found on filesystem: {filename}")
        
        file_size = bundle_path.stat().st_size
        if file_size == 0:
            logger.error(f"Bundle file is empty: {bundle_path}")
            raise BundleNotFoundError(f"Bundle file is empty: {filename}")
        
        logger.info(f"Bundle found: {filename} ({file_size} bytes) at {bundle_path}")
        return bundle_path
    
    def get_bundle_info(self, filename: str) -> dict:
        """Get bundle file information."""
        bundle_path = self.get_bundle_path(filename)
        
        stat = bundle_path.stat()
        return {
            "filename": filename,
            "path": str(bundle_path),
            "size": stat.st_size,
            "modified": stat.st_mtime,
            "exists": True
        }
    
    def validate_bundle(self, filename: str) -> bool:
        """Validate bundle file."""
        try:
            bundle_path = self.get_bundle_path(filename)
            
            # Basic validation
            if not filename.endswith('.raucb'):
                logger.warning(f"Bundle has invalid extension: {filename}")
                return False
            
            # Check file size (should be > 0)
            if bundle_path.stat().st_size == 0:
                logger.warning(f"Bundle is empty: {filename}")
                return False
            
            # TODO: Add RAUC bundle format validation
            
            return True
        except BundleNotFoundError:
            return False
        except Exception as e:
            logger.error(f"Bundle validation failed for {filename}: {e}")
            return False