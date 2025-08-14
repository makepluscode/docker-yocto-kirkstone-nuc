#!/usr/bin/env bash
# dlt-receive.sh – Dashboard DLT log receiver
# Usage:
#   ./dlt-receive.sh [NUC_IP] [PORT] [EXTRA_DLT_OPTIONS]
# Example:
#   ./dlt-receive.sh                              # use default IP 192.168.1.100, port 3490
#   ./dlt-receive.sh 192.168.1.100               # custom IP, default port 3490
#   ./dlt-receive.sh 192.168.1.100 3491 -e DBO   # custom IP + port + filter for dashboard app
#   ./dlt-receive.sh 192.168.1.100 3490 -e RUC   # filter for RAUC manager logs
#
set -e

if [[ "$1" == "-h" || "$1" == "--help" ]]; then
  sed -n '2,14p' "$0"
  exit 0
fi

# Default values
DEFAULT_IP="192.168.1.100"
DEFAULT_PORT="3490"

# Color definitions using tput (more compatible)
if [[ -t 1 ]] && tput colors >/dev/null 2>&1; then
  RED=$(tput setaf 1 2>/dev/null || echo "")
  GREEN=$(tput setaf 2 2>/dev/null || echo "")
  YELLOW=$(tput setaf 3 2>/dev/null || echo "")
  BLUE=$(tput setaf 4 2>/dev/null || echo "")
  CYAN=$(tput setaf 6 2>/dev/null || echo "")
  BOLD=$(tput bold 2>/dev/null || echo "")
  NC=$(tput sgr0 2>/dev/null || echo "")
else
  RED="" GREEN="" YELLOW="" BLUE="" CYAN="" BOLD="" NC=""
fi

# Parse arguments
NUC_IP=""
PORT=""
DLT_ARGS=()

# Check if first argument looks like an IP address
if [[ "$1" =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]; then
  NUC_IP="$1"
  shift
  # Check if second argument is a port number
  if [[ "$1" =~ ^[0-9]+$ ]]; then
    PORT="$1"
    shift
  fi
fi

# Use defaults if not set
NUC_IP="${NUC_IP:-$DEFAULT_IP}"
PORT="${PORT:-$DEFAULT_PORT}"

# Remaining arguments are DLT options
DLT_ARGS=("$@")

# Verify dlt-receive exists
if ! command -v dlt-receive >/dev/null 2>&1; then
  echo "❌ dlt-receive not found. Install it (e.g., sudo apt install dlt-daemon)" >&2
  exit 2
fi

echo "📡 ${CYAN}대시보드 DLT 로그를 위해 ${BOLD}${YELLOW}$NUC_IP:$PORT${NC}${CYAN}에 연결 중...${NC}"
echo "💡 ${GREEN}대시보드 앱 ID: ${BOLD}${YELLOW}DBO${NC} (대시보드 애플리케이션)"
echo "💡 ${GREEN}RAUC 매니저 컨텍스트: ${BOLD}${YELLOW}RUC${NC}"
echo "💡 ${GREEN}UI 플로우 컨텍스트: ${BOLD}${YELLOW}UIF${NC}"
echo "💡 ${BLUE}대시보드 로그만 필터링하려면 ${BOLD}${YELLOW}-e DBO${NC}${BLUE} 사용${NC}"
echo ""

# Use ASCII payload output (-a) by default. dlt-receive requires the host argument LAST.
echo "실행할 명령: dlt-receive -a -p \"$PORT\" ${DLT_ARGS[*]} \"$NUC_IP\""
exec dlt-receive -a -p "$PORT" "${DLT_ARGS[@]}" "$NUC_IP"