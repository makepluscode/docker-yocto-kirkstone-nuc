# RAUC Updater Testing Guide

This guide covers automated testing capabilities for the RAUC updater tool, including end-to-end testing scripts and batch testing scenarios.

## Overview

The testing framework provides:

- **Automated end-to-end testing**: Complete RAUC update workflow from start to finish
- **Batch testing**: Multiple devices and scenarios
- **Detailed logging and reporting**: Comprehensive test results with HTML reports
- **Configuration management**: Flexible test configuration system

## Test Scripts

### 1. `test_rauc_auto.sh` - Automated End-to-End Test

**Purpose**: Performs a complete automated RAUC update test from start to finish.

**Features**:
- ✅ Prerequisites checking
- ✅ Initial system state capture
- ✅ SSH key setup and authentication
- ✅ Connection testing
- ✅ RAUC bundle update
- ✅ System reboot monitoring
- ✅ Update verification
- ✅ Cleanup and reporting

**Usage**:
```bash
# Basic usage (uses default bundle and target)
./test_rauc_auto.sh

# Custom bundle file
./test_rauc_auto.sh /path/to/bundle.raucb

# Custom bundle and target
./test_rauc_auto.sh /path/to/bundle.raucb 192.168.1.150

# Help
./test_rauc_auto.sh --help
```

**Output**:
- Real-time colored console output
- Detailed log file with timestamps
- Test summary with timing and results

### 2. `test_batch.sh` - Batch Testing

**Purpose**: Runs multiple test scenarios across multiple devices automatically.

**Features**:
- Multiple test scenarios (basic, double update)
- Multiple device support
- Network setup integration
- HTML report generation
- Parallel test execution

**Usage**:
```bash
# Run all tests on all configured devices
./test_batch.sh

# Run basic tests only
./test_batch.sh -s basic

# Test specific device only
./test_batch.sh -d 192.168.1.100

# Setup network and run double tests
./test_batch.sh --network-setup -s double

# Help
./test_batch.sh --help
```

### 3. `test_config.sh` - Test Configuration

**Purpose**: Centralized configuration for all test scripts.

**Features**:
- Default test parameters
- Environment variable overrides
- Configuration validation
- Multiple device definitions

## Test Configuration

### Default Settings

```bash
# Target configuration
RAUC_TEST_BUNDLE_DIR="../../image"
RAUC_TEST_BUNDLE_NAME="nuc-image-qt5-bundle-intel-corei7-64.raucb"
RAUC_TEST_HOST="192.168.1.100"
RAUC_TEST_USER="root"
RAUC_TEST_PASSWORD="root"

# Timeouts
RAUC_TEST_CONNECTION_TIMEOUT="10"     # Connection timeout
RAUC_TEST_REBOOT_TIMEOUT="300"       # Max wait for reboot
RAUC_TEST_RETRY_INTERVAL="15"        # Retry interval during reboot

# Behavior
RAUC_TEST_CLEANUP_AFTER="true"       # Cleanup after tests
RAUC_TEST_VERBOSE="true"             # Verbose output
```

### Environment Variable Overrides

You can override any configuration using environment variables:

```bash
# Override target host
export TARGET_HOST="192.168.1.150"
./test_rauc_auto.sh

# Override reboot timeout
export REBOOT_TIMEOUT="600"
./test_rauc_auto.sh

# Override multiple settings
export TARGET_HOST="192.168.1.150"
export TARGET_PASSWORD="mypassword"
export RAUC_TEST_VERBOSE="false"
./test_batch.sh
```

## Test Scenarios

### Basic Test Scenario

**Description**: Single RAUC update with A/B slot switching.

**Steps**:
1. Check prerequisites and initial state
2. Setup SSH authentication
3. Test connection to target
4. Upload and install RAUC bundle
5. Wait for system reboot
6. Verify slot switch and system state
7. Cleanup temporary files

**Expected Result**: System boots from opposite slot after update.

### Double Test Scenario

**Description**: Two consecutive RAUC updates returning to original state.

**Steps**:
1. Perform first basic update (A→B or B→A)
2. Wait for system stabilization
3. Perform second basic update (B→A or A→B)
4. Verify return to original slot

**Expected Result**: System returns to original slot after two updates.

## Running Tests

### Prerequisites

1. **Environment Setup**:
   ```bash
   cd tools/updater
   uv sync
   ```

2. **Network Configuration** (if needed):
   ```bash
   # Automatic setup
   ./test_batch.sh --network-setup
   
   # Manual setup
   ../../connect.sh
   ```

3. **Bundle File**: Ensure RAUC bundle exists:
   ```bash
   ls -la ../../image/nuc-image-qt5-bundle-intel-corei7-64.raucb
   ```

### Single Device Testing

**Quick Test**:
```bash
# Run complete end-to-end test
./test_rauc_auto.sh

# Expected output:
# ✅ Prerequisites check passed
# ✅ SSH key setup completed
# ✅ Connection test passed
# ✅ RAUC update completed
# ✅ System reboot verified
# ✅ Slot switch confirmed: B → A
# 🎯 End-to-End RAUC Update Test COMPLETED!
```

**Custom Configuration**:
```bash
# Custom bundle and target
./test_rauc_auto.sh /data/custom-bundle.raucb 192.168.1.150

# With environment overrides
TARGET_PASSWORD="custom123" REBOOT_TIMEOUT="600" ./test_rauc_auto.sh
```

### Multiple Device Testing

**Basic Batch Test**:
```bash
# Edit test_config.sh to add your devices
export RAUC_TEST_DEVICES=(
    "192.168.1.100"
    "192.168.1.101"
    "192.168.1.102"
)

# Run batch tests
./test_batch.sh
```

**Scenario-Specific Testing**:
```bash
# Run double update tests only
./test_batch.sh -s double

# Test single device with all scenarios
./test_batch.sh -d 192.168.1.100
```

## Test Results and Logging

### Log Files

**Individual Tests**:
- Location: `./logs/test_results_YYYYMMDD_HHMMSS.log`
- Format: Timestamped entries with level indicators
- Content: Detailed execution log with all commands and outputs

**Batch Tests**:
- Location: `./logs/batch_test_YYYYMMDD_HHMMSS.log`
- Results Directory: `./logs/results_YYYYMMDD_HHMMSS/`
- Individual test logs: `results_*/basic_192_168_1_100.log`

### HTML Reports

Batch testing generates HTML reports with:
- Test configuration summary
- Individual test results with pass/fail status
- Detailed logs for each test
- Overall success rate and timing

**Example Report**:
```
Test Report: ./logs/results_20240806_210000/test_report.html
- Total Tests: 4
- Passed: 4
- Success Rate: 100%
- Duration: 15m 30s
```

### Console Output

**Success Indicators**:
```
✅ SSH key setup completed
✅ Connection test passed
✅ RAUC update completed in 27 seconds
✅ System reboot verified after 45 seconds
✅ Slot switch confirmed: B → A
🎯 End-to-End RAUC Update Test COMPLETED!
```

**Error Indicators**:
```
✗ Connection error: timed out
✗ RAUC installation failed
✗ Slot switch verification failed
❌ SOME TESTS FAILED
```

## Troubleshooting

### Common Issues

**1. Connection Timeouts**:
```bash
# Increase timeout values
export RAUC_TEST_CONNECTION_TIMEOUT="30"
export RAUC_TEST_REBOOT_TIMEOUT="600"
./test_rauc_auto.sh
```

**2. SSH Key Issues**:
```bash
# Clear old keys and retry
ssh-keygen -f ~/.ssh/known_hosts -R 192.168.1.100
./test_rauc_auto.sh
```

**3. Bundle Not Found**:
```bash
# Verify bundle path
ls -la ../../image/nuc-image-qt5-bundle-intel-corei7-64.raucb

# Use custom bundle
./test_rauc_auto.sh /path/to/your/bundle.raucb
```

**4. Network Issues**:
```bash
# Run network setup
./test_batch.sh --network-setup

# Manual network check
ping 192.168.1.100
ssh root@192.168.1.100 "echo test"
```

### Debug Mode

**Enable Verbose Output**:
```bash
export RAUC_TEST_VERBOSE="true"
./test_rauc_auto.sh
```

**Check Individual Components**:
```bash
# Test connection only
uv run rauc-updater test --host 192.168.1.100 --verbose

# Check RAUC status
ssh root@192.168.1.100 "rauc status"

# Check system logs
ssh root@192.168.1.100 "journalctl -u rauc -n 20"
```

## Integration with CI/CD

### Example GitHub Actions Workflow

```yaml
name: RAUC Update Test
on: [push, pull_request]

jobs:
  rauc-test:
    runs-on: self-hosted
    steps:
      - uses: actions/checkout@v3
      
      - name: Setup Test Environment
        run: |
          cd tools/updater
          uv sync
      
      - name: Run RAUC Tests
        run: |
          cd tools/updater
          ./test_batch.sh -s basic
      
      - name: Upload Test Results
        uses: actions/upload-artifact@v3
        with:
          name: rauc-test-results
          path: tools/updater/logs/
```

### Jenkins Pipeline

```groovy
pipeline {
    agent any
    stages {
        stage('RAUC Test') {
            steps {
                dir('tools/updater') {
                    sh 'uv sync'
                    sh './test_batch.sh'
                }
            }
            post {
                always {
                    archiveArtifacts artifacts: 'tools/updater/logs/**/*', fingerprint: true
                    publishHTML([
                        allowMissing: false,
                        alwaysLinkToLastBuild: true,
                        keepAll: true,
                        reportDir: 'tools/updater/logs/results_*',
                        reportFiles: 'test_report.html',
                        reportName: 'RAUC Test Report'
                    ])
                }
            }
        }
    }
}
```

## Advanced Usage

### Custom Test Scenarios

Create custom test scenarios by extending the batch test script:

```bash
# Add to test_batch.sh
run_custom_test() {
    local device="$1"
    # Your custom test logic here
}

# Add to scenario list in test_config.sh
export RAUC_TEST_SCENARIOS=(
    "basic"
    "double"
    "custom"
)
```

### Performance Testing

Monitor update performance:

```bash
# Time critical operations
time ./test_rauc_auto.sh

# Monitor system resources during update
ssh root@192.168.1.100 "top -b -n1" &
./test_rauc_auto.sh
```

### Multi-Site Testing

Test across multiple networks:

```bash
# Configure different site networks
export RAUC_TEST_DEVICES=(
    "192.168.1.100"    # Site A
    "10.0.1.100"       # Site B  
    "172.16.1.100"     # Site C
)

./test_batch.sh
```

## Best Practices

1. **Test Regularly**: Run automated tests on every bundle build
2. **Document Results**: Keep test logs for debugging and analysis
3. **Monitor Timing**: Track update performance over time
4. **Verify Rollback**: Test recovery scenarios periodically
5. **Network Isolation**: Test in controlled network environments
6. **Version Control**: Track test configurations and scripts

This testing framework provides comprehensive coverage of RAUC update scenarios and can be easily integrated into development workflows for continuous validation.