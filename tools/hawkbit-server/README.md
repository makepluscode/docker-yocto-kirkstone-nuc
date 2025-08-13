# Hawkbit Server

A simple Hawkbit server implementation using FastAPI for OTA (Over-The-Air) updates.

## Features

- Poll endpoint for device updates
- Feedback endpoint for update status
- Bundle download endpoint
- Simple deployment management
- RESTful API following Hawkbit protocol

## Installation

Using uv:

```bash
uv sync
```

## Running the server

```bash
uv run uvicorn hawkbit_server.main:app --host 0.0.0.0 --port 8080 --reload
```

## API Endpoints

- `GET /{tenant}/controller/v1/{controller_id}` - Poll for updates
- `POST /{tenant}/controller/v1/{controller_id}/deploymentBase/{execution_id}/feedback` - Send feedback
- `GET /download/{filename}` - Download bundle

## Configuration

The server uses a simple in-memory storage for deployments. In production, you should implement a proper database backend. 