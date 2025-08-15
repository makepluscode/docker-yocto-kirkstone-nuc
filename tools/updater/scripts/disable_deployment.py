#!/usr/bin/env python3
"""Script to temporarily disable a deployment."""

import requests
import json
import sys

def disable_deployment(execution_id):
    """Disable a deployment by setting active=False."""
    url = "http://localhost:8080/admin/deployments"
    
    # Get current deployments
    try:
        response = requests.get(url)
        if response.status_code == 200:
            deployments = response.json()["deployments"]
            
            # Find the deployment to disable
            for deployment in deployments:
                if deployment["execution_id"] == execution_id:
                    print(f"Found deployment: {execution_id}")
                    print(f"Current status: active={deployment.get('active', True)}")
                    
                    # Create a modified deployment with active=False
                    modified_deployment = deployment.copy()
                    modified_deployment["active"] = False
                    
                    # Update the deployment
                    update_response = requests.put(f"{url}/{execution_id}", json=modified_deployment)
                    if update_response.status_code == 200:
                        print(f"Successfully disabled deployment: {execution_id}")
                        return True
                    else:
                        print(f"Failed to disable deployment: {update_response.status_code}")
                        return False
            
            print(f"Deployment {execution_id} not found")
            return False
            
    except requests.exceptions.RequestException as e:
        print(f"Error connecting to server: {e}")
        return False

def enable_deployment(execution_id):
    """Enable a deployment by setting active=True."""
    url = "http://localhost:8080/admin/deployments"
    
    try:
        response = requests.get(url)
        if response.status_code == 200:
            deployments = response.json()["deployments"]
            
            for deployment in deployments:
                if deployment["execution_id"] == execution_id:
                    print(f"Found deployment: {execution_id}")
                    
                    # Create a modified deployment with active=True
                    modified_deployment = deployment.copy()
                    modified_deployment["active"] = True
                    
                    # Update the deployment
                    update_response = requests.put(f"{url}/{execution_id}", json=modified_deployment)
                    if update_response.status_code == 200:
                        print(f"Successfully enabled deployment: {execution_id}")
                        return True
                    else:
                        print(f"Failed to enable deployment: {update_response.status_code}")
                        return False
            
            print(f"Deployment {execution_id} not found")
            return False
            
    except requests.exceptions.RequestException as e:
        print(f"Error connecting to server: {e}")
        return False

def list_deployments():
    """List all deployments."""
    url = "http://localhost:8080/admin/deployments"
    
    try:
        response = requests.get(url)
        if response.status_code == 200:
            deployments = response.json()["deployments"]
            print("Current deployments:")
            for deployment in deployments:
                status = "ACTIVE" if deployment.get("active", True) else "DISABLED"
                print(f"  {deployment['execution_id']}: {status}")
                print(f"    Version: {deployment['version']}")
                print(f"    Size: {deployment['size']} bytes")
                print(f"    Filename: {deployment['filename']}")
                print()
        else:
            print(f"Failed to get deployments: {response.status_code}")
            
    except requests.exceptions.RequestException as e:
        print(f"Error connecting to server: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage:")
        print("  python disable_deployment.py list")
        print("  python disable_deployment.py disable <execution_id>")
        print("  python disable_deployment.py enable <execution_id>")
        sys.exit(1)
    
    command = sys.argv[1]
    
    if command == "list":
        list_deployments()
    elif command == "disable" and len(sys.argv) >= 3:
        execution_id = sys.argv[2]
        disable_deployment(execution_id)
    elif command == "enable" and len(sys.argv) >= 3:
        execution_id = sys.argv[2]
        enable_deployment(execution_id)
    else:
        print("Invalid command. Use 'list', 'disable <execution_id>', or 'enable <execution_id>'")
        sys.exit(1) 