#!/usr/bin/env python3
"""Test script for refactored updater server."""

import os
import sys
import tempfile
from pathlib import Path

# Add the current directory to path so we can import the modules
sys.path.insert(0, str(Path(__file__).parent))

def test_config():
    """Test configuration loading and validation."""
    print("Testing configuration...")
    
    from updater_server.config import Config, ConfigurationError
    
    # Test default config
    config = Config.from_env()
    print(f"✓ Default config loaded: {config.SERVER_NAME} v{config.SERVER_VERSION}")
    
    # Test validation
    try:
        config.validate()
        print("✓ Configuration validation passed")
    except ConfigurationError as e:
        print(f"✗ Configuration validation failed: {e}")
        return False
    
    return True


def test_services():
    """Test service layer functionality."""
    print("\nTesting services...")
    
    from updater_server.services import DeploymentService, BundleService, FeedbackService
    from updater_server.utils.url_builder import URLBuilder
    from updater_server.models import FeedbackRequest, FeedbackExecution, FeedbackResult, FeedbackStatus
    
    # Test URL builder
    url_builder = URLBuilder(base_url="http://test:8080")
    download_url = url_builder.build_download_url("test.raucb")
    print(f"✓ URL builder works: {download_url}")
    
    # Test deployment service
    deployment_service = DeploymentService(url_builder)
    deployments = deployment_service.get_all_deployments()
    print(f"✓ Deployment service works: {len(deployments)} deployments")
    
    # Test bundle service
    bundle_service = BundleService()
    print("✓ Bundle service initialized")
    
    # Test feedback service
    feedback_service = FeedbackService()
    
    # Create test feedback
    feedback = FeedbackRequest(
        id="test-feedback",
        execution=FeedbackExecution(
            result=FeedbackResult(
                finished=FeedbackStatus.PROCEEDING,
                progress=50,
                details=["Test progress update"]
            )
        )
    )
    
    response = feedback_service.process_feedback("test", "device-001", "exec-001", feedback)
    print(f"✓ Feedback service works: {response['status']}")
    
    return True


def test_validation():
    """Test validation utilities."""
    print("\nTesting validation...")
    
    from updater_server.utils.validators import (
        validate_tenant, validate_controller_id, validate_filename, sanitize_input
    )
    
    # Test tenant validation
    assert validate_tenant("default") == True
    assert validate_tenant("test-tenant") == True
    assert validate_tenant("") == False
    assert validate_tenant("tenant with spaces") == False
    print("✓ Tenant validation works")
    
    # Test controller ID validation
    assert validate_controller_id("device-001") == True
    assert validate_controller_id("device.test") == True
    assert validate_controller_id("") == False
    print("✓ Controller ID validation works")
    
    # Test filename validation
    assert validate_filename("test.raucb", [".raucb"]) == True
    assert validate_filename("test.txt", [".raucb"]) == False
    assert validate_filename("../evil.raucb") == False
    print("✓ Filename validation works")
    
    # Test input sanitization
    clean = sanitize_input("<script>alert('xss')</script>")
    assert "<" not in clean and ">" not in clean
    print("✓ Input sanitization works")
    
    return True


def test_exceptions():
    """Test custom exceptions."""
    print("\nTesting exceptions...")
    
    from updater_server.exceptions import (
        DeploymentNotFoundError, BundleNotFoundError, ValidationError
    )
    
    # Test exception hierarchy
    try:
        raise DeploymentNotFoundError("Test deployment not found")
    except DeploymentNotFoundError as e:
        print(f"✓ DeploymentNotFoundError works: {e}")
    
    try:
        raise BundleNotFoundError("Test bundle not found")
    except BundleNotFoundError as e:
        print(f"✓ BundleNotFoundError works: {e}")
    
    return True


def test_app_creation():
    """Test FastAPI app creation."""
    print("\nTesting app creation...")
    
    try:
        from updater_server.main_refactored import create_app
        
        app = create_app()
        print(f"✓ FastAPI app created successfully")
        print(f"✓ App title: {app.title}")
        print(f"✓ App version: {app.version}")
        
        return True
    except Exception as e:
        print(f"✗ App creation failed: {e}")
        return False


def main():
    """Run all tests."""
    print("=== Refactored Updater Server Tests ===\n")
    
    tests = [
        test_config,
        test_services, 
        test_validation,
        test_exceptions,
        test_app_creation
    ]
    
    passed = 0
    failed = 0
    
    for test in tests:
        try:
            if test():
                passed += 1
            else:
                failed += 1
        except Exception as e:
            print(f"✗ Test {test.__name__} failed with exception: {e}")
            failed += 1
    
    print(f"\n=== Test Results ===")
    print(f"✓ Passed: {passed}")
    print(f"✗ Failed: {failed}")
    print(f"Total: {passed + failed}")
    
    if failed == 0:
        print("\n🎉 All tests passed! Refactored server is working correctly.")
        return 0
    else:
        print(f"\n❌ {failed} tests failed. Please check the implementation.")
        return 1


if __name__ == "__main__":
    sys.exit(main())