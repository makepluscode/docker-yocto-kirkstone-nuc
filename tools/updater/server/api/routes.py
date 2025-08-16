"""API routes for updater server."""

from fastapi import APIRouter, HTTPException, Request
from fastapi.responses import FileResponse, JSONResponse

from ..services import DeploymentService, BundleService, FeedbackService
from ..models import FeedbackRequest
from ..utils.logger import StructuredLogger
from ..utils.validators import validate_tenant, validate_controller_id
from ..exceptions import (
    DeploymentNotFoundError, DeploymentCreationError,
    BundleNotFoundError, ValidationError
)
from .. import config


def create_router(
    deployment_service: DeploymentService,
    bundle_service: BundleService,
    feedback_service: FeedbackService
) -> APIRouter:
    """Create API router with all endpoints."""
    
    router = APIRouter()
    logger = StructuredLogger(__name__)
    
    @router.get("/")
    async def root():
        """Root endpoint."""
        return {
            "message": "Updater Server is running",
            "version": config.SERVER_VERSION,
            "https_enabled": config.ENABLE_HTTPS,
            "gui_enabled": config.ENABLE_GUI,
            "protocol": "https" if config.ENABLE_HTTPS else "http"
        }
    
    @router.get("/{tenant}/controller/v1/{controller_id}")
    async def poll_for_updates(tenant: str, controller_id: str, request: Request):
        """Poll for updates - updater protocol endpoint."""
        logger.log_request("GET", request.url.path, tenant=tenant, controller=controller_id)
        
        # Validate inputs
        if not validate_tenant(tenant):
            raise HTTPException(status_code=400, detail="Invalid tenant")
        if not validate_controller_id(controller_id):
            raise HTTPException(status_code=400, detail="Invalid controller ID")
        
        try:
            poll_response = deployment_service.get_deployment_for_device(tenant, controller_id)
            
            if poll_response is None:
                logger.log_response(204, request.url.path, message="No deployments")
                return JSONResponse(status_code=204, content=None)
            
            logger.log_response(200, request.url.path, deployment_id=poll_response.deployment.id)
            return poll_response
            
        except Exception as e:
            logger.log_error(e, "poll_for_updates", tenant=tenant, controller=controller_id)
            raise HTTPException(status_code=500, detail="Internal server error")
    
    @router.post("/{tenant}/controller/v1/{controller_id}/deploymentBase/{execution_id}/feedback")
    async def send_feedback(
        tenant: str,
        controller_id: str,
        execution_id: str,
        feedback: FeedbackRequest,
        request: Request
    ):
        """Send feedback - updater protocol endpoint."""
        logger.log_request(
            "POST", request.url.path,
            tenant=tenant, controller=controller_id, execution=execution_id
        )
        
        # Validate inputs
        if not validate_tenant(tenant):
            raise HTTPException(status_code=400, detail="Invalid tenant")
        if not validate_controller_id(controller_id):
            raise HTTPException(status_code=400, detail="Invalid controller ID")
        
        try:
            logger.log_feedback(
                controller_id,
                feedback.execution.result.finished.value,
                feedback.execution.result.progress
            )
            
            response = feedback_service.process_feedback(
                tenant, controller_id, execution_id, feedback
            )
            
            logger.log_response(200, request.url.path, action=response.get("action"))
            return response
            
        except Exception as e:
            logger.log_error(e, "send_feedback", controller=controller_id, execution=execution_id)
            raise HTTPException(status_code=500, detail="Internal server error")
    
    @router.get("/download/{filename}")
    async def download_bundle(filename: str, request: Request):
        """Download bundle endpoint."""
        logger.log_request("GET", request.url.path, filename=filename)
        
        try:
            bundle_path = bundle_service.get_bundle_path(filename)
            
            logger.log_response(
                200, request.url.path,
                size=bundle_path.stat().st_size,
                path=str(bundle_path)
            )
            
            return FileResponse(
                path=bundle_path,
                filename=filename,
                media_type="application/octet-stream"
            )
            
        except BundleNotFoundError as e:
            logger.log_error(e, "download_bundle", filename=filename)
            raise HTTPException(status_code=404, detail=str(e))
        except Exception as e:
            logger.log_error(e, "download_bundle", filename=filename)
            raise HTTPException(status_code=500, detail="Internal server error")
    
    # Admin endpoints
    @router.get("/admin/deployments")
    async def list_deployments(request: Request):
        """Admin endpoint to list all deployments."""
        logger.log_request("GET", request.url.path)
        
        try:
            deployments = deployment_service.get_all_deployments()
            
            response_data = {
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
            
            logger.log_response(200, request.url.path, count=len(deployments))
            return response_data
            
        except Exception as e:
            logger.log_error(e, "list_deployments")
            raise HTTPException(status_code=500, detail="Internal server error")
    
    @router.post("/admin/deployments")
    async def create_deployment(deployment: dict, request: Request):
        """Admin endpoint to create a new deployment."""
        logger.log_request("POST", request.url.path)
        
        try:
            deployment_config = deployment_service.create_deployment(deployment)
            
            logger.log_deployment("CREATED", deployment_config.execution_id)
            logger.log_response(201, request.url.path, execution_id=deployment_config.execution_id)
            
            return {
                "status": "created",
                "execution_id": deployment_config.execution_id
            }
            
        except DeploymentCreationError as e:
            logger.log_error(e, "create_deployment")
            raise HTTPException(status_code=400, detail=str(e))
        except Exception as e:
            logger.log_error(e, "create_deployment")
            raise HTTPException(status_code=500, detail="Internal server error")
    
    @router.put("/admin/deployments/{execution_id}")
    async def update_deployment(execution_id: str, deployment: dict, request: Request):
        """Admin endpoint to update a deployment."""
        logger.log_request("PUT", request.url.path, execution_id=execution_id)
        
        try:
            deployment_config = deployment_service.update_deployment(execution_id, deployment)
            
            logger.log_deployment("UPDATED", execution_id)
            logger.log_response(200, request.url.path, execution_id=execution_id)
            
            return {
                "status": "updated",
                "execution_id": execution_id
            }
            
        except DeploymentNotFoundError as e:
            logger.log_error(e, "update_deployment", execution_id=execution_id)
            raise HTTPException(status_code=404, detail=str(e))
        except Exception as e:
            logger.log_error(e, "update_deployment", execution_id=execution_id)
            raise HTTPException(status_code=500, detail="Internal server error")
    
    @router.delete("/admin/deployments/{execution_id}")
    async def delete_deployment(execution_id: str, request: Request):
        """Admin endpoint to delete a deployment."""
        logger.log_request("DELETE", request.url.path, execution_id=execution_id)
        
        try:
            success = deployment_service.delete_deployment(execution_id)
            
            if success:
                logger.log_deployment("DELETED", execution_id)
                logger.log_response(200, request.url.path, execution_id=execution_id)
                return {
                    "status": "deleted",
                    "execution_id": execution_id
                }
            else:
                logger.log_response(404, request.url.path, execution_id=execution_id)
                raise HTTPException(status_code=404, detail="Deployment not found")
                
        except Exception as e:
            logger.log_error(e, "delete_deployment", execution_id=execution_id)
            raise HTTPException(status_code=500, detail="Internal server error")
    
    # Feedback endpoints
    @router.get("/admin/feedback/{execution_id}")
    async def get_deployment_feedback(execution_id: str, request: Request):
        """Get feedback for a specific deployment."""
        logger.log_request("GET", request.url.path, execution_id=execution_id)
        
        try:
            feedback = feedback_service.get_deployment_feedback(execution_id)
            status = feedback_service.get_deployment_status(execution_id)
            
            response_data = {
                "execution_id": execution_id,
                "status": status,
                "feedback": feedback
            }
            
            logger.log_response(200, request.url.path, feedback_count=len(feedback))
            return response_data
            
        except Exception as e:
            logger.log_error(e, "get_deployment_feedback", execution_id=execution_id)
            raise HTTPException(status_code=500, detail="Internal server error")
    
    return router