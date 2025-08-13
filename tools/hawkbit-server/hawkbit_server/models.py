"""Pydantic models for Hawkbit server API."""

from typing import List, Optional, Dict, Any
from pydantic import BaseModel, Field
from enum import Enum


class FeedbackStatus(str, Enum):
    """Feedback status values."""
    PROCEEDING = "proceeding"
    SUCCESS = "success"
    FAILURE = "failure"


class FeedbackResult(BaseModel):
    """Feedback result model."""
    finished: FeedbackStatus
    progress: int = Field(ge=0, le=100)
    details: List[str] = Field(default_factory=list)


class FeedbackExecution(BaseModel):
    """Feedback execution model."""
    result: FeedbackResult


class FeedbackRequest(BaseModel):
    """Feedback request model."""
    id: str
    execution: FeedbackExecution


class DeploymentChunk(BaseModel):
    """Deployment chunk model."""
    version: str
    name: str = "default"
    part: str = "os"
    type: str = "application/octet-stream"


class DeploymentInfo(BaseModel):
    """Deployment info model."""
    chunks: List[DeploymentChunk] = Field(default_factory=list)
    type: str = "application/octet-stream"


class ArtifactLink(BaseModel):
    """Artifact link model."""
    href: str


class ArtifactLinks(BaseModel):
    """Artifact links model."""
    download_http: ArtifactLink = Field(alias="download-http")


class Artifact(BaseModel):
    """Artifact model."""
    filename: str
    size: int
    hashes: Dict[str, str] = Field(default_factory=dict)
    links: ArtifactLinks = Field(alias="_links", serialization_alias="_links")
    
    model_config = {"populate_by_name": True}


class Deployment(BaseModel):
    """Deployment model."""
    id: str
    deployment: DeploymentInfo
    artifacts: List[Artifact] = Field(default_factory=list)


class PollResponse(BaseModel):
    """Poll response model."""
    deployment: Optional[Deployment] = None


class DeploymentConfig(BaseModel):
    """Deployment configuration model."""
    execution_id: str
    version: str
    description: str
    download_url: str
    filename: str
    size: int
    active: bool = True 