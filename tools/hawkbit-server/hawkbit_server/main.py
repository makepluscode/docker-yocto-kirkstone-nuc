"""Main FastAPI application for Hawkbit server."""

import os
import logging
from typing import Optional
from fastapi import FastAPI, HTTPException, Request
from fastapi.responses import FileResponse, JSONResponse
from fastapi.middleware.cors import CORSMiddleware
import aiofiles

from .models import (
    PollResponse, Deployment, DeploymentInfo, DeploymentChunk,
    Artifact, ArtifactLinks, ArtifactLink, FeedbackRequest
)
from .storage import storage

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

app = FastAPI(
    title="Hawkbit Server",
    description="Simple Hawkbit server implementation for OTA updates",
    version="0.1.0"
)

# Add CORS middleware
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


@app.get("/")
async def root():
    """Root endpoint."""
    return {"message": "Hawkbit Server is running", "version": "0.1.0"}


@app.get("/{tenant}/controller/v1/{controller_id}")
async def poll_for_updates(tenant: str, controller_id: str):
    """
    Poll for updates - Hawkbit protocol endpoint.
    
    This endpoint is called by devices to check for available updates.
    Returns either a deployment object or HTTP 204 (No Content).
    """
    logger.info(f"Poll request from tenant: {tenant}, controller: {controller_id}")
    
    # Get active deployments
    active_deployments = storage.get_active_deployments()
    
    if not active_deployments:
        logger.info("No active deployments available")
        return JSONResponse(status_code=204, content=None)
    
    # For simplicity, return the first active deployment
    # In a real implementation, you might want to check device compatibility
    deployment_config = active_deployments[0]
    
    # Create deployment response
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
    
    # Create artifact with download link - construct absolute URL
    base_url = "http://192.168.1.101:8080"  # Use the host IP that clients can reach
    download_url = f"{base_url}{deployment_config.download_url}"
    
    artifact_link = ArtifactLink(href=download_url)
    artifact_links = ArtifactLinks(**{"download-http": artifact_link})
    
    artifact = Artifact(
        filename=deployment_config.filename,
        size=deployment_config.size,
        hashes={},  # You could add actual file hashes here
        links=artifact_links
    )
    
    deployment = Deployment(
        id=deployment_config.execution_id,
        deployment=deployment_info,
        artifacts=[artifact]
    )
    
    response = PollResponse(deployment=deployment)
    
    # Debug: Log the actual JSON being sent to client
    import json
    response_dict = response.model_dump()
    response_json = json.dumps(response_dict, indent=2)
    logger.info(f"Returning deployment: {deployment_config.execution_id}")
    logger.info(f"JSON Response: {response_json}")
    return response


@app.post("/{tenant}/controller/v1/{controller_id}/deploymentBase/{execution_id}/feedback")
async def send_feedback(
    tenant: str, 
    controller_id: str, 
    execution_id: str, 
    feedback: FeedbackRequest
):
    """
    Send feedback - Hawkbit protocol endpoint.
    
    This endpoint receives feedback from devices about update progress and status.
    """
    logger.info(f"Feedback from tenant: {tenant}, controller: {controller_id}, execution: {execution_id}")
    logger.info(f"Feedback status: {feedback.execution.result.finished}")
    logger.info(f"Progress: {feedback.execution.result.progress}%")
    
    if feedback.execution.result.details:
        logger.info(f"Details: {feedback.execution.result.details}")
    
    # In a real implementation, you would store this feedback in a database
    # For now, we just log it
    
    return {"status": "received"}


@app.get("/download/{filename}")
async def download_bundle(filename: str):
    """
    Download bundle endpoint.
    
    Serves bundle files for download by devices.
    """
    logger.info(f"Download request for: {filename}")
    
    bundle_path = storage.get_bundle_path(filename)
    if not bundle_path:
        raise HTTPException(status_code=404, detail="Bundle not found")
    
    logger.info(f"Serving bundle: {bundle_path}")
    return FileResponse(
        path=bundle_path,
        filename=filename,
        media_type="application/octet-stream"
    )


@app.get("/admin/deployments")
async def list_deployments():
    """Admin endpoint to list all deployments."""
    deployments = storage.get_active_deployments()
    return {
        "deployments": [
            {
                "execution_id": d.execution_id,
                "version": d.version,
                "description": d.description,
                "filename": d.filename,
                "size": d.size,
                "active": d.active
            }
            for d in deployments
        ]
    }


@app.post("/admin/deployments")
async def create_deployment(deployment: dict):
    """Admin endpoint to create a new deployment."""
    try:
        deployment_config = DeploymentConfig(**deployment)
        storage.add_deployment(deployment_config)
        return {"status": "created", "execution_id": deployment_config.execution_id}
    except Exception as e:
        raise HTTPException(status_code=400, detail=str(e))


@app.delete("/admin/deployments/{execution_id}")
async def delete_deployment(execution_id: str):
    """Admin endpoint to delete a deployment."""
    if storage.remove_deployment(execution_id):
        return {"status": "deleted", "execution_id": execution_id}
    else:
        raise HTTPException(status_code=404, detail="Deployment not found")


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8080) 