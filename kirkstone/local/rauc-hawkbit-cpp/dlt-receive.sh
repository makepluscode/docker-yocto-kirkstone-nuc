#!/bin/bash

# DLT Logging Script for RAUC Hawkbit C++ Client
# This script helps debug the Hawkbit C++ updater by filtering and displaying relevant DLT logs

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

# Configuration
APP_NAME="RHCP"
HAWK_CONTEXT="HAWK"
RAUC_CONTEXT="RAUC"
UPDT_CONTEXT="UPDT"
LOG_LEVEL="INFO"

# Function to show usage
show_usage() {
    echo -e "${BOLD}DLT Logging Script for RAUC Hawkbit C++ Client${NC}"
    echo ""
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -a, --app NAME        Application name (default: RHCP)"
    echo "  -c, --context CONTEXT Context to filter (HAWK|RAUC|UPDT|ALL)"
    echo "  -l, --level LEVEL     Log level (ERROR|WARN|INFO|DEBUG|VERBOSE)"
    echo "  -f, --follow          Follow logs in real-time"
    echo "  -s, --save FILE       Save logs to file"
    echo "  -h, --help            Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 -f                    # Follow all logs in real-time"
    echo "  $0 -c HAWK -l DEBUG      # Show Hawkbit context logs with DEBUG level"
    echo "  $0 -c ALL -s hawkbit.log # Save all logs to file"
    echo "  $0 -c RAUC -f            # Follow RAUC context logs"
    echo ""
}

# Function to check if DLT is available
check_dlt() {
    if ! command -v dlt-receive &> /dev/null; then
        echo -e "${RED}Error: dlt-receive command not found${NC}"
        echo "Please install DLT (Diagnostic Log and Trace) tools:"
        echo "  sudo apt-get install dlt-daemon dlt-tools"
        exit 1
    fi
}

# Function to start DLT daemon if not running
start_dlt_daemon() {
    if ! systemctl is-active --quiet dlt-daemon; then
        echo -e "${YELLOW}Starting DLT daemon...${NC}"
        sudo systemctl start dlt-daemon
        sleep 2
    fi
}

# Function to build dlt-receive command
build_dlt_command() {
    local cmd="dlt-receive"
    
    # Add application filter
    cmd="$cmd -a $APP_NAME"
    
    # Add context filters
    if [[ "$CONTEXT" == "ALL" ]]; then
        cmd="$cmd -c $HAWK_CONTEXT -c $RAUC_CONTEXT -c $UPDT_CONTEXT"
    else
        cmd="$cmd -c $CONTEXT"
    fi
    
    # Add log level
    cmd="$cmd -l $LOG_LEVEL"
    
    # Add verbose flag for more details
    cmd="$cmd -v"
    
    # Add follow flag if requested
    if [[ "$FOLLOW" == "true" ]]; then
        cmd="$cmd -f"
    fi
    
    echo "$cmd"
}

# Function to filter and format logs
filter_logs() {
    local input_file="$1"
    local output_file="$2"
    
    if [[ -n "$output_file" ]]; then
        echo -e "${GREEN}Saving logs to: $output_file${NC}"
        # Save raw logs
        dlt-receive -a $APP_NAME -c $HAWK_CONTEXT -c $RAUC_CONTEXT -c $UPDT_CONTEXT -l $LOG_LEVEL -v -f > "$output_file" 2>&1 &
        DLT_PID=$!
        echo -e "${YELLOW}DLT logging started (PID: $DLT_PID)${NC}"
        echo -e "${CYAN}Press Ctrl+C to stop logging${NC}"
        
        # Wait for user to stop
        trap "kill $DLT_PID 2>/dev/null; echo -e '\n${GREEN}Logging stopped${NC}'; exit 0" INT
        wait $DLT_PID
    else
        # Display logs with color coding
        dlt-receive -a $APP_NAME -c $HAWK_CONTEXT -c $RAUC_CONTEXT -c $UPDT_CONTEXT -l $LOG_LEVEL -v -f | while IFS= read -r line; do
            # Color code based on context
            if [[ "$line" == *"$HAWK_CONTEXT"* ]]; then
                echo -e "${BLUE}$line${NC}"
            elif [[ "$line" == *"$RAUC_CONTEXT"* ]]; then
                echo -e "${GREEN}$line${NC}"
            elif [[ "$line" == *"$UPDT_CONTEXT"* ]]; then
                echo -e "${CYAN}$line${NC}"
            elif [[ "$line" == *"ERROR"* ]]; then
                echo -e "${RED}$line${NC}"
            elif [[ "$line" == *"WARN"* ]]; then
                echo -e "${YELLOW}$line${NC}"
            else
                echo "$line"
            fi
        done
    fi
}

# Function to show system status
show_status() {
    echo -e "${BOLD}=== RAUC Hawkbit C++ Client Status ===${NC}"
    echo ""
    
    # Check if service is running
    if systemctl is-active --quiet rauc-hawkbit-cpp; then
        echo -e "${GREEN}✓ rauc-hawkbit-cpp service is running${NC}"
    else
        echo -e "${RED}✗ rauc-hawkbit-cpp service is not running${NC}"
    fi
    
    # Check if RAUC service is running
    if systemctl is-active --quiet rauc; then
        echo -e "${GREEN}✓ RAUC service is running${NC}"
    else
        echo -e "${RED}✗ RAUC service is not running${NC}"
    fi
    
    # Check if DLT daemon is running
    if systemctl is-active --quiet dlt-daemon; then
        echo -e "${GREEN}✓ DLT daemon is running${NC}"
    else
        echo -e "${RED}✗ DLT daemon is not running${NC}"
    fi
    
    echo ""
}

# Function to show recent logs
show_recent_logs() {
    echo -e "${BOLD}=== Recent DLT Logs ===${NC}"
    echo ""
    
    # Get recent logs from journalctl
    journalctl -u rauc-hawkbit-cpp --since "5 minutes ago" -n 20 --no-pager | while IFS= read -r line; do
        if [[ "$line" == *"ERROR"* ]]; then
            echo -e "${RED}$line${NC}"
        elif [[ "$line" == *"WARN"* ]]; then
            echo -e "${YELLOW}$line${NC}"
        else
            echo "$line"
        fi
    done
    
    echo ""
}

# Parse command line arguments
CONTEXT="ALL"
FOLLOW="false"
SAVE_FILE=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -a|--app)
            APP_NAME="$2"
            shift 2
            ;;
        -c|--context)
            CONTEXT="$2"
            shift 2
            ;;
        -l|--level)
            LOG_LEVEL="$2"
            shift 2
            ;;
        -f|--follow)
            FOLLOW="true"
            shift
            ;;
        -s|--save)
            SAVE_FILE="$2"
            shift 2
            ;;
        -h|--help)
            show_usage
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            show_usage
            exit 1
            ;;
    esac
done

# Validate context
if [[ "$CONTEXT" != "ALL" && "$CONTEXT" != "$HAWK_CONTEXT" && "$CONTEXT" != "$RAUC_CONTEXT" && "$CONTEXT" != "$UPDT_CONTEXT" ]]; then
    echo -e "${RED}Invalid context: $CONTEXT${NC}"
    echo "Valid contexts: ALL, $HAWK_CONTEXT, $RAUC_CONTEXT, $UPDT_CONTEXT"
    exit 1
fi

# Validate log level
if [[ "$LOG_LEVEL" != "ERROR" && "$LOG_LEVEL" != "WARN" && "$LOG_LEVEL" != "INFO" && "$LOG_LEVEL" != "DEBUG" && "$LOG_LEVEL" != "VERBOSE" ]]; then
    echo -e "${RED}Invalid log level: $LOG_LEVEL${NC}"
    echo "Valid levels: ERROR, WARN, INFO, DEBUG, VERBOSE"
    exit 1
fi

# Check DLT availability
check_dlt

# Start DLT daemon if needed
start_dlt_daemon

# Show status
show_status

# Show recent logs if not following
if [[ "$FOLLOW" != "true" ]]; then
    show_recent_logs
fi

# Build and execute DLT command
echo -e "${BOLD}Starting DLT logging...${NC}"
echo -e "${CYAN}Application: $APP_NAME${NC}"
echo -e "${CYAN}Context: $CONTEXT${NC}"
echo -e "${CYAN}Log Level: $LOG_LEVEL${NC}"
echo -e "${CYAN}Follow: $FOLLOW${NC}"
if [[ -n "$SAVE_FILE" ]]; then
    echo -e "${CYAN}Save to: $SAVE_FILE${NC}"
fi
echo ""

# Execute DLT command
if [[ -n "$SAVE_FILE" ]]; then
    filter_logs "" "$SAVE_FILE"
else
    filter_logs
fi 