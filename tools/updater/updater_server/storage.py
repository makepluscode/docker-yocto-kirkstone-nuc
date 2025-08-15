"""Simple in-memory storage for updater deployments."""

import os
import hashlib
from typing import Dict, Optional, List
from pathlib import Path
from .models import DeploymentConfig


class DeploymentStorage:
    """Simple in-memory storage for deployments."""
    
    def __init__(self, bundle_dir: str = "bundle"):
        self.bundle_dir = Path(bundle_dir)
        self.bundle_dir.mkdir(exist_ok=True)
        self.deployments: Dict[str, DeploymentConfig] = {}
        self._load_existing_bundles()
    
    def _load_existing_bundles(self):
        """Load existing bundles from the bundle directory."""
        if not self.bundle_dir.exists():
            return
        
        for bundle_file in self.bundle_dir.glob("*.raucb"):
            if bundle_file.is_file():
                # Create a deployment for existing bundle
                execution_id = f"exec-{bundle_file.stem}"
                version = bundle_file.stem.split("-")[-1] if "-" in bundle_file.stem else "1.0.0"
                

                
                deployment = DeploymentConfig(
                    execution_id=execution_id,
                    version=version,
                    description=f"Bundle: {bundle_file.name}",
                    download_url=f"/download/{bundle_file.name}",
                    filename=bundle_file.name,
                    size=bundle_file.stat().st_size,
                    active=True
                )
                
                self.deployments[execution_id] = deployment
                print(f"Loaded existing bundle: {bundle_file.name}")
    
    def add_deployment(self, deployment: DeploymentConfig) -> bool:
        """Add a new deployment."""
        self.deployments[deployment.execution_id] = deployment
        return True
    
    def get_deployment(self, execution_id: str) -> Optional[DeploymentConfig]:
        """Get a deployment by execution ID."""
        return self.deployments.get(execution_id)
    
    def get_active_deployments(self) -> List[DeploymentConfig]:
        """Get all active deployments."""
        active_deployments = [deployment for deployment in self.deployments.values() if deployment.active]
        # Sort by size (largest first) to prioritize the main bundle
        active_deployments.sort(key=lambda x: x.size, reverse=True)
        return active_deployments
    
    def remove_deployment(self, execution_id: str) -> bool:
        """Remove a deployment."""
        if execution_id in self.deployments:
            del self.deployments[execution_id]
            return True
        return False
    
    def get_bundle_path(self, filename: str) -> Optional[Path]:
        """Get the full path to a bundle file."""
        bundle_path = self.bundle_dir / filename
        if bundle_path.exists() and bundle_path.is_file():
            return bundle_path
        return None
    
    def calculate_file_hash(self, filepath: Path, algorithm: str = "sha256") -> str:
        """Calculate hash of a file."""
        hash_obj = hashlib.new(algorithm)
        with open(filepath, 'rb') as f:
            for chunk in iter(lambda: f.read(4096), b""):
                hash_obj.update(chunk)
        return hash_obj.hexdigest()


# Global storage instance
storage = DeploymentStorage() 