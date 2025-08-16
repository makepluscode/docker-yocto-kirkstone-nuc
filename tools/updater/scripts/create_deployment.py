#!/usr/bin/env python3
"""
Script to create deployment from latest bundle
Automatically creates an active deployment for the most recent bundle file
"""

import sys
import requests
import json
from pathlib import Path

# Add the project root to Python path
sys.path.insert(0, str(Path(__file__).parent))

from gui.bundle_manager import bundle_manager

def create_deployment_from_latest():
    """Create deployment from the latest bundle file."""
    
    # Scan for bundles
    print("🔍 Scanning for latest bundle...")
    bundles = bundle_manager.scan_bundles()
    
    if not bundles:
        print("❌ No bundle files found")
        return False
    
    latest = bundle_manager.get_latest_bundle()
    print(f"✅ Latest bundle: {latest['name']}")
    print(f"📦 Version: {latest['version']}")
    print(f"📏 Size: {latest['size_mb']} MB")
    
    # Create deployment configuration
    execution_id = f"nuc-update-{latest['version'].replace('.', '-').replace(':', '-')}"
    download_url = f"http://192.168.1.101:8080/download/{latest['name']}"
    
    deployment_data = {
        "execution_id": execution_id,
        "version": latest['version'],
        "description": f"Latest NUC Update - {latest['name']}",
        "download_url": download_url,
        "filename": latest['name'],
        "size": latest['size'],
        "active": True
    }
    
    print(f"🚀 Creating deployment: {execution_id}")
    
    # Create deployment via API
    try:
        response = requests.post(
            "http://localhost:8080/admin/deployments",
            json=deployment_data,
            headers={"Content-Type": "application/json"}
        )
        
        if response.status_code == 200:
            result = response.json()
            print(f"✅ Deployment created successfully!")
            print(f"📋 Execution ID: {result.get('execution_id')}")
            return True
        else:
            print(f"❌ Failed to create deployment: {response.status_code}")
            print(f"Response: {response.text}")
            return False
            
    except Exception as e:
        print(f"❌ Error creating deployment: {e}")
        return False

def check_deployments():
    """Check current deployments."""
    try:
        response = requests.get("http://localhost:8080/admin/deployments")
        if response.status_code == 200:
            deployments = response.json()
            print(f"📋 Current deployments: {len(deployments.get('deployments', []))}")
            for dep in deployments.get('deployments', []):
                status = "✅ Active" if dep.get('active') else "❌ Inactive"
                print(f"   {status} - {dep.get('execution_id')} ({dep.get('version')})")
        else:
            print(f"❌ Failed to get deployments: {response.status_code}")
    except Exception as e:
        print(f"❌ Error checking deployments: {e}")

def test_poll_endpoint():
    """Test the poll endpoint."""
    try:
        response = requests.get("http://localhost:8080/DEFAULT/controller/v1/nuc-device-001")
        if response.status_code == 200:
            print("✅ Poll endpoint returns deployment (HTTP 200)")
            data = response.json()
            if 'deployment' in data:
                print(f"📦 Deployment ID: {data['deployment'].get('id')}")
                print(f"🔄 Version: {data['deployment']['deployment']['chunks'][0].get('version')}")
        elif response.status_code == 204:
            print("⚠️ Poll endpoint returns no content (HTTP 204) - no active deployments")
        else:
            print(f"❌ Poll endpoint error: {response.status_code}")
    except Exception as e:
        print(f"❌ Error testing poll endpoint: {e}")

def main():
    """Main function."""
    print("🚀 Updater Deployment Creator")
    print("=" * 40)
    
    # Check current status
    print("\n📊 Current Status:")
    check_deployments()
    test_poll_endpoint()
    
    print("\n🔄 Creating deployment from latest bundle...")
    if create_deployment_from_latest():
        print("\n📊 Updated Status:")
        check_deployments()
        test_poll_endpoint()
        
        print("\n✅ Target device should now receive updates!")
        print("💡 The device will download from: http://192.168.1.101:8080/download/[bundle-name]")
    else:
        print("\n❌ Failed to create deployment")

if __name__ == "__main__":
    main() 