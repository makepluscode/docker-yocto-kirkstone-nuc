#!/bin/bash

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Default paths
BUILD_DIR="./kirkstone/build"
DEPLOY_DIR="$BUILD_DIR/tmp-glibc/deploy"
IMAGES_DIR="$DEPLOY_DIR/images/intel-corei7-64"
PKGDATA_DIR="$BUILD_DIR/tmp-glibc/pkgdata"

print_header() {
    echo -e "${BLUE}╔══════════════════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║${CYAN} $1 ${BLUE}║${NC}"
    echo -e "${BLUE}╚══════════════════════════════════════════════════════════════════════════════╝${NC}"
}

print_subheader() {
    echo -e "${PURPLE}▶ $1${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_info() {
    echo -e "${CYAN}ℹ️  $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

check_files() {
    if [ ! -d "$IMAGES_DIR" ]; then
        print_error "Images directory not found: $IMAGES_DIR"
        exit 1
    fi
}

# Function to analyze manifest files (패키지 목록)
analyze_manifest() {
    print_header "📋 MANIFEST FILE ANALYSIS (Yocto 자동 생성)"
    
    manifest_files=$(find "$IMAGES_DIR" -name "*.manifest" 2>/dev/null | head -5)
    
    if [ -z "$manifest_files" ]; then
        print_error "No manifest files found"
        return
    fi
    
    echo "$manifest_files" | while read manifest; do
        if [ -n "$manifest" ]; then
            image_name=$(basename "$manifest" .manifest)
            package_count=$(wc -l < "$manifest")
            
            print_subheader "이미지: $image_name ($package_count packages)"
            
            echo
            echo "🔸 패키지 카테고리별 통계:"
            awk '{print $1}' "$manifest" | cut -d'-' -f1 | sort | uniq -c | sort -nr | head -15 | \
            while read count category; do
                printf "   %-20s: %3d packages\n" "$category" "$count"
            done
            
            echo
            echo "🔸 아키텍처별 분포:"
            awk '{print $2}' "$manifest" | sort | uniq -c | sort -nr | \
            while read count arch; do
                printf "   %-20s: %3d packages\n" "$arch" "$count"
            done
            
            echo
            echo "🔸 상위 20개 패키지 (알파벳 순):"
            head -20 "$manifest" | while read pkg arch version; do
                printf "   %-35s %-15s %s\n" "$pkg" "$arch" "$version"
            done
            
            echo
            echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        fi
    done
}

# Function to analyze testdata.json files (상세 빌드 정보)
analyze_testdata() {
    print_header "📊 TESTDATA JSON ANALYSIS (Yocto 자동 생성)"
    
    testdata_files=$(find "$IMAGES_DIR" -name "*.testdata.json" 2>/dev/null | head -3)
    
    if [ -z "$testdata_files" ]; then
        print_error "No testdata.json files found"
        return
    fi
    
    echo "$testdata_files" | while read testdata; do
        if [ -n "$testdata" ]; then
            image_name=$(basename "$testdata" .testdata.json)
            
            print_subheader "이미지: $image_name"
            
            # 빌드 설정 정보
            echo "🔸 빌드 설정:"
            jq -r '
                "   Machine: " + (.MACHINE // "N/A") + 
                "\n   Distro: " + (.DISTRO // "N/A") + 
                "\n   Tune: " + (.DEFAULTTUNE // "N/A") + 
                "\n   Package Classes: " + (.PACKAGE_CLASSES // "N/A")
            ' "$testdata" 2>/dev/null || echo "   JSON parsing failed"
            
            # 이미지 특성
            echo
            echo "🔸 이미지 특성:"
            jq -r '
                "   Image Install: " + (.IMAGE_INSTALL // "N/A") + 
                "\n   Extra Features: " + (.EXTRA_IMAGE_FEATURES // "N/A")
            ' "$testdata" 2>/dev/null || echo "   JSON parsing failed"
            
            echo
            echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        fi
    done
}

# Function to analyze pkgdata (패키지 상세 정보)
analyze_pkgdata() {
    print_header "📦 PKGDATA ANALYSIS (Yocto 자동 생성)"
    
    if [ ! -d "$PKGDATA_DIR" ]; then
        print_error "Pkgdata directory not found: $PKGDATA_DIR"
        return
    fi
    
    print_subheader "패키지 데이터 통계"
    
    # 전체 패키지 수
    total_pkgs=$(find "$PKGDATA_DIR" -name "*.pkgdata" | wc -l)
    print_info "총 pkgdata 파일: $total_pkgs"
    
    echo
    echo "🔸 패키지 크기 순위 (상위 20개):"
    
    # 패키지 크기 정보 추출
    find "$PKGDATA_DIR" -name "*.pkgdata" | while read pkgfile; do
        pkg_name=$(basename "$pkgfile" .pkgdata)
        
        # PKGSIZE 또는 FILES_INFO에서 크기 정보 추출
        size=$(grep "^PKGSIZE:" "$pkgfile" 2>/dev/null | cut -d: -f2 | tr -d ' ')
        
        if [ -n "$size" ] && [ "$size" != "" ]; then
            echo "$pkg_name $size"
        fi
    done | sort -k2 -nr | head -20 | while read name size; do
        if [ -n "$size" ] && [ "$size" -gt 0 ] 2>/dev/null; then
            size_kb=$(echo "scale=1; $size/1024" | bc 2>/dev/null || echo "N/A")
            size_mb=$(echo "scale=2; $size/1024/1024" | bc 2>/dev/null || echo "N/A")
            printf "   %-40s %10s bytes (%s KB, %s MB)\n" "$name" "$size" "$size_kb" "$size_mb"
        fi
    done
    
    echo
    echo "🔸 런타임 의존성이 많은 패키지 (상위 10개):"
    find "$PKGDATA_DIR" -name "*.pkgdata" | while read pkgfile; do
        pkg_name=$(basename "$pkgfile" .pkgdata)
        rdepends_count=$(grep "^RDEPENDS:" "$pkgfile" 2>/dev/null | wc -w)
        if [ "$rdepends_count" -gt 1 ]; then
            echo "$pkg_name $rdepends_count"
        fi
    done | sort -k2 -nr | head -10 | while read name count; do
        printf "   %-40s %3d dependencies\n" "$name" "$count"
    done
}

# Function to analyze deployed packages (배포된 패키지)
analyze_deployed_packages() {
    print_header "📦 DEPLOYED PACKAGES ANALYSIS (Yocto 자동 생성)"
    
    for pkg_format in ipk rpm; do
        pkg_dir="$DEPLOY_DIR/$pkg_format"
        
        if [ -d "$pkg_dir" ]; then
            print_subheader "${pkg_format^^} 패키지"
            
            total_packages=$(find "$pkg_dir" -name "*.$pkg_format" | wc -l)
            total_size=$(find "$pkg_dir" -name "*.$pkg_format" -exec du -b {} \; 2>/dev/null | awk '{sum+=$1} END {print sum}')
            total_size_mb=$(echo "scale=2; $total_size/1024/1024" | bc 2>/dev/null || echo "N/A")
            
            print_info "총 $total_packages 개 패키지, $total_size_mb MB"
            
            echo
            echo "🔸 크기별 상위 15개 패키지:"
            find "$pkg_dir" -name "*.$pkg_format" -exec du -b {} \; 2>/dev/null | sort -nr | head -15 | \
            while read size filepath; do
                pkg_name=$(basename "$filepath")
                size_kb=$(echo "scale=1; $size/1024" | bc 2>/dev/null || echo "N/A")
                printf "   %-50s %8s bytes (%s KB)\n" "$pkg_name" "$size" "$size_kb"
            done
            
            echo
        fi
    done
}

# Function to analyze image files (이미지 파일)
analyze_image_files() {
    print_header "💿 IMAGE FILES ANALYSIS (Yocto 자동 생성)"
    
    print_subheader "생성된 이미지 파일들"
    
    find "$IMAGES_DIR" -type f \( -name "*.wic" -o -name "*.ext4" -o -name "*.tar.gz" \) | \
    while read imgfile; do
        img_name=$(basename "$imgfile")
        img_size=$(du -h "$imgfile" 2>/dev/null | cut -f1)
        img_size_bytes=$(du -b "$imgfile" 2>/dev/null | cut -f1)
        
        printf "   %-60s %8s (%s bytes)\n" "$img_name" "$img_size" "$img_size_bytes"
    done
    
    echo
    print_subheader "환경 설정 파일들"
    
    find "$IMAGES_DIR" -name "*.env" | while read envfile; do
        env_name=$(basename "$envfile")
        echo "   📄 $env_name"
        head -5 "$envfile" | sed 's/^/      /'
        echo
    done
}

# Function to generate storage usage report
analyze_storage_usage() {
    print_header "💾 STORAGE USAGE ANALYSIS"
    
    print_subheader "디렉토리별 저장공간 사용량"
    
    echo "   빌드 전체:"
    du -sh "$BUILD_DIR" 2>/dev/null | awk '{printf "      Total: %s\n", $1}'
    
    echo
    echo "   세부 디렉토리:"
    for dir in "tmp-glibc/deploy" "tmp-glibc/work" "sstate-cache" "downloads" "cache"; do
        if [ -d "$BUILD_DIR/$dir" ]; then
            size=$(du -sh "$BUILD_DIR/$dir" 2>/dev/null | cut -f1)
            printf "      %-20s: %s\n" "$dir" "$size"
        fi
    done
    
    echo
    print_subheader "이미지 디렉토리 상세"
    du -sh "$IMAGES_DIR"/* 2>/dev/null | head -10 | while read size file; do
        filename=$(basename "$file")
        printf "   %-50s: %s\n" "$filename" "$size"
    done
}

# Function to generate individual image reports
generate_image_reports() {
    print_header "📄 INDIVIDUAL IMAGE REPORTS GENERATION"
    
    # Create report directory if it doesn't exist
    mkdir -p report
    
    timestamp=$(date +"%Y%m%d-%H%M")
    
    manifest_files=$(find "$IMAGES_DIR" -name "*.manifest" 2>/dev/null)
    
    if [ -z "$manifest_files" ]; then
        print_error "No manifest files found"
        return
    fi
    
    # Get unique clean names to avoid duplicate reports
    unique_names=$(echo "$manifest_files" | while read manifest; do
        if [ -n "$manifest" ]; then
            image_name=$(basename "$manifest" .manifest)
            clean_name=$(echo "$image_name" | sed 's/-[0-9]\{8\}-[0-9]\{6\}.*$//' | sed 's/-intel-corei7-64.*$//' | sed 's/\.rootfs$//')
            echo "$clean_name"
        fi
    done | sort -u)
    
    echo "$unique_names" | while read clean_name; do
        if [ -n "$clean_name" ]; then
            # Find the most recent manifest file for this image
            manifest=$(find "$IMAGES_DIR" -name "*${clean_name}*.manifest" | head -1)
            image_name=$(basename "$manifest" .manifest)
            
            report_file="report/yocto-report-${clean_name}_${timestamp}.md"
            
            print_info "이미지 '$clean_name' 리포트 생성 중..."
            
            {
                echo "# Yocto Build Report: $clean_name"
                echo
                echo "**Generated:** $(date)"
                echo "**Image:** $image_name"
                echo
                
                # Package count
                package_count=$(wc -l < "$manifest")
                echo "## 📋 Package Summary"
                echo
                echo "- **Total Packages:** $package_count"
                
                # Find corresponding testdata.json file
                testdata_file=$(find "$IMAGES_DIR" -name "*${clean_name}*.testdata.json" | head -1)
                if [ -n "$testdata_file" ] && [ -f "$testdata_file" ]; then
                    machine=$(grep '"MACHINE"' "$testdata_file" | sed 's/.*"MACHINE": *"\([^"]*\)".*/\1/' | head -1)
                    distro=$(grep '"DISTRO"' "$testdata_file" | sed 's/.*"DISTRO": *"\([^"]*\)".*/\1/' | head -1)
                    tune=$(grep '"DEFAULTTUNE"' "$testdata_file" | sed 's/.*"DEFAULTTUNE": *"\([^"]*\)".*/\1/' | head -1)
                    echo "- **Machine:** ${machine:-N/A}"
                    echo "- **Distro:** ${distro:-N/A}"
                    echo "- **Tune:** ${tune:-N/A}"
                fi
                echo
                
                # Architecture distribution
                echo "## 🏗️ Architecture Distribution"
                echo
                echo "| Architecture | Package Count |"
                echo "|--------------|---------------|"
                awk '{print $2}' "$manifest" | sort | uniq -c | sort -nr | \
                while read count arch; do
                    printf "| %-20s | %3d |\n" "$arch" "$count"
                done
                echo
                
                # Category statistics
                echo "## 📊 Package Categories (Top 20)"
                echo
                echo "| Category | Package Count |"
                echo "|----------|---------------|"
                awk '{print $1}' "$manifest" | cut -d'-' -f1 | sort | uniq -c | sort -nr | head -20 | \
                while read count category; do
                    printf "| %-20s | %3d |\n" "$category" "$count"
                done
                echo
                
                # Package list
                echo "## 📦 Complete Package List"
                echo
                echo "| Package Name | Architecture | Version |"
                echo "|--------------|--------------|---------|"
                while read pkg arch version; do
                    printf "| %-30s | %-15s | %-20s |\n" "$pkg" "$arch" "$version"
                done < "$manifest"
                echo
                
                # Image files
                echo "## 💿 Generated Image Files"
                echo
                echo "| File Name | Size |"
                echo "|-----------|------|"
                find "$IMAGES_DIR" -name "*${clean_name}*" -type f \( -name "*.wic" -o -name "*.ext4" -o -name "*.tar.gz" \) | \
                while read imgfile; do
                    img_name=$(basename "$imgfile")
                    img_size=$(du -h "$imgfile" 2>/dev/null | cut -f1)
                    printf "| %-50s | %8s |\n" "$img_name" "$img_size"
                done
                echo
                
                # Package sizes if available
                if [ -d "$PKGDATA_DIR" ]; then
                    echo "## 📏 Package Sizes (Top 20)"
                    echo
                    echo "| Package Name | Size (bytes) | Size (KB) | Size (MB) |"
                    echo "|--------------|--------------|-----------|-----------|"
                    
                    # Get packages from manifest
                    awk '{print $1}' "$manifest" | while read pkg_name; do
                        # Find pkgdata file
                        pkgdata_file=$(find "$PKGDATA_DIR" -name "${pkg_name}.pkgdata" 2>/dev/null | head -1)
                        if [ -n "$pkgdata_file" ] && [ -f "$pkgdata_file" ]; then
                            size=$(grep "^PKGSIZE:" "$pkgdata_file" 2>/dev/null | cut -d: -f2 | tr -d ' ')
                            if [ -n "$size" ] && [ "$size" != "" ] && [ "$size" -gt 0 ] 2>/dev/null; then
                                size_kb=$(echo "scale=1; $size/1024" | bc 2>/dev/null || echo "N/A")
                                size_mb=$(echo "scale=2; $size/1024/1024" | bc 2>/dev/null || echo "N/A")
                                echo "$pkg_name $size $size_kb $size_mb"
                            fi
                        fi
                    done | sort -k2 -nr | head -20 | while read name size size_kb size_mb; do
                        printf "| %-30s | %10s | %8s | %8s |\n" "$name" "$size" "$size_kb" "$size_mb"
                    done
                fi
                echo
                
                # Build information
                echo "## 🔧 Build Information"
                echo
                echo "- **Build Directory:** $BUILD_DIR"
                echo "- **Images Directory:** $IMAGES_DIR"
                echo "- **Report Generated:** $(date)"
                
                if [ -n "$testdata_file" ] && [ -f "$testdata_file" ]; then
                    image_install=$(grep '"IMAGE_INSTALL"' "$testdata_file" | sed 's/.*"IMAGE_INSTALL": *"\([^"]*\)".*/\1/' | head -1)
                    extra_features=$(grep '"EXTRA_IMAGE_FEATURES"' "$testdata_file" | sed 's/.*"EXTRA_IMAGE_FEATURES": *"\([^"]*\)".*/\1/' | head -1)
                    echo "- **Image Install:** ${image_install:-N/A}"
                    echo "- **Extra Features:** ${extra_features:-N/A}"
                fi
                
            } > "$report_file"
            
            print_success "리포트 생성 완료: $report_file"
        fi
    done
}

# Function to generate comprehensive report
generate_comprehensive_report() {
    timestamp=$(date +"%Y%m%d_%H%M%S")
    report_file="yocto_native_analysis_$timestamp.txt"
    
    print_header "📄 COMPREHENSIVE REPORT GENERATION"
    
    {
        echo "════════════════════════════════════════════════════════════════════════════════"
        echo "                    YOCTO NATIVE FILE ANALYSIS REPORT"
        echo "                         $(date)"
        echo "════════════════════════════════════════════════════════════════════════════════"
        echo
        
        # 빌드 환경 정보
        echo "BUILD ENVIRONMENT:"
        echo "=================="
        echo "Build Directory: $BUILD_DIR"
        echo "Images Directory: $IMAGES_DIR"
        echo "Analysis Timestamp: $(date)"
        echo
        
        # manifest 파일 분석 결과
        echo "MANIFEST FILES (Image Package Lists):"
        echo "====================================="
        find "$IMAGES_DIR" -name "*.manifest" | while read manifest; do
            image_name=$(basename "$manifest" .manifest)
            package_count=$(wc -l < "$manifest")
            echo "Image: $image_name ($package_count packages)"
            
            echo "Top categories:"
            awk '{print $1}' "$manifest" | cut -d'-' -f1 | sort | uniq -c | sort -nr | head -10 | \
            while read count category; do
                printf "  %-20s: %3d packages\n" "$category" "$count"
            done
            echo
        done
        
        # 저장공간 사용량
        echo "STORAGE USAGE:"
        echo "============="
        du -sh "$BUILD_DIR" 2>/dev/null | awk '{print "Total build directory: " $1}'
        for dir in "tmp-glibc/deploy" "tmp-glibc/work" "sstate-cache"; do
            if [ -d "$BUILD_DIR/$dir" ]; then
                du -sh "$BUILD_DIR/$dir" 2>/dev/null | awk -v d="$dir" '{print d ": " $1}'
            fi
        done
        
    } > "$report_file"
    
    print_success "상세 리포트 저장됨: $report_file"
}

# Main menu
print_header "🚀 YOCTO NATIVE FILE ANALYZER"
print_info "Yocto 빌드 과정에서 자동 생성된 파일들을 활용한 분석 도구"

check_files

echo
echo "분석 옵션을 선택하세요:"
echo "  1. 📋 Manifest 파일 분석 (패키지 목록)"
echo "  2. 📊 TestData JSON 분석 (빌드 정보)"
echo "  3. 📦 PkgData 분석 (패키지 상세 정보)"
echo "  4. 📦 배포 패키지 분석 (IPK/RPM)"
echo "  5. 💿 이미지 파일 분석"
echo "  6. 💾 저장공간 사용량 분석"
echo "  7. 📄 종합 리포트 생성 (TXT)"
echo "  8. 📝 개별 이미지 리포트 생성 (MD)"
echo "  9. 🔄 전체 분석 실행"

read -p "선택 (1-9, 기본값: 1): " CHOICE

case "${CHOICE:-1}" in
    1) analyze_manifest ;;
    2) analyze_testdata ;;
    3) analyze_pkgdata ;;
    4) analyze_deployed_packages ;;
    5) analyze_image_files ;;
    6) analyze_storage_usage ;;
    7) generate_comprehensive_report ;;
    8) generate_image_reports ;;
    9) 
        analyze_manifest
        echo
        analyze_testdata  
        echo
        analyze_pkgdata
        echo
        analyze_deployed_packages
        echo
        analyze_image_files
        echo
        analyze_storage_usage
        ;;
    *) 
        print_warning "잘못된 선택입니다. Manifest 분석을 실행합니다."
        analyze_manifest
        ;;
esac

echo
print_success "분석 완료! 🎉" 