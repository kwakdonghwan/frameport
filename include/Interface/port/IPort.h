// 본 소스코드는 BSD 3-Clause 라이선스를 따릅니다.
// This file is licensed under the BSD 3-Clause License.
// 개인프로젝트 코드이며 수정, 배포, 상업적이용은 자유로우나 상단 주석을 제거시
// 저작권 보호법 위반입니다.
/*
 * Copyright (c) 2025, 곽동환 <arbiter1225@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the author nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef NEXUM_COM_EXTERNAL_PORT_IPORT_H
#define NEXUM_COM_EXTERNAL_PORT_IPORT_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../method/IMethod.h"

class IFrame;

/**
 * @brief 외부 접점(Port)와 FrameBus 연동을 위한 추상 인터페이스
 *
 * IPort는 다양한 외부 시스템(입출력, 센서, 통신 등)과 내부 프레임
 * 구조(IFrame)를 연결하고 데이터 송수신, 콜백 구독을 통합 관리하기 위한 추상화
 * 레이어입니다. 모든 Port 구현체는 본 인터페이스를 상속받아야 하며, 멀티 프레임
 * 연결 및 다양한 데이터 타입 전달/구독이 가능합니다.
 */
class IPort : public IMethod {
 public:
  /** @brief 가상 소멸자 */
  virtual ~IPort() = default;

  /**
   * @brief 포트 이름 반환
   * @return std::string 포트의 식별자
   */
  virtual std::string name() const = 0;

  /**
   * @brief 포트 타입 반환 (예: CAN, LIN, TCP 등)
   * @return std::string 포트 타입 문자열
   */
  virtual std::string type() const = 0;

  /**
   * @brief 포트 오픈(연결) 시도
   * @return 성공 여부
   */
  virtual bool open() = 0;

  /**
   * @brief 포트 닫기/해제
   */
  virtual void close() = 0;

  /**
   * @brief FrameBus 내 프레임 연결
   * @param frameName FrameBus에 등록된 프레임명
   * @return 성공 여부
   */
  virtual bool connectFrame(const std::string& frameName) = 0;

  /**
   * @brief 프레임 연결 해제
   * @param frameName FrameBus에 등록된 프레임명
   */
  virtual void disconnectFrame(const std::string& frameName) = 0;

  /**
   * @brief 프레임의 개별 시그널에 값 전달 (Publish 없음)
   * @param frameName 프레임명
   * @param signal 시그널명
   * @param value 전달 값
   * @return 성공 여부
   */
  virtual bool setSignalToFrame(const std::string& frameName,
                                const std::string& signal,
                                const std::any& value) = 0;

  /**
   * @brief 프레임의 시그널에 값 전달 및 바로 Publish (전파)
   * @param frameName 프레임명
   * @param signal 시그널명
   * @param value 전달 값
   * @return 성공 여부
   */
  virtual bool setSignalToFrameWithPublish(const std::string& frameName,
                                           const std::string& signal,
                                           const std::any& value) = 0;

  /**
   * @brief 프레임에 Raw 데이터 전달 (Publish 없음)
   * @param frameName 프레임명
   * @param data 데이터 포인터
   * @param size 데이터 길이 (바이트)
   * @return 성공 여부
   */
  virtual bool setRawDataToFrame(const std::string& frameName, const char* data,
                                 size_t size) = 0;

  /**
   * @brief 프레임에 Raw 데이터 전달 및 바로 Publish
   * @param frameName 프레임명
   * @param data 데이터 포인터
   * @param size 데이터 길이 (바이트)
   * @return 성공 여부
   */
  virtual bool setRawDataToFrameWithPublish(const std::string& frameName,
                                            const char* data, size_t size) = 0;

  /**
   * @brief 프레임의 시그널 값을 std::any로 조회
   * @param frameName 프레임명
   * @param signal 시그널명
   * @return std::any 조회된 값
   */
  virtual std::any getSignalFromFrameAsAny(const std::string& frameName,
                                           const std::string& signal) = 0;

  /**
   * @brief 프레임의 시그널 값을 타입 안전하게 조회
   * @tparam T 반환 타입
   * @param frameName 프레임명
   * @param signal 시그널명
   * @return T 변환된 값
   */
  template <typename T>
  T getSignalFromFrame(const std::string& frameName,
                       const std::string& signal) {
    return std::any_cast<T>(getSignalFromFrameAsAny(frameName, signal));
  }

  /**
   * @brief 프레임의 Raw 데이터 블록을 콜백을 통해 읽기
   * @param frameName 프레임명
   * @param cb (data, size) 형태의 콜백 함수
   * @return 성공 여부
   */
  virtual bool getRawDataFromFrame(
      const std::string& frameName,
      std::function<void(const char*, size_t)> cb) = 0;

  /**
   * @brief 프레임 데이터 콜백 구독 (스레드 분리, 비동기 방식)
   * @param frameName 프레임명
   * @param cb 데이터 수신 시 호출될 콜백
   * @return uint64_t 콜백 인스턴스 ID
   */
  virtual uint64_t subscribeFrame(
      const std::string& frameName,
      std::function<void(const char*, size_t)> cb) = 0;

  /**
   * @brief 프레임 데이터 콜백 구독 (Direct: 호출 스레드에서 직접 호출)
   * @param frameName 프레임명
   * @param cb 데이터 수신 시 호출될 콜백
   * @return uint64_t 콜백 인스턴스 ID
   */
  virtual uint64_t subscribeFrameDirect(
      const std::string& frameName,
      std::function<void(const char*, size_t)> cb) = 0;

  /**
   * @brief 프레임 콜백 구독 해제
   * @param callbackId 구독 시 반환받은 인스턴스 ID
   */
  virtual void unsubscribeFrame(uint64_t callbackId) = 0;
};

#endif