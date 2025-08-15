"""Deployment service for managing update deployments."""

import logging
from typing import List, Optional
from pathlib import Path

from ..models import (
    PollResponse, Deployment, DeploymentInfo, DeploymentChunk,
    Artifact, ArtifactLinks, ArtifactLink, DeploymentConfig
)
from ..storage import storage
from ..utils.url_builder import URLBuilder
from ..exceptions import DeploymentNotFoundError, DeploymentCreationError

logger = logging.getLogger(__name__)


class DeploymentService:
    """Service for managing deployments."""
    
    def __init__(self, url_builder: URLBuilder):
        self.url_builder = url_builder
    
    def get_deployment_for_device(self, tenant: str, controller_id: str) -> Optional[PollResponse]:
        """Get deployment for a specific device."""
        logger.info(f"Checking deployments for tenant: {tenant}, controller: {controller_id}")
        
        active_deployments = storage.get_active_deployments()
        logger.info(f"Found {len(active_deployments)} active deployments")
        
        if not active_deployments:
            logger.info("No active deployments available")
            return None
        
        # Select deployment (could add device compatibility logic here)
        deployment_config = self._select_deployment_for_device(
            active_deployments, tenant, controller_id
        )
        
        if not deployment_config:
            logger.info("No compatible deployment found for device")
            return None
        
        logger.info(f"Selected deployment {deployment_config.execution_id} for device")
        
        return self._create_poll_response(deployment_config)
    
    def _select_deployment_for_device(
        self, 
        deployments: List[DeploymentConfig], 
        tenant: str, 
        controller_id: str
    ) -> Optional[DeploymentConfig]:
        """Select the best deployment for a device."""
        # For now, return the first (largest) deployment
        # Future: Add device compatibility, targeting logic
        return deployments[0] if deployments else None
    
    def _create_poll_response(self, deployment_config: DeploymentConfig) -> PollResponse:
        """Create a poll response from deployment config."""
        deployment_chunk = DeploymentChunk(
            version=deployment_config.version,
            name="default",
            part="os",
            type="application/octet-stream"
        )
        
        deployment_info = DeploymentInfo(
            chunks=[deployment_chunk],
            type="application/octet-stream"
        )
        
        # Build download URL using URL builder
        download_url = self.url_builder.build_download_url(deployment_config.filename)
        
        artifact_link = ArtifactLink(href=download_url)
        artifact_links = ArtifactLinks(**{"download-http": artifact_link})
        
        artifact = Artifact(
            filename=deployment_config.filename,
            size=deployment_config.size,
            hashes={},  # TODO: Add file hash calculation
            links=artifact_links
        )
        
        deployment = Deployment(
            id=deployment_config.execution_id,
            deployment=deployment_info,
            artifacts=[artifact]
        )
        
        logger.info(f"Created poll response for deployment {deployment_config.execution_id}")
        return PollResponse(deployment=deployment)
    
    def create_deployment(self, deployment_data: dict) -> DeploymentConfig:
        """Create a new deployment."""
        try:
            deployment_config = DeploymentConfig(**deployment_data)
            storage.add_deployment(deployment_config)
            logger.info(f"Created deployment {deployment_config.execution_id}")
            return deployment_config
        except Exception as e:
            logger.error(f"Failed to create deployment: {e}")
            raise DeploymentCreationError(f"Invalid deployment data: {e}")
    
    def get_deployment(self, execution_id: str) -> Optional[DeploymentConfig]:
        """Get deployment by execution ID."""
        deployment = storage.get_deployment(execution_id)
        if not deployment:
            logger.warning(f"Deployment not found: {execution_id}")
        return deployment
    
    def get_all_deployments(self) -> List[DeploymentConfig]:
        """Get all active deployments."""
        return storage.get_active_deployments()
    
    def update_deployment(self, execution_id: str, update_data: dict) -> DeploymentConfig:
        """Update an existing deployment."""
        deployment = storage.get_deployment(execution_id)
        if not deployment:
            raise DeploymentNotFoundError(f"Deployment not found: {execution_id}")
        
        # Update deployment fields
        for key, value in update_data.items():
            if hasattr(deployment, key):
                setattr(deployment, key, value)
        
        storage.add_deployment(deployment)
        logger.info(f"Updated deployment {execution_id}")
        return deployment
    
    def delete_deployment(self, execution_id: str) -> bool:
        """Delete a deployment."""
        if storage.remove_deployment(execution_id):
            logger.info(f"Deleted deployment {execution_id}")
            return True
        else:
            logger.warning(f"Deployment not found for deletion: {execution_id}")
            return False