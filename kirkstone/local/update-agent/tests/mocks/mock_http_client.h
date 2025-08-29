/**
 * @file mock_http_client.h
 * @brief HTTP 클라이언트 모킹 인터페이스
 *
 * ServerAgent의 HTTP 통신을 모킹하기 위한 인터페이스입니다.
 * 실제 네트워크 호출 없이 테스트할 수 있도록 합니다.
 */

#ifndef MOCK_HTTP_CLIENT_H
#define MOCK_HTTP_CLIENT_H

#include <gmock/gmock.h>
#include <string>

/**
 * @class MockHttpClient
 * @brief HTTP 클라이언트 모킹 클래스
 *
 * ServerAgent의 HTTP 통신을 모킹하여
 * 네트워크 의존성 없이 테스트할 수 있도록 합니다.
 */
class MockHttpClient {
public:
    virtual ~MockHttpClient() = default;

    /**
     * @brief HTTP GET 요청 모킹
     * @param url 요청할 URL
     * @param response 응답 데이터를 받을 문자열 참조
     * @return 요청 성공 여부
     */
    MOCK_METHOD(bool, get, (const std::string& url, std::string& response), ());

    /**
     * @brief HTTP POST 요청 모킹
     * @param url 요청할 URL
     * @param data 전송할 데이터
     * @param response 응답 데이터를 받을 문자열 참조
     * @return 요청 성공 여부
     */
    MOCK_METHOD(bool, post, (const std::string& url, const std::string& data, std::string& response), ());

    /**
     * @brief 파일 다운로드 모킹
     * @param url 다운로드할 URL
     * @param filepath 저장할 파일 경로
     * @return 다운로드 성공 여부
     */
    MOCK_METHOD(bool, downloadFile, (const std::string& url, const std::string& filepath), ());

    /**
     * @brief 연결 상태 확인 모킹
     * @return 연결 가능 여부
     */
    MOCK_METHOD(bool, isConnected, (), (const));
};

#endif // MOCK_HTTP_CLIENT_H
