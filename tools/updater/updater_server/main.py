"""Main FastAPI application for updater server."""

import os
import signal
import asyncio
import logging
import ssl
from pathlib import Path
from typing import Optional
from fastapi import FastAPI, HTTPException, Request
from fastapi.responses import FileResponse, JSONResponse
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
import aiofiles
import uvicorn

from .models import (
    PollResponse, Deployment, DeploymentInfo, DeploymentChunk,
    Artifact, ArtifactLinks, ArtifactLink, FeedbackRequest, DeploymentConfig
)
from .storage import storage
from . import config

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

app = FastAPI(
    title="Updater Server",
    description="Simple updater server implementation for OTA updates with HTTPS support",
    version="0.2.0"
)

# Mount static files if GUI is enabled
if config.ENABLE_GUI and config.STATIC_PATH_OBJ.exists():
    app.mount("/static", StaticFiles(directory=str(config.STATIC_PATH_OBJ)), name="static")
    logger.info(f"Static files mounted from: {config.STATIC_PATH_OBJ}")

# Mount GUI files if available
if config.ENABLE_GUI and config.GUI_PATH_OBJ.exists():
    app.mount("/gui", StaticFiles(directory=str(config.GUI_PATH_OBJ), html=True), name="gui")
    logger.info(f"GUI files mounted from: {config.GUI_PATH_OBJ}")

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
    return {
        "message": "Updater Server is running",
        "version": "0.2.0",
        "https_enabled": config.ENABLE_HTTPS,
        "gui_enabled": config.ENABLE_GUI,
        "protocol": "https" if config.ENABLE_HTTPS else "http"
    }


@app.get("/{tenant}/controller/v1/{controller_id}")
async def poll_for_updates(tenant: str, controller_id: str):
    """
    Poll for updates - updater protocol endpoint.
    
    This endpoint is called by devices to check for available updates.
    Returns either a deployment object or HTTP 204 (No Content).
    """
    logger.info(f"=== Poll request received ===")
    logger.info(f"Tenant: {tenant}, Controller: {controller_id}")
    
    # Get active deployments
    active_deployments = storage.get_active_deployments()
    logger.info(f"Active deployments count: {len(active_deployments)}")
    
    if not active_deployments:
        logger.info("No active deployments, returning HTTP 204")
        return JSONResponse(status_code=204, content=None)
    
    # For simplicity, return the first active deployment
    # In a real implementation, you might want to check device compatibility
    deployment_config = active_deployments[0]
    logger.info(f"Selected deployment: {deployment_config.execution_id}")
    logger.info(f"Deployment version: {deployment_config.version}")
    logger.info(f"Deployment filename: {deployment_config.filename}")
    logger.info(f"Deployment size: {deployment_config.size}")
    logger.info(f"Deployment download_url: {deployment_config.download_url}")
    
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
    logger.info(f"Constructed download URL: {download_url}")
    
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
    
    logger.info(f"=== Returning deployment response ===")
    logger.info(f"Deployment ID: {deployment_config.execution_id}")
    logger.info(f"Download URL in response: {download_url}")
    return response


@app.post("/{tenant}/controller/v1/{controller_id}/deploymentBase/{execution_id}/feedback")
async def send_feedback(
    tenant: str, 
    controller_id: str, 
    execution_id: str, 
    feedback: FeedbackRequest
):
    """
    Send feedback - updater protocol endpoint.
    
    This endpoint receives feedback from devices about update progress and status.
    """
    logger.info(f"Update progress: {feedback.execution.result.progress}% - {feedback.execution.result.finished}")
    
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
    logger.info(f"=== Download request received ===")
    logger.info(f"Requested filename: {filename}")
    
    bundle_path = storage.get_bundle_path(filename)
    logger.info(f"Bundle path from storage: {bundle_path}")
    
    if not bundle_path:
        logger.error(f"Bundle not found in storage: {filename}")
        raise HTTPException(status_code=404, detail="Bundle not found")
    
    # Check if file actually exists
    import os
    if not os.path.exists(bundle_path):
        logger.error(f"Bundle file does not exist on filesystem: {bundle_path}")
        raise HTTPException(status_code=404, detail="Bundle file not found on filesystem")
    
    # Get file size
    file_size = os.path.getsize(bundle_path)
    logger.info(f"Bundle file size: {file_size} bytes")
    
    if file_size == 0:
        logger.error(f"Bundle file is empty: {bundle_path}")
        raise HTTPException(status_code=404, detail="Bundle file is empty")
    
    logger.info(f"Serving bundle: {filename} ({file_size} bytes) from: {bundle_path}")
    
    response = FileResponse(
        path=bundle_path,
        filename=filename,
        media_type="application/octet-stream"
    )
    
    logger.info(f"=== FileResponse created successfully ===")
    logger.info(f"Starting file transfer for: {filename}")
    logger.info(f"File size to transfer: {file_size} bytes")
    return response


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


@app.put("/admin/deployments/{execution_id}")
async def update_deployment(execution_id: str, deployment: dict):
    """Admin endpoint to update a deployment."""
    try:
        # Get existing deployment
        existing_deployment = storage.get_deployment(execution_id)
        if not existing_deployment:
            raise HTTPException(status_code=404, detail="Deployment not found")
        
        # Update the deployment
        for key, value in deployment.items():
            if hasattr(existing_deployment, key):
                setattr(existing_deployment, key, value)
        
        # Update in storage
        storage.add_deployment(existing_deployment)
        return {"status": "updated", "execution_id": execution_id}
    except Exception as e:
        raise HTTPException(status_code=400, detail=str(e))


class GracefulKiller:
    """Handle graceful shutdown signals."""
    
    def __init__(self):
        self.kill_now = False
        signal.signal(signal.SIGINT, self._handle_signal)
        signal.signal(signal.SIGTERM, self._handle_signal)
    
    def _handle_signal(self, signum, frame):
        logger.info(f"Received signal {signum}, initiating graceful shutdown...")
        self.kill_now = True


def create_ssl_context():
    """Create SSL context for HTTPS."""
    if not config.ENABLE_HTTPS:
        return None
        
    ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    
    # Check if certificate files exist
    if not config.SSL_CERT_PATH.exists():
        logger.warning(f"SSL certificate not found: {config.SSL_CERT_PATH}")
        return None
        
    if not config.SSL_KEY_PATH.exists():
        logger.warning(f"SSL key not found: {config.SSL_KEY_PATH}")
        return None
    
    try:
        ssl_context.load_cert_chain(
            str(config.SSL_CERT_PATH),
            str(config.SSL_KEY_PATH)
        )
        logger.info("SSL context created successfully")
        return ssl_context
    except Exception as e:
        logger.error(f"Failed to create SSL context: {e}")
        return None

async def main():
    """Main function with graceful shutdown handling."""
    killer = GracefulKiller()
    
    # Create SSL context if HTTPS is enabled
    ssl_context = create_ssl_context()
    
    # Determine port and protocol
    port = config.HTTPS_PORT if ssl_context else config.PORT
    protocol = "https" if ssl_context else "http"
    
    # Create uvicorn config
    uvicorn_config = uvicorn.Config(
        app,
        host=config.HOST,
        port=port,
        log_level=config.LOG_LEVEL.lower(),
        ssl_keyfile=str(config.SSL_KEY_PATH) if ssl_context else None,
        ssl_certfile=str(config.SSL_CERT_PATH) if ssl_context else None,
        ssl_ca_certs=str(config.SSL_CA_PATH) if ssl_context and config.SSL_CA_PATH.exists() else None
    )
    
    server = uvicorn.Server(uvicorn_config)
    
    try:
        print(f"Updater Server started on {protocol}://{config.HOST}:{port}")
        if ssl_context:
            print("🔒 HTTPS/TLS encryption enabled")
        else:
            print("⚠️  HTTP mode - consider enabling HTTPS for production")
        print("Press Ctrl+C to stop gracefully")
        print("="*50)
        
        # Start server in a coroutine
        server_task = asyncio.create_task(server.serve())
        
        # Monitor for shutdown signal
        while not killer.kill_now:
            await asyncio.sleep(0.1)
        
        # Graceful shutdown
        print("\nShutting down server...")
        server.should_exit = True
        await server_task
        
    except KeyboardInterrupt:
        print("\nReceived keyboard interrupt, shutting down...")
    except Exception as e:
        logger.error(f"Server error: {e}")
    finally:
        print("Server stopped gracefully")


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nServer interrupted by user") 