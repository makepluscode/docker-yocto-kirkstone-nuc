#!/usr/bin/env python3
"""Simple test client for Hawkbit server."""

import requests
import json
import sys
import os
from pathlib import Path

# Server configuration
SERVER_URL = "http://localhost:8080"
TENANT = "default"
CONTROLLER_ID = "test-device-001"


def test_poll_for_updates():
    """Test polling for updates."""
    print("Testing poll for updates...")
    
    url = f"{SERVER_URL}/{TENANT}/controller/v1/{CONTROLLER_ID}"
    response = requests.get(url)
    
    print(f"Status Code: {response.status_code}")
    
    if response.status_code == 200:
        data = response.json()
        print("Response:")
        print(json.dumps(data, indent=2))
        
        if data.get("deployment"):
            deployment = data["deployment"]
            print(f"\nDeployment ID: {deployment['id']}")
            print(f"Version: {deployment['deployment']['chunks'][0]['version']}")
            if deployment.get("artifacts"):
                artifact = deployment["artifacts"][0]
                print(f"Artifact: {artifact['filename']}")
                print(f"Download URL: {artifact['_links']['download-http']['href']}")
        
        return data
    elif response.status_code == 204:
        print("No updates available (HTTP 204)")
        return None
    else:
        print(f"Error: {response.text}")
        return None


def test_send_feedback(execution_id, status="success", progress=100):
    """Test sending feedback."""
    print(f"\nTesting send feedback for execution: {execution_id}")
    
    url = f"{SERVER_URL}/{TENANT}/controller/v1/{CONTROLLER_ID}/deploymentBase/{execution_id}/feedback"
    
    feedback_data = {
        "id": execution_id,
        "execution": {
            "result": {
                "finished": status,
                "progress": progress,
                "details": [f"Test feedback: {status}"]
            }
        }
    }
    
    response = requests.post(url, json=feedback_data)
    
    print(f"Status Code: {response.status_code}")
    if response.status_code == 200:
        print("Feedback sent successfully")
        return True
    else:
        print(f"Error: {response.text}")
        return False


def test_download_bundle(filename):
    """Test downloading a bundle."""
    print(f"\nTesting download bundle: {filename}")
    
    url = f"{SERVER_URL}/download/{filename}"
    response = requests.get(url, stream=True)
    
    print(f"Status Code: {response.status_code}")
    
    if response.status_code == 200:
        # Save to test file
        test_file = f"test_download_{filename}"
        with open(test_file, 'wb') as f:
            for chunk in response.iter_content(chunk_size=8192):
                f.write(chunk)
        
        print(f"Bundle downloaded to: {test_file}")
        print(f"Size: {os.path.getsize(test_file)} bytes")
        return True
    else:
        print(f"Error: {response.text}")
        return False


def test_admin_endpoints():
    """Test admin endpoints."""
    print("\nTesting admin endpoints...")
    
    # List deployments
    url = f"{SERVER_URL}/admin/deployments"
    response = requests.get(url)
    
    print(f"List deployments status: {response.status_code}")
    if response.status_code == 200:
        data = response.json()
        print("Deployments:")
        print(json.dumps(data, indent=2))
        return data.get("deployments", [])
    else:
        print(f"Error: {response.text}")
        return []


def main():
    """Main test function."""
    print("Hawkbit Server Test Client")
    print("=" * 40)
    
    # Test admin endpoints first
    deployments = test_admin_endpoints()
    
    # Test poll for updates
    poll_result = test_poll_for_updates()
    
    if poll_result and poll_result.get("deployment"):
        deployment = poll_result["deployment"]
        execution_id = deployment["id"]
        
        # Test sending feedback
        test_send_feedback(execution_id, "proceeding", 50)
        test_send_feedback(execution_id, "success", 100)
        
        # Test downloading bundle if available
        if deployment.get("artifacts"):
            artifact = deployment["artifacts"][0]
            filename = artifact["filename"]
            test_download_bundle(filename)
    
    print("\nTest completed!")


if __name__ == "__main__":
    main() 