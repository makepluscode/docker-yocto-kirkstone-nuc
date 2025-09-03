#!/bin/bash

# Updater Server 실행 스크립트
# uv를 사용하여 main.py의 CLI 모드를 실행합니다

set -e  # 오류 발생 시 스크립트 종료

echo "🚀 Updater Server 시작 중..."
echo "📁 작업 디렉토리: $(pwd)"
echo "🐍 Python 환경: uv"
echo ""

# uv가 설치되어 있는지 확인
if ! command -v uv &> /dev/null; then
    echo "❌ 오류: uv가 설치되어 있지 않습니다."
    echo "   설치 방법: curl -LsSf https://astral.sh/uv/install.sh | sh"
    exit 1
fi

# main.py 파일이 존재하는지 확인
if [ ! -f "main.py" ]; then
    echo "❌ 오류: main.py 파일을 찾을 수 없습니다."
    echo "   현재 디렉토리에서 실행해주세요: $(pwd)"
    exit 1
fi

# pyproject.toml 파일이 존재하는지 확인
if [ ! -f "pyproject.toml" ]; then
    echo "❌ 오류: pyproject.toml 파일을 찾을 수 없습니다."
    echo "   프로젝트 루트 디렉토리에서 실행해주세요."
    exit 1
fi

echo "✅ 환경 확인 완료"
echo "🔧 uv run main.py cli 실행 중..."
echo ""

# uv를 사용하여 main.py cli 실행
exec uv run main.py cli "$@"