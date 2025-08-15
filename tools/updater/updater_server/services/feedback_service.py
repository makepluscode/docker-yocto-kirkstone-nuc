"""Feedback service for handling device update feedback."""

import logging
from typing import Dict, List, Optional
from datetime import datetime

from ..models import FeedbackRequest, FeedbackStatus

logger = logging.getLogger(__name__)


class FeedbackService:
    """Service for handling device feedback."""
    
    def __init__(self):
        # In-memory storage for feedback history
        # In production, this should be a database
        self.feedback_history: Dict[str, List[dict]] = {}
    
    def process_feedback(
        self, 
        tenant: str, 
        controller_id: str, 
        execution_id: str, 
        feedback: FeedbackRequest
    ) -> dict:
        """Process device feedback."""
        logger.info(
            f"Processing feedback - Tenant: {tenant}, Controller: {controller_id}, "
            f"Execution: {execution_id}"
        )
        
        # Log feedback details
        result = feedback.execution.result
        logger.info(
            f"Update progress: {result.progress}% - Status: {result.finished.value}"
        )
        
        if result.details:
            logger.info(f"Details: {', '.join(result.details)}")
        
        # Store feedback in history
        feedback_record = {
            "timestamp": datetime.now().isoformat(),
            "tenant": tenant,
            "controller_id": controller_id,
            "execution_id": execution_id,
            "progress": result.progress,
            "status": result.finished.value,
            "details": result.details,
            "feedback_id": feedback.id
        }
        
        device_key = f"{tenant}:{controller_id}"
        if device_key not in self.feedback_history:
            self.feedback_history[device_key] = []
        
        self.feedback_history[device_key].append(feedback_record)
        
        # Handle different feedback statuses
        response = self._handle_feedback_status(result.finished, feedback_record)
        
        logger.info(f"Feedback processed successfully for {device_key}")
        return response
    
    def _handle_feedback_status(self, status: FeedbackStatus, feedback_record: dict) -> dict:
        """Handle different feedback statuses."""
        if status == FeedbackStatus.PROCEEDING:
            return self._handle_proceeding_feedback(feedback_record)
        elif status == FeedbackStatus.SUCCESS:
            return self._handle_success_feedback(feedback_record)
        elif status == FeedbackStatus.FAILURE:
            return self._handle_failure_feedback(feedback_record)
        else:
            logger.warning(f"Unknown feedback status: {status}")
            return {"status": "received", "action": "none"}
    
    def _handle_proceeding_feedback(self, feedback_record: dict) -> dict:
        """Handle proceeding status feedback."""
        progress = feedback_record["progress"]
        logger.info(f"Update in progress: {progress}%")
        
        # Could add progress tracking logic here
        return {
            "status": "received",
            "action": "continue",
            "message": f"Update progress: {progress}%"
        }
    
    def _handle_success_feedback(self, feedback_record: dict) -> dict:
        """Handle success status feedback."""
        logger.info(f"Update completed successfully for {feedback_record['controller_id']}")
        
        # Could trigger post-update actions here
        return {
            "status": "received",
            "action": "complete",
            "message": "Update completed successfully"
        }
    
    def _handle_failure_feedback(self, feedback_record: dict) -> dict:
        """Handle failure status feedback."""
        logger.error(
            f"Update failed for {feedback_record['controller_id']}: "
            f"{', '.join(feedback_record['details'])}"
        )
        
        # Could trigger rollback or retry logic here
        return {
            "status": "received",
            "action": "failed",
            "message": "Update failed",
            "details": feedback_record["details"]
        }
    
    def get_device_feedback_history(
        self, 
        tenant: str, 
        controller_id: str, 
        limit: int = 100
    ) -> List[dict]:
        """Get feedback history for a device."""
        device_key = f"{tenant}:{controller_id}"
        history = self.feedback_history.get(device_key, [])
        return history[-limit:] if limit else history
    
    def get_deployment_feedback(
        self, 
        execution_id: str
    ) -> List[dict]:
        """Get all feedback for a specific deployment."""
        feedback = []
        for device_history in self.feedback_history.values():
            for record in device_history:
                if record["execution_id"] == execution_id:
                    feedback.append(record)
        
        return sorted(feedback, key=lambda x: x["timestamp"])
    
    def get_deployment_status(self, execution_id: str) -> dict:
        """Get overall status for a deployment."""
        feedback = self.get_deployment_feedback(execution_id)
        
        if not feedback:
            return {
                "execution_id": execution_id,
                "status": "no_feedback",
                "device_count": 0,
                "progress": {}
            }
        
        # Aggregate status by device
        devices = {}
        for record in feedback:
            device_id = record["controller_id"]
            if device_id not in devices:
                devices[device_id] = {
                    "latest_progress": 0,
                    "latest_status": "unknown",
                    "latest_timestamp": None
                }
            
            # Update with latest feedback
            if (devices[device_id]["latest_timestamp"] is None or 
                record["timestamp"] > devices[device_id]["latest_timestamp"]):
                devices[device_id].update({
                    "latest_progress": record["progress"],
                    "latest_status": record["status"],
                    "latest_timestamp": record["timestamp"]
                })
        
        return {
            "execution_id": execution_id,
            "status": "active",
            "device_count": len(devices),
            "devices": devices
        }