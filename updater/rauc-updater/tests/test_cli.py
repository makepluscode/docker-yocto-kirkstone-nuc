"""Basic tests for CLI functionality."""

import pytest
from pathlib import Path
from click.testing import CliRunner

from rauc_updater.cli import cli


def test_cli_version():
    """Test CLI version command."""
    runner = CliRunner()
    result = runner.invoke(cli, ['--version'])
    assert result.exit_code == 0


def test_cli_help():
    """Test CLI help command."""
    runner = CliRunner()
    result = runner.invoke(cli, ['--help'])
    assert result.exit_code == 0
    assert 'RAUC Updater Tool' in result.output


def test_test_command_help():
    """Test test command help."""
    runner = CliRunner()
    result = runner.invoke(cli, ['test', '--help'])
    assert result.exit_code == 0
    assert 'Test connection to target device' in result.output


def test_update_command_help():
    """Test update command help."""
    runner = CliRunner()
    result = runner.invoke(cli, ['update', '--help'])
    assert result.exit_code == 0
    assert 'Update target device with RAUC bundle' in result.output


def test_status_command_help():
    """Test status command help."""
    runner = CliRunner()
    result = runner.invoke(cli, ['status', '--help'])
    assert result.exit_code == 0
    assert 'Show RAUC status on target device' in result.output


def test_cleanup_command_help():
    """Test cleanup command help."""
    runner = CliRunner()
    result = runner.invoke(cli, ['cleanup', '--help'])
    assert result.exit_code == 0
    assert 'Clean up temporary files on target device' in result.output


def test_update_command_with_nonexistent_bundle():
    """Test update command with non-existent bundle file."""
    runner = CliRunner()
    result = runner.invoke(cli, ['update', '/nonexistent/bundle.raucb'])
    assert result.exit_code == 2  # Click's error code for invalid input


def test_update_command_with_dummy_bundle(tmp_path):
    """Test update command with dummy bundle (will fail on connection)."""
    # Create a dummy bundle file
    bundle_path = tmp_path / "test-bundle.raucb"
    bundle_path.write_text("dummy bundle content")
    
    runner = CliRunner()
    result = runner.invoke(cli, [
        'update', str(bundle_path),
        '--host', '127.0.0.1',  # Use localhost to avoid network issues
        '--port', '9999',  # Non-existent port
        '--timeout', '5'  # Short timeout
    ])
    
    # Should fail with connection error
    assert result.exit_code == 1
    assert 'Connection' in result.output or 'failed' in result.output